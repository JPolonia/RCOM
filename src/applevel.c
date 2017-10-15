#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <math.h>
#include "linklayer.h"

#define FALSE 0
#define TRUE 1

#define PACKET_LEN  (((MAX_SIZE - 5) / 2) - 1)

#define TRAILER_SIZE 4

#define DATA_LEN  (PACKET_LEN - TRAILER_SIZE)


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

int sendStartPacket(int fd,int fileSize, char *fileName){ //funciona em principio
    
    char buffer[DATA_LEN]; //nome da constante pode mudar porque não faz sentido
    
    int size = controlPacket(buffer, fileSize, fileName, 0x02);
    
    while(llwrite(fd, buffer, size) < 0 );
    
    return size;
}

int sendEndPacket(int fd, int fileSize, char *fileName){  //funciona em principio
    
    char buffer[DATA_LEN]; //nome da constante pode mudar porque não faz sentido
    
    int size = controlPacket(buffer, fileSize, fileName, 0x03);
    
    while(llwrite(fd, buffer, size) < 0 );
    
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
    buffer[i] = tamanho & ((2^8)-1); //L1
    i++;
    
    for(j = 0; j< tamanho; j++){
        buffer[i] = data[j];
        i++;
    }
    return i;
}

FILE *openFileTransmmiter(char *pathToFile){ //funciona
    FILE *file = fopen(pathToFile, "rb");
    if (file == NULL) {
        printf("Erro ao abrir ficheiro\n");
        assert(file != NULL);
        return NULL;
    }
    return file;
}

FILE *openFileReceiver(char *pathToFile){ //funciona
    FILE *file = fopen(pathToFile, "wb");
    if (file == NULL) {
        printf("Erro ao abrir ficheiro\n");
        assert(file != NULL);
        return NULL;
    }
    return file;
}

size_t sendData(FILE *file, int fd){ //funciona
    char buffer[DATA_LEN];
    char packet[PACKET_LEN];
    size_t bytesRead = 0;
    size_t total = 0;
    
    int packetSize = 0;
    
    int seqNumber = 0;
    
    while(bytesRead = fread(buffer, 1, DATA_LEN-1, file), bytesRead != 0){
        
        packetSize = dataPacket(packet, seqNumber, bytesRead, buffer);
        assert(packetSize == (bytesRead + TRAILER_SIZE) );
        
        
        
        while(llwrite(fd, packet, packetSize) < 0 ){
            printf("Bloqueado porque llwrite retorna < 0...\n");
        }
        
        total = total + bytesRead;
        seqNumber++;
        if(seqNumber == 256) seqNumber = 0;
    }
    return total;
}

int getSeqNumber(char *packet){
    return (int)packet[1];
}

int getPacketSize(char * packet){
    return (256 * (int)packet[2]) + (int)packet[3];
}

size_t receiveData(FILE *file, int fd){ //funciona
    char packet[PACKET_LEN];
    size_t bytesWritten = 0;
    size_t total = 0;
    
    int packetSizeRead = 0;
    int packetSize= 0;
    
    int receivedEnd = 0;
    
    int seqNumber = 0;
    
    while(receivedEnd == 0){
        packetSizeRead = llread(fd, packet);
        
        if(packet[0] == 0x03){ //flag de end
            receivedEnd = 1;
            continue;
        }
        else if(packet[0] == 0x01){ //pacote de dados
            assert(seqNumber == getSeqNumber(packet)); //verifica que recebemos o pacote de dados certo
            
            packetSize = getPacketSize(packet);
            
            assert(packetSize == (packetSizeRead-4)); //verifica que o pacote tem a quantidade de dados que vem descrita no cabeçalho
            
            bytesWritten = fwrite(&packet[4], 1, packetSizeRead-4, file);
            assert(bytesWritten == (packetSizeRead-4) ); //verifica que escreveu tudo
            
            total = total + bytesWritten;
            
            printf("Received Packet with seqNumber = %d\n", seqNumber);
            
            seqNumber++;
            if(seqNumber == 256) seqNumber = 0;
        }
        else{
            printf("Received wrong packet.\n");
        }
        
    }
    return total;
}

int getFileSize(char *buff, int sizeBuff){
    int fileSize = 0;
    int i;
    for(i = 0; i<sizeBuff ; i++){
        fileSize = fileSize + buff[i] * int_pow(256,i);
    }
    return fileSize;
}

int receiveStart(int fd, char *fileName){
    int tamanho = 0;
    int i = 0;
    int j = 0;
    int paramLen = 0;
    char packet[DATA_LEN + TRAILER_SIZE];
    while(1){
        llread(fd, packet);
        i = 0;
        if(packet[i] == 0x02){
            i++;
            
            if(packet[i] == 0x00){ // file size
                i++;
                paramLen = packet[i];
                printf("paramLen = %d\n", paramLen);
                
                i++;
                tamanho = getFileSize(&packet[i], paramLen);
                i = i + paramLen;
                
                assert(packet[i] == 0x01); //file name
                i++;
                paramLen = packet[i];
                printf("paramLen = %d\n", paramLen);
                i++;
                
                for(j = 0; j< paramLen; j++){
                    fileName[j] = packet[i];
                    i++;
                }
                fileName[j] = 0;
            }
            break;
        }
    }
    
    
    return tamanho;
}





void closeFile(FILE *file){
    fclose(file);
}


    
    





