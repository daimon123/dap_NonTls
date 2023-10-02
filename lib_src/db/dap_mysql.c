//
// Created by KimByoungGook on 2020-06-12.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "com/dap_com.h"
#include "db/dap_mysql.h"
#include "secure/dap_secure.h"

int 			g_nUtf8Flag = 0;
int 			g_nSafeFetchRow = 0;


// MySql 접속 및 DB 선택
_MYSQL_CONINFO* fdb_SqlDbConnect(char *host, int port, char *id , char *pwd, char *dbName, char *mySock)
{
    _MYSQL_CONINFO* stptrMycon = NULL;
    my_bool        bReconnect = 1;

    if(fcom_malloc((void**)&stptrMycon, sizeof(_MYSQL_CONINFO)) != 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed ");
        return NULL;
    }

    stptrMycon->connect = mysql_init((MYSQL*)NULL);

    /* Mysql 재 Connect 옵션 추가. 2020.10.20 */
    mysql_options(stptrMycon->connect, MYSQL_OPT_RECONNECT, &bReconnect);

    strcpy(stptrMycon->cpHost, host);
    stptrMycon->nPort = port;
    strcpy(stptrMycon->cpId, id);
    strcpy(stptrMycon->cpPwd, pwd);
    strcpy(stptrMycon->cpDbName, dbName);
    if(strlen(mySock) > 1)
        strcpy(stptrMycon->mysqlSock, mySock);

    if(strlen(mySock) > 1)
    {
        stptrMycon->connect = mysql_real_connect(stptrMycon->connect, host, id, pwd, dbName, port, (char*)mySock, CLIENT_MULTI_STATEMENTS);
    }
    else
    {
        stptrMycon->connect = mysql_real_connect(stptrMycon->connect, host, id, pwd, dbName, port, (char*)NULL, CLIENT_MULTI_STATEMENTS);
    }

    if( stptrMycon->connect == NULL)
    {
        stptrMycon->nErrCode = -1;
        strcpy(stptrMycon->cpErrMsg, "Conncet Fail");

        return stptrMycon;
    }

    return stptrMycon;
}


void fdb_SqlClose(_MYSQL_CONINFO* param_con)
{

    if(param_con)
    {
        if(param_con->result)
             mysql_free_result(param_con->result);
        mysql_close(param_con->connect);

        /*
         * 2019-06-20 17:43:00
         * valgrind에서 leak이 발생되어 주석처리
         */
        //con->connect = mysql_init((MYSQL*)NULL);

        free(param_con);
        param_con = NULL;
    }
    mysql_thread_end();
    mysql_library_end();


}

void fdb_SqlError(_MYSQL_CONINFO* con)
{
    con->nErrCode = mysql_errno(con->connect);
    strcpy(con->cpErrMsg, mysql_error(con->connect));
}

unsigned long fdb_SqlInsertId(_MYSQL_CONINFO* con)
{
    return mysql_insert_id(con->connect);
}
int fdb_GetError(char *cpErrorCode, char *cpErrorId, char *cpReason)
{
    char	sqlCode[5];
    char	sqlBuf[128];
    int		rowCnt;

    memset(sqlCode, 0x00, sizeof(sqlCode));
    memset(sqlBuf , 0x00, sizeof(sqlBuf));
    strcpy(sqlCode,	cpErrorCode);

    snprintf(sqlBuf, sizeof(sqlBuf),"select ltrim(rtrim(ERRORID)), ltrim(rtrim(CONTENT)) "
                    "from SYS_OMC_ERROR_TB "
                    "where CODE='%s'", sqlCode);

    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d)msg(%s)",
                g_stMyCon->nErrCode,
                g_stMyCon->cpErrMsg);

        return -1;
    }

    if(rowCnt > 0)
    {
        if(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            strcpy(cpErrorId,	g_stMyCon->row[0]);
            strcpy(cpReason,	g_stMyCon->row[1]);
        }
    }
    fdb_SqlFreeResult(g_stMyCon);

    return	1;
}
int fdb_SqlDbReconnect(_MYSQL_CONINFO* con)
{
    con->connect = mysql_init((MYSQL*)NULL);

    if(strlen(con->mysqlSock) > 1)
    {
        con->connect = mysql_real_connect(con->connect, con->cpHost, con->cpId,
                                      con->cpPwd, con->cpDbName, con->nPort, con->mysqlSock, 0);
    }
    else
    {
        con->connect = mysql_real_connect(con->connect, con->cpHost, con->cpId,
                                          con->cpPwd, con->cpDbName, con->nPort, (char*)NULL, 0);
    }

    if( con->connect == NULL)
    {
        con->nErrCode = -1;
        strcpy(con->cpErrMsg, "Conncet Fail");
        WRITE_CRITICAL(CATEGORY_DB,"Fail Re mysql_real_connect host(%s) id(%s) pwd(%s) dbname(%s) port(%d) mysqlSock(%s)",
                       con->cpHost,
                       con->cpId,
                       con->cpPwd,
                       con->cpDbName,
                       con->nPort,
                       con->mysqlSock
                       );

        return -1;
    }

    WRITE_INFO(CATEGORY_DB,"Succeed in reconnect db");

    return 0;
}
unsigned long fdb_SqlQuery(_MYSQL_CONINFO* con, char *query)
{
    if(con->connect == NULL)
    {
        if(con->connect == NULL)
            return -1;
    }

    if(mysql_query(con->connect, query) < 0)
    {
        fdb_SqlError(con);
        return -2;
    }

    if(mysql_errno(con->connect) == 2006)	// 접속 끊김
    {
        if(fdb_SqlDbReconnect(con) == 0)
        {
            if(mysql_query(con->connect, query) < 0)
            {
                fdb_SqlError(con);
                return -3;
            }
        }
        else
        {
            WRITE_CRITICAL(CATEGORY_DB,"fdb_SqlDbReconnect Failed!! ");
            //sql_error(con); -> 접속이 끊긴상태이므로 에러코드 못가져온다.
            return -4;
        }
    }

    con->result = mysql_store_result(con->connect);

    if(con->result)	// 리턴된 row가 있다 : 즉 select 문 실행시
    {
        fdb_SqlError(con);
        return mysql_num_rows(con->result);
    }
    else			// 리턴된 row가 없다 : 즉 select 이외의 쿼리문 실행시
    {
        if( mysql_field_count(con->connect) == 0 )
        {
            fdb_SqlError(con);
            return mysql_affected_rows(con->connect);	// 최근쿼리의 영향 받은 row갯수
        }
        else		// 잘못된 쿼리 실행시 /
        {
            fdb_SqlError(con);
            return -5;
        }
    }
}

unsigned long fdb_SqlQuery2(_MYSQL_CONINFO* con, char *query)
{
    if(con->connect == NULL)
    {
        fdb_SqlDbReconnect(con);
        if(con->connect == NULL)
            return -1;
    }

    if(mysql_query(con->connect, query) < 0)
    {
        fdb_SqlError(con);
        return -2;
    }

    if(mysql_errno(con->connect) == 2006)	// 접속 끊김
    {
        if(fdb_SqlDbReconnect(con) == 0)
        {
            if(mysql_query(con->connect, query) < 0)
            {
                fdb_SqlError(con);
                return -3;
            }
        }
        else
        {
            //sql_error(con); -> 접속이 끊긴상태이므로 에러코드 못가져온다.
            return -4;
        }
    }

    if( mysql_field_count(con->connect) == 0 )
    {
        fdb_SqlError(con);

        return mysql_affected_rows(con->connect);	// 최근쿼리의 영향 받은 row갯수
    }
    else		// 잘못된 쿼리 실행시 /
    {
        fdb_SqlError(con);
        return -5;
    }
}
void fdb_SqlFreeResult(_MYSQL_CONINFO* con)
{
    if (con->result)
    {
        mysql_free_result(con->result);
        con->result = NULL;
    }
}

int fdb_SqlFetchRow(_MYSQL_CONINFO * con)
{
    con->row = mysql_fetch_row(con->result);

    if(con->row == NULL)
        return -1;
    else
        return 0;
}

int fdb_ConnectDbPool( _MYSQL_CONINFO** param_Conn, int param_ConnCount )
{
    char 	dbIp[15+1];
    char	dbId[64];
    char	dbPwd[64];
    char 	char_set[10];
    char 	mysql_sock[128];

    char	tmp_dbid[256];
    char	tmp_dbpwd[256];
    char	tmp_dbname[256];

    int     local_nLoop = 0;
    int 	dbPort = 0;
    int		rxt = 0;
    int		decrypto_size = 0;

    memset(dbIp, 0x00, sizeof(dbIp));
    memset(dbId, 0x00, sizeof(dbId));
    memset(dbPwd, 0x00, sizeof(dbPwd));
    memset(g_szDbName, 0x00, sizeof(g_szDbName));
    memset(mysql_sock, 0x00, sizeof(mysql_sock));
    memset(char_set, 0x00, sizeof(char_set));
    memset(dbId, 0x00, sizeof(dbId));
    memset(dbPwd, 0x00, sizeof(dbPwd));

    fcom_GetProfile("MYSQL","DB_IP",dbIp,"127.0.0.1");
    fcom_GetProfile("MYSQL","DB_ID",tmp_dbid,"master");
    fcom_GetProfile("MYSQL","DB_PASSWORD",tmp_dbpwd,"master");
    fcom_GetProfile("MYSQL","DB_NAME",tmp_dbname,"master");
    fcom_GetProfile("MYSQL","DB_SOCK",mysql_sock,"");
    fcom_GetProfile("MYSQL","DB_CHARSET",char_set,"utf8");

    dbPort = fcom_GetProfileInt("MYSQL","DB_PORT",3306);
    g_nSafeFetchRow = fcom_GetProfileInt("MYSQL","SAFE_FETCH_ROW",1000);

    fsec_DecryptStr((char*)&tmp_dbid, (char*)&dbId, &decrypto_size);
    fsec_DecryptStr((char*)&tmp_dbpwd, (char*)&dbPwd, &decrypto_size);
    fsec_DecryptStr((char*)&tmp_dbname, (char*)&g_szDbName, &decrypto_size);

    sleep(0);
    WRITE_DEBUG(CATEGORY_DEBUG,"DB Connection Info [%s][%d][%s][%s][%s]", dbIp, dbPort, dbId, dbPwd, g_szDbName);

    if (strcasecmp(char_set,"utf8") == 0)
        g_nUtf8Flag = 1;
    else
        g_nUtf8Flag = 0;

    for(local_nLoop = 0; local_nLoop < param_ConnCount; local_nLoop++)
    {
        if (strlen(mysql_sock) > 1)
        {
            param_Conn[local_nLoop] = fdb_SqlDbConnect(dbIp, dbPort, dbId, dbPwd, g_szDbName, mysql_sock);
            if (param_Conn[local_nLoop]->nErrCode != 0) {
                WRITE_CRITICAL(CATEGORY_DEBUG, "Fail in db connect, errcode(%d)msg(%s)",
                               param_Conn[local_nLoop]->nErrCode,
                               param_Conn[local_nLoop]->cpErrMsg);
                fdb_SqlClose(param_Conn[local_nLoop]);
                return -1;
            }
        }
        else
        {
            param_Conn[local_nLoop] = fdb_SqlDbConnect(dbIp, dbPort, dbId, dbPwd, g_szDbName, "");
            if(param_Conn[local_nLoop]->nErrCode != 0)
            {
                WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in db connect, errcode(%d)msg(%s)",
                               param_Conn[local_nLoop]->nErrCode,
                               param_Conn[local_nLoop]->cpErrMsg);
                fdb_SqlClose(param_Conn[local_nLoop]);
                return -1;
            }
        }

        if (strcmp(char_set,"none") != 0)
        {
            rxt = fdb_SetCharacterPool(param_Conn[local_nLoop] , char_set);
            if(rxt < 0)
            {
                return -1;
            }
        }
    }

    WRITE_INFO(CATEGORY_DEBUG,"Function End DB ");

    return 0;
}

int fdb_ConnectDB(void)
{
    char 	dbIp[15+1];
    char	dbId[64];
    char	dbPwd[64];
    char 	char_set[10];
    char 	mysql_sock[128];

    char	tmp_dbid[256];
    char	tmp_dbpwd[256];
    char	tmp_dbname[256];

    int 	dbPort;
    int		rxt;
    int		decrypto_size;

    memset(dbIp, 0x00, sizeof(dbIp));
    memset(dbId, 0x00, sizeof(dbId));
    memset(dbPwd, 0x00, sizeof(dbPwd));
    memset(g_szDbName, 0x00, sizeof(g_szDbName));
    memset(mysql_sock, 0x00, sizeof(mysql_sock));
    memset(char_set, 0x00, sizeof(char_set));
    memset(dbId, 0x00, sizeof(dbId));
    memset(dbPwd, 0x00, sizeof(dbPwd));

    fcom_GetProfile("MYSQL","DB_IP",dbIp,"127.0.0.1");
    fcom_GetProfile("MYSQL","DB_ID",tmp_dbid,"master");
    fcom_GetProfile("MYSQL","DB_PASSWORD",tmp_dbpwd,"master");
    fcom_GetProfile("MYSQL","DB_NAME",tmp_dbname,"master");
    fcom_GetProfile("MYSQL","DB_SOCK",mysql_sock,"");
    fcom_GetProfile("MYSQL","DB_CHARSET",char_set,"utf8");

    dbPort = fcom_GetProfileInt("MYSQL","DB_PORT",3306);
    g_nSafeFetchRow = fcom_GetProfileInt("MYSQL","SAFE_FETCH_ROW",1000);

    fsec_DecryptStr((char*)&tmp_dbid, (char*)&dbId, &decrypto_size);
    fsec_DecryptStr((char*)&tmp_dbpwd, (char*)&dbPwd, &decrypto_size);
    fsec_DecryptStr((char*)&tmp_dbname, (char*)&g_szDbName, &decrypto_size);

    sleep(0);
    WRITE_DEBUG(CATEGORY_DEBUG,"DB Connection Info [%s][%d][%s][%s][%s]", dbIp, dbPort, dbId, dbPwd, g_szDbName);

    if (strcasecmp(char_set,"utf8") == 0)
        g_nUtf8Flag = 1;
    else
        g_nUtf8Flag = 0;

    if (strlen(mysql_sock) > 1)
    {
        g_stMyCon = fdb_SqlDbConnect(dbIp, dbPort, dbId, dbPwd, g_szDbName, mysql_sock);
        if(g_stMyCon->nErrCode != 0)
        {
            WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in db connect, errcode(%d)msg(%s)",
                    g_stMyCon->nErrCode,
                    g_stMyCon->cpErrMsg);
            fdb_SqlClose(g_stMyCon);
            return -1;
        }
    }
    else
    {
        g_stMyCon = fdb_SqlDbConnect(dbIp, dbPort, dbId, dbPwd, g_szDbName, "");
        if(g_stMyCon->nErrCode != 0)
        {
            WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in db connect, errcode(%d)msg(%s)",
                           g_stMyCon->nErrCode,
                           g_stMyCon->cpErrMsg);
            fdb_SqlClose(g_stMyCon);
            return -1;
        }
    }

    //IsActiveDB = 1;

    //MyIsam은 트랜잭션을 지원하지 않는다. 무조건 autocommit=true;
    //set_autocommit(0);

    if (strcmp(char_set,"none")!=0)
    {
//        rxt = fdb_SetCharacter(char_set);
        rxt = fdb_SetCharSet(g_stMyCon->connect, char_set);
        if(rxt < 0)
            return -1;
    }
    WRITE_INFO(CATEGORY_DEBUG,"Function End DB Handle : %d ",g_stMyCon);

    return 0;
}
int fdb_SetCharacter(char* param_StrCharSet)
{
    char	sqlBuf[32] = {0x00,};

    if (strlen(param_StrCharSet) < 1)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Invalid Charset : [%s]", param_StrCharSet);
        return -1;
    }


    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    sprintf	(sqlBuf, "SET NAMES %s", param_StrCharSet);
    WRITE_DEBUG(CATEGORY_DB,"Execute Encoding Chearset : [%s]", param_StrCharSet);

    fdb_SqlQuery2(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in set names, errcode(%d),msg(%s)",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);
    }

    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in set names, errcode(%d),msg(%s)",
                g_stMyCon->nErrCode,
                g_stMyCon->cpErrMsg);

        return -1;
    }

    return 0;
}

int fdb_SetCharSet(MYSQL* param_Mysql, char* param_CharName )
{
    int local_nRet = 0;
    const char* local_ptrRet = NULL;

    local_nRet = mysql_set_character_set(param_Mysql, param_CharName);
    if ( local_nRet != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Mysql Set Character Set(%s) Failed", param_CharName);
        return (-1);
    }

    local_ptrRet = mysql_character_set_name(param_Mysql);
    WRITE_DEBUG(CATEGORY_DB,"Mysql Set Character Set Success (%s) ", local_ptrRet);

    return 0;


}

int fdb_SetCharacterPool(_MYSQL_CONINFO* param_Conn , char* param_StrCharSet)
{
    char	sqlBuf[31 +1] ;

    if (strlen(param_StrCharSet) < 1)
        return -1;

    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    sprintf	(sqlBuf, "SET NAMES %s", param_StrCharSet);

    fdb_SqlQuery2(param_Conn, sqlBuf);
    if(param_Conn->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in set names, errcode(%d),msg(%s)",
                       param_Conn->nErrCode,
                       param_Conn->cpErrMsg);
        return -1;
    }

    return 0;
}

int fdb_SqlCommit(_MYSQL_CONINFO* con)
{
    if(con->connect == NULL)
    {
        fdb_SqlDbReconnect(con);
        if(con->connect == NULL)
            return -1;
    }

    if(mysql_query(con->connect, "commit") < 0)
    {
        fdb_SqlError(con);
        return -2;
    }

    return 0;
}

int fdb_RetryConnectDb(void)
{
    int rxt, count=0;
    int err_code = -1;

    while(err_code < 0)
    {
        if(count > 30) break;

        sleep(1);

        rxt = fdb_ConnectDB();
        if(rxt < 0)
        {
            WRITE_CRITICAL(CATEGORY_DB,"Fail in db connection");
            err_code = -1;
        }
        else
        {
            WRITE_INFO(CATEGORY_DB,"Succeed in db connection");
            err_code = 1;
        }
        count++;
    }

    return RET_SUCC;
}