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

*******************************************************************/

#include "opennec.h"

/*-------------------------------------------------------------------*/

/* cmset sets up the complex structure matrix in the array cm */
void cmset(nec_context_t *ctx, int nrow, complex double *cm, double rkhx, int iexkx)
{
  int mp2, neq, npeq, it, i, j, i1, i2, in2;
  int im1, im2, ist, ij, ipr, jss, jm1, jm2, jst, k, ka, kk;
  complex double zaj, deter, *scm = NULL;

  mp2 = 2 * ctx->geometry.mp;
  npeq = ctx->geometry.np + mp2;
  neq = ctx->geometry.n + 2 * ctx->geometry.m;
  ctx->smat.nop = neq / npeq;

  ctx->dataj.rkh = rkhx;
  ctx->dataj.iexk = iexkx;
  it = ctx->matpar.nlast;

  for( i = 0; i < nrow; i++ )
	for( j = 0; j < it; j++ )
	  cm[i+j*nrow]= CPLX_00;

  i1= 1;
  i2= it;
  in2= i2;

  if( in2 > ctx->geometry.np)
	in2= ctx->geometry.np;

  im1= i1- ctx->geometry.np;
  im2= i2- ctx->geometry.np;

  if( im1 < 1)
	im1=1;

  ist=1;
  if( i1 <= ctx->geometry.np)
	ist= ctx->geometry.np- i1+2;

  /* wire source loop */
  if( ctx->geometry.n != 0)
  {
	for( j = 1; j <= ctx->geometry.n; j++ )
	{
	  trio(ctx, j);
	  for( i = 0; i < ctx->segj.jsno; i++ )
	  {
		ij = ctx->segj.jco[i];
		ctx->segj.jco[i] = (( ij - 1 ) / ctx->geometry.np) * mp2 + ij;
	  }

	  if( i1 <= in2)
		cmww(ctx, j, i1, in2, cm, nrow, cm, nrow, 1);

	  if( im1 <= im2)
		cmws(ctx, j, im1, im2, &cm[(ist - 1) * nrow], nrow, cm, 1);

	  /* matrix elements modified by loading */
	  if( ctx->zload.nload == 0)
		continue;

	  if( j > ctx->geometry.np)
		continue;

	  ipr = j;
	  if( (ipr < 1) || (ipr > it) )
		continue;

	  zaj = ctx->zload.zarray[j-1];

	  for( i = 0; i < ctx->segj.jsno; i++ )
	  {
		jss = ctx->segj.jco[i];
		cm[(jss - 1) + (ipr - 1) * nrow] -= ( ctx->segj.ax[i] + ctx->segj.cx[i] ) * zaj;
	  }

	} /* for( j = 1; j <= n; j++ ) */

  } /* if( n != 0) */

  if( ctx->geometry.m != 0)
  {
	/* matrix elements for patch current sources */
	jm1 = 1 - ctx->geometry.mp;
	jm2 = 0;
	jst = 1 - mp2;

	for( i = 0; i < ctx->smat.nop; i++ )
	{
	  jm1 += ctx->geometry.mp;
	  jm2 += ctx->geometry.mp;
	  jst += npeq;

	  if( i1 <= in2)
		cmsw(ctx, jm1, jm2, i1, in2, &cm[(jst - 1)], cm, 0, nrow, 1);

	  if( im1 <= im2)
		cmss(ctx, jm1, jm2, im1, im2, &cm[(jst - 1) + (ist - 1) * nrow], nrow, 1);
	}

  } /* if( m != 0) */

  if( ctx->matpar.icase == 1)
	return;

  /* Allocate to scratch memory */
  size_t mreq = (size_t)ctx->geometry.np2m;
  mreq *= sizeof(complex double);
  mem_alloc(ctx, (void *)&scm, mreq );

  /* combine elements for symmetry modes */
  for( i = 0; i < it; i++ )
  {
	for( j = 0; j < npeq; j++ )
	{
	  for( k = 0; k < ctx->smat.nop; k++ )
	  {
		ka= j+ k*npeq;
		scm[k]= cm[ka+i*nrow];
	  }

	  deter= scm[0];

	  for( kk = 1; kk < ctx->smat.nop; kk++ )
		deter += scm[kk];

	  cm[j+i*nrow]= deter;

	  for( k = 1; k < ctx->smat.nop; k++ )
	  {
		ka= j+ k*npeq;
		deter= scm[0];

		for( kk = 1; kk < ctx->smat.nop; kk++ )
		{
		  deter += scm[kk]* ctx->smat.ssx[k+kk*ctx->smat.nop];
		  cm[ka+i*nrow]= deter;
		}

	  } /* for( k = 1; k < smat.nop; k++ ) */

	} /* for( j = 0; j < npeq; j++ ) */

  } /* for( i = 0; i < it; i++ ) */

  mem_free(ctx, (void *)&scm );

  return;
}

/*-----------------------------------------------------------------------*/

/* cmss computes matrix elements for surface-surface interactions. */
void cmss(nec_context_t *ctx, int j1, int j2, int im1, int im2,
    complex double *cm, int nrow, int itrp )
{
  int i1, i2, icomp, ii1, i, il, ii2, jj1, j, jl, /*jl2,*/ jj2;
  double t1xi, t1yi, t1zi, t2xi, t2yi, t2zi, xi, yi, zi;
  complex double g11, g12, g21, g22;

  i1=( im1+1)/2;
  i2=( im2+1)/2;
  icomp= i1*2-3;
  ii1=-2;
  if( icomp+2 < im1)
	ii1=-3;

  /* loop over observation patches */
  il = -1;
  for( i = i1; i <= i2; i++ )
  {
	il++;
	icomp += 2;
	ii1 += 2;
	ii2 = ii1+1;

	t1xi = ctx->geometry.t1x[il] * ctx->geometry.psalp[il];
	t1yi = ctx->geometry.t1y[il] * ctx->geometry.psalp[il];
	t1zi = ctx->geometry.t1z[il] * ctx->geometry.psalp[il];
	t2xi = ctx->geometry.t2x[il] * ctx->geometry.psalp[il];
	t2yi = ctx->geometry.t2y[il] * ctx->geometry.psalp[il];
	t2zi = ctx->geometry.t2z[il] * ctx->geometry.psalp[il];
	xi = ctx->geometry.px[il];
	yi = ctx->geometry.py[il];
	zi = ctx->geometry.pz[il];

	/* loop over source patches */
	jj1=-2;
	for( j = j1; j <= j2; j++ )
	{
	  jl=j-1;
	  jj1 += 2;
	  jj2 = jj1+1;

	  ctx->dataj.s = ctx->geometry.pbi[jl];
	  ctx->dataj.xj = ctx->geometry.px[jl];
	  ctx->dataj.yj = ctx->geometry.py[jl];
	  ctx->dataj.zj = ctx->geometry.pz[jl];
	  ctx->dataj.t1xj = ctx->geometry.t1x[jl];
	  ctx->dataj.t1yj = ctx->geometry.t1y[jl];
	  ctx->dataj.t1zj = ctx->geometry.t1z[jl];
	  ctx->dataj.t2xj = ctx->geometry.t2x[jl];
	  ctx->dataj.t2yj = ctx->geometry.t2y[jl];
	  ctx->dataj.t2zj = ctx->geometry.t2z[jl];

	  hintg(ctx, xi, yi, zi);

	  g11 = -( t2xi * ctx->dataj.exk + t2yi * ctx->dataj.eyk + t2zi * ctx->dataj.ezk );
	  g12 = -( t2xi * ctx->dataj.exs + t2yi * ctx->dataj.eys + t2zi * ctx->dataj.ezs );
	  g21 = -( t1xi * ctx->dataj.exk + t1yi * ctx->dataj.eyk + t1zi * ctx->dataj.ezk );
	  g22 = -( t1xi * ctx->dataj.exs + t1yi * ctx->dataj.eys + t1zi * ctx->dataj.ezs );

	  if( i == j )
	  {
		g11 -= .5;
		g22 += .5;
	  }

	  /* normal fill */
	  if( itrp == 0)
	  {
		if( icomp >= im1 )
		{
		  cm[ii1+jj1*nrow]= g11;
		  cm[ii1+jj2*nrow]= g12;
		}

		if( icomp >= im2 )
		  continue;

		cm[ii2+jj1*nrow]= g21;
		cm[ii2+jj2*nrow]= g22;
		continue;

	  } /* if( itrp == 0) */

	  /* transposed fill */
	  if( icomp >= im1 )
	  {
		cm[jj1+ii1*nrow]= g11;
		cm[jj2+ii1*nrow]= g12;
	  }

	  if( icomp >= im2 )
		continue;

	  cm[jj1+ii2*nrow]= g21;
	  cm[jj2+ii2*nrow]= g22;

	} /* for( j = j1; j <= j2; j++ ) */

  } /* for( i = i1; i <= i2; i++ ) */

  return;
}

/*-----------------------------------------------------------------------*/

/* computes matrix elements for e along wires due to patch current */
void cmsw(nec_context_t *ctx, int j1, int j2, int i1, int i2, complex double *cm,
    complex double *cw, int ncw, int nrow, int itrp )
{
  int jsnox; /* -1 offset to "jsno" for array indexing */
  complex double emel[9];

  jsnox = ctx->segj.jsno-1;

  if( itrp >= 0)
  {
	int k, icgo, i, ipch, jl, j, js, il, ip;
	double xi, yi, zi, cabi, sabi, salpi, fsign=1., pyl, pxl;

	k=-1;
	icgo=0;

	/* observation loop */
	for( i = i1-1; i < i2; i++ )
	{
	  k++;
	  xi = ctx->geometry.x[i];
	  yi = ctx->geometry.y[i];
	  zi = ctx->geometry.z[i];
	  cabi = ctx->geometry.cab[i];
	  sabi = ctx->geometry.sab[i];
	  salpi = ctx->geometry.salp[i];
	  ipch=0;

	  if( ctx->geometry.icon1[i] >= PCHCON)
	  {
		ipch= ctx->geometry.icon1[i]-PCHCON;
		fsign=-1.;
	  }

	  if( ctx->geometry.icon2[i] >= PCHCON)
	  {
		ipch= ctx->geometry.icon2[i]-PCHCON;
		fsign=1.;
	  }

	  /* source loop */
	  jl = -1;
	  for( j = j1; j <= j2; j++ )
	  {
		jl += 2;
		js = j-1;
		ctx->dataj.t1xj = ctx->geometry.t1x[js];
		ctx->dataj.t1yj = ctx->geometry.t1y[js];
		ctx->dataj.t1zj = ctx->geometry.t1z[js];
		ctx->dataj.t2xj = ctx->geometry.t2x[js];
		ctx->dataj.t2yj = ctx->geometry.t2y[js];
		ctx->dataj.t2zj = ctx->geometry.t2z[js];
		ctx->dataj.xj = ctx->geometry.px[js];
		ctx->dataj.yj = ctx->geometry.py[js];
		ctx->dataj.zj = ctx->geometry.pz[js];
		ctx->dataj.s = ctx->geometry.pbi[js];

		/* ground loop */
		for( ip = 1; ip <= ctx->gnd.ksymp; ip++ )
		{
		  ctx->dataj.ipgnd= ip;

		  if( ((ipch == j) || (icgo != 0)) && (ip != 2) )
		  {
			if( icgo <= 0 )
			{
			  pcint(ctx, xi, yi, zi, cabi, sabi, salpi, emel);

			  pyl= PI* ctx->geometry.si[i]* fsign;
			  pxl= sin( pyl);
			  pyl= cos( pyl);
			  ctx->dataj.exc= emel[8]* fsign;

			  trio(ctx, i+1);

			  il= i-ncw;
			  if( i < ctx->geometry.np)
				il += (il/ctx->geometry.np)*2*ctx->geometry.mp;

			  if( itrp == 0 )
				cw[k+il*nrow] +=
				  ctx->dataj.exc * ( ctx->segj.ax[jsnox] + ctx->segj.bx[jsnox] * pxl + ctx->segj.cx[jsnox] * pyl );
			  else
				cw[il+k*nrow] +=
				  ctx->dataj.exc * ( ctx->segj.ax[jsnox] + ctx->segj.bx[jsnox] * pxl + ctx->segj.cx[jsnox] * pyl );

			} /* if( icgo <= 0 ) */

			if( itrp == 0)
			{
			  cm[k+(jl-1)*nrow]= emel[icgo];
			  cm[k+jl*nrow]    = emel[icgo+4];
			}
			else
			{
			  cm[(jl-1)+k*nrow]= emel[icgo];
			  cm[jl+k*nrow]    = emel[icgo+4];
			}

			icgo++;
			if( icgo == 4)
			  icgo=0;

			continue;

		  } /* if( ((ipch == (j+1)) || (icgo != 0)) && (ip != 2) ) */

		  unere(ctx, xi, yi, zi);

		  /* normal fill */
		  if( itrp == 0)
		  {
			cm[k+(jl-1)*nrow] += ctx->dataj.exk* cabi+ ctx->dataj.eyk* sabi+ ctx->dataj.ezk* salpi;
			cm[k+jl*nrow]     += ctx->dataj.exs* cabi+ ctx->dataj.eys* sabi+ ctx->dataj.ezs* salpi;
			continue;
		  }

		  /* transposed fill */
		  cm[(jl-1)+k*nrow] += ctx->dataj.exk* cabi+ ctx->dataj.eyk* sabi+ ctx->dataj.ezk* salpi;
		  cm[jl+k*nrow]     += ctx->dataj.exs* cabi+ ctx->dataj.eys* sabi+ ctx->dataj.ezs* salpi;

		} /* for( ip = 1; ip <= gnd.ksymp; ip++ ) */

	  } /* for( j = j1; j <= j2; j++ ) */

	} /* for( i = i1-1; i < i2; i++ ) */

  } /* if( itrp >= 0) */

  return;
}

/*-----------------------------------------------------------------------*/

/* cmws computes matrix elements for wire-surface interactions */
void cmws(nec_context_t *ctx, int j, int i1, int i2, complex double *cm,
    int nr, complex double *cw, int itrp )
 {
  int ipr, i, ipatch, ik, js=0, ij, jx;
  double xi, yi, zi, tx, ty, tz;
  complex double etk, ets, etc;

  j--;
  ctx->dataj.s = ctx->geometry.si[j];
  ctx->dataj.b = ctx->geometry.bi[j];
  ctx->dataj.xj = ctx->geometry.x[j];
  ctx->dataj.yj = ctx->geometry.y[j];
  ctx->dataj.zj = ctx->geometry.z[j];
  ctx->dataj.cabj = ctx->geometry.cab[j];
  ctx->dataj.sabj = ctx->geometry.sab[j];
  ctx->dataj.salpj = ctx->geometry.salp[j];

  /* observation loop */
  ipr= -1;
  for( i = i1; i <= i2; i++ )
  {
	ipr++;
	ipatch=(i+1)/2;
	ik= i-( i/2)*2;

	if( (ik != 0) || (ipr == 0) )
	{
	  js= ipatch-1;
	  xi= ctx->geometry.px[js];
	  yi= ctx->geometry.py[js];
	  zi= ctx->geometry.pz[js];
	  hsfld(ctx, xi, yi, zi, 0.);

	  if( ik != 0 )
	  {
		tx= ctx->geometry.t2x[js];
		ty= ctx->geometry.t2y[js];
		tz= ctx->geometry.t2z[js];
	  }
	  else
	  {
		tx= ctx->geometry.t1x[js];
		ty= ctx->geometry.t1y[js];
		tz= ctx->geometry.t1z[js];
	  }

	} /* if( (ik != 0) || (ipr == 0) ) */
	else
	{
	  tx= ctx->geometry.t1x[js];
	  ty= ctx->geometry.t1y[js];
	  tz= ctx->geometry.t1z[js];

	} /* if( (ik != 0) || (ipr == 0) ) */

	etk=-( ctx->dataj.exk* tx+ ctx->dataj.eyk* ty+ ctx->dataj.ezk* tz)* ctx->geometry.psalp[js];
	ets=-( ctx->dataj.exs* tx+ ctx->dataj.eys* ty+ ctx->dataj.ezs* tz)* ctx->geometry.psalp[js];
	etc=-( ctx->dataj.exc* tx+ ctx->dataj.eyc* ty+ ctx->dataj.ezc* tz)* ctx->geometry.psalp[js];

	/* fill matrix elements.  element locations */
	/* determined by connection data. */

	/* normal fill */
	if( itrp == 0)
	{
	  for( ij = 0; ij < ctx->segj.jsno; ij++ )
	  {
		jx= ctx->segj.jco[ij]-1;
		cm[ipr+jx*nr] += etk* ctx->segj.ax[ij]+ ets* ctx->segj.bx[ij]+ etc* ctx->segj.cx[ij];
	  }

	  continue;
	} /* if( itrp == 0) */

	/* transposed fill */
	if( itrp != 2)
	{
	  for( ij = 0; ij < ctx->segj.jsno; ij++ )
	  {
		jx= ctx->segj.jco[ij]-1;
		cm[jx+ipr*nr] += etk* ctx->segj.ax[ij]+ ets* ctx->segj.bx[ij]+ etc* ctx->segj.cx[ij];
	  }

	  continue;
	} /* if( itrp != 2) */

	/* transposed fill - c(ws) and d(ws)prime (=cw) */
	for( ij = 0; ij < ctx->segj.jsno; ij++ )
	{
	  jx= ctx->segj.jco[ij]-1;
	  if( jx < nr)
		cm[jx+ipr*nr] += etk* ctx->segj.ax[ij]+ ets* ctx->segj.bx[ij]+ etc* ctx->segj.cx[ij];
	  else
	  {
		jx -= nr;
		cw[jx+ipr*nr] += etk* ctx->segj.ax[ij]+ ets* ctx->segj.bx[ij]+ etc* ctx->segj.cx[ij];
	  }
	} /* for( ij = 0; ij < segj.jsno; ij++ ) */

  } /* for( i = i1; i <= i2; i++ ) */

  return;
}

/*-----------------------------------------------------------------------*/

/* cmww computes matrix elements for wire-wire interactions */
void cmww(nec_context_t *ctx, int j, int i1, int i2, complex double *cm,
    int nr, complex double *cw, int nw, int itrp)
 {
  int ipr, iprx, i, ij, jx;
  double xi, yi, zi, ai, cabi, sabi, salpi;
  complex double etk, ets, etc;

  /* set source segment parameters */
  jx = j;
  j--;
  ctx->dataj.s = ctx->geometry.si[j];
  ctx->dataj.b = ctx->geometry.bi[j];
  ctx->dataj.xj = ctx->geometry.x[j];
  ctx->dataj.yj = ctx->geometry.y[j];
  ctx->dataj.zj = ctx->geometry.z[j];
  ctx->dataj.cabj = ctx->geometry.cab[j];
  ctx->dataj.sabj = ctx->geometry.sab[j];
  ctx->dataj.salpj = ctx->geometry.salp[j];

  /* decide whether ext. t.w. approx. can be used */
  if( ctx->dataj.iexk != 0)
  {
	ipr = ctx->geometry.icon1[j];
	if (ipr > PCHCON) ctx->dataj.ind1 = 0;
	else if( ipr < 0 )
	{
	  ipr= -ipr;
	  iprx= ipr-1;

	  if( -ctx->geometry.icon1[iprx] != jx )	ctx->dataj.ind1 = 2;
	  else
	  {
		xi= fabs( ctx->dataj.cabj* ctx->geometry.cab[iprx]+ ctx->dataj.sabj*
			ctx->geometry.sab[iprx]+ ctx->dataj.salpj* ctx->geometry.salp[iprx]);
		if( (xi < 0.999999) || (fabs(ctx->geometry.bi[iprx]/ctx->dataj.b-1.) > 1.e-6) )
		  ctx->dataj.ind1=2;
		else
		  ctx->dataj.ind1=0;

	  } /* if( -data.icon1[iprx] != jx ) */

	} /* if( ipr < 0 ) */
	else
	{
	  iprx = ipr-1;
	  if( ipr == 0 ) ctx->dataj.ind1=1;
	  else
	  {
		if( ipr != jx )
		{
		  if( ctx->geometry.icon2[iprx] != jx ) ctx->dataj.ind1=2;
		  else
		  {
			xi= fabs( ctx->dataj.cabj* ctx->geometry.cab[iprx]+ ctx->dataj.sabj*
				ctx->geometry.sab[iprx]+ ctx->dataj.salpj* ctx->geometry.salp[iprx]);
			if( (xi < 0.999999) || (fabs(ctx->geometry.bi[iprx]/ctx->dataj.b-1.) > 1.e-6) )
			  ctx->dataj.ind1=2;
			else
			  ctx->dataj.ind1=0;

		  } /* if( data.icon2[iprx] != jx ) */

		} /* if( ipr != jx ) */
		else
		  if( ctx->dataj.cabj* ctx->dataj.cabj+ ctx->dataj.sabj* ctx->dataj.sabj > 1.e-8)
			ctx->dataj.ind1=2;
		  else
			ctx->dataj.ind1=0;

	  } /* if( ipr == 0 ) */

	} /* if( ipr < 0 ) */

	ipr = ctx->geometry.icon2[j];
	if (ipr > PCHCON) ctx->dataj.ind2 = 2;
	else if( ipr < 0 )
	{
	  ipr= -ipr;
	  iprx = ipr-1;
	  if( -ctx->geometry.icon2[iprx] != jx )
		ctx->dataj.ind2=2;
	  else
	  {
		xi= fabs( ctx->dataj.cabj* ctx->geometry.cab[iprx]+ ctx->dataj.sabj*
			ctx->geometry.sab[iprx]+ ctx->dataj.salpj* ctx->geometry.salp[iprx]);
		if( (xi < 0.999999) || (fabs(ctx->geometry.bi[iprx]/ctx->dataj.b-1.) > 1.e-6) )
		  ctx->dataj.ind2=2;
		else
		  ctx->dataj.ind2=0;

	  } /* if( -data.icon1[iprx] != jx ) */

	} /* if( ipr < 0 ) */
	else
	{
	  iprx = ipr-1;
	  if( ipr == 0 ) ctx->dataj.ind2=1;
	  else
	  {
		if( ipr != jx )
		{
		  if( ctx->geometry.icon1[iprx] != jx )
			ctx->dataj.ind2=2;
		  else
		  {
			xi= fabs( ctx->dataj.cabj* ctx->geometry.cab[iprx]+ ctx->dataj.sabj*
				ctx->geometry.sab[iprx]+ ctx->dataj.salpj* ctx->geometry.salp[iprx]);
			if( (xi < 0.999999) || (fabs(ctx->geometry.bi[iprx]/ctx->dataj.b-1.) > 1.e-6) )
			  ctx->dataj.ind2=2;
			else
			  ctx->dataj.ind2=0;

		  } /* if( data.icon2[iprx] != jx ) */

		} /* if( ipr != jx ) */
		else
		  if( ctx->dataj.cabj* ctx->dataj.cabj+ ctx->dataj.sabj* ctx->dataj.sabj > 1.e-8)
			ctx->dataj.ind2=2;
		  else
			ctx->dataj.ind2=0;

	  } /* if( ipr == 0 ) */

	} /* if( ipr < 0 ) */

  } /* if( dataj.iexk != 0) */

  /* observation loop */
  ipr=-1;
  for( i = i1-1; i < i2; i++ )
  {
	ipr++;
	ij= i-j;
	xi= ctx->geometry.x[i];
	yi= ctx->geometry.y[i];
	zi= ctx->geometry.z[i];
	ai= ctx->geometry.bi[i];
	cabi= ctx->geometry.cab[i];
	sabi= ctx->geometry.sab[i];
	salpi= ctx->geometry.salp[i];

	efld( ctx, xi, yi, zi, ai, ij);

	etk= ctx->dataj.exk* cabi+ ctx->dataj.eyk* sabi+ ctx->dataj.ezk* salpi;
	ets= ctx->dataj.exs* cabi+ ctx->dataj.eys* sabi+ ctx->dataj.ezs* salpi;
	etc= ctx->dataj.exc* cabi+ ctx->dataj.eyc* sabi+ ctx->dataj.ezc* salpi;

	/* fill matrix elements. element locations */
	/* determined by connection data. */

	/* normal fill */
	if( itrp == 0)
	{
	  for( ij = 0; ij < ctx->segj.jsno; ij++ )
	  {
		jx= ctx->segj.jco[ij]-1;
		cm[ipr+jx*nr] += etk* ctx->segj.ax[ij]+ ets* ctx->segj.bx[ij]+ etc* ctx->segj.cx[ij];
	  }
	  continue;
	}

	/* transposed fill */
	if( itrp != 2)
	{
	  for( ij = 0; ij < ctx->segj.jsno; ij++ )
	  {
		jx= ctx->segj.jco[ij]-1;
		cm[jx+ipr*nr] += etk* ctx->segj.ax[ij]+ ets* ctx->segj.bx[ij]+ etc* ctx->segj.cx[ij];
	  }
	  continue;
	}

	/* trans. fill for c(ww) - test for elements for d(ww)prime.  (=cw) */
	for( ij = 0; ij < ctx->segj.jsno; ij++ )
	{
	  jx= ctx->segj.jco[ij]-1;
	  if( jx < nr)
		cm[jx+ipr*nr] += etk* ctx->segj.ax[ij]+ ets* ctx->segj.bx[ij]+ etc* ctx->segj.cx[ij];
	  else
	  {
		jx -= nr;
		cw[jx*ipr*nw] += etk* ctx->segj.ax[ij]+ ets* ctx->segj.bx[ij]+ etc* ctx->segj.cx[ij];
	  }
	} /* for( ij = 0; ij < segj.jsno; ij++ ) */

  } /* for( i = i1-1; i < i2; i++ ) */

  return;
}

/*-----------------------------------------------------------------------*/

/* etmns fills the array e with the negative of the */
/* electric field incident on the structure. e is the */
/* right hand side of the matrix equation. */
void etmns(nec_context_t *ctx, double p1, double p2, double p3, double p4,
    double p5, double p6, int ipr, complex double *e )
{
  int i, is, i1, i2=0, neq;
  double cth, sth, cph, sph, cet, set, pxl, pyl, pzl, wx;
  double wy, wz, qx, qy, qz, arg, ds, dsh, rs, r;
  complex double cx, cy, cz, er, et, ezh, erh, rrv=CPLX_00, rrh=CPLX_00, tt1, tt2;

  neq = ctx->geometry.n + 2 * ctx->geometry.m;
  ctx->vsorc.nqds = 0;

  /* applied field of voltage sources for transmitting case */
  if( (ipr == 0) || (ipr == 5) )
  {
	for( i = 0; i < neq; i++ )
	  e[i] = CPLX_00;

	if( ctx->vsorc.nsant != 0)
	{
	  for( i = 0; i < ctx->vsorc.nsant; i++ )
	  {
		is = ctx->vsorc.isant[i] - 1;
		e[is] = -ctx->vsorc.vsant[i] / ( ctx->geometry.si[is] * ctx->geometry.wlam );
	  }
	}

	if( ctx->vsorc.nvqd == 0)
	  return;

	for( i = 0; i < ctx->vsorc.nvqd; i++ )
	{
	  is= ctx->vsorc.ivqd[i];
	  qdsrc( ctx, is, ctx->vsorc.vqd[i], e);
	}
	return;

  } /* if( (ipr == 0) || (ipr == 5) ) */

  /* incident plane wave, linearly polarized. */
  if( ipr <= 3)
  {
	cth = cos( p1 );
	sth = sin( p1 );
	cph = cos( p2 );
	sph = sin( p2 );
	cet = cos( p3 );
	set = sin( p3 );
	pxl= cth* cph* cet- sph* set;
	pyl= cth* sph* cet+ cph* set;
	pzl= -sth* cet;
	wx= -sth* cph;
	wy= -sth* sph;
	wz= -cth;
	qx= wy* pzl- wz* pyl;
	qy= wz* pxl- wx* pzl;
	qz= wx* pyl- wy* pxl;

	if( ctx->gnd.ksymp != 1)
	{
	  if( ctx->gnd.iperf != 1)
	  {
		rrv = csqrt(1. - ctx->gnd.zrati * ctx->gnd.zrati * sth * sth);
		rrh = ctx->gnd.zrati * cth;
		rrh = ( rrh - rrv ) / ( rrh + rrv );
		rrv = ctx->gnd.zrati * rrv;
		rrv = -( cth - rrv ) / ( cth + rrv );
	  }
	  else
	  {
		rrv = -CPLX_10;
		rrh = -CPLX_10;
	  } /* if( gnd.iperf != 1) */

	} /* if( gnd.ksymp != 1) */

	if( ipr == 1)
	{
	  if( ctx->geometry.n != 0)
	  {
		for( i = 0; i < ctx->geometry.n; i++ )
		{
		  arg= -TP*( wx * ctx->geometry.x[i] + wy * ctx->geometry.y[i] + wz * ctx->geometry.z[i] );
		  e[i]=-( pxl * ctx->geometry.cab[i] + pyl * ctx->geometry.sab[i] + pzl * ctx->geometry.salp[i] ) * cmplx( cos( arg ), sin( arg) );
		}

		if( ctx->gnd.ksymp != 1)
		{
		  tt1=( pyl* cph- pxl* sph)*( rrh- rrv);
		  cx= rrv* pxl- tt1* sph;
		  cy= rrv* pyl+ tt1* cph;
		  cz= -rrv* pzl;

		  for( i = 0; i < ctx->geometry.n; i++ )
		  {
			arg= -TP*( wx* ctx->geometry.x[i]+ wy* ctx->geometry.y[i]- wz* ctx->geometry.z[i]);
			e[i]= e[i]-( cx* ctx->geometry.cab[i]+ cy* ctx->geometry.sab[i]+
				cz* ctx->geometry.salp[i])* cmplx(cos( arg), sin( arg));
		  }

		} /* if( gnd.ksymp != 1) */

	  } /* if( data.n != 0) */

	  if( ctx->geometry.m == 0)
		return;

	  i= -1;
	  i1= ctx->geometry.n-2;
	  for( is = 0; is < ctx->geometry.m; is++ )
	  {
		i++;
		i1 += 2;
		i2 = i1+1;
		arg= -TP*( wx* ctx->geometry.px[i]+ wy* ctx->geometry.py[i]+ wz* ctx->geometry.pz[i]);
		tt1= cmplx( cos( arg), sin( arg))* ctx->geometry.psalp[i]* RETA;
		e[i2]=( qx* ctx->geometry.t1x[i]+ qy* ctx->geometry.t1y[i]+ qz* ctx->geometry.t1z[i])* tt1;
		e[i1]=( qx* ctx->geometry.t2x[i]+ qy* ctx->geometry.t2y[i]+ qz* ctx->geometry.t2z[i])* tt1;
	  }

	  if( ctx->gnd.ksymp == 1)
		return;

	  tt1=( qy* cph- qx* sph)*( rrv- rrh);
	  cx=-( rrh* qx- tt1* sph);
	  cy=-( rrh* qy+ tt1* cph);
	  cz= rrh* qz;

	  i= -1;
	  i1= ctx->geometry.n-2;
	  for( is = 0; is < ctx->geometry.m; is++ )
	  {
		i++;
		i1 += 2;
		i2 = i1+1;
		arg= -TP*( wx* ctx->geometry.px[i]+ wy* ctx->geometry.py[i]- wz* ctx->geometry.pz[i]);
		tt1= cmplx( cos( arg), sin( arg))* ctx->geometry.psalp[i]* RETA;
		e[i2]= e[i2]+( cx* ctx->geometry.t1x[i]+ cy* ctx->geometry.t1y[i]+ cz* ctx->geometry.t1z[i])* tt1;
		e[i1]= e[i1]+( cx* ctx->geometry.t2x[i]+ cy* ctx->geometry.t2y[i]+ cz* ctx->geometry.t2z[i])* tt1;
	  }
	  return;

	} /* if( ipr == 1) */

	/* incident plane wave, elliptic polarization. */
	tt1=-(CPLX_01)* p6;
	if( ipr == 3)
	  tt1= -tt1;

	if( ctx->geometry.n != 0)
	{
	  cx= pxl+ tt1* qx;
	  cy= pyl+ tt1* qy;
	  cz= pzl+ tt1* qz;

	  for( i = 0; i < ctx->geometry.n; i++ )
	  {
		arg= -TP*( wx* ctx->geometry.x[i]+ wy* ctx->geometry.y[i]+ wz* ctx->geometry.z[i]);
		e[i]=-( cx* ctx->geometry.cab[i]+ cy* ctx->geometry.sab[i]+ cz*
			ctx->geometry.salp[i])* cmplx( cos( arg), sin( arg));
	  }

	  if( ctx->gnd.ksymp != 1)
	  {
		tt2=( cy* cph- cx* sph)*( rrh- rrv);
		cx= rrv* cx- tt2* sph;
		cy= rrv* cy+ tt2* cph;
		cz= -rrv* cz;

		for( i = 0; i < ctx->geometry.n; i++ )
		{
		  arg= -TP*( wx* ctx->geometry.x[i]+ wy* ctx->geometry.y[i]- wz* ctx->geometry.z[i]);
		  e[i]= e[i]-( cx* ctx->geometry.cab[i]+ cy* ctx->geometry.sab[i]+
			  cz* ctx->geometry.salp[i])* cmplx(cos( arg), sin( arg));
		}

	  } /* if( gnd.ksymp != 1) */

	} /* if( n != 0) */

	if( ctx->geometry.m == 0)
	  return;

	cx= qx- tt1* pxl;
	cy= qy- tt1* pyl;
	cz= qz- tt1* pzl;

	i= -1;
	i1= ctx->geometry.n-2;
	for( is = 0; is < ctx->geometry.m; is++ )
	{
	  i++;
	  i1 += 2;
	  i2 = i1+1;
	  arg= -TP*( wx* ctx->geometry.px[i]+ wy* ctx->geometry.py[i]+ wz* ctx->geometry.pz[i]);
	  tt2= cmplx( cos( arg), sin( arg))* ctx->geometry.psalp[i]* RETA;
	  e[i2]=( cx* ctx->geometry.t1x[i]+ cy* ctx->geometry.t1y[i]+ cz* ctx->geometry.t1z[i])* tt2;
	  e[i1]=( cx* ctx->geometry.t2x[i]+ cy* ctx->geometry.t2y[i]+ cz* ctx->geometry.t2z[i])* tt2;
	}

	if( ctx->gnd.ksymp == 1)
	  return;

	tt1=( cy* cph- cx* sph)*( rrv- rrh);
	cx=-( rrh* cx- tt1* sph);
	cy=-( rrh* cy+ tt1* cph);
	cz= rrh* cz;

	i= -1;
	i1= ctx->geometry.n-2;
	for( is=0; is < ctx->geometry.m; is++ )
	{
	  i++;
	  i1 += 2;
	  i2 = i1+1;
	  arg= -TP*( wx* ctx->geometry.px[i]+ wy* ctx->geometry.py[i]- wz* ctx->geometry.pz[i]);
	  tt1= cmplx( cos( arg), sin( arg))* ctx->geometry.psalp[i]* RETA;
	  e[i2]= e[i2]+( cx* ctx->geometry.t1x[i]+ cy* ctx->geometry.t1y[i]+ cz* ctx->geometry.t1z[i])* tt1;
	  e[i1]= e[i1]+( cx* ctx->geometry.t2x[i]+ cy* ctx->geometry.t2y[i]+ cz* ctx->geometry.t2z[i])* tt1;
	}

	return;

  } /* if( ipr <= 3) */

  /* incident field of an elementary current source. */
  wz= cos( p4);
  wx= wz* cos( p5);
  wy= wz* sin( p5);
  wz= sin( p4);
  ds= p6*59.958;
  dsh= p6/(2.* TP);

  is= 0;
  i1= ctx->geometry.n-2;
  for( i = 0; i < ctx->geometry.npm; i++ )
  {
	if( i >= ctx->geometry.n )
	{
	  i1 += 2;
	  i2 = i1+1;
	  pxl= ctx->geometry.px[is]- p1;
	  pyl= ctx->geometry.py[is]- p2;
	  pzl= ctx->geometry.pz[is]- p3;
	}
	else
	{
	  pxl= ctx->geometry.x[i]- p1;
	  pyl= ctx->geometry.y[i]- p2;
	  pzl= ctx->geometry.z[i]- p3;
	}

	rs= pxl* pxl+ pyl* pyl+ pzl* pzl;
	if( rs < 1.0e-30)
	  continue;

	r= sqrt( rs);
	pxl= pxl/ r;
	pyl= pyl/ r;
	pzl= pzl/ r;
	cth= pxl* wx+ pyl* wy+ pzl* wz;
	sth= sqrt(1.- cth* cth);
	qx= pxl- wx* cth;
	qy= pyl- wy* cth;
	qz= pzl- wz* cth;

	arg= sqrt( qx* qx+ qy* qy+ qz* qz);
	if( arg >= 1.e-30)
	{
	  qx= qx/ arg;
	  qy= qy/ arg;
	  qz= qz/ arg;
	}
	else
	{
	  qx=1.;
	  qy=0.;
	  qz=0.;

	} /* if( arg >= 1.e-30) */

	arg= -TP* r;
	tt1= cmplx( cos( arg), sin( arg));

	if( i < ctx->geometry.n )
	{
	  tt2= cmplx(1.0,-1.0/( r* TP))/ rs;
	  er= ds* tt1* tt2* cth;
	  et=.5* ds* tt1*((CPLX_01)* TP/ r+ tt2)* sth;
	  ezh= er* cth- et* sth;
	  erh= er* sth+ et* cth;
	  cx= ezh* wx+ erh* qx;
	  cy= ezh* wy+ erh* qy;
	  cz= ezh* wz+ erh* qz;
	  e[i]=-( cx* ctx->geometry.cab[i]+ cy* ctx->geometry.sab[i]+ cz* ctx->geometry.salp[i]);
	}
	else
	{
	  pxl= wy* qz- wz* qy;
	  pyl= wz* qx- wx* qz;
	  pzl= wx* qy- wy* qx;
	  tt2= dsh* tt1* cmplx(1./ r, TP)/ r* sth* ctx->geometry.psalp[is];
	  cx= tt2* pxl;
	  cy= tt2* pyl;
	  cz= tt2* pzl;
	  e[i2]= cx* ctx->geometry.t1x[is]+ cy* ctx->geometry.t1y[is]+ cz* ctx->geometry.t1z[is];
	  e[i1]= cx* ctx->geometry.t2x[is]+ cy* ctx->geometry.t2y[is]+ cz* ctx->geometry.t2z[is];
	  is++;
	} /* if( i < ctx->geometry.n) */

  } /* for( i = 0; i < ctx->geometry.npm; i++ ) */

  return;
}

/*-----------------------------------------------------------------------*/

/* subroutine to factor a matrix into a unit lower triangular matrix */
/* and an upper triangular matrix using the gauss-doolittle algorithm */
/* presented on pages 411-416 of a. ralston--a first course in */
/* numerical analysis.  comments below refer to comments in ralstons */
/* text.    (matrix transposed.) */

void factr(nec_context_t *ctx, int n, complex double *a, int *ip, int ndim)
{
  int r, rm1, rp1, pj, pr, iflg, k, j, jp1, i;
  double dmax, elmag;
  complex double arj, *scm = NULL;

  /* Allocate to scratch memory */
  size_t mreq = (size_t)ctx->geometry.np2m;
  mreq *= sizeof(complex double);
  mem_alloc(ctx, (void *)&scm, mreq );

  /* Un-transpose the matrix for Gauss elimination */
  for( i = 1; i < n; i++ )
	for( j = 0; j < i; j++ )
	{
	  arj = a[i+j*ndim];
	  a[i+j*ndim] = a[j+i*ndim];
	  a[j+i*ndim] = arj;
	}

  iflg=FALSE;
  /* step 1 */
  for( r = 0; r < n; r++ )
  {
	for( k = 0; k < n; k++ )
	  scm[k]= a[k+r*ndim];

	/* steps 2 and 3 */
	rm1= r;
	if( rm1 > 0)
	{
	  for( j = 0; j < rm1; j++ )
	  {
		pj= ip[j]-1;
		arj= scm[pj];
		a[j+r*ndim]= arj;
		scm[pj]= scm[j];
		jp1= j+1;

		for( i = jp1; i < n; i++ )
		  scm[i] -= a[i+j*ndim]* arj;

	  } /* for( j = 0; j < rm1; j++ ) */

	} /* if( rm1 >= 0.) */

	/* step 4 */
	dmax= creal( scm[r]*conj(scm[r]) );

	rp1= r+1;
	ip[r]= rp1;
	if( rp1 < n)
	{
	  for( i = rp1; i < n; i++ )
	  {
		elmag= creal( scm[i]* conj(scm[i]) );
		if( elmag >= dmax)
		{
		  dmax= elmag;
		  ip[r]= i+1;
		}
	  }
	} /* if( rp1 < n) */

	if( dmax < 1.e-10)
	  iflg=TRUE;

	pr= ip[r]-1;
	a[r+r*ndim]= scm[pr];
	scm[pr]= scm[r];

	/* step 5 */
	if( rp1 < n)
	{
	  arj=1./ a[r+r*ndim];

	  for( i = rp1; i < n; i++ )
		a[i+r*ndim]= scm[i]* arj;
	}

	if( iflg == TRUE )
	{
	  fprintf( ctx->output_fp,
		  "\n  PIVOT(%d)= %16.8E", r, dmax );
	  iflg=FALSE;
	}

  } /* for( r=0; r < n; r++ ) */

  mem_free(ctx, (void *)&scm );

  return;
}

/*-----------------------------------------------------------------------*/

/* factrs, for symmetric structure, transforms submatricies to form */
/* matricies of the symmetric modes and calls routine to factor */
/* matricies.  if no symmetry, the routine is called to factor the */
/* complete matrix. */
void factrs(nec_context_t *ctx, int np, int nrow, complex double *a, int *ip )
{
  int kk, ka;

  ctx->smat.nop = nrow/np;
  for( kk = 0; kk < ctx->smat.nop; kk++ )
  {
	ka= kk* np;
	factr(ctx, np, &a[ka], &ip[ka], nrow );
  }
  return;
}

/*-----------------------------------------------------------------------*/

/* fblock sets parameters for out-of-core */
/* solution for the primary matrix (a) */
void fblock(nec_context_t *ctx, int nrow, int ncol, int imax, int ipsym )
{
  int i, j, k, ka, kk;
  double phaz, arg;
  complex double deter;

  if( nrow*ncol <= imax)
  {
	ctx->matpar.npblk= nrow;
	ctx->matpar.nlast= nrow;
	ctx->matpar.imat= nrow* ncol;

	if( nrow == ncol)
	{
	  ctx->matpar.icase=1;
	  return;
	}
	else
	  ctx->matpar.icase=2;

  } /* if( nrow*ncol <= imax) */

  ctx->smat.nop = ncol/nrow;
  if( ctx->smat.nop*nrow != ncol)
  {
	fprintf( ctx->output_fp,
		"\n  SYMMETRY ERROR - NROW: %d NCOL: %d", nrow, ncol );
	stop(ctx, -1);
  }

  /* set up smat.ssx matrix for rotational symmetry. */
  if( ipsym <= 0)
  {
	phaz = TP / ctx->smat.nop;

	for( i = 1; i < ctx->smat.nop; i++ )
	{
	  for( j= i; j < ctx->smat.nop; j++ )
	  {
		arg = phaz * (double)i * (double)j;
		ctx->smat.ssx[i + j * ctx->smat.nop]= cmplx( cos( arg ), sin( arg) );
		ctx->smat.ssx[j + i * ctx->smat.nop]= ctx->smat.ssx[i + j * ctx->smat.nop];
	  }
	}
	return;

  } /* if( ipsym <= 0) */

  /* set up smat.ssx matrix for plane symmetry */
  kk=1;
  ctx->smat.ssx[0]=CPLX_10;

  k = 2;
  for( ka = 1; k != ctx->smat.nop; ka++ )
	k *= 2;

  for( k = 0; k < ka; k++ )
  {
	for( i = 0; i < kk; i++ )
	{
	  for( j = 0; j < kk; j++ )
	  {
		deter= ctx->smat.ssx[i+j*ctx->smat.nop];
		ctx->smat.ssx[i+(j+kk)*ctx->smat.nop]= deter;
		ctx->smat.ssx[i+kk+(j+kk)*ctx->smat.nop]= -deter;
		ctx->smat.ssx[i+kk+j*ctx->smat.nop]= deter;
	  }
	}
	kk *= 2;

  } /* for( k = 0; k < ka; k++ ) */

  return;
}

/*-----------------------------------------------------------------------*/


/* subroutine to solve the matrix equation lu*x=b where l is a unit */
/* lower triangular matrix and u is an upper triangular matrix both */
/* of which are stored in a.  the rhs vector b is input and the */
/* solution is returned through vector b.   (matrix transposed. */
void solve(nec_context_t *ctx, int n, complex double *a, int *ip,
    complex double *b, int ndim )
{
  int i, ip1, j, k, pia;
  complex double sum, *scm = NULL;

  /* Allocate to scratch memory */
  size_t mreq = (size_t)ctx->geometry.np2m;
  mreq *= sizeof(complex double);
  mem_alloc(ctx, (void *)&scm, mreq );

  /* forward substitution */
  for( i = 0; i < n; i++ )
  {
	pia= ip[i]-1;
	scm[i]= b[pia];
	b[pia]= b[i];
	ip1= i+1;

	if( ip1 < n)
	  for( j = ip1; j < n; j++ )
		b[j] -= a[j+i*ndim]* scm[i];
  }

  /* backward substitution */
  for( k = 0; k < n; k++ )
  {
	i= n-k-1;
	sum=CPLX_00;
	ip1= i+1;

	if( ip1 < n)
	  for( j = ip1; j < n; j++ )
		sum += a[i+j*ndim]* b[j];

	b[i]=( scm[i]- sum)/ a[i+i*ndim];
  }

  mem_free(ctx, (void *)&scm );

  return;
}

/*-----------------------------------------------------------------------*/

/* subroutine solves, for symmetric structures, handles the */
/* transformation of the right hand side vector and solution */
/* of the matrix eq. */
void solves(nec_context_t *ctx, complex double *a, int *ip, complex double *b,
    int neq, int nrh, int np, int n, int mp, int m)
{
  int npeq, nrow, ic, i, kk, ia, ib, j, k;
  double fnop, fnorm;
  complex double  sum, *scm = NULL;

  npeq= np+ 2*mp;
  ctx->smat.nop = neq/npeq;
  fnop= ctx->smat.nop;
  fnorm=1./ fnop;
  nrow= neq;

  /* Allocate to scratch memory */
  size_t mreq = (size_t)ctx->geometry.np2m;
  mreq *= sizeof(complex double);
  mem_alloc(ctx, (void *)&scm, mreq );

  if( ctx->smat.nop != 1)
  {
	for( ic = 0; ic < nrh; ic++ )
	{
	  if( (n != 0) && (m != 0) )
	  {
		for( i = 0; i < neq; i++ )
		  scm[i]= b[i+ic*neq];

		kk=2* mp;
		ia= np-1;
		ib= n-1;
		j= np-1;

		for( k = 0; k < ctx->smat.nop; k++ )
		{
		  if( k != 0 )
		  {
			for( i = 0; i < np; i++ )
			{
			  ia++;
			  j++;
			  b[j+ic*neq]= scm[ia];
			}

			if( k == (ctx->smat.nop-1) )
			  continue;

		  } /* if( k != 0 ) */

		  for( i = 0; i < kk; i++ )
		  {
			ib++;
			j++;
			b[j+ic*neq]= scm[ib];
		  }

		} /* for( k = 0; k < smat.nop; k++ ) */

	  } /* if( (n != 0) && (m != 0) ) */

	  /* transform matrix eq. rhs vector according to symmetry modes */
	  for( i = 0; i < npeq; i++ )
	  {
		for( k = 0; k < ctx->smat.nop; k++ )
		{
		  ia= i+ k* npeq;
		  scm[k]= b[ia+ic*neq];
		}

		sum= scm[0];
		for( k = 1; k < ctx->smat.nop; k++ )
		  sum += scm[k];

		b[i+ic*neq]= sum* fnorm;

		for( k = 1; k < ctx->smat.nop; k++ )
		{
		  ia= i+ k* npeq;
		  sum= scm[0];

		  for( j = 1; j < ctx->smat.nop; j++ )
			sum += scm[j]* conj( ctx->smat.ssx[k+j*ctx->smat.nop]);

		  b[ia+ic*neq]= sum* fnorm;
		}

	  } /* for( i = 0; i < npeq; i++ ) */

	} /* for( ic = 0; ic < nrh; ic++ ) */

  } /* if( smat.nop != 1) */

  /* solve each mode equation */
  for( kk = 0; kk < ctx->smat.nop; kk++ )
  {
     ia= kk* npeq;
     ib= ia;

     for( ic = 0; ic < nrh; ic++ )
       solve(ctx, npeq, &a[ib], &ip[ia], &b[ia+ic*neq], nrow );

   } /* for( kk = 0; kk < smat.nop; kk++ ) */

  if( ctx->smat.nop == 1)
  {
	mem_free(ctx, (void *)&scm );
	return;
  }

  /* inverse transform the mode solutions */
  for( ic = 0; ic < nrh; ic++ )
  {
	for( i = 0; i < npeq; i++ )
	{
	  for( k = 0; k < ctx->smat.nop; k++ )
	  {
		ia= i+ k* npeq;
		scm[k]= b[ia+ic*neq];
	  }

	  sum= scm[0];
	  for( k = 1; k < ctx->smat.nop; k++ )
		sum += scm[k];

	  b[i+ic*neq]= sum;
	  for( k = 1; k < ctx->smat.nop; k++ )
	  {
		ia= i+ k* npeq;
		sum= scm[0];

		for( j = 1; j < ctx->smat.nop; j++ )
		  sum += scm[j]* ctx->smat.ssx[k+j*ctx->smat.nop];

		b[ia+ic*neq]= sum;
	  }

	} /* for( i = 0; i < npeq; i++ ) */

	if( (n == 0) || (m == 0) )
	  continue;

	for( i = 0; i < neq; i++ )
	  scm[i]= b[i+ic*neq];

	kk=2* mp;
	ia= np-1;
	ib= n-1;
	j= np-1;

	for( k = 0; k < ctx->smat.nop; k++ )
	{
	  if( k != 0 )
	  {
		for( i = 0; i < np; i++ )
		{
		  ia++;
		  j++;
		  b[ia+ic*neq]= scm[j];
		}

		if( k == ctx->smat.nop)
		  continue;

	  } /* if( k != 0 ) */

	  for( i = 0; i < kk; i++ )
	  {
		ib++;
		j++;
		b[ib+ic*neq]= scm[j];
	  }

	} /* for( k = 0; k < smat.nop; k++ ) */

  } /* for( ic = 0; ic < nrh; ic++ ) */

  mem_free(ctx, (void *)&scm );

  return;
}

/*-----------------------------------------------------------------------*/
