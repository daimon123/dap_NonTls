DIRS = com ipc db secure linuxke sock json


all:
	@for d in ${DIRS}; do \
	( \
		cd $$d; \
		${MAKE} all; \
		/bin/echo " " \
	); done

install:
	@for d in ${DIRS}; do \
	( \
		cd $$d; \
		${MAKE} install; \
		/bin/echo " " \
	); done


clean:
	@for d in ${DIRS}; do \
	( \
		cd $$d; \
		${MAKE} clean; \
		/bin/echo " " \
	); done
