// OPERATING SYSTEMS P101 - LAB 2
// EMMA LEROUX 304174 & GUILLEM ARÉVALO 306124


#include <sys/wait.h>
#include <unistd.h> 
#include <fcntl.h> 
#include <stdlib.h> 
#include <string.h> 
#include <signal.h>
#include "circularBuffer.h"
#include "splitCommand.h"

#define BUFFER_SIZE 1024


// Helper function: reading a full line using the circular buffer as in lab 1
int read_line(int fd, CircularBuffer *cb, char *line, int max_len) {
    static int reachedEOF = 0;
    char linearBuf[BUFFER_SIZE];

    while (1) {
        // Check if a full line is available
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
            return 0;  // buffer is full 
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

    // INPUT SYNTAX
    // (SINGLE | PIPED | CONCURRENT), written in a line, 
    // followed by a command in the next line, with possible CLI arguments separated by single spaces. 
    // In the case of piped commands, then it will have two lines, each one expressing a command.

    signal(SIGCHLD, SIG_IGN); // Signal handling for killing zombies (from concurrent mode)

    // We initialize the circular and linear buffers
    CircularBuffer cb;
    buffer_init(&cb, BUFFER_SIZE);
    char line[BUFFER_SIZE];

    while (read_line(0, &cb, line, BUFFER_SIZE)) {

        // OPTION 1: EXIT
        if (strcmp(line, "EXIT") == 0) {break;}

        // OPTION 2: SINGLE
        if (strcmp(line, "SINGLE") == 0) {
            // If no full line, we exit
            if (!read_line(0, &cb, line, BUFFER_SIZE)) {break;}
            // If there is a full line, we split the command
            char **cmd = split_command(line);
            // Then create a child process to execute it
            pid_t pid = fork();
            if (pid < 0) {
                perror("Fork failed");
                exit(1);
            }
            if (pid == 0) {
                execvp(cmd[0], cmd);
                exit(1);
            } else {
                waitpid(pid, NULL, 0);
            }
        }

        // OPTION 3: PIPED
        if (strcmp(line, "PIPED") == 0) {
            // We read the first command
            if (!read_line(0, &cb, line, BUFFER_SIZE)) {break;}
            char **cmd1 = split_command(line);

            // We create the pipe
            int fd[2];
            pipe(fd);
            if (pipe(fd) < 0) {
                perror("Pipe failed");
                exit(1);
            }
            // First process
            pid_t pid1 = fork();
            if (pid1 < 0) {
                perror("Fork failed");
                exit(1);
            }
            if (pid1 == 0) {
                dup2(fd[1], 1);
                close(fd[0]); 
                close(fd[1]);
                execvp(cmd1[0], cmd1); 
                exit(1);
            } 
            // We read the second command
            if (!read_line(0, &cb, line, BUFFER_SIZE)) {break;}
            char **cmd2 = split_command(line);
            // Second process
            pid_t pid2 = fork();
            if (pid2 < 0) {
                perror("Fork failed");
                exit(1);
            }
            if (pid2 == 0) {
                dup2(fd[0], 0);
                close(fd[1]); 
                close(fd[0]);
                execvp(cmd2[0], cmd2); 
                exit(1);
            }
            // Parent process
            close(fd[0]);
            close(fd[1]);
            // We wait for both children to finish
            waitpid(pid1, NULL, 0);
            waitpid(pid2, NULL, 0);
            
        }

        // OPTION 4: CONCURRENT
        if (strcmp(line, "CONCURRENT") == 0) {
            // Reading and splitting the command
            if (!read_line(STDIN_FILENO, &cb, line, sizeof(line))) { break; }
            char **cmd = split_command(line);
            pid_t pid = fork();
            if (pid < 0) {
                perror("Fork failed");
                exit(1);
            }
            // Child process
            if (pid == 0) {
                execvp(cmd[0], cmd);
                exit(1);
            }
            // The parent process does not wait for it to finish
            // We kill the zombie with signal(SIGCHLD, SIG_IGN); in line 65
        }
    }
    buffer_deallocate(&cb);
    return 0;
}