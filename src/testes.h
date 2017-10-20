#ifndef TESTES_H_
#define TESTES_H_

#pragma once

typedef struct testes {
	int ferTestActive;
	double headerErrorRate;
	double packetErrorRate;

	int TpropTestActive;
	int addedDelay;

	int capacityTestActive;
} testes;


void initStat();

int insertHeaderError(unsigned char *buff, int length, double errorRate);

int insertPacketError(unsigned char *buff, int length, double errorRate);


#endif /* testes_H_ */
