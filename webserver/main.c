#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

#define PORT 80
#define BUFFER_SIZE 65535

char* strnlcpy(const char* source) {
    if (source == NULL || *source == '\0')
        return NULL;

    int length = 0;
    const char* p = source;
    while (*p != '\n' && *p != '\0') {
        length++;
        p++;
    }

    char* result = (char*)malloc(length + 1);
    if (result == NULL)
        return NULL;
    strncpy(result, source, length);
    result[length] = '\0';
    return result;
}

void* request_handler(void *arg) {
    int client_socket = *((int *)arg);
    char buffer[BUFFER_SIZE] = {0,};
    int valread = read(client_socket, buffer, BUFFER_SIZE);
    if (valread <= 0) {
        close(client_socket);
        return NULL;
    }


    char* token = strtok(buffer, "\r\n");
    printf("\nConnected: %s\n", token);

    char* http_method = strtok(token, " ");
    char* requested_resource = strtok(NULL, " ");
    
    if (strcmp(http_method, "GET") == 0) {
        FILE* file;
        char* response_body;
        int file_size;
        char* query_params = NULL;

        if (strcmp(requested_resource, "/") == 0) {
            file = fopen("./src/index.html", "r");
        } 
        else if (strcmp(requested_resource, "/favicon.ico") == 0) {
            file = fopen("./src/favicon.ico", "r");
        } 
        else if (strcmp(requested_resource, "/about") == 0) {
            file = fopen("./src/about.html", "r");
        } 
        else if (strcmp(requested_resource, "/post") == 0) {
            file = fopen("./src/post.html", "r");
        } 
        else if (strncmp(requested_resource, "/result",strlen("/result")) == 0) {
            if (strchr(requested_resource, '?'))
                query_params = strchr(requested_resource, '?')+1;
            file = fopen("./src/result.html", "r");
        } 
        else {
            file = fopen("./src/404.html", "r");
        }

        if (file == NULL) {
            perror("Error opening file");
            exit(EXIT_FAILURE);
        }

        fseek(file, 0, SEEK_END);
        file_size = ftell(file);
        rewind(file);

        int query_size = 0;
        char query_set[1000];
        if(query_params){
            char* query=strtok(query_params,"&");
            while(query !=NULL){ 
                sprintf(query_set+query_size,"<li>%s\n",query);
                query_size += strlen(query)+5;
                query = strtok(NULL, "&");
            }
        }
        response_body = malloc(file_size + query_size +1);
        if (response_body == NULL) {
            perror("Memory allocation failed");
            exit(EXIT_FAILURE);
        }

        fread(response_body, 1, file_size, file);
        strncpy(response_body+file_size,query_set,query_size);
        response_body[file_size+query_size] = '\0';

        fclose(file);

        char response_headers[1000];
        snprintf(response_headers, sizeof(response_headers), "HTTP/1.1 200 OK\nContent-Type: text/html; charset=UTF-8\nContent-Length: %d\n\n", file_size+query_size);

        send(client_socket, response_headers, strlen(response_headers), 0);
        send(client_socket, response_body, file_size+query_size, 0);

        free(response_body);
    } 
    else if (strcmp(http_method, "POST") == 0) {
        if (strcmp(requested_resource, "/post") == 0) {
            char* substr = buffer;
            for(int i = 0; i < 3; i++){
                substr = substr + strlen(substr)+1;
            }
            char* content_length_str = strnlcpy(strstr(substr, "Content-Length: ")+strlen("Content-Length: "));
            char* postdata_start = strstr(substr, "\r\n\r\n")+4;
            
            for(int i = 0; i < 65535; i++){
                printf("%c",*(buffer+i));
            }

            if (content_length_str != NULL && postdata_start != NULL) {
                int content_length = atoi(content_length_str);
                printf("content length :%d\n", content_length);
                char* postdata = malloc(content_length+10);
                strncpy(postdata,postdata_start,content_length);

                printf("postdata: %s\n", postdata);


                char response_headers[1000];
                sprintf(response_headers,"HTTP/1.1 301 Moved Permanently\nContent-Type: text/html; charset=UTF-8\nLocation: /result?%s\n\n",postdata);
                send(client_socket, response_headers, strlen(response_headers), 0);
                send(client_socket, postdata_start, strlen(postdata_start), 0);
            }
        }
    } 
    else {
        printf("Not allowed Method\n");
    }

    close(client_socket);

    return NULL;
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in sin;
    socklen_t len = sizeof(sin);
    if (getsockname(server_fd, (struct sockaddr *)&sin, &len) == -1)
        perror("getsockname");
    else
        printf("\nServer running on %s:%d\n\n", inet_ntoa(sin.sin_addr), ntohs(sin.sin_port));

    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    while (1) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            continue;
        }

        pthread_t tid;
        if (pthread_create(&tid, NULL, request_handler, &new_socket) != 0) {
            perror("pthread_create");
            printf("Failed to create thread, but keeping the connection open.\n");
        }
        pthread_detach(tid);
    }

    return 0;
}
