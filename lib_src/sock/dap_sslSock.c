//
// Created by KimByoungGook on 2020-06-22.
//


/*---------------------------------------------------------------------
 *  *
 *   *  FUNCTION NAME   : socketSend
 *    *  PURPOSE         : socket library send function
 *     *  INPUT ARGUMENTS : SocketFd, stData, nLength, RxtErr
 *      *  RETURN VALUE    : none
 *       *  GLOBAL VARIABLES
 *        *    1) EXTERNAL   : none
 *         *    2) LOCAL      : none
 *          *
 *          ----------------------------------------------------------------------*/
#include    <stdio.h>
#include    <fcntl.h>
#include    <stdlib.h>
#include    <unistd.h>
#include    <string.h>
#include    <time.h>
#include    <sys/time.h>
#include    <errno.h>
#include    <sys/types.h>
#include    <sys/sysinfo.h>
#include    <sys/wait.h>

#include    <signal.h>
#include    <sys/socket.h>
#include    <netinet/in.h>
#include    <limits.h>
#include    <netdb.h>
#include    <arpa/inet.h>

#include <openssl/rsa.h>       /* SSLeay stuff */
#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include "com/dap_com.h"
#include "com/dap_req.h"

#define CHK_NULL(x) if ((x)==NULL) exit (1)
#define CHK_ERR(err,s) if ((err)==-1) { perror(s); exit(1); }
#define CHK_SSL(err) if ((err)==-1) { ERR_print_errors_fp(stderr); exit(2); }
#define USE_SSL

#include "sock/dap_sock.h"
#include "secure/dap_secure.h"

static unsigned char ENC_AESKEY[] =
        {
                0x31, 0xf7, 0xe5, 0xab, 0x43, 0xd2,
                0x43, 0x9f, 0x87, 0xf1, 0x13, 0x6f,
                0x14, 0xcd, 0x96, 0x5f,	0xff, 0x95,
                0x10, 0x7c, 0x6b, 0xc8, 0x4d, 0x8c,
                0x93, 0xbb, 0x0e, 0x88, 0xd8, 0xe8,
                0xe9, 0xf5
        };

static unsigned char ENC_IV[] =
        {
                0x32, 0xf8, 0xe6, 0xac, 0x44,
                0xd3, 0x44, 0xa0, 0x88, 0xf2,
                0x14, 0x70, 0x15, 0xce, 0x07, 0x60,
                0x11, 0x45, 0x35, 0xce, 0x17, 0x50,
                0x12, 0x46, 0x36, 0xc6, 0x18, 0x51,
                0x13, 0x42, 0x31, 0xc6
        };


void fsock_CloseOpenssl(SSL *p_ssl)
{
    int sslFd;

    /* Free the SSL structure */
    if(p_ssl != NULL)
    {
        sslFd = SSL_get_fd(p_ssl);

        /* Shutdown the client side of the SSL connection */
        /* Terminate communication on a socket */
        SSL_shutdown(p_ssl);
        SSL_free(p_ssl);
        if(sslFd > 0)
        {
            close(sslFd);
            sslFd = 0;
        }
        p_ssl = NULL;
    }

}

void fsock_InitOpenssl()
{
    /* SSL 통신을 위해 암호와 해시 알고리즘 로드 */
    SSL_library_init();

    /* 에러 기록을 남기기 위한 에러 관련 문자열 로드 */
    SSL_load_error_strings();

    /* 모든 알고리즘 로드 */
    OpenSSL_add_all_algorithms();

    ERR_load_BIO_strings();
    ERR_load_crypto_strings();
}

SSL_CTX* fsock_CreateContext()
{
    const SSL_METHOD *method;
    SSL_CTX *ctx;
    // SSL 버전3 프로토콜 사용
    method = SSLv23_server_method();
    // SSL 컨텍스트 생성
    ctx = SSL_CTX_new(method);
    if (ctx == NULL)
    {
        perror("Unable to create SSL context");
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    SSL_CTX_set_mode(ctx, SSL_MODE_AUTO_RETRY);

    return ctx;
}
void fsock_CleanupOpenssl()
{
    ERR_free_strings();
    EVP_cleanup();
    ERR_remove_state(0);
}

int fsock_SslSocketSend(SSL *ssl, char * stData, int nLength)
{
    int     nRet;
    int     nLeft, nWritten;
    char    *cpPtr;


    nLeft = nLength;

    cpPtr = (char *)stData;

    while (nLeft > 0)
    {

        nWritten = SSL_write(ssl, cpPtr, nLeft);

        if (nWritten <= 0)
        {

            WRITE_CRITICAL(CATEGORY_IPC,"SSL_write Failed code : (%d) msg(%s) ",errno,strerror(errno));
//            SSL_free(ssl);

            return -1;
        };

        nLeft -= nWritten;
        cpPtr += nWritten;
    }
    nRet = ( nLength - nLeft);

    return nRet;

}
//i32 AstSocket::ReadSslPacket(i32 fd, void* buf, size_t len)
//{
//    int ssl_len = SSL_read(_ssl, buf, len);
//    DebugPrintf("[%s %d] SSL read ok fd:%d buf_len:%d ssl_len:%d tid:%d\n", __FILE__, __LINE__, _sock_fd, len, ssl_len, TcpServer::Instance()->GetCurrentTid());
//    if (ssl_len < 0)
//    {
////        glogger->debug("SSL Read Packet len : {} error : {} msg : {} state:{}",
////                      ssl_len, errno, strerror(errno), SSL_state_string(_ssl));
//        if (memcmp(SSL_state_string(_ssl), "SSLOK", 5) != 0)
//        {
//            int error = SSL_get_error(_ssl, ssl_len);
//            switch(error)
//            {
//                case SSL_ERROR_NONE:             { glogger->debug("SSL_ERROR_NONE");              break;   }
//                case SSL_ERROR_SYSCALL:          { glogger->debug("SSL_ERROR_SYSCALL");           break;   }
//                case SSL_ERROR_WANT_WRITE:       { glogger->debug("SSL_ERROR_WANT_WRITE");        break;   }
//                case SSL_ERROR_WANT_READ:
//                {
//                    glogger->debug("SSL_ERROR_WANT_READ");
//                    errno = EAGAIN;
//                    break;
//                }
//                case SSL_ERROR_WANT_X509_LOOKUP: { glogger->debug("SSL_ERROR_WANT_X509_LOOKUP");  break;   }
//                case SSL_ERROR_SSL:              { glogger->debug("SSL_ERROR_SSL");               break;   }
//                case SSL_ERROR_ZERO_RETURN:      { glogger->debug("SSL_ERROR_ZERO_RETURN");       break;   }
//                default:
//                {
//                    glogger->debug("SSL Error Msg : {}", ERR_error_string(ERR_get_error(),NULL) );
//                    break;
//                }
//            }
//        }
//
//        return ssl_len;
//    }
//    return ssl_len;
//}
int fsock_SslSocketRecv(SSL *ssl, char *stData, int nLength)
{
    char           *cpPtr;
    int             nThisRead = -1;
    int             nBytesRead = 0;
    int             Stat = 1;
    int             local_cnt = 0;

    cpPtr = stData;

    while ( (nBytesRead < nLength)  && (Stat > 0) )
    {
        if(local_cnt > 100000)
            break;

        nThisRead = SSL_read(ssl, cpPtr, nLength - nBytesRead);
        if (nThisRead <= 0)
        {
            return nThisRead;
        }

        nBytesRead += nThisRead;
        cpPtr += nThisRead;

        sleep(0);
        local_cnt++;
    } // while

    if ( Stat <= 0 )
    {
        return Stat;
    }

    stData = cpPtr - nBytesRead;

    return nBytesRead;
}





void fsock_ConfigureContext(SSL_CTX *ctx, char* CertFile, char* KeyFile)
{
#ifdef __CA__
    /* set maximum depth for the certificate chain */
    //SSL_CTX_set_verify_depth(ctx, 1);
    /* set voluntary certification mode*/
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);
    /* set mandatory certification mode*/
    //SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER|SSL_VERIFY_FAIL_IF_NO_PEER_CERT, verify_callback);
    /* load CA certificate file */
    if (SSL_CTX_load_verify_locations(ctx, CaFile, NULL) <=0)
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
#endif
    SSL_CTX_set_ecdh_auto(ctx, 1);

    // 자신의 인증서를 파일에서 로딩한다.
    if (SSL_CTX_use_certificate_file(ctx, CertFile, SSL_FILETYPE_PEM) <= 0)
    {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

#ifdef __CA__
    /* set server private key password */
    SSL_CTX_set_default_passwd_cb_userdata(ctx, (void*)PRIKEY_PASSWD);
#endif

    // 자신의 개인키를 파일에서 로딩한다.
    if (SSL_CTX_use_PrivateKey_file(ctx, KeyFile, SSL_FILETYPE_PEM) <= 0)
    {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    // 읽은 인증서와 개인키가 맞는지 확인 한다.
    if (!SSL_CTX_check_private_key(ctx)) {
        fprintf(stderr,"Private key does not match the certificate public key\n");
        exit(EXIT_FAILURE);
    }

#ifdef __CA__
    /* set SSL cipher type */
    SSL_CTX_set_cipher_list(ctx, ALGO_TYPE);
#endif
}


int fsock_OpensslLibShowCerts(SSL* ssl)
{
    X509 *cert;
    char *line;

    //클라이언트 인증서를 받음
    cert = SSL_get_peer_certificate(ssl); /* Get certificates (if available) */
    if ( cert != NULL )
    {
        WRITE_INFO(CATEGORY_INFO,"Client certificates" );

        line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
        WRITE_INFO(CATEGORY_INFO,"Subject: %s " , line);
        if(line != NULL)
            free(line);

        line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
        WRITE_INFO(CATEGORY_INFO,"Issuer: %s " , line);
        if(line != NULL)
            free(line);

        X509_free(cert);
        return 0;
    }
    else
    {
        //printf("No certificates.");
        //LogRet("No certificates\n");
        return -1;
    }


    return 0;
}



int	fsock_SendAck(int sock, char *p_msgType, int p_msgCode, int p_retryCnt, int retrySleep)
{
    int		    rxt;
    int			retryCnt;
    CRhead		SHead;
    char szEncHeaderbuffer[32] = {0x00,};
    int encrypto_size = 0;

    memset(&SHead, 0x00, sizeof(SHead));

    strcpy(SHead.msgtype, p_msgType);
    SHead.msgcode  = htons(p_msgCode);
    SHead.msgleng = htonl(2);

    encrypto_size = fsec_Encrypt((char*)&SHead, sizeof(SHead), ENC_AESKEY, ENC_IV, szEncHeaderbuffer);
    rxt = fsock_Send(sock, (char *)szEncHeaderbuffer, encrypto_size);
    if(rxt <= 0)
    {
        WRITE_WARNING(CATEGORY_DEBUG, "Retry, because fail in send, errno(%d)code(%d) ",
                      errno,p_msgCode);
        retryCnt = 0;
        while(retryCnt < p_retryCnt)
        {
            fcom_Msleep(retrySleep);
            rxt = fsock_Send(sock, (char *)szEncHeaderbuffer, encrypto_size);
            if(rxt <= 0)
            {
                retryCnt++;
            }
            else
            {
                break;
            }
        }
        if(retryCnt == p_retryCnt)
        {
            WRITE_CRITICAL(CATEGORY_DEBUG,"Fail in send, retry(%d)errno(%d)code(%d) ",
                           retryCnt,errno,p_msgCode);
            return -1;
        }
    }

    WRITE_INFO(CATEGORY_DEBUG,"[ACK]Send succeed, code(%d) Len (%d)", p_msgCode,encrypto_size);

    return 0;
}

int	fsock_SendAckJson(int sock,
                      char *p_msgType,
                      int p_msgCode,
                      char *p_jsonData,
                      int p_jsonSize,
                      char* p_logip,
                      int retryAckCount,
                      int retryAcksleep
)
{
    int		    rxt;
    int		    DataEncrypto_size;
    int		    HeaderEncrypto_size;
    int			retryCnt;
    char        szEncHeaderBuffer[32] = {0x00,};
    char		*data = NULL;
    char		*EncBuffer = NULL;

    CRhead		SHead;

    memset(&SHead,   0x00, sizeof(SHead));

    data = (char *)malloc(p_jsonSize);
    memset(data, 0x00, p_jsonSize);
    memcpy(data, (char *)p_jsonData, p_jsonSize);
    if(fcom_malloc((void**)&EncBuffer, p_jsonSize+4096) != 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"fcom_malloc Failed " );
        fcom_MallocFree((void**)&data);
        return (-1);
    }

    DataEncrypto_size = fsec_Encrypt((char*)data, p_jsonSize, ENC_AESKEY, ENC_IV, EncBuffer);

    strcpy(SHead.msgtype, p_msgType);
    SHead.msgcode  = htons(p_msgCode);
    SHead.msgleng = htonl(DataEncrypto_size + 2); //include msgCode size(2)

    HeaderEncrypto_size = fsec_Encrypt((char*)&SHead, sizeof(SHead), ENC_AESKEY, ENC_IV, szEncHeaderBuffer);

    rxt = fsock_Send(sock, (char *)szEncHeaderBuffer, HeaderEncrypto_size);
    if(rxt <= 0)
    {
        WRITE_CRITICAL_IP(p_logip,"[ACK] Retry, because fail in send, errno(%d)code(%d)",
                          errno,p_msgCode);
        retryCnt = 0;
        while(retryCnt < retryAckCount)
        {
            fcom_Msleep(retryAcksleep);
            rxt = fsock_Send(sock, szEncHeaderBuffer, HeaderEncrypto_size);
            if(rxt <= 0)
            {
                retryCnt++;
            }
            else
            {
                break;
            }
        }
        if(retryCnt == retryAckCount)
        {
            WRITE_CRITICAL_IP(p_logip,"[ACK] Fail in send, retry(%d)errno(%d)code(%d)",
                              retryCnt,errno,p_msgCode);
            fcom_MallocFree((void**)&data);
            fcom_MallocFree((void**)&EncBuffer);
            return -1;
        }
    }

    rxt = fsock_Send(sock, (char *)EncBuffer, DataEncrypto_size);
    if(rxt <= 0)
    {
        WRITE_CRITICAL_IP(p_logip,"[ACK] Retry, because fail in send, errno(%d)code(%d)",
                          errno,p_msgCode);
        retryCnt = 0;
        while(retryCnt < retryAckCount)
        {
            fcom_Msleep(retryAcksleep);
            rxt = fsock_Send(sock, EncBuffer, DataEncrypto_size);
            if(rxt <= 0)
            {
                retryCnt++;
            }
            else
            {
                break;
            }
        }
        if(retryCnt == retryAckCount)
        {
            WRITE_CRITICAL_IP(p_logip,"[ACK] Fail in send, retry(%d)errno(%d)code(%d)",
                              retryCnt,errno,p_msgCode);
            fcom_MallocFree((void**)&data);
            fcom_MallocFree((void**)&EncBuffer);
            return -1;
        }
    }

    fcom_MallocFree((void**)&data);
    fcom_MallocFree((void**)&EncBuffer);

    WRITE_INFO_IP(p_logip,"[ACK] Succeed in send, code(%d) len (%d) ", p_msgCode,rxt);
    return 0;
}

int fsock_PreAuth(SSL *ssl, int csock, char* cpip, char* mType, int* mCode, int* mLeng, int retry_cnt, int retry_sleep)
{
    int		rxt;
    int		msgLeng = 0;
    int		msgCode = 0;
    char	msgType[10+1] = {0x00,};
    char    local_buff[12+1] = {0x00,};
    CRhead	RHead;

    memset((void *)&RHead, 0x00, sizeof(RHead));

    /* SSL Hand Shake Auth Retry */
    SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);

    rxt = fsock_SslSocketRecv(ssl, (char *)&RHead, sizeof(RHead));
    if (rxt <= 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"fsock_PreAuth return %d ", rxt );
        return -1;
    }

    if (rxt != sizeof(RHead))
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"fsock_PreAuth Failed " );
        return -2;
    }

    memset( msgType    , 0x00, sizeof(msgType));
    memset( local_buff , 0x00, sizeof(local_buff));

    strncpy(local_buff, RHead.msgtype,10);
    memcpy(msgType, local_buff ,strlen(local_buff) );

    msgLeng = ntohl(RHead.msgleng);
    if( msgLeng < 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"fsock_PreAuth Header Msg Length < 0 ");
    }

    msgCode = ntohs(RHead.msgcode);
    if( msgCode < 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"fsock_PreAuth Header Msg Code Value < 0" );
    }

    WRITE_INFO_IP(cpip, "Receive ip(%s)type(%s)leng(%d)code(%d),sock(%d)",
                  cpip,msgType,msgLeng,msgCode,csock);

    if(	memcmp(RHead.msgtype, DAP_AGENT, 10) != 0 &&
           memcmp(RHead.msgtype, DAP_MANAGE, 10) != 0)
    {
        if(	strncmp(msgType, DAP_AGENT, 10) == 0)
        {
            rxt = fsock_SendAck(ssl, msgType, DATACODE_RTN_FAIL, retry_cnt, retry_sleep);
        }
        else if(strncmp(msgType, DAP_MANAGE, 10) != 0)
        {
            rxt = fsock_SendAck(ssl, msgType, MANAGE_RTN_FAIL, retry_cnt, retry_sleep);
        }

        if(rxt < 0)
        {
            WRITE_CRITICAL_IP(cpip, "Fail in send socket, error(%s)cpip(%s)",
                              strerror(errno),cpip);
            return -3;
        }
        WRITE_CRITICAL_IP(cpip, "Fail in recv, type(%s)cpip(%s)",
                          msgType,cpip);

        return -4;
    }

    if(msgCode == DATACODE_RTN_SUCCESS
       || msgCode == DATACODE_RTN_FAIL
       || msgCode == MANAGE_RTN_SUCCESS
       || msgCode == MANAGE_RTN_FAIL
       || msgCode == MANAGE_PING)
    {
        WRITE_INFO_IP(cpip, "Receive return code(%d)", msgCode);
        return -5;
    }

    strcpy(mType, msgType);
    *mCode = msgCode;
    *mLeng = msgLeng;

    //WRITE_INFO_IP(cpip, "Result sock(%d)mType(%s)mCode(%d)mLeng(%d)",
    //			csock,mType,*mCode,*mLeng);

    return 0;
}