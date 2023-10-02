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

    /* ����� ���� ���丮������ DIR *�� return �޽��ϴ�. */
    if((dir_ptr = opendir(path)) == NULL) {
        /* path�� ���丮�� �ƴ϶�� �����ϰ� �����մϴ�. */
        return unlink(path);
    }

    /* ���丮�� ó������ ���� �Ǵ� ���丮���� ������� �Ѱ��� �н��ϴ�. */
    while((file = readdir(dir_ptr)) != NULL) {
        // readdir ������ ���ϸ� �߿� ���� ���丮�� ��Ÿ�״� . �� ���ԵǾ� �����Ƿ�
        // ���� �ݺ��� ������ �������� ���ϸ��� . �̸� skip �ؾ� ��
        if(strcmp(file->d_name, ".") == 0 || strcmp(file->d_name, "..") == 0) {
            continue;
        }

        sprintf(filename, "%s/%s", path, file->d_name);

        /* ������ �Ӽ�(������ ����, ũ��, ����/���� �ð� ���� ��� ���Ͽ� */
        if(lstat(filename, &buf) == -1) {
            continue;
        }

        if(S_ISDIR(buf.st_mode)) { // �˻��� �̸��� �Ӽ��� ���丮�̸�
            /* �˻��� ������ directory�̸� ���ȣ��� ���� ���丮�� �ٽ� �˻� */
            if(rmdirs(filename, force) == -1 && !force) {
                return -1;
            }
        } else if(S_ISREG(buf.st_mode) || S_ISLNK(buf.st_mode)) { // �Ϲ����� �Ǵ� symbolic link �̸�
            if(unlink(filename) == -1 && !force) {
                return -1;
            }
        }
    }

    /* open�� directory ������ close �մϴ�. */
    closedir(dir_ptr);

    return rmdir(path);
}

int fIBK_LogFileDelete(char* szIBKLogHome)
{
    time_t now;
    struct tm t;

    time(&now);

    //���� ��¥/�ð� ���
    t = *localtime(&now);

    //������ �� �ð� ���� Ȯ��
    t.tm_mday -= 7;

    //��¥ �� ���
    mktime(&t);

    int nYear = t.tm_year + 1900;
    int nMonth = t.tm_mon + 1;
    int nDay = t.tm_mday;
    int nValue = 0;

    DIR *dir;
    struct dirent *ent;

    char szLogPath[1024];
    char szTargetPath[1024];

    // �⵵ ���丮 Ȯ��
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

    // �� ���丮 Ȯ��
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

    // �� ���丮 Ȯ��
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
