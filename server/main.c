#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>

#define DEFAULTTIMEOUT 6 // Time out for zero connections
#define BACKLOG 10   // How many pending connections queue will hold
#define MAXDATASIZE 10000 // max number of bytes we can get at once

void sigchld_handler(int s){
  while(waitpid(-1, NULL, WNOHANG) > 0);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa){
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
        return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void killProcess(int sigNum){
    printf("server: TIMEOUT ! connection has been closed\n");
    kill(getpid(), SIGKILL);
}


int main(int argc, char *argv[]){

    printf("server: starting\n");

    int serverSocket; // Socket descriptor for server
    int clientSocket; // Socket descriptor for client

    struct addrinfo hints; // Server address information
    struct addrinfo *servinfo; // Will point to the results
    int serverAddressStatus; // Status off getting address info of the server
    struct addrinfo *p; // Used to loop to find the available address info

    struct sockaddr_storage their_addr; // Connector's address information
    socklen_t sin_size; // Connector's address size

    struct sigaction sa;

    int yes=1; // Used to make the address available

    char s[INET6_ADDRSTRLEN]; // Used for transforming IP into string


    printf("server: constructing local data address struct\n");

    // Check for port number
    if (argc != 2) {
        fprintf(stderr,"usage: server port\n");
        exit(1);
    }

    /*  Construct local address structure */
    memset(&hints, 0, sizeof hints); // Zero out structure
    hints.ai_family = AF_UNSPEC; // Don't care IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
    hints.ai_flags = AI_PASSIVE; // Fill in my IP for me

     printf("server: getting address information status and checking it\n");

    if ((serverAddressStatus = getaddrinfo(NULL, argv[1], &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(serverAddressStatus));
        return 1;
    }

    // servinfo now points to a linked list of 1 or more struct addrinfos
    // ... do everything until you don't need servinfo anymore ....

     printf("server: looping to find address info and binding to the port\n");

    // Loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        // Make a socket
        if ((serverSocket = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        // lose the pesky "Address already in use" error message
        if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &yes,sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        // Bind it to the port we passed in to getaddrinfo()
        if (bind(serverSocket, p->ai_addr, p->ai_addrlen) == -1) {
            close(serverSocket);
            perror("server: bind");
            continue;
        }
        break;
    }

    if (p == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        return 2;
    }

    printf("server: clearing address info struct\n");

    freeaddrinfo(servinfo); // Free the linked-list

    printf("server: listening to incomming connections\n");

    if (listen(serverSocket, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    printf("server: waiting for connections...\n");

    int numberOfConnections = 0 ;
    double x = 100;

    while(1) {  // main accept() loop
        sin_size = sizeof their_addr;
        clientSocket = accept(serverSocket, (struct sockaddr *)&their_addr, &sin_size);
        if (clientSocket == -1) {
            perror("accept");
            continue;
        }

        // convert the IP to a string and print it
        inet_ntop(their_addr.ss_family,get_in_addr((struct sockaddr *)&their_addr),s, sizeof s);
        printf("server: got connection from %s\n", s);

        if (!fork()) { // this is the child process

            numberOfConnections++;

            //Add signal to kill the process
            signal(SIGALRM,killProcess);

            // Calculate time out
            x = x - numberOfConnections;
            x = x/100;
            x = x * DEFAULTTIMEOUT;

            // Handel client requests
            handelCleint(clientSocket , x);

            printf("server: connection to %s has been closed\n", s);

            numberOfConnections--;

            exit(0);
        }
        close(clientSocket);  // parent doesn't need this
    }

    return 0;
}

void handelCleint(int clientSocket , double timeOut){
    char requestBuffer[MAXDATASIZE]; // Buffer to revieve messages
    memset(requestBuffer, 0, MAXDATASIZE); // Clear the buffer
    int recvMsgSize = 0; // Size of recieved messages

    alarm(timeOut); //set alarm

    /*  Receive  message from client */
    if  ((recvMsgSize = recv(clientSocket, requestBuffer, MAXDATASIZE, 0)) < 0) {
        perror("recieve");
    }

    alarm(0); // stop alarm

    printf("cleint: %s\n",requestBuffer);

    while(recvMsgSize > 0){
        // Find out where everything is
        const char *start_of_path = strchr(requestBuffer, ' ') + 1;
        const char *start_of_query = strchr(start_of_path, ' ') + 1;

        // Get the right amount of memory
        char path[start_of_query - start_of_path];

        // Copy the strings into our memory
        strncpy(path, start_of_path,  start_of_query - start_of_path);

        // Null terminators (because strncpy does not provide them)
        path[sizeof(path)] = 0;

        printf("server: request path is %s\n",path);

        if(parse(requestBuffer) == 1){
            printf("server: request is GET\n");
            handelGetRequest(clientSocket);
        }else{
            printf("server: request is POST\n");

            // Find content lenght
            char contentLegth[100];
            int index = 0 ;
            char *contentLenght = "content-lenght =";
            char *pch = strstr(requestBuffer, contentLenght);
            pch = pch + 17;
            while(*pch != '\n'){
                contentLegth[index] = *pch;
                index++;
                pch++;
            }
            int x = convertStringToNum(contentLegth);

            handelPostRequest(clientSocket , x);
        }

        printf("------------------------------------------------------------------\n");
        printf("server: waiting for next request...\n");

        memset(requestBuffer, 0, MAXDATASIZE); // Clear the buffer

         alarm(timeOut); //set alarm

        // Check for more requests
        if  ((recvMsgSize = recv(clientSocket, requestBuffer, MAXDATASIZE, 0)) < 0) {
            perror("recieve");
        }

        printf("cleint: %s\n",requestBuffer);

         alarm(0); //stop alarm
    }
}

int parse(const char* line){
    if (strstr(line, "GET") != NULL) {
        return 1;
    }
    return 0;
}

int convertStringToNum(char *str){
    int result;
    int puiss;

    result = 0;
    puiss = 1;
    while (('-' == (*str)) || ((*str) == '+')){
        if (*str == '-')
            puiss = puiss * -1;
            str++;
    }
    while ((*str >= '0') && (*str <= '9')){
        result = (result * 10) + ((*str) - '0');
        str++;
    }
    return (result * puiss);
}

void handelGetRequest(int clientSocket){
    char sendbuffer[MAXDATASIZE]; // Buffer to send messages
    memset(sendbuffer, 0, MAXDATASIZE); // Clear the buffer
    int sendMsgSize = 0; // Size of sent messages

    //FILE *fp = fopen("/home/ahmed/Desktop/Programming Assignment 1/server/pic.png", "rb");
    FILE *fp = fopen("/home/ahmed/Desktop/Programming Assignment 1/server/server", "rb");
    if(fp == NULL){
        perror("File");
        send(clientSocket , "HTTP/1.1 404 Not Found\n" , 26, 0);
    }else{
        send(clientSocket , "HTTP/1.1 200 OK\r\n\r\n" , 19, 0);
        while( (sendMsgSize = fread(sendbuffer, 1, sizeof(sendbuffer), fp))>0 ){
            send(clientSocket, sendbuffer, sendMsgSize, 0);
            memset(sendbuffer, 0, MAXDATASIZE);
        }
    }
    fclose(fp);
}

void handelPostRequest(int clientSocket , int contentLength){
    char recieveBuffer[MAXDATASIZE]; // Buffer to revieve messages
    memset(recieveBuffer, 0, MAXDATASIZE); // Clear the buffer
    int recvMsgSize; // Size of recieved messages

    FILE* fp = fopen( "/home/ahmed/Desktop/Programming Assignment 1/server/client", "wb");
    int tot=0;
    if(fp != NULL){
        while( (recvMsgSize = recv(clientSocket, recieveBuffer, contentLength,0))> 0  && tot < contentLength) {
            tot+=recvMsgSize;
            fwrite(recieveBuffer, 1, recvMsgSize, fp);
            printf("client: %s\n",recieveBuffer);
            memset(recieveBuffer, 0, MAXDATASIZE);
        }

        printf("Received byte: %d\n",tot);
        if (recvMsgSize<0){
            perror("Receiving");
        }

        fclose(fp);

        // Send ACK to cleint
        send(clientSocket , "ACK\n" , 5, 0);

        printf("server: ACK\n");
    } else {
        perror("File");
    }
}
