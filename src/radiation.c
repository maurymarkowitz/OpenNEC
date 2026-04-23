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
 * - far_e_field(): Calculate far-field `Eθ`/`Eφ` at observation angles `θ`, `φ`,
 *   summing segment contributions with sine/cosine/current expansions and
 *   appropriate phasing. Ground models adjust fields via reflection terms.
 * - Handle perfect and planar ground, cliff, and radial wire ground screen
 *   cases by computing reflection coefficients based on geometry and medium.
 * - Support image contributions where applicable to account for ground/cliff
 *   effects on the observed fields.
 *
 * Operates on `context_t`, reading geometry (`geometry`), current
 * expansions (`crnt`), and ground parameters (`gnd`) to assemble the far
 * field at requested angles.
 *****************************************************************************/


#include "internals.h"
#include "radiation.h"
#include "fields.h"
#include "calculations.h"

/* Forward declarations for internal functions */
/* Formerly nec2c: ffld */
static void far_e_field(context_t *restrict ctx, double thet, double phi, complex double *restrict eth, complex double *restrict eph);
/* Formerly nec2c: fflds */
static void far_e_field_surface(context_t *restrict ctx, double rox, double roy, double roz, complex double *restrict scur, complex double *restrict ex, complex double *restrict ey, complex double *restrict ez);
/* Formerly nec2c: gfld */
static void radiated_field_with_ground(context_t *restrict ctx, double rho, double phi, double rz, complex double *restrict eth, complex double *restrict epi, complex double *restrict erd, complex double ux, int ksymp);

/*-----------------------------------------------------------------------*/

/* ffld calculates the far zone radiated electric fields, */
/* the factor exp(j*k*r)/(r/lamda) not included */
/* Formerly nec2c: ffld */
void far_e_field(context_t *restrict ctx, double thet, double phi,
	complex double *restrict eth, complex double *restrict eph )
{
  int k, i, ip;
  bool jump;
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

  jump = false;
  if( ctx->geometry.num_segs != 0)
  {
	/* loop for structure image if any */
	/* calculation of reflection coeffecients */
	for( k = 0; k < ctx->gnd.has_ground; k++ )
	{
	  if( k != 0 )
	  {
		/* for perfect ground */
		if( ctx->gnd.is_perfect == 1)
		{
		  rrv=-CPLX_10;
		  rrh=-CPLX_10;
		}
		else
		{
		  /* for infinite planar ground */
		  zrsin= csqrt(1.- ctx->gnd.impedance_ratio* ctx->gnd.impedance_ratio* thz* thz);
		  rrv=-( roz- ctx->gnd.impedance_ratio* zrsin)/( roz+ ctx->gnd.impedance_ratio* zrsin);
		  rrh=( ctx->gnd.impedance_ratio* roz- zrsin)/( ctx->gnd.impedance_ratio* roz+ zrsin);

		} /* if( gnd.is_perfect == 1) */

		/* for the cliff problem, two reflction coefficients calculated */
		if( ctx->gnd.far_field_type > 1)
		{
		  rrv1= rrv;
		  rrh1= rrh;
		  tthet= tan( thet);

		  if( ctx->gnd.far_field_type != 4)
		  {
			zrsin= csqrt(1.- ctx->gnd.impedance_ratio2* ctx->gnd.impedance_ratio2* thz* thz);
			rrv2=-( roz- ctx->gnd.impedance_ratio2* zrsin)/( roz+ ctx->gnd.impedance_ratio2* zrsin);
			rrh2=( ctx->gnd.impedance_ratio2* roz- zrsin)/( ctx->gnd.impedance_ratio2* roz+ zrsin);
			darg= -TP*2.* ctx->gnd.cliff_height* roz;
		  }
		} /* if( gnd.far_field_type > 1) */

		roz= -roz;
		ccx= cix;
		ccy= ciy;
		ccz= ciz;

	  } /* if( k != 0 ) */

	  cix=CPLX_00;
	  ciy=CPLX_00;
	  ciz=CPLX_00;

	  /* loop over structure segments */
	  for( i = 0; i < ctx->geometry.num_segs; i++ )
	  {
		omega=-( rox* ctx->geometry.dir_cos_x[i]+ roy* ctx->geometry.dir_cos_y[i]+ roz* ctx->geometry.dir_cos_z[i]);
		el= PI* ctx->geometry.half_len[i];
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
		rr= a* ctx->crnt.a_real[i]+ b* ctx->crnt.b_imag[i]+ c* ctx->crnt.c_real[i];
		ri= a* ctx->crnt.a_imag[i]- b* ctx->crnt.b_real[i]+ c* ctx->crnt.c_imag[i];
		arg= TP*( ctx->geometry.x_center[i]* rox+ ctx->geometry.y_center[i]* roy+ ctx->geometry.z_center[i]* roz);

		if( (k != 1) || (ctx->gnd.far_field_type < 2) )
		{
		  /* summation for far field integral */
		  exa= cmplx( cos( arg), sin( arg))* cmplx( rr, ri);
		  cix= cix+ exa* ctx->geometry.dir_cos_x[i];
		  ciy= ciy+ exa* ctx->geometry.dir_cos_y[i];
		  ciz= ciz+ exa* ctx->geometry.dir_cos_z[i];
		  continue;
		}

		/* calculation of image contribution */
		/* in cliff and ground screen problems */

		/* specular point distance */
		dr= ctx->geometry.z_center[i]* tthet;

		d= dr* phy+ ctx->geometry.x_center[i];
		if( ctx->gnd.far_field_type == 2)
		{
		  if(( ctx->gnd.cliff_dist- d) > 0.)
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
		} /* if( gnd.far_field_type == 2) */
		else
		{
		  d= sqrt( d*d + (ctx->geometry.y_center[i]-dr*phx)*(ctx->geometry.y_center[i]-dr*phx) );
		  if( ctx->gnd.far_field_type == 3)
		  {
			if(( ctx->gnd.cliff_dist- d) > 0.)
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
		  } /* if( gnd.far_field_type == 3) */
		  else
		  {
			if(( ctx->gnd.screen_wire_len- d) >= 0.)
			{
			  /* radial wire ground screen reflection coefficient */
			  d= d+ ctx->gnd.screen_inner_r;
			  zscrn= ctx->gnd.screen_impedance* d* log( d/ ctx->gnd.screen_inner_r);
			  zscrn=( zscrn* ctx->gnd.impedance_ratio)/( ETA* ctx->gnd.impedance_ratio+ zscrn);
			  zrsin= csqrt(1.- zscrn* zscrn* thz* thz);
			  rrv=( roz+ zscrn* zrsin)/(- roz+ zscrn* zrsin);
			  rrh=( zscrn* roz+ zrsin)/( zscrn* roz- zrsin);
			} /* if(( gnd.screen_wire_len- d) < 0.) */
			else
			{
			  if( ctx->gnd.far_field_type == 4)
			  {
				rrv= rrv1;
				rrh= rrh1;
			  } /* if( gnd.far_field_type == 4) */
			  else
			  {
				if( ctx->gnd.far_field_type == 5)
				  d= dr* phy+ ctx->geometry.x_center[i];

				if(( ctx->gnd.cliff_dist- d) > 0.)
				{
				  rrv= rrv1;
				  rrh= rrh1;
				}
				else
				{
				  rrv= rrv2;
				  rrh= rrh2;
				  arg= arg+ darg;
				} /* if(( gnd.cliff_dist- d) > 0.) */

			  } /* if( gnd.far_field_type == 4) */

			} /* if(( gnd.screen_wire_len- d) < 0.) */

		  } /* if( gnd.far_field_type == 3) */

		} /* if( gnd.far_field_type == 2) */

		/* contribution of each image segment modified by */
		/* reflection coef, for cliff and ground screen problems */
		exa= cmplx( cos( arg), sin( arg))* cmplx( rr, ri);
		tix= exa* ctx->geometry.dir_cos_x[i];
		tiy= exa* ctx->geometry.dir_cos_y[i];
		tiz= exa* ctx->geometry.dir_cos_z[i];
		cdp=( tix* phx+ tiy* phy)*( rrh- rrv);
		cix= cix+ tix* rrv+ cdp* phx;
		ciy= ciy+ tiy* rrv+ cdp* phy;
		ciz= ciz- tiz* rrv;

	  } /* for( i = 0; i < n; i++ ) */

	  if( k == 0 )
		continue;

	  /* calculation of contribution of structure image for infinite ground */
	  if( ctx->gnd.far_field_type < 2)
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

	} /* for( k=0; k < gnd.has_ground; k++ ) */

	if( ctx->geometry.num_patches > 0)
	  jump = true;
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
  for( ip = 0; ip < ctx->gnd.has_ground; ip++ )
  {
	rfl= -rfl;
	rrz= roz* rfl;
	far_e_field_surface( ctx, rox, roy, rrz, &ctx->crnt.surface_cur[ctx->geometry.num_segs], &gx, &gy, &gz);

	if( ip != 1 )
	{
	  ex= gx;
	  ey= gy;
	  ez= gz;
	  continue;
	}

	if( ctx->gnd.is_perfect == 1)
	{
	  gx= -gx;
	  gy= -gy;
	  gz= -gz;
	}
	else
	{
	  rrv= csqrt(1.- ctx->gnd.impedance_ratio* ctx->gnd.impedance_ratio* thz* thz);
	  rrh= ctx->gnd.impedance_ratio* roz;
	  rrh=( rrh- rrv)/( rrh+ rrv);
	  rrv= ctx->gnd.impedance_ratio* rrv;
	  rrv=-( roz- rrv)/( roz+ rrv);
	  *eth=( gx* phx+ gy* phy)*( rrh- rrv);
	  gx= gx* rrv+ *eth* phx;
	  gy= gy* rrv+ *eth* phy;
	  gz= gz* rrv;

	} /* if( gnd.is_perfect == 1) */

	ex= ex+ gx;
	ey= ey+ gy;
	ez= ez- gz;

  } /* for( ip = 0; ip < gnd.has_ground; ip++ ) */

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
/* Formerly nec2c: fflds */
void far_e_field_surface(context_t *restrict ctx, double rox, double roy, double roz,
	complex double *restrict scur, complex double *restrict ex,
	complex double *restrict ey, complex double *restrict ez )
{
  int i, j, k;
  double arg;
  /* double d, rr, omega, el, top, bot, sill, too, boo, b, c, a; */
  /* complex double sc, tix, tiy, tiz, tcb, tcs, tcx, tcy, tcz, t1, t2; */
  /* double dx, dy, dz; */
  /* complex double exa, exb; */
  /* complex double gx, gy, gz; */
  complex double ct;

  double *xs = ctx->geometry.patch_x_center;
  double *ys = ctx->geometry.patch_y_center;
  double *zs = ctx->geometry.patch_z_center;
  double *s = ctx->geometry.patch_area;

  *ex=CPLX_00;
  *ey=CPLX_00;
  *ez=CPLX_00;

  i= -1;
  for( j = 0; j < ctx->geometry.num_patches; j++ )
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
/* Formerly nec2c: gfld */
void radiated_field_with_ground(context_t *restrict ctx, double rho, double phi, double rz,
	complex double *restrict eth, complex double *restrict epi,
	complex double *restrict erd, complex double ux, int ksymp )
{
  int k;
  complex double erv, ezv, erh, ezh, eph;
  /* complex double gx, gy, gz; */

  *eth = CPLX_00;
  *epi = CPLX_00;
  *erd = CPLX_00;

  for( k = 0; k < ksymp; k++ )
  {
    ground_wave_field(ctx, &erv, &ezv, &erh, &ezh, &eph );
    *eth += erv* ux;
    *epi += ezv* ux;
    *erd += erh* ux;
   }

   return;
}

/*-----------------------------------------------------------------------*/

/* compute radiation pattern, gain, normalized gain */

#include "internals.h"

/* rdpat_calc - Calculate radiation patterns (no output)
 * Refactored version that only performs calculations and stores results in ctx->rpat
 */
/* Formerly nec2c: rdpat */
void compute_radiation_pattern(context_t *ctx)
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
  if (ctx->gnd.far_field_type > 1 && ctx->gnd.far_field_type != 4) {
    if ((ctx->gnd.far_field_type == 2) || (ctx->gnd.far_field_type == 5))
      strcpy(ctx->rpat.ground_cliff_type, "LINEAR");
    else if ((ctx->gnd.far_field_type == 3) || (ctx->gnd.far_field_type == 6))
      strcpy(ctx->rpat.ground_cliff_type, "CIRCULAR");
    
    ctx->gnd.cliff_dist = ctx->fpat.cliff_dist / ctx->geometry.wavelength;
    ctx->gnd.cliff_height = ctx->fpat.cliff_height / ctx->geometry.wavelength;
    ctx->gnd.impedance_ratio2 = csqrt(1./ cmplx(ctx->fpat.epsr2, -ctx->fpat.sigma2 * ctx->geometry.wavelength * 59.96));
  }
  
  /* Calculate range factor if specified */
  ctx->rpat.exrm = 0.0;
  ctx->rpat.exra = 0.0;
  if (ctx->fpat.range >= 1.0e-20) {
    ctx->rpat.exrm = 1.0 / ctx->fpat.range;
    double exra_tmp = ctx->fpat.range / ctx->geometry.wavelength;
    ctx->rpat.exra = -360.0 * (exra_tmp - floor(exra_tmp));
  }
  
  /* Allocate memory for gain normalization buffer if needed */
  if (ctx->fpat.normalize_gain > 0) {
    size_t mreq = (size_t)(ctx->fpat.num_theta * ctx->fpat.num_phi);
    mreq *= sizeof(double);
    mem_alloc(ctx, (void *)&gain, mreq);
  }
  
  /* Calculate gain factors */
  if ((ctx->fpat.excitation_type == 0) || (ctx->fpat.excitation_type == 5)) {
    gcop = ctx->geometry.wavelength * ctx->geometry.wavelength * 2.0 * PI / (376.73 * ctx->fpat.power_in);
    prad = ctx->fpat.power_in - ctx->fpat.ohmic_loss - ctx->fpat.network_loss;
    gcon = gcop;
    if (ctx->fpat.gain_type != 0)
      gcon = gcon * ctx->fpat.power_in / prad;
  }
  else if (ctx->fpat.excitation_type == 4) {
    ctx->fpat.power_in = 394.51 * ctx->fpat.exc_param6 * ctx->fpat.exc_param6 * ctx->geometry.wavelength * ctx->geometry.wavelength;
    gcop = ctx->geometry.wavelength * ctx->geometry.wavelength * 2.0 * PI / (376.73 * ctx->fpat.power_in);
    prad = ctx->fpat.power_in - ctx->fpat.ohmic_loss - ctx->fpat.network_loss;
    gcon = gcop;
    if (ctx->fpat.gain_type != 0)
      gcon = gcon * ctx->fpat.power_in / prad;
  }
  else {
    gcon = 4.0 * PI / (1.0 + ctx->fpat.exc_param6 * ctx->fpat.exc_param6);
    gcop = gcon;
  }
  
  /* Allocate storage for radiation pattern points */
  ctx->rpat.num_points = ctx->fpat.num_theta * ctx->fpat.num_phi;
  size_t mreq = (size_t)ctx->rpat.num_points * sizeof(rpat_point_t);
  mem_alloc(ctx, (void **)&ctx->rpat.points, mreq);
  
  /* Initialize calculation variables */
  i = 0;
  point_idx = 0;
  gmax = -1.0e+10;
  pint = 0.0;
  tmp1 = ctx->fpat.phi_step * TA;
  tmp2 = 0.5 * ctx->fpat.theta_step * TA;
  phi = ctx->fpat.phi_start - ctx->fpat.phi_step;
  
  /* Main calculation loop over phi and theta */
  for (kph = 1; kph <= ctx->fpat.num_phi; kph++) {
    phi += ctx->fpat.phi_step;
    pha = phi * TA;
    thet = ctx->fpat.theta_start - ctx->fpat.theta_step;
    
    for (kth = 1; kth <= ctx->fpat.num_theta; kth++) {
      thet += ctx->fpat.theta_step;
      
      /* Skip if beyond 90 degrees with symmetry */
      if ((ctx->gnd.has_ground == 2) && (thet > 90.01) && (ctx->gnd.far_field_type != 1))
        continue;
      
      tha = thet * TA;
      rpat_point_t *pt = &ctx->rpat.points[point_idx];
      pt->theta = thet;
      pt->phi = phi;
      
      /* Calculate E-fields */
      if (ctx->gnd.far_field_type != 1) {
        far_e_field(ctx, tha, pha, &eth, &eph);
        pt->erdm = 0.0;
        pt->erda = 0.0;
      }
      else {
        radiated_field_with_ground(ctx, ctx->fpat.range / ctx->geometry.wavelength, pha, thet / ctx->geometry.wavelength,
             &eth, &eph, &erd, ctx->gnd.impedance_ratio, ctx->gnd.has_ground);
        pt->erdm = cabs(erd);
        pt->erda = complex_angle_deg(ctx, erd);
      }
      
      ethm2 = creal(eth * conj(eth));
      ethm = sqrt(ethm2);
      etha = complex_angle_deg(ctx, eth);
      ephm2 = creal(eph * conj(eph));
      ephm = sqrt(ephm2);
      epha = complex_angle_deg(ctx, eph);
      
      /* Calculate polarization for far field */
      if (ctx->gnd.far_field_type != 1) {
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
        if (ctx->fpat.normalize_gain > 0) {
          i++;
          switch (ctx->fpat.normalize_gain) {
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
        if (ctx->fpat.avg_power_flag != 0) {
          tstor1 = gcop * (ethm2 + ephm2);
          tmp3 = tha - tmp2;
          tmp4 = tha + tmp2;
          
          if (kth == 1)
            tmp3 = tha;
          else if (kth == ctx->fpat.num_theta)
            tmp4 = tha;
          
          da = fabs(tmp1 * (cos(tmp3) - cos(tmp4)));
          if ((kph == 1) || (kph == ctx->fpat.num_phi))
            da *= 0.5;
          pint += tstor1 * da;
        }
        
        /* Scale and adjust E-field values for output */
        ethm = ethm * ctx->geometry.wavelength;
        ephm = ephm * ctx->geometry.wavelength;
        
        if (ctx->fpat.range >= 1.0e-20) {
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
      
      /* Normalize linear polarization to match NEC-2D convention:
         when one field component is negligibly small, its phase is
         undefined floating-point noise; set it to 0. */
      if (pt->pol_sense == 0 /* LINEAR */) {
        double ethm2_adj = ethm * ethm;
        double ephm2_adj = ephm * ephm;
        if (ethm2_adj <= 1.0e-10 * (ephm2_adj + 1.0e-30))
          etha = 0.0;
        if (ephm2_adj <= 1.0e-10 * (ethm2_adj + 1.0e-30))
          epha = 0.0;
      }

      pt->ethm = ethm;
      pt->etha = etha;
      pt->ephm = ephm;
      pt->epha = epha;
      
      point_idx++;
    } /* for kth */
  } /* for kph */
  
  /* Calculate average power if requested */
  if (ctx->fpat.avg_power_flag != 0) {
    tmp3 = ctx->fpat.theta_start * TA;
    tmp4 = tmp3 + ctx->fpat.theta_step * TA * (double)(ctx->fpat.num_theta - 1);
    tmp3 = fabs(ctx->fpat.phi_step * TA * (double)(ctx->fpat.num_phi - 1) * (cos(tmp3) - cos(tmp4)));
    pint /= tmp3;
    ctx->rpat.solid_angle = tmp3 / PI;
    ctx->rpat.pint = pint;
  }
  else {
    ctx->rpat.pint = 0.0;
    ctx->rpat.solid_angle = 0.0;
  }
  
  /* Store maximum gain for normalization */
  if (ctx->fpat.normalize_gain > 0) {
    if (fabs(ctx->fpat.norm_gain) > 1.0e-20)
      gmax = ctx->fpat.norm_gain;
    ctx->rpat.gmax = gmax;
  }
  else {
    ctx->rpat.gmax = 0.0;
  }
  
  /* Free temporary gain buffer */
  if (gain != NULL)
    mem_free(ctx, (void *)&gain);
}
