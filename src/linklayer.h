#ifndef LINKLAYER_H_
#define LINKLAYER_H_

#pragma once

#include <termios.h>

#define MAX_SIZE 4096
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

typedef struct testes {
	int ferActive;
	int TpropActive;
} testes;

extern linkLayer* ll;

int initLinkLayer(char* port,int baudRate,unsigned int sequenceNumber,
	unsigned int timeout,unsigned int numTransmissions);
int openSerialPort(char* port);
int closeSerialPort( int fd);
int initTermios(int fd);

int readpacket(int fd, unsigned char *buffer, unsigned char mode); //OK
//devolve trama inteira incluindo 0x7e no inicio e fim em buffer
//mode = TRANSMITTER ou RECEIVER


int llopen(int fd, unsigned char mode); //OK
//retorna 1 em caso de sucesso, -1 em caso de erro
//mode = TRANSMITTER ou RECEIVER
//se mode == RECEIVER bloqueia até establecer ligação
//se mode == TRANSMITTER tenta establecer ligação algumas vezes e depois retorna -1 caso não consiga

int llread(int fd,unsigned char *buffer);
//recebe pacote de dados para buffer
//retorna tamanho de buffer

int destuffing( unsigned char *buff, unsigned char *buffDestuff); //OK
//devolve campo de dados + BCC em buffDestuff
//retorna tamanho de buffDestuff

unsigned char xor_result(unsigned char *array, int tam); //OK
//faz xor normalmente e retorna

int stuffing(unsigned char *buff, unsigned char BCC2 , unsigned char *stuffedBuffer, int length); //OK (falta verificar com BCC2)
//faz stuffing do bloco de dados + BCC
//retorna tamanho de stuffedBuffer

int llwrite(int fd, unsigned char *buffer, int length);
//escreve pacote de dados de buffer, assume que pacote tem lenght carateres
//retorna tamanho da frame ou -1 em caso de erro

int llclose(int fd, unsigned char mode);
//mode = TRANSMITTER ou RECEIVER

#endif /* linklayer_H_ */
