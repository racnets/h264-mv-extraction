/*
 * extract.c
 *
 *  Created on: 22.12.2013
 *      Author: carsten
 */
#include <stdio.h>

#include <libavcodec/avcodec.h>

int mbIndex(int mb_x, int mb_y);

int mb_width;
int mb_height;
int mb_stride;
int smb_sample_log2;
int smb_stride;
int smb_width;
int smb_height;

void setup(AVFrame* frame) {
	static int index = -1;

	if (frame->coded_picture_number != index) {
		/* get codec context from frame */
		AVCodecContext *codecCtx = frame->owner;
		/* get frame size in macroblocks */
		mb_width = (codecCtx->width + 15) >> 4;
		mb_height = (codecCtx->height + 15) >> 4;
		mb_stride = mb_width + 1;

		/*
		 * log2 of the size of the block which a single vector in motion_val represents:
		 * (4->16x16, 3->8x8, 2-> 4x4, 1-> 2x2)
		 * usually 4x4 in h264 codec */
		smb_sample_log2 = 4 - frame->motion_subsample_log2;
		smb_stride = mb_width << smb_sample_log2;
		smb_width = mb_width << smb_sample_log2;
		smb_height = mb_height << smb_sample_log2;

		index = frame->coded_picture_number;
	}
}

inline int mbIndex(int mb_x, int mb_y) {
	return mb_x + mb_y * mb_stride;
}

char getMBTypeName(int type) {
	if (type & MB_TYPE_INTRA_PCM) return 'd';
	else if (type & MB_TYPE_INTRA4x4) return 'i';
	else if (type & MB_TYPE_INTRA16x16) return 'I';
	else if (type & MB_TYPE_SKIP) return 's';
	else if (type & MB_TYPE_16x16) return 'p';
	else if (type & MB_TYPE_16x8) return '-';
	else if (type & MB_TYPE_8x16) return '|';
	else if (type & MB_TYPE_8x8) return '+';
	else return 'o';
}

char getMBTypeForMB(AVFrame* frame, int x, int y) {
	setup(frame);

	return getMBTypeName(frame->mb_type[mbIndex(x, y)]);
}

char getMBTypeForPixel(AVFrame* frame, int x, int y) {
	setup(frame);

	int mb_x = x >> 4;
	int mb_y = y >> 4;

	return getMBTypeName(frame->mb_type[mbIndex(mb_x, mb_y)]);
}

/*
 * returns >0 if mv is valid (P-type macroblocks) otherwise 0.
 * return value equals number of summands for mb mv.
 */
int getMVForMB(AVFrame* frame, int mb_x, int mb_y, int* mv_x, int* mv_y) {
	setup(frame);

	int mb_type = frame->mb_type[mbIndex(mb_x, mb_y)];

	if (mb_type & (MB_TYPE_L0L1 | MB_TYPE_SKIP)) {
		// P-type
	    int smb_index = (mb_x + mb_y * smb_stride) << smb_sample_log2;
	    int smb_per_half_mb = (1 << smb_sample_log2) / 2;

	    if (mb_type & MB_TYPE_16x16) {
		    *mv_x = frame->motion_val[0][smb_index][0];
			*mv_y = frame->motion_val[0][smb_index][1];
			return 1;
		} else if (mb_type & MB_TYPE_16x8) {
			int _mv_x = frame->motion_val[0][smb_index][0];
			int _mv_y = frame->motion_val[0][smb_index][1];
			/* calculate sub-macroblock index */
        	smb_index = smb_index + (smb_stride * smb_per_half_mb);
			_mv_x += frame->motion_val[0][smb_index][0];
			_mv_y += frame->motion_val[0][smb_index][1];
			*mv_x = _mv_x / 2;
			*mv_y = _mv_y / 2;
			return 2;
		} else if (mb_type & MB_TYPE_8x16) {
			int _mv_x = frame->motion_val[0][smb_index][0];
			int _mv_y = frame->motion_val[0][smb_index][1];
			/* calculate sub-macroblock index */
        	smb_index = smb_index + smb_per_half_mb;
			_mv_x += frame->motion_val[0][smb_index][0];
			_mv_y += frame->motion_val[0][smb_index][1];
			*mv_x = _mv_x / 2;
			*mv_y = _mv_y / 2;
			return 2;
		} else if (mb_type & MB_TYPE_8x8) {
			int _mv_x = frame->motion_val[0][smb_index][0];
			int _mv_y = frame->motion_val[0][smb_index][1];
			/* calculate sub-macroblock index */
        	int _smb_index = smb_index + smb_per_half_mb;
			_mv_x += frame->motion_val[0][_smb_index][0];
			_mv_y += frame->motion_val[0][_smb_index][1];
        	_smb_index = smb_index + (smb_stride * smb_per_half_mb);
			_mv_x += frame->motion_val[0][_smb_index][0];
			_mv_y += frame->motion_val[0][_smb_index][1];
        	_smb_index = smb_index + smb_per_half_mb + (smb_stride * smb_per_half_mb);
			_mv_x += frame->motion_val[0][_smb_index][0];
			_mv_y += frame->motion_val[0][_smb_index][1];
			*mv_x = _mv_x / 4;
			*mv_y = _mv_y / 4;
			return 4;
		}
	}

	return 0;
}
