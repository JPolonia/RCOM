#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "linklayer.h"
#include "alarm.h"



#define BAUDRATE B38400

#define MAX_TRIES 3
#define TIMEOUT 3

linkLayer* ll;

int main(int argc, char** argv)
{
    int fd, e;
    char mode;
    char file_name[] = "ola";
    char read_str[255];
    //int fd_file;  
	

    if ( (argc < 3) || 
         ((strcmp("/dev/ttyS0", argv[1])) && (strcmp("/dev/ttyS1", argv[1])) ) ||
         ((strcmp("TRANSMITTER", argv[2])) && (strcmp("RECEIVER", argv[2])) )) {
      printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1 RECEIVER\n");
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
    
    /*Open File to be transfered*/
    //fd_file = open(file_name, O_RDONLY);
    

    //Initialize alarm
    initAlarm();

    
    mode = (strcmp("TRANSMITTER", argv[2])) ? RECEIVER : TRANSMITTER;
    llopen(fd, mode);
    switch(mode){
        case TRANSMITTER: llwrite(fd, file_name,strlen(file_name));
                          break;
        case RECEIVER:    llread(fd,read_str);
                          printf("STR: %s \n",read_str);
                          break;
      }
    

    sleep(1);
   
    return 0;
}