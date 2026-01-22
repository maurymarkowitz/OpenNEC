#ifndef	NEC2C_H
#define	NEC2C_H 1

#include <complex.h>
#include <stdio.h>
#include <signal.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/times.h>

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

/* common constants */
#define PI		3.141592654
#define	POT		1.570796327
#define	TP		6.283185308
#define	PTP		.6283185308
#define	TPJ		(0.0+I*6.283185308)
#define PI8		25.13274123
#define PI10	31.41592654
#define	TA		1.745329252E-02
#define	TD		57.29577951
#define	ETA		376.73
#define	CVEL	299.8
#define	RETA	2.654420938E-3
#define	TOSP	1.128379167
#define ACCS	1.E-12
#define	SP		1.772453851
#define	FPI		12.56637062
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
#define	LINE_LEN	132

/*** Structs encapsulating global ("common") variables ***/
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
typedef struct
{
  int
	n,		/* Number of wire segments */
	np,		/* Number of wire segments in symmetry cell */
	m,		/* Number of surface patches */
	mp,		/* Number of surface patches in symmetry cell */
	npm,	/* = n+m  */
	np2m,	/* = n+2m */
	np3m,	/* = n+3m */
	ipsym,	/* Symmetry flag */
	*icon1, /* Segments end 1 connection */
	*icon2,	/* Segments end 2 connection */
	*itag;	/* Segments tag number */

  /* Wire segment data */
  double
	*x1, *y1, *z1,	/* End 1 coordinates of wire segments */
	*x2, *y2, *z2,	/* End 2 coordinates of wire segments */
	*x, *y, *z,		/* Coordinates of segment centers */
	*si, *bi,		/* Length and radius of segments  */
	*cab,			/* cos(a)*cos(b) */
	*sab,			/* cos(a)*sin(b) */
	*salp,			/* Z component - sin(a) */

	/* Surface patch data */
	*px, *py, *pz,		/* Coordinates of patch center */
	*t1x, *t1y, *t1z,	/* Coordinates of t1 vector */
	*t2x, *t2y, *t2z,	/* Coordinates of t2 vector */
	*pbi,				/* Patch surface area */
	*psalp,				/* Z component - sin(a) */

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
	ifar,	/* Int flag in RP card, for far field calculations */
	iperf,	/* Type of ground flag */
	nradl;	/* Number of radials in ground screen */

  double
	t2,		/* Const for radial wire ground impedance */
	cl,		/* Distance in wavelengths of cliff edge from origin */
	ch,		/* Cliff height in wavelengths */
	scrwl,	/* Wire length in radial ground screen normalized to w/length */
	scrwr;	/* Radius of wires in screen in wavelengths */

  complex double
	zrati,	/* Ground medium [Er-js/wE0]^-1/2 */
	zrati2,	/* As above for 2nd ground medium */
	t1,		/* Const for radial wire ground impedance */
	frati;	/* (k1^2-k2^2)/(k1^2+k2^2), k1=w(E0Mu0)^1/2, k1=k2/ZRATI */

} gnd_t;

/* common  /gwav/ */
typedef struct
{
  double
	r1,		/* Distance from current element to point where field is evaluated  */
	r2,		/* Distance from image of element to point where field is evaluated */
	zmh,	/* Z-Z', Z is height of field evaluation point */
	zph;	/* Z+Z', Z' is height of current element */

  complex double
	u,		/* (Er-jS/WE0)^-1/2 */
	u2,		/* u^2 */
	xx1,	/* G1*exp(jkR1.r[i])  */
	xx2;	/* G2*exp(jkR2.r'[i]) */

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
	imat;	/* Storage reserved in CM for primary NGF matrix A */

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
	pin,	/* Total input power from sources */
	pnls;	/* Power lost in networks */

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
	epsr,	/* Relative dielectric constant of ground */
	sig,	/* Conductivity of ground */
	scrwlt,	/* Length of radials in ground screen approximation */
	scrwrt,	/* Radius of wires in ground screen approximation */
	fmhz;	/* Frequency in MHz */

} save_t;

/* common  /segj/ */
typedef struct
{
  int
	*jco,	/* Stores connection data */
	jsno,	/* Total number of entries in ax, bx, cx */
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
	nvqd,	/* Number of applied current-slope discontinuity sources */
	nqds;	/* Same as above (?) */

  complex double
	*vqd,	/* Voltage of applied-current slope discontinuity sources */
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

/* Returns the complex double of the arguments */
#define cmplx(r, i) ((r)+I*(i))

/*------------------------------------------------------------------------*/

/* Function prototypes produced by cproto */
/* calculations.c */
void cabc(complex double *curx);
void couple(complex double *cur, double wlam);
void load(int *ldtyp, int *ldtag, int *ldtagf, int *ldtagt, double *zlr, double *zli, double *zlc);
void gf(double zk, double *co, double *si);
double db10(double x);
double db20(double x);
void intrp(double x, double y, complex double *f1, complex double *f2, complex double *f3, complex double *f4);
void intx(double el1, double el2, double b, int ij, double *sgr, double *sgi);
int min(int a, int b);
void test(double f1r, double f2r, double *tr, double f1i, double f2i, double *ti, double dmin);
void sbf(int i, int is, double *aa, double *bb, double *cc);
void tbf(int i, int icap);
void trio(int j);
void zint(double sigl, double rolam, complex double *zt);
double cang(complex double z);
/* fields.c */
void efld(double xi, double yi, double zi, double ai, int ij);
void eksc(double s, double z, double rh, double xk, int ij, complex double *ezs, complex double *ers, complex double *ezc, complex double *erc, complex double *ezk, complex double *erk);
void ekscx(double bx, double s, double z, double rhx, double xk, int ij, int inx1, int inx2, complex double *ezs, complex double *ers, complex double *ezc, complex double *erc, complex double *ezk, complex double *erk);
void gh(double zk, double *hr, double *hi);
void gwave(complex double *erv, complex double *ezv, complex double *erh, complex double *ezh, complex double *eph);
void gx(double zz, double rh, double xk, complex double *gz, complex double *gzp);
void gxx(double zz, double rh, double a, double a2, double xk, int ira, complex double *g1, complex double *g1p, complex double *g2, complex double *g2p, complex double *g3, complex double *gzp);
void hfk(double el1, double el2, double rhk, double zpkx, double *sgr, double *sgi);
void hintg(double xi, double yi, double zi);
void hsfld(double xi, double yi, double zi, double ai);
void hsflx(double s, double rh, double zpx, complex double *hpk, complex double *hps, complex double *hpc);
void nefld(double xob, double yob, double zob, complex double *ex, complex double *ey, complex double *ez);
void nfpat(void);
void nhfld(double xob, double yob, double zob, complex double *hx, complex double *hy, complex double *hz);
void pcint(double xi, double yi, double zi, double cabi, double sabi, double salpi, complex double *e);
void unere(double xob, double yob, double zob);
/* geometry.c */
void arc(int itg, int ns, double rada, double ang1, double ang2, double rad);
void conect(int ignd);
void datagn(void);
void helix(double s, double hl, double a1, double b1, double a2, double b2, double rad, int ns, int itg);
int isegno(int itagi, int mx);
void move(double rox, double roy, double roz, double xs, double ys, double zs, int its, int nrpt, int itgi);
void patch(int nx, int ny, double ax1, double ay1, double az1, double ax2, double ay2, double az2, double ax3, double ay3, double az3, double ax4, double ay4, double az4);
void subph(int nx, int ny);
void readgm(char *gm, int *i1, int *i2, double *x1, double *y1, double *z1, double *x2, double *y2, double *z2, double *rad);
void reflc(int ix, int iy, int iz, int itx, int nop);
void wire(double xw1, double yw1, double zw1, double xw2, double yw2, double zw2, double rad, double rdel, double rrad, int ns, int itg);
/* ground.c */
void rom2(double a, double b, complex double *sum, double dmin);
void sflds(double t, complex double *e);
/* input.c */
void qdsrc(int is, complex double v, complex double *e);
void readmn(char *gm, int *i1, int *i2, int *i3, int *i4, double *f1, double *f2, double *f3, double *f4, double *f5, double *f6);
/* main.c */
int main(int argc, char **argv);
void Null_Pointers(void);
void prnt(int in1, int in2, int in3, double fl1, double fl2, double fl3, double fl4, double fl5, double fl6, char *ia, int ichar);
/* matrix.c */
void cmset(int nrow, complex double *cm, double rkhx, int iexkx);
void cmss(int j1, int j2, int im1, int im2, complex double *cm, int nrow, int itrp);
void cmsw(int j1, int j2, int i1, int i2, complex double *cm, complex double *cw, int ncw, int nrow, int itrp);
void cmws(int j, int i1, int i2, complex double *cm, int nr, complex double *cw, int itrp);
void cmww(int j, int i1, int i2, complex double *cm, int nr, complex double *cw, int nw, int itrp);
void etmns(double p1, double p2, double p3, double p4, double p5, double p6, int ipr, complex double *e);
void factr(int n, complex double *a, int *ip, int ndim);
void factrs(int np, int nrow, complex double *a, int *ip);
void fblock(int nrow, int ncol, int imax, int ipsym);
void solve(int n, complex double *a, int *ip, complex double *b, int ndim);
void solves(complex double *a, int *ip, complex double *b, int neq, int nrh, int np, int n, int mp, int m);
/* misc.c */
void usage(void);
void abort_on_error(int why);
void secnds(double *x);
int stop(int flag);
int load_line(char *buff, FILE *pfile);
void mem_alloc(void **ptr, size_t req);
void mem_realloc(void **ptr, size_t req);
void free_ptr(void **ptr);
/* network.c */
void netwk(complex double *cm, int *ip, complex double *einc);
/* radiation.c */
void ffld(double thet, double phi, complex double *eth, complex double *eph);
void fflds(double rox, double roy, double roz, complex double *scur, complex double *ex, complex double *ey, complex double *ez);
void gfld(double rho, double phi, double rz, complex double *eth, complex double *epi, complex double *erd, complex double ux, int ksymp);
void rdpat(void);
/* somnec.c */
void somnec(double epr, double sig, double fmhz);
void bessel(complex double z, complex double *j0, complex double *j0p);
void evlua(complex double *erv, complex double *ezv, complex double *erh, complex double *eph);
void fbar(complex double p, complex double *r);
void gshank(complex double start, complex double dela, complex double *sum, int nans, complex double *seed, int ibk, complex double bk, complex double delb);
void hankel(complex double z, complex double *h0, complex double *h0p);
void lambda(double t, complex double *xlam, complex double *dxlam);
void rom1(int n, complex double *sum, int nx);
void saoa(double t, complex double *ans);
#endif

