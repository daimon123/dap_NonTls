//
// Created by KimByoungGook on 2020-06-22.
//

#ifndef DAP_QDEF_H
#define DAP_QDEF_H

//#define     QCNT	100
#define     QCNT	200
//#define		QBUFSIZE  204800
#define		DATABUFSIZE  204800


#pragma pack(1)
typedef struct
{
    int packtype;
    unsigned char buf[DATABUFSIZE];
} _DAP_QUEUE_BUF;
#pragma pack()


typedef struct
{
    long	cur_time;
    long	last_job_time;
    long	last_send_time;
}_DAP_LINKER_INFO;

_DAP_LINKER_INFO g_stLinkerInfo;


#endif //DAP_QDEF_H
