#include <stdio.h>
#include <stdlib.h>
#include <time.h>

//#include <opencv/cv.h>
//#include <opencv/highgui.h>

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>

#include "decoder.h"


/*
 * extract motion vector
 */
int extractMV(AVCodecContext *codecCtx, int stream, AVFrame *frame) {
	const int mb_width = (codecCtx->width + 15) >> 4;
	const int mb_height = (codecCtx->height + 15) >> 4;

    /*
     * log2 of the size of the block which a single vector in motion_val represents:
     * (4->16x16, 3->8x8, 2-> 4x4, 1-> 2x2)
     * usually 4x4 in h264 codec */
    const int mv_sample_log2 = 4 - frame->motion_subsample_log2;
    const int mv_stride = mb_width << mv_sample_log2;
    const int mv_width = mb_width << mv_sample_log2;
	const int mv_height = mb_height << mv_sample_log2;

	int gmv_x = 0;
	int gmv_y = 0;
	static int pic = 0;

    /* check if motion information is present */
    if (frame->motion_val == NULL) return;

    /* check if frame is a (forward!) predicted frame */
	if (frame->pict_type != AV_PICTURE_TYPE_P ) return;

    /* for every sub-macroblock */
    int x, y;
    for (y = 0; y < mv_height; ++y) {
    	for (x = 0; x < mv_width; ++x) {
    		/* calculate sub-macroblock index */
        	int mv_index = x + y * mv_stride;
        	int mv_x = frame->motion_val[0][mv_index][0];
        	int mv_y = frame->motion_val[0][mv_index][1];
//        	printf("%d/%d\t", mv_x, mv_y);
        	gmv_x += mv_x;
        	gmv_y += mv_y;
    	}
//    	printf("\n");
    }
    gmv_x = gmv_x / mv_height / mv_width;
    gmv_y = gmv_y / mv_height / mv_width;
//    printf("%d: %d/%d\n", ++pic, gmv_x, gmv_y);
    return gmv_x && gmv_y;
}

/*
 * compute global motion vector
 */
int computeGMV(AVCodecContext *codecCtx, AVFrame *frame) {
	const int mb_width = (codecCtx->width + 15) >> 4;
	const int mb_height = (codecCtx->height + 15) >> 4;
	const int mb_stride = mb_width + 1;

   /*
    * log2 of the size of the block which a single vector in motion_val represents:
    * (4->16x16, 3->8x8, 2-> 4x4, 1-> 2x2)
    * usually 4x4 in h264 codec */
   const int mv_sample_log2 = 4 - frame->motion_subsample_log2;
   const int mv_stride = mb_width << mv_sample_log2;
   const int mv_width = mb_width << mv_sample_log2;
   const int mv_height = mb_height << mv_sample_log2;

   int gmv_x = 0;
   int gmv_y = 0;
   static int pic = 0;

   /* check if motion information is present */
   if (frame->motion_val == NULL) return 0;

   /* check if frame is a (forward!) predicted frame */
   if (frame->pict_type != AV_PICTURE_TYPE_P ) return 0;

   /* for each macroblock */
   int mb_x, mb_y;
   for (mb_y = 0; mb_y < mb_height; ++mb_y) {
	   for (mb_x = 0; mb_x < mb_width; ++mb_x) {
		   /* calculate macroblock index */
		   int mb_index = mb_x + mb_y * mb_stride;

		   /* not uses list0 or list1
		    * -> only P type mb's containing valid motion information
		    * which are: P16x16, P16x8, P8x16, P8x8, P_Skip
		    */
		   if (!(frame->mb_type[mb_index] & MB_TYPE_L0L1)) continue;

		   if (frame->mb_type[mb_index] & MB_TYPE_16x16) {
			   /* calculate sub-macroblock index */
			   int smb_x = mb_x << mv_sample_log2;
			   int smb_y = mb_y << mv_sample_log2;
			   int smb_index = smb_x + smb_y * mv_stride;

			   /* no sub-macroblocks - 2x2 times the same sub-mb
			    * and each sub-mb contains 2^mv_sample_log2 mv blocks */
			   gmv_x += (4 * frame->motion_val[0][smb_index][0]) << mv_sample_log2;
			   gmv_y += (4 * frame->motion_val[0][smb_index][1]) << mv_sample_log2;
		   } else if (frame->mb_type[mb_index] & MB_TYPE_16x8) {
			   /* calculate sub-macroblock index */
			   int smb_x = mb_x << mv_sample_log2;
			   int smb_y = mb_y << mv_sample_log2;
			   int smb_index = smb_x + smb_y * mv_stride;

			   /* 2 sub-macroblocks - so for each 2 times the same sub-mb
			    * and each sub-mb contains 2^mv_sample_log2 mv blocks */
			   /* 1st sub-mb */
			   gmv_x += (2 * frame->motion_val[0][smb_index][0]) << mv_sample_log2;
			   gmv_y += (2 * frame->motion_val[0][smb_index][1]) << mv_sample_log2;

			   /* 2nd sub-mb */
			   smb_index = smb_x + (mv_sample_log2 + smb_y) * mv_stride;
			   gmv_x += (2 * frame->motion_val[0][smb_index][0]) << mv_sample_log2;
			   gmv_y += (2 * frame->motion_val[0][smb_index][1]) << mv_sample_log2;
		   } else if (frame->mb_type[mb_index] & MB_TYPE_8x16) {
			   /* calculate sub-macroblock index */
			   int smb_x = mb_x << mv_sample_log2;
			   int smb_y = mb_y << mv_sample_log2;
			   int smb_index = smb_x + smb_y * mv_stride;

			   /* 2 sub-macroblocks - so for each 2 times the same sub-mb
			    * and each sub-mb contains 2^mv_sample_log2 mv blocks */
			   /* 1st sub-mb */
			   gmv_x += (2 * frame->motion_val[0][smb_index][0]) << mv_sample_log2;
			   gmv_y += (2 * frame->motion_val[0][smb_index][1]) << mv_sample_log2;

			   /* 2nd sub-mb */
			   smb_index = mv_sample_log2 + smb_x + smb_y * mv_stride;
			   gmv_x += (2 * frame->motion_val[0][smb_index][0]) << mv_sample_log2;
			   gmv_y += (2 * frame->motion_val[0][smb_index][1]) << mv_sample_log2;
//		   } else if (frame->mb_type[mb_index] & MB_TYPE_8x8) {
		   } else {
			   /* calculate sub-macroblock index */
			   int smb_x = mb_x << mv_sample_log2;
			   int smb_y = mb_y << mv_sample_log2;
			   int smb_index = smb_x + smb_y * mv_stride;

			   /* 4 sub-macroblocks
			    * and each sub-mb contains 2^mv_sample_log2 mv blocks */
			   /* 1st sub-mb */
			   gmv_x += (frame->motion_val[0][smb_index][0]) << mv_sample_log2;
			   gmv_y += (frame->motion_val[0][smb_index][1]) << mv_sample_log2;

			   /* 2nd sub-mb */
			   smb_index = mv_sample_log2 + smb_x + smb_y * mv_stride;
			   gmv_x += (frame->motion_val[0][smb_index][0]) << mv_sample_log2;
			   gmv_y += (frame->motion_val[0][smb_index][1]) << mv_sample_log2;

			   /* 3rd sub-mb */
			   smb_index = mv_sample_log2 + smb_x + (smb_y + mv_sample_log2) * mv_stride;
			   gmv_x += (frame->motion_val[0][smb_index][0]) << mv_sample_log2;
			   gmv_y += (frame->motion_val[0][smb_index][1]) << mv_sample_log2;

			   /* 4th sub-mb */
			   smb_index = smb_x + (smb_y + mv_sample_log2) * mv_stride;
			   gmv_x += (frame->motion_val[0][smb_index][0]) << mv_sample_log2;
			   gmv_y += (frame->motion_val[0][smb_index][1]) << mv_sample_log2;
		   }
	   }
   }
   gmv_x = gmv_x / mv_height / mv_width;
   gmv_y = gmv_y / mv_height / mv_width;
//   printf("gmx: %d/%d\n", gmv_x, gmv_y);
   return gmv_x & gmv_y;
}

void compareFrame(AVCodecContext *codecCtx0, AVCodecContext *codecCtx1, int stream, AVFrame *frame0, AVFrame *frame1) {
	int mb_height;
	int mb_width;
	const int mv_sample_log2 = 2;
	const int shift          = 2;

	if (codecCtx0->height == codecCtx1->height && \
		codecCtx0->width == codecCtx1->width && \
		codecCtx0->codec->capabilities == codecCtx1->codec->capabilities ) {
		mb_height 	 = (codecCtx0->height + 15) / 16;
		mb_width       = (codecCtx0->width + 15) / 16;
	} else {
		fprintf(stderr, "AVCodecContext difference detected in height, width or codec capabilities\n");
		return;
	}
	const int mb_stride = mb_width + 1;
	const int mv_stride = (mb_width << mv_sample_log2);
    const int mf_stride = 2*((codecCtx0->height + 7) / 8);



    /* check if motion information is present */
    if (frame0->motion_val == NULL && frame1->motion_val == NULL) return;
    if ((frame0->motion_val == NULL && frame1->motion_val != NULL) || \
    	(frame0->motion_val != NULL && frame1->motion_val == NULL)) {
		fprintf(stderr, "AVCodecContext difference detected: only one contains motion information\n");
    	return;
    }

    /* check if frame is a (forward!) predicted frame */
	if (frame0->pict_type != AV_PICTURE_TYPE_P && frame0->pict_type != AV_PICTURE_TYPE_P) return;
    if (frame0->pict_type != frame1->pict_type){
		fprintf(stderr, "AVCodecContext difference detected: picture type differs\n");
    	return;
    }

    /* for every macroblock */
    int mb_x, mb_y;
    for (mb_y = 0; mb_y < mb_height; ++mb_y) {
    	for (mb_x = 0; mb_x < mb_width; ++mb_x) {
    		/* calculate macroblock index */
        	int mb_index = mb_x + mb_y * mb_stride;
        	/* calculate motion field index */
        	int mf_index = 2 * 2 * mb_x + 2 * mb_y * mf_stride;

        	if (frame0->mb_type[mb_index] != frame1->mb_type[mb_index]) {
        		fprintf(stderr, "AVCodecContext difference detected: mb_type @index %d differs\n",mb_index);
            	return;
        	}
//        	printf(" %x: ", frame->mb_type[mb_index]);
//        	if (frame->mb_type[mb_index] & MB_TYPE_16x16) printf("16x16;");
//        	if (frame->mb_type[mb_index] & MB_TYPE_16x8) printf("16x8;");
//        	if (frame->mb_type[mb_index] & MB_TYPE_8x16) printf("8x16;");
//        	if (frame->mb_type[mb_index] & MB_TYPE_8x8) printf("8x8;");
//        	if (frame->mb_type[mb_index] & MB_TYPE_L0) printf("L0;");
//        	if (frame->mb_type[mb_index] & MB_TYPE_L1) printf("L1;");
//        	printf(")");

//        	/* not uses list0 or list1 */
//        	if (!(frame->mb_type[mb_index] & MB_TYPE_L0L1)) {
////        		printf("\n");
//        		continue;
//        	}
//
//        	int i;
//        	if (frame->mb_type[mb_index] & MB_TYPE_8x8) {
////            	printf("8x8\t");
//        		for (i = 0; i < 4; ++i) {
//                    int xy = (mb_x * 2 + (i & 1) + (mb_y * 2 + (i >> 1)) * mv_stride) << (mv_sample_log2 - 1);
//                    int mx = (frame->motion_val[0][xy][0] >> shift);
//                    int my = (frame->motion_val[0][xy][1] >> shift);
//                    int mf_xy = mf_index + 2 * (i & 1) + (i >> 1) * mf_stride;
//                    mf[mf_xy] = mx;
//                    mf[mf_xy + 1] = my;
////                    printf("%d/%d", mx, my);
//        		}
//        	}
//        	else if (frame->mb_type[mb_index] & MB_TYPE_16x8) {
////            	printf("16x8\t");
//        		for (i = 0; i < 2; ++i) {
//                    int xy = (mb_x * 2 + (mb_y * 2 + i) * mv_stride) << (mv_sample_log2 - 1);
//                    int mx = (frame->motion_val[0][xy][0] >> shift);
//                    int my = (frame->motion_val[0][xy][1] >> shift);
//                    int mf_xy = mf_index + i * mf_stride;
//                    mf[mf_xy] = mx;
//                    mf[mf_xy + 1] = my;
//                    mf[mf_xy + 2] = mx;
//                    mf[mf_xy + 3] = my;
////                    printf("%d/%d", mx, my);
//        		}
//        	}
//        	else if (frame->mb_type[mb_index] & MB_TYPE_8x16) {
////            	printf("8x16\t");
//        		for (i = 0; i < 2; ++i) {
//                    int xy = (mb_x * 2 + i + (mb_y * 2) * mv_stride) << (mv_sample_log2 - 1);
//                    int mx = (frame->motion_val[0][xy][0] >> shift);
//                    int my = (frame->motion_val[0][xy][1] >> shift);
//                    int mf_xy = mf_index + 2 * i;
//                    mf[mf_xy] = mx;
//                    mf[mf_xy + 1] = my;
//                    mf[mf_xy + mf_stride] = mx;
//                    mf[mf_xy + mf_stride + 1] = my;
////                    printf("%d/%d", mx, my);
//        		}
//        	}
//        	else if (frame->mb_type[mb_index] & MB_TYPE_16x16) {
////            	printf("16x16\t");
//                int xy = (mb_x * 2 + (mb_y * 2) * mv_stride) << (mv_sample_log2 - 1);
//                int mx = (frame->motion_val[0][xy][0] >> shift);
//                int my = (frame->motion_val[0][xy][1] >> shift);
//                int mf_xy = mf_index;
//                mf[mf_xy] = mx;
//                mf[mf_xy + 1] = my;
//                mf[mf_xy + 2] = mx;
//                mf[mf_xy + 3] = my;
//                mf[mf_xy + mf_stride] = mx;
//                mf[mf_xy + mf_stride + 1] = my;
//                mf[mf_xy + mf_stride + 2] = mx;
//                mf[mf_xy + mf_stride + 3] = my;
////                printf("%d/%d", mx, my);
//        	}
////        	printf("\n");
        }
    }
}


int h264test (int argc, char **argv) {
    const char *src_filename = NULL;

    if (argc != 2) {
        fprintf(stderr, "usage: %s input_file\n"
                "\n", argv[0]);
        exit(1);
    }

    src_filename = argv[1];
    AVFrame *frame;

//    cvNamedWindow("H.264", CV_WINDOW_AUTOSIZE);
again:
	{
		if (setupDecoder(src_filename, &frame, 0)) exit(1);

		/* allocate motion field */
//		int x_res = (videoDecCtx->width + 7) / 8;
//		int y_res = (videoDecCtx->height + 7) / 8;
//		motionField = malloc(x_res * y_res * 2 * sizeof(int));

		/* timings */
		clock_t duration = clock();
		int c = 0;
		int d = 0;

		/* read one frame each */
//		while (av_read_frame(pFormatCtx, &packet) >= 0) {
		while (decodeFrame(frame) > 0) {
			c++;
//			extractMVFieldFromFrame(frame);
		}

		duration = clock() - duration;
		float seconds = (float)duration / (float)CLOCKS_PER_SEC;
		float fps = c/seconds;
		printf("decoded %d frames in: %d ticks(%f seconds) = %f fps\n", c, (int)duration, seconds, fps);
		printf("%d contained motion information\n", d);

		static int repeat = 0;
		static float fps_sum = 0;
		fps_sum += fps;
		if (++repeat < 1) goto again;

		printf("average fps: %f\n", fps_sum/repeat);
	}

//	cvDestroyAllWindows();

    return EXIT_SUCCESS;
}
