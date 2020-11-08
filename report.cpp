#include "stdafx.h"
#include "report.h"

report::report():cmd(REPORT_LENTH)
{

	this->reset_rpt();

}

report::~report()
{

}

void report::get_state()
{

	this->get_header();//分析头部
	socket_mngr::combine_bytes((void*)(this->buffer + 153), this->duration); //获取异常持续时间

	socket_mngr::combine_bytes((void*)(this->buffer + 28), this->year); //获取年
	socket_mngr::combine_bytes((void*)(this->buffer + 29), this->mouth); //获取月
	socket_mngr::combine_bytes((void*)(this->buffer + 30), this->date); //获取日
	socket_mngr::combine_bytes((void*)(this->buffer + 31), this->hour); //获取时
	socket_mngr::combine_bytes((void*)(this->buffer + 32), this->min); //获取分
	socket_mngr::combine_bytes((void*)(this->buffer + 33), this->sec); //获取秒
	socket_mngr::combine_bytes((void*)(this->buffer + 34), this->day); //获取星期

}

void report::reset_rpt()
{

	this->reset_cmd();
	this->duration = 0;

	this->year = 0; //年
	this->mouth = 0;//月
	this->date = 0; //日
	this->hour = 0; //时
	this->min = 0;  //分
	this->sec = 0;  //秒
	this->day = 0;  //星期

}

void report::Creat_Get_tail(char* buffer, char* on_line)
{

	//json格式：?time=20000101050203&duration=98001&on_line=2
	//cout << dec << this->duration << endl;

	char temp[12] = { 0 }; //数字转化为的字符串缓存 长整形最多10位，带上负号11个字符。
	char timestr[15] = { 0 }; //时间字符串

	int16_t year_cache = this->year + 2000;//计算年
	if (year_cache > 9999 || year_cache < -9999)
	{

		year_cache = 0;

	}
	sprintf_s(temp, 12, "%04d", year_cache); //将年转化为字符串
	strcat_s(timestr, 15, temp); //将年记录进字符串

	if (this->mouth > 12)
	{

		this->mouth = 0;

	}
	sprintf_s(temp, 12, "%02d", this->mouth); //将月转化为字符串
	strcat_s(timestr, 15, temp); //月年记录进字符串

	if (this->date > 31)
	{

		this->date = 0;

	}
	sprintf_s(temp, 12, "%02d", this->date); //将日转化为字符串
	strcat_s(timestr, 15, temp); //将日记录进字符串

	if (this->hour > 23)
	{

		this->hour = 0;

	}
	sprintf_s(temp, 12, "%02d", this->hour); //将时转化为字符串
	strcat_s(timestr, 15, temp); //将时记录进字符串

	if (this->min > 59)
	{

		this->min = 0;

	}
	sprintf_s(temp, 12, "%02d", this->min); //将分转化为字符串
	strcat_s(timestr, 15, temp); //将分记录进字符串

	//秒不大于59
	if (this->sec > 59)
	{

		this->sec = 0;

	}
	sprintf_s(temp, 12, "%02d", this->sec); //将秒转化为字符串
	strcat_s(timestr, 15, temp); //将秒记录进字符串

	memset((char*)buffer, 0, JSON_BUFFER_SIZE);//全部归0

	strcpy_s(buffer, JSON_BUFFER_SIZE, "?time=");//当前时间
	strcat_s(buffer, JSON_BUFFER_SIZE, timestr);
	strcat_s(buffer, JSON_BUFFER_SIZE, "&duration=");

	sprintf_s(temp, 12, "%d", this->duration);
	strcat_s(buffer, JSON_BUFFER_SIZE, temp);

	strcat_s(buffer, JSON_BUFFER_SIZE, "&on_line=");
	strcat_s(buffer, JSON_BUFFER_SIZE, on_line);

}

void report::Creat_Alert_Http(char* httpbuffer, size_t &real_len, char* on_line)
{

	memset(httpbuffer, 0, HTTP_BUFFER_SIZE);//缓存清空

	char lenthstr[22] = { 0 };//数字转化为字符串的缓存
	sprintf_s(lenthstr, 22, "%I64d", this->SN);//将SN转化为字符串

	//创建get请求尾巴
	char tail_buffer[JSON_BUFFER_SIZE] = { 0 };
	this->Creat_Get_tail(tail_buffer, on_line);

	strcpy_s(httpbuffer, HTTP_BUFFER_SIZE, "GET /alert_000B.php");
	strcat_s(httpbuffer, HTTP_BUFFER_SIZE, tail_buffer);//get请求尾巴
	strcat_s(httpbuffer, HTTP_BUFFER_SIZE, " HTTP/1.1\r\nConnection:Keep-Alive\r\nHost:");
	strcat_s(httpbuffer, HTTP_BUFFER_SIZE, config::Alert_host);//APIhost拷贝进字符串
	strcat_s(httpbuffer, HTTP_BUFFER_SIZE, "\r\nAccept:*/*\r\nMac-Type:");
	strcat_s(httpbuffer, HTTP_BUFFER_SIZE, this->type);
	strcat_s(httpbuffer, HTTP_BUFFER_SIZE, "\r\nMac-Sn:");
	strcat_s(httpbuffer, HTTP_BUFFER_SIZE, lenthstr);
	strcat_s(httpbuffer, HTTP_BUFFER_SIZE, "\r\n\r\n");

	real_len = strlen(httpbuffer); //http请求post数据后无回车

	//cout << "alert"<< real_len << endl;
}