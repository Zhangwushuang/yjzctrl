#pragma once
#include "cmd.h"

#define NO_CMD_FROM 0x00
#define CMD_FROM_WAN 0x01
#define CMD_FROM_LAN 0x02

#define MAC_ON_CTRL uint8_t(0xFF)
#define MAC_OFF_LINE uint8_t(0xFE)
#define INVALID_CMD uint8_t(0xFD)

class report :
	public cmd
{
public:

	uint32_t duration; //异常持续时间（单位：秒）

	int8_t year; //年
	uint8_t mouth;//月
	uint8_t date; //日
	uint8_t hour; //时
	uint8_t min;  //分
	uint8_t sec;  //秒
	uint8_t day;  //星期

	report();
	~report();

	void get_state();//分析收到的数据
	void reset_rpt();//重置cmd

	void Creat_Alert_Http(char* httpbuffer, size_t &real_len, char* on_line = "1");

private:
	void Creat_Get_tail(char* buffer, char* on_line = "1");//创建jason字符串 1为在线 0为离线报告 2为连线报告
};

