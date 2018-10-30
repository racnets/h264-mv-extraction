/*
 * analyse.h
 *
 *  Created on: 22.01.2014
 *
 *  Author: racnets
 */

#ifndef ANALYSE_H_
#define ANALYSE_H_

int doAnalyse(AVFrame *frame, const char* dir, int analyse, int *mvCount, int *mv0, int *mvn0, double *mvX, double *mvY);

#endif /* ANALYSE_H_ */
