#ifndef LINKLAYER_H_
#define LINKLAYER_H_

#pragma once

#define MAX_SIZE 256
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

#define TRANSMITTER 0x03
#define RECEIVER 0x01


typedef struct linkLayer {
	char port[20]; //Port /dev/ttySx
	int baudRate; //Transmission speed
	unsigned int sequenceNumber; //Frame sequence number (0, 1)
	unsigned int timeout; //Valor do temporizador
	unsigned int numTransmissions; //Numero Tentativas em caso de falha
	char frame[MAX_SIZE]; //Trama
	struct termios oldtio,newtio; //Struct to save old and new termios
} linkLayer;

extern linkLayer* ll;

/*typedef enum {
	C_SET = 0x03, C_UA = 0x07, C_RR = 0x05, C_REJ = 0x01, C_DISC = 0x0B
} ControlField;

typedef enum {
	A_TX = 0x03, A_RX = 0x01
} AdressField;gi

typedef struct SUframe {
	char F;
	AdressField A;
	ControlField C;
	char BCC;
} SUFrame;

typedef struct Iframe {
	struct SUframe;
	char* Payload;
} IFrame;*/	

int initLinkLayer(char* port,int baudRate,unsigned int sequenceNumber,
	unsigned int timeout,unsigned int numTransmissions);

void readpacket(int fd,unsigned char *buffer,int state, char mode);
int llopen(int fd, char flag);
int llread(int fd, char *buffer);
int size_stuffing(char *buff);
void stuffing(char *buff,char *stuffed_buffer);
int llwrite(int fd, char *buffer, int length);
int llclose(int fd);

#endif /* linklayer_H_ */