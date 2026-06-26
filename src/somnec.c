/******************************************************************************
 * somnec.c
 *
 * somnec.c generates interpolation grids for ground-influenced field
 * components using modified Sommerfeld integrals. These precomputed grids
 * are used by NEC’s ground models to efficiently evaluate reflection and
 * transmission effects for various observation geometries.
 *
 * Major responsibilities include:
 * - somnec(): Compute and populate three interpolation grids (ar1/ar2/ar3)
 *   spanning different (r, theta) regions. Each grid stores complex field
 *   components (Erv, Ezv, Erh, Eph) derived from Sommerfeld integrals.
 * - Initialize medium parameters (epscf) from `epr` (relative permittivity),
 *   `sig` (conductivity), and `fmhz` (frequency) and set derived constants.
 * - Loop over radius and angle to evaluate integrals via `evaluate_sommerfeld_integrals()`, apply
 *   phasor scaling, and write results into the grid arrays in `ggrid`.
 * - Handle the r=0 limit for the first grid using closed-form expressions.
 *
 * These routines operate on `context_t`, primarily the `ggrid` common
 * block for grid settings and output arrays, and rely on shared constants and
 * helper evaluators to produce consistent data for ground calculations.
 *****************************************************************************/

#include "internals.h"
#include "somnec.h"
#include "calculations.h"

/* Forward declarations for internal functions */
static void bessel(context_t *ctx, complex double z, complex double *j0, complex double *j0p);
/* Formerly nec2c: evlua */
static void evaluate_sommerfeld_integrals(context_t *ctx, complex double *erv, complex double *ezv, complex double *erh, complex double *eph);
/* Formerly nec2c: gshank */
static int shanks_integration(context_t *ctx, complex double start, complex double dela, complex double *sum, int nans, complex double *seed, int ibk, complex double bk, complex double delb);
static int hankel(context_t *ctx, complex double z, complex double *h0, complex double *h0p);
/* Formerly nec2c: lambda */
static void sommerfeld_lambda(context_t *ctx, double t, complex double *xlam, complex double *dxlam);
/* Formerly nec2c: rom1 */
static void romberg_integrate_1d(context_t *ctx, int n, complex double *sum, int nx);
/* Formerly nec2c: saoa */
static void sommerfeld_asymptotic(context_t *ctx, double t, complex double *ans);

/*-----------------------------------------------------------------------*/

/* This is the "main" of somnec */
void somnec(context_t *ctx, double epr, double sig, double fmhz )
{
  int k, nth, ith, irs, ir, nr;
  double tim, wlam, tst, dr, dth=0.0, r, rk, thet, tfac1, tfac2;
  complex double erv, ezv, erh, eph, cl1, cl2, con;

  if(sig >= 0.) {
    wlam=CVEL/fmhz;
    ctx->ggrid.dielectric=cmplx(epr,-sig*wlam*59.96);
  }
  else
    ctx->ggrid.dielectric=cmplx(epr,sig);

  get_time_ms(ctx, &tst);
  ctx->somnec.evlcom.ck2 = TP;
  ctx->somnec.evlcom.ck2sq = ctx->somnec.evlcom.ck2 * ctx->somnec.evlcom.ck2;

  /* sommerfeld integral evaluation uses exp(-jwt), nec uses exp(+jwt), */
  /* hence need conjg(ggrid.dielectric).  conjugate of fields occurs in subroutine */
  /* evlua. */

  ctx->somnec.evlcom.ck1sq=ctx->somnec.evlcom.ck2sq*conj(ctx->ggrid.dielectric);
  ctx->somnec.evlcom.ck1=csqrt(ctx->somnec.evlcom.ck1sq);
  ctx->somnec.evlcom.ck1r=creal(ctx->somnec.evlcom.ck1);
  ctx->somnec.evlcom.tkmag=100.*cabs(ctx->somnec.evlcom.ck1);
  ctx->somnec.evlcom.tsmag=100.*creal(ctx->somnec.evlcom.ck1*conj(ctx->somnec.evlcom.ck1));
  ctx->somnec.evlcom.cksm=ctx->somnec.evlcom.ck2sq/(ctx->somnec.evlcom.ck1sq+ctx->somnec.evlcom.ck2sq);
  ctx->somnec.evlcom.ct1=.5*(ctx->somnec.evlcom.ck1sq-ctx->somnec.evlcom.ck2sq);
  erv=ctx->somnec.evlcom.ck1sq*ctx->somnec.evlcom.ck1sq;
  ezv=ctx->somnec.evlcom.ck2sq*ctx->somnec.evlcom.ck2sq;
  ctx->somnec.evlcom.ct2=.125*(erv-ezv);
  erv *= ctx->somnec.evlcom.ck1sq;
  ezv *= ctx->somnec.evlcom.ck2sq;
  ctx->somnec.evlcom.ct3=.0625*(erv-ezv);

  /* loop over 3 grid regions */
  for(k = 0; k < 3; k++) {
    nr=ctx->ggrid.grid_nx[k];
    nth=ctx->ggrid.grid_ny[k];
    dr=ctx->ggrid.grid_dx[k];
    dth=ctx->ggrid.grid_dy[k];
    r=ctx->ggrid.grid_x0[k]-dr;
    irs=1;
    if(k == 0) {
      r=ctx->ggrid.grid_x0[k];
      irs=2;
    }
    
    /*  loop over r.  (r=sqrt(rho**2 + (z+h)**2)) */
    for(ir = irs-1; ir < nr; ir++) {
      r += dr;
      thet = ctx->ggrid.grid_y0[k]-dth;
      
      /* loop over theta.  (theta=atan((z+h)/rho)) */
      for(ith = 0; ith < nth; ith++) {
        thet += dth;
        ctx->somnec.evlcom.rho=r*cos(thet);
        ctx->somnec.evlcom.zph=r*sin(thet);
        if(ctx->somnec.evlcom.rho < 1.e-7)
          ctx->somnec.evlcom.rho=1.e-8;
        if(ctx->somnec.evlcom.zph < 1.e-7)
          ctx->somnec.evlcom.zph=0.;
        
        evaluate_sommerfeld_integrals(ctx, &erv, &ezv, &erh, &eph );
        
        rk=ctx->somnec.evlcom.ck2*r;
        con=-CONST1*r/cmplx(cos(rk),-sin(rk));
        
        switch( k ) {
          case 0:
            ctx->ggrid.table1[ir+ith*11+  0]=erv*con;
            ctx->ggrid.table1[ir+ith*11+110]=ezv*con;
            ctx->ggrid.table1[ir+ith*11+220]=erh*con;
            ctx->ggrid.table1[ir+ith*11+330]=eph*con;
            break;
            
          case 1:
            ctx->ggrid.table2[ir+ith*17+  0]=erv*con;
            ctx->ggrid.table2[ir+ith*17+ 85]=ezv*con;
            ctx->ggrid.table2[ir+ith*17+170]=erh*con;
            ctx->ggrid.table2[ir+ith*17+255]=eph*con;
            break;
            
          case 2:
            ctx->ggrid.table3[ir+ith*9+  0]=erv*con;
            ctx->ggrid.table3[ir+ith*9+ 72]=ezv*con;
            ctx->ggrid.table3[ir+ith*9+144]=erh*con;
            ctx->ggrid.table3[ir+ith*9+216]=eph*con;
            
        } /* switch( k ) */
      } /* for( ith = 0; ith < nth; ith++ ) */
    } /* for( ir = irs-1; ir < nr; ir++) */
  } /* for( k = 0; k < 3; k++) */

  /* fill grid 1 for r equal to zero. */
  cl2=-CONST4*(ctx->ggrid.dielectric-1.)/(ctx->ggrid.dielectric+1.);
  cl1=cl2/(ctx->ggrid.dielectric+1.);
  ezv=ctx->ggrid.dielectric*cl1;
  thet=-dth;
  nth=ctx->ggrid.grid_ny[0];

  for( ith = 0; ith < nth; ith++ )
  {
  thet += dth;
  if( (ith+1) != nth )
  {
    tfac2=cos(thet);
    tfac1=(1.-sin(thet))/tfac2;
    tfac2=tfac1/tfac2;
    erv=ctx->ggrid.dielectric*cl1*tfac1;
    erh=cl1*(tfac2-1.)+cl2;
    eph=cl1*tfac2-cl2;
  }
  else
  {
    erv=0.;
    erh=cl2-.5*cl1;
    eph=-erh;
  }

  ctx->ggrid.table1[0+ith*11+  0]=erv;
  ctx->ggrid.table1[0+ith*11+110]=ezv;
  ctx->ggrid.table1[0+ith*11+220]=erh;
  ctx->ggrid.table1[0+ith*11+330]=eph;
  }

  get_time_ms(ctx, &tim);
  tim -= tst;

  return;
}

/*-----------------------------------------------------------------------*/

/* bessel evaluates the zero-order bessel function */
/* and its derivative for complex argument z. */
void bessel(context_t *ctx, complex double z, complex double *j0, complex double *j0p )
{
  int k, ib;
  double zms;
  complex double p0z, p1z, q0z, q1z, zi, zi2, zk, cz, sz, j0x=CPLX_00, j0px=CPLX_00;

  /* initialization of constants */
  if( ! ctx->somnec.bessel.init )
  {
	int i;
	for( k = 1; k <= 25; k++ )
	{
	  i = k-1;
	  ctx->somnec.bessel.a1[i]=-.25/(k*k);
	  ctx->somnec.bessel.a2[i]=1.0/(k+1.0);
	}

	for( i = 1; i <= 101; i++ )
	{
	  double tst = 1.0;
	  for( k = 0; k < 24; k++ )
	  {
		ctx->somnec.bessel.init = k;
		tst *= -i*ctx->somnec.bessel.a1[k];
		if( tst < 1.0e-6 )
		  break;
	  }

	  ctx->somnec.bessel.m[i-1] = ctx->somnec.bessel.init+1;
	} /* for( i = 1; i<= 101; i++ ) */

	ctx->somnec.bessel.init = true;
  } /* if(init == 0) */

  zms=creal(z*conj(z));
  if(zms <= 1.e-12)
  {
	*j0=CPLX_10;
	*j0p=-.5*z;
	return;
  }

  ib=0;
  if(zms <= 37.21)
  {
	if(zms > 36.)
	  ib=1;

	/* series expansion */
	int iz=(int)zms;
	int miz=ctx->somnec.bessel.m[iz];
	*j0=CPLX_10;
	*j0p=*j0;
	zk=*j0;
	zi=z*z;

	for( k = 0; k < miz; k++ )
	{
	  zk *= ctx->somnec.bessel.a1[k]*zi;
	  *j0 += zk;
	  *j0p += ctx->somnec.bessel.a2[k]*zk;
	}
	*j0p *= -.5*z;

	if(ib == 0)
	  return;

	j0x=*j0;
	j0px=*j0p;
  }

  /* asymptotic expansion */
  zi=1./z;
  zi2=zi*zi;
  p0z=1.+(P20*zi2-P10)*zi2;
  p1z=1.+(P11-P21*zi2)*zi2;
  q0z=(Q20*zi2-Q10)*zi;
  q1z=(Q11-Q21*zi2)*zi;
  zk=cexp(CPLX_01*(z-POF));
  zi2=1./zk;
  cz=.5*(zk+zi2);
  sz=CPLX_01*.5*(zi2-zk);
  zk=C3*csqrt(zi);
  *j0=zk*(p0z*cz-q0z*sz);
  *j0p=-zk*(p1z*sz+q1z*cz);

  if(ib == 0)
	return;

  zms=cos((sqrt(zms)-6.)*PI10);
  *j0=.5*(j0x*(1.+zms)+ *j0*(1.-zms));
  *j0p=.5*(j0px*(1.+zms)+ *j0p*(1.-zms));

  return;
}

/*-----------------------------------------------------------------------*/

/* evlua controls the integration contour in the complex */
/* lambda plane for evaluation of the sommerfeld integrals */
/* Formerly nec2c: evlua */
void evaluate_sommerfeld_integrals(context_t *ctx, complex double *erv, complex double *ezv,
	complex double *erh, complex double *eph )
{
  int i, jump;

  ctx->somnec.evlua.del=ctx->somnec.evlcom.zph;
  if( ctx->somnec.evlcom.rho > ctx->somnec.evlua.del )
	ctx->somnec.evlua.del=ctx->somnec.evlcom.rho;

  if(ctx->somnec.evlcom.zph >= 2.*ctx->somnec.evlcom.rho)
  {
	/* bessel function form of sommerfeld integrals */
	ctx->somnec.evlcom.jh=0;
	ctx->somnec.cntour.a=CPLX_00;
	ctx->somnec.evlua.del=1./ctx->somnec.evlua.del;

	if( ctx->somnec.evlua.del > ctx->somnec.evlcom.tkmag)
	{
	  ctx->somnec.cntour.b=cmplx(.1*ctx->somnec.evlcom.tkmag,-.1*ctx->somnec.evlcom.tkmag);
	  romberg_integrate_1d(ctx,6,ctx->somnec.evlua.sum,2);
	  ctx->somnec.cntour.a=ctx->somnec.cntour.b;
	  ctx->somnec.cntour.b=cmplx(ctx->somnec.evlua.del,-ctx->somnec.evlua.del);
	  romberg_integrate_1d (ctx,6,ctx->somnec.evlua.ans,2);
	  for( i = 0; i < 6; i++ )
		ctx->somnec.evlua.sum[i] += ctx->somnec.evlua.ans[i];
	}
	else
	{
	  ctx->somnec.cntour.b=cmplx(ctx->somnec.evlua.del,-ctx->somnec.evlua.del);
	  romberg_integrate_1d(ctx,6,ctx->somnec.evlua.sum,2);
	}

	ctx->somnec.evlua.delta=PTP*ctx->somnec.evlua.del;
	shanks_integration(ctx,ctx->somnec.cntour.b,ctx->somnec.evlua.delta,ctx->somnec.evlua.ans,6,ctx->somnec.evlua.sum,0,ctx->somnec.cntour.b,ctx->somnec.cntour.b);
	ctx->somnec.evlua.ans[5] *= ctx->somnec.evlcom.ck1;

	/* conjugate since nec uses exp(+jwt) */
	*erv=conj(ctx->somnec.evlcom.ck1sq*ctx->somnec.evlua.ans[2]);
	*ezv=conj(ctx->somnec.evlcom.ck1sq*(ctx->somnec.evlua.ans[1]+ctx->somnec.evlcom.ck2sq*ctx->somnec.evlua.ans[4]));
	*erh=conj(ctx->somnec.evlcom.ck2sq*(ctx->somnec.evlua.ans[0]+ctx->somnec.evlua.ans[5]));
	*eph=-conj(ctx->somnec.evlcom.ck2sq*(ctx->somnec.evlua.ans[3]+ctx->somnec.evlua.ans[5]));

	return;

  } /* if(zph >= 2.*rho) */

  /* hankel function form of sommerfeld integrals */
  ctx->somnec.evlcom.jh=1;
  ctx->somnec.evlua.cp1=cmplx(0.0,.4*ctx->somnec.evlcom.ck2);
  ctx->somnec.evlua.cp2=cmplx(.6*ctx->somnec.evlcom.ck2,-.2*ctx->somnec.evlcom.ck2);
  ctx->somnec.evlua.cp3=cmplx(1.02*ctx->somnec.evlcom.ck2,-.2*ctx->somnec.evlcom.ck2);
  ctx->somnec.cntour.a=ctx->somnec.evlua.cp1;
  ctx->somnec.cntour.b=ctx->somnec.evlua.cp2;
  romberg_integrate_1d(ctx,6,ctx->somnec.evlua.sum,2);
  ctx->somnec.cntour.a=ctx->somnec.evlua.cp2;
  ctx->somnec.cntour.b=ctx->somnec.evlua.cp3;
  romberg_integrate_1d(ctx,6,ctx->somnec.evlua.ans,2);

  for( i = 0; i < 6; i++ )
	ctx->somnec.evlua.sum[i]=-(ctx->somnec.evlua.sum[i]+ctx->somnec.evlua.ans[i]);

  /* path from imaginary axis to -infinity */
  if(ctx->somnec.evlcom.zph > .001*ctx->somnec.evlcom.rho)
	ctx->somnec.evlua.slope=ctx->somnec.evlcom.rho/ctx->somnec.evlcom.zph;
  else
	ctx->somnec.evlua.slope=1000.;

  ctx->somnec.evlua.del=PTP/ctx->somnec.evlua.del;
  ctx->somnec.evlua.delta=cmplx(-1.0,ctx->somnec.evlua.slope)*ctx->somnec.evlua.del/sqrt(1.+ctx->somnec.evlua.slope*ctx->somnec.evlua.slope);
  ctx->somnec.evlua.delta2=-conj(ctx->somnec.evlua.delta);
  shanks_integration(ctx,ctx->somnec.evlua.cp1,ctx->somnec.evlua.delta,ctx->somnec.evlua.ans,6,ctx->somnec.evlua.sum,0,ctx->somnec.evlua.bk,ctx->somnec.evlua.bk);
  ctx->somnec.evlua.rmis=ctx->somnec.evlcom.rho*(creal(ctx->somnec.evlcom.ck1)-ctx->somnec.evlcom.ck2);

  jump = false;
  if( (ctx->somnec.evlua.rmis >= 2.*ctx->somnec.evlcom.ck2) && (ctx->somnec.evlcom.rho >= 1.e-10) )
  {
	if(ctx->somnec.evlcom.zph >= 1.e-10)
	{
	  ctx->somnec.evlua.bk=cmplx(-ctx->somnec.evlcom.zph,ctx->somnec.evlcom.rho)*(ctx->somnec.evlcom.ck1-ctx->somnec.evlua.cp3);
	  ctx->somnec.evlua.rmis=-creal(ctx->somnec.evlua.bk)/fabs(cimag(ctx->somnec.evlua.bk));
	  if(ctx->somnec.evlua.rmis > 4.*ctx->somnec.evlcom.rho/ctx->somnec.evlcom.zph)
		jump = true;
	}

	if( ! jump )
	{
	  /* integrate up between branch cuts, then to + infinity */
	  ctx->somnec.evlua.cp1=ctx->somnec.evlcom.ck1-(.1+I*0.2);
	  ctx->somnec.evlua.cp2=ctx->somnec.evlua.cp1+.2;
	  ctx->somnec.evlua.bk=cmplx(0.,ctx->somnec.evlua.del);
	  shanks_integration(ctx,ctx->somnec.evlua.cp1,ctx->somnec.evlua.bk,ctx->somnec.evlua.sum,6,ctx->somnec.evlua.ans,0,ctx->somnec.evlua.bk,ctx->somnec.evlua.bk);
	  ctx->somnec.cntour.a=ctx->somnec.evlua.cp1;
	  ctx->somnec.cntour.b=ctx->somnec.evlua.cp2;
	  romberg_integrate_1d(ctx,6,ctx->somnec.evlua.ans,1);
	  for( i = 0; i < 6; i++ )
		ctx->somnec.evlua.ans[i] -= ctx->somnec.evlua.sum[i];

	  shanks_integration(ctx,ctx->somnec.evlua.cp3,ctx->somnec.evlua.bk,ctx->somnec.evlua.sum,6,ctx->somnec.evlua.ans,0,ctx->somnec.evlua.bk,ctx->somnec.evlua.bk);
	  shanks_integration(ctx,ctx->somnec.evlua.cp2,ctx->somnec.evlua.delta2,ctx->somnec.evlua.ans,6,ctx->somnec.evlua.sum,0,ctx->somnec.evlua.bk,ctx->somnec.evlua.bk);
	}

	jump = true;

  } /* if( (rmis >= 2.*ck2) || (rho >= 1.e-10) ) */
  else
	jump = false;

  if( ! jump )
  {
	/* integrate below branch points, then to + infinity */
	for( i = 0; i < 6; i++ )
	  ctx->somnec.evlua.sum[i]=-ctx->somnec.evlua.ans[i];

	ctx->somnec.evlua.rmis=creal(ctx->somnec.evlcom.ck1)*1.01;
	if( (ctx->somnec.evlcom.ck2+1.) > ctx->somnec.evlua.rmis )
	  ctx->somnec.evlua.rmis=ctx->somnec.evlcom.ck2+1.;

	ctx->somnec.evlua.bk=cmplx(ctx->somnec.evlua.rmis,.99*cimag(ctx->somnec.evlcom.ck1));
	ctx->somnec.evlua.delta=ctx->somnec.evlua.bk-ctx->somnec.evlua.cp3;
	ctx->somnec.evlua.delta *= ctx->somnec.evlua.del/cabs(ctx->somnec.evlua.delta);
	shanks_integration(ctx,ctx->somnec.evlua.cp3,ctx->somnec.evlua.delta,ctx->somnec.evlua.ans,6,ctx->somnec.evlua.sum,1,ctx->somnec.evlua.bk,ctx->somnec.evlua.delta2);

  } /* if( ! jump ) */

  ctx->somnec.evlua.ans[5] *= ctx->somnec.evlcom.ck1;

  /* conjugate since nec uses exp(+jwt) */
  *erv=conj(ctx->somnec.evlcom.ck1sq*ctx->somnec.evlua.ans[2]);
  *ezv=conj(ctx->somnec.evlcom.ck1sq*(ctx->somnec.evlua.ans[1]+ctx->somnec.evlcom.ck2sq*ctx->somnec.evlua.ans[4]));
  *erh=conj(ctx->somnec.evlcom.ck2sq*(ctx->somnec.evlua.ans[0]+ctx->somnec.evlua.ans[5]));
  *eph=-conj(ctx->somnec.evlcom.ck2sq*(ctx->somnec.evlua.ans[3]+ctx->somnec.evlua.ans[5]));

  return;
}

/*-----------------------------------------------------------------------*/

/* fbar is sommerfeld attenuation function for numerical distance p */
/* Formerly nec2c: fbar */
void norton_attenuation_factor(context_t *ctx, complex double p, complex double *fbar )
{
  int i, minus;
  double tms, sms;
  complex double z, zs, sum, pow, term;

  z= CPLX_01* csqrt( p);
  if( cabs( z) <= 3.)
  {
	/* series expansion */
	zs= z* z;
	sum= z;
	pow= z;

	for( i = 1; i <= 100; i++ )
	{
	  pow= -pow* zs/ (double)i;
	  term= pow/(2.* i+1.);
	  sum= sum+ term;
	  tms= creal( term* conj( term));
	  sms= creal( sum* conj( sum));
	  if( tms/sms < ACCS)
		break;
	}

	*fbar=1.-(1.- sum* TOSP)* z* cexp( zs)* SP;
	return;

  } /* if( cabs( z) <= 3.) */

  /* asymptotic expansion */
  if( creal( z) < 0.)
  {
	minus=1;
	z= -z;
  }
  else
	minus=0;

  zs=.5/( z* z);
  sum=CPLX_00;
  term=CPLX_10;

  for( i = 1; i <= 6; i++ )
  {
	term = -term*(2.*i -1.)* zs;
	sum += term;
  }

  if( minus == 1)
	sum -= 2.* SP* z* cexp( z* z);
  *fbar= -sum;
}

/*-----------------------------------------------------------------------*/

/* gshank integrates the 6 sommerfeld integrals from start to */
/* infinity (until convergence) in lambda.  at the break point, bk, */
/* the step increment may be changed from dela to delb.  shank's */
/* algorithm to accelerate convergence of a slowly converging series */
/* is used */
/* Formerly nec2c: gshank */
int shanks_integration(context_t *ctx, complex double start, complex double dela,
	complex double *sum, int nans, complex double *seed,
	int ibk, complex double bk, complex double delb )
{
  int ibx, j, i, jm, intx, inx, brk=0;
  complex double a1, a2, as1, as2, del, aa;
  complex double q1[6][20], q2[6][20], ans1[6], ans2[6];

  ctx->somnec.gshank.rbk=creal(bk);
  del=dela;
  if(ibk == 0)
	ibx=1;
  else
	ibx=0;

  for( i = 0; i < nans; i++ )
    ans2[i] = seed[i];
  ctx->somnec.cntour.b = start;

  for( intx = 1; intx <= MAXH; intx++ )
  {
    inx=intx-1;
    ctx->somnec.cntour.a=ctx->somnec.cntour.b;
    ctx->somnec.cntour.b += del;

    if( (ibx == 0) && (creal(ctx->somnec.cntour.b) >= ctx->somnec.gshank.rbk) )
    {
      /* hit break point.  reset seed and start over. */
      ibx=1;
      ctx->somnec.cntour.b=bk;
      del=delb;
      romberg_integrate_1d(ctx,nans,sum,2);
      for( i = 0; i < nans; i++ )
        ans2[i] += sum[i];
      intx = 0;
      continue;
    } /* if( (ibx == 0) && (creal(b) >= rbk) ) */

    romberg_integrate_1d(ctx,nans,sum,2);
    for( i = 0; i < nans; i++ )
      ans1[i] = ans2[i]+sum[i];
    ctx->somnec.cntour.a=ctx->somnec.cntour.b;
    ctx->somnec.cntour.b += del;
    if( (ibx == 0) && (creal(ctx->somnec.cntour.b) >= ctx->somnec.gshank.rbk) )
    {
      /* hit break point.  reset seed and start over. */
      ibx=2;
      ctx->somnec.cntour.b=bk;
      del=delb;
      romberg_integrate_1d(ctx,nans,sum,2);
      for( i = 0; i < nans; i++ )
        ans2[i] += sum[i];
      intx = 0;
      continue;
    } /* if( (ibx == 0) && (creal(b) >= rbk) ) */

    romberg_integrate_1d(ctx,nans,sum,2);
    for( i = 0; i < nans; i++ )
      ans2[i]=ans1[i]+sum[i];

    ctx->somnec.gshank.den=0.;
    for( i = 0; i < nans; i++ )
    {
	  as1=ans1[i];
	  as2=ans2[i];

	  if(intx >= 2)
	  {
		for( j = 1; j < intx; j++ )
		{
		  jm=j-1;
		  aa=q2[i][jm];
		  a1=q1[i][jm]+as1-2.*aa;

		  if( (creal(a1) != 0.) || (cimag(a1) != 0.) )
		  {
			a2=aa-q1[i][jm];
			a1=q1[i][jm]-a2*a2/a1;
		  }
		  else
			a1=q1[i][jm];

		  a2=aa+as2-2.*as1;
		  if( (creal(a2) != 0.) || (cimag(a2) != 0.) )
			a2=aa-(as1-aa)*(as1-aa)/a2;
		  else
			a2=aa;

		  q1[i][jm]=as1;
		  q2[i][jm]=as2;
		  as1=a1;
		  as2=a2;
		} /* for( j = 1; i < intx; i++ ) */

	  } /* if(intx >= 2) */

	  q1[i][intx-1]=as1;
	  q2[i][intx-1]=as2;
	  ctx->somnec.gshank.amg=fabs(creal(as2))+fabs(cimag(as2));
	  if(ctx->somnec.gshank.amg > ctx->somnec.gshank.den)
		ctx->somnec.gshank.den=ctx->somnec.gshank.amg;

	} /* for( i = 0; i < nans; i++ ) */

	ctx->somnec.gshank.denm=1.e-3*ctx->somnec.gshank.den*CRIT;
	jm=intx-3;
	if(jm < 1)
	  jm=1;

	for( j = jm-1; j < intx; j++ )
	{
	  brk = false;
	  for( i = 0; i < nans; i++ )
	  {
		a1=q2[i][j];
		ctx->somnec.gshank.den=(fabs(creal(a1))+fabs(cimag(a1)))*CRIT;
		if(ctx->somnec.gshank.den < ctx->somnec.gshank.denm)
		  ctx->somnec.gshank.den=ctx->somnec.gshank.denm;
		a1=q1[i][j]-a1;
		ctx->somnec.gshank.amg=fabs(creal(a1)+fabs(cimag(a1)));
		if(ctx->somnec.gshank.amg > ctx->somnec.gshank.den)
		{
		  brk = true;
		  break;
		}

	  } /* for( i = 0; i < nans; i++ ) */

	  if( brk ) break;

	} /* for( j = jm-1; j < intx; j++ ) */

	if( ! brk )
	{
	  for( i = 0; i < nans; i++ )
		sum[i]=.5*(q1[i][inx]+q2[i][inx]);
	  return 0;
	}

  } /* for( intx = 1; intx <= maxh; intx++ ) */

  /* No convergence */
  add_error(ctx, &ctx->errors, "No convergence in shanks_integration()", FATAL);
  return -1;
}

/*-----------------------------------------------------------------------*/

/* hankel evaluates hankel function of the first kind,   */
/* order zero, and its derivative for complex argument z */
int hankel(context_t *ctx, complex double z, complex double *h0, complex double *h0p )
{
  int k, ib;
  complex double clogz, j0, j0p, p0z, p1z, q0z, q1z, y0=CPLX_00, y0p=CPLX_00, zi, zi2, zk;

  /* initialization of constants */
  if( ! ctx->somnec.hankel.init )
  {
	int i;
	ctx->somnec.hankel.psi=-GAMMA;
	for( k = 1; k <= 25; k++ )
	{
	  i = k-1;
	  ctx->somnec.hankel.a1[i]=-.25/(k*k);
	  ctx->somnec.hankel.a2[i]=1.0/(k+1.0);
	  ctx->somnec.hankel.psi += 1.0/k;
	  ctx->somnec.hankel.a3[i]=ctx->somnec.hankel.psi+ctx->somnec.hankel.psi;
	  ctx->somnec.hankel.a4[i]=(ctx->somnec.hankel.psi+ctx->somnec.hankel.psi+1.0/(k+1.0))/(k+1.0);
	}

	for( i = 1; i <= 101; i++ )
	{
	  ctx->somnec.hankel.tst=1.0;
	  for( k = 0; k < 24; k++ )
	  {
		ctx->somnec.hankel.init = k;
		ctx->somnec.hankel.tst *= -i*ctx->somnec.hankel.a1[k];
		if(ctx->somnec.hankel.tst*ctx->somnec.hankel.a3[k] < 1.e-6)
		  break;
	  }
	  ctx->somnec.hankel.m[i-1]=ctx->somnec.hankel.init+1;
	}

	ctx->somnec.hankel.init = true;

  } /* if( ! init ) */

  ctx->somnec.hankel.zms=creal(z*conj(z));
  if(ctx->somnec.hankel.zms == 0.) {
    add_error(ctx, &ctx->errors, "Hankel function invalid for z=0", FATAL);
    return -1;
  }

  ib=0;
  if(ctx->somnec.hankel.zms <= 16.81)
  {
	if(ctx->somnec.hankel.zms > 16.)
	  ib=1;

	/* series expansion */
	int iz=(int)ctx->somnec.hankel.zms;
	int miz=ctx->somnec.hankel.m[iz];
	j0=CPLX_10;
	j0p=j0;
	y0=CPLX_00;
	y0p=y0;
	zk=j0;
	zi=z*z;

	for( k = 0; k < miz; k++ )
	{
	  zk *= ctx->somnec.hankel.a1[k]*zi;
	  j0 += zk;
	  j0p += ctx->somnec.hankel.a2[k]*zk;
	  y0 += ctx->somnec.hankel.a3[k]*zk;
	  y0p += ctx->somnec.hankel.a4[k]*zk;
	}

	j0p *= -.5*z;
	clogz=clog(.5*z);
	y0=(2.*j0*clogz-y0)/PI+C2;
	y0p=(2./z+2.*j0p*clogz+.5*y0p*z)/PI+C1*z;
	*h0=j0+CPLX_01*y0;
	*h0p=j0p+CPLX_01*y0p;

	if(ib == 0)
	  return 0;

	y0=*h0;
	y0p=*h0p;

  } /* if(zms <= 16.81) */

  /* asymptotic expansion */
  zi=1./z;
  zi2=zi*zi;
  p0z=1.+(P20*zi2-P10)*zi2;
  p1z=1.+(P11-P21*zi2)*zi2;
  q0z=(Q20*zi2-Q10)*zi;
  q1z=(Q11-Q21*zi2)*zi;
  zk=cexp(CPLX_01*(z-POF))*csqrt(zi)*C3;
  *h0=zk*(p0z+CPLX_01*q0z);
  *h0p=CPLX_01*zk*(p1z+CPLX_01*q1z);

  if(ib == 0)
    return 0;

  ctx->somnec.hankel.zms=cos((sqrt(ctx->somnec.hankel.zms)-4.)*31.41592654);
  *h0=.5*(y0*(1.+ctx->somnec.hankel.zms)+ *h0*(1.-ctx->somnec.hankel.zms));
  *h0p=.5*(y0p*(1.+ctx->somnec.hankel.zms)+ *h0p*(1.-ctx->somnec.hankel.zms));

  return 0;
}

/*-----------------------------------------------------------------------*/

/* compute integration parameter xlam=lambda from parameter t. */
/* Formerly nec2c: lambda */
void sommerfeld_lambda(context_t *ctx, double t, complex double *xlam, complex double *dxlam )
{
  *dxlam=ctx->somnec.cntour.b-ctx->somnec.cntour.a;
  *xlam=ctx->somnec.cntour.a+*dxlam*t;
  return;
}

/*-----------------------------------------------------------------------*/

/* rom1 integrates the 6 sommerfeld integrals from a to b in lambda. */
/* the method of variable interval width romberg integration is used. */
/* Formerly nec2c: rom1 */
void romberg_integrate_1d(context_t *ctx, int n, complex double *sum, int nx )
{
  int jump, lstep, nogo, i, ns, nt;

  lstep=0;
  ctx->somnec.rom1.z=0.;
  ctx->somnec.rom1.ze=1.;
  ctx->somnec.rom1.s=1;
  ctx->somnec.rom1.ep=ctx->somnec.rom1.s/(1.e4*NM);
  ctx->somnec.rom1.zend=ctx->somnec.rom1.ze-ctx->somnec.rom1.ep;
  for( i = 0; i < n; i++ )
    sum[i]=CPLX_00;
  ns=nx;
  nt=0;
  sommerfeld_asymptotic(ctx, ctx->somnec.rom1.z, ctx->somnec.rom1.g1);

  jump = false;
  while( true )
  {
    if( ! jump )
    {
      ctx->somnec.rom1.dz=ctx->somnec.rom1.s/ns;
      if( (ctx->somnec.rom1.z+ctx->somnec.rom1.dz) > ctx->somnec.rom1.ze )
      {
        ctx->somnec.rom1.dz=ctx->somnec.rom1.ze-ctx->somnec.rom1.z;
        if( ctx->somnec.rom1.dz <= ctx->somnec.rom1.ep )
          return;
      }

      ctx->somnec.rom1.dzot=ctx->somnec.rom1.dz*.5;
      sommerfeld_asymptotic(ctx, ctx->somnec.rom1.z+ctx->somnec.rom1.dzot, ctx->somnec.rom1.g3);
      sommerfeld_asymptotic(ctx, ctx->somnec.rom1.z+ctx->somnec.rom1.dz, ctx->somnec.rom1.g5);

    } /* if( ! jump ) */

    nogo=false;
    for( i = 0; i < n; i++ )
    {
      ctx->somnec.rom1.t00=(ctx->somnec.rom1.g1[i]+ctx->somnec.rom1.g5[i])*ctx->somnec.rom1.dzot;
      ctx->somnec.rom1.t01[i]=(ctx->somnec.rom1.t00+ctx->somnec.rom1.dz*ctx->somnec.rom1.g3[i])*.5;
      ctx->somnec.rom1.t10[i]=(4.*ctx->somnec.rom1.t01[i]-ctx->somnec.rom1.t00)/3.;

      /* test convergence of 3 point romberg result */
      test_romberg_convergence(ctx, creal(ctx->somnec.rom1.t01[i]), creal(ctx->somnec.rom1.t10[i]), &ctx->somnec.rom1.tr, cimag(ctx->somnec.rom1.t01[i]), cimag(ctx->somnec.rom1.t10[i]), &ctx->somnec.rom1.ti, 0. );
      if( (ctx->somnec.rom1.tr > CRIT) || (ctx->somnec.rom1.ti > CRIT) )
        nogo = true;
    }

    if( ! nogo )
    {
      for( i = 0; i < n; i++ )
        sum[i] += ctx->somnec.rom1.t10[i];
      nt += 2;

      ctx->somnec.rom1.z += ctx->somnec.rom1.dz;
      if(ctx->somnec.rom1.z > ctx->somnec.rom1.zend)
        return;

      for( i = 0; i < n; i++ )
        ctx->somnec.rom1.g1[i]=ctx->somnec.rom1.g5[i];

      if( (nt >= NTS) && (ns > nx) )
      {
        ns=ns/2;
        nt=1;
      }

      jump = false;
      continue;

    } /* if( ! nogo ) */

    sommerfeld_asymptotic(ctx, ctx->somnec.rom1.z+ctx->somnec.rom1.dz*.25, ctx->somnec.rom1.g2);
    sommerfeld_asymptotic(ctx, ctx->somnec.rom1.z+ctx->somnec.rom1.dz*.75, ctx->somnec.rom1.g4);
    nogo=false;
    for( i = 0; i < n; i++ )
    {
      ctx->somnec.rom1.t02=(ctx->somnec.rom1.t01[i]+ctx->somnec.rom1.dzot*(ctx->somnec.rom1.g2[i]+ctx->somnec.rom1.g4[i]))*.5;
      ctx->somnec.rom1.t11=(4.*ctx->somnec.rom1.t02-ctx->somnec.rom1.t01[i])/3.;
      ctx->somnec.rom1.t20[i]=(16.*ctx->somnec.rom1.t11-ctx->somnec.rom1.t10[i])/15.;

      /* test convergence of 5 point romberg result */
      test_romberg_convergence(ctx, creal(ctx->somnec.rom1.t11), creal(ctx->somnec.rom1.t20[i]), &ctx->somnec.rom1.tr, cimag(ctx->somnec.rom1.t11), cimag(ctx->somnec.rom1.t20[i]), &ctx->somnec.rom1.ti, 0. );
      if( (ctx->somnec.rom1.tr > CRIT) || (ctx->somnec.rom1.ti > CRIT) )
        nogo = true;
    }

    if( ! nogo )
    {
      for( i = 0; i < n; i++ )
        sum[i] += ctx->somnec.rom1.t20[i];

      nt++;
      ctx->somnec.rom1.z += ctx->somnec.rom1.dz;
      if(ctx->somnec.rom1.z > ctx->somnec.rom1.zend)
        return;

      for( i = 0; i < n; i++ )
        ctx->somnec.rom1.g1[i]=ctx->somnec.rom1.g5[i];

      if( (nt >= NTS) && (ns > nx) )
      {
        ns=ns/2;
        nt=1;
      }

      jump = false;
      continue;

    } /* if( ! nogo ) */

    nt=0;
    if(ns < NM)
    {
      ns *= 2;
      ctx->somnec.rom1.dz=ctx->somnec.rom1.s/ns;
      ctx->somnec.rom1.dzot=ctx->somnec.rom1.dz*.5;

      for( i = 0; i < n; i++ )
      {
        ctx->somnec.rom1.g5[i]=ctx->somnec.rom1.g3[i];
        ctx->somnec.rom1.g3[i]=ctx->somnec.rom1.g2[i];
      }

      jump = true;
      continue;

    } /* if(ns < nm) */

    if( ! lstep )
    {
      lstep = true;
      sommerfeld_lambda(ctx, ctx->somnec.rom1.z, &ctx->somnec.rom1.t00, &ctx->somnec.rom1.t11 );
    }

    for( i = 0; i < n; i++ )
      sum[i] += ctx->somnec.rom1.t20[i];

    nt++;
    ctx->somnec.rom1.z += ctx->somnec.rom1.dz;
    if(ctx->somnec.rom1.z > ctx->somnec.rom1.zend)
      return;

    for( i = 0; i < n; i++ )
      ctx->somnec.rom1.g1[i]=ctx->somnec.rom1.g5[i];

    jump = false;

  } /* while( true ) */

}

/*-----------------------------------------------------------------------*/

/* saoa computes the integrand for each of the 6 sommerfeld */
/* integrals for source and observer above ground */
/* Formerly nec2c: saoa */
void sommerfeld_asymptotic(context_t *ctx, double t, complex double *ans)
{
  double xlr;
  complex double xl, dxl, cgam1, cgam2, b0, b0p, com, dgam, den1, den2;

  sommerfeld_lambda(ctx, t, &xl, &dxl);
  if( ctx->somnec.evlcom.jh == 0 )
  {
    /* bessel function form */
    bessel(ctx, xl*ctx->somnec.evlcom.rho, &b0, &b0p);
    b0  *=2.;
    b0p *=2.;
    cgam1=csqrt(xl*xl-ctx->somnec.evlcom.ck1sq);
    cgam2=csqrt(xl*xl-ctx->somnec.evlcom.ck2sq);
    if(creal(cgam1) == 0.)
      cgam1=cmplx(0.,-fabs(cimag(cgam1)));
    if(creal(cgam2) == 0.)
      cgam2=cmplx(0.,-fabs(cimag(cgam2)));
  }
  else
  {
    /* hankel function form */
    hankel(ctx, xl*ctx->somnec.evlcom.rho, &b0, &b0p);
    com=xl-ctx->somnec.evlcom.ck1;
    cgam1=csqrt(xl+ctx->somnec.evlcom.ck1)*csqrt(com);
    if(creal(com) < 0. && cimag(com) >= 0.)
      cgam1=-cgam1;
    com=xl-ctx->somnec.evlcom.ck2;
    cgam2=csqrt(xl+ctx->somnec.evlcom.ck2)*csqrt(com);
    if(creal(com) < 0. && cimag(com) >= 0.)
      cgam2=-cgam2;
  }

  xlr=creal(xl*conj(xl));
  if(xlr >= ctx->somnec.evlcom.tsmag)
  {
    double sign;
    if(cimag(xl) >= 0.)
    {
      xlr=creal(xl);
      if(xlr >= ctx->somnec.evlcom.ck2)
      {
        if(xlr <= ctx->somnec.evlcom.ck1r)
          dgam=cgam2-cgam1;
        else
        {
          sign=1.;
          dgam=1./(xl*xl);
          dgam=sign*((ctx->somnec.evlcom.ct3*dgam+ctx->somnec.evlcom.ct2)*dgam+ctx->somnec.evlcom.ct1)/xl;
        }
      }
      else
      {
        sign=-1.;
        dgam=1./(xl*xl);
        dgam=sign*((ctx->somnec.evlcom.ct3*dgam+ctx->somnec.evlcom.ct2)*dgam+ctx->somnec.evlcom.ct1)/xl;
      } /* if(xlr >= ck2) */

    } /* if(cimag(xl) >= 0.) */
    else
    {
      sign=1.;
      dgam=1./(xl*xl);
      dgam=sign*((ctx->somnec.evlcom.ct3*dgam+ctx->somnec.evlcom.ct2)*dgam+ctx->somnec.evlcom.ct1)/xl;
    }

  } /* if(xlr < tsmag) */
  else
    dgam=cgam2-cgam1;

  den2=ctx->somnec.evlcom.cksm*dgam/(cgam2*(ctx->somnec.evlcom.ck1sq*cgam2+ctx->somnec.evlcom.ck2sq*cgam1));
  den1=1./(cgam1+cgam2)-ctx->somnec.evlcom.cksm/cgam2;
  com=dxl*xl*cexp(-cgam2*ctx->somnec.evlcom.zph);
  ans[5]=com*b0*den1/ctx->somnec.evlcom.ck1;
  com *= den2;

  if(ctx->somnec.evlcom.rho != 0.)
  {
    b0p=b0p/ctx->somnec.evlcom.rho;
    ans[0]=-com*xl*(b0p+b0*xl);
    ans[3]=com*xl*b0p;
  }
  else
  {
    ans[0]=-com*xl*xl*.5;
    ans[3]=ans[0];
  }

  ans[1]=com*cgam2*cgam2*b0;
  ans[2]=-ans[3]*cgam2*ctx->somnec.evlcom.rho;
  ans[4]=com*b0;

  return;
}
