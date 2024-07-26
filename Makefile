CC=gcc
CFLAGS=-Iinclude -Llib -lSDL3 -rpath /usr/local/lib

all:
	$(CC) $(CFLAGS) -o build/Wildfire src/main.c src/firesim.c
clean:
	rm -f ./build/*.o ./build/Wildfire