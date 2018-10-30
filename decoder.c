/*
 * decoder.c
 *
 * Created on: 22.12.2013
 * Last edited: 16.06.2017
 *
 * Author: racnets
 */

#include <stdio.h>

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>

#include "main.h"

AVFormatContext *pFormatCtx;
AVCodecContext *videoDecCtx;
AVCodec *videoCodec;
AVPacket packet = {0};
FILE *of = NULL;

int videoStream;

int setupDecoder(const char *file, AVFrame **frame, int verbose, int experimental) {

	pFormatCtx = NULL;
	videoDecCtx = NULL;
	videoCodec = NULL;

	/***************************************************
	 * open and demuxing
	 */
	/* register all formats and codecs */
	av_register_all();

	/* Open video file */
	if (avformat_open_input(&pFormatCtx, file, NULL, NULL) != 0) {
		fprintf(stderr, "Could not open source file %s\n", file);
		return EXIT_FAILURE;
	}

	if (verbose) {
		/* Retrieve stream information */
   		if (avformat_find_stream_info(pFormatCtx, NULL) >= 0) {
   			/* dump stream information */
   			av_dump_format(pFormatCtx, 0, file, 0);
   		}
	}

	/* select first video stream */
	videoStream = -1;
	int i;
	for (i=0; i<pFormatCtx->nb_streams; i++) {
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			videoStream = i;
			break;
		}
	}

	if (videoStream == -1) {
		fprintf(stderr, "Could not find any video stream\n");
		return EXIT_FAILURE;
	}

	/***************************************************
	 * codec context
	 */
	/* Get a pointer to the codec context for the video stream */
	videoDecCtx = pFormatCtx->streams[videoStream]->codec;
	/* Find the decoder for the video stream */
	videoCodec = avcodec_find_decoder(videoDecCtx->codec_id);
	if (videoCodec == NULL) {
		fprintf(stderr, "Could not find decoder\n");
		return EXIT_FAILURE;
	}

	/* Inform the codec that we can handle truncated bitstreams -- i.e.,
	 * bitstreams where frame boundaries can fall in the middle of packets
	 */
	if (videoCodec->capabilities & CODEC_CAP_TRUNCATED)
		videoDecCtx->flags |= CODEC_FLAG_TRUNCATED;

	/* Open codec */
	if (avcodec_open2(videoDecCtx, videoCodec, NULL) < 0) {
		fprintf(stderr, "Could not open decoder\n");
		return EXIT_FAILURE;
	} else {
		verbose_printf("found decoder: %s\n", videoCodec->name);
	}

	/* allocate AVFrame structure */
	*frame = avcodec_alloc_frame();

	/* setup debug */
//    av_log_set_level(AV_LOG_DEBUG);
//	videoDecCtx->debug |= FF_DEBUG_PICT_INFO;
//	videoDecCtx->debug |= FF_DEBUG_MB_TYPE;
//	videoDecCtx->debug |= FF_DEBUG_MV;
//	videoDecCtx->debug |= FF_DEBUG_STARTCODE;

	if (experimental) {
		videoDecCtx->experimental_mode = 0;
		videoDecCtx->experimental_mode |= DECODE_MV_ONLY;
		videoDecCtx->experimental_mode |= DISABLE_DEBLOCKING_FILTER;
	}

	/* setup packet */
	av_init_packet(&packet);

	return EXIT_SUCCESS;
}

int decodeFrame(AVFrame *frame)
{
	// get next frame
	if (av_read_frame(pFormatCtx, &packet) >= 0) {		
		// decode frame
		int got_picture;
		if (packet.stream_index == videoStream) {
			avcodec_decode_video2(videoDecCtx, frame, &got_picture, &packet);
		}
		
		if (got_picture) {	
			if (of != NULL) {
				/* write output file */
				fwrite(packet.data, 1, packet.size, of);
			}
			return EXIT_FAILURE;
		} else {
			fprintf(stderr, "error decoding frame\n");			
			if (frame->format == AV_PIX_FMT_NONE ) {
				fprintf(stderr, "unknown pixel format\n");			
			}
			if (frame->mb_type == NULL) {
				fprintf(stderr, "missing motion informations\n");			
			}
		}
	} 
	
	/* clean up */
	av_free_packet(&packet);
	avcodec_close(videoDecCtx);
	avformat_close_input(&pFormatCtx);
	if (of != NULL) fclose(of);
	return EXIT_SUCCESS;
}

int setupWriteThrough(const char* filename) {
	of = fopen(filename, "wb");

	if (of != NULL) return EXIT_SUCCESS;
	else return EXIT_FAILURE;
}
