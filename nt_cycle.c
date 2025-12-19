#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>      // For O_* constants
#include <sys/mman.h>   // For shm_open, mmap
#include <sys/stat.h>   // For mode constants
#include <sys/shm.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include "nt_pthreads.h"
#include "nt_cycle.h"
#define BUFFER_SIZE 1024  // 数据缓冲区大小

// 创建或打开共享的任务队列
cycle_t *task_queue_create(const char *name, int max_length)
{
    int shm_fd;
    cycle_t *queue;
    size_t total_size = sizeof(cycle_t) + max_length * sizeof(node_t);

    // 创建或打开共享内存对象
    shm_fd = shm_open(name, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        return NULL;
    }

    // 设置共享内存大小
    if (ftruncate(shm_fd, total_size) == -1) {
        perror("ftruncate");
        close(shm_fd);
        return NULL;
    }

    // 映射共享内存到进程地址空间
    queue = mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (queue == MAP_FAILED) {
        perror("mmap");
        close(shm_fd);
        return NULL;
    }

    // 初始化任务队列
    queue->max_length = max_length;

    // 初始化信号量
    if (sem_init(&queue->mutex, 1, 1) == -1) { // pshared = 1 for interprocess
        perror("sem_init mutex");
        munmap(queue, total_size);
        close(shm_fd);
        return NULL;
    }
    if (sem_init(&queue->slots, 1, max_length) == -1) {
        perror("sem_init slots");
        munmap(queue, total_size);
        close(shm_fd);
        return NULL;
    }
    if (sem_init(&queue->items, 1, 0) == -1) {
        perror("sem_init items");
        munmap(queue, total_size);
        close(shm_fd);
        return NULL;
    }

    // 初始化环形链表节点
    for (int i = 0; i < max_length; i++) {
        queue->nodes[i].next = &queue->nodes[(i + 1) % max_length];
    }
    queue->head = &queue->nodes[0];
    queue->tail = &queue->nodes[0];

    close(shm_fd);
    return queue;
}

// 打开已存在的共享任务队列
cycle_t *task_queue_open(const char *name, int max_length) {
    int shm_fd;
    cycle_t *queue;
    size_t total_size = sizeof(cycle_t) + max_length * sizeof(node_t);

    // 打开共享内存对象
    shm_fd = shm_open(name, O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        return NULL;
    }

    // 映射共享内存到进程地址空间
    queue = mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (queue == MAP_FAILED) {
        perror("mmap");
        close(shm_fd);
        return NULL;
    }

    // 重新链接环形链表节点
    for (int i = 0; i < max_length; i++) {
        queue->nodes[i].next = &queue->nodes[(i + 1) % max_length];
    }

    close(shm_fd);
    return queue;
}

// 销毁共享的任务队列
void task_queue_destroy(const char *name, cycle_t *queue)
{
    size_t total_size = sizeof(cycle_t) + queue->max_length * sizeof(node_t);

    // 销毁信号量
    sem_destroy(&queue->mutex);
    sem_destroy(&queue->slots);
    sem_destroy(&queue->items);

    // 解除映射
    munmap(queue, total_size);

    // 删除共享内存对象
    shm_unlink(name);
}

// 将任务加入队列
int task_queue_push(cycle_t *queue, task_t *task)
{
    // 等待可用槽位
    sem_wait(&queue->slots);

    // 加锁
    sem_wait(&queue->mutex);

    // 添加任务到尾节点
    queue->tail->task = *task;

    // 移动尾指针
    queue->tail = queue->tail->next;

    // 解锁
    sem_post(&queue->mutex);

    // 增加可用任务数
    sem_post(&queue->items);

    return 0;
}

// 从队列中获取任务
int task_queue_pop(cycle_t *queue, task_t *task) {
    // 等待可用任务
    sem_wait(&queue->items);

    // 加锁
    sem_wait(&queue->mutex);

    // 获取任务
    *task = queue->head->task;

    // 移动头指针
    queue->head = queue->head->next;

    // 解锁
    sem_post(&queue->mutex);

    // 增加可用槽位
    sem_post(&queue->slots);

    return 0;
}

void *worker_thread(void *arg)
{
    cycle_t *queue = (cycle_t *)arg;
    task_t task;

    while (1) {
        // 获取任务
        task_queue_pop(queue, &task);

        // 处理任务
        printf("Processing task from client fd=%d, data=%s\n", task.client_fd, (char*)task.data);

        // 示例：将数据回显给客户端
        write(task.client_fd, task.data, task.data_len);

        // 如果需要关闭客户端连接，可以在这里关闭
        // close(task.client_fd);
    }
    return NULL;
}

unsigned cycle_worker(void *arg)
{
    cycle_t *queue = NULL;
    queue = ((nt_thread_args_t *)arg)->args;
    task_t task;

    while (1) {
        // 获取任务
        task_queue_pop(queue, &task);

        // 处理任务
        printf("Processing task from client fd=%d, data=%s\n", task.client_fd, (char*)task.data);

#if 0
        char reply[1036] = {0};
        snprintf(reply, 1036, "reply [%s].", task.data);
        // 示例：将数据回显给客户端
        write(task.client_fd, reply, strlen(reply));
#endif
        // 如果需要关闭客户端连接，可以在这里关闭
        // close(task.client_fd);
    }
    return 0;
}

#if 0
// 数据大小，可以根据需要调整
#define DATA_SIZE 256

// 环形缓冲区节点结构
typedef struct cycle_node {
    char data[DATA_SIZE];          // 节点数据
    volatile int valid;            // 数据有效标志，1 表示有效，0 表示无效
    // 注意：由于共享内存中无法使用指针，因此我们使用数组和索引来模拟
} node_t;

// 环形缓冲区结构
typedef struct {
    size_t size;                   // 节点总数
    size_t write_index;            // 写入位置索引
    size_t read_index;             // 读取位置索引
    sem_t mutex;                   // 互斥信号量，保护索引的访问
    node_t nodes[];    // 环形缓冲区节点数组
} cycle_t;

// 创建或打开共享的环形缓冲区
cycle_t *cycle_create(const char *name, size_t node_count) {
    int shm_fd;
    cycle_t *rb;
    size_t total_size = sizeof(cycle_t) + node_count * sizeof(node_t);

    // 创建或打开共享内存对象
    shm_fd = shm_open(name, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        return NULL;
    }

    // 设置共享内存大小
    if (ftruncate(shm_fd, total_size) == -1) {
        perror("ftruncate");
        close(shm_fd);
        return NULL;
    }

    // 映射共享内存到进程地址空间
    rb = mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (rb == MAP_FAILED) {
        perror("mmap");
        close(shm_fd);
        return NULL;
    }

    // 初始化环形缓冲区
    rb->size = node_count;
    rb->write_index = 0;
    rb->read_index = 0;

    // 初始化信号量
    if (sem_init(&rb->mutex, 1, 1) == -1) { // pshared = 1 for interprocess
        perror("sem_init mutex");
        munmap(rb, total_size);
        close(shm_fd);
        return NULL;
    }

    // 初始化节点的 valid 标志
    for (size_t i = 0; i < node_count; i++) {
        rb->nodes[i].valid = 0;
    }

    close(shm_fd);
    return rb;
}

// 打开已存在的共享环形缓冲区
cycle_t *cycle_open(const char *name, size_t node_count) {
    int shm_fd;
    cycle_t *rb;
    size_t total_size = sizeof(cycle_t) + node_count * sizeof(node_t);

    // 打开共享内存对象
    shm_fd = shm_open(name, O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        return NULL;
    }

    // 映射共享内存到进程地址空间
    rb = mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (rb == MAP_FAILED) {
        perror("mmap");
        close(shm_fd);
        return NULL;
    }

    close(shm_fd);
    return rb;
}

// 销毁共享的环形缓冲区
void cycle_destroy(const char *name, cycle_t *rb) {
    size_t total_size = sizeof(cycle_t) + rb->size * sizeof(node_t);

    // 销毁信号量
    sem_destroy(&rb->mutex);

    // 解除映射
    munmap(rb, total_size);

    // 删除共享内存对象
    shm_unlink(name);
}

// 写入数据到环形缓冲区
ssize_t cycle_write(cycle_t *rb, const char *data, size_t len) {
    size_t i;
    for (i = 0; i < len; ) {
        // 加锁
        sem_wait(&rb->mutex);

        // 写入数据
        memcpy(rb->nodes[rb->write_index].data, &data[i], DATA_SIZE);
        rb->nodes[rb->write_index].valid = 1;

        // 移动写索引
        rb->write_index = (rb->write_index + 1) % rb->size;

        // 如果写索引追上读索引，意味着覆盖未读取的数据
        if (rb->write_index == rb->read_index) {
            // 读索引也前进，表示数据被覆盖
            rb->read_index = (rb->read_index + 1) % rb->size;
        }

        // 解锁
        sem_post(&rb->mutex);

        i += DATA_SIZE;
    }
    return len;
}

// 从环形缓冲区读取数据
ssize_t cycle_read(cycle_t *rb, char *data, size_t len) {
    size_t i;
    for (i = 0; i < len; ) {
        // 加锁
        sem_wait(&rb->mutex);

        // 检查当前节点是否有有效数据
        if (rb->nodes[rb->read_index].valid == 1) {
            // 读取数据
            memcpy(&data[i], rb->nodes[rb->read_index].data, DATA_SIZE);
            rb->nodes[rb->read_index].valid = 0;

            // 移动读索引
            rb->read_index = (rb->read_index + 1) % rb->size;

            i += DATA_SIZE;
        } else {
            // 没有数据可读，解锁并返回
            sem_post(&rb->mutex);
            break;
        }

        // 解锁
        sem_post(&rb->mutex);
    }
    return i; // 返回实际读取的字节数
}
#endif
