include $(HOME)/workspace/dap/Make.cf

LDFLAGS = ${LIBDIR}
CFLAGS  = $(INCDIR) $(MYSQL_INC)

OBJS	= dap_checkdb.o \
	  dap_mysql.o \
	  dap_trandb.o 

TARGET  = libdb.a

.c.o:
	$(CC) -c  $(CFLAGS) -W -Wall $(ORA_INC)  $<

all:    $(TARGET) install


$(TARGET) :  $(OBJS)
	$(RM)   $@
	ar rcv $@ $(OBJS)

touch:
	$(TOUCH) *.c

clean:
	${RM} ${TARGET} $(EXTLIB) *.o core .make.state afiedt.buf
	${RM} $(DAP_HOME)/lib/${TARGET}

install:
	${CP} ${TARGET} $(DAP_HOME)/lib

delobj :
	${RM} ${TARGET}
