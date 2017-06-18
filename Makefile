TARGET = h264-mv-extractor

C_SRCS = analyse.c decoder.c extract.c i2c.c logger.c main.c visualize.c 

libav = ../libav_custom

C_CFLAGS = -Wall -pedantic -O0 -std=c99
C_CFLAGS += -I.
C_CFLAGS += -I$(libav)
C_DFLAGS =
C_DFLAGS += -DWITH_CV
C_LDFLAGS = -lavformat -lavcodec -lavutil
ifneq ($(libav),)
	C_LDFLAGS += -L$(libav)/libavformat -L$(libav)/libavcodec -L$(libav)/libavutil
	C_RTLPATH = -Wl,-rpath,$(libav)/libavformat -Wl,-rpath,$(libav)/libavcodec -Wl,-rpath,$(libav)/libavutil
endif

ifeq (WITH_CV, $(findstring WITH_CV, $(C_DFLAGS)))
 	C_LDFLAGS += -lopencv_core -lopencv_highgui -lm
#        C_CFLAGS += `pkg-config --cflags --libs opencv`  
endif

C_EXT = c

C_OBJS = $(patsubst %.$(C_EXT), %.o, $(C_SRCS))

C = gcc
all: $(TARGET)

$(TARGET): $(CPP_OBJS) $(C_OBJS)
	$(C) $(C_CFLAGS) $(C_LDFLAGS) $(C_RTLPATH) `pkg-config --cflags --libs opencv`  -o $(TARGET) $(C_OBJS)

$(C_OBJS): %.o: %.$(C_EXT)
	$(C) $(C_CFLAGS) $(C_DFLAGS) $(C_LDFLAGS) -c $< -o $@

clean:
	$(RM) $(TARGET) $(C_OBJS)
