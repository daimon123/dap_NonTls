include $(HOME)/workspace/dap/Make.cf
LDFLAGS = ${LIBDIR} -ldb
CFLAGS  = $(INCDIR)

OBJS	= dap_json.o \
	 dap_json_parse.o

TARGET  = libjson.a

.c.o:
	$(CC) -c  $(CFLAGS) -W -Wall -Wno-pointer-sign $(ORA_INC) $< -ldb

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
