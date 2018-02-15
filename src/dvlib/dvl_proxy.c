#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include "dvl_proxy.h"
#include "dvl.h"

/* AF_INET socket stuff */
#include <arpa/inet.h> 
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>


#define DVL_PROXY_SRV_DEFAULT_IP "127.0.0.1"
#define DVL_PROXY_SRV_DEFAULT_APP_PORT 8888
#define DVL_PROXY_SRV_DEFAULT_JOB_PORT 8889



#define ENV_IP "DV_PROXY_SRV_IP"
#define ENV_PORT "DV_PROXY_SRV_PORT"



#define CONNECTED 0
#define SOCK_CREATE_ERROR -1
#define INVALID_IP -2
#define CONNECTION_ERROR -3

#define CONN_TRIES 10
char * jobid;
int sockfd=0;


int dvl_srv_connect(){

    int res;
    struct sockaddr_in srv_addr;

    int port = (dvl.is_simulator) ? DVL_PROXY_SRV_DEFAULT_JOB_PORT : DVL_PROXY_SRV_DEFAULT_APP_PORT;

    char * srv_ip = (getenv(ENV_IP)!=NULL) ? getenv(ENV_IP) : DVL_PROXY_SRV_DEFAULT_IP;

    port = (getenv(ENV_PORT)!=NULL) ? atoi(getenv(ENV_PORT)) : port;
    
    uint16_t srv_port = htons(port);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Error while creating socket");
	    exit(-1);
    }

    memset(&srv_addr, '0', sizeof(srv_addr));
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_port = srv_port;
    res = inet_pton(AF_INET, srv_ip, &srv_addr.sin_addr);
    if (res<=0) {
        sockfd=0;
        DVLPRINT("Invald IP address\n");
        return INVALID_IP;
    }

    printf("connecting to %s:%u\n", srv_ip, port);
    int tries=0;
    res = -1;
    while (res<0 && tries<CONN_TRIES){
    	res = connect(sockfd, (struct sockaddr *)&srv_addr, sizeof(srv_addr));
    	if (res<0) {
            DVLPRINT("Error while connecting: %i (%i/%i)\n", res, tries+1, CONN_TRIES);
            tries++;
            sleep(3*(tries+1));
    	}
    }

    if (res<0) {
        perror("Failed to connect!");
        exit(-1);
    }
    
    return CONNECTED;
}

int dvl_srv_disconnect(){
    if (sockfd) close(sockfd);
    sockfd=0;
    return DVL_SUCCESS;
}

int dvl_check_connection(){
    int res;
    if (!sockfd) {
        res = dvl_srv_connect();
        if (res!=CONNECTED) return res;
    }
    return CONNECTED;
}



int dvl_send_message(char * buff, int size, int disconnect){

	 
    int res = dvl_check_connection();
    if (res!=CONNECTED) return res;
    DVLPRINT("Sending message: (len: %i): %s\n", size, buff);

    res = write(sockfd, buff, size);

    printf("write res: %i\n", res);
    if (res<0){
        dvl_srv_disconnect();
        DVLPRINT("Error while writing to socket: %i (message: %s)\n", res, buff);
        return DVL_ERROR;
    } 

    if (disconnect) dvl_srv_disconnect();
    
    return DVL_SUCCESS;

}

int dvl_recv_message(char * buff, int size, int disconnect){
    
    int res = dvl_check_connection();
    if (res!=CONNECTED) return res;

    res = read(sockfd, buff, size-1);
    if (res==0){
        dvl_srv_disconnect();
        printf("Server socket has been closed\n");
        return DVL_ERROR;
    }else if (res<0){
        dvl_srv_disconnect();
        perror("Read failed!");
        return DVL_ERROR;
    }
    

    buff[res] = '\0';



    if (disconnect) dvl_srv_disconnect();
    return res;
}




/*
int dvl_srv_signal_variable(dvl_t * dvl, const char * datafile, const char * varname){
    char buff[BUFFER_SIZE];
    
    int res=snprintf(buff, BUFFER_SIZE, "VAR:%s:%s", datafile, varname);
    if (res>BUFFER_SIZE) {
        dvl_srv_disconnect(dvl);
        DVLPRINT("Send buffer is too small\n");
        return DVL_ERROR;
    }
    return dvl_send_message(dvl, buff, BUFFER_SIZE);
}

*/

/*
int dvl_srv_mark_complete(dvl_t * dvl, const char * datafile){
    char buff[BUFFER_SIZE];

    int res=snprintf(buff, BUFFER_SIZE, "FILE:%s:%s", jobid, datafile);
    if (res>BUFFER_SIZE) {
        dvl_srv_disconnect(dvl);
        DVLPRINT("Send buffer is too small\n");
        return DVL_ERROR;
    }
    return dvl_send_message(dvl, buff, res);
}

*/

