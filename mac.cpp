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

	memset((void*)this->mac_type, 0, 5);//���͹�0
	memcpy((void*)this->mac_type, (void*)(this->rpt->type), 4);//��ȡ����

	this->SN = this->rpt->SN;//д��SN

}

uint8_t mac::get_infor_from_db(mysqlctrl* db)
{

	if (0 != mysql_ping(db->pmydata))
	{

		return 2;//���ݿ�����ʧ��

	}

	char query[120] = { '\0' };
	memcpy((void*)query, "SELECT `mac_psw`, `ctrl_psw`, `Api_id`, `mac_type`, `t_zone` FROM `tbl_mac` WHERE `mac_id` = ", 93);

	sprintf_s(query + 93, 22, "%I64d", this->SN); //��SN�����ַ���

	size_t query_lenth = strlen(query); //���㳤��

	size_t searchres = mysql_real_query(db->pmydata, query, query_lenth);

	if (searchres != 0)
	{

		return 3;//������������

	}


	MYSQL_RES* results = mysql_store_result(db->pmydata);

	if (results == nullptr)
	{

		return 4;//��ȡ�����ʧ��

	}


	MYSQL_ROW row = mysql_fetch_row(results);

	if (row == NULL)
	{

		mysql_free_result(results);
		return 5;//�޽��	

	}

	memcpy((void*)this->psw, row[0], 12);

	memcpy((void*)this->ctrl_psw, row[1], 12);

	size_t api_id_len = strlen(row[2]);

	if (api_id_len > 49 || 0 != strcmp(this->mac_type,row[3])) //���
	{

		mysql_free_result(results);
		return 6;

	}

	memcpy((void*)this->Api_id, row[2], api_id_len);

	sscanf_s(row[4], "%lf", &(this->t_zone)); //��ȡʱ��

	mysql_free_result(results);
	return 0;//�����ɹ�

}

bool mac::chk_report()
{

	bool type_same = (0 == strcmp(this->mac_type, this->rpt->type)); //����������һ��
	bool sn_same = (this->SN == this->rpt->SN); //���к�һ��
	bool psw_same = (0 == strcmp(this->psw,this->rpt->psw)); //����һ��

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

	this->updatatime = 0;//���ӻ����һ�λ�����ݵ�ʱ��
	this->last_alert_time = 0;//���һ�η��;���ʱ���쳣״̬������ʱ��
	this->first_rpt = true;
	this->heart_beat_time = 0;
	this->t_zone = 0;//ʱ��

	if (this->Sock != INVALID_SOCKET) 
	{
	
		closesocket(this->Sock); //�ر��׽���
		this->Sock = INVALID_SOCKET;//���׽���ϵ��
	
	}

}

bool mac::reply_error(_PER_NOMA_HANDLE* data, uint8_t error)
{

	bool temp_bool = true;//Ĭ����վ��Ȼ����

	if (data != nullptr)
	{

		if (data->Sock != INVALID_SOCKET)
		{

			//��������type,�����Ի����á�
			data->Per_io_data->MSG_lenth = CMD_HEADER_LENTH;//����Ҫ���͵ĳ���Ϊ CMD_HEADER_LENTH ֻ����һ��ͷ��
			data->Per_io_data->Flag = 0;//���ñ�־λΪ0

			//���÷��͵�����
			memset(data->Per_io_data->buffer, 0, data->Per_io_data->buffersize); //per_io��buffȫ������

			uint16_t lenth = CMD_HEADER_LENTH;//д�볤������
			socket_mngr::create_bytes(lenth, data->Per_io_data->buffer);

			memcpy((data->Per_io_data->buffer) + 2, "0000", 4);//д��type
			memset((data->Per_io_data->buffer) + 6, 'X', 20);//SN 4 ��psw 12 ȫ������Ϊ X

			//д��������
			memset((data->Per_io_data->buffer) + 26, error, 1);

			uint8_t temp = CMD_FROM_WAN;
			memset((data->Per_io_data->buffer) + 27, temp, 1);

			temp_bool = socket_mngr::Put_WSASend(data->Sock, data->Per_io_data); //���ͱ����php��վ���ɹ�

		}

	}

	return temp_bool;

}

void mac::Creat_cmd_header(char* buffer,uint16_t lenth)//��������ͷ��
{

	ZeroMemory(buffer, lenth); //��ջ�����

	socket_mngr::create_bytes(lenth,buffer);//������copy������
	memcpy(buffer + 2, this->mac_type, 4);//������copy��������
	socket_mngr::create_bytes(this->SN, buffer + 6);//�����к�copy������
	memcpy(buffer + 14, this->ctrl_psw, 12);//���������copy��������
	buffer[27] = 1; //�������Ϊ1

}

void mac::Creat_Chk_time(char* buffer)
{

	this->Creat_cmd_header(buffer,58); //����ͷ��

	buffer[26] = uint8_t(0xfb);//�������

	time_t sever_t = _time64(NULL) + int(this->t_zone * 3600);//��ȡ��ǰʱ�������ʱ��ת��

	tm tstr = { 0 };

	gmtime_s(&tstr,&sever_t); //ת��Ϊʱ��

	buffer[28] = tstr.tm_year - 100;
	buffer[29] = tstr.tm_mon + 1;
	buffer[30] = tstr.tm_mday;
	buffer[31] = tstr.tm_hour;
	buffer[32] = tstr.tm_min;
	buffer[33] = tstr.tm_sec;
	buffer[34] = tstr.tm_wday;

	//���
	for (int i = 35; i < 58; ++i)
	{

		buffer[i] = byte(0xff);

	}

}

void mac::exchange_buffer() 
{

	this->rpt->change_buffer_ptr(this->Recv_IO->buffer);

}