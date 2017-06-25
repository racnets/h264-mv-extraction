/*
 * gtk-viewer.h
 * 
 * Last modified: 18.06.2017
 *
 * Author: racnets 
 */

#ifndef GTK_VIEWER_H
#define GTK_VIEWER_H

int viewer_set_avFrame(AVFrame *frame, int mvs, double motX, double motY);
int viewer_update(void);
int viewer_is_paused(void);
int viewer_init(int *argc, char **argv[]);
	
#endif /* GTK_VIEWER_H */
