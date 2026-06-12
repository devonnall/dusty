CC := clang
CFLAGS := -Wall -Wextra -Wno-misleading-indentation -I./include -fsanitize=address -fno-omit-frame-pointer -g

SRCS := ./src/main.c ./src/dscanner.c ./src/fileio.c
TARGET := ./bin/dusty

$(TARGET): $(SRCS)
	mkdir -p bin
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET)

clean:
	rm -f $(TARGET)
	rm -rf *.dSYM
