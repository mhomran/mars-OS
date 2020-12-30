BUILD_DIR = build
CC = gcc

CFLAGS ?= -g -Wall
SRCS := $(shell find . -name "*.c")
OBJS := $(SRCS:.c=.out)

.PHONY: all
all: $(OBJS)

%.out: %.c
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $(BUILD_DIR)/$@


.PHONY: clean
clean:
	rm -f -r $(BUILD_DIR) 

.PHONY: run
run:	
	./$(BUILD_DIR)/process_generator.out

.PHONY: generate_test
generate_test:
	./$(BUILD_DIR)/test_generator.out

