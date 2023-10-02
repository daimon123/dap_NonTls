//
// Created by KimByoungGook on 2020-10-07.
//

#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/file.h>
#include <errno.h>
#include <time.h>

#include "com/dap_com.h"
#include "db/dap_mysql.h"
#include "db/dap_checkdb.h"
#include "ipc/dap_Queue.h"
#include "secure/dap_secure.h"

#include "schd.h"


int fschd_SetLocalSyncUserLinkTb()
{
    int		rowCnt = 0;
    int		mergeCnt = 0;
    char    sqlBuf[1024 +1] = {0x00,};
    char    cpDate[20 +1] = {0x00,};

    fcom_time2str(time(NULL), cpDate, "YYYYMMDD\0");

    WRITE_INFO(CATEGORY_DEBUG,"start " );

    /* TEMP 테이블 생성 */
    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    sprintf(sqlBuf, "CREATE OR REPLACE TABLE BF_USER_LINK_TB LIKE USER_LINK_TB ");
    fdb_SqlQuery2(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d)msg(%s)",
                       g_stMyCon->nErrCode,g_stMyCon->cpErrMsg );
        return -1;
    }

    /* MANAGER에서 추가한 USER_LINK정보 TEMP에 복사. */
    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    sprintf(sqlBuf, "INSERT INTO BF_USER_LINK_TB (select * from USER_LINK_TB where US_SQ IN ( select US_SQ from USER_TB where US_AUTO_SYNC = '0') )");
    fdb_SqlQuery2(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d)msg(%s)",
                       g_stMyCon->nErrCode,g_stMyCon->cpErrMsg );
        return -1;
    }

    /* SQ SELECT 인사그룹파일에 있는 유저만 Link 된다. */
    memset(sqlBuf, 0x00, sizeof(sqlBuf));
//    strcpy(sqlBuf, 	"select UG_SQ, US_SQ "
//                      "from USER_TB A, "
//                      "("
//                      "	select y.UG_SQ as UG_SQ, x.EMN as EMN "
//                      "	from SYNC_USER_TB x "
//                      "	left join USER_GROUP_TB y on "
//                      "	CONVERT(x.BE_TEAM_CD, UNSIGNED INTEGER) = CONVERT(y.UG_CODE, UNSIGNED INTEGER) "
//                      ") B "
//                      "where A.US_SNO = B.EMN");
    strcpy(sqlBuf, 	"select UG_SQ, US_SQ "
                      "from USER_TB A, "
                      "("
                      "	select y.UG_SQ as UG_SQ, x.EMN as EMN "
                      "	from SYNC_USER_TB x "
                      "	left join USER_GROUP_TB y on "
                      "	x.BE_TEAM_CD = y.UG_CODE"
                      ") B "
                      "where A.US_SNO = B.EMN");

    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d)msg(%s)",
                       g_stMyCon->nErrCode,g_stMyCon->cpErrMsg );
        return -1;
    }

    WRITE_INFO(CATEGORY_DEBUG,"rowCnt(%d) ",rowCnt );

    if(rowCnt > 0)
    {
        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            if (g_stMyCon->row[0] == NULL) continue;
            if (g_stMyCon->row[1] == NULL) continue;

            memset(sqlBuf, 0x00, sizeof(sqlBuf));
            sprintf(sqlBuf, "insert into BF_USER_LINK_TB "
                            "("
                            "UG_SQ,"
                            "US_SQ"
                            ") values ("
                            "%llu,"
                            "%llu"
                            ") "
                            "on duplicate key update "
                            "UG_SQ=%llu,"
                            "US_SQ=%llu",
                    (g_stMyCon->row[0] != NULL) ? atoll(g_stMyCon->row[0]):0,
                    (g_stMyCon->row[1] != NULL) ? atoll(g_stMyCon->row[1]):0,
                    (g_stMyCon->row[0] != NULL) ? atoll(g_stMyCon->row[0]):0,
                    (g_stMyCon->row[1] != NULL) ? atoll(g_stMyCon->row[1]):0 );

            mergeCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
            if(g_stMyCon->nErrCode != 0)
            {
                WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d)msg(%s)",
                               g_stMyCon->nErrCode,g_stMyCon->cpErrMsg );
            }
        }
    }

    fdb_SqlFreeResult(g_stMyCon);

    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    sprintf(sqlBuf, " RENAME TABLE USER_LINK_TB TO DAP_SYNC_BACKUP.USER_LINK_TB_%s, BF_USER_LINK_TB TO USER_LINK_TB ",cpDate);

    fdb_SqlQuery2(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d)msg(%s)",
                       g_stMyCon->nErrCode,g_stMyCon->cpErrMsg );
    }

    WRITE_DEBUG(CATEGORY_DB, "Proc row(%d)merge(%d)", rowCnt,mergeCnt);

    return mergeCnt;
}

int fschd_SetLocalSyncUserTb()
{
    int		rowCnt = 0;
    int		udtCnt = 0;
    int		mergeCnt = 0;
    char    cpDate[20 +1] = {0x00,};
    char    szFlagFilePath[256 +1] = {0x00,};
    char*	sqlBuf = NULL;

    fcom_time2str(time(NULL), cpDate, "YYYYMMDD\0");

    WRITE_DEBUG(CATEGORY_DEBUG,"Start " );

    if(fcom_malloc((void**)&sqlBuf, sizeof(char) * 5120) != 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
        return (-1);
    }

    memset(szFlagFilePath, 0x00, sizeof(szFlagFilePath));

    /* TEMP 테이블 생성 */
    sprintf	(sqlBuf, "CREATE OR REPLACE TABLE BF_USER_TB "
                        "("
                        "`US_SQ`           bigint(20) 		unsigned zerofill  NOT NULL AUTO_INCREMENT COMMENT 'USER_TB 고유번호', "
                        "`US_ID`           varchar(20)     CHARACTER SET utf8 COLLATE utf8_general_ci DEFAULT NULL COMMENT '아이디', "
                        "`US_PASSWD`       varchar(256)    CHARACTER SET utf8 COLLATE utf8_general_ci DEFAULT NULL COMMENT '패스워드', "
                        "`US_NAME`         varchar(32)     CHARACTER SET utf8 COLLATE utf8_general_ci DEFAULT NULL COMMENT '사용자명', "
                        "`US_SNO`          varchar(32)     CHARACTER SET utf8 COLLATE utf8_general_ci DEFAULT NULL COMMENT '사용자 식별번호(사번)', "
                        "`US_EMAIL`        varchar(128)    CHARACTER SET utf8 COLLATE utf8_general_ci DEFAULT NULL COMMENT '이메일 주소', "
                        "`US_PHONE`        varchar(64)     CHARACTER SET utf8 COLLATE utf8_general_ci DEFAULT NULL COMMENT '전화번호', "
                        "`US_CELL_PHONE`   varchar(64)     CHARACTER SET utf8 COLLATE utf8_general_ci DEFAULT NULL COMMENT '핸드폰 번호', "
                        "`US_IP`           varchar(128)    CHARACTER SET utf8 COLLATE utf8_general_ci DEFAULT NULL COMMENT '지정 IP, 세미콜론으로 분류하여 다중입력', "
                        "`US_STATE`        char(1)         CHARACTER SET utf8 COLLATE utf8_general_ci DEFAULT '1' COMMENT '사용자상태', "
                        "`US_DESC`         varchar(128)    CHARACTER SET utf8 COLLATE utf8_general_ci DEFAULT NULL COMMENT '사용자에 대한 추가 설명', "
                        "`US_DB_ROUTER`    tinyint(2)      DEFAULT 0 COMMENT 'DBIF라우터', "
                        "`US_LOG_LEVEL`    tinyint(2)      DEFAULT 0 COMMENT '사용자로그레벨', "
                        "`US_AUTO_SYNC`    char(1)         CHARACTER SET utf8 COLLATE utf8_general_ci DEFAULT '0' COMMENT '자동동기화유무', "
                        "`US_FLAG`         char(1)         CHARACTER SET utf8 COLLATE utf8_general_ci DEFAULT 'A' COMMENT '스케줄러 작업 상태', "
                        "`US_CREATE_TIME`  datetime        DEFAULT NULL COMMENT '생성일자', "
                        "`US_MODIFY_TIME`  datetime        DEFAULT NULL COMMENT '수정일자', "
                        "`USCOMP_FLAG`     varchar(2)      CHARACTER SET utf8 COLLATE utf8_general_ci DEFAULT '0' COMMENT '0:본사직원,1:외주직원',"
                        "`COMPANY_NAME`    varchar(22)     CHARACTER SET utf8 COLLATE utf8_general_ci DEFAULT ' ' COMMENT '회사명', "
                        "PRIMARY KEY (`US_SQ`),"
                        "CONSTRAINT `US_SNO` UNIQUE (`US_SNO`), "
                        "INDEX `USER_TB_IDX2` (`US_IP`), "
                        "INDEX `USER_TB_IDX1` (`US_NAME`) "
                        ") "
                        "ENGINE=Aria DEFAULT CHARSET=utf8 COLLATE=utf8_general_ci ROW_FORMAT=Page ");

    /* TEMP 테이블 CREATE */
    fdb_SqlQuery2(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d)msg(%s)",
                       g_stMyCon->nErrCode,g_stMyCon->cpErrMsg);
    }

    /* 기존 USER_TB에서 해당 SYNC테이블의 존재하는 사원번호 데이터만 TEMP테이블에 복사. */
    memset(sqlBuf, 0x00, sizeof(char) * 5120);
    sprintf	(sqlBuf, " INSERT INTO BF_USER_TB ( "
                        "SELECT A.* FROM USER_TB A, SYNC_USER_TB B "
                        "WHERE A.US_SNO = B.EMN "
                        "AND A.US_AUTO_SYNC = '1' "
                        ")");

    fdb_SqlQuery2(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d)msg(%s)",
                       g_stMyCon->nErrCode,g_stMyCon->cpErrMsg);
    }

    /* DAP Manager에서 Insert한 유저 TEMP 테이블에 복사 */
    memset(sqlBuf, 0x00, sizeof(char) * 5120);
    sprintf	(sqlBuf, " INSERT INTO BF_USER_TB ( SELECT * FROM USER_TB WHERE US_AUTO_SYNC ='0' )");
    fdb_SqlQuery2(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d)msg(%s)",
                       g_stMyCon->nErrCode,g_stMyCon->cpErrMsg);
    }

    /* SYNC Table Select */
    memset(sqlBuf, 0x00, sizeof(char) * 5120);
    strcpy	(sqlBuf,"select "
                      "EMN,"
                      "AEMP_NM,"
                      "EAD,"
                      "EMP_CPN,"
                      "BE_TEAM_CD, "
                      "USCOMP_FLAG, "
                      "COMPANY_NAME "
                      "from SYNC_USER_TB");

    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d)msg(%s)",
                       g_stMyCon->nErrCode,g_stMyCon->cpErrMsg);
    }

    WRITE_DEBUG(CATEGORY_DEBUG, "SYNC_USER_TB rowCnt(%d) ", rowCnt );

    if(rowCnt > 0)
    {
        WRITE_DEBUG(CATEGORY_DEBUG,"BF_USER_TB Try update state '0'");

        memset(sqlBuf, 0x00, sizeof(char) * 5120);
        snprintf(sqlBuf, 5120,"%s","update BF_USER_TB set US_STATE='0' where US_STATE='1' and US_AUTO_SYNC='1'");

        udtCnt = fdb_SqlQuery2(g_stMyCon, sqlBuf);
        if(g_stMyCon->nErrCode != 0)
        {
            WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d)msg(%s)",
                           g_stMyCon->nErrCode,g_stMyCon->cpErrMsg);
        }
        WRITE_DEBUG(CATEGORY_DEBUG, "Proc udtCnt(%d)", udtCnt);

        int		encMailSize = 0;
        int		encPhoneSize = 0;
        char 	encMail[1024 +1] = {0x00};
        char 	encPhone[1024 +1] = {0x00};
        char	currDate[19 +1] = {0x00};
        time_t	currTime = time((time_t) 0);

        memset	(currDate, 0x00, sizeof(currDate));
        fcom_time2str(currTime, currDate, "YYYY-MM-DD hh:mm:ss\0");

        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            memset(encMail, 0x00, sizeof(encMail));
            memset(encPhone, 0x00, sizeof(encPhone));
            if(g_stMyCon->row[2] != 0x00)
            {
                fsec_EncryptStr(g_stMyCon->row[2], strlen(g_stMyCon->row[2]), encMail, &encMailSize);
            }
            if(g_stMyCon->row[3] != 0x00)
            {
                fsec_EncryptStr(g_stMyCon->row[3], strlen(g_stMyCon->row[3]), encPhone, &encPhoneSize);
            }

            memset(sqlBuf, 0x00, sizeof(char) * 5120);
            sprintf	(sqlBuf, "INSERT INTO BF_USER_TB ("
                                "US_SNO,"
                                "US_NAME,"
                                "US_EMAIL,"
                                "US_CELL_PHONE,"
                                "US_AUTO_SYNC,"
                                "US_FLAG,"
                                "US_CREATE_TIME,"
                                "USCOMP_FLAG,"
                                "COMPANY_NAME"
                                ") values ("
                                "'%s',"
                                "'%s',"
                                "'%s',"
                                "'%s',"
                                "'1',"
                                "'A',"
                                "'%s',"
                                "'%s',"
                                "'%s'"
                                ") "
                                "on duplicate key update "
                                "US_SNO='%s',"
                                "US_NAME='%s',"
                                "US_EMAIL='%s',"
                                "US_CELL_PHONE='%s',"
                                "US_STATE='1',"
                                "US_AUTO_SYNC='1',"
                                "US_FLAG='M',"
                                "US_MODIFY_TIME='%s'",
                        (g_stMyCon->row[0] != NULL) ? g_stMyCon->row[0]:"",	// EMN
                        (g_stMyCon->row[1] != NULL) ? g_stMyCon->row[1]:"",	// AEMP_NM
                        encMail,		// EAD
                        encPhone,		// EMP_CPN
                        currDate,       // MOD TIME
                        ((char*)g_stMyCon->row[5] != NULL) ? g_stMyCon->row[5]:"", // USCOMP_FLAG
                        ((char*)g_stMyCon->row[6] != NULL) ? g_stMyCon->row[6]:"", // COMPANY_NAME
                        (g_stMyCon->row[0] != NULL) ? g_stMyCon->row[0]:"",	// EMN
                        (g_stMyCon->row[1] != NULL) ? g_stMyCon->row[1]:"",	// AEMP_NM
                        encMail,		// EAD
                        encPhone,		// EMP_CPN
                        currDate                        );


            //LogDRet(5, "|%-8s|- sqlBuf(%s)(%d)\n", STR_DEBUG,sqlBuf,strlen(sqlBuf));
            mergeCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
            if(g_stMyCon->nErrCode != 0)
            {
                WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d)msg(%s)",
                               g_stMyCon->nErrCode,g_stMyCon->cpErrMsg);
                if(sqlBuf != NULL)
                    free(sqlBuf);
                fdb_SqlFreeResult(g_stMyCon);

                return -1;
            }

        } // while
    }

    fdb_SqlFreeResult(g_stMyCon);

    WRITE_INFO(CATEGORY_DB, "SYNC_USER_TB Insert Success " );

    /* TEMP테이블 RENAME 및 기존 테이블 백업 */
    memset(sqlBuf, 0x00, sizeof(char) * 5120);
    sprintf(sqlBuf, "RENAME TABLE USER_TB TO DAP_SYNC_BACKUP.USER_TB_%s, BF_USER_TB TO USER_TB",cpDate);

    fdb_SqlQuery2(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d)msg(%s)",
                       g_stMyCon->nErrCode,g_stMyCon->cpErrMsg);
    }
    if(sqlBuf != NULL)
        free(sqlBuf);

    WRITE_INFO(CATEGORY_DEBUG,"Proc row(%d)merge(%d)",rowCnt,mergeCnt );

    /* ------------------------------------------------------------------- */
    return mergeCnt;
}

int fschd_SetLocalSyncGroupLinkRenameTb()
{
    int		rowCnt = 0;
    int		mergeCnt = 0;
    unsigned long long pSq = 0;
    unsigned long long cSq = 0;
    char    sqlBuf[512 +1] = {0x00};
    char    cpDate[20 +1] = {0x00};

    fcom_time2str(time(NULL), cpDate, "YYYYMMDD\0");


    /* TEMP 테이블 생성 */
    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    sprintf(sqlBuf, "CREATE OR REPLACE TABLE BF_USER_GROUP_LINK_TB LIKE USER_GROUP_LINK_TB ");
    fdb_SqlQuery2(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) errmsg(%s)",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);
        return -1;
    }

    /* MANAGER에서 추가한 USER_LINK정보 TEMP에 복사. */
    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    sprintf(sqlBuf, "INSERT INTO BF_USER_GROUP_LINK_TB (select * from USER_GROUP_LINK_TB where UG_SQ_P IN ("
                    " select UG_SQ from USER_GROUP_TB where UG_AUTO_SYNC = '0') OR UG_SQ_C IN (select UG_SQ from USER_GROUP_TB where UG_AUTO_SYNC = '0' ))");

    fdb_SqlQuery2(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) errmsg(%s)",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);
        return -1;
    }

    /* SYNC에서 가져온 GROUP  SELECT  */
    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    strcpy	(sqlBuf,"select "
                      "B.UG_SQ as UG_SQ_P, B.UPDEPT_CODE, A.UG_SQ as UG_SQ_C, B.DEPT_CODE "
                      "from USER_GROUP_TB A, "
                      "("
                      "	select "
                      "	y.UG_SQ as UG_SQ, x.DEPT_CODE as DEPT_CODE, x.UPDEPT_CODE as UPDEPT_CODE "
                      "	from SYNC_GROUP_TB x "
                      "	left join USER_GROUP_TB y on x.UPDEPT_CODE=y.UG_CODE "
                      ") B "
                      "where A.UG_CODE = B.DEPT_CODE");

    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) errmsg(%s)",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);
        return -1;
    }

    WRITE_DEBUG(CATEGORY_DB,"rowCnt(%d) ",rowCnt );

    if(rowCnt > 0)
    {
        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            // higher-dept
            if(g_stMyCon->row[0] != NULL)
                pSq = atol(g_stMyCon->row[0]);
            else
            {
                // ibk기업은행에만 적용됨, '9999'는 ibk기업은행 최상위코드
                if (!strcmp(g_stMyCon->row[1], "9999")) pSq = 0;
                else
                {
                    WRITE_CRITICAL(CATEGORY_DB,"Not found dept(%s) info",g_stMyCon->row[1] );
                    continue;
                }
            }
            // sub-dept
            if(g_stMyCon->row[2] != NULL) 	cSq = atol(g_stMyCon->row[2]);
            else 						    cSq = 0;

            memset(sqlBuf, 0x00, sizeof(sqlBuf));
            sprintf	(sqlBuf,"insert into BF_USER_GROUP_LINK_TB "
                               "("
                               "UG_SQ_P,"
                               "UG_SQ_C,"
                               "UG_FLAG"
                               ") values( "
                               "%llu,"
                               "%llu,"
                               "'A'"
                               ") "
                               "on duplicate key update "
                               "UG_SQ_P=%llu,"
                               "UG_SQ_C=%llu,"
                               "UG_FLAG='M'",
                        pSq,
                        cSq,
                        pSq,
                        cSq);

            //LogDRet(5, "|%-8s|- sqlBuf(%s)(%d)\n", STR_DEBUG,sqlBuf,strlen(sqlBuf));
            mergeCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
            if(g_stMyCon->nErrCode != 0)
            {
                WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) errmsg(%s)",
                               g_stMyCon->nErrCode,
                               g_stMyCon->cpErrMsg);
            }
        }
    }

    fdb_SqlFreeResult(g_stMyCon);
    /* TEMP 테이블 RENAME */
    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    sprintf(sqlBuf, " RENAME TABLE USER_GROUP_LINK_TB TO DAP_SYNC_BACKUP.USER_GROUP_LINK_TB_%s, BF_USER_GROUP_LINK_TB TO USER_GROUP_LINK_TB ",cpDate);

    fdb_SqlQuery2(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) errmsg(%s)",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);
    }

    WRITE_DEBUG(CATEGORY_DB,"Proc row(%d)merge(%d) ",rowCnt,mergeCnt );

    return mergeCnt;
}

int fschd_SetLocalSyncRenameGroupTb()
{
    int		rowCnt = 0;
    int		mergeCnt = 0;
    char	strDeptName[128 +1] = {0x00,};
    char	strDeptDesc[128 +1] = {0x00,};
    char	sqlBuf[2048 +1] = {0x00,};
    char    timebuf[20 +1] = {0x00,};
    int     Loop = 0;
    int*    ptrUserDeptCode = NULL;

    ptrUserDeptCode = NULL;

    memset(sqlBuf, 0x00, sizeof(sqlBuf));

    fcom_time2str(time(NULL), timebuf, "YYYYMMDD\0");

    /* TEMP 테이블 생성 */
    sprintf(sqlBuf, "CREATE OR REPLACE TABLE BF_USER_GROUP_TB "
                    "("
                    "    `UG_SQ`           bigint(20)       unsigned zerofill  NOT NULL AUTO_INCREMENT COMMENT '고유번호',"
                    "    `UG_CODE`         varchar(10)      CHARACTER SET utf8 COLLATE utf8_general_ci DEFAULT NULL COMMENT '부서코드',"
                    "    `UG_NAME`         varchar(100)     CHARACTER SET utf8 COLLATE utf8_general_ci NOT NULL COMMENT '부서명',"
                    "    `UG_DESC`         varchar(128)     CHARACTER SET utf8 COLLATE utf8_general_ci DEFAULT '' COMMENT '부서에 대한 추가 설명',"
                    "    `UG_MANAGER`      varchar(32)      CHARACTER SET utf8 COLLATE utf8_general_ci DEFAULT '' COMMENT '부서장',"
                    "    `UG_DATE`         varchar(10)      CHARACTER SET utf8 COLLATE utf8_general_ci DEFAULT '' COMMENT '유효기간',"
                    "    `UG_USE`          char(1)          CHARACTER SET utf8 COLLATE utf8_general_ci DEFAULT 'Y' COMMENT '사용유무',"
                    "    `UG_CREATE_TIME`  datetime         DEFAULT NULL COMMENT '생성일자',"
                    "    `UG_MODIFY_TIME`  datetime         DEFAULT NULL COMMENT '수정일자',"
                    "    `UG_FLAG`         char(1)          CHARACTER SET utf8 COLLATE utf8_general_ci DEFAULT 'A' COMMENT '스케줄러 작업 상태',"
                    "    `UG_AUTO_SYNC`    char(1)           CHARACTER SET utf8 COLLATE utf8_general_ci DEFAULT '0' COMMENT '자동동기화유무', "
                    "    PRIMARY KEY (`UG_SQ`),"
                    "    CONSTRAINT `UG_CODE` UNIQUE (`UG_CODE`),"
                    "    INDEX `USER_GROUP_TB_IDX1` (`UG_NAME`) "
                    ")"
                    "ENGINE=Aria DEFAULT CHARSET=utf8 COLLATE=utf8_general_ci ROW_FORMAT=Page ");


    fdb_SqlQuery2(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) errmsg(%s) ",g_stMyCon->nErrCode,g_stMyCon->cpErrMsg );
        return -1;
    }

    /* 테이블 부서번호(DEPT_CODE) 데이터 SELECT .*/
    memset(sqlBuf, 0x00, sizeof(sqlBuf));
    sprintf(sqlBuf, " SELECT DEPT_CODE FROM SYNC_GROUP_TB ORDER BY DEPT_CODE ASC ");
    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) errmsg(%s) ",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);
        return -1;
    }

    WRITE_DEBUG(CATEGORY_DB,"rowCnt (%d) ",rowCnt );

    if(rowCnt > 0)
    {
        Loop = 0;

        if(fcom_malloc((void**)&ptrUserDeptCode, sizeof(int) * rowCnt) != 0)
        {
            WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
            return (-1);
        }

        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            ptrUserDeptCode[Loop++] = atoi(g_stMyCon->row[0]);
        }
    }
    fdb_SqlFreeResult(g_stMyCon);
    /* ------------------------------------------------------------------- */


    /* 기존 USER_GROUP_TB에서 해당 SYNC테이블의 존재하는 부서번호 데이터만 TEMP테이블에 복사. */
    for(Loop = 0; Loop < rowCnt; Loop++)
    {
        sprintf(sqlBuf, "INSERT INTO BF_USER_GROUP_TB ( SELECT * FROM USER_GROUP_TB WHERE UG_CODE = '%d' AND UG_AUTO_SYNC = '1') ",
                ptrUserDeptCode[Loop]);

        fdb_SqlQuery2(g_stMyCon, sqlBuf);
        if(g_stMyCon->nErrCode != 0)
        {
            WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) msg(%s) ",
                           g_stMyCon->nErrCode,
                           g_stMyCon->cpErrMsg);
        }

    }

    /* MANAGER에서 추가한 GROUP TEMP테이블에 복사 */
    sprintf(sqlBuf, "INSERT INTO BF_USER_GROUP_TB ( SELECT * FROM USER_GROUP_TB WHERE UG_AUTO_SYNC ='0' ) ");

    fdb_SqlQuery2(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) errmsg(%s) ",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);
    }

    /* SELECT SYNC 테이블(인사파일 테이블) */
    memset	(sqlBuf, 0x00, sizeof(sqlBuf));
    sprintf	(sqlBuf,"SELECT "
                       "DEPT_CODE,"
                       "DEPT_NAME,"
                       "DEPT_DESC,"
                       "DEPT_DATE,"
                       "UPDEPT_CODE "
                       "from SYNC_GROUP_TB");

    //LogDRet(5, "|%-8s|- sqlBuf(%s)(%d)\n", STR_DEBUG,sqlBuf,strlen(sqlBuf));
    rowCnt = fdb_SqlQuery(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) errmsg(%s) ",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);

        free(ptrUserDeptCode);
        return -1;
    }

    WRITE_DEBUG(CATEGORY_DB,"rowCnt(%d) ",rowCnt );

    if(rowCnt > 0)
    {
        char	currDate[19+1];
        time_t	currTime = time((time_t) 0);

        memset	(currDate, 0x00, sizeof(currDate));
        fcom_time2str(currTime, currDate, "YYYY-MM-DD hh:mm:ss\0");

        while(fdb_SqlFetchRow(g_stMyCon) == 0)
        {
            memset(strDeptName, 0x00, sizeof(strDeptName));
            memset(strDeptDesc, 0x00, sizeof(strDeptDesc));
            if (g_stMyCon->row[1] != NULL)
            {
                strcpy(strDeptName, g_stMyCon->row[1]);
                fcom_Trim(strDeptName, strlen(strDeptName));
            }
            if (g_stMyCon->row[2] != NULL)
            {
                strcpy(strDeptDesc, g_stMyCon->row[2]);
                fcom_Trim(strDeptDesc, strlen(strDeptDesc));
            }
            memset	(sqlBuf, 0x00, sizeof(sqlBuf));
            sprintf	(sqlBuf, "INSERT INTO BF_USER_GROUP_TB ("
                                "UG_CODE,"
                                "UG_NAME,"
                                "UG_DESC,"
                                "UG_DATE,"
                                "UG_MANAGER,"
                                "UG_FLAG,"
                                "UG_CREATE_TIME,"
                                "UG_AUTO_SYNC"
                                ") values ("
                                "'%s',"
                                "'%s',"
                                "'%s',"
                                "'%s',"
                                "'',"
                                "'A',"
                                "'%s',"
                                "'1'"
                                ") "
                                "on duplicate key update "
                                "UG_CODE='%s',"
                                "UG_NAME='%s',"
                                "UG_DESC='%s',"
                                "UG_DATE='%s',"
                                "UG_MANAGER='',"
                                "UG_FLAG='M',"
                                "UG_MODIFY_TIME='%s'",
                        g_stMyCon->row[0],	// DEPT_CODE
                        strDeptName,		// DEPT_NAME
                        strDeptDesc,		// DEPT_DESC
                        g_stMyCon->row[3],	// DEPT_DATE
                        currDate,
                        g_stMyCon->row[0],	// DEPT_CODE
                        strDeptName,		// DEPT_NAME
                        strDeptDesc,		// DEPT_DESC
                        g_stMyCon->row[3],	// DEPT_DATE
                        currDate
            );

            mergeCnt += fdb_SqlQuery2(g_stMyCon, sqlBuf);
            if(g_stMyCon->nErrCode != 0)
            {
                WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) errmsg(%s)",
                               g_stMyCon->nErrCode,
                               g_stMyCon->cpErrMsg);
                fdb_SqlFreeResult(g_stMyCon);
                free(ptrUserDeptCode);
                return -1;
            }
        }
    }

    fdb_SqlFreeResult(g_stMyCon);

    /* 기존 테이블 백업 */
    sprintf(sqlBuf, "RENAME TABLE USER_GROUP_TB TO DAP_SYNC_BACKUP.USER_GROUP_TB_%s, BF_USER_GROUP_TB TO USER_GROUP_TB ", timebuf);

    fdb_SqlQuery2(g_stMyCon, sqlBuf);
    if(g_stMyCon->nErrCode != 0)
    {
        WRITE_CRITICAL(CATEGORY_DB,"Fail in query, errcode(%d) errmsg(%s) ",
                       g_stMyCon->nErrCode,
                       g_stMyCon->cpErrMsg);
    }

    WRITE_DEBUG(CATEGORY_DB,"Proc row(%d)merge(%d) ",
                rowCnt,
                mergeCnt);

    free(ptrUserDeptCode);

    return mergeCnt;
}


