CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -pedantic -O2
TARGET = myshell
SRC = myshell.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

.PHONY: all clean
