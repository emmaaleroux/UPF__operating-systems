#include <sys/wait.h>
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

int main() {
    char line[1024];
    while (fgets(line, sizeof(line), stdin)) {
        line[strcspn(line, "\n")] = '\0';  // remove newline


        if (strcmp(line, "EXIT") == 0) {break;}


        if (strcmp(line, "SINGLE") == 0) {
            // Then it will read the command and arguments in the next line. 
            fgets(line, sizeof(line), stdin);
            line[strcspn(line, "\n")] = '\0';
            char **cmd = split_command(line);
            // It will create a new process using fork(). 
            int pid = fork();
            if (pid == 0) {
                // The child process then replaces its program image by invoking execvp() with the parsed command and its arguments. 
                execvp(cmd[0], cmd);
                exit(1);
            } else {
                waitpid(pid, NULL, 0);
            }
        }


        if (strcmp(line, "PIPED") == 0) {
            // if it is a PIPED execution, it will need to read a second line and create a second process, 
            // as well as creating the pipe and use dup2() to connect both processes before the execvp. 
            fgets(line, sizeof(line), stdin);
            line[strcspn(line, "\n")] = '\0';
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

            fgets(line, sizeof(line), stdin);
            line[strcspn(line, "\n")] = '\0';
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

            fgets(line, sizeof(line), stdin);
            line[strcspn(line, "\n")] = '\0';
            char **cmd = split_command(line);

            int pid = fork();         
            
            if (pid == 0) {
                execvp(cmd[0], cmd);
                exit(1); // becomes zombie --> how do we avoid it?
            }

        }
        
    }
    return 0;

}

// HINTS

// 1. You need to use strcmp for comparing strings. Remember that read will also return the ‘\n’. 
// 2. The command line must be parsed into a NULL-terminated array of strings suitable for use with execvp. 
    // You can do it yourself, or use the given function split_command 
// 3. When reading from the keyboard with a large buffer, each read will return information in a line. 
    // However, when the standard input is redirected to a file you will need to use either a circular buffer, as in last practice to correctly split in lines. 