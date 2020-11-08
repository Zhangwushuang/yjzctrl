#include "stdafx.h"
#include "config.h"

LPCTSTR const config::inipath(INI_PATH);
char config::local_ip[20] = {'\0'};//����IP
uint16_t config::se_port = 0; //��Ҫ�����Ķ˿�
uint16_t config::maxthread = 0; //Ϊ�����ӵĻ���������߳�����
uint16_t config::valthread = 0;
uint32_t config::maxcnnt = 0; //����������Ŀͻ�������
sockaddr_in config::mac_addr = { 0 };//�豸���ӽ���ʱ�����ĵ�ַ

char config::db_host[120] = { '\0' }; //mysql��������  
uint16_t config::db_port = 0;//mysql��ʹ�õĶ˿� 
char config::db_user[30] = { '\0' };//���ݿ��¼�ʺ�  
char config::db_pwd[30] = { '\0' };//����������
char config::db_database[30] = { '\0' };//���ݿ���

char config::user_ip[20] = { '\0' };//����IP
uint16_t config::user_port = 0;//�û������ӵĶ˿�
uint16_t config::userthreads = 0;//�����û��˵��߳�����

char config::Alert_ip[20] = { '\0' };//����������ĵ�ַ
uint16_t config::Alert_port = 0;//����������Ķ˿�
char config::Alert_host[50] = { '\0' };//�����������������
sockaddr_in config::alert_addr = { 0 };//����������ĵ�ַ
uint32_t config::Alert_buffsize = 0;
uint32_t config::alert_time = 0;

uint32_t config::Alert_space = 0;//���ξ�����ʱ����
uint16_t config::Alert_thread = 0;

config::config()
{



}

bool config::loadconfig()
{
	//��ȡ�����ļ�
	GetPrivateProfileString("Server", "Se_ip", "localhost", config::local_ip, 20, config::inipath);//����IP��ַ
	config::maxcnnt = GetPrivateProfileInt("Server", "maxcnnt", 10000, config::inipath); //����������Ŀͻ������� 
	config::se_port = GetPrivateProfileInt("Server", "Se_port", 11968, config::inipath); //��ȡ�����ļ���Ҫ�����Ķ˿�
	config::maxthread = GetPrivateProfileInt("Server", "maxthread", 1, config::inipath); //Ϊ�����ӵĻ���������߳�����
	config::valthread = GetPrivateProfileInt("Server", "validatethread", 1, config::inipath); //Ϊ�����ӵĻ���������߳�����

	socket_mngr::setaddrstru(config::local_ip, config::se_port, config::mac_addr);

	GetPrivateProfileString("Mysql","db_host", "localhost", config::db_host, 120, config::inipath);//mysql��������
	GetPrivateProfileString("Mysql","db_database", "yjzctrl", config::db_database, 30, config::inipath);//mysql���ݿ���
	GetPrivateProfileString("Mysql","db_user","yjzctrl", config::db_user, 30, config::inipath); //mysql��¼��
	GetPrivateProfileString("Mysql","db_pwd", "yjzctrl", config::db_pwd, 30, config::inipath); //mysql��¼����
	config::db_port = GetPrivateProfileInt("Mysql", "db_port", 3306, config::inipath); //mysql�˿ں�


	GetPrivateProfileString("user", "user_ip", "127.0.0.1", config::user_ip, 20, config::inipath); //�û�������IP
	config::user_port = GetPrivateProfileInt("user", "user_port", 11960, config::inipath); //�û������ӵĶ˿�
	config::userthreads = GetPrivateProfileInt("user", "userthreads", 1, config::inipath); //�����û����߳�����

	GetPrivateProfileString("ALERT", "Alert_ip", "not_set", config::Alert_ip, 20, config::inipath);
	GetPrivateProfileString("ALERT", "Alert_host", "not_set", config::Alert_host, 50, config::inipath);
	config::alert_time = (GetPrivateProfileInt("ALERT", "Alert_abtim", 300, config::inipath));
	config::Alert_port = GetPrivateProfileInt("ALERT", "Alert_port", 80, config::inipath);
	config::Alert_space = (GetPrivateProfileInt("ALERT", "Alert_space", 1800, config::inipath));
	config::Alert_thread = GetPrivateProfileInt("ALERT", "Alertthread", 1, config::inipath); //���;�����߳�����

	config::Alert_buffsize = 2048 * config::maxcnnt / 10 / config::maxthread / config::Alert_space * 180;

	socket_mngr::setaddrstru(config::Alert_ip, config::Alert_port, config::alert_addr);
	//ÿ���̸߳�����һ����Alert������������Ϣ���׽���
	//Ԥ����10%�Ļ�����Ҫ���;���
	//�����СΪ 180����Ҫ���͵��ֽ���

	return true;

}

bool config::showconfig()
{

	cout << "******************************" << endl;
	cout << " " << endl;

	cout << "ini_path:" << config::inipath << endl;

	cout << " " << endl;

	cout << "Localserver ip:" << config::local_ip << endl;
	cout << "Localserver port:" << config::se_port << endl;
	cout << "Max connect:" << config::maxcnnt << endl;
	cout << "Max thread:" << config::maxthread << endl;

	cout << " " << endl;

	cout << "Db host:" << config::db_host << endl;
	cout << "Db port:" << config::db_port << endl;
	cout << "Db user:" << config::db_user << endl;
	cout << "Db psw:" << config::db_pwd << endl;
	cout << "Db name:" << config::db_database << endl;

	cout << " " << endl;

	cout << "Users connet to IP:" << config::user_ip << endl;
	cout << "Users connet to port:" << config::user_port << endl;
	cout << "Max thread for users:" << config::userthreads << endl;

	cout << " " << endl;

	//cout << "Apihost:" << config::APIhost << endl;
	//cout << "Apiip:" << config::APIip << endl;
	//cout << "Api key:" << config::APIkey << endl;
	//cout << "Api port:" << config::api_port << endl;
	//cout << "Report timespace:" << config::timespace << endl;
	//cout << "Requststr:" << config::Requststr << endl;
	//cout << "Send_buffsize:" << config::API_buffsize << endl;
	//cout << "timechkspace:" << config::timechkspace << endl;
	//cout << " " << endl;

	cout << "Alert_ip:" << config::Alert_ip << endl;
	cout << "Alert_port:" << config::Alert_port << endl;
	cout << "Alert_host:" << config::Alert_host << endl;
	cout << "Alert_space:" << config::Alert_space << endl;
	cout << "Alert_abtime:" << config::alert_time << endl;

	cout << " " << endl;
	cout << "******************************" << endl;
	return true;

}

config::~config()
{
}
