/*
 * main.c
 *
 * Created on:  23.12.2013
 * Last edited: 16.06.2017
 *
 * Author: racnets
 */

#include <stdio.h>   // printf
#include <getopt.h>  // getoptlong
#include <time.h>

#include <libavformat/avformat.h>

#include "analyse.h"
#include "decoder.h"
#include "extract.h"
#include "logger.h"
#include "visualize.h"

const char *srcFilename = NULL;
const char *logFilename = NULL;
const char *analysisFilename = NULL;
const char *outputFilename = NULL;
int visualize = 0;
int analyse = 0;
int verbose = 0;

static void print_usage(const char *prog)
{
	printf("Usage: %s -i INPUT [-alovV]\n", prog);
	puts(" general\n"
	     "  -i IN   --input IN       \"pipe:N\" for numbered piped input\n"
	     "  -o OUT  --output OUT     output to pass the input through\n"
	     "  -f FILE --file FILE      log file to write to\n"
	     "  -a[DIR] --analyse[=DIR]  analyse the incoming data (optional: set directory to write data for each frame)\n"
	     "  -V      --visualize      visualize the analysed data\n"
	     "  -v      --verbose        be verbose\n"
	     "  -w      --werbose        be wery verbose\n");
	exit(1);
}

static void parse_opts(int argc, char *argv[])
{
	//forbid error message
	opterr=0;

	while (1) {
		static const struct option lopts[] = {
			{ "input",     1, 0, 'i' },
			{ "output",    1, 0, 'o' },
			{ "file",      1, 0, 'f' },
			{ "help",      0, 0, 'h' },
			{ "analyse",   2, 0, 'a' },
			{ "visualize", 0, 0, 'V' },
			{ "verbose",   0, 0, 'v' },
			{ "werbose",   0, 0, 'w' },
			{ NULL, 0, 0, 0 },
		};
		int c;

		c = getopt_long(argc, argv, "i:o:f:a::hVvw", lopts, NULL);

		if (c == -1)
			break;

		switch (c) {
			case 'i':
				srcFilename = optarg;
				break;
			case 'o':
				logFilename = optarg;
				break;
			case 'a':
				analyse = 1;
				if (optarg) analysisFilename = optarg;
				break;
			case 'V':
				visualize = 1;
				break;
			case 'w':
				verbose = 2;
				break;
			case 'v':
				verbose = 1;
				break;
			case 'f':
				logFilename = optarg;
				break;
			case 'h':
				print_usage(argv[0]);
				break;
			default:;
		}
	}
}

int main(int argc, char *argv[]) 
{

	printf("\nh264-stream motion vector extraction tool\n");

	if (argc < 2) {
		print_usage(argv[0]);
		return EXIT_FAILURE;
	}

	parse_opts(argc, argv);
	
	if (srcFilename == NULL) {
		printf("no input defined\n");
		return EXIT_FAILURE;
	}

	AVFrame *frame;

	if (setupDecoder(srcFilename, &frame, verbose)) {
		printf("error setting up decoder for input: %s\n", srcFilename);
		return EXIT_FAILURE;
	}

	if (outputFilename != NULL) {
		if (setupWriteThrough(outputFilename)) {
			printf("error setting up output: %s\n", outputFilename);
			return EXIT_FAILURE;
		}
	}

	if (visualize) {
		if (setupVisualize("motion")) {
			printf("error setting up visualisation\n");
			return EXIT_FAILURE;
		}
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

	return EXIT_SUCCESS;
}
