
#ifndef UTILS_H      // ← if not defined
#define UTILS_H      // ← then define it

#include<stdio.h>
#include<regex.h>
#include<string.h>
#include<ctype.h>
#include<fcntl.h>
#include<stdbool.h>
#include<sys/stat.h>
#include<errno.h>
#include<sys/socket.h>
#include<netinet/in.h>
// Function declarations
void *handle_req(void *arg);
char *url_decode(const char *src);
const char *get_file_extension(const char *file_name);
void build_http_response(const char *file_name,
    const char *file_ext,
    char *response,
    size_t *response_len);
const char *get_mime_type(const char *file_ext);

#define BUFFER_SIZE 104857600

#endif // UTILS_H     // ← end guard


void *handle_req(void *arg){
    int client_fd = *((int*) arg);
    char *buffer = (char *)malloc(BUFFER_SIZE * sizeof(char));
    
    //recive request data and store into buffer
    ssize_t byte_recived = recv(client_fd, buffer, BUFFER_SIZE,0);
    if (byte_recived > 0){
        //check if request if GET
        regex_t regex;
        regcomp(&regex, "^GET /([^ ]*) HTTP/1", REG_EXTENDED ); //search if get is in header text
        regmatch_t matches[2];
        
        //this will match our regex pattern in char array if text of req data 
        if (regexec(&regex, buffer, 2 ,matches, 0)==0){
            //extract filename from req and decode URL
            buffer[matches[1].rm_eo] = '\0';
            const char *url_encoded_file_name = buffer + matches[1].rm_so;
            char *file_name = url_decode(url_encoded_file_name);
            printf("user requested: %s\n", file_name);


            //get file extension
            char file_ext[32];
            strcpy(file_ext, get_file_extension(file_name));

            //build standard HTTP respone
            char *response = (char *)malloc(BUFFER_SIZE *2* sizeof(char));
            size_t response_len;
            build_http_response(file_name, file_ext,response, &response_len);

            //send HTTP respone to client
            int send_code;
            if((send_code = send(client_fd, response, response_len, 0)) == -1){
                perror("Error sending response");
              
            }


            //free
            free(file_name);
            free(response);

        }
        //free regex
        regfree(&regex);

    }
    //remove all resouces for req
    close(client_fd);
    free(arg);
    free(buffer);
    return NULL;
}

char *url_decode(const char *src){
        size_t src_len = strlen(src);
        char *decoded = malloc(src_len +1); //length plus 1 for terminate char 
        size_t decoded_len =0;

        //decode %2x to hex
        for (size_t i=0; i<src_len; i++){
            if(src[i]=='%' && i +2 <src_len){
                int hex_val;
                sscanf(src+i +1, '%2x', hex_val);
                decoded[decoded_len++] =hex_val;
                i +=2;

            }else{
                decoded[decoded_len++] = src[i];
            }
        }
        //add null terminator
        decoded[decoded_len] ='\0';
        return decoded;
}

const char *get_file_extension(const char *file_name){
    const char *dot = strrchr(file_name, '.');
    if (!dot || dot==file_name){
        return "";
    }
    return dot+1;
}

void build_http_response(const char *file_name,
                        const char *file_ext,
                        char *response,
                        size_t *response_len){
    
    //build HTTP header
    const char *mime_type = get_mime_type(file_ext);
    //printf("mime type: %s\n", mime_type);
    char *header = (char *)malloc(BUFFER_SIZE * sizeof(char));
    // snprintf(header, BUFFER_SIZE,
    //         "HTTP/1.1 200 OK\r\n",
    //         "Content-Type: %s\r\n",
    //         "\r\n",
    //         mime_type);
    snprintf(header, BUFFER_SIZE,
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: %s\r\n"
            "\r\n",
            mime_type);

    //if file does not exist response error
    int file_fd = open(file_name, O_RDONLY);
    if (file_fd == -1){
        snprintf(response, BUFFER_SIZE,
                "HTTP/1.1 404 Not Found\r\n",
                "Content-Type: text/plain\r\n",
                "\r\n"
                "404 Not Found");
        *response_len =strlen(response);
        return;
    }

    //get file size for content-length
    struct stat file_stat;
    fstat(file_fd, &file_stat);
    off_t file_size = file_stat.st_size;

    //copy header to response buffer
    *response_len = 0 ;
    memcpy(response, header, strlen(header));
    *response_len += strlen(header);

    //copy file data to response buffer 
    ssize_t bytes_read;
    while((bytes_read = read(file_fd,
                            response + *response_len,
                            BUFFER_SIZE- *response_len)) >0){
            *response_len +=bytes_read;
            }

    free(header);
    close(file_fd);
}

const char *get_mime_type(const char *file_ext){

    if(strcasecmp(file_ext, "html")== 0 || strcasecmp(file_ext, "htm")==0){    
        printf("get_mime_called: %s\t , mime= text/html\n", file_ext);
        return "text/html";
    } else if(strcasecmp(file_ext, "txt")== 0){    
        printf("get_mime_called: %s\t , mime= text/plain\n", file_ext);
        return "text/plain";
    } else if(strcasecmp(file_ext, "jpg")== 0 || strcasecmp(file_ext, "jepg")== 0){    
        printf("get_mime_called: %s\t , mime= image/jpeg\n", file_ext);
        return "image/jpeg";
    } else if(strcasecmp(file_ext, "png")== 0 ){    
        printf("get_mime_called: %s\t , mime= image/png\n", file_ext);
        return "image/png";
    } else{    
        printf("get_mime_called: %s\t , mime= application/octet-stream\n", file_ext);
        return "application/octet-stream";
    }
}