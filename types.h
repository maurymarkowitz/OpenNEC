/*******************************************************************
 *
 * types.h
 *
 * types.h defines the many data structures that are used to pass
 * data around the calculation system. In nec2c these were defined
 * in nec2c.h, and have been moved here for clarity. OpenNEC also
 * adds new types for the Deck and Card, so they can be passed
 * instead of using globals. Other new types include Error and
 * Errors, KeyValue, and various definitions of measurements and such.
 *
 *******************************************************************/

#ifndef types_h
#define types_h

#include <complex.h>
#include <stdio.h>
#include <stdbool.h>  // we will use the bool type!

// OpenNEC generally allows commas or any whitespace between fields
#ifndef OUR_WHITESPACE_DEF
#define OUR_WHITESPACE_DEF
#define OUR_WHITESPACE ", \t\n\r\v\f\0" // should comma be a separator? look for examples
#endif

// these are the markers for inline comments
#ifndef OUR_COMMENTS_DEF
#define OUR_COMMENTS_DEF
#define OUR_COMMENTS "!'#"
#endif

// these are the separators within an OpenNEC extension list
#ifndef OUR_SEPARATORS_DEF
#define OUR_SEPARATORS_DEF
#define OUR_SEPARATORS ";"
#endif

// these are the delimeters between the key and value pairs
#ifndef OUR_DELIMETERS_DEF
#define OUR_DELIMETERS_DEF
#define OUR_DELIMETERS "=:"
#endif

/* card field names, like "I1" of "F4" */
#ifndef FIELD_NAMES_DEF
#define FIELD_NAMES_DEF
#define NUM_FIELD_NAMES 10
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
#define NUM_GEOMETRY_CODES  12
extern char *geometry_codes[NUM_GEOMETRY_CODES];
#endif

#ifndef ONEC_CODES_DEF
#define ONEC_CODES_DEF
#define NUM_ONEC_CODES 4
extern char *onec_codes[NUM_ONEC_CODES];
#endif

#ifndef UNITS_DEF
#define UNITS_DEF
#define NUM_UNIT_CODES 8
extern char *unit_codes[NUM_UNIT_CODES];
extern double unit_mult[NUM_UNIT_CODES];
#endif

/*** Structs encapsulating global ("common") variables ***/

/*** Error levels are used internally, external software should use negatives ***/
enum error_level { MINOR, PROBLEM, FATAL };    // 0 = warning, 1 = error, 2 = fatal, <0 informational

/*** Error has information about a single error or warning ***/
typedef struct
{
  int severity;
  char *message;  // the error string
} Error;

/*** Errors is a list generated during a particular stage, typically
there will be different error lists for import, sanity checks, running
and export
 ***/
typedef struct
{
  int num_errors; // total number of errors in this list
  Error *errors;  // pointer to a list of errors
} Errors;

/*** KeyValue is a key:value pair used to store an OpenNEC extension on a card ***/
typedef struct KeyValue
{
  char *key;
  char *value;
  char separator; // what separator was used, a colon or an equals?
  struct KeyValue* next;
} KeyValue;
//typedef struct KeyValue KeyValue;

/*** Card encapsulates a single card ***/
typedef struct
{
  // used to track whether this card has been edited since being read
  bool edited;
  
  // raw data from the original card
  char *orig_str;     // the original line, as read from the file in raw format
  char *card_str;     // the "card part" of the string, everything in front of the comment (if one exists)
  
  // processed NEC2 data
  int card_num;       // card (line) number within the deck. mostly used for error reporting ("Card X has an error")
  char card_code[2];  // the two-letter code for this card, or one letter for some comment formats
  int i1, i2, i3, i4; // various bits read from the cards - i1 is normally the tag, for instance
  double f1, f2, f3;  // various floats/doubles read from the cards
  double f4, f5, f6;
  double f7, f8, f9;
  double f10;         // last possible input, happens in a GW/GC pair
  
  // onec values
  int m1, m2, m3;     // measurement units on the fields, or 0 for "default"
  int m4, m5, m6;
  int m7, m8, m9;
  int m10;
  char *if1, *if2;    // holds the formula for each of the possible fields, i or f
  char *if3, *if4;    // formulas can also be placed in the extensions
  char *ff1, *ff2;
  char *ff3, *ff4;
  char *ff5, *ff6;
  char *ff7, *ff8;
  char *ff9, *ff10;

  // onec extensions
  char extn_code[1];  // the one-letter code that marked the extension or inline comment, if any
  char *extn_str;     // the entire inline comment, anything after the comment marker
  char *name;         // name for this card, if present
  char *group;        // group name, used to collect multiple cards into groups
  char *comment;      // if a comment was found, it's placed here, this is not the same as extn_str, it might be comment:
  KeyValue *pairs;    // pairs of name:value key/value entries, this will **not** include a comment if there was one
  KeyValue *formulas; // pairs of variable=formula pairs found in SY cards or in the extensions area
} Card;

/*** Deck encapsulates a single deck of cards ***/
typedef struct
{
  int num_cards;      // total number of cards read in, including any trailing lines
  Card *cards;        // array of cards
  int comment_start;  // card number of the start of the comments section, normally 0. -1 if there are no CM or CE cards
  int comment_end;    // card number of the last continuous CM card, or the CE card if present. -1 if there are no CM or CE cards
  int geometry_start; // card number of the first geometry card, which definitely should exist. -1 if not found
  int geometry_end;   // card number of the GE card, which also has to exist. -1 if not found
  int deck_end;       // card number of the EN card or the last card in the deck otherwise. -1 if not found
  char cmt_code[1];   // the default marker to use for comments, !, $ or '
  int unit_val;       // if there is a single GS, this is the f1 value, otherwise 1
  int unit_typ;       // if there is a single GS, and we recognize the value, put out index here
  KeyValue *symbols;  // any variables read in from SY cards, consisting of name/inital value pairs
  KeyValue *formulas; // any *global* formulas found on any of the cards, consists of variable=formula pairs
} Deck;

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

/* common  /data/ (geometry data) */
/* Holds segment and patch data for the entire geometry of the deck.
* A given deck will have only one data_t object at a given time.
* This is populated by parsing the geometry section of the deck, and
* can be used in external programs to build 3D models and similar
* tasks.
*/
typedef struct
{
	int
		n,		  /* Number of wire segments */
		np,		  /* Number of wire segments in symmetry cell */
		m,		  /* Number of surface patches */
		mp,		  /* Number of surface patches in symmetry cell */
		npm,	  /* = n+m  */
		np2m,	  /* = n+2m */
		np3m,	  /* = n+3m */
		ipsym,	/* Symmetry flag */
		*icon1, /* Segments end 1 connection */
		*icon2,	/* Segments end 2 connection */
		*itag;	/* Segments tag number */

	double
    /* Wire segment data */
    *x1, *y1, *z1,	/* End 1 coordinates of wire segments */
		*x2, *y2, *z2,	/* End 2 coordinates of wire segments */
		*x, *y, *z,		  /* Coordinates of segment centers */
		*si, *bi,		    /* Length and radius of segments  */
		*cab,			      /* cos(a)*cos(b) */
		*sab,			      /* cos(a)*sin(b) */
		*salp,			    /* Z component - sin(a) */

    /* Surface patch data */
		*px, *py, *pz,		/* Coordinates of patch center */
		*t1x, *t1y, *t1z,	/* Coordinates of t1 vector */
		*t2x, *t2y, *t2z,	/* Coordinates of t2 vector */
		*pbi,				      /* Patch surface area */
		*psalp,				    /* Z component - sin(a) */
  
    /* Wavelength in meters */
    wlam;
} data_t;

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
		pnls;	  /* Power lost in networks */

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

	double
		epsr,	  /* Relative dielectric constant of ground */
		sig,	  /* Conductivity of ground */
		scrwlt,	/* Length of radials in ground screen approximation */
		scrwrt,	/* Radius of wires in ground screen approximation */
		fmhz;	  /* Frequency in MHz */

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
	complex double *zarray;	/* = Zi/(Di/lambda) */
} zload_t;

#endif /* types_h */
