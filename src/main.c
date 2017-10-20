#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <assert.h>
#include <time.h>
#include "linklayer.h"
#include "alarm.h"
#include "applevel.h"
#include "testes.h"

#define ACTIVE 1

#define BAUDRATE B38400

#define MAX_TRIES 3
#define TIMEOUT 3



linkLayer* ll;

void teste_cli();

int main(int argc, char** argv)
{
    int fd, e;
    unsigned char mode;
    int fileSize = 0;
    char fileName[30];
    FILE *f;
    int bytesReceived = 0;
    int bytesTransmitted = 0;

	//Para contar tempo
	long start,end;
	struct timeval timecheck;
	

    if ( (argc < 3) || 
         ((strcmp("/dev/ttyS0", argv[1])) && (strcmp("/dev/ttyS1", argv[1])) ) ||
         ((strcmp("TRANSMITTER", argv[2])) && (strcmp("RECEIVER", argv[2])) )) {
      printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1 MODE\n");
      exit(1);
    }


    /*//Initialize linkLayer
    e = initLinkLayer(argv[1],BAUDRATE,0,TIMEOUT,MAX_TRIES, MAX_SIZE);
    if (e <0) { printf("Couldnt initialize linklayer\n");; exit(-1); }*/
	teste_cli(argv);

    //Open Serial Port
    fd = openSerialPort(ll->port);
    if (fd <0) {perror(ll->port); exit(-1); }
   

    //Initialize Termios
    e = initTermios(fd);
    if (e <0) { printf("Couldnt initialize Termios\n");; exit(-1); }

    //Initialize alarm
	initAlarm();

	//Initialize testes
	initStat();

    
    mode = (strcmp("TRANSMITTER", argv[2])) ? RECEIVER : TRANSMITTER;

	if(mode == TRANSMITTER){
        
        printf("Insira o caminho para o ficheiro que pretende transferir:\n");
        if(scanf("%s", fileName) < 0){
            printf("Erro em scanf()\n");
            return -1;
        }
	
		//Começar a contar tempo
		gettimeofday(&timecheck,NULL);
		start = (long)timecheck.tv_sec * 1000 + (long)timecheck.tv_usec / 1000;

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

		printf("File size = %d\nTransmitted %d bytes\n", fileSize, bytesTransmitted);

		//Fim da contagem do tempo
		gettimeofday(&timecheck,NULL);
		end = (long)timecheck.tv_sec * 1000 + (long)timecheck.tv_usec / 1000;
	   
		printf("%ld milliseconds elapsed\n", (end-start));
		
	}
	else if(mode == RECEIVER){

		//Começar a contar tempo
		start = time(NULL);

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


void teste_cli(char** argv){
	int e;

	char command;
	int mode = 0; //=1 -> testes ativados

	//FER
	double headerErrorRate;
	double packetErrorRate;

	//T_PROP
	int t_prop; //em milisegundos

	//C -> BaudRate
	int nbaud;
	int baud;

	//Tamanho da trama
	int max_size;//OK

	
	int max_tries;//OK
	int time_out;//OK

	printf("Qual o tempo de timeout? (secs)\n");
	scanf("%d", &time_out);
	assert(time_out >= 0);

	printf("Qual o número máximo de tentativas após timeout?\n");
	scanf("%d", &max_tries);
	assert(max_tries >= 0);

	printf("Qual o tamanho máximo que a trama pode ter?\n");
	scanf("%d", &max_size);
	assert(max_size > 20);

	printf("Pretende entrar em modo de teste? (s/n)");
	scanf("%c", &command);

	if(command == 's'){
		mode = ACTIVE;
		
		printf("Qual a probabilidade de haver erro no cabeçalho das tramas I? (entre 0 e 1)\n");
		scanf("%lf", &headerErrorRate);
		assert(headerErrorRate <= 0 || headerErrorRate >=1);
			
		printf("Qual a probabilidade de haver erro nos dados das tramas I? (entre 0 e 1)\n");
		scanf("%lf", &packetErrorRate);
		assert(packetErrorRate > 0 && packetErrorRate < 1);

		printf("Qual o atraso adicional vai ser inserido?\n");
		scanf("%d", &t_prop);
		assert(t_prop > 0);

		printf("Qual dos seguintes valores pretende inserir?\n");
		printf("1-2400 2-4800 3-9600 4-19200 5-38400 6-57600 7-115200 8-230400 9-460800?\n");
		scanf("%d", &nbaud);
		assert(nbaud >= 1 && nbaud <= 9);

		switch (nbaud){
			case 1:	
				baud = B2400;
				break;
			case 2:	
				baud = B4800;
				break;
			case 3:	
				baud = B9600;
				break;
			case 4:	
				baud = B19200;
				break;
			case 5:	
				baud = B38400;
				break;
			case 6:	
				baud = B57600;
				break;
			case 7:	
				baud = B115200;
				break;
			case 8:	
				baud = B230400;
				break;
			case 9:	
				baud = B460800;
				break;
	
		}
		
	}
	else{
		mode = !ACTIVE;
	}	

	

	//preencher estruturas_______________________________________________

	//Initialize linkLayer

	if(mode == ACTIVE){ //MODO TESTE
		e = initLinkLayer(argv[1], baud,0, time_out, max_tries, max_size);
		if (e <0) { printf("Couldnt initialize linklayer\n"); exit(-1); }
	}
	else{ //MODO NORMAL
		e = initLinkLayer(argv[1],BAUDRATE,0,TIMEOUT,MAX_TRIES, max_size);
    	if (e <0) { printf("Couldnt initialize linklayer\n");; exit(-1); }
	}



}


