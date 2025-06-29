#include "utils.h"
#include <sys/select.h>
#include <fcntl.h>

#include<stdio.h>
#include<error.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<pthread.h>
#include<signal.h>
#include<unistd.h>
#include<stdatomic.h>

#define PORT 6969
#define MAX_THREADS 1024

volatile sig_atomic_t keep_running = 1;
pthread_t thread_list[MAX_THREADS];
int thread_count = 0;
pthread_mutex_t thread_list_mutex = PTHREAD_MUTEX_INITIALIZER;

void handle_signal(int sig) {
    keep_running = 0;
}

volatile sig_atomic_t sigint_received = 0;

void handle_sigint(int sig) {
    sigint_received = 1;  // just set the flag
}

    
int main(){
     
    int socket_fd; //file discriptor for socket by kernel 
    struct sockaddr_in server_addr;

    //creating socket
    if((socket_fd = socket(AF_INET, SOCK_STREAM,0))< 0){
        perror("Error occured while creating socket!!");
        exit(EXIT_FAILURE);
    };

    int opt = 1; //set socket option to 1 to reuse port usually it wait 60 sec to flush packet data auto
    if(setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))){
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }

    //set socket non blocking
    fcntl(socket_fd, F_SETFL, O_NONBLOCK);

    //config socket
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    signal(SIGINT, handle_signal);
    // signal(SIGTERM, handle_signal);

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

    // while(1){
    //     //store client info
    //     struct sockaddr_in client_addr;
    //     socklen_t client_addr_len = sizeof(client_addr);
    //     int *client_fd = malloc(sizeof(int)); //it will return pointer to number where file discripter is 
        
    //     while(keep_running){
    //     //accept client req
    //     if((*client_fd = accept(socket_fd,
    //                             (struct sockaddr *)&client_addr,
    //                             &client_addr_len)) < 0){
    //         perror("client req accept failed");
    //         continue;
    //         }

    //     //create new thread for each req to handle it non blocking
    //     // pthread_t thread_id;
    //     // pthread_create(&thread_id, NULL, handle_req, (void *)client_fd);
    //     // pthread_detach(thread_id);

    //     pthread_t thread_id;
    //     pthread_create(&thread_id, NULL, handle_req,)
    //     }

    // }

    while (keep_running)
    {
        //check for user quit signal
        if (sigint_received) {
        printf("\nDo you really want to exit? (y/n): ");
        // fflush(stdin);
        char input[4];
        fgets(input, sizeof(input), stdin);
        if (input[0] == 'y' || input[0] == 'Y') {
            keep_running = 0;
            break;
        } else {
            sigint_received = 0;
        }
    }
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(socket_fd, &read_fds);
         struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        int activity = select(socket_fd + 1, &read_fds, NULL, NULL, &timeout);
        if (activity < 0) {
            if (errno == EINTR) continue;
            perror("select failed");
            break;
        } else if (activity == 0) {
            continue; // timeout, check sigint_received again
        }
        //store client info
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int *client_fd = malloc(sizeof(int)); //it will return pointer to number where file discripter is 
   
        //accept client req
        *client_fd = accept(socket_fd,
                                (struct sockaddr *)&client_addr,
                                &client_addr_len);
        if(*client_fd < 0){
            if (errno == EINTR){
                free(client_fd);
                continue;
            }
            perror("client req accept failed");
            free(client_fd);
            continue;
            }
        pthread_t thread_id;
        if(pthread_create(&thread_id, NULL, handle_req, (void *)client_fd) != 0){
            perror("pthread_create failed");
            free(client_fd);
            continue;
        }

        pthread_mutex_lock(&thread_list_mutex);
        if(thread_count < MAX_THREADS){
            thread_list[thread_count++] =thread_id;
        }else{
            pthread_detach(thread_id);
        }
        pthread_mutex_unlock(&thread_list_mutex);


    }
    
    printf("Shutting down server gracefully...\n");
    close(socket_fd);
    //join threads
    pthread_mutex_lock(&thread_list_mutex);
    for(int i=0; i<thread_count; i++){
        pthread_join(thread_list[i], NULL);
    }
    pthread_mutex_unlock(&thread_list_mutex);
    printf("Bye!\n");
    return 0;
}