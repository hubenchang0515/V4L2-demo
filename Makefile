
TARGET := camera
OBJS := main.o v4l2.o
CFLAGS := -DDEBUG
LIBS := -lSDL2 -ljpeg

.PHONY: all clean

all: $(TARGET)

clean:
	rm -f $(OBJS)

$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(LIBS)

main.o: v4l2.h main.c
v4l2.o: v4l2.h v4l2.c
