#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include <assert.h>



#include "linklayer.h"
#include "alarm.h"

const int FLAG_RCV = 0x7E;
const int ESCAPE = 0x7D;
const int ESCAPE1 = 0x5E;
const int ESCAPE2 = 0x5D;

const int A = 0x03;

const int C_SET = 0x03;
const int C_UA = 0x07;
const int C_I_0 = 0x00;
const int C_I_1 = 0x40;

volatile int STOP=FALSE;
volatile int SNQNUM = 0;

linkLayer* ll;

int initLinkLayer(char* port,int baudRate,unsigned int sequenceNumber,
				  unsigned int timeout,unsigned int numTransmissions){

	ll = (linkLayer*) malloc(sizeof(linkLayer));
	strcpy(ll->port, port);
	ll->baudRate = baudRate;
	ll->sequenceNumber = sequenceNumber;
	ll->timeout = timeout;
	ll->numTransmissions = numTransmissions;

	return 1;
}


int openSerialPort(char* port){
	/*Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.*/    
	return open(port, O_RDWR | O_NOCTTY );
}

int closeSerialPort( int fd){
	tcsetattr(fd,TCSANOW,&ll->oldtio);
	close(fd);
	return 1;
}

int initTermios(int fd){
	// save current port setting
	if ( tcgetattr(fd,&ll->oldtio) == -1) { 
		perror("tcgetattr");
		exit(-1);
	}
	
	bzero(&ll->newtio, sizeof(ll->newtio));
	ll->newtio.c_cflag = ll->baudRate | CS8 | CLOCAL | CREAD;
	ll->newtio.c_iflag = IGNPAR;
	ll->newtio.c_oflag = 0;

	/* set input mode (non-canonical, no echo,...) */
	ll->newtio.c_lflag = 0;
	ll->newtio.c_cc[VTIME]    = 1;   /* inter-character timer unused */
	ll->newtio.c_cc[VMIN]     = 0;   /* blocking read until 5 chars received */
/* VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a 
	leitura do(s) próximo(s) caracter(es)*/
	tcflush(fd, TCIOFLUSH);

	if ( tcsetattr(fd,TCSANOW,&ll->newtio) == -1) {
	perror("tcsetattr");
	exit(-1);
	}

	return 1;
}



void readpacket(int fd, unsigned char *buffer, unsigned char mode){ //Funciona

	int res = 0;
	int state = 1;
	int i = 0;
	while(state != 5){

		if(alarmFlag && (mode==TRANSMITTER) ){ 		
			break;
		}

		switch(state){
			case 1: res = read(fd,buffer+i,1);
				if((buffer[i] == FLAG_RCV) && res){
					i++;
					state = 2;
				}
				break;
			case 2: res = read(fd,buffer+i,1);

				if (((buffer[i])!= FLAG_RCV) && res){
					i++;
					state = 3;
				}			
				break;
			case 3: res = read(fd,buffer+i,1);
				if((buffer[i]) == FLAG_RCV){
					state = 4;
				} else i++;
				break;
			case 4: 
				printf("OK!\n");
				i=0; 			
				state = 5;
				break;
		}
		
		//printf("STATE %d   - buffer[%d] = 0x%02x - res = %d\n", state, i, buffer[i],res);


	}

	//readpacket(fd, buffer, state, mode, x);
}

int llopen(int fd, unsigned char flag){
	
	unsigned char msg[5];
	unsigned char buff[5];
	int error;
	int res;

	msg[0] = FLAG_RCV;
	msg[1] = A;
	msg[4] = FLAG_RCV;
	
	printf("*** Trying to establish a connection. ***\n");
	switch(flag){
		case TRANSMITTER:   printf("TRANSMISTTER\n");
							msg[2] = C_SET;
							msg[3] = A^C_SET;
							alarmCounter =	0;
							error = 1;
							while(alarmCounter <= ll->numTransmissions && buff[4] != FLAG_RCV && error){
								if(alarmFlag){
							  		alarm(ll->timeout); //activa alarme de 3s
							 		alarmFlag=0;
								}
								/*Envia trama SET*/							
								res = write(fd,msg,5);
								printf("%d bytes sent\n",res);
								/*Espera pela resposta UA*/				
								readpacket(fd,buff,TRANSMITTER);
								error = ((buff[3]!=(buff[1]^buff[2]))|| buff[2]!=C_UA) ? 1 : 0;
								
							}							
							break;

		case RECEIVER: 		printf("RECEIVER\n");
							/*Espera pela trama SET*/
							readpacket(fd,buff,RECEIVER);
							error = ((buff[3]!=(buff[1]^buff[2]))|| buff[2]!=C_SET) ? 1 : 0;
							if (error) printf("PAROU!! buff[0]=%d  buff[1]=%d  buff[2]=%d  buff[3]=%d  buff[4]=%d\n",buff[0],buff[1],buff[2],buff[3],buff[4]);
							else {
								/*Envia resposta UA*/
								msg[2] = C_UA;
								msg[3] = A^C_UA;
								res = write(fd,msg,5);
								printf("%d bytes sent\n",res);}
							break;

		default: 		
							return -1;
	}

	if(error){
		printf("*** Error establishing a connection: ***\n");
		printf("Espected: %d (C_SET)  Received: %d\n",C_SET,buff[2]);
		printf("Espected: %d (BCC)  Received: %d\n",buff[1]^buff[2],buff[3]);
		return -1;}		
	
	printf("*** Successfully established a connection. ***\n");
	//printf("PAROU!! buff[0]=%d  buff[1]=%d  buff[2]=%d  buff[3]=%d  buff[4]=%d\n",buff[0],buff[1],buff[2],buff[3],buff[4]);
	return 1; 
	
}

int destuffing( unsigned char *buff, char *buffDestuff){ //Funciona
	int i=4, j=0;

	while(buff[i]!=0x7e){ //OK
		if(buff[i]==0x7d && buff[i+1]==0x5e){ //OK
			buffDestuff[j]=0x7e;
			i=i+2;
			j++;		
		}
		else if(buff[i]==0x7d && buff[i+1]==0x5d){ //OK
			buffDestuff[j]=0x7d;
			i=i+2;
			j++;		
		}
		else{ //OK
			buffDestuff[j]=buff[i];
			i++;
			j++;
		}
	}
	return j; 
}

unsigned char xor_result(char *array, int tam){ //funciona!!
	unsigned char xor;
	int i=2;

	xor = array[0] ^ array[1];

	for(i=2; i<tam; i++){
		xor = xor ^ array[i];
	}
	return xor;
}


int llread(int fd, char *buffer){

	unsigned char buff[MAX_SIZE];
	char buff_destuff[MAX_SIZE];
	unsigned char RR[5] = {0x7e, 0x03, 0x01, 0x03^0x01, 0x7e};
	int tam, state=1, i;

	while(state!=7){
		printf("STATE %d - llread\n",state);
		switch(state){
			case 1:	readpacket(fd, buff, RECEIVER);
					state=2;

					for(i=0;i< + 6;i++){
					printf("buff[%d] = 0x%02x %c \n",i,buff[i],buff[i]);
					}

					break;
		
			case 2:	tam = destuffing(buff, buff_destuff);
						//if(buff_destuff[tam-1]!=xor_result(buff_destuff, tam)) {
							//REG
							//state=1;  // verificar!!!
							//}
						//else 

					for(i=0;i<tam + 6;i++){
					printf("buff_destuff[%d] = 0x%02x %c \n",i,buff_destuff[i],buff_destuff[i]);
					}
					return 1;
					state=3;
						break;

			case 3:	if(buff[3]!=(buff[1]^buff[2]))
							state=1;
						else 
							state=4;
						break;
								
			case 4:	if( buff[2]==0x00 && ll->sequenceNumber==1 ){
							assert(write(fd, RR, 5) == 5);
							state=1;
							}
						else 
							state=5;
						break;
						
			case 5:	if(buff[2]==0x40 && ll->sequenceNumber==0){
							//RR = {0x7E, 0x03, 0x21, , 0x7E};
							RR[2]=0x21;
			    			RR[3]=0x03^0x21;
							assert(write(fd, RR, 5) == 5);
							state=1;
							}
						else 
							state=6;								
						break;
					
			case 6:	buff_destuff[tam-1] = '\0';  //para strcpy funcionar
						strcpy(buffer, buff_destuff+4);
						//RR[0]= enviar rr
						if( buff[2]==0x00){
							ll->sequenceNumber=1;
							assert(write(fd, RR, 5) == 5);
							state=1;
							}
						else if(buff[2]==0x40){
							ll->sequenceNumber=0;
							//RR = {0x7E, 0x03, 0x21, 0x03^0x21, 0x7E};
			    			RR[2]=0x21;
			    			RR[3]=0x03^0x21;
							assert(write(fd, RR, 5) == 5);
							state=1;
						}							
						else state=7;
						break;

			case 7:	printf("Read OK!");
				
			}
		}
	return strlen(buffer);
}

int size_stuffing(char *buff, int length){
	int i,new_size=0;
	for(i=1;i<length-1;i++){
		new_size += (buff[i] == FLAG_RCV || buff[i] == ESCAPE)  ? 1 : 0;
	}
	return length + new_size;
}

int stuffing(char *buff, char *stuffedBuffer, int length){ //funciona
	int i = 0, j = 0;

	for(i=0;i<length;i++){ //OK
		if(buff[i] == FLAG_RCV){ //OK
			stuffedBuffer[j] = ESCAPE;
			stuffedBuffer[j+1]	= ESCAPE1;
			j = j + 2;
		}
		else if(buff[i] == ESCAPE){ //OK
			stuffedBuffer[j] = ESCAPE;
			stuffedBuffer[j+1]	= ESCAPE2;
			j = j + 2;
		}
		else{ //OK
			stuffedBuffer[j] = buff[i]; 
			j++;
		}		
	}
	return j;	 
}

int llwrite(int fd,char *buffer , int length){
	int i,res,new_size;
	//unsigned char RR[255];
	unsigned char BCC2;
	char *stuffed_buffer;
	char *trama;

	// Calc BCC2
	/*for(i=1;i<length;i++){
		BCC2 ^= buffer[i];
	}*/

	BCC2 = xor_result(buffer, length);

	// Encapsulamento da trama: F + A + C + BCC + buffer + BCC2 + F
	trama = (char *) malloc(sizeof(char) * length + 6);
	trama[0] = FLAG_RCV;
	trama[1] = A;
	trama[2] = (SNQNUM) ? C_I_1 : C_I_0;
	trama[3] = trama[1]^trama[2];
	memcpy(trama + 4,buffer,length); //strncpy(dest, src + beginIndex, endIndex - beginIndex);
	trama[length + 4] = BCC2;
	trama[length + 5] = FLAG_RCV;
	
	// Stuffing da trama
	new_size = size_stuffing(trama,length + 6);
	stuffed_buffer = (char *) malloc(sizeof(char) * (new_size+1));
	stuffing(trama,stuffed_buffer,length + 6);

	for(i=0;i<length + 6;i++){
		printf("stuffed_buffer[%d] = 0x%02x %c \n",i,stuffed_buffer[i],stuffed_buffer[i]);
	}

	/* Envia trama TRAMA I*/							
	res = write(fd,stuffed_buffer,length + 6);
	printf("%d bytes sent\n",res);

	/* Free Memmory*/
	//free(trama);
	//free(stuffed_buffer);
	
	/* Espera pela resposta RR*/				
	//readpacket(fd,RR,255,TRANSMITTER);
	
	// Verificar RR


	SNQNUM = (SNQNUM) ? 0 : 1;		

	return 0;
}
int llclose(int fd){
	return 0;
}
