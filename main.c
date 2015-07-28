/*
 * main.c
 *
 *  Created on: 23.12.2013
 *      Author: carsten
 */
#include <stdio.h>			// printf
#include <time.h>

#include <libavformat/avformat.h>

#include "analyse.h"
#include "decoder.h"
#include "extract.h"
#include "logger.h"
#include "visualize.h"

#define HELP_TEXT "usage: %s INPUT {-l FILE -o FILE -v}			\\
		\n   INPUT    	: \"pipe:N\" for numbered piped input	\\
		\n   l FILE   	: log to FILE							\\
		\n   o FILE   	: output to FILE						\\
		\n   v        	: visualize								\\
		\n   a {FOLDER} : analyse								\\
		", argv[0]

int main (int argc, char **argv) {
    const char *srcFilename = NULL;
    const char *logFilename = NULL;
    const char *analysisFilename = NULL;
    const char *outputFilename = NULL;
    int visualize = 0;
    int analyse = 0;

    if (argc < 2) {
    	fprintf(stderr, HELP_TEXT);
        exit(1);
    } else {
    	srcFilename = argv[1];
    	int i;
    	for (i = 2; i < argc; i++) {
    		if (!strcmp(argv[i],"-l") && (argc > (i+1))) logFilename = argv[++i];
    		else if (!strcmp(argv[i],"-o") && (argc > (i+1))) outputFilename = argv[++i];
    		else if (!strcmp(argv[i],"-a")) {
    			analyse = 1;
        		if ((argc > (i+1)) && (strcmp(argv[i+1],"-"))) analysisFilename = argv[++i];
    		}
    		else if (!strcmp(argv[i],"-v")) visualize = 1;
    	}
    }

    AVFrame *frame;

	if (setupDecoder(srcFilename, &frame, 0)) exit(1);

	if (outputFilename != NULL) {
		if (setupWriteThrough(outputFilename)) {
			fprintf(stderr, "error setting up output: %s\n", outputFilename);
			exit(1);
		}
	}

	if (visualize) {
		if (setupVisualize("motion")) exit(1);
	}

	if (setupLogger(logFilename)) {
	}

	/* timings */
	clock_t duration = clock();
	int c = 0;
	while (decodeFrame(frame) > 0) {
		c++;
		int mvAmount = 0;
		if (analyse) mvAmount = doAnalyse(frame, analysisFilename);
		if (visualize) {
			if (doVisualize(frame)) break;
		}
		doLogging(frame->coded_picture_number, mvAmount);
	}

	/* timing evaluation */
	duration = clock() - duration;
	float seconds = (float)duration / (float)CLOCKS_PER_SEC;
	float fps = c/seconds;
	printf("decoded %d frames in: %d ticks(%f seconds) = %f fps\n", c, (int)duration, seconds, fps);

	visualizeCleanUp();
	loggingCleanUp();

	return 0;
}
