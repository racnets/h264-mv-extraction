/*
 * extract.h
 *
 *  Created on: 22.12.2013
 *      Author: carsten
 */

#ifndef EXTRACT_H_
#define EXTRACT_H_

char getMBTypeForMB(AVFrame* frame, int x, int y);
int getMVForMB(AVFrame* frame, int mb_x, int mb_y, int* mv_x, int* mv_y);

#endif /* EXTRACT_H_ */
