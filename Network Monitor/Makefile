CC=g++
CFLAGS=-I
CFLAGS+=-Wall
FILES=networkMonitor.cpp
FILES1+=intfMonitor.cpp

netmonitor: .
	$(CC) $(CFLAGS) -o networkMonitor $(FILES)

interfmonitor: .
	$(CC) $(CFLAGS) -o intfMonitor $(FILES1)

clean:
	rm -f *.o  networkMonitor intfmonitor

all: networkMonitor intfMonitor