#include "stdafx.h"
#include "header.h"

header::header()
{

}


header::~header()
{
}

void header::reset_header()
{

	this->SN = 0;
	memset(this->psw, 0, 13);

}