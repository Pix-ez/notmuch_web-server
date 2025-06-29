#include "utils.h"
#include<stdio.h>
#include<error.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<pthread.h>


#define PORT 6969


int main(){
    int socket_fd; //file discriptor for socket by kernel 
    struct sockaddr_in server_addr;

    //creating socket
    if((socket_fd = socket(AF_INET, SOCK_STREAM,0))< 0){
        perror("Error occured while creating socket!!");
        exit(EXIT_FAILURE);
    };

    //config socket
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    //bind socket to port
    if (bind(socket_fd,
            (struct sockaddr *)&server_addr,
            sizeof(server_addr)) < 0){
                perror("Binding port failed");
                exit(EXIT_FAILURE);
            };
        
    //now listen for connections
    if (listen(socket_fd, 10) < 0){
        perror("listening failed");
        exit(EXIT_FAILURE);
    }
    printf("My simple web-server \nIt's listening on port %d\n", PORT);

    while(1){
        //store client info
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int *client_fd = malloc(sizeof(int)); //it will return pointer to number where file discripter is 

        //accept client req
        if((*client_fd = accept(socket_fd,
                                (struct sockaddr *)&client_addr,
                                &client_addr_len)) < 0){
            perror("client req accept failed");
            continue;
            }

        //create new thread for each req to handle it non blocking
        pthread_t thread_id;
        pthread_create(&thread_id, NULL, handle_req, (void *)client_fd);
        pthread_detach(thread_id);

    }
    close(socket_fd);
    return 0;
}