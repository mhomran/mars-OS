BUILD_DIR = build
CC = gcc

CFLAGS ?= -g -Wall
SRCS := $(shell find . -name "*.c")
OBJS := $(SRCS:.c=.out)

.PHONY: all
all:
	$(CC) $(CFLAGS) producer.c -o $(BUILD_DIR)/producer.out
	$(CC) $(CFLAGS) consumer.c -o $(BUILD_DIR)/consumer.out

producer.out: producer.c
	$(CC) $(CFLAGS) producer.c -o $(BUILD_DIR)/producer.out

consumer.out: consumer.c
	$(CC) $(CFLAGS) consumer.c -o $(BUILD_DIR)/consumer.out

.PHONY: clean
clean:
	rm -f -r $(BUILD_DIR) 

.PHONY: run_producer
run_producer:
	./$(BUILD_DIR)/producer.out

.PHONY: run_consumer
run_consumer:
	./$(BUILD_DIR)/consumer.out

.PHONY: run_valgrind_p
run_valgrind_p:	
	valgrind ./$(BUILD_DIR)/producer.out

.PHONY: run_valgrind_c
run_valgrind_c:	
	valgrind ./$(BUILD_DIR)/consumer.out