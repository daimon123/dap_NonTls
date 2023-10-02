include $(HOME)/workspace/dap/Make.cf

RM	= \rm -f
CP	= cp

TARGET	= dap_pcif

INCLUDE	= $(INCDIR) $(MYSQL_INC)
#LIBRARY	= $(LIBDIR) $(MYSQL_LIB) -lcom -ldb -lke -lsecure -lipc -ljson -lsock -lpthread -lcrypto -lm -lssl -ljansson -lmysqlclient
#LIBRARY	= $(LIBDIR) $(MYSQL_LIB) -lssl -lcrypto -lcom -ldb -lke -lsecure -lipc -ljson -lsock -lpthread -lm -ljansson -lmysqlclient_r -ldl
#LIBRARY	= $(LIBDIR) $(MYSQL_LIB) -lssl -lcrypto -lcom -ldb -lke -lsecure -lipc -ljson -lsock -lcrypto -lssl -lpthread -lm -ljansson -lmysqlclient_r -ldl
LIBRARY	= $(LIBDIR) $(MYSQL_LIB) -lssl -lcrypto -lcom -ldb -lke -lsecure -lipc -ljson -lsock -lssl -lcrypto -lpthread -lm -ljansson -lmysqlclient_r -ldl
CFLAGS	= ${CCOPTIONS} -DDEBUG

OBJS	= pcif_main.o \
	  pcif_util.o \
	  pcif_db.o \
	  pcif_socket.o \
	  pcif_jason.o \
	  pcif_parse.o \
	  pcif_log.o \
	  pcif_fork.o \
	  pcif_init.o \
	  pcif_serverlog.o \
	  pcif_signal.o \
	  pcif_thread_work.o \
	  pcif_idx.o \
	  pcif_dbfile.o 

.SUFFIXES : .c .o .h

.c.o:
	sh /home/intent/workspace/dap/proc_inc/make_current_date.sh
	$(CC) -c -W -Wall -Wno-pointer-sign -Wno-sign-compare -Wno-unused-parameter -D_GNU_SOURCE $(CFLAGS) $(INCLUDE) $<

all:	$(TARGET) install

$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(LIBRARY)

clean:
	${RM} $(TARGET) *.o core

install:
	${CP} $(TARGET) ../../bin

