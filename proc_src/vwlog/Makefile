include $(HOME)/workspace/dap/Make.cf

RM	= \rm -f
CP	= cp

TARGET1	= dap_vwlog

TARGET2 = dap_tail

INCLUDE	= $(INCDIR) $(MYSQL_INC)
LIBRARY1 = $(LIBDIR) $(MYSQL_LIB) -lssl -lcrypto -lcom -lipc -ldb -lke -lsecure -lsock -lpthread -lcrypto -lssl -lm -lmysqlclient_r -ldl -lssl -lcrypto
#LIBRARY1 = $(LIBDIR) $(MYSQL_LIB) -lc -ldb -lcom -ldl -lke -lsecure -lsock -lipc -lpthread -lcrypto -lm -lssl  -lmysqlclient_r -ldl
LIBRARY2 = $(LIBDIR) $(MYSQL_LIB) -lssl -lcrypto -lcom -lipc -ldb -lke -lsecure -lsock -lpthread -lcrypto -lssl -ljansson -lm -lmysqlclient_r -ldl -lssl -lcrypto
#LIBRARY2 = $(LIBDIR) $(MYSQL_LIB) -lc -ldb -lcom -ldl -lke -lsecure -lsock -lipc -lpthread -lcrypto -lm -lssl  -lmysqlclient_r -ldl
CFLAGS	= ${CCOPTIONS} -DDEBUG

OBJ1	= vwlog_main.o 

OBJ2    = tail_main.o

.SUFFIXES : .c .o .h

.c.o:
	$(CC) -c -W -Wall -Wno-unused-parameter $(CFLAGS) $(INCLUDE) $<

all:	$(TARGET1) $(TARGET2) install

$(TARGET1): $(OBJ1)
	$(CC) -o $(TARGET1) $(OBJ1) $(LIBRARY1)

$(TARGET2): $(OBJ2)
	$(CC) -o $(TARGET2) $(OBJ2) $(LIBRARY2)

clean:
	${RM} $(TARGET1) *.o core
	${RM} $(TARGET2) *.o core

install:
	${CP} $(TARGET1) ../../bin
	${CP} $(TARGET2) ../../bin
