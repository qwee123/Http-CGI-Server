#include <iostream>
#include "shellCmd.h"

const char *normalDelim = "|";
const char *plusErrDelim = "!";

void removeCarrige(char *s){
    int len = strlen(s);
    char* last = len > 0?s + len -1:s;

    if(*last == '\r')	*last = '\0';
    return ;
}

void parseCmd(char *cmd,vector<struct Command> *parsedc){
    char* sub_cmd;
    char* saveptr;
    vector<char*> pipe_cmds;
    sub_cmd = strtok_r(cmd,normalDelim,&saveptr); // split by '|'
    while(sub_cmd){
	char* sub_sub_cmd = strtok(sub_cmd,plusErrDelim); //split by '!'
	while(sub_sub_cmd){
	    pipe_cmds.push_back(sub_sub_cmd);
	    pipe_cmds.push_back((char*)plusErrDelim);  //push '!'
	    sub_sub_cmd = strtok(NULL,plusErrDelim);
	}
	pipe_cmds.pop_back();
	pipe_cmds.push_back((char*)normalDelim); //push "|"
        sub_cmd = strtok_r(NULL,normalDelim,&saveptr);
    }
    pipe_cmds.pop_back();
    // remove '\r' from the end if there is any.
    if((int)pipe_cmds.size() > 0) 	removeCarrige(pipe_cmds.back());	    

    const char* delim2 = " ";
    for(int i = 0;i < (int)pipe_cmds.size();i++){
	if(!strcmp(pipe_cmds[i],"|")||!strcmp(pipe_cmds[i],"!")){
	    (*parsedc).back().pipe_type = *pipe_cmds[i];
	    continue;
	}
        else if(!strcmp(pipe_cmds[i],"")|| !strcmp(pipe_cmds[i],"\r\n") || !strcmp(pipe_cmds[i],"\n")){
            continue;
        }

	if(i>0&&pipe_cmds[i][0] != ' '){ //pipe_cmds[i] should be something like "1 number " or " number test.html"
	    char *pipe_delay = strtok(pipe_cmds[i],delim2);
	    (*parsedc).back().delay = pipe_delay;
	    sub_cmd = strtok(NULL,delim2);
            if(!sub_cmd)    continue;
	}
	else
	    sub_cmd = strtok(pipe_cmds[i],delim2);
		
	(*parsedc).push_back(*(new struct Command));
	while(sub_cmd){
	    if(sub_cmd != ""&& !isUserPipe(sub_cmd,&(*parsedc).back()))   (*parsedc).back().cmd.push_back(sub_cmd);
	    sub_cmd = strtok(NULL,delim2);
	}
    }

    vector<char*>().swap(pipe_cmds); //release pipe_cmds
}

bool isUserPipe(char *read_cmd, struct Command *list){
    if(*read_cmd == '<' && *(read_cmd+1) != '\0'){ //e.g. cat <1, something like cat < file should be filterd out.
        (*list).user_in_pipe = atoi(read_cmd+1);                                        //(p.s. the next "space" is already replaced by the '\0' due tp strtok!)
        return true;
    }
    else if(*read_cmd == '>' && *(read_cmd+1) != '\0'){
        (*list).user_out_pipe = atoi(read_cmd+1);
        return true;
    }
    else return false;
}

