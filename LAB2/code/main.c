#include <sys/wait.h>
#include <unistd.h> 
#include <fcntl.h> 
#include <stdlib.h> 
#include <string.h> 
#include <signal.h>

#include "circularBuffer.h"
#include "splitCommand.h"

#define BUFFER_SIZE 1024

// INPUT SYNTAX
// (SINGLE | PIPED | CONCURRENT), written in a line, 
// followed by a command in the next line, with possible CLI arguments separated by single spaces. 
// In the case of piped commands, then it will have two lines, each one expressing a command.

// INPUT EXAMPLE
    // SINGLE 
    // ls -l /home 
    // PIPED 
    // ps aux  
    // grep root 
    // CONCURRENT 
    // sleep 10 
    // EXIT

// Helper function: reading a full line using the circular buffer
int read_line(int fd, CircularBuffer *cb, char *line, int max_len) {
    static int reachedEOF = 0;
    char linearBuf[BUFFER_SIZE];

    while (1) {

        // Check if a full line is already available
        int line_size = buffer_size_next_element(cb, '\n', reachedEOF);
        if (line_size > 0) {

            if (line_size > max_len)
                line_size = max_len;

            for (int i = 0; i < line_size; i++)
                line[i] = buffer_pop(cb);

            line[line_size - 1] = '\0';  // remove newline
            return 1; 
        }

        if (reachedEOF) {return 0;}  // no more input

        int free = buffer_free_bytes(cb);
        if (free <= 0) {
            return 0;  // buffer full 
        }
        int bytesRead = read(fd, linearBuf, free);

        if (bytesRead == 0) {
            reachedEOF = 1;
        } else if (bytesRead < 0) {
            return 0;
        } else {
            for (int i = 0; i < bytesRead; i++) {
                buffer_push(cb, linearBuf[i]);
            }
        }
    }
}


int main() {

    CircularBuffer cb;
    buffer_init(&cb, BUFFER_SIZE);

    signal(SIGCHLD, SIG_IGN); // Signal handling for killing zombies

    char line[BUFFER_SIZE];

    while (read_line(0, &cb, line, BUFFER_SIZE)) {

        if (strcmp(line, "EXIT") == 0) {break;}

        if (strcmp(line, "SINGLE") == 0) {
            
            if (!read_line(0, &cb, line, BUFFER_SIZE)) {break;}             line[strcspn(line, "\n")] = '\0';
            char **cmd = split_command(line);

            int pid = fork();
            if (pid == 0) {
                execvp(cmd[0], cmd);
                exit(1);
            } else {
                waitpid(pid, NULL, 0);
            }
        }


        if (strcmp(line, "PIPED") == 0) {
            // if it is a PIPED execution, it will need to read a second line and create a second process, 
            // as well as creating the pipe and use dup2() to connect both processes before the execvp. 
            
            if (!read_line(0, &cb, line, BUFFER_SIZE)) {break;}
            char **cmd1 = split_command(line);

            // We create the pipe before the 2 fork()
            int fd[2];
            pipe(fd);

            int pid1 = fork();
            if (pid1 == 0) {
                dup2(fd[1], STDOUT_FILENO);
                close(fd[0]); 
                close(fd[1]);
                execvp(cmd1[0], cmd1); 
                exit(1);
            } 

            if (!read_line(0, &cb, line, BUFFER_SIZE)) {break;}
            char **cmd2 = split_command(line);

            int pid2 = fork();
            if (pid2 == 0) {
                dup2(fd[0], STDIN_FILENO);
                close(fd[1]); 
                close(fd[0]);
                execvp(cmd2[0], cmd2); 
                exit(1);
            }

            close(fd[0]);
            close(fd[1]);
            // If it is not a CONCURRENT execution, use waitpid() to wait for the previous process 
            // (in the case of the piped, you will need to wait for both of them).
            waitpid(pid1, NULL, 0);
            waitpid(pid2, NULL, 0);
            
        }
        if (strcmp(line, "CONCURRENT") == 0) {

            if (!read_line(STDIN_FILENO, &cb, line, sizeof(line)))
                break;

            char **cmd = split_command(line);

            int pid = fork();
            if (pid == 0) {
                execvp(cmd[0], cmd);
                exit(1);
            }
        }
        
    }
    buffer_deallocate(&cb);
    return 0;
}

// HINTS

// 1. You need to use strcmp for comparing strings. Remember that read will also return the ‘\n’. 
// 2. The command line must be parsed into a NULL-terminated array of strings suitable for use with execvp. 
    // You can do it yourself, or use the given function split_command 
// 3. When reading from the keyboard with a large buffer, each read will return information in a line. 
    // However, when the standard input is redirected to a file you will need to use either a circular buffer, as in last practice to correctly split in lines. 