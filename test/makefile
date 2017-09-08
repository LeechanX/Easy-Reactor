CXX=g++
CFLAGS=-g -O2 -Wall -fPIC -Wno-deprecated

INC=-I../include
LIB=-L../lib -lereactor /usr/local/lib/libprotobuf.a -lrt
OBJS = $(addsuffix .o, $(basename $(wildcard *.cc)))

all:
	$(CXX) -o server.prog $(CFLAGS) server.cc echoMsg.pb.cc $(INC) $(LIB)
	$(CXX) -o benchmark.prog $(CFLAGS) benchmark.cc echoMsg.pb.cc $(INC) $(LIB)

clean:
	-rm -f *.o *.d server client
