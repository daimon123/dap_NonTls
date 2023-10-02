include $(HOME)/workspace/dap/Make.cf

RM	= \rm -f
CP	= cp

TARGET	= dap_dblog

INCLUDE	= $(INCDIR) $(MYSQL_INC)
LIBRARY	= $(LIBDIR) $(MYSQL_LIB) -ldb -lcom -lke -lsock -lsecure -lipc -lpthread -lcrypto -lm -lmysqlclient_r -ldl
CFLAGS	= ${CCOPTIONS} -DDEBUG

OBJS	= dblog_main.o \

.SUFFIXES : .c .o .h

.c.o:
	sh /home/intent/workspace/dap/proc_inc/make_current_date.sh
	$(CC) -c -W -Wall -Wno-unused-parameter $(CFLAGS) $(INCLUDE) $<

all:	$(TARGET) install

$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(LIBRARY)

clean:
	${RM} $(TARGET) *.o core

install:
	${CP} $(TARGET) ../../bin

