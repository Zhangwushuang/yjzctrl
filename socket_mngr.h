#pragma once
#include <iostream>
#include <winsock2.h>
#include <WS2tcpip.h>
#include <mswsock.h>
#include <MSTcpIP.h>
#include <atomic>
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "mswsock.lib")

/*��IOtype�ı�־*/
#define IOCP_ACCEPTEX 0x01
#define IOCP_SND 0x02
#define IOCP_RECV 0x03 
#define IOCP_DISCONNECTEX 0x04

//*****************������Ĵ�С******************
//�����豸�ĸ������С
#define REPORT_LENTH 206 //�豸�������ƶ˵ı��泤��(����ͷ)
#define MAX_CMD_LENTH 58 //���ƶ˷����豸������Ļ��峤��(����ͷ)
#define CMD_HEADER_LENTH 28 //����ͱ����ͷ�����ȣ���cmdcode��cmdno��

//����API��alert������(��Щ�׽������ӵ�Զ�̷�������ÿ���߳�һ��,�����첽��ʽ��������ɶ˿�)
#define JSON_BUFFER_SIZE 512 //����api��������Jason�ַ����Ļ����С
#define HTTP_BUFFER_SIZE 512 //����api��������http�ַ����Ļ����С
#define API_RECV_LENTH 8192 //��API���������ջظ��Ļ����С

//����php��վ�Ļ����С������������ acceptex -> recv -> send -> disconnectex��ѭ��
//ͬһʱ��ֻ��һ�������������ֻ��Ҫһ����IO����
#define SITE_BUFF_LENTH 206 //���ֵ�� REPORT_LENTH �� MAX_CMD_LENTH �нϴ���Ǹ�
//************************
//����
#define INI_PATH ".\\config.ini" //�����ļ�·��

using namespace std;

//���������
struct _PER_HANDLE { //��������ݵĻ��ṹ�壬Ҳ���ڼ���������վ������׽��֣���һ����

	SOCKET   Sock;

	sockaddr_in remote_Address; //Զ�˵�ַ(ע�⣺����listen���׽���ʱΪ�����ĵ�ַ�Ͷ˿�)

	_PER_HANDLE();
	~_PER_HANDLE();

};

//��IO����
struct _PER_IO {

	OVERLAPPED  m_Overlapped;          // ÿһ���ص�I/O���������Ҫ��һ��             

	uint8_t  IOtype;              // ��־����ص�I/O��������ʲô��

	const size_t buffersize; //�������ߴ�

	char*  buffer;    //IO�Ļ�����

	DWORD  MSG_lenth; //ʵ��Ҫ����/���յĳ��ȡ�

	DWORD  Flag;

	//û�б�Ҫreset

	_PER_IO(size_t buffer_size);

	~_PER_IO();

};

/************************���ӿ�����վ��IOCP���ýṹ��*************************/
//��IO����
struct _PER_USER_ACCEPT : _PER_IO { //listen��socket acceptexʱʹ�õĽṹ�壬�̳���_PER_IO�������̶���

	void* const per_handle; //���õ��׽��ֵĵ��������( _PER_NOMA_HANDLE)

	_PER_USER_ACCEPT(void* handle);
	~_PER_USER_ACCEPT();

};

struct _PER_NOMA_HANDLE : _PER_HANDLE { //��listen���׽�������ʹ�õĵ��������

	//��Щ�׽������̣�acceptex(��������) -> recv��������� -> do sth ->send ��������Ϣ�� -> disconnectex ���ر����ӣ�;
	//����ÿ��ֻͶ��һ��IO��������per_IO_dataֻ��Ҫһ����

	char* cmdBuffer;//���һ��δ�ܽ��յ�һ����������ѽ��յ��Ĳ��ֱ����ڴ�
	 
	uint16_t    cmdrecvedlen;//�Ѿ����յ��ĳ���

	_PER_IO* const Per_io_data;//�ڴ�socket�Ͻ���recv��send��disconnectex�����ĵ�IO���ݵ�ָ��

	_PER_USER_ACCEPT* const per_accept;//��socket������(acceptex)ʱ�ĵ�IO����

	_PER_NOMA_HANDLE();
	~_PER_NOMA_HANDLE();

};

/***********************�����豸��IOCP���ýṹ��**************************/
//�豸�ĵ�������ݾ��� class mac

class socket_mngr
{

public:

	//ex����ָ��
	static LPFN_ACCEPTEX m_lpfnAcceptEx;//acceptex�ĺ���ָ��
	static LPFN_DISCONNECTEX m_lpfndisconnectEx;//disconnectex�ĺ���ָ��
	static LPFN_CONNECTEX m_lpfnconnectEx;//connectex�ĺ���ָ��

	static void get_ex_func_pointer(SOCKET sock);//��ȡ����ex������ָ��

	//��Сͷ��غ�������Ƭ��һ��ʹ��Сͷ��
	static void get_resort_ptr();//�жϷ�������Сͷ����ȡָ��
	template<typename COMBINE_TYPE>
	static void combine_bytes(void* buffer, COMBINE_TYPE &valueout)//��ϱ�����Ϊ����;
	{

		socket_mngr::re_sort_bytes(buffer, (void*)&valueout, sizeof(COMBINE_TYPE));

	};

	template<typename COMBINE_TYPE>
	static void create_bytes(COMBINE_TYPE valuein,void* buffer)//������ת��Ϊ������;
	{

		socket_mngr::re_sort_bytes((void*)&valuein, buffer, sizeof(COMBINE_TYPE));

	};

	//socket��غ���
	static uint8_t startwsock(WSADATA &wsaData); //��ʼ��socket
	static uint8_t setaddrstru(char* addrstr, uint16_t port, sockaddr_in &ServerAddress); //���õ�ַ�ṹ��
	static SOCKET create_listen_sock(sockaddr_in &ServerAddress, unsigned int maxwait = 5);//��ʼ������һ���׽���
	static bool set_keepalive(SOCKET sock, unsigned long waittime, unsigned long spacetime);//�������趨�������
	static void Sock_FIONBIO_Set(SOCKET sock, bool block);//�����׽����趨Ϊ����/������ģʽ,trueΪ����

	/*���acceptex��connectex��disconnectex������ֱ�ӹر��׽��֣�Ȼ����������һ����������Դ����*/
	static bool Put_acceptex(SOCKET listened, _PER_USER_ACCEPT* data);//Ͷ��һ��acceptex����,�������Ƿ�ɹ�
	static bool Put_disconnectex(SOCKET sock, _PER_IO* per_io_data);//Ͷ��һ��disconnectex����,�������Ƿ�ɹ�

	/*��ʹ����0(���������)����ɶ˿�Ҳ�����֪ͨ�����Ե������̴߳�����,
	������ִ�������ɱ�־������֣�����false,������disconnectex*/
	static bool Put_WSARecv(SOCKET sock, _PER_IO* per_io_data);//Ͷ��һ��wsarecv����,�������Ƿ�ɹ�
	static bool Put_WSASend(SOCKET sock, _PER_IO* per_io_data);//Ͷ��һ��wsasend����,�������Ƿ�ɹ�
	

//private:
	//�����������ڴ�Сͷ
	static void(*re_sort_bytes)(void* in, void* out, size_t lenth);//��������
	static void re_sort_bytes_lit(void* in, void* out, size_t lenth);//��������(Сͷʱ)
	static void re_sort_bytes_big(void* in, void* out, size_t lenth);//��������(��ͷʱ)

	//ȫ���Ǿ�̬�ģ���ֹʵ����
	socket_mngr();
	~socket_mngr();
};

