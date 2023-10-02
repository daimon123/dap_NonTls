include $(HOME)/workspace/dap/Make.cf

RM	= \rm -f
CP	= cp

TARGET	= dap_alarm

INCLUDE	= $(INCDIR) $(MYSQL_INC)
LIBRARY= $(LIBDIR) $(MYSQL_LIB) -lcom -ldb -lke -lsecure -lsock -lipc -lpthread -lcrypto -lm -lmysqlclient_r -ldl
CFLAGS	= ${CCOPTIONS} -DDEBUG

OBJS	= alarm_main.o \
	  alarm_db.o \

.SUFFIXES : .c .o .h

.c.o:
	sh /home/intent/workspace/dap/proc_inc/make_current_date.sh
	$(CC) -c -Wall -Wno-pointer-sign $(CFLAGS) $(INCLUDE) $<

all:	$(TARGET) install

$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(LIBRARY)

clean:
	${RM} $(TARGET) *.o core

install:
	${CP} $(TARGET) ../../bin

