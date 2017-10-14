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

void readpacket(int fd, unsigned char *buffer, unsigned char mode); //OK
//devolve trama inteira incluindo 0x7e no inicio e fim em buffer
//mode = TRANSMITTER ou RECEIVER


int llopen(int fd, unsigned char flag);

int llread(int fd, char *buffer);

int destuffing( unsigned char *buff, char *buffDestuff); //OK
//devolve campo de dados + BCC em buffDestuff
//retorna tamanho de buffDestuff

unsigned char xor_result(char *array, int tam); //OK
//faz xor normalmente e retorna

int size_stuffing(char *buff, int length);
int stuffing(char *buff,char *stuffedBuffer, int length); //OK
//faz stuffing do bloco de dados + BCC
//retorna tamanho de stuffedBuffer

int llwrite(int fd,char *buffer, int length);
int llclose(int fd);

#endif /* linklayer_H_ */
