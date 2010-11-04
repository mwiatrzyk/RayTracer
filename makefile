CC=gcc
CFLAGS=-c -Wall -O2 -DINT_ALG=2
LDFLAGS=-lm

SDIR=./src
ODIR=./obj

SOURCES=main.c bitmap.c scene.c error.c raytrace.c stringtools.c preprocess.c intersection.c
HEADERS=bitmap.h scene.h error.h raytrace.h vectormath.h stringtools.h preprocess.h intersection.h
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
