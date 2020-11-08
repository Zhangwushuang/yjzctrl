#pragma once
#include "socket_mngr.h"
#include <WINDOWS.h>
#include <iostream>

using namespace std;

class config
{
public:

	/*****�����豸�������******/
	static char local_ip[20];//�豸����ʱ�����ı���IP
	static uint16_t se_port; //�豸����ʱ�����Ķ˿�

	static uint16_t maxthread; //Ϊ�����ӵ��豸������߳�����
	static uint16_t valthread; //���������ӵĻ������߳�����

	static uint32_t maxcnnt; //����������Ŀͻ�������
	static sockaddr_in mac_addr;//�豸���ӽ���ʱ�����ĵ�ַ

	/**********����PHP��վ�������**************/
	static char user_ip[20];//PHP��վ�����ı���IP
	static uint16_t user_port;//PHP��վ�����Ķ˿�
	static uint16_t userthreads;//����PHP��վ���߳�����

	/************���ӱ����������������******/
	static char Alert_ip[20];//����������ĵ�ַ
	static uint16_t Alert_port;//����������Ķ˿�
	static char Alert_host[50];//�����������������

	static uint16_t Alert_thread;//��������߳�����

	static uint32_t Alert_space;//���ξ�����ʱ����
	static uint32_t Alert_buffsize;//���ͻ���Ĵ�С

	static sockaddr_in alert_addr;//����������ĵ�ַ
	static uint32_t alert_time;//�쳣�����೤ʱ�䱨��

	/*******���ݿ��������**************/
	static char db_host[120]; //mysql��������  
	static uint16_t db_port;//mysql��ʹ�õĶ˿� 
	static char db_user[30];//���ݿ��¼�ʺ�  
	static char db_pwd[30];//����������
	static char db_database[30];//���ݿ���

	//****************************
	static bool showconfig();
	static bool loadconfig();

private:
	static const LPCTSTR inipath;

	//ȫ���Ǿ�̬�ģ�������ʵ����
	config();
	~config();
};

