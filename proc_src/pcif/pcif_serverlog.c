//
// Created by KimByoungGook on 2020-10-12.
//
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <sys/epoll.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <netinet/if_ether.h>
#include <linux/sockios.h>
#include <sys/socket.h>
#include <dirent.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/poll.h>
#include <sys/epoll.h>

#include <string.h>
#include <dirent.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/crypto.h>



#include "db/dap_mysql.h"
#include "db/dap_checkdb.h"
#include "com/dap_com.h"
#include "com/dap_req.h"
#include "ipc/dap_Queue.h"
#include "sock/dap_sock.h"
#include "json/dap_json.h"

#include "pcif.h"


int	fpcif_DistServerTail(
        char*	logip,
        char*	p_distServerIp,
        int		p_msgCode,
        char*	p_jsonData,
        int		p_jsonSize,
        SSL_Connection**    p_sslcon
)
{
    int			rxt = 0;
    char		caFullPath[296+1]   = {0x00,};
    char		certFullPath[286+1] = {0x00,};
    char		keyFullPath[286+1]  = {0x00,};

    SSL_Connection *sslcon = NULL;

    memset(caFullPath   , 0x00, sizeof(caFullPath));
    memset(certFullPath , 0x00, sizeof(certFullPath));
    memset(keyFullPath  , 0x00, sizeof(keyFullPath));

    sprintf(caFullPath, "%s/%s",
            g_stProcPcifInfo.certPath,
            g_stProcPcifInfo.caFile);
    sprintf(certFullPath, "%s/%s",
            g_stProcPcifInfo.certPath,
            g_stProcPcifInfo.certFile);
    sprintf(keyFullPath, "%s/%s",
            g_stProcPcifInfo.certPath,
            g_stProcPcifInfo.keyFile);
/*
	WRITE_INFO_IP(logip, "- distServerIp(%s)",  p_distServerIp);
	WRITE_INFO_IP(logip, "- listenPort(%d)",  atoi(pcPort));
	WRITE_INFO_IP(logip, "- caFullPath(%s)",  caFullPath);
	WRITE_INFO_IP(logip, "- certFullPath(%s))", certFullPath);
	WRITE_INFO_IP(logip, "- keyFullPath(%s)", keyFullPath);
*/
    // 1. connect (server -> distribution server)
    sslcon = ssl_connect(p_distServerIp, atoi(g_stProcPcifInfo.pcPort), certFullPath, keyFullPath);
    if(sslcon == NULL)
    {
        WRITE_CRITICAL_IP(logip, "Fail in ssl connect to server(%s)", p_distServerIp);
        return -1;
    }

    WRITE_INFO_IP(logip, "[REQ] Succeed in connect to server(%s)code(%d)size(%d)",
                  p_distServerIp,p_msgCode,p_jsonSize );

    // 2. send req json (server -> distribution server)
    rxt = fpcif_SendJsonToServer(sslcon->ssl, p_msgCode, p_jsonData, p_jsonSize);
    if( rxt < 0 )
    {
        ssl_disconnect(sslcon);
        return -1;
    }

    WRITE_INFO_IP(logip, "[REQ] Succeed in send json to server(%s)code(%d)size(%d)",
                  p_distServerIp,p_msgCode,p_jsonSize );

    if(p_sslcon != NULL)
        *p_sslcon = sslcon;


    return 0;
}

int	fpcif_DistServerLog(
        int		sockMng,
        char*	p_mngIp,
        char*	p_distServerIp,
        int		p_msgCode,
        char*	p_jsonData,
        int		p_jsonSize)
{

    int			rxt          = 0;
    int			msgCode      = 0;
    int			msgLeng      = 0;
    int			jsonSize     = 0;
    int			fileSize     = 0;
    int			distWaitTime = 0;
    char		fileName[50 +1]      = {0x00,};
    char		msgType[10 +1]       = {0x00,};
    char		caFullPath[296 +1]   = {0x00,};
    char		certFullPath[286 +1] = {0x00,};
    char		keyFullPath[286 +1]  = {0x00,};
    char		strMgwId[10 +1]      = {0x00,};
    char*		tmpJson = NULL;
    const char*	tmpFileName = NULL;
    SSL_Connection *sslcon = NULL;
    struct		pollfd fds;
    CRhead	RHead;


    memset(caFullPath   , 0x00, sizeof(caFullPath));
    memset(certFullPath , 0x00, sizeof(certFullPath));
    memset(keyFullPath  , 0x00, sizeof(keyFullPath));
    memset(&fds         , 0x00, sizeof(struct pollfd));
    memset(&RHead       , 0x00, sizeof(CRhead));

    sprintf(caFullPath, "%s/%s",
            g_stProcPcifInfo.certPath,
            g_stProcPcifInfo.caFile);
    sprintf(certFullPath, "%s/%s",
            g_stProcPcifInfo.certPath,
            g_stProcPcifInfo.certFile);
    sprintf(keyFullPath, "%s/%s",
            g_stProcPcifInfo.certPath,
            g_stProcPcifInfo.keyFile);

    // 2. connect (server -> distribution server)
    sslcon = ssl_connect(p_distServerIp,
                         atoi(g_stProcPcifInfo.pcPort),
                         certFullPath,
                         keyFullPath);

    if(sslcon == NULL)
    {
        WRITE_CRITICAL_IP(p_mngIp, "Fail in ssl connect to server(%s)", p_distServerIp);
        return -1;
    }

    WRITE_INFO_IP(p_mngIp, "[REQ] Succeed in connect to server(%s)code(%d)size(%d)",
                  p_distServerIp,p_msgCode,p_jsonSize );
    //show_certs_info(sslcon->ssl);

    // 3. send req json (server -> distribution server)
    rxt = fpcif_SendJsonToServer(sslcon->ssl, p_msgCode, p_jsonData, p_jsonSize);
    if( rxt < 0 )
    {
        ssl_disconnect(sslcon);
        return -1;
    }

    WRITE_INFO_IP(p_mngIp, "[REQ] Succeed in send json to server(%s)code(%d)size(%d)",
                  p_distServerIp,p_msgCode,p_jsonSize );

    // 4. recv result (agent -> server)
    fds.fd		= sslcon->sock;
    fds.events	= POLLIN;

    distWaitTime = fcom_GetProfileInt( "PCIF", "DISTRIBUTION_WAIT_TIME", 10);

    rxt = poll(&fds, 1, (distWaitTime*1000));
    if(rxt == 0)
    {
        WRITE_CRITICAL_IP(p_mngIp, "[RCV] Fail in receive poll from server(%s)errno(%d)wait(%d)",
                          p_distServerIp,errno,distWaitTime);
        ssl_disconnect(sslcon);
        return -1;
    }
    else if(rxt > 0)
    {
        memset(&RHead, 0x00, sizeof(RHead));
        rxt = fsock_SslSocketRecv(sslcon->ssl, (char *)&RHead, sizeof(RHead));
        if(rxt <=  0)
        {
            WRITE_CRITICAL_IP(p_mngIp, "[RCV] Fail in recv header from server(%s)errno(%d)",
                              p_distServerIp,errno);
            ssl_disconnect(sslcon);
            return -2;
        }
    }
    else
    {
        WRITE_CRITICAL_IP(p_mngIp, "[RCV] Fail in poll, errno(%d)", errno);
        ssl_disconnect(sslcon);
        return -1;
    }

    memset(msgType, 0x00, sizeof(msgType));
    strcpy(msgType, RHead.msgtype);
    msgLeng = ntohl(RHead.msgleng);
    msgCode = ntohs(RHead.msgcode);

    if(	strncmp(msgType, DAP_AGENT, 10) != 0 )
    {
        WRITE_INFO_IP(p_mngIp, "[RCV] Unknown receive, type(%d)", msgType);
        return -1;
    }

    if(fcom_malloc((void**)&tmpJson, sizeof(char)*((msgLeng-2)+1)) != 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
        return (-1);
    }

    rxt = fsock_SslSocketRecv(sslcon->ssl, tmpJson, msgLeng-2);
    if(rxt <= 0)
    {
        fcom_MallocFree((void**)&tmpJson);
        WRITE_CRITICAL_IP(p_mngIp, "[RCV] Fail in recv json from server(%s)errno(%d)",
                          p_distServerIp,errno);
        ssl_disconnect(sslcon);
        return -1;
    };

    if(rxt != msgLeng-2)
    {
        fcom_MallocFree((void**)&tmpJson);
        WRITE_CRITICAL_IP(p_mngIp, "[RCV] Fail in rxt(%d)<>len(%d)", rxt,msgLeng);
        rxt = fsock_SendAck(sslcon->ssl, msgType, DATACODE_RTN_FAIL,
                            g_stProcPcifInfo.cfgRetryAckCount,
                            g_stProcPcifInfo.retryAckSleep);
        if(rxt < 0)
        {
            WRITE_CRITICAL_IP(p_mngIp, "[ACK] Fail in send ack to server(%s)error(%s)",
                              p_distServerIp,strerror(errno));
        }
        return -1;
    }

    jsonSize = strlen(tmpJson);
//    WRITE_INFO_IP(p_mngIp, "Recv json(%s) ", tmpJson );
    WRITE_INFO_JSON(p_mngIp, "Recv json(%s) ", tmpJson );

    WRITE_INFO_IP(p_mngIp, "[RCV] Succeed in recv json from server(%s)size(%d)",
                  p_distServerIp,jsonSize );

    // 6. recv file binary (server -> 1st server)
    if( msgCode == MANAGE_FORWARD_SERVER_LOGLIST )
    {
        // Send result json (server -> manager)
        rxt = fpcif_SendJsonToManager(sockMng, MANAGE_RSP_LOGLIST, tmpJson, jsonSize);
        if( rxt < 0 )
        {
            WRITE_CRITICAL_IP(p_mngIp, "[RSP] Fail in send json to manager(%s)errno(%s)",
                              p_mngIp,strerror(errno));
            fcom_MallocFree((void**)&tmpJson);
            return -1;
        }
        WRITE_INFO_IP(p_mngIp, "[RSP] Succeed in send json to manager(%s)code(%d)size(%d)",
                      p_mngIp,MANAGE_RSP_LOGLIST,jsonSize );

        fcom_MallocFree((void**)&tmpJson);
    }
    else
    {
        // Send result json (server -> manager)
        rxt = fpcif_SendJsonToManager(sockMng, MANAGE_RSP_LOGFILE, tmpJson, jsonSize);
        if( rxt < 0 )
        {
            WRITE_CRITICAL_IP(p_mngIp, "[RSP] Fail in send json to manager(%s)errno(%s)",
                              p_mngIp,strerror(errno));

            fcom_MallocFree((void**)&tmpJson);
            ssl_disconnect(sslcon);
            return -1;
        }

        WRITE_INFO_IP(p_mngIp, "[RSP] Succeed in send json to manager(%s)code(%d)size(%d)",
                      p_mngIp,MANAGE_RSP_LOGFILE,jsonSize );

        memset(strMgwId, 0x00, sizeof(strMgwId));
//        rxt = fcom_GetTagValue(tmpJson, "{\"", "\": {", strMgwId);
        rxt = fcom_GetTagValue(tmpJson, "{\"", "\": {", strMgwId, sizeof(strMgwId) );

        json_error_t	error;
        json_t *jsonRoot = json_loads(tmpJson, 0, &error);
        if(!jsonRoot)
        {
            WRITE_INFO_IP(p_mngIp, "[ACK] Fail in json parse line(%d)(%s)server(%s)",
                          error.line,error.text,p_distServerIp );

            // 6-1. send ack fail (server -> agent)
            rxt = fsock_SendAck(sslcon->ssl, msgType, DATACODE_RTN_FAIL,
                                g_stProcPcifInfo.cfgRetryAckCount,
                                g_stProcPcifInfo.retryAckSleep);
            if(rxt < 0)
            {
                WRITE_CRITICAL_IP(p_mngIp, "[ACK] Fail in send ack to server(%s)errno(%s)",
                                  p_distServerIp,strerror(errno));
            }

            fcom_MallocFree((void**)&tmpJson);
            ssl_disconnect(sslcon);

            return -1;
        }

        fcom_MallocFree((void**)&tmpJson);

        json_unpack(jsonRoot, "{s:{s:s, s:i}}", strMgwId,"file_name",&tmpFileName,"file_size",&fileSize);
        memset(fileName, 0x00, sizeof(fileName));
        if(tmpFileName != NULL)
        {
            strcpy(fileName, tmpFileName);
            tmpFileName = NULL;
        }
        json_decref(jsonRoot);

        WRITE_INFO_IP(p_mngIp, "Unpack mgw_id: '%s' ", strMgwId );
        WRITE_INFO_IP(p_mngIp, "Unpack file_name: '%s' ", fileName );
        WRITE_INFO_IP(p_mngIp, "Unpack file_size: %d ", fileSize );

        if( strlen(fileName) > 0 && fileSize > 0 )
        {
            char*   tmpBin = NULL;

            // 6-2. recv binary (agent -> server)
            if(fcom_malloc((void**)&tmpBin, sizeof(char)*fileSize) != 0)
            {
                WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
                return (-1);
            }
            rxt = fsock_SslSocketRecv(sslcon->ssl, tmpBin, fileSize);
            if(rxt <= 0)
            {
                fcom_MallocFree((void**)&tmpBin);
                WRITE_CRITICAL_IP(p_mngIp, "[RCV] Fail in recv binary from server(%s)errno(%d)",
                                  p_distServerIp,errno);
                ssl_disconnect(sslcon);
                return -1;
            }
            WRITE_INFO_IP(p_mngIp, "[RCV] Succeed in recv binary from server(%s)size(%d) ",
                          p_distServerIp,fileSize );

            // Send file binary (server -> manager)
            rxt = fpcif_SendToBinary(sockMng, tmpBin, fileSize);
            if( rxt < 0 )
            {
                WRITE_INFO_IP(p_mngIp, "[RSP] Fail in send binary to manager(%s)code(%d)size(%d) ",
                              p_mngIp,MANAGE_RSP_LOGLIST,fileSize );
            }
            else
            {
                WRITE_INFO_IP(p_mngIp, "[RSP] Succeed in send binary to manager(%s)code(%d)size(%d) ",
                              p_mngIp,MANAGE_RSP_LOGLIST,fileSize );
            }

            fcom_MallocFree((void**)&tmpBin);
        }
        else if( strlen(fileName) > 0 && fileSize <= 0 )
        {
            WRITE_CRITICAL_IP(p_mngIp, "Invalied file, name(%s)size(%d)server(%s)",
                              fileName,fileSize,p_distServerIp);
            ssl_disconnect(sslcon);

            return -1;
        }
        else
        {
            WRITE_CRITICAL_IP(p_mngIp, "Not found file, name(%s)size(%d)server(%s)",
                              fileName,fileSize,p_distServerIp);
        }
    } // if( msgCode == MANAGE_RSP_LOGFILE )

    ssl_disconnect(sslcon);

    return 0;
}
int fpcif_CopyServerLog(
        char 	*logPath,
        char	*cat,
        char 	*sDate,
        char	*eDate,
        char	*p_tmpLogDir,
        char	*findIp,
        char	*findStr,
        int		*findCnt,
        char	*p_logip
)
{
    int		rxt = 0;
    int		tokenCnt = 0;
    char	command[256 +1] = {0x00,};
    char	tmpDir[256 +1]  = {0x00,};
    char	tmpCat[128 +1]  = {0x00,};
    char	tmpDate[10 +1]  = {0x00,};
    char	tmpIp[15 +1]    = {0x00,};

    char*   ptr          = NULL;
    char*   token        = NULL;
    struct  dirent* dirp = NULL;
    DIR*    pLogDir      = NULL;
    char*   TempPtr      = NULL;
    struct 	stat	statbuf;


    memset(&statbuf, 0x00, sizeof(struct dirent));

    WRITE_DEBUG(CATEGORY_DEBUG,"copy server log : %s ",logPath);

    if(!(pLogDir = opendir(logPath)))
    {
        WRITE_CRITICAL_IP(p_logip, "Fail in server log dir(%s) ", logPath);
        return 0;
    }
    else
    {
        chdir(logPath);
        while((dirp = readdir(pLogDir)) != NULL)
        {
            WRITE_DEBUG(CATEGORY_DEBUG,"copy server log : %s ",dirp->d_name);
            stat(dirp->d_name, &statbuf);
            if (strcmp(dirp->d_name, ".") && strcmp(dirp->d_name, ".."))
            {

                WRITE_DEBUG(CATEGORY_DEBUG,"copy server log : %s ",dirp->d_name);
                //  If category is 'all'
                if(!strcmp(cat, "all"))
                {
                    // If dir
                    if(S_ISDIR(statbuf.st_mode))
                    {
                        memset(tmpDir, 0x00, sizeof(tmpDir));
                        sprintf(tmpDir, "%s%s/", p_tmpLogDir,dirp->d_name);

                        // Exception
                        if( !strcmp(dirp->d_name, "crontab") ||
                            !strcmp(dirp->d_name, "echo") ||
                            !strcmp(dirp->d_name, "eor") ||
                            !strcmp(dirp->d_name, "ex") )
                            continue;

                        WRITE_DEBUG(CATEGORY_DEBUG,"copy server log : %s ",dirp->d_name);

                        if( (ptr = strstr(tmpDir, "pcif")) != NULL ) // If pcif
                        {
                            if( strlen(ptr) <= 5 )
                            {
                                if( access(tmpDir, R_OK) != 0 )
                                {
                                    WRITE_INFO_IP(p_logip, "Make dir(%s) ", tmpDir );
                                    if (mkdir(tmpDir, S_IRUSR|S_IWUSR|S_IXUSR|S_IROTH) != 0)
                                    {
                                        WRITE_CRITICAL_IP(p_logip, "Fail in create directory(%s)",
                                                          tmpDir);
                                    }
                                }
                            }
                        }
                        else
                        {
                            if( access(tmpDir, R_OK) != 0 )
                            {
                                WRITE_INFO_IP(p_logip, "Make dir(%s) ", tmpDir );
                                if (mkdir(tmpDir, S_IRUSR|S_IWUSR|S_IXUSR|S_IROTH) != 0)
                                {
                                    WRITE_CRITICAL_IP(p_logip, "Fail in create directory(%s)",
                                                      tmpDir);
                                }
                            }
                        }
                        ptr = NULL;
                        WRITE_DEBUG(CATEGORY_DEBUG,"copy server log : %s ",dirp->d_name);
                        fpcif_CopyServerLog(dirp->d_name, cat, sDate, eDate,
                                            tmpDir, findIp, findStr, findCnt, p_logip);
                    }
                        // If file
                    else
                    {
                        ptr = strstr(p_tmpLogDir, "pcif");

                        if( ptr != NULL && strlen(ptr+5) > 11 )
                        {
                            memset(tmpDate, 0x00, sizeof(tmpDate));
                            strncpy(tmpDate, ptr+5, 10);
                            fcom_ReplaceAll(tmpDate, "/", "", tmpDate);
                            memset(tmpIp, 0x00, sizeof(tmpIp));
                            strcpy(tmpIp, ptr+16);
                            fcom_ReplaceAll(tmpIp, "/", ".", tmpIp);
                            strcat(tmpIp, dirp->d_name);
                            WRITE_INFO_IP(p_logip, "tmpDate(%s)tmpIp(%s) ",
                                          tmpDate,tmpIp);
                            if( strncmp(tmpDate, sDate, strlen(sDate)) >= 0 &&
                                strncmp(tmpDate, eDate, strlen(eDate)) <= 0 )
                            {
                                if( strlen(findIp) > 0 && strstr(p_tmpLogDir, "pcif") != NULL )
                                {
                                    if( fpcif_ChkFindIp(tmpIp, findIp) == 0 ) //pcif ����
                                    {
                                        ptr = NULL;
                                        continue;
                                    }
                                }
                                if( strlen(findStr) > 0 )
                                {
                                    if( fpcif_ChkFindStr(dirp->d_name, findStr) == 0 )
                                    {
                                        ptr = NULL;
                                        continue;
                                    }
                                }
                                if (access(p_tmpLogDir, R_OK) != 0)
                                {
                                    WRITE_INFO_IP(p_logip, "Make dir(%s) ", p_tmpLogDir );
                                    rxt = fcom_MkPath(p_tmpLogDir, 0755);
                                    if( rxt < 0 )
                                    {
                                        WRITE_CRITICAL_IP(p_logip, "Fail in create directory(%s) ",
                                                          p_tmpLogDir);
                                        ptr = NULL;
                                        continue;
                                    }
                                    else
                                    {
                                        chmod(p_tmpLogDir, S_IRUSR|S_IWUSR|S_IXUSR|S_IROTH);
                                    }
                                }
                                memset	(command, 0x00, sizeof(command));
                                sprintf	(command, "cp %s %s", dirp->d_name, p_tmpLogDir);
                                WRITE_INFO_IP(p_logip, "Copy file(%s)(%d) ",
                                              command,*findCnt );
                                system	(command);
                                *findCnt = *findCnt + 1;
                            }
                        }
                        else // If not pcif
                        {
                            // compare date
                            ptr = strrchr(dirp->d_name, '.');
                            if( ptr != NULL && (strncmp(ptr+1, sDate, strlen(sDate)) >= 0 &&
                                                strncmp(ptr+1, eDate, strlen(eDate)) <= 0) ) {
                                //LogRet("--> findStr(%s)", findStr);
                                if( strlen(findStr) > 0 )
                                {
                                    if( fpcif_ChkFindStr(dirp->d_name, findStr) == 0 )
                                    {
                                        ptr = NULL;
                                        continue;
                                    }
                                }
                                if (access(p_tmpLogDir, R_OK) != 0)
                                {
                                    WRITE_INFO_IP(p_logip, "Make dir(%s) ", p_tmpLogDir );
                                    if (mkdir(p_tmpLogDir, S_IRUSR|S_IWUSR|S_IXUSR|S_IROTH) != 0)
                                    {
                                        WRITE_CRITICAL_IP(p_logip, "Fail in create directory(%s) ",
                                                          p_tmpLogDir);
                                        ptr = NULL;
                                        continue;
                                    }
                                }
                                memset	(command, 0x00, sizeof(command));
                                sprintf	(command, "cp %s %s", dirp->d_name, p_tmpLogDir);
                                WRITE_INFO_IP(p_logip, "Copy file(%s)(%d) ",
                                              command,*findCnt );
                                system	(command);
                                *findCnt = *findCnt + 1;
                            }
                        }

                        ptr = NULL;
                    }
                }
                    // If category is 'pcif,dbif...'
                else
                {
                    // If dir
                    if(S_ISDIR(statbuf.st_mode))
                    {
                        // Exception
                        if( !strcmp(dirp->d_name, "crontab") ||
                            !strcmp(dirp->d_name, "echo") ||
                            !strcmp(dirp->d_name, "eor") ||
                            !strcmp(dirp->d_name, "ex") )
                        {
                            continue;
                        }

                        memset(tmpDir, 0x00, sizeof(tmpDir));
                        sprintf(tmpDir, "%s%s/", p_tmpLogDir,dirp->d_name);

                        // pcif 일별 나올때까지 돌린다.
                        if( (ptr = strstr(p_tmpLogDir, "pcif")) != NULL )
                        {
                            ptr = NULL;
                            fpcif_CopyServerLog(dirp->d_name, cat, sDate, eDate,
                                                tmpDir, findIp, findStr, findCnt, p_logip);

                            ptr = NULL;
                        }

                        tokenCnt = fcom_TokenCnt(cat, ",");
                        if( tokenCnt > 0 )
                        {
                            memset(tmpCat, 0x00, sizeof(tmpCat));
                            strcpy(tmpCat, cat);
                            token = strtok_r(tmpCat, ",",&TempPtr);
                            while(token != NULL)
                            {
                                if(!strcmp(token, dirp->d_name))
                                {
                                    if (access(tmpDir, R_OK) != 0)
                                    {
                                        WRITE_INFO_IP(p_logip, "Make dir(%s) ", tmpDir );
                                        rxt = fcom_MkPath(tmpDir, 0755);
                                        if( rxt < 0 )
                                        {
                                            WRITE_CRITICAL_IP(p_logip, "Fail in create directory(%s) ",
                                                              tmpDir);
                                        }
                                        else
                                        {
                                            chmod(tmpDir, S_IRUSR|S_IWUSR|S_IXUSR|S_IROTH);
                                        }
                                    }
                                    fpcif_CopyServerLog(dirp->d_name, token, sDate, eDate,
                                                        tmpDir, findIp, findStr, findCnt, p_logip);
                                }
                                token = strtok_r(NULL, ",",&TempPtr);
                            }
                        }
                        else
                        {
                            if(!strcmp(cat, dirp->d_name))
                            {
                                memset(tmpDir, 0x00, sizeof(tmpDir));
                                sprintf(tmpDir, "%s%s/", p_tmpLogDir,dirp->d_name);
                                WRITE_INFO_IP(p_logip, "Dir(%s) ", tmpDir );
                                if (access(tmpDir, R_OK) != 0)
                                {
                                    WRITE_INFO_IP(p_logip, "Make dir(%s)", tmpDir);
                                    rxt = fcom_MkPath(tmpDir, 0755);
                                    if( rxt < 0 )
                                    {
                                        WRITE_CRITICAL_IP(p_logip, "Fail in create directory(%s)",
                                                          tmpDir);
                                    }
                                    else
                                    {
                                        chmod(tmpDir, S_IRUSR|S_IWUSR|S_IXUSR|S_IROTH);
                                    }
                                }
                                fpcif_CopyServerLog(dirp->d_name, cat, sDate, eDate,
                                                    tmpDir, findIp, findStr, findCnt, p_logip);
                            }
                        }
                    }
                        // If file
                    else
                    {
                        ptr = strstr(p_tmpLogDir, "pcif");

                        if( ptr != NULL && strlen(ptr+5) > 11 ) // If pcif/2019/10/28
                        {
                            memset(tmpDate, 0x00, sizeof(tmpDate));
                            strncpy(tmpDate, ptr+5, 10);
                            fcom_ReplaceAll(tmpDate, "/", "", tmpDate);
                            memset(tmpIp, 0x00, sizeof(tmpIp));
                            strcpy(tmpIp, ptr+16);
                            fcom_ReplaceAll(tmpIp, "/", ".", tmpIp);
                            strcat(tmpIp, dirp->d_name);
                            WRITE_INFO_IP(p_logip, "- tmpDate(%s)tmpIp(%s)",
                                          tmpDate,tmpIp);
                            if( strncmp(tmpDate, sDate, strlen(sDate)) >= 0 &&
                                strncmp(tmpDate, eDate, strlen(eDate)) <= 0 )
                            {
                                if( strlen(findIp) > 0 && strstr(p_tmpLogDir, "pcif") != NULL )
                                {
                                    if( fpcif_ChkFindIp(tmpIp, findIp) == 0 ) //pcif ����
                                    {
                                        ptr = NULL;
                                        continue;
                                    }
                                }
                                if( strlen(findStr) > 0 )
                                {
                                    if( fpcif_ChkFindStr(dirp->d_name, findStr) == 0 )
                                    {
                                        ptr = NULL;
                                        continue;
                                    }
                                }
                                if (access(p_tmpLogDir, R_OK) != 0)
                                {
                                    WRITE_INFO_IP(p_logip, "Make dir(%s) ", p_tmpLogDir );
                                    rxt = fcom_MkPath(p_tmpLogDir, 0755);
                                    if( rxt < 0 )
                                    {
                                        WRITE_CRITICAL_IP(p_logip, "Fail in create directory(%s) ",
                                                          p_tmpLogDir);
                                        ptr = NULL;
                                        continue;
                                    }
                                    else
                                    {
                                        chmod(p_tmpLogDir, S_IRUSR|S_IWUSR|S_IXUSR|S_IROTH);
                                    }
                                }
                                memset	(command, 0x00, sizeof(command));
                                sprintf	(command, "cp %s %s", dirp->d_name, p_tmpLogDir);
                                WRITE_INFO_IP(p_logip, "Copy file(%s)(%d) ",
                                              command,*findCnt );
                                system	(command);
                                *findCnt = *findCnt + 1;
                            }
                        }
                        else // If not pcif
                        {
                            // compare date
                            ptr = strrchr(dirp->d_name, '.');
                            if( ptr != NULL && (strncmp(ptr+1, sDate, strlen(sDate)) >= 0 &&
                                                strncmp(ptr+1, eDate, strlen(eDate)) <= 0) )
                            {
                                if( strlen(findStr) > 0 )
                                {
                                    if( fpcif_ChkFindStr(dirp->d_name, findStr) == 0 )
                                    {
                                        ptr = NULL;
                                        continue;
                                    }
                                }
                                if (access(p_tmpLogDir, R_OK) != 0)
                                {
                                    WRITE_INFO_IP(p_logip, "Make dir(%s) ", p_tmpLogDir );
                                    if (mkdir(p_tmpLogDir, S_IRUSR|S_IWUSR|S_IXUSR|S_IROTH) != 0)
                                    {
                                        WRITE_CRITICAL_IP(p_logip, "Fail in create directory(%s)",
                                                          p_tmpLogDir);
                                        ptr = NULL;
                                        continue;
                                    }
                                }
                                memset	(command, 0x00, sizeof(command));
                                sprintf	(command, "cp %s %s", dirp->d_name, p_tmpLogDir);
                                WRITE_INFO_IP(p_logip, "Copy file(%s)(%d) ", command,*findCnt );
                                system	(command);
                                *findCnt = *findCnt + 1;
                            }
                        }
                        ptr = NULL;
                    }
                }
            }
            else
            {
                continue;
            }
        } // while

        chdir("..");
        closedir(pLogDir);
    }

    return RET_SUCC;
}

int fpcif_ReqServerLogFile(char* LogPath,
                           char* TarHome ,
                           char* category,
                           char* StartDate,
                           char* EndDate,
                           char* FindIp,
                           char* FindStr,
                           int* FindCnt,
                           char* LogIP)
{
    int     nCategoryCnt = 0;
    int     nLoop        = 0;
    char    strMgwId[4 +1]      = {0x00,};
    char    CopyCategory[32 +1] = {0x00,};
    char*   CategoryToken       = NULL;
    char*   TempPtr             = NULL;

    sprintf(strMgwId,"%d",g_stServerInfo.stDapComnInfo.nCfgMgwId);
    sprintf(CopyCategory,"%s",category);
    nCategoryCnt = fcom_TokenCnt(category,",");

    if(nCategoryCnt > 0)
    {
        CategoryToken = strtok_r(CopyCategory,",",&TempPtr);
        for(nLoop = 0; nLoop <= nCategoryCnt; nLoop++)
        {
            WRITE_DEBUG_IP(LogIP,"Req Server LogFile Copy : [%s] ",CategoryToken);
            fpcif_GetFileNameCopy(LogPath,TarHome, CategoryToken,StartDate,EndDate,FindIp,FindStr,FindCnt,LogIP);
            CategoryToken = strtok_r(NULL,",",&TempPtr);
        }

    }
    else
    {
        fpcif_GetFileNameCopy(LogPath,TarHome, category,StartDate,EndDate,FindIp,FindStr,FindCnt,LogIP);
        WRITE_DEBUG_IP(LogIP,"Req Server LogFile Copy : [%s] ",category);
    }

    return 0;
}
int fpcif_ReqServerLogList(json_t** jsonRootLogList,
                           char* LogPath,
                           char* category,
                           char* StartDate,
                           char* EndDate,
                           char* FindIp,
                           int* categoryCount,
                           char* LogIP)
{
    json_t* value           = NULL;
    char*   CategoryToken = NULL;
    char*   TempPtr       = NULL;
    int     nCategoryCnt = 0;
    int     nLoop = 0;
    char    strMgwId[4+1]       = {0x00,};
    char    CopyCategory[32+1]  = {0x00,};

    int     nFindCnt = 0;


    sprintf(strMgwId,"%d",g_stServerInfo.stDapComnInfo.nCfgMgwId);
    sprintf(CopyCategory,"%s",category);
    nCategoryCnt = fcom_TokenCnt(category,",");
    *jsonRootLogList = json_object();
    json_object_set_new(*jsonRootLogList, strMgwId, json_object()); /* manager ID Json Set */

    if(nCategoryCnt > 0)
    {
        CategoryToken = strtok_r(CopyCategory,",",&TempPtr);

        for(nLoop = 0; nLoop <= nCategoryCnt; nLoop++)
        {
            *categoryCount = *categoryCount + 1;
            json_object_set(json_object_get(*jsonRootLogList, strMgwId), CategoryToken, json_object()); /* category Json Set  */
            fpcif_GetFileName(*jsonRootLogList, LogPath, CategoryToken, StartDate,EndDate,FindIp,"",&nFindCnt,LogIP);
            value = json_integer(nFindCnt);
            json_object_set(json_object_get(json_object_get(*jsonRootLogList, strMgwId), CategoryToken), "size", value); /* File Count Set */
            json_decref(value);
            nFindCnt = 0;
            CategoryToken = strtok_r(NULL,",",&TempPtr);
            value = json_integer(*categoryCount);
            json_object_set(json_object_get(*jsonRootLogList, strMgwId), "size", value); /* Category Count Set */
            json_decref(value);
        }
    }
    else
    {
        *categoryCount = *categoryCount + 1;
        json_object_set(json_object_get(*jsonRootLogList, strMgwId), category, json_object()); /* category Json Set  */
        fpcif_GetFileName(*jsonRootLogList, LogPath, category, StartDate,EndDate,FindIp,"",&nFindCnt,LogIP);
        value = json_integer(nFindCnt);
        json_object_set(json_object_get(json_object_get(*jsonRootLogList, strMgwId), category), "size", value); /* File Count Set */
        json_decref(value);

        value = json_integer(*categoryCount);
        json_object_set(json_object_get(*jsonRootLogList, strMgwId), "size", value);
        json_decref(value);
    }

    return 0;
}

int fpcif_GetFileName(json_t* rootLogList,
                      char* LogPath,
                      char* category,
                      char* StartDate,
                      char* EndDate,
                      char* FindIp,
                      char* FindStr,
                      int*   param_FindCnt,
                      char* LogIP)
{
    int nRet = 0;
    int nBreakCount = 0;

    char szTempPath[256 +1] = {0x00,};
    char szTempFile[256 +1] = {0x00,};
    char szFileDate[10+ 1] = {0x00,};
    char szFileIp[15 +1] = {0x00,};

    char szReplaceStart[32 +1] = {0x00,};
    char szReplaceEnd[32 +1] = {0x00,};
    char szReplaceFileDate[10+1] = {0x00,};
    char szReplaceFileIp[15+1] = {0x00,};
    char strCnt[4] = {0x00,};
    char strKey[4] = {0x00,};
    char *temp = NULL;

    json_t	*value = NULL;
    DIR     *pLogDir = NULL;
    struct  dirent  *dirp = NULL;
    struct  stat  statbuf;

    memset(&statbuf, 0x00, sizeof(struct stat));


    memset(szFileDate, 0x00, sizeof(szFileDate));

    sprintf(strKey,"%d",g_stServerInfo.stDapComnInfo.nCfgMgwId);

    // 카테고리가 pcif가 아니면.
    if(strcmp(category,"pcif") != 0 )
    {
        // $DAP_HOME/log/service
        if(!(pLogDir = opendir(LogPath)))
        {
            WRITE_CRITICAL_IP(LogIP,"Fail opendir %s ",LogPath);
            return 0;
        }
        else
        {
            chdir(LogPath);
            while((dirp = readdir(pLogDir)) != NULL)
            {
                memset(szTempPath, 0x00, sizeof(szTempPath));
                snprintf(szTempPath, sizeof(szTempPath),"%s/%s",LogPath,dirp->d_name);
                stat(szTempPath, &statbuf);
                if (strcmp(dirp->d_name, ".") && strcmp(dirp->d_name, "..")) /* 현재 디렉토리, 상위 디렉토리아니면 */
                {
                    /* 디렉토리면 */
                    if(S_ISDIR(statbuf.st_mode))
                    {
                        memset(szTempPath, 0x00, sizeof(szTempPath));
                        snprintf(szTempPath, sizeof(szTempPath),"%s/%s",LogPath,dirp->d_name);
                        //재귀함수 호출
                        fpcif_GetFileName(rootLogList,szTempPath, category, StartDate, EndDate, FindIp, FindStr, param_FindCnt, LogIP);
                    }
                    /* 파일이면 */
                    else
                    {
                        memset(szTempPath, 0x00, sizeof(szTempPath));
                        snprintf(szTempPath, sizeof(szTempPath),"%s/%s",LogPath,dirp->d_name);
                        temp = strchr(szTempPath,'.');
                        if(temp != NULL && strstr(szTempPath, category) != NULL)
                        {
                            memcpy(szFileDate,temp+1,8);

                            fcom_ReplaceAll(StartDate,"-","",szReplaceStart);
                            fcom_ReplaceAll(EndDate,"-","",szReplaceEnd);

                            /* 파일 날짜 검사 */
                            if(atoi(szReplaceStart) <= atoi(szFileDate) &&
                               atoi(szFileDate)  <= atoi(szReplaceEnd)  &&
                               (strstr(szTempPath,category)) != NULL )
                            {
                                *param_FindCnt = *param_FindCnt + 1;
                                sprintf(strCnt,"%d",*param_FindCnt);

                                json_object_set(
                                        json_object_get(
                                                json_object_get(rootLogList, strKey), category), strCnt, json_object());

                                sprintf(szTempFile,"%s",szTempPath);
                                value = json_string(szTempFile);

                                json_object_set(
                                        json_object_get(
                                                json_object_get(
                                                        json_object_get(rootLogList, strKey), category), strCnt), "name", value);
                                json_decref(value);

                                stat(szTempPath,&statbuf);

                                value = json_integer(statbuf.st_size);
                                json_object_set(
                                        json_object_get(
                                                json_object_get(
                                                        json_object_get(rootLogList, strKey), category), strCnt), "size", value);
                                json_decref(value);
                                WRITE_DEBUG_IP(LogIP,"Find SERVICE File Name  : [%s] ",szTempPath);
                            }
                        }
                    }
                }
                /* 무한루프 방지 */
                nBreakCount++;
                if(nBreakCount > 1000)
                    break;
            }
        }
    }
    /* 카테고리 pcif */
    if(strcmp(category,"pcif") == 0 )
    {
        if(!(pLogDir = opendir(LogPath)))
        {
            WRITE_CRITICAL(CATEGORY_DEBUG,"Fail opendir %s ",LogPath );
            return 0;
        }
        else
        {
            chdir(LogPath);
            while((dirp = readdir(pLogDir)) != NULL)
            {
                memset(szTempPath, 0x00, sizeof(szTempPath));
                snprintf(szTempPath, sizeof(szTempPath),"%s/%s",LogPath,dirp->d_name);
                stat(szTempPath, &statbuf);
                if (strcmp(dirp->d_name, ".") && strcmp(dirp->d_name, "..")) /* 현재 디렉토리, 상위 디렉토리아니면 */
                {
                    /* 디렉토리면 */
                    if(S_ISDIR(statbuf.st_mode))
                    {
                        memset(szTempPath, 0x00, sizeof(szTempPath));
                        snprintf(szTempPath, sizeof(szTempPath),"%s/%s",LogPath,dirp->d_name);
                        fpcif_GetFileName(rootLogList,szTempPath, category, StartDate, EndDate, FindIp, FindStr, param_FindCnt, LogIP);
                    }
                    /* 파일이면 */
                    else
                    {
                        memset(szTempPath, 0x00, sizeof(szTempPath));
                        snprintf(szTempPath, sizeof(szTempPath),"%s/%s",LogPath,dirp->d_name);
                        temp = strchr(szTempPath,'.');
                        if(temp == NULL && strstr(szTempPath,category) != NULL)
                        {
                            temp = strstr(szTempPath,"pcif");
                            /* File 날짜 파싱 */
                            memcpy(szFileDate,temp+5,sizeof(szFileDate)-1);
                            /* File IP 파싱 */
                            memcpy(szFileIp,temp+16,sizeof(szFileIp)-1);
                            fcom_ReplaceAll(StartDate,"-","",szReplaceStart);
                            fcom_ReplaceAll(EndDate,"-","",szReplaceEnd);
                            fcom_ReplaceAll(szFileIp,"/",".",szReplaceFileIp);
                            fcom_ReplaceAll(szFileDate,"/","",szReplaceFileDate);

                            /* 파일 날짜 검사 */
                            if(atoi(szReplaceStart) <= atoi(szReplaceFileDate) &&
                               atoi(szReplaceFileDate) <= atoi(szReplaceEnd)   &&
                               (strstr(szTempPath,category)) != NULL            )
                            {
                                /* IP 검사 */
                                if(strcmp(FindIp,szReplaceFileIp) == 0)
                                {
                                    *param_FindCnt = *param_FindCnt + 1;
                                    sprintf(strCnt,"%d",*param_FindCnt);

                                    sprintf(szTempFile,"%s",szTempPath);

                                    value = json_string(szTempFile);
                                    json_object_set(
                                            json_object_get(
                                                    json_object_get(rootLogList, strKey), category), strCnt, json_object());

                                    json_object_set(
                                            json_object_get(
                                                    json_object_get(
                                                            json_object_get(rootLogList, strKey), category), strCnt), "name", value);
                                    json_decref(value);

                                    if((nRet = stat(szTempPath,&statbuf)) != 0)
                                    {
                                        WRITE_DEBUG_IP(LogIP,"Get Log FileList stat error %s ",strerror(errno));
                                        value = json_integer(0);
                                    }
                                    else
                                        value = json_integer(statbuf.st_size);

                                    json_object_set(
                                            json_object_get(
                                                    json_object_get(
                                                            json_object_get(rootLogList, strKey), category), strCnt), "size", value);
                                    json_decref(value);
                                    WRITE_DEBUG_IP(LogIP,"Find PCIF File Name  : [%s] ",szTempPath);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    closedir(pLogDir);
    return 0;
}


int fpcif_GetFileNameCopy(
        char* LogPath,
        char* TarHome,
        char* category,
        char* StartDate,
        char* EndDate,
        char* FindIp,
        char* FindStr,
        int* FindCnt,
        char* LogIP)
{
    char szTempPath[256+1] = {0x00,};
    char szFileDate[10+1] = {0x00,};
    char szFileIp[15+1] = {0x00,};

    char szReplaceStart[32] = {0x00,};
    char szReplaceEnd[32] = {0x00,};
    char szReplaceFileDate[10+1] = {0x00,};
    char szReplaceFileIp[15+1] = {0x00,};

    char szCmd[256+1] = {0x00,};
    DIR     *pLogDir = NULL;
    struct  dirent  *dirp = NULL;
    char *temp = NULL;
    struct  stat  statbuf;



    memset(szFileDate, 0x00, sizeof(szFileDate));
    memset(&statbuf, 0x00, sizeof(struct stat));

    // 카테고리가 pcif가 아니면.
    if(strcmp(category,"pcif") != 0)
    {
        // $DAP_HOME/log/service
        if(!(pLogDir = opendir(LogPath)))
        {
            WRITE_CRITICAL_IP(LogIP,"Fail opendir %s ",LogPath);
            return 0;
        }
        else
        {
            chdir(LogPath);
            while((dirp = readdir(pLogDir)) != NULL)
            {
                memset(szTempPath, 0x00, sizeof(szTempPath));
                sprintf(szTempPath, "%s/%s",LogPath,dirp->d_name);
                stat(szTempPath, &statbuf);
                if (strcmp(dirp->d_name, ".") && strcmp(dirp->d_name, "..")) /* ���� ���丮, ���� ���丮�ƴϸ� */
                {
                    /* 디렉토리면 */
                    if(S_ISDIR(statbuf.st_mode))
                    {
                        memset(szTempPath, 0x00, sizeof(szTempPath));
                        snprintf(szTempPath, sizeof(szTempPath),"%s/%s",LogPath,dirp->d_name);

                        /* 재귀함수 호출 */
                        fpcif_GetFileNameCopy(szTempPath,TarHome, category, StartDate, EndDate, FindIp, FindStr, FindCnt, LogIP);
                    }
                    /* 파일이면 */
                    else
                    {
                        memset(szTempPath, 0x00, sizeof(szTempPath));
                        snprintf(szTempPath, sizeof(szTempPath),"%s/%s",LogPath,dirp->d_name);
                        temp = strchr(szTempPath,'.');
                        if(temp != NULL && strstr(szTempPath,category) != NULL)
                        {
                            memcpy(szFileDate,temp+1,8);

                            fcom_ReplaceAll(StartDate,"-","",szReplaceStart);
                            fcom_ReplaceAll(EndDate,"-","",szReplaceEnd);
                            /* 파일 날짜 검사 */
                            if(atoi(szReplaceStart) <= atoi(szFileDate) &&
                               atoi(szFileDate)  <= atoi(szReplaceEnd)  &&
                               (strstr(szTempPath,category)) != NULL     )
                            {
                                *FindCnt = *FindCnt + 1;
                                snprintf(szCmd,sizeof(szCmd),"cp %s %s",szTempPath,TarHome);
                                system(szCmd);
                            }
                        }
                    }
                }
            }
        }
    }

    /* 카테고리 pcif */
    if(strcmp(category,"pcif") == 0)
    {
        if(!(pLogDir = opendir(LogPath)))
        {
            WRITE_CRITICAL_IP(LogIP,"Fail opendir %s ",LogPath);
            return 0;
        }
        else
        {
            chdir(LogPath);
            while((dirp = readdir(pLogDir)) != NULL)
            {
                memset(szTempPath, 0x00, sizeof(szTempPath));
                sprintf(szTempPath, "%s/%s",LogPath,dirp->d_name);
                stat(szTempPath, &statbuf);
                if (strcmp(dirp->d_name, ".") && strcmp(dirp->d_name, "..")) /* 현재 디렉토리, 상위 디렉토리아니면 */
                {
                    /* 디렉토리면 */
                    if(S_ISDIR(statbuf.st_mode))
                    {
                        snprintf(szTempPath, sizeof(szTempPath),"%s/%s",LogPath,dirp->d_name);
                        /* 재귀함수 호출 */
                        fpcif_GetFileNameCopy(szTempPath,TarHome, category, StartDate, EndDate, FindIp, FindStr, FindCnt, LogIP);
                    }
                    /* 파일이면 */
                    else
                    {
                        snprintf(szTempPath, sizeof(szTempPath),"%s/%s",LogPath,dirp->d_name);
                        temp = strchr(szTempPath,'.');
                        if(temp == NULL && strstr(szTempPath,category))
                        {
                            temp = strstr(szTempPath,"pcif");
                            /* File 날짜 파싱 */
                            memcpy(szFileDate,temp+5,sizeof(szFileDate)-1);
                            /* File IP 파싱 */
                            memcpy(szFileIp,temp+16,sizeof(szFileIp)-1);
                            fcom_ReplaceAll(StartDate,"-","",szReplaceStart);
                            fcom_ReplaceAll(EndDate,"-","",szReplaceEnd);
                            fcom_ReplaceAll(szFileIp,"/",".",szReplaceFileIp);
                            fcom_ReplaceAll(szFileDate,"/","",szReplaceFileDate);

                            /* 파일 날짜 검사 */
                            if(atoi(szReplaceStart) <= atoi(szReplaceFileDate) &&
                               atoi(szReplaceFileDate) <= atoi(szReplaceEnd)   &&
                               (strstr(szTempPath,category)) != NULL            )
                            {
                                /* IP 검사 */
                                if(strcmp(FindIp,szReplaceFileIp) == 0)
                                {
                                    *FindCnt = *FindCnt + 1;
                                    snprintf(szCmd,sizeof(szCmd),"cp %s %s",szTempPath,TarHome);
                                    system(szCmd);
                                    WRITE_DEBUG_IP(LogIP,"Find PCIF File Name  : [%s] ",szTempPath);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    closedir(pLogDir);
    return 0;
}

