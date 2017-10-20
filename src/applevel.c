#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <math.h>
#include "linklayer.h"

#define FALSE 0
#define TRUE 1

#define PACKET_LEN  (((ll->max_size - 5) / 2) - 1)
#define TRAILER_SIZE 4
#define DATA_LEN  (PACKET_LEN - TRAILER_SIZE)


int int_pow(int base, int exp){ //semelhante a pow() mas com int's
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

int numberOfOctets(int number){  //determina a quantidade de octetos necessários para guardar number
    int n = 0;
    while (number != 0) {
        number >>= 8;
        n ++;
    }
    return n;
}

int controlPacket(unsigned char *buffer, int fileSize, char *fileName, unsigned char C){  //constroi pacote de controlo em buffer a partir de fileSize, fileName C
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
    for(j = 0 ; j < nOctets ; j++){
        buffer[i] = (char)( (fileSize & (int_pow(2, exp)-1)   ) >> (exp-8));// anda de 8 em 8 bits e arrasta para a direita;
        exp = exp - 8;
        i++;
    }
    
    buffer[i] = 0x01; //file name flag
    i++;
    
    buffer[i] = (char) strlen(fileName); //file name lenght
    i++;
    
    for(j = 0; j < strlen(fileName); j++){ //copia file name
        buffer[i] = fileName[j];
        i++;
    }
    
    return i; //retorna tamanho do pacote
  
    
}

int sendStartPacket(int fd,int fileSize, char *fileName){ //envia START packet
    
    unsigned char buffer[DATA_LEN]; //limite de octetos no buffer é igual ao máximo de dados que podem ser enviados
    
    int size = controlPacket(buffer, fileSize, fileName, 0x02); //C = 0x02 -> START
    
    if(llwrite(fd, buffer, size) < 0 ){ //envia
        printf("llwrite() retornou valor negativo\n");
        return -1;
    }
    
    return size; //retorna tamanho do pacote
}

int sendEndPacket(int fd, int fileSize,char *fileName){  //envia END packet
    
    unsigned char buffer[DATA_LEN]; //limite de octetos no buffer é igual ao máximo de dados que podem ser enviados
    
    int size = controlPacket(buffer, fileSize, fileName, 0x03); //C= = 0x03 -> END
    
    if(llwrite(fd, buffer, size) < 0 ){ //envia
        printf("llwrite() retornou valor negativo\n");
        return -1;
    }
    
    return size; //retorna tamanho do pacote
}

int dataPacket(unsigned char *buffer, int seqNumber, int tamanho,unsigned char *data ){ //falta verificar
    int i = 0;
    int j = 0;
    
    buffer[i] = 0x01; //data control flag C
    i++;
    
    buffer[i] = (char) seqNumber; //Sequence Number N
    i++;
    
    buffer[i] = tamanho >> 8;  //L2
    i++;
    
    buffer[i] = tamanho & 0b11111111; //L1
    i++;
    
    for(j = 0; j< tamanho; j++){
        buffer[i] = data[j];
        i++;
    }
    return i;
}

FILE *openFileTransmmiter(char *pathToFile){ //funciona
    FILE *file = fopen(pathToFile, "rb");
    return file;
}

FILE *openFileReceiver(char *pathToFile){ //funciona
    FILE *file = fopen(pathToFile, "wb");
    return file;
}

size_t sendData(FILE *file, int fd){ //funciona
    unsigned char buffer[DATA_LEN];
    unsigned char packet[PACKET_LEN];
    size_t bytesRead = 0;
    size_t total = 0;
    
    int packetSize = 0;
    int packetNumber = 0;
    int seqNumber = 0;
    
    while(bytesRead = fread(buffer, 1, DATA_LEN-1, file), bytesRead != 0){
        
        packetSize = dataPacket(packet, seqNumber, bytesRead, buffer);
        if(packetSize != (bytesRead + TRAILER_SIZE) ){
            printf("Erro em dataPacket().\n");
            return -1;
        }
        
        
        
        if(llwrite(fd, packet, packetSize) < 0 ){
            printf("llwrite() retornou valor negativo\n");
            printf("Erro ao enviar packet %d\n", packetNumber);
            return -1;
        }
        
        //printf("Packet number %d sent\n", packetNumber);
        
        total = total + bytesRead;
        packetNumber++;
        seqNumber++;
        if(seqNumber == 256) seqNumber = 0;
    }
    return total;
}

int getSeqNumber(unsigned char *packet){
    return (int)packet[1];
}

int getPacketSize(unsigned char * packet){
    int packetSize = 0;
    packetSize = packet[2] * 256;
    packetSize = packetSize + packet[3];
    return packetSize;
}

size_t receiveData(FILE *file, int fd){ //funciona
    unsigned char packet[PACKET_LEN];
    size_t bytesWritten = 0;
    size_t total = 0;
    
    int packetSizeRead = 0;
    int packetSize= 0;
    
    int receivedEnd = 0;
    
    int packetNumber = 0;
    int seqNumber = 0;
    
    while(receivedEnd == 0){
        packetSizeRead = llread(fd, packet);
        
        if(packet[0] == 0x03){ //flag de end
            receivedEnd = 1;
            continue;
        }
        else if(packet[0] == 0x01){ //pacote de dados
            if(seqNumber != getSeqNumber(packet)){ //verifica que recebemos o pacote de dados certo
                printf("Foi recebido pacote errado!\n");
                printf("Queriamos sequence number = %d\n", seqNumber);
                printf("Recebemos sequence number = %d\n", getSeqNumber(packet));
                return -1;
            }
            
            packetSize = getPacketSize(packet);
            
            if(packetSize != (packetSizeRead-4)){//verifica que o pacote tem a quantidade de dados que vem descrita no cabeçalho
                printf("Numero de bytes no pacote não corresponde ao que deveria chegar!\n");
                printf("Queriamos receber %d bytes.\n", packetSize);
                printf("Recebemos %d bytes\n", packetSizeRead-4);
                return -1;
                
            }
            
            bytesWritten = fwrite(&packet[4], 1, packetSizeRead-4, file);
            if(bytesWritten != (packetSizeRead-4) ){ //verifica que escreveu tudo
                printf("Não foi possivel escrever tudo no ficheiro!\n");
                printf("Queriamos escrever %d bytes.\n", packetSizeRead-4);
                printf("Escrevemos %zu bytes\n", bytesWritten);
                return -1;
            }
            total = total + bytesWritten;
            
            //printf("Packet number %d received\n", packetNumber);
            packetNumber++;
            seqNumber++;
            if(seqNumber == 256) seqNumber = 0;
        }
        else{
            printf("Received wrong packet.\n");
        }
        
    }
    return total;
}

int getFileSize(unsigned char *buff, int sizeBuff){
    int fileSize = 0;
    int i;
    for(i = 0; i<sizeBuff ; i++){
        
        //printf("buff[%d] = 0x%02x\n", i, buff[i]);
        
        fileSize = fileSize + buff[i] * int_pow(256, sizeBuff - 1 - i);
    }
    return fileSize;
}

int receiveStart(int fd,char *fileName){
    int tamanho = 0;
    int i = 0;
    int j = 0;
    int paramLen = 0;
    unsigned char packet[DATA_LEN + TRAILER_SIZE];
    while(1){
        llread(fd, packet);
        i = 0;
        if(packet[i] == 0x02){
            i++;
            
            if(packet[i] == 0x00){ // file size
                i++;
                paramLen = packet[i];
                
                i++;
                tamanho = getFileSize(&packet[i], paramLen);
                i = i + paramLen;
                
                //printf("tamanho = %d\n", tamanho);
                
                if(packet[i] != 0x01){ //não vem file name
                    printf("Queriamos que o 2º parametro fosse o File Name mas não chegou isso.\n");
                    return -1;
                }
                i++;
                paramLen = packet[i];
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


    
    





