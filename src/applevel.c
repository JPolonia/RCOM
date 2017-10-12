#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define FALSE 0
#define TRUE 1

#define MAX_SIZE 256

int numberOfOctets(int number){
    int n = 0;
    while (number != 0) {
        number >>= 8;
        n ++;
    }
    return n;
}

int controlPacket(char *buffer, int fileSize, char *fileName, char C){
    int i = 0;
    int j = 0;
    int exp = 0;
    int sum = 0;
    
    
    buffer[i] = C; //start/end control flag
    i++;
    
    buffer[i] = 0x00; //file size flag
    i++;
    
    buffer[i] = (char) numberOfOctets(fileSize); //file size number of bytes
    i++;
    
    exp = 0;
    sum = 0;
    for(j = 0 ; j < numberOfOctets(fileSize) ; j++){
        buffer[i] = (char)( (fileSize & (2^(exp+8) - sum)) >> exp);// anda de 8 em 8 bits e arrasta para a direita;
        exp = exp + 8;
        i++;
        sum = sum + 2^exp;
    }
    
    buffer[i] = 0x01; //file name flag
    i++;
    
    buffer[i] = (char) strlen(fileName);
    i++;
    
    for(j = 0; j < strlen(fileName); j++){
        buffer[i] = fileName[j];
        i++;
    }
    
    return i;
  
    
}

int startPacket(char *buffer, int fileSize, char *fileName){
    return controlPacket(buffer, fileSize, fileName, 0x02);
}

int endPacket(char *buffer, int fileSize, char *fileName){
    return controlPacket(buffer, fileSize, fileName, 0x03);
}

int dataPacket(char *buffer, int seqNumber, int tamanho, char *data ){
    int i = 0;
    int j = 0;
    
    buffer[i] = 0x01; //data control flag C
    i++;
    
    buffer[i] = (char) seqNumber; //Sequence Number N
    i++;
    
    buffer[i] = tamanho >> 8;  //L2
    i++;
    buffer[i] = tamanho & (2^8); //L1
    i++;
    
    for(j = 0; j< tamanho; j++){
        buffer[i] = data[j];
        i++;
    }
    return i;
}


