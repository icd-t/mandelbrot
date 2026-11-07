TARGET = mandelgen
CC = g++
CVER = 26
CFLAGS = -O3 -march=native -g -Wall -Wextra --std=c++$(CVER) 

SRCS = ./src/main.cpp
OBJS = $(SRCS:.cpp=.o)

LIBS = -lquadmath

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CLFAGS) $(OBJS) -o $(TARGET) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean: 
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
