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
// Returns 1 if a full line was read, 0 if EOF or error.
int read_line(int fd, CircularBuffer *cb, char *line, int max_len) {
    static int reachedEOF = 0;
    char linearBuf[BUFFER_SIZE];

    while (1) {
        // Check if a full line is available
        int line_size = buffer_size_next_element(cb, '\n', reachedEOF);
        if (line_size > 0) {
            // Prevent overflow if line longer than allowed
            if (line_size > max_len) {
                line_size = max_len;
            }
            // Extract characters from circular buffer into line
            for (int i = 0; i < line_size; i++) {
                line[i] = buffer_pop(cb);
            }
            // Replace newline with string terminator
            line[line_size - 1] = '\0';
            return 1; 
        }
        // If EOF was reached and no full line remains
        if (reachedEOF) {return 0;}   
        // Check how much space is left in circular buffer
        int free = buffer_free_bytes(cb);
        if (free <= 0) {
            return 0;  // Buffer full, cannot read more
        }
        // Read from file descriptor into temporary buffer
        int bytesRead = read(fd, linearBuf, free);
        if (bytesRead == 0) {
            // EOF reached
            reachedEOF = 1;
        } else if (bytesRead < 0) {
            // Read error
            return 0;
        } else {
            // Push read bytes into circular buffer
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

    // Ignore SIGCHLD so terminated background processes
    // (CONCURRENT mode) do not become zombies
    signal(SIGCHLD, SIG_IGN);

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
            pipe(fd); // fd[0] = read end, fd[1] = write end
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
                dup2(fd[1], 1); // Redirect stdout to pipe write end
                // Close unused descriptors
                close(fd[0]);   // Not reading
                close(fd[1]);   // Already duplicated
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
                dup2(fd[0], 0); // Redirect stdin to pipe read end
                // Close unused descriptors
                close(fd[1]); // Not writing
                close(fd[0]); // Already duplicated
                execvp(cmd2[0], cmd2); 
                exit(1);
            }
            // Parent closes both ends
            close(fd[0]);
            close(fd[1]);
            // We wait for both children to finish
            waitpid(pid1, NULL, 0);
            waitpid(pid2, NULL, 0);
            
        }

        // OPTION 4: CONCURRENT
        if (strcmp(line, "CONCURRENT") == 0) { // Execute command without waiting
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