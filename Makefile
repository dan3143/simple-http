CC = gcc

CFLAGS  = -Wall -g -Iinclude -pthread -lssl -lcrypto
LDFLAGS = -pthread

TARGET_NAME = simple-http
SRC_DIR = src
BUILD_DIR = build

SRCS := $(shell find $(SRC_DIR) -name '*.c')
OBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))

TARGET = $(BUILD_DIR)/$(TARGET_NAME)

$(BUILD_DIR)/core/log.o: CFLAGS += -DLOG_USE_COLOR

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean