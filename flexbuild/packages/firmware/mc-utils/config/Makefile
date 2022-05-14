SOURCES := $(shell find $(SOURCEDIR) -name '*.dts')
OBJECTS = $(SOURCES:.dts=.dtb)
DTC = dtc

all: $(OBJECTS)

%.dtb: %.dts
	$(DTC) -I dts -O dtb $< -o $@

clean:
	-rm -rf $(OBJECTS)

.PHONY: all clean
