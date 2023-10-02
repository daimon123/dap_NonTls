//
// Created by KimByoungGook on 2020-06-15.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "com/dap_def.h"
#include "com/dap_com.h"
#include "db/dap_checkdb.h"
#include "db/dap_mysql.h"


#include "secure/dap_secure.h"

/* Declare : dap_checkdb.h  */
/* 2020.10.08 -> CTRL_PROCESS_CPU_TB 추가. */
char g_exTbItem[62][TABLE_LENGTH] =
{
        "BLUETOOTH", "CONFIG", "CPU", "DETECT_PRINTER_PORT", "DETECT_PROCESS",                  // 5
        "DISK_REG_LINK", "DISK", "EVENT", "HW_BASE", "INFRARED_DEVICE",                         // 10
        "MANAGER_EVENT", "MANAGER", "NET_ADAPTER", "NET_CONNECTION", "NET_DRIVE",               // 15
        "NET_PRINTER", "NET_SCAN", "OS_ACCOUNT", "OS", "PROCESS",                               // 20
        "ROUTER", "RULE", "RULE_EXCEPT", "SHARE_FOLDER",                                        // 24
        "SYSTEM", "SYS_ALARM", "SYS_CONFIG_CHANGE", "SYS_FILESYSTEM", "SYS_OMC_ERRORLOG",       // 29
        "SYS_OMC_ERROR", "SYS_PINFO", "SYS_QUEUE", "SYS_SYSTEM", "SYS_THRESHOLD",               // 34
        "USER_GROUP_LINK", "USER_GROUP", "USER_LINK", "USER", "WATCH_SERVER",                   // 39
        "WIFI", "STD_EVENT", "STW_EVENT", "STM_EVENT", "ALARM",                                 // 44
        "DETECT_LINK", "DETECT_GROUP", "DETECT_GROUP_LINK", "RULE_DETECT", "RULE_DETECT_LINK",  // 49
        "CONNECT_EXT", "RULE_SCHEDULE", "UPGRADE_AGENT", "WIN_DRV", "STATS_GW",                 // 54
        "SYNC_USER", "SYNC_GROUP", "RDP_SESSION", "CTRL_PROCESS_CPU", "STD_EVENT_REALTIME",  // 59
        "STM_EVENT_REALTIME","STW_EVENT_REALTIME"                                         // 61
};

int fdb_NameFilter(char *sqlbuff, char *table_name)
{
    char tmpbuff[5120];
    char tmpTableName[128];
    char index_name[50];
    int  i,j,k,slen;
    int	 firstWork = 1;

    slen = strlen(sqlbuff);
    memset(tmpbuff, 0x00, sizeof(tmpbuff));

    for (i=0,j=0; i<slen; i++)
    {
        if (sqlbuff[i] != '{')
        {
            tmpbuff[j++] = sqlbuff[i];
        }
        else
        {
            if(!strncasecmp(tmpbuff, "CREATE TABLE ", 13))
            {
                strcat(tmpbuff, table_name);
            }
            else
            {
                if(firstWork) //인덱스이름
                {
                    memset(tmpTableName, 0x00, sizeof(tmpTableName));
                    memset(index_name, 0x00, sizeof(index_name));
                    strcpy(tmpTableName, table_name);
                    fcom_StrTokenR(tmpTableName, ".", 1, index_name);
                    strcat(tmpbuff, index_name);
                    firstWork = 0;
                }
                else
                {
                    strcat(tmpbuff, table_name);
                }
            }
            j=strlen(tmpbuff);
            for (k=0;k<40;k++,i++) //length {STRING}
            {
                if(sqlbuff[i] == '}')
                {
                    break;
                }
            }
        }
    }

    strcpy(sqlbuff, tmpbuff);

    return RET_SUCC;
}

/*
 * Desc: 뷰 Script에서 필터링한다.
 * Date: 2019.00.00
 * Auth: jhchoi
 */
int fdb_ViewFilter(char *sqlbuff, char *view_name, char *event_name)
{
    char tmpbuff[5012];
    char tmpTableName[128];
    char table_name[50];
    char evType[128];
    int  i,j,k,slen;
    int	 workFlag = 1;

    slen = strlen(sqlbuff);
    memset(tmpbuff, 0x00, sizeof(tmpbuff));

    for (i=0,j=0; i<slen; i++)
    {
        if (sqlbuff[i] != '{')
        {
            tmpbuff[j++] = sqlbuff[i];
        }
        else
        {
            if(workFlag == 1)
            {
                strcat(tmpbuff, view_name);
                workFlag ++;
            }
            else if(workFlag == 2)
            {
                strcpy(tmpTableName, view_name);
                memset(table_name, 0x00, sizeof(table_name));
                fcom_ReplaceAll(tmpTableName, "VW_", "", table_name);
                strcat(tmpbuff, table_name);
                workFlag ++;
            }
            else if(workFlag == 3 || workFlag == 5)
            {
                memset(evType, 0x00, sizeof(evType));
                if(strstr(view_name, "CPU") != NULL)
                {
                    sprintf(evType, "%d", CPU);
                }
                else if(strstr(view_name, "SYSTEM") != NULL)
                {
                    sprintf(evType, "%d,%d,%d", SYSTEM,VIRTUAL_MACHINE,CONNECT_EXT_SVR);
                }
                else if(strstr(view_name, "NET_ADAPTER") != NULL)
                {
                    sprintf(evType, "%d,%d,%d,%d", NET_ADAPTER_OVER,NET_ADAPTER_DUPIP,NET_ADAPTER_DUPMAC,NET_ADAPTER_MULIP);
                }
                else if(strstr(view_name, "WIFI") != NULL)
                {
                    sprintf(evType, "%d", WIFI);
                }
                else if(strstr(view_name, "BLUETOOTH") != NULL)
                {
                    sprintf(evType, "%d", BLUETOOTH);
                }
                else if(strstr(view_name, "NET_CONNECTION") != NULL)
                {
                    sprintf(evType, "%d", NET_CONNECTION);
                }
                else if(strstr(view_name, "DISK") != NULL)
                {
                    sprintf(evType, "%d,%d,%d,%d,%d,%d,%d", DISK,DISK_REG,DISK_NEW,DISK_MOBILE_READ,DISK_MOBILE_WRITE,DISK_MOBILE,DISK_HIDDEN);
                }
                else if(strstr(view_name, "NET_DRIVE") != NULL)
                {
                    sprintf(evType, "%d", NET_DRIVE);
                }
                else if(strstr(view_name, "SHARE_FOLDER") != NULL)
                {
                    sprintf(evType, "%d", SHARE_FOLDER);
                }
                else if(strstr(view_name, "INFRARED_DEVICE") != NULL)
                {
                    sprintf(evType, "%d", INFRARED_DEVICE);
                }
                else if(strstr(view_name, "PROCESS") != NULL)
                {
                    sprintf(evType, "%d,%d,%d,%d", PROCESS,PROCESS_WHITE,PROCESS_BLACK,PROCESS_ACCESSMON);
                }
                else if(strstr(view_name, "ROUTER") != NULL)
                {
                    sprintf(evType, "%d", ROUTER);
                }
                else if(strstr(view_name, "NET_PRINTER") != NULL)
                {
                    sprintf(evType, "%d", NET_PRINTER);
                }
                else if(strstr(view_name, "OS_ACCOUNT") != NULL)
                {
                    sprintf(evType, "%d", OS_ACCOUNT);
                }
                else if(strstr(view_name, "OS") != NULL)
                {
                    sprintf(evType, "%d", OPERATING_SYSTEM);
                }
                else
                {
                    WRITE_CRITICAL(CATEGORY_DEBUG, "Undefined ev_type(%s)", view_name);
                }
                strcat(tmpbuff, evType);
                workFlag ++;
            }
            else if(workFlag == 4)
            {
                strcat(tmpbuff, event_name);
                workFlag ++;
            }

            j=strlen(tmpbuff);
            for (k=0;k<30;k++,i++) //length {STRING}
            {
                if(sqlbuff[i] == '}')
                {
                    break;
                }
            }
        }
    }
    strcpy(sqlbuff, tmpbuff);

    return RET_SUCC;
}



int fdb_ExecuteQuery(char *buf, char *p_tbTableName)
{
    int		alterCnt=0;
    char	sqlBuf[256];
    char    cpRes[MAX_BUF];

    memset(cpRes, 0x00, sizeof(cpRes));
    memset(sqlBuf, 0x00, sizeof(sqlBuf));

    fcom_ReplaceAll(buf, ",", ";", cpRes);
    sprintf(sqlBuf, "ALTER TABLE %s.%s ADD %s", g_szDbName,p_tbTableName,cpRes);


    alterCnt = fdb_SqlQuery2(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB, "Fail in query, errcode(%d),msg(%s)",
                g_stMyCon->nErrCode,
                g_stMyCon->cpErrMsg);

        return -1;
    }

    if(alterCnt > 0)
    {
        WRITE_CRITICAL(CATEGORY_DB, "Delete errorlog, alterCnt(%d)",
                alterCnt);
    }

    return alterCnt;
}
int fdb_ExecuteScript(char *table_name, char *filepath)
{
    int		i,j;
    FILE	*r_file;
    char	buff[512];
    char	lineBuff[512];
    char	SQLTmp[1024];

    int  slen, blen;
    char SQLCommand[5120];

    r_file = fopen(filepath, "rt");
    if (r_file == NULL)
        return -1;

    memset(SQLCommand, 0x00, sizeof(SQLCommand));
    while( fgets(lineBuff, 512, r_file)!=NULL )
    {
        // Decode
        memset(buff, 0x00, sizeof(buff));
        blen = fsec_DataAESDecrypt(lineBuff, "INTENT00", buff);

        //blen = strlen(buff);
        for (j=0;j<blen;j++)
        {
            if (buff[j] == ' ')
                continue;
            else
                goto bpoint;
        }

bpoint:
        if (buff[j] == '#') continue;

        strcpy(SQLTmp, buff);
        slen = strlen(SQLTmp);
        for (i=0;i<slen;i++)
        {
            if ((SQLTmp[i] == ';')||(SQLTmp[i] == '\\'))
            {
                SQLTmp[i] = '\0';
                strcat(SQLCommand, SQLTmp);

                fdb_NameFilter(SQLCommand, table_name);

                fdb_SqlQuery2(g_stMyCon, SQLCommand);
                if(g_stMyCon->nErrCode != 0)
                {
                    WRITE_CRITICAL(CATEGORY_DB,	"Fail in execute, errcode(%d)errmsg(%s) ",
                            g_stMyCon->nErrCode,
                            g_stMyCon->cpErrMsg);
                }
                else
                {
                    WRITE_INFO(CATEGORY_DB, "Succeed in execute");
                }
                memset(SQLCommand, 0x00, sizeof(SQLCommand));
                strcpy(SQLTmp, buff+i+1);
                //LogDRet(5, "--> SQLTmp[%s]\n", SQLTmp);
                break;
            }
        }
        strcat(SQLCommand, SQLTmp);
    } //while

    if (strlen(SQLCommand) < 5)
    {
        fclose(r_file);
        return 0;
    }

    WRITE_DEBUG(CATEGORY_DEBUG,"SQL EXECUTE : [%s] ", SQLCommand);
    fdb_SqlQuery2(g_stMyCon, SQLCommand);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in execute, errcode(%d)errmsg(%s)",
                g_stMyCon->nErrCode,
                g_stMyCon->cpErrMsg);
    }
    else
    {
        WRITE_INFO(CATEGORY_DB, "Succeed in execute");
    }

    memset(SQLCommand, 0x00, sizeof(SQLCommand));

    fclose(r_file);

    return 0;
}

int fdb_ExecuteScriptTB(char *filepath)
{
    int  j;
    FILE *r_file;
    char lineBuff[512] = {0x00,};
    char buff[256] = {0x00,};
    char SQLTmp[1024] = {0x00,};
    char strServerId[1+1] = {0x00,};

    int  slen, blen;
    char SQLCommand[6144] = {0x00,};

    r_file = fopen(filepath, "rt");
    if (r_file == NULL)
        return -1;

    memset(SQLCommand, 0x00, sizeof(SQLCommand));
    while( fgets(lineBuff, 512, r_file)!=NULL )
    {
        // Decode
        memset(buff, 0x00, sizeof(buff));
        fsec_DataAESDecrypt(lineBuff, "INTENT00", buff);

        // replace {SERVER_ID}
        if( strstr(buff, "{SERVER_ID}") != NULL )
        {
            memset(strServerId, 0x00, sizeof(strServerId));
            sprintf(strServerId, "%d", g_stServerInfo.stDapComnInfo.nCfgMgwId);
            fcom_ReplaceAll(buff, "{SERVER_ID}", strServerId, buff);

            WRITE_INFO(CATEGORY_DB,"Replace after buff(%s)",buff);
        }

        /*
         * for문안에서 name_filter 하지 않고 위에서
         * replaceAll하기 때문에 blen 다시 계산해야함
         * 타 ExecuteScript..()에서는 name_filter를 사용하기 때문에
         * 다시 계산할 필요없음
         */
        blen = strlen(buff);
        for (j=0;j<blen;j++)
        {
            if (buff[j] == ' ')
                continue;
            else
                goto bpoint;
        }

bpoint:
        if (buff[j] == '#')
            continue;


        strcpy(SQLTmp, buff);
        fcom_Rtrim(SQLTmp,strlen(SQLTmp));
        slen = strlen(SQLTmp);

        /* 2020.10.12 / KBG */
        /* Query안에 ';' 데이터 있을시 오류로 인하여 로직 변경. */
        /*for (i=0;i<slen;i++)
        {
            if ((SQLTmp[i] == ';')||(SQLTmp[i] == '\\'))
            {
                SQLTmp[i] = '\0';
                strcat(SQLCommand, SQLTmp);

                fdb_SqlQuery2(g_stMyCon, SQLCommand);
                if(g_stMyCon->nErrCode != 0)
                {
                    WRITE_CRITICAL(CATEGORY_DB, "Fail in execute, errcode(%d)errmsg(%s)|%s",
                            g_stMyCon->nErrCode,
                            g_stMyCon->cpErrMsg,
                            __func__);
                }
                else
                {
                    WRITE_INFO(CATEGORY_DB,"Succeed in execute|%s",__func__ );
                }

                memset(SQLCommand, 0x00, sizeof(SQLCommand));
                strcpy(SQLTmp, buff+i+1);
                break;
            }
        }*/

        if(SQLTmp[slen -1] == ';' || SQLTmp[slen -1] == '\\')
        {
            SQLTmp[slen -1] = 0x00;
            strcat(SQLCommand, SQLTmp);

            fdb_SqlQuery2(g_stMyCon, SQLCommand);
            if(g_stMyCon->nErrCode != 0)
            {
                WRITE_CRITICAL(CATEGORY_DB, "Fail in execute, errcode(%d)errmsg(%s)",
                               g_stMyCon->nErrCode,
                               g_stMyCon->cpErrMsg);
                WRITE_CRITICAL(CATEGORY_DB,"Fail Execute Script : [%s]",SQLCommand);
            }
            else
            {
                WRITE_INFO(CATEGORY_DB,"Succeed in execute");
                WRITE_DEBUG(CATEGORY_DB,"Success Execute Script : [%s] ",SQLCommand);
            }

            memset(SQLCommand, 0x00, sizeof(SQLCommand));
            memset(SQLTmp, 0x00, sizeof(SQLTmp));
        }
        else
            strcat(SQLCommand, SQLTmp);
    } //while

    if (strlen(SQLCommand) < 5)
    {
        fclose(r_file);
        return 0;
    }

    fdb_SqlQuery2(g_stMyCon, SQLCommand);

    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB, "Fail in execute, errcode(%d)errmsg(%s)",
                g_stMyCon->nErrCode,
                g_stMyCon->cpErrMsg);
    }
    else
    {
        WRITE_INFO(CATEGORY_DB,"Succeed in execute");

    }

    memset(SQLCommand, 0x00, sizeof(SQLCommand));

    fclose(r_file);

    return 0;
}

int fdb_GetExistsDatabase(char *p_DBName)
{
    int		rowCnt;
    char	sqlBuf[64];

    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    sprintf(sqlBuf, "show databases like '%s'", p_DBName);

    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB, "Fail in query, errcode(%d)msg(%s)",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);
    }

    fdb_SqlFreeResult(g_stMyCon);

    return rowCnt;
}
int fdb_CreateDatabase(char* p_DBName)
{
    char	sqlBuf[63 +1];

    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    sprintf(sqlBuf, "CREATE DATABASE IF NOT EXISTS %s ", p_DBName);

    fdb_SqlQuery2(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB, "Fail in query, errcode(%d)msg(%s)",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);

        return (-1);
    }

    return 0;
}

int fdb_GetExistsColumns(char *p_tbDBName, char *p_tbTableName, char *p_column)
{
    int		rowCnt = 0;
    int		resCnt = 0;
    char	sqlBuf[256 +1] = {0x00,};

    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    sprintf(sqlBuf, "select count(*) from INFORMATION_SCHEMA.COLUMNS "
                    "where table_schema='%s' and table_name='%s' and column_name='%s'",
            p_tbDBName,p_tbTableName,p_column);

    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);

    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB, "Fail in query, errcode(%d),msg(%s)",
                g_stMyCon->nErrCode,
                g_stMyCon->cpErrMsg);
    }

    if(rowCnt > 0)
    {
        if(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            resCnt = atoi(g_stMyCon->row[0]);
        }
    }

//    sql_free_result(myCon);
    fdb_SqlFreeResult(g_stMyCon);



    return resCnt;
}

int fdb_GetExistsTable(char *p_histDBName, char *p_histTBName)
{
    int		rowCnt = 0;
    int		resCnt = 0;
    char	sqlBuf[256 +1] = {0x00,};
    char	tmpHistTableName[50 +1] = {0x00,};
    char	tableName[50 +1] = {0x00,};

    memset(tmpHistTableName, 0x00, sizeof(tmpHistTableName));
    memset(tableName, 0x00, sizeof(tableName));

    strcpy(tmpHistTableName, p_histTBName);
    fcom_StrTokenR(tmpHistTableName, ".", 1, tableName);


    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    sprintf(sqlBuf, "select count(*) from INFORMATION_SCHEMA.tables "
                    "where table_schema='%s' and table_name='%s'",
            p_histDBName,tableName);

    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s)",
                g_stMyCon->nErrCode,
                g_stMyCon->cpErrMsg);
    }

    if(rowCnt > 0)
    {
        if(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            resCnt = atoi(g_stMyCon->row[0]);
        }
    }

    fdb_SqlFreeResult(g_stMyCon);

    WRITE_INFO(CATEGORY_DB, "Proc rowCnt(%d)resCnt(%d)", rowCnt,resCnt);

    return resCnt;
}


//테이블검증을위한함수
int fdb_CheckExistsTable(char *p_histDBName, char *p_histTBName)
{
    int		rowCnt = 0;
    int		resCnt = 0;
    char	sqlBuf[256 +1] = {0x00,};

    memset(sqlBuf, 0x00, sizeof(sqlBuf));

    sprintf(sqlBuf, "select count(*) from INFORMATION_SCHEMA.tables "
                    "where table_schema='%s' and table_name='%s'",
            p_histDBName,p_histTBName);

    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB, "Fail in query, errcode(%d),msg(%s)",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);
    }

    if(rowCnt > 0)
    {
        if(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            resCnt = atoi(g_stMyCon->row[0]);
        }
    }
    fdb_SqlFreeResult(g_stMyCon);

    WRITE_INFO(CATEGORY_DB, "Proc rowCnt(%d)resCnt(%d)",
               rowCnt,
               resCnt);

    return resCnt;
}

int fdb_CheckScriptColumns(char *cpLFile, char *p_tbTableName)
{
    int		rxt;
    FILE    *fp = NULL;
    int     line = 0;
    char    cpBuf[MAX_BUF];
    char	colName[30];
    char	lineBuff[1024];

    WRITE_INFO(CATEGORY_DB,"Checking cpLFile: %s", cpLFile);

    fp = fopen(cpLFile, "r");
    if(fp != NULL)
    {
        while(1)
        {
            if(fgets(lineBuff, MAX_BUF, fp) == NULL) break;

            memset(cpBuf, 0x00, sizeof(cpBuf));
            fsec_DataAESDecrypt(lineBuff, "INTENT00", cpBuf);

            if( (strstr(cpBuf, "CREATE TABLE") != NULL)
               || (strstr(cpBuf, "DROP TABLE") != NULL) )
                continue;

            if( (strstr(cpBuf, "PRIMARY KEY") != NULL)
               || (strstr(cpBuf, "UNIQUE KEY") != NULL)
               || (strstr(cpBuf, "ENGINE=") != NULL) )
                break;

            memset(colName, 0x00, sizeof(colName));
            fcom_GetFQNum(cpBuf, " ", colName);

            WRITE_INFO(CATEGORY_DB, "Checking colName: %s(%s)",
                    p_tbTableName,
                    colName);

            rxt = fdb_GetExistsColumns(g_szDbName, p_tbTableName, colName);
            if(rxt == 0)
            {
                WRITE_DEBUG(CATEGORY_DB,"Not found colmn(%s)table(%s)dbname(%s)",
                        colName,
                        p_tbTableName,
                        g_szDbName);

                rxt = fdb_ExecuteQuery(cpBuf, p_tbTableName);
                if(rxt <= 0)
                {
                    WRITE_CRITICAL(CATEGORY_DB,"Fail in alter query, rxt(%d)",
                            rxt);
                }
            }
            line++;
        }
        fclose(fp);
    }

    return 0;
}



int fdb_CheckTbTables(void)
{
    int		i, rxt;
    int		tbItemCnt;
    char	tbTableName[30];
    char	tbScriptName[128];
    char	tbDataName[30];
    char	tmpBuf[256];
    char	tmpBuf2[256];
    FILE    *fp = NULL;

    memset(tbScriptName, 0x00, sizeof(tbScriptName));
    memset(tmpBuf, 0x00, sizeof(tmpBuf));
    memset(tmpBuf2, 0x00, sizeof(tmpBuf2));
    memset(tbDataName, 0x00, sizeof(tbDataName));
    memset(tbTableName, 0x00, sizeof(tbTableName));

    //create tb database
    rxt = fdb_GetExistsDatabase("DAP_DB");
    if(rxt == 0)
    {
        WRITE_CRITICAL(CATEGORY_DB, "Not found database(%s) ", "DAP_DB");
        return -1;
    }

    /* 인사연동 테이블 백업 데이터베이스 */
    rxt = fdb_GetExistsDatabase("DAP_SYNC_BACKUP");
    if(rxt == 0)
    {
        WRITE_INFO(CATEGORY_DB, "Not found database(%s) ", "DAP_SYNC_BACKUP");
        if(fdb_CreateDatabase("DAP_SYNC_BACKUP") != 0)
        {
            WRITE_CRITICAL(CATEGORY_DB,"Create Database (%s) Failed",
                           "DAP_SYNC_BACKUP");
            return (-1);
        }
    }

    //create tb table
    tbItemCnt = sizeof(g_exTbItem)/sizeof(char[TABLE_LENGTH]);
    WRITE_INFO(CATEGORY_DB, "tbItemCnt(%d)", tbItemCnt);

    for(i=0; i<tbItemCnt; i++)
    {
        sprintf(tbScriptName, "create_%s_TB", g_exTbItem[i]);
        sprintf(tbTableName, "%s_TB", g_exTbItem[i]);

        WRITE_INFO(CATEGORY_DB, "tbTableName(%s)", tbTableName);

        sprintf(tmpBuf, "%s/script/table_script/%s.sql", getenv("DAP_HOME"),tbScriptName);
        sprintf(tbDataName, "insert_%s_TB", g_exTbItem[i]);
        sprintf(tmpBuf2, "%s/script/data_script/%s.sql", getenv("DAP_HOME"),tbDataName);

        rxt = fdb_CheckExistsTable(g_szDbName, tbTableName);
        if(rxt == 0)
        {
            //create tb_table
            fdb_ExecuteScriptTB(tmpBuf);

            //insert tb_data
            fp = fopen(tmpBuf2, "r");
            if(fp != NULL)
            {
                fdb_ExecuteScriptTB(tmpBuf2);
                fclose(fp);
            }
        }
        else
        {
            //check tb_table column
            fdb_CheckScriptColumns(tmpBuf, tbTableName);
            // insert server_id data
            if( !strcmp(tbDataName, "insert_SYS_ALARM_TB") ||
                !strcmp(tbDataName, "insert_SYS_CONFIG_CHANGE_TB") ||
                !strcmp(tbDataName, "insert_SYS_FILESYSTEM_TB") ||
                !strcmp(tbDataName, "insert_SYS_PINFO_TB") ||
                !strcmp(tbDataName, "insert_SYS_QUEUE_TB") ||
                !strcmp(tbDataName, "insert_SYS_THRESHOLD_TB"))
            {
                fp = fopen(tmpBuf2, "r");
                if(fp != NULL)
                {
                    fdb_ExecuteScriptTB(tmpBuf2);
                    fclose(fp);
                }
            }
        }
    } // for

    return 0;
}


int fdb_CheckNotiDb(char *proc)
{
    int     local_nRet  = 0;
    int		sqlMgwId    = 0;
    int		rowCnt      = 0;
    int		updateCnt   = 0;
    char	sqlName[80 +1];
    char	tmpBuf[255 +1];
    char	sqlBuf[255 +1];


    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    memset(tmpBuf, 0x00, sizeof(tmpBuf));
    memset(sqlName, 0x00, sizeof(sqlName));

    sqlMgwId = g_stServerInfo.stDapComnInfo.nCfgMgwId;

    sprintf(sqlBuf, "select NAME from SYS_CONFIG_CHANGE_TB where MGWID=%d and CHGFLAG=1 and PROCESS='%s'", sqlMgwId,proc);


    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d)msg(%s)",
                g_stMyCon->nErrCode,
                g_stMyCon->cpErrMsg);
        fdb_SqlFreeResult(g_stMyCon);

        return -1;
    }

    if(rowCnt > 0)
    {
        updateCnt = 0;

        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            strcpy(sqlName,	g_stMyCon->row[0]);

            sprintf(tmpBuf, "touch %s", sqlName);
            system(tmpBuf);

            WRITE_INFO(CATEGORY_DEBUG,"touch system(%s)",tmpBuf);

            memset(sqlBuf, 0x00, sizeof(sqlBuf));

            sprintf(sqlBuf, "update SYS_CONFIG_CHANGE_TB set CHGFLAG = 0 "
                            "WHERE MGWID=%d and PROCESS='%s' and NAME='%s'",
                    sqlMgwId,proc,sqlName );

            local_nRet = fdb_SqlQuery2(g_stMyCon, sqlBuf);

            if ( local_nRet < 0 )
            {
                WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d)msg(%s)",
                               g_stMyCon->nErrCode,
                               g_stMyCon->cpErrMsg);

                fdb_SqlFreeResult(g_stMyCon);

                return -1;
            }
            else
            {
                updateCnt += local_nRet;
            }
        }
    }

    fdb_SqlFreeResult(g_stMyCon);

    return 1;
}



int fdb_CleanAlarm()
{
    unsigned int	sqlMgwId    = 0;
    int				updateCnt   = 0;
    char			sqlBuf[255 +1];

    sqlMgwId = g_stServerInfo.stDapComnInfo.nCfgMgwId;

    memset(sqlBuf, 0x00, sizeof(sqlBuf));

    sprintf(sqlBuf, "update SYS_ALARM_TB set "
                    "ERRORLEVEL='',"
                    "STATE='',"
                    "ERRORID='',"
                    "DETAIL='' "
                    "where MGWID=%d", sqlMgwId);


    updateCnt = fdb_SqlQuery2(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d)errmsg(%s)",
                g_stMyCon->nErrCode,
                g_stMyCon->cpErrMsg);

        return -1;
    }

    WRITE_INFO(CATEGORY_DB,"clean_alarm updateCnt(%d)", updateCnt);

    return	1;
}

int fdb_DeleteDbByDay(char *tbName, int valid_day)
{
    int				deleteCnt = 0;
    char			delDbDay[20];
    char			sqlBuf[127 +1];
    time_t			delTime;

    delTime = time((time_t) 0)- (valid_day * ONE_DAY);

    memset(delDbDay, 0x00, sizeof(delDbDay));
    memset(sqlBuf, 0x00, sizeof(sqlBuf));

    fcom_time2str(delTime, delDbDay, "YYYY-MM-DD\0");

    if (!strcmp(tbName, "MANAGER_EVENT_TB"))
    {
        snprintf	(sqlBuf, sizeof(sqlBuf),
                "delete from %s where MNE_RECORD_TIME < '%s'", tbName,delDbDay);
    }
    else
    {
        snprintf	(sqlBuf, sizeof(sqlBuf),
                "delete from %s where CREATEDATE < '%s'", tbName,delDbDay);
    }

    deleteCnt = fdb_SqlQuery2(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d)msg(%s)",
                g_stMyCon->nErrCode,
                g_stMyCon->cpErrMsg);

        return -1;
    }

    if(deleteCnt > 0)
        WRITE_CRITICAL(CATEGORY_DB,"Delete errorlog, deleteCnt(%d)",deleteCnt);

    return deleteCnt;
}


int fdb_DeleteSyncTableByDay()
{
    int     rowCnt = 0;
    int     nCfgDay = 0;
    int     nCnt = 0;
    int     nLoop = 0;
    char    sqlBuf[256] = {0x00,};
    char    szYYYYMMDD[16+1] = {0x00,};
    char**  ptrTableName = NULL;

    struct tm      stTmOld;
    time_t AgoTime;

    nCfgDay = fcom_GetProfileInt("KEEPLOG","MAX_BACKUP_KEEP_DAY",7);

    AgoTime = time(NULL) - (ONE_DAY * nCfgDay);

    localtime_r(&AgoTime,&stTmOld);

    snprintf(szYYYYMMDD, sizeof(szYYYYMMDD),"%04d-%02d-%02d",
             stTmOld.tm_year+1900,
             stTmOld.tm_mon+1,
             stTmOld.tm_mday);


    snprintf(sqlBuf,sizeof(sqlBuf),"SELECT TABLE_NAME "
                                        " FROM information_schema.TABLES "
                                        " WHERE TABLE_SCHEMA = 'DAP_SYNC_BACKUP' "
                                        " AND TABLE_NAME LIKE 'USER_%%' "
                                        " AND CREATE_TIME < '%s' ",
                                        szYYYYMMDD);


    WRITE_DEBUG(CATEGORY_DEBUG,"Drop Table Select Query : [%s] ",sqlBuf);

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
        ptrTableName = (char**)malloc(sizeof(char*) * rowCnt);
        while (fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            ptrTableName[nCnt] = (char*)malloc(sizeof(char) * 50);
            memset(ptrTableName[nCnt], 0x00, sizeof(char) * 50);

            snprintf(ptrTableName[nCnt],50,"%s",g_stMyCon->row[0]);
            nCnt++;
        }
    }

    fdb_SqlFreeResult(g_stMyCon);

    for(nLoop = 0; nLoop < nCnt; nLoop++)
    {
        memset(sqlBuf, 0x00, sizeof(sqlBuf));
        snprintf(sqlBuf,sizeof(sqlBuf),"DROP TABLE DAP_SYNC_BACKUP.%s",ptrTableName[nLoop]);

        WRITE_DEBUG(CATEGORY_DEBUG,"Drop Table Query2 [%s]",sqlBuf);

        fdb_SqlQuery2(g_stMyCon,sqlBuf);
        if(g_stMyCon->nErrCode != 0)
        {
            WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d)msg(%s)",
                           g_stMyCon->nErrCode,
                           g_stMyCon->cpErrMsg);
        }
    }

    for(nLoop = 0; nLoop < nCnt; nLoop++)
    {
        if(ptrTableName[nLoop] != NULL)
            free(ptrTableName[nLoop]);
    }

    if(ptrTableName != NULL)
    {
        free(ptrTableName);
    }

    return 0;

}

int fdb_PutSessionQuery()
{
    int	rowCnt;

    rowCnt = fdb_SqlQuery(g_stMyCon, "select sysdate()");
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in , errcode(%d)errmsg(%s)",
                g_stMyCon->nErrCode,
                g_stMyCon->cpErrMsg);

        return -1;
    }
    fdb_SqlFreeResult(g_stMyCon);

    if(rowCnt < 1)
        return -1;

    return 0;
}

/*
 * Desc: DAP History 데이타베이스명의 postfix를 구한다.
 * Date: 2019.00.00
 * Auth: jhchoi
 */
int fdb_GetHistoryDbTime(char *p_dbFixed, int iday)
{
    char 	buff[30] = {0x00,};
    _DAP_DB_CONFIG* pstDbConfig;

    pstDbConfig = &g_stServerInfo.stDapDbConfigInfo;

    if(iday != 0)
    {
        fcom_GetTime(buff, -86400*(iday));
    }
    else
    {
        fcom_GetTime(buff, 0);
    }

    WRITE_INFO(CATEGORY_INFO,"buff(%s)histDBPrefix(%s)histDBKind(%d)",
            buff,
            pstDbConfig->szCfgHistDBPrefix,
            pstDbConfig->nCfgHistDbKind);

    if(pstDbConfig->nCfgHistDBPrefix == 0) //String
    {
        if(pstDbConfig->nCfgHistDbKind == 2) //yeer
        {
            memcpy(p_dbFixed, buff, 4);
        }
        else if(pstDbConfig->nCfgHistDbKind == 1) //month
        {
            memcpy(p_dbFixed, buff, 6);
        }
        else if(pstDbConfig->nCfgHistDbKind == 0) //no
        {
            strcpy(p_dbFixed, "");
        }
        else //year
        {
            memcpy(p_dbFixed, buff, 4);
        }
    }

    WRITE_INFO(CATEGORY_DB,"p_dbFixed(%s) ",p_dbFixed);

    return 0;
}

/*
 * Desc: 테이블명의 postfix를 구한다.
 * Date: 2019.00.00
 * Auth: jhchoi
 */
int fdb_GetHistoryTableTime(char *p_tableFixed,
                            char *p_yearbuffer,
                            int iday)
{
    _DAP_DB_CONFIG* pstDbConfig;
    char 	buff[30] = {0x00,};

    pstDbConfig = &g_stServerInfo.stDapDbConfigInfo;

    if(iday != 0)
    {
        fcom_GetTime(buff, -86400*(iday));
    }
    else
    {
        fcom_GetTime(buff, 0);
    }


    WRITE_INFO(CATEGORY_DB,"buff(%s)histDBKind(%d)histTableKind(%d)",
            buff,
            pstDbConfig->nCfgHistDbKind,
            pstDbConfig->nCfgHistTableKind);

    if(pstDbConfig->nCfgHistTableKind == TABLE_KIND_MM) //MONTH
    {
        memcpy(p_tableFixed, buff+4, 2);
    }
    else if(pstDbConfig->nCfgHistTableKind == TABLE_KIND_DD) //DAY
    {
        memcpy(p_tableFixed, buff+6, 2);
    }
    else if(pstDbConfig->nCfgHistTableKind == TABLE_KIND_FIX) //FIX
    {
        strcpy(p_tableFixed, "");
    }
    else //month
    {
        memcpy(p_tableFixed, buff+4, 2);
    }

    if(p_yearbuffer != NULL)
    {
        memcpy(p_yearbuffer, buff, 4);
    }

    WRITE_INFO(CATEGORY_DB,"p_tableFixed(%s:%s)",
            p_yearbuffer,
            p_tableFixed);

    return 0;
}


/*
 * Desc: 현재 날짜를 기준으로 history table을 가져온다.
 * Date: 2018.00.00
 * Auth: jhchoi
 */
int fdb_GetHistoryTableName(char *tableName, char *logTable, char *p_prefix, char *p_postfix)
{
    _DAP_DB_CONFIG* pstDbConfig;

    char	tmpPrefix[6+1];
    char	tmpPostfix[2+1];
    char	tmpTableName[30];


    pstDbConfig = &g_stServerInfo.stDapDbConfigInfo;

    memset(tmpTableName, 0x00, sizeof(tmpTableName));

    if(! strncmp(tableName+strlen(tableName)-3, "_TB", 3))
    {
        memcpy(tmpTableName, tableName, strlen(tableName)-3);
    }
    else
    {
        memcpy(tmpTableName, tableName, strlen(tableName));
    }

    WRITE_INFO(CATEGORY_DB,"p_prefix(%s)p_postfix(%s)" \
                           "histDBKind(%d)histTableKind(%d)",
                           p_prefix,
                           p_postfix,
                           pstDbConfig->nCfgHistDbKind,
                           pstDbConfig->nCfgHistTableKind);


    memset(tmpPrefix, 0x00, sizeof(tmpPrefix));
    memset(tmpPostfix, 0x00, sizeof(tmpPostfix));

    if(strlen(p_prefix) == 0)
    {
        fdb_GetHistoryDbTime(tmpPrefix, 0);
    }
    else
    {
        strcpy(tmpPrefix, p_prefix);
    }


    if(strlen(p_postfix) == 0)
    {
        fdb_GetHistoryTableTime(tmpPostfix, NULL, 0);
    }
    else
    {
        strcpy(tmpPostfix, p_postfix);
    }

    WRITE_INFO(CATEGORY_DB,"tmpPrefix(%s)tmpPostfix(%s)",
            tmpPrefix,
            tmpPostfix);


    if(pstDbConfig->nCfgHistDBPrefix == 0) //String
    {
        if(pstDbConfig->nCfgHistDbKind == 0) //DB: FIXED
        {
            if(pstDbConfig->nCfgHistTableKind == TABLE_KIND_FIX) //TABLE: FIXED
            {
                sprintf(logTable, "%s.%s_HISTORY", pstDbConfig->szCfgHistDBPrefix, tmpTableName);
            }
            else //MONTH or DAY
            {
                sprintf(logTable, "%s.%s_HISTORY_%s", pstDbConfig->szCfgHistDBPrefix, tmpTableName, tmpPostfix);
            }
        }
        else //DB: YEAR or MONTH
        {
            if(pstDbConfig->nCfgHistTableKind == TABLE_KIND_FIX) //TABLE: FIXED
            {
                sprintf(logTable, "%s_%s.%s_HISTORY", pstDbConfig->szCfgHistDBPrefix, tmpPrefix, tmpTableName);
            }
            else //TABLE: MONTH or DAY
            {
                sprintf(logTable, "%s_%s.%s_HISTORY_%s",
                        pstDbConfig->szCfgHistDBPrefix,
                        tmpPrefix,
                        tmpTableName,
                        tmpPostfix);
            }
        }
    }

    else if(pstDbConfig->nCfgHistDBPrefix == -1) //NO
    {
        if(pstDbConfig->nCfgHistTableKind == TABLE_KIND_FIX) //TABLE: FIXED
        {
            sprintf(logTable, "%s_HISTORY", tmpTableName);
        }
        else //TABLE: MONTH or DAY
        {
            sprintf(logTable, "%s_HISTORY_%s", tmpTableName,tmpPostfix);
        }
    }

    WRITE_INFO(CATEGORY_INFO,"logTable(%s)",logTable);

    return 0;
}


int fdb_LsystemInit()
{
    char szTemp[12+1];
    _DAP_DB_CONFIG* pstDbConfig;

    pstDbConfig = &g_stServerInfo.stDapDbConfigInfo;

    memset(szTemp, 0x00, sizeof(szTemp));
    fcom_GetProfile("COMMON","HISTORY_DB_PREFIX",szTemp,"DAP_HISTORY");
    snprintf(pstDbConfig->szCfgHistDBPrefix, sizeof(pstDbConfig->szCfgHistDBPrefix),"%s",szTemp);

    if(strcasecmp(szTemp,"DAP_HISTORY") != 0)
        pstDbConfig->nCfgHistDBPrefix = -1;
    else
        pstDbConfig->nCfgHistDBPrefix = 0;

    memset(szTemp, 0x00, sizeof(szTemp));
    fcom_GetProfile("COMMON","HISTORY_DB_POSTFIX",szTemp,"YYYY");

    if (strcasecmp(szTemp, "YYYY") == 0)
    {
        pstDbConfig->nCfgHistDbKind = 2;
    }
    else if (strcasecmp(szTemp, "YYYYMM") == 0)
    {
        pstDbConfig->nCfgHistDbKind = 1;
    }
    else if (strcasecmp(szTemp, "FIXED") == 0)
    {
        pstDbConfig->nCfgHistDbKind = 0;
    }
    else
    {
        pstDbConfig->nCfgHistDbKind = 2;
    }

    memset(szTemp, 0x00, sizeof(szTemp));
    fcom_GetProfile("COMMON","HISTORY_TABLE_POSTFIX",szTemp,"MM");
    if (strcasecmp(szTemp, "MM") == 0)
    {
        pstDbConfig->nCfgHistTableKind = TABLE_KIND_MM;
        pstDbConfig->nCfgHistTableNext = 60;
    }
    else if (strcasecmp(szTemp, "DD") == 0)
    {
        pstDbConfig->nCfgHistTableKind = TABLE_KIND_DD;
        pstDbConfig->nCfgHistTableNext = 1;
    }
    else if (strcasecmp(szTemp, "FIXED") == 0)
    {
        pstDbConfig->nCfgHistTableKind = TABLE_KIND_FIX;
        pstDbConfig->nCfgHistTableNext = 0;
    }
    else
    {
        pstDbConfig->nCfgHistTableKind = TABLE_KIND_MM;
        pstDbConfig->nCfgHistTableNext = 60;
    }

    return 1;

}

int fdb_SelectCountHbSq(unsigned long long 	p_hbSq, // hb_sq of HW_BASE_TB
                        char*				p_tableName) // table name
{
    int		rowCnt;
    int		resCnt = 0;
    char	sqlBuf[128];

    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    sprintf(sqlBuf, "select count(*) from %s where HB_SQ=%llu", p_tableName,p_hbSq);

    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);
    }

    if(rowCnt > 0)
    {
        if(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            resCnt = atoi(g_stMyCon->row[0]);
        }
    }

    fdb_SqlFreeResult(g_stMyCon);

    WRITE_INFO(CATEGORY_INFO, "Proc rowCnt(%d)resCnt(%d)", rowCnt,resCnt);

    return resCnt;
}

int fdb_SelectSqWinDrv(
        unsigned long long	p_hbSq, 		// hb_sq of HW_BASE_TB
        char*				p_dvName,		// db_name of WIN_DRV
        char*				p_dvService,	// dv_service of WIN_DRV
        char*				p_dvDriver,		// dv_driver of WIN_DRV
        unsigned long long* p_tbSq)		// result variable
{
    int		rowCnt = 0;
    char	sqlBuf[511 +1] = {0x00,};
    unsigned long long seqNo = 0;

    memset(sqlBuf, 0x00, sizeof(sqlBuf) );
    snprintf(sqlBuf, sizeof(sqlBuf), "select DV_SQ from WIN_DRV_TB "
                    "where HB_SQ=%llu "
                    "and DV_DRIVER='%s' "
                    "and DV_NAME='%s' "
                    "and DV_SERVICE='%s'",
            p_hbSq,p_dvDriver,p_dvName,p_dvService);

    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);
    }

    if(rowCnt > 0)
    {
        if(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            seqNo = atoll(g_stMyCon->row[0]);
        }
    }
    fdb_SqlFreeResult(g_stMyCon);

    *p_tbSq = seqNo;

    WRITE_INFO(CATEGORY_DB, "Proc rowCnt(%d)tbSq(%llu) ", rowCnt,*p_tbSq);

    return rowCnt;
}


int fdb_GetIpByBase(
        char*	p_type, 		// distinguished columns
        char*	p_userKey, 		// pc unique key
        char*	p_hbAccessRes)	// result variable
{
    int		rowCnt = 0;
    char	sqlColumn[15 +1] = {0x00,};
    char	sqlBuf[128 +1] = {0x00,};

    memset(sqlColumn, 0x00, sizeof(sqlColumn));
    if(!strcmp(p_type, "IP"))
    {
        strcpy(sqlColumn, "HB_ACCESS_IP");
    }
    else if(!strcmp(p_type, "MAC"))
    {
        strcpy(sqlColumn, "HB_ACCESS_MAC");
    }
    else
    {
        WRITE_INFO(CATEGORY_DB, "Undefine type(%s) ", p_type);
        return -1;
    }

    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    sprintf(sqlBuf, "select %s from HW_BASE_TB where HB_UNQ='%s'", sqlColumn,p_userKey);

    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);
    }

    if(rowCnt > 0)
    {
        if(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            sprintf(p_hbAccessRes, "%s",g_stMyCon->row[0]);
        }
    }

    fdb_SqlFreeResult(g_stMyCon);

    WRITE_INFO(CATEGORY_INFO, "Proc rowCnt(%d)resValue(%s) ",
            rowCnt,p_hbAccessRes);

    return rowCnt;
}