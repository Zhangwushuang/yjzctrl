#pragma once
#include <shared_mutex>
#include <atomic>
#include "header.h"
#include "report.h"
#include "mysqlctrl.h"
#include "http_mngr.h"

class mac :
	public header, public _PER_HANDLE
{
public:
	char mac_type[5];//�豸����

	char ctrl_psw[13];//����(����->�豸)

	double t_zone;//�豸����ʱ��

	char Api_id[50];//APIid

	//**************************************

	_PER_IO* const Recv_IO;//����recv�ĵ�IO���ݵ�ָ��

	//**************************************

	report* const rpt;//���һ���ϱ��ĵ�ַ

	uint64_t updatatime;//���ӻ����һ�λ�����ݵ�ʱ��

	uint64_t last_alert_time;//���һ�η��;���ʱ��ʱ��

	bool first_rpt;//�����ж��Ƿ������߱���
	
	uint64_t heart_beat_time;//���һ�η�����������ʱ��

	shared_mutex locker;

	mac();
	~mac();


	void rpt2mac();//�����������Ϣ������mac(��������ʱ)

	//�����̰߳�ȫ���������
	uint8_t get_infor_from_db(mysqlctrl* db);//�����ݿ��ȡ��Ϣ(�������̵߳���)����ʱ�������Ӧ��ȫ�����أ�ָ��Ҳ��������map��
	void exchange_buffer();//����Recv_IO��rpt��buffer�����̰߳�ȫ
	bool chk_report();//�Ƚ�report��mac�Ƿ���������̰߳�ȫ
	void reset_mac(); //���̰߳�ȫ,recv����ʧ��ʱ���ô˷���
	static bool reply_error(_PER_NOMA_HANDLE* data, uint8_t error); //��ʾ�豸���ߣ����������޷�����
	void Creat_Chk_time(char* buffer);//������ʱ����

private:
	void Creat_cmd_header(char* buffer, uint16_t lenth);//��������ͷ��

};