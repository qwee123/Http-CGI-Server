#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include "passiveSocket.h"

u_short portbase = 0;

int passiveSock(const char *service, const char *trans_proto, int qlen){  
    struct servent *pse;      // ptr to service info entry
    struct protoent *ppe;     // ptr to protocol info entry
    struct sockaddr_in sin;   // internet endpoint addresss

    int sockfd, type;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;

    if(pse = getservbyname(service, trans_proto))
        sin.sin_port = htons(ntohs((u_short)pse->s_port + portbase));
    else if((sin.sin_port = htons((u_short)atoi(service))) == 0){
        std::cerr << "Cannot get "<< service << " service entry!" << std::endl;
        exit(-1);
    }

    if((ppe = getprotobyname(trans_proto)) == 0){
        std::cerr << "Cannot get " << trans_proto << " protocol entry!" << std::endl;
        exit(-1);
    }
    
    if(!strcmp(trans_proto, "tcp"))
        type = SOCK_STREAM;
    else if(!strcmp(trans_proto, "udp"))
        type = SOCK_DGRAM;

    sockfd = socket(PF_INET, type, ppe->p_proto);
    if(sockfd < 0)
        std::cerr << "Cannot create socket!" << std::endl;   

    int reuse_flag = 1;
    if(setsockopt(sockfd,SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse_flag, sizeof(int)) < 0)
        std::cerr << "Cannot set socket option SO_REUSEADDR" << std::endl;
    
    if(bind(sockfd, (struct sockaddr *)&sin, sizeof(sin)) < 0)
        std::cerr << "Cannot bind to " << sin.sin_port << " port" << std::endl;
    
    if(type == SOCK_STREAM && listen(sockfd, qlen) < 0){
        std::cerr << "Cannot listen on " << sin.sin_port << " port" << std::endl;
        exit(-1);
    }
    return sockfd;
}

int passiveTCP(const char *service, int qlen){
    return passiveSock(service, "tcp", qlen);
}

int passiceUDP(const char *service){
    return passiveSock(service, "udp", 0);
}
