//
// Created by KimByoungGook on 2020-06-12.
//

#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>

#include "com/dap_com.h"
#include "db/dap_mysql.h"


#include "master.h"

static proc_info  g_stProcTable[HASH_SIZE];


//int fmaster_CheckConfigFlag(void)
//{
//    int nLocalCnt = 0;
//
//    while(1)
//    {
//        if(g_config_flag == 0x01)
//        {
//            nLocalCnt++;
//            if(nLocalCnt > 2000)
//            {
//                return 0;
//            }
//
//            fcom_SleepWait(5);
//            continue;
//        }
//        else
//            break;
//    }
//    return 0;
//
//
//}
int fmaster_GetOneprocessInfo(char* pname,char* parg)
{
    int         pid;
    DIR         *pProcDir;
    struct top_proc  *proc;
    struct dirent    *pDirent;
    char        arg[1024];
    char        procFullName[1024];
    char        *ptr;
    int         pcnt = 0 ;

    pid         =   0;

    memset(procFullName,    0x00,   sizeof(procFullName));
    seteuid(0);
    WRITE_INFO(CATEGORY_DEBUG,"Function Start " );
    if(chdir("/proc"))
    {
        WRITE_MAJOR(CATEGORY_DEBUG,"Fail in chdir error(%d)",
                strerror(errno));

        return      -1;
    }

    if(!(pProcDir = opendir("/proc")))
    {
        WRITE_MAJOR(CATEGORY_DEBUG,"Fail in opendir error(%d)",strerror(errno) );
        return      -1;
    }

    strcat(procFullName,   (const char *)pname);

    if(strcmp((const char *)parg, ""))
    {
        strcat(procFullName, " ");
        strcat(procFullName, (const char *)parg);
    }

    while ((pDirent = readdir(pProcDir)) != NULL)
    {
        if (!isdigit(pDirent->d_name[0]))
            continue;

        pid = atoi(pDirent->d_name);

        /* look up hash table entry */
        proc = (struct top_proc*)&g_stProcTable[HASH(pid)];

        read_one_proc_stat(pid, proc);

        if (proc->state == 0)
            continue;

        memset((char*)arg, 0x00,   sizeof(arg));

        strcpy(arg, proc->name);
        ptr = (char *)strstr(arg, pname);

        if(ptr == NULL)
        {
            continue;
        }

        if(!strncmp((char*)ptr,procFullName ,strlen(procFullName)))
        {
            if(proc->ppid == 1)
                pcnt++;
        }
    }
    closedir(pProcDir);
    WRITE_INFO(CATEGORY_DEBUG,"Function End " );
    return pcnt;
}
int fmaster_ReadPinfo(_PROCESS_INFO *pinfo)
{
    register int	loop;
    int		sqlMgwId		=	0;

    int		rowCnt;
    char	sqlBuf[256];

    memset(sqlBuf, 0x00, sizeof(sqlBuf));

    sqlMgwId = g_stServerInfo.stDapComnInfo.nCfgMgwId;

    sprintf(sqlBuf, "select	MGWID,"
                    "IDX,"
                    "PNAME,"
                    "ARGS,"
                    "MAXNUM,"
                    "ifnull(STATUS, ''),"
                    "ifnull(PID, 0),"
                    "PPID,"
                    "STIME,"
                    "KTIME,"
                    "PCPU,"
                    "PSIZE,"
                    "PPRIORITY,"
                    "ifnull(AUTOFLAG, '') "
                    "from	SYS_PINFO_TB "
                    "where 	MGWID = %d and autoflag = 'A' "
                    "order by PNAME, ARGS", sqlMgwId);

    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);

    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB, "Fail in query, errcode(%d)errmsg(%s)",
                g_stMyCon->nErrCode,
                g_stMyCon->cpErrMsg);

        return -1;
    }

    loop = 0;
    if(rowCnt > 0)
    {
        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            if (g_stMyCon->row[0] != NULL)
                pinfo[loop].mgwid		= atoi(	g_stMyCon->row[0]);
            if (g_stMyCon->row[1] != NULL)
                pinfo[loop].index		= atoi(	g_stMyCon->row[1]);
            if (g_stMyCon->row[2] != NULL)
                strcpy(pinfo[loop].pname,		g_stMyCon->row[2]);
            if (g_stMyCon->row[3] != NULL)
                strcpy(pinfo[loop].args,		g_stMyCon->row[3]);
            if (g_stMyCon->row[4] != NULL)
                pinfo[loop].maxnum		= atol(	g_stMyCon->row[4]);
            if (g_stMyCon->row[5] != NULL)
                pinfo[loop].status		=		*g_stMyCon->row[5];
            if (g_stMyCon->row[6] != NULL)
                pinfo[loop].pid			= atol(	g_stMyCon->row[6]);
            if (g_stMyCon->row[7] != NULL)
                pinfo[loop].ppid		= atol(	g_stMyCon->row[7]);
            if (g_stMyCon->row[8] != NULL)
                pinfo[loop].stime		= atol(	g_stMyCon->row[8]);
            if (g_stMyCon->row[9] != NULL)
                pinfo[loop].ktime		= atol(	g_stMyCon->row[9]);
            if (g_stMyCon->row[10] != NULL)
                pinfo[loop].p_cpu		= atol(	g_stMyCon->row[10]);
            if (g_stMyCon->row[11] != NULL)
                pinfo[loop].p_size		= atol(	g_stMyCon->row[11]);
            if (g_stMyCon->row[12] != NULL)
                pinfo[loop].p_priority	= atoi(	g_stMyCon->row[12]);
            if (g_stMyCon->row[13] != NULL)
                pinfo[loop].autoflag	=		*g_stMyCon->row[13];
            WRITE_DEBUG(CATEGORY_DEBUG,"Process Name : [%s] ",pinfo[loop].pname);
            loop++;
        }
    }
    fdb_SqlFreeResult(g_stMyCon);

    return loop;
}
int fmaster_SaveProcessInfo(void)
{
    g_ptrProcessInfo = (_PROCESS_INFO*)g_stProcessInfo;

    if(fmaster_GetProcessInfo(g_ptrProcessInfo) < 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in get process info" );
        return	-1;
    }

    if(fmaster_UpdatePinfo(g_ptrProcessInfo, gProcess) < 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in update pinfo" );
        return	-2;
    }

    return TRUE;
}
int fmaster_UpdatePinfo(_PROCESS_INFO *pinfo, int nProcess)
{
    register int	loop;
    int			updateCnt = 0;
    int         local_nRet = 0;

    char		sqlBuf[512];


    memset(sqlBuf, 0x00, sizeof(sqlBuf));

    for(loop = 0; loop < nProcess; loop++)
    {
        snprintf(sqlBuf, sizeof(sqlBuf),
                        "update SYS_PINFO_TB set "
                        "STATUS='%c',"
                        "PID=%ld,"
                        "PPID=%ld,"
                        "STIME=%ld,"
                        "KTIME=%ld,"
                        "PCPU=%ld,"
                        "PSIZE=%ld,"
                        "PPRIORITY=%d,"
                        "UPDATETIME=sysdate() "
                        "where MGWID=%d "
                        "and PNAME='%s'"
                        "and ifnull(ARGS, 'NULL')='%s'",
                pinfo[loop].status,
                pinfo[loop].pid,
                pinfo[loop].ppid,
                pinfo[loop].stime,
                pinfo[loop].ktime,
                pinfo[loop].p_cpu,
                pinfo[loop].p_size,
                pinfo[loop].p_priority,
                pinfo[loop].mgwid,
                pinfo[loop].pname,
                pinfo[loop].args
        );

        local_nRet = fdb_SqlQuery2(g_stMyCon, sqlBuf);

        if ( local_nRet < 0 )
        {
            WRITE_CRITICAL(CATEGORY_DB,"Fail in query(%s)", sqlBuf);
            return -1;
        }
        else
        {
            updateCnt += local_nRet;
        }
    }


    WRITE_INFO(CATEGORY_DB,"Update Cnt(%d)",updateCnt );

    return loop;
}

int fmaster_GetProcessInfo(_PROCESS_INFO *pInfo)
{
    int nDbProcCnt  = 0;
    int	nFlagFound  = 0;
    int nPid        = 0;

    char	arg[1024]           = {0x00,};
    char	procFullName[256]   = {0x00,};

    char*   ptr = NULL;
    struct	top_proc* proc       = NULL;
    struct	dirent*   pDirent    = NULL;
    DIR*    pProcDir = NULL;


    if(chdir("/proc"))
    {
        WRITE_CRITICAL(CATEGORY_DEBUG, "Fail in chdir error(%s)",
                       strerror(errno));
        return		-1;
    }

    nPid			=	0;

    for(nDbProcCnt = 0; nDbProcCnt < gProcess; nDbProcCnt++)
    {
        nFlagFound =	FALSE;

        memset((char*)procFullName, 0x00, sizeof(procFullName));
        strcat((char*)procFullName,	(const char *)pInfo[nDbProcCnt].pname);

        if(!(pProcDir = opendir("/proc")))
        {
            WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in opendir error(%s)",
                           strerror(errno));
            closedir(pProcDir);

            return		-1;
        }

        if(strcmp((const char *)pInfo[nDbProcCnt].args, ""))
        {
            strcat(procFullName, " ");
            strcat(procFullName, (const char *)pInfo[nDbProcCnt].args);
        }

        while ((pDirent = readdir(pProcDir)) != NULL)
        {
            if (!isdigit(pDirent->d_name[0]))
                continue;

            nPid = atoi(pDirent->d_name);

            proc = (struct top_proc*)&ptable[HASH(nPid)];
            if(read_one_proc_stat(nPid, proc) == 0)
            {
                continue;
            }

            memset((char*)arg,	0x00,	sizeof(arg));
            strcpy(arg, proc->arg);
            ptr = (char *)strstr(arg, pInfo[nDbProcCnt].pname);

            if(ptr == NULL)
            {
                continue;
            }

            if(!strncmp((char*)ptr,procFullName,strlen(ptr)))
            {
                if(proc->ppid == 1)
                {
                    nFlagFound	            =	TRUE;
                    pInfo[nDbProcCnt].status		=	'S';
                    pInfo[nDbProcCnt].pid		=	nPid;
                    pInfo[nDbProcCnt].ppid		=	proc->ppid;

                    pInfo[nDbProcCnt].stime		=	proc->stime;
                    pInfo[nDbProcCnt].ktime		=	0;
                    pInfo[nDbProcCnt].p_cpu		=	(((double)proc->pcpu)/0x8000*100);
                    pInfo[nDbProcCnt].p_size		=	proc->rss;
                    pInfo[nDbProcCnt].p_priority	=	proc->pri;

                    break;
                }
            }
        }
        closedir(pProcDir);

        if(nFlagFound == FALSE)
        {
            WRITE_DEBUG(CATEGORY_DEBUG,"Not found: kill(%s)",pInfo[nDbProcCnt].pname );
            pInfo[nDbProcCnt].status	= 'K';
        }
    }

    return 1;
}

int fmaster_CheckProcAlarm(_PROCESS_INFO *pInfo, int nProc)
{
    int		rowCnt = 0;
    int		resCnt = 0;
    int		i = 0;
    int		procAlarm 	= 	FALSE;

    int		insertCnt = 0;
    int		updateCnt = 0;
    int		sqlMgwId =	0;
    char	sqlState = 0x00;

    char	cpErrorId[50 +1] = {0x00,};
    char	cpReason[1000 +1] = {0x00,};
    char	sqlCategory[21] = {0x00,};
    char	sqlErrorLevel[11] = {0x00,};
    char	sqlErrorId[50 +1] = {0x00,};
    char	sqlDetail[1001] = {0x00,};
    char	sqlBuf[1024] = {0x00,};

    memset(sqlCategory, 0x00, sizeof(sqlCategory));
    memset(sqlErrorLevel, 0x00, sizeof(sqlErrorLevel));
    memset(sqlErrorId, 0x00, sizeof(sqlErrorId));
    memset(sqlDetail, 0x00, sizeof(sqlDetail));
    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    memset(cpErrorId, 0x00, sizeof(cpErrorId));

    strcpy(sqlCategory,"PROCESS");
    strcpy(sqlErrorLevel,"CRITICAL");
    strcpy(sqlErrorId, cpErrorId);


    sqlMgwId = g_stServerInfo.stDapComnInfo.nCfgMgwId;

    sqlState = 'S';

    fdb_GetError((char *)"1030", cpErrorId, cpReason);

    strcat(cpReason, " [");

    for(i = 0; i < nProc; i++)
    {
        if(pInfo[i].status == 'K')
        {
            WRITE_CRITICAL(CATEGORY_DEBUG,"Process killed[%s][%d]",
                                            pInfo[i].pname,
                                            i);

            procAlarm	=	TRUE;
            strcat(cpReason,		pInfo[i].pname);
            strcat(cpReason,		" ");
        }
    }

    strcat(cpReason, "]");
    strcpy(sqlDetail, cpReason);

    WRITE_INFO(CATEGORY_INFO,"ProcAlarm(%d)",procAlarm);

    if(procAlarm == TRUE)
    {
        sprintf(sqlBuf, "select count(*) from SYS_ALARM_TB "
                        "where MGWID=%d and CATEGORY='%s' and STATE='S'",
                sqlMgwId,sqlCategory);

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
                resCnt = atoi(g_stMyCon->row[0]);
            }
            WRITE_INFO(CATEGORY_DB,"resCnt(%d) ",resCnt );
        }

        fdb_SqlFreeResult(g_stMyCon);

        /* All �ּ�ó�� */
//        insert_smsalarm(sqlMgwId, sqlDetail);

        if(resCnt > 0)
            return 1;

        memset(sqlBuf, 0x00, sizeof(sqlBuf));
        sprintf(sqlBuf, "update	SYS_ALARM_TB set "
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
                        sqlCategory );

        updateCnt = fdb_SqlQuery2(g_stMyCon, sqlBuf);
        if(g_stMyCon->nErrCode != 0)
        {
            WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d),msg(%s)",
                    g_stMyCon->nErrCode,
                    g_stMyCon->cpErrMsg);

            return -1;
        }

        WRITE_INFO(CATEGORY_DB,"Update %s Cnt(%d)", "SYS_ALARM_TB", updateCnt);

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
            WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d),msg(%s)",
                    g_stMyCon->nErrCode,
                    g_stMyCon->cpErrMsg);

            return -1;
        }

        WRITE_INFO(CATEGORY_DB,"insertCnt(%d)", insertCnt);

    }
    else
    {
        memset(sqlBuf, 0x00, sizeof(sqlBuf));
        sprintf(sqlBuf, "select count(*) from SYS_ALARM_TB "
                        "where MGWID=%d AND CATEGORY='%s' AND STATE='E'",
                sqlMgwId,sqlCategory);


        rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
        if(g_stMyCon->nErrCode != 0)
        {
            WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d),msg(%s)",
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

        if(resCnt > 0)
            return 1;

        memset(sqlBuf, 0x00, sizeof(sqlBuf));
        sprintf	(sqlBuf, "update SYS_ALARM_TB set "
                            "UPDATEDATE=sysdate(), STATE='E' "
                            "where MGWID=%d AND CATEGORY='%s'",
                    sqlMgwId,
                    sqlCategory
        );

        updateCnt = fdb_SqlQuery2(g_stMyCon, sqlBuf);
        if(g_stMyCon->nErrCode != 0)
        {
            WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d),msg(%s)",
                    g_stMyCon->nErrCode,
                    g_stMyCon->cpErrMsg);

            return -1;
        }

//        fdb_SqlCommit(g_stMyCon);
    }

    return 0;
}

int fmaster_InvokeProcess(_PROCESS_INFO* pInfo)
{
    char    cpProcFull[511 +1] = {0x00,};
    char    cpProcName[511 +1] = {0x00,};
    int     i;
    int     local_nStartCnt = 0;

    for(i = 0; i < gProcess; i++)
    {
        sprintf(cpProcName, "%s/bin/%s",getenv("DAP_HOME"), pInfo[i].pname);

        if(pInfo[i].status == 'K' && access(cpProcName, X_OK) == 0 && pInfo[i].autoflag == 'A')
        {
            sprintf(cpProcFull, "%s %s &", cpProcName, pInfo[i].args);
            if(pInfo[i].pid != 0)
            {
                kill( pInfo[i].pid, 15);
            }

            WRITE_CRITICAL_DBLOG(g_stServerInfo.stDapComnInfo.szServerIp, CATEGORY_DB,
                                 "Invoke Process name %s ", pInfo[i].pname);

            system(cpProcFull);
            local_nStartCnt++;
        }
    }

    if ( local_nStartCnt > 0)
    {
        sleep(1);
        fmaster_SaveProcessInfo();
    }

    return      TRUE;
}

int fmaster_KillProcess(_PROCESS_INFO *pInfo)
{
    char	cpProc[100];
    int		i;


    for(i = 0; i < gProcess; i++)
    {
        sprintf(cpProc, "%s/bin/%s", g_stServerInfo.stDapComnInfo.szDapHome, pInfo[i].pname);
        switch(pInfo[i].autoflag)
        {
            case 'K' :
            {
                if(pInfo[i].pid != 0 && access(cpProc, X_OK) == 0)
                {
                    if(kill(pInfo[i].pid, 0) == 0)
                    {
                        if(kill(pInfo[i].pid, SIGTERM) < 0)
                        {

                            WRITE_CRITICAL(CATEGORY_DEBUG,"Can't kill process(%s)",
                                    cpProc);
                            return		-1;
                        }
                    }
                }
                break;
            }
            case 'A' :
            case 'N' :
            default  :
                break;
        }
    }

    return TRUE;
}

int fmaster_KillAllProcess(_PROCESS_INFO* pSysInfo)
{
    unsigned long   local_nDblogPid = 0;
    int		i;
    char	cpProc[100] = {0x00,};
    char    local_szDblogProc[100 +1] = {0x00,};

    for(i = 0; i < MAX_PROCESS_COUNT; i++)
    {
        /** dblog process의 경우 다른 프로세스의 log db처리를 위해 맨 나중에 정리한다. **/
        if(strcasecmp(pSysInfo[i].pname, "dap_dblog") == 0)
        {
            snprintf(local_szDblogProc, sizeof(local_szDblogProc),
                     "%s/bin/%s",
                     g_stServerInfo.stDapComnInfo.szDapHome, pSysInfo[i].pname);
            local_nDblogPid = pSysInfo[i].pid;
        }
        else
        {
            memset(cpProc, 0x00, sizeof(cpProc));
            sprintf(cpProc, "%s/bin/%s", g_stServerInfo.stDapComnInfo.szDapHome, pSysInfo[i].pname);
            if(pSysInfo[i].pid != 0 && access(cpProc, X_OK) == 0)
            {
                if(kill(pSysInfo[i].pid, SIGTERM) < 0)
                {
                    WRITE_CRITICAL(CATEGORY_DEBUG,"Can't kill process(%s)pid(%d)signal(%d)",
                                   cpProc,
                                   pSysInfo[i].pid,
                                   SIGTERM);
                }
                else
                {
                    WRITE_INFO(CATEGORY_DEBUG,"Killed process process(%s)pid(%d)signal(%d)",
                               cpProc,
                               pSysInfo[i].pid,
                               SIGTERM);
                }
            }
        }
    }

    sleep(3);

    // dblog process 정리처리
    if(local_nDblogPid != 0 && access(local_szDblogProc, X_OK) == 0)
    {
        if(kill(local_nDblogPid, SIGTERM) < 0)
        {
            WRITE_CRITICAL(CATEGORY_DEBUG,"Can't kill process(%s)pid(%d)signal(%d)",
                           local_szDblogProc,
                           local_nDblogPid,
                           SIGTERM);
        }
        else
        {
            WRITE_INFO(CATEGORY_DEBUG,"Killed process process(%s)pid(%d)signal(%d)",
                       local_szDblogProc,
                       local_nDblogPid,
                       SIGTERM);
        }
    }

    return		RET_SUCC;
}



int fmaster_ReloadConfig(void)
{
    struct stat		stStatBuf;

    memset(&stStatBuf, 0x00, sizeof(struct stat));

    if(g_pstNotiMaster[PROCESS_CHANGE].reload || g_nStartFlag == 0)
    {
        memset(g_stProcessInfo, 0x00, sizeof(g_stProcessInfo));
        g_ptrProcessInfo = (_PROCESS_INFO *)g_stProcessInfo;

        if((gProcess = fmaster_ReadPinfo(g_ptrProcessInfo)) < 0)
        {
            WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in load process config" );
            return	-1;
        }
        stat(g_pstNotiMaster[PROCESS_CHANGE].szNotiFileName, &stStatBuf);
        g_pstNotiMaster[PROCESS_CHANGE].lastModify	=	stStatBuf.st_mtime;
        g_pstNotiMaster[PROCESS_CHANGE].reload		=	FALSE;
        g_nStartFlag = 1;
    }

    return TRUE;
}

