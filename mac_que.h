#pragma once
#include <list>
#include <mutex>
#include "mac.h"
/*�����豸�أ��̰߳�ȫ�������п���Ҫ��ͷ����ӣ���ʹ��list����queue*/
class mac_que
{
public:
	mac* get(); //��ȡһ���豸����
	void put(mac* &pmachine); //���豸�����ָ���������
	void give_back(mac* &pmachine);//�黹
	void print_length();//����ߴ�
	void mass_add(uint32_t num);

	mac_que();
	~mac_que();

private:
	list<mac*>* const que;
	mutex locker;

};

