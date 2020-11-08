#include "stdafx.h"
#include "cmd.h"


cmd::cmd(uint16_t buffer_len):buffer_lenth(buffer_len)
{

	this->buffer = (char*)malloc(buffer_len);
	this->reset_cmd();

}

cmd::~cmd()
{

	free(this->buffer);
	this->buffer = nullptr;

}

void cmd::change_buffer_ptr(char* &buffer_input)
{

	char* temp = this->buffer;
	this->buffer = buffer_input;
	buffer_input = temp;

}

void cmd::get_header()
{
	this->reset_cmd();

	socket_mngr::combine_bytes(this->buffer,this->lenth);//获取传送的长度

	memcpy((void*)this->type, (void*)(this->buffer + 2), 4);//获取type

	socket_mngr::combine_bytes((void*)(this->buffer + 6), this->SN);//获取序列号

	memcpy((void*)this->psw, (void*)(this->buffer + 14), 12);

	this->cmdcode = *((uint8_t*)(this->buffer + 26));
	this->cmdno = *((uint8_t*)(this->buffer + 27));

}

void cmd::reset_cmd()
{

	this->reset_header();
	this->lenth = 0;
	this->cmdcode = 0;
	this->cmdno = 0;
	memset(this->type, 0, 4);

}

bool cmd::cmd_correct()
{

	if (this->cmdcode == 0x00) 
	{

		return false;

	}

	if (this->lenth == CMD_LENTH)
	{

		return true;

	}

	return false;

}