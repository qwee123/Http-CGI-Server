#include <iostream>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include "shell/multiProcShell.h"
#include "passiveSocket/passiveSocket.h"
#define MAX_USER 30
#define INFO_ID 10
#define BS_ID 11
#define FIFO_ID 12
using namespace std;

extern int errno;

int getShmid(int);
struct user* getShmptr(int);
void clearShm();

void signalHandler(int signo){
    if(SIGINT == signo){
        clearShm();
        cout << "Exit" << endl;
        exit(0);
    }
    else if(SIGCHLD == signo){
        int pid,status;
        while((pid = waitpid(-1,&status,WNOHANG) > 0))   ;
    }
}

int getShmid(int id){
    int shmid;
    key_t key = ftok(".",id);
    if((shmid = shmget(key, 0, IPC_EXCL|0666)) < 0){
        cerr << "Cannot locate shared memory!" << endl;
        return -1;
    }
    return shmid;
}

struct user* getShmptr(int id){
    int shmid;
    if((shmid = getShmid(id)) == -1)    return NULL;

    struct user* shmptr;
    if((shmptr = (struct user *)shmat(shmid,NULL,0)) == NULL){
        cerr << "shmat error: " << strerror(errno) << endl;
        return NULL;    
    }   
    return shmptr;
}

void clearShm(){
    int shmid; 
    if((shmid = getShmid(INFO_ID)) > 0){  //clear user info
        if((shmctl(shmid, IPC_RMID, 0)) < 0)    cerr << "shmctl error: " << strerror(errno) << endl;
    }

    if((shmid = getShmid(BS_ID)) > 0){  //clear message
        if((shmctl(shmid, IPC_RMID, 0)) < 0)    cerr << "shmctl error: " << strerror(errno) << endl;
    }

    if((shmid = getShmid(FIFO_ID)) > 0){  //clear FIFO_mesg
        if((shmctl(shmid, IPC_RMID, 0)) < 0)    cerr << "shmctl error: " << strerror(errno) << endl;
    }
    return ;
}

int addUser(int pid,struct sockaddr_in *cli_addr){
    key_t key = ftok(".",INFO_ID);
    int shmid;
    if((shmid = shmget(key,0,IPC_EXCL|0666)) == -1){
        cerr << "shmget error: " << strerror(errno) << endl;
        exit(-1);
    }

    struct user *shmptr;
    if((shmptr = (struct user *)shmat(shmid,NULL,0)) == NULL){
        cerr << "shmat error: " << strerror(errno) << endl;
        exit(-1);    
    }

    int index = 0;
    for(;index < MAX_USER && shmptr[index].uid != 0;index++) ;
    if(index == MAX_USER){
        cerr << "Room is full, please try later!" << endl;
        shmdt(shmptr);
        return -1;
    }
    struct user &client = shmptr[index];
    client.uid = index+1;
    client.pid = pid;
    strcpy(client.username,"(no name)");
    inet_ntop(AF_INET, &((*cli_addr).sin_addr), client.ipaddr, INET_ADDRSTRLEN);
    client.port_num = ntohs((*cli_addr).sin_port);
    
    shmdt(shmptr);
    return index+1;
}

void createShareMem(key_t key,size_t size){
    int shmid;
    void *shmptr;
    if((shmid = shmget(key, size, IPC_CREAT|0666)) < 0){
        cerr << "Cannot locate shared memory!" << endl;
        exit(-1);
    }
    if((shmptr = (void *)shmat(shmid,NULL,0)) == NULL){
        cerr << "shmat error: " << strerror(errno) << endl;
        exit(-1);
    }
    memset(shmptr,0,size); 
    shmdt(shmptr);  
}

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

    key_t key = ftok(".",INFO_ID);
    createShareMem(key,sizeof(struct user)*MAX_USER);  //create memory for user_info
    key = ftok(".",BS_ID);
    createShareMem(key,1024);  //create memory for message
    key = ftok(".",FIFO_ID);
    createShareMem(key,sizeof(struct MessageInfo)+1);

    msockfd = passiveTCP(service, 1);   

    signal(SIGCHLD, signalHandler);
    signal(SIGINT, signalHandler);
    for(;;){
        clilen = sizeof(cli_addr);
        newsockfd = accept(msockfd, (struct sockaddr*)&cli_addr, &clilen);
        if(newsockfd < 0){
            cerr << "server: cannot accept." << endl;
            sleep(1);
            continue;
        }

        //fork until success
        while((childpid = fork()) < 0){
            cout << "Cannot fork...retry!" << endl;
            sleep(2);
        }

        if(childpid == 0){
            int uid;
            if((uid = addUser(getpid(),&cli_addr)) < 1){ //Room is full
                close(newsockfd);
                exit(0);
            }
            cout << "new Client! " << uid << endl;

            dup2(newsockfd,STDIN_FILENO);
            dup2(newsockfd,STDOUT_FILENO);
            dup2(newsockfd,STDERR_FILENO);
            close(newsockfd);
            execShell(uid,envp);
            exit(0);
        }
        else{
            close(newsockfd);
        }
    }

}
