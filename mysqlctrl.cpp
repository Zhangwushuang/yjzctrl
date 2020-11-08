#include "stdafx.h"
#include "mysqlctrl.h"


mysqlctrl::mysqlctrl(char* host, uint16_t port, char* user, char* pwd, char* database):
    db_host(host),
	db_port(port),
	db_user(user),
	db_pwd(pwd),
	db_database(database)
{

	cout << " " << endl;

	cout << "the db_host is " << this->db_host << endl;
	cout << "the db_port is " << this->db_port << endl;
	cout << "the db_database is " << this->db_database << endl;
	cout << "the db_user is " << this->db_user << endl;
	cout << "the db_pwd is " << this->db_pwd << endl;

	cout << " " << endl;

	if (0 == mysql_library_init(0, NULL, NULL)) {  //初始化数据库

		cout << "mysql_library_init() succeed" << endl;

	}
	else {

		cout << "mysql_library_init() failed" << endl;

	}

	this->pmydata = mysql_init(nullptr);

	if (NULL != this->pmydata) {

		cout << "mysql_init() succeed" << endl;

	}
	else {

		cout << "mysql_init() failed" << endl;

	}

}


mysqlctrl::~mysqlctrl()
{
}


bool mysqlctrl::db_connect()
{
	
	MYSQL* cnntres = mysql_real_connect(this->pmydata,this->db_host,this->db_user,this->db_pwd,this->db_database,this->db_port,NULL,0);

	if (cnntres == nullptr)
	{

		cout << "mysql connect failed：" << mysql_error(this->pmydata)<<endl;
		return false;

	}
	else
	{

		cout << "mysql connect success" << endl;
		return true;

	}

}
