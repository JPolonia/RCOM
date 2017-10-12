#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "linklayer.h"
#include "alarm.h"


linkLayer* ll;

int main(int argc, char** argv)
{
    int fd;
    struct termios oldtio,newtio;
    /*char file_name[] = "test.png";
    int fd_file;   */
	

    if ( (argc < 2) || 
  	     ((strcmp("/dev/ttyS0", argv[1])!=0) && 
  	      (strcmp("/dev/ttyS1", argv[1])!=0) )) {
      printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
      exit(1);
    }


	//Initialize linkLayer
	ll = (linkLayer*) malloc(sizeof(linkLayer));
	strcpy(ll->port, argv[1]);
	ll->baudRate = BAUDRATE;
	ll->sequenceNumber = 0;
	ll->timeout = TIMEOUT;
	ll->numTransmissions = MAX_TRIES;



  /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
  */    
    fd = open(ll->port, O_RDWR | O_NOCTTY );
    if (fd <0) {perror(ll->port); exit(-1); }


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
    
    /*Open File to be transfered*/
    //fd_file = open(file_name, O_RDONLY);
 

    //Initialize alarm
	  initAlarm();

    llopen(fd, RECEIVER);
    //llwrite(fd, file_name,sizeof(file_name));
    
    

    sleep(1);
    tcsetattr(fd,TCSANOW,&oldtio);
    close(fd);
    return 0;
}