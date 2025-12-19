CC := gcc
TARGET := nt_shm

SRCS := common.c ddl.c fatal.c hashset.c nt_algodefs.c nt_binaryheap.c nt_cycle.c nt_daemon.c nt_hashmap.c nt_log.c nt_mem.c nt_mutexs.c nt_phreads.c nt_ptcl.c nt_shm.c nt_socket.c
OBJS := $(SRCS:.c=.o)
DEPS := $(OBJS:.o=.d)

CSTD := -std=c11
WARN := -w
INC  := -I.
DEFS := -D_POSIX_C_SOURCE=200809L

CFLAGS := $(CSTD) $(WARN) $(INC) $(DEFS) -O0 -g3 -MMD -MP
LDFLAGS :=
LDLIBS  :=

.PHONY: all clean
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS) $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS) $(DEPS)

-include $(DEPS)
