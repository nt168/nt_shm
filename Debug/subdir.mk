################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../common.c \
../fatal.c \
../hashset.c \
../nt_algodefs.c \
../nt_binaryheap.c \
../nt_cycle.c \
../nt_daemon.c \
../nt_hashmap.c \
../nt_log.c \
../nt_mem.c \
../nt_mutexs.c \
../nt_phreads.c \
../nt_ptcl.c \
../nt_shm.c \
../nt_socket.c 

C_DEPS += \
./common.d \
./fatal.d \
./hashset.d \
./nt_algodefs.d \
./nt_binaryheap.d \
./nt_cycle.d \
./nt_daemon.d \
./nt_hashmap.d \
./nt_log.d \
./nt_mem.d \
./nt_mutexs.d \
./nt_phreads.d \
./nt_ptcl.d \
./nt_shm.d \
./nt_socket.d 

OBJS += \
./common.o \
./fatal.o \
./hashset.o \
./nt_algodefs.o \
./nt_binaryheap.o \
./nt_cycle.o \
./nt_daemon.o \
./nt_hashmap.o \
./nt_log.o \
./nt_mem.o \
./nt_mutexs.o \
./nt_phreads.o \
./nt_ptcl.o \
./nt_shm.o \
./nt_socket.o 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean--2e-

clean--2e-:
	-$(RM) ./common.d ./common.o ./fatal.d ./fatal.o ./hashset.d ./hashset.o ./nt_algodefs.d ./nt_algodefs.o ./nt_binaryheap.d ./nt_binaryheap.o ./nt_cycle.d ./nt_cycle.o ./nt_daemon.d ./nt_daemon.o ./nt_hashmap.d ./nt_hashmap.o ./nt_log.d ./nt_log.o ./nt_mem.d ./nt_mem.o ./nt_mutexs.d ./nt_mutexs.o ./nt_phreads.d ./nt_phreads.o ./nt_ptcl.d ./nt_ptcl.o ./nt_shm.d ./nt_shm.o ./nt_socket.d ./nt_socket.o

.PHONY: clean--2e-

