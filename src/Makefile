# this makefile is intended for g++ on Linux

CC = g++
CFLAGS = -c -Wall -O3 -Wno-psabi
# CFLAGS = -c
LDFLAGS = -pthread
INCLUDE = -I../include/
SOURCES = project.cpp \
		  iofunc.cpp \
		  filter.cpp \
		  fourier.cpp \
		  genfunc.cpp \
		  logfunc.cpp \
		  sampling.cpp \
		  rf_fe_block.cpp \
		  mono_block.cpp \
		  channel.cpp \
		  stereo_block.cpp \
		  rds_block.cpp \
		  performance.cpp

OBJECTS = $(SOURCES:.cpp=.o)
EXECUTABLE = project

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

.cpp.o:
	$(CC) $(CFLAGS) $(INCLUDE) $< -o $@

clean:
	-rm $(OBJECTS)
