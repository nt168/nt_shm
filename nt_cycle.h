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

#define BUFFER_SIZE 1024  // 数据缓冲区大小

#define QUEUE_NAME "/my_task_queue"
#define MAX_QUEUE_LENGTH 100
#define THREAD_POOL_SIZE 1

//// 任务结构
//typedef struct {
//    int client_fd;           // 客户端套接字
//    void* data;//[BUFFER_SIZE];  // 接收到的数据
//    ssize_t data_len;        // 数据长度
//} task_t;
//
//// 任务节点结构
//typedef struct task_node {
//    task_t task;                  // 任务数据
//    struct task_node *next;       // 指向下一个节点
//} node_t;
//
//// 任务队列结构
//typedef struct {
//    node_t *head;            // 头指针（读取位置）
//    node_t *tail;            // 尾指针（写入位置）
//    int max_length;               // 队列最大长度
//    sem_t mutex;                  // 互斥信号量，保护队列访问
//    sem_t slots;                  // 可用槽位信号量
//    sem_t items;                  // 可用任务信号量
//    node_t nodes[];          // 任务节点数组（柔性数组成员）
//} cycle_t;
//

// 任务结构
typedef struct {
    int client_fd;           // 客户端套接字
    void* data;//[BUFFER_SIZE];  // 接收到的数据
    ssize_t data_len;        // 数据长度
} task_t;

// 任务节点结构
typedef struct task_node {
    task_t task;                  // 任务数据
    struct task_node *next;       // 指向下一个节点
} node_t;

// 任务队列结构
typedef struct {
    node_t *head;            // 头指针（读取位置）
    node_t *tail;            // 尾指针（写入位置）
    int max_length;               // 队列最大长度
    sem_t mutex;                  // 互斥信号量，保护队列访问
    sem_t slots;                  // 可用槽位信号量
    sem_t items;                  // 可用任务信号量
    node_t nodes[];          // 任务节点数组（柔性数组成员）
} cycle_t;

// 创建或打开共享的任务队列
cycle_t *task_queue_create(const char *name, int max_length);
// 打开已存在的共享任务队列
cycle_t *task_queue_open(const char *name, int max_length);
// 销毁共享的任务队列
void task_queue_destroy(const char *name, cycle_t *queue);
int task_queue_push(cycle_t *queue, task_t *task);
void *worker_thread(void *arg);
unsigned cycle_worker(void *arg);
