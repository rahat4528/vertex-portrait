CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2
LDFLAGS = -lm

OBJS = canvas.o draw.o face.o
TARGET = face

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

canvas.o: canvas.c canvas.h
	$(CC) $(CFLAGS) -c canvas.c

draw.o: draw.c draw.h canvas.h
	$(CC) $(CFLAGS) -c draw.c

face.o: face.c canvas.h draw.h
	$(CC) $(CFLAGS) -c face.c

run: $(TARGET)
	./$(TARGET)

print: $(TARGET)
	./$(TARGET) --print

clean:
	rm -f $(OBJS) $(TARGET) $(TARGET).exe face.ppm face.bmp

.PHONY: all run print clean
