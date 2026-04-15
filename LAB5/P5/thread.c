/*
 * (C) 2026, Cornell University
 * All rights reserved.
 *
 * Description: cooperative multithreading and synchronization
 */

#include <sys/queue.h>
#include "print.c"
#include "thread.h"
#include <stdint.h>

/* Student's code goes here (Cooperative Threads). */
/* Define the TCB and helper functions (if needed) for multi-threading. */

/* Student's code ends here. */

void thread_cleanup(){
    for (int i = 1; i < 32; i++) {
        printf("Freeing memory of %d\n", i);
        if (TCB[i].status == THREAD_DYING) {
            TCB[i].status = THREAD_EMPTY;
            free(TCB[i].sp);
        }
    }
}
/* Student's code ends here. */


void thread_init() {
    /* Student's code goes here (Cooperative Threads). */
    TCB[0].status = THREAD_RUNNING;
    max_id = 0;
    TCB[0].id = max_id;
    current_idx = 0;

    // All other, will be empty
    for (int i =1; i < 32; i++) {
        TCB[i].status = THREAD_EMPTY;
    }

    /* Student's code ends here. */
}

void print_TCB() {
    for (int i =0; i < 32; i++) {
        if (TCB[i].status == THREAD_EMPTY)
            printf("Thread %d is %s \n", i, "EMPTY");
        if (TCB[i].status == THREAD_READY)
            printf("Thread %d is %s \n", i, "READY");
        if (TCB[i].status == THREAD_RUNNING)
            printf("Thread %d is %s \n", i, "RUNNING");
        if (TCB[i].status == THREAD_DYING)
            printf("Thread %d is %s \n", i, "DYING");

    }
}

void ctx_entry() {
    /* Student's code goes here (Cooperative Threads). */
    for (int i = 0; i < 32; ++i) {
        if (TCB[i].status == THREAD_STARTING) {
            TCB[i].status = THREAD_RUNNING;
            TCB[current_idx].status = THREAD_READY;
            current_idx = i;
            TCB[i].f_init(TCB[i].arg_init); // wil not return from here
            thread_exit();
        } 
    }  
    /* Student's code ends here. */
}

void thread_create(void (*entry)(void *arg), void *arg, int stack_size) {
    
    // Do a cleanup
    thread_cleanup();

    /* Student's code goes here (Cooperative Threads). */
    // Look for an empty position
    for (int i = 0; i < 32; i++) {
        if (TCB[i].status == THREAD_EMPTY){
            
            // Create the TCB
            TCB[i].status = THREAD_STARTING; // Need to do it before, otherwise it will not work
            TCB[i].f_init = entry;
            TCB[i].arg_init =arg;
            TCB[i].id = max_id +1;
            ++max_id;

            char* child_stack = malloc(STACK_SIZE); 
            ctx_start(&TCB[current_idx].sp, child_stack + STACK_SIZE); // Change from old to new, saves all the registers of old
            // from this point on, I cannot use any local variable
            return;
        }
    }

    /* Student's code ends here. */
}

void thread_yield() {
    /* Student's code goes here (Cooperative Threads). */
    for (int i = 0; i < 32; i++) {
        if (TCB[i].status == THREAD_READY){
            if (TCB[current_idx].status == THREAD_RUNNING) { // to avoid reviving it, if dead
                TCB[current_idx].status = THREAD_READY;
            }
            int old_idx = current_idx;
            current_idx = i;
            TCB[current_idx].status = THREAD_RUNNING;

            
            ctx_switch(&TCB[old_idx].sp, TCB[current_idx].sp); // Change from old to new, saves all the registers of old
            return;
        }
    }
    /* Student's code ends here. */
}


/* GB Student's code ends here*/
void thread_exit() {
    // Check if I am the last thread.

    TCB[current_idx].status = THREAD_DYING;
    // Cannot do free yet, cause all local variables would be destroyed, and I could not continue
    // I need to wait, and do the clean up from another thread

    for (int i = 0; i < 32; i++) {
        if (TCB[i].status == THREAD_READY){
            thread_yield();
        }
    }

    thread_cleanup();

    // If there is no other thread to jump, end
    _end();
    /* Student's code ends here. */
}

// Part of the locks

void cv_init(struct cv *condition) {
    /* Student's code goes here (Cooperative Threads). */
    condition->waiting_threads = queue_new();
    condition->nThreadsWaiting = 0;
    /* Student's code ends here. */
}

void cv_wait(struct cv *condition) {
    /* Student's code goes here (Cooperative Threads). */
    queue_enqueue(condition->waiting_threads, (void *) current_idx);
    TCB[current_idx].status = THREAD_WAITING;
    condition->nThreadsWaiting++;
    thread_yield();
    /* Student's code ends here. */
}

void cv_signal(struct cv *condition) {
    /* Student's code goes here (Cooperative Threads). */
    int thread_to_wake_up;

    if (queue_dequeue(condition->waiting_threads, (void **) &thread_to_wake_up) != -1) {
        TCB[thread_to_wake_up].status = THREAD_READY;
        condition->nThreadsWaiting--;
    }
    /* Student's code ends here. */
}

#define BUF_SIZE 3
void* buffer[BUF_SIZE];
int count = 0;
int head = 0, tail = 0;
struct cv nonempty, nonfull;

void produce(void* item) {
    while (1) {
        while (count == BUF_SIZE) cv_wait(&nonfull);
        /* At this point, the buffer is not full. */

        /* Student's code goes here (Cooperative Threads). */
        /* Print out producer thread ID and the item pointer */
        printf("Producing, count = %d\n'", count);

        /* Student's code ends here. */
        buffer[tail] = item;
        tail = (tail + 1) % BUF_SIZE;
        count += 1;
        cv_signal(&nonempty);
    }
}

void consume(void *arg) {
    while (1) {
        while (count == 0) cv_wait(&nonempty);
        /* At this point, the buffer is not empty. */

        /* Student's code goes here (Cooperative Threads). */
        /* Print out producer thread ID and the item pointer */
        printf("Consuming, count = %d\n", count);

        /* Student's code ends here. */
        void* result = buffer[head];
        head = (head + 1) % BUF_SIZE;
        count -= 1;
        cv_signal(&nonfull);
    }
}



int main() {
    thread_init();
    cv_init(&nonempty);
    cv_init(&nonfull);
    uint64_t t;
    asm volatile ("rdtime %0" : "=r"(t));
    printf("Time = %d\n", t);

    for (int i = 0; i < 5; i++)
        thread_create(consume, NULL, STACK_SIZE / 16);

    for (int i = 0; i < 5; i++)
        thread_create(produce, NULL, STACK_SIZE / 16);

    printf("main thread exits\n\r");
    thread_exit();

    asm volatile ("rdtime %0" : "=r"(t));
    printf("Time = %d\n", t);

}

/*
void child(void* arg) {
    for (int i = 0; i < 10; i++) {
        printf("%s is in for loop i=%d\n\r", arg, i);
        thread_yield();
    }
}


int main() {
    thread_init();
    thread_create(child, "Child thread", STACK_SIZE / 16);
    for (int i = 0; i < 10; i++) {
        printf("Main thread is in for loop i=%d\n\r", i);
        thread_yield();
    }
    thread_exit();
}
*/