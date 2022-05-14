ALLOBJ=crconf.o 

TARGETS=crconf

all: $(TARGETS) $(LIBS)

crconf: $(ALLOBJ) $(LIBNETLINK) 

install: all
	install -m 0755 $(TARGETS) $(DESTDIR)$(SBINDIR)

clean:
	rm -f $(ALLOBJ) $(TARGETS)

LDLIBS += -ldl
LDFLAGS += -Wl,-export-dynamic

