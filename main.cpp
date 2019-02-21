//
//  main.cpp
//  shell
//
//  Created by Natalie Wintour on 2/8/19.
//  Copyright © 2019 Natalie Wintour. All rights reserved.
//
#include "shelpers.hpp"

#include <cstring>
#include <iostream>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

using namespace std;

// close fds between fork and exec

int main(int argc, const char * argv[]) {
    //    int buf;
    string line;
  // cout << "Shell running." << endl; old debug 
    printPrompt();
    while(getline(cin, line)) {
        if (line == "exit" || line == "Exit" || line == "logout" || line == "Logout") {
            exit(0);
        } else if(line == "cd") {
            if(chdir(getenv("HOME")) != 0) {
                perror("cd error");
            }
        } else {
            vector<string> tokens = tokenize(line);
            vector<Command> commands = getCommands(tokens);
            vector<pid_t> childPids;
            
            for(int i = 0; i < commands.size(); ++i) {
                pid_t p = fork();
                if (p < 0) {
                    perror("Fork failure.");
                    exit(EXIT_FAILURE);
                } else if (p == 0) { // child

                    if(commands[i].fdStdin != 0) {
                        dup2(commands[i].fdStdin, 0);
                    } else if(commands[i].fdStdout != 1) {
                        dup2(commands[i].fdStdout, 1);
                    }
                    
                    for(int k = 0; k < commands.size(); ++k) {
                        if(commands[k].fdStdin != 0) {
                            close(commands[k].fdStdin);
                        } else if(commands[k].fdStdout != 1) {
                            close(commands[k].fdStdout);
                        }
                    }
                    
                    if (execvp(commands[i].exec.c_str(), const_cast<char**>(commands[i].argv.data())) < 0) {
                        perror("Exec failure.");
                        exit(EXIT_FAILURE);
                    }
                    
                } else { // parent
                    childPids.push_back(p);
                    // wait(NULL);
                }
            }
            for(int k = 0; k < commands.size(); ++k) {
                if(commands[k].fdStdin != 0) {
                    close(commands[k].fdStdin);
                } else if(commands[k].fdStdout != 1) {
                    close(commands[k].fdStdout);
                }
            }
            for(int j = 0; j < childPids.size(); ++j) {
                waitpid(childPids.at(j), NULL, 0);
            }
        }
        printPrompt();
    }
}
