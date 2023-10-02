//
// Created by KimByoungGook on 2020-06-12.
//
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <time.h>
#include <math.h>
#include <sys/vfs.h>
#include <iconv.h>

#include "com/dap_def.h"
#include "com/dap_com.h"
#include "db/dap_mysql.h"
#include "ipc/dap_Queue.h"





/* Daemonisze Fuctnion */
/* Link : https://potatogim.net/wiki/%EB%A6%AC%EB%88%85%EC%8A%A4_%ED%94%84%EB%A1%9C%EA%B7%B8%EB%9E%98%EB%B0%8D/%EB%8D%B0%EB%AA%A8%EB%82%98%EC%9D%B4%EC%A6%88 */
pid_t fcom_SetDeamon(int param_CurLimit, int param_MaxLimit)
{
    pid_t pid = 0;

    if (param_CurLimit < param_MaxLimit)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG, "[ STOP ] because open file limit, curMax=%d < limitMax=%d",
                param_CurLimit,
                param_MaxLimit);
    }
    else
    {
        /* Parent Process */
        if((pid = fork()) > 0)
        {
            printf("[ START ] \n");
            exit(0);
        }
        else
        {
            /* Child Process */
            chdir("/");
            close(0); //or dup(0) or dup2(0,STDOUT_FILENO)
            close(1); // or dup(1) or dup2(1,STDOUT_FILENO)
            close(2); // or dup(2) or dup2(2,STDOUT_FILENO)
            open("/dev/null", O_RDONLY); /* 0 */
            open("/dev/null", O_WRONLY); /* 1 */
            open("/dev/null", O_WRONLY); /* 2 */
            return 0;
        }
    }
    return pid;
}
void fcom_ExitServer(int param_nNum)
{
    fdb_SqlClose(g_stMyCon);
    fipc_FQClose(DBLOG_QUEUE);

    WRITE_CRITICAL(CATEGORY_DEBUG,"Terminated process, signal(%d)error(%s)",
            param_nNum,
            strerror(errno));

    if(g_pstNotiMaster != NULL)
    {
        free(g_pstNotiMaster);
        g_pstNotiMaster = NULL;
    }

    fFile_cleanupMasterPid(g_stServerInfo.stDapComnInfo.szPidPath,"dap_master");

    exit(0);
}

void fcom_BufferFree(char *buf)
{
    if(buf != NULL)
    {
        free(buf);
        buf = NULL;
    }
}

int	fcom_MallocFree ( void **pre_point )
{

    if( *pre_point == NULL )
    {
        return -1;
    }
    else
    {
        free ( *pre_point );
        *pre_point = NULL;
        return 0;
    }
}

int fcom_ThreadCreate(void *threadp, PTHREAD_START start_func, void *arg, size_t nStackSize)
{
    pthread_attr_t attributes;
    size_t          local_StackSize = 0;

    if(pthread_attr_init(&attributes) != 0)
    {
        perror("thread create error : ");
        WRITE_CRITICAL(CATEGORY_DEBUG,"Thread Create Attr Init Fail (%d) (%s) ",errno, strerror(errno) );
        return -1;
    }

    /* Default Stack Size 로깅용 */
//    pthread_attr_getstacksize(&attributes, &local_StackSize);
//    WRITE_DEBUG(CATEGORY_DEBUG,"Default Thread Stack Size : [%ld] |%s",local_StackSize,__func__ );

    /* pthread set 스택 사이즈 */
    if(pthread_attr_setstacksize(&attributes, nStackSize) != 0)
    {
        perror("thread pthread_attr_setstacksize : ");
        WRITE_CRITICAL(CATEGORY_DEBUG,"Thread pthread_attr_setstacksize (%d) (%s) ",errno, strerror(errno) );
        return -1;
    }

    pthread_attr_getstacksize(&attributes, &local_StackSize);
    WRITE_DEBUG(CATEGORY_DEBUG,"Set Thread Stack Size : [%ld] ",local_StackSize );

    if(pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_DETACHED) != 0)
    {
        pthread_attr_destroy(&attributes);
        perror("thread create error : ");
        WRITE_CRITICAL(CATEGORY_DEBUG,"Thread Create Attr Detatch Fail (%d) (%s) ",errno, strerror(errno) );
        return -1;
    }
    if(pthread_create((pthread_t *)threadp, &attributes, start_func, (void *)arg) != 0)
    {
        pthread_attr_destroy(&attributes);
        perror("thread create error : ");
        WRITE_CRITICAL(CATEGORY_DEBUG,"Thread Create pthread_create Fail (%d) (%s) ",errno, strerror(errno) );
        return -1;
    }

    fcom_SleepWait(5);

    if(pthread_attr_destroy(&attributes) != 0)
    {
        perror("thread create error : ");
        WRITE_CRITICAL(CATEGORY_DEBUG,"Thread Create pthread_attr_destroy Fail (%d) (%s) ",errno, strerror(errno) );
        return -1;
    }

    return 0;
}

int fcom_ArgParse(char **argv)
{
    _DAP_COMN_INFO* pstComnInfo;

    char* cptrProcName    = NULL;
    char* cptrSep         = NULL;

    /* ------------------------------------------------------------------------- */
    /* 01. Initialize                                                            */
    /* ------------------------------------------------------------------------- */
    pstComnInfo   = &(g_stServerInfo.stDapComnInfo);
    cptrSep       = NULL;
    cptrProcName  = NULL;

    /* ------------------------------------------------------------------------- */
    /* 02. Do-Tx 									                             */
    /* ------------------------------------------------------------------------- */

    if( (cptrSep = strrchr(argv[0], '/') ) != NULL )
    {
        cptrProcName = (cptrSep + 1);
    }
    else
    {
        cptrProcName = argv[0];
    }

    snprintf(pstComnInfo->szProcName,
             sizeof(pstComnInfo->szProcName  ),
             "%s",
             cptrProcName);
    snprintf(pstComnInfo->szDebugName,
             sizeof(pstComnInfo->szDebugName  ),
             "%s",
             argv[1]);

    printf("Process Name %s : Argument Debug Name : %s \n",
            pstComnInfo->szProcName,
            pstComnInfo->szDebugName);

//    pstComnInfo->nArgMinInterval = 5;

    /* ------------------------------------------------------------------------- */
    /* 03. Finalize 								                             */
    /* ------------------------------------------------------------------------- */

    return 0;

}

int fcom_GetFileRows(char *file)
{
    int retRows = 0;

    char ch = 0x00;
    char oldch = 0x00;

    FILE *fp = fopen(file, "r");
    if (fp == NULL)
        return -1;

    /* 2020.08.14 아래 로직으로 대체 */
//	while(!feof(fp))
//	{
//		ch = fgetc(fp);
//        if(ch == '\n')
//        {
//            retRows++;
//        }
//    }

    for (ch = getc(fp); ; ch = getc(fp))
    {
        if(oldch != '\n' && ch == EOF) /* 파일 마지막 라인 개행 아닌경우 예외로 라인 카운트 추가 */
            retRows = retRows + 1;
        if (ch == '\n') // Increment count if this character is newline
            retRows = retRows + 1;
        if(ch == EOF)  break;
        oldch = ch;
    }

    fclose(fp);

    return retRows;
}

int fcom_GetFileCols(char *file)
{
    int retCols = 0;
    char ch;

    FILE *fp = fopen(file, "r");
    if (fp == NULL)
        return -1;

    while(!feof(fp))
    {
        ch = fgetc(fp);
        if(ch == '\n')
        {
            break;
        }
        else if(ch == '|')
        {
            retCols++;
        }
    }
    if(fp != NULL)
        fclose(fp);

    return retCols + 1;
}

int fcom_GetFileSize(char *fullPath, char *fileName)
{
    int     fsize = 0;
    char	fullFilePath[256 +1] = {0x00,};
    FILE    *fp = NULL;

    memset(fullFilePath, 0x00, sizeof(fullFilePath));
    sprintf(fullFilePath, "%s/%s", fullPath,fileName);

    fp = fopen(fullFilePath, "rb");
    if(fp == NULL)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG, "Fail in fopen, path(%s)", fullFilePath);
        return -1;
    }
    //파일 크기 구하기
    fseek(fp, 0, SEEK_END);
    fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    fclose(fp);

    return fsize;
}
int fcom_IsNumber(char *numStr)
{
    int i = 0;
    int sLen = 0;

    sLen = strlen(numStr);
    for (i=0;i<sLen;i++)
    {
        if ((numStr[i] < '0') || (numStr[i] > '9')) return 0;
    }
    return 1;
}
int fcom_GetBiggerNumber(int a, int b)
{
    return (a > b) ? a : b;
}



char* fcom_Ui2Ip(unsigned int ipAsUInt)
{
    int r = 0;
    char buf[4] = {0x00,};
    char *ip = malloc(16*sizeof(char)); // 사용 시 free() 주의
    int exponent;

    memset(ip, 0x00, 16*sizeof(char));

    for(exponent = 3; exponent >= 0; --exponent)
    {
         r = ipAsUInt / pow(256, exponent);
        sprintf(buf, "%d", r);
        strcat(ip, buf);
        strcat(ip, ".");
        ipAsUInt -= r*pow(256, exponent);
    }
    ip[strlen(ip)-1] = 0;
    return ip;
}

void fcom_GetUniqId(char *p_userKey )
{
    time_t          local_get_now;
    struct  tm      local_tm;
    struct timeval  local_time;
    struct timespec local_reqtime;
    int             local_pid = 0;

    // 0.1 초 100000000
    local_reqtime.tv_sec = 0;
    local_reqtime.tv_nsec = 1000000;
    nanosleep(&local_reqtime, NULL);

    local_pid = getpid();

    local_get_now = time ( NULL );
    localtime_r ( &local_get_now , &local_tm );
    gettimeofday(&local_time, NULL);


    sprintf(p_userKey, "%02d%.2d%.2d%.2d%.2d%.2d%06d%.1d%.1d", (local_tm.tm_year+1900)%100,
            local_tm.tm_mon+1, local_tm.tm_mday, local_tm.tm_hour, local_tm.tm_min, local_tm.tm_sec,
            (int)local_time.tv_usec, (local_pid%100)/10, (local_pid%100)%10);

    return;
}

int fcom_FileCheckModify(char* CheckFilePath, time_t* LastModify)
{
    struct stat		statBuf;

    stat(CheckFilePath,&statBuf);

    if(statBuf.st_mtime > *LastModify)
    {
        *LastModify = statBuf.st_mtime;
        return 1;
    }


    return 0;

}

short fcom_procFileCheckStatus(char* preFilepathFilename, char* processFileName )
{
    struct 	statfs 	local_file_stat;
    char szFullPath[256 +1] = {0x00,};

    memset(&local_file_stat, 0x00, sizeof(struct statfs));

    if(processFileName != NULL)
    {
        sprintf(szFullPath, "%s/%s", preFilepathFilename, processFileName);
    }
    else // processFileName == NULL
        sprintf(szFullPath, "%s", preFilepathFilename);

    if ( statfs ( (char *)szFullPath , &local_file_stat ) == 0 )
    {
        return 0;	 // 파일 있음
    }
    else
    {
        return -1;	// 파일 없음
    }

}


// 파일이 존재하는지 검사한다.
short fcom_fileCheckStatus(char* preFilepathFilename)
{
    struct 	statfs 	local_file_stat;

    memset(&local_file_stat, 0x00, sizeof(struct statfs));


    if ( statfs ( (char *)preFilepathFilename , &local_file_stat ) == 0 )
    {
        return 0;	 // 파일 있음
    }
    else
    {
        return -1;	// 파일 없음
    }

}

int fcom_MakeFlagFile(char* FilePath)
{
    FILE	*local_fp = NULL;
    char	 local_buff[10 +1] = {0x00,};


    if ( ( local_fp = fopen ( FilePath , "w" ) ) == NULL )
    {
        return -1;
    }
    else
    {
        sprintf( local_buff , "%d" , getpid() );
        fputs ( local_buff , local_fp );

        fclose ( local_fp );

        return 0;
    }
}

// 파일이 존재하는지 검사하고 지운다.
short fcom_FileCheckAndDelete(char* preFilepath, char* processName )
{
    struct 	statfs 	local_file_stat;
    char szFullPath[256 +1] = {0x00,};

    memset(&local_file_stat, 0x00, sizeof(struct statfs));


    sprintf(szFullPath, "%s/%s.pid",preFilepath, processName);

    if ( statfs ( (char *)szFullPath , &local_file_stat ) == 0 )
    {
        unlink( szFullPath );
        WRITE_DEBUG(CATEGORY_DEBUG,"Pid File Is Exist %s",szFullPath);
        return 0;	 // 파일 있음
    }
    else
    {
        WRITE_DEBUG(CATEGORY_DEBUG,"Pid File Is Not Exist %s",szFullPath);
        return -1;	// 파일 없음
    }

}



int fcom_EucKrTOUtf8(char *pre_buff , int pre_buff_memory_size )
{
    char		*inBuf=NULL;
    char		*outBuf=NULL;
    int			inBufSize = pre_buff_memory_size + 1;
    int			outBufSize =pre_buff_memory_size * 3 + 1;
    int			local_size = 0;
    int         local_idx = 0;
    size_t		local_tmp = 0;


    if(fcom_malloc((void**)&inBuf, inBufSize) != 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed ");
        return (-1);
    }
    if(fcom_malloc((void**)&outBuf, outBufSize) != 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed ");
        return (-1);
    }

    iconv_t	 local_cd = iconv_open("UTF-8", "EUC-KR");

    if (local_cd == (iconv_t)(-1))
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"iconv_open ");
        if(inBuf != NULL)
            free(inBuf);
        if(outBuf != NULL)
            free(outBuf);

        return (-1);
    }

    // 데이타 복제
    size_t readBytes = inBufSize -1;
    memcpy(  inBuf , pre_buff , inBufSize -1 );
    inBuf[readBytes] = '\0';

    // convert
    size_t writeBytes = outBufSize;

    char* in = inBuf;
    char* out = outBuf;

    local_tmp = iconv(local_cd, &in, &readBytes, &out, &writeBytes);
    if ( local_tmp > 0 )
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"failed to iconv errno:%d EILSEQ:%d",errno,EILSEQ);
        iconv_close(local_cd);
        if(inBuf != NULL)
            free(inBuf);
        if(outBuf != NULL)
            free(outBuf);

        return (-1);
    }

    outBuf[writeBytes] = '\0';
    local_size = strlen( outBuf );

    for(local_idx = strlen(outBuf); local_idx >= 0; local_idx--)
    {
        if(outBuf[local_idx] == 0)
            break;

        if(outBuf[local_idx] > 127 || outBuf[local_idx] < 0)
            outBuf[local_idx] = 42;

    }

    // 넘겨준 버퍼에 글자가 충분히 들어감
    if( local_size <= pre_buff_memory_size )
    {
        memcpy(  pre_buff ,   outBuf,  local_size );
        pre_buff[local_size] = 0x00;
    }
    else //local_size > pre_buff_memory_size
    {
        // 넘겨준 버퍼보다, 글자가 더 많은 경우는 글자를 자르고 넘겨줌
        pre_buff[pre_buff_memory_size] = 0x00;
        memcpy(  pre_buff ,   outBuf,  pre_buff_memory_size-1 );

        // error 로그 여기서 한줄 찍을것, 글자가 덜 들어감
    }

    iconv_close(local_cd);

    if(inBuf != NULL)
        free(inBuf);
    if(outBuf != NULL)
        free(outBuf);

    return 0;
}


int	fcom_malloc ( void **pre_point , size_t pre_size )
{
    if( *pre_point == NULL )
    {
        *pre_point = ( void *) malloc ( pre_size );
        if( *pre_point == NULL )
        {
            WRITE_CRITICAL(CATEGORY_DEBUG,"Memory allocation error , malloc is NULL ! ");
            return -1;
        }

        memset( *pre_point , 0x00 , pre_size );
        return 0;
    }
    else
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Request memory pointer is not NULL ! ");
        return -1;
    }
    return 0;
}

void fcom_DumpBinData( char* param_PreDataBuff, int param_PreDataBuffSize )
{
    int			local_nCutLine = 0;
    int			local_nPutFor  = 0;
    int			local_nMybuff  = 0;
    char		local_szMybuff[60 +1] = {0x00,};


    if ( param_PreDataBuffSize <= 0 )
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"line num : (%d) Packet Size is Zero or Minus value : %d", param_PreDataBuffSize);
        return;
    }


    memset( local_szMybuff , 0x00 , sizeof( local_szMybuff ) );
    WRITE_CRITICAL(CATEGORY_DEBUG,"Dump : %d -------------------------------------------------------------------------------\n", param_PreDataBuffSize );
    printf("Dump : %d -------------------------------------------------------------------------------\n", param_PreDataBuffSize );


    while ( local_nPutFor < param_PreDataBuffSize )
    {
        if( local_nCutLine > 20 )
        {
            WRITE_DEBUG(CATEGORY_DEBUG,"\t");

            while ( local_nMybuff < local_nCutLine  )
            {
//                if ( local_szMybuff[local_nMybuff] >= 0x20 &&  local_szMybuff[local_nMybuff] <= 0x7e )
//                {
                    printf("%c", local_szMybuff[local_nMybuff] );
//                }
//                else {  WRITE_DEBUG(CATEGORY_DEBUG,"."); printf("."); }

                local_nMybuff++;
            }

            local_nCutLine = 0;
            memset( local_szMybuff , 0x00 , sizeof( local_szMybuff ) );
            local_nMybuff = 0;
            printf("\n");
        }
//        WRITE_DEBUG("[%2x]",(unsigned char)param_PreDataBuff[local_nPutFor] );
        printf("[%2x]",(unsigned char)param_PreDataBuff[local_nPutFor] );
        local_szMybuff[local_nCutLine] = param_PreDataBuff[local_nPutFor];

        local_nPutFor ++;
        local_nCutLine ++;
    }   // while


    local_nPutFor = local_nCutLine;

    while ( local_nPutFor <= 20 )
    {
        WRITE_DEBUG(CATEGORY_DEBUG,"[  ]");
        printf("[  ]");
        local_nPutFor++;
    }

    WRITE_DEBUG(CATEGORY_DEBUG,"\t");
    printf("\t");

    while ( local_nMybuff < local_nCutLine  )
    {
        if ( local_szMybuff[local_nMybuff] >= 0x20 && local_szMybuff[local_nMybuff] <= 0x7e )
        {
            WRITE_DEBUG(CATEGORY_DEBUG,"%c", local_szMybuff[local_nMybuff] );
            printf("%c", local_szMybuff[local_nMybuff] );
        }
        else
        {
            WRITE_DEBUG(CATEGORY_DEBUG,".");
            printf(".");
        }

        local_nMybuff++;
    }

    WRITE_DEBUG(CATEGORY_DEBUG,"\n--------------------------------------------------------------------------------------------------------------\n");
    printf("\n--------------------------------------------------------------------------------------------------------------\n");
    fflush(stdout);

    return;
}