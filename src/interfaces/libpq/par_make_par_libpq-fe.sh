#!/bin/sh
set -ex
gcc -c -o par_libpq-fe.o par_libpq-fe.c -I ../../include
gcc -c -o par_config.o par_config.c -I ../../include
ar rcs libparq.a par_libpq-fe.o par_config.o \
	fe-auth.o fe-connect.o fe-exec.o fe-misc.o fe-print.o fe-lobj.o \
	fe-protocol2.o fe-protocol3.o pqexpbuffer.o pqsignal.o fe-secure.o \
	libpq-events.o md5.o ip.o wchar.o encnames.o noblock.o pgstrcasecmp.o \
	thread.o strlcpy.o
ranlib libparq.a
