#include "stdafx.h"
#include "mac.h"

mac::mac():Recv_IO(new _PER_IO(REPORT_LENTH)),rpt(new report())
{

	this->Recv_IO->IOtype = IOCP_RECV;
	reset_mac();

}

mac::~mac()
{

	delete this->rpt;
	delete this->Recv_IO;

}

/********************************/

void mac::rpt2mac()
{

	memset((void*)this->mac_type, 0, 5);//类型归0
	memcpy((void*)this->mac_type, (void*)(this->rpt->type), 4);//获取类型

	this->SN = this->rpt->SN;//写入SN

}

uint8_t mac::get_infor_from_db(mysqlctrl* db)
{

	if (0 != mysql_ping(db->pmydata))
	{

		return 2;//数据库连接失败

	}

	char query[120] = { '\0' };
	memcpy((void*)query, "SELECT `mac_psw`, `ctrl_psw`, `Api_id`, `mac_type`, `t_zone` FROM `tbl_mac` WHERE `mac_id` = ", 93);

	sprintf_s(query + 93, 22, "%I64d", this->SN); //将SN输入字符串

	size_t query_lenth = strlen(query); //计算长度

	size_t searchres = mysql_real_query(db->pmydata, query, query_lenth);

	if (searchres != 0)
	{

		return 3;//搜索发生错误

	}


	MYSQL_RES* results = mysql_store_result(db->pmydata);

	if (results == nullptr)
	{

		return 4;//读取结果集失败

	}


	MYSQL_ROW row = mysql_fetch_row(results);

	if (row == NULL)
	{

		mysql_free_result(results);
		return 5;//无结果	

	}

	memcpy((void*)this->psw, row[0], 12);

	memcpy((void*)this->ctrl_psw, row[1], 12);

	size_t api_id_len = strlen(row[2]);

	if (api_id_len > 49 || 0 != strcmp(this->mac_type,row[3])) //如果
	{

		mysql_free_result(results);
		return 6;

	}

	memcpy((void*)this->Api_id, row[2], api_id_len);

	sscanf_s(row[4], "%lf", &(this->t_zone)); //获取时区

	mysql_free_result(results);
	return 0;//搜索成功

}

bool mac::chk_report()
{

	bool type_same = (0 == strcmp(this->mac_type, this->rpt->type)); //控制器类型一致
	bool sn_same = (this->SN == this->rpt->SN); //序列号一致
	bool psw_same = (0 == strcmp(this->psw,this->rpt->psw)); //口令一致

	return (type_same && sn_same && psw_same);

}

void mac::reset_mac()
{

	this->reset_header();

	if (this->rpt != nullptr)
	{

		this->rpt->reset_rpt();

	}

	memset((void*)this->ctrl_psw, 0, 13);
	memset((void*)this->Api_id, 0, 50);
	memset((void*)this->mac_type, 0, 5);

	this->updatatime = 0;//连接或最后一次获得数据的时间
	this->last_alert_time = 0;//最后一次发送警告时，异常状态持续的时间
	this->first_rpt = true;
	this->heart_beat_time = 0;
	this->t_zone = 0;//时区

	if (this->Sock != INVALID_SOCKET) 
	{
	
		closesocket(this->Sock); //关闭套接字
		this->Sock = INVALID_SOCKET;//将套接字系好
	
	}

}

bool mac::reply_error(_PER_NOMA_HANDLE* data, uint8_t error)
{

	bool temp_bool = true;//默认网站仍然连接

	if (data != nullptr)
	{

		if (data->Sock != INVALID_SOCKET)
		{

			//无需设置type,函数自会设置。
			data->Per_io_data->MSG_lenth = CMD_HEADER_LENTH;//设置要发送的长度为 CMD_HEADER_LENTH 只传送一个头部
			data->Per_io_data->Flag = 0;//设置标志位为0

			//设置发送的内容
			memset(data->Per_io_data->buffer, 0, data->Per_io_data->buffersize); //per_io的buff全部置零

			uint16_t lenth = CMD_HEADER_LENTH;//写入长度数据
			socket_mngr::create_bytes(lenth, data->Per_io_data->buffer);

			memcpy((data->Per_io_data->buffer) + 2, "0000", 4);//写入type
			memset((data->Per_io_data->buffer) + 6, 'X', 20);//SN 4 和psw 12 全部设置为 X

			//写入错误代码
			memset((data->Per_io_data->buffer) + 26, error, 1);

			uint8_t temp = CMD_FROM_WAN;
			memset((data->Per_io_data->buffer) + 27, temp, 1);

			temp_bool = socket_mngr::Put_WSASend(data->Sock, data->Per_io_data); //发送报告给php网站不成功

		}

	}

	return temp_bool;

}

void mac::Creat_cmd_header(char* buffer,uint16_t lenth)//创建命令头部
{

	ZeroMemory(buffer, lenth); //清空缓冲区

	socket_mngr::create_bytes(lenth,buffer);//将长度copy进缓冲
	memcpy(buffer + 2, this->mac_type, 4);//将类型copy进缓冲区
	socket_mngr::create_bytes(this->SN, buffer + 6);//将序列号copy进缓冲
	memcpy(buffer + 14, this->ctrl_psw, 12);//将命令口令copy进缓冲区
	buffer[27] = 1; //命令序号为1

}

void mac::Creat_Chk_time(char* buffer)
{

	this->Creat_cmd_header(buffer,58); //创建头部

	buffer[26] = uint8_t(0xfb);//命令代号

	time_t sever_t = _time64(NULL) + int(this->t_zone * 3600);//获取当前时间戳并依时区转换

	tm tstr = { 0 };

	gmtime_s(&tstr,&sever_t); //转化为时间

	buffer[28] = tstr.tm_year - 100;
	buffer[29] = tstr.tm_mon + 1;
	buffer[30] = tstr.tm_mday;
	buffer[31] = tstr.tm_hour;
	buffer[32] = tstr.tm_min;
	buffer[33] = tstr.tm_sec;
	buffer[34] = tstr.tm_wday;

	//填充
	for (int i = 35; i < 58; ++i)
	{

		buffer[i] = byte(0xff);

	}

}

void mac::exchange_buffer() 
{

	this->rpt->change_buffer_ptr(this->Recv_IO->buffer);

}