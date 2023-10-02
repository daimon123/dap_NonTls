include $(HOME)/workspace/dap/Make.cf

RM	= \rm -f
CP	= cp

TARGET1	= dap_schd
TARGET2	= dap_dir

INCLUDE	= $(INCDIR) $(MYSQL_INC)
LIBRARY1 = $(LIBDIR) $(MYSQL_LIB) -lcom -ldb -lke -lsecure -lipc -lsock -lpthread -lcrypto -lm -lmysqlclient_r -ldl
LIBRARY2 = $(LIBDIR) $(MYSQL_LIB) -lcom -ldb -lke -lsecure -lipc -lsock -lpthread -lcrypto -lm -lmysqlclient_r -ldl
CFLAGS	= ${CCOPTIONS} -DDEBUG

OBJS1	= schd_db.o \
	  schd_main.o
OBJS2	= schd_dir_main.o \
	  schd_dir_db.o \
	  schd_dir_ibk.o

.SUFFIXES : .c .o .h

.c.o:
	sh /home/intent/workspace/dap/proc_inc/make_current_date.sh
	$(CC) -c -W -Wall -Wno-unused-parameter $(CFLAGS) $(INCLUDE) $<

all:	$(TARGET1) $(TARGET2) install

$(TARGET1): $(OBJS1)
	$(CC) -o $(TARGET1) $(OBJS1) $(LIBRARY1)

$(TARGET2): $(OBJS2)
	$(CC) -o $(TARGET2) $(OBJS2) $(LIBRARY2)

clean:
	${RM} $(TARGET1) *.o core
	${RM} $(TARGET2) *.o core

install:
	${CP} $(TARGET1) ../../bin
	${CP} $(TARGET2) ../../bin
