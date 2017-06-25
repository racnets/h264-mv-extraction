TARGET = h264-mv-extractor

C_SRCS = analyse.c decoder.c extract.c i2c.c logger.c main.c
C_EXT = c

libav = ../libav_custom

CC = gcc
CFLAGS = -Wall 
#CFLAGS +=-pedantic -O0 
CFLAGS += -std=c99
CFLAGS += -DDEBUG_VERBOSE
LDFLAGS =
LDLIBS =

ifeq ($(libav),)
$(error libav not found - target needs to be defined in the Makefile!)
endif
LDLIBS = -lavformat -lavcodec -lavutil
CFLAGS += -I$(libav)
LDLIBS += -L$(libav)/libavformat -L$(libav)/libavcodec -L$(libav)/libavutil
LDFLAGS += -Wl,-rpath,$(libav)/libavformat -Wl,-rpath,$(libav)/libavcodec -Wl,-rpath,$(libav)/libavutil

GUILIB = gtk+-3.0
# add graphical output if supported by host system
ifeq ($(shell pkg-config --exists $(GUILIB) && echo 1), 1)
$(info library found - compiling with GUI)
	C_SRCS += gtk-viewer.c
	CFLAGS += `pkg-config --cflags $(GUILIB)` -D GTK_GUI
	LDLIBS += `pkg-config --libs $(GUILIB)`
else
$(info no graphic library found - compiling without GUI)
endif


C_OBJS = $(patsubst %.$(C_EXT), %.o, $(C_SRCS))

all: $(TARGET)

$(TARGET): $(C_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(LDLIBS) -o $(TARGET) $(C_OBJS)

$(C_OBJS): %.o: %.$(C_EXT)
	$(CC) $(CFLAGS) $(LDFLAGS) $(LDLIBS) -c $< -o $@

clean:
	$(RM) $(TARGET) $(C_OBJS)
