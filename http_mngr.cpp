#include "stdafx.h"
#include "http_mngr.h"

http_node::http_node():buffer(new char[HTTP_BUFFER_SIZE])
{

	this->real_lenth = 0;
	memset(this->buffer, 0, HTTP_BUFFER_SIZE);//清零

}

http_node::~http_node() {

	delete this->buffer;

}

//-----------------------
http_node* http_que::get() {

	http_node*  temp = nullptr;

	this->locker.lock();

	if (this->http_list->empty()) {//如果为空且是池子,则直接new一个给出去

		if (this->is_pool) {

			temp = new (nothrow) http_node();
		
		}

	} else {//如果不空则从池子里拿一个出去
	
		temp = this->http_list->front();//获得第一个元素
		this->http_list->pop_front();//删除第一个元素
	
	}

	this->locker.unlock();

	return temp;

}

void http_que::put(http_node* &pbuffer)
{
	if (pbuffer != nullptr)
	{

		this->locker.lock();
		(this->http_list)->push_back(pbuffer); //将指针放进尾部
		this->locker.unlock();

		pbuffer = nullptr; //原指针归nullptr

	}

}

http_que::http_que(bool pool):is_pool(pool),http_list(new list<http_node*>)
{

}

http_que::~http_que()
{

	list<http_node*>::iterator it = this->http_list->begin();

	this->locker.lock();

	while (it != this->http_list->end()) {

		if (nullptr != *it) {
		
			delete *it;

		}

		++it;

	}

	delete this->http_list;

	this->locker.unlock();

}

//--------------------------
http_que* const http_mngr::pool(new http_que(true));

void http_mngr::send_http()
{

	int recv_len = 0;
	char revbuffer[API_RECV_LENTH];

	//从队列中取出需要发送的请求,循环发送
	while (true) {

		//Sleep(100);
		//循环接收，清空接收缓冲区
		do
		{

			recv_len = recv(this->sock, revbuffer, API_RECV_LENTH, 0);

		} while (recv_len > 0);
	
		http_node* temp = this->que->get();//从队列中取出一个要发送的请求
	
		if (temp == nullptr) {//如果为空指针
		
			break;
		
		}

		//cout << "flag111" << endl;

		if (temp->real_lenth == send(this->sock, temp->buffer, temp->real_lenth, NULL))//如果send成功
		{
			//cout << "flag222" << endl;
			http_mngr::pool->put(temp);//将发送过的放回池子

		}
		else 
		{
		//	cout << "flag333" << endl;
			this->que->put(temp);//将其放回等待队列
			this->create_sock_connect(); //重新连接

		}

	}

}

void http_mngr::create_sock_connect()
{

	if (this->sock != INVALID_SOCKET) //如果套接字已经存在
	{

		closesocket(this->sock); //关闭套接字

	}

	this->sock = WSASocketW(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED); //建立套接字
	socket_mngr::Sock_FIONBIO_Set(this->sock, true); //设置套接字为阻塞模式(连接时采用阻塞模式)

	setsockopt(this->sock, SOL_SOCKET, SO_SNDBUF, (char*)&(this->sock_snd_buff_size), sizeof(uint32_t));//设定发送缓冲区大小
	connect(this->sock, (sockaddr *)this->remote, sizeof(sockaddr));//连接服务器(同步)

	socket_mngr::Sock_FIONBIO_Set(sock, false); //设置套接字为非阻塞模式

}

http_mngr::http_mngr(sockaddr_in* remote_addr, uint32_t sock_snd_size) :
	remote(remote_addr),
	sock_snd_buff_size(sock_snd_size),
	que(new http_que())
{

	this->sock = INVALID_SOCKET;

}

http_mngr::~http_mngr()
{

	delete this->que;//删除队列

}

//-------------------------
http_mngr* http_mngrs::alert_que = nullptr;//警告队列
