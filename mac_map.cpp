#include "stdafx.h"
#include "mac_map.h"

//---------------------------------------------//
mac* bucket::find_mac(uint64_t mac_sn)//获取机器
{

	mac* tp = nullptr;

	this->locker.lock_shared();

	forward_list<mac*>::iterator p = this->mac_list.begin();//建立一个迭代器,从头部开始找

	while ( p != this->mac_list.end() )
	{
	
		if ((*p)->SN == mac_sn) {
		
			tp = *p;
			break;
		
		}
	
		++p;
	
	}

	this->locker.unlock_shared();

	return tp;

}

void bucket::add_mac(mac* new_mac)//放入机器
{

	this->locker.lock();

	this->mac_list.push_front(new_mac);

	this->locker.unlock();

}

uint32_t bucket::clean(mac_que* recycleBin) //清理桶
{
	
	uint64_t now_time = GetTickCount64();//现在的时间
	uint32_t dorped_mac = 0;//计数

	mac* tp_mac = nullptr;//临时对象
	http_node* tp_node = nullptr;

	this->locker.lock();//锁定桶

	forward_list<mac*>::iterator tp = this->mac_list.before_begin();//这个删除时
	forward_list<mac*>::iterator p = this->mac_list.begin();//建立一个迭代器,从头部开始找

	while (p != this->mac_list.end())
	{

		tp_mac = *p;

		if (tp_mac == nullptr) { //如果为空指针
		
			p = this->mac_list.erase_after(tp);//删除此节点
			continue;
		
		}

		(tp_mac->locker).lock();//锁定设备

		if (now_time - (tp_mac->updatatime) > 60000) { //如果发现一分钟内没有上报数据
		
			tp_node = http_mngr::pool->get();//从池子里取出一个http缓存

			if (nullptr != tp_node) {//如果非空
			
				tp_mac->rpt->Creat_Alert_Http(tp_node->buffer, tp_node->real_lenth,"0");//创建http字符
				http_mngrs::alert_que->que->put(tp_node);//放入发送队列
			
			}

			tp_mac->reset_mac();//重置设备
			recycleBin->put(*p);//将设备放回池子(已经是空指针了)

			p = this->mac_list.erase_after(tp);//删除此节点
			
			++ dorped_mac;//计数+1

		}
		else 
		{
		
			tp = p;
			++p;
		
		}

		(tp_mac->locker).unlock();//解锁设备

	}

	this->locker.unlock();//解锁桶
	
	return dorped_mac; 

}

bucket::bucket(){


}

bucket::~bucket() {


}

//-----------------------------------------------//
uint32_t mac_map::bucket_num = 0;//bucket的数量
bucket**  mac_map::buckets = nullptr;//bucket的数组

void mac_map::init_map(uint32_t num)
{

	mac_map::bucket_num = num; //记录桶的数目
	mac_map::buckets = new bucket*[num];//申请空间

	for (uint32_t i = 0; i < num; ++i) {
	
		buckets[i] = new bucket();
	
	}

	cout << "buckets number is " << num << endl;

}

mac* mac_map::find_mac(uint64_t key)
{

	uint32_t num_buck = mac_map::hash_func(key);//计算处于哪个桶内
	return (mac_map::buckets[num_buck])->find_mac(key);//获取设备指针

}

void mac_map::add_mac(mac* new_mac)
{

	uint32_t num_buck = mac_map::hash_func(new_mac->SN);//计算需要放入哪个桶内
	(mac_map::buckets[num_buck])->add_mac(new_mac);

}

void mac_map::clean_map(mac_que* recycleBin)
{

	uint32_t droped = 0;

	//cout << "Cleaning map..." << endl;

	for (uint32_t i = 0; i < mac_map::bucket_num; ++i) {

		droped = droped + mac_map::buckets[i]->clean(recycleBin);

	}

//	cout << dec << droped << " macs cleaned!" << endl;

}

uint32_t mac_map::hash_func(uint64_t sn) {

	return sn % mac_map::bucket_num;

}

mac_map::mac_map()
{
}

mac_map::~mac_map()
{
}