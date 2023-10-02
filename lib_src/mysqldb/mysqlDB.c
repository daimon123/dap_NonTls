#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <mysqlDB.h>
#include "common.h"

/* 사용한 MySql function
mysql_init()			: mysql 초기화 함수 입니다.  return 값은 연결식별값 (MYSQL*)
						  실패하면 FALSE 가 리턴 됩니다.
mysql_real_connect()	: mysql 접속 함수 입니다.  mysql_real_connect() 함수만을 이용합니다. 
mysql_select_db()		: mysql DB 선택 함수입니다. 어떤 DB를 선택 할것인지 하는 것이죠
mysql_close()			: mysql을 서버와의 접속을 끊습니다.  
mysql_query()			: 쿼리를 실행 시킵니다.
mysql_fetch_row()		: Result Set 에서 하나의 로우를 배열로 가져 옵니다.
mysql_store_result()	: Result Set 을 저장 합니다.
mysql_free_reslut()		: Result Set 을 메모리에서 제거 합니다.
mysql_errno()			: 에러 번호를 리턴합니다.
mysql_error()			: 에러에 대한 설명을 리턴합니다. 
mysql_num_rows()		: 총 몇 개의 ROW가 있는지 리턴
mysql_field_count()		: 필드의 수를 리턴
mysql_affected_rows()	: query로 영향을 받은 ROW의 수를 리턴
mysql_insert_id()		: 최근 INSERT에서 만들어진 id (generated id)를 구한다
mysql_data_seek()		: 임의의 ROW에 접근을 하도록 하는 함수
mysql_fetch_fields()	: 배열 형대로 result의 필드에 대한 정보를 한꺼번에 리턴
mysql_ping()			: 연결 중인지를 리턴한다. 연결이 끊어 졌을 경우
						  다시 연결을 시도(client가 접속 종료하고 콜하면 무한루프에 빠짐)
*/

/*
쿼리한 필드들 나열 
MYSQL_RES *     STDCALL mysql_list_fields(MYSQL *mysql, const char *table, const char *wild);
*/
//extern int	UTF8;
extern char	debugName[10];
/*
Mysql_ConInfo* mysql_connect()
{
	int		dbPort;
    char    dbIp[15+1];
    char    dbId[20];
    char    dbPwd[20];
    char    dbName[20];
    char    char_set[10];
    char    mysql_sock[128];

	Mysql_ConInfo* myCon = NULL;

	//char mysql_id[100];
	//char mysql_pwd[100];
	//char mysql_db[100];

	char tmp_ip[100];	
	char tmp_id[100];
	char tmp_pwd[100];
	char tmp_db[100];

	GetProfile("MYSQL", "DB_IP",		dbIp,		"127.0.0.1");
	GetProfile("MYSQL", "DB_ID",		dbId,		"msgpgw");
	GetProfile("MYSQL", "DB_PWD",		dbPwd,		"msgskfo");
	GetProfile("MYSQL", "DB_NAME",		dbName,		"msgpgw");
	GetProfile("MYSQL", "DB_PORT",		dbPort,		"msgpgw");
    GetProfile("MYSQL", "DB_SOCK",      mysql_sock, "");
    GetProfile("MYSQL", "DB_CHARSET",   char_set,   "");
    dbPort = GetProfileInt("MYSQL", "DB_PORT", 3306);

    if (strcasecmp(char_set,"utf8") == 0) UTF8 = 1;
    else UTF8 = 0;

	
	//memset(mysql_id, 0x00, sizeof(mysql_id));
	//dataAESDecrypt(tmp_id, MGWPK, (char*)&mysql_id);
	
	//memset(mysql_pwd, 0x00, sizeof(mysql_pwd));
	//dataAESDecrypt(tmp_pwd, MGWPK, (char*)&mysql_pwd);
	
	//memset(mysql_db, 0x00, sizeof(mysql_db));
	//dataAESDecrypt(tmp_db, MGWPK, (char*)&mysql_db);


    if (strlen(mysql_sock) > 1)
    {
        myCon = sql_db_connect(dbIp, dbPort, dbId, dbPwd, dbName, mysql_sock);
        if(myCon->nErrCode != 0)
        {
            LogRet("%-8s|%-8s|Fail in db connect, errcode(%d)errmsg(%s)|%s\n", debugName,"ERROR",
                myCon->nErrCode,myCon->cpErrMsg,__func__);
            return myCon;
        }
    }
    else
    {
        myCon = sql_db_connect(dbIp, dbPort, dbId, dbPwd, dbName, (char *)NULL);
        if(myCon->nErrCode != 0)
        {
            LogRet("%-8s|%-8s|Fail in db connect, errcode(%d)errmsg(%s)|%s\n", debugName,"ERROR",
                myCon->nErrCode,myCon->cpErrMsg,__func__);
            return myCon;
        }
    }

	if (strcmp(char_set,"none")!=0) set_character(char_set);

	return myCon;
}
*/

// MySql 접속 해제
void sql_close(Mysql_ConInfo* con)
{
	if(con->result) mysql_free_result(con->result);
	mysql_close(con->connect);

	/*
	 * 2019-06-20 17:43:00
	 * valgrind에서 leak이 발생되어 주석처리 
	 */
	//con->connect = mysql_init((MYSQL*)NULL);

	if(con)	free(con);
}

// MySql 에러 메시지, 코드 저장
void sql_error(Mysql_ConInfo* con)
{
	con->nErrCode = mysql_errno(con->connect);
	strcpy(con->cpErrMsg, mysql_error(con->connect));
	//printf("Error=%d , %s\n", con->nErrCode, con->cpErrMsg);
}


// MySql 접속 및 DB 선택
Mysql_ConInfo* sql_db_connect(char *host, int port, char *id , char *pwd, char *dbName, char *mySock)
{
	Mysql_ConInfo* con = (Mysql_ConInfo*)malloc(sizeof(Mysql_ConInfo));
	memset(con, 0x00, sizeof(Mysql_ConInfo));

	con->connect = mysql_init((MYSQL*)NULL);
	strcpy(con->cpHost, host);
	con->nPort = port;
	strcpy(con->cpId, id);
	strcpy(con->cpPwd, pwd);
	strcpy(con->cpDbName, dbName);
	if(strlen(mySock) > 1) strcpy(con->mysqlSock, mySock);

	if(strlen(mySock) > 1) 
	{
		//con->connect = mysql_real_connect(con->connect, host, id, pwd, dbName, port, (char*)mySock, CLIENT_SSL);
		con->connect = mysql_real_connect(con->connect, host, id, pwd, dbName, port, (char*)mySock, 0);
	} 
	else 
	{
		//con->connect = mysql_real_connect(con->connect, host, id, pwd, dbName, port, (char*)NULL, CLIENT_SSL);
		con->connect = mysql_real_connect(con->connect, host, id, pwd, dbName, port, (char*)NULL, 0);
	}

	if( con->connect == NULL)
	{
		con->nErrCode = -1;
		strcpy(con->cpErrMsg, "Conncet Fail");

		return con;
	}

	return con;
}

// MySql DB 재 접속
int sql_db_reconnect(Mysql_ConInfo* con)
{
	con->connect = mysql_init((MYSQL*)NULL);
	
//LogRet("Mysql ReConnect1 id=[%s] pwd=[%s] db=[%s]\n", con->cpId, con->cpPwd, con->cpDbName);
	con->connect = mysql_real_connect(con->connect, con->cpHost, con->cpId,
						con->cpPwd, con->cpDbName, con->nPort, con->mysqlSock, 0);

	if( con->connect == NULL)
	{
		con->nErrCode = -1;
		strcpy(con->cpErrMsg, "Conncet Fail");
		
		return -1;
	}
	
	LogRet("%-8s|%-8s|Succeed in reconnect db|%s\n", debugName,"INFO",__func__);
	
	return 0;
}

// 쿼리의 영향을 받은 row갯수를 리턴한다. -1 : query fail , else : 영향받은갯수
unsigned long sql_query(Mysql_ConInfo* con, char *query)
{
	unsigned long cnt;
	if(con->connect == NULL)
	{
		sql_db_reconnect(con);
		if(con->connect == NULL)
			return -1;
	}

	if(mysql_query(con->connect, query) < 0)
	{
		sql_error(con);
		return -2;
	}
		
	if(mysql_errno(con->connect) == 2006)	// 접속 끊김
	{
		if(sql_db_reconnect(con) == 0)
		{
			if(mysql_query(con->connect, query) < 0)
			{
				sql_error(con);
				return -3;
			}
		}
		else
		{
			//sql_error(con); -> 접속이 끊긴상태이므로 에러코드 못가져온다.
			return -4;
		}
	}

	con->result = mysql_store_result(con->connect);

	if(con->result)	// 리턴된 row가 있다 : 즉 select 문 실행시
	{
		sql_error(con);
		return mysql_num_rows(con->result);
	}
	else			// 리턴된 row가 없다 : 즉 select 이외의 쿼리문 실행시
	{
		if( mysql_field_count(con->connect) == 0 )
		{
			sql_error(con);
			return mysql_affected_rows(con->connect);	// 최근쿼리의 영향 받은 row갯수
		}
		else		// 잘못된 쿼리 실행시 /
		{
			sql_error(con);
			return -5;
		}
	}
}

unsigned long sql_query2(Mysql_ConInfo* con, char *query)
{
	unsigned long cnt;
	if(con->connect == NULL)
	{
		sql_db_reconnect(con);
		if(con->connect == NULL)
			return -1;
	}

	if(mysql_query(con->connect, query) < 0)
	{
		sql_error(con);
		return -2;
	}
		
	if(mysql_errno(con->connect) == 2006)	// 접속 끊김
	{
		if(sql_db_reconnect(con) == 0)
		{
			if(mysql_query(con->connect, query) < 0)
			{
				sql_error(con);
				return -3;
			}
		}
		else
		{
			//sql_error(con); -> 접속이 끊긴상태이므로 에러코드 못가져온다.
			return -4;
		}
	}

	if( mysql_field_count(con->connect) == 0 ) // INSERT , UPDATE , DELETE
	{
		sql_error(con);
		//LogRet("==> mysql_affected_rows=[%d][%s]\n", mysql_affected_rows(con->connect),query);
		return mysql_affected_rows(con->connect);	// 최근쿼리의 영향 받은 row갯수
	}
	else		// 잘못된 쿼리 실행시 /
	{
		sql_error(con);
		return -5;
	}
}

unsigned long sql_lock(Mysql_ConInfo* con, char *query)
{
	unsigned long cnt;
	if(con->connect == NULL)
	{
		sql_db_reconnect(con);
		if(con->connect == NULL)
			return -1;
	}

	if(mysql_query(con->connect, query) < 0)
	{
		sql_error(con);
		return -2;
	}
		
	if(mysql_errno(con->connect) == 2006)	// 접속 끊김
	{
		if(sql_db_reconnect(con) == 0)
		{
			if(mysql_query(con->connect, query) < 0)
			{
				sql_error(con);
				return -3;
			}
		}
		else
		{
			//sql_error(con); -> 접속이 끊긴상태이므로 에러코드 못가져온다.
			return -4;
		}
	}
}

// 쿼리 결과의 필드 갯수
int sql_field_count(Mysql_ConInfo* con)
{
	return mysql_field_count(con->connect);
}

// 최근 INSERT에서 만들어진 id (generated id)를 구한다.
unsigned long sql_insert_id(Mysql_ConInfo* con)
{
	return mysql_insert_id(con->connect);
}

// 쿼리 결과(MYSQL_RES) free
void sql_free_result(Mysql_ConInfo* con)
{
	if (con->result) 
	{
		mysql_free_result(con->result);
		con->result = NULL;
	}
}

// 쿼리 결과값을 하나씩 배열로 가져옴
int sql_fetch_row(Mysql_ConInfo* con)
{
	con->row = mysql_fetch_row(con->result);

	if(con->row == NULL)
		return -1;
	else
		return 0;
}

// 쿼리 결과에서의 offset 이동
void sql_data_seek(Mysql_ConInfo* con, int offset)
{
	mysql_data_seek(con->result, offset);
}

// 필드 이름 리턴
char * sql_field_name(Mysql_ConInfo* con, int idx)
{
	if((idx + 1) > mysql_field_count(con->connect))
		return NULL;

	con->field = mysql_fetch_fields(con->result);
	return (char *)con->field[idx].name;
}

// 필드 값 리턴
char * sql_field_value(Mysql_ConInfo* con, int idx)
{
	if((idx + 1) > mysql_field_count(con->connect))
		return NULL;

	return (char *)con->row[idx];
}

// 서버연결상태 확인 - 서버가 접속을 끊었으면 : ping() 재 접속
int sql_ping(Mysql_ConInfo* con)
{
	int result;

	if(con->connect == NULL)
		return -1;
		
	result = mysql_ping(con->connect);
	
	if(result != 0)
		return -2;
	/*
	if(result == 0)
		return result;
	else
		return sql_db_reconnect(con);
	*/
}

int sql_commit(Mysql_ConInfo* con)
{
	if(con->connect == NULL)
	{
		sql_db_reconnect(con);
		if(con->connect == NULL)
			return -1;
	}

	if(mysql_query(con->connect, "commit") < 0)
	{
		sql_error(con);
		return -2;
	}
	
	return 0;	
}

void mysql_exam()
{
	Mysql_ConInfo* myCon;
	int cnt;
	int i;

	myCon = sql_db_connect("127.0.0.1", 3306, "sms" , "sms123", "sms", (char *)NULL);
	if(myCon->nErrCode != 0)
	{
		printf("ErrCode=[%d] ErrMsg=[%s]\n", myCon->nErrCode, myCon->cpErrMsg);
		sql_close(myCon);
		return;
	}


	cnt = sql_query(myCon, "insert into address values('aaaa', 'bbb', 1)");
	if(myCon->nErrCode != 0)
	{
		printf("ErrCode=[%d] ErrMsg=[%s]\n", myCon->nErrCode, myCon->cpErrMsg);
	}
	printf("insert row=[%d] field[%d]\n", cnt, sql_field_count(myCon));


	cnt = sql_query(myCon, "SELECT * from address");
	if(myCon->nErrCode != 0)
	{
		printf("ErrCode=[%d] ErrMsg=[%s]\n", myCon->nErrCode, myCon->cpErrMsg);
	}
	int fcnt = sql_field_count(myCon);
	printf("select row=[%d] field[%d]\n", cnt, fcnt);

	
	printf("field name : ");
	for(i = 0; i < fcnt; i++)
		printf("[%s] ", sql_field_name(myCon, i));
	printf("\n");
	
	while(sql_fetch_row(myCon) == 0)
	{
		printf("field value : ");
		for(i = 0; i < fcnt; i++)
			printf("[%s], ", sql_field_value(myCon, i));
		printf("\n");
	}


	cnt = sql_query(myCon, "delete from address");
	if(myCon->nErrCode != 0)
	{
		printf("ErrCode=[%d] ErrMsg=[%s]\n", myCon->nErrCode, myCon->cpErrMsg);
	}
	printf("delete row=[%d] field[%d]\n", cnt, sql_field_count(myCon));
	
	printf("pint[%d]\n", sql_ping(myCon));

	printf("MYSQL.unix_socket=[%s]\n", myCon->connect->host);
	sql_close(myCon);


	printf("MYSQL.unix_socket=[%s]\n", myCon->connect->host);

}
