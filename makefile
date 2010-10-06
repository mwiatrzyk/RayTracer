CC=gcc
CFLAGS=-c -Wall
LDFLAGS=
SOURCES=main.c imagelib.c
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=raytrace

__start__: $(EXECUTABLE)
	./$(EXECUTABLE)

all: $(SOURCES) $(EXECUTABLE)
	
$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

