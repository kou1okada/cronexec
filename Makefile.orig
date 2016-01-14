
PREFIX=/usr/local/bin
CC=gcc

all : cronexec schedexec

cronexec : cronexec.c Makefile
	$(CC) $(COPT) -o cronexec cronexec.c
	strip cronexec

schedexec : schedexec.c crontab.c crontab.h wait_time.c Makefile
	$(CC) $(COPT) -o schedexec schedexec.c crontab.c wait_time.c
	strip schedexec

install:
	mkdir -p $(PREFIX)
	cp cronexec $(PREFIX)/cronexec
	cp schedexec $(PREFIX)/schedexec

