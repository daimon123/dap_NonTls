//
// Created by KimByoungGook on 2020-10-22.
//

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pthread.h>

#include "sock/dap_sock.h"
#include "pcif.h"

int fpcif_GetIdxByManagerIpIdSt(char *p_userIp, char* p_userId)
{
    int 	idx = 0;
    int		tokenCnt = 0;
    char* 	tmpIp = NULL;
    char*	tokenIp = NULL;
    char*   ptrRemain = NULL;
    size_t  nUserIdLen = 0;
    size_t  nUserIpLen = 0;

    if(p_userId == NULL)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"User ID Length Is NULL!");
        return (-1);
    }
    else
    {
        nUserIdLen = strlen(p_userId);
    }

    if(p_userIp == NULL)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"User IP Length Is NULL!");
        return (-1);
    }
    else
    {
        nUserIpLen = strlen(p_userIp);
    }


    for(idx=0; idx < g_stProcPcifInfo.nTotManager; idx++)
    {
        tokenCnt = fcom_TokenCnt(pManagerInfo[idx].mn_ipaddr, ",");
        if( tokenCnt > 0 )
        {
            if(fcom_malloc((void**)&tmpIp, sizeof(char)*(strlen(pManagerInfo[idx].mn_ipaddr)+1)) != 0)
            {
                WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
                return (-1);
            }

            snprintf(tmpIp, sizeof(char)*(strlen(pManagerInfo[idx].mn_ipaddr)),"%s",pManagerInfo[idx].mn_ipaddr );
            tokenIp = strtok_r(tmpIp, ",",&ptrRemain);
            while( tokenIp != NULL )
            {
                if ( (memcmp(tokenIp, p_userIp, nUserIpLen) == 0) &&
                     (memcmp(pManagerInfo[idx].mn_id, p_userId, nUserIdLen) == 0) )
                {
                    fcom_MallocFree((void**)&tmpIp);
                    tokenIp = NULL;
                    return idx;
                }
                tokenIp = strtok_r(NULL, ",",&ptrRemain);
            }
            fcom_MallocFree((void**)&tmpIp);
        }
        else
        {
            if ( (memcmp( pManagerInfo[idx].mn_ipaddr, p_userIp, nUserIpLen) == 0) &&
                 (memcmp( pManagerInfo[idx].mn_id    , p_userId, nUserIdLen) == 0)   )
            {
                return idx;
            }
        }
    }

    return -1;
}

int fpcif_GetIdxByConfigSt(char *p_cfName)
{
    int idx;

    for(idx=0; idx<g_stProcPcifInfo.nTotConfig; idx++)
    {
        if(!strcasecmp(pConfigInfo[idx].cfname, p_cfName))
        {
            return idx;
        }
    }

    return -1;
}

int fpcif_GetIdxByUserIp(char *p_userIp)
{
    int 	idx = 0;
    int		tokenCnt = 0;
    char* 	tmpIp = NULL;
    char*	tokenIp = NULL;
    char*   ptrRemain = NULL;
    size_t  nUserIpLen = 0;


    if(p_userIp == NULL)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"User IP Length Is NULL!");
        return (-1);
    }
    else
    {
        nUserIpLen = strlen(p_userIp);
    }


    for(idx=0; idx < g_stProcPcifInfo.nTotUser; idx++)
    {
        tokenCnt = fcom_TokenCnt(pUserInfo[idx].us_ip, ",");
        if( tokenCnt > 0 )
        {
            if(fcom_malloc((void**)&tmpIp, sizeof(char)*(strlen(pUserInfo[idx].us_ip)+1) ) != 0)
            {
                WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
                return (-1);
            }

            snprintf(tmpIp, sizeof(char)*(strlen(pUserInfo[idx].us_ip)),"%s",pUserInfo[idx].us_ip);
            tokenIp = strtok_r(tmpIp, ",",&ptrRemain);
            while( tokenIp != NULL )
            {
                if(memcmp(tokenIp, p_userIp, nUserIpLen) == 0)
                {
                    fcom_MallocFree((void**)&tmpIp);
                    tokenIp = NULL;
                    return idx;
                }
                tokenIp = strtok_r(NULL, ",",&ptrRemain);
            }
            fcom_MallocFree((void**)&tmpIp);
        }
        else
        {
            if(memcmp(pUserInfo[idx].us_ip, p_userIp, nUserIpLen) == 0)
            {
                return idx;
            }
        }
    }

    return -1;
}

int fpcif_GetIdxByUserSno(char *p_sno)
{
    int 	idx = 0;
    size_t  nSnoLen = 0;

    if(p_sno == NULL)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"User SNO Length Is NULL!");
        return (-1);
    }
    else
    {
        nSnoLen = strlen(p_sno);
    }


    for(idx=0; idx < g_stProcPcifInfo.nTotUser; idx++)
    {
        if(memcmp(pUserInfo[idx].us_sno, p_sno, nSnoLen) == 0)
        {
            return idx;
        }
    }

    return -1;
}

int fpcif_GetIdxByManagerSt(char *p_userId)
{
    int idx = 0;
    size_t nMnIdLen = 0;

    if(p_userId == NULL)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Manager Id Length Is NULL!");
        return (-1);
    }
    else
    {
        nMnIdLen = strlen(p_userId);
    }


    for(idx=0; idx < g_stProcPcifInfo.nTotManager; idx++)
    {
        if(memcmp(pManagerInfo[idx].mn_id, p_userId, nMnIdLen) == 0) //��ҹ��ڱ���������
        {
            return idx;
        }
    }

    return -1;
}

int fpcif_GetIdxByGroupLinkSt(int p_targetValue, int p_userGroup)
{
    int idx;

    for(idx=0; idx < g_stProcPcifInfo.nTotGroupLink; idx++)
    {
        if(pGroupLink[idx].ug_sq_p == p_targetValue)
        {
            if(pGroupLink[idx].ug_sq_c == p_userGroup)
            {
                return idx;
            }
        }
    }

    return -1;
}

int fpcif_GetIdxByBaseIp(char *p_ip)
{
    int idx = 0;
    size_t nIpLen = 0;

    if(p_ip == NULL)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"User IP Length Is NULL!");
        return (-1);
    }
    else
    {
        nIpLen = strlen(p_ip);
    }



    for(idx=0; idx < g_stProcPcifInfo.nTotBase; idx++)
    {
        if(memcmp(pBaseInfo[idx].hb_access_ip, p_ip, nIpLen) == 0)
        {
            return idx;
        }
    }

    return -1;
}

int fpcif_GetIdxByBaseKey(char *p_userKey, int ext)
{
    int idx = 0;

    size_t nUserKeyLen = 0;

    if(p_userKey == NULL)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"User Key Length Is NULL!");
        return (-1);
    }
    else
    {
        nUserKeyLen = strlen(p_userKey);
    }


    for(idx=0; idx < g_stProcPcifInfo.nTotBase; idx++)
    {
        if(memcmp(pBaseInfo[idx].hb_unq, p_userKey, nUserKeyLen) == 0)
        {
            if(ext == 1) //External�� ���κ������������°͵� �� ���ڴ�
            {
                return idx;
            }
            else
            {
                //insert_empty_base ��� �� hb_first_time ���� ��ϵ�
                //hb_first_time�� ���ٴ� ���� main_board ������ �ȵ��°�
                if(strlen(pBaseInfo[idx].hb_first_time) > 18)
                {
                    return idx;
                }
            }
        }
    }

    return -1;
}

int fpcif_GetIdxByScheduleSt(unsigned long long p_sq)
{
    int             idx = 0;

    for(idx=0; idx <= pSchdInfo->tot_cnt; idx++) //0���ͽ����ϱ⶧����
    {
        if( pSchdInfo->ru_sq[idx] == p_sq)
        {
            return idx;
        }
    }

    return -1;
}

int fpcif_GetIdxByDetectSt(unsigned long long p_sq, int p_type)
{
    int             idx;

    for(idx=0; idx<=pDetectInfo->tot_cnt; idx++)
    {
        if( pDetectInfo->ru_sq[idx] == p_sq)
        {
            if(p_type == pDetectInfo->rd_type[idx])
            {
                return idx;
            }
        }
    }

    return -1;
}