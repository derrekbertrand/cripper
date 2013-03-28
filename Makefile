CC=gcc
CFLAGS=-c -g -Wall
LDFLAGS=
LIBS= $(shell pkg-config allegro-5 allegro_dialog-5 allegro_image-5 --libs)
SOURCES=main.c lookup3.c
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=cripper

all: $(SOURCES) $(EXECUTABLE)
	
$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@ $(LIBS)

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf *o $(EXECUTABLE)
