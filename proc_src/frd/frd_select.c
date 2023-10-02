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
#include "frd.h"


char* frd_GetPrevHistSq(char *p_tableName, char *p_colName, unsigned long long p_Sq)
{
    int		rowCnt = 0;
    int		loop = 0;
    char    sqlBuf[127 +1] = {0x00,};
    char	*result = NULL;

    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    snprintf(sqlBuf, sizeof(sqlBuf), "select %s from %s where HB_SQ=%llu", p_colName,p_tableName,p_Sq);

    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode  != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                       g_stMyCon->nErrCode ,
                       g_stMyCon->cpErrMsg);
        WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                             "Fail in query, errcode(%d) errmsg(%s)",
                             g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);

        frd_WriteSqlFail( sqlBuf, strlen(sqlBuf) );
    }

    if(rowCnt > 0)
    {
        if (rowCnt > 10)
        {
            rowCnt = 10;
        }

        if(fcom_malloc((void**)&result, sizeof(char)*(30*rowCnt)+1) != 0)
        {
            WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );

            return NULL;
        }

        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            if(g_stMyCon->row[0] != NULL)
            {
                if(loop == 0)
                {
                    sprintf(result, "%llu", atoll(g_stMyCon->row[0]));
                }
                else
                {
                    sprintf(result+strlen(result), ",%llu", atoll(g_stMyCon->row[0]));
                }
                if(g_stMyCon->row[1] != NULL)
                {
                    sprintf(result+strlen(result), "(%s)", g_stMyCon->row[1]);
                }
                loop++;
                if (loop >= 10)
                {
                    break; // 20200204 jhchoi
                }

            }
        }
    }
    else
    {
        fdb_SqlFreeResult(g_stMyCon);
        WRITE_INFO(CATEGORY_DB,	"Proc rowCnt(%d)result(%s)",
                   rowCnt, result);
        return NULL;
    }

    fdb_SqlFreeResult(g_stMyCon);

    WRITE_INFO(CATEGORY_DB,	"Proc rowCnt(%d)result(%s) ",
               rowCnt, result);

    return result;
}

char* frd_GetPrevHistSqByBase(char *p_tableName, char *p_colName, char *p_Sq)
{
    int		rowCnt = 0;
    int		loop = 0;
    char	*result = NULL;
    char    sqlBuf[128 +1] = {0x00,};

    memset	(sqlBuf, 0x00, sizeof(sqlBuf));
    snprintf(sqlBuf, sizeof(sqlBuf),"select %s from %s where HB_UNQ='%s'",
            p_colName,p_tableName,p_Sq);

    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode  != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s)",
                       g_stMyCon->nErrCode ,
                       g_stMyCon->cpErrMsg);
        WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                             "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);

        frd_WriteSqlFail( sqlBuf, strlen(sqlBuf) );
    }

    if(rowCnt > 0)
    {
        if(fcom_malloc((void**)&result, sizeof(char)*(30*rowCnt)+1) != 0)
        {
            WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );

            return NULL;
        }

        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            if(g_stMyCon->row[0] != NULL)
            {
                if(loop == 0)
                {
                    sprintf(result, "%llu", atoll(g_stMyCon->row[0]));
                }
                else
                {
                    sprintf(result+strlen(result), ",%llu", atoll(g_stMyCon->row[0]));
                }

                if(g_stMyCon->row[1] != NULL)
                {
                    sprintf(result+strlen(result), "(%s)", g_stMyCon->row[1]);
                }
                loop++;
            }
        }
    }
    else
    {
        fdb_SqlFreeResult(g_stMyCon);
        WRITE_INFO(CATEGORY_DB,	"Proc rowCnt(%d)result(%s) ", rowCnt, result);

        return NULL;
    }

    fdb_SqlFreeResult(g_stMyCon);

    WRITE_INFO(CATEGORY_DB,	"Proc rowCnt(%d)result(%s) ",rowCnt, result);

    return result;
}

char* frd_GetPrevHistSqWinDrv(	char *p_tableName,
                                char *p_colName,
                                unsigned long long p_Sq )
{
    int		rowCnt = 0;
    int		loop = 0;
    char    sqlBuf[511 +1] = {0x00,};
    char	*result = NULL;

    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    snprintf(sqlBuf, sizeof(sqlBuf),"select %s from %s where DV_SQ=%llu",
            p_colName, p_tableName, p_Sq);

    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s)",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);
        WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                             "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);

        frd_WriteSqlFail( sqlBuf, strlen(sqlBuf) );
    }

    loop = 0;
    if(rowCnt > 0)
    {
        if (rowCnt > 10) // 20200204 jhchoi 너무많으면메모리오류
        {
            rowCnt = 10;
        }
        if(fcom_malloc( (void**)&result, (sizeof(char)*(30*rowCnt))+1 ) != 0)
        {
            WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
            fdb_SqlFreeResult(g_stMyCon);

            return NULL;
        }
        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            if(g_stMyCon->row[0] != NULL)
            {
                if(loop == 0)
                {
                    sprintf(result, "%llu", atoll(g_stMyCon->row[0]));
                }
                else
                {
                    sprintf(result+strlen(result), ",%llu", atoll(g_stMyCon->row[0]));
                }

                if(g_stMyCon->row[1] != NULL)
                {
                    sprintf(result+strlen(result), "(%s)", g_stMyCon->row[1]);
                }
                loop++;
                if (loop >= 10) // 20200204 jhchoi 너무많으면메모리오류
                {
                    break;
                }
            }
        }
    }
    else
    {
        fdb_SqlFreeResult(g_stMyCon);
        WRITE_INFO(CATEGORY_DB,	"Proc rowCnt(%d)result(%s)", rowCnt, result);
        return NULL;
    }

    fdb_SqlFreeResult(g_stMyCon);

    WRITE_INFO(CATEGORY_DB,	"Proc rowCnt(%d)result(%s) ", rowCnt, result);

    return result;
}


int frd_selectHbsqByHwBaseTb(char*				p_userKey, // pc unique key
                               unsigned long long*	p_hbSq) // result variable
{
    char    sqlBuf[128 +1] = {0x00,};
    int		rowCnt = 0;
    unsigned long long seqNo = 0;

    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    snprintf(sqlBuf,sizeof(sqlBuf), "select HB_SQ from HW_BASE_TB where HB_UNQ='%s'", p_userKey);

    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode  != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                       g_stMyCon->nErrCode ,
                       g_stMyCon->cpErrMsg);
        WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                             "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);

        frd_WriteSqlFail( sqlBuf, strlen(sqlBuf) );

    }

    if(rowCnt > 0)
    {
        if(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            seqNo = atol(g_stMyCon->row[0]);
        }
    }

    fdb_SqlFreeResult(g_stMyCon);

    *p_hbSq = seqNo;

    WRITE_INFO(CATEGORY_DB, "Proc rowCnt(%d)hbSq(%llu) ", rowCnt, *p_hbSq);

    return rowCnt;
}

int	frd_SelectEvsqEventTb(unsigned long long	p_hbSq, // hb_sq of hw_base_tb
                               int p_evType, // event type
                               unsigned long long *p_evSq) // ev_sq of event_tb

{
    int					rowCnt = 0;
    unsigned long long	seqNo = 0;
    char				sqlBuf[128 +1] = {0x00,};


    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    snprintf(sqlBuf, sizeof(sqlBuf),"select EV_SQ from EVENT_TB "
                    "where HB_SQ=%llu and EV_TYPE=%d and EV_EXIST=1",
            p_hbSq,p_evType);

    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode  != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                       g_stMyCon->nErrCode ,
                       g_stMyCon->cpErrMsg);
        WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                             "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);

        frd_WriteSqlFail( sqlBuf, strlen(sqlBuf) );

    }

    if(rowCnt > 0)
    {
        if(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            seqNo = atol(g_stMyCon->row[0]);
        }
    }

    fdb_SqlFreeResult(g_stMyCon);

    *p_evSq = seqNo;

    WRITE_INFO(CATEGORY_DB, "Proc evSq(%llu)rowCnt(%d) ", *p_evSq, rowCnt);

    return rowCnt;
}

int frd_SelectCountNewByDisk(
        unsigned long long 	p_hbSq, // hb_sq of HW_BASE_TB
        _DAP_DISK *			p_DK,   // structs of Disk
        char*				res)    // result variable
{
    int				i = 0, idx = 0;
    int				existCnt = 0;
    int				rowCnt = 0;
    int				chkDiskCnt = 0;
    int				resCnt = 0;
    int				chkDiskIdx[10] = {0,};
    char			sqlBuf[128 +1] = {0x00,};

    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    snprintf(sqlBuf, sizeof(sqlBuf),"select DK_NAME,DK_PHYSICAL_SN from DISK_TB where HB_SQ=%llu", p_hbSq);

    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);
        WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                             "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);

        frd_WriteSqlFail( sqlBuf, strlen(sqlBuf) );

    }

    if(rowCnt > 0)
    {
        i = 0;
        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            for(idx=0; idx<p_DK->size; idx++)
            {
                if (!strcmp(p_DK->DiskValue[idx].dk_name, g_stMyCon->row[0]) &&
                    !strcmp(p_DK->DiskValue[idx].dk_physical_sn, g_stMyCon->row[1]))
                {
                    chkDiskCnt++;
                    chkDiskIdx[i] = idx;
                    i++;
                }
            }
        }
    }

    fdb_SqlFreeResult(g_stMyCon);

    /* 경우의 수
    *	[DB]	[DK]
    *	A,B,C	B,C
    *	B,C		A,B,C
    *	A,C		B,C
    */
    if(p_DK->size == chkDiskCnt) //Agent DISK가 DB DISK와 같다면
    {
        resCnt = (chkDiskCnt - rowCnt); //DISK_NEW 해지라면 -1, 왜냐하면DB에는1개더있기때문
    }
    else
    {
        resCnt = 1; //새로운디스크추가
        for(idx=0; idx<p_DK->size; idx++)
        {
            existCnt = 0;
            for(i=0; i<10; i++)
            {
                if(idx == chkDiskIdx[i]) existCnt++;
            }
            if(existCnt == 0)
            {
                //새로추가된 DISK, string값을 넣는다
                if(strlen(res) == 0)
                {
                    if(strlen(p_DK->DiskValue[idx].dk_model) > 0)
                    {
                        sprintf(res, "%s^%s",
                                p_DK->DiskValue[idx].dk_model,
                                p_DK->DiskValue[idx].dk_interface_type);
                    }
                    else
                    {
                        sprintf(res, "%s^%s",
                                p_DK->DiskValue[idx].dk_desc,
                                p_DK->DiskValue[idx].dk_interface_type);
                    }
                }
                else
                {
                    if(strlen(p_DK->DiskValue[idx].dk_model) > 0)
                    {
                        sprintf(res+strlen(res), ",%s^%s",
                                p_DK->DiskValue[idx].dk_model,
                                p_DK->DiskValue[idx].dk_interface_type);
                    }
                    else
                    {
                        sprintf(res+strlen(res), ",%s^%s",
                                p_DK->DiskValue[idx].dk_desc,
                                p_DK->DiskValue[idx].dk_interface_type);
                    }
                }
            }
        }
    }

    WRITE_INFO(CATEGORY_DB,	"Proc rowCnt(%d)chkDiskCnt(%d)resCnt(%d)newDisk(%s) ",
               rowCnt,chkDiskCnt,resCnt,res);

    return resCnt;
}

int frd_SelectCountDuplByBase(
        char*			    p_type,		// distinguished columns
        char*			    p_userKey,	// pc unique key
        _DAP_NETADAPTER*	p_NA,		// structs of NetAdapter
        char*			    res		// result variables
)
{
    int		i = 0;
    int		idx = 0;
    int		rowCnt = 0;
    int		rowAllCnt = 0;
    int		resCnt = 0;
    int		loop = 0;
    int		firstFlag = 1;
    int		tokenCnt = 0;
    unsigned long long  seqNo = 0;
    char	sqlColumn[15] = {0x00,};
    char	sqlValue[512+1] = {0x00,};
    char	sqlFlag[30] = {0x00,};
    char	sqlBuf[1024] = {0x00,};
    char	detail[512+1] = {0x00,};
    char	naValue[17+1] = {0x00,};
    char	tmpDetail[32] = {0x00,};
    char	hbAccessRes[17+1] = {0x00,}; //ip or mac
    char	**arrStr = NULL;

    //return
    //0 : 사용 안하는 IP로 변경된 경우(해지)
    //1 : 자기자신만 존재하는 경우  정상(해지)
    //2 이상: 중복이벤트

    memset(sqlColumn, 0x00, sizeof(sqlColumn));
    memset(sqlFlag, 0x00, sizeof(sqlFlag));
    memset(detail, 0x00, sizeof(detail));

    if(!strcmp(p_type, "IP"))
    {
        strcpy(sqlColumn, "HB_ACCESS_IP");
        strcpy(sqlFlag, "NET_ADAPTER_DUPL_IP");
    }
    else if(!strcmp(p_type, "MAC"))
    {
        strcpy(sqlColumn, "HB_ACCESS_MAC");
        strcpy(sqlFlag, "NET_ADAPTER_DUPL_MAC");
    }
    else
    {
        WRITE_INFO(CATEGORY_DB, "Undefine type(%s) ", p_type);
        return -1;
    }

    WRITE_INFO(CATEGORY_DB,"Proc size(%d) ", p_NA->size);
    for(idx=0; idx < p_NA->size; idx++)
    {
        memset(sqlValue, 0x00, sizeof(sqlValue));
        if(!strcmp(p_type, "IP"))
        {
            snprintf(sqlValue, sizeof(sqlValue), "%s", p_NA->NetAdapterValue[idx].na_ipv4);
        }
        else if(!strcmp(p_type, "MAC"))
        {
            snprintf(sqlValue, sizeof(sqlValue), "%s", p_NA->NetAdapterValue[idx].na_mac);
        }

        if(strlen(sqlValue) < 7)
            continue;

        /* 요청받은 MAC주소 또는 IP가 여러개인경우 */
        tokenCnt = fcom_TokenCnt(sqlValue, ",");
        if(tokenCnt > 0)
        {
            arrStr = fcom_Jsplit(sqlValue, ',');
            for(i=0; i<=tokenCnt; i++)
            {
                memset(sqlBuf, 0x00, sizeof(sqlBuf));
                snprintf(sqlBuf, sizeof(sqlBuf),"select HB_SQ from HW_BASE_TB where %s='%s'",
                         sqlColumn,
                         arrStr[i]);

                rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
                rowAllCnt += rowCnt;
                if(g_stMyCon->nErrCode != 0)
                {
                    WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                                   g_stMyCon->nErrCode,
                                   g_stMyCon->cpErrMsg);
                    WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                                         "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);

                    frd_WriteSqlFail( sqlBuf, strlen(sqlBuf) );

                }

                WRITE_INFO(CATEGORY_DB,"rowCnt(%d)rowAllCnt(%d) ", rowCnt,rowAllCnt);
                if(rowCnt > 1) //자기자신빼고 > 1
                {
                    if(firstFlag == 1)
                    {
                        firstFlag = 0;
                        snprintf(detail, sizeof(detail),"%s(", arrStr[i]);
                    }
                    else
                    {
                        strcat(detail, ",");
                        strcat(detail, arrStr[i]);
                        strcat(detail, "(");
                    }

                    loop = 0;
                    while((g_stMyCon) == 0)
                    {
                        loop++;
                        memset(tmpDetail, 0x00, sizeof(tmpDetail));
                        snprintf(tmpDetail, sizeof(tmpDetail),"%llu", atoll(g_stMyCon->row[0]));
                        /* strcat 오버플로우 방지 */
                        if(strlen(detail) + strlen(tmpDetail) > sizeof(detail))
                        {
                            break;
                        }
                        else
                        {
                            strcat(detail, tmpDetail);
                        }

                        if(loop < rowCnt)
                        {
                            strcat(detail, ",");
                        }

                    }
                    strcat(detail, ")");
                    resCnt += loop; //중복이있으면

                }// if(rowCnt > 1)
                else if(rowCnt == 1) //자신이IP를바꾼것
                {
                    if(fdb_SqlFetchRow(g_stMyCon) == 0)
                    {
                        seqNo = atol(g_stMyCon->row[0]);
                    }

                    //정책보다 먼저na가 들어올수도 있기때문에
                    memset(hbAccessRes, 0x00, sizeof(hbAccessRes));
                    if(!strcmp(p_type, "IP"))
                    {
                        fdb_GetIpByBase("IP", p_userKey, hbAccessRes);
                    }
                    else if(!strcmp(p_type, "MAC"))
                    {
                        fdb_GetIpByBase("MAC", p_userKey, hbAccessRes);
                    }
                    WRITE_INFO(CATEGORY_DB,	"- hbAccessRes(%s)arrStr[%d](%s) ", hbAccessRes, i, arrStr[i] );

                    if(strcmp(hbAccessRes, arrStr[i]) != 0)
                    {
                        if(firstFlag == 1)
                        {
                            firstFlag = 0;
                            sprintf(detail, "%s(", arrStr[i]);
                        }
                        else
                        {
                            strcat(detail, ",");
                            strcat(detail, arrStr[i]);
                            strcat(detail, "(");
                        }
                        memset(tmpDetail, 0x00, sizeof(tmpDetail));
                        snprintf(tmpDetail, sizeof(tmpDetail),"%llu", seqNo);
                        strcat(detail, tmpDetail);
                        strcat(detail, ")");
                        resCnt += 2; //merge_event_tb에서 DUPIP는 2이상이어야됨
                    }
                    else //자기자신IP만 정상적인상황
                    {
                        resCnt += rowCnt;
                    }
                } // else if(rowCnt == 1)
                else
                {
                    //hw_base_tb에 존재하지 않는 IP로 IP변경한 경우로 패스
                    WRITE_INFO(CATEGORY_DB,	"Maybe ip changed, arrStr(%s)", arrStr[i]);
                }

                fdb_SqlFreeResult(g_stMyCon);

            } // for
            if(arrStr != NULL)
            {
                free(arrStr);
                arrStr = NULL;
            }
        }
        else /* 요청받은 MAC주소 및 IP주소가 한개인경우 */
        {
            memset(sqlBuf, 0x00, sizeof(sqlBuf));
            snprintf(sqlBuf, sizeof(sqlBuf), "select HB_SQ from HW_BASE_TB where %s='%s'", sqlColumn,sqlValue);

            rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
            rowAllCnt += rowCnt;
            if(g_stMyCon->nErrCode != 0)
            {
                WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s)",
                               g_stMyCon->nErrCode,
                               g_stMyCon->cpErrMsg);
                WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                                     "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);

                frd_WriteSqlFail( sqlBuf, strlen(sqlBuf) );
            }

            if(rowCnt > 1) //자기자신빼고 > 1
            {
                if(!strcmp(p_type, "IP"))
                {
                    if(firstFlag == 1)
                    {
                        firstFlag = 0;
                        snprintf(detail, sizeof(detail), "%s(", p_NA->NetAdapterValue[idx].na_ipv4);
                    }
                    else
                    {
                        strcat(detail, ",");
                        strcat(detail, p_NA->NetAdapterValue[idx].na_ipv4);
                        strcat(detail, "(");
                    }
                }
                else if(!strcmp(p_type, "MAC"))
                {
                    if(firstFlag == 1)
                    {
                        firstFlag = 0;
                        snprintf(detail, sizeof(detail),"%s(", p_NA->NetAdapterValue[idx].na_mac);
                    }
                    else
                    {
                        strcat(detail, ",");
                        strcat(detail, p_NA->NetAdapterValue[idx].na_mac);
                        strcat(detail, "(");
                    }
                }
                //LogRet("step2 detail[%s]\n", detail);
                loop = 0;
                while(fdb_SqlFetchRow(g_stMyCon) == 0)
                {
                    loop++;
                    memset(tmpDetail, 0x00, sizeof(tmpDetail));
                    snprintf(tmpDetail, sizeof(tmpDetail), "%llu", atoll(g_stMyCon->row[0]));

                    /* strcat 오버플로우 방지 */
                    if(strlen(detail) + strlen(tmpDetail) > sizeof(detail))
                    {
                        break;
                    }
                    else
                    {
                        strcat(detail, tmpDetail);
                    }
                    if(loop < rowCnt)
                    {
                        strcat(detail, ",");
                    }

                }
                strcat(detail, ")");
                resCnt += loop; //�ߺ���������
            } //if(rowCnt > 1)
            else if(rowCnt == 1) //HW_BASE_TB에 이미 존재하는 IP로 자신의 IP를변경한 경우
            {
                if(fdb_SqlFetchRow(g_stMyCon) == 0)
                {
                    seqNo = atol(g_stMyCon->row[0]);
                }

                //정책보다 먼저na가 들어올수도 있기때문에
                memset(hbAccessRes, 0x00, sizeof(hbAccessRes));
                memset(naValue, 0x00, sizeof(naValue));
                if(!strcmp(p_type, "IP"))
                {
                    fdb_GetIpByBase("IP", p_userKey, hbAccessRes);
                    snprintf(naValue, sizeof(naValue), "%s", p_NA->NetAdapterValue[idx].na_ipv4);
                }
                else if(!strcmp(p_type, "MAC"))
                {
                    fdb_GetIpByBase("MAC", p_userKey, hbAccessRes);
                    snprintf(naValue, sizeof(naValue), "%s", p_NA->NetAdapterValue[idx].na_mac);
                }

                WRITE_INFO(CATEGORY_DB,	"- hbAccessRes(%s)naValue(%s)", hbAccessRes,naValue );

                if(strcmp(hbAccessRes, naValue) != 0)
                {
                    if(firstFlag == 1)
                    {
                        firstFlag = 0;
                        snprintf(detail, sizeof(detail),"%s(", naValue);
                    }
                    else
                    {
                        strcat(detail, ",");
                        strcat(detail, naValue);
                        strcat(detail, "(");
                    }
                    memset(tmpDetail, 0x00, sizeof(tmpDetail));
                    snprintf(tmpDetail, sizeof(tmpDetail), "%llu", seqNo);
                    strcat(detail, tmpDetail);
                    strcat(detail, ")");
                    resCnt += 2; //merge_event_tb에서 DUPIP는 2이상이어야 감지됨
                }
                else  //자기자신ip만 정상적인상황
                {
                    resCnt += rowCnt;
                }
            }
            else
            {
                //hw_base_tb에 존재하지 않는 IP로 IP변경한 경우로 패스
                WRITE_INFO(CATEGORY_DB,	"Maybe ip change, ip(%s)",
                           p_NA->NetAdapterValue[idx].na_ipv4);
            }
            fdb_SqlFreeResult(g_stMyCon);
        }//else
    }

    strcpy(res, detail);

    WRITE_INFO(CATEGORY_DB,	"(%s) Proc rowAllCnt(%d)resCnt(%d)detail(%s) ",
               sqlFlag, rowAllCnt, resCnt, detail);

    return resCnt;
}

int frd_CheckUnregDisk(_DAP_DISK *p_DK, char *p_userKey)
{
    int					i = 0, rxt = 0;
    int					rowCnt = 0;
    int					resCnt = 0;
    int					regDisk = 0;
    unsigned long long	hbSq = 0;
    char				sqlBuf[128 +1] = {0x00,};

    if (strcmp(p_userKey, "unknown") == 0)
    {
        WRITE_CRITICAL(CATEGORY_DB, "Fail in user_key(%s)", p_userKey);
        return 0;
    }

    rxt = frd_selectHbsqByHwBaseTb(p_userKey, &hbSq);
    if(rxt <= 0)
    {
        WRITE_CRITICAL(CATEGORY_DB, "Not found hb_sq, userKey(%s) ", p_userKey);
        return -2;
    }

    snprintf(sqlBuf, sizeof(sqlBuf),"select DK_PHYSICAL_SN from DISK_REG_LINK_TB "
                    "where DR_REG=1 and HB_SQ=%llu", hbSq);

    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s)",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);
        WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                             "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);

        frd_WriteSqlFail( sqlBuf, strlen(sqlBuf) );

    }

    if(rowCnt > 0)
    {
        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            for(i=0; i<p_DK->size; i++)
            {
                if(!strcmp(p_DK->DiskValue[i].dk_physical_sn, g_stMyCon->row[0])) //������
                {
                    regDisk += 1;
                }
            }
        }
    }
    else
    {
        fdb_SqlFreeResult(g_stMyCon);
        WRITE_INFO(CATEGORY_DB, "No disks registered. rowCnt(%d)", rowCnt);
        return -1;
    }
    fdb_SqlFreeResult(g_stMyCon);

    resCnt = (p_DK->size - regDisk); //모두 등록된 디스크라면 0이 출력

    WRITE_INFO(CATEGORY_DB, "Proc rowCnt(%d)regDisk(%d/%d)",
               rowCnt,regDisk,p_DK->size);

    return resCnt;
}


int frd_CheckChangeUserinfo(unsigned long long p_seq, char *p_ip)
{
    int		rowCnt=0;
    int		resCnt=0;
    int		tokenCnt=0;
    char    sqlBuf[128 +1] = {0x00,};
    char	sqlVal[256 +1] = {0x00,};
    char	*tokenIp = NULL;

    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    snprintf(sqlBuf, sizeof(sqlBuf), "select US_IP from USER_TB where US_SQ=%llu", p_seq);

    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode  != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                       g_stMyCon->nErrCode ,
                       g_stMyCon->cpErrMsg);
        WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                             "Fail in query, errcode(%d) errmsg(%s)", g_stMyCon->nErrCode, g_stMyCon->cpErrMsg);

        frd_WriteSqlFail( sqlBuf, strlen(sqlBuf) );

    }

    if (rowCnt > 0)
    {
        if (fdb_SqlFetchRow(g_stMyCon) == 0) // only one value
        {
            if (g_stMyCon->row[0] != NULL)
            {
                tokenCnt = fcom_TokenCnt(g_stMyCon->row[0], ",");
                if (tokenCnt > 0)
                {
                    memset	(sqlVal, 0x00, sizeof(sqlVal));
                    snprintf(sqlVal, sizeof(sqlVal),"%s",g_stMyCon->row[0]);
                    tokenIp = strtok(sqlVal, ",");
                    while (tokenIp != NULL)
                    {
                        if(!strcmp(p_ip, tokenIp))  //같으면
                        {
                            resCnt = 0;
                            tokenIp = NULL;
                            break;
                        }
                        else  //다르면
                        {
                            resCnt = 1;
                            WRITE_INFO(CATEGORY_DB, "User IP changed, cur(%s)db(%s)",
                                       p_ip,tokenIp);
                        }
                        tokenIp = strtok(NULL, ",");
                    }
                }
                else
                {
                    if(strcmp(p_ip, g_stMyCon->row[0])) //다르면
                    {
                        resCnt = 1;
                        WRITE_INFO(CATEGORY_DB, "User IP changed, cur(%s)db(%s)",
                                   p_ip, g_stMyCon->row[0]);
                    }
                }
            }
            else // US_IP is NULL 초기값 셋팅
            {
                resCnt = 999;
            }
        }
    }

    fdb_SqlFreeResult(g_stMyCon);

    WRITE_INFO(CATEGORY_DB,	"Proc resCnt(%d) ", resCnt);

    return resCnt;
}