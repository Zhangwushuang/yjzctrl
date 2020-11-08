#include "stdafx.h"
#include "mac_map.h"

//---------------------------------------------//
mac* bucket::find_mac(uint64_t mac_sn)//��ȡ����
{

	mac* tp = nullptr;

	this->locker.lock_shared();

	forward_list<mac*>::iterator p = this->mac_list.begin();//����һ��������,��ͷ����ʼ��

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

void bucket::add_mac(mac* new_mac)//�������
{

	this->locker.lock();

	this->mac_list.push_front(new_mac);

	this->locker.unlock();

}

uint32_t bucket::clean(mac_que* recycleBin) //����Ͱ
{
	
	uint64_t now_time = GetTickCount64();//���ڵ�ʱ��
	uint32_t dorped_mac = 0;//����

	mac* tp_mac = nullptr;//��ʱ����
	http_node* tp_node = nullptr;

	this->locker.lock();//����Ͱ

	forward_list<mac*>::iterator tp = this->mac_list.before_begin();//���ɾ��ʱ
	forward_list<mac*>::iterator p = this->mac_list.begin();//����һ��������,��ͷ����ʼ��

	while (p != this->mac_list.end())
	{

		tp_mac = *p;

		if (tp_mac == nullptr) { //���Ϊ��ָ��
		
			p = this->mac_list.erase_after(tp);//ɾ���˽ڵ�
			continue;
		
		}

		(tp_mac->locker).lock();//�����豸

		if (now_time - (tp_mac->updatatime) > 60000) { //�������һ������û���ϱ�����
		
			tp_node = http_mngr::pool->get();//�ӳ�����ȡ��һ��http����

			if (nullptr != tp_node) {//����ǿ�
			
				tp_mac->rpt->Creat_Alert_Http(tp_node->buffer, tp_node->real_lenth,"0");//����http�ַ�
				http_mngrs::alert_que->que->put(tp_node);//���뷢�Ͷ���
			
			}

			tp_mac->reset_mac();//�����豸
			recycleBin->put(*p);//���豸�Żس���(�Ѿ��ǿ�ָ����)

			p = this->mac_list.erase_after(tp);//ɾ���˽ڵ�
			
			++ dorped_mac;//����+1

		}
		else 
		{
		
			tp = p;
			++p;
		
		}

		(tp_mac->locker).unlock();//�����豸

	}

	this->locker.unlock();//����Ͱ
	
	return dorped_mac; 

}

bucket::bucket(){


}

bucket::~bucket() {


}

//-----------------------------------------------//
uint32_t mac_map::bucket_num = 0;//bucket������
bucket**  mac_map::buckets = nullptr;//bucket������

void mac_map::init_map(uint32_t num)
{

	mac_map::bucket_num = num; //��¼Ͱ����Ŀ
	mac_map::buckets = new bucket*[num];//����ռ�

	for (uint32_t i = 0; i < num; ++i) {
	
		buckets[i] = new bucket();
	
	}

	cout << "buckets number is " << num << endl;

}

mac* mac_map::find_mac(uint64_t key)
{

	uint32_t num_buck = mac_map::hash_func(key);//���㴦���ĸ�Ͱ��
	return (mac_map::buckets[num_buck])->find_mac(key);//��ȡ�豸ָ��

}

void mac_map::add_mac(mac* new_mac)
{

	uint32_t num_buck = mac_map::hash_func(new_mac->SN);//������Ҫ�����ĸ�Ͱ��
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