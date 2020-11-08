#pragma once
#include <winsock2.h>
#include <Windows.h>
#include "mysql.h"
#include <iostream>
#include <forward_list>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "libmysql.lib") 

using namespace std;

class mysqlctrl
{
public:

	MYSQL* pmydata;
	bool db_connect();
	mysqlctrl(char* host, uint16_t port, char* user, char* pwd, char* database);
	~mysqlctrl();

private:
	char* const db_host; //mysql��������  
	uint16_t const db_port;//mysql��ʹ�õĶ˿� 
	char* const db_user;//���ݿ��¼�ʺ�  
	char* const db_pwd;//����������
	char* const db_database;//���ݿ���

};

