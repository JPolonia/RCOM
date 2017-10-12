#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

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

/*int initLinkLayer(){
	
}*/


void readpacket(int fd,unsigned char *buffer,int state, char mode){
	//int c=100;
	int res;
	//while(state!=4){
		switch(state){
			case 1: res = read(fd,buffer,1);
				if(*buffer == FLAG_RCV){
					buffer++;
					state = 2;
				}
				break;
			case 2: res = read(fd,buffer,1);
				if (((*buffer)!=FLAG_RCV) && res){
					buffer++;
					state = 3;
				}			
				break;
			case 3: res = read(fd,buffer,1);
				if((*buffer) == FLAG_RCV){
					state = 4;
				} else buffer++;
				break;
			case 4: 
				printf("OK!\n"); 			
				return;
		}
		printf("STATE %d   -  %d Expected: %d  res: %d\n",state,buffer[0],FLAG_RCV,res);
		
	if(alarmFlag && (mode==TRANSMITTER) ){ 		
		return;
	}
	//}
	readpacket(fd,buffer,state,mode);
}

int llopen(int fd, char flag){
	
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
								readpacket(fd,buff,1,TRANSMITTER);
								error = ((buff[3]!=(buff[1]^buff[2]))|| buff[2]!=C_UA) ? 1 : 0;
								
							}							
							break;

		case RECEIVER: 		printf("RECEIVER\n");
							/*Espera pela trama SET*/
							readpacket(fd,buff,1,RECEIVER);
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

int llread(int fd, char *buffer){

}

int size_stuffing(char *buff){
	int i,new_size=0;
	for(i=1;i<sizeof(buff)-1;i++){
		new_size += (buff[i] == FLAG_RCV || buff[i] == ESCAPE)  ? 1 : 0;
	}
	return sizeof(buff) + new_size;
}

void stuffing(char *buff,char *stuffed_buffer){
	int i,index;

	index = 1;

	for(i=0;i<sizeof(buff);i++){
		stuffed_buffer[i] = buff[i];
		if(buff[i] == FLAG_RCV){
			stuffed_buffer[index] = ESCAPE;
			stuffed_buffer[++index]	= ESCAPE1;
		}
		if(buff[i] == ESCAPE){
			stuffed_buffer[index] = ESCAPE;
			stuffed_buffer[++index]	= ESCAPE2;
		}
	}	 
}

int llwrite(int fd, char *buffer, int length){
	int i,res,new_size;
	char RR[255];
	char BCC2 = buffer[0];
	char *stuffed_buffer;
	char *trama;

	// Calc BCC2
	for(i=1;i<length;i++){
		BCC2 ^= buffer[i];
	}

	// Encapsulamento da trama: F + A + C + BCC + buffer + BCC2 + F
	trama = (char *) malloc(sizeof(char *) * length + 6);
	trama[0] = FLAG_RCV;
	trama[1] = A;
	trama[2] = (SNQNUM) ? C_I_1 : C_I_0;
	trama[3] = trama[1]^trama[2];
	memcpy(trama + 4,buffer,length); //strncpy(dest, src + beginIndex, endIndex - beginIndex);
	trama[length + 4] = BCC2;
	trama[length + 5] = FLAG_RCV;
	
	// Stuffing da trama
	new_size = size_stuffing(buffer);
	stuffed_buffer = (char *) malloc(sizeof(char *) * (new_size+1));
	stuffing(buffer,stuffed_buffer);

	/* Envia trama TRAMA I*/							
	res = write(fd,stuffed_buffer,sizeof(stuffed_buffer));
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