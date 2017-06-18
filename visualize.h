/*
 * visualize.h
 *
 *  Created on: 03.01.2014
 *  
 *  Author: racnets
 */

#ifndef VISUALIZE_H_
#define VISUALIZE_H_

int setupVisualize(const char *name);
int doVisualize(AVFrame *frame);
void visualizeCleanUp(void);

#endif /* VISUALIZE_H_ */
