#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <sys/timeb.h>

int fIBK_GetLogTime(char *timebuf)
{
    struct timeb t;
    struct tm timeinfo;
    int len;

    ftime(&t);

    localtime_r(&t.time, &timeinfo);

    len = sprintf(timebuf, "%04d-%02d-%02d %02d:%02d:%02d.%03u",
                  timeinfo.tm_year + 1900,
                  timeinfo.tm_mon + 1,
                  timeinfo.tm_mday,
                  timeinfo.tm_hour,
                  timeinfo.tm_min,
                  timeinfo.tm_sec,
                  t.millitm);

    return len;
}

int fIBK_GetLogTimeStr(char *timebuf)
{
    struct timeb t;
    struct tm timeinfo;
    int len;

    ftime(&t);

    localtime_r(&t.time, &timeinfo);

    len = sprintf(timebuf, "%04d%02d%02d_%02d%02d%02d%03u",
                  timeinfo.tm_year + 1900,
                  timeinfo.tm_mon + 1,
                  timeinfo.tm_mday,
                  timeinfo.tm_hour,
                  timeinfo.tm_min,
                  timeinfo.tm_sec,
                  t.millitm);

    return len;
}

int fIBK_MkPath(char* file_path, mode_t mode)
{
    //assert(file_path && *file_path);
    char* p;

    for (p=strchr(file_path+1, '/'); p; p=strchr(p+1, '/'))
    {
        *p='\0';

        if (mkdir(file_path, mode)==-1)
        {
            if (errno != EEXIST)
            {
                *p='/';
                return -1;
            }
        }

        *p='/';
    }
    p = NULL;

    return 0;
}

int rmdirs(const char *path, int force)
{
    DIR *  dir_ptr      = NULL;
    struct dirent *file = NULL;
    struct stat   buf;
    char   filename[1024];

    /* 목록을 읽을 디렉토리명으로 DIR *를 return 받습니다. */
    if((dir_ptr = opendir(path)) == NULL) {
        /* path가 디렉토리가 아니라면 삭제하고 종료합니다. */
        return unlink(path);
    }

    /* 디렉토리의 처음부터 파일 또는 디렉토리명을 순서대로 한개씩 읽습니다. */
    while((file = readdir(dir_ptr)) != NULL) {
        // readdir 읽혀진 파일명 중에 현재 디렉토리를 나타네는 . 도 포함되어 있으므로
        // 무한 반복에 빠지지 않으려면 파일명이 . 이면 skip 해야 함
        if(strcmp(file->d_name, ".") == 0 || strcmp(file->d_name, "..") == 0) {
            continue;
        }

        sprintf(filename, "%s/%s", path, file->d_name);

        /* 파일의 속성(파일의 유형, 크기, 생성/변경 시간 등을 얻기 위하여 */
        if(lstat(filename, &buf) == -1) {
            continue;
        }

        if(S_ISDIR(buf.st_mode)) { // 검색된 이름의 속성이 디렉토리이면
            /* 검색된 파일이 directory이면 재귀호출로 하위 디렉토리를 다시 검색 */
            if(rmdirs(filename, force) == -1 && !force) {
                return -1;
            }
        } else if(S_ISREG(buf.st_mode) || S_ISLNK(buf.st_mode)) { // 일반파일 또는 symbolic link 이면
            if(unlink(filename) == -1 && !force) {
                return -1;
            }
        }
    }

    /* open된 directory 정보를 close 합니다. */
    closedir(dir_ptr);

    return rmdir(path);
}

int fIBK_LogFileDelete(char* szIBKLogHome)
{
    time_t now;
    struct tm t;

    time(&now);

    //현재 날짜/시간 계산
    t = *localtime(&now);

    //일주일 전 시간 정보 확인
    t.tm_mday -= 7;

    //날짜 재 계산
    mktime(&t);

    int nYear = t.tm_year + 1900;
    int nMonth = t.tm_mon + 1;
    int nDay = t.tm_mday;
    int nValue = 0;

    DIR *dir;
    struct dirent *ent;

    char szLogPath[1024];
    char szTargetPath[1024];

    // 년도 디렉토리 확인
    sprintf(szLogPath, "%s/", szIBKLogHome);

    dir = opendir (szLogPath);

    if (dir != NULL)
    {
        /* print all the files and directories within directory */
        while ((ent = readdir (dir)) != NULL)
        {
            if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0){
                continue;
            }

            nValue = atoi(ent->d_name);

            if (nValue < nYear)
            {
                sprintf(szTargetPath, "%s/%04d", szIBKLogHome, nValue);
                rmdirs( szTargetPath, 1 );
            }
        }

        closedir (dir);
    }

    // 월 디렉토리 확인
    sprintf(szLogPath, "%s/%04d/", szIBKLogHome, nYear);

    dir = opendir (szLogPath);

    if (dir != NULL)
    {
        while ((ent = readdir (dir)) != NULL)
        {
            if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0){
                continue;
            }

            nValue = atoi(ent->d_name);

            if (nValue < nMonth)
            {
                sprintf(szTargetPath, "%s/%d/%02d", szIBKLogHome, nYear, nValue);
                rmdirs( szTargetPath, 1 );
            }
        }

        closedir (dir);
    }

    // 일 디렉토리 확인
    sprintf(szLogPath, "%s/%04d/%02d/", szIBKLogHome, nYear, nMonth);

    dir = opendir (szLogPath);

    if (dir != NULL)
    {
        while ((ent = readdir (dir)) != NULL)
        {
            if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0){
                continue;
            }

            nValue = atoi(ent->d_name);

            if (nValue < nDay)
            {
                sprintf(szTargetPath, "%s/%04d/%02d/%02d", szIBKLogHome, nYear, nMonth, nValue);
                rmdirs( szTargetPath, 1 );
            }
        }

        closedir (dir);
    }

    return 0;
}
