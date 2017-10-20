#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <math.h>
#include "linklayer.h"
#include "progressbar.h"


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

int dataPacket(unsigned char *buffer, int seqNumber, int tamanho,unsigned char *data ){ //controi pacote de dados em buffer
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
    
    for(j = 0; j< tamanho; j++){ //copia data para buffer
        buffer[i] = data[j];
        i++;
    }
    return i; //retorna tamanho do pacote de dados
}

FILE *openFileTransmmiter(char *pathToFile){ //abre ficheiro com permissoes de leitura em modo binário
    FILE *file = fopen(pathToFile, "rb");
    return file;
}

FILE *openFileReceiver(char *pathToFile){ //abre ficheiro com permissoes de escrita em modo binário
    FILE *file = fopen(pathToFile, "wb");
    return file;
}

size_t sendData(FILE *file, int fd,int fileSize){ //lê blocos do ficheiro, e envia em pacotes
    unsigned char buffer[DATA_LEN]; //para ler bloco
    unsigned char packet[PACKET_LEN]; //para construir pacote
    size_t bytesRead = 0;
    size_t total = 0;
    
    int packetSize = 0;
    int packetNumber = 0;
    int seqNumber = 0;
    
    while(bytesRead = fread(buffer, 1, DATA_LEN-1, file), bytesRead != 0){ //tenta enviar até não conseguir ler mais nada do ficheiro
        
        packetSize = dataPacket(packet, seqNumber, bytesRead, buffer); //controi pacote
        if(packetSize != (bytesRead + TRAILER_SIZE) ){ //verifica que o tamanho do pacote só acresce do número de bytes do cabeçalho em relação ao bloco de dados
            printf("Erro em dataPacket().\n");
            return -1;
        }
        
        
        
        if(llwrite(fd, packet, packetSize) < 0 ){ //envia pacote
            printf("llwrite() retornou valor negativo\n");
            printf("Erro ao enviar packet %d\n", packetNumber);
            return -1;
        }
        
        //printf("Packet number %d sent\n", packetNumber);
        
        //atualizar progress bar
        printProgress( (double)total/(double)fileSize );
        
        total = total + bytesRead; //total guarda numero total de bytes enviados
        packetNumber++; //numero do pacote atual para efeitos de debugging
        seqNumber++; //numero do pacote em módulo 255 para incluir no cabeçalho do pacote
        if(seqNumber == 256) seqNumber = 0;
    }
    printf("\n"); // por causa do progress bar
    return total;
}

int getSeqNumber(unsigned char *packet){ //determina nr de sequencia do pacote
    return (int)packet[1];
}

int getPacketSize(unsigned char * packet){ //determina tamanho do pacote
    int packetSize = 0;
    packetSize = packet[2] * 256;
    packetSize = packetSize + packet[3];
    return packetSize;
}

size_t receiveData(FILE *file, int fd){ //recebe pacotes de dados e escreve bloco a bloco num ficheiro até receber o pacote de controlo END
    unsigned char packet[PACKET_LEN];
    size_t bytesWritten = 0;
    size_t total = 0;
    
    int packetSizeRead = 0;
    int packetSize= 0;
    
    int receivedEnd = 0;
    
    int packetNumber = 0;
    int seqNumber = 0;
    
    while(receivedEnd == 0){ //receivedEND é a flag que indica que foi recebido pacote END
        packetSizeRead = llread(fd, packet); //obtem novo pacote
        
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
            
            packetSize = getPacketSize(packet); //obtem tamanho pacote
            
            if(packetSize != (packetSizeRead-4)){//verifica que o pacote tem a quantidade de dados que vem descrita no cabeçalho
                printf("Numero de bytes no pacote não corresponde ao que deveria chegar!\n");
                printf("Queriamos receber %d bytes.\n", packetSize);
                printf("Recebemos %d bytes\n", packetSizeRead-4);
                return -1;
                
            }
            
            bytesWritten = fwrite(&packet[4], 1, packetSizeRead-4, file); //escreve dados no ficheiro
            
            if(bytesWritten != (packetSizeRead-4) ){ //verifica que escreveu tudo
                printf("Não foi possivel escrever tudo no ficheiro!\n");
                printf("Queriamos escrever %d bytes.\n", packetSizeRead-4);
                printf("Escrevemos %zu bytes\n", bytesWritten);
                return -1;
            }
            
            total = total + bytesWritten; //soma de todos os bytes recebidos
            
            //printf("Packet number %d received\n", packetNumber);
            packetNumber++; //numero do pacote para efeitos de debugging
            seqNumber++;  //numero da sequencia em modulo 255 para determinar se recebemos pacote certo
            if(seqNumber == 256) seqNumber = 0;
        }
        else{
            printf("Received wrong packet.\n");
        }
        
    }
    return total; //retorna total de bytes que recebeu / escreveu no ficheiro
}

int getFileSize(unsigned char *buff, int sizeBuff){ //determina tamanho do ficheiro a partir do cabeçalho do pacote, tem de receber *buff a apontar para a primeira posição do número e o numero de bytes do numero em sizeBuff
    int fileSize = 0;
    int i;
    for(i = 0; i<sizeBuff ; i++){
        fileSize = fileSize + buff[i] * int_pow(256, sizeBuff - 1 - i);
    }
    return fileSize;
}

int receiveStart(int fd,char *fileName){ //espera pelo pacote START sem timeout -> bloqueia
    int tamanho = 0;
    int i = 0;
    int j = 0;
    int paramLen = 0;
    unsigned char packet[DATA_LEN + TRAILER_SIZE];
    while(1){
        llread(fd, packet); //obtem pacote
        i = 0;
        if(packet[i] == 0x02){ //é pacote START
            i++;
            
            if(packet[i] == 0x00){ //flag file size
                i++;
                paramLen = packet[i]; //numero de bytes ocupados por file size
                
                i++;
                tamanho = getFileSize(&packet[i], paramLen); //obtem file size
                i = i + paramLen;
                
                if(packet[i] != 0x01){ //não vem file name
                    printf("Queriamos que o 2º parametro fosse o File Name mas não chegou isso.\n");
                    return -1;
                }
                i++;
                paramLen = packet[i]; //numero de bytes ocupados pelo file name
                i++;
                
                for(j = 0; j< paramLen; j++){ //copa file name para fileName
                    fileName[j] = packet[i];
                    i++;
                }
                fileName[j] = 0; //adiciona '\0' para que seja uma string válida
            }
            break;
        }
    }
    return tamanho; //retorna tamanho do ficheiro que vai ser recebido
}

void closeFile(FILE *file){ //fecha ficheiro, podia ser descartado mas existe para haver consistência com as funções de abrir ficheiro
    fclose(file);
}


    
    





