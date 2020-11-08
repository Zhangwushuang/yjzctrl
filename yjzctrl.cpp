// yjzctrl.cpp : �������̨Ӧ�ó������ڵ㡣
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

/*������ɶ˿�*/
HANDLE IOCP_MAC = nullptr; //���ӻ�������ɶ˿�
HANDLE IOCP_CTRL = nullptr; //����PHP�������ɶ˿�

//�ս���ʱʹ�õ��豸����ؼ��ȴ�����
mac_que* const mac_pool(new mac_que()); //�豸��
mac_que* const wait_que(new mac_que()); //�ȴ�����

/*��ϣ�����ڱ������ӽ����Ļ�������Ϣ,������ȫ��̬��Ա������ʵ�������ɲ鿴mac_map.h��*/


/******
���豸�������WSASEND��ʱ��_PER_IO_DATE�ء�

�豸����ÿ���豸������һ��ר�е�_PER_IO_DATE�����ڽ��գ�WSArecv����
��վ��ĸ����׽���ÿ��ֻͶ��һ��IO�������Ը���һ��ר�е�_PER_IO_DATE��
*************/

bool mac_sevice()//������Ĺ����߳�(ֻ����recv�ͷ���������,ֱ���첽����,��������ɶ˿�)
{

	cout << "macworker ready,IOCP handle is " << hex << (unsigned long long)IOCP_MAC << endl;

	mac* pmacdata = nullptr; //��ɶ˿ڽ��յ��ĵ�������ݻ���
	_PER_IO* piodata = nullptr;//��ɶ˿ڽ��յ��ĵ�IO���ݻ���
	DWORD dwt = 0; //��ɶ˿ڴ��͵����ݳ���
	bool mac_droped = false;//�豸�Ƿ���߻��߱������

	char cmd_buffer[58];//����Ļ���
	http_node* http_tp = nullptr;

	while (true)
	{

		pmacdata = nullptr;
		piodata = nullptr;

		//�ȴ�ʱ������,�������ߵ�ȫ���������,����ר�ŵ��̴߳���(���߳�ֻ����ɹ����յ��豸�����ı���)
		//��Ϊ������������ض���ʱ�������̻߳���յ��ߵ��豸���󣬵�Io���ݱ������豸��������Բ���й¶
		if (GetQueuedCompletionStatus(IOCP_MAC, &dwt, (PULONG_PTR)&pmacdata, (LPOVERLAPPED*)&piodata, WSA_INFINITE)) { //�����ɶ˿ڷ���
		
			if (pmacdata == nullptr || piodata == nullptr || piodata != pmacdata->Recv_IO || dwt != REPORT_LENTH)
			{

		//		cout << "MAC_IOCP UNKNOW ERROR 1 !" << endl;
				continue;

			}

			pmacdata->locker.lock();//��������

			pmacdata->exchange_buffer();
			pmacdata->rpt->get_state();//�����յ�������

			if (!pmacdata->chk_report()) {//�ж��豸���кźͿ���(�����һ��)

				pmacdata->locker.unlock();//��������
				continue;

			}

			pmacdata->updatatime = GetTickCount64(); //��¼��ȡ���ݵ�ʱ��

			//�Ƿ������߱���
			if (pmacdata->first_rpt)
			{
			//	cout << "online_flag " << endl;
				http_tp = http_mngr::pool->get();//�ӳ�������ȡһ������

				if (nullptr != http_tp) {//����ȡ�ɹ�

					pmacdata->rpt->Creat_Alert_Http(http_tp->buffer, http_tp->real_lenth, "2");//���http�ִ�
					http_mngrs::alert_que->que->put(http_tp);//����������
					pmacdata->first_rpt = false;//����״̬

				}

			}
		
			//�ж��Ƿ����쳣����
			if ((pmacdata->rpt->duration > config::alert_time) && ((pmacdata->last_alert_time == 0) || (pmacdata->updatatime - pmacdata->last_alert_time >= config::Alert_space))) //��������豸�����쳣״̬�����涨ֵ
			{

			//	cout << dec << pmacdata->rpt->duration << " ab_flag 1 " << config::alert_time << endl;

				http_tp = http_mngr::pool->get();//�ӳ�������ȡһ������

				if (nullptr != http_tp) {//����ȡ�ɹ�

					pmacdata->rpt->Creat_Alert_Http(http_tp->buffer, http_tp->real_lenth);//���http�ִ�
					http_mngrs::alert_que->que->put(http_tp);//����������
					pmacdata->last_alert_time = pmacdata->updatatime;//��¼�쳣���淢��ʱ��

				}

			}

			//�����Ҫ����(����������ʱ��),ÿ15�뷢һ��������
			if(pmacdata->heart_beat_time == 0 || pmacdata->updatatime - pmacdata->heart_beat_time > 10){
			
				pmacdata->Creat_Chk_time(cmd_buffer); //������ʱ������
				pmacdata->heart_beat_time = pmacdata->updatatime;//��¼����������ʱ��
				send(pmacdata->Sock, cmd_buffer, 58, NULL);//ֱ���첽send

			}

			//��Ͷһ��recv�����ڷ��ص���recv������ʱ�ż���Ͷ��recv
			//������typeΪ���գ�Ͷ��ʱ�Ի�����
			pmacdata->Recv_IO->MSG_lenth = REPORT_LENTH;//ʵ����Ҫ���� REPORT_LENTH ���ȵ�����
			pmacdata->Recv_IO->Flag = MSG_WAITALL; //��־Ϊ�ȴ��������ݵ���
			
			//Ͷ��һ��recv����
			mac_droped = !socket_mngr::Put_WSARecv(pmacdata->Sock, pmacdata->Recv_IO);

			pmacdata->locker.unlock();//��������

		}
	
	}

	return true;

}

bool site_service(_PER_HANDLE* listened)//�û��๤���߳�
{

	cout << "Siteworker ready,IOCP handle is " << hex << (unsigned long long)IOCP_CTRL << endl;

	cmd* cmd_analyzer = new cmd(); //���ڷ�������

	const _PER_HANDLE* const listened_sock_data = listened;//�������׽��ֵĵ�IO����

	//�������ṹ��ָ��,�������ɶ˿�ȡ����per_handle��per_IO_data
	_PER_HANDLE* per_socket_info = nullptr; 
	_PER_IO* per_io_info = nullptr;

	DWORD dwt = 0; //IOCPʵ�ʴ��������

	bool site_droped = false;//�����Ƿ��ѶϿ�
	bool mac_droped = false;//�豸�Ƿ�����

	int8_t cmd_sta = 0;//1,��ת���ҿ�ת����0����ת����-1��ת������������;
	bool cmd_correct = false;//�����Ƿ���Ҫת��

	int8_t cmd_valid = 0;//����δ������ȫ(1),��������(0),�����޷�����(-1)

	_PER_IO* temp_io_data = nullptr; //acceptex�з���ʱ������һ��Ͷ��recv�ĵ�IO���ݵ���ʱ����
	_PER_NOMA_HANDLE* temp_sock_data = nullptr;//send��recvʱ��ʱ���浥�������
	//����������ʱ����ֻ��Ϊ��ָ������ת��תȥ�ķ�

	char* buffer_bg = nullptr;//����������ڽ��յ��������ݺ�Ŀհ���ʼλ�õ���ʱ����

	mac* mac_temp = nullptr;

	while (true){
	
		//��ʱ����ȫ����0
		temp_io_data = nullptr;
		temp_sock_data = nullptr;
		buffer_bg = nullptr;
		mac_temp = nullptr;

		site_droped = false;//����δ�Ͽ�
		cmd_valid = 0;//����δ������ȫ(1),��������(0),�����޷�����(-1)

		per_socket_info = nullptr;
		per_io_info = nullptr;

		mac_droped = false;

		dwt = 0; //IOCPʵ�ʴ��������

		//������ɶ˿�
		site_droped = !GetQueuedCompletionStatus(IOCP_CTRL, &dwt, (PULONG_PTR)&per_socket_info, (LPOVERLAPPED*)&per_io_info, WSA_INFINITE);//ÿ��IO�ĵȴ�ʱ������

		if (per_socket_info == nullptr || per_io_info == nullptr) //���������ɶ˿ڱ��������� ��ʱ site_droped = true
		{

			cout << "CTRL_IOCP UNKNOW ERROR 1 !" << endl;
			continue;

		}

		if (!site_droped) 
		{

			switch (per_io_info->IOtype) //���IO������
			{

			case IOCP_SND: //snd����,����ͳһDISCONNECTEX

				site_droped = true; 
				break;

			case IOCP_DISCONNECTEX://����Ͷ��acceptex,���ʧ�ܣ����Ϊδ���ӣ�����ͳһDISCONNECTEX

				site_droped = !socket_mngr::Put_acceptex(listened->Sock, ((_PER_NOMA_HANDLE*)per_socket_info)->per_accept);
				break;

			case IOCP_ACCEPTEX: //Ͷ��recv(��һ�ֵ�һ��)

				temp_sock_data = (_PER_NOMA_HANDLE*)(((_PER_USER_ACCEPT*)per_io_info)->per_handle);
				temp_io_data = ((_PER_NOMA_HANDLE*)(((_PER_USER_ACCEPT*)per_io_info)->per_handle))->Per_io_data;//��һ��recv�����ĵ�IO����

				memset(temp_sock_data->cmdBuffer,0 ,MAX_CMD_LENTH);//cmdbuff��0
				temp_sock_data->cmdrecvedlen = 0;//���ȼ�¼��0

				temp_io_data->Flag = 0;//����flag
				temp_io_data->MSG_lenth = SITE_BUFF_LENTH;//����ʵ��������յĳ���

				socket_mngr::Sock_FIONBIO_Set(temp_sock_data->Sock, false); //����Ϊ������ģʽ
				socket_mngr::set_keepalive(temp_sock_data->Sock, 6000, 2000);//���ñ�����ƣ�6�룬ÿ2�뷢��һ������

				site_droped = !socket_mngr::Put_WSARecv(temp_sock_data->Sock, temp_io_data); //Ͷ��recv

				break;

			case IOCP_RECV: //�ж��Ƿ�����㹻���ݣ����û�У��������ա�����У�����������һ���
				
				//�����recv��send���򵥾������Ϊ _PER_NOMA_HANDLE����
				temp_sock_data = (_PER_NOMA_HANDLE*)per_socket_info;

				//���ʵ�ʴ��͵ĳ���+�Ѿ�����ĳ��ȳ��������������󳤶���
				if ((dwt + temp_sock_data->cmdrecvedlen) > MAX_CMD_LENTH)
				{

					cmd_valid = -1;

				}
				else //�������յ������ݿ�������
				{
				
					//��ν��յ���ʼλ��λ��buff��ָ������Ѿ����յ��ĳ���
					buffer_bg = temp_sock_data->cmdBuffer + temp_sock_data->cmdrecvedlen;

					//�����յ������ݿ�������
					memcpy(buffer_bg, per_io_info->buffer, dwt);
					temp_sock_data->cmdrecvedlen = dwt + temp_sock_data->cmdrecvedlen;//�������յ��ĳ���

					//����Ѿ��յ��ĳ���
					if (temp_sock_data->cmdrecvedlen < CMD_HEADER_LENTH) //������յ��ĳ��Ȳ���ͷ�����ȣ���Ͷ��һ��recv����
					{

						cmd_valid = 1;

					}
					else //���ȿ����Ѿ�����ͷ���ȵĻ�
					{
					    //ǰ�����ֽ��ǳ���
						socket_mngr::combine_bytes((void*)temp_sock_data->cmdBuffer, cmd_analyzer->lenth);

						if (cmd_analyzer->lenth > MAX_CMD_LENTH)
						{
	
							cmd_valid = -1;

						}
						else 
						{
							//����Ѿ����յ��ĳ�����ȻС�ڰ�ͷ��¼�ĳ���
							if (temp_sock_data->cmdrecvedlen < cmd_analyzer->lenth)
							{

								cmd_valid = 1;

							}
							else if (temp_sock_data->cmdrecvedlen == cmd_analyzer->lenth) //����յ��ĳ������õ���
							{

								cmd_valid = 0;

							}
							else //ʣ�������Ǵ�����
							{

								cmd_valid = -1;

							}
						
						}

					}
		
				}

				if (cmd_valid > 0)//�������δ������ȫ������Ͷ��recv
				{

					per_io_info->Flag = 0;//����flag
					per_io_info->MSG_lenth = SITE_BUFF_LENTH;//����ʵ��������յĳ���
					site_droped = !socket_mngr::Put_WSARecv(temp_sock_data->Sock, per_io_info); //Ͷ��recv

				}
				else if (cmd_valid < 0)//�������Ȳ���
				{

					site_droped = !mac::reply_error(temp_sock_data, INVALID_CMD);

				}
				else //������������������
				{

					cmd_analyzer->change_buffer_ptr(temp_sock_data->cmdBuffer);//����һ�µ�IO��������cmd�������Ļ�����ָ��
					cmd_analyzer->get_header();//�Ѱ�ͷ��������

	
					mac_temp = mac_map::find_mac(cmd_analyzer->SN);

					if (mac_temp == nullptr) {
							
						site_droped = !mac::reply_error(temp_sock_data, MAC_OFF_LINE);//�豸����
						
					}
					else
					{

						mac_temp->locker.lock();

						if ((mac_temp->Sock == INVALID_SOCKET) ||
							(0 != strcmp(cmd_analyzer->type, mac_temp->mac_type)) ||
							(mac_temp->SN != cmd_analyzer->SN) ||
							(0 != strcmp(cmd_analyzer->psw, mac_temp->ctrl_psw)))
						{
		
							site_droped = !mac::reply_error(temp_sock_data, MAC_OFF_LINE);//���豸������

						}
						else
						{

							if (cmd_analyzer->cmd_correct()) //���������Ҫ���ͣ���ֱ���첽���͡�ֱ�ӱ���״̬��ʧ���򱨸�drop��
							{

								//ֱ���첽send
								mac_droped = !(58 == send(mac_temp->Sock, cmd_analyzer->buffer, 58, NULL));

							}
								
							if (!mac_droped)//���豸δ����
							{

								memcpy(per_io_info->buffer, mac_temp->rpt->buffer, REPORT_LENTH);
								per_io_info->MSG_lenth = REPORT_LENTH;
								per_io_info->Flag = 0;
								site_droped = !socket_mngr::Put_WSASend(temp_sock_data->Sock, per_io_info);//�����豸״̬��php��վ

							}
							
						}


						mac_temp->locker.unlock();
						
					}

					if (mac_droped){

						site_droped = !mac::reply_error(temp_sock_data, MAC_OFF_LINE);//���豸������
						
					}
					
				}

				break;

			default:
				break;

			}
		
		}

		if (site_droped) //���ߴ���disconnectex
		{

			socket_mngr::Put_disconnectex(per_socket_info->Sock, per_io_info);

		}

	}

	return true;

}

//���������߳�
void map_clean() {

	while (true) {
	
		Sleep(15000);//��Ϣ15��

		mac_map::clean_map(mac_pool);
	
	
	}

}

//���淢���߳�
void alt_send() {

	http_mngrs::alert_que->create_sock_connect();//���ӷ�����

	while (true) {

		Sleep(15000);//��Ϣ15��
		http_mngrs::alert_que->send_http();

	}

}

//��֤�߳�
void validate() {

	mac* mac_temp = nullptr;//��������豸����
	bool cnnt_suc = false;
	uint8_t db_res = 0;//���ݿⷵ�صĴ���

	//*************************����db******************
	//��ʼ�����ݿ�
	mysqlctrl* dbctrl = new mysqlctrl(config::db_host, config::db_port, config::db_user, config::db_pwd, config::db_database);

	//�������ݿ�;
	if (dbctrl->db_connect() == false)
	{

		cout << "Connect db failed!" << endl;
		system("pause");
		return;

	}

	while (true) {

		//�ӳ������ȡһ������
		mac_temp = wait_que->get();
		
		if (mac_temp == nullptr) {
		
			Sleep(5000);//����5��
			continue;//��һ��

		}

		if ((GetTickCount64() - mac_temp->updatatime) < 20000)//����ȴ�����20��
		{

			wait_que->give_back(mac_temp);//�Żض��У������еĽ���֮�󶼵ȴ����ٵȴ�20��
			Sleep(5000);//����5��
			continue;//��һ��

		}

		cnnt_suc = true;

		//����Ƿ��յ��㹻����
		if (recv(mac_temp->Sock, mac_temp->Recv_IO->buffer, mac_temp->Recv_IO->buffersize, 0) < REPORT_LENTH)
		{

			cnnt_suc = false;//��δͨ��

		}

		//�����յ������ݣ����Ƚϳ���
		if (cnnt_suc)
		{

			mac_temp->exchange_buffer(); //���յ������ݽ���cmd;
			mac_temp->rpt->get_state(); //��������

			if (mac_temp->rpt->lenth != REPORT_LENTH)
			{

				cnnt_suc = false;//��δͨ��

			}

		}

		//�����ͷ�ĳ��ȶ����ˣ������ݿ��ȡpsw��sn
		if (cnnt_suc) //����յ��㹻���ݣ������ݿ��ȡ��Ϣ
		{

			mac_temp->rpt2mac();//д��SN��type

			db_res = mac_temp->get_infor_from_db(dbctrl); //���Դ����ݿ�������

			while (db_res == 2) //�������ֵ��ʾ���ݿ�����ʧ��
			{

				while (dbctrl->db_connect() == false) //ÿ10������һ�����ݿ�
				{

					Sleep(10000); //����10��

				}

				db_res = mac_temp->get_infor_from_db(dbctrl);

			}

			if (db_res != 0)
			{
				//int db_error = db_res;
				//		cout << "DB ERROR CODE: " << db_error << endl;

				cnnt_suc = false;//��δ���ӳɹ�

			}

		}

		//����ɹ������ݿ��ȡ�����ݣ����sn��psw
		if (cnnt_suc)
		{

			cnnt_suc = mac_temp->chk_report();

			/*	if (cnnt_suc) {

			cout << "New mac connected!" << endl;
			cout << "SN:" << mac_temp->SN << " MAC_PSW:" << mac_temp->psw << " CTRL_PSW:" << mac_temp->ctrl_psw << endl;
			cout << "*" << endl;

			} */

		}

		//���psw��snһ�£���ӵ���ɶ˿�
		if (cnnt_suc)
		{

			mac_temp->updatatime = GetTickCount64(); //��¼ʱ��
			socket_mngr::Sock_FIONBIO_Set(mac_temp->Sock, true);//����socketΪ������ʽ
			socket_mngr::set_keepalive(mac_temp->Sock, 30000, 5000);//���ñ��� 30��

			if (NULL == CreateIoCompletionPort((HANDLE)mac_temp->Sock, IOCP_MAC, (ULONG_PTR)mac_temp, 0))//��socket������ɶ˿�
			{

				cnnt_suc = false;

			}

		}

		//�����ӵ���ɶ˿ڳɹ���Ͷ��һ��recv
		if (cnnt_suc)
		{

			mac_temp->Recv_IO->MSG_lenth = REPORT_LENTH;//ʵ����Ҫ���� REPORT_LENTH ���ȵ�����
			mac_temp->Recv_IO->Flag = MSG_WAITALL; //��־Ϊ�ȴ��������ݵ���
			cnnt_suc = socket_mngr::Put_WSARecv(mac_temp->Sock, mac_temp->Recv_IO); //Ͷ��һ��recv����

		}

		//���Ͷ��recv�ɹ�,����map
		if (cnnt_suc)
		{

			mac_map::add_mac(mac_temp); //��ӽ�map

		}

		//����������κ�һ������
		if (!cnnt_suc)
		{

			closesocket(mac_temp->Sock); //�ر��׽���
			mac_temp->reset_mac();//�����豸
			mac_pool->put(mac_temp);//������Żس���

		}

	}

}

//���߳�
int main()
{
	/*
	*���̵߳Ĺ�����
	*1��������ɶ˿ڣ�������һ������Ӵ����еĻ�����һ������Ӵ����ƶ�����
	*2�����������������Ķ˿ڣ��Ƿ����µĻ������룬�����룬���������ɶ˿�1
	*3�����������׽��֣������������ɶ˿ڣ��Ա����ƶ�����
	*/

	/*��ʼ��socket*/
	WSADATA WSAdata;
	socket_mngr::startwsock(WSAdata);	

	/*�����ļ�����*/
	config::loadconfig(); //��ȡ�����ļ�
	config::showconfig(); //��ʾ����ֵ

	socket_mngr::get_resort_ptr();//�жϷ������Ĵ�Сͷ

	//�������������������Ĺ�����
	http_mngrs::alert_que = new http_mngr(&(config::alert_addr), config::Alert_buffsize);

	//******************׼���豸����ؼ��ȴ�����****************
	mac_pool->mass_add(config::maxcnnt); //���豸����������豸

	//***********׼����ϣ��,���������ӵĻ��������ָ�뽫�����ڴ�*************
	mac_map::init_map(config::maxcnnt);

	//********************����������ɶ˿�*************************
	IOCP_CTRL = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);//��������PHP����ɶ˿�
	IOCP_MAC = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);//�������ӻ�����API����������ɶ˿�

	if (IOCP_MAC == NULL || IOCP_CTRL == NULL)
	{
		cout << "creat IOCP failed!" << endl;
		system("pause");
		return 0;

	}
	//********************����������ɶ˿ڽ���*************************

	//*********��������PHP��վ������׽��ֲ����������ɶ˿�***********
	_PER_HANDLE* listen_sock_per_handle_data = new _PER_HANDLE;//��������PHP��վ������׽��ֵĶ˿ڵĵ��������

//	cout << "a" << endl;
	socket_mngr::setaddrstru(config::user_ip, config::user_port, listen_sock_per_handle_data->remote_Address);//����socketaddr�ṹ��
	listen_sock_per_handle_data->Sock = socket_mngr::create_listen_sock(listen_sock_per_handle_data->remote_Address);//����һ�����ڼ����û�������׽���
	socket_mngr::get_ex_func_pointer(listen_sock_per_handle_data->Sock);//��ȡex����ָ��
//	cout << "b" << endl;

	//������PHP��վ������׽��ּ�����ɶ˿�
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

	//����500���ڹ����߳�������socket��Ͷ��acceptex��IOCP����������PHP��վ)
	DWORD addrsbuffersize = (sizeof(sockaddr_in) + 16);

	DWORD lpdwBytesReceived = 0;

	size_t sock_mun = config::userthreads * 500;

	for (int i = 0; i < sock_mun; ++i)//��Ϊ���ƶ˵��׽�����Ҫ�ȴ��豸����Ӧ��IOʱ��ϳ�
	{

		//׼�������õ��׽��ֵĵ��������
		_PER_NOMA_HANDLE* per_noma_sock_handle = new _PER_NOMA_HANDLE();//��Ҫ�����õ�socket�ĵ��������(�ڲ�������һ�����ڸ���ʱ�ĵ�IO����)
		per_noma_sock_handle->Sock = WSASocketW(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);//�����׽���

		socket_mngr::Sock_FIONBIO_Set(per_noma_sock_handle->Sock, false); //�趨�׽���Ϊ������ģʽ

		//������ɶ˿�
		if (NULL == CreateIoCompletionPort((HANDLE)per_noma_sock_handle->Sock, IOCP_CTRL, (ULONG_PTR)per_noma_sock_handle, 0))//���������ɶ˿�ʧ��
		{

			cout << "SOCKET Join CTRL IOCP failed" << endl;
			system("pause");
			return 0;

		}

		//Ͷ��acceptex
		if (!socket_mngr::Put_acceptex(listen_sock_per_handle_data->Sock, per_noma_sock_handle->per_accept))
		{

			cout << "acceptex failed!" << endl;
			system("pause");
			return 0;

		}

	}

	for (int i = 0; i < config::userthreads; ++i)//���������߳�
	{

		thread* userworker = new thread(bind(site_service, listen_sock_per_handle_data));
		userworker->detach();

	}

	//******************************************************************
	//���������豸���ӵ��׽���

	//cout << "c" << endl;
	SOCKET mac_sockListen = socket_mngr::create_listen_sock(config::mac_addr);
	//cout << "d" << endl;
	//������������Ĺ����߳�
	for (int i = 0; i < config::maxthread; ++i)
	{

		thread* worker = new thread(mac_sevice);
		worker->detach();

	}

	//�����������,���漰�豸��֤�߳�
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

	/*****׼��ѭ���ñ���****/
	mac* mac_temp = nullptr;
	int addrlenth = sizeof(sockaddr);

	//******************************ѭ����ʼ***********************************
	while(true){

		//���豸�����ȡһ���豸����
		mac_temp = mac_pool->get();

		//�жϳ������Ƿ����豸����
		if (nullptr == mac_temp)//�������ؿ���
		{
			
			Sleep(300000); //����5����
			continue;
				
		}

		socket_mngr::Sock_FIONBIO_Set(mac_sockListen, true);//���������׽�������Ϊ������ʽ
		
		mac_temp->Sock = accept(mac_sockListen, (sockaddr*)&mac_temp->remote_Address, &addrlenth);//accept
		mac_temp->updatatime = GetTickCount64();//��¼����ʱ��
		wait_que->put(mac_temp);//���������ȴ�����

	}

    system("pause");
    return 1;

}










