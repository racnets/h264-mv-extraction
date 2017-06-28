/*
 * main.c
 *
 * Created on:  23.12.2013
 * Last edited: 25.06.2017
 *
 * Author: racnets
 */

#include <stdio.h>   // printf
#include <getopt.h>  // getoptlong
#include <unistd.h>  // usleep
#include <time.h>

#include <libavformat/avformat.h>

#include "analyse.h"
#include "decoder.h"
#include "extract.h"
#include "logger.h"
#ifdef GTK_GUI
#include "gtk-viewer.h"
#endif //GTK_GUI

const char *srcFilename = NULL;
const char *logFilename = NULL;
const char *analysisFilename = NULL;
const char *outputFilename = NULL;
int visualize = 0;
int analyse = 0;
int verbose = 0;

int is_verbose() {
	return verbose;
}

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

	if (visualize) {
#ifdef GTK_GUI
		/* set up GUI */
		if (viewer_init(&argc, &argv) == EXIT_FAILURE) {
			printf("error setting up visualisation\n");
			return EXIT_FAILURE;
		}
#else
		printf("no GUI supported for current host OS configuration\n");
		visualize = 0;
#endif //GTK_GUI
	}

	AVFrame *frame;

	if (setupDecoder(srcFilename, &frame, verbose, visualize)) {
		printf("error setting up decoder for input: %s\n", srcFilename);
		return EXIT_FAILURE;
	}

	if (outputFilename != NULL) {
		if (setupWriteThrough(outputFilename)) {
			printf("error setting up output: %s\n", outputFilename);
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
		int mvAmountZero = 0;
		int mvAmountNotZero = 0;
		double mvSumX = 0;
		double mvSumY = 0;
		if (analyse || visualize) doAnalyse(frame, analysisFilename, &mvAmount, &mvAmountZero, &mvAmountNotZero, &mvSumX, &mvSumY);
#ifdef GTK_GUI
		if (visualize) {
			while (viewer_is_paused()) {
				if (viewer_update() == EXIT_FAILURE) break;
				//~ nanosleep((const struct timespec[]){{0, 500000000L}}, NULL);
			}
			if (viewer_set_avFrame(frame, mvAmount, (float)mvSumX/mvAmountNotZero, (float)mvSumY/mvAmountNotZero) == EXIT_FAILURE) break;
			if (viewer_update() == EXIT_FAILURE) break;
		}
#endif //GTK_GUI
		doLogging(frame->coded_picture_number, mvAmount, mvAmountZero, mvAmountNotZero, mvSumX, mvSumY);
	}

	/* timing evaluation */
	duration = clock() - duration;
	float seconds = (float)duration / (float)CLOCKS_PER_SEC;
	float fps = c/seconds;
	printf("decoded %d frames in: %d ticks(%f seconds) = %f fps\n", c, (int)duration, seconds, fps);

#ifdef GTK_GUI
	//visualizeCleanUp();
#endif //GTK_GUI
	loggingCleanUp();

	return EXIT_SUCCESS;
}
