#pragma once
#include <string>
#include "socket_mngr.h"
#include "config.h"

using namespace std;

class header
{
public:

	uint64_t SN;//���к�
	char psw[13];//����(�豸->����)

protected:
	header();
	~header();

	void reset_header();
};

