#ifndef MESSAGEBOARD_H
#define MESSAGEBOARD_H

void * MessageBoard(void * socket);
int getStringSize(char stringToRead[]);
int getIntFromString(int startingIndex, char stringToRead[],
                     int sizeOfString, int *parseIndex);

#endif
