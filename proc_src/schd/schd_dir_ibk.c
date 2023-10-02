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

struct _stGroup
{
    char DeptCode[20+1];   /* 부서코드      */
    char DepteName[300+1]; /* 부서명       */
    char DeptDesc[400+1];  /* 부서에대한설명 */
    char UpDeptCode[20+1]; /* 상위부서코드   */
    char DeptDate[20+1]; /* 부서일자      */
};

struct _stUser
{
    char Userid[30 + 1]; /* 직원번호 */
    char Name[40 + 1]; /* 직원명 */
    char BranchCode[10 + 1]; /* 관할부점코드 */
    char BranchName[160 + 1]; /* 소속명 */
    char JoinDate[30 + 1];  /* 입행일자 */
    char WorkplaceCode[10 + 1]; /* 실근무지 부점코드 */
    char WorkplaceName[160 + 1]; /* 실근무지명 */
    char BelongTeamCode[10 + 1]; /* 소속팀 코드*/
    char BelongTeamName[160 + 1]; /* 소속팀 이름 */
    char JrsdHqcd[10 + 1];   /* 관할본부코드 */
    char BelongTeamName2[160 + 1]; /* 소속팀 이름 */
    char MndtCd[10 + 1]; /* 보임코드 */
    char JbclCd[10 + 1]; /* 직급코드 */
    char PositionCd[10 + 1];/* 직위코드*/
    char PositionName[160 + 1];  /* 약칭 직위명 */
    char TitleCode[10 + 1];/* 직책코드 */
    char EtbnDcd[10 + 1];/* 입행구분코드 */
    char Telephone[30 + 1]; /* 휴대폰번호 */
    char Email[64 + 1]; /* 이메일 */
    char BirthDay[30 + 1]; /* 생년월일 */
    char EtbnYnd[30 + 1]; /* 입행년월일*/
    char RetireDate[30 + 1]; /* 퇴직년월일 */
    char Seumu[10 + 1]; /* 서무구분*/
    char Chul[10 + 1]; /* 조직속성코드 */
};

struct _stOutsourcingUser
{
    char Userid[10+1]; /* 직원번호 */
    char EnableFlag[1+1];
    char Name[40+1]; /* 직원명 */
    char BranchCode[10+1]; /* 관할부점코드 */
    char BranchName[160+1]; /* 소속명 */
    char JoinDate[30+1]; /* 입행일자 */
    char WorkplaceCode[10+1]; /* 실근무지 부점코드 */
    char WorkplaceName[160+1]; /* 실근무지명 */
    char BelongTeamCode[10+1]; /* 소속팀 코드*/
    char BelongTeamName[160+1]; /* 소속팀 이름 */
    char MndtCd[10+1]; /* 보임코드 */
    char JbclCd[10+1]; /* 직급코드 */
    char PositionCd[10+1];/* 직위코드*/
    char PositionName[160+1]; /* 약칭 직위명 */
    char TitleCode[10+1]; /* 직책코드 */
    char Telephone[30+1]; /* 휴대폰번호 */
    char Email[64+1];/* 이메일 */
    char BirthDay[30+1];/* 생년월일 */
    char RetireDate[30+1];/* 퇴직년월일 */
    char Seumu[10+1];/* 서무구분*/
    char Chul[10+1];/* 조직속성코드 */
    char CompanyName[64+1];/* 회사명 */
};



static int fstParseUser(struct _stUser* pstUser, char* Filepath, char* szSyncType);
static int fstParseGroup(struct _stGroup* pstGroup, char* Filepath, char* szSyncType);
int fstParseExUser(struct _stOutsourcingUser* pstExUser, char* Filepath, char* szSyncType);

int fschd_ParseSyncFile(int nFlag, char* filepath, char* SyncType)
{
    int FileRowCnt = 0;
    int nRet = 0;

    struct _stGroup* pstGroup= NULL;
    struct _stUser*  pstUser = NULL;
    struct _stOutsourcingUser*  pstOutsourcingUser= NULL;

    FileRowCnt = fcom_GetFileRows(filepath);
    if(FileRowCnt > 0)
    {
        switch(nFlag)
        {
            case 1:
                if(fcom_malloc((void**)&pstGroup, sizeof(struct _stGroup) * FileRowCnt) != 0)
                {
                    WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
                    return (-1);
                }
                nRet = fstParseGroup(pstGroup, filepath, SyncType);
                if(nRet != 0)
                {
                    WRITE_CRITICAL(CATEGORY_DEBUG,"Parse Group Fail " );
                }
                break;
            case 2:
                if(fcom_malloc((void**)&pstUser, sizeof(struct _stUser) * FileRowCnt) != 0)
                {
                    WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
                    return (-1);
                }
                nRet = fstParseUser(pstUser, filepath, SyncType);
                if(nRet != 0)
                {
                    WRITE_CRITICAL(CATEGORY_DEBUG,"Parse User Fail " );
                }
                break;
            case 3:
                if(fcom_malloc((void**)&pstOutsourcingUser, sizeof(struct _stOutsourcingUser) * FileRowCnt) != 0)
                {
                    WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
                    return (-1);
                }

                nRet = fstParseExUser(pstOutsourcingUser, filepath, SyncType);
                if(nRet != 0)
                {
                    WRITE_CRITICAL(CATEGORY_DEBUG,"Parse ExUser Fail " );
                }
                break;
            default:
            WRITE_CRITICAL(CATEGORY_DEBUG,"Unknown Type %d ",nFlag );
                break;
        }
    }
    else
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in %s Get File Rows ",filepath );
        return (-1);
    }

    return nRet;

}

static int fstParseGroup(struct _stGroup* pstGroup, char* Filepath, char* szSyncType)
{
    FILE* fp = NULL;
    FILE* convFp = NULL;
    char* cPtr = NULL;

    char szTemp[1024 + 1] = {0x00,};
    char szReplaceBuffer[1024 +1] = {0x00,};
    char szQuery[1024 +1] = {0x00,};
    char szConvFilePath[256 +1] = {0x00,};

    int TotTokenCnt = 0;
    int tokCnt = 0;
    int UserIdx = 0;
    int nLoop = 0;


    memset(szTemp, 0x00, sizeof(szTemp));
    memset(szReplaceBuffer, 0x00, sizeof(szReplaceBuffer));
    memset(szQuery, 0x00, sizeof(szQuery));
    memset(szConvFilePath, 0x00, sizeof(szConvFilePath));


    fp = fopen(Filepath, "r");
    while (fgets(szTemp, sizeof(szTemp), fp) != NULL)
    {
        fcom_ReplaceAll(szTemp, "|", " |", szReplaceBuffer);
        TotTokenCnt = fcom_TokenCnt(szReplaceBuffer,"|");
        cPtr = strtok(szReplaceBuffer, "|");
        if(cPtr != NULL)
            fcom_Rtrim(cPtr,strlen(cPtr));

        // �μ��ڵ�
        snprintf(pstGroup[UserIdx].DeptCode, sizeof(pstGroup[UserIdx].DeptCode), "%d", atoi(cPtr));
        for(tokCnt = 0; tokCnt < TotTokenCnt; tokCnt ++)
        {
            cPtr = strtok(NULL, "|");
            if(cPtr != NULL)
                fcom_Rtrim(cPtr,strlen(cPtr));

            switch(tokCnt)
            {
                case 0: // 부서명
                {
                    snprintf(pstGroup[UserIdx].DepteName, sizeof(pstGroup[UserIdx].DepteName), "%s", cPtr);
                    if(fcom_EucKrTOUtf8( pstGroup[UserIdx].DepteName, sizeof(pstGroup[UserIdx].DepteName)) != 0)
                    {
                        snprintf(pstGroup[UserIdx].DepteName,sizeof(pstGroup[UserIdx].DepteName),"%s",cPtr);
                    }

                    break;
                }
                case 1: // 부서설명
                {
                    snprintf(pstGroup[UserIdx].DeptDesc, sizeof(pstGroup[UserIdx].DeptDesc), "%s", cPtr);
                    if(fcom_EucKrTOUtf8( pstGroup[UserIdx].DeptDesc, sizeof(pstGroup[UserIdx].DeptDesc)) != 0)
                    {
                        snprintf(pstGroup[UserIdx].DeptDesc,sizeof(pstGroup[UserIdx].DeptDesc),"%s",cPtr);
                    }
                    break;
                }
                case 2: // 상위부서코드
                {
                    snprintf(pstGroup[UserIdx].UpDeptCode, sizeof(pstGroup[UserIdx].UpDeptCode), "%d", atoi(cPtr));
                    break;
                }
                case 3: // 부서일자
                {
                    snprintf(pstGroup[UserIdx].DeptDate, sizeof(pstGroup[UserIdx].DeptDate), "%s", cPtr);
                    break;
                }
            }
        }
        memset(szTemp, 0x00, sizeof(szTemp));
        UserIdx++;
    }

    fclose(fp);

    /* Convert 파일명_IBK */
    memset(szTemp, 0x00, sizeof(szTemp));
    snprintf(szTemp, sizeof(szTemp), "_%s",szSyncType);
    strcat(Filepath,szTemp);
    convFp = fopen(Filepath,"w");

    memset(szTemp, 0x00, sizeof(szTemp));
    snprintf(szTemp, sizeof(szTemp), "# %s | %s | %s | %s | %s\n",
             "DEPT_CODE",
             "DEPT_NAME",
             "DEPT_DESC",
             "UPDEPT_CODE",
             "DEPT_DATE");

    fprintf(convFp,"%s",szTemp);

    /* Convert File 생성 */
    for(nLoop = 0; nLoop < UserIdx; nLoop++)
    {
        memset(szTemp, 0x00, sizeof(szTemp));
        snprintf(szTemp, sizeof(szTemp), "%s | %s | %s | %s | %s\n",
                 pstGroup[nLoop].DeptCode,
                 pstGroup[nLoop].DepteName   ,
                 pstGroup[nLoop].DeptDesc    ,
                 pstGroup[nLoop].UpDeptCode  ,
                 pstGroup[nLoop].DeptDate    );
        fprintf(convFp,"%s",szTemp);

    }
    fclose(convFp);

    for(nLoop = 0; nLoop < UserIdx; nLoop++)
    {
        memset(szQuery, 0x00, sizeof(szQuery));

        sprintf(szQuery, " INSERT INTO SYNC_GROUP_TB( "
                         " DEPT_CODE, "
                         " DEPT_NAME, "
                         " DEPT_DESC, "
                         " UPDEPT_CODE, "
                         " DEPT_DATE )"
                         "VALUES ( "
                         " '%s',"
                         " '%s',"
                         " '%s',"
                         " '%s',"
                         " '%s' )",
                pstGroup[nLoop].DeptCode    ,
                pstGroup[nLoop].DepteName   ,
                pstGroup[nLoop].DeptDesc    ,
                pstGroup[nLoop].UpDeptCode  ,
                pstGroup[nLoop].DeptDate    );


        fdb_SqlQuery2(g_stMyCon, szQuery);
        if(g_stMyCon->nErrCode != 0)
        {
            WRITE_CRITICAL(CATEGORY_DB,"Fail in query(%s), errmsg(%s)",szQuery,g_stMyCon->cpErrMsg );
        }

    }
    if(pstGroup != NULL)
        free(pstGroup);

    WRITE_INFO(CATEGORY_DEBUG,"insert SYNC_GROUP_TB Success (%d)",UserIdx);

    return 0;

}

static int fstParseUser(struct _stUser* pstUser, char* Filepath, char* szSyncType)
{
    int TotTokenCnt = 0;
    int tokCnt = 0;
    int UserIdx = 0;
    int nLoop = 0;

    FILE* fp = NULL;
    FILE* convFp = NULL;
    char* cPtr = NULL;

    char szTemp[1024 + 1] = {0x00,};
    char szReplaceBuffer[1024 +1] = {0x00,};
    char szQuery[1024 +1] = {0x00,};


    memset(szTemp           , 0x00, sizeof(szTemp));
    memset(szReplaceBuffer  , 0x00, sizeof(szReplaceBuffer));
    memset(szQuery          , 0x00, sizeof(szQuery));

    fp = fopen(Filepath, "r");
    while (fgets(szTemp, sizeof(szTemp), fp) != NULL)
    {
        TotTokenCnt = fcom_TokenCnt(szTemp,"|");
        fcom_ReplaceAll(szTemp, "|", " |", szReplaceBuffer);
        cPtr = strtok(szReplaceBuffer, "|");
        if(cPtr != NULL)
            fcom_Rtrim(cPtr,strlen(cPtr));

        snprintf(pstUser[UserIdx].Userid, sizeof(pstUser[UserIdx].Userid), "%s", cPtr);
        for(tokCnt = 0; tokCnt < TotTokenCnt; tokCnt ++)
        {
            cPtr = strtok(NULL, "|");
            if(cPtr != NULL)
                fcom_Rtrim(cPtr,strlen(cPtr));
            switch(tokCnt)
            {
                case 0: // 직원명
                {
                    snprintf(pstUser[UserIdx].Name,sizeof(pstUser[UserIdx].Name),"%s",cPtr);
                    if(fcom_EucKrTOUtf8( pstUser[UserIdx].Name, sizeof(pstUser[UserIdx].Name)) != 0)
                    {
                        snprintf(pstUser[UserIdx].Name,sizeof(pstUser[UserIdx].Name),"%s",cPtr);
                    }
                    break;
                }
                case 1: //관할부점코드
                {
                    snprintf(pstUser[UserIdx].BranchCode, sizeof(pstUser[UserIdx].BranchCode), "%s", cPtr);
                    break;
                }
                case 2: //소속명
                {
                    snprintf(pstUser[UserIdx].BranchName,sizeof(pstUser[UserIdx].BranchName),"%s",cPtr);
                    if(fcom_EucKrTOUtf8( pstUser[UserIdx].BranchName, sizeof(pstUser[UserIdx].BranchName)) != 0)
                    {
                        snprintf(pstUser[UserIdx].BranchName,sizeof(pstUser[UserIdx].BranchName),"%s",cPtr);
                    }
                    break;
                }
                case 3: //현재소속년월일
                {
                    snprintf(pstUser[UserIdx].JoinDate, sizeof(pstUser[UserIdx].JoinDate), "%s", cPtr);
                    break;
                }
                case 4: //실근무지부점코드
                {
                    snprintf(pstUser[UserIdx].WorkplaceCode, sizeof(pstUser[UserIdx].WorkplaceCode), "%s", cPtr);
                    break;
                }
                case 5: //소속명
                {
                    snprintf(pstUser[UserIdx].WorkplaceName,sizeof(pstUser[UserIdx].WorkplaceName),"%s",cPtr);
                    if(fcom_EucKrTOUtf8( pstUser[UserIdx].WorkplaceName, sizeof(pstUser[UserIdx].WorkplaceName)) != 0)
                    {
                        snprintf(pstUser[UserIdx].WorkplaceName,sizeof(pstUser[UserIdx].WorkplaceName),"%s",cPtr);
                    }
                    break;
                }
                case 6: //소속팀코드
                {
                    snprintf(pstUser[UserIdx].BelongTeamCode, sizeof(pstUser[UserIdx].BelongTeamCode), "%d",atoi(cPtr));
                    break;
                }
                case 7: //소속명
                {
                    snprintf(pstUser[UserIdx].BelongTeamName,sizeof(pstUser[UserIdx].BelongTeamName),"%s",cPtr);
                    if(fcom_EucKrTOUtf8( pstUser[UserIdx].BelongTeamName, sizeof(pstUser[UserIdx].BelongTeamName)) != 0)
                    {
                        snprintf(pstUser[UserIdx].BelongTeamName,sizeof(pstUser[UserIdx].BelongTeamName),"%s",cPtr);
                    }
                    break;
                }
                case 8: //관할본부코드
                {
                    snprintf(pstUser[UserIdx].JrsdHqcd, sizeof(pstUser[UserIdx].JrsdHqcd), "%s",cPtr);
                    break;
                }
                case 9://소속명
                {
                    snprintf(pstUser[UserIdx].BelongTeamName2,sizeof(pstUser[UserIdx].BelongTeamName2),"%s",cPtr);
                    if(fcom_EucKrTOUtf8( pstUser[UserIdx].BelongTeamName2, sizeof(pstUser[UserIdx].BelongTeamName2)) != 0)
                    {
                        snprintf(pstUser[UserIdx].BelongTeamName2,sizeof(pstUser[UserIdx].BelongTeamName2),"%s",cPtr);
                    }
                    break;
                }
                case 10:  //보임코드
                {
                    snprintf(pstUser[UserIdx].MndtCd, sizeof(pstUser[UserIdx].MndtCd), "%s",cPtr);
                    break;
                }
                case 11: //직급코드
                {
                    snprintf(pstUser[UserIdx].JbclCd, sizeof(pstUser[UserIdx].JbclCd), "%s",cPtr);
                    break;
                }
                case 12: //직위코드
                {
                    snprintf(pstUser[UserIdx].PositionCd, sizeof(pstUser[UserIdx].PositionCd), "%s", cPtr);
                    break;
                }
                case 13: //약칭직위명
                {
                    snprintf(pstUser[UserIdx].PositionName,sizeof(pstUser[UserIdx].PositionName),"%s",cPtr);
                    if(fcom_EucKrTOUtf8( pstUser[UserIdx].PositionName, sizeof(pstUser[UserIdx].PositionName)) != 0)
                    {
                        snprintf(pstUser[UserIdx].PositionName,sizeof(pstUser[UserIdx].PositionName),"%s",cPtr);
                    }
                    break;
                }
                case 14: //직책코드
                {
                    snprintf(pstUser[UserIdx].TitleCode, sizeof(pstUser[UserIdx].TitleCode), "%s",cPtr);
                    break;
                }
                case 15: //입행구분코드
                {
                    snprintf(pstUser[UserIdx].EtbnDcd, sizeof(pstUser[UserIdx].EtbnDcd), "%s",cPtr);
                    break;
                }
                case 16: //휴대폰번호
                {
                    snprintf(pstUser[UserIdx].Telephone, sizeof(pstUser[UserIdx].Telephone), "%s",cPtr);
                    break;
                }
                case 17://이메일주소
                {
                    snprintf(pstUser[UserIdx].Email, sizeof(pstUser[UserIdx].Email), "%s", cPtr);
                    if(fcom_ChkStrHangle(pstUser[UserIdx].Email) == 1)
                    {
                        if(fcom_EucKrTOUtf8( pstUser[UserIdx].Email, sizeof(pstUser[UserIdx].Email)) != 0)
                            snprintf(pstUser[UserIdx].Email,sizeof(pstUser[UserIdx].Email),"%s",cPtr);
                    }
                    break;
                }
                case 18: //생년월일
                {
                    snprintf(pstUser[UserIdx].BirthDay, sizeof(pstUser[UserIdx].BirthDay),"%s", cPtr);
                    break;
                }
                case 19: //입행년월일
                {
                    snprintf(pstUser[UserIdx].EtbnYnd, sizeof(pstUser[UserIdx].EtbnYnd),"%s", cPtr);
                    break;
                }
                case 20://퇴직년월일
                {
                    snprintf(pstUser[UserIdx].RetireDate, sizeof(pstUser[UserIdx].RetireDate),"%s", cPtr);
                    break;
                }
                case 21:  //서무구분
                {
                    snprintf(pstUser[UserIdx].Seumu, sizeof(pstUser[UserIdx].Seumu), "%s",cPtr);
                    break;
                }
                case 22: //조직 속성코드
                {
                    snprintf(pstUser[UserIdx].Chul, sizeof(pstUser[UserIdx].Chul), "%s",cPtr);
                    break;
                }
            }
        }
        UserIdx++;
    }

    fclose(fp);

    /* Convert 파일명_IBK */
    memset(szTemp, 0x00, sizeof(szTemp));
    snprintf(szTemp, sizeof(szTemp), "_%s",szSyncType);
    strcat(Filepath,szTemp);
    convFp = fopen(Filepath,"w");

    memset(szTemp, 0x00, sizeof(szTemp));
    snprintf(szTemp, sizeof(szTemp),
                    "# %s | %s | %s | %s | %s |" //5
                     " %s | %s | %s | %s | %s |" //10
                     " %s | %s | %s | %s | %s |" //15
                     " %s | %s | %s | %s | %s |" //20
                     " %s | %s | %s | %s | %s |" //25
                     " %s\n",
                 "EMN", // 직원번호
                 "AEMP_NM", //직원명
                 "BJURIS_BRCD", //관할부점코드
                 "CBENM", //환할부점명
                 "PRS_BE_YMD", //현재소속년월일
                 "MWK_BRCD",//실근무지부점코드
                 "GBENM", // 실근무지명
                 "BE_TEAM_CD",//소속팀코드
                 "BBENM",  //소속명
                 "BJURIS_HQCD", //관할본부코드
                 "EBENM", //관할본부명
                 "MNDT_CD", //보임명
                 "JBCL_CD", //직급코드
                 "JBTT_CD", //직위코드
                 "ABNM_JTM", //약칭직위명
                 "DUCD", //직책코드
                 "ETBN_DCD", //입행구분코드
                 "EMP_CPN", //전화번호
                 "EAD", //이메일
                 "RSNO", //주민등록번호
                 "ENBK_YMD", //입행년월일
                 "RETI_YMD",  //퇴직년월일
                 "SEUMU", //서무구분
                 "CHUL", //조직속성코드
                 "USCOMP_FLAG", //본사직원 or 외주직원 코드
                 "COMPANY_NAME " ); //회사명 , 26

    fprintf(convFp,"%s",szTemp);

    /* Convert File 생성 */
    for(nLoop = 0; nLoop < UserIdx; nLoop++)
    {
        memset(szTemp, 0x00, sizeof(szTemp));
        snprintf(szTemp, sizeof(szTemp),
                 " %s | %s | %s | %s | %s |" //5
                 " %s | %s | %s | %s | %s |" //10
                 " %s | %s | %s | %s | %s |" //15
                 " %s | %s | %s | %s | %s |" //20
                 " %s | %s | %s | %s | %s |" //25
                 " %s\n", //26
                 pstUser[nLoop].Userid,
                 pstUser[nLoop].Name,
                 pstUser[nLoop].BranchCode,
                 pstUser[nLoop].BranchName,
                 pstUser[nLoop].JoinDate, //5

                 pstUser[nLoop].WorkplaceCode,
                 pstUser[nLoop].WorkplaceName,
                 pstUser[nLoop].BelongTeamCode,
                 pstUser[nLoop].BelongTeamName,
                 pstUser[nLoop].JrsdHqcd, //10

                 pstUser[nLoop].BelongTeamName2,
                 pstUser[nLoop].MndtCd,
                 pstUser[nLoop].JbclCd,
                 pstUser[nLoop].PositionCd,
                 pstUser[nLoop].PositionName, //15

                 pstUser[nLoop].TitleCode,
                 pstUser[nLoop].EtbnDcd,
                 pstUser[nLoop].Telephone,
                 pstUser[nLoop].Email,
                 pstUser[nLoop].BirthDay, //20

                 pstUser[nLoop].EtbnYnd,
                 pstUser[nLoop].RetireDate,
                 pstUser[nLoop].Seumu,
                 pstUser[nLoop].Chul,
                 "0", //25

                 " " //26
                 );

                 fprintf(convFp,"%s",szTemp);

    }
    fclose(convFp);

    for(nLoop = 0; nLoop < UserIdx; nLoop++)
    {
        memset(szQuery, 0x00, sizeof(szQuery));

        sprintf(szQuery, " INSERT INTO SYNC_USER_TB( "
                         "EMN, " // 직원번호
                         "AEMP_NM, "//직원명
                         "BJURIS_BRCD, " //관할부점코드
                         "CBENM, "//환할부점명
                         "PRS_BE_YMD, " //현재소속년월일
                         "MWK_BRCD, " //실근무지부점코드
                         "GBENM, "  // 실근무지명
                         "BE_TEAM_CD, "  //소속팀코드
                         "BBENM, " //소속명
                         "BJURIS_HQCD, " //관할본부코드
                         "EBENM, " //관할본부명
                         "MNDT_CD, " //보임명
                         "JBCL_CD, " //직급코드
                         "JBTT_CD, " //직위코드
                         "ABNM_JTM, " //약칭직위명
                         "DUCD, " //직책코드
                         "ETBN_DCD, " //입행구분코드
                         "EMP_CPN, " //전화번호
                         "EAD, " //이메일
                         "RSNO, " //주민등록번호
                         "ENBK_YMD, " //입행년월일
                         "RETI_YMD, " //퇴직년월일
                         "SEUMU, "//서무구분
                         "CHUL, " //조직속성코드
                         "USCOMP_FLAG,  " //본사직원 or 외주직원 코드
                         "COMPANY_NAME ) "//회사명 , 26
                         " VALUES (   '%s',"
                         " '%s',"
                         " '%s',"
                         " '%s',"
                         " '%s',"

                         " '%s',"
                         " '%s',"
                         " '%s',"
                         " '%s',"
                         " '%s',"

                         " '%s',"
                         " '%s',"
                         " '%s',"
                         " '%s',"
                         " '%s',"

                         " '%s',"
                         " '%s',"
                         " '%s',"
                         " '%s',"
                         " '%s',"

                         " '%s',"
                         " '%s',"
                         " '%s',"
                         " '%s',"
                         " '%s',"

                         " '%s' )",
                pstUser[nLoop].Userid,
                pstUser[nLoop].Name,
                pstUser[nLoop].BranchCode,
                pstUser[nLoop].BranchName,
                pstUser[nLoop].JoinDate,

                pstUser[nLoop].WorkplaceCode,
                pstUser[nLoop].WorkplaceName,
                pstUser[nLoop].BelongTeamCode,
                pstUser[nLoop].BelongTeamName,
                pstUser[nLoop].JrsdHqcd,

                pstUser[nLoop].BelongTeamName2,
                pstUser[nLoop].MndtCd,
                pstUser[nLoop].JbclCd,
                pstUser[nLoop].PositionCd,
                pstUser[nLoop].PositionName,

                pstUser[nLoop].TitleCode,
                pstUser[nLoop].EtbnDcd,
                pstUser[nLoop].Telephone,
                pstUser[nLoop].Email,
                pstUser[nLoop].BirthDay,

                pstUser[nLoop].EtbnYnd,
                pstUser[nLoop].RetireDate,
                pstUser[nLoop].Seumu,
                pstUser[nLoop].Chul,
                "0",

                " ");

        fdb_SqlQuery2(g_stMyCon, szQuery);
        if(g_stMyCon->nErrCode != 0)
        {
            WRITE_CRITICAL(CATEGORY_DB,"Fail in query(%s), errmsg(%s)",szQuery,g_stMyCon->cpErrMsg );
        }
    }
    if(pstUser != NULL)
        free(pstUser);

    WRITE_INFO(CATEGORY_DEBUG,"insert SYNC_USER_TB Success (%d)",UserIdx );

    return 0;


}

int fstParseExUser(struct _stOutsourcingUser* pstExUser, char* Filepath, char* szSyncType)
{
    FILE *fp = NULL;
    FILE* convFp = NULL;
    char* cPtr = NULL;

    char szTemp[1024 + 1] = {0x00,};
    char szReplaceBuffer[1024 +1] = {0x00,};
    char szQuery[1024 +1] = {0x00,};

    int TotTokenCnt = 0;
    int tokCnt = 0;
    int UserIdx = 0;
    int nLoop = 0;


    memset(szTemp, 0x00, sizeof(szTemp));
    memset(szReplaceBuffer, 0x00, sizeof(szReplaceBuffer));
    memset(szQuery, 0x00, sizeof(szQuery));

    fp = fopen(Filepath,"r");
    memset(szTemp, 0x00, sizeof(szTemp));

    while(fgets(szTemp, sizeof(szTemp), fp) != NULL)
    {
        TotTokenCnt = fcom_TokenCnt(szTemp,"|");
        fcom_ReplaceAll(szTemp, "|", " |", szReplaceBuffer);
        cPtr = strtok(szReplaceBuffer,"|");
        if(cPtr != NULL)
            fcom_Rtrim(cPtr,strlen(cPtr));
        snprintf(pstExUser[UserIdx].Userid, sizeof(pstExUser[UserIdx].Userid), "%s", cPtr);
        for(tokCnt = 0; tokCnt < TotTokenCnt; tokCnt ++)
        {
            cPtr = strtok(NULL,"|");
            if(cPtr != NULL)
                fcom_Rtrim(cPtr,strlen(cPtr));
            switch(tokCnt)
            {
                case 0: // Enable Flag
                {
                    snprintf(pstExUser[UserIdx].EnableFlag,
                             sizeof(pstExUser[UserIdx].EnableFlag),
                             "%s", cPtr);
                    break;
                }
                case 1: // Name
                {
                    snprintf(pstExUser[UserIdx].Name,sizeof(pstExUser[UserIdx].Name),"%s",cPtr);
                    if(fcom_EucKrTOUtf8( pstExUser[UserIdx].Name, sizeof(pstExUser[UserIdx].Name)) != 0)
                    {
                        snprintf(pstExUser[UserIdx].Name,sizeof(pstExUser[UserIdx].Name),"%s",cPtr);
                    }
                    break;
                }
                case 2: // Email
                {
                    snprintf(pstExUser[UserIdx].Email, sizeof(pstExUser[UserIdx].Email), "%s", cPtr);
                    if(fcom_ChkStrHangle(pstExUser[UserIdx].Email) == 1)
                    {
                        if(fcom_EucKrTOUtf8( pstExUser[UserIdx].Email, sizeof(pstExUser[UserIdx].Email)) != 0)
                            snprintf(pstExUser[UserIdx].Email,sizeof(pstExUser[UserIdx].Email),"%s",cPtr);
                    }

                    break;
                }
                case 12: //Branch Code
                {
                    snprintf(pstExUser[UserIdx].BranchCode, sizeof(pstExUser[UserIdx].BranchCode), "%s", cPtr);
                    break;
                }
                case 13: // Branch Name
                {
                    snprintf(pstExUser[UserIdx].BranchName,sizeof(pstExUser[UserIdx].BranchName),"%s",cPtr);
                    if(fcom_EucKrTOUtf8( pstExUser[UserIdx].BranchName, sizeof(pstExUser[UserIdx].BranchName)) != 0)
                    {
                        snprintf(pstExUser[UserIdx].BranchName,sizeof(pstExUser[UserIdx].BranchName),"%s",cPtr);
                    }
                    break;
                }
                case 15: //Workplace Code
                {
                    snprintf(pstExUser[UserIdx].WorkplaceCode, sizeof(pstExUser[UserIdx].WorkplaceCode), "%s", cPtr);
                    break;
                }
                case 16: //Workplace Name
                {
                    snprintf(pstExUser[UserIdx].WorkplaceName,sizeof(pstExUser[UserIdx].WorkplaceName),"%s",cPtr);
                    if(fcom_EucKrTOUtf8( pstExUser[UserIdx].WorkplaceName, sizeof(pstExUser[UserIdx].WorkplaceName)) != 0)
                    {
                        snprintf(pstExUser[UserIdx].WorkplaceName,sizeof(pstExUser[UserIdx].WorkplaceName),"%s",cPtr);
                    }
                    break;
                }
                case 17: //Belong Team Code
                {
                    snprintf(pstExUser[UserIdx].BelongTeamCode, sizeof(pstExUser[UserIdx].BelongTeamCode), "%d", atoi(cPtr));
                    break;
                }
                case 18: //Belong Team Name
                {
                    snprintf(pstExUser[UserIdx].BelongTeamName,sizeof(pstExUser[UserIdx].BelongTeamName),"%s",cPtr);
                    if(fcom_EucKrTOUtf8( pstExUser[UserIdx].BelongTeamName, sizeof(pstExUser[UserIdx].BelongTeamName)) != 0)
                    {
                        snprintf(pstExUser[UserIdx].BelongTeamName,sizeof(pstExUser[UserIdx].BelongTeamName),"%s",cPtr);
                    }
                    break;
                }
                case 23: // Position Code
                {
                    snprintf(pstExUser[UserIdx].PositionCd, sizeof(pstExUser[UserIdx].PositionCd), "%s", cPtr);
                    break;
                }
                case 24: // Position Name
                {
                    snprintf(pstExUser[UserIdx].PositionName,sizeof(pstExUser[UserIdx].PositionName),"%s",cPtr);
                    if(fcom_EucKrTOUtf8( pstExUser[UserIdx].PositionName, sizeof(pstExUser[UserIdx].PositionName)) != 0)
                    {
                        snprintf(pstExUser[UserIdx].PositionName,sizeof(pstExUser[UserIdx].PositionName),"%s",cPtr);
                    }
                    break;
                }
                case 28: //Join Date
                {
                    snprintf(pstExUser[UserIdx].JoinDate, sizeof(pstExUser[UserIdx].JoinDate), "%s", cPtr);
                    break;
                }
                case 29: //Retire Date
                {
                    snprintf(pstExUser[UserIdx].RetireDate, sizeof(pstExUser[UserIdx].RetireDate), "%s", cPtr);
                    break;
                }
                case 31: // BirthDate
                {
                    snprintf(pstExUser[UserIdx].BirthDay, sizeof(pstExUser[UserIdx].BirthDay), "%s", cPtr);
                    break;
                }
                case 34: //TelePhone
                {
                    char Temp1[3 +1] = {0x00,};//010
                    char Temp2[4 +1] = {0x00,};//1111(중간번호)
                    char Temp3[4 +1] = {0x00,};//2222(앞에번호)

                    memset(pstExUser[UserIdx].Telephone, 0x00, sizeof(pstExUser[UserIdx].Telephone));
                    snprintf(pstExUser[UserIdx].Telephone, sizeof(pstExUser[UserIdx].Telephone), "%s", cPtr);
                    if(strlen(pstExUser[UserIdx].Telephone) == 11 &&
                       memcmp(&(pstExUser[UserIdx].Telephone[0]),"010",3) == 0) /* 국제전화 또는 전화번호 없는경우 포맷팅 X.*/
                    {
                        memcpy(Temp1,&(pstExUser[UserIdx].Telephone[0]),3);
                        memcpy(Temp2,&(pstExUser[UserIdx].Telephone[3]),4);
                        memcpy(Temp3,&(pstExUser[UserIdx].Telephone[7]),4);

                        memset(pstExUser[UserIdx].Telephone, 0x00, sizeof(pstExUser[UserIdx].Telephone));
                        sprintf(pstExUser[UserIdx].Telephone,"%s-%s-%s",Temp1,Temp2,Temp3);
                    }
                    break;
                }
                case 36: //Company Name
                {
                    snprintf(pstExUser[UserIdx].CompanyName, sizeof(pstExUser[UserIdx].CompanyName), "%s", cPtr);
                    if(fcom_EucKrTOUtf8( pstExUser[UserIdx].CompanyName, sizeof(pstExUser[UserIdx].CompanyName)) != 0)
                    {
                        snprintf(pstExUser[UserIdx].CompanyName,sizeof(pstExUser[UserIdx].CompanyName),"%s",cPtr);
                    }
                    break;
                }
            }

        }
        UserIdx++;
    }

    if(fp != NULL)
        fclose(fp);

    /* Convert 파일명_IBK */
    memset(szTemp, 0x00, sizeof(szTemp));
    snprintf(szTemp, sizeof(szTemp), "_%s",szSyncType);
    strcat(Filepath,szTemp);
    convFp = fopen(Filepath,"w");

    memset(szTemp, 0x00, sizeof(szTemp));
    snprintf(szTemp, sizeof(szTemp),
             "# %s | %s | %s | %s | %s |" //5
             " %s | %s | %s | %s | %s |" //10
             " %s | %s | %s | %s | %s |" //15
             " %s | %s | %s | %s | %s |" //20
             " %s | %s | %s | %s | %s |" //25
             " %s\n",
             "EMN", // 직원번호
             "AEMP_NM",  //직원명
             "BJURIS_BRCD",//관할부점코드
             "CBENM", //환할부점명
             "PRS_BE_YMD", //현재소속년월일

             "MWK_BRCD", //실근무지부점코드
             "GBENM", // 실근무지명
             "BE_TEAM_CD", //소속팀코드
             "BBENM", //소속명
             "BJURIS_HQCD", //관할본부코드

             "EBENM", //관할본부명
             "MNDT_CD", //보임명
             "JBCL_CD",  //직급코드
             "JBTT_CD", //직위코드
             "ABNM_JTM", //약칭직위명

             "DUCD", //직책코드
             "ETBN_DCD", //입행구분코드
             "EMP_CPN", //전화번호
             "EAD", //이메일
             "RSNO", //주민등록번호

             "ENBK_YMD", //입행년월일
             "RETI_YMD",//퇴직년월일
             "SEUMU",//서무구분
             "CHUL", //조직속성코드
             "USCOMP_FLAG", //본사직원 or 외주직원 코드

             "COMPANY_NAME " );//회사명 , 26

    fprintf(convFp,"%s",szTemp);

    /* Convert File ���� */
    for(nLoop = 0; nLoop < UserIdx; nLoop++)
    {
        memset(szTemp, 0x00, sizeof(szTemp));
        snprintf(szTemp, sizeof(szTemp),
                 " %s | %s | %s | %s | %s |" //5
                 " %s | %s | %s | %s | %s |" //10
                 " %s | %s | %s | %s | %s |" //15
                 " %s | %s | %s | %s | %s |" //20
                 " %s | %s | %s | %s | %s |" //25
                 " %s\n", //26
                 pstExUser[nLoop].Userid,  //직원번호
                 pstExUser[nLoop].Name,//직원명
                 pstExUser[nLoop].BranchCode, //관할부점코드
                 pstExUser[nLoop].BranchName, //관할부점명
                 pstExUser[nLoop].JoinDate,  //현재소속년월일

                 pstExUser[nLoop].WorkplaceCode, //실근무지 부점코드
                 pstExUser[nLoop].WorkplaceName, //실근무지명
                 pstExUser[nLoop].BelongTeamCode,  //소속팀코드
                 pstExUser[nLoop].BelongTeamName, //소속팀명
                 "",  //관할본부코드

                 "", //관할본부명
                 pstExUser[nLoop].MndtCd, //보임명
                 pstExUser[nLoop].JbclCd, //직급코드
                 pstExUser[nLoop].PositionCd, //직위코드
                 pstExUser[nLoop].PositionName,//직위명

                 pstExUser[nLoop].TitleCode, //직책코드
                 "", //입행구분코드
                 pstExUser[nLoop].Telephone, //전화번호
                 pstExUser[nLoop].Email, //이메일
                 pstExUser[nLoop].BirthDay, //생년월일

                 "",//입행 년월일
                 pstExUser[nLoop].RetireDate,  //퇴직년월일
                 pstExUser[nLoop].Seumu, //서무구분
                 pstExUser[nLoop].Chul, //조직속성코드
                 "1",  //본사직원

                 pstExUser[nLoop].CompanyName);//회사명

        fprintf(convFp,"%s",szTemp);

    }
    fclose(convFp);

    /* ------------------------------------------------------------------- */
    /* INSERT SYNC_USER_TB */
    for(nLoop = 0; nLoop < UserIdx; nLoop++)
    {
        memset(szQuery, 0x00, sizeof(szQuery));
        sprintf(szQuery, " INSERT INTO SYNC_USER_TB( "
                         "EMN, " // 직원번호
                         "AEMP_NM, " //직원명
                         "BJURIS_BRCD, " //관할부점코드
                         "CBENM, " //관환할부점명
                         "PRS_BE_YMD, " //현재소속년월일
                         "MWK_BRCD, " //실근무지부점코드
                         "GBENM, "  // 실근무지명
                         "BE_TEAM_CD, " //소속팀코드
                         "BBENM, " //소속명
                         "BJURIS_HQCD, " //관할본부코드
                         "EBENM, " //관할본부명
                         "MNDT_CD, "  //보임명
                         "JBCL_CD, " //직급코드
                         "JBTT_CD, " //직위코드
                         "ABNM_JTM, " //약칭직위명
                         "DUCD, "//직책코드
                         "ETBN_DCD, "//입행구분코드
                         "EMP_CPN, "//전화번호
                         "EAD, " //이메일
                         "RSNO, " //주민등록번호
                         "ENBK_YMD, " //입행년월일
                         "RETI_YMD, " //퇴직년월일
                         "SEUMU, " //서무구분
                         "CHUL, "//조직속성코드
                         "USCOMP_FLAG,  "//본사직원 or 외주직원 코드
                         "COMPANY_NAME ) " //회사명 , 26
                         " VALUES (   '%s',"
                         " '%s',"
                         " '%s',"
                         " '%s',"
                         " '%s',"

                         " '%s',"
                         " '%s',"
                         " '%s',"
                         " '%s',"
                         " '%s',"

                         " '%s',"
                         " '%s',"
                         " '%s',"
                         " '%s',"
                         " '%s',"

                         " '%s',"
                         " '%s',"
                         " '%s',"
                         " '%s',"
                         " '%s',"

                         " '%s',"
                         " '%s',"
                         " '%s',"
                         " '%s',"
                         " '%s',"

                         " '%s' )", //26
                pstExUser[nLoop].Userid, //직원번호
                pstExUser[nLoop].Name,//직원명
                pstExUser[nLoop].BranchCode,//관할부점코드
                pstExUser[nLoop].BranchName, //관할부점명
                pstExUser[nLoop].JoinDate,  //현재소속년월일

                pstExUser[nLoop].WorkplaceCode,//실근무지 부점코드
                pstExUser[nLoop].WorkplaceName,//실근무지명
                pstExUser[nLoop].BelongTeamCode, //소속팀코드
                pstExUser[nLoop].BelongTeamName, //소속팀명
                "", //관할본부코드

                "",//관할본부명
                pstExUser[nLoop].MndtCd,  //보임명
                pstExUser[nLoop].JbclCd, //직급코드
                pstExUser[nLoop].PositionCd,//직위코드
                pstExUser[nLoop].PositionName,//직위명

                pstExUser[nLoop].TitleCode,//직책코드
                "",//입행구분코드
                pstExUser[nLoop].Telephone,  //전화번호
                pstExUser[nLoop].Email, //이메일
                pstExUser[nLoop].BirthDay, //생년월일

                "", //입행 년월일
                pstExUser[nLoop].RetireDate, //퇴직년월일
                pstExUser[nLoop].Seumu, //서무구분
                pstExUser[nLoop].Chul, //조직속성코드
                "1", //본사직원

                pstExUser[nLoop].CompanyName //회사명
        );

        fdb_SqlQuery2(g_stMyCon, szQuery);
        if(g_stMyCon->nErrCode != 0)
        {
            WRITE_CRITICAL(CATEGORY_DB,"Fail in query(%s), errmsg(%s)",szQuery,g_stMyCon->cpErrMsg );
        }
    }
    if(pstExUser != NULL)
        free(pstExUser);

    WRITE_INFO(CATEGORY_DEBUG,"insert EX_SYNC_USER_TB Success (%d)",nLoop );

    return 0;

}
