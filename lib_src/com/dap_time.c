//
// Created by KimByoungGook on 2020-06-11.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "com/dap_com.h"

int fcom_GetFormatTime(enum ENUM_TIMEFORMAT param_format, char* paramBuffer, int BufferLen)
{
    struct tm local;
    char temp[32+1];


    memset(temp, 0x00, sizeof(temp));

    time_t now = time(NULL);
    localtime_r(&now, &local);

    snprintf(temp,sizeof(temp),"%04d%02d%02d%02d%02d%02d",
             local.tm_year + 1900,
             local.tm_mon + 1,
             local.tm_mday,
             local.tm_hour,
             local.tm_min,
             local.tm_sec);

    switch(param_format)
    {
        case YYYYMMDDHHmmSS:
            temp[strlen(temp)] = 0x00;
            memcpy(paramBuffer,temp,BufferLen);
            break;
        case YYYYMMDDHHmm:
            temp[strlen(temp)-2] = 0x00;
            memcpy(paramBuffer,temp,BufferLen);
            break;
        case YYYYMMDDHH:
            temp[strlen(temp)-4] = 0x00;
            memcpy(paramBuffer,temp,BufferLen);
            break;
        case YYYYMMDD:
            temp[strlen(temp)-6] = 0x00;
            memcpy(paramBuffer,temp,BufferLen);
            break;
        case YYMMDDHHmmSS:
            temp[strlen(temp)] = 0x00;
            memcpy(paramBuffer,temp+2,BufferLen);
            break;
        case YYMMDDHHmm:
            temp[strlen(temp)-2] = 0x00;
            memcpy(paramBuffer,temp+2,BufferLen);
            break;
        case YYMMDDHH:
            temp[strlen(temp)-4] = 0x00;
            memcpy(paramBuffer,temp+2,BufferLen);
            break;
        case YYMMDD:
            temp[strlen(temp)-6] = 0x00;
            memcpy(paramBuffer,temp+2,BufferLen);
            break;
        case YYMM:
            temp[strlen(temp)-8] = 0x00;
            memcpy(paramBuffer,temp+2,BufferLen);
            break;
        case MMDDHHmmSS:
            temp[strlen(temp)] = 0x00;
            memcpy(paramBuffer,temp+4,BufferLen);
            break;
        case MMDDHHmm:
            temp[strlen(temp)-2] = 0x00;
            memcpy(paramBuffer,temp+4,BufferLen);
            break;
        case DDHHmm:
            temp[strlen(temp)-2] = 0x00;
            memcpy(paramBuffer,temp+6,BufferLen);
            break;
        case HHmmss:
            temp[strlen(temp)] = 0x00;
            memcpy(paramBuffer,temp+8,BufferLen);
            break;
    }
    return 0;
}
time_t	fcom_str2time(char *cpSrc, char * format)
{

    struct tm   spTm;
    int			index = 0;
    int			leng;
    char		*cpPtr;

    leng = strlen(format);
    memset(&spTm,0,sizeof(spTm));

    if(leng < 0) return 0;

    for(index = 0; index < leng ;)
    {

        switch(format[index])
        {
            case 's':
            {
                if(format[index + 1] != 's')
                {
                    index ++;
                    break;
                };
                spTm.tm_sec =
                        ((cpSrc[index] - 0x30) * 10) +
                        (cpSrc[index + 1] - 0x30);
                index = index + 2;
                break;
            }
            case 'm':
            {
                if(format[index + 1] != 'm')
                {
                    index ++;
                    break;
                };
                spTm.tm_min =
                        ((cpSrc[index] - 0x30) * 10) +
                        (cpSrc[index + 1] - 0x30);
                index = index + 2;
                break;
            };
            case 'h':
            {
                if(format[index + 1] != 'h')
                {
                    index ++;
                    break;
                };
                spTm.tm_hour =
                        ((cpSrc[index] - 0x30) * 10) +
                        (cpSrc[index + 1] - 0x30);
                index = index + 2;
                break;
            };
            case 'D':
            {
                if(format[index + 1] != 'D')
                {
                    index ++;
                    break;
                };
                spTm.tm_mday =
                        ((cpSrc[index] - 0x30) * 10) +
                        (cpSrc[index + 1] - 0x30);
                index = index + 2;
                break;
            };
            case 'M':
            {
                if(format[index + 1 ] != 'M')
                {
                    index ++;
                    break;
                };
                spTm.tm_mon =
                        ((cpSrc[index] - 0x30) * 10) +
                        (cpSrc[index + 1] - 0x30) - 1;

                index = index + 2;
                break;
            };
            case 'Y':
            {

                cpPtr = &format[index];

                if(memcmp(cpPtr ,"YYYY", 4) == 0)
                {

                    spTm.tm_year =
                            ((cpSrc[index]     - 0x30) * 1000) +
                            ((cpSrc[index + 1] - 0x30) * 100) +
                            ((cpSrc[index + 2] - 0x30) * 10) +
                            ((cpSrc[index + 3] - 0x30)) - 1900;

                    index = index + 4;
                    break;
                }
                if(format[index + 1 ] != 'Y')
                {
                    index ++;
                    break;
                };
                spTm.tm_year =
                        ((cpSrc[index] - 0x30) * 10) +
                        (cpSrc[index + 1] - 0x30);

                index = index + 2;
                break;


            };
            default :
            {
                index ++;
            };

        }

    }
#ifdef	DEBUG
    prtmktime(spTm);
#endif

    return mktime(&spTm);
}

char* fcom_time2str(time_t nTime, char* cpTmp,char* format )
{
    struct tm   stTm;
    int			index = 0;
    int			leng;
    char		*cpPtr;

    memset(&stTm, 0x00, sizeof(struct tm));
    leng = strlen(format);

    localtime_r(&nTime,&stTm);

    if(leng < 0)
        return NULL;

    memset(cpTmp, 0, leng + 1);

    for(index = 0; index < leng ;)
    {
        switch(format[index])
        {
            case 's':
            {
                if(format[index + 1] != 's')
                {
                    cpTmp[index] = format[index];
                    index ++;
                    break;
                };
                cpTmp[index] = (stTm.tm_sec / 10 ) + 0x30;
                cpTmp[index + 1] = (stTm.tm_sec % 10 ) + 0x30;

                index = index + 2;
                break;
            }
            case 'm':
            {
                if(format[index + 1] != 'm')
                {
                    cpTmp[index] = format[index];
                    index ++;
                    break;
                };
                cpTmp[index] = (stTm.tm_min / 10 ) + 0x30;
                cpTmp[index + 1] = (stTm.tm_min % 10 ) + 0x30;
                index = index + 2;
                break;
            };
            case 'h':
            {
                if(format[index + 1] != 'h')
                {
                    cpTmp[index] = format[index];
                    index ++;
                    break;
                };
                cpTmp[index] = (stTm.tm_hour / 10 ) + 0x30;
                cpTmp[index + 1] = (stTm.tm_hour % 10 ) + 0x30;
                index = index + 2;
                break;
            };
            case 'D':
            {
                if(format[index + 1] != 'D')
                {
                    cpTmp[index] = format[index];
                    index ++;
                    break;
                };
                cpTmp[index] = (stTm.tm_mday / 10 ) + 0x30;
                cpTmp[index + 1] = (stTm.tm_mday % 10 ) + 0x30;
                index = index + 2;
                break;
            };
            case 'M':
            {
                if(format[index + 1 ] != 'M')
                {
                    cpTmp[index] = format[index];
                    index ++;
                    break;
                };
                cpTmp[index] = ((stTm.tm_mon + 1) / 10 ) + 0x30;
                cpTmp[index + 1] = ( (stTm.tm_mon + 1)  % 10 ) + 0x30;
                index = index + 2;
                break;
            };
            case 'Y':
            {

                cpPtr = &format[index];

                if(memcmp(cpPtr ,"YYYY", 4) == 0)
                {

                    stTm.tm_year = stTm.tm_year + 1900;

                    cpTmp[index + 2] = (stTm.tm_year % 100 )/10 + 0x30;
                    cpTmp[index + 3] = (stTm.tm_year % 100 )%10 + 0x30;

                    cpTmp[index] = ((stTm.tm_year / 100 )/10) + 0x30;
                    cpTmp[index + 1] = ((stTm.tm_year / 100 ) %10)  + 0x30;

                    index = index + 4;
                    break;
                }
                if(format[index + 1 ] != 'Y')
                {
                    cpTmp[index] = format[index];
                    index ++;
                    break;
                };

                stTm.tm_year = stTm.tm_year + 1900;

                cpTmp[index] = (stTm.tm_year % 100 )/10 + 0x30;
                cpTmp[index + 1] = (stTm.tm_year % 100 )/10 + 0x30;

                index = index + 2;
                break;


            };
            default :
            {
                cpTmp[index] = format[index];
                index ++;
            };
        }
    }

    return cpTmp;
}


void fcom_GetSysdate(char* timebuf)
{
    time_t t;
    struct tm stpTm;

    time(&t);
    localtime_r(&t, &stpTm);
    sprintf(timebuf, "%.4d%.2d%.2d%.2d%.2d%.2d",
            stpTm.tm_year + 1900,
            stpTm.tm_mon + 1,
            stpTm.tm_mday,
            stpTm.tm_hour,
            stpTm.tm_min,
            stpTm.tm_sec);
}
int fcom_GetSysMinute()
{

    time_t    t;
    struct tm stpTm;
    time(&t);
    localtime_r(&t, &stpTm);

    return (int)stpTm.tm_min;

}



time_t  fcom_CalcTime(int value)
{
    time_t  cTime;
    int     tmp;
    struct  tm  stTm;

    cTime   = time(0);
    localtime_r(&cTime,&stTm);

    tmp     = stTm.tm_min / value;
    stTm.tm_min = (tmp + 1)  * value;
    tmp = 0;
    if(stTm.tm_min  == 60)
    {
        stTm.tm_min = 0;
        tmp = 60 * 60;
    };
    stTm.tm_sec = 0;

    cTime = mktime(&stTm) + tmp;

    return cTime;
}

void fcom_Msleep(time_t msec)
{
    struct timespec rqtp;

    rqtp.tv_sec	= msec/1000; //seconds
    rqtp.tv_nsec = (msec % 1000) * 1000000; // 1 / 100000000
    nanosleep( &rqtp, NULL );

}

int fcom_GetAgoTime(int nAgoDay, char* param_TimeBuff)
{
    time_t local_time;
    struct tm stTime;

    memset(&stTime, 0x00, sizeof(struct tm));

    local_time = time(NULL) - (ONE_DAY * nAgoDay);
    localtime_r(&local_time, &stTime);

    sprintf(param_TimeBuff, "%04d%02d%02d%02d%02d%02d",
            stTime.tm_year+1900, stTime.tm_mon+1, stTime.tm_mday, stTime.tm_hour, stTime.tm_min, stTime.tm_sec);

    return RET_SUCC;

}


int fcom_GetTime(char *timebuff, int sec)
{
    struct tm stTime;
    time_t cDate;

    memset(&stTime, 0x00, sizeof(struct tm));
    cDate = time(NULL)-sec;
    localtime_r(&cDate, &stTime);

    sprintf(timebuff, "%04d%02d%02d%02d%02d%02d",
            stTime.tm_year+1900, stTime.tm_mon+1, stTime.tm_mday, stTime.tm_hour, stTime.tm_min, stTime.tm_sec);

    return 0;
}

int fcom_GetDayOfWeek(char *date)
{
    int		year = 0, month = 0, day = 0;
    int		i;
    int		loop = 0;
    char	tmpDate[10+1] = {0x00,};
    int		months[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    long	total = 0L;
    char	*tokenKey = NULL;
    char    *TempPtr  = NULL;

    memset(tmpDate, 0x00, sizeof(tmpDate));
    strcpy(tmpDate, date);
    tokenKey = strtok_r(tmpDate, "-",&TempPtr);
    loop = 0;
    while(tokenKey != NULL)
    {
        switch(loop)
        {
            case 0:
                year = atoi(tokenKey);
                break;
            case 1:
                month = atoi(tokenKey);
                break;
            case 2:
                day = atoi(tokenKey);
                break;
        }
        loop++;
        tokenKey = strtok_r(NULL, "-",&TempPtr);
    }

    total = (year-1)*365 + (year-1)/4 - (year-1)/100 + (year-1)/400;
    if( !(year%4) && (year%100 || !(year%400)) )
        months[1]++;
    for(i=0; i<month-1; i++)
        total += months[i];

    total += day;

    return total%7;
}

void fcom_DayofWeek2Str(int weekNum, char *res)
{
    if		( weekNum == 0 ) strcpy(res, "SUN");
    else if	( weekNum == 1 ) strcpy(res, "MON");
    else if	( weekNum == 2 ) strcpy(res, "TUE");
    else if	( weekNum == 3 ) strcpy(res, "WED");
    else if	( weekNum == 4 ) strcpy(res, "THU");
    else if	( weekNum == 5 ) strcpy(res, "FRI");
    else if	( weekNum == 6 ) strcpy(res, "SAT");
}

unsigned short fcom_GetDayOfMonth(struct tm *st)
{
    unsigned short dayofweek;
    unsigned short day;

    //dayofweek = st->wDayOfWeek;
    dayofweek = st->tm_wday;
    //day = st->wDay;
    day = st->tm_mday;
    while (day > 1)
    {
        day--;
        if (dayofweek == 0)
        {
            dayofweek = 6;
        }
        else
        {
            dayofweek--;
        }
    }
    return dayofweek;
}

/*
 * 일요일부터 Week 시작하기위해
 * 기존 소스는 월요일부터 Week의 시작 [while(day < today)]
 * 일요일부터 Week의 시작을 위해 변경 [while(day <= today)]
 */
unsigned short fcom_GetWeekOfMonth(struct tm *st)
{
    unsigned short today;
    unsigned short firstSunday_day; //�ſ� ù°�� �Ͽ����� ��¥
    unsigned short first_dayofweek;
    unsigned short week;
    unsigned short day;
    unsigned short dayofweek;

    //-----------------------
    //이번달 1일의 요일을 구한다.
    first_dayofweek = fcom_GetDayOfMonth(st);
    //------------------------
    //매월 첫번째 일요일의 날짜 구하기
    firstSunday_day = 1;
    dayofweek = first_dayofweek;
    while (dayofweek != 0)
    {
        firstSunday_day++;
        if (dayofweek == 6)
        {
            dayofweek = 0;
        }
        else
        {
            dayofweek++;
        }
    }
    //------------------------
    //오늘이 몇번째 주인지 판단
    week = 1;
    day = firstSunday_day;
    today = st->tm_mday;

    while (day <= today) // 변경된 소스로 일요일부터 주 시작
    {
        day += 7;
        week++;
    }
    return week;
}

void fcom_GetDateyyyy4digit( char *pre_date_time )
{

    time_t				local_get_now;
    struct	tm			local_tm;

    local_get_now = time ( NULL );
    localtime_r ( &local_get_now , &local_tm );

    /*-- 4 자리  2007 --*/
    sprintf( pre_date_time , "%.4d" , local_tm.tm_year+1900 );

    return;
}


void fcom_GetDatemm2digit( char *pre_date_time )
{

    time_t				local_get_now;
    struct	tm			local_tm;

    local_get_now = time ( NULL );
    localtime_r ( &local_get_now , &local_tm );

    /*-- 2 자리  03 --*/
    sprintf( pre_date_time , "%.2d" , local_tm.tm_mon+1 );
    return;
}

void fcom_GetDateDD2digit( char *pre_date_time )
{
    time_t				local_get_now;
    struct	tm			local_tm;

    local_get_now = time ( NULL );
    localtime_r ( &local_get_now , &local_tm );

    /*-- 2 자리  03 --*/
    sprintf( pre_date_time , "%.2d" , local_tm.tm_mday );
    return;
}


void fcom_SleepWait( int pre_int_wait )
{
    struct timespec 	local_reqtime;
    long	local_wait_nsec = 0;
    // 1 초 1,000,000,000
    // 0.1 초  100,000,000   : 5
    // 0.01 초  10,000,000   : 4
    // 0.001 초  1,000,000   : 3
    // 0.0001 초  100,000    : 2
    // 0.00001 초  10,000    : 1
    switch(  pre_int_wait )
    {
        case  1 : local_wait_nsec = 10000;
            break;
        case  2 : local_wait_nsec = 100000;
            break;
        case  3 : local_wait_nsec = 1000000;
            break;
        case  4 : local_wait_nsec = 10000000;
            break;
        case  5 : local_wait_nsec = 100000000;
            break;
        default : local_wait_nsec = 1000000000;
    }

    local_reqtime.tv_sec = 0;
    local_reqtime.tv_nsec = local_wait_nsec;
    nanosleep(&local_reqtime, NULL);
    return;
}

static int isLeapYear(int year)
{
    return ( (!(year%4) && (year%100) ) || !(year%400) );
}

long fcom_GetDaysTo(char *yyyymmdd)
{
    int int_year, int_month, int_day, i, pivot_year = 1;
    char c_year[4+1], c_month[2+1], c_day[2+1];
    int month_data[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
    long day_sum = 0;

    memset(c_year,  0x00, 5);
    memset(c_month, 0x00, 3);
    memset(c_day,   0x00, 3);

    memcpy(c_year,  yyyymmdd,   4);
    memcpy(c_month, yyyymmdd+4, 2);
    memcpy(c_day,   yyyymmdd+6, 2);

    int_year  = atoi(c_year);
    int_month = atoi(c_month);
    int_day   = atoi(c_day);

    for (i=pivot_year; i<int_year; i++) day_sum = day_sum + 365 + isLeapYear(i);
    for (i=0; i<int_month-1; i++)
    {
        if (i==1) day_sum += (month_data[i] + isLeapYear(int_year));
        else day_sum += month_data[i];
    }

    day_sum += int_day;
    return day_sum;
}

long fcom_GetRelativeDate(char* aDate, char* bDate)
{
    return abs(fcom_GetDaysTo(aDate) - fcom_GetDaysTo(bDate));
}

//01년 01월부터 파라미터까지 몇일 지났는지 반환하는 함수
int fcom_ToDay(int param_YYYY, int param_MM, int param_Day)
{
    int local_Month[] = {0,31,28,31,30,31,30,31,31,30,31,30,31};
    int local_nDay = 0;
    int i;
    for(i=1; i<param_YYYY; i++)
    {
        local_nDay += (i % 4 == 0 && i % 100 != 0) || ( i % 400 == 0 ) ? 366 : 365;
    }

    if( ( param_YYYY%4 == 0 && param_YYYY%100 != 0) || param_YYYY%400==0 )
        local_Month[2]++;

    for(i = 1; i < param_MM; i++)
    {
        local_nDay += local_Month[i];
    }
    return local_nDay + param_Day;
}

// 파라미터 년,일,월의 diff day를 반환하는 함수
int fcom_DiffDay(int param_nYYYY1, int param_nMM1, int param_nDD1, int param_nYYYY2, int param_nMM2, int param_nDD2)
{
    int a = fcom_ToDay(param_nYYYY1, param_nMM1, param_nDD1);
    int b = fcom_ToDay(param_nYYYY2, param_nMM2, param_nDD2);
    return b-a;
}