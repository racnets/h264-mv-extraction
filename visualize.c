#include <libavformat/avformat.h>
#include "visualize.h"

#ifdef WITH_CV
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include "extract.h"

CvMat* img;

void setPix(CvMat* img, int x, int y, int color) {
	if (img->type & CV_8UC3) {
		int i = y*img->step + x*3;
		img->data.ptr[i] = (color & 0x000000FF);
		img->data.ptr[i+1] = ((color & 0x0000FF00) >> 8);
		img->data.ptr[i+2] = ((color & 0x00FF0000) >> 16);
	} else { // CV_8UC1:
		int i = y*img->step + x;
		img->data.ptr[i] = (color & 0x000000FF);
	}
}

void drawMbType(CvMat* img, AVFrame *frame) {
	int mb_width = (frame->width + 15) >> 4;
	int mb_height = (frame->height + 15) >> 4;

	int x,y;
	double color;
	for (y=0; y< mb_height; y++) {
		for (x=0; x< mb_width; x++) {
			switch (getMBTypeForMB(frame, x, y)) {
				case 'i':
					color = 0xF0F0F0;
					break;
				case 'I':
					color = 0xE0E0E0;
					break;
				case 'p':
				case '-':
				case '|':
				case '+':
					color = 0x00FF00;
					break;
				case 's':
					color = 0x00AA00;
					break;
				default: color = 0xFFFF00;
			}
			cvRectangle(img, cvPoint(x << 4, y << 4), cvPoint((x << 4) + 15, (y << 4) + 15), cvColorToScalar(color, img->type), CV_FILLED, 8, 0);
		}
	}
}

void drawMv(CvMat* img, AVFrame *frame) {
	int mb_width = (frame->width + 15) >> 4;
	int mb_height = (frame->height + 15) >> 4;

	int x,y,mb_x,mb_y;
	int mv_x, mv_y;
	double color = 0xFFFFFF;
	for (mb_y=0; mb_y< mb_height; mb_y++) {
		for (mb_x=0; mb_x< mb_width; mb_x++) {
			if (getMVForMB(frame, mb_x, mb_y, &mv_x, &mv_y)) {
				x = (mb_x << 4) + 7;
				y = (mb_y << 4) + 7;
				cvLine(img, cvPoint(x, y), cvPoint(x + mv_x, y + mv_y), cvColorToScalar(color, img->type), 1, CV_AA, 0);
				cvCircle(img, cvPoint(x, y), 2, cvColorToScalar(color, img->type), 1, 8, 0);
			}

		}
	}
}

int setupVisualize(const char *name) {
	cvNamedWindow(name, CV_WINDOW_NORMAL | CV_WINDOW_KEEPRATIO);
	img = NULL;

	return 0;
}

int doVisualize(AVFrame* frame) {
	if (img != NULL) cvReleaseMat(&img);
	img = cvCreateMat(frame->height, frame->width, CV_8UC3);

	drawMbType(img, frame);
	drawMv(img, frame);

	cvShowImage("motion",img);
	if ((char)cvWaitKey(0) == 27) {
		printf("abort\n");
		return 1;
	}
	return 0;
}

void visualizeCleanUp(void) {
	cvDestroyAllWindows();
}

#else

int setupVisualize(const char *name) {
	return 0;
}

int doVisualize(AVFrame *frame) {
	return 0;
}

void visualizeCleanUp(void) {}

#endif
