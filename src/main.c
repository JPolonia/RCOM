#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "alarm.h"
#include "applevel.h"

//#include <sys/types.h>
//#include <sys/stat.h>
//#include <fcntl.h>

//#define BAUDRATE B38400

//#define MAX_TRIES 3
//#define TIMEOUT 3

//linkLayer* ll;

int main(int argc, char** argv)
{
    //FILE *f = openFileTransmmiter("src/applevel.c");
    //sendData(f, 4);
    
    //FILE *f = openFileReceiver("src/teste.txt");
    //printf("TOTAL = %d\n", receiveData(f, 4)  );
 
    
    //char fileName[20];
    //int tamanho = receiveStart(4, fileName);
    //printf("tamanho = %d \nname = %s \n", tamanho, fileName);
    
    
    //exemplo de utilização RECEIVER::::_____________________________
    
    
    
    /*
    int fileSize;
    char fileName[];
    FILE *f;
    bytesReceived = 0;
    
    fileSize = receiveStart(fd, fileName); //espera por START e guarda tamanho e nome
    
    f = openFileReceiver(fileName); //cria ficheiro
    
    bytesReceived = receiveData(f, fd); //recebe e escreve dados
    
    if(bytesReceived == fileSize){
        printf("OK!!");
    }
    else{
        printf("NOT OK...");
    }
     
    fclose(f);
    */
    
    
    
    //termina exemplo_________________________________________________
    
    //exemplo de utilização TRANSMITTER::::___________________________
    
    
    
    
    /*
    int fileSize;
    char fileName[] = "o caminho para o ficheiro fica aqui :)";
    int bytesTransmitted = 0;
    
    FILE *f = openFileTransmmiter(fileName); //abre ficheiro
    
    fseek(f, 0L, SEEK_END); //obtem tamanho do ficheiro
    fileSize = ftell(f);
    rewind(f);
    
    sendStartPacket(fd, fileSize, fileName);
    
    sendData(f, fd);
    
    sendEndPacket(fd, fileSize, fileName);
    
    fclose(f);
    */
    
    
    
    
    //termina exemplo_________________________________________________
    
    

    
}
