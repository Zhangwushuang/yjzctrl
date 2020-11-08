#include "stdafx.h"
#include "socket_mngr.h"

//ex函数的指针
LPFN_ACCEPTEX socket_mngr::m_lpfnAcceptEx = nullptr;
LPFN_DISCONNECTEX socket_mngr::m_lpfndisconnectEx = nullptr;
LPFN_CONNECTEX socket_mngr::m_lpfnconnectEx = nullptr;

void socket_mngr::get_ex_func_pointer(SOCKET sock)
{

	//获取AcceptEx函数指针
	GUID GuidEx = WSAID_ACCEPTEX;        // GUID是识别Ex函数必须的  
	DWORD dwBytes = 0;

	int a = WSAIoctl(sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &GuidEx, sizeof(GuidEx), &socket_mngr::m_lpfnAcceptEx, sizeof(socket_mngr::m_lpfnAcceptEx), &dwBytes, NULL, NULL);

	//获取disconnectex函数指针
	GuidEx = WSAID_DISCONNECTEX;
	dwBytes = 0;
	
	int b = WSAIoctl(sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &GuidEx, sizeof(GuidEx), &socket_mngr::m_lpfndisconnectEx, sizeof(socket_mngr::m_lpfndisconnectEx), &dwBytes, NULL, NULL);

	//获取connectex函数指针
	GuidEx = WSAID_CONNECTEX;
	dwBytes = 0;

	int c = WSAIoctl(sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &GuidEx, sizeof(GuidEx), &socket_mngr::m_lpfnconnectEx, sizeof(socket_mngr::m_lpfnconnectEx), &dwBytes, NULL, NULL);

	if ((SOCKET_ERROR == a)||(SOCKET_ERROR == b)||(SOCKET_ERROR == c)) { //如果有一个获取不成功
	
		cout << "Get EX_Func pointer failed!" << endl;
		system("pause");
		
	}

}

//resort函数的指针（用于处理大小头）
void(*socket_mngr::re_sort_bytes)(void* in, void* out, size_t lenth) = nullptr;

void socket_mngr::re_sort_bytes_big(void* in, void* out, size_t lenth) //big-ending时的重排函数
{

	for (uint16_t i = 0; i < lenth; ++i)
	{

		*((char*)out + lenth - 1 - i) = *((char*)in + i);

	}


}

void socket_mngr::re_sort_bytes_lit(void* in, void* out, size_t lenth) //little-ending时的组合函数
{

	memcpy(out,in, lenth);

}

void socket_mngr::get_resort_ptr()
{

	uint16_t x = 1;
	if (*(char *)&x == 1) //小头
	{

		socket_mngr::re_sort_bytes = socket_mngr::re_sort_bytes_lit;

	}
	else
	{

		socket_mngr::re_sort_bytes = socket_mngr::re_sort_bytes_big;

	}

}

//socket的相关设置
uint8_t socket_mngr::startwsock(WSADATA &wsaData)
{
	int rc = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (rc != 0) {
		cout << "Failed to start winsock." << endl;
		return 1;
	}
	else
	{

		cout << "Init socket successed. " << endl;
		return 0;

	}
}

uint8_t socket_mngr::setaddrstru(char* addrstr, uint16_t port, sockaddr_in &ServerAddress)
{

	memset((char *)&ServerAddress, 0, sizeof(ServerAddress)); //将结构体所占内存全部置0

	ServerAddress.sin_family = AF_INET;//选择地址组(ipv4)

	uint32_t addrtemp;

	inet_pton(AF_INET, addrstr, &addrtemp);

	ServerAddress.sin_addr.s_addr = addrtemp;

	ServerAddress.sin_port = htons(port);//端口,转变字节序

	return 1;

}

SOCKET socket_mngr::create_listen_sock(sockaddr_in &ServerAddress, unsigned int maxwait)
{

	SOCKET sockListen = WSASocketW(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);

	if (sockListen == INVALID_SOCKET)
	{

		cout << "Create socket failed: " << dec << WSAGetLastError() << endl;
		system("pause");
		return INVALID_SOCKET;

	}

		cout << "Create socket successed. " << endl;

	if (0 != ::bind(sockListen, (LPSOCKADDR)&ServerAddress, sizeof(ServerAddress)))
	{

		cout << "bind socket failed: " << dec << WSAGetLastError() << endl;
		closesocket(sockListen);
		return INVALID_SOCKET;

	}

		cout << "bind socket successed. " << endl;

	// 开始监听

	if (0 != listen(sockListen, maxwait))
	{

		cout << "listen socket failed: " << dec << WSAGetLastError() << endl;
		closesocket(sockListen);
		return INVALID_SOCKET;

	}

		cout << "listen socket successed. " << endl;
		return sockListen;

}

bool socket_mngr::set_keepalive(SOCKET sock,unsigned long waittime, unsigned long spacetime)
{

	BOOL bKeepAlive = TRUE;

	setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (char*)&bKeepAlive, sizeof(bKeepAlive));//开启保活机制

	//设定保活机制的参数
	tcp_keepalive alive_in = { 0 };
	tcp_keepalive alive_out = { 0 };
	alive_in.keepalivetime = waittime;                // 开始首次KeepAlive探测前的TCP空闲时间
	alive_in.keepaliveinterval = spacetime;                // 两次KeepAlive探测间的时间间隔
	alive_in.onoff = TRUE;
	unsigned long ulBytesReturn = 0;

	BOOL nRet = WSAIoctl(sock, SIO_KEEPALIVE_VALS, &alive_in, sizeof(alive_in),
		&alive_out, sizeof(alive_out), &ulBytesReturn, NULL, NULL);

	if (nRet == SOCKET_ERROR)
	{
		return false;
	}
	else
	{
		return true;
	}


}

void socket_mngr::Sock_FIONBIO_Set(SOCKET sock,bool block)
{
	//0为阻塞，1为非阻塞
	u_long mode = 1;

	if (block)
	{

		mode = 0;

	}

	ioctlsocket(sock, FIONBIO, &mode);

}

//投递完成端口请求
bool socket_mngr::Put_acceptex(SOCKET listened, _PER_USER_ACCEPT* data)
{

	bool temp = true;

	/*如果accpetex出错，disconnect*/

	ZeroMemory((char *)&(data->m_Overlapped), sizeof(OVERLAPPED)); //将OVERLAPPED置0
	ZeroMemory(data->buffer, data->buffersize);//缓冲区置0
	data->IOtype = IOCP_ACCEPTEX;

	DWORD addrsbuffersize = (sizeof(sockaddr_in) + 16); //地址结构体的长度
	DWORD lpdwBytesReceived = 0; //不接收数据，直接返回

	if (!socket_mngr::m_lpfnAcceptEx(listened, ((_PER_NOMA_HANDLE*)(data->per_handle))->Sock, data->buffer, 0, addrsbuffersize, addrsbuffersize, &lpdwBytesReceived, &(data->m_Overlapped)))
	{

		int Error = WSAGetLastError();

		if (Error == WSAEFAULT) //如果套接字无效，重建套接字
		{

			((_PER_NOMA_HANDLE*)(data->per_handle))->Sock = WSASocketW(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);//重建套接字
			socket_mngr::Sock_FIONBIO_Set(((_PER_NOMA_HANDLE*)(data->per_handle))->Sock, false); //设定套接字为非阻塞模式

			if (!socket_mngr::m_lpfnAcceptEx(listened, ((_PER_NOMA_HANDLE*)(data->per_handle))->Sock, data->buffer, 0, addrsbuffersize, addrsbuffersize, &lpdwBytesReceived, &(data->m_Overlapped)))
			{

				if (ERROR_IO_PENDING != WSAGetLastError())//返回值不等于IO_PENDING
				{

					temp = false;

				}

			}

		}
		else if (ERROR_IO_PENDING != Error)//返回值不等于IO_PENDING
		{

			temp = false;

		}

	}

	return temp;

}

bool socket_mngr::Put_disconnectex(SOCKET sock, _PER_IO* per_io_data)//投递一个disconnectex请求,并返回是否成功
{

	/*disconnectex不会失败，如果失败，那是代码有错误*/
	per_io_data->IOtype = IOCP_DISCONNECTEX;
	ZeroMemory((char *)&(per_io_data->m_Overlapped), sizeof(OVERLAPPED)); //将OVERLAPPED置0
	socket_mngr::m_lpfndisconnectEx(sock, &(per_io_data->m_Overlapped), TF_REUSE_SOCKET, 0);

	return true;

}

bool socket_mngr::Put_WSARecv(SOCKET sock, _PER_IO* per_io_data)
{

	bool temp = true;
	/*即使返回0(即立即完成)，完成端口也会给出通知，所以到工作线程处理即可,
	如果出现错误，则完成标志不会出现，返回false,接下来disconnectex*/
	ZeroMemory((char*)&(per_io_data->m_Overlapped), sizeof(OVERLAPPED)); //将OVERLAPPED置0
	ZeroMemory(per_io_data->buffer, per_io_data->buffersize);//缓冲区置0
	per_io_data->IOtype = IOCP_RECV;

	WSABUF buff = { 0 };
	buff.buf = per_io_data->buffer;
	buff.len = per_io_data->buffersize;

	if (SOCKET_ERROR == WSARecv(sock, &buff, 1, &(per_io_data->MSG_lenth), &(per_io_data->Flag), &(per_io_data->m_Overlapped), NULL))//如果没有立即成功
	{

		if (ERROR_IO_PENDING != WSAGetLastError())//返回值不等于IO_PENDING
		{

			temp = false;

		}

	}

	return temp;

}

bool socket_mngr::Put_WSASend(SOCKET sock, _PER_IO* per_io_data)
{

	bool temp = true;
	/*即使返回0(即立即完成)，完成端口也会给出通知，所以到工作线程处理即可,
	如果出现错误，则完成标志不会出现，返回false,接下来disconnectex*/
	ZeroMemory((char*)&(per_io_data->m_Overlapped), sizeof(OVERLAPPED)); //将OVERLAPPED置0(结构体的地址就是首个成员的地址)
	per_io_data->IOtype = IOCP_SND;

	WSABUF buff = { 0 };
	buff.buf = per_io_data->buffer; //缓冲区指针
	buff.len = per_io_data->MSG_lenth; //需要发送的长度

	DWORD snd_len = 0;

	if (SOCKET_ERROR == WSASend(sock, &buff, 1, &snd_len, per_io_data->Flag, &(per_io_data->m_Overlapped), NULL))//如果立即成功
	{

		if (ERROR_IO_PENDING != WSAGetLastError())//返回值不等于IO_PENDING
		{

			temp = false;

		}

	}

	return temp;

}

//构造/析构函数
socket_mngr::socket_mngr()
{

}

socket_mngr::~socket_mngr()
{

}

//完成端口使用的结构体
_PER_HANDLE::_PER_HANDLE()
{

	Sock = INVALID_SOCKET;
	ZeroMemory((void*)&remote_Address, sizeof(remote_Address));

}

_PER_HANDLE::~_PER_HANDLE()
{

}

_PER_IO::_PER_IO(size_t buffer_size):buffersize(buffer_size)
{

	buffer = (char*)malloc(buffer_size); //申请空间
	memset(buffer, 0 , buffer_size);

}

_PER_IO::~_PER_IO()
{

	free((void*)buffer);//释放缓冲区空间
	buffer = nullptr;

}

_PER_USER_ACCEPT::_PER_USER_ACCEPT(void* handle):
	_PER_IO( 2 * (sizeof(sockaddr_in) + 16) + MAX_CMD_LENTH),
	per_handle(handle)
{

}

_PER_USER_ACCEPT::~_PER_USER_ACCEPT() 
{

}

_PER_NOMA_HANDLE::_PER_NOMA_HANDLE():
	Per_io_data(new _PER_IO(SITE_BUFF_LENTH)),
	per_accept(new _PER_USER_ACCEPT((void*)this))
{

	cmdBuffer = (char*)malloc(MAX_CMD_LENTH);
	memset(cmdBuffer, 0 , MAX_CMD_LENTH);
	Per_io_data->Flag = 0;
	cmdrecvedlen = 0;//已经接收到的长度

}

_PER_NOMA_HANDLE::~_PER_NOMA_HANDLE()
{

	free((void*)cmdBuffer);//释放缓冲区空间
	delete Per_io_data;
	delete per_accept;

}