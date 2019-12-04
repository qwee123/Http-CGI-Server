#ifndef MULTIPROCSHELL_H
#define MULTIPROCSHELL_H
#include <vector>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "shellCmd.h"

extern int getShmid(int);
extern struct user* getShmptr(int); 

struct user{
    int uid;
    int pid;
    char username[20];
    char ipaddr[INET_ADDRSTRLEN];
    unsigned short int port_num; 
};

struct MessageInfo{
    char type[10];
    int send_id;
    int recv_id;
};

struct RDFIFO{
    int fd;
    int send_id;
};

void safeClose(int);
void clearRDFIFO(std::vector<struct RDFIFO>::iterator);
void bsHandler();
void customedHandler(int);
void addFIFO(int);
void initialize(char**);
pid_t getFork();
int execShell(int,char* []);
void function_who();
void function_name(char*);
void function_yell(char*);
void function_tell(char*);
bool checkUserExist(int, struct user*);
bool validName(char*, struct user*);
void broadcast(char *str,struct MessageInfo*);
void login();
void logout();
int findOutpipe(int);
int findOutUserPipe(int);
int checkPipe();
int checkUserPipe(int);
void pipeCountDown();
int mergePipeDelay(int);
void removePipe(int);
void pipeRedirection(int,int,char,std::vector<char*>*,bool,bool,char*);
void redirectInpipe(int);
void redirectUserInpipe(int);
void redirectOutpipe(int,char);
void redirectUserOutpipe(int,char*);
void broadcastUserOut(int, char *);
void broadcastUserIn(int, char *);
void broadcastUserInOut(int, int, char*);
void userPipeBroadcast(int, int, bool, bool,char*);
void execPipeCmd(std::vector<struct Command>*, char*);
void function_ls(std::vector<char*>*);
void function_cat(std::vector<char*>*);
void function_executable(std::vector<char*>*);
void printEnv(char*, char**);
void setEnv(char*);

struct delayPipe{
    int fd[2];
    int delay_count;
};

#endif
