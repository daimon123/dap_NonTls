include $(HOME)/dap/cfg/Make.cf
.KEEP_STATE:

LDFLAGS     = -L${LIBDIR} 
CFLAGS      =$(INCDIR) $(MYSQL_INC)

OBJS        =  mysqlDB.o


TARGET		= libmysqldb.a

.c.o:
	$(CC) -c  $(CFLAGS) $(INCLUDE)  $<

all:	$(TARGET) install


$(TARGET) :  $(OBJS)
	$(RM)   $@
	ar rcv $@ $(OBJS)

touch:
	$(TOUCH) *.c

clean:
	${RM} ${TARGET} *.o core

install:
	${CP} ${TARGET} $(DAP_HOME)/lib

delobj :
	${RM} ${TARGET}


# Dependency
$(OBJS): $(COMINCDIR)/mysqldb/*.h
