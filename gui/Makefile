# Makefile — simulacion_gui (Canal de Barcos)
# Compatible con Linux y Windows (MinGW/MSYS2)
#
# Linux — instalar raylib:
#   Ubuntu/Debian : sudo apt install libraylib-dev
#   Fedora        : sudo dnf install raylib-devel
#   Arch          : sudo pacman -S raylib
#   Desde fuente  : https://github.com/raysan5/raylib
#
# Windows (MSYS2 ucrt64):
#   pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-raylib

CC     = gcc
SRC    = simulacion_gui.c
CFLAGS = -Wall -O2

# ── Deteccion de plataforma ──────────────────────────────────────────
ifeq ($(OS),Windows_NT)
    TARGET  = simulacion_gui.exe
    LDFLAGS = -lraylib -lopengl32 -lgdi32 -lwinmm -mwindows
    RM      = del /Q
else
    TARGET  = simulacion_gui
    LDFLAGS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
    RM      = rm -f
endif

# ── Reglas ───────────────────────────────────────────────────────────
all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) $(LDFLAGS) -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

clean:
	$(RM) $(TARGET)

.PHONY: all run clean
