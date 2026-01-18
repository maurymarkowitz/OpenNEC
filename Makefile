CC = gcc
CFLAGS = -I. -Isrc -O2 -Wall
LDFLAGS =

SOURCES = src/main.c src/input.c src/output.c src/deck.c src/tests.c src/geometry.c src/calculations.c src/fields.c src/ground.c src/matrix.c src/network.c src/radiation.c src/somnec.c src/misc.c src/shared.c src/types.c src/tinyexpr.c

LIB_SOURCES = $(filter-out src/main.c, $(SOURCES))
LIB_OBJECTS = $(LIB_SOURCES:.c=.o)
LIBRARY = libonec.a

EXECUTABLE = onec

all: $(EXECUTABLE)

$(LIBRARY): $(LIB_OBJECTS)
	ar rcs $@ $^

$(EXECUTABLE): src/main.o $(LIBRARY)
	$(CC) $(LDFLAGS) src/main.o $(LIBRARY) -o $@ -lm

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(LIB_OBJECTS) src/main.o $(LIBRARY) $(EXECUTABLE)

.PHONY: all clean