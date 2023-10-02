//
// Created by KimByoungGook on 2020-06-23.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>



#include "com/dap_def.h"
#include "com/dap_com.h"

#include "report.h"



int fcom_SetCustLang(unsigned char *p_lang)
{
    memset(g_szCustLang, 0x00, sizeof(g_szCustLang));
    if(strlen(p_lang) > 0)
        strcpy(g_szCustLang, p_lang);
    return 0;
}

char* fcom_GetCustLang()
{
    return g_szCustLang;
}

void fcom_GetStrLevel(int level, char* res)
{
    switch(level)
    {
        case PASS:
            strcpy(res, "PASS");
            break;
        case DROP:
            strcpy(res, "DROP");
            break;
        case INFO:
            strcpy(res, "INFO");
            break;
        case WARNING:
            strcpy(res, "WARNING");
            break;
        case CRITICAL:
            strcpy(res, "CRITICAL");
            break;
        case BLOCK:
            strcpy(res, "BLOCK");
            break;
        default:
            strcpy(res, "UNKNOWN");
            break;
    }
}

int fcom_GetNumberLevel(char* level)
{
    if		( !strncmp(level, "PASS", strlen(level)) ) 		return 0;
    else if	( !strncmp(level, "DROP", strlen(level)) ) 		return 1;
    else if	( !strncmp(level, "INFO", strlen(level)) ) 		return 2;
    else if	( !strncmp(level, "WARNING", strlen(level)) ) 	return 3;
    else if	( !strncmp(level, "CRITICAL", strlen(level)) ) 	return 4;
    else if	( !strncmp(level, "BLOCK", strlen(level)) ) 	return 5;
    else													return 2;
}

void fcom_GetStrType2(int type, char* res, char* param_confLang)
{
    switch(type)
    {
        case MAIN_BOARD:
            if(strlen(g_szCustLang) > 0)
            {
                if(!strncmp(g_szCustLang, "kr", 2))
                {
                    strcpy(res, "메인보드");
                }
                else
                {
                    strcpy(res, "MAIN_BOARD");
                }
            }
            else
            {
                if(!strncmp(param_confLang, "kr", 2))
                {
                    strcpy(res, "메인보드");
                }
                else
                {
                    strcpy(res, "MAIN_BOARD");
                }
            }
            break;
        case SYSTEM:
            if(strlen(g_szCustLang) > 0)
            {
                if(!strncmp(g_szCustLang, "kr", 2))
                {
                    strcpy(res, "시스템");
                }
                else
                {
                    strcpy(res, "SYSTEM");
                }
            }
            else
            {
                if(!strncmp(param_confLang, "kr", 2))
                {
                    strcpy(res, "시스템");
                }
                else
                {
                    strcpy(res, "SYSTEM");
                }
            }
            break;
        case OPERATING_SYSTEM:
            if(strlen(g_szCustLang) > 0)
            {
                if(!strncmp(g_szCustLang, "kr", 2))
                {
                    strcpy(res, "운영체제");
                }
                else
                {
                    strcpy(res, "운영체제");
                }
            }
            else
            {
                if(!strncmp(param_confLang, "kr", 2))
                {
                    strcpy(res, "OPERATING_SYSTEM");
                }
                else
                {
                    strcpy(res, "OPERATING_SYSTEM");
                }
            }
            break;
        case CPU:
            if(strlen(g_szCustLang) > 0)
            {
                if(!strncmp(g_szCustLang, "kr", 2))
                {
                    strcpy(res, "CPU");
                }
                else
                {
                    strcpy(res, "CPU");
                }
            }
            else
            {
                if(!strncmp(param_confLang, "kr", 2))
                {
                    strcpy(res, "CPU");
                }
                else
                {
                    strcpy(res, "CPU");
                }
            }
            break;
        case NET_ADAPTER:
//            if(strlen(custLang) > 0)
            if(strlen(g_szCustLang) > 0)
            {
                if(!strncmp(g_szCustLang, "kr", 2))
                {
                    strcpy(res, "네트워크어댑터");
                }
                else
                {
                    strcpy(res, "NET_ADAPTER");
                }
            }
            else
            {
                if(!strncmp(param_confLang, "kr", 2))
                {
                    strcpy(res, "네트워크어댑터");
                }
                else
                {
                    strcpy(res, "NET_ADAPTER");
                }
            }
            break;
        case WIFI:
            if(strlen(g_szCustLang) > 0)
            {
                if(!strncmp(g_szCustLang, "kr", 2)) {
                    strcpy(res, "와이파이");
                } else {
                    strcpy(res, "WIFI");
                }
            } else {
                if(!strncmp(param_confLang, "kr", 2)) {
                    strcpy(res, "와이파이");
                } else {
                    strcpy(res, "WIFI");
                }
            }
            break;
        case BLUETOOTH:
            if(strlen(g_szCustLang) > 0)
            {
                if(!strncmp(g_szCustLang, "kr", 2))
                {
                    strcpy(res, "블루투스");
                }
                else
                {
                    strcpy(res, "BLUETOOTH");
                }
            }
            else
            {
                if(!strncmp(param_confLang, "kr", 2))
                {
                    strcpy(res, "블루투스");
                }
                else
                {
                    strcpy(res, "BLUETOOTH");
                }
            }
            break;
        case NET_CONNECTION:
            if(strlen(g_szCustLang) > 0)
            {
                if(!strncmp(g_szCustLang, "kr", 2))
                {
                    strcpy(res, "네트워크연결");
                }
                else
                {
                    strcpy(res, "NET_CONNECTION");
                }
            }
            else
            {
                if(!strncmp(g_szCustLang, "kr", 2))
                {
                    strcpy(res, "네트워크연결");
                }
                else
                {
                    strcpy(res, "NET_CONNECTION");
                }
            }
            break;
        case DISK:
            if(strlen(g_szCustLang) > 0)
            {
                if(!strncmp(g_szCustLang, "kr", 2))
                {
                    strcpy(res, "디스크");
                }
                else
                {
                    strcpy(res, "DISK");
                }
            }
            else
            {
                if(!strncmp(param_confLang, "kr", 2))
                {
                    strcpy(res, "디스크");
                }
                else
                {
                    strcpy(res, "DISK");
                }
            }
            break;
        case NET_DRIVE:
            if(strlen(g_szCustLang) > 0)
            {
                if(!strncmp(g_szCustLang, "kr", 2))
                {
                    strcpy(res, "네트워크드라이브");
                }
                else
                {
                    strcpy(res, "NET_DRIVE");
                }
            }
            else
            {
                if(!strncmp(param_confLang, "kr", 2))
                {
                    strcpy(res, "네트워크드라이브");
                }
                else
                {
                    strcpy(res, "NET_DRIVE");
                }
            }
            break;
        case OS_ACCOUNT:
            if(strlen(g_szCustLang) > 0)
            {
                if(!strncmp(g_szCustLang, "kr", 2))
                {
                    strcpy(res, "운영체제계정");
                }
                else
                {
                    strcpy(res, "OS_ACCOUNT");
                }
            }
            else
            {
                if(!strncmp(param_confLang, "kr", 2))
                {
                    strcpy(res, "운영체제계정");
                }
                else
                {
                    strcpy(res, "OS_ACCOUNT");
                }
            }
            break;
        case SHARE_FOLDER:
            if(strlen(g_szCustLang) > 0)
            {
                if(!strncmp(g_szCustLang, "kr", 2))
                {
                    strcpy(res, "공유폴더");
                }
                else
                {
                    strcpy(res, "SHARE_FOLDER");
                }
            }
            else
            {
                if(!strncmp(param_confLang, "kr", 2))
                {
                    strcpy(res, "공유폴더");
                }
                else
                {
                    strcpy(res, "SHARE_FOLDER");
                }
            }
            break;
        case INFRARED_DEVICE:
            if(strlen(g_szCustLang) > 0)
            {
                if(!strncmp(g_szCustLang, "kr", 2))
                {
                    strcpy(res, "적외선장치");
                }
                else
                {
                    strcpy(res, "INFRARED_DEVICE");
                }
            }
            else
            {
                if(!strncmp(param_confLang, "kr", 2))
                {
                    strcpy(res, "적외선장치");
                }
                else
                {
                    strcpy(res, "INFRARED_DEVICE");
                }
            }
            break;
        case PROCESS:
            if(strlen(g_szCustLang) > 0)
            {
                if(!strncmp(g_szCustLang, "kr", 2))
                {
                    strcpy(res, "프로세스");
                }
                else
                {
                    strcpy(res, "PROCESS");
                }
            }
            else
            {
                if(!strncmp(param_confLang, "kr", 2))
                {
                    strcpy(res, "프로세스");
                }
                else
                {
                    strcpy(res, "PROCESS");
                }
            }
            break;
        case ROUTER:
            if(strlen(g_szCustLang) > 0)
            {
                if(!strncmp(g_szCustLang, "kr", 2))
                {
                    strcpy(res, "공유기");
                }
                else
                {
                    strcpy(res, "ROUTER");
                }
            }
            else
            {
                if(!strncmp(param_confLang, "kr", 2))
                {
                    strcpy(res, "공유기");
                }
                else
                {
                    strcpy(res, "ROUTER");
                }
            }
            break;
        case NET_PRINTER:
            if(strlen(g_szCustLang) > 0)
            {
                if(!strncmp(g_szCustLang, "kr", 2))
                {
                    strcpy(res, "네트워크프린터");
                }
                else
                {
                    strcpy(res, "NET_PRINTER");
                }
            }
            else
            {
                if(!strncmp(param_confLang, "kr", 2))
                {
                    strcpy(res, "네트워크프린터");
                }
                else
                {
                    strcpy(res, "NET_PRINTER");
                }
            }
            break;
        case NET_SCAN:
            if(strlen(g_szCustLang) > 0)
            {
                if(!strncmp(g_szCustLang, "kr", 2))
                {
                    strcpy(res, "네트워크스캔");
                }
                else
                {
                    strcpy(res, "NET_SCAN");
                }
            }
            else
            {
                if(!strncmp(param_confLang, "kr", 2))
                {
                    strcpy(res, "네트워크스캔");
                }
                else
                {
                    strcpy(res, "NET_SCAN");
                }
            }
            break;
        case ARP:
            if(strlen(g_szCustLang) > 0)
            {
                if(!strncmp(g_szCustLang, "kr", 2))
                {
                    strcpy(res, "ARP");
                }
                else
                {
                    strcpy(res, "ARP");
                }
            }
            else
            {
                if(!strncmp(param_confLang, "kr", 2))
                {
                    strcpy(res, "ARP");
                }
                else
                {
                    strcpy(res, "ARP");
                }
            }
            break;
        case VIRTUAL_MACHINE:
            if(strlen(g_szCustLang) > 0)
            {
                if(!strncmp(g_szCustLang, "kr", 2))
                {
                    strcpy(res, "가상머신");
                }
                else
                {
                    strcpy(res, "VIRTUAL_MACHINE");
                }
            }
            else
            {
                if(!strncmp(param_confLang, "kr", 2))
                {
                    strcpy(res, "가상머신");
                }
                else
                {
                    strcpy(res, "VIRTUAL_MACHINE");
                }
            }
            break;
        case CONNECT_EXT_SVR:
            if(strlen(g_szCustLang) > 0)
            {
                if(!strncmp(g_szCustLang, "kr", 2))
                {
                    strcpy(res, "외부네트워크");
                }
                else
                {
                    strcpy(res, "CONNECT_EXT_SVR");
                }
            }
            else
            {
                if(!strncmp(param_confLang, "kr", 2))
                {
                    strcpy(res, "외부네트워크");
                }
                else
                {
                    strcpy(res, "CONNECT_EXT_SVR");
                }
            }
            break;
        case NET_ADAPTER_OVER:
            if(strlen(g_szCustLang) > 0)
            {
                if(!strncmp(g_szCustLang, "kr", 2))
                {
                    strcpy(res, "네트워크어댑터2개이상");
                }
                else
                {
                    strcpy(res, "NET_ADAPTER_OVER");
                }
            }
            else
            {
                if(!strncmp(param_confLang, "kr", 2))
                {
                    strcpy(res, "네트워크어댑터2개이상");
                }
                else
                {
                    strcpy(res, "NET_ADAPTER_OVER");
                }
            }
            break;
        case NET_ADAPTER_DUPIP:
            if(strlen(g_szCustLang) > 0)
            {
                if(!strncmp(g_szCustLang, "kr", 2))
                {
                    strcpy(res, "IP주소중복탐지");
                }
                else
                {
                    strcpy(res, "NET_ADAPTER_DUPIP");
                }
            }
            else
            {
                if(!strncmp(param_confLang, "kr", 2))
                {
                    strcpy(res, "IP주소중복탐지");
                }
                else
                {
                    strcpy(res, "NET_ADAPTER_DUPIP");
                }
            }
            break;
        case NET_ADAPTER_DUPMAC:
            if(strlen(g_szCustLang) > 0)
            {
                if(!strncmp(g_szCustLang, "kr", 2))
                {
                    strcpy(res, "MAC주소중복탐지");
                }
                else
                {
                    strcpy(res, "NET_ADAPTER_DUPMAC");
                }
            } else {
                if(!strncmp(param_confLang, "kr", 2)) {
                    strcpy(res, "MAC주소중복탐지");
                } else {
                    strcpy(res, "NET_ADAPTER_DUPMAC");
                }
            }
            break;
        case DISK_REG:
            if(strlen(g_szCustLang) > 0)
            {
                if(!strncmp(g_szCustLang, "kr", 2))
                {
                    strcpy(res, "미등록디스크");
                }
                else
                {
                    strcpy(res, "DISK_REG");
                }
            }
            else
            {
                if(!strncmp(param_confLang, "kr", 2))
                {
                    strcpy(res, "미등록디스크");
                }
                else
                {
                    strcpy(res, "DISK_REG");
                }
            }
            break;
        case DISK_HIDDEN:
            if(strlen(g_szCustLang) > 0)
            {
                if(!strncmp(g_szCustLang, "kr", 2))
                {
                    strcpy(res, "디스크숨김");
                }
                else
                {
                    strcpy(res, "DISK_HIDDEN");
                }
            }
            else
            {
                if(!strncmp(param_confLang, "kr", 2))
                {
                    strcpy(res, "디스크숨김");
                }
                else
                {
                    strcpy(res, "DISK_HIDDEN");
                }
            }
            break;
        case DISK_NEW:
            if(strlen(g_szCustLang) > 0)
            {
                if(!strncmp(g_szCustLang, "kr", 2))
                {
                    strcpy(res, "새로운디스크");
                }
                else
                {
                    strcpy(res, "DISK_NEW");
                }
            }
            else
            {
                if(!strncmp(param_confLang, "kr", 2))
                {
                    strcpy(res, "새로운디스크");
                }
                else
                {
                    strcpy(res, "DISK_NEW");
                }
            }
            break;
        case DISK_MOBILE:
            if(strlen(g_szCustLang) > 0)
            {
                if(!strncmp(g_szCustLang, "kr", 2))
                {
                    strcpy(res, "이동식디스크");
                }
                else
                {
                    strcpy(res, "DISK_MOBILE");
                }
            }
            else
            {
                if(!strncmp(param_confLang, "kr", 2))
                {
                    strcpy(res, "이동식디스크");
                }
                else
                {
                    strcpy(res, "DISK_MOBILE");
                }
            }
            break;
        case DISK_MOBILE_READ:
            if(strlen(g_szCustLang) > 0)
            {
                if(!strncmp(g_szCustLang, "kr", 2))
                {
                    strcpy(res, "디스크읽기권한");
                }
                else
                {
                    strcpy(res, "DISK_MOBILE_READ");
                }
            }
            else
            {
                if(!strncmp(param_confLang, "kr", 2))
                {
                    strcpy(res, "디스크읽기권한");
                }
                else
                {
                    strcpy(res, "DISK_MOBILE_READ");
                }
            }
            break;
        case DISK_MOBILE_WRITE:
            if(strlen(g_szCustLang) > 0)
            {
                if(!strncmp(g_szCustLang, "kr", 2))
                {
                    strcpy(res, "디스크쓰기권한");
                }
                else
                {
                    strcpy(res, "DISK_MOBILE_WRITE");
                }
            }
            else
            {
                if(!strncmp(param_confLang, "kr", 2))
                {
                    strcpy(res, "디스크쓰기권한");
                }
                else
                {
                    strcpy(res, "DISK_MOBILE_WRITE");
                }
            }
            break;
        case PROCESS_WHITE:
            if(strlen(g_szCustLang) > 0)
            {
                if(!strncmp(g_szCustLang, "kr", 2))
                {
                    strcpy(res, "화이트리스트프로세스");
                }
                else
                {
                    strcpy(res, "PROCESS_WHITE");
                }
            }
            else
            {
                if(!strncmp(param_confLang, "kr", 2))
                {
                    strcpy(res, "화이트리스트프로세스");
                }
                else
                {
                    strcpy(res, "PROCESS_WHITE");
                }
            }
            break;
        case PROCESS_BLACK:
            if(strlen(g_szCustLang) > 0)
            {
                if(!strncmp(g_szCustLang, "kr", 2))
                {
                    strcpy(res, "블랙리스트프로세스");
                }
                else
                {
                    strcpy(res, "PROCESS_BLACK");
                }
            }
            else
            {
                if(!strncmp(param_confLang, "kr", 2))
                {
                    strcpy(res, "블랙리스트프로세스");
                }
                else
                {
                    strcpy(res, "PROCESS_BLACK");
                }
            }
            break;
        case PROCESS_ACCESSMON:
            if(strlen(g_szCustLang) > 0)
            {
                if(!strncmp(g_szCustLang, "kr", 2))
                {
                    strcpy(res, "접속감시대상프로세스");
                }
                else
                {
                    strcpy(res, "PROCESS_ACCESSMON");
                }
            }
            else
            {
                if(!strncmp(param_confLang, "kr", 2))
                {
                    strcpy(res, "접속감시대상프로세스");
                }
                else
                {
                    strcpy(res, "PROCESS_ACCESSMON");
                }
            }
            break;
        case NET_ADAPTER_MULIP:
            if(strlen(g_szCustLang) > 0)
            {
                if(!strncmp(g_szCustLang, "kr", 2))
                {
                    strcpy(res, "IP주소다중설정탐지");
                }
                else
                {
                    strcpy(res, "NET_ADAPTER_MULIP");
                }
            }
            else
            {
                if(!strncmp(param_confLang, "kr", 2))
                {
                    strcpy(res, "IP주소다중설정탐지");
                }
                else
                {
                    strcpy(res, "NET_ADAPTER_MULIP");
                }
            }
            break;
        case EXTERNAL_CONN:
            if(strlen(g_szCustLang) > 0)
            {
                if(!strncmp(g_szCustLang, "kr", 2))
                {
                    strcpy(res, "외부접속탐지");
                }
                else
                {
                    strcpy(res, "EXTERNAL_CONN");
                }
            }
            else
            {
                if(!strncmp(param_confLang, "kr", 2))
                {
                    strcpy(res, "외부접속탐지");
                }
                else
                {
                    strcpy(res, "EXTERNAL_CONN");
                }
            }
            break;
        case WIN_DRV:
            if(strlen(g_szCustLang) > 0)
            {
                if(!strncmp(g_szCustLang, "kr", 2))
                {
                    strcpy(res, "윈도우드라이버장치");
                }
                else
                {
                    strcpy(res, "WIN_DRV");
                }
            } else {
                if(!strncmp(param_confLang, "kr", 2)) {
                    strcpy(res, "윈도우드라이버장치");
                } else {
                    strcpy(res, "WIN_DRV");
                }
            }
            break;
        case RDP_SESSION:
            if(strlen(g_szCustLang) > 0)
            {
                if(!strncmp(g_szCustLang, "kr", 2))
                {
                    strcpy(res, "원격데스크톱연결탐지");
                }
                else
                {
                    strcpy(res, "RDP_SESSION");
                }
            }
            else
            {
                if(!strncmp(param_confLang, "kr", 2))
                {
                    strcpy(res, "원격데스크톱연결탐지");
                }
                else
                {
                    strcpy(res, "RDP_SESSION");
                }
            }
            break;
        case SSO_CERT:
            if(strlen(g_szCustLang) > 0)
            {
                if(!strncmp(g_szCustLang, "kr", 2))
                {
                    strcpy(res, "SSO인증여부");
                }
                else
                {
                    strcpy(res, "SSO_CERT");
                }
            }
            else
            {
                if(!strncmp(param_confLang, "kr", 2))
                {
                    strcpy(res, "SSO인증여부");
                }
                else
                {
                    strcpy(res, "SSO_CERT");
                }
            }
            break;
        case PROCESS_BLOCK:
            if(strlen(g_szCustLang) > 0)
            {
                if(!strncmp(g_szCustLang, "kr", 2))
                {
                    strcpy(res, "프로세스차단");
                }
                else
                {
                    strcpy(res, "PROCESS_BLOCK");
                }
            }
            else
            {
                if(!strncmp(param_confLang, "kr", 2))
                {
                    strcpy(res, "프로세스차단");
                }
                else
                {
                    strcpy(res, "PROCESS_BLOCK");
                }
            }
            break;
        case CPU_USAGE_ALARM:
            if(strlen(g_szCustLang) > 0)
            {
                if(!strncmp(g_szCustLang, "kr", 2))
                {
                    strcpy(res, "CPU사용률알람");
                }
                else
                {
                    strcpy(res, "CPU_USAGE_ALARM");
                }
            }
            else
            {
                if(!strncmp(param_confLang, "kr", 2))
                {
                    strcpy(res, "CPU사용률알람");
                }
                else
                {
                    strcpy(res, "CPU_USAGE_ALARM");
                }
            }
            break;

        case CPU_USAGE_CONTROL:
            if(strlen(g_szCustLang) > 0)
            {
                if(!strncmp(g_szCustLang, "kr", 2))
                {
                    strcpy(res, "CPU사용통제");
                }
                else
                {
                    strcpy(res, "CPU_USAGE_CONTROL");
                }
            }
            else
            {
                if(!strncmp(param_confLang, "kr", 2))
                {
                    strcpy(res, "CPU사용통제");
                }
                else
                {
                    strcpy(res, "CPU_USAGE_CONTROL");
                }
            }
            break;

        default:
            if(strlen(g_szCustLang) > 0)
            {
                if(!strncmp(g_szCustLang, "kr", 2))
                {
                    strcpy(res, "알수없음");
                }
                else
                {
                    strcpy(res, "UNKNOWN");
                }
            }
            else
            {
                if(!strncmp(param_confLang, "kr", 2))
                {
                    strcpy(res, "알수없음");
                }
                else
                {
                    strcpy(res, "UNKNOWN");
                }
            }
            break;
    }
}
char* fcom_getStrType(int type)
{
    switch(type)
    {
        case MAIN_BOARD:
            if(strlen(fcom_GetCustLang()) > 0)
            {
                if(!strncmp(fcom_GetCustLang(), "kr", 2))
                {
                    return "메인보드";
                }
                else
                {
                    return "MAIN_BOARD";
                }
            }
            else
            {
                if(!strncmp(fcom_GetCustLang(), "kr", 2))
                {
                    return "메인보드";
                }
                else
                {
                    return "MAIN_BOARD";
                }
            }
            break;
        case SYSTEM:
            if(strlen(fcom_GetCustLang()) > 0)
            {
                if(!strncmp(fcom_GetCustLang(), "kr", 2))
                {
                    return "시스템";
                }
                else
                {
                    return "SYSTEM";
                }
            }
            else
            {
                if(!strncmp(g_stProcReportInfo.szConfMailLang, "kr", 2))
                {
                    return "시스템";
                }
                else
                {
                    return "SYSTEM";
                }
            }
            break;
        case OPERATING_SYSTEM:
            if(strlen(fcom_GetCustLang()) > 0)
            {
                if(!strncmp(fcom_GetCustLang(), "kr", 2))
                {
                    return "운영체제";
                }
                else
                {
                    return "OPERATING_SYSTEM";
                }
            }
            else
            {
                if(!strncmp(g_stProcReportInfo.szConfMailLang, "kr", 2))
                {
                    return "운영체제";
                }
                else
                {
                    return "OPERATING_SYSTEM";
                }
            }
            break;
        case CPU:
            if(strlen(fcom_GetCustLang()) > 0)
            {
                if(!strncmp(fcom_GetCustLang(), "kr", 2))
                {
                    return "CPU";
                }
                else
                {
                    return "CPU";
                }
            }
            else
            {
                if(!strncmp(g_stProcReportInfo.szConfMailLang, "kr", 2))
                {
                    return "CPU";
                }
                else
                {
                    return "CPU";
                }
            }
            break;
        case NET_ADAPTER:
            if(strlen(fcom_GetCustLang()) > 0)
            {
                if(!strncmp(fcom_GetCustLang(), "kr", 2))
                {
                    return "네트워크어댑터";
                }
                else
                {
                    return "NET_ADAPTER";
                }
            }
            else
            {
                if(!strncmp(g_stProcReportInfo.szConfMailLang, "kr", 2))
                {
                    return "네트워크어댑터";
                }
                else
                {
                    return "NET_ADAPTER";
                }
            }
            break;
        case WIFI:
            if(strlen(fcom_GetCustLang()) > 0)
            {
                if(!strncmp(fcom_GetCustLang(), "kr", 2))
                {
                    return "와이파이";
                }
                else
                {
                    return "WIFI";
                }
            }
            else
            {
                if(!strncmp(g_stProcReportInfo.szConfMailLang, "kr", 2))
                {
                    return "와이파이";
                }
                else
                {
                    return "WIFI";
                }
            }
            break;
        case BLUETOOTH:
            if(strlen(fcom_GetCustLang()) > 0)
            {
                if(!strncmp(fcom_GetCustLang(), "kr", 2))
                {
                    return "블루투스";
                }
                else
                {
                    return "BLUETOOTH";
                }
            }
            else
            {
                if(!strncmp(g_stProcReportInfo.szConfMailLang, "kr", 2))
                {
                    return "블루투스";
                }
                else
                {
                    return "BLUETOOTH";
                }
            }
            break;
        case NET_CONNECTION:
            if(strlen(fcom_GetCustLang()) > 0)
            {
                if(!strncmp(fcom_GetCustLang(), "kr", 2))
                {
                    return "네트워크연결";
                }
                else
                {
                    return "NET_CONNECTION";
                }
            }
            else
            {
                if(!strncmp(g_stProcReportInfo.szConfMailLang, "kr", 2))
                {
                    return "네트워크연결";
                }
                else
                {
                    return "NET_CONNECTION";
                }
            }
            break;
        case DISK:
            if(strlen(fcom_GetCustLang()) > 0)
            {
                if(!strncmp(fcom_GetCustLang(), "kr", 2))
                {
                    return "디스크";
                }
                else
                {
                    return "DISK";
                }
            }
            else
            {
                if(!strncmp(g_stProcReportInfo.szConfMailLang, "kr", 2))
                {
                    return "디스크";
                }
                else
                {
                    return "DISK";
                }
            }
            break;
        case NET_DRIVE:
            if(strlen(fcom_GetCustLang()) > 0)
            {
                if(!strncmp(fcom_GetCustLang(), "kr", 2))
                {
                    return "네트워크드라이브";
                }
                else
                {
                    return "NET_DRIVE";
                }
            }
            else
            {
                if(!strncmp(g_stProcReportInfo.szConfMailLang, "kr", 2))
                {
                    return "네트워크드라이브";
                }
                else
                {
                    return "NET_DRIVE";
                }
            }
            break;
        case OS_ACCOUNT:
            if(strlen(fcom_GetCustLang()) > 0)
            {
                if(!strncmp(fcom_GetCustLang(), "kr", 2))
                {
                    return "운영체제계정";
                }
                else
                {
                    return "OS_ACCOUNT";
                }
            }
            else
            {
                if(!strncmp(g_stProcReportInfo.szConfMailLang, "kr", 2))
                {
                    return "운영체제계정";
                }
                else
                {
                    return "OS_ACCOUNT";
                }
            }
            break;
        case SHARE_FOLDER:
            if(strlen(fcom_GetCustLang()) > 0)
            {
                if(!strncmp(fcom_GetCustLang(), "kr", 2))
                {
                    return "공유폴더";
                }
                else
                {
                    return "SHARE_FOLDER";
                }
            }
            else
            {
                if(!strncmp(g_stProcReportInfo.szConfMailLang, "kr", 2))
                {
                    return "공유폴더";
                }
                else
                {
                    return "SHARE_FOLDER";
                }
            }
            break;
        case INFRARED_DEVICE:
            if(strlen(fcom_GetCustLang()) > 0)
            {
                if(!strncmp(fcom_GetCustLang(), "kr", 2))
                {
                    return "적외선장치";
                }
                else
                {
                    return "INFRARED_DEVICE";
                }
            }
            else
            {
                if(!strncmp(g_stProcReportInfo.szConfMailLang, "kr", 2))
                {
                    return "적외선장치";
                }
                else
                {
                    return "INFRARED_DEVICE";
                }
            }
            break;
        case PROCESS:
            if(strlen(fcom_GetCustLang()) > 0)
            {
                if(!strncmp(fcom_GetCustLang(), "kr", 2))
                {
                    return "프로세스";
                }
                else
                {
                    return "PROCESS";
                }
            }
            else
            {
                if(!strncmp(g_stProcReportInfo.szConfMailLang, "kr", 2))
                {
                    return "프로세스";
                }
                else
                {
                    return "PROCESS";
                }
            }
            break;
        case ROUTER:
            if(strlen(fcom_GetCustLang()) > 0)
            {
                if(!strncmp(fcom_GetCustLang(), "kr", 2))
                {
                    return "공유기";
                }
                else
                {
                    return "ROUTER";
                }
            }
            else
            {
                if(!strncmp(g_stProcReportInfo.szConfMailLang, "kr", 2))
                {
                    return "공유기";
                }
                else
                {
                    return "ROUTER";
                }
            }
            break;
        case NET_PRINTER:
            if(strlen(fcom_GetCustLang()) > 0)
            {
                if(!strncmp(fcom_GetCustLang(), "kr", 2))
                {
                    return "네트워크프린터";
                }
                else
                {
                    return "NET_PRINTER";
                }
            }
            else
            {
                if(!strncmp(g_stProcReportInfo.szConfMailLang, "kr", 2))
                {
                    return "네트워크프린터";
                }
                else
                {
                    return "NET_PRINTER";
                }
            }
            break;
        case NET_SCAN:
            if(strlen(fcom_GetCustLang()) > 0)
            {
                if(!strncmp(fcom_GetCustLang(), "kr", 2))
                {
                    return "네트워크스캔";
                }
                else
                {
                    return "NET_SCAN";
                }
            }
            else
            {
                if(!strncmp(g_stProcReportInfo.szConfMailLang, "kr", 2))
                {
                    return "네트워크스캔";
                }
                else
                {
                    return "NET_SCAN";
                }
            }
            break;
        case ARP:
            if(strlen(fcom_GetCustLang()) > 0)
            {
                if(!strncmp(fcom_GetCustLang(), "kr", 2))
                {
                    return "ARP";
                }
                else
                {
                    return "ARP";
                }
            }
            else
            {
                if(!strncmp(g_stProcReportInfo.szConfMailLang, "kr", 2))
                {
                    return "ARP";
                }
                else
                {
                    return "ARP";
                }
            }
            break;
        case VIRTUAL_MACHINE:
            if(strlen(fcom_GetCustLang()) > 0)
            {
                if(!strncmp(fcom_GetCustLang(), "kr", 2))
                {
                    return "가상시스템";
                }
                else
                {
                    return "VIRTUAL_MACHINE";
                }
            }
            else
            {
                if(!strncmp(g_stProcReportInfo.szConfMailLang, "kr", 2))
                {
                    return "가상시스템";
                }
                else
                {
                    return "VIRTUAL_MACHINE";
                }
            }
            break;
        case CONNECT_EXT_SVR:
            if(strlen(fcom_GetCustLang()) > 0)
            {
                if(!strncmp(fcom_GetCustLang(), "kr", 2))
                {
                    return "외부네트워크";
                }
                else
                {
                    return "CONNECT_EXT_SVR";
                }
            }
            else
            {
                if(!strncmp(g_stProcReportInfo.szConfMailLang, "kr", 2))
                {
                    return "외부네트워크";
                }
                else
                {
                    return "CONNECT_EXT_SVR";
                }
            }
            break;
        case NET_ADAPTER_OVER:
            if(strlen(fcom_GetCustLang()) > 0)
            {
                if(!strncmp(fcom_GetCustLang(), "kr", 2))
                {
                    return "네트워크어댑터2개이상";
                }
                else
                {
                    return "NET_ADAPTER_OVER";
                }
            }
            else
            {
                if(!strncmp(g_stProcReportInfo.szConfMailLang, "kr", 2))
                {
                    return "네트워크어댑터2개이상";
                }
                else
                {
                    return "NET_ADAPTER_OVER";
                }
            }
            break;
        case NET_ADAPTER_DUPIP:
            if(strlen(fcom_GetCustLang()) > 0)
            {
                if(!strncmp(fcom_GetCustLang(), "kr", 2))
                {
                    return "IP주소중복탐지";
                }
                else
                {
                    return "NET_ADAPTER_DUPIP";
                }
            }
            else
            {
                if(!strncmp(g_stProcReportInfo.szConfMailLang, "kr", 2))
                {
                    return "IP주소중복탐지";
                }
                else
                {
                    return "NET_ADAPTER_DUPIP";
                }
            }
            break;
        case NET_ADAPTER_DUPMAC:
            if(strlen(fcom_GetCustLang()) > 0)
            {
                if(!strncmp(fcom_GetCustLang(), "kr", 2))
                {
                    return "MAC주소중복탐지";
                }
                else
                {
                    return "NET_ADAPTER_DUPMAC";
                }
            }
            else
            {
                if(!strncmp(g_stProcReportInfo.szConfMailLang, "kr", 2))
                {
                    return "MAC주소중복탐지";
                }
                else
                {
                    return "NET_ADAPTER_DUPMAC";
                }
            }
            break;
        case DISK_REG:
            if(strlen(fcom_GetCustLang()) > 0)
            {
                if(!strncmp(fcom_GetCustLang(), "kr", 2))
                {
                    return "미등록디스크";
                }
                else
                {
                    return "DISK_REG";
                }
            }
            else
            {
                if(!strncmp(g_stProcReportInfo.szConfMailLang, "kr", 2))
                {
                    return "미등록디스크";
                }
                else
                {
                    return "DISK_REG";
                }
            }
            break;
        case DISK_HIDDEN:
            if(strlen(fcom_GetCustLang()) > 0)
            {
                if(!strncmp(fcom_GetCustLang(), "kr", 2))
                {
                    return "디스크숨김";
                }
                else
                {
                    return "DISK_HIDDEN";
                }
            }
            else
            {
                if(!strncmp(g_stProcReportInfo.szConfMailLang, "kr", 2))
                {
                    return "디스크숨김";
                }
                else
                {
                    return "DISK_HIDDEN";
                }
            }
            break;
        case DISK_NEW:
            if(strlen(fcom_GetCustLang()) > 0)
            {
                if(!strncmp(fcom_GetCustLang(), "kr", 2))
                {
                    return "새로운디스크";
                }
                else
                {
                    return "DISK_NEW";
                }
            }
            else
            {
                if(!strncmp(g_stProcReportInfo.szConfMailLang, "kr", 2))
                {
                    return "새로운디스크";
                }
                else
                {
                    return "DISK_NEW";
                }
            }
            break;
        case DISK_MOBILE:
            if(strlen(fcom_GetCustLang()) > 0)
            {
                if(!strncmp(fcom_GetCustLang(), "kr", 2))
                {
                    return "이동식디스크";
                }
                else
                {
                    return "DISK_MOBILE";
                }
            }
            else
            {
                if(!strncmp(g_stProcReportInfo.szConfMailLang, "kr", 2))
                {
                    return "이동식디스크";
                }
                else
                {
                    return "DISK_MOBILE";
                }
            }
            break;
        case DISK_MOBILE_READ:
            if(strlen(fcom_GetCustLang()) > 0)
            {
                if(!strncmp(fcom_GetCustLang(), "kr", 2))
                {
                    return "디스크읽기권한";
                }
                else
                {
                    return "DISK_MOBILE_READ";
                }
            }
            else
            {
                if(!strncmp(g_stProcReportInfo.szConfMailLang, "kr", 2))
                {
                    return "디스크읽기권한";
                }
                else
                {
                    return "DISK_MOBILE_READ";
                }
            }
            break;
        case DISK_MOBILE_WRITE:
            if(strlen(fcom_GetCustLang()) > 0)
            {
                if(!strncmp(fcom_GetCustLang(), "kr", 2))
                {
                    return "디스크쓰기권한";
                }
                else
                {
                    return "DISK_MOBILE_WRITE";
                }
            }
            else
            {
                if(!strncmp(g_stProcReportInfo.szConfMailLang, "kr", 2))
                {
                    return "디스크쓰기권한";
                }
                else
                {
                    return "DISK_MOBILE_WRITE";
                }
            }
            break;
        case PROCESS_WHITE:
            if(strlen(fcom_GetCustLang()) > 0)
            {
                if(!strncmp(fcom_GetCustLang(), "kr", 2))
                {
                    return "화이트리스트프로세스";
                }
                else
                {
                    return "PROCESS_WHITE";
                }
            }
            else
            {
                if(!strncmp(g_stProcReportInfo.szConfMailLang, "kr", 2))
                {
                    return "화이트리스트프로세스";
                }
                else
                {
                    return "PROCESS_WHITE";
                }
            }
            break;
        case PROCESS_BLACK:
            if(strlen(fcom_GetCustLang()) > 0)
            {
                if(!strncmp(fcom_GetCustLang(), "kr", 2))
                {
                    return "블랙리스트프로세스";
                }
                else
                {
                    return "PROCESS_BLACK";
                }
            }
            else
            {
                if(!strncmp(g_stProcReportInfo.szConfMailLang, "kr", 2))
                {
                    return "블랙리스트프로세스";
                }
                else
                {
                    return "PROCESS_BLACK";
                }
            }
            break;
        case PROCESS_ACCESSMON:
            if(strlen(fcom_GetCustLang()) > 0)
            {
                if(!strncmp(fcom_GetCustLang(), "kr", 2))
                {
                    return "접속감시대상프로세스";
                }
                else
                {
                    return "PROCESS_ACCESSMON";
                }
            }
            else
            {
                if(!strncmp(g_stProcReportInfo.szConfMailLang, "kr", 2))
                {
                    return "접속감시대상프로세스";
                }
                else
                {
                    return "PROCESS_ACCESSMON";
                }
            }
            break;
        case NET_ADAPTER_MULIP:
            if(strlen(fcom_GetCustLang()) > 0)
            {
                if(!strncmp(fcom_GetCustLang(), "kr", 2))
                {
                    return "IP주소다중설정탐지";
                }
                else
                {
                    return "NET_ADAPTER_MULIP";
                }
            }
            else
            {
                if(!strncmp(g_stProcReportInfo.szConfMailLang, "kr", 2))
                {
                    return "IP주소다중설정탐지";
                }
                else
                {
                    return "NET_ADAPTER_MULIP";
                }
            }
            break;
        case EXTERNAL_CONN:
            if(strlen(fcom_GetCustLang()) > 0)
            {
                if(!strncmp(fcom_GetCustLang(), "kr", 2))
                {
                    return "외부접속탐지";
                }
                else
                {
                    return "EXTERNAL_CONN";
                }
            }
            else
            {
                if(!strncmp(g_stProcReportInfo.szConfMailLang, "kr", 2))
                {
                    return "외부접속탐지";
                }
                else
                {
                    return "EXTERNAL_CONN";
                }
            }
            break;
        case WIN_DRV:
            if(strlen(fcom_GetCustLang()) > 0)
            {
                if(!strncmp(fcom_GetCustLang(), "kr", 2))
                {
                    return "윈도우드라이버장치";
                }
                else
                {
                    return "WIN_DRV";
                }
            }
            else
            {
                if(!strncmp(g_stProcReportInfo.szConfMailLang, "kr", 2))
                {
                    return "윈도우드라이버장치";
                }
                else
                {
                    return "WIN_DRV";
                }
            }
            break;
        case RDP_SESSION:
            if(strlen(fcom_GetCustLang()) > 0)
            {
                if(!strncmp(fcom_GetCustLang(), "kr", 2))
                {
                    return "원격데스크톱연결탐지";
                }
                else
                {
                    return "RDP_SESSION";
                }
            }
            else
            {
                if(!strncmp(g_stProcReportInfo.szConfMailLang, "kr", 2))
                {
                    return "원격데스크톱연결탐지";
                }
                else
                {
                    return "RDP_SESSION";
                }
            }
            break;
        case SSO_CERT:
            if(strlen(fcom_GetCustLang()) > 0)
            {
                if(!strncmp(fcom_GetCustLang(), "kr", 2))
                {
                    return "SSO인증여부";
                }
                else
                {
                    return "SSO_CERT";
                }
            }
            else
            {
                if(!strncmp(g_stProcReportInfo.szConfMailLang, "kr", 2))
                {
                    return "SSO인증여부";
                }
                else
                {
                    return "SSO_CERT";
                }
            }
            break;
        case PROCESS_BLOCK:
            if(strlen(fcom_GetCustLang()) > 0)
            {
                if(!strncmp(fcom_GetCustLang(), "kr", 2))
                {
                    return "프로세스차단";
                }
                else
                {
                    return "PROCESS_BLOCK";
                }
            }
            else
            {
                if(!strncmp(g_stProcReportInfo.szConfMailLang, "kr", 2))
                {
                    return "프로세스차단";
                }
                else
                {
                    return "PROCESS_BLOCK";
                }
            }
            break;

        case CPU_USAGE_ALARM:
            if(strlen(fcom_GetCustLang()) > 0)
            {
                if(!strncmp(fcom_GetCustLang(), "kr", 2))
                {
                    return "CPU사용률알람";
                }
                else
                {
                    return "CPU_USAGE_ALARM";
                }
            }
            else
            {
                if(!strncmp(g_stProcReportInfo.szConfMailLang, "kr", 2))
                {
                    return "CPU사용률알람";
                }
                else
                {
                    return "CPU_USAGE_ALARM";
                }
            }
            break;

        case CPU_USAGE_CONTROL:
            if(strlen(fcom_GetCustLang()) > 0)
            {
                if(!strncmp(fcom_GetCustLang(), "kr", 2))
                {
                    return "CPU사용통제";
                }
                else
                {
                    return "CPU_USAGE_CONTROL";
                }
            }
            else
            {
                if(!strncmp(g_stProcReportInfo.szConfMailLang, "kr", 2))
                {
                    return "CPU사용통제";
                }
                else
                {
                    return "CPU_USAGE_CONTROL";
                }
            }
            break;

        default:
            if(strlen(fcom_GetCustLang()) > 0)
            {
                if(!strncmp(fcom_GetCustLang(), "kr", 2))
                {
                    return "알수없음";
                }
                else
                {
                    return "UNKNOWN";
                }
            }
            else
            {
                if(!strncmp(g_stProcReportInfo.szConfMailLang, "kr", 2))
                {
                    return "알수없음";
                }
                else
                {
                    return "UNKNOWN";
                }
            }
            break;
    }
}

int fcom_GetReportFile(char *fPath, char *fName, char *res)
{
    int     fsize = 0;
    char    repFullName[256];
    FILE    *fp = NULL;

    memset(repFullName, 0x00, sizeof(repFullName));
    sprintf(repFullName, "%s/%s", fPath, fName);

    fp = fopen(repFullName, "rb");
    if(fp == NULL)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in fopen, path(%s)",
                repFullName);
        return -1;
    }
    //파일 크기 구하기
    fseek(fp, 0, SEEK_END);
    fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    fread(res, fsize, 1, fp);
    fclose(fp);

    return fsize;
}
int fcom_ReportHeaderFilter(char *jsbuff, char *sChart, char *sUtil, int totSize)
{
    char *tmpbuff = NULL;
    int  i = 0,j = 0,k = 0,slen = 0;
    int	 workFlag = 1;

    slen = strlen(jsbuff);
    if(fcom_malloc((void**)&tmpbuff, sizeof(char)*(totSize+1)) != 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed");
        return (-1);
    }

    for (i=0,j=0; i<slen; i++)
    {
        if (jsbuff[i] != '@')
        {
            tmpbuff[j++] = jsbuff[i];
        }
        else
        {
            if(workFlag == 1)
            {
                strcat(tmpbuff, sChart);
                workFlag++;
            }
            else if(workFlag == 2)
            {
                strcat(tmpbuff, sUtil);
                workFlag++;
            }

            j=strlen(tmpbuff);
            for (k=0;k<20;k++,i++) //length {STRING}
            {
                if(jsbuff[i] == '#')
                {
                    break;
                }
            }
        }
    }

    strcpy(jsbuff, tmpbuff);
    fcom_MallocFree((void**)&tmpbuff);

    return strlen(jsbuff);
}

int fcom_ReportBodyFilter(
        char *jsbuff,
        char *strRange,
        char *strTable,
        int	 dFlag,
        int  cFlag,
        int	 totSize,
        char *grpName,
        char *szConfMailLang
)
{
    char *tmpbuff = NULL;
    int  i = 0,j = 0,k = 0,slen = 0;
    int	 workFlag = 1;
    char subject[50 +1] = {0x00,};
    char custSubject[50 +1] = {0x00,};

    if(strlen(g_szCustLang) > 0)
    {
        if(!strncmp(g_szCustLang, "kr", 2))
        {
            if(dFlag == 1)
            {
                strcpy(subject, " 일일 정기 보고서 ");
            }
            else if(dFlag == 2)
            {
                strcpy(subject, " 주간 정기 보고서 ");
            }
            else if(dFlag == 3)
            {
                strcpy(subject, " 월간 정기 보고서 ");
            }
            else
            {
                if(strlen(grpName) > 0)
                {
                    memset(custSubject, 0x00, sizeof(custSubject));
                    if(cFlag == 1)		sprintf(custSubject, " 일일 [ %s ] 그룹 보고서 ", grpName);
                    else if(cFlag == 2)	sprintf(custSubject, " 주간 [ %s ] 그룹 보고서 ", grpName);
                    else if(cFlag == 3)	sprintf(custSubject, " 월간 [ %s ] 그룹 보고서 ", grpName);
                }
                else
                {
                    if(cFlag == 1)
                    {
                        strcpy(custSubject, " 일일 요청 보고서 ");
                    }
                    else if(cFlag == 2)
                    {
                        strcpy(custSubject, " 주간 요청 보고서 ");
                    }
                    else if(cFlag == 3)
                    {
                        strcpy(custSubject, " 월간 요청 보고서 ");
                    }
                }
            }
        }
        else
        {
            if(dFlag == 1)
            {
                strcpy(subject, " Daily Regular Report ");
            }
            else if(dFlag == 2)
            {
                strcpy(subject, " Weekly Regular Report ");
            }
            else if(dFlag == 3)
            {
                strcpy(subject, " Monthly Regular Report ");
            }
            else
            {
                if(strlen(grpName) > 0)
                {
                    memset(custSubject, 0x00, sizeof(custSubject));
                    if(cFlag == 1)		sprintf(custSubject, " Daily [ %s ] Group Report ", grpName);
                    else if(cFlag == 2)	sprintf(custSubject, " Weekly [ %s ] Group Report ", grpName);
                    else if(cFlag == 3)	sprintf(custSubject, " Monthly [ %s ] Group Report ", grpName);
                }
                else
                {
                    if(cFlag == 1)
                    {
                        strcpy(custSubject, " Daily Request Report ");
                    }
                    else if(cFlag == 2)
                    {
                        strcpy(custSubject, " Weekly Request Report ");
                    }
                    else if(cFlag == 3)
                    {
                        strcpy(custSubject, " Monthly Request Report ");
                    }
                }
            }
        }
    } else {
        if(!strncmp(szConfMailLang, "kr", 2))
        {
            if(dFlag == 1)
            {
                strcpy(subject, " 일일 정기 보고서 ");
            }
            else if(dFlag == 2)
            {
                strcpy(subject, " 주간 정기 보고서 ");
            }
            else if(dFlag == 3)
            {
                strcpy(subject, " 월간 정기 보고서 ");
            }
            else
            {
                if(strlen(grpName) > 0)
                {
                    memset(custSubject, 0x00, sizeof(custSubject));
                    if(cFlag == 1)		sprintf(custSubject, " 일일 [ %s ] 그룹 보고서 ", grpName);
                    else if(cFlag == 2)	sprintf(custSubject, " 주간 [ %s ] 그룹 보고서 ", grpName);
                    else if(cFlag == 3)	sprintf(custSubject, " 월간 [ %s ] 그룹 보고서 ", grpName);
                }
                else
                {
                    if(cFlag == 1)
                    {
                        strcpy(custSubject, " 일일 요청 보고서 ");
                    }
                    else if(cFlag == 2)
                    {
                        strcpy(custSubject, " 주간 요청 보고서 ");
                    }
                    else if(cFlag == 3)
                    {
                        strcpy(custSubject, " 월간 요청 보고서 ");
                    }
                }
            }
        }
        else
        {
            if(dFlag == 1)
            {
                strcpy(subject, " Daily Regular Report ");
            }
            else if(dFlag == 2)
            {
                strcpy(subject, " Weekly Regular Report ");
            }
            else if(dFlag == 3)
            {
                strcpy(subject, " Monthly Regular Report ");
            }
            else
            {
                if(strlen(grpName) > 0)
                {
                    memset(custSubject, 0x00, sizeof(custSubject));
                    if(cFlag == 1)		sprintf(custSubject, " Daily [ %s ] Group Report ", grpName);
                    else if(cFlag == 2)	sprintf(custSubject, " Weekly [ %s ] Group Report ", grpName);
                    else if(cFlag == 3)	sprintf(custSubject, " Monthly [ %s ] Group Report ", grpName);
                }
                else
                {
                    if(cFlag == 1)
                    {
                        strcpy(custSubject, " Daily Request Report ");
                    }
                    else if(cFlag == 2)
                    {
                        strcpy(custSubject, " Weekly Request Report ");
                    }
                    else if(cFlag == 3)
                    {
                        strcpy(custSubject, " Monthly Request Report ");
                    }
                }
            }
        }
    }
    slen = strlen(jsbuff);
    if(fcom_malloc((void**)&tmpbuff, sizeof(char)*(totSize+1)) != 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed");
        return (-1);
    }

    for (i=0,j=0; i<slen; i++)
    {
        if (jsbuff[i] != '@')
        {
            tmpbuff[j++] = jsbuff[i];
        }
        else
        {
            if(workFlag == 1)
            {
                if(dFlag != 0)		strcat(tmpbuff, subject);
                else				strcat(tmpbuff, custSubject);
                workFlag++;
            }
            else if(workFlag == 2)
            {
                strcat(tmpbuff, strRange);
                workFlag++;
            }
            else if(workFlag == 3)
            {
                strcat(tmpbuff, strTable);
                workFlag++;
            }

            j=strlen(tmpbuff);
            for (k=0;k<20;k++,i++) //length {STRING}
            {
                if(jsbuff[i] == '#')
                {
                    break;
                }
            }
        }
    }

    strcpy(jsbuff, tmpbuff);
//    fcom_BufferFree(tmpbuff);
    fcom_MallocFree((void**)&tmpbuff);

    return strlen(jsbuff);
}
int fcom_ReportFooterFilter(char *jsbuff, int totSize, char* szConfMailLang)
{
    char *tmpbuff = NULL;
    int  i = 0,j = 0,k = 0,slen = 0;
    int	 workFlag = 1;
    char sRankTitle[64 +1] = {0x00,};
    char sTypeTitle[64 +1] = {0x00,};
    char sGroupTitle[64 +1] = {0x00,};

    if(strlen(g_szCustLang) > 0)
    {
        if(!strncmp(g_szCustLang, "kr", 2))
        {
            strcpy(sRankTitle, "< IP별 이벤트 TOP 10 >");
            strcpy(sTypeTitle, "< 항목별 이벤트 >");
            strcpy(sGroupTitle, "< 그룹별 이벤트 >");
        }
        else
        {
            strcpy(sRankTitle, "< Event IP Address >");
            strcpy(sTypeTitle, "< Event Category >");
            strcpy(sGroupTitle, "< Event Group >");
        }
    }
    else
    {
        if(!strncmp(szConfMailLang, "kr", 2))
        {
            strcpy(sRankTitle, "< IP별 이벤트 TOP 10 >");
            strcpy(sTypeTitle, "< 항목별 이벤트 >");
            strcpy(sGroupTitle, "< 그룹별 이벤트 >");
        }
        else
        {
            strcpy(sRankTitle, "< Event IP Address >");
            strcpy(sTypeTitle, "< Event Category >");
            strcpy(sGroupTitle, "< Event Group >");
        }
    }

    slen = strlen(jsbuff) ;
    if(fcom_malloc((void**)&tmpbuff,sizeof(char)*(totSize+1)) != 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed");
        return (-1);
    }

    for (i=0,j=0; i<slen; i++)
    {
        if (jsbuff[i] != '@')
        {
            tmpbuff[j++] = jsbuff[i];
        }
        else
        {
            if(workFlag == 1)
            {
                strcat(tmpbuff, sRankTitle);
                workFlag++;
            }
            else if(workFlag == 2)
            {
                strcat(tmpbuff, sTypeTitle);
                workFlag++;
            }
            else if(workFlag == 3)
            {
                strcat(tmpbuff, sGroupTitle);
                workFlag++;
            }

            j=strlen(tmpbuff);
            for (k=0;k<10;k++,i++) //length {STRING}
            {
                if(jsbuff[i] == '#')
                {
                    break;
                }
            }
        }
    }

    strcpy(jsbuff, tmpbuff);
//    fcom_BufferFree(tmpbuff);
    fcom_MallocFree((void**)&tmpbuff);

    return strlen(jsbuff);
}


int fcom_ReportPieFilter(char *jsbuff, char *sVar, char *jsonData, char* szConfMailLang, char* szConfMailClosedView)
{
    int  i,j,k,slen;
    int  workFlag = 1;
    char tmpbuff[4096];
    char sTitle[64];
    char sLevel[64];


    if(strlen(g_szCustLang) > 0)
    {
        if(!strncmp(g_szCustLang, "kr", 2))
        {
            strcpy(sTitle, "< 이벤트 건수 >");
            if( !strcmp(szConfMailClosedView, "yes") )
            {
//                strcpy(sLevel, "'해지','경고','위험','차단'");
                strcpy(sLevel, "'경고','위험','차단'");
            }
            else
            {
                strcpy(sLevel, "'경고','위험','차단'");
            }
        }
        else
        {
            strcpy(sTitle, "< Event Count >");
            if( !strcmp(szConfMailClosedView, "yes") )
            {
//                strcpy(sLevel, "'Close','Warning','Critical','Block'");
                strcpy(sLevel, "'Warning','Critical','Block'");
            }
            else
            {
                strcpy(sLevel, "'Warning','Critical','Block'");
            }
        }
    }
    else
    {
        if(!strncmp(szConfMailLang, "kr", 2))
        {
            strcpy(sTitle, "< 이벤트 건수 >");
            if( !strcmp(szConfMailClosedView, "yes") )
            {
//                strcpy(sLevel, "'해지','경고','위험','차단'");
                strcpy(sLevel, "'경고','위험','차단'");
            }
            else
            {
                strcpy(sLevel, "'경고','위험','차단'");
            }
        }
        else
        {
            strcpy(sTitle, "< Event Count >");
            if( !strcmp(szConfMailClosedView, "yes") )
            {
//                strcpy(sLevel, "'Close','Warning','Critical','Block'");
                strcpy(sLevel, "'Warning','Critical','Block'");
            }
            else
            {
                strcpy(sLevel, "'Warning','Critical','Block'");
            }
        }
    }

    slen = strlen(jsbuff);
    memset(tmpbuff, 0x00, sizeof(tmpbuff));

    for (i=0,j=0; i<slen; i++)
    {
        if (jsbuff[i] != '@')
        {
            tmpbuff[j++] = jsbuff[i];
        }
        else
        {
            if(workFlag == 1)
            {
                strcat(tmpbuff, sVar);
                workFlag++;
            }
            else if(workFlag == 2)
            {
                strcat(tmpbuff, jsonData);
                workFlag++;
            }
            else if(workFlag == 3)
            {
                strcat(tmpbuff, sLevel);
                workFlag++;
            }
            else if(workFlag == 4)
            {
                strcat(tmpbuff, sTitle);
                workFlag++;
            }

            j=strlen(tmpbuff);
            for (k=0;k<10;k++,i++) //length {STRING}
            {
                if(jsbuff[i] == '#')
                {
                    break;
                }
            }
        }
    }

    strcpy(jsbuff, tmpbuff);

    return strlen(jsbuff);
}

int fcom_ReportLineFilter(
        char *jsbuff,
        char *sVar,
        char *sTp,
        char *sClose,
        char *sWarn,
        char *sCrit,
        char *sBlok
)
{
    char tmpbuff[5012];
    int  i,j,k,slen;
    int  workFlag = 1;
    char sTitle[64];
    char label1[10];
    char label2[10];
    char label3[10];
    char label4[10];

    if(strlen(g_szCustLang) > 0)
    {
        if(!strncmp(g_szCustLang, "kr", 2))
        {
            strcpy(sTitle, "< 이벤트 타임라인 >");
            if( !strcmp(g_stProcReportInfo.szConfMailClosedView, "yes") )
            {
                strcpy(label1, "'해지'");
            }
            strcpy(label2, "'경고'");
            strcpy(label3, "'위험'");
            strcpy(label4, "'차단'");
        }
        else
        {
            strcpy(sTitle, "< Event Timeline >");
            if( !strcmp(g_stProcReportInfo.szConfMailClosedView, "yes") )
            {
                strcpy(label1, "'Close'");
            }

            strcpy(label2, "'Warning'");
            strcpy(label3, "'Critical'");
            strcpy(label4, "'Block'");
        }
    }
    else
    {
        if(!strncmp(g_stProcReportInfo.szConfMailLang, "kr", 2))
        {
            strcpy(sTitle, "< 이벤트 타임라인 >");
            if( !strcmp(g_stProcReportInfo.szConfMailClosedView, "yes") )
            {
                strcpy(label1, "'해지'");
            }

            strcpy(label2, "'경고'");
            strcpy(label3, "'위험'");
            strcpy(label4, "'차단'");
        }
        else
        {
            strcpy(sTitle, "< Event Timeline >");
            if( !strcmp(g_stProcReportInfo.szConfMailClosedView, "yes") )
            {
                strcpy(label1, "'Close'");
            }

            strcpy(label2, "'Warning'");
            strcpy(label3, "'Critical'");
            strcpy(label4, "'Block'");
        }
    }

    slen = strlen(jsbuff);
    memset(tmpbuff, 0x00, sizeof(tmpbuff));

    for (i=0,j=0; i<slen; i++)
    {
        if (jsbuff[i] != '@')
        {
            tmpbuff[j++] = jsbuff[i];
        }
        else
        {
            if( !strcmp(g_stProcReportInfo.szConfMailClosedView, "yes") )
            {
                if(workFlag == 1)
                {
                    strcat(tmpbuff, sVar);
                    workFlag++;
                }
                else if(workFlag == 2)
                {
                    strcat(tmpbuff, sTp);
                    workFlag++;
                }
                else if(workFlag == 3)
                {
                    strcat(tmpbuff, label1);
                    workFlag++;
                }
                else if(workFlag == 4)
                {
                    strcat(tmpbuff, sClose);
                    workFlag++;
                }
                else if(workFlag == 5)
                {
                    strcat(tmpbuff, label2);
                    workFlag++;
                }
                else if(workFlag == 6)
                {
                    strcat(tmpbuff, sWarn);
                    workFlag++;
                }
                else if(workFlag == 7)
                {
                    strcat(tmpbuff, label3);
                    workFlag++;
                }
                else if(workFlag == 8)
                {
                    strcat(tmpbuff, sCrit);
                    workFlag++;
                }
                else if(workFlag == 9)
                {
                    strcat(tmpbuff, label4);
                    workFlag++;
                }
                else if(workFlag == 10)
                {
                    strcat(tmpbuff, sBlok);
                    workFlag++;
                }
                else if(workFlag == 11)
                {
                    strcat(tmpbuff, sTitle);
                    workFlag++;
                }
            }
            else
            {
                if(workFlag == 1)
                {
                    strcat(tmpbuff, sVar);
                    workFlag++;
                }
                else if(workFlag == 2)
                {
                    strcat(tmpbuff, sTp);
                    workFlag++;
                }
                else if(workFlag == 3)
                {
                    strcat(tmpbuff, label2);
                    workFlag++;
                }
                else if(workFlag == 4)
                {
                    strcat(tmpbuff, sWarn);
                    workFlag++;
                }
                else if(workFlag == 5)
                {
                    strcat(tmpbuff, label3);
                    workFlag++;
                }
                else if(workFlag == 6)
                {
                    strcat(tmpbuff, sCrit);
                    workFlag++;
                }
                else if(workFlag == 7)
                {
                    strcat(tmpbuff, label4);
                    workFlag++;
                }
                else if(workFlag == 8)
                {
                    strcat(tmpbuff, sBlok);
                    workFlag++;
                }
                else if(workFlag == 9)
                {
                    strcat(tmpbuff, sTitle);
                    workFlag++;
                }
            }
            j=strlen(tmpbuff);
            for (k=0;k<10;k++,i++) //length {STRING}
            {
                if(jsbuff[i] == '#')
                {
                    break;
                }
            }
        }
    }

    strcpy(jsbuff, tmpbuff);

    return strlen(jsbuff);
}

int fcom_ReportBarFilter(
        char *jsbuff,
        char *sVar,
        char *sTp,
        char *sClose,
        char *sWarn,
        char *sCrit,
        char *sBlok
)
{
    char tmpbuff[5012];
    int  i,j,k,slen;
    int  workFlag = 1;
    char label1[10];
    char label2[10];
    char label3[10];
    char label4[10];

    slen = strlen(jsbuff);
    memset(tmpbuff, 0x00, sizeof(tmpbuff));

    if(strlen(g_szCustLang) > 0)
    {
        if(!strncmp(g_szCustLang, "kr", 2))
        {
            if( !strcmp(g_stProcReportInfo.szConfMailClosedView, "yes") )
            {
                strcpy(label1, "'해지'");
            }
            strcpy(label2, "'경고'");
            strcpy(label3, "'위험'");
            strcpy(label4, "'차단'");
        }
        else
        {
            if( !strcmp(g_stProcReportInfo.szConfMailClosedView, "yes") )
            {
                strcpy(label1, "'Close'");
            }
            strcpy(label2, "'Warning'");
            strcpy(label3, "'Critical'");
            strcpy(label4, "'Block'");
        }
    }
    else
    {
        if(!strncmp(g_stProcReportInfo.szConfMailLang, "kr", 2))
        {
            if( !strcmp(g_stProcReportInfo.szConfMailClosedView, "yes") )
            {
                strcpy(label1, "'해지'");
            }
            strcpy(label2, "'경고'");
            strcpy(label3, "'위험'");
            strcpy(label4, "'차단'");
        }
        else
        {
            if( !strcmp(g_stProcReportInfo.szConfMailClosedView, "yes") )
            {
                strcpy(label1, "'Close'");
            }
            strcpy(label2, "'Warning'");
            strcpy(label3, "'Critical'");
            strcpy(label4, "'Block'");
        }
    }

    for (i=0,j=0; i<slen; i++)
    {
        if (jsbuff[i] != '@')
        {
            tmpbuff[j++] = jsbuff[i];
        }
        else
        {
            if( !strcmp(g_stProcReportInfo.szConfMailClosedView, "yes") )
            {
                if(workFlag == 1)
                {
                    strcat(tmpbuff, sVar);
                    workFlag++;
                }
                else if(workFlag == 2)
                {
                    strcat(tmpbuff, sTp);
                    workFlag++;
                }
                else if(workFlag == 3)
                {
                    strcat(tmpbuff, label1);
                    workFlag++;
                }
                else if(workFlag == 4)
                {
                    strcat(tmpbuff, sClose);
                    workFlag++;
                }
                else if(workFlag == 5)
                {
                    strcat(tmpbuff, label2);
                    workFlag++;
                }
                else if(workFlag == 6)
                {
                    strcat(tmpbuff, sWarn);
                    workFlag++;
                }
                else if(workFlag == 7)
                {
                    strcat(tmpbuff, label3);
                    workFlag++;
                }
                else if(workFlag == 8)
                {
                    strcat(tmpbuff, sCrit);
                    workFlag++;
                }
                else if(workFlag == 9)
                {
                    strcat(tmpbuff, label4);
                    workFlag++;
                }
                else if(workFlag == 10)
                {
                    strcat(tmpbuff, sBlok);
                    workFlag++;
                }
            }
            else
            {
                if(workFlag == 1)
                {
                    strcat(tmpbuff, sVar);
                    workFlag++;
                }
                else if(workFlag == 2)
                {
                    strcat(tmpbuff, sTp);
                    workFlag++;
                }
                else if(workFlag == 3)
                {
                    strcat(tmpbuff, label2);
                    workFlag++;
                }
                else if(workFlag == 4)
                {
                    strcat(tmpbuff, sWarn);
                    workFlag++;
                }
                else if(workFlag == 5)
                {
                    strcat(tmpbuff, label3);
                    workFlag++;
                }
                else if(workFlag == 6)
                {
                    strcat(tmpbuff, sCrit);
                    workFlag++;
                }
                else if(workFlag == 7)
                {
                    strcat(tmpbuff, label4);
                    workFlag++;
                }
                else if(workFlag == 8)
                {
                    strcat(tmpbuff, sBlok);
                    workFlag++;
                }
            }

            j=strlen(tmpbuff);
            for (k=0;k<10;k++,i++) //length {STRING}
            {
                if(jsbuff[i] == '#')
                {
                    break;
                }
            }
        }
    }

    strcpy(jsbuff, tmpbuff);

    return strlen(jsbuff);
}
