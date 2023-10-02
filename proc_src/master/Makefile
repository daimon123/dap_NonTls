include $(HOME)/workspace/dap/Make.cf

RM	= \rm -f
CP	= cp

TARGET	= dap_master

INCLUDE	= $(INCDIR) $(MYSQL_INC)
LIBRARY	= $(LIBDIR) $(MYSQL_LIB) -lc -ldb -lcom -lke -lsecure -lsock -lipc -lpthread -lcrypto -lm -lssl  -lmysqlclient_r -ldl
CFLAGS	= ${CCOPTIONS} -DDEBUG

OBJS	= master_main.o \
	  master_process.o \
	  master_delete.o \
	  master_util.o \
	  master_signal.o

.SUFFIXES : .c .o .h

.c.o:
	sh /home/intent/workspace/dap/proc_inc/make_current_date.sh
	$(CC) -c -W -Wall -Wno-unused-function -Wno-pointer-sign -Wno-unused-parameter -Wno-return-local-addr$(CFLAGS) $(INCLUDE) $<

all:	$(TARGET) install

$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(LIBRARY)

clean:
	${RM} $(TARGET) *.o core

install:
	${CP} $(TARGET) ../../bin

