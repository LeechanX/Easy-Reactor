CXX=g++
CFLAGS=-g -O2 -Wall -fPIC -Wno-deprecated

INC=-I../../include
LIB=-L../../lib -lereactor -lprotobuf -lrt
OBJS = $(addsuffix .o, $(basename $(wildcard *.cc)))

all:
	echoMsg.proto
	$(CXX) -o server $(CFLAGS) server.cc echoMsg.pb.cc $(INC) $(LIB)
	$(CXX) -o client $(CFLAGS) client.cc echoMsg.pb.cc $(INC) $(LIB)

clean:
	-rm -f *.o *.d server client
