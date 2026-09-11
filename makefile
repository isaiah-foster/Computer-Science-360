CC = gcc
CFLAGS = -Wall -std=c99 -g
TARGET = pa0
SRC = pa0_foster.c command.c tree.c storage.c

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(SRC) command.h tree.h storage.h
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)
