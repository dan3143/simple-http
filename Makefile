CC = gcc
CFLAGS = -Wall -g
TARGET_NAME = simple-http
BUILD_DIR ?= ./build

SRCS = $(wildcard *.c)
OBJS = $(addprefix $(BUILD_DIR)/, $(SRCS:.c=.o))
TARGET = $(BUILD_DIR)/$(TARGET_NAME)

all: $(BUILD_DIR) $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean