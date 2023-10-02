include $(HOME)/dap/cfg/Make.cf
.KEEP_STATE:

LDFLAGS     = 	${LIBDIR}  $(ISAMLIB) -lkstat -lm -lpthread
CFLAGS      =	$(INCDIR)

OBJS        =  squeue.o rqueue.o fqueue.o
TARGET		= libgpipe.a

.c.o:
	$(CC) -c  $(CFLAGS) $<

#all:	$(TARGET) $(EXTLIB) install
all:	$(TARGET) install


$(TARGET) :  $(OBJS)
	$(RM)   $@
	ar rcv $@ $(OBJS)

$(EXTLIB) :  $(EXTOBJS)
	$(LD) -o $@ $(ORA815_SHARED_LDFLAG) $(EXTOBJS) $(LDFLAGS)

touch:
	$(TOUCH) *.c

clean:
	${RM} ${TARGET} $(EXTLIB) *.o core .make.state afiedt.buf

install:
	${CP} ${TARGET} $(DAP_HOME)/lib
#	${CP} ${EXTLIB} $(HOME)/lib

delobj :
	${RM} ${TARGET}

