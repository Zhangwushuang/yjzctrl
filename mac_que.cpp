#include "stdafx.h"
#include "mac_que.h"

mac* mac_que::get()
{

	mac*  temp = nullptr;

	this->locker.lock();

	if (!this->que->empty())
	{

		temp = this->que->front();//获得第一个元素
		this->que->pop_front();//删除第一个元素

	}

	this->locker.unlock();

	return temp;

}

void mac_que::mass_add(uint32_t num) {

	for (uint32_t i = 0; i < num; ++i)
	{

		this->que->push_back(new mac());//将新设备加入池子

	}

}

void mac_que::put(mac* &pmachine)
{

	if (pmachine != nullptr)
	{

		this->locker.lock();

		this->que->push_back(pmachine); //将指针放进尾部

		this->locker.unlock();
		pmachine = nullptr; //原指针归nullptr

	}

}

void mac_que::give_back(mac* &pmachine) {

	if (pmachine != nullptr)
	{

		this->locker.lock();
		this->que->push_front(pmachine); //将指针归还到头部
		this->locker.unlock();
		pmachine = nullptr; //原指针归nullptr

	}

}

void mac_que::print_length()
{

	this->locker.lock();

	cout << oct << this->que->size() << " devices in pool!" << endl;

	this->locker.unlock();

}

mac_que::mac_que():que(new list<mac*>)
{

}

mac_que::~mac_que()
{

	list<mac*>::iterator it = this->que->begin();

	this->locker.lock();

	while (it != this->que->end()) {

		if (nullptr != *it) {

			delete *it;

		}

		++it;

	}

	delete this->que;

	this->locker.unlock();

}