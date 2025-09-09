/****************
LE2: Introduction to Unnamed Pipes
****************/
#include <unistd.h> // pipe, fork, dup2, execvp, close
#include <sys/wait.h>
#include <iostream>   // std::cout, std::cerr
using namespace std;

int main () {
    // lists all the files in the root directory in the long format
    char* cmd1[] = {(char*) "ls", (char*) "-al", (char*) "/", nullptr};
    // translates all input from lowercase to uppercase
    char* cmd2[] = {(char*) "tr", (char*) "a-z", (char*) "A-Z", nullptr};

    // TODO: add functionality
    // Create pipe
    int fds[2];
    if (pipe(fds) == -1) {
        cerr << "pipe failed\n";
        return 1;
    }

    // Create child to run first command
    pid_t pid = fork();
    if (pid == -1) {
        cerr << "fork failed\n";
        return 1;
    }
    if (pid == 0) {
        // In child, redirect output to write end of pipe
        dup2(fds[1], 1);
        // Close the read end of the pipe on the child side.
        close(fds[0]);
        // In child, execute the command
        execvp(cmd1[0], cmd1);
        return 1;
    }
    else {
        // Create another child to run second command
        pid = fork();
        if (pid == -1) {
            cerr << "fork failed\n";
            return 1;
        }
        if (pid == 0) {
            // In child, redirect input to the read end of the pipe
            // read(fds[0], stdin, sizeof(stdin));
            dup2(fds[0], 0);
            // Close the write end of the pipe on the child side.
            close(fds[1]);
            // Execute the second command.
            execvp(cmd2[0], cmd2);
            return 1;
        } else {
            // Reset the input and output file descriptors of the parent.
            wait(nullptr);
            close(fds[0]);
            close(fds[1]);
        }
        
    }
}
