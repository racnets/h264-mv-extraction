/*
 * i2c.c
 *
 *  Created on: 03.01.2014
 *      Author: carsten
 */

#include <unistd.h>			// read, write
#include <linux/i2c-dev.h>	// I2C_SLAVE
#include <fcntl.h>			//open
#include <sys/ioctl.h>		// ioctl
#include <stdint.h>

int i2c = 0;
uint8_t buf[10];

int i2cInit(const char* dev, int address) {
	i2c = open(dev, O_RDWR);
	if (i2c < 0) return 1;

	if (ioctl(i2c, I2C_SLAVE, address)) return 1;
	return 0;
}

uint16_t i2cReadW(uint8_t address) {
	if (i2c) {
		buf[0] = address;
		write(i2c, buf, 1);
		read(i2c, buf, 2);

		return (uint16_t) ((buf[1] << 8) | buf[0]);
	} else return -1;
}

uint32_t i2cReadL(uint8_t address) {
	if (i2c) {
		buf[0] = address;
		write(i2c, buf, 1);
		read(i2c, buf, 4);

		return (uint32_t) ((buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0] );
	} else return -1;
}

uint8_t i2cReadB(uint8_t address) {
	if (i2c) {
		buf[0] = address;
		write(i2c, buf, 1);
		read(i2c, buf, 1);

		return buf[0];
	} else return -1;
}
