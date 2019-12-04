#include <iostream>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <wait.h>
#include <signal.h>
#include "np_shell.h"
using namespace std;

vector<delayPipe> pipes;

void childHandler(int signo){
    int status;
    int pid;
    while((pid = waitpid(-1,&status,WNOHANG)) > 0) ;
}

void initialize(){
    unsetenv("PATH");
    setenv("PATH","bin:.",1);
}

int execShell(char* envp[]){
    initialize();    
    signal(SIGCHLD,childHandler);
    bool exitflag = false;   
    char *cmd = new char[15000];
    vector<struct Command> parsed_cmd;

    while(!exitflag){
        cout << "% "<< flush;
        cin.getline(cmd,15000);
	parseCmd(cmd,&parsed_cmd);
	if((int)parsed_cmd.size() < 1)	continue;

	if(!strcmp(parsed_cmd[0].cmd[0],"exit"))
	    exitflag = true;
	else if(!strcmp(parsed_cmd[0].cmd[0],"printenv"))
	    printEnv(&parsed_cmd[0].cmd,envp);
	else if(!strcmp(parsed_cmd[0].cmd[0],"setenv"))
	    setEnv(&parsed_cmd[0].cmd);
	else{
	    execPipeCmd(&parsed_cmd);
	}
	parsed_cmd.clear();
    }
    return 0;
}

int checkPipe(){
    for(int i = 0;i < pipes.size();i++){
 	if(pipes[i].delay_count == 0)	return i;
    }
    return -1;
}

int mergePipeDelay(int delay){
    for(int i = 0;i < pipes.size();i++)
	if(pipes[i].delay_count == delay)    return i;
    return -1;
}

void pipeCountDown(){
    for(int i = 0;i < pipes.size();i++)
	pipes[i].delay_count--;
}

void removePipe(int index){
    vector<delayPipe>::iterator itr = pipes.begin() + index;
    pipes.erase(itr,itr+1);
}

int findOutpipe(int delay_int){
    int out_pipe = mergePipeDelay(delay_int); //-1 means need to push new delayPipe, other return values mean there is already one in vector.
    if(out_pipe == -1){ //out_pipe should >= -1 in normal
    	int pipefd[2];
	if(pipe(pipefd) < 0){
            cerr << "Cannot pipe()!" <<endl;
	    exit(-1);
	}
	struct delayPipe p = {.fd = {pipefd[0],pipefd[1]}, .delay_count = delay_int};
	out_pipe = pipes.size();
	pipes.push_back(p);
    }
    return out_pipe;	
}

void redirectInpipe(int in_pipe){
    close(pipes[in_pipe].fd[1]);
    dup2(pipes[in_pipe].fd[0],STDIN_FILENO);
    close(pipes[in_pipe].fd[0]);
}

void redirectOutpipe(int out_pipe,char pipeType){
    close(pipes[out_pipe].fd[0]);
    dup2(pipes[out_pipe].fd[1],STDOUT_FILENO);
    if(pipeType == *plusErrDelim)
        dup2(pipes[out_pipe].fd[1],STDERR_FILENO);
    close(pipes[out_pipe].fd[1]);
}

pid_t getFork(){
    pid_t pid = fork();
    int retry = 0;
    while(pid < 0){
        if((++retry) > 20){
	    cerr << "Cannot fork!!" << endl;
	    exit(-1);
	}
	sleep(1);
	pid = fork();
    }
    return pid;
}

void pipeRedirection(int in_pipe, int out_pipe, char pipeType,vector<char*> *cmd){
    if(in_pipe > -1)
	redirectInpipe(in_pipe);
    if(out_pipe > -1)
	redirectOutpipe(out_pipe,pipeType);

    int cmd_len = (*cmd).size();  //*cmd.cmd is a vector<char*>
    if(cmd_len>2 && !strcmp((*cmd)[cmd_len-2],">")){
    	int fd = open((*cmd).back(),O_RDWR|O_CREAT|O_TRUNC,S_IWUSR|S_IRUSR);
	dup2(fd,STDOUT_FILENO);
	close(fd);
	(*cmd).pop_back();
	(*cmd).pop_back();
    }
    (*cmd).push_back(NULL);//exec() based function need a NULL at the end of argv
}

void execPipeCmd(vector<struct Command> *parsed_cmd){
    int pipe_num = (*parsed_cmd).size(); 
    int status;

    for(int i = 0;i < pipe_num;i++){
	pipeCountDown();

    	int out_pipe;
	char *delay = (*parsed_cmd)[i].delay;
	if(!(i == pipe_num-1&&delay == NULL)){
            int delay_int = delay == NULL?1:atoi(delay);
            out_pipe = findOutpipe(delay_int);
	}
        else  //output directly
            out_pipe = -1;
   
        // find if the pipe is '|','!' or NULL
	char pipeType = (*parsed_cmd)[i].pipe_type;

	int in_pipe = checkPipe(); //check if this command is piped. return pipe index, or -1(no pipe)
	pid_t pid = getFork(); // fork until success,or exit if cannot fork after 20 retries.
        
	if(pid == 0){
            pipeRedirection(in_pipe,out_pipe,pipeType,&((*parsed_cmd)[i].cmd));
	    if(!strcmp((*parsed_cmd)[i].cmd[0],"ls")){
		function_ls(&((*parsed_cmd)[i]).cmd);
	    }
	    else if(!strcmp((*parsed_cmd)[i].cmd[0],"cat")){
		function_cat(&((*parsed_cmd)[i]).cmd);
	    }
	    else{
		function_executable(&((*parsed_cmd)[i].cmd));
	    }
	    cout << "Weird! Why are u seeing this!?" <<endl;
	    exit(0);
        }
	else{
	    if(in_pipe > -1){
		close(pipes[in_pipe].fd[0]);
		close(pipes[in_pipe].fd[1]);
		removePipe(in_pipe);
	    }
	    //wait(NULL);
	    if(i == pipe_num-1&&delay == NULL)
	        waitpid(pid,&status,0);
	}
    }    
}

void printEnv(vector<char*> *args,char **envp){
    char *var;
    pipeCountDown();
    if((*args).size() > 1){
	if((var = getenv((*args)[1])) != NULL)
    	    cout << var << endl;
    }
    else{
    	for(char **ptr = envp;*ptr != 0; ptr++)
	    cout << (*ptr) <<endl;
    }   
}

void setEnv(vector<char*> *args){
    pipeCountDown();
    if((*args).size() < 3)    return;
    setenv((*args)[1],(*args)[2],1);
}

void function_ls(vector<char*> *args){
    if(execvp("ls",(*args).data())<0){
    	cerr << "Unknown command: [" <<(*args)[0] << "]." << endl;
    	exit(-1);
    }
}

void function_cat(vector<char*> *args){    
    if(execvp("cat",(*args).data()) < 0){
        cerr << "Unknown command: [" <<(*args)[0] << "]." << endl;
    	exit(-1);
    }
}

void function_executable(vector<char*> *args){    
    cout << flush;
    int result = execvp((*args)[0],(*args).data());
    if(result == -1){
        cerr << "Unknown command: [" << (*args)[0] << "]." << endl;
    	exit(0);
    }
    else if(result < -1){
    	cerr << "Error with executable!" <<endl;
    }
}
