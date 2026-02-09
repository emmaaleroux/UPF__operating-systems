#include <unistd.h> 
#include <fcntl.h> 
#include <stdlib.h> 
#include <string.h> 

#include "circularBuffer.h"
#include "splitCommand.h"


// INPUT SYNTAX
// (SINGLE | PIPED | CONCURRENT), written in a line, 
// followed by a command in the next line, with possible CLI arguments separated by single spaces. 
// In the case of piped commands, then it will have two lines, each one expressing a command.

// INPUT EXAMPLE
    // SINGLE 
    // ls -l /home 
    // PIPE 
    // ps aux  
    // grep root 
    // CONCURRENT 
    // sleep 10 
    // EXIT

int main(int argc, char *argv[]) {
    // GENERAL EXECUTION FLOW
    while (true) {
        // It will read a line, and determine the execution mode. 
        // If it is EXIT it will end the process.

        // Then it will read the command and arguments in the next line. 
        // It will create a new process using fork(). 
        // The child process then replaces its program image by invoking execvp() with the parsed command and its arguments. 

            // if it is a PIPED execution, it will need to read a second line and create a second process, 
            // as well as creating the pipe and use dup2() to connect both processes before the execvp. 

        // If it is not a CONCURRENT execution, use  waitpid() to wait for the previous process (in the case of the piped, you will need to wait for both of them). 
        // Search in the linux documentation how to use waitpid, and how it is slightly different from the wait command seen in class.
    }

}

// HINTS

// 1. You need to use strcmp for comparing strings. Remember that read will also return the ‘\n’. 
// 2. The command line must be parsed into a NULL-terminated array of strings suitable for use with execvp. 
    // You can do it yourself, or use the given function split_command 
// 3. When reading from the keyboard with a large buffer, each read will return information in a line. 
    // However, when the standard input is redirected to a file you will need to use either a circular buffer, as in last practice to correctly split in lines. 