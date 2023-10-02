DIRS = libzdb



all: install


install:
		@for d in ${DIRS}; do \
		( /bin/echo "<< Change directory: `pwd`/$$d >>"; \
			cd $$d; \
			rm -rf ~/lib/dbpool; \
			cp -R lib ~/lib/dbpool; \
			/bin/echo "cp -R lib ~/lib/dbpool"; \
			rm -rf ~/include/dbpool; \
			cp -R include/zdb ~/include/dbpool; \
			/bin/echo "cp -R include/zdb ~/include/dbpool"; \
			/bin/echo " " \
		); done

