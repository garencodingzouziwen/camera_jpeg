# CROSS_COMPILE = mips-openwrt-linux-musl-
# CC = $(CROSS_COMPILE)gcc
CC = gcc
TARGET = camera_picture
DEPS = $(shell find ./ -name "*.h")
SRC  = $(shell find ./ -name "*.c")
OBJ  = $(SRC:%.c=%.o)
ROOT_DIR = .
INCLUDES :=
INCLUDES += -I$(ROOT_DIR)/include
CFLAGS += -pipe -Wall $(INCLUDES)

$(TARGET): $(OBJ)
	$(CC) -o $(TARGET) $(OBJ)
	rm -rf $(OBJ)

%.o: %.c $(DEPS) 
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ) $(TARGET)