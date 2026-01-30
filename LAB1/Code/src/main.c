#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#include "circularBuffer.h"

int main(int argc, char *argv[]) {
    if (argc != 4) {return 1;}
    
    char *format = argv[1];
    char *path = argv[2];
    int bufferSize = atoi(argv[3]);
    
    if (bufferSize <= 0) {return 1;}

    // We check the format
    int mode;
    if (strcmp(format, "binary") == 0) {
        mode = 1;
    } else if (strcmp(format, "text") == 0) {
        mode = 2;
    } else {
        return 1;
    }

    // We open the file
    int fd = open(path, O_RDONLY);
    if (fd < 0) {return 1;}
    
    
    // [Hint 2:] While it is possible to directly read from the file to the circular buffer, the logic is 
    // more complicated. The easiest version is to use a secondary linear buffer, with the same size, to where do read operations. 
    char buffer[bufferSize];
    long long sum = 0;

    // If we work with the binary file
    if (mode == 1) {
        
        // Since each element has a fixed size, reading is more efficient when the buffer size is a multiple of the integer size. 
        // If necessary, you should adjust the buffer size so that only complete elements are read, even if this means using slightly fewer bytes per read.
        int intSize = sizeof(int);
        int usableBytes = bufferSize - (bufferSize % intSize);
        if (usableBytes == 0)
            close(fd);
            return 1;
        int n;

        while ((n = read(fd, buffer, usableBytes)) > 0) {

            /*
             * Interpret the raw bytes as integers.
             * This is safe because we ensured complete elements.
             */
            int *numbers = (int *)buffer;

            int count = n / intSize;

            for (int i = 0; i < count; i++) {
                sum += numbers[i];
            }
        }
            
    }


    // If we work with the text file
    if (mode == 2) {

        CircularBuffer cb;
        buffer_init(&cb, bufferSize);

        ssize_t bytesRead;
        int reachedEOF = 0;

        while (1) {

            /* Read from file into linear buffer */
            bytesRead = read(fd, buffer, bufferSize);

            if (bytesRead == 0)
                reachedEOF = 1;
            if (bytesRead < 0)
                break;

            /* Push read bytes into the circular buffer */
            //for (int i = 0; i < bytesRead; i++) {
               // if (buffer_free_bytes(&cb) > 0) {
                 //   break;
               // }
              //  buffer_push(&cb, buffer[i]);
            //}
            int i = 0;
        while (i < bytesRead) {

    /* If buffer is full, stop pushing */
            if (buffer_free_bytes(&cb) == 0){
                break;
            }
                

            buffer_push(&cb, buffer[i]);
            i++;
}

            /*
             * Extract complete elements (numbers) from the circular buffer.
             * buffer_size_next_element tells us when a full number is available.
             */
            int elemSize;
            while ((elemSize =
                    buffer_size_next_element(&cb, ',', reachedEOF)) != -1) {

                char numberStr[32];
                int idx = 0;

                /* Pop exactly elemSize bytes */
                for (int i = 0; i < elemSize; i++) {
                    unsigned char c = buffer_pop(&cb);

                    /* Ignore delimiter and newline */
                    if (c != ',' && c != '\n') {
                        numberStr[idx++] = c;
                    }
                }

                numberStr[idx] = '\0';

                /* Convert text to integer and add to sum */
                sum += atoll(numberStr);
            }

            if (reachedEOF)
                break;
        }

        buffer_deallocate(&cb);

        /*
        int n;
        while((n = read(fd, buffer, sizeof(buffer))) > 0){
            for (int i = 0; i < n; i++) {
                if (buffer[i] == ',') {
                    string[string_len] = '\0';
                    sscanf(string, "%d", &sum);
                    sum += num;
                    string_len = 0;
                } else {
                    string[string_len++] = buffer[i];
                }
            }
        }
        */
    }

    close(fd);

    // convert sum to string
    char out[32];
    int len = 0;

    long long temp = sum;

    if (temp < 0) {
        out[len++] = '-';
        temp = -temp;
    }

    char rev[32];
    int r = 0;

    if (temp == 0)
        rev[r++] = '0';

    while (temp > 0) {
        rev[r++] = '0' + (temp % 10);
        temp /= 10;
    }

    for (int i = r - 1; i >= 0; i--)
        out[len++] = rev[i];

    out[len++] = '\n';

    write(1, out, len);

    //write(1, &sum, sizeof(sum)); // 1 is standard output
    return 0;
}