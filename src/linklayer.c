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
#include "testes.h"


#define ERROR_ACTIVE 0
#define HEADER_ERROR_RATE 0.1
#define PACKET_ERROR_RATE 0.3

#define CMD_SIZE 5

const int T_PROP = 500; //em ms

const int FLAG_RCV = 0x7E;
const int ESCAPE = 0x7D;
const int ESCAPE1 = 0x5E;
const int ESCAPE2 = 0x5D;

const int A = 0x03;
const int A_1 = 0x01;

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

int error_UA = 0;

int initLinkLayer(char* port,int baudRate,unsigned int sequenceNumber,unsigned int timeout,unsigned int numTransmissions, int max_size){
	ll = (linkLayer*) malloc(sizeof(linkLayer));
	strcpy(ll->port, port);
	ll->baudRate = baudRate;
	ll->sequenceNumber = sequenceNumber;
	ll->timeout = timeout;
	ll->numTransmissions = numTransmissions;
	ll->max_size = max_size;
	return 1;
}


int openSerialPort(char* port){
	/*Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.*/    
	return open(port, O_RDWR | O_NOCTTY );
}

int closeSerialPort(int fd){
	tcsetattr(fd,TCSANOW,&ll->oldtio);
	close(fd);
	return 1;
}

int initTermios(int fd){
	/*Save old port settings*/
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


/*READS FRAME UNTIL FLAG_RCV AND RETURN SIZE OF BUFFER*/
int readFrame(int fd, unsigned char *buffer, unsigned char mode, int state, int index){ 

    int bytesRead;
    
    if(tt->debug && !index) printf("STATE 1 - WAITING FOR FLAG_RCV\n");

    if(alarmFlag && (mode==TRANSMITTER))  return -1;

    switch(state){
        /*STATE 1: START - READS 1 CHAR UNTIL RECEIVES FLAG_RCV*/
        case 1: bytesRead  = read(fd,buffer+index,1);
                if((buffer[index] == FLAG_RCV) && bytesRead){
                    index++;
                    state = 2;
                }
                break;
        
        /*STATE 2: FLAG RCV - READS 1 CHAR UNTIL RECEIVES DIFFERENT THAN FLAG_RCV*/
        case 2: bytesRead  = read(fd,buffer+index,1);
                if (((buffer[index])!= FLAG_RCV) && bytesRead){
                    index++;
                    state = 3;
                }			
                break;

        /*STATE 3 - READS 1 CHAR OF DATA UNTIL RECEIVES A FLAG_RCV*/
        case 3: bytesRead  = read(fd,buffer+index,1);
                if((buffer[index]) == FLAG_RCV){
                    index++;
                    state = 4;
                } else index++;
                break;

        /*STATE 4 - SUCESSFULLY READ BUFFER! RETURN LENGTH OF BUFFER*/
        case 4: 
                if(tt->debug) printf("Frame received!\n");
                return index;
    }

    if(tt->debug && index)printf("STATE %d   - buffer[%d] - 0x%02x ASCII: %c   bytesRead: %d\n",state,index,buffer[index-1],buffer[index-1],bytesRead);
    
    return readFrame(fd, buffer, mode,state, index);
}

int llopen(int fd, unsigned char mode, int state){ 
	
	unsigned char *msg;
    unsigned char *buff;
	int error;
    int bytesWritten;
    int bytesRead;

    msg = (unsigned char *) malloc(sizeof(unsigned char) * CMD_SIZE);
    buff = (unsigned char *) malloc(sizeof(unsigned char) * CMD_SIZE);

    bytesWritten = 0;
    bytesRead = 0;

	msg[0] = FLAG_RCV;
	msg[1] = A;
	msg[4] = FLAG_RCV;
	
	printf("*** Trying to establish a connection. ***\n");
	switch(mode){
		case TRANSMITTER:
                            msg[2] = C_SET;
                            msg[3] = A^C_SET;
                            alarmCounter =    1;
                            error = 1;
                            while(alarmCounter <= ll->numTransmissions && error){
                                
                                alarm(0);
                                alarm(ll->timeout); //activa alarme de 3s
                                alarmFlag=0;

                                /*Envia trama SET*/
                                bytesWritten = write(fd,msg,CMD_SIZE);
                                printf("%d bytes sent\n",bytesWritten);
                                if(bytesWritten != CMD_SIZE){
                                    printf("Error Sending SET mensage... bytesWritten:%d Expected:%d \n",bytesWritten,CMD_SIZE);

                                }
                                
                                bytesRead = readFrame(fd,buff,TRANSMITTER,1,0); //Espera pela resposta UA
                                if(bytesRead != CMD_SIZE){
                                    printf("Error Receiving UA mensage... bytesRead:%d Expected:%d \n",bytesRead,CMD_SIZE);
                                }
                                error = ((buff[3]!=(buff[1]^buff[2]))|| buff[2]!=C_UA) ? 1 : 0;
                                
                            }
                            break;

		case RECEIVER:
                            while(1){ //Espera pela trama SET
                                if(state==1){
                                    bytesRead = readFrame(fd,buff,RECEIVER,1,0);
									error = ((buff[3]!=(buff[1]^buff[2]))|| buff[2]!=C_SET) ? 1 : 0;
                                }else error = 0;
                                
                                if (error) {
                                    printf("Received an invalid frame\n");
                                    continue;
                                }else {//Envia resposta UA
                                    msg[2] = C_UA;
                                    msg[3] = A^C_UA;

									//Test UA ERROR
                                    if(error_UA) error_UA = 0;
									else bytesWritten = write(fd,msg,5); 
										
                                    if(tt->debug) printf("Receiver UA: %d bytes sent\n",bytesWritten);
                                    break;
                                }
                                state = 1;
                                
                            }
                            break;
            
		default:
                            return -1;
    }
    
    free(msg);
    free(buff);

	if(error){
		printf("*** Error establishing a connection: ***\n");
		return -1;
    }
    printf("*** Successfully established a connection. ***\n");
    alarm(0); //cancela alarme anterior
	return 1;	
}

int llclose(int fd, unsigned char mode){
    unsigned char DISC[] = {FLAG_RCV, A, C_DISC, A^C_DISC,FLAG_RCV};
    unsigned char UA[] = {FLAG_RCV, A_1, C_UA, A_1^C_UA,FLAG_RCV}; //A = 0x01
    unsigned char *buff;
    int error,i;
    int res;
    int bytesRead;

    buff = (unsigned char *) malloc(sizeof(unsigned char) * CMD_SIZE);
    
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
                if(tt->debug) printf("%d bytes sent\n",res);
                if(tt->debug) printf("DISC sent\n");
                
                bytesRead = readFrame(fd,buff,TRANSMITTER,1,0); //Espera pela resposta DISC
                error = ((buff[3]!=(buff[1]^buff[2]))|| buff[2]!=C_DISC) ? 1 : 0;
            }
            alarm(0);
            res = write(fd, UA, 5);
            if(tt->debug) printf("%d bytes sent\n",res);
            if(tt->debug) printf("UA sent\n");
            break;
            
        case RECEIVER:
            while(1){ //Espera pela trama DISC
                DISC[1] = 0x01; //comando enviado pelo recetor
                DISC[3] = DISC[1]^DISC[2];
                alarm(0);
                alarmFlag=0;
                bytesRead = readFrame(fd,buff,RECEIVER,1,0); //esperamos por DISC
                if ((buff[3]!=(buff[1]^buff[2])) || (buff[2]!=C_DISC)) {
                    printf("Received a frame but it isn't DISC, still waiting for DISC\n");
                    continue;
                }
                if(tt->debug) printf("DISC received\n");
                
                error = 1;
                alarmCounter = 1;
                while(alarmCounter <= ll->numTransmissions){ //transmissão de DISC
                    
                    alarm(0);
                    alarm(ll->timeout); //activa alarme de 3s
                    alarmFlag=0;
                    
                    //Envia trama DISC
                    res = write(fd, DISC, 5);
                    if(tt->debug)  printf("%d bytes sent\n",res);
					if(tt->debug)  printf("DISC sent\n");
					
					for(i=0;i<5;i++){
						buff[i] = 0;
					}
                    
                    bytesRead = readFrame(fd,buff,TRANSMITTER,1,0); //Espera pela resposta UA
					error = ((buff[3]!=(buff[1]^buff[2])) || buff[2]!=C_UA) ? 1 : 0;
					if(error) printf("PAROU!! buff[0]=0x%02x buff[1]=0x%02x  buff[2]=0x%02x  buff[3]=0x%02x  buff[4]=0x%02x\n",buff[0],buff[1],buff[2],buff[3],buff[4]);
                    if(!error) break;
                }
                if(!error){
                    if(tt->debug) printf("Received UA\n");
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
    free(buff);
	sleep(1);
    closeSerialPort(fd); //fazemos isto aqui?
	sleep(1);
    printf("*** Successfully closed the connection. ***\n");
    alarm(0); //cancela alarme anterior
    return 1;
}


unsigned char xor_result(unsigned char *array, int tam){ 
	unsigned char xor=array[0];
	int i;

	for(i=1; i<tam; i++)
        xor = xor ^ array[i];
        
	return xor;
}

/*---------------------------------------------------------------------------------*/
/*---------------------------------RECEIVER----------------------------------------*/
/*---------------------------------------------------------------------------------*/

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

int llread(int fd,unsigned char *buffer){

	unsigned char buff[ll->max_size]; //para receber trama inteira
	unsigned char buff_destuff[ll->max_size-5];  //para receber campo de dados + BCC
	unsigned char RR[5] = {FLAG_RCV, A, C_RR_0, A^C_RR_0, FLAG_RCV};
    unsigned char REJ[5] = {FLAG_RCV, A, C_REJ_0, A^C_REJ_0, FLAG_RCV};
	int tam = 1, state=1, i = 0, length = 0;

	while(state!=5){
		if(tt->debug)  printf("STATE %d - llread\n",state);
		switch(state){
			case 1:  //recebe trama
                    length = readFrame(fd, buff, RECEIVER,1,0);
                    state=2;

                    //Geração de erros_____________________________________________
                    if(tt->active){
                        insertHeaderError(buff, length, tt->headerErrorRate);
                        insertPacketError(buff, length, tt->packetErrorRate);

                        usleep(T_PROP * 1000);
                    }
                    //_____________________________________________________________

                    if(tt->debug) for(i=0;i< + 6;i++) printf("buff[%d] = 0x%02x %c \n",i,buff[i],buff[i]);
                    break;
		
			case 2:  //verifica se tem erro no cabeçalho
                    if(buff[3]==(buff[1]^buff[2]))
                        state=3;
                    else
                        state=1;
                    break;
            case 3: //Verifica se a trama é uma trama SET!!
                    if(buff[2]==C_SET){
                        printf("Erro recebeu um trama SET!\n");
						printf("Voltando para o llopen...\n");
                        llopen(fd,RECEIVER,2);
                        state = 1;
                        break;
                    }
                    //verifica se trama é repetida
                    if( buff[2]==C_I_0 && ll->sequenceNumber==1 ){ //recebemos 0 e queriamos 1
                        RR[2] = C_RR_1; //queremos seq 1
                        RR[3] = RR[1]^RR[2];
                        if(write(fd, RR, 5) != 5){
                            printf("Falha no envio de RR\n");
                        }
                        if(tt->debug) printf("Trama repetida, RR envidado\n");
                        state=1;
                    }
                    else if(buff[2]==C_I_1 && ll->sequenceNumber==0){ //recebemos os 1 e queriamos 0
                        RR[2] = C_RR_0; //queremos seq 0
                        RR[3] = RR[1]^RR[2];
                        if(write(fd, RR, 5) != 5){
                            printf("Falha no envio de RR\n");
                        }
                        if(tt->debug) printf("Trama repetida, RR envidado\n");
                        state=1;
                    }
                    else{ //recebemos o que queriamos
                        state=4;
                    }
                    break;
								
			case 4: //faz destuffing e verifica BCC2
                    tam = destuffing(buff, buff_destuff); //tam = dados+BCC2
                    
                    if(tt->debug) printf("buff_destuff[tam-1] = BCC2 = 0x%02x\nxor_result = 0x%02x\n", buff_destuff[tam-1], xor_result(buff_destuff, tam-1));
                    
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
                            if(tt->debug) printf("RR1 enviado\n");
                        }
                        else if(ll->sequenceNumber  == 1){ //envia RR0
                            ll->sequenceNumber = 0;
                            RR[2] = C_RR_0;
                            RR[3] = RR[1]^RR[2];
                            if(write(fd, RR, 5) != 5){
                                printf("Falha ao enviar RR\n");
                            }
                            if(tt->debug) printf("RR0 enviado\n");
                        }
                        if(tt->debug)  printf("*** Received valid frame ***\n");
                        state = 5;
                    }
                    else { //dados invalidos, enviar REJ
                        if(ll->sequenceNumber == 0){
                            REJ[2] = C_REJ_0;
                            REJ[3] = (REJ[1]^REJ[2]);
                            if(write(fd, REJ, 5) != 5){
                                printf("Falha ao enviar REJ\n");
                            }
                            if(tt->debug) {
                                printf("REJ0 enviado\n");
                                for(i = 0;i < 5;i++) printf("REJ[%d] = 0x%02x\n", i, REJ[i]);
                            }
                        }
                        else if(ll->sequenceNumber == 1){
                            REJ[2] = C_REJ_1;
                            REJ[3] = (REJ[1]^REJ[2]);
                            if(write(fd, REJ, 5) != 5){
                                printf("Falha ao enviar REJ\n");
                            }
                            if(tt->debug){
                                printf("REJ1 enviado\n");
                                for(i = 0;i < 5;i++) printf("REJ[%d] = 0x%02x\n", i, REJ[i]);
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

/*---------------------------------------------------------------------------------*/
/*--------------------------------TRANSMITTER--------------------------------------*/
/*---------------------------------------------------------------------------------*/

int stuffing(unsigned char *data,unsigned char BCC2, unsigned char *stuffedBuffer, int length){ //falta verificar com BCC2
    int i = 0, j = 0;
    /*Stuff Data*/
    for(i=0;i<length;i++){
        switch(data[i]){
            case 0x7E:      stuffedBuffer[j++] = ESCAPE;
                            stuffedBuffer[j++]	= ESCAPE1;
                            break;
            case 0x7D:      stuffedBuffer[j++] = ESCAPE;
                            stuffedBuffer[j++]	= ESCAPE2;
                            break;
            default:        stuffedBuffer[j++] = data[i]; 
                            break;
        }
    }
    /*Stuff BCC2*/
    switch(BCC2){
        case 0x7E:      stuffedBuffer[j++] = ESCAPE;
                        stuffedBuffer[j++]	= ESCAPE1;
                        break;
        case 0x7D:      stuffedBuffer[j++] = ESCAPE;
                        stuffedBuffer[j++]	= ESCAPE2;
                        break;
        default:        stuffedBuffer[j++] = data[i]; 
                        break;
    }
	return j;	 
}

int llwrite(int fd, unsigned char *buffer , int length){

    unsigned char BCC2;
    unsigned char *trama;
    int stuffedLength = 0;
    int frameSize = 0;
    int error = 1;
    int bytesWritten = 0;
    int bytesRead;
    int i = 0;
    unsigned char ack[ll->max_size];
    
    if( ((2*(length+1)) + 5) >  ll->max_size){ //verifica que buffer cabe na trama
        printf("Este pacote não cabe na trama.\n");
        return -1;
    }
   
    trama = (unsigned char *) malloc(sizeof(unsigned char) * ll->max_size);
    
    
    
    trama[0] = FLAG_RCV; 
    trama[1] = A;
    trama[2] = (ll->sequenceNumber) ? C_I_1 : C_I_0;
    trama[3] = trama[1]^trama[2];
    
    BCC2 = xor_result(buffer, length); //Calcula BCC2
    stuffedLength = stuffing(buffer,BCC2,&trama[4], length); //Stuffing dos dados + BCC2
    
    trama[4 + stuffedLength] = FLAG_RCV; 
    
    frameSize = stuffedLength + 5;
    
    /*------------------------INIT-TRANSMISSION----------------------------------------*/
    alarmCounter = 1; 
    while(alarmCounter <= ll->numTransmissions && error){
        
        bytesWritten = write(fd ,trama ,frameSize);  //Envia trama I
        if(tt->debug) printf("%d bytes sent, frame size = %d\n", bytesWritten, frameSize);
        
        for(i = 0; i < 5; i++) ack[i] = 0;        
        
        alarm(0);
        alarm(ll->timeout); //activa alarme de 3s
        alarmFlag=0;
        
        bytesRead = readFrame(fd, ack, TRANSMITTER,1,0); //Espera pela resposta RR ou REJ
        
        if(ack[3]!=(ack[1]^ack[2])) {    //ack inválido
            printf("*** ACK é inválido ***\n");
            for(i = 0;i < 5;i++) printf("ACK[%d] = 0x%02x\n", i, ack[i]);
            continue;
        }else{    //ack válido
            if((ack[2] == C_RR_0) && (ll->sequenceNumber == 1)){ //Recebemos RR0
                if(tt->debug) printf("*** RR0 received ***\n");
                ll->sequenceNumber = 0;
                error = 0;
                alarm(0); //cancela alarme anterior
                break;
            }else if((ack[2] == C_RR_1) && (ll->sequenceNumber == 0)){ //Recebemos RR1
                if(tt->debug) printf("*** RR1 received ***\n");
                ll->sequenceNumber = 1;
                error = 0;
                alarm(0); //cancela alarme anterior
                break;
            }else if((ack[2] == C_REJ_0) || (ack[2] == C_REJ_1)){  //Recebemos REJ
                if(tt->debug) printf("*** REJ received ***\n");
                alarmCounter = 1; //começamos transmissão de novo?
            }else{ //ack inválido
                printf("*** ACK é inválido ***\n");
                for(i = 0;i < 5;i++) printf("ACK[%d] = 0x%02x\n", i, ack[i]);
                error = 1;
            }
        }
    }
    
    if (error == 1){
        return -1;
    }
    
	return frameSize;
}
