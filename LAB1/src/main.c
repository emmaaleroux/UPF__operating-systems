// OPERATING SYSTEMS - LAB 1
// EMMA LEROUX 304174 & GUILLEM ARÉVALO 306124

#include <unistd.h> // read(), write(), close()
#include <fcntl.h> // open(), O_RDONLY
#include <stdlib.h> // atoi(), atoll()
#include <string.h> // strcmp()

#include "circularBuffer.h"


int main(int argc, char *argv[]) {
    if (argc != 4) {return 1;} // We expect 3 arguments: ./main <binary|text> <file_path> <buffer_size>
    
    char *format = argv[1];
    char *path = argv[2];
    int bufferSize = atoi(argv[3]);
    
    if (bufferSize <= 0) {return 1;} // We avoid invalid buffer sizes

    // We check the format from the input
    // BINARY = 1 and TEXT = 2
    int mode;
    if (strcmp(format, "binary") == 0) {
        mode = 1;
    } else if (strcmp(format, "text") == 0) {
        mode = 2;
    } else {
        return 1;
    }

    // We open the file (read only)
    int fd = open(path, O_RDONLY);
    if (fd < 0) {return 1;}
    
    // We use a secondary linear buffer, with the same size, to where we read operations 
    char buffer[bufferSize];
    // We initialize the sum variable, that we will output in the end
    int sum = 0;


    // CASE 1: BINARY FILE

    if (mode == 1) {
        
        // Each element has a fixed size (sizeof(int))
        // We adjust the buffer size so that it is a multiple of the integer size. 
        // If necessary, you should adjust the buffer size so that only complete elements are read, even if this means using slightly fewer bytes per read.
        int intSize = sizeof(int);
        int usableBytes = bufferSize - (bufferSize % intSize);

        if (usableBytes == 0) { 
            close(fd);
            return 1;
        }

        int n;
        while ((n = read(fd, buffer, usableBytes)) > 0) {
            // Raw bytes to integers and count the number of read integers
            int *numbers = (int *)buffer;
            int count = n / intSize;
            // We compute the sum
            for (int i = 0; i < count; i++) {
                sum += numbers[i];
            }
        }
            
    }


    // CASE 2: TEXT FILE

    if (mode == 2) {

        // We initialize the circular buffer
        CircularBuffer cb;
        buffer_init(&cb, bufferSize);

        int reachedEOF = 0;

        
        while (!reachedEOF || buffer_used_bytes(&cb) > 0) {
            // Only read if there is space in the circular buffer
            int free = buffer_free_bytes(&cb);
            if (free > 0 && !reachedEOF) {
                int bytesRead = read(fd, buffer, free);
                if (bytesRead == 0)
                    reachedEOF = 1;
                else if (bytesRead < 0)
                    break;

                for (int i = 0; i < bytesRead; i++)
                    buffer_push(&cb, buffer[i]);
            }
            // Pop only complete numbers
            int elemSize;
            while ((elemSize = buffer_size_next_element(&cb, ',', reachedEOF)) != -1) {
                char numberStr[32];
                int idx = 0;
                for (int i = 0; i < elemSize; i++) {
                    char c = buffer_pop(&cb);
                    if (c != ',' && c != '\n')
                        numberStr[idx++] = c;
                }
                numberStr[idx] = '\0';
                sum += atoi(numberStr); // Convert to int and add to sum
            }
        }
        buffer_deallocate(&cb);
    }

    close(fd);

    // We convert sum to string (variable out)
    char out[32];
    int len = 0;

    int temp = sum; // To do so, we use a temporary variable
    // If the sum is negative, we add the - and process it as a positive number
    if (temp < 0) {
        out[len++] = '-';
        temp = -temp;
    }
    // We extract the digits in reverse
    char rev[32];
    int r = 0;
    if (temp == 0) {rev[r++] = '0';}
    while (temp > 0) {
        rev[r++] = '0' + (temp % 10); // Get last digit
        temp /= 10; // Remove last digit
    }
    // Now we save it into out in the right order
    for (int i = r - 1; i >= 0; i--) {
        out[len++] = rev[i];
    }
    out[len++] = '\n';

    write(1, out, len); // We output the final sum
    return 0;
}