#ifndef OPENNEC_INTERNALS_H
#define OPENNEC_INTERNALS_H

#include "opennec.h"

/* common constants - Internal physics and grid parameters */
#define	POT		(M_PI / 2.0)
#define	PTP		(M_PI / 5.0)
#define	TPJ		(0.0+I*(2.0 * M_PI))
#define PI8		(8.0 * M_PI)
#define PI10	(10.0 * M_PI)
#define FPI     (4.0 * M_PI)
#define	ETA		376.73031346
#define	RETA	(1.0 / ETA)
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
#define	PCHCON  10000

/* carriage return and line feed */
#define	CR	0x0d
#define	LF	0x0a

/* Internal structure definitions moved from types.h for Opaque Handle support */

typedef struct
{
  double
    *a_real,	/* Ai/lambda, real part */      /* air — Fortran AIR */
    *a_imag,	/* Ai/lambda, imaginary part */  /* aii — Fortran AII */
    *b_real,	/* Bi/lambda, real part */       /* bir — Fortran BIR */
    *b_imag,	/* Bi/lambda, imaginary part */  /* bii — Fortran BII */
    *c_real,	/* Ci/lambda, real part */       /* cir — Fortran CIR */
    *c_imag;	/* Ci/lambda, imaginary part */  /* cii — Fortran CII */

  complex double *surface_cur; /* Amplitude of basis function */ /* cur — Fortran CUR */
} current_t; /* Formerly: Fortran /CRNT/ → nec2c: crnt_t */

/** common  /geometry/ (geometry data)
 */
typedef struct geometry_t /* Formerly: Fortran /DATA/ → nec2c: data_t */
{
	int
		num_segs,         /* n — Fortran N: total wire segment count */
		num_segs_sym,     /* np — Fortran NP: segments in symmetry cell */
		num_patches,      /* m — Fortran M: surface patch count */
		num_patches_sym,  /* mp — Fortran MP: patches in symmetry cell */
		num_segs_and_patches, /* npm — N+M */
		num_segs_2xpatches,   /* np2m — N+2M */
		num_segs_3xpatches,   /* np3m — N+3M */
		symmetry_flag,    /* ipsym — Fortran IPSYM: symmetry flag (0/1/2/3/negative) */
		*seg_end1_conn,   /* icon1 — Fortran ICON1: segment end-1 connection index */
		*seg_end2_conn,   /* icon2 — Fortran ICON2: segment end-2 connection index */
		*tag_nums,        /* Fortran ITAG: segment tag numbers */
    	*card_nums,       /* OpenNEC: source card line numbers */
    	*patch_card_nums; /* OpenNEC: source card line numbers for patches */

	double
    	// Wire segment data
    	*end1_x, *end1_y, *end1_z, /* x1/y1/z1 — Fortran X/Y/Z (pre-CABC): seg end-1 coords */
		*end2_x, *end2_y, *end2_z, /* x2/y2/z2 — SI/ALP/BET (EQUIV): seg end-2 coords */
		*x_center, *y_center, *z_center, /* x/y/z — Fortran X/Y/Z: seg center coords (wavelengths) */
		*half_len,   /* si — Fortran SI: segment half-length (wavelengths) */
		*radius,     /* bi — Fortran BI: segment radius (wavelengths) */
		*dir_cos_x,  /* cab — Fortran CAB: cos(α)·cos(β) direction cosine */
		*dir_cos_y,  /* sab — Fortran SAB: cos(α)·sin(β) direction cosine */
		*dir_cos_z,  /* salp — Fortran SALP: sin(α) direction cosine */

    	// Surface patch data
		*patch_x_center, *patch_y_center, *patch_z_center, /* px/py/pz — patch center coords */
		*patch_t1x, *patch_t1y, *patch_t1z, /* t1x/t1y/t1z — patch tangent vector T1 */
		*patch_t2x, *patch_t2y, *patch_t2z, /* t2x/t2y/t2z — patch tangent vector T2 */
		*patch_area,      /* pbi — Fortran BI (patch EQUIV): patch area (wavelengths²) */
		*patch_normal_z,  /* psalp — Fortran SALP (patch EQUIV): patch normal direction cosine */

    	wavelength; /* wlam — Fortran WLAM: wavelength in meters */
  
  // list of errors added while processing this geometry
  errors_list_t errors;
} geometry_t;

/* common  /dataj/ */
/* Formerly: Fortran /DATAJ/ → nec2c: dataj_t */
typedef struct
{
	int
		use_extended_kernel,  /* iexk — Fortran IEXK: extended thin-wire kernel flag */
		end1_kernel_type,     /* ind1 — Fortran IND1: end-1 kernel indicator */
		end1_kernel_deferred, /* indd1 — Fortran INDD1: end-1 deferred kernel indicator */
		end2_kernel_type,     /* ind2 — Fortran IND2: end-2 kernel indicator */
		end2_kernel_deferred, /* indd2 — Fortran INDD2: end-2 deferred kernel indicator */
		ground_image_pass;    /* ipgnd — Fortran IPGND: ground image loop pass (1 or 2) */

	double
		seg_half_len,    /* s — Fortran S: source segment half-length */
		seg_radius,      /* b — Fortran B: source segment radius / patch T2X */
		src_x,           /* xj — Fortran XJ: source segment center x */
		src_y,           /* yj — Fortran YJ: source segment center y */
		src_z,           /* zj — Fortran ZJ: source segment center z */
		src_dir_cos_x,   /* cabj — Fortran CABJ: cos(α)cos(β) for source segment */
		src_dir_cos_y,   /* sabj — Fortran SABJ: cos(α)sin(β) for source segment */
		src_dir_cos_z,   /* salpj — Fortran SALPJ: sin(α) for source segment */
		k_half_len,      /* rkh — Fortran RKH: k × half-length */
		patch_t1x,       /* t1xj — Fortran CABJ(EQUIV): patch T1 x-component */
		patch_t1y,       /* t1yj — Fortran SABJ(EQUIV): patch T1 y-component */
		patch_t1z,       /* t1zj — Fortran SALPJ(EQUIV): patch T1 z-component */
		patch_t2x,       /* t2xj — Fortran B(EQUIV): patch T2 x-component */
		patch_t2y,       /* t2yj — Fortran IND1(EQUIV): patch T2 y-component */
		patch_t2z;       /* t2zj — Fortran IND2(EQUIV): patch T2 z-component */

	complex double
		e_const_x,  /* exk — Fortran EXK: E-field contribution (constant current), x */
		e_const_y,  /* eyk — Fortran EYK: E-field contribution (constant current), y */
		e_const_z,  /* ezk — Fortran EZK: E-field contribution (constant current), z */
		e_sin_x,    /* exs — Fortran EXS: E-field contribution (sine current), x */
		e_sin_y,    /* eys — Fortran EYS: E-field contribution (sine current), y */
		e_sin_z,    /* ezs — Fortran EZS: E-field contribution (sine current), z */
		e_cos_x,    /* exc — Fortran EXC: E-field contribution (cosine current), x */
		e_cos_y,    /* eyc — Fortran EYC: E-field contribution (cosine current), y */
		e_cos_z;    /* ezc — Fortran EZC: E-field contribution (cosine current), z */

} segment_t;

/* common  /fpat/ */
/* Formerly: Fortran /FPAT/ → nec2c: fpat_t */
typedef struct
{
	int
		is_near_field,  /* near — Fortran NEAR: near-field flag */
		near_field_type, /* nfeh — Fortran NFEH: 0=E, 1=H */
		grid_nx,        /* nrx — Fortran NRX: near-field grid x dimension */
		grid_ny,        /* nry — Fortran NRY: near-field grid y dimension */
		grid_nz,        /* nrz — Fortran NRZ: near-field grid z dimension */
		num_theta,      /* nth — Fortran NTH: number of theta angles */
		num_phi,        /* nph — Fortran NPH: number of phi angles */
		gain_type,      /* ipd — Fortran IPD: power/directive gain selector */
		avg_power_flag, /* iavp — Fortran IAVP: average power integration */
		normalize_gain, /* inor — Fortran INOR: normalized gain flag */
		pol_axis,       /* iax — Fortran IAX: polarization axis selector */
		excitation_type; /* ixtyp — Fortran IXTYP: excitation type */

	double
		theta_start,    /* thets — Fortran THETS: starting theta (deg) */
		phi_start,      /* phis — Fortran PHIS: starting phi (deg) */
		theta_step,     /* dth — Fortran DTH: theta step (deg) */
		phi_step,       /* dph — Fortran DPH: phi step (deg) */
		range,          /* rfld — Fortran RFLD: range to field point */
		norm_gain,      /* gnor — Fortran GNOR: normalization gain */
		cliff_dist,     /* clt — Fortran CLT: cliff edge distance */
		cliff_height,   /* cht — Fortran CHT: cliff height */
		epsr2,          /* Fortran EPSR2: second ground medium dielectric */
		sigma2,         /* sig2 — Fortran SIG2: second ground conductivity */
		exc_param6,     /* xpr6 — Fortran XPR6: excitation parameter 6 */
		power_in,       /* pinr — Fortran PINR: input power (watts) */
		network_loss,   /* pnlr — Fortran PNLR: network power loss (watts) */
		ohmic_loss,     /* ploss — Fortran PLOSS: ohmic loss (watts) */
		grid_x0,        /* xnr — Fortran XNR: near-field grid origin x */
		grid_y0,        /* ynr — Fortran YNR: near-field grid origin y */
		grid_z0,        /* znr — Fortran ZNR: near-field grid origin z */
		grid_dx,        /* dxnr — Fortran DXNR: near-field grid spacing x */
		grid_dy,        /* dynr — Fortran DYNR: near-field grid spacing y */
		grid_dz;        /* dznr — Fortran DZNR: near-field grid spacing z */

} field_pattern_t;

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
	int    pol_sense; /* 0=LINEAR, 1=RIGHT, 2=LEFT, 3=BLANK (no radiation) */
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

/* Near-field data point (E or H field at one observation location) */
typedef struct
{
	double xob, yob, zob;       /* observation point coordinates (metres) */
	complex double ex, ey, ez;  /* field components (E or H) */
} near_field_point_t;

/* Near-field results accumulated by compute_near_field() */
typedef struct
{
	int num_points;
	near_field_point_t *points;
	int nfeh;   /* 0 = E-field, 1 = H-field */
} near_field_results_t;

/*common  /ggrid/ */
/* Formerly: Fortran /GGRID/ → nec2c: ggrid_t */
typedef struct
{
	int
		grid_nx[3],  /* nxa — Fortran NXA(3): grid point counts */
		grid_ny[3];  /* nya — Fortran NYA(3) */

	double
		grid_dx[3],  /* dxa — Fortran DXA(3): grid spacing */
		grid_dy[3],  /* dya — Fortran DYA(3) */
		grid_x0[3],  /* xsa — Fortran XSA(3): grid origin */
		grid_y0[3];  /* ysa — Fortran YSA(3) */

	complex double
		dielectric,  /* epscf — Fortran EPSCF: complex ground dielectric */
		*table1,     /* ar1 — Fortran AR1(11,10,4): Sommerfeld table 1 */
		*table2,     /* ar2 — Fortran AR2(17,5,4): Sommerfeld table 2 */
		*table3;     /* ar3 — Fortran AR3(9,8,4): Sommerfeld table 3 */

} green_grid_t;

/* common  /gnd/ */
/* Formerly: Fortran /GND/ → nec2c: gnd_t */
typedef struct
{
	int
		has_ground,   /* ksymp — Fortran KSYMP: ground presence flag (1=none, 2=ground) */
		far_field_type, /* ifar — Fortran IFAR: far-field ground interaction type */
		is_perfect,   /* iperf — Fortran IPERF: perfect ground flag */
		num_radials;  /* nradl — Fortran NRADL: number of radial wires in screen */

	double
		screen_inner_r, /* t2 — Fortran T2: screen inner radius intermediate */
		cliff_dist,     /* cl — Fortran CL: cliff edge distance (wavelengths) */
		cliff_height,   /* ch — Fortran CH: cliff height (wavelengths) */
		screen_wire_len,    /* scrwl — Fortran SCRWL: screen wire length */
		screen_wire_radius; /* scrwr — Fortran SCRWR: screen wire radius */

	complex double
		impedance_ratio,  /* zrati — Fortran ZRATI: ground impedance ratio */
		impedance_ratio2, /* zrati2 — Fortran ZRATI2: second medium impedance ratio */
		screen_impedance, /* t1 — Fortran T1: wire screen impedance intermediate */
		fresnel_ratio;    /* frati — Fortran FRATI: Fresnel reflection boundary param */

} ground_params_t;

/* common  /gwav/ */
/* Formerly: Fortran /GWAV/ → nec2c: gwav_t */
typedef struct
{
	double
		range1,   /* r1 — Fortran R1: distance to source (image 1) */
		range2,   /* r2 — Fortran R2: distance to source (image 2) */
		z_img1,   /* zmh — Fortran ZMH: z − z', height difference */
		z_img2;   /* zph — Fortran ZPH: z + z', height sum */

	complex double
		impedance_ratio,    /* u — Fortran U: ground impedance ratio */
		impedance_ratio_sq, /* u2 — Fortran U2: impedance ratio squared */
		cur_phase1,         /* xx1 — Fortran XX1: current moment × phase factor 1 */
		cur_phase2;         /* xx2 — Fortran XX2: current moment × phase factor 2 */

} ground_wave_t;

/* common  /incom/ */
/* Formerly: Fortran /INCOM/ → nec2c: incom_t */
typedef struct
{
	int32_t use_sommerfeld; /* isnor — Fortran ISNOR: 1=Sommerfeld, 0=Norton */

	double
		obs_x,      /* xo — Fortran XO: observation point x */
		obs_y,      /* yo — Fortran YO: observation point y */
		obs_z,      /* zo — Fortran ZO: observation point z */
		sin_alpha,  /* sn — Fortran SN: sin(α) of source segment */
		dir_cos_x,  /* xsn — Fortran XSN: horizontal direction cosine x */
		dir_cos_y;  /* ysn — Fortran YSN: horizontal direction cosine y */

} green_params_t;

/* common  /matpar/ (matrix parameters) */
/* Formerly: Fortran /MATPAR/ → nec2c: matpar_t */
typedef struct
{
	int
		storage_case,    /* icase — Fortran ICASE: matrix storage mode (1-5) */
		block_rows,      /* npblk — Fortran NPBLK: rows per block */
		last_block_rows, /* nlast — Fortran NLAST: rows in last block */
		core_used;       /* imat — Fortran IMAT: complex words of core storage */

} matrix_params_t;

/* common  /netcx/ */
typedef struct
{
	int
		check_asymmetry,  /* masym — Fortran MASYM: matrix asymmetry check flag */
		num_eq,           /* neq — Fortran NEQ: total equations (matrix size) */
		num_eq_sym,       /* npeq — Fortran NPEQ: equations per symmetry section */
		num_eq_ngf,       /* neq2 — Fortran NEQ2: NGF new-structure unknowns */
		num_networks,     /* nonet — Fortran NONET: number of two-port networks */
		network_type,     /* ntsol — Fortran NTSOL: network solution type */
		print_net_data,   /* nprint — Fortran NPRINT: network data print flag */
		*net_seg1,        /* iseg1 — Fortran ISEG1: network port-1 segment numbers */
		*net_seg2,        /* iseg2 — Fortran ISEG2: network port-2 segment numbers */
		*net_types;       /* ntyp — Fortran NTYP: network type codes */

	double
		*y11_real,  /* x11r — Fortran X11R: admittance Y11 real */
		*y11_imag,  /* x11i — Fortran X11I: admittance Y11 imaginary */
		*y12_real,  /* x12r — Fortran X12R: admittance Y12 real */
		*y12_imag,  /* x12i — Fortran X12I: admittance Y12 imaginary */
		*y22_real,  /* x22r — Fortran X22R: admittance Y22 real */
		*y22_imag,  /* x22i — Fortran X22I: admittance Y22 imaginary */
		power_in,         /* pin — Fortran PIN: total input power from sources */
		power_net_loss,   /* pnls — Fortran PNLS: power lost in networks */
		max_asymmetry,    /* asmx: maximum relative asymmetry */
		rms_asymmetry,    /* asa: RMS relative asymmetry */
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

	complex double input_impedance; /* zped — Fortran ZPED: network input impedance */

} network_context_t; /* Formerly: Fortran /NETCX/ → nec2c: netcx_t */

/* common  /plot/ */
/* Formerly: Fortran /PLOT/ → nec2c: plot_t */
typedef struct
{
	int
		plot_type,      /* iplp1 — Fortran IPLP1: plot type selector */
		plot_axis,      /* iplp2 — Fortran IPLP2: plot axis/variable selector */
		plot_component, /* iplp3 — Fortran IPLP3: plot component selector */
		plot_gain_type; /* iplp4 — Fortran IPLP4: plot gain/field selector */

} plot_params_t;

/* common  /save/ */
/* Formerly: Fortran /SAVE/ → nec2c: save_t */
typedef struct
{
	int *pivot; /* ip — Fortran IP: LU factorization pivot indices */

	int
		num_freq,       /* nfrq: number of frequency steps */
		freq_step_type; /* ifrq: frequency step type (0=linear, 1=multiplicative) */

	double
		ground_epsr,        /* epsr — Fortran EPSR: relative dielectric constant */
		ground_sigma,       /* sig — Fortran SIG: ground conductivity (S/m) */
		screen_wire_len,    /* scrwlt — Fortran SCRWLT: screen radial wire length (m) */
		screen_wire_radius, /* scrwrt — Fortran SCRWRT: screen radial wire radius (m) */
		freq_mhz,           /* fmhz — Fortran FMHZ: current frequency (MHz) */
		freq_step,          /* delfrq: frequency step size */
		first_fr_mhz;       /* frequency from the first FR card; used as LD6 design-freq default */

} run_params_t;

/* common  /segj/ */
/* Formerly: Fortran /SEGJ/ → nec2c: segj_t */
typedef struct
{
	int
		*junction_segs,   /* jco — Fortran JCO: connection segment index array */
		num_junction_segs, /* jsno — Fortran JSNO: number of junction entries */
		max_connections;  /* maxcon: allocated connection capacity */

	double
		*coeff_const, /* ax — Fortran AX: constant-current basis coefficients */
		*coeff_sine,  /* bx — Fortran BX: sine-current basis coefficients */
		*coeff_cos;   /* cx — Fortran CX: cosine-current basis coefficients */

} segment_junction_t;

/* common  /smat/ */
/* Formerly: Fortran /SMAT/ → nec2c: smat_t */
typedef struct
{
	int num_sections; /* nop: number of symmetry sections (= NEQ/NPEQ) */

	complex double *mode_matrix; /* ssx — Fortran SSX(16,16): symmetry mode transformation matrix */

} symmetry_matrix_t;

/* common  /tmi/ */
/* Formerly: Fortran /TMI/ → nec2c: tmi_t */
typedef struct
{
	int kernel_type; /* ij — Fortran IJX: kernel type selector (0/±1) */

	double
		seg_center_z, /* zpk — Fortran ZPK: segment center z-coordinate */
		k_radius_sq;  /* rkb2 — Fortran RKB2: (k·radius)² */

} wire_e_integration_t;

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
/* Formerly: Fortran local HFK vars → nec2c: tmh_t */
typedef struct
{
	double
		seg_center_z, /* zpka — Fortran ZPKA: segment center z-coord */
		k_radius;     /* rhks — Fortran RHKS: k·radius (H-field version) */

} wire_h_integration_t;

/* common  /vsorc/ */
/* Formerly: Fortran /VSORC/ → nec2c: vsorc_t */
typedef struct
{
	int
		*vsrc_segs,       /* isant — Fortran ISANT: voltage source segment numbers */
		*qdsrc_segs,      /* ivqd — Fortran IVQD: charge-disc source segment numbers */
		*qdsrc_indices,   /* iqds — Fortran IQDS: charge-disc source indices */
		num_vsrcs,        /* nsant — Fortran NSANT: number of voltage sources */
		num_qdsrcs,       /* nvqd — Fortran NVQD: number of charge-disc sources */
		num_qdsrcs_used;  /* nqds — Fortran NQDS: charge-disc sources used */

	complex double
		*qdsrc_voltages,        /* vqd — Fortran VQD: charge-disc source voltages */
		*qdsrc_voltages_saved,  /* vqds — Fortran VQDS: charge-disc voltages (saved) */
		*vsrc_voltages;         /* vsant — Fortran VSANT: voltage source voltages */

} voltage_sources_t;

/* One row of CP (coupling) output, accumulated during calculation */
typedef struct {
  int tag1, seg1, segno1;
  int tag2, seg2, segno2;
  bool is_error;             /* true when coupling is outside [0,1] */
  double coupling_db;        /* max coupling in dB (valid when !is_error) */
  double zl_real, zl_imag;  /* load impedance   (valid when !is_error) */
  double zin_real, zin_imag;/* input impedance  (valid when !is_error) */
  double c_value;            /* raw c value      (valid when is_error)  */
} coupling_row_t;

/* common  /yparm/ */
typedef struct
{
	int
		num_pairs,    /* ncoup — Fortran NCOUP: number of coupling pairs */
		coupling_flag, /* icoup — Fortran ICOUP: coupling computation flag */
		*pair_tags,   /* nctag — Fortran NCTAG: coupling pair tag numbers */
		*pair_segs;   /* ncseg — Fortran NCSEG: coupling pair segment numbers */

	complex double
		*y11,  /* y11a — Fortran Y11A: self-admittance Y11 array */
		*y12;  /* y12a — Fortran Y12A: mutual admittance Y12 array */

  /* accumulated CP output rows — rendered by write_nec_output() */
  coupling_row_t *coupling_rows;
  int num_coupling_rows;
  int coupling_rows_cap;

} coupling_params_t;

/* common  /zload/ */
/* Formerly: Fortran /ZLOAD/ → nec2c: zload_t */
typedef struct
{
	int num_loads;	/* nload — Fortran NLOAD: number of loading networks */
	
	int
		*load_types,    /* ldtyp — Fortran LDTYP: loading type codes */
		*load_tags,     /* ldtag — Fortran LDTAG: loading tag numbers */
		*load_tag_from, /* ldtagf — Fortran LDTAGF: loading tag range start */
		*load_tag_to,   /* ldtagt — Fortran LDTAGT: loading tag range end */
		*ldcard_num;	/* Deck line number of the originating LD card */
	
	double
		*load_r,    /* zlr — Fortran ZLR: R value (Ω, H, or F depending on type) */
		*load_l,    /* zli — Fortran ZLI: L value */
		*load_c,    /* zlc — Fortran ZLC: C value */
		*load_freq; /* LD type-6 design frequency (MHz); 0 = use first FR card */
	
	complex double *seg_impedance; /* zarray — Fortran ZARRAY: per-segment normalized impedance */
} impedance_loading_t;

/* Structure for storing loading output data */
typedef struct loading_output_t {
    int tag;           /* Tag number */
    int tagf;          /* Segment start */
    int tagt;          /* Segment end */
    double conductivity; /* Conductivity value (WIRE type) */
    double f1;         /* Real impedance / resistance (FIXED IMPEDANCE: zlr) */
    double f2;         /* Imaginary impedance / reactance (FIXED IMPEDANCE: zli) */
    char type[20];     /* Loading type description */
} loading_output_t;

typedef struct loading_outputs_t {
    int count;
    int capacity;
    loading_output_t *entries;
} loading_outputs_t;

/* context_t structure containing all context variables */
struct context_t
{
	geometry_t geometry;
	geometry_t ignored_geometry;
	current_t crnt;
	segment_t dataj;
	FILE *input_fp;
	FILE *output_fp;
	FILE *error_fp;
	FILE *green_fp;
	FILE *plot_fp;
	char *source_filename;  /**< Path of the input deck file, used to derive default NGF filenames. NULL when reading stdin. */

	/* Numerical Green's Function (NGF) state */
	bool has_ngf;              /* True if NGF segments were loaded via GF card */
	int ngf_n_segs;            /* Number of NGF segments loaded from GF file */
	int ngf_neq;               /* Matrix size stored in the NGF file */
	double ngf_fmhz;           /* Frequency (MHz) at which NGF was computed */
	complex double *ngf_cm;    /* Cached CM matrix from NGF file (ngf_neq x ngf_neq, col-major) */
	field_pattern_t fpat;
	green_grid_t ggrid;
	ground_params_t gnd;
	ground_wave_t gwav;
	green_params_t incom;
	matrix_params_t matpar;
	network_context_t netcx;
	plot_params_t plot;
	run_params_t save;
	segment_junction_t segj;
	symmetry_matrix_t smat;
	wire_e_integration_t tmi;
	voltage_sources_t vsorc;
	coupling_params_t yparm;
	impedance_loading_t zload;
	loading_outputs_t loading_outputs;
	
	/* Thread-safety state (formerly static globals) */
	somnec_t somnec;
	intrp_t intrp;
	wire_h_integration_t tmh;
	
	/* Radiation pattern results */
	rpat_results_t rpat;
	
	/* Near-field results */
	near_field_results_t nfr;
	
	/* Error and message tracking */
	errors_list_t errors;
	outputs_list_t outputs;
	
	/* Logging callbacks */
	log_callback_t log_callback;
	void *log_user_data;
	
	/* Timing data for output */
	double mat_fill_time;   /* Matrix fill time in seconds */
	double mat_factor_time; /* Matrix factor time in seconds */
	double start_time;      /* Start time for total runtime calculation */
	
	/* Output format selection */
	int output_format;      /* OUTPUT_FORMAT_NEC2C or OUTPUT_FORMAT_ORIGINAL */
	int line_ending;        /* 0=LF (Unix), 1=CRLF (Windows); default 1 for Fortran */
	
	/* Batch processing state for XQ command support */
	int current_card_idx;   /* Current position in deck for batch processing */
	int batch_start_card;   /* Start of current batch (inclusive) */
	int batch_end_card;     /* End of current batch (inclusive) */
	int card_number_offset; /* Starting card number for current batch */
	int iflow;              /* Processing state: 1=FR, 2=CP, 3=LD, 6=NT/TL, 7-11=execution */
	int eval_depth;         /* To track recursion depth during symbol evaluation */
	bool xt_terminated;       /* True if simulation was halted by an XT card; no output is expected */
	bool wg_after_cmset;      /* True if WG card opened green_fp: write binary NGF then stop */
	bool frequency_loop_ran;        /* True if execute_frequency_loop() was called for at least one batch */
	bool freq_step_output_written;  /* True once per-step output has been written inside the freq loop */
	bool patterns_output_for_freq;  /* True if RP/NE/NH output was written for current frequency (to avoid duplicates) */
	bool preamble_written;          /* True once the geometry preamble has been written for this section */
	bool step_size_warned;    /* True once the Romberg step-size-limited warning has been emitted */
};

/* Internal initialization and cleanup (called by create_context/destroy_context) */
void context_init(context_t *ctx);
void context_cleanup(context_t *ctx);

#endif /* OPENNEC_INTERNALS_H */
