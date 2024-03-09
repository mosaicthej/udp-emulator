.PHONY: clean all

CC = gcc
XCC = gcc-10
CFLAGS = -g
CPPFLAGS = -std=gnu90 -Wall -Wextra -pedantic
ARCHIVE = ar -r -c -s

#LISTS = list_adders list_movers list_removers
# use the minimal version for now. (listmin.h)

all: partA-endpoint testlist

partA-endpoint: partA-endpoint.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $<

testlist: testlist.o liblistmin.a
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $< -L. -llistmin

testlist.o: testlist.c list.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ -c testlist.c -I.


# liblist.a: list.h $(LISTS:=.o)
#	$(ARCHIVE) liblist.a $(LISTS:=.o)

# $(LISTS:=.o): list.h $(LISTS:=.c)
# 	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ -c $(@:.o=.c) -I.

# above took care of the all list_*.o files, each recepie is the same

#list_adders.o: list.h list_adders.c
#	$(CC) $(CFLAGS) $(CPPFLAGS) -o list_adders.o -c list_adders.c -I.

# using the minimal version of list library. (listmin.c)
liblistmin.a: listmin.o listmin.h
	$(ARCHIVE) $@ $<

listmin.o: listmin.c listmin.h 
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ -c $< -I.



clean:
	rm -f *.o *.a partA-endpoint testlist
