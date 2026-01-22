/*** Translated to the C language by N. Kyriazis  20 Aug 2003 ***

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

 ******************************************************************/

#include "nec2c.h"
#include "shared.h"

/*-----------------------------------------------------------------------*/

/* ffld calculates the far zone radiated electric fields, */
/* the factor exp(j*k*r)/(r/lamda) not included */
void ffld( double thet, double phi,
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
  if( data.n != 0)
  {
	/* loop for structure image if any */
	/* calculation of reflection coeffecients */
	for( k = 0; k < gnd.ksymp; k++ )
	{
	  if( k != 0 )
	  {
		/* for perfect ground */
		if( gnd.iperf == 1)
		{
		  rrv=-CPLX_10;
		  rrh=-CPLX_10;
		}
		else
		{
		  /* for infinite planar ground */
		  zrsin= csqrt(1.- gnd.zrati* gnd.zrati* thz* thz);
		  rrv=-( roz- gnd.zrati* zrsin)/( roz+ gnd.zrati* zrsin);
		  rrh=( gnd.zrati* roz- zrsin)/( gnd.zrati* roz+ zrsin);

		} /* if( gnd.iperf == 1) */

		/* for the cliff problem, two reflction coefficients calculated */
		if( gnd.ifar > 1)
		{
		  rrv1= rrv;
		  rrh1= rrh;
		  tthet= tan( thet);

		  if( gnd.ifar != 4)
		  {
			zrsin= csqrt(1.- gnd.zrati2* gnd.zrati2* thz* thz);
			rrv2=-( roz- gnd.zrati2* zrsin)/( roz+ gnd.zrati2* zrsin);
			rrh2=( gnd.zrati2* roz- zrsin)/( gnd.zrati2* roz+ zrsin);
			darg= -TP*2.* gnd.ch* roz;
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
	  for( i = 0; i < data.n; i++ )
	  {
		omega=-( rox* data.cab[i]+ roy* data.sab[i]+ roz* data.salp[i]);
		el= PI* data.si[i];
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
		rr= a* crnt.air[i]+ b* crnt.bii[i]+ c* crnt.cir[i];
		ri= a* crnt.aii[i]- b* crnt.bir[i]+ c* crnt.cii[i];
		arg= TP*( data.x[i]* rox+ data.y[i]* roy+ data.z[i]* roz);

		if( (k != 1) || (gnd.ifar < 2) )
		{
		  /* summation for far field integral */
		  exa= cmplx( cos( arg), sin( arg))* cmplx( rr, ri);
		  cix= cix+ exa* data.cab[i];
		  ciy= ciy+ exa* data.sab[i];
		  ciz= ciz+ exa* data.salp[i];
		  continue;
		}

		/* calculation of image contribution */
		/* in cliff and ground screen problems */

		/* specular point distance */
		dr= data.z[i]* tthet;

		d= dr* phy+ data.x[i];
		if( gnd.ifar == 2)
		{
		  if(( gnd.cl- d) > 0.)
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
		  d= sqrt( d*d + (data.y[i]-dr*phx)*(data.y[i]-dr*phx) );
		  if( gnd.ifar == 3)
		  {
			if(( gnd.cl- d) > 0.)
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
			if(( gnd.scrwl- d) >= 0.)
			{
			  /* radial wire ground screen reflection coefficient */
			  d= d+ gnd.t2;
			  zscrn= gnd.t1* d* log( d/ gnd.t2);
			  zscrn=( zscrn* gnd.zrati)/( ETA* gnd.zrati+ zscrn);
			  zrsin= csqrt(1.- zscrn* zscrn* thz* thz);
			  rrv=( roz+ zscrn* zrsin)/(- roz+ zscrn* zrsin);
			  rrh=( zscrn* roz+ zrsin)/( zscrn* roz- zrsin);
			} /* if(( gnd.scrwl- d) < 0.) */
			else
			{
			  if( gnd.ifar == 4)
			  {
				rrv= rrv1;
				rrh= rrh1;
			  } /* if( gnd.ifar == 4) */
			  else
			  {
				if( gnd.ifar == 5)
				  d= dr* phy+ data.x[i];

				if(( gnd.cl- d) > 0.)
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
		tix= exa* data.cab[i];
		tiy= exa* data.sab[i];
		tiz= exa* data.salp[i];
		cdp=( tix* phx+ tiy* phy)*( rrh- rrv);
		cix= cix+ tix* rrv+ cdp* phx;
		ciy= ciy+ tiy* rrv+ cdp* phy;
		ciz= ciz- tiz* rrv;

	  } /* for( i = 0; i < n; i++ ) */

	  if( k == 0 )
		continue;

	  /* calculation of contribution of structure image for infinite ground */
	  if( gnd.ifar < 2)
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

	if( data.m > 0)
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
  for( ip = 0; ip < gnd.ksymp; ip++ )
  {
	rfl= -rfl;
	rrz= roz* rfl;
	fflds( rox, roy, rrz, &crnt.cur[data.n], &gx, &gy, &gz);

	if( ip != 1 )
	{
	  ex= gx;
	  ey= gy;
	  ez= gz;
	  continue;
	}

	if( gnd.iperf == 1)
	{
	  gx= -gx;
	  gy= -gy;
	  gz= -gz;
	}
	else
	{
	  rrv= csqrt(1.- gnd.zrati* gnd.zrati* thz* thz);
	  rrh= gnd.zrati* roz;
	  rrh=( rrh- rrv)/( rrh+ rrv);
	  rrv= gnd.zrati* rrv;
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
void fflds( double rox, double roy, double roz,
	complex double *scur, complex double *ex,
	complex double *ey, complex double *ez )
{
  double *xs, *ys, *zs, *s;
  int j, i, k;
  double arg;
  complex double ct;

  xs = data.px;
  ys = data.py;
  zs = data.pz;
  s = data.pbi;

  *ex=CPLX_00;
  *ey=CPLX_00;
  *ez=CPLX_00;

  i= -1;
  for( j = 0; j < data.m; j++ )
  {
	i++;
	arg= TP*( rox* xs[i]+ roy* ys[i]+ roz* zs[i]);
	ct= cmplx( cos( arg)* s[i], sin( arg)* s[i]);
	k=3*j;
	*ex += scur[k  ]* ct;
	*ey += scur[k+1]* ct;
	*ez += scur[k+2]* ct;
  }

  ct= rox* *ex+ roy* *ey+ roz* *ez;
  *ex= CONST4*( ct* rox- *ex);
  *ey= CONST4*( ct* roy- *ey);
  *ez= CONST4*( ct* roz- *ez);

  return;
}

/*-----------------------------------------------------------------------*/

/* gfld computes the radiated field including ground wave. */
void gfld( double rho, double phi, double rz,
	complex double *eth, complex double *epi,
	complex double *erd, complex double ux, int ksymp )
{
  int i, k;
  double b, r, thet, arg, phx, phy, rx, ry, dx, dy, dz, rix, riy, rhs, rhp;
  double rhx, rhy, calp, cbet, sbet, cph, sph, el, rfl, riz, thx, thy, thz;
  double rxyz, rnx, rny, rnz, omega, sill, top, bot, a, too, boo, c, rr, ri;
  complex double cix, ciy, ciz, exa, erv;
  complex double ezv, erh, eph, ezh, ex, ey;

  r= sqrt( rho*rho+ rz*rz );
  if( (ksymp == 1) || (cabs(ux) > .5) || (r > 1.e5) )
  {
	/* computation of space wave only */
	if( rz >= 1.0e-20)
	  thet= atan( rho/ rz);
	else
	  thet= PI*.5;

	ffld( thet, phi, eth, epi);
	arg= -TP* r;
	exa= cmplx( cos( arg), sin( arg))/ r;
	*eth= *eth* exa;
	*epi= *epi* exa;
	*erd=CPLX_00;
	return;
  } /* if( (ksymp == 1) && (cabs(ux) > .5) && (r > 1.e5) ) */

  /* computation of space and ground waves. */
  gwav.u= ux;
  gwav.u2= gwav.u* gwav.u;
  phx= -sin( phi);
  phy= cos( phi);
  rx= rho* phy;
  ry= -rho* phx;
  cix=CPLX_00;
  ciy=CPLX_00;
  ciz=CPLX_00;

  /* summation of field from individual segments */
  for( i = 0; i < data.n; i++ )
  {
	dx= data.cab[i];
	dy= data.sab[i];
	dz= data.salp[i];
	rix= rx- data.x[i];
	riy= ry- data.y[i];
	rhs= rix* rix+ riy* riy;
	rhp= sqrt( rhs);

	if( rhp >= 1.0e-6)
	{
	  rhx= rix/ rhp;
	  rhy= riy/ rhp;
	}
	else
	{
	  rhx=1.;
	  rhy=0.;
	}

	calp=1.- dz* dz;
	if( calp >= 1.0e-6)
	{
	  calp= sqrt( calp);
	  cbet= dx/ calp;
	  sbet= dy/ calp;
	  cph= rhx* cbet+ rhy* sbet;
	  sph= rhy* cbet- rhx* sbet;
	}
	else
	{
	  cph= rhx;
	  sph= rhy;
	}

	el= PI* data.si[i];
	rfl=-1.;

	/* integration of (current)*(phase factor) over segment and image for */
	/* constant, sine, and cosine current distributions */
	for( k = 0; k < 2; k++ )
	{
	  rfl= -rfl;
	  riz= rz- data.z[i]* rfl;
	  rxyz= sqrt( rix* rix+ riy* riy+ riz* riz);
	  rnx= rix/ rxyz;
	  rny= riy/ rxyz;
	  rnz= riz/ rxyz;
	  omega=-( rnx* dx+ rny* dy+ rnz* dz* rfl);
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
	  rr= a* crnt.air[i]+ b* crnt.bii[i]+ c* crnt.cir[i];
	  ri= a* crnt.aii[i]- b* crnt.bir[i]+ c* crnt.cii[i];
	  arg= TP*( data.x[i]* rnx+ data.y[i]* rny+ data.z[i]* rnz* rfl);
	  exa= cmplx( cos( arg), sin( arg))* cmplx( rr, ri)/ TP;

	  if( k != 1 )
	  {
		gwav.xx1= exa;
		gwav.r1= rxyz;
		gwav.zmh= riz;
		continue;
	  }

	  gwav.xx2= exa;
	  gwav.r2= rxyz;
	  gwav.zph= riz;

	} /* for( k = 0; k < 2; k++ ) */

	/* call subroutine to compute the field */
	/* of segment including ground wave. */
	gwave( &erv, &ezv, &erh, &ezh, &eph);
	erh= erh* cph* calp+ erv* dz;
	eph= eph* sph* calp;
	ezh= ezh* cph* calp+ ezv* dz;
	ex= erh* rhx- eph* rhy;
	ey= erh* rhy+ eph* rhx;
	cix= cix+ ex;
	ciy= ciy+ ey;
	ciz= ciz+ ezh;

  } /* for( i = 0; i < n; i++ ) */

  arg= -TP* r;
  exa= cmplx( cos( arg), sin( arg));
  cix= cix* exa;
  ciy= ciy* exa;
  ciz= ciz* exa;
  rnx= rx/ r;
  rny= ry/ r;
  rnz= rz/ r;
  thx= rnz* phy;
  thy= -rnz* phx;
  thz= -rho/ r;
  *eth= cix* thx+ ciy* thy+ ciz* thz;
  *epi= cix* phx+ ciy* phy;
  *erd= cix* rnx+ ciy* rny+ ciz* rnz;

  return;
}

/*-----------------------------------------------------------------------*/

/* compute radiation pattern, gain, normalized gain */
void rdpat( void )
{
  char  *hpol[3] = { "LINEAR", "RIGHT ", "LEFT  " };
  char  *igtp[2] = { "----- POWER GAINS ----- ", "--- DIRECTIVE GAINS ---" };
  char  *igax[4] = { " MAJOR", " MINOR", " VERTC", " HORIZ" };
  char *igntp[5] = { " MAJOR AXIS", "  MINOR AXIS",
	"    VERTICAL", "  HORIZONTAL", "       TOTAL " };

  char *hclif=NULL, *isens;
  int i, j, jump, itmp1, itmp2, kth, kph, itmp3, itmp4;
  double exrm=0., exra=0., prad, gcon, gcop, gmax, pint, tmp1, tmp2;
  double phi, pha, thet, tha, erdm=0., erda=0., ethm2, ethm, *gain = NULL;
  double etha, ephm2, ephm, epha, tilta, emajr2, eminr2, axrat;
  double dfaz, dfaz2, cdfaz, tstor1=0., tstor2, stilta, gnmj;
  double gnmn, gnv, gnh, gtot, tmp3, tmp4, da, tmp5, tmp6;
  complex double  eth, eph, erd;

  /* Allocate memory to gain buffer */
  if( fpat.inor > 0 )
  {
	size_t mreq = (size_t)(fpat.nth * fpat.nph);
	mreq *= sizeof(double);
	mem_alloc( (void *)&gain, mreq );
  }

  if( gnd.ifar > 1)
  {
	fprintf( output_fp, "\n\n\n"
		"                               "
		"------ FAR FIELD GROUND PARAMETERS ------\n\n" );

	jump = FALSE;
	if( gnd.ifar > 3)
	{
	  fprintf( output_fp, "\n"
		  "                               "
		  "--- RADIAL WIRE GROUND SCREEN ---\n"
		  "                               "
		  "NUM OF WIRES= %d\n"
		  "                               "
		  "WIRE LENGTH= %8.2f METERS\n"
		  "                               "
		  "WIRE RADIUS= %10.3E METERS",
		  gnd.nradl, save.scrwlt, save.scrwrt );

	  if( gnd.ifar == 4)
		jump = TRUE;

	} /* if( gnd.ifar > 3) */

	if( ! jump )
	{
	  if( (gnd.ifar == 2) || (gnd.ifar == 5) )
		hclif= "LINEAR";
	  if( (gnd.ifar == 3) || (gnd.ifar == 6) )
		hclif= "CIRCULAR";

	  gnd.cl= fpat.clt/ data.wlam;
	  gnd.ch= fpat.cht/ data.wlam;
	  gnd.zrati2= csqrt(1./ cmplx( fpat.epsr2,- fpat.sig2* data.wlam*59.96));

	  fprintf( output_fp, "\n"
		  "                               "
		  "--- %s CLIFF ---\n"
		  "                               "
		  "EDGE DISTANCE= %9.2f METERS\n"
		  "                               "
		  "       HEIGHT= %9.2f METERS\n"
		  "                               "
		  "--- SECOND MEDIUM ---\n"
		  "                               "
		  "RELATIVE DIELECTRIC CONST= %10.3f\n"
		  "                               "
		  "      GROUND CONDUCTIVITY= %10.3f MHOS",
		  hclif, fpat.clt, fpat.cht, fpat.epsr2, fpat.sig2 );

	} /* if( ! jump ) */

  } /* if( gnd.ifar > 1) */

  if( gnd.ifar == 1)
  {
	fprintf( output_fp, "\n\n\n"
		"                             "
		"------- RADIATED FIELDS NEAR GROUND --------\n\n"
		"    ------- LOCATION -------     --- E(THETA) ---    "
		" ---- E(PHI) ----    --- E(RADIAL) ---\n"
		"      RHO    PHI        Z           MAG    PHASE     "
		"    MAG    PHASE        MAG     PHASE\n"
		"    METERS DEGREES    METERS      VOLTS/M DEGREES   "
		"   VOLTS/M DEGREES     VOLTS/M  DEGREES" );
  }
  else
  {
	itmp1=2* fpat.iax;
	itmp2= itmp1+1;

	fprintf( output_fp, "\n\n\n"
		"                             "
		"---------- RADIATION PATTERNS -----------\n" );

	if( fpat.rfld >= 1.0e-20)
	{
	  exrm=1./ fpat.rfld;
	  exra= fpat.rfld/ data.wlam;
	  exra=-360.*( exra- floor( exra));

	  fprintf( output_fp, "\n"
		  "                             "
		  "RANGE: %13.6E METERS\n"
		  "                             "
		  "EXP(-JKR)/R: %12.5E AT PHASE: %7.2f DEGREES\n",
		  fpat.rfld, exrm, exra );
	}

	fprintf( output_fp, "\n"
		" ---- ANGLES -----     %23s      ---- POLARIZATION ----  "
		" ---- E(THETA) ----    ----- E(PHI) ------\n"
		"  THETA      PHI      %6s   %6s    TOTAL       AXIAL    "
		"  TILT  SENSE   MAGNITUDE    PHASE    MAGNITUDE     PHASE\n"
		" DEGREES   DEGREES        DB       DB       DB       RATIO  "
		" DEGREES            VOLTS/M   DEGREES     VOLTS/M   DEGREES",
		igtp[fpat.ipd], igax[itmp1], igax[itmp2] );

  } /* if( gnd.ifar == 1) */

  if( (fpat.ixtyp == 0) || (fpat.ixtyp == 5) )
  {
	gcop= data.wlam* data.wlam*2.* PI/(376.73* fpat.pinr);
	prad= fpat.pinr- fpat.ploss- fpat.pnlr;
	gcon= gcop;
	if( fpat.ipd != 0)
	  gcon= gcon* fpat.pinr/ prad;
  }
  else
	if( fpat.ixtyp == 4)
	{
	  fpat.pinr=394.51* fpat.xpr6* fpat.xpr6* data.wlam* data.wlam;
	  gcop= data.wlam* data.wlam*2.* PI/(376.73* fpat.pinr);
	  prad= fpat.pinr- fpat.ploss- fpat.pnlr;
	  gcon= gcop;
	  if( fpat.ipd != 0)
		gcon= gcon* fpat.pinr/ prad;
	}
	else
	{
	  gcon=4.* PI/(1.+ fpat.xpr6* fpat.xpr6);
	  gcop= gcon;
	}

  i=0;
  gmax=-1.e+10;
  pint=0.;
  tmp1= fpat.dph* TA;
  tmp2=.5* fpat.dth* TA;
  phi= fpat.phis- fpat.dph;

  for( kph = 1; kph <= fpat.nph; kph++ )
  {
	phi += fpat.dph;
	pha= phi* TA;
	thet= fpat.thets- fpat.dth;

	for( kth = 1; kth <= fpat.nth; kth++ )
	{
	  thet += fpat.dth;
	  if( (gnd.ksymp == 2) && (thet > 90.01) && (gnd.ifar != 1) )
		continue;

	  tha= thet* TA;
	  if( gnd.ifar != 1)
		ffld( tha, pha, &eth, &eph);
	  else
	  {
		gfld( fpat.rfld/data.wlam, pha, thet/data.wlam,
			&eth, &eph, &erd, gnd.zrati, gnd.ksymp);
		erdm= cabs( erd);
		erda= cang( erd);
	  }

	  ethm2= creal( eth* conj( eth));
	  ethm= sqrt( ethm2);
	  etha= cang( eth);
	  ephm2= creal( eph* conj( eph));
	  ephm= sqrt( ephm2);
	  epha= cang( eph);

	  /* elliptical polarization calc. */
	  if( gnd.ifar != 1)
	  {
		if( (ethm2 <= 1.0e-20) && (ephm2 <= 1.0e-20) )
		{
		  tilta=0.;
		  emajr2=0.;
		  eminr2=0.;
		  axrat=0.;
		  isens= " ";
		}
		else
		{
		  dfaz= epha- etha;
		  if( epha >= 0.)
			dfaz2= dfaz-360.;
		  else
			dfaz2= dfaz+360.;

		  if( fabs(dfaz) > fabs(dfaz2) )
			dfaz= dfaz2;

		  cdfaz= cos( dfaz* TA);
		  tstor1= ethm2- ephm2;
		  tstor2=2.* ephm* ethm* cdfaz;
		  tilta=.5* atan2( tstor2, tstor1);
		  stilta= sin( tilta);
		  tstor1= tstor1* stilta* stilta;
		  tstor2= tstor2* stilta* cos( tilta);
		  emajr2= -tstor1+ tstor2+ ethm2;
		  eminr2= tstor1- tstor2+ ephm2;
		  if( eminr2 < 0.)
			eminr2=0.;

		  axrat= sqrt( eminr2/ emajr2);
		  tilta= tilta* TD;
		  if( axrat <= 1.0e-5)
			isens= hpol[0];
		  else
			if( dfaz <= 0.)
			  isens= hpol[1];
			else
			  isens= hpol[2];

		} /* if( (ethm2 <= 1.0e-20) && (ephm2 <= 1.0e-20) ) */

		gnmj= db10( gcon* emajr2);
		gnmn= db10( gcon* eminr2);
		gnv = db10( gcon* ethm2);
		gnh = db10( gcon* ephm2);
		gtot= db10( gcon*(ethm2+ ephm2) );

		if( fpat.inor > 0)
		{
		  i++;
		  switch( fpat.inor )
		  {
			case 1:
			  tstor1= gnmj;
			  break;

			case 2:
			  tstor1= gnmn;
			  break;

			case 3:
			  tstor1= gnv;
			  break;

			case 4:
			  tstor1= gnh;
			  break;

			case 5:
			  tstor1= gtot;
		  }

		  gain[i-1]= tstor1;
		  if( tstor1 > gmax)
			gmax= tstor1;

		} /* if( fpat.inor > 0) */

		if( fpat.iavp != 0)
		{
		  tstor1= gcop*( ethm2+ ephm2);
		  tmp3= tha- tmp2;
		  tmp4= tha+ tmp2;

		  if( kth == 1)
			tmp3= tha;
		  else
			if( kth == fpat.nth)
			  tmp4= tha;

		  da= fabs( tmp1*( cos( tmp3)- cos( tmp4)));
		  if( (kph == 1) || (kph == fpat.nph) )
			da *=.5;
		  pint += tstor1* da;

		  if( fpat.iavp == 2)
			continue;
		}

		if( fpat.iax != 1)
		{
		  tmp5= gnmj;
		  tmp6= gnmn;
		}
		else
		{
		  tmp5= gnv;
		  tmp6= gnh;
		}

		ethm= ethm* data.wlam;
		ephm= ephm* data.wlam;

		if( fpat.rfld >= 1.0e-20 )
		{
		  ethm= ethm* exrm;
		  etha= etha+ exra;
		  ephm= ephm* exrm;
		  epha= epha+ exra;
		}

		fprintf( output_fp, "\n"
			" %7.2f %9.2f  %8.2f %8.2f %8.2f %11.4f"
			" %9.2f %6s %11.4E %9.2f %11.4E %9.2f",
			thet, phi, tmp5, tmp6, gtot, axrat,
			tilta, isens, ethm, etha, ephm, epha );

		if( plot.iplp1 != 3)
		  continue;

		if( plot.iplp3 != 0)
		{
		  if( plot.iplp2 == 1 )
		  {
			if( plot.iplp3 == 1 )
			  fprintf( plot_fp, "%12.4E %12.4E %12.4E\n", thet, ethm, etha );
			else
			  if( plot.iplp3 == 2 )
				fprintf( plot_fp, "%12.4E %12.4E %12.4E\n", thet, ephm, epha );
		  }

		  if( plot.iplp2 == 2 )
		  {
			if( plot.iplp3 == 1 )
			  fprintf( plot_fp, "%12.4E %12.4E %12.4E\n", phi, ethm, etha );
			else
			  if( plot.iplp3 == 2 )
				fprintf( plot_fp, "%12.4E %12.4E %12.4E\n", phi, ephm, epha );
		  }
		}

		if( plot.iplp4 == 0 )
		  continue;

		if( plot.iplp2 == 1 )
		{
		  switch( plot.iplp4 )
		  {
			case 1:
			  fprintf( plot_fp, "%12.4E %12.4E\n", thet, tmp5 );
			  break;
			case 2:
			  fprintf( plot_fp, "%12.4E %12.4E\n", thet, tmp6 );
			  break;
			case 3:
			  fprintf( plot_fp, "%12.4E %12.4E\n", thet, gtot );
		  }
		}

		if( plot.iplp2 == 2 )
		{
		  switch( plot.iplp4 )
		  {
			case 1:
			  fprintf( plot_fp, "%12.4E %12.4E\n", phi, tmp5 );
			  break;
			case 2:
			  fprintf( plot_fp, "%12.4E %12.4E\n", phi, tmp6 );
			  break;
			case 3:
			  fprintf( plot_fp, "%12.4E %12.4E\n", phi, gtot );
		  }
		}

		continue;
	  } /* if( gnd.ifar != 1) */

	  fprintf( output_fp, "\n"
		  " %9.2f %7.2f %9.2f  %11.4E %7.2f  %11.4E %7.2f  %11.4E %7.2f",
		  fpat.rfld, phi, thet, ethm, etha, ephm, epha, erdm, erda );

	} /* for( kth = 1; kth <= fpat.nth; kth++ ) */

  } /* for( kph = 1; kph <= fpat.nph; kph++ ) */

  if( fpat.iavp != 0)
  {
	tmp3= fpat.thets* TA;
	tmp4= tmp3+ fpat.dth* TA* (double)( fpat.nth-1);
	tmp3= fabs( fpat.dph* TA* (double)( fpat.nph-1)*( cos( tmp3)- cos( tmp4)));
	pint /= tmp3;
	tmp3 /= PI;

	fprintf( output_fp, "\n\n\n"
		"  AVERAGE POWER GAIN: %11.4E - SOLID ANGLE"
		" USED IN AVERAGING: (%+7.4f)*PI STERADIANS",
		pint, tmp3 );
  }

  if( fpat.inor > 0)
  {
	if( fabs( fpat.gnor) > 1.0e-20)
	  gmax= fpat.gnor;
	itmp1=( fpat.inor-1);

	fprintf( output_fp,	"\n\n\n"
		"                             "
		" ---------- NORMALIZED GAIN ----------\n"
		"                                      %6s GAIN\n"
		"                                  "
		" NORMALIZATION FACTOR: %.2f db\n\n"
		"    ---- ANGLES ----                ---- ANGLES ----"
		"                ---- ANGLES ----\n"
		"    THETA      PHI        GAIN      THETA      PHI  "
		"      GAIN      THETA      PHI       GAIN\n"
		"   DEGREES   DEGREES        DB     DEGREES   DEGREES "
		"       DB     DEGREES   DEGREES       DB",
		igntp[itmp1], gmax );

	itmp2= fpat.nph* fpat.nth;
	itmp1=( itmp2+2)/3;
	itmp2= itmp1*3- itmp2;
	itmp3= itmp1;
	itmp4=2* itmp1;

	if( itmp2 == 2)
	  itmp4--;

	for( i = 0; i < itmp1; i++ )
	{
	  itmp3++;
	  itmp4++;
	  j= i/ fpat.nth;
	  tmp1= fpat.thets+ (double)( i - j*fpat.nth )* fpat.dth;
	  tmp2= fpat.phis+ (double)(j)* fpat.dph;
	  j=( itmp3-1)/ fpat.nth;
	  tmp3= fpat.thets+ (double)( itmp3- j* fpat.nth-1)* fpat.dth;
	  tmp4= fpat.phis+ (double)(j)* fpat.dph;
	  j=( itmp4-1)/ fpat.nth;
	  tmp5= fpat.thets+ (double)( itmp4- j* fpat.nth-1)* fpat.dth;
	  tmp6= fpat.phis+ (double)(j)* fpat.dph;
	  tstor1= gain[i]- gmax;

	  if( ((i+1) == itmp1) && (itmp2 != 0) )
	  {
		if( itmp2 != 2)
		{
		  tstor2= gain[itmp3-1]- gmax;
		  fprintf( output_fp, "\n"
			  " %9.2f %9.2f %9.2f   %9.2f %9.2f %9.2f   ",
			  tmp1, tmp2, tstor1, tmp3, tmp4, tstor2 );
		  free_ptr( (void *)&gain );
		  return;
		}

		fprintf( output_fp, "\n"
			" %9.2f %9.2f %9.2f   ",
			tmp1, tmp2, tstor1 );
		free_ptr( (void *)&gain );
		return;

	  } /* if( ((i+1) == itmp1) && (itmp2 != 0) ) */

	  tstor2= gain[itmp3-1]- gmax;
	  pint= gain[itmp4-1]- gmax;

	  fprintf( output_fp, "\n"
		  " %9.2f %9.2f %9.2f   %9.2f %9.2f %9.2f   %9.2f %9.2f %9.2f",
		  tmp1, tmp2, tstor1, tmp3, tmp4, tstor2, tmp5, tmp6, pint );

	} /* for( i = 0; i < itmp1; i++ ) */
  }

  free_ptr( (void *)&gain );

  return;
}

/*-----------------------------------------------------------------------*/

