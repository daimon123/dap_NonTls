//
// Created by KimByoungGook on 2020-06-22.
//
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <openssl/ssl.h>
#include <sys/fcntl.h>
#include <errno.h>

#include "com/dap_com.h"
#include "secure/dap_secure.h"
#include "secure/dap_SSLHelper.h"

pthread_mutex_t smutex = PTHREAD_MUTEX_INITIALIZER;

static pthread_mutex_t *lock_cs;
static long *lock_count;

SSL_Connection *ssl_connect(const char *host, int port, const char *certfile, const char *keyfile)
{
    int err;

    SSL_Connection *sslcon = NULL;
    sslcon = (SSL_Connection *)malloc(sizeof(SSL_Connection));
    if(sslcon == NULL)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG, "Could not allocate memory for SSL Connection " );
        return NULL;
    }

    /* Load encryption & hashing algorithms for the SSL program */
    pthread_mutex_lock(&smutex);
    SSL_library_init();
    pthread_mutex_unlock(&smutex);

    /* Load the error strings for SSL & CRYPTO APIs */
    /*
    pthread_mutex_lock(&smutex);
    SSL_load_error_strings();
    pthread_mutex_unlock(&smutex);
    */

    /* Create an SSL_METHOD structure (choose an SSL/TLS protocol version) */
    sslcon->meth = (SSL_METHOD*)SSLv23_client_method();

    /* Create an SSL_CTX structure */
    sslcon->ctx = SSL_CTX_new(sslcon->meth);
    if(!sslcon->ctx)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Could not get SSL Context " );
        fcom_MallocFree((void**)&sslcon);
        return NULL;
    }

#ifdef __CA__
    /* set maximum depth for the certificate chain */
    //SSL_CTX_set_verify_depth(ctx, 1);
    /* set verify mode*/
    SSL_CTX_set_verify(sslcon->ctx, SSL_VERIFY_PEER, verify_callback);

	/* Load the CA from the Path */
	if(SSL_CTX_load_verify_locations(sslcon->ctx, cafile, certpath) <= 0)
	{
		// Handle failed load here
		//TLOG(smutex, "%s\n", "Failed to set CA location...");
		LogRet("%s\n", "Failed to set CA location...");
		ERR_print_errors_fp(stderr);
		return NULL;
	}
#endif

    /* Load the client certificate into the SSL_CTX structure */
    if (SSL_CTX_use_certificate_file(sslcon->ctx, certfile, SSL_FILETYPE_PEM) <= 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Cannot use Certificate File" );
        ERR_print_errors_fp(stderr);
        fcom_MallocFree((void**)&sslcon);
        return NULL;
    }

#ifdef __CA__
    /* set server private key password */
    SSL_CTX_set_default_passwd_cb_userdata(sslcon->ctx, (void*)PRIKEY_PASSWD);
#endif

    /* Load the private-key corresponding to the client certificate */
    if (SSL_CTX_use_PrivateKey_file(sslcon->ctx, keyfile, SSL_FILETYPE_PEM) <= 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Cannot use Private Key")
        ERR_print_errors_fp(stderr);
        fcom_MallocFree((void**)&sslcon);
        return NULL;
    }

    /* Check if the client certificate and private-key matches */
    if (!SSL_CTX_check_private_key(sslcon->ctx))
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Private key does not match the certificate public key");
        fcom_MallocFree((void**)&sslcon);
        return NULL;
    }

    /* Set up a TCP socket */
    sslcon->sock = socket (PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(sslcon->sock == -1)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Could not get Socket")
        fcom_MallocFree((void**)&sslcon);
        return NULL;
    }

    memset (&sslcon->server_addr, '\0', sizeof(sslcon->server_addr));
    sslcon->server_addr.sin_family      = AF_INET;
    sslcon->server_addr.sin_port        = htons(port);       /* Server Port number */
    sslcon->host_info = gethostbyname(host);
    if(sslcon->host_info)
    {
        /* Take the first IP */
        struct in_addr *address = (struct in_addr*)sslcon->host_info->h_addr_list[0];
        sslcon->server_addr.sin_addr.s_addr = inet_addr(inet_ntoa(*address)); /* Server IP */
    }
    else
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Could not resolve hostname %s",host );
        fcom_MallocFree((void**)&sslcon);
        return NULL;
    }

    /* Establish a TCP/IP connection to the SSL client */
    err = connect(sslcon->sock, (struct sockaddr*) &sslcon->server_addr, sizeof(sslcon->server_addr));
    if(err == -1)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Could not connect");
        fcom_MallocFree((void**)&sslcon);
        return NULL;
    }

    /* An SSL structure is created */
    sslcon->ssl = SSL_new(sslcon->ctx);
    if(!sslcon->ssl)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Could not get SSL Socket" );
        fcom_MallocFree((void**)&sslcon);
        return NULL;
    }

    /* Assign the socket into the SSL structure (SSL and socket without BIO) */
    SSL_set_fd(sslcon->ssl, sslcon->sock);

    /* Perform SSL Handshake on the SSL client */
    err = SSL_connect(sslcon->ssl);
    if(err <= 0)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Could not connect to SSL Server " );
        fcom_MallocFree((void**)&sslcon);
        return NULL;
    }

    return sslcon;
}



void ssl_disconnect(SSL_Connection *sslcon)
{
    int err;

    if(sslcon == NULL)
    {
        return;
    }

    /* Shutdown the client side of the SSL connection */
    err = SSL_shutdown(sslcon->ssl);
    if(err == -1)
    {
        WRITE_CRITICAL(CATEGORY_DEBUG,"Could not shutdown SSL" );
        return;
    }

    /* Terminate communication on a socket */
    err = close(sslcon->sock);
    if(err == -1)
    {
        return;
    }

    /* Free the SSL structure */
    SSL_free(sslcon->ssl);

    /* Free the SSL_CTX structure */
    SSL_CTX_free(sslcon->ctx);

    /* Free the sslcon */
    if(sslcon != NULL)
    {
        free(sslcon);
        sslcon = NULL;
    }

}

void ssl_free(SSL_Connection *sslcon)
{
    /* Free the sslcon */
    if(sslcon != NULL)
    {
        free(sslcon);
        sslcon = NULL;
    }
}

void pthreads_locking_callback(int mode, int type, char *file, int line)
{
# if 0
    fprintf(stderr, "thread=%4d mode=%s lock=%s %s:%d\n",
            CRYPTO_thread_id(),
            (mode & CRYPTO_LOCK) ? "l" : "u",
            (type & CRYPTO_READ) ? "r" : "w", file, line);
# endif
# if 0
    if (CRYPTO_LOCK_SSL_CERT == type)
        fprintf(stderr, "(t,m,f,l) %ld %d %s %d\n",
                CRYPTO_thread_id(), mode, file, line);
# endif
    if (mode & CRYPTO_LOCK)
    {
        pthread_mutex_lock(&(lock_cs[type]));
        lock_count[type]++;
    }
    else
    {
        pthread_mutex_unlock(&(lock_cs[type]));
    }
}


unsigned long pthreads_thread_id(void)
{
    unsigned long ret;

    ret = (unsigned long)pthread_self();
    return (ret);
}


void thread_setup(void)
{
    int i;

    lock_cs = OPENSSL_malloc(CRYPTO_num_locks() * sizeof(pthread_mutex_t));
    lock_count = OPENSSL_malloc(CRYPTO_num_locks() * sizeof(long));
    if (!lock_cs || !lock_count)
    {
        /* Nothing we can do about this...void function! */
        if (lock_cs)
            OPENSSL_free(lock_cs);
        if (lock_count)
            OPENSSL_free(lock_count);
        return;
    }

    for (i = 0; i < CRYPTO_num_locks(); i++)
    {
        lock_count[i] = 0;
        pthread_mutex_init(&(lock_cs[i]), NULL);
    }

    CRYPTO_set_id_callback((unsigned long (*)())pthreads_thread_id);
    CRYPTO_set_locking_callback((void (*)())pthreads_locking_callback);

}
void thread_cleanup(void)
{
    int i;

    CRYPTO_set_locking_callback(NULL);
    for (i = 0; i < CRYPTO_num_locks(); i++)
    {
        pthread_mutex_destroy(&(lock_cs[i]));
    }
    OPENSSL_free(lock_cs);
    OPENSSL_free(lock_count);

}

