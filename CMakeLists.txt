

##############################################################################
#  Intent Secure Inc. DAP Server Cmake File                                  #
#  최초 작성일 2020.11.03                                                      #
##############################################################################
# 참고링크
# https://gist.github.com/luncliff/6e2d4eb7ca29a0afd5b592f72b80cb5c
# https://www.tuwlab.com/ece/27260

# *****  옵션정보 *****

# 전처리기 추가( -D )
# ADD_DEFINITIONS() - 전처리기 매크로 추가
# ADD_DEFINITIONS( -DICACHE_FLASH -DMY_DEBUG=1 )

# 헤더 디렉토리 추가 (-I)
# INCLUDE_DIRECTORIES ( include driver/include )

# 링크 디렉토리 추가 (-L)
# LINK_DIRECTORIES ( <디렉토리> <디렉토리> ... )

# 컴파일 옵션 추가
# ADD_COMPILE_OPTIONS( -g -Wall )

# 링크 옵션 추가
# LINK_LIBRARIES ( uart wifi )

# 빌드 대상 라이브러리 생성
# ADD_LIBRARY ( app STATIC foo.c bar.c )

# 빌드 대상 바이너리 추가
# ADD_EXECUTABLE ( app.out main.c foo.c bar.c )

##############################################################################

# C MAKE 버전
cmake_minimum_required(VERSION 2.8)

# 프로젝트명
project(dap_v2 C)

set(CMAKE_C_STANDARD 11)


# Remote Maria Library PATH
set(LIB_MYSQL_HEADER_PATH
        /usr/include/mysql/*.h
)


# Remote Openssl Library PATH
set(LIB_OPENSSL_HEADER
        /home/intent/openssl-1.0.2k/include
)



# Local DAP COM Library
file(GLOB USER_LIB_COM_HEADER
        lib_inc/com/*.h
)
file(GLOB USER_LIB_COM_SOURCE
        lib_src/com/*.c
)


# Local DAP DB Library Header
file(GLOB USER_LIB_DB_HEADER
        lib_inc/db/*.h
)
file(GLOB USER_LIB_DB_SOURCE
        lib_src/db/*.c
)


# Local DAP IPC Library Header
file(GLOB USER_LIB_IPC_HEADER
        lib_inc/ipc/*.h
)
file(GLOB USER_LIB_IPC_SOURCE
        lib_src/ipc/*.c
)


# Local DAP JSON Library Header
file(GLOB USER_LIB_JSON_HEADER
        lib_inc/json/*.h
)
file(GLOB USER_LIB_JSON_SOURCE
        lib_src/json/*.c
)


# Local DAP LINUXKE Library Header
file(GLOB USER_LIB_LINUXKE_HEADER
        lib_inc/linuxke/*.h
)
file(GLOB USER_LIB_LINUXKE_SOURCE
        lib_src/linuxke/*.c
)

# Local DAP LINUXKE Library Header
file(GLOB USER_LIB_SECURE_HEADER
        lib_inc/secure/*.h
)
file(GLOB USER_LIB_SECURE_SOURCE
        lib_src/secure/*.c
)

# Local DAP SOCK Library Header
file(GLOB USER_LIB_SOCK_HEADER
        lib_inc/sock/*.h
)
file(GLOB USER_LIB_SOCK_SOURCE
        lib_src/sock/*.c
)


# 전처리 추가(-D)
ADD_DEFINITIONS( -D_GNU_SOURCE -D_DAP_)

# 컴파일 옵션추가
ADD_COMPILE_OPTIONS( -g -c -W -Wall -Wno-pointer-sign -Wno-sign-compare -Wno-unused-parameter)

# 헤더 디렉토리 추가 (-I)
INCLUDE_DIRECTORIES (
        ${LIB_MYSQL_HEADER_PATH}
        ${LIB_OPENSSL_HEADER}
        lib_inc
        proc_inc
)

# 링크 디렉토리 추가 (-L)
LINK_DIRECTORIES (
        /usr/lib64/mysql
)


# User Library 빌드
#libcom.a
ADD_LIBRARY (
        com
        STATIC
        ${USER_LIB_COM_SOURCE}
        lib_src/com/dap_pidfile.c)
#libdb.a
ADD_LIBRARY (
        db
        STATIC
        ${USER_LIB_DB_SOURCE}
)
#libipc.a
ADD_LIBRARY (
        ipc
        STATIC
        ${USER_LIB_IPC_SOURCE}
)
#libjson.a
ADD_LIBRARY (
        json
        STATIC
        ${USER_LIB_JSON_SOURCE}
)
# libke.a
ADD_LIBRARY (
        ke
        STATIC
        ${USER_LIB_LINUXKE_SOURCE}
)
#libsecure.a
ADD_LIBRARY (
        secure
        STATIC
        ${USER_LIB_SECURE_SOURCE}
)
#libsock.a
ADD_LIBRARY (
        sock
        STATIC
        ${USER_LIB_SOCK_SOURCE}
)




##################################################
# Target : DAP_ALARM
file(GLOB DAP_ALARM_SRC
        proc_src/alarm/*.c
)
add_executable(dap_alarm
        ${DAP_ALARM_SRC}
)
# 라이브러리 링크 추가(-l)
TARGET_LINK_LIBRARIES( dap_alarm
        com db ke secure ipc pthread crypto m mysqlclient_r
)
##################################################



##################################################
## Target : DAP_DBIF
#file(GLOB DAP_DBIF_SRC
#        proc_src/dbif/*.c
#)
#add_executable(dap_dbif
#        ${DAP_DBIF_SRC}
#)
# Target : DAP_DBIF
file(GLOB DAP_FRD_SRC
        proc_src/frd/*.c
)
add_executable(dap_frd
        ${DAP_FRD_SRC} proc_src/frd/frd_file.c proc_src/frd/frd_init.c proc_src/frd/frd_signal.c proc_src/frd/frd_dump.c)

file(GLOB DAP_FW_SRC
        proc_src/policy_fw/*.c
        )
add_executable(dap_fw
        ${DAP_FW_SRC})
## 라이브러리 링크 추가(-l)
#TARGET_LINK_LIBRARIES( dap_dbif
#        db com ke secure ipc pthread crypto json ssl m mysqlclient_r
#)
## Target 사용자 정의 Command 추가.
#ADD_CUSTOM_COMMAND( TARGET dap_dbif
#    COMMAND /bin/cp dap_dbif dap_dbif_01
#    COMMAND /bin/cp dap_dbif dap_dbif_02
#    COMMAND /bin/cp dap_dbif dap_dbif_03
#    COMMAND /bin/cp dap_dbif dap_dbif_04
#    COMMAND /bin/cp dap_dbif dap_dbif_05
#    COMMAND /bin/cp dap_dbif dap_dbif_06
#    COMMAND /bin/cp dap_dbif dap_dbif_07
#    COMMAND /bin/cp dap_dbif dap_dbif_08
#    COMMAND /bin/cp dap_dbif dap_dbif_09
#    COMMAND /bin/cp dap_dbif dap_dbif_10
#    COMMAND rm dap_dbif
#)
##################################################


##################################################
# Target : DAP_DBLOG
file(GLOB DAP_DBLOG_SRC
        proc_src/dblog/*.c
)
add_executable(dap_dblog
        ${DAP_DBLOG_SRC}
)
# 라이브러리 링크 추가(-l)
TARGET_LINK_LIBRARIES( dap_dblog
        db com ke secure ipc pthread crypto m mysqlclient_r
)
##################################################


##################################################
# Target : DAP_MASTER
file(GLOB DAP_MASTER_SRC
        proc_src/master/*.c
)
add_executable(dap_master
        ${DAP_MASTER_SRC}
        proc_src/master/master_signal.c)
# 라이브러리 링크 추가(-l)
TARGET_LINK_LIBRARIES( dap_master
        c db com ke secure ipc pthread crypto m ssl mysqlclient_r
)
##################################################


##################################################
# Target : DAP_PCIF
file(GLOB DAP_PCIF_SRC
        proc_src/pcif/*.c
)
add_executable(dap_pcif
        ${DAP_PCIF_SRC}
        proc_src/pcif/pcif_dbfile.c)
# 라이브러리 링크 추가(-l)
TARGET_LINK_LIBRARIES( dap_pcif
        com db ke secure ipc json sock pthread crypto m ssl jansson mysqlclient_r
)
##################################################



##################################################
# Target : DAP_PRMON
file(GLOB DAP_PRMON_SRC
        proc_src/prmon/*.c
)
add_executable(dap_prmon
        ${DAP_PRMON_SRC}
)
# 라이브러리 링크 추가(-l)
TARGET_LINK_LIBRARIES( dap_prmon
        com db ke secure ipc pthread crypto m mysqlclient_r
)
##################################################


##################################################
# Target : DAP_PROXY
file(GLOB DAP_PROXY_SRC
        proc_src/proxy/*.c
)
add_executable(dap_proxy
        ${DAP_PROXY_SRC}
)
# 라이브러리 링크 추가(-l)
TARGET_LINK_LIBRARIES( dap_proxy
        com ke ipc db secure json sock pthread crypto m ssl mysqlclient_r
)
##################################################


##################################################
# Target : DAP_REPORT
file(GLOB DAP_REPORT_SRC
        proc_src/report/*.c
)
add_executable(dap_report
        ${DAP_REPORT_SRC}
        proc_src/report/report_history_select.c)
# 라이브러리 링크 추가(-l)
TARGET_LINK_LIBRARIES( dap_report
        com db ke secure ipc pthread crypto m mysqlclient_r
)
TARGET_COMPILE_OPTIONS( dap_report
        PUBLIC
        -Wno-format
)
##################################################



##################################################
# Target : DAP_SCHD
file(GLOB DAP_SCHD_SRC
        proc_src/schd/schd_main.c
        proc_src/schd/schd_db.c
)
add_executable(dap_schd
        ${DAP_SCHD_SRC}
)
# 라이브러리 링크 추가(-l)
TARGET_LINK_LIBRARIES( dap_schd
        com db ke secure ipc pthread crypto m mysqlclient_r
)
##################################################


##################################################
# Target : DAP_DIR
file(GLOB DAP_DIR_SRC
        proc_src/schd/schd_dir_main.c
        proc_src/schd/schd_dir_ibk.c
        proc_src/schd/schd_dir_db.c
)
add_executable(dap_dir
        ${DAP_DIR_SRC}
)
# 라이브러리 링크 추가(-l)
TARGET_LINK_LIBRARIES( dap_dir
        com db ke secure ipc pthread crypto m mysqlclient_r
)
##################################################


##################################################
# Target : DAP_SYSMAN
file(GLOB DAP_SYSMAN_SRC
        proc_src/sysman/*.c
)
add_executable(dap_sysman
        ${DAP_SYSMAN_SRC}
)
# 라이브러리 링크 추가(-l)
TARGET_LINK_LIBRARIES( dap_sysman
        com db ke secure ipc pthread crypto m mysqlclient_r
)
##################################################


##################################################
# Target : DAP_TAIL
file(GLOB DAP_TAIL_SRC
        proc_src/vwlog/tail_main.c
)
add_executable(dap_tail
        ${DAP_TAIL_SRC}
)
# 라이브러리 링크 추가(-l)
TARGET_LINK_LIBRARIES( dap_tail
        com ipc db ke secure pthread crypto ssl jansson sock m mysqlclient_r
)
##################################################


##################################################
# Target : DAP_VWLOG
file(GLOB DAP_VWLOG_SRC
        proc_src/vwlog/vwlog_main.c
)
add_executable(dap_vwlog
        ${DAP_VWLOG_SRC}
)
# 라이브러리 링크 추가(-l)
TARGET_LINK_LIBRARIES( dap_vwlog
        com ipc db ke secure pthread crypto ssl jansson sock m mysqlclient_r
)
##################################################


##################################################
# Target : DAP_UTIL_CONN
file(GLOB DAP_UTIL_CONN_SRC
        proc_src/util/util_conn_main.c
        proc_src/util/util_connect.c
)
add_executable(dap_connect
        ${DAP_UTIL_CONN_SRC}
)
# 라이브러리 링크 추가(-l)
TARGET_LINK_LIBRARIES( dap_connect
        com db ke secure ipc json sock pthread crypto m ssl jansson mysqlclient_r
)
##################################################



##################################################
# Target : DAP_UTIL_STRESS
file(GLOB DAP_UTIL_STRESS_SRC
        proc_src/util/util_stress_main.c
)
add_executable(dap_stress
        ${DAP_UTIL_STRESS_SRC}
)
# 라이브러리 링크 추가(-l)
TARGET_LINK_LIBRARIES( dap_stress
        com db ke secure ipc json sock pthread crypto m ssl jansson mysqlclient_r
)
##################################################

##################################################
# Target : DAP_UTIL_STRESS
file(GLOB SYSLOG_MON_SRC
        proc_src/util/syslog_main.c
)
add_executable(syslog_mon
        ${SYSLOG_MON_SRC}
)
# 라이브러리 링크 추가(-l)
TARGET_LINK_LIBRARIES( syslog_mon
        com db ke secure ipc json sock pthread crypto m ssl
        )
##################################################


##################################################
# Target : DAP_LISTEN_TEST
file(GLOB DAP_LISTEN_TEST
        proc_src/util/listen_test.c
        )
add_executable(listen_test
        ${DAP_LISTEN_TEST}
        )
# 라이브러리 링크 추가(-l)
TARGET_LINK_LIBRARIES( listen_test
        com db ke secure ipc json sock pthread crypto m ssl
        )
##################################################