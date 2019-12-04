#include <iostream>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <wait.h>
#include <signal.h>
#include <stdlib.h>
#include "singleProcShell.h"
using namespace std;

void childHandler(int signo){
    int status;
    int pid;
    while((pid = waitpid(-1,&status,WNOHANG)) > 0) ;
}

SingleProcShell::SingleProcShell(int sock, char **ori_envp, struct sockaddr_in *sockinfo,int id){
    ori_cmd = new char[15000]; //for pipe message usage.
    user_id = id;
    sockfd = sock;
    envp.push_back(pair<string,string>("PATH","bin:."));
    resetEnvp(ori_envp);
    inet_ntop(AF_INET,&((*sockinfo).sin_addr), ipaddr, INET_ADDRSTRLEN);
    port_num = ntohs((*sockinfo).sin_port);
   
    login();

    strcpy(username,"(no name)");
    char* broadstr = new char[80];
    sprintf(broadstr,"*** User '%s' entered from %s:%hu. ***\n",username,ipaddr,port_num);
    cout << broadstr; //The instance havn't been pushed into the vector yet, therefore need self-output the broadcast message!
    broadcast(broadstr);

    signal(SIGCHLD,childHandler);
    cout << "% " << flush;  
    delete broadstr;  
}

SingleProcShell::~SingleProcShell(){
    logout();
    clearUserPipe();
    vector<pair<string, string>>().swap(envp);
    vector<delayPipe>().swap(pipes);
}

int SingleProcShell::execShell(char **ori_envp){
    resetEnvp(ori_envp);
 
    char *first_word = new char[50];
    memset(first_word,0,50);
    char *cmd = new char[15000];
    ori_cmd = new char[15000]; //for pipe message usage. 
    memset(this->ori_cmd,0,15000);    
    vector<struct Command> parsed_cmd;
    
    cin.getline(cmd,15000);
    strncpy(ori_cmd, cmd, 15000);
    
    if(cmd[strlen(cmd)-1] == '\r')    cmd[strlen(cmd)-1] = '\0';
    char *first_white = strchr(cmd,' ');
    if(first_white != NULL)    strncpy(first_word, cmd, first_white - cmd); 
    else    strncpy(first_word, cmd,50);   

    if(strlen(first_word) > 0){
        if(!strcmp(first_word,"exit"))
            return 1; 
        else if(!strcmp(first_word,"printenv"))
            printEnv(first_white);
        else if(!strcmp(first_word,"setenv"))
            setEnv(first_white);
        else if(!strcmp(first_word,"name"))
            function_name(first_white);
        else if(!strcmp(first_word,"who"))
            function_who();
        else if(!strcmp(first_word,"yell"))
            function_yell(first_white);
        else if(!strcmp(first_word,"tell"))
            function_tell(first_white);
        else{
            parseCmd(cmd, &parsed_cmd);
            if((int)parsed_cmd.size() > 0)    execPipeCmd(&parsed_cmd);
        }
    }
    else{
        parseCmd(cmd, &parsed_cmd);
        if((int)parsed_cmd.size() > 0)    execPipeCmd(&parsed_cmd);
    }

    cout << "% " << flush; //succesfully complete commands
    
    vector<struct Command>().swap(parsed_cmd);
    delete this->ori_cmd;
    delete cmd;
    delete first_word;
    return 0;
}

void SingleProcShell::broadcast(char *str){
    for(auto itr = client_shells.begin();itr != client_shells.end(); itr++){
        if((*itr) == NULL)	continue;
        if(send((*itr)->sockfd, (const void*)str, strlen(str), 0) < 0)    cerr << "Broadcast failed on "<< (*itr)->username << endl;
    }
}

void SingleProcShell::function_name(char *cmd){
    pipeCountDown();
    if(cmd != NULL && validName(cmd)){
        strncpy(this->username, cmd+1, 20);
        char *bs = new char[80];
        sprintf(bs, "*** User from %s:%hu is named '%s'. ***\n", this->ipaddr, this->port_num, this->username);
        broadcast(bs);
        delete bs;
    }
}

bool SingleProcShell::validName(char *name){
    for(auto itr = client_shells.begin();itr != client_shells.end();itr++){
        if((*itr) == NULL)    continue;
        
        if(!strcmp(name+1,(*itr)->username)){
            cout << "*** User '" << name << "' already exists. ***" << endl;
            return false;
       }     
    }
    return true;
}

void SingleProcShell::function_who(){
    pipeCountDown();
    cout << "<ID>	<nickname>	<IP:port>	<indicate me>" << endl;
    for(auto itr = client_shells.begin();itr != client_shells.end();itr++){
        if((*itr) == NULL)    continue;
        cout << (*itr)->user_id <<"	"<< (*itr)->username <<"	"
          << (*itr)->ipaddr <<":"<< (*itr)->port_num;

        if((*itr)->user_id == user_id)    cout << "	<-me";
        cout << endl;
    }

}

void SingleProcShell::function_yell(char *cmd){
    pipeCountDown();
    char *broadstr = new char[1100];
    sprintf(broadstr, "*** %s yelled ***: %s\n", this->username, cmd+1);
    broadcast(broadstr);
    delete broadstr;
}

void SingleProcShell::function_tell(char *cmd){
    pipeCountDown();
    if(cmd != NULL){
        int recv_id = atoi(strtok(cmd+1," "));
        char *content = cmd + strlen(cmd) + 1;
        if((*content) != 0 && checkUserExist(recv_id)){
            int recv_sockfd = client_shells[recv_id-1]->sockfd; //user_id is related to the index in vector            
            char *ms = new char[1100];
            sprintf(ms, "*** %s told you ***: %s\n", this->username, content); 
            if((send(recv_sockfd, (const void*)ms, strlen(ms), 0) < 0))    cerr << "Tell Falied!!" << endl;
            delete ms;
        }
        else{
            cerr << "*** Error: user #" << recv_id << " does not exist yet. ***" << endl;
        }
    }
}

bool SingleProcShell::checkUserExist(int id){  //id is related to the index in vector
    if(id > 0)    return client_shells[id-1] != NULL;
    else false;
}

void SingleProcShell::resetEnvp(char **ori_envp){
    for(char **ptr = ori_envp;*ptr != 0; ptr++){
	unsetenv(*ptr); 
    }
    for(auto itr = envp.begin();itr != envp.end();itr++){
        setenv((*itr).first.c_str(),(*itr).second.c_str(),1);
    }
}

void SingleProcShell::login(){
    cout << "****************************************" << endl;
    cout << "** Welcome to the information server. **" << endl;
    cout << "****************************************" << endl;
    
}

void SingleProcShell::logout(){
    char *bs = new char[40];
    sprintf(bs, "*** User '%s' left. ***\n", this->username);
    broadcast(bs);
    delete bs;
}

int SingleProcShell::checkPipe(){
    for(int i = 0;i < pipes.size();i++){
 	if(pipes[i].delay_count == 0)	return i;
    }
    return -1;
}

int SingleProcShell::checkUserPipe(int sender_id){
    if(!checkUserExist(sender_id)){
        cerr << "*** Error: user #" << sender_id << " does not exist yet. ***" << endl;
        return -1;
    }

    for(int index = 0;index < user_pipes.size(); index++){
        if(user_pipes[index].send_id == sender_id && user_pipes[index].recv_id == this->user_id){
            return index;
        }
    }
    cerr << "*** Error: the pipe #" << sender_id <<"->#"<<this->user_id << " does not exist yet. ***" << endl;
    return -1;
}

int SingleProcShell::mergePipeDelay(int delay){
    for(int i = 0;i < pipes.size();i++)
	if(pipes[i].delay_count == delay)    return i;
    return -1;
}

void SingleProcShell::pipeCountDown(){
    for(int i = 0;i < pipes.size();i++)
	pipes[i].delay_count--;
}

void SingleProcShell::removePipe(int index){
    vector<delayPipe>::iterator itr = pipes.begin() + index;
    pipes.erase(itr,itr+1);
}

void SingleProcShell::removeUserPipe(int index){
    vector<UserPipe>::iterator itr = user_pipes.begin() + index;
    user_pipes.erase(itr,itr+1);
}

void SingleProcShell::clearUserPipe(){
    for(int index = 0;index < user_pipes.size(); index++){
        if(user_pipes[index].send_id == this->user_id || user_pipes[index].recv_id == this->user_id){
            pid_t pid= fork();
            if(pid == 0){
                close(user_pipes[index].fd[1]);
                dup2(user_pipes[index].fd[0],STDIN_FILENO);
                int fd = open("/dev/null",O_RDWR);
                dup2(fd,STDOUT_FILENO);
                close(fd);

                if(execlp("./bin/cat","./bin/cat",NULL) < 0){
                    cerr << "Truncate error!" << endl;
                    exit(-1);
                }
            }
            else{ 
                close(user_pipes[index].fd[0]);
                close(user_pipes[index].fd[1]);   
                removeUserPipe(index);
                int status;
                waitpid(pid,&status,0);
                index--;  //removeUserPipe will elimate the current pipe
            }       
        }
    }
}

int SingleProcShell::findOutpipe(int delay_int){
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

void SingleProcShell::redirectInpipe(int in_pipe){
    close(pipes[in_pipe].fd[1]);
    dup2(pipes[in_pipe].fd[0],STDIN_FILENO);
    close(pipes[in_pipe].fd[0]);
}

void SingleProcShell::redirectUserInpipe(int in_pipe){
    close(user_pipes[in_pipe].fd[1]);
    dup2(user_pipes[in_pipe].fd[0],STDIN_FILENO);
    close(user_pipes[in_pipe].fd[0]);
}

void SingleProcShell::redirectOutpipe(int out_pipe,char pipeType){
    close(pipes[out_pipe].fd[0]);
    dup2(pipes[out_pipe].fd[1],STDOUT_FILENO);
    if(pipeType == *plusErrDelim)
        dup2(pipes[out_pipe].fd[1],STDERR_FILENO);
    close(pipes[out_pipe].fd[1]);
}

void SingleProcShell::redirectUserOutpipe(int out_pipe){
    close(user_pipes[out_pipe].fd[0]);
    dup2(user_pipes[out_pipe].fd[1],STDOUT_FILENO);
    close(user_pipes[out_pipe].fd[1]);
}

void SingleProcShell::broadcastUserIn(int in_pipe){
    int send_id = user_pipes[in_pipe].send_id;
    char *bs = new char[15000];
    sprintf(bs, "*** %s (#%d) just received from %s (#%d) by '%s' ***\n", this->username, this->user_id, client_shells[send_id-1]->username, send_id, this->ori_cmd);
    broadcast(bs);
    delete bs; 
}

void SingleProcShell::broadcastUserOut(int out_pipe){
    int recv_id = user_pipes[out_pipe].recv_id;
    char *bs = new char[15000];
    sprintf(bs, "*** %s (#%d) just piped '%s' to %s (#%d) ***\n", this->username, this->user_id, this->ori_cmd, client_shells[recv_id-1]->username, recv_id);
    broadcast(bs);
    delete bs; 
}

int SingleProcShell::createUserPipe(int sender, int recver){
    int pipefd[2];
    if(pipe(pipefd) < 0){
        cerr << "Cannot pipe()!" << endl;
        exit(-1);
    }
    struct UserPipe up = {.fd = {pipefd[0],pipefd[1]}, .send_id = sender, .recv_id = recver};   
    user_pipes.push_back(up);
    return user_pipes.size()-1;
}

int SingleProcShell::findOutUserPipe(int recv_user){
    for(auto itr = user_pipes.begin();itr != user_pipes.end();itr++){
        if((*itr).send_id == this->user_id && (*itr).recv_id == recv_user){
            cerr << "*** Error: the pipe #" << this->user_id << "->#" << recv_user << " already exists. ***" << endl;
            return -1; 
        }
    }

    if(checkUserExist(recv_user))    return createUserPipe(this->user_id,recv_user);
    else{
        cerr << "*** Error: user #" << recv_user << " does not exist yet. ***" << endl;
        return -1;
    }
}

pid_t SingleProcShell::getFork(){
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

void SingleProcShell::pipeRedirection(int in_pipe, int out_pipe, char pipeType,vector<char*> *cmd,bool nor_out,bool nor_in){
    if(in_pipe > -1){
	if(nor_in)    redirectInpipe(in_pipe);
        else    redirectUserInpipe(in_pipe);
    }
    if(out_pipe > -1){
        if(nor_out)    redirectOutpipe(out_pipe,pipeType);
        else    redirectUserOutpipe(out_pipe);
    }

    //Invalid user pipe >> /dev/null
    if(!nor_in && in_pipe == -1){
        int fd = open("/dev/null",O_RDWR);
        dup2(fd, STDIN_FILENO);
        close(fd);
    }

    if(!nor_out && out_pipe == -1){
        int fd = open("/dev/null",O_RDWR);
        dup2(fd, STDOUT_FILENO);
        close(fd);
    }

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

void SingleProcShell::execPipeCmd(vector<struct Command> *parsed_cmd){
    int pipe_num = (*parsed_cmd).size(); 
    int status;

    for(int i = 0;i < pipe_num;i++){
	pipeCountDown();

    	int out_pipe;
        bool enable_nor_out = true; //either normal or user pipe
	bool enable_nor_in = true;
        char *delay = (*parsed_cmd)[i].delay;
	if(!(i == pipe_num-1&&delay == NULL)){ //normal outpipe
            int delay_int = delay == NULL?1:atoi(delay);
            out_pipe = findOutpipe(delay_int);
	}
        else if((*parsed_cmd)[i].user_out_pipe > 0){  //user pipe
            enable_nor_out = false;
            delay = (char *)1; //main process won't wait
            out_pipe = findOutUserPipe((*parsed_cmd)[i].user_out_pipe);
        }
        else  //output directly
            out_pipe = -1;
   
        // find if the pipe is '|','!' or NULL
	char pipeType = (*parsed_cmd)[i].pipe_type;

        int in_pipe = -1;
        if((*parsed_cmd)[i].user_in_pipe < 0) //normal pipe
            in_pipe = checkPipe(); //check if this command is piped. return pipe index, or -1(no pipe)
	else if((*parsed_cmd)[i].user_in_pipe > 0){
            enable_nor_in = false;
            in_pipe = checkUserPipe((*parsed_cmd)[i].user_in_pipe);
        }

        //in_pipe, out_pipe atr the indexes of vector pipe
        if((!enable_nor_in) && in_pipe > -1)    broadcastUserIn(in_pipe);
        if((!enable_nor_out) && out_pipe > -1)    broadcastUserOut(out_pipe);

        pid_t pid = getFork(); // fork until success,or exit if cannot fork after 20 retries.
	if(pid == 0){
            pipeRedirection(in_pipe,out_pipe,pipeType,&((*parsed_cmd)[i].cmd),enable_nor_out,enable_nor_in);    
        
            if(!strcmp((*parsed_cmd)[i].cmd[0],"ls"))
		function_ls(&((*parsed_cmd)[i]).cmd);
	    else if(!strcmp((*parsed_cmd)[i].cmd[0],"cat"))
		function_cat(&((*parsed_cmd)[i]).cmd);
	    else
		function_executable(&((*parsed_cmd)[i].cmd));
	    
            cout << "Weird! Why are u seeing this!?" <<endl;
	    exit(0);
        }
	else{
	    if(in_pipe > -1){
                if(enable_nor_in){
		    close(pipes[in_pipe].fd[0]);
		    close(pipes[in_pipe].fd[1]);
		    removePipe(in_pipe);
                }
                else{
                    close(user_pipes[in_pipe].fd[0]);
                    close(user_pipes[in_pipe].fd[1]);
                    removeUserPipe(in_pipe);   
                }
	    }
            
            if(!enable_nor_out)    sleep(1); //wait for error message, if there is any.

	    if(i == pipe_num-1&&delay == NULL)
	        waitpid(pid,&status,0);
	}
    }    
}

void SingleProcShell::printEnv(char *args){
    pipeCountDown();

    char *val;
    const char *delim = " ";
    char *var;
    if(args != NULL && (var = strtok(args+1, delim)) != NULL){
	if((val = getenv(var)) != NULL)
    	    cout << val << endl;
    }
    else{
    	for(auto ptr = envp.begin();ptr != envp.end(); ptr++)
	    cout << (*ptr).first << "=" << (*ptr).second <<endl;
    }   
}

void SingleProcShell::setEnv(char* args){
    pipeCountDown();
    const char *delim = " ";
    char *var;
    if(args != NULL && (var = strtok(args+1, delim)) != NULL){
        char *val;
        if((val = strtok(NULL,delim)) != NULL){
            auto ptr = envp.begin();
            for(;ptr != envp.end(); ptr++){
                if(!strcmp((*ptr).first.c_str(), var)){
                    (*ptr).second.assign(val);
                    break;
                }
            }
            if(ptr == envp.end())    envp.push_back(pair<string, string>(var,val));

            setenv(var,val,1);
        }
    }
}

void SingleProcShell::function_ls(vector<char*> *args){
    if(execvp("ls",(*args).data())<0){
    	cerr << "Unknown command: [" <<(*args)[0] << "]." << endl;
    	exit(-1);
    }
}

void SingleProcShell::function_cat(vector<char*> *args){   
    if(execvp("cat",(*args).data()) < 0){
        cerr << "Unknown command: [" <<(*args)[0] << "]." << endl;
    	exit(-1);
    }
}

void SingleProcShell::function_executable(vector<char*> *args){    
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
