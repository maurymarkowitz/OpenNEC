
/*******************************************************************
 *
 * opennec.h
 *
 * opennec.h is the main header file for the OpenNEC library,
 * additional imports for input/output and similar also appear
 * in main.c and it's associated files.
 *
 * Original readme follows.
 *******************************************************************/

/*******************************************************************
Program NEC(input,tape5=input,output,tape11,tape12,tape13,tape14,
 tape15,tape16,tape20,tape21)
 
 Numerical Electromagnetics Code (NEC2)  developed at Lawrence
 Livermore lab., Livermore, CA.  (contact G. Burke at 415-422-8414
 for problems with the NEC code. For problems with the vax implem-
 entation, contact J. Breakall at 415-422-8196 or E. Domning at 415
 422-5936)
 file created 4/11/80.
 
 ***********Notice**********
 This computer code material was prepared as an account of work
 sponsored by the United States government.  Neither the United
 States nor the United States Department Of Energy, nor any of
 their employees, nor any of their contractors, subcontractors,
 or their employees, makes any warranty, express or implied, or
 assumes any legal liability or responsibility for the accuracy,
 completeness or usefulness of any information, apparatus, product
 or process disclosed, or represents that its use would not infringe
 privately-owned rights.
 
 *******************************************************************/

#ifndef	OPENNEC_H
#define	OPENNEC_H 1

#define VERSION_STRING "2.0"

#include <math.h>
#include <complex.h>    // MS's impementation of C99's complex is broken
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <ctype.h>      // has isspace, perhaps could be macroed out
#include <unistd.h>     // this has getopt, but MS doesn't support it
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/times.h>  // and MS doesn't support this either

//#define complex _complex

//#ifdef _COMPLEX_DEFINED
//#endif

#include "types.h"

void free_deck(deck_t *deck);
#include "proto.h"

#ifndef	TRUE
#define	TRUE	1
#endif

#ifndef	FALSE
#define	FALSE	0
#endif

/* commonly used complex constants */
#define	CPLX_00	(0.0+I*0.0)
#define	CPLX_01	(0.0+I*1.0)
#define	CPLX_10	(1.0+I*0.0)
#define	CPLX_11	(1.0+I*1.0)

/* macro that returns the complex double of the arguments */
#define cmplx(r, i) ((r)+I*(i))

/* common constants */
#define PI		3.141592654
#define	POT		1.570796327
#define	TP		6.283185308
#define	PTP		.6283185308
#define	TPJ		(0.0+I*6.283185308)
#define PI8		25.13274123
#define PI10	31.41592654
#define FPI     12.56637062
#define	TA		1.745329252E-02 // degrees to radians
#define	TD		57.29577951     // radians to degrees
#define	ETA		376.73
#define	CVEL	299.8
#define	RETA	2.654420938E-3
#define	TOSP	1.128379167
#define ACCS	1.E-12
#define	SP		1.772453851
#define	CCJ		(0.0-I*0.01666666667)
#define	CONST1	(0.0+I*4.771341189)
#define	CONST2	4.771341188
#define	CONST3	(0.0-I*29.97922085)
#define	CONST4	(0.0+I*188.365)
#define	GAMMA	.5772156649
#define C1		-.02457850915
#define C2		.3674669052
#define C3		.7978845608
#define P10		.0703125
#define P20		.1121520996
#define Q10		.125
#define Q20		.0732421875
#define P11		.1171875
#define P21		.1441955566
#define Q11		.375
#define Q21		.1025390625
#define POF		.7853981635
#define MAXH	20
#define CRIT	1.0E-4
#define NM		131072
#define NTS		4
#define	SMIN	1.e-3

/* Replaces the "10000" limit used to */
/* identify segment/patch connections */
#define	PCHCON  100000

/* carriage return and line feed */
#define	CR	0x0d
#define	LF	0x0a

/* max length of a line read from input file */
#define	MAX_LINE_LEN 255
/* max length of a path/filename */
#define MAX_PATH_LEN 255
/* max length of a single error message */
#define MAX_ERROR_LEN 255
/* max length of a unit code in a geometry card */
#define MAX_UNIT_LEN 5

#endif
