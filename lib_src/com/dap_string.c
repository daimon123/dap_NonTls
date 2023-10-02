//
// Created by KimByoungGook on 2020-06-10.
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "com/dap_com.h"

void fcom_Rtrim(char *data, int len)
{
    int cnt;

    if (len != 0)
    {
        for (cnt = len - 1; cnt >= 0; cnt--)
        {
            if (data[cnt] != ' ' && data[cnt] != '\t' && data[cnt] != '\n') return;
            if (data[cnt] == ' ' || data[cnt] == '\t' || data[cnt] == '\n') data[cnt] = 0x00;
        }
    }
}


void fcom_Ltrim( char *str )
{
    int len = strlen(str);

    if(len != 0)
    {
        for ( ; *str; str++ )
        {
            if ( *str != ' ' && *str != '\t' && *str != '\n')
            {
                break;
            }
        }
    }
}

void fcom_Trim( char *str, int len )
{
    fcom_Ltrim(str);
    fcom_Rtrim(str, len);
}

void fcom_SpaceTrim(char *str, int len)
{
    char temp[2048];
    int cnt, cnt1 = 0;

    memset(temp, 0x00, sizeof(temp));

    for (cnt = 0; cnt < len; cnt++)
    {
        if (str[cnt] == ' ') continue;
        else temp[cnt1++] = str[cnt];
    }

    strcpy(str, temp);
}

char* fcom_StrUpper(char *str)
{
    char *cp;

    if (!str)
    {
        return NULL;
    }
    for (cp = str; *cp; cp++)
        if (*cp >= 'a' && *cp <= 'z')
            *cp -= 32;

    return str;
}

int fcom_ChkStrHangle(char* buffer)
{
    int nIdx;

    for(nIdx = 0; nIdx < strlen(buffer); nIdx++)
    {
        if(buffer[nIdx] & 0x80)
        {
            return 1;
        }
    }
    return 0;
}

void fcom_ParseStringByToken(char* szStr, char* szTok, char *res, int nOrder)
{
    char* pch = szStr;    //파일 한 줄씩 인덱싱한 포인터배열 참조

    char* pOld = pch;
    char *pNew;
    char Tmp[1024] = {0,};
    int len;
    int nCount=0;
    char* resultString = NULL;    //결과 스트링 배열 포인터 배열

    while (pOld)
    {
        pNew = strstr(pOld, szTok);
        if (pNew)    //구분자로 찾았으면
        {
            len = pNew-pOld;
            strncpy(Tmp, pOld, len);
            Tmp[len] = 0;
            pOld = pNew+strlen(szTok);
        }
        else
        {
            // 마지막 부분은 구분자가 없다!
            strcpy(Tmp, pOld);
            pOld = NULL; // 탈출 조건
        }

        if(nCount == nOrder)
        {
            resultString = strdup(Tmp);
            strcpy(res, resultString);
            fcom_MallocFree((void**)&resultString);
            return;
        }
        nCount++;
    }
}


void fcom_ReplaceAll(char *inBuf, char *olds, char *news, char *res)
{
    char *result = NULL, *sr = NULL;
    size_t offet = 0, count = 0;
    size_t oldlen = strlen(olds); //if (oldlen < 1) return inBuf;
    size_t newlen = strlen(news);

    if (oldlen < 1)
        return;

    if (newlen != oldlen)
    {
        for (offet = 0; inBuf[offet] != '\0';)
        {
            if (strncmp(&inBuf[offet], olds, oldlen) == 0)
            {
                count++;
                offet += oldlen;
            }
            else
            {
                offet++;
            }
        }
    }
    else
        offet = strlen(inBuf);


    if(fcom_malloc((void**)&result, offet + 1 + count * (newlen - oldlen)) != 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
        return;
    }

    sr = result;
    while (*inBuf)
    {
        if (strncmp(inBuf, olds, oldlen) == 0)
        {
            strncpy(sr, news, newlen);
            sr += newlen;
            inBuf  += oldlen;
        }
        else
        {
            *sr++ = *inBuf++;
        }
    }
    *sr = '\0';

    strcpy(res, result);
    fcom_MallocFree((void**)&result);

    //return res;
}

void fcom_GetFQNum(char *orgstr, char *mark, char *substr)
{
    int orgOffset,subOffset;
    subOffset = 0;
    for(orgOffset = 0; orgOffset < (int)strlen(orgstr); orgOffset++)
    {
        if(strncmp(orgstr + orgOffset, (char *)mark, strlen((char *)mark)) == 0)
        {
            break;
        }
        substr[subOffset] = orgstr[orgOffset];
        subOffset++;
    }
}

int fcom_StrToken(char *str, char *mark, int cnt, char* res)
{
    int FindOffset = 0;
    char *point;

    point = (char *)strtok(str, mark);

    if((point != NULL) && (cnt == FindOffset))
    {
        strcpy(res, point);
        return sizeof(res);
    }
    FindOffset++;

    while(point != NULL)
    {
        point = (char *)strtok(NULL, mark);
        if((point != NULL) && (cnt == FindOffset))
        {
            strcpy(res, point);
            point = NULL;
            return sizeof(res);
        }

        FindOffset++;
    }

    return 0;
}
/* strtok() 함수가 Thread Safe 하지 않아서 새로 만듬. */
int fcom_StrTokenR(char *str, char *mark, int cnt, char* res)
{
    int FindOffset = 0;
    char *RemainPtr = NULL;
    char *point = NULL;

    point = (char *)strtok_r(str, mark,&RemainPtr);

    if((point != NULL) && (cnt == FindOffset))
    {
        strcpy(res, point);
        return sizeof(res);
    }
    FindOffset++;

    while(point != NULL)
    {
        point = (char *)strtok_r(NULL, mark,&RemainPtr);
        if((point != NULL) && (cnt == FindOffset))
        {
            strcpy(res, point);
            point = NULL;
            return sizeof(res);
        }

        FindOffset++;
    }

    return 0;
}

int fcom_TokenCnt(char *orgstr, char *mark)
{
    int mark_cnt = 0;
    int strIdx = 0;

    for(strIdx = 0; strIdx < (int)strlen(orgstr); strIdx ++)
    {
        if(strncmp(orgstr + strIdx, (char *)mark, strlen((char *)mark)) == 0)
            mark_cnt ++;
    }
    return mark_cnt;
}

void fcom_SubStr(int start, int len, char *orgstr, char *substr)
{
    int orgOffset,subOffset;
    subOffset = 0;

//    if( strlen(orgstr)  < start+len  )
//    {
//        len =  len - (  (start+len) - strlen(orgstr) );
//    }

    for(orgOffset = start - 1; orgOffset < start + len - 1; orgOffset++)
    {
        substr[subOffset] = orgstr[orgOffset];
        subOffset++;
    }
}


int fcom_GetStringPos(char *s, char c)
{
    int offset;

    for ( offset = 0; s[offset] != '\0'; offset++ )
    {
        if ( s[offset] == c )
            return offset;
    }

    return -1;
}
int fcom_GetRoundTagValue(char*in, char* out)
{
    char* s_pos;
    char* e_pos;

    if((s_pos = strrchr(in, '(')) == NULL)
    {
        s_pos = in;
        return -1;
    }
    else {
        s_pos += strlen("(");
    }

    if((e_pos = strrchr(s_pos, ')')) == NULL)
    {
        s_pos = in;

    }
    else {
        strncpy(out,s_pos, e_pos-s_pos);
    }

    return (e_pos + strlen(")")) - in;
}

int fcom_GetReversePos(char *s, char c)
{
    int i;
    int len;

    len = strlen(s);

    for ( i = len-1; i >= 0; i-- )
    {
        if ( s[i] == c )
            return i;
    }

    return -1;
}

int fcom_getBClassIp(char *str, char* res)
{
    int		loop;
    int		bFirst = 1;
    char	tmpData[5] = {0x00,};
    char*	point = NULL;
    char*   ptr = NULL;

    point = (char *)strtok_r(str, ".",&ptr);

    loop = 0;
    while(point != NULL)
    {
        if(loop > 2) break;
        if(bFirst)
        {
            strcat(res, point);
            bFirst = 0;
        }
        else
        {
            memset(tmpData, 0x00, sizeof(tmpData));
            sprintf(tmpData, ".%s", point);
            strcat(res, tmpData);
        }
        point = strtok_r(NULL, ".",&ptr);
        loop++;
    }

    return 0;
}



//int fcom_GetTagValue(char*in, char* tag1, char* tag2, char* out)
int fcom_GetTagValue(char*in, char* tag1, char* tag2, char* out, int outBufferSize)
{
    unsigned int local_nStrSize = 0;

    char* s_pos;
    char* e_pos;

    if( ( s_pos = strstr(in, tag1)) == NULL )
    {
        s_pos = in;
        return -1;
    }
    else
    {
        s_pos += strlen(tag1);
    }

    if( ( e_pos = strstr(s_pos, tag2)) == NULL )
    {
        s_pos = in;

    }
    else
    {
        // 메모리 터지므로 수정.
        local_nStrSize = e_pos - s_pos;
        if ( local_nStrSize > outBufferSize)
        {
            local_nStrSize = outBufferSize-1;
        }

//        strncpy(out,s_pos, e_pos-s_pos);
        strncpy(out,s_pos, local_nStrSize);

//        local_nStrSize = strlen(s_pos);
//        if ( local_nStrSize > outBufferSize )
//        {
//            local_nStrSize = outBufferSize;
//        }
//        snprintf(out, local_nStrSize, "%s", s_pos );
    }

    return (e_pos + strlen(tag2)) - in;
}


char** fcom_Jsplit(char* tmp, char ch)
{
    int	i = 0;
    int	j = 0;
    int count = 0;
    int ch_count = 0;
    int length = 0;
    int max_length = 0;
    int index = 0;
    char **return_array = NULL;

    while(1)
    {
        length++;
        if(*(tmp + count) == ch)
        {
            ch_count++;
            if(max_length < length)
            {
                max_length = length;
                length = 0;
            }
        }
        count++;
        if(*(tmp + count) == 0)
        {
            break;
        }
    }
    ch_count++;

    return_array = (char**)malloc(sizeof(char*) * ch_count);
    memset(return_array, 0x00, sizeof(char*) * ch_count);

    for(i=0; i<ch_count; i++)
    {
        return_array[i] = (char*) malloc(( sizeof(char) * max_length) +1 ) ;
        memset(return_array[i], 0x00, ( sizeof(char) * max_length ) +1);

        for(j=0; j<max_length; j++)
        {
            return_array[i][j] = *(tmp + index);
            index++;
            if(*(tmp + index) == ch)
            {
                index++;
                break;
            }
            else if(*(tmp + index) == 0)
            {
                break;
            }
        }
        return_array[i][j+1] = '\0';
    }
    return return_array;
}

int fcom_Str2ValToken(char *str, char *mark, char* res1, char* res2)
{
    char *point = NULL;
    char *RemainPtr = NULL;

    point = (char *)strtok_r(str, mark,&RemainPtr);

    if(point != NULL)
    {
        strcpy(res1, point);
        point = NULL;
    }
    point = (char *)strtok_r(NULL, mark,&RemainPtr);
    if(point != NULL)
    {
        strcpy(res2, point);
        point = NULL;
    }

    return 0;
}

char *fcom_StringAllocExing(int len)
{
    char *tmp;

    tmp=(char*)malloc(len*sizeof(char));
    if(tmp==NULL)
    {
        printf("**Error while allocating string.\n");
        exit((-1));
    }

    return(tmp);
}

void* fcom_BufferAllocReturning(int len,int *result)
{
    void *tmp;

    tmp=(void*)malloc(len*sizeof(void));
    if(tmp==NULL)
    {
        printf("**Error while allocating buffer.\n");
        *result=1;
    }
    else
    {
        *result=0;
    }

    return(tmp);
}

int fcom_IsDigit(char* Buffer)
{
    int nIdx = 0;

    for(nIdx = 0; nIdx < strlen(Buffer); nIdx++)
    {
        /* 숫자 */
        if(isdigit(Buffer[nIdx]) != 0 )
            return (-1);
    }
    /* 문자 */
    return 0;
}
