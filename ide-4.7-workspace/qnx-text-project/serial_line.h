/*
 * serial_line.h
 *
 *  Created on: Nov 8, 2021
 *      Author: user
 */

#ifndef SERIAL_LINE_H_
#define SERIAL_LINE_H_

#define COMMAND_A 0
#define COMMAND_B 1

void initSerialLine();

void prepareToSendOnSerialLine(unsigned char* in, unsigned char* out);

void sendOnSerialLine(unsigned char* data, int size, int isTaskMemoryCheck);

int receiveCommand();

#endif /* SERIAL_LINE_H_ */
