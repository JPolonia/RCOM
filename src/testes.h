#ifndef TESTES_H_
#define TESTES_H_

#pragma once



typedef struct testes {
	int active;
	double headerErrorRate;
	double packetErrorRate;
	int addedDelay;
    int debug;
} testes;

extern testes *tt;

int insertHeaderError(unsigned char *buff, int length, double errorRate);

int insertPacketError(unsigned char *buff, int length, double errorRate);

void teste_cli(char** argv);


#endif /* testes_H_ */
