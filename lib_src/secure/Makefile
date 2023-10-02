include $(HOME)/workspace/dap/Make.cf
LDFLAGS = ${LIBDIR} -lssl
CFLAGS  = $(INCDIR) 

OBJS	= dap_aes.o \
	  dap_crypto.o \
	  dap_SslHelper.o \
	  dap_blowfish.o \
	  crypt_blowfish.o

TARGET  = libsecure.a

.c.o:
	$(CC) -c -Wall -Wno-pointer-sign $(CFLAGS) $(ORA_INC)  $<

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
