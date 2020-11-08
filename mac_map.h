#pragma once
#include <shared_mutex>
#include <mutex>
#include <forward_list>
#include "mac_que.h"
#include "socket_mngr.h"
#include "http_mngr.h"

using namespace std;

/*map 说明*/
//采用静态数组实现,最初初始化时申请完空间即不再改动,所以无需加锁

//bucket采用list实现
//如需增删节点，必须持有独占锁。(add和clean)
//如只需读取内容，持有shared锁。
//无需修改节点的内容,因为新机器上线时添加于链表的头部,查找设备时也从头部开始,所以新的会覆盖旧的。

//四类线程需要访问共享的数据结构,其锁定关系分析如下

//连接设备的完成端口线程
//其它连接设备的线程，有可能通过完成端口获取了这个对象的指针(而不经过hash map)
//此线程只持有设备锁,故不会造成死锁.但须验证SN,以免脏读(获取指针之后,锁定之前已经被修改)

//用于连接PHP网站的线程
//需要从表中获取指定的设备对象的指针（只读）。
//其流程为 锁定桶-> 获取设备指针 -> 解锁桶 -> 锁定设备 ->操作 -> 解锁设备。
//因为总是只持有一把锁,故不会造成死锁
//连接PHP网站的线程，可能在该设备上次处于容器中时就获得了这个对象的指针(之后被clean清除掉,亦有可能被重用了)。
//连接PHP网站的线程不可能造成死锁，但接下来的操作需要验证一下SN，以免脏读。
//前三个步骤已被打包为 find_mac() 方法。 

//处理新设备连入的线程
//锁定设备->(投入完成端口,此时网站还找不到,但完成端口已经可以获到指针)->锁定桶(并先在链表的头部添加设备)->释放桶->释放设备

//清理线程(clean)
//clean时锁定顺序为 (锁定顺序为 锁定桶 -> 锁定桶中设备 -> 释放桶中设备-> 释放桶)

//放入之前由连入线程锁定的设备不存在于哈希表中,故清理线程锁定设备不可能和连入线程冲突.

//http池子及队列由于已经封装于方法内(锁定后立即会被释放,不会要求持有其他锁),故不会造成死锁
//设备池由于已经封装于方法内(锁定后立即会被释放,不会要求持有其他锁),故不会造成死锁

class bucket {

public:
	mac* find_mac(uint64_t mac_sn);//获取机器
	void add_mac(mac* new_mac);//放入机器

	uint32_t clean(mac_que* recycleBin); //清理桶

	bucket();
	~bucket();

private:
	shared_mutex locker;
	forward_list<mac*> mac_list;

};

class mac_map
{
public:

	static void init_map(uint32_t machinenum);//初始化(主线程使用)

	static mac* find_mac(uint64_t key);//找到机器（线程安全）
	static void add_mac(mac* new_mac);//添加机器(线程安全)

	static void clean_map(mac_que* recycleBin);//清理容器(线程安全)

private:
	static uint32_t bucket_num;//bucket的数量
	static bucket**  buckets;//bucket的数组

	static uint32_t hash_func(uint64_t sn);//哈希函数

	mac_map();
	~mac_map();
};
