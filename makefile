CC      = c++
CFLAGS  = -O3
OPTION  = -std=c++11
LIBS    = -L$(LIB) -lboost_system -lboost_thread -pthread -lpthread

# Used a quick install of boost, header only files needed for boost
LIB = ../boost_1_80_0

all: server client

server: serverO.o
	$(CC) -o $@ $^ $(LDFLAGS) $(LIBS)

client: clientO.o
	$(CC) -o $@ $^ $(LDFLAGS) $(LIBS)
	
serverO.o: serverO.cpp protocol.hpp
	$(CC) $(OPTION) -c $(CFLAGS) serverO.cpp

clientO.o: clientO.cpp protocol.hpp
	$(CC) $(OPTION) -c $(CFLAGS) clientO.cpp

.PHONY: clean

clean:
	rm *.o
	rm serverO clientO
