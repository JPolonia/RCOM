/*Non-Canonical Input Processing*/


#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#define BAUDRATE B38400
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

#define TRANSMITTER 0x03
#define RECEIVER 0x01

#define MAX_tentativas 3

const int FLAG_RCV = 0x7E;
const int A = 0x03;
const int C_SET = 0x03;
const int C_UA = 0x07;


volatile int STOP=FALSE;

int ALARME_flag=1, ALARME_conta=1;

void atende(){//atende alarme
	printf("ALARME # %d\n", ALARME_conta);
	ALARME_flag=1;
	ALARME_conta++;
}

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
		
	if(ALARME_flag && (mode==TRANSMITTER) ){ 		
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

	printf("*** Trying to establish a connection. ***\n");
	switch(flag){
		case TRANSMITTER:   printf("TRANSMISTTER\n");
							msg[0] = FLAG_RCV;
							msg[1] = A;
							msg[2] = C_SET;
							msg[3] = A^C_SET;
							msg[4] = FLAG_RCV;
							ALARME_conta =	0;
							error = 1;
							while(ALARME_conta < MAX_tentativas+1 && buff[4] != FLAG_RCV && error){
								if(ALARME_flag){
							  		alarm(3); //activa alarme de 3s
							 		ALARME_flag=0;
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
								msg[0] = FLAG_RCV;
								msg[1] = A;
								msg[2] = C_UA;
								msg[3] = A^C_UA;
								msg[4] = FLAG_RCV;
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
	
	return 0;
}
int llwrite(int fd, char *buffer, int length){
	
	return 0;
}
int llclose(int fd){
	return 0;
}

int main(int argc, char** argv)
{
    int fd;
    struct termios oldtio,newtio;
   
	

    if ( (argc < 2) || 
  	     ((strcmp("/dev/ttyS0", argv[1])!=0) && 
  	      (strcmp("/dev/ttyS1", argv[1])!=0) )) {
      printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
      exit(1);
    }


  /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
  */
    
    fd = open(argv[1], O_RDWR | O_NOCTTY );
    if (fd <0) {perror(argv[1]); exit(-1); }

    if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
      perror("tcgetattr");
      exit(-1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME]    = 1;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 0;   /* blocking read until 5 chars received */

  /* 
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a 
    leitura do(s) próximo(s) caracter(es)
  */

    tcflush(fd, TCIOFLUSH);

    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

 

	(void) signal(SIGALRM, atende);  // instala  rotina que atende interrupcao
	llopen(fd, TRANSMITTER);

    sleep(1);
    tcsetattr(fd,TCSANOW,&oldtio);
    close(fd);
    return 0;
}
