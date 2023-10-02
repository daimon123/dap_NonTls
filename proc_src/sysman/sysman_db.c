//
// Created by KimByoungGook on 2020-06-19.
//
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/statvfs.h>

#include "com/dap_com.h"
#include "db/dap_mysql.h"
#include "db/dap_defield.h"


int fsysdb_GetFileStat(_DAP_DB_FILSYSTEM_STAT *pFileStat)
{
    int		rowCnt = 0;
    int		loop = 0;
    int		sqlMgwId = 0;
    char	sqlBuf[128 +1] = {0x00,};

    memset(sqlBuf, 0x00, sizeof(sqlBuf));

    sqlMgwId = g_stServerInfo.stDapComnInfo.nCfgMgwId;

    sprintf(sqlBuf, "select FILESYSTEM from SYS_FILESYSTEM_TB where MGWID = %d", sqlMgwId);


    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d)msg(%s) ",
                g_stMyCon->nErrCode,
                g_stMyCon->cpErrMsg);
    }

    loop = 0;
    if(rowCnt > 0)
    {
        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            strcpy(pFileStat[loop].filesystem, g_stMyCon->row[0]);
            loop++;
        }
    }
    fdb_SqlFreeResult(g_stMyCon);

    WRITE_CRITICAL(CATEGORY_INFO,"rowCnt(%d) loop(%d)",
            rowCnt,
            loop);

    return loop;
}


int fsysdb_LoadThreshold(_DAP_DB_THRESHOLD_INFO *pThreshold)
{
    int			rowCnt = 0;
    int			loop = 0;
    char		sqlBuf[128 +1] = {0x00,};
    int		    sqlMgwId = 0;

    sqlMgwId = g_stServerInfo.stDapComnInfo.nCfgMgwId;

    memset(sqlBuf, 0x00, sizeof(sqlBuf));

    sprintf(sqlBuf, "SELECT ELEMENT,"   \
							"CRITICAL,"	\
							"MAJOR,"	\
							"MINOR "	\
					"FROM    SYS_THRESHOLD_TB " \
                    " WHERE MGWID = %d " \
					"ORDER BY ELEMENT",
					sqlMgwId);


    WRITE_DEBUG(CATEGORY_DEBUG,"Execute Query : [%s] ",sqlBuf);

    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d)msg(%s)",
                g_stMyCon->nErrCode,
                g_stMyCon->cpErrMsg);
    }

    if(rowCnt > 0)
    {
        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            snprintf(pThreshold[loop].category,sizeof(pThreshold[loop].category),"%s",g_stMyCon->row[0]);
            pThreshold[loop].critical		= atoi(g_stMyCon->row[1]);
            pThreshold[loop].major			= atoi(g_stMyCon->row[2]);
            pThreshold[loop].minor			= atoi(g_stMyCon->row[3]);

            WRITE_DEBUG(CATEGORY_DB,"rowCnt(%d) loop(%d) value(%s)(%d)(%d)(%d)",
                        rowCnt,
                        loop,
                        pThreshold[loop].category,
                        pThreshold[loop].critical,
                        pThreshold[loop].major,
                        pThreshold[loop].minor);
            loop++;
        }
    }
    fdb_SqlFreeResult(g_stMyCon);


    return loop;
}

int fsysdb_GetSysDiskInfo(_DAP_DB_FILSYSTEM_STAT *pSysFile, int nProcess)
{
    int					i = 0;
    long                lnTotal = 0;
    long                lnBlock = 0;
    long                lnUsed = 0;
    long                lnAvail = 0;
    double              dCapacity = 0.0;
    double				temp = 0.0;
    float               fOffset = 0.0;
    struct  statvfs     stBuf;

    memset(&stBuf, 0x00, sizeof(statvfs));

    WRITE_INFO(CATEGORY_INFO,"Start Begin" );


    for(i = 0; i < nProcess; i++)
    {
        WRITE_INFO(CATEGORY_DEBUG,"filesystem(%s)",
                pSysFile[i].filesystem);

        if (statvfs(pSysFile[i].filesystem, &stBuf) != 0)
        {
            return	-3;
        }
        lnBlock = stBuf.f_frsize;
        fOffset = lnBlock / 1024;
        lnTotal = stBuf.f_blocks * fOffset;
        lnUsed = (stBuf.f_blocks - stBuf.f_bfree) * fOffset;
        lnAvail = stBuf.f_bavail * fOffset;

        if (lnUsed+lnAvail)
        {
            temp = ((lnUsed * 1.0) / ((lnUsed+lnAvail) * 1.0 )) * 100;
            dCapacity = (int)(temp);
        }
        else
        {
            dCapacity = 0;
        }

        pSysFile[i].total		=	lnTotal;
        pSysFile[i].used		=	lnUsed;
        pSysFile[i].avail		=	lnAvail;
        pSysFile[i].capacity	=	dCapacity;

        WRITE_INFO(CATEGORY_INFO,"File System Total %s (%d:%d:%d:%d)",
                pSysFile[i].filesystem,
                pSysFile[i].total,
                pSysFile[i].used,
                pSysFile[i].avail,
                pSysFile[i].capacity);
    }

    WRITE_INFO(CATEGORY_INFO,"End " );

    return	1;
}

int fsysdb_GetQueueStat(_DAP_DB_QUEUE_STAT *pQueueStat)
{
    int		rowCnt = 0;
    int		loop = 0;
    int		sqlMgwId = 0;
    char	sqlBuf[128 +1] = {0x00,};

    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    sqlMgwId = g_stServerInfo.stDapComnInfo.nCfgMgwId;

    sprintf(sqlBuf, "select NAME from SYS_QUEUE_TB where MGWID = %d", sqlMgwId);

    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d)msg(%s)",
                g_stMyCon->nErrCode,
                g_stMyCon->cpErrMsg);
    }

    if(rowCnt > 0)
    {
        loop = 0;

        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            strcpy(pQueueStat[loop].name, g_stMyCon->row[0]);
            loop++;
        }
    }
    fdb_SqlFreeResult(g_stMyCon);

    return loop;
}

int fsysdb_UpdateFileStat(_DAP_DB_FILSYSTEM_STAT *pFileStat, int nMount)
{
    int		sqlMgwId		=	0;
    char	sqlBuf[1024 +1] = {0x00,};
    int		loop = 0;
    int		mergeCnt = 0;

    WRITE_INFO(CATEGORY_INFO,"nMount(%d)",nMount );

    sqlMgwId = g_stServerInfo.stDapComnInfo.nCfgMgwId;

    for(loop = 0; loop < nMount; loop++)
    {
        memset(sqlBuf, 0x00, sizeof(sqlBuf));

        sprintf(sqlBuf, "insert into SYS_FILESYSTEM_TB"
                        "("
                        "MGWID,"
                        "TOTAL,"
                        "USED,"
                        "AVAIL,"
                        "CAPACITY,"
                        "FILESYSTEM"
                        ") VALUES ("
                        "%d,"
                        "%d,"
                        "%d,"
                        "%d,"
                        "%d,"
                        "'%s'"
                        ")"
                        "on duplicate key update "
                        "MGWID=%d,"
                        "TOTAL=%d,"
                        "USED=%d,"
                        "AVAIL=%d,"
                        "CAPACITY=%d,"
                        "FILESYSTEM='%s'",
                sqlMgwId,
                pFileStat[loop].total,
                pFileStat[loop].used,
                pFileStat[loop].avail,
                pFileStat[loop].capacity,
                pFileStat[loop].filesystem,
                sqlMgwId,
                pFileStat[loop].total,
                pFileStat[loop].used,
                pFileStat[loop].avail,
                pFileStat[loop].capacity,
                pFileStat[loop].filesystem
        );

        mergeCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);

        if(g_stMyCon->nErrCode != 0)
        {
            WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d)msg(%s)",
                    g_stMyCon->nErrCode,
                    g_stMyCon->cpErrMsg);

            return -1;
        }
    }

//    fdb_SqlCommit(g_stMyCon);

    WRITE_INFO(CATEGORY_DB,"mergeCnt(%d)",mergeCnt );

    return 1;
}

int fsysdb_UpdateSysInfo(_DAP_DB_SYSTEM_LOAD_INFO *pSysLoad)
{
    int			mergeCnt = 0;
    char		sqlBuf[1024 +1] = {0x00,};

    memset(sqlBuf, 0x00, sizeof(sqlBuf));

    sprintf(sqlBuf, "insert into SYS_SYSTEM_TB ("
                    "MGWID,"
                    "MEM_TOT,"
                    "MEM_USED,"
                    "MEM_FREE,"
                    "MEM_PERUSED,"
                    "CPU_USER,"
                    "CPU_SYS,"
                    "CPU_IDLE"
                    ") VALUES ("
                    "%d,"
                    "%d,"
                    "%d,"
                    "%d,"
                    "%d,"
                    "%d,"
                    "%d,"
                    "%d"
                    ") on duplicate key update "
                    "MGWID=%d,"
                    "MEM_TOT=%d,"
                    "MEM_USED=%d,"
                    "MEM_FREE=%d,"
                    "MEM_PERUSED=%d,"
                    "CPU_USER=%d,"
                    "CPU_SYS=%d,"
                    "CPU_IDLE=%d",
            pSysLoad->mgwid,
            pSysLoad->mem_tot,
            pSysLoad->mem_used,
            pSysLoad->mem_free,
            pSysLoad->mem_perused,
            pSysLoad->cpu_user,
            pSysLoad->cpu_sys,
            pSysLoad->cpu_idle,
            pSysLoad->mgwid,
            pSysLoad->mem_tot,
            pSysLoad->mem_used,
            pSysLoad->mem_free,
            pSysLoad->mem_perused,
            pSysLoad->cpu_user,
            pSysLoad->cpu_sys,
            pSysLoad->cpu_idle
    );

    mergeCnt = fdb_SqlQuery2(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d)msg(%s)",
                g_stMyCon->nErrCode,
                g_stMyCon->cpErrMsg);
        return -1;
    }

    WRITE_INFO(CATEGORY_INFO,"mergeCnt(%d)",
            mergeCnt);

    return		1;
}

int fsysdb_UpdateSysAlarm(_DAP_DB_THRESHOLD_INFO *pThreshold, int value, char *pReason)
{
    int			sqlMgwId = 0;
    int			rowCnt = 0;
    int			resCnt = 0;
    int			insertCnt = 0;
    int			updateCnt = 0;
    char		sqlState;
    char		cpErrorId[41 +1] = {0x00,};
    char		cpReason[1000 +1] = {0x00,};
    char		sqlCategory[80 +1] = {0x00,};
    char		sqlErrorLevel[11 +1] = {0x00,};
    char		sqlErrorId[41 +1] = {0x00,};
    char		sqlDetail[1000 +1] = {0x00,};
    char		sqlBuf[1024 +1] = {0x00,};


    sqlMgwId = g_stServerInfo.stDapComnInfo.nCfgMgwId;
    sqlState = 'S';

    memset(sqlCategory, 0x00, sizeof(sqlCategory));
    memset(sqlErrorId, 0x00, sizeof(sqlErrorId));

    snprintf(sqlCategory, sizeof(sqlCategory), "%s",pThreshold->category);
    snprintf(sqlErrorId, sizeof(sqlErrorId), "%s","");

    if(!strcmp(pThreshold->category, "CPU"))
    {
        WRITE_CRITICAL(CATEGORY_DB,"CPU: value(%d), critical(%d)major(%d)minor(%d)",
                value,
                pThreshold->critical,
                pThreshold->major,
                pThreshold->minor);

        if(value == 0)
        {
            return		1;
        }

        if(value > pThreshold->minor)
        {
            sqlState	=	'E';
        }
        else
        {
            if(value <= pThreshold->minor && value > pThreshold->major)
            {
                snprintf(sqlErrorLevel,sizeof(sqlErrorLevel),"%s","MINOR");
                fdb_GetError("2022", cpErrorId, cpReason);
            }
            else if(value <= pThreshold->major && value > pThreshold->critical)
            {
                snprintf(sqlErrorLevel, sizeof(sqlErrorLevel), "%s","MAJOR");
                fdb_GetError("2012", cpErrorId, cpReason);
            }
            else
            {
                snprintf(sqlErrorLevel, sizeof(sqlErrorLevel), "%s","CRITICAL");
                fdb_GetError("2002", cpErrorId, cpReason);
            }

            snprintf(sqlErrorId, sizeof(sqlErrorId),"%s",cpErrorId);
            strcat(cpReason,			"[");
            strcat(cpReason,			pReason);
            strcat(cpReason,			"]");
            snprintf(sqlDetail, sizeof(sqlDetail), "%s",cpReason);
        }
    }
    else if(!strcmp(pThreshold->category, "MEMORY"))
    {
        WRITE_INFO(CATEGORY_DB,"MEMORY: value(%d), critical(%d)major(%d)minor(%d)",
                    value,
                    pThreshold->critical,
                    pThreshold->major,
                    pThreshold->minor);

        if(value < pThreshold->minor)
        {
            sqlState	=	'E';
        }
        else
        {
            if(value < pThreshold->major && value >= pThreshold->minor)
            {
                snprintf(sqlErrorLevel, sizeof(sqlErrorLevel), "%s","MINOR");
                fdb_GetError("2021", cpErrorId, cpReason);
            }
            else if(value < pThreshold->critical && value >= pThreshold->major)
            {
                snprintf(sqlErrorLevel, sizeof(sqlErrorLevel), "%s","MAJOR");
                fdb_GetError("2011", cpErrorId, cpReason);
            }
            else
            {
                snprintf(sqlErrorLevel, sizeof(sqlErrorLevel), "%s","CRITICAL");
                fdb_GetError("2001", cpErrorId, cpReason);
            }

            snprintf(sqlErrorId, sizeof(sqlErrorId), "%s", cpErrorId);
            strcat(cpReason,			"[");
            strcat(cpReason,			pReason);
            strcat(cpReason,			"]");
            snprintf(sqlDetail, sizeof(sqlErrorId), "%s", cpReason);

        }
    }
    else if(!strcmp(pThreshold->category, "DISK"))
    {
        WRITE_INFO(CATEGORY_DB,"DISK: value(%d), critical(%d)major(%d)minor(%d)",
                value,
                pThreshold->critical,
                pThreshold->major,
                pThreshold->minor);

        if(value < pThreshold->minor)
        {
            sqlState	=	'E';
        }
        else
        {
            if(value < pThreshold->major && value >= pThreshold->minor)
            {
                snprintf(sqlErrorLevel,sizeof(sqlErrorLevel),"%s","MINOR");
                fdb_GetError("2023", cpErrorId, cpReason);
            }
            else if(value < pThreshold->critical && value >= pThreshold->major)
            {
                snprintf(sqlErrorLevel,sizeof(sqlErrorLevel),"%s","MAJOR");
                fdb_GetError("2013", cpErrorId, cpReason);
            }
            else if(value >= pThreshold->critical)
            {
                snprintf(sqlErrorLevel,sizeof(sqlErrorLevel),"%s","CRITICAL");
                fdb_GetError("2003", cpErrorId, cpReason);
            }

            snprintf(sqlErrorId, sizeof(sqlErrorId), "%s", cpErrorId);
            strcat(cpReason,			"[");
            strcat(cpReason,			pReason);
            strcat(cpReason,			"]");
            snprintf(sqlDetail, sizeof(sqlDetail), "%s", cpReason);
        }
    }
    else if(!strcmp(pThreshold->category, "QUEUE"))
    {
        WRITE_INFO(CATEGORY_DB,"QUEUE: value(%d), critical(%d)major(%d)minor(%d)",
                value,
                pThreshold->critical,
                pThreshold->major,
                pThreshold->minor);

        if(value < pThreshold->minor)
        {
            sqlState	=	'E';
        }
        else
        {
            if(value < pThreshold->major && value >= pThreshold->minor)
            {
                snprintf(sqlErrorLevel, sizeof(sqlErrorLevel), "%s", "MINOR");
                fdb_GetError("2024", cpErrorId, cpReason);
            }
            else if(value < pThreshold->critical && value >= pThreshold->major)
            {
                snprintf(sqlErrorLevel, sizeof(sqlErrorLevel), "%s", "MAJOR");
                fdb_GetError("2014", cpErrorId, cpReason);
            }
            else
            {
                snprintf(sqlErrorLevel, sizeof(sqlErrorLevel), "%s", "MAJOR");
                fdb_GetError("2004", cpErrorId, cpReason);
            }
            snprintf(sqlErrorId, sizeof(sqlErrorId), "%s", cpErrorId);
            strcat(cpReason,			"[");
            strcat(cpReason,			pReason);
            strcat(cpReason,			"]");
            snprintf(sqlDetail, sizeof(sqlDetail), "%s", cpReason);
        }
    }
    else
    {
        WRITE_WARNING(CATEGORY_DB,"Unsupported item(%s)",
                pThreshold->category);
    }

    WRITE_INFO(CATEGORY_DB,"Get sqlState(%c)",sqlState );

    if(sqlState	== 'S')
    {
        /* KBG */
        /*
        insert_smsalarm(sqlMgwId, sqlDetail);
        */

        memset(sqlBuf, 0x00, sizeof(sqlBuf));
        sprintf(sqlBuf, "update SYS_ALARM_TB set "
                        "UPDATEDATE=sysdate(),"
                        "ERRORLEVEL='%s',"
                        "STATE='%c',"
                        "ERRORID='%s',"
                        "DETAIL='%s' "
                        "where MGWID=%d "
                        "and CATEGORY='%s'",
                sqlErrorLevel,
                sqlState,
                sqlErrorId,
                sqlDetail,
                sqlMgwId,
                sqlCategory
        );


        updateCnt = fdb_SqlQuery2(g_stMyCon, sqlBuf);

        if(g_stMyCon->nErrCode != 0)
        {
            WRITE_CRITICAL(CATEGORY_DB, "Fail in query, errcode(%d)Msg(%s)",
                    g_stMyCon->nErrCode,
                    g_stMyCon->cpErrMsg);

            return -1;
        }

//        fdb_SqlCommit(g_stMyCon);
    }
    else
    {
        memset(sqlBuf, 0x00, sizeof(sqlBuf));
        sprintf(sqlBuf, "update SYS_ALARM_TB set "
                        "UPDATEDATE=sysdate(), "
                        "STATE='%c' "
                        "where MGWID=%d "
                        "and CATEGORY='%s'",
                sqlState,
                sqlMgwId,
                sqlCategory
        );

        updateCnt = fdb_SqlQuery2(g_stMyCon, sqlBuf);

        if(g_stMyCon->nErrCode != 0)
        {

            WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d)Msg(%s)",
                    g_stMyCon->nErrCode,
                    g_stMyCon->cpErrMsg);

            return -1;
        }

//        fdb_SqlCommit(g_stMyCon);
    }


    if(sqlState == 'S')
    {
        memset(sqlBuf, 0x00, sizeof(sqlBuf));
        sprintf(sqlBuf, "select count(*) from SYS_OMC_ERRORLOG_TB "
                        "where MGWID=%d and FLAG='S' and ERRORID='%s'",
                sqlMgwId,sqlErrorId);

        rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
        if(g_stMyCon->nErrCode != 0)
        {
            WRITE_CRITICAL(CATEGORY_DB, "Fail in query, errcode(%d)Msg(%s)",
                    g_stMyCon->nErrCode,
                    g_stMyCon->cpErrMsg);

            return -1;
        }

        if(rowCnt > 0)
        {
            if(fdb_SqlFetchRow(g_stMyCon) == 0)
            {
                resCnt = atoi(g_stMyCon->row[0]);
            }
        }

        fdb_SqlFreeResult(g_stMyCon);

        if(resCnt == 0)
        {
            memset(sqlBuf, 0x00, sizeof(sqlBuf));
            sprintf(sqlBuf, "insert into SYS_OMC_ERRORLOG_TB"
                            "("
                            "CREATEDATE,"
                            "MGWID,"
                            "ERRORID,"
                            "ERRORLEVEL,"
                            "DETAIL,"
                            "CATEGORY,"
                            "FLAG"
                            ") VALUES ("
                            "sysdate(),"
                            "%d,"
                            "'%s',"
                            "'%s',"
                            "'%s',"
                            "'%s',"
                            "'%c'"
                            ")",
                    sqlMgwId,
                    sqlErrorId,
                    sqlErrorLevel,
                    sqlDetail,
                    sqlCategory,
                    sqlState
            );

            insertCnt = fdb_SqlQuery2(g_stMyCon, sqlBuf);
            if(g_stMyCon->nErrCode != 0)
            {
                WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d)msg(%s)",
                        g_stMyCon->nErrCode,
                        g_stMyCon->cpErrMsg);

                return -1;
            }

            if(insertCnt > 0)
            {
                WRITE_INFO(CATEGORY_DB,"Insert SYS_OMC_ERRORLOG_TB, count(%d)",
                        insertCnt );
            }
//            fdb_SqlCommit(g_stMyCon);
        }
    }
    else
    {

        memset(sqlBuf, 0x00, sizeof(sqlBuf));
        sprintf(sqlBuf, "update SYS_OMC_ERRORLOG_TB set "
                        "ENDDATE=sysdate(), "
                        "FLAG='%c' "
                        "where MGWID=%d and FLAG='S' and CATEGORY='%s'",
                sqlState,sqlMgwId,sqlCategory);

        updateCnt = fdb_SqlQuery2(g_stMyCon, sqlBuf);
        if(g_stMyCon->nErrCode != 0)
        {
            WRITE_CRITICAL(CATEGORY_DB, "Fail in query, errcode(%d)msg(%s)",
                    g_stMyCon->nErrCode,
                    g_stMyCon->cpErrMsg);

            return -1;
        }

        if(updateCnt > 0)
            WRITE_INFO(CATEGORY_DB,"Update SYS_OMC_ERRORLOG_TB, count(%d)",updateCnt );

//        fdb_SqlCommit(g_stMyCon);
    }

    return		1;
}