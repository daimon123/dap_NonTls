include $(HOME)/workspace/dap/Make.cf
LDFLAGS = ${LIBDIR} -ldb
CFLAGS  = $(INCDIR)

OBJS	= dap_fQueue.o \
	  dap_sQueue.o \
	  dap_linker.o

TARGET  = libipc.a

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
