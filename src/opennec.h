
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

#define VERSION_STRING "1.1"

#include <math.h>
#include <complex.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <stdbool.h>

/* Public API headers */
#include "types.h"
#include "misc.h"
#include "deck.h"
#include "input.h"
#include "output.h"
#include "geometry.h"
#include "control.h"
#include "fields.h"
#include "ground.h"
#include "calculations.h"
#include "matrix.h"
#include "network.h"
#include "radiation.h"
#include "somnec.h"
#include "tests.h"

/* Public constants */
#define	MAX_LINE_LEN 255
#define MAX_PATH_LEN 255
#define MAX_ERROR_LEN 255
#define MAX_UNIT_LEN 5

/* commonly used complex constants */
#define	CPLX_00	(0.0+I*0.0)
#define	CPLX_01	(0.0+I*1.0)
#define	CPLX_10	(1.0+I*0.0)
#define	CPLX_11	(1.0+I*1.0)

/* macro that returns the complex double of the arguments */
#define cmplx(r, i) ((r)+I*(i))

/* Physical constants for public use */
#define PI		3.141592654
#define	TP		6.283185308
#define	TA		1.745329252E-02 // degrees to radians
#define	TD		57.29577951     // radians to degrees
#define	CVEL	299.8           // speed of light in m/us (NEC standard)

#endif
