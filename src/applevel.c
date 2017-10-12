#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <math.h>

#define FALSE 0
#define TRUE 1

#define MAX_SIZE 10

int int_pow(int base, int exp){ // funciona
    int result = 1;
    while (exp)
    {
        if (exp & 1)
            result *= base;
        exp /= 2;
        base *= base;
    }
    return result;
}

int numberOfOctets(int number){  //funciona
    int n = 0;
    while (number != 0) {
        number >>= 8;
        n ++;
    }
    return n;
}

int controlPacket(char *buffer, int fileSize, char *fileName, char C){  //funciona acho :P
    int i = 0;
    int j = 0;
    int exp = 0;
    int nOctets = 0;
    
    
    buffer[i] = C; //start/end control flag
    i++;
    
    buffer[i] = 0x00; //file size flag
    i++;
    
    nOctets = numberOfOctets(fileSize);
    buffer[i] = (char) nOctets; //file size number of bytes
    i++;
    
    exp = 8 * nOctets;
    for(j = 0 ; j < nOctets ; j++){ //OK
        buffer[i] = (char)( (fileSize & (int_pow(2, exp)-1)   ) >> (exp-8));// anda de 8 em 8 bits e arrasta para a direita;
        //printf("\n%d\n", buffer[i]);
        
        exp = exp - 8;
        i++;
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

int startPacket(char *buffer, int fileSize, char *fileName){ //funciona
    return controlPacket(buffer, fileSize, fileName, 0x02);
}

int endPacket(char *buffer, int fileSize, char *fileName){  //funciona
    return controlPacket(buffer, fileSize, fileName, 0x03);
}

int dataPacket(char *buffer, int seqNumber, int tamanho, char *data ){ //falta verificar
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

FILE *openFile(char *pathToFile){ //funciona
    FILE *file = fopen(pathToFile, "rb");
    if (file == NULL) {
        printf("Erro ao abrir ficheiro\n");
        return NULL;
    }
    return file;
}

size_t sendData(FILE *file, int fd){ //funciona
    char buffer[MAX_SIZE];
    size_t bytesRead = 0;
    size_t total = 0;
    
    int seqNumber = 0;
    
    while(bytesRead = fread(buffer, 1, MAX_SIZE-1, file), bytesRead != 0){
        
        //falta chamar dataPacket()
        
        
        //llwrite(fd, buffer, bytesRead); //retirar quando juntar código
        total = total + bytesRead;
        seqNumber++;
    }
    return total;
}

void closeFile(FILE *file){
    fclose(file);
}
    
    
    
    





