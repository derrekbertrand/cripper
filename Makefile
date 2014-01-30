CC=g++
CFLAGS=-c -Wall
LDFLAGS=
LIBS= $(shell pkg-config allegro_monolith-5 --libs)
SOURCES=main.cpp lookup3.c
OBJECTS=$(SOURCES:.*=.o)
EXECUTABLE=cripper

all: $(OBJECTS) $(EXECUTABLE)
	
$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@ $(LIBS)

.cpp.o:
	$(CC) $(CFLAGS) $<

.c.o: 
	$(CC) $(CFLAGS) $<

clean:
	rm -rf *o $(EXECUTABLE)
