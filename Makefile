
EXE = wildfire
OUTPUT_DIR = build
SOURCES = src/main.c src/firesim.c src/map/mapgen.c src/util.c
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
	$(CC) $(CFLAGS) -o $(OUTPUT_DIR)/$@ $^ $(LIBS)

clean:
	rm -f $(OUTPUT_DIR)/$(EXE) $(OBJS)
