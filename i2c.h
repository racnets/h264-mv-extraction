/*
 * i2c.h
 *
 *  Created on: 03.01.2014
 *      Author: carsten
 */

#ifndef I2C_H_
#define I2C_H_
#include <stdint.h>

int i2cInit(const char* dev, int address);
uint32_t i2cReadL(uint8_t address);
uint16_t i2cReadW(uint8_t address);
uint8_t i2cReadB(uint8_t address);

#endif /* I2C_H_ */
