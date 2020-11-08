#include "stdafx.h"
#include "http_mngr.h"

http_node::http_node():buffer(new char[HTTP_BUFFER_SIZE])
{

	this->real_lenth = 0;
	memset(this->buffer, 0, HTTP_BUFFER_SIZE);//����

}

http_node::~http_node() {

	delete this->buffer;

}

//-----------------------
http_node* http_que::get() {

	http_node*  temp = nullptr;

	this->locker.lock();

	if (this->http_list->empty()) {//���Ϊ�����ǳ���,��ֱ��newһ������ȥ

		if (this->is_pool) {

			temp = new (nothrow) http_node();
		
		}

	} else {//���������ӳ�������һ����ȥ
	
		temp = this->http_list->front();//��õ�һ��Ԫ��
		this->http_list->pop_front();//ɾ����һ��Ԫ��
	
	}

	this->locker.unlock();

	return temp;

}

void http_que::put(http_node* &pbuffer)
{
	if (pbuffer != nullptr)
	{

		this->locker.lock();
		(this->http_list)->push_back(pbuffer); //��ָ��Ž�β��
		this->locker.unlock();

		pbuffer = nullptr; //ԭָ���nullptr

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

	//�Ӷ�����ȡ����Ҫ���͵�����,ѭ������
	while (true) {

		//Sleep(100);
		//ѭ�����գ���ս��ջ�����
		do
		{

			recv_len = recv(this->sock, revbuffer, API_RECV_LENTH, 0);

		} while (recv_len > 0);
	
		http_node* temp = this->que->get();//�Ӷ�����ȡ��һ��Ҫ���͵�����
	
		if (temp == nullptr) {//���Ϊ��ָ��
		
			break;
		
		}

		//cout << "flag111" << endl;

		if (temp->real_lenth == send(this->sock, temp->buffer, temp->real_lenth, NULL))//���send�ɹ�
		{
			//cout << "flag222" << endl;
			http_mngr::pool->put(temp);//�����͹��ķŻس���

		}
		else 
		{
		//	cout << "flag333" << endl;
			this->que->put(temp);//����Żصȴ�����
			this->create_sock_connect(); //��������

		}

	}

}

void http_mngr::create_sock_connect()
{

	if (this->sock != INVALID_SOCKET) //����׽����Ѿ�����
	{

		closesocket(this->sock); //�ر��׽���

	}

	this->sock = WSASocketW(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED); //�����׽���
	socket_mngr::Sock_FIONBIO_Set(this->sock, true); //�����׽���Ϊ����ģʽ(����ʱ��������ģʽ)

	setsockopt(this->sock, SOL_SOCKET, SO_SNDBUF, (char*)&(this->sock_snd_buff_size), sizeof(uint32_t));//�趨���ͻ�������С
	connect(this->sock, (sockaddr *)this->remote, sizeof(sockaddr));//���ӷ�����(ͬ��)

	socket_mngr::Sock_FIONBIO_Set(sock, false); //�����׽���Ϊ������ģʽ

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

	delete this->que;//ɾ������

}

//-------------------------
http_mngr* http_mngrs::alert_que = nullptr;//�������
