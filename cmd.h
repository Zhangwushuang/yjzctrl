#pragma once
#include "header.h"

#define CMD_LENTH 58

class cmd :
	public header
{

public:

	char type[5];

	uint16_t lenth; //����ĳ���
	int8_t cmdno; //�������Դ����ʶ�������Դ��������Ӧ�����������������ģ�
	uint8_t cmdcode; //����Ĵ��ţ���ʶҪ��ʲô��

	char* buffer;//����Ļ���
	const uint16_t buffer_lenth; //����ĳ���

	void change_buffer_ptr(char* &buffer_input);//�����������
	void get_header();//����ͷ��
	void reset_cmd();//��������

	bool cmd_correct(); //�������Ĵ���ͳ����Ƿ��ܺ���

	cmd(uint16_t buffer_len = MAX_CMD_LENTH);
	~cmd();
};

