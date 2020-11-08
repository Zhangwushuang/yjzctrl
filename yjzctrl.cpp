// yjzctrl.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <winsock2.h>
#include <Windows.h>
#include <iostream>
#include <thread>
#include <shared_mutex>
#include <forward_list>
#include <list>
#include <vector>
#include <atomic>
#include <time.h>
#include "config.h"
#include "mysql.h"
#include "mysqlctrl.h"
#include "mac_que.h"
#include "mac_map.h"
#include "cmd.h"
#include "http_mngr.h"
#include <cstdlib>
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "libmysql.lib") 

using namespace std;

/*两个完成端口*/
HANDLE IOCP_MAC = nullptr; //连接机器的完成端口
HANDLE IOCP_CTRL = nullptr; //连接PHP程序的完成端口

//刚接入时使用的设备对象池及等待队列
mac_que* const mac_pool(new mac_que()); //设备池
mac_que* const wait_que(new mac_que()); //等待队列

/*哈希表，用于保存连接进来的机器的信息,由于是全静态成员，无需实例化，可查看mac_map.h。*/


/******
向设备发送命令（WSASEND）时的_PER_IO_DATE池。

设备侧是每个设备对象都有一个专有的_PER_IO_DATE，用于接收（WSArecv）。
网站侧的各个套接字每次只投递一个IO请求，所以各有一个专有的_PER_IO_DATE。
*************/

bool mac_sevice()//机器侧的工作线程(只负责recv和发送心跳包,直接异步发送,不经过完成端口)
{

	cout << "macworker ready,IOCP handle is " << hex << (unsigned long long)IOCP_MAC << endl;

	mac* pmacdata = nullptr; //完成端口接收到的单句柄数据缓存
	_PER_IO* piodata = nullptr;//完成端口接收到的单IO数据缓存
	DWORD dwt = 0; //完成端口传送的数据长度
	bool mac_droped = false;//设备是否掉线或者报告出错

	char cmd_buffer[58];//命令的缓存
	http_node* http_tp = nullptr;

	while (true)
	{

		pmacdata = nullptr;
		piodata = nullptr;

		//等待时间无限,机器掉线等全部不加理会,交给专门的线程处理(此线程只处理成功接收到设备发来的报告)
		//因为以上两种情况必定超时。回收线程会回收掉线的设备对象，单Io数据保存在设备对象里，所以不会泄露
		if (GetQueuedCompletionStatus(IOCP_MAC, &dwt, (PULONG_PTR)&pmacdata, (LPOVERLAPPED*)&piodata, WSA_INFINITE)) { //如果完成端口返回
		
			if (pmacdata == nullptr || piodata == nullptr || piodata != pmacdata->Recv_IO || dwt != REPORT_LENTH)
			{

		//		cout << "MAC_IOCP UNKNOW ERROR 1 !" << endl;
				continue;

			}

			pmacdata->locker.lock();//锁定机器

			pmacdata->exchange_buffer();
			pmacdata->rpt->get_state();//解析收到的数据

			if (!pmacdata->chk_report()) {//判断设备序列号和口令(如果不一致)

				pmacdata->locker.unlock();//解锁机器
				continue;

			}

			pmacdata->updatatime = GetTickCount64(); //记录获取数据的时间

			//是否发送上线报告
			if (pmacdata->first_rpt)
			{
			//	cout << "online_flag " << endl;
				http_tp = http_mngr::pool->get();//从池子里提取一个缓存

				if (nullptr != http_tp) {//若提取成功

					pmacdata->rpt->Creat_Alert_Http(http_tp->buffer, http_tp->real_lenth, "2");//组合http字串
					http_mngrs::alert_que->que->put(http_tp);//将其放入队列
					pmacdata->first_rpt = false;//更改状态

				}

			}
		
			//判断是否发送异常警告
			if ((pmacdata->rpt->duration > config::alert_time) && ((pmacdata->last_alert_time == 0) || (pmacdata->updatatime - pmacdata->last_alert_time >= config::Alert_space))) //如果报告设备处于异常状态超过规定值
			{

			//	cout << dec << pmacdata->rpt->duration << " ab_flag 1 " << config::alert_time << endl;

				http_tp = http_mngr::pool->get();//从池子里提取一个缓存

				if (nullptr != http_tp) {//若提取成功

					pmacdata->rpt->Creat_Alert_Http(http_tp->buffer, http_tp->real_lenth);//组合http字串
					http_mngrs::alert_que->que->put(http_tp);//将其放入队列
					pmacdata->last_alert_time = pmacdata->updatatime;//记录异常报告发送时间

				}

			}

			//如果需要心跳(心跳包即对时包),每15秒发一次心跳包
			if(pmacdata->heart_beat_time == 0 || pmacdata->updatatime - pmacdata->heart_beat_time > 10){
			
				pmacdata->Creat_Chk_time(cmd_buffer); //建立对时的命令
				pmacdata->heart_beat_time = pmacdata->updatatime;//记录心跳包发送时间
				send(pmacdata->Sock, cmd_buffer, 58, NULL);//直接异步send

			}

			//再投一个recv，仅在返回的是recv且正常时才继续投递recv
			//无需标记type为接收，投递时自会设置
			pmacdata->Recv_IO->MSG_lenth = REPORT_LENTH;//实际需要接收 REPORT_LENTH 长度的数据
			pmacdata->Recv_IO->Flag = MSG_WAITALL; //标志为等待所有数据到齐
			
			//投递一个recv请求
			mac_droped = !socket_mngr::Put_WSARecv(pmacdata->Sock, pmacdata->Recv_IO);

			pmacdata->locker.unlock();//解锁机器

		}
	
	}

	return true;

}

bool site_service(_PER_HANDLE* listened)//用户侧工作线程
{

	cout << "Siteworker ready,IOCP handle is " << hex << (unsigned long long)IOCP_CTRL << endl;

	cmd* cmd_analyzer = new cmd(); //用于分析命令

	const _PER_HANDLE* const listened_sock_data = listened;//监听的套接字的单IO数据

	//两个基结构体指针,保存从完成端口取出的per_handle和per_IO_data
	_PER_HANDLE* per_socket_info = nullptr; 
	_PER_IO* per_io_info = nullptr;

	DWORD dwt = 0; //IOCP实际传输的数据

	bool site_droped = false;//连接是否已断开
	bool mac_droped = false;//设备是否离线

	int8_t cmd_sta = 0;//1,需转发且可转发，0无需转发，-1需转发但有命令在途
	bool cmd_correct = false;//命令是否需要转发

	int8_t cmd_valid = 0;//命令未接收完全(1),命令正常(0),命令无法解析(-1)

	_PER_IO* temp_io_data = nullptr; //acceptex有返回时保存下一步投递recv的单IO数据的临时变量
	_PER_NOMA_HANDLE* temp_sock_data = nullptr;//send和recv时临时保存单句柄数据
	//以上两个临时变量只是为了指针类型转来转去的烦

	char* buffer_bg = nullptr;//保存命令缓存在接收到部分内容后的空白起始位置的临时变量

	mac* mac_temp = nullptr;

	while (true){
	
		//临时变量全部归0
		temp_io_data = nullptr;
		temp_sock_data = nullptr;
		buffer_bg = nullptr;
		mac_temp = nullptr;

		site_droped = false;//连接未断开
		cmd_valid = 0;//命令未接收完全(1),命令正常(0),命令无法解析(-1)

		per_socket_info = nullptr;
		per_io_info = nullptr;

		mac_droped = false;

		dwt = 0; //IOCP实际传输的数据

		//监听完成端口
		site_droped = !GetQueuedCompletionStatus(IOCP_CTRL, &dwt, (PULONG_PTR)&per_socket_info, (LPOVERLAPPED*)&per_io_info, WSA_INFINITE);//每个IO的等待时间无限

		if (per_socket_info == nullptr || per_io_info == nullptr) //如果出错，完成端口本身有问题 此时 site_droped = true
		{

			cout << "CTRL_IOCP UNKNOW ERROR 1 !" << endl;
			continue;

		}

		if (!site_droped) 
		{

			switch (per_io_info->IOtype) //检查IO的类型
			{

			case IOCP_SND: //snd结束,后面统一DISCONNECTEX

				site_droped = true; 
				break;

			case IOCP_DISCONNECTEX://继续投递acceptex,如果失败，标记为未连接，后面统一DISCONNECTEX

				site_droped = !socket_mngr::Put_acceptex(listened->Sock, ((_PER_NOMA_HANDLE*)per_socket_info)->per_accept);
				break;

			case IOCP_ACCEPTEX: //投递recv(这一轮第一次)

				temp_sock_data = (_PER_NOMA_HANDLE*)(((_PER_USER_ACCEPT*)per_io_info)->per_handle);
				temp_io_data = ((_PER_NOMA_HANDLE*)(((_PER_USER_ACCEPT*)per_io_info)->per_handle))->Per_io_data;//下一步recv操作的单IO数据

				memset(temp_sock_data->cmdBuffer,0 ,MAX_CMD_LENTH);//cmdbuff归0
				temp_sock_data->cmdrecvedlen = 0;//长度记录归0

				temp_io_data->Flag = 0;//设置flag
				temp_io_data->MSG_lenth = SITE_BUFF_LENTH;//设置实际允许接收的长度

				socket_mngr::Sock_FIONBIO_Set(temp_sock_data->Sock, false); //设置为非阻塞模式
				socket_mngr::set_keepalive(temp_sock_data->Sock, 6000, 2000);//设置保活机制，6秒，每2秒发送一次心跳

				site_droped = !socket_mngr::Put_WSARecv(temp_sock_data->Sock, temp_io_data); //投递recv

				break;

			case IOCP_RECV: //判断是否接收足够数据，如果没有，继续接收。如果有，分析命令，查找机器
				
				//如果是recv和send，则单句柄数据为 _PER_NOMA_HANDLE类型
				temp_sock_data = (_PER_NOMA_HANDLE*)per_socket_info;

				//如果实际传送的长度+已经缓存的长度超过命令允许的最大长度了
				if ((dwt + temp_sock_data->cmdrecvedlen) > MAX_CMD_LENTH)
				{

					cmd_valid = -1;

				}
				else //否则将新收到的数据拷进缓存
				{
				
					//这次接收的起始位置位于buff的指针加上已经接收到的长度
					buffer_bg = temp_sock_data->cmdBuffer + temp_sock_data->cmdrecvedlen;

					//将新收到的数据拷进缓存
					memcpy(buffer_bg, per_io_info->buffer, dwt);
					temp_sock_data->cmdrecvedlen = dwt + temp_sock_data->cmdrecvedlen;//加上新收到的长度

					//检查已经收到的长度
					if (temp_sock_data->cmdrecvedlen < CMD_HEADER_LENTH) //如果接收到的长度不够头部长度，再投第一个recv请求
					{

						cmd_valid = 1;

					}
					else //长度可能已经够包头长度的话
					{
					    //前两个字节是长度
						socket_mngr::combine_bytes((void*)temp_sock_data->cmdBuffer, cmd_analyzer->lenth);

						if (cmd_analyzer->lenth > MAX_CMD_LENTH)
						{
	
							cmd_valid = -1;

						}
						else 
						{
							//如果已经接收到的长度依然小于包头记录的长度
							if (temp_sock_data->cmdrecvedlen < cmd_analyzer->lenth)
							{

								cmd_valid = 1;

							}
							else if (temp_sock_data->cmdrecvedlen == cmd_analyzer->lenth) //如果收到的长度正好等于
							{

								cmd_valid = 0;

							}
							else //剩下来就是大于了
							{

								cmd_valid = -1;

							}
						
						}

					}
		
				}

				if (cmd_valid > 0)//如果命令未接收完全，继续投递recv
				{

					per_io_info->Flag = 0;//设置flag
					per_io_info->MSG_lenth = SITE_BUFF_LENTH;//设置实际允许接收的长度
					site_droped = !socket_mngr::Put_WSARecv(temp_sock_data->Sock, per_io_info); //投递recv

				}
				else if (cmd_valid < 0)//如果命令长度不对
				{

					site_droped = !mac::reply_error(temp_sock_data, INVALID_CMD);

				}
				else //如果正常，则解析命令
				{

					cmd_analyzer->change_buffer_ptr(temp_sock_data->cmdBuffer);//交换一下单IO缓冲区和cmd分析器的缓冲区指针
					cmd_analyzer->get_header();//把包头分析出来

	
					mac_temp = mac_map::find_mac(cmd_analyzer->SN);

					if (mac_temp == nullptr) {
							
						site_droped = !mac::reply_error(temp_sock_data, MAC_OFF_LINE);//设备掉线
						
					}
					else
					{

						mac_temp->locker.lock();

						if ((mac_temp->Sock == INVALID_SOCKET) ||
							(0 != strcmp(cmd_analyzer->type, mac_temp->mac_type)) ||
							(mac_temp->SN != cmd_analyzer->SN) ||
							(0 != strcmp(cmd_analyzer->psw, mac_temp->ctrl_psw)))
						{
		
							site_droped = !mac::reply_error(temp_sock_data, MAC_OFF_LINE);//报设备不在线

						}
						else
						{

							if (cmd_analyzer->cmd_correct()) //如果命令需要发送，则直接异步发送。直接报告状态（失败则报告drop）
							{

								//直接异步send
								mac_droped = !(58 == send(mac_temp->Sock, cmd_analyzer->buffer, 58, NULL));

							}
								
							if (!mac_droped)//若设备未离线
							{

								memcpy(per_io_info->buffer, mac_temp->rpt->buffer, REPORT_LENTH);
								per_io_info->MSG_lenth = REPORT_LENTH;
								per_io_info->Flag = 0;
								site_droped = !socket_mngr::Put_WSASend(temp_sock_data->Sock, per_io_info);//发送设备状态给php网站

							}
							
						}


						mac_temp->locker.unlock();
						
					}

					if (mac_droped){

						site_droped = !mac::reply_error(temp_sock_data, MAC_OFF_LINE);//报设备不在线
						
					}
					
				}

				break;

			default:
				break;

			}
		
		}

		if (site_droped) //掉线处理disconnectex
		{

			socket_mngr::Put_disconnectex(per_socket_info->Sock, per_io_info);

		}

	}

	return true;

}

//掉线清理线程
void map_clean() {

	while (true) {
	
		Sleep(15000);//休息15秒

		mac_map::clean_map(mac_pool);
	
	
	}

}

//警告发送线程
void alt_send() {

	http_mngrs::alert_que->create_sock_connect();//连接服务器

	while (true) {

		Sleep(15000);//休息15秒
		http_mngrs::alert_que->send_http();

	}

}

//验证线程
void validate() {

	mac* mac_temp = nullptr;//待处理的设备对象
	bool cnnt_suc = false;
	uint8_t db_res = 0;//数据库返回的代码

	//*************************连接db******************
	//初始化数据库
	mysqlctrl* dbctrl = new mysqlctrl(config::db_host, config::db_port, config::db_user, config::db_pwd, config::db_database);

	//连接数据库;
	if (dbctrl->db_connect() == false)
	{

		cout << "Connect db failed!" << endl;
		system("pause");
		return;

	}

	while (true) {

		//从池子里获取一个对象
		mac_temp = wait_que->get();
		
		if (mac_temp == nullptr) {
		
			Sleep(5000);//挂起5秒
			continue;//下一轮

		}

		if ((GetTickCount64() - mac_temp->updatatime) < 20000)//如果等待不足20秒
		{

			wait_que->give_back(mac_temp);//放回队列，即所有的进来之后都等待至少等待20秒
			Sleep(5000);//挂起5秒
			continue;//下一轮

		}

		cnnt_suc = true;

		//检查是否收到足够数据
		if (recv(mac_temp->Sock, mac_temp->Recv_IO->buffer, mac_temp->Recv_IO->buffersize, 0) < REPORT_LENTH)
		{

			cnnt_suc = false;//则未通过

		}

		//解析收到的数据，并比较长度
		if (cnnt_suc)
		{

			mac_temp->exchange_buffer(); //将收到的数据交给cmd;
			mac_temp->rpt->get_state(); //解析命令

			if (mac_temp->rpt->lenth != REPORT_LENTH)
			{

				cnnt_suc = false;//则未通过

			}

		}

		//如果包头的长度对上了，从数据库读取psw和sn
		if (cnnt_suc) //如果收到足够数据，从数据库读取信息
		{

			mac_temp->rpt2mac();//写入SN及type

			db_res = mac_temp->get_infor_from_db(dbctrl); //尝试从数据库获得数据

			while (db_res == 2) //如果返回值提示数据库连接失败
			{

				while (dbctrl->db_connect() == false) //每10秒重连一次数据库
				{

					Sleep(10000); //挂起10秒

				}

				db_res = mac_temp->get_infor_from_db(dbctrl);

			}

			if (db_res != 0)
			{
				//int db_error = db_res;
				//		cout << "DB ERROR CODE: " << db_error << endl;

				cnnt_suc = false;//则未连接成功

			}

		}

		//如果成功从数据库获取到数据，检查sn和psw
		if (cnnt_suc)
		{

			cnnt_suc = mac_temp->chk_report();

			/*	if (cnnt_suc) {

			cout << "New mac connected!" << endl;
			cout << "SN:" << mac_temp->SN << " MAC_PSW:" << mac_temp->psw << " CTRL_PSW:" << mac_temp->ctrl_psw << endl;
			cout << "*" << endl;

			} */

		}

		//如果psw和sn一致，添加到完成端口
		if (cnnt_suc)
		{

			mac_temp->updatatime = GetTickCount64(); //记录时间
			socket_mngr::Sock_FIONBIO_Set(mac_temp->Sock, true);//设置socket为阻塞方式
			socket_mngr::set_keepalive(mac_temp->Sock, 30000, 5000);//设置保活 30秒

			if (NULL == CreateIoCompletionPort((HANDLE)mac_temp->Sock, IOCP_MAC, (ULONG_PTR)mac_temp, 0))//将socket加入完成端口
			{

				cnnt_suc = false;

			}

		}

		//如果添加到完成端口成功，投递一个recv
		if (cnnt_suc)
		{

			mac_temp->Recv_IO->MSG_lenth = REPORT_LENTH;//实际需要接收 REPORT_LENTH 长度的数据
			mac_temp->Recv_IO->Flag = MSG_WAITALL; //标志为等待所有数据到齐
			cnnt_suc = socket_mngr::Put_WSARecv(mac_temp->Sock, mac_temp->Recv_IO); //投递一个recv请求

		}

		//如果投递recv成功,加入map
		if (cnnt_suc)
		{

			mac_map::add_mac(mac_temp); //添加进map

		}

		//如果以上有任何一步出错。
		if (!cnnt_suc)
		{

			closesocket(mac_temp->Sock); //关闭套接字
			mac_temp->reset_mac();//重置设备
			mac_pool->put(mac_temp);//将对象放回池子

		}

	}

}

//主线程
int main()
{
	/*
	*主线程的工作：
	*1、建立完成端口（两个，一个负责接待所有的机器，一个负责接待控制端请求）
	*2、负责监听机器连入的端口，是否有新的机器连入，若连入，将其加入完成端口1
	*3、建立数个套接字，并将其加入完成端口，以备控制端连接
	*/

	/*初始化socket*/
	WSADATA WSAdata;
	socket_mngr::startwsock(WSAdata);	

	/*配置文件载入*/
	config::loadconfig(); //读取配置文件
	config::showconfig(); //显示配置值

	socket_mngr::get_resort_ptr();//判断服务器的大小头

	//建立连接其他服务器的管理器
	http_mngrs::alert_que = new http_mngr(&(config::alert_addr), config::Alert_buffsize);

	//******************准备设备对象池及等待队列****************
	mac_pool->mass_add(config::maxcnnt); //向设备池批量添加设备

	//***********准备哈希表,所有已连接的机器对象的指针将保存在此*************
	mac_map::init_map(config::maxcnnt);

	//********************建立两个完成端口*************************
	IOCP_CTRL = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);//建立连接PHP的完成端口
	IOCP_MAC = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);//建立连接机器、API服务器的完成端口

	if (IOCP_MAC == NULL || IOCP_CTRL == NULL)
	{
		cout << "creat IOCP failed!" << endl;
		system("pause");
		return 0;

	}
	//********************建立两个完成端口结束*************************

	//*********建立监听PHP网站请求的套接字并将其加入完成端口***********
	_PER_HANDLE* listen_sock_per_handle_data = new _PER_HANDLE;//建立监听PHP网站请求的套接字的端口的单句柄数据

//	cout << "a" << endl;
	socket_mngr::setaddrstru(config::user_ip, config::user_port, listen_sock_per_handle_data->remote_Address);//建立socketaddr结构体
	listen_sock_per_handle_data->Sock = socket_mngr::create_listen_sock(listen_sock_per_handle_data->remote_Address);//建立一个用于监听用户请求的套接字
	socket_mngr::get_ex_func_pointer(listen_sock_per_handle_data->Sock);//获取ex函数指针
//	cout << "b" << endl;

	//将监听PHP网站请求的套接字加入完成端口
	if (NULL == CreateIoCompletionPort((HANDLE)listen_sock_per_handle_data->Sock, IOCP_CTRL, (ULONG_PTR)listen_sock_per_handle_data, 0))
	{

		cout << "Create user IOCP failed" << endl;
		system("pause");
		return 0;

	}
	else 
	{
	
		cout << "Create user IOCP success!" << endl;

	}
	//*****************************************************************

	//建立500倍于工作线程数量的socket并投递acceptex至IOCP（用于连接PHP网站)
	DWORD addrsbuffersize = (sizeof(sockaddr_in) + 16);

	DWORD lpdwBytesReceived = 0;

	size_t sock_mun = config::userthreads * 500;

	for (int i = 0; i < sock_mun; ++i)//因为控制端的套接字需要等待设备的响应，IO时间较长
	{

		//准备被复用的套接字的单句柄数据
		_PER_NOMA_HANDLE* per_noma_sock_handle = new _PER_NOMA_HANDLE();//需要被复用的socket的单句柄数据(内部包含了一个用于复用时的单IO数据)
		per_noma_sock_handle->Sock = WSASocketW(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);//建立套接字

		socket_mngr::Sock_FIONBIO_Set(per_noma_sock_handle->Sock, false); //设定套接字为非阻塞模式

		//绑定至完成端口
		if (NULL == CreateIoCompletionPort((HANDLE)per_noma_sock_handle->Sock, IOCP_CTRL, (ULONG_PTR)per_noma_sock_handle, 0))//如果加入完成端口失败
		{

			cout << "SOCKET Join CTRL IOCP failed" << endl;
			system("pause");
			return 0;

		}

		//投递acceptex
		if (!socket_mngr::Put_acceptex(listen_sock_per_handle_data->Sock, per_noma_sock_handle->per_accept))
		{

			cout << "acceptex failed!" << endl;
			system("pause");
			return 0;

		}

	}

	for (int i = 0; i < config::userthreads; ++i)//建立工作线程
	{

		thread* userworker = new thread(bind(site_service, listen_sock_per_handle_data));
		userworker->detach();

	}

	//******************************************************************
	//建立监听设备连接的套接字

	//cout << "c" << endl;
	SOCKET mac_sockListen = socket_mngr::create_listen_sock(config::mac_addr);
	//cout << "d" << endl;
	//建立服务机器的工作线程
	for (int i = 0; i < config::maxthread; ++i)
	{

		thread* worker = new thread(mac_sevice);
		worker->detach();

	}

	//启动清理掉线,警告及设备验证线程
	for (int i = 0; i < config::Alert_thread; ++i)
	{
		thread* alt_sender = new thread(alt_send);
		alt_sender->detach();
	}

	for (int i = 0; i < config::valthread; ++i)
	{
		thread* mac_validator = new thread(validate);
		mac_validator->detach();
	}

	thread* map_cleaner = new thread(map_clean);
	map_cleaner->detach();

	/*****准备循环用变量****/
	mac* mac_temp = nullptr;
	int addrlenth = sizeof(sockaddr);

	//******************************循环开始***********************************
	while(true){

		//从设备池里获取一个设备对象
		mac_temp = mac_pool->get();

		//判断池子里是否还有设备对象
		if (nullptr == mac_temp)//如果对象池空了
		{
			
			Sleep(300000); //挂起5分钟
			continue;
				
		}

		socket_mngr::Sock_FIONBIO_Set(mac_sockListen, true);//将监听的套接字设置为阻塞方式
		
		mac_temp->Sock = accept(mac_sockListen, (sockaddr*)&mac_temp->remote_Address, &addrlenth);//accept
		mac_temp->updatatime = GetTickCount64();//记录连接时间
		wait_que->put(mac_temp);//将对象加入等待队列

	}

    system("pause");
    return 1;

}










