/*
 *  FRGPSTesting.c
 *  GPSPrototype
 *
 *  Created by Nathan Vander Wilt on 4/10/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#include "FRGPSTesting.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

#include <sys/types.h>

/*
 Serial programming rules:
 1. Don't expect bytes to keep coming.
 2. Don't expect bytes to stop coming.
 3. Don't expect bytes that came to be the right bytes.
 */


/*
 garmin serial packet
 {
 int8_t dle1;
 u_int8_t type;
 u_int8_t size;
 u_int8_t data[size];
 u_int8_t checksum;	// 2's complement of the sum of all bytes from .type to .data[size-1]
 int8_t dle2;
 int8_t etx;
 }
 */

/*
 Checksum field ( from http://linux.die.net/man/5/srec_fpc )
 
 This field is a one byte 2's-complement checksum of the entire record.
 To create the checksum make a one byte sum from all of the bytes from all of the fields of the record:
 Then take the 2's-complement of this sum to create the final checksum. The 2's-complement is simply inverting
 all bits and then increment by 1 (or using the negative operator). Checking the checksum at the receivers end
 is done by adding all bytes together including the checksum itself, discarding all carries, and the result must
 be $00. The padding bytes at the end of the line, should they exist, should not be included in checksum. But it
 doesn't really matter if they are, for their influence will be 0 anyway.
 */


struct garmin_packet {
	char typeID;
	size_t dataSize;
	char* data;
};

typedef struct garmin_packet* FRGarminPacketRef;

FRGarminPacketRef FRGarminPacketCreateFromData(char typeID, const char* data, unsigned int size) {
	FRGarminPacketRef packet = (FRGarminPacketRef)malloc( sizeof(struct garmin_packet) );
	if (packet) {
		packet->typeID = typeID;
		packet->dataSize = size;
		packet->data = NULL;
	}
	if (packet && size) {
		char* dataCopy = (char*)malloc(size);
		if (dataCopy) {
			memcpy(dataCopy, data, size);
			packet->data = dataCopy;
		}
		else {
			free(packet);
			packet = NULL;
		}
	}
	return packet;
}

void FRGarminPacketDestroy(FRGarminPacketRef packet) {
	if (packet->data) free(packet->data);
	free(packet);
}

static const char FR_ETX = 3;
static const char FR_ACK = 6;
static const char FR_DLE = 16;
static const char FR_NAK = 21;

// Basic Packet IDs
enum { 
	Pid_Protocol_Array = 253,	/* may not be implemented in all devices */ 
	Pid_Product_Rqst = 254,
	Pid_Product_Data = 255,
	Pid_Ext_Product_Data = 248	/* may not be implemented in all devices */ 
};

char nextDataByte(int fd, char* checksum) {
	char byte;
	int bytesRead = read(fd, &byte, 1);
	(void)bytesRead;
	if (byte == FR_DLE) {
		int bytesRead = read(fd, &byte, 1);
		(void)bytesRead;
	}
	*checksum += byte;
	return byte;
}

FRGarminPacketRef readSerialPacket(int fd) {
	/* !!!: Errors are not handled. */
	struct garmin_packet incomingPacket;
	(void)incomingPacket;
	
	char buffer[1];
	size_t bufferSize = sizeof(buffer);
	(void)bufferSize;
	ssize_t bytesRead;
	
	bytesRead = read(fd, buffer, 1);
	(void)bytesRead;
	if (*buffer != FR_DLE) goto error;
	
	char calculatedChecksum = 0;
	
	incomingPacket.typeID = nextDataByte(fd, &calculatedChecksum);
	
	incomingPacket.dataSize = nextDataByte(fd, &calculatedChecksum);
	
	u_int32_t totalRead = 0;
	incomingPacket.data = (char*)malloc(incomingPacket.dataSize);
	if (!incomingPacket.data) goto error;
	while (totalRead < incomingPacket.dataSize) {
		incomingPacket.data[totalRead++] = nextDataByte(fd, &calculatedChecksum);
	}
	
	/* Read the checksum byte, but we don't actually need it:
	 The checksum is the two's complement of the sum (S) of bytes up to now, so if we add it
	 to the sum of received bytes, we should end up with zero, since ~S+1 is just -S.
	 */
	(void)nextDataByte(fd, &calculatedChecksum);
	if (calculatedChecksum) goto error;
	
	bytesRead = read(fd, buffer, 1);
	(void)bytesRead;
	if (*buffer != FR_DLE) goto error;
	
	bytesRead = read(fd, buffer, 1);
	(void)bytesRead;
	if (*buffer != FR_ETX) goto error;
	
	return FRGarminPacketCreateFromData(incomingPacket.typeID, incomingPacket.data, incomingPacket.dataSize);
	
error:
	printf("Error reading serial packet!\n");
	return NULL;
}

void writeDataByte(int fd, char byte, char* checksum) {
	ssize_t bytesWritten = write(fd, &byte, 1);
	(void)bytesWritten;
	if (byte == FR_DLE) {
		ssize_t bytesWritten = write(fd, &byte, 1);
		(void)bytesWritten;
	}
	if (checksum) *checksum += byte;
}

void writeSerialPacket(int fd, FRGarminPacketRef outgoingPacket, int destroyPacket) {
	char outgoingByte;
	char checksum = 0;
	
	ssize_t bytesWritten = write(fd, &FR_DLE, 1);
	(void)bytesWritten;
	
	outgoingByte = outgoingPacket->typeID;
	writeDataByte(fd, outgoingByte, &checksum);
	
	outgoingByte = outgoingPacket->dataSize;
	writeDataByte(fd, outgoingByte, &checksum);
	
	size_t dataIdx;
	for (dataIdx = 0; dataIdx < outgoingPacket->dataSize; ++dataIdx) {
		outgoingByte = outgoingPacket->data[dataIdx];
		writeDataByte(fd, outgoingByte, &checksum);
	}
	
	checksum = -checksum; //checksum = ~checksum + 1;
	
	bytesWritten = write(fd, &checksum, 1);
	(void)bytesWritten;
	
	bytesWritten = write(fd, &FR_DLE, 1);
	(void)bytesWritten;
	
	bytesWritten = write(fd, &FR_ETX, 1);
	(void)bytesWritten;
	
	if (destroyPacket) FRGarminPacketDestroy(outgoingPacket);
}


static struct termios gOriginalOptions;

int configuredDescriptor(const char* deviceFile) {
	int fd = open(deviceFile, O_RDWR | O_NOCTTY | O_NONBLOCK);
	if (fd == -1) goto error;
	
	int err;
	// set exclusive (for non-root anyway) access to the file
	err = ioctl(fd, TIOCEXCL);
	if (err == -1) goto error;
	
	// clear the O_NONBLOCK flag so subsequent I/O will block
	err = fcntl(fd, F_SETFL, 0);
	if (err == -1) goto error;
	
	// store the original device settings
	err = tcgetattr(fd, &gOriginalOptions);
	if (err) goto error;
	
	struct termios desiredOptions = gOriginalOptions;
	/*
	 Set noncanonical mode and some timeout values.
	 See http://www.gnu.org/software/libtool/manual/libc/Noncanonical-Input.html for details.
	 */
	cfmakeraw(&desiredOptions);
	desiredOptions.c_cc[VMIN] = 1;
	desiredOptions.c_cc[VTIME] = 10;
	// set input/output speeds
	cfsetispeed(&desiredOptions, B9600);
	cfsetospeed(&desiredOptions, B9600);
	
	err = tcsetattr(fd, TCSANOW, &desiredOptions);
	if (err) goto error;
	
	return fd;
	
error:
	printf("Error!\n");
	return -1;
}

void finishDescriptor(int fd) {
	(void)tcsetattr(fd, TCSANOW, &gOriginalOptions);
	(void)close(fd);	
}


void testUsingDeviceFile(const char* deviceFile) {
	printf("Testing communications using device-file %s\n", deviceFile);
	int fd = configuredDescriptor(deviceFile);
	
	FRGarminPacketRef requestInfo = FRGarminPacketCreateFromData(Pid_Product_Rqst, NULL, 0);
	writeSerialPacket(fd, requestInfo, 1);
	
	FRGarminPacketRef ackPacket = readSerialPacket(fd);
	printf("Response packet: %s\n", ackPacket->typeID == FR_ACK ? "Acknowledged." : "NotACK!");
	FRGarminPacketDestroy(ackPacket);
	
	FRGarminPacketRef infoPacket = readSerialPacket(fd);
	printf("Received %i bytes in infoPacket\n", (int)infoPacket->dataSize);
	printf("Device ID: %s\n", infoPacket->data+4);
	FRGarminPacketDestroy(ackPacket);
	
	finishDescriptor(fd);
}
