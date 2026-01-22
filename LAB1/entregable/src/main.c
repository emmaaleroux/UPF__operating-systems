#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#include "..\..\circularBuffer.h"

int main(int argc, char *argv[]) {
    if (argc != 4) {return 1;}
    
    char *format = argv[1];
    char *pathToFile = argv[2];
    int sizeOfTheBuffer = (int)argv[3];

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
    int fd = open(pathToFile, O_RDONLY);

    char buffer[sizeOfTheBuffer];
    // [Hint 2:] While it is possible to directly read from the file to the circular buffer, the logic is 
    // more complicated. The easiest version is to use a secondary linear buffer, with the same size, to where do read operations. 
    int sum = 0;

    // If we work with the binary file
    if (mode == 1) {
        // Since each element has a fixed size, reading is more efficient when the buffer size is a multiple of the integer size. 
        // If necessary, you should adjust the buffer size so that only complete elements are read, even if this means using slightly fewer bytes per read.
    }


    // If we work with the text file
    if (mode == 2) {

    }

    close(fd);
    write(1, sum, sizeof(sum)); // 1 is standard output
    return 0;
}