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
	char mac_type[5];//设备类型

	char ctrl_psw[13];//口令(控制->设备)

	double t_zone;//设备所在时区

	char Api_id[50];//APIid

	//**************************************

	_PER_IO* const Recv_IO;//用于recv的单IO数据的指针

	//**************************************

	report* const rpt;//最后一次上报的地址

	uint64_t updatatime;//连接或最后一次获得数据的时间

	uint64_t last_alert_time;//最后一次发送警告时的时间

	bool first_rpt;//用于判断是否发送上线报告
	
	uint64_t heart_beat_time;//最后一次发送心跳包的时间

	shared_mutex locker;

	mac();
	~mac();


	void rpt2mac();//将报告里的信息更新至mac(用于连接时)

	//均非线程安全，必须加锁
	uint8_t get_infor_from_db(mysqlctrl* db);//从数据库读取信息(仅由主线程调用)，此时完成请求应已全部返回，指针也不存在于map中
	void exchange_buffer();//交换Recv_IO和rpt的buffer，非线程安全
	bool chk_report();//比较report和mac是否相符，非线程安全
	void reset_mac(); //非线程安全,recv返回失败时调用此方法
	static bool reply_error(_PER_NOMA_HANDLE* data, uint8_t error); //提示设备掉线，或者命令无法解析
	void Creat_Chk_time(char* buffer);//创建对时命令

private:
	void Creat_cmd_header(char* buffer, uint16_t lenth);//创建命令头部

};