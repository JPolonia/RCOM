#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>


void initStat(){
	srand ( time(NULL) );
}

int insertHeaderError(unsigned char *buff, int length, double errorRate){
	if(rand() < (RAND_MAX+1u) / (1/errorRate)){
		buff[3] = buff[3] ^ 0xFF;
	}
	return 1;
}

int insertPacketError(unsigned char *buff, int length, double errorRate){
	if(rand() < (RAND_MAX+1u) / (1/errorRate)){
		buff[length - 2] = buff[length - 2] ^ 0xFF;
	}
	return 1;
}
