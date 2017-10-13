#ifndef APPLEVEL_H_
#define APPLEVEL_H_


int int_pow(int base, int exp);

int numberOfOctets(int number);

int controlPacket(char *buffer, int fileSize, char *fileName, char C);

int startPacket(char *buffer, int fileSize, char *fileName);

int endPacket(char *buffer, int fileSize, char *fileName);

int dataPacket(char *buffer, int seqNumber, int tamanho, char *data );

FILE *openFileTransmmiter(char *pathToFile);

FILE *openFileReceiver(char *pathToFile);

size_t sendData(FILE *file, int fd);

int getSeqNumber(char *packet);

int getPacketSize(char * packet);

int getFileSize(char *buff, int sizeBuff);

int receiveStart(int fd, char *fileName);

size_t receiveData(FILE *file, int fd);

void closeFile(FILE *file);

#endif /* applevel_H_ */
