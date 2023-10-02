//
// Created by KimByoungGook on 2021-03-11.
//
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <pthread.h>

#include "com/dap_com.h"
#include "ipc/dap_Queue.h"
#include "ipc/dap_qdef.h"

#include "db/dap_mysql.h"
#include "db/dap_checkdb.h"
#include "com/dap_req.h"
#include "json/dap_json.h"
#include "ipc/dap_Queue.h"
#include "sock/dap_sock.h"
#include "frd.h"
#include "dap_version.h"

/** 마지막 FileName의 Index를 가져온다. **/
int frd_GetLastFileIndex(char* param_FileName)
{
    int  local_LastIndex = 0;
    int  local_nFileIndex = 0;
    int  local_nForkIdx = 0;

    char local_szFileName[32 +1]    = {0x00,};
    char local_szFileIndex[5 +1]    = {0x00,};
    char local_szFilePath[256 +1]   = {0x00,};

    DIR* local_pDir                 = NULL;
    struct dirent* local_pstDirent  = NULL;
    char* ptrTok                    = NULL;

    local_nForkIdx = g_nForkIdx;

    if ( local_nForkIdx == FORK_POLICY )
    {
        snprintf( local_szFilePath, sizeof(local_szFilePath), "%s", g_stProcFrdInfo.szPolicyFilePath);
    }
    else if( local_nForkIdx == FORK_SERVICE )
    {
        snprintf( local_szFilePath, sizeof(local_szFilePath), "%s", g_stProcFrdInfo.szPolicyFilePath);
    }
    else
    {
        snprintf( local_szFilePath, sizeof(local_szFilePath), "%s", g_stProcFrdInfo.szDtFilePath);
    }


    if ( (local_pDir = opendir(local_szFilePath) ) == NULL)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG, "Open Dir Failed ");
        return (-1);
    }

    while( (local_pstDirent = readdir(local_pDir)) != NULL)
    {
        /** . 상위디렉토리 파일 Skip **/
        if(local_pstDirent->d_name[0] == '.')
            continue;

        if( memcmp(local_pstDirent->d_name, param_FileName,8) != 0)
            continue;

        memset(local_szFileName,    0x00, sizeof(local_szFileName));
        memset(local_szFileIndex,   0x00, sizeof(local_szFileIndex));

        snprintf(local_szFileName, sizeof(local_szFileName), "%s", local_pstDirent->d_name);

        strtok_r(local_szFileName, ".", &ptrTok);
        snprintf(local_szFileIndex, sizeof(local_szFileIndex), "%s", ptrTok);
        local_nFileIndex = atoi(local_szFileIndex);


        if ( local_LastIndex == 0)
        {
            local_LastIndex = local_nFileIndex;
        }
        else
        {
            if( local_nFileIndex > local_LastIndex )
            {
                local_LastIndex = local_nFileIndex;
            }
        }

    }

    if ( local_pDir != NULL)
    {
        closedir(local_pDir);
    }

    if(local_LastIndex == 0)
        local_LastIndex++;

    return local_LastIndex;

}


/** 날짜와 상관없이 파일명의 제일 처음 인덱스를 반환 **/
int frd_GetFirstFile(char* param_FileName, char* param_MMDD, int param_MMDDSize, char* param_TodayMMDD)
{
    int  local_FirstIndex = 0;
    int  local_nFileIndex = 0;
    int  local_nForkIdx = 0;
    int  local_nMMDD    = 0;
    int  local_nParamTodyMMDD   = 0;

    char local_szFileName[32 +1]    = {0x00,};
    char local_szFileMMDD[4 +1]     = {0x00,};
    char local_szFileIndex[5 +1]    = {0x00,};
    char local_szFilePath[256 +1]   = {0x00,};

    DIR* local_pDir                 = NULL;
    struct dirent* local_pstDirent  = NULL;
    char* ptrTok                    = NULL;
    char* ptrTokReturn              = NULL;

    local_nForkIdx = g_nForkIdx;
    local_nParamTodyMMDD = atoi(param_TodayMMDD);

    if ( local_nForkIdx == FORK_POLICY )
    {
        snprintf( local_szFilePath, sizeof(local_szFilePath), "%s", g_stProcFrdInfo.szPolicyFilePath);
    }
    else
    {
        snprintf( local_szFilePath, sizeof(local_szFilePath), "%s", g_stProcFrdInfo.szDtFilePath);
    }


    if ( (local_pDir = opendir(local_szFilePath) ) == NULL)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG, "Open Dir Failed ");
        return (-1);
    }

    while( (local_pstDirent = readdir(local_pDir)) != NULL)
    {
        /** . 상위디렉토리 파일 Skip **/
        if(local_pstDirent->d_name[0] == '.')
            continue;

        if( memcmp(local_pstDirent->d_name, param_FileName,8) != 0)
            continue;

        memset(local_szFileName,    0x00, sizeof(local_szFileName));
        memset(local_szFileMMDD,    0x00, sizeof(local_szFileMMDD));
        memset(local_szFileIndex,   0x00, sizeof(local_szFileIndex));

        snprintf(local_szFileName, sizeof(local_szFileName), "%s", local_pstDirent->d_name);

        ptrTokReturn = strtok_r(local_szFileName, ".", &ptrTok);
        ptrTokReturn = strtok_r(NULL, "_", &ptrTok);
        snprintf(local_szFileMMDD, sizeof(local_szFileMMDD), "%s",   ptrTokReturn );
        snprintf(local_szFileIndex, sizeof(local_szFileIndex), "%s", ptrTok);
        local_nFileIndex = atoi(local_szFileIndex);
        local_nMMDD      = atoi(local_szFileMMDD);

        if(local_nMMDD < local_nParamTodyMMDD)
        {
            snprintf(param_MMDD, param_MMDDSize, "%s", local_szFileMMDD);
            if( local_nFileIndex < local_FirstIndex )
            {
                local_FirstIndex = local_nFileIndex;
            }
        }
        else /** 일자가 같을 때 **/
        {
            /** 일자 다른 파일이 존재하지 않는 경우만 **/
            if( param_MMDD[0] == 0x00)
            {
                if ( local_FirstIndex == 0)
                {
                    local_FirstIndex = local_nFileIndex;
                }
                else
                {
                    if( local_nFileIndex < local_FirstIndex )
                    {
                        local_FirstIndex = local_nFileIndex;
                    }
                }

            }
        }

    }

    if ( local_pDir != NULL)
    {
        closedir(local_pDir);
    }

    if(local_FirstIndex == 0)
        local_FirstIndex++;

    if(param_MMDD[0] == 0x00)
        snprintf(param_MMDD, param_MMDDSize, "%s", param_TodayMMDD);


    return local_FirstIndex;
}


/** 처음의 FileName의 Index를 가져온다. **/
int frd_GetFirstFileIndex(char* param_FileName)
{
    int  local_FirstIndex = 0;
    int  local_nFileIndex = 0;
    int  local_nForkIdx = 0;

    char local_szFileName[32 +1]    = {0x00,};
    char local_szFileIndex[5 +1]    = {0x00,};
    char local_szFilePath[256 +1]   = {0x00,};

    DIR* local_pDir                 = NULL;
    struct dirent* local_pstDirent  = NULL;
    char* ptrTok                    = NULL;

    local_nForkIdx = g_nForkIdx;


    if ( local_nForkIdx == FORK_POLICY )
    {
        snprintf( local_szFilePath, sizeof(local_szFilePath), "%s", g_stProcFrdInfo.szPolicyFilePath);
    }
    else
    {
        snprintf( local_szFilePath, sizeof(local_szFilePath), "%s", g_stProcFrdInfo.szDtFilePath);
    }


    if ( (local_pDir = opendir(local_szFilePath) ) == NULL)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG, "Open Dir Failed ");
        return (-1);
    }

    while( (local_pstDirent = readdir(local_pDir)) != NULL)
    {
        /** . 상위디렉토리 파일 Skip **/
        if(local_pstDirent->d_name[0] == '.')
            continue;

        if( memcmp(local_pstDirent->d_name, param_FileName,8) != 0)
            continue;

        memset(local_szFileName,    0x00, sizeof(local_szFileName));
        memset(local_szFileIndex,   0x00, sizeof(local_szFileIndex));

        snprintf(local_szFileName, sizeof(local_szFileName), "%s", local_pstDirent->d_name);

        strtok_r(local_szFileName, ".", &ptrTok);
        snprintf(local_szFileIndex, sizeof(local_szFileIndex), "%s", ptrTok);
        local_nFileIndex = atoi(local_szFileIndex);

        if ( local_FirstIndex == 0 )
        {
            local_FirstIndex = local_nFileIndex;
        }
        else
        {
            if( local_nFileIndex < local_FirstIndex )
            {
                local_FirstIndex = local_nFileIndex;
            }
        }
    }

    if ( local_pDir != NULL)
    {
        closedir(local_pDir);
    }

    if(local_FirstIndex == 0)
        local_FirstIndex++;


    return local_FirstIndex;

}

/** 오늘 날짜의 마지막 File Name을 가져온다. **/
int frd_GetFileName(int param_ForkIdx, char* param_FileName, int param_FileNameSize)
{
    char            local_szTmp[32 +1] = {0x00,};
    int             local_nFileIndex = 0;

    /** Fork Index별 검출데이터 파일의 오늘 날짜의 마지막 파일 Index를 가져온다. **/
    if ( param_ForkIdx == FORK_POLICY )
    {
        local_nFileIndex = frd_GetLastFileIndex("FW_POLICY");
        snprintf( param_FileName, param_FileNameSize, "%s.%05d",
                  "FW_POLICY", local_nFileIndex);
    }
    else if( param_ForkIdx == FORK_SERVICE )
    {
        local_nFileIndex = frd_GetLastFileIndex("FW_SERVICE");
        snprintf( param_FileName, param_FileNameSize, "%s.%05d",
                  "FW_SERVICE", local_nFileIndex);
    }
    else // param_ForkIdx >= 2
    {
        // 파일번호는 0번부터 시작하므로.
        snprintf(local_szTmp, sizeof(local_szTmp), "%s_%02d", "FW_DT", param_ForkIdx - FORK_DETECT );
        local_nFileIndex = frd_GetFirstFileIndex( local_szTmp );
        snprintf( param_FileName, param_FileNameSize, "%s.%05d",
                  local_szTmp,
                  local_nFileIndex);
    }

    g_stProcFrdInfo.nFileIdx = local_nFileIndex;

    if ( g_stProcFrdInfo.nFileIdx > g_stProcFrdInfo.nCfgMaxFileIndex)
    {
        g_stProcFrdInfo.nFileIdx = 1;
    }

    return 0;

}

int frd_ClearPolicyFile(char* param_FileName ,int param_FileIndex)
{
    int  local_nFileIndex = 0;

    char local_szFileName[32 +1]    = {0x00,};
    char local_szFileIndex[5 +1]    = {0x00,};
    char local_szFilePath[256 +1]   = {0x00,};
    char local_szRemoveFileName[32 +1] = {0x00,};
    char local_szRemoveTemp[256 +1] = {0x00,};

    DIR* local_pDir                 = NULL;
    struct dirent* local_pstDirent  = NULL;
    char* ptrTok                    = NULL;

    snprintf( local_szFilePath, sizeof(local_szFilePath), "%s", g_stProcFrdInfo.szPolicyFilePath);

    if ( (local_pDir = opendir(local_szFilePath) ) == NULL)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG, "Open Dir Failed ");
        return (-1);
    }

    while( (local_pstDirent = readdir(local_pDir)) != NULL)
    {
        /** . 상위디렉토리 파일 Skip **/
        if(local_pstDirent->d_name[0] == '.')
            continue;

        if( memcmp(local_pstDirent->d_name, param_FileName, 8) != 0)
            continue;

        memset(local_szFileName,    0x00, sizeof(local_szFileName));
        memset(local_szFileIndex,   0x00, sizeof(local_szFileIndex));

        snprintf(local_szFileName, sizeof(local_szFileName), "%s", local_pstDirent->d_name);
        snprintf(local_szRemoveFileName, sizeof(local_szRemoveFileName), "%s", local_szFileName);

        strtok_r(local_szFileName, ".", &ptrTok);
        snprintf(local_szFileIndex, sizeof(local_szFileIndex), "%s", ptrTok);
        local_nFileIndex = atoi(local_szFileIndex);


        if( local_nFileIndex < param_FileIndex ) /** 파일 인덱스가 파라미터 인덱스보다 작으면. **/
        {
            snprintf(local_szRemoveTemp, sizeof(local_szRemoveTemp),
                     "%s%s",g_stProcFrdInfo.szPolicyFilePath, local_szRemoveFileName);

            WRITE_DEBUG(CATEGORY_DEBUG, "Delete : [%s] ", local_szRemoveTemp);
            unlink(local_szRemoveTemp);
        }

    }

    if ( local_pDir != NULL)
    {
        closedir(local_pDir);
    }

    return 0;
}

int frd_ClearDtFile(char* param_FileName , int param_FileIndex, char* param_ExceptFileName)
{
    int  local_nFileSize  = 0;

    char local_szFileName[32 +1]    = {0x00,};
    char local_szFileMMDD[4 +1]     = {0x00,};
    char local_szFileIndex[5 +1]    = {0x00,};
    char local_szFilePath[256 +1]   = {0x00,};
    char local_szRemoveTemp[256 +1] = {0x00,};

    DIR* local_pDir                 = NULL;
    struct dirent* local_pstDirent  = NULL;
    FILE*          local_Fp         = NULL;

    snprintf( local_szFilePath, sizeof(local_szFilePath), "%s", g_stProcFrdInfo.szDtFilePath);

    if ( (local_pDir = opendir(local_szFilePath) ) == NULL)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG, "Open Dir Failed ");
        return (-1);
    }

    while( (local_pstDirent = readdir(local_pDir)) != NULL)
    {
        /** . 상위디렉토리 파일 Skip **/
        if(local_pstDirent->d_name[0] == '.')
            continue;

        if( memcmp(local_pstDirent->d_name, param_FileName, 8) != 0)
            continue;

        memset(local_szFileName,    0x00, sizeof(local_szFileName));
        memset(local_szFileMMDD,    0x00, sizeof(local_szFileMMDD));
        memset(local_szFileIndex,   0x00, sizeof(local_szFileIndex));

        snprintf(local_szFileName, sizeof(local_szFileName), "%s", local_pstDirent->d_name);
        snprintf(local_szRemoveTemp, sizeof(local_szRemoveTemp), "%s%s", local_szFilePath, local_szFileName);

        local_Fp = fopen(local_szRemoveTemp, "r");
        if ( local_Fp == NULL)
        {
            WRITE_CRITICAL(CATEGORY_DEBUG,"File Open Failed (%s)", local_szRemoveTemp);
            continue;
        }

        fseek(local_Fp, 0, SEEK_END);
        local_nFileSize = ftell(local_Fp);

        /** 파일명은 같고 파일 사이즈가 0이면 **/
        if ( local_nFileSize == 0 && strcmp(local_pstDirent->d_name, param_ExceptFileName) != 0)
        {
            WRITE_DEBUG(CATEGORY_DEBUG,"Delete : [%s] ", local_pstDirent->d_name );
            unlink(local_szRemoveTemp);
        }
        fclose(local_Fp);
    }

    if ( local_pDir != NULL)
    {
        closedir(local_pDir);
    }

    return 0;
}

int frd_CheckEofFile(FILE* param_Fp, int param_nForkIdx)
{
    int             local_nReadReturnSize     = 0;
    int             local_nReadSize   = 0;
    char            local_szAgentStatus[1 + 1] = {0x00,};
    char            local_szHbSq[20 +1]        = {0x00,};
    _DAP_QUEUE_BUF  local_QueueBuf;



    while( !feof(param_Fp) )
    {
        memset(&local_QueueBuf, 0x00, sizeof(_DAP_QUEUE_BUF) );

        if ( param_nForkIdx == FORK_POLICY )
        {
            local_nReadSize = sizeof(_DAP_AGENT_INFO) + sizeof(local_QueueBuf.packtype);
            local_nReadReturnSize = fread( &local_QueueBuf, sizeof(char), local_nReadSize, param_Fp );
        }
        else if(param_nForkIdx == FORK_SERVICE )
        {
            local_nReadSize = sizeof(_DAP_AGENT_INFO) + sizeof(local_QueueBuf.packtype) + sizeof(local_szAgentStatus) + sizeof(local_szHbSq);
            local_nReadReturnSize = fread( &local_QueueBuf, sizeof(char), local_nReadSize, param_Fp );
        }
        else
        {
            local_nReadReturnSize = fread( &local_QueueBuf, sizeof(char), sizeof(_DAP_QUEUE_BUF), param_Fp );
        }

        WRITE_DEBUG(CATEGORY_DEBUG,"Remain EOF : [%d]", local_nReadReturnSize);

        frd_BufferToDBMS(param_nForkIdx, &local_QueueBuf);
    }

    return 0;
}


int frd_FileRead(int param_ForkIdx, FILE** param_Fp, _DAP_QUEUE_BUF* param_PackBuffer, int* param_PackType, int* FileNextFlag)
{
    int            local_nReadSize      = 0;
    unsigned int   local_nFileSize      = 0;
    int            local_nPolicySize    = 0;

    _DAP_QUEUE_BUF          local_QueueBuf;
    _DAP_AGENT_INFO         local_AgentInfo;

    memset(&local_AgentInfo,    0x00, sizeof(_DAP_AGENT_INFO));
    memset(&local_QueueBuf,     0x00, sizeof(_DAP_QUEUE_BUF));

    while(1)
    {
        if ( param_ForkIdx == FORK_POLICY )
        {
            local_nPolicySize = sizeof(_DAP_AGENT_INFO)+sizeof(local_QueueBuf.packtype);

            fflush(*param_Fp);
            local_nReadSize = fread( &local_QueueBuf, sizeof(char), local_nPolicySize, *param_Fp );

            if(local_nReadSize <= 0) /** Reading Byte Is Zero **/
            {
                local_nFileSize = ftell(*param_Fp); //현재 읽은파일 사이즈 반환.

                if ( local_nFileSize >= g_stProcFrdInfo.nCfgMaxFileSize       ||
                     fcom_fileCheckStatus( g_stProcFrdInfo.szFileNextPath ) == 0 )
                {
                    frd_GetNextFile( param_ForkIdx );

                    /** 파일의 FP가 변경되었으므로, File Stream 포인터 재 참조. **/
                    *param_Fp = g_stProcFrdInfo.Fp;
                    *FileNextFlag = 1;
                }

                fcom_SleepWait(3); // 0.001

                continue;
            }
            else
            {
                if ( local_nReadSize != local_nPolicySize ) /** Reading Error **/
                {
                    WRITE_CRITICAL(CATEGORY_DEBUG,"Data Read Error (%d):(%d)",
                                   local_nReadSize, local_nPolicySize)
                    return (-1);
                }
            }

            memcpy( param_PackBuffer, &local_QueueBuf, sizeof(_DAP_QUEUE_BUF) );

            break;
        }
        else if( param_ForkIdx == FORK_SERVICE )
        {
            char local_szAgentStatus[1 + 1] = {0x00,};
            char local_szHbSq[20 +1]        = {0x00,};
            local_nPolicySize = sizeof(_DAP_AGENT_INFO) +
                                sizeof(local_QueueBuf.packtype) +
                                sizeof(local_szAgentStatus) +
                                sizeof(local_szHbSq);

            fflush(*param_Fp);
            local_nReadSize = fread( &local_QueueBuf, sizeof(char), local_nPolicySize, *param_Fp );

            if(local_nReadSize <= 0) /** Reading Byte Is Zero **/
            {
                local_nFileSize = ftell(*param_Fp); //현재 읽은파일 사이즈 반환.

                if ( local_nFileSize >= g_stProcFrdInfo.nCfgMaxFileSize       ||
                     fcom_fileCheckStatus( g_stProcFrdInfo.szFileNextPath ) == 0 )
                {
                    frd_GetNextFile( param_ForkIdx );

                    /** 파일의 FP가 변경되었으므로, File Stream 포인터 재 참조. **/
                    *param_Fp = g_stProcFrdInfo.Fp;
                    *FileNextFlag = 1;
                }

                fcom_SleepWait(3); // 0.001

                continue;
            }
            else
            {
                if ( local_nReadSize != local_nPolicySize ) /** Reading Error **/
                {
                    WRITE_CRITICAL(CATEGORY_DEBUG,"Data Read Error (%d):(%d)",
                                   local_nReadSize, local_nPolicySize)
                    return (-1);
                }
            }

            memcpy( param_PackBuffer, &local_QueueBuf, sizeof(_DAP_QUEUE_BUF) );

            break;

        }
        else //검출 데이터
        {
            fflush(*param_Fp);
            local_nReadSize = fread( &local_QueueBuf, sizeof(char), sizeof(_DAP_QUEUE_BUF), *param_Fp );

            if(local_nReadSize <= 0) /** Reading Byte Is Zero **/
            {
                local_nFileSize = ftell(*param_Fp); //현재 읽은파일 사이즈 반환.

                if (  local_nFileSize >= g_stProcFrdInfo.nCfgMaxFileSize       ||
                      fcom_fileCheckStatus( g_stProcFrdInfo.szFileNextPath ) == 0)
                {
                    frd_GetNextFile( param_ForkIdx );

                    /** 파일의 FP가 변경되었으므로, File Stream 포인터 재 참조. **/
                    *param_Fp = g_stProcFrdInfo.Fp;
                    *FileNextFlag = 1;
                }

                fcom_SleepWait(3);
                continue;
            }
            else
            {
                if ( local_nReadSize != sizeof(_DAP_QUEUE_BUF) ) /** Reading Error **/
                {
                    WRITE_CRITICAL(CATEGORY_DEBUG, "Data Read Error (%d) (%d)",
                                   local_nReadSize, sizeof(_DAP_QUEUE_BUF))
                    return (-1);
                }
            }

            memcpy( param_PackBuffer, &local_QueueBuf, sizeof(_DAP_QUEUE_BUF) );
            break;
        }
    }

    return local_nReadSize;

}


int frd_GetNextFile(int param_ForkIdx )
{
    char local_szTmp[32 +1] = {0x00,};

    if ( param_ForkIdx == FORK_POLICY)
    {
        if ( g_stProcFrdInfo.Fp != NULL)
        {
            /** 다음 파일 존재여부 체크. **/
            while(1)
            {
                if ( fcom_fileCheckStatus( g_stProcFrdInfo.szFileNextPath ) == 0)
                {
                    WRITE_DEBUG(CATEGORY_DEBUG,"File Exist (%s)", g_stProcFrdInfo.szFileNextPath);
                    break;
                }
                else
                {
                    fcom_SleepWait(5);
                    continue;
                }
            }

            if( g_stProcFrdInfo.Fp != NULL)
            {
                /** 파일의 EOF인지 검사, EOF가 아니면 추가로 File Read 및 DB처리 **/
                frd_CheckEofFile( g_stProcFrdInfo.Fp , param_ForkIdx );

                fflush(g_stProcFrdInfo.Fp);
                fclose(g_stProcFrdInfo.Fp);
                g_stProcFrdInfo.Fp = NULL;
            }

            /** 현재 파일 지움 **/
            unlink(g_stProcFrdInfo.szFileFullPath);

            /** Next 파일명 및 Path 저장 **/
            snprintf( g_stProcFrdInfo.szFileFullPath, sizeof(g_stProcFrdInfo.szFileFullPath),
                      "%s", g_stProcFrdInfo.szFileNextPath);
            snprintf( g_stProcFrdInfo.szFileName, sizeof(g_stProcFrdInfo.szFileName),
                      "%s", g_stProcFrdInfo.szNextFileName);

            g_stProcFrdInfo.Fp = fopen(g_stProcFrdInfo.szFileFullPath, "rb");
            if ( g_stProcFrdInfo.Fp == NULL)
            {
                WRITE_CRITICAL(CATEGORY_DEBUG,"Fopen Failed (%s)", g_stProcFrdInfo.szFileFullPath);
                return (-1);
            }
            WRITE_DEBUG(CATEGORY_DEBUG,"Policy Rename Success : [%s] ", g_stProcFrdInfo.szFileFullPath);

            g_stProcFrdInfo.nNextIdx++;

            if ( g_stProcFrdInfo.nNextIdx > g_stProcFrdInfo.nCfgMaxFileIndex )
            {
                g_stProcFrdInfo.nNextIdx = 1;
            }

            snprintf( g_stProcFrdInfo.szNextFileName, sizeof(g_stProcFrdInfo.szNextFileName),
                      "%s.%05d",
                      "FW_POLICY",
                      g_stProcFrdInfo.nNextIdx);

            snprintf(g_stProcFrdInfo.szFileNextPath,
                     sizeof(g_stProcFrdInfo.szFileNextPath),
                     "%s%s",
                     g_stProcFrdInfo.szPolicyFilePath,
                     g_stProcFrdInfo.szNextFileName);

            WRITE_DEBUG(CATEGORY_DEBUG,"Policy Cur: [%s] Next : [%s]", g_stProcFrdInfo.szFileFullPath, g_stProcFrdInfo.szFileNextPath);
        }
    }
    else if(param_ForkIdx == FORK_SERVICE )
    {
        if ( g_stProcFrdInfo.Fp != NULL)
        {
            /** 다음 파일 존재여부 체크. **/
            while(1)
            {
                if ( fcom_fileCheckStatus( g_stProcFrdInfo.szFileNextPath ) == 0)
                {
                    WRITE_DEBUG(CATEGORY_DEBUG,"File Exist (%s)", g_stProcFrdInfo.szFileNextPath);
                    break;
                }
                else
                {
                    fcom_SleepWait(5);
                    continue;
                }
            }

            if( g_stProcFrdInfo.Fp != NULL)
            {
                /** 파일의 EOF인지 검사, EOF가 아니면 추가로 File Read 및 DB처리 **/
                frd_CheckEofFile( g_stProcFrdInfo.Fp , param_ForkIdx );

                fflush(g_stProcFrdInfo.Fp);
                fclose(g_stProcFrdInfo.Fp);
                g_stProcFrdInfo.Fp = NULL;
            }

            /** 현재 파일 지움 **/
            unlink(g_stProcFrdInfo.szFileFullPath);

            /** Next 파일명 및 Path 저장 **/
            snprintf( g_stProcFrdInfo.szFileFullPath, sizeof(g_stProcFrdInfo.szFileFullPath),
                      "%s", g_stProcFrdInfo.szFileNextPath);
            snprintf( g_stProcFrdInfo.szFileName, sizeof(g_stProcFrdInfo.szFileName),
                      "%s", g_stProcFrdInfo.szNextFileName);

            g_stProcFrdInfo.Fp = fopen(g_stProcFrdInfo.szFileFullPath, "rb");
            if ( g_stProcFrdInfo.Fp == NULL)
            {
                WRITE_CRITICAL(CATEGORY_DEBUG,"File open Failed (%s)", g_stProcFrdInfo.szFileFullPath);
                return (-1);
            }
            WRITE_DEBUG(CATEGORY_DEBUG,"Service Rename Success : [%s] ", g_stProcFrdInfo.szFileFullPath);

            g_stProcFrdInfo.nNextIdx++;

            if ( g_stProcFrdInfo.nNextIdx > g_stProcFrdInfo.nCfgMaxFileIndex )
            {
                g_stProcFrdInfo.nNextIdx = 1;
            }

            snprintf( g_stProcFrdInfo.szNextFileName, sizeof(g_stProcFrdInfo.szNextFileName),
                      "%s.%05d",
                      "FW_SERVICE",
                      g_stProcFrdInfo.nNextIdx);

            snprintf(g_stProcFrdInfo.szFileNextPath,
                     sizeof(g_stProcFrdInfo.szFileNextPath),
                     "%s%s",
                     g_stProcFrdInfo.szPolicyFilePath,
                     g_stProcFrdInfo.szNextFileName);

            WRITE_DEBUG(CATEGORY_DEBUG,"Service Cur: [%s] Next : [%s]", g_stProcFrdInfo.szFileFullPath, g_stProcFrdInfo.szFileNextPath);
        }

    }
    else
    {
        if ( g_stProcFrdInfo.Fp != NULL)
        {
            /** 다음 파일 존재여부 체크. **/
            while(1)
            {
                if ( fcom_fileCheckStatus( g_stProcFrdInfo.szFileNextPath ) == 0)
                {
                    break;
                }
                else
                {
                    fcom_SleepWait(5);
                    continue;
                }
            }

            if ( g_stProcFrdInfo.Fp != NULL)
            {
                /** 파일의 EOF인지 검사, EOF가 아니면 추가로 File Read 및 DB처리 **/
                frd_CheckEofFile( g_stProcFrdInfo.Fp , param_ForkIdx );

                fflush(g_stProcFrdInfo.Fp);
                fclose(g_stProcFrdInfo.Fp);
                g_stProcFrdInfo.Fp = NULL;
            }
            unlink(g_stProcFrdInfo.szFileFullPath);

            /** Next 파일명 및 Path 저장 **/
            snprintf( g_stProcFrdInfo.szFileFullPath, sizeof(g_stProcFrdInfo.szFileFullPath),
                      "%s", g_stProcFrdInfo.szFileNextPath);
            snprintf( g_stProcFrdInfo.szFileName, sizeof(g_stProcFrdInfo.szFileName),
                      "%s", g_stProcFrdInfo.szNextFileName);


            g_stProcFrdInfo.Fp = fopen(g_stProcFrdInfo.szFileFullPath, "rb");
            if ( g_stProcFrdInfo.Fp == NULL)
            {
                WRITE_CRITICAL(CATEGORY_DEBUG,"Fopen Failed (%s)", g_stProcFrdInfo.szFileFullPath);
                return (-1);
            }

            g_stProcFrdInfo.nNextIdx++;

            if ( g_stProcFrdInfo.nNextIdx > g_stProcFrdInfo.nCfgMaxFileIndex )
            {
                g_stProcFrdInfo.nNextIdx = 1;
            }

            snprintf(local_szTmp, sizeof(local_szTmp), "%s_%02d", "FW_DT", param_ForkIdx - FORK_DETECT );
            snprintf( g_stProcFrdInfo.szNextFileName, sizeof(g_stProcFrdInfo.szNextFileName),
                      "%s.%05d",
                      local_szTmp,
                      g_stProcFrdInfo.nNextIdx);

            snprintf(g_stProcFrdInfo.szFileNextPath,
                     sizeof(g_stProcFrdInfo.szFileNextPath),
                     "%s%s",
                     g_stProcFrdInfo.szDtFilePath,
                     g_stProcFrdInfo.szNextFileName);
            WRITE_DEBUG(CATEGORY_DEBUG,"DT Cur: [%s] Next : [%s]", g_stProcFrdInfo.szFileFullPath, g_stProcFrdInfo.szFileNextPath);
        }
    }

    return 0;
}

int frd_GetLastPosition(void)
{
    unsigned int  local_nReadSize = 0;
    char local_szFilePath[256 +1] = {0x00,};
    char local_szFileName[32 +1]  = {0x00,};
    char local_szFileLastPos[8 +1] = {0x00,};
    FILE* local_Fp = NULL;

    snprintf( local_szFileName, sizeof(local_szFileName), "%s_%d", "FW_DT_LAST", g_nForkIdx - FORK_DETECT);
    snprintf( local_szFilePath, sizeof(local_szFilePath), "%s%s", g_stProcFrdInfo.szDtFilePath, local_szFileName );

    local_Fp = fopen(local_szFilePath,"rb");
    if ( local_Fp == NULL)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"File (%s) Open Failed ", g_stProcFrdInfo.szFileFullPath );
        return (-1);
    }

    local_nReadSize = fread(local_szFileLastPos, sizeof(char), sizeof(local_szFileLastPos), local_Fp);

    if ( local_nReadSize >= g_stProcFrdInfo.nCfgMaxFileSize)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"File Size Is Invalid ");
        fclose(local_Fp);
        return (-1);
    }

    if ( local_szFileLastPos[0] == 0x00 || atoi(local_szFileLastPos) <= 0 )
    {
        g_stProcFrdInfo.nLastReadPosition = 0;
    }
    else
    {
        g_stProcFrdInfo.nLastReadPosition = atoi(local_szFileLastPos);
    }

    if ( local_Fp != NULL )
    {
        fclose(local_Fp);
    }


    return 0;
}

int frd_SetLastPosition(void)
{
    int  local_nLastPosition = 0;
    char local_szFilePath[256 +1] = {0x00,};
    char local_szFileName[32 +1]  = {0x00,};
    char local_szFileLastPos[8 +1] = {0x00,};
    FILE* local_Fp = NULL;

    snprintf( local_szFileName, sizeof(local_szFileName), "%s_%d", "FW_DT_LAST", g_nForkIdx - FORK_DETECT);
    snprintf( local_szFilePath, sizeof(local_szFilePath), "%s%s", g_stProcFrdInfo.szDtFilePath, local_szFileName );

    local_Fp = fopen(local_szFilePath,"w");
    if ( local_Fp == NULL)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"File (%s) Open Failed ", g_stProcFrdInfo.szFileFullPath );
        return (-1);
    }

    local_nLastPosition = ftell(g_stProcFrdInfo.Fp);
    snprintf(local_szFileLastPos, sizeof(local_szFileLastPos), "%d", local_nLastPosition);
    fwrite(local_szFileLastPos, strlen(local_szFileLastPos), sizeof(char), local_Fp);

    fclose(local_Fp);

    return 0;

}

void frd_WriteSqlFail(char* param_SqlBuffer, size_t param_SqlBufferLen )
{
    char* local_ptrSqlBuffer = NULL;

    if ( param_SqlBufferLen > 0 )
    {
        fcom_malloc( (void**) &local_ptrSqlBuffer, param_SqlBufferLen +2 );
        memcpy ( local_ptrSqlBuffer, param_SqlBuffer, param_SqlBufferLen );
    }


    if ( g_stProcFrdInfo.SqlFailFp != NULL )
    {
        /** File Write **/
        fprintf( g_stProcFrdInfo.SqlFailFp, "%s;\n", local_ptrSqlBuffer);
        fflush( g_stProcFrdInfo.SqlFailFp);
    }

    fcom_MallocFree( (void**) &local_ptrSqlBuffer );

    return;

}

