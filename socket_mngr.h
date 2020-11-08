#pragma once
#include <iostream>
#include <winsock2.h>
#include <WS2tcpip.h>
#include <mswsock.h>
#include <MSTcpIP.h>
#include <atomic>
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "mswsock.lib")

/*各IOtype的标志*/
#define IOCP_ACCEPTEX 0x01
#define IOCP_SND 0x02
#define IOCP_RECV 0x03 
#define IOCP_DISCONNECTEX 0x04

//*****************各缓存的大小******************
//连接设备的各缓存大小
#define REPORT_LENTH 206 //设备发往控制端的报告长度(含包头)
#define MAX_CMD_LENTH 58 //控制端发往设备的命令的缓冲长度(含包头)
#define CMD_HEADER_LENTH 28 //命令和报告的头部长度（含cmdcode及cmdno）

//连接API与alert服务器(这些套接字连接到远程服务器，每个线程一个,采用异步方式，不用完成端口)
#define JSON_BUFFER_SIZE 512 //发往api服务器的Jason字符串的缓存大小
#define HTTP_BUFFER_SIZE 512 //发往api服务器的http字符串的缓存大小
#define API_RECV_LENTH 8192 //从API服务器接收回复的缓存大小

//连接php网站的缓存大小，由于它采用 acceptex -> recv -> send -> disconnectex的循环
//同一时间只有一个完成请求，所以只需要一个单IO数据
#define SITE_BUFF_LENTH 206 //这个值是 REPORT_LENTH 和 MAX_CMD_LENTH 中较大的那个
//************************
//其他
#define INI_PATH ".\\config.ini" //配置文件路径

using namespace std;

//单句柄数据
struct _PER_HANDLE { //单句柄数据的基结构体，也用于监听来自网站请求的套接字（仅一个）

	SOCKET   Sock;

	sockaddr_in remote_Address; //远端地址(注意：用于listen的套接字时为监听的地址和端口)

	_PER_HANDLE();
	~_PER_HANDLE();

};

//单IO数据
struct _PER_IO {

	OVERLAPPED  m_Overlapped;          // 每一个重叠I/O网络操作都要有一个             

	uint8_t  IOtype;              // 标志这个重叠I/O操作是做什么的

	const size_t buffersize; //缓冲区尺寸

	char*  buffer;    //IO的缓冲区

	DWORD  MSG_lenth; //实际要发送/接收的长度。

	DWORD  Flag;

	//没有必要reset

	_PER_IO(size_t buffer_size);

	~_PER_IO();

};

/************************连接控制网站的IOCP所用结构体*************************/
//单IO数据
struct _PER_USER_ACCEPT : _PER_IO { //listen的socket acceptex时使用的结构体，继承自_PER_IO（数量固定）

	void* const per_handle; //复用的套接字的单句柄数据( _PER_NOMA_HANDLE)

	_PER_USER_ACCEPT(void* handle);
	~_PER_USER_ACCEPT();

};

struct _PER_NOMA_HANDLE : _PER_HANDLE { //除listen的套接字以外使用的单句柄数据

	//这些套接字流程：acceptex(接受连接) -> recv（接受命令） -> do sth ->send （返回信息） -> disconnectex （关闭连接）;
	//由于每次只投递一个IO请求，所以per_IO_data只需要一个。

	char* cmdBuffer;//如果一次未能接收到一条完整命令，已接收到的部分保存在此
	 
	uint16_t    cmdrecvedlen;//已经接收到的长度

	_PER_IO* const Per_io_data;//在此socket上进行recv、send、disconnectex操作的单IO数据的指针

	_PER_USER_ACCEPT* const per_accept;//此socket被复用(acceptex)时的单IO数据

	_PER_NOMA_HANDLE();
	~_PER_NOMA_HANDLE();

};

/***********************连接设备的IOCP所用结构体**************************/
//设备的单句柄数据就是 class mac

class socket_mngr
{

public:

	//ex函数指针
	static LPFN_ACCEPTEX m_lpfnAcceptEx;//acceptex的函数指针
	static LPFN_DISCONNECTEX m_lpfndisconnectEx;//disconnectex的函数指针
	static LPFN_CONNECTEX m_lpfnconnectEx;//connectex的函数指针

	static void get_ex_func_pointer(SOCKET sock);//获取三个ex函数的指针

	//大小头相关函数（单片机一律使用小头）
	static void get_resort_ptr();//判断服务器大小头并获取指针
	template<typename COMBINE_TYPE>
	static void combine_bytes(void* buffer, COMBINE_TYPE &valueout)//组合比特流为数据;
	{

		socket_mngr::re_sort_bytes(buffer, (void*)&valueout, sizeof(COMBINE_TYPE));

	};

	template<typename COMBINE_TYPE>
	static void create_bytes(COMBINE_TYPE valuein,void* buffer)//将数据转化为比特流;
	{

		socket_mngr::re_sort_bytes((void*)&valuein, buffer, sizeof(COMBINE_TYPE));

	};

	//socket相关函数
	static uint8_t startwsock(WSADATA &wsaData); //初始化socket
	static uint8_t setaddrstru(char* addrstr, uint16_t port, sockaddr_in &ServerAddress); //设置地址结构体
	static SOCKET create_listen_sock(sockaddr_in &ServerAddress, unsigned int maxwait = 5);//开始并监听一个套接字
	static bool set_keepalive(SOCKET sock, unsigned long waittime, unsigned long spacetime);//开启并设定保活机制
	static void Sock_FIONBIO_Set(SOCKET sock, bool block);//设置套接字设定为阻塞/非阻塞模式,true为阻塞

	/*如果acceptex、connectex、disconnectex出错，则直接关闭套接字，然后重新申请一个，其他资源复用*/
	static bool Put_acceptex(SOCKET listened, _PER_USER_ACCEPT* data);//投递一个acceptex请求,并返回是否成功
	static bool Put_disconnectex(SOCKET sock, _PER_IO* per_io_data);//投递一个disconnectex请求,并返回是否成功

	/*即使返回0(即立即完成)，完成端口也会给出通知，所以到工作线程处理即可,
	如果出现错误，则完成标志不会出现，返回false,接下来disconnectex*/
	static bool Put_WSARecv(SOCKET sock, _PER_IO* per_io_data);//投递一个wsarecv请求,并返回是否成功
	static bool Put_WSASend(SOCKET sock, _PER_IO* per_io_data);//投递一个wsasend请求,并返回是否成功
	

//private:
	//重新排序，用于大小头
	static void(*re_sort_bytes)(void* in, void* out, size_t lenth);//重新排序
	static void re_sort_bytes_lit(void* in, void* out, size_t lenth);//重新排序(小头时)
	static void re_sort_bytes_big(void* in, void* out, size_t lenth);//重新排序(大头时)

	//全部是静态的，禁止实例化
	socket_mngr();
	~socket_mngr();
};

