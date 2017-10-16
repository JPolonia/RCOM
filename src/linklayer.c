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
				printf("Frame received!\n");
				i=0; 			
				state = 5;
				break;
		}
		
		//printf("STATE %d   - buffer[%d] = 0x%02x - res = %d\n", state, i, buffer[i],res);


	}

	//readpacket(fd, buffer, state, mode, x);
}

int llopen(int fd, unsigned char mode){ //funciona
	
	unsigned char msg[5];
	unsigned char buff[5];
	int error;
	int res;

	msg[0] = FLAG_RCV;
	msg[1] = A;
	msg[4] = FLAG_RCV;
	
	printf("*** Trying to establish a connection. ***\n");
	switch(mode){
		case TRANSMITTER: //OK
            printf("TRANSMISTTER\n");
            msg[2] = C_SET;
            msg[3] = A^C_SET;
            alarmCounter =    1;
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

		case RECEIVER: //OK
            printf("RECEIVER\n");
            while(1){ //Espera pela trama SET
                readpacket(fd,buff,RECEIVER);
                error = ((buff[3]!=(buff[1]^buff[2]))|| buff[2]!=C_SET) ? 1 : 0;
                if (error) {
                    printf("Received an invalid frame\n");
                    continue;
                }
                else {
                    /*Envia resposta UA*/
                    msg[2] = C_UA;
                    msg[3] = A^C_UA;
                    res = write(fd,msg,5);
                    printf("%d bytes sent\n",res);
                    break;
                }
            }
            break;
            
		default:
            return -1;
	}

	if(error){
		printf("*** Error establishing a connection: ***\n");
		return -1;
        
    }
    else{
        printf("*** Successfully established a connection. ***\n");
    }
	return 1; 
	
}

int destuffing( unsigned char *buff, unsigned char *buffDestuff){ //Funciona
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

unsigned char xor_result(unsigned char *array, int tam){ //funciona!!
	unsigned char xor;
	int i=2;

	xor = array[0] ^ array[1];

	for(i=2; i<tam; i++){
		xor = xor ^ array[i];
	}
	return xor;
}


int llread(int fd,unsigned char *buffer){

	unsigned char buff[MAX_SIZE]; //para receber trama inteira
	unsigned char buff_destuff[MAX_SIZE-5];  //para receber campo de dados + BCC
	unsigned char RR[5] = {0x7e, 0x03, 0x01, 0x03^0x01, 0x7e};
    unsigned char REJ[5] = {0x7e, 0x03, 0x05, 0x03^0x05, 0x7e};
	int tam = 1, state=1, i = 0;

	while(state!=5){
		//printf("STATE %d - llread\n",state);
		switch(state){
			case 1:  //recebe trama
                readpacket(fd, buff, RECEIVER);
                state=2;
                /*for(i=0;i< + 6;i++){
                    printf("buff[%d] = 0x%02x %c \n",i,buff[i],buff[i]);
                }*/
                break;
		
			case 2:  //verifica se tem erro no cabeçalho
                if(buff[3]==(buff[1]^buff[2]))
                    state=3;
                else
                    state=1;
                break;

			case 3:  //verifica se trama é repetida
                if( buff[2]==0x00 && ll->sequenceNumber==1 ){ //recebemos 0 e queriamos 1
                    RR[2] = 0x21; //queremos seq 1
                    RR[3] = RR[1]^RR[2];
                    if(write(fd, RR, 5) != 5){
                        printf("Falha no envio de RR\n");
                    }
                    state=1;
                }
                else if(buff[2]==0x40 && ll->sequenceNumber==0){ //recebemos os 1 e queriamos 0
                    RR[2] = 0x01; //queremos seq 0
                    RR[3] = RR[1]^RR[2];
                    if(write(fd, RR, 5) != 5){
                        printf("Falha no envio de RR\n");
                    }
                    state=1;
                }
                else{ //recebemos o que queriamos
                    state=4;
                }
                break;
								
			case 4: //faz destuffing e verifica BCC2
                tam = destuffing(buff, buff_destuff); //tam = dados+BCC2
                
                //printf("buff_destuff[tam-1] = BCC2 = 0x%02x\nxor_result = 0x%02x\n", buff_destuff[tam-1], xor_result(buff_destuff, tam-1));
                
                if( buff_destuff[tam-1] == xor_result(buff_destuff, tam-1) ) { //dados validos
                    for( i = 0; i< tam-1; i++){ //preenche buffer de retorno
                        buffer[i] = buff_destuff[i];
                    }
                    if(ll->sequenceNumber  == 0){ //envia RR1
                        ll->sequenceNumber = 1;
                        RR[2] = 0x21;
                        RR[3] = RR[1]^RR[2];
                        if(write(fd, RR, 5) != 5){
                            printf("Falha ao enviar RR\n");
                        }
                    }
                    else if(ll->sequenceNumber  == 1){ //envia RR0
                        ll->sequenceNumber = 0;
                        RR[2] = 0x01;
                        RR[3] = RR[1]^RR[2];
                        if(write(fd, RR, 5) != 5){
                            printf("Falha ao enviar RR\n");
                        }
                    }
                    printf("*** Received valid frame ***\n");
                    state = 5;
                }
                else { //dados invalidos, enviar REJ
                    if(ll->sequenceNumber == 0){
                        REJ[2] = 0x05;
                        REJ[3] = REJ[1]^REJ[2];
                        if(write(fd, REJ, 5) != 5){
                            printf("Falha ao enviar REJ\n");
                        }
                    }
                    else if(ll->sequenceNumber == 1){
                        REJ[2] = 0x25;
                        REJ[3] = REJ[1]^REJ[2];
                        if(write(fd, REJ, 5) != 5){
                            printf("Falha ao enviar REJ\n");
                        }
                    }
                    state = 1;
                }
                
            default:
                break;
			}
		}    
	return tam-1;
}

int size_stuffing(unsigned char *buff, int length){
	int i,new_size=0;
	for(i=1;i<length-1;i++){
		new_size += (buff[i] == FLAG_RCV || buff[i] == ESCAPE)  ? 1 : 0;
	}
	return length + new_size;
}

int stuffing(unsigned char *buff, unsigned char BCC2, unsigned char *stuffedBuffer, int length){ //falta verificar com BCC2
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
    if(BCC2 == FLAG_RCV){
        stuffedBuffer[j] = ESCAPE;
        stuffedBuffer[j+1]    = ESCAPE1;
        j = j + 2;
    }
    else if(BCC2 == ESCAPE){
        stuffedBuffer[j] = ESCAPE;
        stuffedBuffer[j+1]    = ESCAPE2;
        j = j + 2;
    }
    else{
        stuffedBuffer[j] = BCC2;
        j++;
    }
	return j;	 
}

int llwrite(int fd, unsigned char *buffer , int length){

    unsigned char BCC2;
    unsigned char trama[MAX_SIZE];
    int dataAndBCC2Length = 0;
    int frameSize = 0;
    int error = 1;
    int bytesWritten = 0;
    unsigned char ack[MAX_SIZE];
    
    if( ((2*(length+1)) + 5) >  MAX_SIZE){ //verifica que buffer cabe na trama
        printf("Este pacote não cabe na trama.\n");
        return -1;
    }
    
    BCC2 = xor_result(buffer, length); //calcula BCC2
    
    trama[0] = FLAG_RCV; //preenche trama
    trama[1] = A;
    trama[2] = (ll->sequenceNumber) ? C_I_1 : C_I_0;
    trama[3] = trama[1]^trama[2];
    
    dataAndBCC2Length = stuffing(buffer, BCC2, &trama[4], length);
    
    trama[4 + dataAndBCC2Length] = FLAG_RCV; //termina de preencher trama
    
    frameSize = dataAndBCC2Length + 5;
    
    alarmCounter = 1; //começa transmissão da trama_______________________
    error = 1;
    while(alarmCounter <= ll->numTransmissions && error){
        if(alarmFlag){
            alarm(ll->timeout); //activa alarme de 3s
            alarmFlag=0;
        }
        
        bytesWritten = write(fd ,trama ,frameSize);  //Envia trama I
        printf("%d bytes sent, frame size = %d\n", bytesWritten, frameSize);
        
        readpacket(fd, ack, TRANSMITTER); //Espera pela resposta RR ou REJ
        
        if(ack[3]!=(ack[1]^ack[2])) {    //ack inválido
            printf("ACK é inválido\n");
            continue;
        }
        else{    //ack válido
            if(ack[2] == 0x21 || ack[2] == 0x01){ //Recebemos RR
                ll->sequenceNumber = (ll->sequenceNumber)? 0 : 1;
                error = 0;
            }
            else if(ack[2] == 0x05 || ack[2] == 0x25){  //Recebemos REJ
                alarmCounter = 1; //começamos transmissão de novo?
            }
            else{ //ack inválido
                printf("ACK é inválido\n");
                error = 1;
            }
        }
    }
    
    if (error == 1){
        return -1;
    }
    
	return frameSize;
}
int llclose(int fd){
	return 0;
}
