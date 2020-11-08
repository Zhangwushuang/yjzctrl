#pragma once
#include "socket_mngr.h"
#include <WINDOWS.h>
#include <iostream>

using namespace std;

class config
{
public:

	/*****连接设备相关配置******/
	static char local_ip[20];//设备连接时监听的本地IP
	static uint16_t se_port; //设备连接时监听的端口

	static uint16_t maxthread; //为已连接的设备服务的线程数量
	static uint16_t valthread; //处理新连接的机器的线程数量

	static uint32_t maxcnnt; //最大允许接入的客户机数量
	static sockaddr_in mac_addr;//设备连接进来时监听的地址

	/**********连接PHP网站相关配置**************/
	static char user_ip[20];//PHP网站监听的本地IP
	static uint16_t user_port;//PHP网站监听的端口
	static uint16_t userthreads;//服务PHP网站的线程数量

	/************连接报警服务器相关配置******/
	static char Alert_ip[20];//警告服务器的地址
	static uint16_t Alert_port;//警告服务器的端口
	static char Alert_host[50];//警告服务器的主机名

	static uint16_t Alert_thread;//处理警告的线程数量

	static uint32_t Alert_space;//两次警告间的时间间隔
	static uint32_t Alert_buffsize;//发送缓存的大小

	static sockaddr_in alert_addr;//警告服务器的地址
	static uint32_t alert_time;//异常持续多长时间报警

	/*******数据库相关配置**************/
	static char db_host[120]; //mysql服务器名  
	static uint16_t db_port;//mysql所使用的端口 
	static char db_user[30];//数据库登录帐号  
	static char db_pwd[30];//服务器密码
	static char db_database[30];//数据库名

	//****************************
	static bool showconfig();
	static bool loadconfig();

private:
	static const LPCTSTR inipath;

	//全部是静态的，不允许实例化
	config();
	~config();
};

