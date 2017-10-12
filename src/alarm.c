#include "alarm.h"
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

int alarmFlag=1, alarmCounter=1;

void alarmHandler(){//atende alarme
	printf("ALARME # %d\n", alarmCounter);
	alarmFlag=1;
	alarmCounter++;
}

void initAlarm(){
    (void) signal(SIGALRM, alarmHandler);  // instala  rotina que atende interrupcao
}