#ifndef APPLEVEL_H_
#define APPLEVEL_H_

#define DATA_LEN  (((MAX_SIZE - 5) / 2) - 1)

int int_pow(int base, int exp);

int numberOfOctets(int number);

int controlPacket(unsigned char *buffer, int fileSize, char *fileName,unsigned char C);

int sendStartPacket(int fd, int fileSize, char *fileName);

int sendEndPacket(int fd, int fileSize, char *fileName);

int dataPacket(unsigned char *buffer, int seqNumber, int tamanho, unsigned char *data );

FILE *openFileTransmmiter(char *pathToFile);

FILE *openFileReceiver(char *pathToFile);

size_t sendData(FILE *file, int fd);

int getSeqNumber(unsigned char *packet);

int getPacketSize(unsigned char * packet);

int getFileSize(unsigned char *buff, int sizeBuff);

int receiveStart(int fd, char *fileName);

size_t receiveData(FILE *file, int fd);

void closeFile(FILE *file);

#endif /* applevel_H_ */
