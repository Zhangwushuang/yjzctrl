#pragma once
#include <mutex>
#include <list>

using namespace std;

class http_node
{
public:
	char* const buffer;
	uint16_t real_lenth;

	http_node(uint16_t len);
	~http_node();

};

class http_que
{
public:

	http_node* get(); //��ȡһ��http������
	void put(http_node* &pbuffer); //��http��������������

	const uint16_t len;//ÿ��http�������ĳ���

	http_que(uint16_t lenth,uint32_t num = 0);
	~http_que();

private:
	list<http_node*>* const http_list;
	mutex locker;

};

