include $(HOME)/workspace/dap/Make.cf

RM	= \rm -f
CP	= cp

TARGET	= dap_dbif

INCLUDE	= $(INCDIR) $(MYSQL_INC)
LIBRARY	= $(LIBDIR) $(MYSQL_LIB) -ldb -lcom -lke -lsecure -lsock -lipc -lpthread -lcrypto -ljson -lssl -lm -lmysqlclient
CFLAGS	= ${CCOPTIONS} -DDEBUG

OBJS	= dbif_main.o \
	  dbif_task.o \
	  dbif_fq.o \
	  dbif_insert.o \
	  dbif_update.o \
	  dbif_select.o \
          dbif_thread.o \
          dbif_util.o

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
	${CP} $(TARGET) ../../bin/dap_dbif01
	${CP} $(TARGET) ../../bin/dap_dbif02
	${CP} $(TARGET) ../../bin/dap_dbif03
	${CP} $(TARGET) ../../bin/dap_dbif04
	${CP} $(TARGET) ../../bin/dap_dbif05
	${CP} $(TARGET) ../../bin/dap_dbif06
	${CP} $(TARGET) ../../bin/dap_dbif07
	${CP} $(TARGET) ../../bin/dap_dbif08
	${CP} $(TARGET) ../../bin/dap_dbif09
	${CP} $(TARGET) ../../bin/dap_dbif10

