#ifndef NP_SHELL_H
#define NP_SHELL_H
#include <vector>
#include <sys/types.h>
#include <unistd.h>
#include "shellCmd.h"

pid_t getFork();
int execShell(char* []);
int findOutpipe(int);
int checkPipe();
void pipeCountDown();
int mergePipeDelay(int);
void removePipe(int);
void pipeRedirection(int,int,char,std::vector<char*>*);
void redirectInpipe(int);
void redirectOutpipe(int,char);
void execPipeCmd(std::vector<struct Command>*);
void function_ls(std::vector<char*>*);
void function_cat(std::vector<char*>*);
void function_executable(std::vector<char*>*);
void printEnv(std::vector<char*>*,char**);
void setEnv(std::vector<char*>*);

struct delayPipe{
    int fd[2];
    int delay_count;
};

#endif
