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

	http_node* get(); //获取一个http缓冲区
	void put(http_node* &pbuffer); //将http缓冲区放入容器

	http_que(bool pool = false);//默认不是池子
	~http_que();

private:
	list<http_node*>* const http_list;
	mutex locker;
	const bool is_pool;//是否为池子(决定队列为空时get,是否new新的)

};

class http_mngr
{
public:

	static http_que* const pool;//http池子
	http_que* const que;//队列

	void create_sock_connect();//创建套接字并连接
	void send_http();//循环发送队列中报告，采用了异步模式

	http_mngr(sockaddr_in* remote_addr, uint32_t sock_snd_size);
	~http_mngr();

private:
	const sockaddr_in* const remote;//远程服务器地址结构体
	const uint32_t sock_snd_buff_size; //套接字的发送缓存的大小
	SOCKET sock;

};

struct http_mngrs {

	static http_mngr* alert_que;//警告队列

};