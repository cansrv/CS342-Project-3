CC = gcc
CFLAGS = -Wall
LDFLAGS = -lm
OBJFILES = dma.o
TARGET = dma

all: $(TARGET)

$(TARGET): $(OBJFILES)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJFILES) $(LDFLAGS)
clean:
	rm -f dma.h.gch $(TARGET) $(OBJFILES) *~