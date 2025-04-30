// lab.c
#include "lab.h"
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct queue {
    void **buffer;             // Circular buffer to hold data
    int capacity;              // Max capacity of the queue
    int front;                 // Index of the front element
    int rear;                  // Index of the next insertion point
    int size;                  // Current number of elements
    bool shutdown_flag;        // Flag for shutdown

    pthread_mutex_t lock;      // Mutex for synchronizing access
    pthread_cond_t not_full;   // Condition variable: wait if full
    pthread_cond_t not_empty;  // Condition variable: wait if empty
} queue_t_internal;

queue_t queue_init(int capacity) {
    if (capacity <= 0) return NULL;

    queue_t_internal *q = malloc(sizeof(queue_t_internal));
    if (!q) return NULL;

    q->buffer = malloc(sizeof(void *) * capacity);
    if (!q->buffer) {
        free(q);
        return NULL;
    }

    q->capacity = capacity;
    q->front = 0;
    q->rear = 0;
    q->size = 0;
    q->shutdown_flag = false;

    pthread_mutex_init(&q->lock, NULL);
    pthread_cond_init(&q->not_full, NULL);
    pthread_cond_init(&q->not_empty, NULL);

    return (queue_t)q;
}

void queue_destroy(queue_t q_ptr) {
    if (!q_ptr) return;
    queue_t_internal *q = (queue_t_internal *)q_ptr;

    pthread_mutex_lock(&q->lock);
    q->shutdown_flag = true;
    pthread_cond_broadcast(&q->not_empty);
    pthread_cond_broadcast(&q->not_full);
    pthread_mutex_unlock(&q->lock);

    pthread_mutex_destroy(&q->lock);
    pthread_cond_destroy(&q->not_full);
    pthread_cond_destroy(&q->not_empty);

    free(q->buffer);
    free(q);
}

void enqueue(queue_t q_ptr, void *data) {
    queue_t_internal *q = (queue_t_internal *)q_ptr;

    pthread_mutex_lock(&q->lock);
    while (q->size == q->capacity && !q->shutdown_flag) {
        pthread_cond_wait(&q->not_full, &q->lock);
    }

    if (q->shutdown_flag) {
        pthread_mutex_unlock(&q->lock);
        return;
    }

    q->buffer[q->rear] = data;
    q->rear = (q->rear + 1) % q->capacity;
    q->size++;

    pthread_cond_signal(&q->not_empty);
    pthread_mutex_unlock(&q->lock);
}

void *dequeue(queue_t q_ptr) {
    queue_t_internal *q = (queue_t_internal *)q_ptr;

    pthread_mutex_lock(&q->lock);
    while (q->size == 0 && !q->shutdown_flag) {
        pthread_cond_wait(&q->not_empty, &q->lock);
    }

    if (q->size == 0 && q->shutdown_flag) {
        pthread_mutex_unlock(&q->lock);
        return NULL;
    }

    void *data = q->buffer[q->front];
    q->front = (q->front + 1) % q->capacity;
    q->size--;

    pthread_cond_signal(&q->not_full);
    pthread_mutex_unlock(&q->lock);
    return data;
}

void queue_shutdown(queue_t q_ptr) {
    queue_t_internal *q = (queue_t_internal *)q_ptr;

    pthread_mutex_lock(&q->lock);
    q->shutdown_flag = true;
    pthread_cond_broadcast(&q->not_full);
    pthread_cond_broadcast(&q->not_empty);
    pthread_mutex_unlock(&q->lock);
}

bool is_empty(queue_t q_ptr) {
    queue_t_internal *q = (queue_t_internal *)q_ptr;

    pthread_mutex_lock(&q->lock);
    bool empty = (q->size == 0);
    pthread_mutex_unlock(&q->lock);

    return empty;
}

bool is_shutdown(queue_t q_ptr) {
    queue_t_internal *q = (queue_t_internal *)q_ptr;

    pthread_mutex_lock(&q->lock);
    bool shutdown = q->shutdown_flag;
    pthread_mutex_unlock(&q->lock);

    return shutdown;
}
