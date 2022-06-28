/*******************************************************************
 *
 * shared.h
 *
 * shared.h contains forwards for shared externs used during
 * calculations. needs to be replaced by a single structure that
 * has all these parts in it so there's no globals
 *
 *******************************************************************/

#ifndef SHARED_H
#define SHARED_H	1

#include "opennec.h"

/*------------------------------------------------------------------------*/

/* common  /geometry/ */
extern geometry_t geometry;

/* common  /crnt/ */
extern crnt_t crnt;

/* common  /dataj/ */
extern dataj_t dataj;

/* pointers to input/output files */
extern FILE *input_fp, *output_fp, *error_fp, *green_fp, *plot_fp;

/* common  /fpat/ */
extern fpat_t fpat;

/*common  /ggrid/ */
extern ggrid_t ggrid;

/* common  /gnd/ */
extern gnd_t gnd;

/* common  /gwav/ */
extern gwav_t gwav;

/* common  /incom/ */
extern incom_t incom;

/* common  /matpar/ */
extern matpar_t matpar;

/* common  /netcx/ */
extern netcx_t netcx;

/* common  /plot/ */
extern plot_t plot;

/* common  /save/ */
extern save_t save;

/* common  /segj/ */
extern segj_t segj;

/* common  /smat/ */
extern smat_t smat;

/* common  /tmi/ */
extern tmi_t tmi;

/* common  /vsorc/ */
extern vsorc_t vsorc;

/* common  /yparm/ */
extern yparm_t yparm;

/* common  /zload/ */
extern zload_t zload;

/*------------------------------------------------------------------------*/

#endif
