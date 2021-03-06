#!/usr/bin/make -f
# crazy level of warnings
CFLAGS:=-Wall -Wextra -Wconversion -Wuninitialized -Wshadow -Wstrict-prototypes -fstrict-aliasing -Wstrict-aliasing -Wpointer-arith -Wcast-align -Wstrict-prototypes -Wold-style-definition -Wmissing-prototypes -Wmissing-declarations -Wredundant-decls -Wnested-externs -std=c99 -pedantic
# debug 
CFLAGS+=-g -pg
# optimize
CFLAGS+=-O1
# turn on POSIX/susv3 
CPPFLAGS:=-D_XOPEN_SOURCE=600

OBJS:=attr.o cgi.o escape.o mapfile.o template.o sha1passwd.o ini.o keygen.o base64.o
libcgi.a : libcgi.a($(OBJS))

testcgi : testcgi.c libcgi.a

.PHONY : clean clean-all

clean : 
	$(RM) $(OBJS) libcgi.a testcgi

clean-all : clean
	$(RM) *~
