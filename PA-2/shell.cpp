#include <iostream>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <vector>
#include <string>

#include "Tokenizer.h"

// all the basic colours for a shell prompt. Note: ANSI color codes
#define PURPLE  "\033[1;35m" // my favorite color
#define RED     "\033[1;31m"
#define GREEN	"\033[1;32m"
#define YELLOW  "\033[1;33m"
#define BLUE	"\033[1;34m"
#define WHITE	"\033[1;37m"
#define NC      "\033[0m"

// named constants
#define MAX_PATH_SIZE 256
// #define MAX_COMMANDS 32

using namespace std;

int main () {
    for (;;) {
        // need date/time, username, and absolute path to current dir
        time_t currTime = time(nullptr);
        string currTimeStr = ctime(&currTime);
        currTimeStr.pop_back(); // remove null terminate so it doesn't output needless line break
        // reomve the year from the string
        currTimeStr.pop_back();
        currTimeStr.pop_back();
        currTimeStr.pop_back();
        currTimeStr.pop_back();

        // absolute file path
        char currDirectory[MAX_PATH_SIZE];
        getcwd(currDirectory, MAX_PATH_SIZE);

        // prompt user for input
        cout << PURPLE << currTimeStr << getenv("USER") << ":" << BLUE << currDirectory << NC << "$" << " ";
        
        // get user inputted command
        string input;
        getline(cin, input);

        if (input == "exit") {  // print exit message and break out of infinite loop
            cout << RED << "Now exiting shell..." << endl << "Goodbye" << NC << endl;
            break;
        }

        // get tokenized commands from user input
        Tokenizer tknr(input);
        if (tknr.hasError()) {  // continue to next prompt if input had an error
            continue;
        }

        // // print out every command token-by-token on individual lines
        // // prints to cerr to avoid influencing autograder
        for (auto cmd : tknr.commands) {
            for (auto str : cmd->args) {
                cerr << "|" << str << "| ";
            }
            if (cmd->hasInput()) {
                cerr << "in< " << cmd->in_file << " ";
            }
            if (cmd->hasOutput()) {
                cerr << "out> " << cmd->out_file << " ";
            }
            cerr << endl;
        }

        // fork to create child
        pid_t pid = fork();
        if (pid < 0) {  // error check
            perror("fork");
            exit(2);
        }

        if (pid == 0) {  // if child, exec to run command
            // run single commands with arguments
            //TODO: this needs to be able to handle commands with any # of arguments
            long unsigned int cmdArrayLength = 1; 
            for (long unsigned int i = 0; i < tknr.commands.size() ; ++i) {
                cmdArrayLength += tknr.commands.at(i)->args.size();
            }
            cout << cmdArrayLength << endl;
            char** args = new char*[cmdArrayLength];
            for (long unsigned int i = 0; i < cmdArrayLength - 1; ++i) {
                args[i] = (char*) tknr.commands.at(0)->args.at(i).c_str();
            }
            args[cmdArrayLength - 1] = nullptr;
            // for (long unsigned int i = 0; i < cmdArrayLength - 1; ++i) {
            //     cout << "args[" << i << "] = " << *args[i];
            // }

            

            // run single commands with no arguments
            // char* args[] = {(char*) tknr.commands.at(0)->args.at(0).c_str(), nullptr};

            if (execvp(args[0], args) < 0) {  // error check
                perror("execvp");
                exit(2);
            }
        }
        else {  // if parent, wait for child to finish
            int status = 0;
            waitpid(pid, &status, 0);
            if (status > 1) {  // exit if child didn't exec properly
                exit(status);
            }
        }
    }
}
