#pragma once
#include <string>
#include "socket_mngr.h"
#include "config.h"

using namespace std;

class header
{
public:

	uint64_t SN;//序列号
	char psw[13];//口令(设备->控制)

protected:
	header();
	~header();

	void reset_header();
};

