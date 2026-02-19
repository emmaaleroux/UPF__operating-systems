#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define MAX_BUFF_SIZE 1024

typedef struct {        // From Hint 2
    char* path; 
    int offset;         // Offset from the beginning of the file (including header) 
    int bytesToRead; 
    // any other info you might want to pass 
} ThreadInfo;

void* thread(void* params) {
    /*
    Each worker thread must process a distinct portion of the image data. 
    The portion assigned to a thread is defined by a starting offset 
    (measured from the beginning of the file, including the header) 
    and a number of bytes to read.
    */
}

int main(int argc, char* argv[]) {

    if (argc != 4) {
        printf("Incorrect usage. Args needed: histogram pathToImage pathToHistogramOut nThreads\n");
        _exit(1);
    }

    /*
    The main reads the image metadata, 
    determines the size of the data segment (excluding the header), 
    and creates the threads.
    */

    int nThreads = atoi(argv[3]);
    pthread_t threads[nThreads];


    // Hint1: Because file descriptors maintain a shared file pointer, using a single shared descriptor across threads may 
    // lead to unintended interference. For this reason, it is recommended that each thread opens the file independently. 



    return 0;
}