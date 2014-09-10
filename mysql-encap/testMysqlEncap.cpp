#include <stdio.h>
#include <string.h>
#include <string>

#include "mysql_encap.h"

using namespace std;

int main(){
	MysqlEncap *sql_conn = new MysqlEncap;
	sql_conn->Connect("localhost","root", "110315");
	
	sql_conn->ExecuteQuery("select * from testdb.student;");
	printf("rows count:%d\n", sql_conn->GetQueryResultCount());
	while(sql_conn->FetchRow()){
		printf("%s,%s\n",sql_conn->GetField("sid"),sql_conn->GetField("name"));
	}
	
//	printf("-------------------------------\n");
//	sql_conn->StartTransaction();
//	
//	sql_conn->Execute("update testdb.student set name = 'chengshuguang' where name = 'testapi';");
//	sql_conn->Execute("insert into testdb.student values (19, 'waba19', 'test2.c');");
//	if (sql_conn->EndTransaction()) {
//		printf("return 1---commit\n");
//	} else {
//		printf("return 0---rollback\n");
//	}

//	printf("-------------------------------\n");
//	sql_conn->ExecuteQuery("select * from testdb.student;");
//	while(sql_conn->FetchRow()){
//		printf("%s,%s\n",sql_conn->GetField("sid"),sql_conn->GetField("name"));
//	}
	delete sql_conn;
	
	return 0;
}
