#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PORT "8080" // the port client will be connecting to
#define MAXDATASIZE 10000 // max number of bytes we can get at once

// get sockaddr, IPv4 or IPv6
void *get_in_addr(struct sockaddr *sa){
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

// TODO : handel larger request size
int main(int argc, char *argv[])
{
     printf("client: starting\n");

    int sockfd; // Socket descriptor for server

    int port; // server port

    struct addrinfo hints; // client address information
    struct addrinfo *servinfo; // Will point to the results
    struct addrinfo *p; // Used to loop to find the available address info
    int rv; // Status off getting address info of the client

    char s[INET6_ADDRSTRLEN]; // Used for transforming IP into string

    // Check for host name and port number
    if (argc < 2 || argc > 3) {
        fprintf(stderr,"usage: client hostname\n");
        exit(1);
    }

    printf("client: constructing local data address struct\n");

    /*  Construct local address structure */
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    printf("client: getting address information status and checking it\n");

    if(argc == 3){
        if ((rv = getaddrinfo(argv[1], argv[2], &hints, &servinfo)) != 0) {
            fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
            return 1;
        }
    }else{
        if ((rv = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0) {
            fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
            return 1;
        }
    }

    // servinfo now points to a linked list of 1 or more struct addrinfos
    // ... do everything until you don't need servinfo anymore ....

    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        // Make a socket
        if ((sockfd = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }

        // Connect to the server
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("client: connect");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }

    printf("client: clearing address info struct\n");

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),s, sizeof s);

    printf("client: connecting to %s\n", s);

    freeaddrinfo(servinfo); // all done with this structure

    readRequestFile(sockfd);

    close(sockfd);

    printf("client: connection to server is closed\n");

    return 0;
}

void readRequestFile(int sockfd){

    char requestBuffer[MAXDATASIZE]; // Buffer for requests
    int requestBufferIndex = 0; // Buffer current index
    char ch; // Current charachter in the buffer

    // Read line from file
    FILE *fp = fopen("/home/ahmed/Desktop/Programming Assignment 1/client/requests", "rb");
    if(fp == NULL){
        perror("File");
    }else{
        while ((ch = fgetc(fp)) != EOF){
            if(ch == '\n'){
                if((ch = fgetc(fp)) == '\n'){
                    // Send get request
                    if (send(sockfd, requestBuffer, MAXDATASIZE, 0) == -1){
                        perror("send");
                    }
                    printf("client: %s\n",requestBuffer);
                    if(parse(requestBuffer) == 1){
                        handelGetRequest(sockfd);
                    }else{
                        handelPostRequest(sockfd);

                        // TODO : check if not ack then send data again
                        // Recieve ACK
                        char ACKBuffer[5];
                        memset(ACKBuffer, 0, 5); // Clear the buffer
                        int recvMsgSize = 0 ;
                        if  ((recvMsgSize = recv(sockfd, ACKBuffer, MAXDATASIZE, 0)) < 0) {
                            perror("recieve");
                        }

                        printf("client: %s",ACKBuffer);
                    }
                    printf("-----------------------------------------------------------------------\n");
                    printf("client: sending next request...\n");
                    memset(requestBuffer, 0, MAXDATASIZE); // Clear the buffer
                    requestBufferIndex = 0;
                }else{
                    requestBuffer[requestBufferIndex++] = '\n';
                    requestBuffer[requestBufferIndex++] = ch;
                }
            }else{
                requestBuffer[requestBufferIndex++] = ch;
            }
        }
    }
    fclose(fp);
}

int parse(const char* line){
    if (strstr(line, "GET") != NULL) {
        return 1;
    }
    return 0;
}

// TODO : handel large recv stream sending next request
void handelGetRequest(int sockfd){
    char recvBuffer[MAXDATASIZE]; // Buffer for get data
    memset(recvBuffer, 0, MAXDATASIZE); // Clear the buffer
    int recvMsgSize = 0 ;

    // Recieve status OK or NOT FOUND
    if  ((recvMsgSize = recv(sockfd, recvBuffer, MAXDATASIZE, 0)) < 0) {
        perror("recieve");
    }

    printf("server: %s",recvBuffer);

    if(strstr(recvBuffer, "200") != NULL) {
        memset(recvBuffer, 0, MAXDATASIZE); // Clear the buffer

        // Recieve data if OK
        if  ((recvMsgSize = recv(sockfd, recvBuffer, MAXDATASIZE, 0)) < 0) {
            perror("recieve");
        }

        printf("server: %s",recvBuffer);

        FILE* fp = fopen( "/home/ahmed/Desktop/Programming Assignment 1/client/server", "wb");
        if(fp != NULL){
            fwrite(recvBuffer, 1, recvMsgSize, fp);
            fclose(fp);
        } else {
            perror("File");
        }
    }

    // Recieve data if OK
    /*if  ((recvMsgSize = recv(sockfd, recvBuffer, MAXDATASIZE, 0)) < 0) {
        perror("recieve");
    }

    printf("server: %s",recvBuffer);*/
}

// TODO : Fetch path from the request
void handelPostRequest(int sockfd){
     char sendBuffer[MAXDATASIZE]; // Buffer for post data
     memset(sendBuffer, 0, MAXDATASIZE); // Clear the buffer
     int sendMsgSize = 0 ;

    FILE *fp = fopen("/home/ahmed/Desktop/Programming Assignment 1/client/client", "rb");
    if(fp == NULL){
        perror("File");
    }else{
        while( (sendMsgSize = fread(sendBuffer, 1, sizeof(sendBuffer), fp))>0 ){
            send(sockfd, sendBuffer, sendMsgSize, 0);
            printf("client: %s",sendBuffer);
            memset(sendBuffer, 0, MAXDATASIZE); // Clear the buffer
        }
    }
    fclose(fp);
}
