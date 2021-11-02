CC=gcc
GG=g++

SG_MONITOR_LIB = libsgm_monitor.so
SG_MONITOR_TEST = sgm_monitor

SG_HEADER_FILES = $(wildcard *.h)

SG_MONITOR_TEST_SOURCES = main.c

SG_MONITOR_TEST_LDFLAGS = '-Wl,-rpath=$$ORIGIN'

all: $(SG_MONITOR_TEST)
	@cp lib/$(SG_MONITOR_LIB) .

$(SG_MONITOR_TEST): $(SG_MONITOR_TEST_SOURCES) lib/$(SG_MONITOR_LIB) $(SG_HEADER_FILES)
	$(CC) -std=gnu99 -o $@ -g -O0 -I/usr/local/include/libdrm/ -I/usr/include/libdrm $(SG_MONITOR_TEST_SOURCES) lib/$(SG_MONITOR_LIB) $(SG_MONITOR_TEST_LDFLAGS) -lm -lpthread


clean:
	@rm -f $(SG_MONITOR_TEST)
	@rm -f $(SG_MONITOR_LIB)

.PHONY: clean all
