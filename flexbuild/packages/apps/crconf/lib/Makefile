CFLAGS += -fPIC

NLOBJ= libnetlink.o

all: libnetlink.a

libnetlink.a: $(NLOBJ)
	$(AR) rcs $@ $(NLOBJ)

install:

clean:
	rm -f $(NLOBJ) libnetlink.a

