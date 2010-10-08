CC=gcc
CFLAGS=-c -Wall
LDFLAGS=-lm

SDIR=./src
ODIR=$(SDIR)/obj

SOURCES=main.c imagelib.c
HEADERS=imagelib.h
EXECUTABLE=raytrace

OBJ=$(SOURCES:.c=.o)
_OBJ=$(patsubst %, $(ODIR)/%, $(OBJ))
_SOURCES=$(patsubst %, $(SDIR)/%, $(SOURCES))
_HEADERS=$(patsubst %, $(SDIR)/%, $(HEADERS))

$(ODIR)/%.o: $(SDIR)/%.c $(_HEADERS)
	$(CC) $(CFLAGS) -o $@ $<

$(EXECUTABLE): $(_OBJ) 
	$(CC) $(LDFLAGS) -o $@ $^
