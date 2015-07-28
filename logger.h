/*
 * logger.h
 *
 *  Created on: 03.01.2014
 *      Author: carsten
 */

#ifndef LOGGER_H_
#define LOGGER_H_

#define SLAVE_ADDRESS	0x18
#define I2C_DEVICE		"/dev/i2c-0"

int setupLogger(const char* filename);
void doLogging(int id, int mvCount);
void loggingCleanUp(void);

#endif /* LOGGER_H_ */
