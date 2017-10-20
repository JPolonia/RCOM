#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <assert.h>
#include "testes.h"
#include "linklayer.h"

#define ACTIVE 1

testes *tt;

int initTestes(	int active,
				double headerErrorRate,
				double packetErrorRate,
				int addedDelay,
                int debug){


	srand( time(NULL) );

	tt = (testes*) malloc(sizeof(testes));
	tt->active = active;
	tt->headerErrorRate = headerErrorRate;
	tt->packetErrorRate = packetErrorRate;
	tt->addedDelay = addedDelay;
    tt->debug = debug;

	return 1;
}

int insertHeaderError(unsigned char *buff, int length, double errorRate){
    if(errorRate != 0){
        if(rand() < (RAND_MAX+1u) / (1/errorRate)){
            buff[3] = buff[3] ^ 0xFF;
        }
    }
	return 1;
}

int insertPacketError(unsigned char *buff, int length, double errorRate){
    if(errorRate != 0){
        if(rand() < (RAND_MAX+1u) / (1/errorRate)){
            buff[length - 2] = buff[length - 2] ^ 0xFF;
        }
    }
	return 1;
}

void teste_cli(char** argv){
	int e;

	int command;
	int mode = 0; //=1 -> testes ativados
    
    //Debug mode
    int debug = 0; //=1 -> debug mode ativado

	//FER
	double headerErrorRate;
	double packetErrorRate;

	//T_PROP
	int t_prop; //em milisegundos

	//C -> BaudRate
	int nbaud; //OK
	int baud; //OK

	//Tamanho da trama
	int max_size;//OK

	
	int max_tries;//OK
	int time_out;//OK

    
    //Obter paramentros__________________________________________________
    
    printf("Pretende entrar em modo de debug? ( 1-sim / 0-não )\n");
    assert(scanf("%d", &debug)>0);
    assert(debug == 1 || debug == 0);
    
    
	printf("Pretende entrar em modo de teste? ( 1-sim / 0-não )\n");
	assert(scanf("%d", &command)>0);
    assert(command == 1 || command == 0);

	if(command == 1){
		mode = ACTIVE;

		printf("Qual o tempo de timeout? (secs)\n");
		assert(scanf("%d", &time_out)>0);
		assert(time_out >= 0);

		printf("Qual o número máximo de tentativas após timeout?\n");
		assert(scanf("%d", &max_tries)>0);
		assert(max_tries >= 0);

		printf("Qual o tamanho máximo que a trama pode ter?\n");
		assert(scanf("%d", &max_size)>0);
		assert(max_size > 20);
		
		printf("Qual a probabilidade de haver erro no cabeçalho das tramas I? (entre 0 e 1)\n");
		assert(scanf("%lf", &headerErrorRate)>0);
		assert(headerErrorRate >= 0 && headerErrorRate < 1);
			
		printf("Qual a probabilidade de haver erro nos dados das tramas I? (entre 0 e 1)\n");
		assert(scanf("%lf", &packetErrorRate)>0);
		assert(packetErrorRate >= 0 && packetErrorRate < 1);

		printf("Qual o atraso adicional que vai ser inserido? (em milisegundos)\n");
		assert(scanf("%d", &t_prop)>0);
		assert(t_prop >= 0);

		printf("Qual das seguintes Baudrates pretende escolher?\n");
		printf("1-2400 2-4800 3-9600 4-19200 5-38400 6-57600 7-115200 8-230400 9-460800?\n");
		assert(scanf("%d", &nbaud)>0);
		assert(nbaud >= 1 && nbaud <= 9);

		switch (nbaud){
			case 1:	
				baud = B2400;
				break;
			case 2:	
				baud = B4800;
				break;
			case 3:	
				baud = B9600;
				break;
			case 4:	
				baud = B19200;
				break;
			case 5:	
				baud = B38400;
				break;
			case 6:	
				baud = B57600;
				break;
			case 7:	
				baud = B115200;
				break;
			case 8:	
				baud = B230400;
				break;
			case 9:	
				baud = B460800;
				break;
	
		}
		
	}
	else{
		mode = !ACTIVE;
	}	

	

	//preencher estruturas_______________________________________________

	if(mode == ACTIVE){ //MODO TESTE
		e = initLinkLayer(argv[1], baud,0, time_out, max_tries, max_size);
		if (e <0) { printf("Couldnt initialize linklayer\n"); exit(-1); }


		e = initTestes(mode, headerErrorRate, packetErrorRate, t_prop, debug);
		if (e <0) { printf("Couldnt initialize testes\n"); exit(-1); }


	}
	else{ //MODO NORMAL

		e = initTestes(mode, 0, 0, 0, debug);
		if (e <0) { printf("Couldnt initialize testes\n"); exit(-1); }

	}
}


