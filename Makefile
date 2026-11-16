CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c11 -Iinclude
LDFLAGS = -lm

SRCS = src/main.c src/file_loader.c src/tokenizer.c src/index.c \
       src/search.c src/persistence.c src/ui.c src/benchmark.c

OBJS = $(SRCS:.c=.o)
TARGET = search_engine

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f src/*.o $(TARGET) test_runner