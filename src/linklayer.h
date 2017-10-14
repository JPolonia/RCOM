#ifndef LINKLAYER_H_
#define LINKLAYER_H_

#pragma once

#include <termios.h>

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

int initLinkLayer(char* port,int baudRate,unsigned int sequenceNumber,
	unsigned int timeout,unsigned int numTransmissions);
int openSerialPort(char* port);
int closeSerialPort( int fd);
int initTermios(int fd);

int readpacket(int fd,unsigned char *buffer,int state, char mode,int index);
int llopen(int fd, char flag);
int llread(int fd, char *buffer);
int destuffing(char *buff, char *buff_destuff);
char xor_result(char *array, int tam);
int size_stuffing(char *buff, int length);
void stuffing(char *buff,char *stuffed_buffer, int length);
int llwrite(int fd, char *buffer, int length);
int llclose(int fd);

#endif /* linklayer_H_ */
