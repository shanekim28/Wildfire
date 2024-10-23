
EXE = wildfire
SOURCES = src/main.c src/firesim.c src/map/mapgen.c 
OBJS = $(addsuffix .o, $(basename $(notdir $(SOURCES))))
LIBS = 

LIBS += -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo 

CC=gcc
CFLAGS=-Iinclude -L./lib -lglew -lSDL3 -rpath /usr/local/lib

%.o:src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o:src/map/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

all: $(EXE)
	@echo Build complete

$(EXE): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

clean:
	rm -f $(EXE) $(OBJS)
