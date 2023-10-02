include $(HOME)/workspace/dap/Make.cf

RM	= \rm -f
CP	= cp

TARGET1	= dap_connect
TARGET2	= dap_stress
TARGET3 = dap_convert_data
TARGET4 = dap_stress_update
TARGET5 = syslog_mon
TARGET6 = listen_test

INCLUDE	= $(INCDIR) $(MYSQL_INC)

LIBRARY1 = $(LIBDIR) $(MYSQL_LIB) -lcom -ldb -lke -lsecure -lipc -ljson -lsock -lpthread -lcrypto -lm -lssl -ljansson -lmysqlclient_r
LIBRARY2 = $(LIBDIR) $(MYSQL_LIB) -lcom -ldb -lke -lsecure -lipc -ljson -lsock -lpthread -lcrypto -lm -lssl -ljansson -lmysqlclient_r
LIBRARY3 = $(LIBDIR) $(MYSQL_LIB) -lcom -ldb -lke -lsecure -lipc -ljson -lsock -lpthread -lcrypto -lm -lssl -ljansson -lmysqlclient_r
LIBRARY4 = $(LIBDIR) $(MYSQL_LIB) -lcom -ldb -lke -lsecure -lipc -ljson -lsock -lpthread -lcrypto -lm -lssl -ljansson -lmysqlclient_r
LIBRARY5 = $(LIBDIR) $(MYSQL_LIB) -lcom -ldb -lke -lsecure -lipc -ljson -lsock -lpthread -lcrypto -lm -lssl -lmysqlclient_r 
LIBRARY6 = $(LIBDIR) $(MYSQL_LIB) -lcom -ldb -lke -lsecure -lipc -ljson -lsock -lpthread -lcrypto -lm -lssl -lmysqlclient_r 
LIBRARY6 = $(LIBDIR) $(MYSQL_LIB) -lsock -lke -ljson -lcom -lipc -ldb -lpthread -lsecure -lcrypto -lm -lssl -lmysqlclient_r

CFLAGS	= ${CCOPTIONS} -DDEBUG

OBJ1	= util_conn_main.o \
	  util_connect.o

OBJ2	= util_stress_main.o \
	  util_connect.o

OBJ3	= convert_data_main.o

OBJ4	= update_stress_main.o 

OBJ5	= syslog_main.o

OBJ6	= listen_test.o

.SUFFIXES : .c .o .h

.c.o:
	$(CC) -c -W -Wall -Wno-pointer-sign -Wno-sign-compare -Wno-unused-parameter -D_GNU_SOURCE $(CFLAGS) $(INCLUDE) $<

all: $(TARGET1) $(TARGET2) $(TARGET3) $(TARGET4) $(TARGET5) $(TARGET6)  install

$(TARGET1): $(OBJ1)
	$(CC) -o $(TARGET1) $(OBJ1) $(LIBRARY1)

$(TARGET2): $(OBJ2)
	$(CC) -o $(TARGET2) $(OBJ2) $(LIBRARY2)

$(TARGET3): $(OBJ3)
	$(CC) -o $(TARGET3) $(OBJ3) $(LIBRARY3)

$(TARGET4): $(OBJ4)
	$(CC) -o $(TARGET4) $(OBJ4) $(LIBRARY4)

$(TARGET5): $(OBJ5)
	$(CC) -o $(TARGET5) $(OBJ5) $(LIBRARY5)

$(TARGET6): $(OBJ6)
	$(CC) -o $(TARGET6) $(OBJ6) $(LIBRARY6)

clean:
	${RM} $(TARGET1) *.o core
	${RM} $(TARGET2) *.o core
	${RM} $(TARGET3) *.o core
	${RM} $(TARGET4) *.o core
	${RM} $(TARGET5) *.o core
	${RM} $(TARGET6) *.o core

install:
	${CP} $(TARGET1) ../../bin
	${CP} $(TARGET2) ../../bin
	${CP} $(TARGET3) ../../bin
	${CP} $(TARGET4) ../../bin
	${CP} $(TARGET5) ../../bin
	${CP} $(TARGET6) ../../bin
	${CP} $(TARGET5) ../../bin
	${CP} $(TARGET6) ../../bin
