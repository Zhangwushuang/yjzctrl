#pragma once
#include "header.h"

#define CMD_LENTH 58

class cmd :
	public header
{

public:

	char type[5];

	uint16_t lenth; //命令的长度
	int8_t cmdno; //命令的来源（标识命令的来源，或者响应的是命令是哪里来的）
	uint8_t cmdcode; //命令的代号（标识要干什么）

	char* buffer;//命令的缓存
	const uint16_t buffer_lenth; //缓存的长度

	void change_buffer_ptr(char* &buffer_input);//交换命令缓冲区
	void get_header();//分析头部
	void reset_cmd();//重置命令

	bool cmd_correct(); //检查命令的代码和长度是否能合上

	cmd(uint16_t buffer_len = MAX_CMD_LENTH);
	~cmd();
};

