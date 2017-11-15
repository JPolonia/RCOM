//
//  download.c
//  

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <assert.h>


int getArgs(char* arg, char** user, char** password, char** host, char** urlPath); //gets args from argv

int main(int argc, char* argv[]){
    
    int bytesSent = 0;
    int bytesReceived = 0;
    
    int s; //socket
    
    struct addrinfo hints;
    struct addrinfo *servinfo;  // will point to the results
    char ipstr[INET6_ADDRSTRLEN];
    int status = 0;
    
    char *user = NULL, *password = NULL, *host = NULL, *urlPath = NULL; //pointers to hold args
    char *arg; //pointer to hold duplicate of argv[1]
    
    if(argc != 2){
        printf("Usage: ./download ftp://...\n");
        return -1;
    }
    arg = strdup(argv[1]);
    if(getArgs(arg, &user, &password, &host, &urlPath) < 0 ){
        printf("Aborting, invalid URL!\n");
        return -1;
    }
    
    printf("\n\n\n");
    if(user != NULL) printf("User: %s\n", user);
    if(password != NULL) printf("Password: %s\n", password);
    if(host != NULL) printf("Host: %s\n", host);
    if(urlPath != NULL) printf("Url Path: %s\n", urlPath);
    printf("\n\n\n");
    
    memset(&hints, 0, sizeof hints); // make sure the struct is empty
    hints.ai_family = AF_INET;     // ipv4
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
    hints.ai_flags = AI_PASSIVE;
    
    // get ready to connect
    status = getaddrinfo(host, "ftp", &hints, &servinfo);
    assert(status == 0);
    
    //print ip address
    void *addr;
    struct sockaddr_in *ipv4 = (struct sockaddr_in*)servinfo->ai_addr;
    addr = &(ipv4->sin_addr);
    inet_ntop(servinfo->ai_family, addr, ipstr, sizeof ipstr);
    printf("IPv4: %s\n", ipstr);
    
    // get file descriptor
    s = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    assert(s != -1);
    
    //não é preciso fazer bind
    //If you are connect()ing to a remote machine and you don't care what your local port is (as is the case with telnet where you only care about the remote port), you can simply call connect(), it'll check to see if the socket is unbound, and will bind() it to an unused local port if necessary.
    
    //conectar!!
    status = connect(s, servinfo->ai_addr, servinfo->ai_addrlen);
    assert(status != -1);
    
    
    
    char resposta[1000];
    bytesReceived = recv(s, resposta, 100, 0);
    assert(bytesReceived > 0);
    printf("\n\n\nReceived: %s\n\n\n", resposta);
    
    char *msg = "USER anonymous\r\n";
    bytesSent = send(s, msg, strlen(msg), 0);
    assert(bytesSent == strlen(msg));
    printf("\n%d bytes sent!\n", bytesSent);
    
    bytesReceived = recv(s, resposta, 100, 0);
    assert(bytesReceived > 0);
    printf("\n\n\nReceived: %s\n\n\n", resposta);

    char *msg2 = "PASS up201428392@fe.up.pt\r\n";
    bytesSent = send(s, msg2, strlen(msg2), 0);
    assert(bytesSent == strlen(msg2));
    printf("\n%d bytes sent!\n", bytesSent);

char *msg3 = "PASV\r\n";
    bytesSent = send(s, msg3, strlen(msg3), 0);
    assert(bytesSent == strlen(msg3));
    printf("\n%d bytes sent!\n", bytesSent);

    bytesReceived = recv(s, resposta, 1000, 0);
    assert(bytesReceived > 0);
    printf("\n\n\nReceived: %s\n\n\n", resposta);
 bytesReceived = recv(s, resposta, 1000, 0);
    assert(bytesReceived > 0);
    printf("\n\n\nReceived: %s\n\n\n", resposta);
 bytesReceived = recv(s, resposta, 1000, 0);
    assert(bytesReceived > 0);
    printf("\n\n\nReceived: %s\n\n\n", resposta);
bytesReceived = recv(s, resposta, 1000, 0);
    assert(bytesReceived > 0);
    printf("\n\n\nReceived: %s\n\n\n", resposta);
bytesReceived = recv(s, resposta, 1000, 0);
    assert(bytesReceived > 0);
    printf("\n\n\nReceived: %s\n\n\n", resposta);
    
    
    
    
    
    
    status = close(s); // fecha socket
    assert(status != -1);
    
    freeaddrinfo(servinfo); //desaloca memória
    
    free(user);
    free(password);
    free(host);
    free(urlPath);
    
	return 0;
}

int getArgs(char* arg, char** user, char** password, char** host, char** urlPath){
    char *tmp;
    int i;
    
    char header[7];
    strncpy(header, arg, 6);
    header[6] = '\0';
    if(strcmp(header, "ftp://") != 0){
        return -1;
    }
    
    tmp = arg+6;
    for(i=0; tmp[i] != ':' && tmp[i] != '@' && tmp[i] != '/' && tmp[i] != '\0'; i++); //vai até ao proximo carater de separação
    switch (tmp[i]) {
        case ':': //é nome de utilizador e vem pass a seguir
            tmp[i] = '\0';
            *user = strdup(tmp);
            tmp = tmp+i+1;
            for(i=0; tmp[i] != ':' && tmp[i] != '@' && tmp[i] != '/' && tmp[i] != '\0'; i++);
            if(tmp[i] != '@'){
                return -1;
            }
            else{
                tmp[i] = '\0';
                *password = strdup(tmp);
                tmp = tmp+i+1;
                for(i=0; tmp[i] != ':' && tmp[i] != '@' && tmp[i] != '/' && tmp[i] != '\0'; i++);
                if(tmp[i] != '/'){
                    return -1;
                }
                else{
                    tmp[i] = '\0';
                    *host = strdup(tmp);
                    tmp = tmp+i+1;
                    for(i=0; tmp[i] != ':' && tmp[i] != '@' && tmp[i] != '/' && tmp[i] != '\0'; i++);
                    if(tmp[i] != '\0'){
                        return -1;
                    }
                    else{
                        *urlPath = strdup(tmp);
                        return 4;
                    }
                }
            }
            break;
        case '@': //é nome de utilizador e não vem pass a seguir
            tmp[i] = '\0';
            *user = strdup(tmp);
            tmp = tmp+i+1;
            for(i=0; tmp[i] != ':' && tmp[i] != '@' && tmp[i] != '/' && tmp[i] != '\0'; i++);
            if(tmp[i] != '/'){
                return -1;
            }
            else{
                tmp[i] = '\0';
                *host = strdup(tmp);
                tmp = tmp+i+1;
                for(i=0; tmp[i] != ':' && tmp[i] != '@' && tmp[i] != '/' && tmp[i] != '\0'; i++);
                if(tmp[i] != '\0'){
                    return -1;
                }
                else{
                    *urlPath = strdup(tmp);
                    return 3;
                }
            }
        case '/': //é host, não veio utilizador e vem caminho url a seguir
            //printf("Encontrou '/' \n");
            tmp[i] = '\0';
            *host = strdup(tmp);
            tmp = tmp+i+1;
            for(i=0; tmp[i] != ':' && tmp[i] != '@' && tmp[i] != '/' && tmp[i] != '\0'; i++);
            if(tmp[i] != '\0'){
                return -1;
            }
            else{
                *urlPath = strdup(tmp);
                return 2;
            }
            
        default:
            return -1;
            break;
    }
    return -1;
}
