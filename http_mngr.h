#pragma once
#include <mutex>
#include <list>
#include "config.h"
#define CNNT_TIMEOUT 30000

class http_node
{
public:
	char* const buffer;
	size_t real_lenth;

	http_node();
	~http_node();

};

class http_que
{
public:

	http_node* get(); //��ȡһ��http������
	void put(http_node* &pbuffer); //��http��������������

	http_que(bool pool = false);//Ĭ�ϲ��ǳ���
	~http_que();

private:
	list<http_node*>* const http_list;
	mutex locker;
	const bool is_pool;//�Ƿ�Ϊ����(��������Ϊ��ʱget,�Ƿ�new�µ�)

};

class http_mngr
{
public:

	static http_que* const pool;//http����
	http_que* const que;//����

	void create_sock_connect();//�����׽��ֲ�����
	void send_http();//ѭ�����Ͷ����б��棬�������첽ģʽ

	http_mngr(sockaddr_in* remote_addr, uint32_t sock_snd_size);
	~http_mngr();

private:
	const sockaddr_in* const remote;//Զ�̷�������ַ�ṹ��
	const uint32_t sock_snd_buff_size; //�׽��ֵķ��ͻ���Ĵ�С
	SOCKET sock;

};

struct http_mngrs {

	static http_mngr* alert_que;//�������

};