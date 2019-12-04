#include <iostream>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "shell/singleProcShell.h"
#include "passiveSocket/passiveSocket.h"
#define MAX_CLIENT_NUM 30
using namespace std;

vector<SingleProcShell*> client_shells;
vector<UserPipe> user_pipes;

vector<SingleProcShell*>::iterator searchShell(int fd){
    for(auto itr = client_shells.begin(); itr != client_shells.end(); itr++){
        if((*itr) != NULL && fd == (*itr)->sockfd)    return itr;
    }
}

void switchFd(int newfd){
    dup2(newfd,STDIN_FILENO);
    dup2(newfd,STDOUT_FILENO);
    dup2(newfd,STDERR_FILENO);
}

void restoreFd(int in, int out, int err){
    dup2(out, STDOUT_FILENO);
    dup2(in, STDIN_FILENO);
    dup2(err, STDERR_FILENO);
}

int main(int argc, char** argv, char* envp[]){
    int msockfd;
    socklen_t clilen;
    struct sockaddr_in cli_addr;
    char *service;
    fd_set rfds;
    fd_set afds;
    int alen, fd, nfds;
    int stdout_fd, stdin_fd, stderr_fd;

    stdout_fd = dup(STDOUT_FILENO);
    stdin_fd = dup(STDIN_FILENO);
    stderr_fd = dup(STDERR_FILENO);

    if(argc < 2){
        cerr << "Too few arguments!" << endl;
    	exit(-1);
    }
    else    service = argv[1];

    msockfd = passiveTCP(service, 1);   

    nfds = getdtablesize();
    FD_ZERO(&afds);
    FD_SET(msockfd, &afds);
    client_shells.assign(MAX_CLIENT_NUM,NULL); //set all ptr to NULL    

    for(;;){
        memcpy(&rfds, &afds, sizeof(rfds));

        if(select((nfds), &rfds, NULL, NULL, NULL) < 0){
            cerr << "server: select error!" << endl;
            continue;
        }
 
        if(FD_ISSET(msockfd, &rfds)){
            int newsockfd;
            clilen = sizeof(cli_addr);
            newsockfd = accept(msockfd, (struct sockaddr*)&cli_addr, &clilen);
            if(newsockfd < 0) cerr << "server: cannot accept." << endl; //need exit?
            
            int index = 0;            
            for(;index < MAX_CLIENT_NUM && client_shells[index] != NULL;index++)  ;
            if(index == MAX_CLIENT_NUM){
                send(newsockfd, "Queue FUll!",11,0);
                close(newsockfd);
                continue;
            }            
            cout << "new client! " << index << endl;
            switchFd(newsockfd);
            SingleProcShell *newcli = new SingleProcShell(newsockfd, envp, &cli_addr,index+1);
            client_shells[index] = newcli;
            FD_SET(newsockfd, &afds);
            restoreFd(stdout_fd, stdin_fd, stderr_fd);
        }
        
        for(int fd = 0; fd < nfds; fd++){
            if(fd == msockfd || !FD_ISSET(fd, &rfds))    continue;
           
            switchFd(fd);
            auto cli = searchShell(fd); //iterator point to shell
            int status;
            status = (*cli)->execShell(envp);
            if(status == 1){
                delete (*cli);
                close(fd);
                (*cli) = NULL;
                FD_CLR(fd, &afds);
            }
            restoreFd(stdout_fd, stdin_fd, stderr_fd);
            cout << "Disconnect!" << endl; 
        }
    }
}
