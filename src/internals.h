#ifndef OPENNEC_INTERNALS_H
#define OPENNEC_INTERNALS_H

#include "opennec.h"

/* common constants - Internal physics and grid parameters */
#define	POT		1.570796327
#define	PTP		.6283185308
#define	TPJ		(0.0+I*6.283185308)
#define PI8		25.13274123
#define PI10	31.41592654
#define FPI     12.56637062
#define	ETA		376.73
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

/* Replaces the "10000" limit used to identiy segment/patch connections */
#define	PCHCON  100000

/* carriage return and line feed */
#define	CR	0x0d
#define	LF	0x0a

/* Internal structure definitions moved from types.h for Opaque Handle support */

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
 */
typedef struct geometry_t
{
	int
		n,		      	// Number of wire segments in total
		np,		      	// Number of wire segments in symmetry cell
		m,		      	// Number of surface patches
		mp,		      	// Number of surface patches in symmetry cell
		npm,	      	// = n+m
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
struct nec_context_t
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
};

/* Internal initialization and cleanup (called by nec_create_context/nec_destroy_context) */
void nec_context_init(nec_context_t *ctx);
void nec_context_cleanup(nec_context_t *ctx);

#endif /* OPENNEC_INTERNALS_H */
