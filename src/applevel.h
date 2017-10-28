#ifndef APPLEVEL_H_
#define APPLEVEL_H_

#define DATA_LEN  (((ll->max_size - 5) / 2) - 1)


int int_pow(int base, int exp);

int numberOfOctets(int number);

int controlPacket(unsigned char *buffer, int fileSize, char *fileName, unsigned char *hash, unsigned char C);

int sendStartPacket(int fd, int fileSize, char *fileName, unsigned char *hash);

int sendEndPacket(int fd, int fileSize, char *fileName, unsigned char *hash);

int dataPacket(unsigned char *buffer, int seqNumber, int tamanho, unsigned char *data );

FILE *openFileTransmmiter(char *pathToFile);

FILE *openFileReceiver(char *pathToFile);

size_t sendData(FILE *file, int fd, int fileSize);

int getSeqNumber(unsigned char *packet);

int getPacketSize(unsigned char * packet);

int getFileSize(unsigned char *buff, int sizeBuff);

int receiveStart(int fd, char *fileName, unsigned char *hash);

size_t receiveData(FILE *file, int fd, int fileSize);

void closeFile(FILE *file);

#endif /* applevel_H_ */
