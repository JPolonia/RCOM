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
#include <fcntl.h>

#include <assert.h>

#define MAX_DIR_CHANGES 20
#define ACK_LEN 1000
#define MAX_CMD_LEN 100

int getDirs(char* path, char **dirs, int maxSteps);

int establishDataConnection(char* host, char* port, struct addrinfo **servinfo_data);

int sendCommand(int socket, char* msg);

int getDataPort(char* buff);

int getArgs(char* arg, char** user, char** password, char** host, char** urlPath); //gets args from argv

int getLine(int socket, char* ack, int buffSize); //recebe resposta do servidor

int main(int argc, char* argv[]){
    
    char cmd[MAX_CMD_LEN]; //string to construct commands
 
    int s; //socket
    
    int s_data; //socket for data
    
    struct addrinfo hints;
    struct addrinfo *servinfo;  // will point to the results
    char ipstr[INET6_ADDRSTRLEN];
    int status = 0;
    
    char *user = NULL, *password = NULL, *host = NULL, *urlPath = NULL; //pointers to hold args
    char *arg; //pointer to hold duplicate of argv[1]
    
    char *dirs[MAX_DIR_CHANGES]; //string array to hold different directories that have to be changed to get to the wanted file
    
    if(argc != 2){
        printf("Usage: ./download ftp://...\n");
        return -1;
    }
    arg = strdup(argv[1]);
    if(getArgs(arg, &user, &password, &host, &urlPath) < 0 ){
        printf("Aborting, invalid URL!\n");
        return -1;
    }
    
    int steps = getDirs(urlPath, dirs, MAX_DIR_CHANGES);
    if(steps < 0){
        return -1;
    }
    
    printf("\n\n\n");
    if(user != NULL){
        printf("User: %s\n", user);
    }
    else{
        user = strdup("anonymous");
        printf("User set to default: %s\n", user);
    }
    if(password != NULL){
        printf("Password: %s\n", password);
    }
    else{
        password = strdup("example@mail.com");
        printf("Password set to default: %s\n", password);
    }
    if(host != NULL) printf("Host: %s\n", host);
    if(urlPath != NULL) printf("Url Path: %s\n", urlPath);
    printf("\n\n\n");
    
    
    
    
    
    memset(&hints, 0, sizeof hints); // make sure the struct is empty
    hints.ai_family = AF_INET;     // ipv4 only
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
    printf("IPv4: %s\n\n", ipstr);
    
    // get file descriptor
    s = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    assert(s != -1);
    
    
    //não é preciso fazer bind
    //If you are connect()ing to a remote machine and you don't care what your local port is (as is the case with telnet where you only care about the remote port), you can simply call connect(), it'll check to see if the socket is unbound, and will bind() it to an unused local port if necessary.
    
    //conectar!!
    status = connect(s, servinfo->ai_addr, servinfo->ai_addrlen);
    assert(status != -1);
    
    char resposta[ACK_LEN];
    
    getLine(s, resposta, ACK_LEN);
    printf("< %s\n\n", resposta);
    
    strcpy(cmd, "USER ");
    sendCommand(s, strcat(strcat(cmd, user), "\r\n"));
    
    getLine(s, resposta, ACK_LEN);
    printf("< %s\n\n", resposta);
    
    strcpy(cmd, "PASS ");
    sendCommand(s, strcat(strcat(cmd, password), "\r\n"));
    
    getLine(s, resposta, ACK_LEN);
    printf("< %s\n\n", resposta);
  
    /*sendCommand(s, "CWD pub\r\n");
    
    getLine(s, resposta, ACK_LEN);
    printf("< %s\n\n", resposta);
    
    sendCommand(s, "CWD Docs\r\n");
    
    getLine(s, resposta, ACK_LEN);
    printf("< %s\n\n", resposta);

    sendCommand(s, "CWD 5DPO\r\n");
    
    getLine(s, resposta, ACK_LEN);
    printf("< %s\n\n", resposta);*/
    
    int i;
    for(i=0; i<steps-1; i++){ //navega até diretório que contem ficheiro desejado
        strcpy(cmd, "CWD ");
        sendCommand(s, strcat(strcat(cmd, dirs[i]), "\r\n"));
        
        getLine(s, resposta, ACK_LEN);
        printf("< %s\n\n", resposta);
    }

	sendCommand(s, "TYPE I\r\n"); //envia comando TYPE IMAGE, para mudar modo 
    
    getLine(s, resposta, ACK_LEN); //resposta contem porta a que devemos ligar
    printf("< %s\n\n", resposta);
    
    sendCommand(s, "PASV\r\n"); //envia comando PASV
    
    getLine(s, resposta, ACK_LEN); //resposta contem porta a que devemos ligar
    printf("< %s\n\n", resposta);
    
    int port = getDataPort(resposta);
    char s_port[10];
    sprintf(s_port, "%d", port);
    
    printf("Port for data connection is %s\n\n", s_port);
    
    //data connection
    struct addrinfo *servinfo_data;
    
    
    memset(&hints, 0, sizeof hints); // make sure the struct is empty
    hints.ai_family = AF_INET;     // ipv4 only
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
    hints.ai_flags = AI_PASSIVE;
    
    status = getaddrinfo(host, s_port, &hints, &servinfo_data);
    assert(status == 0);
    
    s_data = socket(servinfo_data->ai_family, servinfo_data->ai_socktype, servinfo_data->ai_protocol);
    assert(s != -1);
    
    status = connect(s_data, servinfo_data->ai_addr, servinfo_data->ai_addrlen);
    assert(status != -1);
    
    //connection made
    
    strcpy(cmd, "RETR ");
    sendCommand(s, strcat(strcat(cmd, dirs[steps-1]), "\r\n"));
    
    
    //sendCommand(s, "NLST\r\n");
    
    getLine(s, resposta, 1000);
    printf("< %s\n\n", resposta);
    
    
    FILE* f = fopen(dirs[steps-1], "wb");
    assert(f != NULL);
    
    char c;
    while(recv(s_data, &c, 1, MSG_DONTWAIT)!=0){
        //printf("%c", c);
        fwrite(&c, 1, 1, f);
    }
    
    fclose(f);
    
    status = close(s_data); // fecha socket dados
    assert(status != -1);
    
    freeaddrinfo(servinfo_data); //desaloca memória
    
    
    
    status = close(s); // fecha socket controlo
    assert(status != -1);
    
    freeaddrinfo(servinfo); //desaloca memória
    
    free(user);
    free(password);
    free(host);
    free(urlPath);
    
    for(i=0;i<steps;i++){
        free(dirs[i]);
    }
    
	return 0;
}

int getDirs(char* path, char **dirs, int maxSteps){
    int i = 0;
    int j = 0;
    int nsteps = 0;
    char *pathdup = strdup(path);
    while(1){
        if(pathdup[i] == '/'){
            pathdup[i] = '\0';
            dirs[nsteps] = strdup(pathdup+j);
            j=i+1;
            nsteps++;
        }
        else if(pathdup[i] == '\0'){
            dirs[nsteps] = strdup(pathdup+j);
            nsteps++;
            break;
        }
        if(nsteps == maxSteps){
            printf("Url Path can't have more that %d dir jumps\n", maxSteps-1);
            return -1;
        }
        i++;
    }
    free(pathdup);
    return nsteps;
}




int sendCommand(int socket, char* msg){
    int bytesSent = send(socket, msg, strlen(msg), 0);
    assert(bytesSent == strlen(msg));
    printf("> %s\n", msg);
    return 0;
}


int getDataPort(char* buff){
    int i=0;
    char *a, *b;
    while(buff[i]!='(') i++;
    i++;
    while(buff[i]!=',') i++;
    i++;
    while(buff[i]!=',') i++;
    i++;
    while(buff[i]!=',') i++;
    i++;
    while(buff[i]!=',') i++;
    i++;
    a = buff+i;
    while(buff[i]!=',') i++;
    i++;
    b = buff+i;
    return (strtol(a, NULL, 10)<<8) + strtol(b, NULL, 10);
}


int getLine(int socket, char* buff, int buffSize){
    char c;
    int byteReceived = 0;
    int i = 0;
    int longmsg = 0;
    int endlongmsg = 0;
    //int testpos = 0;
    int j = 0;
    while(1){
        byteReceived = recv(socket, &c, 1, MSG_DONTWAIT);
        if(byteReceived == 1){
            if((c == '\n' && longmsg == 0) || (c == '\n' && longmsg == 1 && endlongmsg == 1)) {
                if(i <= buffSize-1){
                    buff[i] = '\0';
                }
                break;
            }
            else if(i == buffSize-1){
                buff[i] = '\0';
            }
            else if(i < buffSize-1){
                buff[i] = c;
            }
            if(i==3){
                if(buff[i] == '-'){
                    longmsg = 1;
                }
            }
            if(longmsg == 1 && i>3){
                if(buff[i] == buff[j]){
                    j++;
                }
                else{
                    j=0;
                }
            }
            if(j >= 3) endlongmsg = 1;
            i++;
        }
    }
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
                    for(i=0; tmp[i] != ':' && tmp[i] != '@' && tmp[i] != '\0'; i++);
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
                for(i=0; tmp[i] != ':' && tmp[i] != '@' && tmp[i] != '\0'; i++);
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
            for(i=0; tmp[i] != ':' && tmp[i] != '@' && tmp[i] != '\0'; i++);
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
