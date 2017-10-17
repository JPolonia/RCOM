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
const int C_REJ_0 = 0x05;
const int C_REJ_1 = 0x25;
const int C_RR_0 = 0x01;
const int C_RR_1 = 0x21;
const int C_DISC = 0x0b;

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
            msg[2] = C_SET;
            msg[3] = A^C_SET;
            alarmCounter =    1;
            error = 1;
            while(alarmCounter <= ll->numTransmissions && error){
                
                alarm(0);
                alarm(ll->timeout); //activa alarme de 3s
                alarmFlag=0;

                /*Envia trama SET*/
                res = write(fd,msg,5);
                printf("%d bytes sent\n",res);
                
                readpacket(fd,buff,TRANSMITTER); //Espera pela resposta UA
                error = ((buff[3]!=(buff[1]^buff[2]))|| buff[2]!=C_UA) ? 1 : 0;
                
            }
            break;

		case RECEIVER: //OK
            while(1){ //Espera pela trama SET
                readpacket(fd,buff,RECEIVER);
                error = ((buff[3]!=(buff[1]^buff[2]))|| buff[2]!=C_SET) ? 1 : 0;
                if (error) {
                    printf("Received an invalid frame\n");
                    continue;
                }
                else {//Envia resposta UA
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
    printf("*** Successfully established a connection. ***\n");
    alarm(0); //cancela alarme anterior
	return 1; 
	
}

int destuffing( unsigned char *buff, unsigned char *buffDestuff){ //Funciona
	int i=4, j=0;

	while(buff[i]!=FLAG_RCV){ //OK
		if(buff[i]==ESCAPE && buff[i+1] == ESCAPE1){ //OK
			buffDestuff[j]=FLAG_RCV;
			i=i+2;
			j++;		
		}
		else if(buff[i]==ESCAPE && buff[i+1] == ESCAPE2){ //OK
			buffDestuff[j]=ESCAPE;
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
	unsigned char RR[5] = {FLAG_RCV, A, C_RR_0, A^C_RR_0, FLAG_RCV};
    unsigned char REJ[5] = {FLAG_RCV, A, C_REJ_0, A^C_REJ_0, FLAG_RCV};
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
                if( buff[2]==C_I_0 && ll->sequenceNumber==1 ){ //recebemos 0 e queriamos 1
                    RR[2] = C_RR_1; //queremos seq 1
                    RR[3] = RR[1]^RR[2];
                    if(write(fd, RR, 5) != 5){
                        printf("Falha no envio de RR\n");
                    }
                    printf("Trama repetida, RR envidado\n");
                    state=1;
                }
                else if(buff[2]==C_I_1 && ll->sequenceNumber==0){ //recebemos os 1 e queriamos 0
                    RR[2] = C_RR_0; //queremos seq 0
                    RR[3] = RR[1]^RR[2];
                    if(write(fd, RR, 5) != 5){
                        printf("Falha no envio de RR\n");
                    }
                    printf("Trama repetida, RR envidado\n");
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
                        RR[2] = C_RR_1;
                        RR[3] = RR[1]^RR[2];
                        if(write(fd, RR, 5) != 5){
                            printf("Falha ao enviar RR\n");
                        }
                        printf("RR1 enviado\n");
                    }
                    else if(ll->sequenceNumber  == 1){ //envia RR0
                        ll->sequenceNumber = 0;
                        RR[2] = C_RR_0;
                        RR[3] = RR[1]^RR[2];
                        if(write(fd, RR, 5) != 5){
                            printf("Falha ao enviar RR\n");
                        }
                        printf("RR0 enviado\n");
                    }
                    printf("*** Received valid frame ***\n");
                    state = 5;
                }
                else { //dados invalidos, enviar REJ
                    if(ll->sequenceNumber == 0){
                        REJ[2] = C_REJ_0;
                        REJ[3] = (REJ[1]^REJ[2]);
                        if(write(fd, REJ, 5) != 5){
                            printf("Falha ao enviar REJ\n");
                        }
                        printf("REJ0 enviado\n");
                        /*for(i = 0;i < 5;i++){
                            printf("REJ[%d] = 0x%02x\n", i, REJ[i]);
                        }*/
                    }
                    else if(ll->sequenceNumber == 1){
                        REJ[2] = C_REJ_1;
                        REJ[3] = (REJ[1]^REJ[2]);
                        if(write(fd, REJ, 5) != 5){
                            printf("Falha ao enviar REJ\n");
                        }
                        printf("REJ1 enviado\n");
                        /*for(i = 0;i < 5;i++){
                            printf("REJ[%d] = 0x%02x\n", i, REJ[i]);
                        }*/
                    }
                    state = 1;
                }
                
            default:
                break;
			}
		}    
	return tam-1;
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
    int i = 0;
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
    
       
        
        bytesWritten = write(fd ,trama ,frameSize);  //Envia trama I
        printf("%d bytes sent, frame size = %d\n", bytesWritten, frameSize);
        
        for(i = 0; i < 5; i++){
            ack[i] = 0;
        }
        
        
        alarm(0);
        alarm(ll->timeout); //activa alarme de 3s
        alarmFlag=0;
        
        readpacket(fd, ack, TRANSMITTER); //Espera pela resposta RR ou REJ
        
        if(ack[3]!=(ack[1]^ack[2])) {    //ack inválido
            printf("*** ACK é inválido ***\n");
            /*for(i = 0;i < 5;i++){
                printf("ACK[%d] = 0x%02x\n", i, ack[i]);
            }*/
            continue;
        }
        else{    //ack válido
            if((ack[2] == C_RR_0) && (ll->sequenceNumber == 1)){ //Recebemos RR0
                printf("*** RR0 received ***\n");
                ll->sequenceNumber = 0;
                error = 0;
                alarm(0); //cancela alarme anterior
                break;
            }
            else if((ack[2] == C_RR_1) && (ll->sequenceNumber == 0)){ //Recebemos RR1
                printf("*** RR1 received ***\n");
                ll->sequenceNumber = 1;
                error = 0;
                alarm(0); //cancela alarme anterior
                break;
            }
            else if((ack[2] == C_REJ_0) || (ack[2] == C_REJ_1)){  //Recebemos REJ
                printf("*** REJ received ***\n");
                alarmCounter = 1; //começamos transmissão de novo?
            }
            else{ //ack inválido
                printf("*** ACK é inválido ***\n");
                /*for(i = 0;i < 5;i++){
                    printf("ACK[%d] = 0x%02x\n", i, ack[i]);
                }*/
                error = 1;
            }
        }
    }
    
    if (error == 1){
        return -1;
    }
    
	return frameSize;
}

int llclose(int fd, unsigned char mode){
    unsigned char DISC[] = {FLAG_RCV, A, C_DISC, A^C_DISC,FLAG_RCV};
    unsigned char UA[] = {FLAG_RCV, 0x01, C_UA, 0x01^C_DISC,FLAG_RCV}; //A = 0x01
    unsigned char buff[5];
    int error,i;
    int res;
    
    printf("*** Trying to close the connection. ***\n");
    switch(mode){
        case TRANSMITTER:
            alarmCounter = 1;
            error = 1;
            while(alarmCounter <= ll->numTransmissions && error){
                
                alarm(0);
                alarm(ll->timeout); //activa alarme de 3s
                alarmFlag=0;
                
                //Envia trama DISC
                res = write(fd,DISC,5);
                printf("%d bytes sent\n",res);
                printf("DISC sent\n");
                
                readpacket(fd,buff,TRANSMITTER); //Espera pela resposta DISC
                error = ((buff[3]!=(buff[1]^buff[2]))|| buff[2]!=C_DISC) ? 1 : 0;
            }
            alarm(0);
            res = write(fd, UA, 5);
            printf("%d bytes sent\n",res);
            printf("UA sent\n");
            break;
            
        case RECEIVER:
            while(1){ //Espera pela trama DISC
                DISC[1] = 0x01; //comando enviado pelo recetor
                DISC[3] = DISC[1]^DISC[2];
                alarm(0);
                alarmFlag=0;
                readpacket(fd,buff,RECEIVER); //esperamos por DISC
                if ((buff[3]!=(buff[1]^buff[2])) || (buff[2]!=C_DISC)) {
                    printf("Received a frame but it isn't DISC, still waiting for DISC\n");
                    continue;
                }
                printf("DISC received\n");
                
                error = 1;
                alarmCounter = 1;
                while(alarmCounter <= ll->numTransmissions){ //transmissão de DISC
                    
                    alarm(0);
                    alarm(ll->timeout); //activa alarme de 3s
                    alarmFlag=0;
                    
                    //Envia trama DISC
                    res = write(fd, DISC, 5);
                    printf("%d bytes sent\n",res);
					printf("DISC sent\n");
					
					for(i=0;i<5;i++){
						buff[i] = 0;
					}
                    
                    readpacket(fd,buff,TRANSMITTER); //Espera pela resposta UA
					error = ((buff[3]!=(buff[1]^buff[2])) || buff[2]!=C_UA) ? 1 : 0;
					if(error)printf("PAROU!! buff[0]=%d  buff[1]=%d  buff[2]=%d  buff[3]=%d  buff[4]=%d\n",buff[0],buff[1],buff[2],buff[3],buff[4]);
                    if(!error) break;
                }
                if(!error){
                    printf("Received UA\n");
                    break;
                }
                else{
					printf("Couldn't receive UA\n");
					
                    return -1;
                }
    
                
            }
            break;
        default:
            return -1;
    }
	sleep(1);
    closeSerialPort(fd); //fazemos isto aqui?
	sleep(1);
    printf("*** Successfully closed the connection. ***\n");
    alarm(0); //cancela alarme anterior
    return 1;
}
