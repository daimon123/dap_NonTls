#DIRS = master prmon sysman vwlog alarm dblog pcif proxy schd dbif report
DIRS = master prmon sysman vwlog alarm dblog pcif proxy schd report frd policy_fw


install:
	@for d in $(DIRS); do \
	( \
		cd $$d; \
		${MAKE} install; \
		/bin/echo " " \
	); done

all:
	@for d in $(DIRS); do \
	( \
		cd $$d; \
		${MAKE} all; \
		/bin/echo " " \
	); done

clean:
	@for d in ${DIRS}; do \
	(\
		cd $$d; \
		${MAKE} clean; \
		/bin/echo " " \
	); done
