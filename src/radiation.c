/******************************************************************************
 * radiation.c
 *
 * radiation.c computes far-zone radiated electric fields from the current
 * distribution on wires, optionally including ground reflections and cliff
 * geometries. It assembles `E_theta` and `E_phi` components by integrating
 * segment contributions with phase and orientation, applying reflection
 * coefficients when ground models are active.
 *
 * Major responsibilities include:
 * - ffld(): Calculate far-field `Eθ`/`Eφ` at observation angles `θ`, `φ`,
 *   summing segment contributions with sine/cosine/current expansions and
 *   appropriate phasing. Ground models adjust fields via reflection terms.
 * - Handle perfect and planar ground, cliff, and radial wire ground screen
 *   cases by computing reflection coefficients based on geometry and medium.
 * - Support image contributions where applicable to account for ground/cliff
 *   effects on the observed fields.
 *
 * Operates on `nec_context_t`, reading geometry (`geometry`), current
 * expansions (`crnt`), and ground parameters (`gnd`) to assemble the far
 * field at requested angles.
 *****************************************************************************/


#include "opennec.h"
#include "radiation.h"
#include "fields.h"
#include "calculations.h"

/* Forward declarations for internal functions */
static void ffld(nec_context_t *ctx, double thet, double phi, complex double *eth, complex double *eph);
static void fflds(nec_context_t *ctx, double rox, double roy, double roz, complex double *scur, complex double *ex, complex double *ey, complex double *ez);
static void gfld(nec_context_t *ctx, double rho, double phi, double rz, complex double *eth, complex double *epi, complex double *erd, complex double ux, int ksymp);

/*-----------------------------------------------------------------------*/

/* ffld calculates the far zone radiated electric fields, */
/* the factor exp(j*k*r)/(r/lamda) not included */
void ffld(nec_context_t *ctx, double thet, double phi,
	complex double *eth, complex double *eph )
{
  int k, i, ip, jump;
  double phx, phy, roz, rozs, thx, thy, thz, rox, roy;
  double tthet=0., darg=0., omega, el, sill, top, bot, a;
  double too, boo, b, c, d, rr, ri, arg, dr, rfl, rrz;
  complex double cix=CPLX_00, ciy=CPLX_00, ciz=CPLX_00;
  complex double exa, ccx=CPLX_00, ccy=CPLX_00, ccz=CPLX_00, cdp;
  complex double zrsin, rrv=CPLX_00, rrh=CPLX_00, rrv1=CPLX_00;
  complex double rrh1=CPLX_00, rrv2=CPLX_00, rrh2=CPLX_00;
  complex double tix, tiy, tiz, zscrn, ex=CPLX_00, ey=CPLX_00, ez=CPLX_00, gx, gy, gz;

  phx= -sin( phi);
  phy= cos( phi);
  roz= cos( thet);
  rozs= roz;
  thx= roz* phy;
  thy= -roz* phx;
  thz= -sin( thet);
  rox= -thz* phy;
  roy= thz* phx;

  jump = FALSE;
  if( ctx->geometry.n != 0)
  {
	/* loop for structure image if any */
	/* calculation of reflection coeffecients */
	for( k = 0; k < ctx->gnd.ksymp; k++ )
	{
	  if( k != 0 )
	  {
		/* for perfect ground */
		if( ctx->gnd.iperf == 1)
		{
		  rrv=-CPLX_10;
		  rrh=-CPLX_10;
		}
		else
		{
		  /* for infinite planar ground */
		  zrsin= csqrt(1.- ctx->gnd.zrati* ctx->gnd.zrati* thz* thz);
		  rrv=-( roz- ctx->gnd.zrati* zrsin)/( roz+ ctx->gnd.zrati* zrsin);
		  rrh=( ctx->gnd.zrati* roz- zrsin)/( ctx->gnd.zrati* roz+ zrsin);

		} /* if( gnd.iperf == 1) */

		/* for the cliff problem, two reflction coefficients calculated */
		if( ctx->gnd.ifar > 1)
		{
		  rrv1= rrv;
		  rrh1= rrh;
		  tthet= tan( thet);

		  if( ctx->gnd.ifar != 4)
		  {
			zrsin= csqrt(1.- ctx->gnd.zrati2* ctx->gnd.zrati2* thz* thz);
			rrv2=-( roz- ctx->gnd.zrati2* zrsin)/( roz+ ctx->gnd.zrati2* zrsin);
			rrh2=( ctx->gnd.zrati2* roz- zrsin)/( ctx->gnd.zrati2* roz+ zrsin);
			darg= -TP*2.* ctx->gnd.ch* roz;
		  }
		} /* if( gnd.ifar > 1) */

		roz= -roz;
		ccx= cix;
		ccy= ciy;
		ccz= ciz;

	  } /* if( k != 0 ) */

	  cix=CPLX_00;
	  ciy=CPLX_00;
	  ciz=CPLX_00;

	  /* loop over structure segments */
	  for( i = 0; i < ctx->geometry.n; i++ )
	  {
		omega=-( rox* ctx->geometry.cab[i]+ roy* ctx->geometry.sab[i]+ roz* ctx->geometry.salp[i]);
		el= PI* ctx->geometry.si[i];
		sill= omega* el;
		top= el+ sill;
		bot= el- sill;

		if( fabs( omega) >= 1.0e-7)
		  a=2.* sin( sill)/ omega;
		else
		  a=(2.- omega* omega* el* el/3.)* el;

		if( fabs( top) >= 1.0e-7)
		  too= sin( top)/ top;
		else
		  too=1.- top* top/6.;

		if( fabs( bot) >= 1.0e-7)
		  boo= sin( bot)/ bot;
		else
		  boo=1.- bot* bot/6.;

		b= el*( boo- too);
		c= el*( boo+ too);
		rr= a* ctx->crnt.air[i]+ b* ctx->crnt.bii[i]+ c* ctx->crnt.cir[i];
		ri= a* ctx->crnt.aii[i]- b* ctx->crnt.bir[i]+ c* ctx->crnt.cii[i];
		arg= TP*( ctx->geometry.x[i]* rox+ ctx->geometry.y[i]* roy+ ctx->geometry.z[i]* roz);

		if( (k != 1) || (ctx->gnd.ifar < 2) )
		{
		  /* summation for far field integral */
		  exa= cmplx( cos( arg), sin( arg))* cmplx( rr, ri);
		  cix= cix+ exa* ctx->geometry.cab[i];
		  ciy= ciy+ exa* ctx->geometry.sab[i];
		  ciz= ciz+ exa* ctx->geometry.salp[i];
		  continue;
		}

		/* calculation of image contribution */
		/* in cliff and ground screen problems */

		/* specular point distance */
		dr= ctx->geometry.z[i]* tthet;

		d= dr* phy+ ctx->geometry.x[i];
		if( ctx->gnd.ifar == 2)
		{
		  if(( ctx->gnd.cl- d) > 0.)
		  {
			rrv= rrv1;
			rrh= rrh1;
		  }
		  else
		  {
			rrv= rrv2;
			rrh= rrh2;
			arg= arg+ darg;
		  }
		} /* if( gnd.ifar == 2) */
		else
		{
		  d= sqrt( d*d + (ctx->geometry.y[i]-dr*phx)*(ctx->geometry.y[i]-dr*phx) );
		  if( ctx->gnd.ifar == 3)
		  {
			if(( ctx->gnd.cl- d) > 0.)
			{
			  rrv= rrv1;
			  rrh= rrh1;
			}
			else
			{
			  rrv= rrv2;
			  rrh= rrh2;
			  arg= arg+ darg;
			}
		  } /* if( gnd.ifar == 3) */
		  else
		  {
			if(( ctx->gnd.scrwl- d) >= 0.)
			{
			  /* radial wire ground screen reflection coefficient */
			  d= d+ ctx->gnd.t2;
			  zscrn= ctx->gnd.t1* d* log( d/ ctx->gnd.t2);
			  zscrn=( zscrn* ctx->gnd.zrati)/( ETA* ctx->gnd.zrati+ zscrn);
			  zrsin= csqrt(1.- zscrn* zscrn* thz* thz);
			  rrv=( roz+ zscrn* zrsin)/(- roz+ zscrn* zrsin);
			  rrh=( zscrn* roz+ zrsin)/( zscrn* roz- zrsin);
			} /* if(( gnd.scrwl- d) < 0.) */
			else
			{
			  if( ctx->gnd.ifar == 4)
			  {
				rrv= rrv1;
				rrh= rrh1;
			  } /* if( gnd.ifar == 4) */
			  else
			  {
				if( ctx->gnd.ifar == 5)
				  d= dr* phy+ ctx->geometry.x[i];

				if(( ctx->gnd.cl- d) > 0.)
				{
				  rrv= rrv1;
				  rrh= rrh1;
				}
				else
				{
				  rrv= rrv2;
				  rrh= rrh2;
				  arg= arg+ darg;
				} /* if(( gnd.cl- d) > 0.) */

			  } /* if( gnd.ifar == 4) */

			} /* if(( gnd.scrwl- d) < 0.) */

		  } /* if( gnd.ifar == 3) */

		} /* if( gnd.ifar == 2) */

		/* contribution of each image segment modified by */
		/* reflection coef, for cliff and ground screen problems */
		exa= cmplx( cos( arg), sin( arg))* cmplx( rr, ri);
		tix= exa* ctx->geometry.cab[i];
		tiy= exa* ctx->geometry.sab[i];
		tiz= exa* ctx->geometry.salp[i];
		cdp=( tix* phx+ tiy* phy)*( rrh- rrv);
		cix= cix+ tix* rrv+ cdp* phx;
		ciy= ciy+ tiy* rrv+ cdp* phy;
		ciz= ciz- tiz* rrv;

	  } /* for( i = 0; i < n; i++ ) */

	  if( k == 0 )
		continue;

	  /* calculation of contribution of structure image for infinite ground */
	  if( ctx->gnd.ifar < 2)
	  {
		cdp=( cix* phx+ ciy* phy)*( rrh- rrv);
		cix= ccx+ cix* rrv+ cdp* phx;
		ciy= ccy+ ciy* rrv+ cdp* phy;
		ciz= ccz- ciz* rrv;
	  }
	  else
	  {
		cix= cix+ ccx;
		ciy= ciy+ ccy;
		ciz= ciz+ ccz;
	  }

	} /* for( k=0; k < gnd.ksymp; k++ ) */

	if( ctx->geometry.m > 0)
	  jump = TRUE;
	else
	{
	  *eth=( cix* thx+ ciy* thy+ ciz* thz)* CONST3;
	  *eph=( cix* phx+ ciy* phy)* CONST3;
	  return;
	}

  } /* if( n != 0) */

  if( ! jump )
  {
	cix=CPLX_00;
	ciy=CPLX_00;
	ciz=CPLX_00;
  }

  /* electric field components */
  roz= rozs;
  rfl=-1.;
  for( ip = 0; ip < ctx->gnd.ksymp; ip++ )
  {
	rfl= -rfl;
	rrz= roz* rfl;
	fflds( ctx, rox, roy, rrz, &ctx->crnt.cur[ctx->geometry.n], &gx, &gy, &gz);

	if( ip != 1 )
	{
	  ex= gx;
	  ey= gy;
	  ez= gz;
	  continue;
	}

	if( ctx->gnd.iperf == 1)
	{
	  gx= -gx;
	  gy= -gy;
	  gz= -gz;
	}
	else
	{
	  rrv= csqrt(1.- ctx->gnd.zrati* ctx->gnd.zrati* thz* thz);
	  rrh= ctx->gnd.zrati* roz;
	  rrh=( rrh- rrv)/( rrh+ rrv);
	  rrv= ctx->gnd.zrati* rrv;
	  rrv=-( roz- rrv)/( roz+ rrv);
	  *eth=( gx* phx+ gy* phy)*( rrh- rrv);
	  gx= gx* rrv+ *eth* phx;
	  gy= gy* rrv+ *eth* phy;
	  gz= gz* rrv;

	} /* if( gnd.iperf == 1) */

	ex= ex+ gx;
	ey= ey+ gy;
	ez= ez- gz;

  } /* for( ip = 0; ip < gnd.ksymp; ip++ ) */

  ex= ex+ cix* CONST3;
  ey= ey+ ciy* CONST3;
  ez= ez+ ciz* CONST3;
  *eth= ex* thx+ ey* thy+ ez* thz;
  *eph= ex* phx+ ey* phy;

  return;
}

/*-----------------------------------------------------------------------*/

/* calculates the xyz components of the electric */
/* field due to surface currents */
void fflds(nec_context_t *ctx, double rox, double roy, double roz,
	complex double *scur, complex double *ex,
	complex double *ey, complex double *ez )
{
  int i, j, k;
  double arg;
  /* double d, rr, omega, el, top, bot, sill, too, boo, b, c, a; */
  /* complex double sc, tix, tiy, tiz, tcb, tcs, tcx, tcy, tcz, t1, t2; */
  /* double dx, dy, dz; */
  /* complex double exa, exb; */
  /* complex double gx, gy, gz; */
  complex double ct;

  double *xs = ctx->geometry.px;
  double *ys = ctx->geometry.py;
  double *zs = ctx->geometry.pz;
  double *s = ctx->geometry.pbi;

  *ex=CPLX_00;
  *ey=CPLX_00;
  *ez=CPLX_00;

  i= -1;
  for( j = 0; j < ctx->geometry.m; j++ )
  {
	i++;
	arg= TP*( rox* xs[i]+ roy* ys[i]+ roz* zs[i]);
	ct= cmplx( cos( arg)* s[i], sin( arg)* s[i]);
	k=3*j;
	*ex += scur[k  ]* ct;
	*ey += scur[k+1]* ct;
	*ez += scur[k+2]* ct;
  }

  ct = rox* *ex+ roy* *ey+ roz* *ez;
  *ex= CONST4*( ct* rox- *ex);
  *ey= CONST4*( ct* roy- *ey);
  *ez= CONST4*( ct* roz- *ez);

  return;
}

/*-----------------------------------------------------------------------*/

/* gfld computes the radiated field including ground wave. */
void gfld(nec_context_t *ctx, double rho, double phi, double rz,
	complex double *eth, complex double *epi,
	complex double *erd, complex double ux, int ksymp )
{
  int k;
  complex double erv, ezv, erh, ezh, eph;
  /* complex double gx, gy, gz; */

  *eth = CPLX_00;
  *epi = CPLX_00;
  *erd = CPLX_00;

  for( k = 0; k < ksymp; k++ )
  {
    gwave(ctx, &erv, &ezv, &erh, &ezh, &eph );
    *eth += erv* ux;
    *epi += ezv* ux;
    *erd += erh* ux;
   }

   return;
}

/*-----------------------------------------------------------------------*/

/* compute radiation pattern, gain, normalized gain */

#include "opennec.h"

/* rdpat_calc - Calculate radiation patterns (no output)
 * Refactored version that only performs calculations and stores results in ctx->rpat
 */
void rdpat(nec_context_t *ctx)
{
  int i, kth, kph, point_idx;
  double prad, gcon, gcop, gmax, pint, tmp1, tmp2;
  double phi, pha, thet, tha, ethm2, ethm, *gain = NULL;
  double etha, ephm2, ephm, epha, tilta, emajr2, eminr2, axrat;
  double dfaz, dfaz2, cdfaz, tstor1, tstor2, stilta, gnmj;
  double gnmn, gnv, gnh, gtot, tmp3, tmp4, da;
  complex double  eth, eph, erd;
  
  /* Free any previous radiation pattern data */
  if (ctx->rpat.points != NULL) {
    mem_free(ctx, (void **)&ctx->rpat.points);
    ctx->rpat.points = NULL;
  }
  
  /* Calculate ground parameters for cliff if needed */
  if (ctx->gnd.ifar > 1 && ctx->gnd.ifar != 4) {
    if ((ctx->gnd.ifar == 2) || (ctx->gnd.ifar == 5))
      strcpy(ctx->rpat.ground_cliff_type, "LINEAR");
    else if ((ctx->gnd.ifar == 3) || (ctx->gnd.ifar == 6))
      strcpy(ctx->rpat.ground_cliff_type, "CIRCULAR");
    
    ctx->gnd.cl = ctx->fpat.clt / ctx->geometry.wlam;
    ctx->gnd.ch = ctx->fpat.cht / ctx->geometry.wlam;
    ctx->gnd.zrati2 = csqrt(1./ cmplx(ctx->fpat.epsr2, -ctx->fpat.sig2 * ctx->geometry.wlam * 59.96));
  }
  
  /* Calculate range factor if specified */
  ctx->rpat.exrm = 0.0;
  ctx->rpat.exra = 0.0;
  if (ctx->fpat.rfld >= 1.0e-20) {
    ctx->rpat.exrm = 1.0 / ctx->fpat.rfld;
    double exra_tmp = ctx->fpat.rfld / ctx->geometry.wlam;
    ctx->rpat.exra = -360.0 * (exra_tmp - floor(exra_tmp));
  }
  
  /* Allocate memory for gain normalization buffer if needed */
  if (ctx->fpat.inor > 0) {
    size_t mreq = (size_t)(ctx->fpat.nth * ctx->fpat.nph);
    mreq *= sizeof(double);
    mem_alloc(ctx, (void *)&gain, mreq);
  }
  
  /* Calculate gain factors */
  if ((ctx->fpat.ixtyp == 0) || (ctx->fpat.ixtyp == 5)) {
    gcop = ctx->geometry.wlam * ctx->geometry.wlam * 2.0 * PI / (376.73 * ctx->fpat.pinr);
    prad = ctx->fpat.pinr - ctx->fpat.ploss - ctx->fpat.pnlr;
    gcon = gcop;
    if (ctx->fpat.ipd != 0)
      gcon = gcon * ctx->fpat.pinr / prad;
  }
  else if (ctx->fpat.ixtyp == 4) {
    ctx->fpat.pinr = 394.51 * ctx->fpat.xpr6 * ctx->fpat.xpr6 * ctx->geometry.wlam * ctx->geometry.wlam;
    gcop = ctx->geometry.wlam * ctx->geometry.wlam * 2.0 * PI / (376.73 * ctx->fpat.pinr);
    prad = ctx->fpat.pinr - ctx->fpat.ploss - ctx->fpat.pnlr;
    gcon = gcop;
    if (ctx->fpat.ipd != 0)
      gcon = gcon * ctx->fpat.pinr / prad;
  }
  else {
    gcon = 4.0 * PI / (1.0 + ctx->fpat.xpr6 * ctx->fpat.xpr6);
    gcop = gcon;
  }
  
  /* Allocate storage for radiation pattern points */
  ctx->rpat.num_points = ctx->fpat.nth * ctx->fpat.nph;
  size_t mreq = (size_t)ctx->rpat.num_points * sizeof(rpat_point_t);
  mem_alloc(ctx, (void **)&ctx->rpat.points, mreq);
  
  /* Initialize calculation variables */
  i = 0;
  point_idx = 0;
  gmax = -1.0e+10;
  pint = 0.0;
  tmp1 = ctx->fpat.dph * TA;
  tmp2 = 0.5 * ctx->fpat.dth * TA;
  phi = ctx->fpat.phis - ctx->fpat.dph;
  
  /* Main calculation loop over phi and theta */
  for (kph = 1; kph <= ctx->fpat.nph; kph++) {
    phi += ctx->fpat.dph;
    pha = phi * TA;
    thet = ctx->fpat.thets - ctx->fpat.dth;
    
    for (kth = 1; kth <= ctx->fpat.nth; kth++) {
      thet += ctx->fpat.dth;
      
      /* Skip if beyond 90 degrees with symmetry */
      if ((ctx->gnd.ksymp == 2) && (thet > 90.01) && (ctx->gnd.ifar != 1))
        continue;
      
      tha = thet * TA;
      rpat_point_t *pt = &ctx->rpat.points[point_idx];
      pt->theta = thet;
      pt->phi = phi;
      
      /* Calculate E-fields */
      if (ctx->gnd.ifar != 1) {
        ffld(ctx, tha, pha, &eth, &eph);
        pt->erdm = 0.0;
        pt->erda = 0.0;
      }
      else {
        gfld(ctx, ctx->fpat.rfld / ctx->geometry.wlam, pha, thet / ctx->geometry.wlam,
             &eth, &eph, &erd, ctx->gnd.zrati, ctx->gnd.ksymp);
        pt->erdm = cabs(erd);
        pt->erda = cang(ctx, erd);
      }
      
      ethm2 = creal(eth * conj(eth));
      ethm = sqrt(ethm2);
      etha = cang(ctx, eth);
      ephm2 = creal(eph * conj(eph));
      ephm = sqrt(ephm2);
      epha = cang(ctx, eph);
      
      /* Calculate polarization for far field */
      if (ctx->gnd.ifar != 1) {
        if ((ethm2 <= 1.0e-20) && (ephm2 <= 1.0e-20)) {
          tilta = 0.0;
          emajr2 = 0.0;
          eminr2 = 0.0;
          axrat = 0.0;
          pt->pol_sense = 0; /* LINEAR */
        }
        else {
          dfaz = epha - etha;
          if (epha >= 0.0)
            dfaz2 = dfaz - 360.0;
          else
            dfaz2 = dfaz + 360.0;
          
          if (fabs(dfaz) > fabs(dfaz2))
            dfaz = dfaz2;
          
          cdfaz = cos(dfaz * TA);
          tstor1 = ethm2 - ephm2;
          tstor2 = 2.0 * ephm * ethm * cdfaz;
          tilta = 0.5 * atan2(tstor2, tstor1);
          stilta = sin(tilta);
          tstor1 = tstor1 * stilta * stilta;
          tstor2 = tstor2 * stilta * cos(tilta);
          emajr2 = -tstor1 + tstor2 + ethm2;
          eminr2 = tstor1 - tstor2 + ephm2;
          if (eminr2 < 0.0)
            eminr2 = 0.0;
          
          axrat = sqrt(eminr2 / emajr2);
          tilta = tilta * TD;
          
          if (axrat <= 1.0e-5)
            pt->pol_sense = 0; /* LINEAR */
          else if (dfaz <= 0.0)
            pt->pol_sense = 1; /* RIGHT */
          else
            pt->pol_sense = 2; /* LEFT */
        }
        
        /* Calculate gains in dB */
        gnmj = db10(ctx, gcon * emajr2);
        gnmn = db10(ctx, gcon * eminr2);
        gnv = db10(ctx, gcon * ethm2);
        gnh = db10(ctx, gcon * ephm2);
        gtot = db10(ctx, gcon * (ethm2 + ephm2));
        
        pt->gnmj = gnmj;
        pt->gnmn = gnmn;
        pt->gnv = gnv;
        pt->gnh = gnh;
        pt->gtot = gtot;
        pt->axrat = axrat;
        pt->tilta = tilta;
        
        /* Store gain for normalization if requested */
        if (ctx->fpat.inor > 0) {
          i++;
          switch (ctx->fpat.inor) {
            case 1: tstor1 = gnmj; break;
            case 2: tstor1 = gnmn; break;
            case 3: tstor1 = gnv; break;
            case 4: tstor1 = gnh; break;
            case 5: tstor1 = gtot; break;
            default: tstor1 = gtot; break;
          }
          
          if (gain != NULL) gain[i-1] = tstor1;
          if (tstor1 > gmax)
            gmax = tstor1;
        }
        
        /* Accumulate for average power if requested */
        if (ctx->fpat.iavp != 0) {
          tstor1 = gcop * (ethm2 + ephm2);
          tmp3 = tha - tmp2;
          tmp4 = tha + tmp2;
          
          if (kth == 1)
            tmp3 = tha;
          else if (kth == ctx->fpat.nth)
            tmp4 = tha;
          
          da = fabs(tmp1 * (cos(tmp3) - cos(tmp4)));
          if ((kph == 1) || (kph == ctx->fpat.nph))
            da *= 0.5;
          pint += tstor1 * da;
        }
        
        /* Scale and adjust E-field values for output */
        ethm = ethm * ctx->geometry.wlam;
        ephm = ephm * ctx->geometry.wlam;
        
        if (ctx->fpat.rfld >= 1.0e-20) {
          ethm = ethm * ctx->rpat.exrm;
          etha = etha + ctx->rpat.exra;
          ephm = ephm * ctx->rpat.exrm;
          epha = epha + ctx->rpat.exra;
        }
      }
      else {
        /* Near field - no polarization calculation */
        pt->gnmj = 0.0;
        pt->gnmn = 0.0;
        pt->gnv = 0.0;
        pt->gnh = 0.0;
        pt->gtot = 0.0;
        pt->axrat = 0.0;
        pt->tilta = 0.0;
        pt->pol_sense = 0;
      }
      
      pt->ethm = ethm;
      pt->etha = etha;
      pt->ephm = ephm;
      pt->epha = epha;
      
      point_idx++;
    } /* for kth */
  } /* for kph */
  
  /* Calculate average power if requested */
  if (ctx->fpat.iavp != 0) {
    tmp3 = ctx->fpat.thets * TA;
    tmp4 = tmp3 + ctx->fpat.dth * TA * (double)(ctx->fpat.nth - 1);
    tmp3 = fabs(ctx->fpat.dph * TA * (double)(ctx->fpat.nph - 1) * (cos(tmp3) - cos(tmp4)));
    pint /= tmp3;
    ctx->rpat.solid_angle = tmp3 / PI;
    ctx->rpat.pint = pint;
  }
  else {
    ctx->rpat.pint = 0.0;
    ctx->rpat.solid_angle = 0.0;
  }
  
  /* Store maximum gain for normalization */
  if (ctx->fpat.inor > 0) {
    if (fabs(ctx->fpat.gnor) > 1.0e-20)
      gmax = ctx->fpat.gnor;
    ctx->rpat.gmax = gmax;
  }
  else {
    ctx->rpat.gmax = 0.0;
  }
  
  /* Free temporary gain buffer */
  if (gain != NULL)
    mem_free(ctx, (void *)&gain);
}
