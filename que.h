#pragma once
#include <list>
#include <mutex>

using namespace std;

template<class T>
class que
{
public:

	T get() {
	
	
	}; //��ȡһ���豸����

	void put(T new_ele) {//���豸�����ָ���������


	}; 

	bool empty() {//�Ƿ�Ϊ��,�̰߳�ȫ�������̵߳��ã�
	
	
	};

	void size() {
	
	
	
	};

	que(uint32_t n = 0);
	~que();

private:
	list<T> que_list;
	mutex locker;

};

