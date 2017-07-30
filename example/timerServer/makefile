CXX=g++
CFLAGS=-g -O2 -Wall -fPIC -Wno-deprecated

INC=-I../../include
LIB=-L../../lib -lereactor -lrt
OBJS = $(addsuffix .o, $(basename $(wildcard *.cc)))

all:
	$(CXX) -o server.prog $(CFLAGS) server.cc $(INC) $(LIB)

clean:
	-rm -f *.o *.d server.prog
