//
// Created by KimByoungGook on 2020-06-12.
//

#ifndef DAP_MYSQL_H
#define DAP_MYSQL_H

#include "db/mysql/mysql.h"

typedef struct
{
    char		cpHost[100];
    int			nPort;
    char		cpId[50];
    char		cpPwd[50];
    char		cpDbName[50];
    char		mysqlSock[128+1];
    MYSQL		*connect;
    MYSQL_RES 	*result;
    MYSQL_FIELD	*field;
    MYSQL_ROW 	row;
    int			nErrCode;
    char		cpErrMsg[1024];
} _MYSQL_CONINFO;


_MYSQL_CONINFO *g_stMyCon;
_MYSQL_CONINFO *g_stMycon2;

char			g_szDbName[64];


_MYSQL_CONINFO* fdb_SqlDbConnect        (char* host, int port, char *id , char *pwd, char *dbName, char *mySock );
int             fdb_SetCharacter        (char* param_StrCharSet                                                 );
int             fdb_SetCharSet          (MYSQL* param_Mysql, char* param_CharName                               );
int             fdb_SetCharacterPool    (_MYSQL_CONINFO* param_Conn , char* param_StrCharSet);
int             fdb_GetError            (char* cpErrorCode, char *cpErrorId, char *cpReason                     );
int             fdb_SqlDbReconnect(_MYSQL_CONINFO* con);
int             fdb_CheckExistsTable    (char* p_histDBName, char *p_histTBName                                 );
int             fdb_RetryConnectDb      (void                                                                   );
int             fdb_ConnectDB           (void                                                                   );
void            fdb_SqlClose            (_MYSQL_CONINFO* param_con                                              );
int             fdb_SqlCommit           (_MYSQL_CONINFO* con                                                    );
void            fdb_SqlError            (_MYSQL_CONINFO* con                                                    );
unsigned long   fdb_SqlInsertId         (_MYSQL_CONINFO* con                                                    );
unsigned long   fdb_SqlQuery            (_MYSQL_CONINFO* con, char *query                                       );
unsigned long   fdb_SqlQuery2           (_MYSQL_CONINFO* con, char *query                                       );
void            fdb_SqlFreeResult       (_MYSQL_CONINFO* con                                                    );
int             fdb_SqlFetchRow         (_MYSQL_CONINFO* con                                                    );
int fdb_ConnectDbPool( _MYSQL_CONINFO** param_Conn, int param_ConnCount );
#endif //DAP_MYSQL_H
