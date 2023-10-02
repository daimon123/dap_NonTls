//
// Created by KimByoungGook on 2020-06-23.
//


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>


#include "db/dap_mysql.h"
#include "db/dap_defield.h"
#include "com/dap_com.h"
#include "secure/dap_secure.h"

int falarm_GetAlarmInfo(char* p_smsIp, int p_smsPort, char* p_smsFrom, char* p_smsLang,char* p_mailFrom,char* p_mailLang)
{
    char    sqlBuf[128];
    int		rowCnt;


    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    strcpy(sqlBuf, "select CF_NAME, CF_VALUE from CONFIG_TB "
                   "where CF_NAME like 'ALARM_%'");

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
        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            if(g_stMyCon->row[0] != NULL)
            {
                if(!strcmp(g_stMyCon->row[0], "ALARM_MAIL_LANG"))
                {
                    if(g_stMyCon->row[1] != NULL)	strcpy(p_mailLang, g_stMyCon->row[1]);
                    else						    strcpy(p_mailLang, "kr");
                }
                else if(!strcmp(g_stMyCon->row[0], "ALARM_MAIL_FROM"))
                {
                    if(g_stMyCon->row[1] != NULL)	strcpy(p_mailFrom, g_stMyCon->row[1]);
                    else						    strcpy(p_mailFrom, "");
                }
                else if(!strcmp(g_stMyCon->row[0], "ALARM_SMS_IP"))
                {
                    if(g_stMyCon->row[1] != NULL)	strcpy(p_smsIp, g_stMyCon->row[1]);
                    else						strcpy(p_smsIp, "");
                }
                else if(!strcmp(g_stMyCon->row[0], "ALARM_SMS_PORT"))
                {
                    if(g_stMyCon->row[1] != NULL)	p_smsPort = atoi(g_stMyCon->row[1]);
                    else						    p_smsPort = 0;
                }
                else if(!strcmp(g_stMyCon->row[0], "ALARM_SMS_FROM"))
                {
                    if(g_stMyCon->row[1] != NULL)	strcpy(p_smsFrom, g_stMyCon->row[1]);
                    else						    strcpy(p_smsFrom, "");
                }
                else if(!strcmp(g_stMyCon->row[0], "ALARM_SMS_LANG"))
                {
                    if(g_stMyCon->row[1] != NULL)	strcpy(p_smsLang, g_stMyCon->row[1]);
                    else						    strcpy(p_smsLang, "kr");
                }
            }
        }
    }

    fdb_SqlFreeResult(g_stMyCon);

    WRITE_INFO(CATEGORY_INFO,"confSmsServerIp   : (%s)" ,p_smsIp   );
    WRITE_INFO(CATEGORY_INFO,"confSmsServerPort : (%d)" ,p_smsPort );
    WRITE_INFO(CATEGORY_INFO,"confSmsFrom       : (%s)" ,p_smsFrom );
    WRITE_INFO(CATEGORY_INFO,"confSmsLang       : (%s)" ,p_smsLang );
    WRITE_INFO(CATEGORY_INFO,"confMailLang      : (%s)" ,p_mailLang);
    WRITE_INFO(CATEGORY_INFO,"confMailFrom      : (%s)" ,p_mailFrom);


    return rowCnt;
}

int falarm_LoadAlarm(_DAP_DB_ALARM_INFO *pAlarmInfo)
{
    int			rowCnt;
    int			loop = 0;
//    int			sqlMgwId	=	0;
    int			decrypto_size;
    char		sqlBuf[256];
    char		tmpEnc[256];

//    sqlMgwId = g_stServerInfo.stDapComnInfo.nCfgMgwId;

    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    sprintf(sqlBuf, "select "
                    "A.AL_DETECT_TYPE,A.AL_DETECT_LEVEL,A.AL_USE,"
                    "B.MN_CELL_PHONE,B.MN_EMAIL "
                    "from ALARM_TB A, MANAGER_TB B "
                    "where A.MN_SQ = B.MN_SQ and A.AL_FLAG != 'D'"
                    "order by A.MN_SQ");


    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d)msg(%s)",
                g_stMyCon->nErrCode,
                g_stMyCon->cpErrMsg);
    }

    WRITE_INFO(CATEGORY_INFO,"rowCnt(%d)",rowCnt );

    loop = 0;
    if(rowCnt > 0)
    {
        WRITE_INFO(CATEGORY_INFO,"[ALARM_INFO]" );

        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            if (g_stMyCon->row[0] != NULL) pAlarmInfo[loop].al_detect_type = atoi(g_stMyCon->row[0]);
            if (g_stMyCon->row[1] != NULL) pAlarmInfo[loop].al_detect_level = *g_stMyCon->row[1];
            if (g_stMyCon->row[2] != NULL) pAlarmInfo[loop].al_use = *g_stMyCon->row[2];
            if (g_stMyCon->row[3] != NULL)
            {
                memset(tmpEnc, 0x00, sizeof(tmpEnc));
                fsec_DecryptStr(g_stMyCon->row[3], (char*)&tmpEnc, &decrypto_size);
                strcpy(pAlarmInfo[loop].mn_cell_phone, tmpEnc);
            }
            if (g_stMyCon->row[4] != NULL)
            {
                memset(tmpEnc, 0x00, sizeof(tmpEnc));
                fsec_DecryptStr(g_stMyCon->row[4], (char*)&tmpEnc, &decrypto_size);
                strcpy(pAlarmInfo[loop].mn_email, tmpEnc);
            }
            WRITE_INFO(CATEGORY_INFO,"Load Alarm Info " );
            WRITE_INFO(CATEGORY_INFO,"mn_cell_phone[%d]: (%s) ",
                    loop,
                    pAlarmInfo[loop].mn_cell_phone);
            WRITE_INFO(CATEGORY_INFO,"mn_email[%d]: (%s) ",
                       loop,
                       pAlarmInfo[loop].mn_email);
            WRITE_INFO(CATEGORY_INFO,"al_detect_type[%d]: (%d) ",
                       loop,
                       pAlarmInfo[loop].al_detect_type);
            WRITE_INFO(CATEGORY_INFO,"al_detect_level[%d]: (%c)",
                       loop,
                       pAlarmInfo[loop].al_detect_level);
            WRITE_INFO(CATEGORY_INFO,"al_flag[%d]: (%c) ",
                       loop,
                       pAlarmInfo[loop].al_use);
           loop++;
        }
    }
    fdb_SqlFreeResult(g_stMyCon);

//    WRITE_INFO(CATEGORY_INFO,"Reload loop(%d)|%s",loop,__func__ );

    return loop;
}