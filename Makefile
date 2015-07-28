TARGET = h264-mv-extractor

C_SRCS = analyse.c decoder.c extract.c i2c.c logger.c main.c visualize.c 

INLCUDES = -I.

C_CFLAGS = -Wall -pedantic -O0 -std=c99
C_DFLAGS = #-DWITH_CV
C_LDFLAGS = -lavformat -lavcodec -lavutil

ifeq (WITH_CV, $(findstring WITH_CV, $(C_DFLAGS)))
 	C_LDFLAGS += -lopencv_core -lopencv_highgui
endif

C_EXT = c

C_OBJS = $(patsubst %.$(C_EXT), %.o, $(C_SRCS))

C = gcc
all: $(TARGET)

$(TARGET): $(CPP_OBJS) $(C_OBJS)
	$(C) $(C_CFLAGS) $(C_LDFLAGS) -o $(TARGET) $(C_OBJS)

$(C_OBJS): %.o: %.$(C_EXT)
	$(C) $(C_CFLAGS) $(C_DFLAGS) $(INCLUDES) -c $< -o $@ 

clean:
	$(RM) $(TARGET) $(C_OBJS)
