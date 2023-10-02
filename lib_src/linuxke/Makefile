include $(HOME)/workspace/dap/Make.cf
LDFLAGS = ${LIBDIR}
CFLAGS  = $(INCDIR)

OBJS	= dap_linux.o \
	  

TARGET  = libke.a

.c.o:
	$(CC) -c -W -Wall $(CFLAGS) $(ORA_INC)  $<

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
