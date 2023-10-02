include $(HOME)/workspace/dap/Make.cf

RM	= \rm -f
CP	= cp

TARGET	= dap_policy_fw

INCLUDE	= $(INCDIR) $(MYSQL_INC)
LIBRARY	= $(LIBDIR) $(MYSQL_LIB) -lcom -ldb -lke -lsecure -lipc -lsock -ljson -lsock -lpthread -lm -lcrypto -lmysqlclient_r -ldl
CFLAGS	= ${CCOPTIONS} -DDEBUG

OBJS	= fw_fork.o \
	  fw_init.o \
	  fw_main.o \
	  fw_main_task.o \
	  fw_util.o 

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

