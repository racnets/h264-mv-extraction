/*
 * decoder.h
 *
 *  Created on: 22.12.2013
 *
 *  Author: racnets
 */

#ifndef DECODER_H_
#define DECODER_H_

int setupDecoder(const char *file, AVFrame **frame, int verbose, int visualize);
int decodeFrame(AVFrame *frame);
int setupWriteThrough(const char* filename);

#endif /* DECODER_H_ */
