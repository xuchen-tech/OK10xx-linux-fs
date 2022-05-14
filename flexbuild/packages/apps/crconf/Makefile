DESTDIR?=/usr
ROOTDIR=$(DESTDIR)
LIBDIR=/usr/lib
SBINDIR=/sbin
MANDIR=/share/man

CC ?= gcc
HOSTCC ?= gcc
CCOPTS = -D_GNU_SOURCE -O2 -Wstrict-prototypes -Wall
CFLAGS = $(CCOPTS) -I../include

LDLIBS += -L../lib -lnetlink

SUBDIRS=lib src

LIBNETLINK=../lib/libnetlink.a 

all:
	@set -e; \
	for i in $(SUBDIRS); \
	do $(MAKE) -C $$i; done


install: all
	install -m 0755 -d $(DESTDIR)$(SBINDIR)
	@for i in $(SUBDIRS); do $(MAKE) -C $$i install; done
	install -m 0755 -d $(DESTDIR)$(MANDIR)/man8
	install -m 0644 $(shell find man/man8 -maxdepth 1 -type f) $(DESTDIR)$(MANDIR)/man8

clean:
	@for i in $(SUBDIRS); \
	do $(MAKE) -C $$i clean; done

.EXPORT_ALL_VARIABLES:
