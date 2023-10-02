//
// Created by KimByoungGook on 2020-06-12.
//

#ifndef DAP_DEFIELD_H
#define DAP_DEFIELD_H

typedef struct
{
    unsigned int mgwid;
    unsigned int total;
    unsigned int used;
    unsigned int avail;
    unsigned int capacity;
    char filesystem[80];
}_DAP_DB_FILSYSTEM_STAT;

typedef struct
{
    char		category[80];
    unsigned int	critical;
    unsigned int	major;
    unsigned int	minor;
} _DAP_DB_THRESHOLD_INFO;

typedef struct
{
    unsigned int	mgwid;
    unsigned int	mem_tot;
    unsigned int	mem_used;
    unsigned int	mem_free;
    unsigned int	mem_perused;
    unsigned int	cpu_user;
    unsigned int	cpu_sys;
    unsigned int	cpu_idle;
} _DAP_DB_SYSTEM_LOAD_INFO;

typedef struct
{
    unsigned int	mgwid;
    unsigned int	total;
    unsigned int	used;
    unsigned int	avail;
    char 			name[30];
}_DAP_DB_QUEUE_STAT;

//typedef struct
//{
//    int pid;
//    char process[20];
//    char errorlevel[10];
//    char detail[512];
//    char flag;
//    char createdate[19+1];
//} _DAP_DB_DBLOG_INFO;

/** 서버 로그수집 Struct **/
typedef struct
{
    int pid;            //서버 프로세스 pid
    char process[20];   //서버 프로세스명
    char logdate[24];   //서버 로그시간, YYYY-MM-DD hhmmss
    char loglevel[10];  //서버 로그레벨
    char logip[16];     //서버 IP
    char logmsg[1024];  //서버 로그메시지
} _DAP_DB_SERVERLOG_INFO;


/** 에이전트 로그수집 Struct **/
typedef struct
{
    char hbsq[20];      //에이전트 장치번호
    char hbunq[24];     //에이전트 유저키
    char ip[15];        //IP
    char process[20];   //PROCESS
    char logdate[24];   //로그시간
    char loglevel[10];  //로그레벨
    char logmsg[1024];  //로그메시지
} _DAP_DB_AGENTLOG_INFO;


typedef struct
{
    unsigned char	mn_cell_phone[32+1];
    unsigned char	mn_email[64+1];
    unsigned short	al_detect_type;
    unsigned char	al_detect_level;
    unsigned char	al_use;
}_DAP_DB_ALARM_INFO;


/*typedef struct
{
    unsigned int mgwid;
    unsigned int total;
    unsigned int used;
    unsigned int avail;
    unsigned int capacity;
    char filesystem[80];
}_DAP_FILSYSTEM_STAT;*/

/*typedef struct
{
    int pid;
    unsigned char process[20];
    unsigned char errorlevel[10];
    unsigned char detail[512];
    unsigned char flag;
    unsigned char createdate[19+1];
} _DAP_DBLOG_TB_INFO;*/

/* DAP Sysman DB File */
/*typedef struct
{
    char		category[80];
    unsigned int	critical;
    unsigned int	major;
    unsigned int	minor;
} _DAP_THRESHOLD_INFO;*/


/*typedef struct
{
    unsigned int	mgwid;
    unsigned int	mem_tot;
    unsigned int	mem_used;
    unsigned int	mem_free;
    unsigned int	mem_perused;
    unsigned int	cpu_user;
    unsigned int	cpu_sys;
    unsigned int	cpu_idle;
} _DAP_SYSTEM_LOAD_INFO;*/


/*typedef struct
{
    unsigned int	mgwid;
    unsigned int	total;
    unsigned int	used;
    unsigned int	avail;
    char 			name[30];
}_DAP_QUEUE_STAT;*/

#endif //DAP_DEFIELD_H
