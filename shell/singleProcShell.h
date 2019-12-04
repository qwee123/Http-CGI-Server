#ifndef SINGLEPROCSHELL_H
#define SINGLEPROCSHELL_H
#include <vector>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <utility>
#include <string>
#include "shellCmd.h"

struct UserPipe{
    int fd[2];
    int send_id;
    int recv_id;
};

class SingleProcShell{

public:
    char* ori_cmd;
    int user_id;
    char username[20];
    int sockfd;
    char ipaddr[INET_ADDRSTRLEN];
    unsigned short int port_num;

    int execShell(char **);
    SingleProcShell(int ,char* [], struct sockaddr_in*,int );
    ~SingleProcShell();

private:
    struct delayPipe{
        int fd[2];
        int delay_count;
    };
    std::vector<delayPipe> pipes;
    std::vector<std::pair<std::string,std::string>> envp; 

    void login();
    void logout();
    void resetEnvp(char **);
    void broadcast(char *);
    pid_t getFork();
    void function_name(char*); 
    bool validName(char*);
    void function_who(); 
    void function_yell(char*);
    void function_tell(char*);
    bool checkUserExist(int);
    int findOutUserPipe(int);
    int findOutpipe(int);
    int checkPipe();
    int checkUserPipe(int);
    void pipeCountDown();
    int mergePipeDelay(int);
    void removePipe(int);
    void removeUserPipe(int);
    void clearUserPipe();
    int createUserPipe(int,int);
    void pipeRedirection(int,int,char,std::vector<char*>*,bool,bool);
    void redirectInpipe(int);
    void redirectUserInpipe(int);
    void redirectOutpipe(int,char);
    void redirectUserOutpipe(int);
    void broadcastUserIn(int);
    void broadcastUserOut(int);
    void execPipeCmd(std::vector<struct Command>*);
    void function_ls(std::vector<char*>*);
    void function_cat(std::vector<char*>*);
    void function_executable(std::vector<char*>*);
    void printEnv(char*);
    void setEnv(char*);

};

extern std::vector<SingleProcShell*> client_shells;
extern std::vector<UserPipe> user_pipes;

#endif
