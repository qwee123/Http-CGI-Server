#include <iostream>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <wait.h>
#include <signal.h>
#include "multiProcShell.h"
#define MAX_USER 30
#define MAX_INFO_TYPE 10
#define INFO_ID 10
#define BS_ID 11
#define FIFO_ID 12
using namespace std;

vector<delayPipe> pipes;
vector<struct RDFIFO> RDFIFO_list;
int uid;
volatile  sig_atomic_t after_bs;

void initialize(char **envp,int id){
    for(char **ptr = envp;*ptr != 0; ptr++)
        unsetenv(*ptr);
    
    setenv("PATH","bin:.",1);
    uid = id;
}

void safeClose(int fdin){
    pid_t pid = getFork();
    if(pid == 0){
        dup2(fdin,STDIN_FILENO);
        close(fdin);
        int fd = open("/dev/null",O_RDWR);
        dup2(fd,STDOUT_FILENO);
        close(fd);

        if(execlp("./bin/cat","./bin/cat",NULL) < 0){
            cerr << "Trancate error" << endl;
            exit(-1);
        }
    }
    else{
        close(fdin);
        int status;
        waitpid(pid,&status,0);
    }
}

void bsHandler(){
    struct MessageInfo &msg = *((struct MessageInfo *)getShmptr(FIFO_ID));
    if((!strcmp(msg.type,"receive") && msg.recv_id == uid)){
        shmdt(&msg);
        return;
    }
    else if(!strcmp(msg.type,"r&p") && msg.send_id == uid){ //in r&p send_id = who 'send' this mesg, recv_id = who 'accept' the pipe
        if(msg.recv_id == uid)    addFIFO(msg.send_id);
        after_bs = 1;
        shmdt(&msg);
        return;
    }

    char *str = (char*)getShmptr(BS_ID);
    send(STDOUT_FILENO,str,strlen(str),0);
    shmdt(str);

    if(!strcmp(msg.type,"pipe") || !strcmp(msg.type,"r&p")){
        if(msg.recv_id == uid){
            addFIFO(msg.send_id);
        }
        after_bs = 1;
    }
    else if(!strcmp(msg.type,"logout")){
        for(auto itr = RDFIFO_list.begin();itr != RDFIFO_list.end();itr++){
            if((*itr).send_id == msg.send_id){
                clearRDFIFO(itr);
                RDFIFO_list.erase(itr,itr+1);
                break;
            } 
        }
    }
    shmdt(&msg);
}

void clearRDFIFO(vector<struct RDFIFO>::iterator itr){
    safeClose((*itr).fd);
    char *fname = new char[15];
    sprintf(fname,"./user_pipe/fifo_%dto%d",(*itr).send_id,uid); 
    unlink(fname);
    delete fname;               
}

void customedHandler(int signo){
    if(signo == SIGUSR1){
        bsHandler();
    }
}

void addFIFO(int send_id){
    struct RDFIFO s;
    char *fname = new char[25];
    sprintf(fname, "./user_pipe/fifo_%dto%d",send_id, uid);
    s.send_id = send_id;
    s.fd = open(fname,O_RDONLY);
    RDFIFO_list.push_back(s);
    delete fname;
}

int execShell(int id,char* envp[]){
    initialize(envp,id);  
    signal(SIGUSR1, customedHandler); 
    login();
  
    bool exitflag = false; 
    char *first_word = new char[50];
    char *cmd = new char[15000];
    char *ori_cmd = new char[15000]; //for pipe message usage
    vector<struct Command> parsed_cmd;
    
    while(!exitflag){
        cout << "% "<< flush;
        memset(ori_cmd,0,15000);
        cin.getline(cmd,15000);
        strncpy(ori_cmd, cmd, 15000);

        if(cmd[strlen(cmd)-1] == '\r')    cmd[strlen(cmd)-1] = '\0';
        
        memset(first_word,0,50);	
        char *first_white = strchr(cmd,' ');
        if(first_white != NULL)    strncpy(first_word, cmd, first_white - cmd); 
        else    strncpy(first_word, cmd,50);   

        if(strlen(first_word) > 0){
            if(!strcmp(first_word,"exit"))    exitflag = true; 
            else if(!strcmp(first_word,"printenv"))
                printEnv(first_white,envp);
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
                if((int)parsed_cmd.size() > 0)    execPipeCmd(&parsed_cmd,ori_cmd);
            }
        }
        else{
            parseCmd(cmd, &parsed_cmd);
            if((int)parsed_cmd.size() > 0)    execPipeCmd(&parsed_cmd,ori_cmd);
        }
        parsed_cmd.clear();
    }

    logout();
    delete ori_cmd;
    delete first_word;
    delete cmd;
    vector<struct Command>().swap(parsed_cmd);
    return 0;
}

void printEnv(char *args, char **envp){
    pipeCountDown();

    char *val;
    const char *delim = " ";
    char *var;
    if(args != NULL && (var = strtok(args+1, delim)) != NULL){
	if((val = getenv(var)) != NULL)
    	    cout << val << endl;
    }
    else{
        for(char **ptr = envp;*ptr != 0; ptr++)
	    cout << (*ptr) <<endl;
    }
}

void setEnv(char* args){
    pipeCountDown();
    const char *delim = " ";
    char *var;
    if(args != NULL && (var = strtok(args+1, delim)) != NULL){
        char *val;
        if((val = strtok(NULL,delim)) != NULL){
            setenv(var,val,1);
        }
    }
}

void broadcast(char *str,struct user* shmptr,struct MessageInfo* info){
    struct MessageInfo *ms_info = (struct MessageInfo *)getShmptr(FIFO_ID);
    memset(ms_info,0,sizeof(struct MessageInfo));
    memcpy(ms_info,info,sizeof(*info));
    shmdt(ms_info);

    char *bs_shm = (char *)getShmptr(BS_ID);
    memset(bs_shm,0,strlen(bs_shm));
    memcpy(bs_shm,str,strlen(str));  
    shmdt(bs_shm);    
    
    int index = 0;
    for(;index < MAX_USER;index++){
        if(shmptr[index].uid == 0)    continue;
        if(kill(shmptr[index].pid,SIGUSR1) < 0)    cerr << "Failed to send SIG1 to " << shmptr[index].uid << " : " << strerror(errno) << endl;
    }
}

void function_name(char *cmd){
    pipeCountDown();
    struct user* shmptr = getShmptr(INFO_ID);
    if(cmd != NULL && validName(cmd,shmptr)){
        struct user &self = shmptr[uid-1];
        if(self.uid == 0){
            cerr << "Failed to find legal user id in name()" << endl;
            shmdt(shmptr);
            return ;
        }
        memset(self.username,0,sizeof(self.username));
        strncpy(self.username, cmd+1, 20);    

        char *bs = new char[80];
        sprintf(bs, "*** User from %s:%hu is named '%s'. ***\n", self.ipaddr, self.port_num, self.username);
        
        struct MessageInfo info;
        strncpy(info.type,"name",MAX_INFO_TYPE);
        info.send_id = uid;
        broadcast(bs,shmptr,&info);
        delete bs;
    }
    shmdt(shmptr);
}

bool validName(char *name, struct user* shmptr){
    for(int index = 0;index < MAX_USER;index++){
        if(shmptr[index].uid == 0)    continue;
        
        if(!strcmp(name+1,shmptr[index].username)){
            cout << "*** User '" << name+1 << "' already exists. ***" << endl;
            return false;
       }     
    }
    return true;
}

void function_who(){
    pipeCountDown();
    struct user* shmptr = getShmptr(INFO_ID);
    if(shmptr == NULL)    return ;

    cout << "<ID>	<nickname>	<IP:port>	<indicate me>" << endl;
    int index = 0;
    for(;index < MAX_USER;index++){
        if(shmptr[index].uid == 0)    continue;
        
        cout << shmptr[index].uid <<"	"<< shmptr[index].username <<"	"
         << shmptr[index].ipaddr <<":"<< shmptr[index].port_num;

        if(shmptr[index].uid == uid)    cout << "	<-me";
        cout << endl;
    }
    shmdt(shmptr);
}

void function_yell(char *cmd){
    pipeCountDown();
    struct user* shmptr = getShmptr(INFO_ID);
    if(shmptr == NULL)    return ;

    char *broadstr = new char[1100];
    sprintf(broadstr, "*** %s yelled ***: %s\n", shmptr[uid-1].username, cmd+1);
    
    struct MessageInfo info;
    strncpy(info.type,"yell",MAX_INFO_TYPE);
    info.send_id = uid;
    broadcast(broadstr,shmptr,&info);
    delete broadstr;
    
    shmdt(shmptr);
}

void function_tell(char *cmd){
    pipeCountDown();
    if(cmd != NULL){
        int recv_id = atoi(strtok(cmd+1," "));
        char *content = cmd + strlen(cmd) + 1;
        struct user* shmptr = getShmptr(INFO_ID);

        if((*content) != 0 && checkUserExist(recv_id,shmptr)){
            char *ms = new char[1100];
            sprintf(ms, "*** %s told you ***: %s\n", shmptr[uid-1].username, content);
 
            char *bs_shm = (char *)getShmptr(BS_ID);
            memset(bs_shm,0,strlen(bs_shm));
            memcpy(bs_shm,ms,strlen(ms));  
            shmdt(bs_shm);

            struct MessageInfo info;
            strncpy(info.type,"tell",MAX_INFO_TYPE);
            if(kill(shmptr[recv_id-1].pid,SIGUSR1) < 0)    cerr << "Failed to send SIG1 to "<< shmptr[recv_id-1].uid << ": " << strerror(errno) << endl;
            delete ms;
        }
        else{
            cerr << "*** Error: user #" << recv_id << " does not exist yet. ***" << endl;
        }
        shmdt(shmptr);   
    }
}

bool checkUserExist(int id,struct user* shmptr){  //id is related to the index in vector
    if(id > 0)    return shmptr[id-1].uid != 0;
    else false;
}

void login(){
    cout << "****************************************" << endl;
    cout << "** Welcome to the information server. **" << endl;
    cout << "****************************************" << endl;
   
    struct user* shmptr = getShmptr(INFO_ID);
    if(shmptr == NULL)    return ;
 
    struct user &cli = shmptr[uid-1]; //uid must > 0
    char *bs = new char[80];
    sprintf(bs, "*** User '%s' entered from %s:%hu. ***\n", cli.username, cli.ipaddr, cli.port_num);
   
    struct MessageInfo info;
    strncpy(info.type,"login",MAX_INFO_TYPE);
    info.send_id = uid;
    broadcast(bs, shmptr, &info);   
 
    shmdt(shmptr);
    delete bs;
}

void logout(){
    struct user *shmptr = getShmptr(INFO_ID);
    if(shmptr == NULL)    return;
    
    char *bs = new char[40];
    sprintf(bs, "*** User '%s' left. ***\n", shmptr[uid-1].username);
    
    struct MessageInfo info;
    strncpy(info.type,"logout",MAX_INFO_TYPE);
    info.send_id = uid;
    broadcast(bs,shmptr,&info);
    delete bs;

    for(auto itr = RDFIFO_list.begin();itr != RDFIFO_list.end();itr++){
        clearRDFIFO(itr);
    }

    memset(&shmptr[uid-1],0,sizeof(struct user));  
    shmdt(shmptr);
    return ;
}

int checkPipe(){
    for(int i = 0;i < pipes.size();i++){
 	if(pipes[i].delay_count == 0)	return i;
    }
    return -1;
}

int checkUserPipe(int sender_id){
    struct user* shmptr = getShmptr(INFO_ID);
    if(shmptr == NULL)    return -1;

    if(!checkUserExist(sender_id,shmptr)){
        cerr << "*** Error: user #" << sender_id << " does not exist yet. ***" << endl;
        return -1;
    }
    shmdt(shmptr);
  
    auto ptr = RDFIFO_list.begin();
    for(;ptr != RDFIFO_list.end();ptr++){
        if((*ptr).send_id == sender_id)    break;
    }
    if(ptr == RDFIFO_list.end()){
        cerr << "*** Error: the pipe #" << sender_id <<"->#"<< uid << " does not exist yet. ***" << endl;
        return -1;
    }
    else return ptr - RDFIFO_list.begin();
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

void removeRDFIFO(int index){
    vector<struct RDFIFO>::iterator itr = RDFIFO_list.begin() + index;
    RDFIFO_list.erase(itr,itr+1);
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

int findOutUserPipe(int recv_user){
    string fifo_path = "./user_pipe/fifo_" + to_string(uid) + "to" + to_string(recv_user);
    int res;
    if((res = mkfifo(fifo_path.c_str(),0666)) < 0){  //file already exist >> pipe already exist!
        if(errno == EEXIST)    cerr << "*** Error: the pipe #" << uid << "->#" << recv_user << " already exists. ***" << endl;
        else    cerr << "Error: " << strerror(errno) << endl;
        return -1;
    } 

    struct user* shmptr = getShmptr(INFO_ID); 
    if(checkUserExist(recv_user,shmptr)){
        shmdt(shmptr);
        return recv_user; //create fifo succesfully, return recv_id
    }
    else{
        cerr << "*** Error: user #" << recv_user << " does not exist yet. ***" << endl;
        unlink(fifo_path.c_str());
        shmdt(shmptr);
        return -1;
    }
}

void redirectInpipe(int in_pipe){
    close(pipes[in_pipe].fd[1]);
    dup2(pipes[in_pipe].fd[0],STDIN_FILENO);
    close(pipes[in_pipe].fd[0]);
}

void filterEOF(int fdout, int fdin){
    char *buf = new char[1024];
    while((read(fdin,buf,1024)) != 0){
        write(fdout,buf,strlen(buf));
        memset(buf,0,1024);
    }
    delete buf;
}

void redirectUserInpipe(int index,char *cmd){
    int pipefd[2]; //filter out EOF
    if(pipe(pipefd) < 0){
        cerr << "Cannot pipe() in redirect userInpipe" << endl;
        exit(-1);
    }

    pid_t pid;
    pid = getFork();
    if(pid == 0){
        close(pipefd[0]);
        int fd = RDFIFO_list[index].fd;

        filterEOF(pipefd[1], fd);
        close(pipefd[1]);
        close(fd);
     
        string fifo_path = "./user_pipe/fifo_" + to_string(RDFIFO_list[index].send_id) + "to" + to_string(uid);    
        unlink(fifo_path.c_str());
        exit(0);
    } 
    else{
        close(pipefd[1]);
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);
    }
}

void redirectOutpipe(int out_pipe,char pipeType){
    close(pipes[out_pipe].fd[0]);
    dup2(pipes[out_pipe].fd[1],STDOUT_FILENO);
    if(pipeType == *plusErrDelim)
        dup2(pipes[out_pipe].fd[1],STDERR_FILENO);
    close(pipes[out_pipe].fd[1]);
}

void redirectUserOutpipe(int recv_id,char *cmd){ //out_pipe is recv_id in userpipe case
    string fifo_path = "./user_pipe/fifo_" + to_string(uid) + "to" + to_string(recv_id); 
    int fd = open(fifo_path.c_str(),O_WRONLY);
    dup2(fd, STDOUT_FILENO); //Blocking mode
    close(fd);
}

void broadcastUserIn(int index, char *cmd){
    int send_id = RDFIFO_list[index].send_id;

    struct MessageInfo info;
    memset(&info,0,sizeof(info));
    info.send_id = send_id;
    info.recv_id = uid;
    strncpy(info.type,"receive",MAX_INFO_TYPE); 
    
    struct user* shmptr = getShmptr(INFO_ID);    
    if(shmptr == NULL)    return;
    struct user &self = shmptr[uid-1];
    
    char *bs = new char[15000];
    sprintf(bs, "*** %s (#%d) just received from %s (#%d) by '%s' ***\n", self.username, self.uid, shmptr[send_id-1].username, send_id, cmd);
    broadcast(bs,shmptr,&info);
    cout << bs;
    memset(bs,0,15000);
    delete bs;
    shmdt(shmptr);
}

void broadcastUserOut(int recv_id,char *cmd){
    struct MessageInfo info;
    memset(&info,0,sizeof(info));
    info.send_id = uid;
    info.recv_id = recv_id;
    strncpy(info.type,"pipe",MAX_INFO_TYPE);    

    struct user* shmptr = getShmptr(INFO_ID);    
    if(shmptr == NULL)    return;
    struct user &self = shmptr[uid-1];

    char *bs = new char[15000];
    sprintf(bs, "*** %s (#%d) just piped '%s' to %s (#%d) ***\n", self.username, self.uid, cmd, shmptr[recv_id-1].username, recv_id);
    broadcast(bs,shmptr,&info);
    memset(bs,0,15000);
    delete bs; 
    shmdt(shmptr);
}

void broadcastUserInOut(int in_index, int out_recv_id, char *cmd){
    int send_id = RDFIFO_list[in_index].send_id;

    struct MessageInfo info;
    memset(&info,0,sizeof(info));
    info.send_id = uid;
    info.recv_id = out_recv_id;
    strncpy(info.type,"r&p",MAX_INFO_TYPE);    

    struct user* shmptr = getShmptr(INFO_ID);    
    if(shmptr == NULL)    return;
    struct user &self = shmptr[uid-1];

    char *bs = new char[30000];

    sprintf(bs, "*** %s (#%d) just received from %s (#%d) by '%s' ***\n*** %s (#%d) just piped '%s' to %s (#%d) ***\n"
             , self.username, self.uid, shmptr[send_id-1].username, send_id, cmd, self.username, self.uid, cmd, shmptr[out_recv_id-1].username, out_recv_id); 
    cout << bs;
    broadcast(bs,shmptr,&info);
    memset(bs,0,30000);
    delete bs; 
    shmdt(shmptr);
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

void userPipeBroadcast(int in_pipe,int out_pipe,bool nor_in,bool nor_out, char* ori_cmd){
    int type = 0;
    type = (type | nor_in) << 1;
    type = (type | nor_out) << 1;
    type = (type | (in_pipe > -1)) << 1;
    type = (type | (out_pipe > -1));
    switch(type){
        case 1: //0001 in_pipe broken, only out_pipe
            broadcastUserOut(out_pipe,ori_cmd);
            break;
        case 2: //0010 out_pipe broken, only in_pipe
            broadcastUserIn(in_pipe,ori_cmd);
            break;
        case 3: //0011 both user pipe
            broadcastUserInOut(in_pipe,out_pipe,ori_cmd);
            break;
        case 6: //0110 only user_in pipe
            broadcastUserIn(in_pipe,ori_cmd);
            break;
        case 7: //0111 only user_in pipe
            broadcastUserIn(in_pipe,ori_cmd);
            break;
        case 9: //1001 only user_out pipe
            broadcastUserOut(out_pipe,ori_cmd);
            break;
        case 11: //1011 only user_out pipe
            broadcastUserOut(out_pipe,ori_cmd);
            break;
        default : break;
    }
}

void pipeRedirection(int in_pipe, int out_pipe, char pipeType,vector<char*> *cmd,bool nor_in, bool nor_out,char *ori_cmd){
    userPipeBroadcast(in_pipe,out_pipe,nor_in,nor_out,ori_cmd);

    if(in_pipe > -1){
        if(nor_in)    redirectInpipe(in_pipe);
        else    redirectUserInpipe(in_pipe,ori_cmd);
    }   
    if(out_pipe > -1){
	if(nor_out)    redirectOutpipe(out_pipe,pipeType);
        else    redirectUserOutpipe(out_pipe,ori_cmd);
    }
    //close all fd in the RDFIFO after redirection
    for(auto itr =RDFIFO_list.begin(); itr != RDFIFO_list.end();itr++)    close((*itr).fd);
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

void execPipeCmd(vector<struct Command> *parsed_cmd,char *ori_cmd){
    int pipe_num = (*parsed_cmd).size(); 
    int status;

    for(int i = 0;i < pipe_num;i++){
	pipeCountDown();
       
        bool enable_nor_out = true; //either normal ot user pipe
        bool enable_nor_in = true;
	
	int in_pipe = -1;
        if((*parsed_cmd)[i].user_in_pipe < 0) //normal pipe
            in_pipe = checkPipe(); //check if this command is piped. return pipe index, or -1(no pipe)
        else if((*parsed_cmd)[i].user_in_pipe > 0){
            enable_nor_in = false;
            in_pipe = checkUserPipe((*parsed_cmd)[i].user_in_pipe); //return fd
        }

    	int out_pipe = -1;
	char *delay = (*parsed_cmd)[i].delay;
	if(!(i == pipe_num-1&&delay == NULL)){
            int delay_int = delay == NULL?1:atoi(delay);
            out_pipe = findOutpipe(delay_int);
	}
        else if((*parsed_cmd)[i].user_out_pipe > 0){
            enable_nor_out = false;
            delay = (char *)1; //main process won't wait.
            out_pipe = findOutUserPipe((*parsed_cmd)[i].user_out_pipe);
        }
  
        // find if the pipe is '|','!' or NULL
	char pipeType = (*parsed_cmd)[i].pipe_type;

        after_bs = 0; //make main process wait for the broadcast
	pid_t pid = getFork(); // fork until success,or exit if cannot fork after 20 retries.
	if(pid == 0){
            pipeRedirection(in_pipe,out_pipe,pipeType,&((*parsed_cmd)[i].cmd),enable_nor_in, enable_nor_out,ori_cmd);
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
                if(enable_nor_in){
		    close(pipes[in_pipe].fd[0]);
		    close(pipes[in_pipe].fd[1]);
		    removePipe(in_pipe);
                }
                else{
                    close(RDFIFO_list[in_pipe].fd);
                    removeRDFIFO(in_pipe);
                }
	    }
            
            if((!enable_nor_out) && out_pipe > -1){
                while(after_bs == 0)    ;
            }
            sleep(1);

	    if(i == pipe_num-1&&delay == NULL)
	        waitpid(pid,&status,0);
	}
    }    
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
