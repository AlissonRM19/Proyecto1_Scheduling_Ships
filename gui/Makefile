# Makefile — simulacion_gui (Canal de Barcos)

CC      = gcc
CFLAGS  = -Wall -O2
TARGET  = simulacion_gui
SRC     = log.c ship_pool.c serial.c scheduler.c draw.c main.c
LDFLAGS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
RM      = rm -f

# ── Reglas ───────────────────────────────────────────────────────────
all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) $(LDFLAGS) -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

clean:
	$(RM) $(TARGET)

.PHONY: all run clean
