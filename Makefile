CFLAGS=-Wall -Wextra -fpic -g -I./include -DHAS_STDINT_H -D_GNU_SOURCE
LDFLAGS=-lpthread -lrt -shared
CC=gcc

SRC_DIR=src
ARCH_DIR=$(SRC_DIR)/arch/linux

ARCH_SRCS= \
	$(ARCH_DIR)/ff_arch_completion_port.c \
	$(ARCH_DIR)/ff_arch_fiber.c \
	$(ARCH_DIR)/ff_arch_file.c \
	$(ARCH_DIR)/ff_arch_misc.c \
	$(ARCH_DIR)/ff_arch_mutex.c \
	$(ARCH_DIR)/ff_arch_net_addr.c \
	$(ARCH_DIR)/ff_arch_tcp.c \
	$(ARCH_DIR)/ff_arch_thread.c \
	$(ARCH_DIR)/ff_arch_udp.c \
	$(ARCH_DIR)/ff_linux_net.c

MAIN_SRCS= \
	$(SRC_DIR)/ff_blocking_queue.c \
	$(SRC_DIR)/ff_blocking_stack.c \
	$(SRC_DIR)/ff_container.c \
	$(SRC_DIR)/ff_core.c \
	$(SRC_DIR)/ff_dictionary.c \
	$(SRC_DIR)/ff_event.c \
	$(SRC_DIR)/ff_fiber.c \
	$(SRC_DIR)/ff_fiberpool.c \
	$(SRC_DIR)/ff_file.c \
	$(SRC_DIR)/ff_hash.c \
	$(SRC_DIR)/ff_log.c \
	$(SRC_DIR)/ff_loopback.c \
	$(SRC_DIR)/ff_malloc.c \
	$(SRC_DIR)/ff_mutex.c \
	$(SRC_DIR)/ff_pipe.c \
	$(SRC_DIR)/ff_pool.c \
	$(SRC_DIR)/ff_queue.c \
	$(SRC_DIR)/ff_read_stream_buffer.c \
	$(SRC_DIR)/ff_semaphore.c \
	$(SRC_DIR)/ff_stack.c \
	$(SRC_DIR)/ff_stream.c \
	$(SRC_DIR)/ff_stream_acceptor.c \
	$(SRC_DIR)/ff_stream_acceptor_tcp.c \
	$(SRC_DIR)/ff_stream_connector.c \
	$(SRC_DIR)/ff_stream_connector_tcp.c \
	$(SRC_DIR)/ff_stream_pipe.c \
	$(SRC_DIR)/ff_stream_tcp.c \
	$(SRC_DIR)/ff_tcp.c \
	$(SRC_DIR)/ff_threadpool.c \
	$(SRC_DIR)/ff_udp.c \
	$(SRC_DIR)/ff_write_stream_buffer.c

FF_LIB_SRCS= \
	$(ARCH_SRCS) \
	$(MAIN_SRCS)

default: all

all: libfiber-framework.so ff-tests

libfiber-framework.so: $(FF_LIB_SRCS)
	$(CC) $(CFLAGS) -o libfiber-framework.so $(FF_LIB_SRCS) $(LDFLAGS)

ff-tests:
	cd ./tests && make ff-tests && cp ff-tests ../
	./ff-tests

clean:
	cd ./tests && make clean
	rm -f libfiber-framework.so ff-tests

