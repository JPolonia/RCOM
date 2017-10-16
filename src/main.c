#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include "linklayer.h"
#include "alarm.h"
#include "applevel.h"



#define BAUDRATE B38400

#define MAX_TRIES 3
#define TIMEOUT 3

linkLayer* ll;

int main(int argc, char** argv)
{
    int fd, e;
    unsigned char mode;
    int fileSize = 0;
    char fileName[30];
    FILE *f;
    int bytesReceived = 0;
    int bytesTransmitted = 0;
	

    if ( (argc < 3) || 
         ((strcmp("/dev/ttyS0", argv[1])) && (strcmp("/dev/ttyS1", argv[1])) ) ||
         ((strcmp("TRANSMITTER", argv[2])) && (strcmp("RECEIVER", argv[2])) )) {
      printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1 MODE\n");
      exit(1);
    }


    //Initialize linkLayer
    e = initLinkLayer(argv[1],BAUDRATE,0,TIMEOUT,MAX_TRIES);
    if (e <0) { printf("Couldnt initialize linklayer\n");; exit(-1); }

    //Open Serial Port
    fd = openSerialPort(ll->port);
    if (fd <0) {perror(ll->port); exit(-1); }
   

    //Initialize Termios
    e = initTermios(fd);
    if (e <0) { printf("Couldnt initialize Termios\n");; exit(-1); }

    //Initialize alarm
	initAlarm();

    
    mode = (strcmp("TRANSMITTER", argv[2])) ? RECEIVER : TRANSMITTER;

	if(mode == TRANSMITTER){
        
        printf("Insira o caminho para o ficheiro que pretende transferir:\n");
        assert(scanf("%s", fileName) >= 0);
        
        llopen(fd, TRANSMITTER);
        
        f = openFileTransmmiter(fileName); //abre ficheiro
        
        fseek(f, 0L, SEEK_END); //obtem tamanho do ficheiro
        fileSize = ftell(f);
        rewind(f);
        
        sendStartPacket(fd, fileSize, fileName);
        
        bytesTransmitted = sendData(f, fd);
        
        sendEndPacket(fd, fileSize, fileName);
        
        fclose(f);
        
        printf("File size = %d\nTransmitted %d bytes\n", fileSize, bytesTransmitted);

		
	}
	else if(mode == RECEIVER){
	
        llopen(fd, RECEIVER);
        
        fileSize = receiveStart(fd, fileName); //espera por START e guarda tamanho e nome
        
        f = openFileReceiver(fileName); //cria ficheiro
        
        bytesReceived = receiveData(f, fd); //recebe e escreve dados no ficheiro
        
        fclose(f);
        
        printf("File size = %d\n", fileSize); //para confirmar que chegou tudo
        printf("Received %d bytes\n", bytesReceived);

	}

    sleep(1);
   
    return 0;
}
