//
// Created by KimByoungGook on 2020-10-28.
//

#ifndef _DAP_VERSION_H
#define _DAP_VERSION_H

/* Make 빌드시 생성되는 헤더파일 */
#include "dap_current_date.h"

#ifndef	VERSION_DF_program_name
#define		VERSION_DF_program_name							"DAP_SERVER-Program"
#endif

#ifndef	VERSION_DF_program_ver
#define		VERSION_DF_program_ver									"2.2.3"
#endif

#ifndef	VERSION_DF_program_desc
#define		VERSION_DF_program_desc									"1.0.0 : Init Release Version \n" \
                                                                    " -> IBK Bank Issue Complete Version\n" \
                                                                    "1.0.1 : - MODIFY Output Log  \n" \
                                                                    " - dbif : MODIFY SEGV \n" \
                                                                    " - sysman : MODIFY SEGV \n" \
                                                                    " - pcif : MODIFY performance improve\n" \
                                                                    "1.0.2 : pcif ssl Accept NonBlock\n"      \
                                                                    " - pcif : ADD agent_self_protect config\n" \
                                                                    " - schd : MODIFY MANAGER_EVENT_TB History\n" \
                                                                    "1.1.0 : pcif ssl Accept NonBlock\n"      \
                                                                    " - pcif : -> Rule White/Black Process Modify\n"\
                                                                    "          -> Delete  Reference Table RULE_DETECT_TB\n"\
                                                                    " - schd : -> Create Next Year History Table Bugs Modify \n" \
                                                                    "1.1.1 : pcif White Process Rule Add \n" \
                                                                    " - pcif : 0.0.0.0, All Ip range Rule Add \n" \
                                                                    "1.1.2 : Except Buffer Overflow \n" \
                                                                    " - dap_report : segv modify\n"    \
                                                                    "2.0.1 : dbif Process Delete\n" \
                                                                    " - DB Data File Handleing \n" \
                                                                    "2.0.2 : Modify Policy File Logic \n"  \
                                                                    "2.0.3 : Add BASE_STATUS_HISTORY Table \n" \
                                                                    " - Add Column RULE_TB Agent Self Protect \n" \
                                                                    "2.0.4 : PCIF Openssl Error Modify \n" \
                                                                    " -  OpenSSL_add_ssl_algorithms -> OpenSSL_add_all_algorithm \n" \
                                                                    " -  method sslv2_3 -> tlsv1_2 \n" \
                                                                    " -  Add Openssl Config Value Accept Block Mode / Non Block Mode \n" \
                                                                    " -  Add frd dump function  \n" \
                                                                    "2.0.5  \n" \
                                                                    " - Modify dap_proxy Memory Leak \n" \
                                                                    " - Modify DB Connection Encoding \n"   \
                                                                    "2.0.6 \n" \
                                                                    " - Modify dap_pcif Update Check Bug \n" \
                                                                    " - Add dap_manager Update Check \n" \
                                                                    "2.1.0 \n"\
                                                                    " - Add Server / Agent Log\n" \
                                                                    "2.1.1 \n"\
                                                                    " - WIN DRV duplicate Bug Fix \n"       \
                                                                    "2.1.2 \n"                             \
                                                                    " - Virtual Process Detect Bug Fix \n" \
                                                                    " 2.1.3 \n" \
                                                                    " - Thread Hang Check -> Pthread Kill \n" \
                                                                    " - dblog default disable \n" \
                                                                    " 2.1.4 \n" \
                                                                    " - 업데이트서버 버전, Manager 버전 불일치시 auto_update_exec false로 전송하도록 변경\n" \
                                                                    " - 환경설정 변경사항 Reload 주기마다 못하는 버그 수정.\n" \
                                                                    " - 접속감시(Access_mon) 기능 버그 수정.\n" \
                                                                    " 2.1.5\n" \
                                                                    " - 접속감시(Access_mon)랑 White Process 정책 중복시 버그 수정.\n" \
                                                                    " - Manager proxy 통신 수정. 필히 Manager Version과 서버 Version 확인 필요.\n" \
                                                                    " - 정책 Process 개수 많은경우 메모리 터짐현상 수정.\n" \
                                                                    "2.1.6 \n" \
                                                                    " - 리포트 이력조회와 카운트 불일치 현상 수정. \n" \
                                                                    "2.2.0 \n" \
                                                                    " - 리포트 이력조회와 카운트 불일치 현상 완료. \n" \
                                                                    " - proxy thread 재사용 로직 수정.\n" \
                                                                    "2.2.1 \n" \
                                                                    " - Process Bug Fix \n"\
                                                                    "2.2.2 \n" \
                                                                    " - DiffTime 버그 수정 \n" \
                                                                    " - Thread HangCheck 원복 \n" \
                                                                    "2.2.3 \n" \
                                                                    " - dap_report 실시간처리  \n" \
                                                                    " - WIN_DRV History DB성능이슈로 주석처리. \n"
#endif


#define DAP_VERSION_MACRO {                                 \
    printf("\n");                                           \
    printf("%s # v%s INTENT SECURE Inc. \n",                \
    VERSION_DF_program_name,                                \
    VERSION_DF_program_ver);                                \
    printf("\n");                                           \
    printf("Release Date -> %s\n",VERSION_DF_compile_time); \
    printf("\n");                                           \
    printf("[%s]",VERSION_DF_program_desc);               \
    printf("\n");                                           \
}



#endif //_DAP_VERSION_H
