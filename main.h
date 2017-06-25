/*
 * main.h
 *
 * Last modified: 18.06.2017
 *
 * Author: racnets 
 */

#ifndef MAIN_H_
#define MAIN_H_

#include <stdio.h>		//fprintf, printf

int is_verbose(void);

/* print only if verbose option is set */
#define verbose_printf(fmt, ...) \
	do { if (is_verbose()) fprintf(stderr, "%s:%d:%s(): " fmt "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__); } while (0)

/* print more if verbose option is set */
#define info_printf(fmt, ...) \
	do { if (is_verbose()) verbose_printf(fmt, ##__VA_ARGS__); else printf(fmt "\n", ##__VA_ARGS__); } while (0)


/* print only if verbose AND DEBUG_VERSION are set */
#ifndef DEBUG_VERBOSE
#define DEBUG_VERBOSE 0
#endif /* DEBUG_VERBOSE */
#define debug_printf(fmt, ...) \
	do { if (DEBUG_VERBOSE) verbose_printf(fmt, ##__VA_ARGS__); } while (0)


#endif /* MAIN_H_ */
