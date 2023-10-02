include $(HOME)/workspace/dap/Make.cf
LDFLAGS	= ${LIBDIR}
CFLAGS	= $(INCDIR)

OBJS	= dap_config.o \
	  dap_log.o \
	  dap_string.o \
	  dap_time.o \
	  dap_util.o \
	  dap_report.o \
	  dap_pidfile.o \
	  dap_update_cfg.o

TARGET	= libcom.a

.c.o:
	$(CC) -c -W -Wall -Wno-sign-compare -Wno-pointer-sign $(CFLAGS)  $<

all:	$(TARGET) install


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


