CC=gcc
CFLAGS=-c -Wall
LDFLAGS=-lm

SDIR=./src
ODIR=$(SDIR)/obj

SOURCES=main.c imagelib.c scene.c error.c
HEADERS=imagelib.h scene.h error.h
EXECUTABLE=raytrace

OBJ=$(SOURCES:.c=.o)
_OBJ=$(patsubst %, $(ODIR)/%, $(OBJ))
_SOURCES=$(patsubst %, $(SDIR)/%, $(SOURCES))
_HEADERS=$(patsubst %, $(SDIR)/%, $(HEADERS))

__start__: $(EXECUTABLE)
	./$(EXECUTABLE)

$(ODIR)/%.o: $(SDIR)/%.c $(_HEADERS)
	$(CC) $(CFLAGS) -o $@ $<

$(EXECUTABLE): $(_OBJ) 
	$(CC) $(LDFLAGS) -o $@ $^
