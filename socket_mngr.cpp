#include "stdafx.h"
#include "socket_mngr.h"

//ex������ָ��
LPFN_ACCEPTEX socket_mngr::m_lpfnAcceptEx = nullptr;
LPFN_DISCONNECTEX socket_mngr::m_lpfndisconnectEx = nullptr;
LPFN_CONNECTEX socket_mngr::m_lpfnconnectEx = nullptr;

void socket_mngr::get_ex_func_pointer(SOCKET sock)
{

	//��ȡAcceptEx����ָ��
	GUID GuidEx = WSAID_ACCEPTEX;        // GUID��ʶ��Ex���������  
	DWORD dwBytes = 0;

	int a = WSAIoctl(sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &GuidEx, sizeof(GuidEx), &socket_mngr::m_lpfnAcceptEx, sizeof(socket_mngr::m_lpfnAcceptEx), &dwBytes, NULL, NULL);

	//��ȡdisconnectex����ָ��
	GuidEx = WSAID_DISCONNECTEX;
	dwBytes = 0;
	
	int b = WSAIoctl(sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &GuidEx, sizeof(GuidEx), &socket_mngr::m_lpfndisconnectEx, sizeof(socket_mngr::m_lpfndisconnectEx), &dwBytes, NULL, NULL);

	//��ȡconnectex����ָ��
	GuidEx = WSAID_CONNECTEX;
	dwBytes = 0;

	int c = WSAIoctl(sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &GuidEx, sizeof(GuidEx), &socket_mngr::m_lpfnconnectEx, sizeof(socket_mngr::m_lpfnconnectEx), &dwBytes, NULL, NULL);

	if ((SOCKET_ERROR == a)||(SOCKET_ERROR == b)||(SOCKET_ERROR == c)) { //�����һ����ȡ���ɹ�
	
		cout << "Get EX_Func pointer failed!" << endl;
		system("pause");
		
	}

}

//resort������ָ�루���ڴ����Сͷ��
void(*socket_mngr::re_sort_bytes)(void* in, void* out, size_t lenth) = nullptr;

void socket_mngr::re_sort_bytes_big(void* in, void* out, size_t lenth) //big-endingʱ�����ź���
{

	for (uint16_t i = 0; i < lenth; ++i)
	{

		*((char*)out + lenth - 1 - i) = *((char*)in + i);

	}


}

void socket_mngr::re_sort_bytes_lit(void* in, void* out, size_t lenth) //little-endingʱ����Ϻ���
{

	memcpy(out,in, lenth);

}

void socket_mngr::get_resort_ptr()
{

	uint16_t x = 1;
	if (*(char *)&x == 1) //Сͷ
	{

		socket_mngr::re_sort_bytes = socket_mngr::re_sort_bytes_lit;

	}
	else
	{

		socket_mngr::re_sort_bytes = socket_mngr::re_sort_bytes_big;

	}

}

//socket���������
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

	memset((char *)&ServerAddress, 0, sizeof(ServerAddress)); //���ṹ����ռ�ڴ�ȫ����0

	ServerAddress.sin_family = AF_INET;//ѡ���ַ��(ipv4)

	uint32_t addrtemp;

	inet_pton(AF_INET, addrstr, &addrtemp);

	ServerAddress.sin_addr.s_addr = addrtemp;

	ServerAddress.sin_port = htons(port);//�˿�,ת���ֽ���

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

	// ��ʼ����

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

	setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, (char*)&bKeepAlive, sizeof(bKeepAlive));//�����������

	//�趨������ƵĲ���
	tcp_keepalive alive_in = { 0 };
	tcp_keepalive alive_out = { 0 };
	alive_in.keepalivetime = waittime;                // ��ʼ�״�KeepAlive̽��ǰ��TCP����ʱ��
	alive_in.keepaliveinterval = spacetime;                // ����KeepAlive̽����ʱ����
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
	//0Ϊ������1Ϊ������
	u_long mode = 1;

	if (block)
	{

		mode = 0;

	}

	ioctlsocket(sock, FIONBIO, &mode);

}

//Ͷ����ɶ˿�����
bool socket_mngr::Put_acceptex(SOCKET listened, _PER_USER_ACCEPT* data)
{

	bool temp = true;

	/*���accpetex����disconnect*/

	ZeroMemory((char *)&(data->m_Overlapped), sizeof(OVERLAPPED)); //��OVERLAPPED��0
	ZeroMemory(data->buffer, data->buffersize);//��������0
	data->IOtype = IOCP_ACCEPTEX;

	DWORD addrsbuffersize = (sizeof(sockaddr_in) + 16); //��ַ�ṹ��ĳ���
	DWORD lpdwBytesReceived = 0; //���������ݣ�ֱ�ӷ���

	if (!socket_mngr::m_lpfnAcceptEx(listened, ((_PER_NOMA_HANDLE*)(data->per_handle))->Sock, data->buffer, 0, addrsbuffersize, addrsbuffersize, &lpdwBytesReceived, &(data->m_Overlapped)))
	{

		int Error = WSAGetLastError();

		if (Error == WSAEFAULT) //����׽�����Ч���ؽ��׽���
		{

			((_PER_NOMA_HANDLE*)(data->per_handle))->Sock = WSASocketW(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);//�ؽ��׽���
			socket_mngr::Sock_FIONBIO_Set(((_PER_NOMA_HANDLE*)(data->per_handle))->Sock, false); //�趨�׽���Ϊ������ģʽ

			if (!socket_mngr::m_lpfnAcceptEx(listened, ((_PER_NOMA_HANDLE*)(data->per_handle))->Sock, data->buffer, 0, addrsbuffersize, addrsbuffersize, &lpdwBytesReceived, &(data->m_Overlapped)))
			{

				if (ERROR_IO_PENDING != WSAGetLastError())//����ֵ������IO_PENDING
				{

					temp = false;

				}

			}

		}
		else if (ERROR_IO_PENDING != Error)//����ֵ������IO_PENDING
		{

			temp = false;

		}

	}

	return temp;

}

bool socket_mngr::Put_disconnectex(SOCKET sock, _PER_IO* per_io_data)//Ͷ��һ��disconnectex����,�������Ƿ�ɹ�
{

	/*disconnectex����ʧ�ܣ����ʧ�ܣ����Ǵ����д���*/
	per_io_data->IOtype = IOCP_DISCONNECTEX;
	ZeroMemory((char *)&(per_io_data->m_Overlapped), sizeof(OVERLAPPED)); //��OVERLAPPED��0
	socket_mngr::m_lpfndisconnectEx(sock, &(per_io_data->m_Overlapped), TF_REUSE_SOCKET, 0);

	return true;

}

bool socket_mngr::Put_WSARecv(SOCKET sock, _PER_IO* per_io_data)
{

	bool temp = true;
	/*��ʹ����0(���������)����ɶ˿�Ҳ�����֪ͨ�����Ե������̴߳�����,
	������ִ�������ɱ�־������֣�����false,������disconnectex*/
	ZeroMemory((char*)&(per_io_data->m_Overlapped), sizeof(OVERLAPPED)); //��OVERLAPPED��0
	ZeroMemory(per_io_data->buffer, per_io_data->buffersize);//��������0
	per_io_data->IOtype = IOCP_RECV;

	WSABUF buff = { 0 };
	buff.buf = per_io_data->buffer;
	buff.len = per_io_data->buffersize;

	if (SOCKET_ERROR == WSARecv(sock, &buff, 1, &(per_io_data->MSG_lenth), &(per_io_data->Flag), &(per_io_data->m_Overlapped), NULL))//���û�������ɹ�
	{

		if (ERROR_IO_PENDING != WSAGetLastError())//����ֵ������IO_PENDING
		{

			temp = false;

		}

	}

	return temp;

}

bool socket_mngr::Put_WSASend(SOCKET sock, _PER_IO* per_io_data)
{

	bool temp = true;
	/*��ʹ����0(���������)����ɶ˿�Ҳ�����֪ͨ�����Ե������̴߳�����,
	������ִ�������ɱ�־������֣�����false,������disconnectex*/
	ZeroMemory((char*)&(per_io_data->m_Overlapped), sizeof(OVERLAPPED)); //��OVERLAPPED��0(�ṹ��ĵ�ַ�����׸���Ա�ĵ�ַ)
	per_io_data->IOtype = IOCP_SND;

	WSABUF buff = { 0 };
	buff.buf = per_io_data->buffer; //������ָ��
	buff.len = per_io_data->MSG_lenth; //��Ҫ���͵ĳ���

	DWORD snd_len = 0;

	if (SOCKET_ERROR == WSASend(sock, &buff, 1, &snd_len, per_io_data->Flag, &(per_io_data->m_Overlapped), NULL))//��������ɹ�
	{

		if (ERROR_IO_PENDING != WSAGetLastError())//����ֵ������IO_PENDING
		{

			temp = false;

		}

	}

	return temp;

}

//����/��������
socket_mngr::socket_mngr()
{

}

socket_mngr::~socket_mngr()
{

}

//��ɶ˿�ʹ�õĽṹ��
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

	buffer = (char*)malloc(buffer_size); //����ռ�
	memset(buffer, 0 , buffer_size);

}

_PER_IO::~_PER_IO()
{

	free((void*)buffer);//�ͷŻ������ռ�
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
	cmdrecvedlen = 0;//�Ѿ����յ��ĳ���

}

_PER_NOMA_HANDLE::~_PER_NOMA_HANDLE()
{

	free((void*)cmdBuffer);//�ͷŻ������ռ�
	delete Per_io_data;
	delete per_accept;

}