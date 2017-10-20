#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <time.h>
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
	clock_t start, end;
	double cpu_time_used;
	

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
        if(scanf("%s", fileName) < 0){
            printf("Erro em scanf()\n");
            return -1;
        }
        
		//Começar a contar tempo
		start = clock();

        if(llopen(fd, TRANSMITTER) < 0 ){
            printf("llopen() falhou\n");
            sleep(1);
            return 0;
        }
        
        f = openFileTransmmiter(fileName); //abre ficheiro
        if (f == NULL) {
            printf("Erro ao abrir ficheiro\n");
            return 0;
        }
        
        fseek(f, 0L, SEEK_END); //obtem tamanho do ficheiro
        fileSize = ftell(f);
        rewind(f);
        
        if(sendStartPacket(fd, fileSize, fileName) < 0){
            fclose(f);
            printf("Erro ao enviar START packet\n");
            return 0;
        }
        
        bytesTransmitted = sendData(f, fd);
        
        if(bytesTransmitted < 0){
            fclose(f);
            printf("Erro ao enviar ficheiro\n");
            return 0;
        }
        
        if(sendEndPacket(fd, fileSize, fileName) < 0){
            fclose(f);
            printf("Erro ao enviar END packet\n");
            return 0;
        }

		
		
        
        fclose(f);
        
        if(llclose(fd, TRANSMITTER)<0){
            printf("llclose() falhou\n");
        }

		//Fim da contagem do tempo com printf
		end = clock();
        
        printf("File size = %d\nTransmitted %d bytes\n", fileSize, bytesTransmitted);
		printf("Tempo de envio: %f\n", ((double)(end-start))/CLOCKS_PER_SEC);


		
	}
	else if(mode == RECEIVER){
	
        if(llopen(fd, RECEIVER) < 0){
            printf("llopen() falhou\n");
            sleep(1);
            return 0;
        }
    
        //printf("llopen() OK\n");
        
        fileSize = receiveStart(fd, fileName); //espera por START e guarda tamanho e nome
        if(fileSize < 1){
            printf("Erro ao receber START packet\n");
            return 0;
        }
        
        //printf("receiveStart() OK\n");
        
        f = openFileReceiver(fileName); //cria ficheiro
        if (f == NULL) {
            printf("Erro ao abrir ficheiro\n");
            return 0;
        }
        
        //printf("openFileReceiver() OK\n");
        
        bytesReceived = receiveData(f, fd); //recebe e escreve dados no ficheiro
        if(bytesReceived < 0){
            printf("Erro ao receber ficheiro\n");
            fclose(f);
            return 0;
        }
        
        fclose(f);
        
        if(llclose(fd, RECEIVER)<0){
            printf("llclose() falhou\n");
        }
        
        printf("File size = %d\n", fileSize); //para confirmar que chegou tudo
        printf("Received %d bytes\n", bytesReceived);

	}

    sleep(1);
   
    return 0;
}
