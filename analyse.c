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
#include "main.h"

#define DEFTOSTRING(x) #x

typedef struct {
	int16_t x_val;
	int16_t y_val;
	int frequency;
} mv_freq_type;

typedef struct mv_freq_list {
	mv_freq_type *val;
	struct mv_freq_list *prev;
	struct mv_freq_list *next;
} mv_freq_list_type;

struct mv_freq_list_type *curr = NULL;

mv_freq_list_type* createElement(mv_freq_list_type *prev, int16_t x_val, int16_t y_val) {
	debug_printf("called with *prev: %#x\tx_val: %d\ty_val: %d", (int)prev, x_val, y_val);

	mv_freq_list_type *element = NULL;
	element = malloc(sizeof(mv_freq_list_type));
	element->val = NULL;
	element->val = malloc(sizeof(mv_freq_type));
	element->val->frequency = 1;
	element->val->x_val = x_val;
	element->val->y_val = y_val;
	element->prev = prev;
	if (prev != NULL) element->prev->next = element;
	element->next = NULL;
	
	return element;
}

mv_freq_list_type* switchListElement(mv_freq_list_type *element) {
	debug_printf("called with *element: %#x", (int)element);

	if (element->prev == NULL) return element;
		
	mv_freq_type *_p = element->prev->val;
	mv_freq_type *_c = element->val;
	
	element->prev->val = _c;
	element->val = _p;
	
	return element->prev;
}

mv_freq_list_type* updateList(mv_freq_list_type* head, int16_t x_val, int16_t y_val) {
	debug_printf("called with *head: %#x\tx_val: %d\ty_val: %d", (int)head, x_val, y_val);

	mv_freq_list_type *curr = head;
	mv_freq_list_type *prev = head;
	
	while (curr != NULL) {
		/* search for entry with same motion values */
		if((curr->val->x_val == x_val) && (curr->val->y_val == y_val)) {
			debug_printf("entry still exists - update frequency %d", curr->val->frequency);
			curr->val->frequency++;
			
			/* re-order entries - sort after frequency */
			while ((curr != NULL) && (curr->prev != NULL) && (curr->prev->val->frequency < curr->val->frequency)) {
				/* switch elements */
				curr = switchListElement(curr);
			}
			break;
		} else {
			/* continue search */
			prev = curr;
			curr = curr->next;
		}
	}
	
	if (curr == NULL) {
		/* create new element */
		curr = createElement(prev, x_val, y_val);
	};

	/* jump to previous head to search for head */
	if (head != NULL) curr = head;
	while (curr->prev != NULL) {
		/* prepare to return to head of list */
		curr = curr->prev;
	}
	return curr;
}

void printAndFreeList(mv_freq_list_type* element, FILE *fd) {
	debug_printf("called");
	
	if (element == NULL) return;

	while (element->prev != NULL) {
		element = element->prev;
	}
	while (element != NULL) {
		mv_freq_list_type* _e = element;
		/* print to file */
		fprintf(fd, "%d\t%d\t%d\n", element->val->frequency, element->val->x_val, element->val->y_val);
		element = element->next;
		
		/* free */
		free(_e->val);
		free(_e);
	}
}

int evaluateList(mv_freq_list_type* element, int *mv0, int *mvn0, double *mvX, double *mvY) {
	debug_printf("called");
	
	if (element == NULL) return EXIT_FAILURE;

	while (element->prev != NULL) {
		element = element->prev;
	}
	
	int _mv0 = 0;
	int _mvn0 = 0;
	double _mvX = 0;
	double _mvY = 0;
	while (element != NULL) {
		if((element->val->x_val == 0) && (element->val->y_val == 0)) _mv0 = element->val->frequency;
		else {
			_mvn0 += element->val->frequency;
			_mvX += element->val->frequency * element->val->x_val;
			_mvY += element->val->frequency * element->val->y_val;
		}
		element = element->next;
	}
	
	*mv0 = _mv0;
	*mvn0 = _mvn0;
	*mvX = _mvX;
	*mvY = _mvY;
	
	return EXIT_SUCCESS;
}

int doAnalyse(AVFrame *frame, const char* dir, int *mvCount, int *mv0, int *mvn0, double *mvX, double *mvY) {
	debug_printf("called");
	
	char* filename = NULL;
	char* filename_mv_freq = NULL;

	if (dir != NULL) {
		mkdir(dir, 0777);
		filename = malloc(strlen(dir) + sizeof("/frame_.dat") + strlen(DEFTOSTRING(INT_MAX)));
		sprintf(filename, "%s/frame_%d.dat", dir, frame->coded_picture_number);
		filename_mv_freq = malloc(strlen(dir) + sizeof("/frame-mv-freq_.dat") + strlen(DEFTOSTRING(INT_MAX)));
		sprintf(filename_mv_freq, "%s/frame-mv-freq_%d.dat", dir, frame->coded_picture_number);
	} else {
		mkdir("mvAnalysis", 0777);
		filename = malloc(sizeof("mvAnalysis/frame_.dat") + strlen(DEFTOSTRING(INT_MAX)));
		sprintf(filename, "mvAnalysis/frame_%d.dat", frame->coded_picture_number);
		filename_mv_freq = malloc(sizeof("mvAnalysis/frame-mv-freq_.dat") + strlen(DEFTOSTRING(INT_MAX)));
		sprintf(filename_mv_freq, "mvAnalysis/frame-mv-freq_%d.dat", frame->coded_picture_number);
	}

	FILE* fd = fopen(filename, "w");
	if (fd == NULL) {
		perror(filename);
		return EXIT_FAILURE;
	}

	FILE* fd_mvf = fopen(filename_mv_freq, "w");
	if (fd_mvf == NULL) {
		perror(filename_mv_freq);
		return EXIT_FAILURE;
	}
	
	
	int _mv0 = 0;
	int _mvn0 = 0;
	double _mvX = 0;
	double _mvY = 0;
	int count = 0;

	mv_freq_list_type *head = NULL;
	int mb_width = (frame->width + 15) >> 4;
	int mb_height = (frame->height + 15) >> 4;
	int mb_x,mb_y;
	int mv_x, mv_y;
	for (mb_y=0; mb_y< mb_height; mb_y++) {
		for (mb_x=0; mb_x< mb_width; mb_x++) {
			if (getMVForMB(frame, mb_x, mb_y, &mv_x, &mv_y)) {
				count++;
				fprintf(fd, "%d\t%d\t%d\t%d\n", mb_x, mb_y, mv_x, mv_y);
				head = updateList(head, mv_x, mv_y);
			}
		}
	}

	evaluateList(head, &_mv0, &_mvn0, &_mvX, &_mvY);
	printAndFreeList(head, fd_mvf);
	
	fclose(fd);
	fclose(fd_mvf);

	if (mvCount != NULL) *mvCount = count;
	if (mv0 != NULL) *mv0 = _mv0;
	if (mvn0 != NULL) *mvn0 = _mvn0;
	if (mvX != NULL) *mvX = _mvX;
	if (mvY != NULL) *mvY = _mvY;

	return EXIT_SUCCESS;
}
