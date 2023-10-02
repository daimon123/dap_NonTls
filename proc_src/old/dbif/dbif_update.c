//
// Created by KimByoungGook on 2020-10-30.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <syslog.h>

#include "db/dap_mysql.h"
#include "db/dap_defield.h"
#include "db/dap_trandb.h"
#include "db/dap_checkdb.h"

#include "com/dap_def.h"
#include "com/dap_com.h"
#include "com/dap_req.h"
#include "dbif.h"


int fdbif_UpdateExternalByHwBaseTb(_DAP_AGENT_INFO *p_AI, int *hbExt)
{
    int					rowCnt = 0;
    int					mergeCnt = 0;
    char				sqlBuf[512 +1] = {0x00,};

    sprintf(sqlBuf, "select HB_EXTERNAL from HW_BASE_TB "
                    "where HB_UNQ='%s'", p_AI->user_key);

    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode  != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                       g_stMyCon->nErrCode ,
                       g_stMyCon->cpErrMsg);
    }

    if(rowCnt > 0)
    {
        if(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            if(g_stMyCon->row[0] != NULL)
            {
                *hbExt = atoi(g_stMyCon->row[0]);
            }
        }
    }

    fdb_SqlFreeResult(g_stMyCon);

    memset	(sqlBuf, 0x00, sizeof(sqlBuf));
    if(rowCnt > 0)
    {
        sprintf	(sqlBuf,"update HW_BASE_TB set "
                           "HB_ACCESS_TIME=sysdate(),"
                           "HB_AGENT_VER='%s',"
                           "HB_EXTERNAL=1,"
                           "HB_DEL=0 "
                           "where HB_UNQ='%s'",
                    p_AI->agent_ver,
                    p_AI->user_key);
    }
    else
    {
        sprintf	(sqlBuf,"insert into HW_BASE_TB "
                           "("
                           "US_SQ,"
                           "HB_UNQ,"
                           "HB_AGENT_VER,"
                           "HB_ACCESS_IP,"
                           "HB_ACCESS_MAC,"
                           "HB_SOCKET_IP,"
                           "HB_ACCESS_TIME,"
                           "HB_EXTERNAL"
                           ")values("
                           "%llu,"
                           "'%s',"
                           "'%s',"
                           "'%s',"
                           "'%s',"
                           "'%s',"
                           "sysdate(),"
                           "1,",
                    p_AI->user_seq,
                    p_AI->user_key,
                    p_AI->agent_ver,
                    p_AI->user_ip,
                    p_AI->user_mac,
                    p_AI->sock_ip);
    }


    mergeCnt = fdb_SqlQuery2(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode  != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                       g_stMyCon->nErrCode ,
                       g_stMyCon->cpErrMsg);

        return -1;
    }

    if(mergeCnt > 0)
    {
        WRITE_INFO(CATEGORY_DB,	"Update access info, mergeCnt(%d)hbExt(%d) ",
                   mergeCnt,*hbExt);
    }


    return mergeCnt;
}



int fdbif_UpdateSysQueueTb(int tot, int value)
{
    int		updateCnt = 0;
    int		sqlMgwId = 0;
    char	sqlBuf[256];
    char	sqlName[30];
    char    sztemp[32+1];

    _DAP_COMN_INFO* pstComnInfo;

    pstComnInfo = &g_stServerInfo.stDapComnInfo;

    sqlMgwId = pstComnInfo->nCfgMgwId;

    snprintf(sztemp, sizeof(sztemp),"%s",
             pstComnInfo->szDebugName[5] == '1' ? "SAVEQ01" :
             pstComnInfo->szDebugName[5] == '2' ? "SAVEQ02" :
             pstComnInfo->szDebugName[5] == '3' ? "SAVEQ03" :
             pstComnInfo->szDebugName[5] == '4' ? "SAVEQ04" :
             pstComnInfo->szDebugName[5] == '5' ? "SAVEQ05" :
             pstComnInfo->szDebugName[5] == '6' ? "SAVEQ06" :
             pstComnInfo->szDebugName[5] == '7' ? "SAVEQ07" :
             pstComnInfo->szDebugName[5] == '8' ? "SAVEQ08" :
             pstComnInfo->szDebugName[5] == '9' ? "SAVEQ09" : "SAVEQ10" );

    strcpy(sqlName, sztemp);

    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    sprintf(sqlBuf, "update SYS_QUEUE_TB "
                    "set TOTAL=%d, USED=%d, AVAIL=%d "
                    "where MGWID=%d and NAME='%s'",
            tot,value,(int)tot-value,sqlMgwId,sqlName);


    updateCnt = fdb_SqlQuery2(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode  != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                       g_stMyCon->nErrCode ,
                       g_stMyCon->cpErrMsg);
        return -1;
    }

    return updateCnt;
}

int fdbif_UpdateAccessByHwBaseTb(_DAP_AGENT_INFO *p_AI, char *p_prefix, char *p_postfix, int* hbExt)
{
    int		rxt = 0;
    int		isChange = 0;
    int		resExt = 0;
    int		udtCnt = 0;
    char    CommaFlag = 0x00;
    char	histTableName[50 +1] = {0x00,};
    char	sqlBuf[512 + 1] = {0x00,};
    char    szStrTemp[256 +1] = {0x00,};

    //ip or mac or hb_del > 0 바뀌면 변경이 있는것으로 확인
    rxt = fdbif_CheckChangeBaseinfo(p_AI->user_key,
                                    p_AI->user_ip,
                                    p_AI->user_mac,
                                    p_AI->hb_del,
                                    p_AI->hb_external,
                                    &resExt);
    if(rxt > 0 )
    {
        isChange = 1;
        memset(histTableName, 0x00, sizeof(histTableName));
        fdb_GetHistoryTableName("HW_BASE_TB", histTableName, p_prefix, p_postfix);
        rxt = fdb_InsertHwBaseHistory(p_AI->user_key, histTableName);
    }
    *hbExt = resExt;
    memset	(sqlBuf, 0x00, sizeof(sqlBuf));

    /* 이전로직 주석처리 .*/
    /* if (strlen(p_AI->access_time) == 0)
     {
         sprintf	(sqlBuf,"update HW_BASE_TB set "
                            "US_SQ=%llu,"
                            "HB_ACCESS_IP='%s',"
                            "HB_ACCESS_MAC='%s',"
                            "HB_SOCKET_IP='%s',"
                            "HB_ACCESS_TIME=sysdate(),"
                            "HB_AGENT_VER='%s',"
                            "HB_EXTERNAL=0,"
                            "HB_DEL=0 "
                            "where HB_UNQ='%s'",
                     p_AI->user_seq,
                     p_AI->user_ip,
                     p_AI->user_mac,
                     p_AI->sock_ip,
                     p_AI->agent_ver,
                     p_AI->user_key);
         //isReload = 1;
     }
     else
     {
         sprintf	(sqlBuf,"update HW_BASE_TB set "
                            "US_SQ=%llu,"
                            "HB_ACCESS_IP='%s',"
                            "HB_ACCESS_MAC='%s',"
                            "HB_SOCKET_IP='%s',"
                            "HB_ACCESS_TIME='%s',"
                            "HB_AGENT_VER='%s',"
                            "HB_EXTERNAL=0,"
                            "HB_DEL=0 "
                            "where HB_UNQ='%s'",
                     p_AI->user_seq,
                     p_AI->user_ip,
                     p_AI->user_mac,
                     p_AI->sock_ip,
                     p_AI->access_time,
                     p_AI->agent_ver,
                     p_AI->user_key);
     }*/

    sprintf(sqlBuf,"UPDATE HW_BASE_TB SET ");
    if(p_AI->user_seq != 0)
    {
        memset(szStrTemp, 0x00, sizeof(szStrTemp));
        if(CommaFlag == 0x00)
        {
            snprintf(szStrTemp,sizeof(szStrTemp)," US_SQ=%llu",p_AI->user_seq);
        }
        else
        {
            snprintf(szStrTemp,sizeof(szStrTemp),", US_SQ=%llu",p_AI->user_seq);
        }
        strcat(sqlBuf,szStrTemp);
        CommaFlag = 0x01;
    }

    if(p_AI->user_ip[0] != 0x00)
    {
        memset(szStrTemp, 0x00, sizeof(szStrTemp));
        if(CommaFlag == 0x00)
        {
            snprintf(szStrTemp,sizeof(szStrTemp)," HB_ACCESS_IP='%s'",p_AI->user_ip);
        }
        else
        {
            snprintf(szStrTemp,sizeof(szStrTemp),", HB_ACCESS_IP='%s'",p_AI->user_ip);
        }
        strcat(sqlBuf,szStrTemp);
        CommaFlag = 0x01;
    }

    if(p_AI->user_mac[0] != 0x00)
    {
        memset(szStrTemp, 0x00, sizeof(szStrTemp));
        if(CommaFlag == 0x00)
        {
            snprintf(szStrTemp,sizeof(szStrTemp)," HB_ACCESS_MAC='%s'",p_AI->user_mac);
        }
        else
        {
            snprintf(szStrTemp,sizeof(szStrTemp),", HB_ACCESS_MAC='%s'",p_AI->user_mac);
        }
        strcat(sqlBuf,szStrTemp);
        CommaFlag = 0x01;
    }

    if(p_AI->sock_ip[0] != 0x00)
    {
        memset(szStrTemp, 0x00, sizeof(szStrTemp));
        if(CommaFlag == 0x00)
        {
            snprintf(szStrTemp,sizeof(szStrTemp)," HB_SOCKET_IP='%s'",p_AI->sock_ip);
        }
        else
        {
            snprintf(szStrTemp,sizeof(szStrTemp),", HB_SOCKET_IP='%s'",p_AI->sock_ip);
        }
        strcat(sqlBuf,szStrTemp);
        CommaFlag = 0x01;
    }

    if(p_AI->access_time[0] != 0x00)
    {
        memset(szStrTemp, 0x00, sizeof(szStrTemp));
        if(CommaFlag == 0x00)
        {
            snprintf(szStrTemp,sizeof(szStrTemp)," HB_ACCESS_TIME='%s'",p_AI->access_time);
        }
        else
        {
            snprintf(szStrTemp,sizeof(szStrTemp),", HB_ACCESS_TIME='%s'",p_AI->access_time);
        }
        strcat(sqlBuf,szStrTemp);
        CommaFlag = 0x01;
    }
    else /* Access Time NULL */
    {
        memset(szStrTemp, 0x00, sizeof(szStrTemp));
        if(CommaFlag == 0x00)
        {
            snprintf(szStrTemp,sizeof(szStrTemp)," HB_ACCESS_TIME=sysdate()");
        }
        else
        {
            snprintf(szStrTemp,sizeof(szStrTemp),", HB_ACCESS_TIME=sysdate()");
        }
        strcat(sqlBuf,szStrTemp);
        CommaFlag = 0x01;

    }

    if(p_AI->agent_ver[0] != 0x00)
    {
        memset(szStrTemp, 0x00, sizeof(szStrTemp));
        if(CommaFlag == 0x00)
        {
            snprintf(szStrTemp,sizeof(szStrTemp)," HB_AGENT_VER='%s'",p_AI->agent_ver);
        }
        else
        {
            snprintf(szStrTemp,sizeof(szStrTemp),", HB_AGENT_VER='%s'",p_AI->agent_ver);
        }
        strcat(sqlBuf,szStrTemp);
        CommaFlag = 0x01;
    }

    /** Agent가  전송한 HW_BASE_TB 컬럼은 무조건 HB_DEL컬럼 0으로 Update **/
    /*if(p_AI->hb_del != 0)
    {
        memset(szStrTemp, 0x00, sizeof(szStrTemp));
        if(CommaFlag == 0x00)
        {
            snprintf(szStrTemp,sizeof(szStrTemp)," HB_DEL=%d",p_AI->hb_del);
        }
        else
        {
            snprintf(szStrTemp,sizeof(szStrTemp),", HB_DEL=%d",p_AI->hb_del);
        }
        strcat(sqlBuf,szStrTemp);
        CommaFlag = 0x01;
    }*/

    memset(szStrTemp, 0x00, sizeof(szStrTemp));
    if(CommaFlag == 0x00)
    {
        snprintf(szStrTemp,sizeof(szStrTemp)," HB_DEL=%d",p_AI->hb_del);
    }
    else
    {
        snprintf(szStrTemp,sizeof(szStrTemp),", HB_DEL=%d",p_AI->hb_del);
    }
    strcat(sqlBuf,szStrTemp);
    CommaFlag = 0x01;


    memset(szStrTemp, 0x00, sizeof(szStrTemp));
    if(CommaFlag == 0x00)
    {
        snprintf(szStrTemp,sizeof(szStrTemp)," HB_EXTERNAL=%d",p_AI->hb_external);
    }
    else
    {
        snprintf(szStrTemp,sizeof(szStrTemp),", HB_EXTERNAL=%d",p_AI->hb_external);
    }
    strcat(sqlBuf,szStrTemp);
    CommaFlag = 0x01;


    snprintf(szStrTemp,sizeof(szStrTemp)," WHERE HB_UNQ='%s'",p_AI->user_key);
    strcat(sqlBuf,szStrTemp);
    WRITE_DEBUG(CATEGORY_DEBUG,"Update Execute : [%s] ",sqlBuf);

    udtCnt = fdb_SqlQuery2(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode  != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                       g_stMyCon->nErrCode ,
                       g_stMyCon->cpErrMsg);
        return -1;
    }


    if(udtCnt > 0)
    {
        WRITE_INFO(CATEGORY_DB, "Update access info, udtCnt(%d)", udtCnt);
    }


    return isChange;
}

int fdbif_UpdateUserTb(_DAP_AGENT_INFO *p_AI, char *p_prefix, char *p_postfix)
{
    int		rxt = 0;
    int		isChange = 0;
    char	histTableName[50 +1] = {0x00,};
    char	sqlBuf[256 +1] = {0x00,};

    //user ip > 0 바뀌면 변경이 있는것으로 확인
    /* USER_IP 변경이 있으면.. */
    if(p_AI->user_ip[0] != 0x00)
    {
        rxt = fdbif_CheckChangeUserinfo(p_AI->user_seq, p_AI->user_ip);
        if(rxt > 0 )
        {
            isChange = 1;
            if (rxt != 999) // 초기값 세팅이 아니라, ip가 변경된 것이라면
            {
                memset(histTableName, 0x00, sizeof(histTableName));
                fdb_GetHistoryTableName("USER_TB", histTableName, p_prefix, p_postfix);
                rxt = fdb_InsertUserHistory(p_AI->user_seq, histTableName);
            }

            memset	(sqlBuf, 0x00, sizeof(sqlBuf));
            sprintf	(sqlBuf,"update USER_TB set "
                               "US_IP='%s',"
                               "US_MODIFY_TIME=sysdate() "
                               "where US_SQ=%llu",
                        p_AI->user_ip,p_AI->user_seq);

            fdb_SqlQuery2(g_stMyCon, sqlBuf);
            if(g_stMyCon->nErrCode  != 0)
            {
                WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s)",
                               g_stMyCon->nErrCode ,
                               g_stMyCon->cpErrMsg);

                return -1;
            }

            WRITE_INFO(CATEGORY_DB, "Change user sq(%llu)ip(%s)",
                       p_AI->user_seq,p_AI->user_ip);

        }
    }

    return isChange;
}




