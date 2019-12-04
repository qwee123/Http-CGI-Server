#ifndef SHELLCMD_H
#define SHELLCMD_H
#include<vector>
#include<string.h>
#include<stdlib.h>
using std::vector;

extern const char *normalDelim;
extern const char *plusErrDelim;

struct Command{
    vector<char* > cmd;
    char pipe_type = 0; 
    char *delay = NULL;
    int user_in_pipe = -1;
    int user_out_pipe = -1;
};

void removeCarrige(char *s);
void parseCmd(char *cmd,vector<struct Command> *parsedc);
bool isUserPipe(char*, struct Command*);

#endif
