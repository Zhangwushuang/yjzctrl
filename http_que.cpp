#include "stdafx.h"
#include "http_que.h"

http_node::http_node(uint16_t len):buffer(new char[len])
{

	this->real_lenth = 0;

}

http_node::~http_node() {

	delete this->buffer;

}

http_node* http_que::get() {

	http_node*  temp = nullptr;

	this->locker.lock();

	if (!(this->http_list->empty())){//���������ӳ�������һ����ȥ
	
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

http_que::http_que(uint16_t lenth, uint32_t num):len(lenth), http_list(new list<http_node*>)
{

	for (uint32_t i = 0; i < num; ++i) {
	
		(this->http_list)->push_back(new http_node(lenth));
	
	}

}

http_que::~http_que()
{

	list<http_node*>::iterator it = this->http_list->begin();

	while (it != this->http_list->end()) {
	
		delete *it;
	
	}

	delete this->http_list;

}
