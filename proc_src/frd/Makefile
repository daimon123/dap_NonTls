include $(HOME)/workspace/dap/Make.cf

RM	= \rm -f
CP	= cp

TARGET	= dap_frd

INCLUDE	= $(INCDIR) $(MYSQL_INC)
LIBRARY	= $(LIBDIR) $(MYSQL_LIB) -ldb -lcom -lke -lsecure -lipc -lsock -lpthread -lcrypto -ljson -lssl -lm -lmysqlclient -ldl
CFLAGS	= ${CCOPTIONS} -DDEBUG

OBJS	= frd_fork.o \
	  frd_fq.o \
	  frd_insert.o \
	  frd_main.o \
	  frd_main_task.o \
	  frd_select.o \
	  frd_task.o \
	  frd_update.o \
	  frd_util.o \
	  frd_file.o \
	  frd_init.o \
	  frd_signal.o \
	  frd_dump.o \
	  frd_syslog.o
	

.SUFFIXES : .c .o .h

.c.o:
	$(CC) -c -W -Wall -Wno-pointer-sign -Wno-unused-parameter -Wno-format-contains-nul $(CFLAGS) $(INCLUDE) $<

all:	$(TARGET) install

$(TARGET): $(OBJS)
	sh /home/intent/workspace/dap/proc_inc/make_current_date.sh
	$(CC) -o $(TARGET) $(OBJS) $(LIBRARY)

clean:
	${RM} $(TARGET) *.o core

install:
	${CP} $(TARGET) ../../bin/
