/* ---------------------------------------------------------------------------
** This software is in the public domain, furnished "as is", without technical
** support, and with no warranty, express or implied, as to its usefulness for
** any purpose. You are free to modify it and use it in any way you want,
** but you have to leave this header intact. Also if you find it useful,  
** a comment about it on blog.toshsoft.de would be nice.
**               
**
** SSLHelper.h
** A Helper class to get the SSL connection up and running
**
** Author: Oliver Pahl
** -------------------------------------------------------------------------*/

#ifndef __SSL_HELPER_H
#define __SSL_HELPER_H

#include <netdb.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
 
#include <openssl/crypto.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

//#define __CA__
#define PRIKEY_PASSWD	"dap_server"						//prikey password
#define ALGO_TYPE		"RC4-MD5"						//algorithm type
//#define SERVER_CERT		"certs/server/server.crt"		//server cert file
//#define SERVER_KEYF		"certs/server/server.key"		//server key file
//#define CLIENT_CERT		"certs/client/client.crt"		//client cert file
//#define CLIENT_KEYF		"certs/client/client.key"		//client key file
//#define ROOTCERTF		"certs/root/root.crt"			//root cert file

typedef struct {
    /* SSL Vars */
    SSL_CTX         *ctx;
    SSL             *ssl;
    SSL_METHOD      *meth;
    X509            *server_cert;
    EVP_PKEY        *pkey;

    /* Socket Communications */
    struct sockaddr_in   server_addr;
    struct hostent      *host_info;
    int                  sock;
} SSL_Connection;






void thread_setup(void);
void thread_cleanup(void);

/* Prototypes */
SSL_Connection* ssl_connect(const char *host, int port, const char *certfile, const char *keyfile);
SSL_Connection* ssl_non_connect(const char *host, int port, const char *certfile, const char *keyfile);
void ssl_free(SSL_Connection *sslcon);
void ssl_disconnect(SSL_Connection *sslcon);

#endif
