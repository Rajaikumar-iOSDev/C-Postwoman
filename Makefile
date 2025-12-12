
CC = gcc
CFLAGS = -O2 -Wall -std=c11
# Use pkg-config to locate raylib includes/libs when available
PKG_CFLAGS := $(shell pkg-config --cflags raylib 2>/dev/null)
PKG_LIBS   := $(shell pkg-config --libs raylib 2>/dev/null)

# Link against curl, pthread and math. If pkg-config provides raylib libs this will
# be appended after them; otherwise we keep -lraylib as a fallback.
LDFLAGS = -lcurl -lpthread -lm
RAYLIB_FALLBACK = -lraylib

# On macOS you may need frameworks; pkg-config often supplies these but keep as fallback
OSX_FRAMEWORKS = -framework Cocoa -framework OpenGL -framework IOKit -framework CoreVideo

SRC = main.c network.c
OUT = PostWoman

all: $(OUT)

$(OUT): $(SRC)
	@echo "Compiling with CFLAGS: $(CFLAGS) $(PKG_CFLAGS)"
	$(CC) $(CFLAGS) $(PKG_CFLAGS) $(SRC) -o $(OUT) $(PKG_LIBS) $(LDFLAGS) $(OSX_FRAMEWORKS) $(if $(PKG_LIBS),,$(RAYLIB_FALLBACK))

clean:
	rm -f $(OUT)
