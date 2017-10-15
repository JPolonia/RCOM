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
    int fd, e, i; //dataSize = 0;
    char mode;
	//unsigned char BCC = 0;
	//char c;
	char dataToSend[200];
    char dataReceived[200];
    //char file_name[] = "ola";
    //char read_str[255];
    //int fd_file;
    int dataReceivedLen = 0;
	

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
		
		dataToSend[0] = 0x01;
		dataToSend[1] = 0x02;
		dataToSend[2] = 0x03;
		dataToSend[3] = 0x7e;
		dataToSend[4] = 0x04;
		dataToSend[5] = 0x05;
		dataToSend[6] = 0x7d;
		dataToSend[7] = 0x06;
		dataToSend[8] = 0x07;
		dataToSend[9] = 0x08;

		//assert(write(fd, dataToSend, 10) == 10);
     
        
        llopen(fd, TRANSMITTER);
        
        llwrite(fd, dataToSend, 10);
        llwrite(fd, dataToSend, 10);
        llwrite(fd, dataToSend, 10);

		
	}
	else if(mode == RECEIVER){
		/*
		readpacket(fd, dataReceived, RECEIVER);
		
		for(i = 0; i < 15; i++){
			printf("dataReceived[%d] = 0x%02x \n", i, dataReceived[i]);
		}
		*/
        
        llopen(fd, RECEIVER);
        
        dataReceivedLen = llread(fd, dataReceived);
        
        for(i = 0; i < dataReceivedLen; i++){
            printf("dataReceived[%d] = 0x%02x\n", i, dataReceived[i]);
        }
        dataReceivedLen = llread(fd, dataReceived);
        
        for(i = 0; i < dataReceivedLen; i++){
            printf("dataReceived[%d] = 0x%02x\n", i, dataReceived[i]);
        }
        dataReceivedLen = llread(fd, dataReceived);
        
        for(i = 0; i < dataReceivedLen; i++){
            printf("dataReceived[%d] = 0x%02x\n", i, dataReceived[i]);
        }

	}
	/*
	buff[0] = 0x7e;
	buff[1] = 'a'; //CAGA
	buff[2] = 'b'; //CAGA
	buff[3] = 'c'; //CAGA
	buff[4] = 'd'; //DADOS COMEÇAM
	buff[5] = 0x7d;
	buff[6] = 0x5d;
	buff[7] = 'g';
	buff[8] = 'h'; //BCC
	buff[9] = 0x7e; 
	 //quero tamanho = 5

	dataSize = stuffing(buff, buffStuff, 10);
	
	for(i = 0; i < dataSize; i++){
		printf("buffStuff[%d] = 0x%02x \n", i, buffStuff[i]);
	}
	*/



    //llopen(fd, mode);
	/*
    switch(mode){
        case TRANSMITTER: llwrite(fd, file_name,strlen(file_name));
                          break;
        case RECEIVER:    llread(fd,read_str);
                          printf("STR: %s \n",read_str);
                          break;
      }
	*/    

    sleep(1);
   
    return 0;
}
