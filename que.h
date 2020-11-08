#pragma once
#include <list>
#include <mutex>

using namespace std;

template<class T>
class que
{
public:

	T get() {
	
	
	}; //获取一个设备对象

	void put(T new_ele) {//将设备对象的指针放入容器


	}; 

	bool empty() {//是否为空,线程安全（由主线程调用）
	
	
	};

	void size() {
	
	
	
	};

	que(uint32_t n = 0);
	~que();

private:
	list<T> que_list;
	mutex locker;

};

