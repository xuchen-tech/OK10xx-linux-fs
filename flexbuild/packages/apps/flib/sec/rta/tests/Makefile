# Copyright 2008-2013 Freescale Semiconductor, Inc.

CC=$(CROSS_COMPILE)gcc
INCLUDES=-I../include
CFLAGS=-Wall -Wshadow -g -O0 $(INCLUDES)
CPPFLAGS=-DRTA_DEBUG
check: CPPFLAGS=-DSUPPRESS_PRINTS

ALL_SRCS=$(wildcard *.c)
LIB_SRCS=test_common.c
LIB_OBJS=$(LIB_SRCS:.c=.o)
LIBS=$(LIB_SRCS:.c=)
TEST_SRCS=$(filter-out $(LIB_SRCS), $(ALL_SRCS))
TEST_OBJS=$(TEST_SRCS:.c=.o)
TESTS=$(TEST_SRCS:.c=)
DEPENDS=$(ALL_SRCS:.c=.d)
LOGS=$(ALL_SRCS:.c=.desc)

# List tests that do not have a golden pattern yet
NO_CHECK_TESTS=capwap no_shift_kasumi_crc_init_var_stim perf_test ref_example test_jump test_key test_seq_ptr
CHECK_TESTS=$(filter-out $(NO_CHECK_TESTS), $(TESTS))

.PHONY: all check clean

all: $(TESTS)

# Build dependencies (*.d files) for source (*.c) files.
# Use the "-MM" compiler flag (ignores system headers).
%.d: %.c
	@set -e; rm -f $@; \
	$(CC) -MM $(CPPFLAGS) $(CFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

# No need to recreate *.d files when cleaning up.
# Also, when building only one test, create dependencies only for that test.
ifneq ($(MAKECMDGOALS), clean)
ifneq (,$(filter $(MAKECMDGOALS), all check))
-include $(DEPENDS)
else
-include $(MAKECMDGOALS).d $(LIB_SRCS:.c=.d)
endif
endif

# Check tests against DXLT golden patterns (dxlt/*.desc).
# Save the logs in *.desc files for tests that fail.
check: $(CHECK_TESTS)
	@echo Checking tests...
	@total=0; \
	passed=0; \
	failed=0; \
	for test in $^; do \
		let total++; \
		./$$test > $$test.desc; \
		if diff $$test.desc dxlt/$$test.desc > /dev/null 2>&1; then \
			rm $$test.desc; \
			let passed++; \
		else \
			echo "$$test failed"; \
			let failed++; \
		fi; \
	done; \
	echo "Done!"; \
	echo "Total  = $$total"; \
	echo "Passed = $$passed"; \
	echo "Failed = $$failed"

$(TESTS): % : %.o $(LIB_OBJS)
	$(CC) -o $@ $^

# We don't want to automatically build the libraries (.o -> elf).
# Thus we have to cancel implicit build rules by overriding them
# using an empty recipe.
$(LIBS): $(LIB_OBJS)
	

clean:
	@rm -f $(TESTS) $(TEST_OBJS) $(LIB_OBJS) $(DEPENDS) $(LOGS)

