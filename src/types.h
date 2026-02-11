/******************************************************************************
 *
 * types.h
 *
 * types.h defines the many data structures that are used to pass
 * data around the calculation system. In nec2c these were defined
 * in nec2c.h, and have been moved here for clarity. OpenNEC also
 * adds new types for the deck_t and card_t, so they can be passed
 * instead of using globals. Other new types include Error and
 * Errors, key_value_t, and various definitions of measurements and such.
 *
 *****************************************************************************/

#pragma once

#ifndef TYPES_H
#define TYPES_H

#include <complex.h>
#include <stdio.h>
#include <stdbool.h>  // we will use the bool type!
#include <time.h>     // for clock_t and timing

// NEC has 4 int fields
#ifndef MAX_INT_FIELDS_DEF
#define MAX_INT_FIELDS_DEF
#define MAX_INT_FIELDS 4
#endif

// NEC has 7 float fields
#ifndef MAX_FLT_FIELDS_DEF
#define MAX_FLT_FIELDS_DEF
#define MAX_FLT_FIELDS 7
#endif

// OpenNEC generally allows commas or any whitespace between fields
#ifndef ONEC_WHITESPACE_DEF
#define ONEC_WHITESPACE_DEF
#define ONEC_WHITESPACE ", \t\n\r\v\f\0"
#endif

// these are the markers for *inline* comments
// does not include #, which is used by nec2c, but that can only
// appear at the start of the line, not inline, because of AWG measurements
#ifndef ONEC_COMMENTS_DEF
#define ONEC_COMMENTS_DEF
#define ONEC_COMMENTS "!'"
#endif

// these are the separators within an OpenNEC extension list
#ifndef ONEC_SEPARATORS_DEF
#define ONEC_SEPARATORS_DEF
#define ONEC_SEPARATORS ";,"
#endif

// these are the delimeters between the keys and values
#ifndef ONEC_DELIMETERS_DEF
#define ONEC_DELIMETERS_DEF
#define ONEC_DELIMETERS "=:"
#endif

/* card field names, like "I1" of "F4" */
#ifndef FIELD_NAMES_DEF
#define FIELD_NAMES_DEF
#define NUM_FIELD_NAMES 11
extern char *field_names[NUM_FIELD_NAMES];
#endif

/* input card mnemonic list */
/* "XT" stands for "exit", added for testing, not included in these lists */
#ifndef COMMENT_CODES_DEF
#define COMMENT_CODES_DEF
#define NUM_COMMENT_CODES  5
extern char *comment_codes[NUM_COMMENT_CODES];
#endif

#ifndef CONTROL_CODES_DEF
#define CONTROL_CODES_DEF
#define NUM_CONTROL_CODES  20
extern char *control_codes[NUM_CONTROL_CODES];
#endif

#ifndef GEOMETRY_CODES_DEF
#define GEOMETRY_CODES_DEF
#define NUM_GEOMETRY_CODES  13
extern char *geometry_codes[NUM_GEOMETRY_CODES];
#endif

#ifndef ONEC_CODES_DEF
#define ONEC_CODES_DEF
#define NUM_ONEC_CODES 4
extern char *onec_codes[NUM_ONEC_CODES];
#endif

/* tinyexpr variable names for field bindings */
#ifndef ONEC_FIELD_VAR_NAMES_DEF
#define ONEC_FIELD_VAR_NAMES_DEF
/*
 * The first entry is an empty string to align with 1-based indexing
 * used throughout the codebase for NEC fields (F1..F7, I1..I4).
 * Index 0 is intentionally unused.
 */
extern const char *fnames[MAX_FLT_FIELDS + 1];
extern const char *inames[MAX_INT_FIELDS + 1];
#endif

/*** Structs encapsulating global ("common") variables */

/*** Error levels are used internally, external software should use negatives ***/
typedef enum { NONE, WARNING, PROBLEM, FATAL } error_level;    // 1 = warning, 2 = error, 3 = fatal, <0 informational

/*** error_t has information about a single error or warning */
typedef struct
{
  int severity;
  char *message;  // the error string
} error_t;

/*** errors_list_t is a list generated during a particular stage, typically
there will be different error lists for import, sanity checks, running
and export
 ***/
typedef struct
{
  int num_errors; // total number of errors in this list
  error_t *errors;  // pointer to a list of errors
} errors_list_t;

/*** outputs_list_t is a list of informational messages generated during
processing, to be output later by output.c instead of direct fprintf
 ***/
typedef struct
{
  int num_messages; // total number of messages in this list
  char **messages;  // pointer to a list of message strings
} outputs_list_t;

/*** key_value_t is a key:value pair used to store an OpenNEC extension on a card */
typedef struct key_value_t
{
	unsigned int magic;
	char *key;
	char *value;
	double fv; // new field for storing a float value
	char separator; // what separator was used, a colon or an equals?
	struct key_value_t* next;
} key_value_t;

/*** card_t encapsulates a single card ***/
typedef struct card_t
{
  // used to track whether this card has been edited since being read
  bool edited;
  
  // raw data from the original card
  char *orig_str;     // the original line, as read from the file in raw format
  char *card_str;     // the "card part" of the string, everything in front of the inline comment (if one exists)
  
  // processed NEC2 data
  char card_code[3];  // the two-letter code for this card, or one letter for some comment formats
  
  // NEC uses i1 through i1 and f1 through f7. We'll put these in an
  // array to ease access when we're looping: f[i]. This could lead
  // to confusion because normally C would be zero-indexed, like f[0].
  // To avoid this we'll make the array one larger than it has to be
  // and just leave the zeroth entry empty.
  int i[5];           // i1 is normally the tag, etc.
  double f[8];        // geometery and so forth
  
  // the values above are the raw inputs, they may include units and/or
  // formulas that need to be calculated. these arrays hold the final
  // values that will be fed into the calculation engines. v for "value"
  int iv[5];
  double fv[8];

  // different cards have different numbers of inputs, so these are
  // used to track how many we actually read in
  int ints_used;      // the number of int parameters
  int flts_used;      // ...and floats
  
  // tags and segments are normally printed as they are calculated,
  // but onec only does that once, so we'll store them here so we
  // can print them out later
  int tag;
  int num_segments;
  int start_segment;
  int end_segment;
  bool int_form_inline[4];// was this formula found inline, or in a comment?
  bool flt_form_inline[8];
  
  // onec extensions
  char extn_code[1];  // the one-letter code that marked the extension or inline comment, if any
  char *extn_str;     // the entire inline comment, anything after the comment marker
  char *comment;      // if a comment was found, it's placed here, this is *not* the same
                      //    as extn_str, it might be found in a 'comment:' key/value pair
  key_value_t *extensns; // pairs of name:value key/value entries, this will **not** include a comment if there was one
  key_value_t *formulas; // pairs of name:value key/value entries for any formulas, inline or in the extension area

  // onec flags - only this one needs to be known during processing
  bool ignore;        // cards can be marked to be deliberately ignored
} card_t;

/*** deck_t encapsulates a single deck of cards ***/
typedef struct deck_t
{
  // input data
  card_t *cards;        // array of cards
  int num_cards;      // total number of cards read in, including any trailing lines
  int comment_start;  // card number of the start of the comments section, normally 0. -1 if there are no CM or CE cards
  int comment_end;    // card number of the last continuous CM card, or the CE card if present. -1 if there are no CM or CE cards
  int geometry_start; // card number of the first geometry card, which definitely should exist. -1 if not found
  int geometry_end;   // card number of the GE card, which also has to exist. -1 if not found
  int deck_end;       // card number of the EN card or the last card in the deck otherwise. -1 if not found
  char cmt_code;      // the default marker to use for inline comments, !, $ or '
  int unit_val;       // if there is a single GS, this is the f1 value, otherwise 1
  int unit_typ;       // if there is a single GS, and we recognize the value, put our index here
  key_value_t **symbols;  // array of pointers to key_value_t nodes in the cards (not owned)
  int num_symbols;        // number of symbols in the array
} deck_t;

/* common  /crnt/ */
typedef struct
{
  double
    *air,	/* Ai/lambda, real part */
    *aii,	/* Ai/lambda, imaginary part */
    *bir,	/* Bi/lambda, real part */
    *bii,	/* Bi/lambda, imaginary part */
    *cir,	/* Ci/lambda, real part */
    *cii;	/* Ci/lambda, imaginary part */

  complex double *cur; /* Amplitude of basis function */
} crnt_t;

/** common  /geometry/ (geometry data)
 * Holds segment and patch data for the entire geometry of the deck.
 * A given deck will have only one geometry_t object at a given time.
 * This is populated by parsing the geometry section of the deck, and
 * can be used in external programs to build 3D models and similar
 * tasks. It only has to be recalculated if the geometry changes,
 * although that might also occur as part of an optimization loop.
 *
 * This is similar to the original NEC version, but has added another
 * array to store the card numbers. This allows a particular segment
 * to be tracked back to the card that created it, even if it does
 * not have a tag number. This was added so that GUIs can determine
 * how to display a particular segment by looking for onec extensions
 * on the card.
 *
 */
typedef struct geometry_t
{
	int
		n,		      	// Number of wire segments in total
		np,		      	// Number of wire segments in symmetry cell
		m,		      	// Number of surface patches
		mp,		      	// Number of surface patches in symmetry cell
		npm,	      	// = n+m
  // FIXME: these two are used only during mallocs and don't seem to be needed
		np2m,	      	// = n+2m
		np3m,	      	// = n+3m
		ipsym,	    	// Symmetry flag
		*icon1,     	// Segments connections on end 1
		*icon2,	    	// Segments connections on end 2
		*tag_nums,		// Segment's tag number, which may be zero
    	*card_nums; 	// which card number generated this bit, never zero
  
	double
    	// Wire segment data
    	*x1, *y1, *z1,	// End 1 coordinates of wire segments
		*x2, *y2, *z2,	// End 2 coordinates of wire segments
		*x, *y, *z,		// Coordinates of segment centers
		*si, *bi,		// Length and radius of segments
		*cab,			// cos(a)*cos(b)
		*sab,			// cos(a)*sin(b)
		*salp,			// Z component - sin(a)

    	// Surface patch data
		*px, *py, *pz,		// Coordinates of patch center
		*t1x, *t1y, *t1z,	// Coordinates of t1 vector
		*t2x, *t2y, *t2z,	// Coordinates of t2 vector
		*pbi,				// Patch surface area
		*psalp,				// Z component - sin(a)

    /* Wavelength in meters */
    wlam;
  
  // list of errors added while processing this geometry
  errors_list_t errors;
} geometry_t;

/* common  /dataj/ */
typedef struct
{
	int
		iexk,
		ind1,
		indd1,
		ind2,
		indd2,
		ipgnd;

	double
		s,
		b,
		xj,
		yj,
		zj,
		cabj,
		sabj,
		salpj,
		rkh,
		t1xj,
		t1yj,
		t1zj,
		t2xj,
		t2yj,
		t2zj;

	complex double
		exk,
		eyk,
		ezk,
		exs,
		eys,
		ezs,
		exc,
		eyc,
		ezc;

} dataj_t;

/* common  /fpat/ */
typedef struct
{
	int
		near,
		nfeh,
		nrx,
		nry,
		nrz,
		nth,
		nph,
		ipd,
		iavp,
		inor,
		iax,
		ixtyp;

	double
		thets,
		phis,
		dth,
		dph,
		rfld,
		gnor,
		clt,
		cht,
		epsr2,
		sig2,
		xpr6,
		pinr,
		pnlr,
		ploss,
		xnr,
		ynr,
		znr,
		dxnr,
		dynr,
		dznr;

} fpat_t;

/* Radiation pattern data point */
typedef struct
{
	double theta;
	double phi;
	double gnmj;      /* major axis gain (dB) */
	double gnmn;      /* minor axis gain (dB) */
	double gnv;       /* vertical gain (dB) */
	double gnh;       /* horizontal gain (dB) */
	double gtot;      /* total gain (dB) */
	double axrat;     /* axial ratio */
	double tilta;     /* tilt angle */
	int    pol_sense; /* 0=LINEAR, 1=RIGHT, 2=LEFT */
	double ethm;      /* E-theta magnitude */
	double etha;      /* E-theta phase */
	double ephm;      /* E-phi magnitude */
	double epha;      /* E-phi phase */
	double erdm;      /* E-radial magnitude (near field only) */
	double erda;      /* E-radial phase (near field only) */
} rpat_point_t;

/* Radiation pattern results */
typedef struct
{
	int num_points;
	rpat_point_t *points;
	double gmax;      /* maximum gain for normalization */
	double pint;      /* average power */
	double solid_angle; /* solid angle used in averaging */
	double exrm;      /* exp(-jkr)/r magnitude */
	double exra;      /* exp(-jkr)/r phase */
	char ground_cliff_type[20]; /* "LINEAR" or "CIRCULAR" for cliff */
} rpat_results_t;

/*common  /ggrid/ */
typedef struct
{
	int
		nxa[3],
		nya[3];

	double
		dxa[3],
		dya[3],
		xsa[3],
		ysa[3];

	complex double
		epscf,
		*ar1,
		*ar2,
		*ar3;

} ggrid_t;

/* common  /gnd/ */
typedef struct
{
	int
		ksymp,	/* Ground flag */
		ifar,	  /* Int flag in RP card, for far field calculations */
		iperf,	/* Type of ground flag */
		nradl;	/* Number of radials in ground screen */

	double
		t2,		  /* Const for radial wire ground impedance */
		cl,		  /* Distance in wavelengths of cliff edge from origin */
		ch,		  /* Cliff height in wavelengths */
		scrwl,	/* Wire length in radial ground screen normalized to w/length */
		scrwr;	/* Radius of wires in screen in wavelengths */

	complex double
		zrati,	/* Ground medium [Er-js/wE0]^-1/2 */
		zrati2,	/* As above for 2nd ground medium */
		t1,		  /* Const for radial wire ground impedance */
		frati;	/* (k1^2-k2^2)/(k1^2+k2^2), k1=w(E0Mu0)^1/2, k1=k2/ZRATI */

} gnd_t;

/* common  /gwav/ */
typedef struct
{
	double
		r1,		  /* Distance from current element to point where field is evaluated  */
		r2,		  /* Distance from image of element to point where field is evaluated */
		zmh,	  /* Z-Z', Z is height of field evaluation point */
		zph;	  /* Z+Z', Z' is height of current element */

	complex double
		u,		  /* (Er-jS/WE0)^-1/2 */
		u2,		  /* u^2 */
		xx1,	  /* G1*exp(jkR1.r[i])  */
		xx2;	  /* G2*exp(jkR2.r'[i]) */

} gwav_t;

/* common  /incom/ */
typedef struct
{
	int isnor;

	double
		xo,
		yo,
		zo,
		sn,
		xsn,
		ysn;

} incom_t;

/* common  /matpar/ (matrix parameters) */
typedef struct
{
	int
		icase,	/* Storage mode of primary matrix */
		npblk,	/* Num of blocks in first (NBLOKS-1) blocks */
		nlast,	/* Num of blocks in last block */
		imat;	  /* Storage reserved in CM for primary NGF matrix A */

} matpar_t;

/* common  /netcx/ */
typedef struct
{
	int
		masym,	/* Matrix symmetry flags */
		neq,
		npeq,
		neq2,
		nonet,	/* Number of two-port networks */
		ntsol,	/* "Network equations are solved" flag */
		nprint,	/* Print control flag */
		*iseg1,	/* Num of seg to which port 1 of network is connected */
		*iseg2,	/* Num of seg to which port 2 of network is connected */
		*ntyp;	/* Type of networks */

	double
		*x11r,	/* Real and imaginary parts of network impedances */
		*x11i,
		*x12r,
		*x12i,
		*x22r,
		*x22i,
		pin,	  /* Total input power from sources */
		pnls,	  /* Power lost in networks */
		asmx,     /* Maximum relative asymmetry */
		asa,      /* RMS relative asymmetry */
		mat_fill_time,   /* Time to fill interaction matrix (msec) */
		mat_factor_time; /* Time to factor interaction matrix (msec) */

	int
		nteq_asym, /* Segment number for max asymmetry (first) */
		ntsc_asym, /* Segment number for max asymmetry (second) */
		nexc;      /* Number of network excitation points stored */

	int *exc_tag;           /* Tag numbers for excitation points */
	int *exc_seg;           /* Segment numbers for excitation points */
	complex double *exc_v;  /* Voltage at excitation points */
	complex double *exc_i;  /* Current at excitation points */
	complex double *exc_z;  /* Impedance at excitation points */
	complex double *exc_y;  /* Admittance at excitation points */
	double *exc_pwr;        /* Power at excitation points */

	int ninp;               /* Number of antenna input points stored */
	int *inp_tag;           /* Tag numbers for antenna input points */
	int *inp_seg;           /* Segment numbers for antenna input points */
	complex double *inp_v;  /* Voltage at input points */
	complex double *inp_i;  /* Current at input points */
	complex double *inp_z;  /* Impedance at input points */
	complex double *inp_y;  /* Admittance at input points */
	double *inp_pwr;        /* Power at input points */

	complex double zped;

} netcx_t;

/* common  /plot/ */
typedef struct
{
	int
		/* Plot control flags */
		iplp1,
		iplp2,
		iplp3,
		iplp4;

} plot_t;

/* common  /save/ */
typedef struct
{
	int *ip;	/* Vector of indices of pivot elements used to factor matrix */

	int
		nfrq,	  /* Number of frequency steps */
		ifrq;	  /* Frequency stepping type (0=linear, 1=multiplicative) */

	double
		epsr,	  /* Relative dielectric constant of ground */
		sig,	  /* Conductivity of ground */
		scrwlt,	/* Length of radials in ground screen approximation */
		scrwrt,	/* Radius of wires in ground screen approximation */
		fmhz,	  /* Frequency in MHz */
		delfrq;	/* Frequency step size */

} save_t;

/* common  /segj/ */
typedef struct
{
	int
		*jco,	  /* Stores connection data */
		jsno,	  /* Total number of entries in ax, bx, cx */
		maxcon; /* Max. no. connections */

	double
		*ax, *bx, *cx;	/* Store constants A, B, C used in current expansion */

} segj_t;

/* common  /smat/ */
typedef struct
{
	int nop; /* My addition */

	complex double *ssx;

} smat_t;

/* common  /tmi/ */
typedef struct
{
	int ij;

	double
		zpk,
		rkb2;

} tmi_t;

/* common /evlcom/ (formerly static globals in somnec.c) */
typedef struct
{
	int jh;
	double ck2, ck2sq, tkmag, tsmag, ck1r, zph, rho;
	complex double ct1, ct2, ct3, ck1, ck1sq, cksm;
} evlcom_t;

/* common /cntour/ (formerly static globals in somnec.c) */
typedef struct
{
	complex double a, b;
} cntour_t;

/* Internal state for somnec functions (formerly static local variables) */
typedef struct
{
	int init;
	int m[101];
	double a1[25], a2[25], a3[25], a4[25], psi, tst, zms;
} som_bh_t;

typedef struct
{
	double del, slope, rmis;
	complex double cp1, cp2, cp3, bk, delta, delta2, sum[6], ans[6];
} som_ev_t;

typedef struct
{
	double rbk, amg, den, denm;
} som_gs_t;

typedef struct
{
	double z, ze, s, ep, zend, dz, dzot, tr, ti;
	complex double t00, t11, t02;
	complex double g1[6], g2[6], g3[6], g4[6], g5[6], t01[6], t10[6], t20[6];
} som_ro_t;

typedef struct
{
	evlcom_t evlcom;
	cntour_t cntour;
	som_bh_t bessel;
	som_bh_t hankel;
	som_ev_t evlua;
	som_gs_t gshank;
	som_ro_t rom1;
} somnec_t;

/* Interpolation state (formerly static globals in calculations.c:intrp) */
typedef struct
{
	int ix, iy, ixs, iys, igrs, ixeg, iyeg;
	int nxm2, nym2, nxms, nyms, nd, ndp;
	double dx, dy, xs, ys, xz, yz;
	complex double a[4][4], b[4][4], c[4][4], d[4][4];
} intrp_t;

/*common  /tmh/ */
typedef struct
{
	double
		zpka,
		rhks;

} tmh_t;

/* common  /vsorc/ */
typedef struct
{
	int
		*isant,	/* Num of segs on which an aplied field source is located */
		*ivqd,	/* Num of segs on which a current-slope discontinuity source is located */
		*iqds,	/* Same as above (?) */
		nsant,	/* Number of applied field voltage sources */
		nvqd,	  /* Number of applied current-slope discontinuity sources */
		nqds;	  /* Same as above (?) */

	complex double
		*vqd,	  /* Voltage of applied-current slope discontinuity sources */
		*vqds,	/* Same as above (?) */
		*vsant;	/* Voltages of applied field voltage sources */

} vsorc_t;

/* common  /yparm/ */
typedef struct
{
	int
		ncoup,	/* Num of segs between which coupling will be computed */
		icoup,	/* Num of segs in the coupling array that have been excited */
		*nctag,	/* Tag number of segments */
		*ncseg;	/* Num of segs in set of segs that have same tag number */

	complex double
		*y11a,	/* Self admittance of segments */
		*y12a;	/* Mutual admittances stored in order 1,2 1,3 2,3 2,4 etc */
  
} yparm_t;

/* common  /zload/ */
typedef struct
{
	int nload;	/* Number of loading networks */
	
	int
		*ldtyp,	/* Type of loading (0=series RLC, 1=parallel RLC, etc.) */
		*ldtag,	/* Tag number for loading */
		*ldtagf,	/* Segment start for loading */
		*ldtagt;	/* Segment end for loading */
	
	double
		*zlr,	/* Loading resistance or impedance (real) */
		*zli,	/* Loading reactance or impedance (imaginary) */
		*zlc;	/* Loading capacitance */
	
	complex double *zarray;	/* = Zi/(Di/lambda) */
} zload_t;

/* Structure for storing loading output data */
typedef struct loading_output_t {
    int tag;           /* Tag number */
    int tagf;          /* Segment start */
    int tagt;          /* Segment end */
    double conductivity; /* Conductivity value */
    char type[20];     /* Loading type description */
} loading_output_t;

typedef struct loading_outputs_t {
    int count;
    int capacity;
    loading_output_t *entries;
} loading_outputs_t;

/* nec_context_t structure containing all context variables */
typedef struct nec_context_t
{
	geometry_t geometry;
	geometry_t ignored_geometry;
	crnt_t crnt;
	dataj_t dataj;
	FILE *input_fp;
	FILE *output_fp;
	FILE *error_fp;
	FILE *green_fp;
	FILE *plot_fp;
	fpat_t fpat;
	ggrid_t ggrid;
	gnd_t gnd;
	gwav_t gwav;
	incom_t incom;
	matpar_t matpar;
	netcx_t netcx;
	plot_t plot;
	save_t save;
	segj_t segj;
	smat_t smat;
	tmi_t tmi;
	vsorc_t vsorc;
	yparm_t yparm;
	zload_t zload;
	loading_outputs_t loading_outputs;
	
	/* Thread-safety state (formerly static globals) */
	somnec_t somnec;
	intrp_t intrp;
	tmh_t tmh;
	
	/* Radiation pattern results */
	rpat_results_t rpat;
	
	/* Error tracking */
	errors_list_t errors;
	
	/* Timing data for output */
	double mat_fill_time;   /* Matrix fill time in seconds */
	double mat_factor_time; /* Matrix factor time in seconds */
	clock_t start_time;     /* Start time for total runtime calculation */
	
	/* Batch processing state for XQ command support */
	int current_card_idx;   /* Current position in deck for batch processing */
	int batch_start_card;   /* Start of current batch (inclusive) */
	int batch_end_card;     /* End of current batch (inclusive) */
	int card_number_offset; /* Starting card number for current batch */
	int iflow;              /* Processing state: 1=FR, 2=CP, 3=LD, 6=NT/TL, 7-11=execution */
	int eval_depth;         /* To track recursion depth during symbol evaluation */
} nec_context_t;

void nec_context_init(nec_context_t *ctx);
void nec_context_cleanup(nec_context_t *ctx);

// typedefs for backward compatibility
typedef card_t Card;
typedef deck_t Deck;

#endif /* TYPES_H */
