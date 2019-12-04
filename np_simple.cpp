#include <iostream>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "shell/np_shell.h"
#include "passiveSocket/passiveSocket.h"
using namespace std;

int main(int argc, char** argv, char* envp[]){
    int msockfd, newsockfd, childpid;
    socklen_t clilen;
    struct sockaddr_in cli_addr;
    char *service;

    if(argc < 2){
        cerr << "Too few arguments!" << endl;
    	exit(-1);
    }
    else    service = argv[1];

    //initialize();
    msockfd = passiveTCP(service, 1);   

    int stdout_fd = dup(STDOUT_FILENO);
    int stdin_fd = dup(STDIN_FILENO);
    int stderr_fd = dup(STDERR_FILENO);
    for(;;){
        clilen = sizeof(cli_addr);
        newsockfd = accept(msockfd, (struct sockaddr*)&cli_addr, &clilen);
        if(newsockfd < 0)  cerr << "server: cannot accept." << endl;
        
        dup2(newsockfd, STDIN_FILENO);
        dup2(newsockfd, STDOUT_FILENO);
        dup2(newsockfd, STDERR_FILENO);         
        execShell(envp);

        dup2(stdin_fd, STDIN_FILENO);
        dup2(stdout_fd, STDOUT_FILENO);
        dup2(stderr_fd, STDERR_FILENO);
        close(newsockfd);
        cout << "Client disconnected!" << endl;
    }
}
