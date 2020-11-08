#include "stdafx.h"
#include "report.h"

report::report():cmd(REPORT_LENTH)
{

	this->reset_rpt();

}

report::~report()
{

}

void report::get_state()
{

	this->get_header();//����ͷ��
	socket_mngr::combine_bytes((void*)(this->buffer + 153), this->duration); //��ȡ�쳣����ʱ��

	socket_mngr::combine_bytes((void*)(this->buffer + 28), this->year); //��ȡ��
	socket_mngr::combine_bytes((void*)(this->buffer + 29), this->mouth); //��ȡ��
	socket_mngr::combine_bytes((void*)(this->buffer + 30), this->date); //��ȡ��
	socket_mngr::combine_bytes((void*)(this->buffer + 31), this->hour); //��ȡʱ
	socket_mngr::combine_bytes((void*)(this->buffer + 32), this->min); //��ȡ��
	socket_mngr::combine_bytes((void*)(this->buffer + 33), this->sec); //��ȡ��
	socket_mngr::combine_bytes((void*)(this->buffer + 34), this->day); //��ȡ����

}

void report::reset_rpt()
{

	this->reset_cmd();
	this->duration = 0;

	this->year = 0; //��
	this->mouth = 0;//��
	this->date = 0; //��
	this->hour = 0; //ʱ
	this->min = 0;  //��
	this->sec = 0;  //��
	this->day = 0;  //����

}

void report::Creat_Get_tail(char* buffer, char* on_line)
{

	//json��ʽ��?time=20000101050203&duration=98001&on_line=2
	//cout << dec << this->duration << endl;

	char temp[12] = { 0 }; //����ת��Ϊ���ַ������� ���������10λ�����ϸ���11���ַ���
	char timestr[15] = { 0 }; //ʱ���ַ���

	int16_t year_cache = this->year + 2000;//������
	if (year_cache > 9999 || year_cache < -9999)
	{

		year_cache = 0;

	}
	sprintf_s(temp, 12, "%04d", year_cache); //����ת��Ϊ�ַ���
	strcat_s(timestr, 15, temp); //�����¼���ַ���

	if (this->mouth > 12)
	{

		this->mouth = 0;

	}
	sprintf_s(temp, 12, "%02d", this->mouth); //����ת��Ϊ�ַ���
	strcat_s(timestr, 15, temp); //�����¼���ַ���

	if (this->date > 31)
	{

		this->date = 0;

	}
	sprintf_s(temp, 12, "%02d", this->date); //����ת��Ϊ�ַ���
	strcat_s(timestr, 15, temp); //���ռ�¼���ַ���

	if (this->hour > 23)
	{

		this->hour = 0;

	}
	sprintf_s(temp, 12, "%02d", this->hour); //��ʱת��Ϊ�ַ���
	strcat_s(timestr, 15, temp); //��ʱ��¼���ַ���

	if (this->min > 59)
	{

		this->min = 0;

	}
	sprintf_s(temp, 12, "%02d", this->min); //����ת��Ϊ�ַ���
	strcat_s(timestr, 15, temp); //���ּ�¼���ַ���

	//�벻����59
	if (this->sec > 59)
	{

		this->sec = 0;

	}
	sprintf_s(temp, 12, "%02d", this->sec); //����ת��Ϊ�ַ���
	strcat_s(timestr, 15, temp); //�����¼���ַ���

	memset((char*)buffer, 0, JSON_BUFFER_SIZE);//ȫ����0

	strcpy_s(buffer, JSON_BUFFER_SIZE, "?time=");//��ǰʱ��
	strcat_s(buffer, JSON_BUFFER_SIZE, timestr);
	strcat_s(buffer, JSON_BUFFER_SIZE, "&duration=");

	sprintf_s(temp, 12, "%d", this->duration);
	strcat_s(buffer, JSON_BUFFER_SIZE, temp);

	strcat_s(buffer, JSON_BUFFER_SIZE, "&on_line=");
	strcat_s(buffer, JSON_BUFFER_SIZE, on_line);

}

void report::Creat_Alert_Http(char* httpbuffer, size_t &real_len, char* on_line)
{

	memset(httpbuffer, 0, HTTP_BUFFER_SIZE);//�������

	char lenthstr[22] = { 0 };//����ת��Ϊ�ַ����Ļ���
	sprintf_s(lenthstr, 22, "%I64d", this->SN);//��SNת��Ϊ�ַ���

	//����get����β��
	char tail_buffer[JSON_BUFFER_SIZE] = { 0 };
	this->Creat_Get_tail(tail_buffer, on_line);

	strcpy_s(httpbuffer, HTTP_BUFFER_SIZE, "GET /alert_000B.php");
	strcat_s(httpbuffer, HTTP_BUFFER_SIZE, tail_buffer);//get����β��
	strcat_s(httpbuffer, HTTP_BUFFER_SIZE, " HTTP/1.1\r\nConnection:Keep-Alive\r\nHost:");
	strcat_s(httpbuffer, HTTP_BUFFER_SIZE, config::Alert_host);//APIhost�������ַ���
	strcat_s(httpbuffer, HTTP_BUFFER_SIZE, "\r\nAccept:*/*\r\nMac-Type:");
	strcat_s(httpbuffer, HTTP_BUFFER_SIZE, this->type);
	strcat_s(httpbuffer, HTTP_BUFFER_SIZE, "\r\nMac-Sn:");
	strcat_s(httpbuffer, HTTP_BUFFER_SIZE, lenthstr);
	strcat_s(httpbuffer, HTTP_BUFFER_SIZE, "\r\n\r\n");

	real_len = strlen(httpbuffer); //http����post���ݺ��޻س�

	//cout << "alert"<< real_len << endl;
}