CXX = g++
CXXFLAGS = -Wall -I/usr/include -I/usr/include/curl
LDFLAGS = -lcurl -pthread

objects = termcomm.o comm.o

all: termcomm

termcomm: $(objects)
	$(CXX) -o termcomm $(objects) $(LDFLAGS)

termcomm.o: termcomm.cpp termcomm.h
	$(CXX) $(CXXFLAGS) -c termcomm.cpp

comm.o: comm.cpp comm.h
	$(CXX) $(CXXFLAGS) -c comm.cpp

run:
	./termcomm

clean:
	rm -f termcomm $(objects)


