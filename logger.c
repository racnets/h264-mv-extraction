/*
 * logger.c
 *
 *  Created on: 03.01.2014
 *      Author: carsten
 */
#include <stdio.h>			//fopen, fprintf, printf
#include <sys/time.h>

#include "logger.h"
#include "i2c.h"

FILE* fd = NULL;
double t0;

double getTime() {
	struct timeval tp;
	gettimeofday( &tp, NULL );
	return tp.tv_sec + tp.tv_usec/1E6;
}

int setupLogger(const char* filename) {
	i2cInit(I2C_DEVICE, SLAVE_ADDRESS);

	t0 = getTime();

	fd = fopen(filename, "w");
	if (fd != NULL) return 0;
	else return 1;
}

void doLogging(int id, int mvCount) {
	double t = getTime();

	if (fd != NULL) {
		fprintf(fd, "%d\t%u\t%u\t%u\t%u\t%u\t%f\t%d\n", id, i2cReadW(0x32), i2cReadW(0x76), i2cReadW(0x78), i2cReadW(0x72), i2cReadW(0x74), t - t0, mvCount);
	} else {
		printf("%d\t%u\t%u\t%u\t%u\t%u\t%f\t%d\n", id, i2cReadW(0x32), i2cReadW(0x76), i2cReadW(0x78), i2cReadW(0x72), i2cReadW(0x74), t - t0, mvCount);
	}
}

void loggingCleanUp(void) {
	if (fd != NULL) fclose(fd);
}
