#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "alarm.h"
#include "applevel.h"

//#include <sys/types.h>
//#include <sys/stat.h>
//#include <fcntl.h>

//#define BAUDRATE B38400

//#define MAX_TRIES 3
//#define TIMEOUT 3

//linkLayer* ll;

int main(int argc, char** argv)
{
    //FILE *f = openFileTransmmiter("src/applevel.c");
    //sendData(f, 4);
    
    //FILE *f = openFileReceiver("src/teste.txt");
    //printf("TOTAL = %d\n", receiveData(f, 4)  );
 
    
    char fileName[20];
    int tamanho = receiveStart(4, fileName);
    printf("tamanho = %d \nname = %s \n", tamanho, fileName);
    

    
}
