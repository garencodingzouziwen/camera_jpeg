#CROSS_COMPILE = mips-openwrt-linux-musl-
#CROSS_COMPILE = /home/garen/share/github_project/openwrt-toolchain-ramips-mt7620_gcc-7.4.0_musl.Linux-x86_64/toolchain-mipsel_24kc_gcc-7.4.0_musl/bin/mipsel-openwrt-linux-musl-
CC = gcc
#CC = $(CROSS_COMPILE)gcc
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