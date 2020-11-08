#pragma once
#include <list>
#include <mutex>
#include "mac.h"
/*机器设备池，线程安全，由于有可能要在头部添加，故使用list而非queue*/
class mac_que
{
public:
	mac* get(); //获取一个设备对象
	void put(mac* &pmachine); //将设备对象的指针放入容器
	void give_back(mac* &pmachine);//归还
	void print_length();//输出尺寸
	void mass_add(uint32_t num);

	mac_que();
	~mac_que();

private:
	list<mac*>* const que;
	mutex locker;

};

