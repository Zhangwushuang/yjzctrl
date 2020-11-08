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

	http_node* get(); //获取一个http缓冲区
	void put(http_node* &pbuffer); //将http缓冲区放入容器

	const uint16_t len;//每个http缓冲区的长度

	http_que(uint16_t lenth,uint32_t num = 0);
	~http_que();

private:
	list<http_node*>* const http_list;
	mutex locker;

};

