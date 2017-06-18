/*
 * analyse.c
 *
 * Created on: 22.01.2014
 * Last edited: 16.06.2017
 *
 * Author: racnets
 */
#include <stdio.h>     //fopen, fprintf, printf
#include <sys/stat.h>  //mkdir
#include <string.h>    //strlen

#include <libavformat/avformat.h>

#include "analyse.h"
#include "extract.h"



int doAnalyse(AVFrame* frame, const char* file) {
	char* filename = NULL;
	if (file != NULL) {
		mkdir(file, 0777);
		filename = malloc(strlen(file) + 20);
		sprintf(filename, "%s/frame_%d.dat", file, frame->coded_picture_number);
	} else {
		mkdir("mvAnalysis", 0777);
		filename = malloc(30);
		sprintf(filename, "mvAnalysis/frame_%d.dat", frame->coded_picture_number);
	}

	FILE* fd = fopen(filename, "w");
	if (fd == NULL) {
		perror(filename);
		return EXIT_FAILURE;
	}

	int mb_width = (frame->width + 15) >> 4;
	int mb_height = (frame->height + 15) >> 4;

	int count = 0;
	int mb_x,mb_y;
	int mv_x, mv_y;
	for (mb_y=0; mb_y< mb_height; mb_y++) {
		for (mb_x=0; mb_x< mb_width; mb_x++) {
			if (getMVForMB(frame, mb_x, mb_y, &mv_x, &mv_y)) {
				count++;
				fprintf(fd, "%d\t%d\t%d\t%d\n", mb_x, mb_y, mv_x, mv_y);
			}
		}
	}

	fclose(fd);
	return count;
}
