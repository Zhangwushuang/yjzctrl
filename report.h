#pragma once
#include "cmd.h"

#define NO_CMD_FROM 0x00
#define CMD_FROM_WAN 0x01
#define CMD_FROM_LAN 0x02

#define MAC_ON_CTRL uint8_t(0xFF)
#define MAC_OFF_LINE uint8_t(0xFE)
#define INVALID_CMD uint8_t(0xFD)

class report :
	public cmd
{
public:

	uint32_t duration; //�쳣����ʱ�䣨��λ���룩

	int8_t year; //��
	uint8_t mouth;//��
	uint8_t date; //��
	uint8_t hour; //ʱ
	uint8_t min;  //��
	uint8_t sec;  //��
	uint8_t day;  //����

	report();
	~report();

	void get_state();//�����յ�������
	void reset_rpt();//����cmd

	void Creat_Alert_Http(char* httpbuffer, size_t &real_len, char* on_line = "1");

private:
	void Creat_Get_tail(char* buffer, char* on_line = "1");//����jason�ַ��� 1Ϊ���� 0Ϊ���߱��� 2Ϊ���߱���
};

