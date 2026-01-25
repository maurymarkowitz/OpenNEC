/******************************************************************************
 * network.c
 *
 * Network solver and support routines for OpenNEC. This module solves for
 * structure currents for a given excitation, including the effect of
 * non-radiating networks if present. Provides core algorithms for network
 * analysis and integration with the main calculation engine.
 *
 * Extracted and refactored from legacy NEC code.
 *
 *****************************************************************************/

#include "opennec.h"

/*-------------------------------------------------------------------*/

/* network solves for structure currents for a given */
/* excitation including the effect of non-radiating networks if */
/* present. */
void network(nec_context_t *ctx, complex double *cm, int *ip, complex double *einc )
{
  int *ipnt = NULL, *nteqa = NULL, *ntsca = NULL;
  int nteq=0, ntsc=0, irow2=0, j, ndimn;
  int neqt, irow1=0, i, isc1=0;
  size_t mreq;
  double pwr;
  complex double *vsrc = NULL, *rhs = NULL, *cmn = NULL;
  complex double *rhnt = NULL, *rhnx = NULL, ymit, vlt, cux=CPLX_00;

  ctx->netcx.pin=0.;
  ctx->netcx.pnls=0.;
  neqt= ctx->netcx.neq+ ctx->netcx.neq2;
  ndimn = (2*ctx->netcx.nonet + ctx->vsorc.nsant);

  /* Calculate transmission line lengths from geometry if not specified */
  for (j = 0; j < ctx->netcx.nonet; j++) {
    if ((ctx->netcx.ntyp[j] >= 2) && (ctx->netcx.x11i[j] <= 0.0)) {
      int seg1_idx = ctx->netcx.iseg1[j] - 1;
      int seg2_idx = ctx->netcx.iseg2[j] - 1;
      double xx = ctx->geometry.x[seg2_idx] - ctx->geometry.x[seg1_idx];
      double yy = ctx->geometry.y[seg2_idx] - ctx->geometry.y[seg1_idx];
      double zz = ctx->geometry.z[seg2_idx] - ctx->geometry.z[seg1_idx];
      ctx->netcx.x11i[j] = ctx->geometry.wlam * sqrt(xx*xx + yy*yy + zz*zz);
    }
  }

  /* Allocate network buffers */
  if( ctx->netcx.nonet != 0 )
  {
    mreq = (size_t)ctx->geometry.np3m;
    mreq *= sizeof(complex double);
    mem_alloc(ctx, (void *)&rhs, mreq );

    mreq = (size_t)ndimn;
    mreq *= sizeof(complex double);
    mem_alloc(ctx, (void *)&rhnt, mreq );
    mem_alloc(ctx, (void *)&rhnx, mreq );
    mreq *= (size_t)ndimn;
    mem_alloc(ctx, (void *)&cmn, mreq );

    mreq = (size_t)ndimn;
    mreq *= sizeof(int);
    mem_alloc(ctx, (void *)&ntsca, mreq );
    mem_alloc(ctx, (void *)&nteqa, mreq );
    mem_alloc(ctx, (void *)&ipnt, mreq );

    mreq = (size_t)ctx->vsorc.nsant;
    mreq *= sizeof(complex double);
    mem_alloc(ctx, (void *)&vsrc, mreq );
  }
  else
  if( ctx->netcx.masym != 0)
  {
    mreq = (size_t)ndimn;
    mreq *= sizeof(int);
    mem_alloc(ctx, (void *)&ipnt, mreq );
  }

  if( ctx->netcx.ntsol == 0)
  {
    int nseg1;
    /* compute relative matrix asymmetry */
    if( ctx->netcx.masym != 0)
    {
      irow1=0;
      if( ctx->netcx.nonet != 0)
      {
        for( i = 0; i < ctx->netcx.nonet; i++ )
        {
          nseg1= ctx->netcx.iseg1[i];
          for( isc1 = 0; isc1 < 2; isc1++ )
          {
            if( irow1 == 0)
            {
              ipnt[irow1]= nseg1;
              nseg1= ctx->netcx.iseg2[i];
              irow1++;
              continue;
            }

            for( j = 0; j < irow1; j++ )
              if( nseg1 == ipnt[j])
                break;

            if( j == irow1 )
            {
              ipnt[irow1]= nseg1;
              irow1++;
            }

            nseg1= ctx->netcx.iseg2[i];

          } /* for( isc1 = 0; isc1 < 2; isc1++ ) */

        } /* for( i = 0; i < ctx->netcx.nonet; i++ ) */

      } /* if( ctx->netcx.nonet != 0) */

      if( ctx->vsorc.nsant != 0)
      {
        for( i = 0; i < ctx->vsorc.nsant; i++ )
        {
          nseg1= ctx->vsorc.isant[i];
          if( irow1 == 0)
          {
            ipnt[irow1]= nseg1;
            irow1++;
            continue;
          }

          for( j = 0; j < irow1; j++ )
            if( nseg1 == ipnt[j])
              break;

          if( j == irow1 )
          {
            ipnt[irow1]= nseg1;
            irow1++;
          }

        } /* for( i = 0; i < ctx->vsorc.nsant; i++ ) */

      } /* if( ctx->vsorc.nsant != 0) */

      if( irow1 >= 2)
      {
        double asmx, asa;
        for( i = 0; i < irow1; i++ )
        {
          isc1= ipnt[i]-1;
          asmx= ctx->geometry.si[isc1];

          for( j = 0; j < neqt; j++ )
            rhs[j] = CPLX_00;

          rhs[isc1] = CPLX_10;
          solves(ctx, cm, ip, rhs, ctx->netcx.neq, 1, ctx->geometry.np, ctx->geometry.n, ctx->geometry.mp, ctx->geometry.m);
          cabc(ctx, rhs);

          for( j = 0; j < irow1; j++ )
          {
            isc1= ipnt[j]-1;
            cmn[j+i*ndimn]= rhs[isc1]/ asmx;
          }

        } /* for( i = 0; i < irow1; i++ ) */

        asmx=0.;
        asa=0.;

        for( i = 1; i < irow1; i++ )
        {
          isc1= i;
          for( j = 0; j < isc1; j++ )
          {
            cux= cmn[i+j*ndimn];
            pwr= cabs(( cux- cmn[j+i*ndimn])/ cux);
            asa += pwr* pwr;

            if( pwr < asmx)
              continue;

            asmx= pwr;
            nteq= ipnt[i];
            ntsc= ipnt[j];

          } /* for( j = 0; j < isc1; j++ ) */

        } /* for( i = 1; i < irow1; i++ ) */

        asa= sqrt( asa*2./ (double)( irow1*( irow1-1)));
        
        /* Store asymmetry data in context for output */
        ctx->netcx.asmx = asmx;
        ctx->netcx.asa = asa;
        ctx->netcx.nteq_asym = nteq;
        ctx->netcx.ntsc_asym = ntsc;

      } /* if( irow1 >= 2) */

    } /* if( ctx->netcx.masym != 0) */

    /* solution of network equations */
    if( ctx->netcx.nonet != 0)
    {
      for( i = 0; i < ndimn; i++ )
      {
        rhnx[i]=CPLX_00;
        for( j = 0; j < ndimn; j++ )
          cmn[j+i*ndimn]=CPLX_00;
      }

      /* sort network and source data and */
      /* assign equation numbers to segments */
      nteq=0;
      ntsc=0;

      for( j = 0; j < ctx->netcx.nonet; j++ )
      {
        int jump1, jump2, isc2=0, nseg2;
        double y11r, y11i, y12r, y12i, y22r, y22i;
        int nseg1;

        nseg1= ctx->netcx.iseg1[j];
        nseg2= ctx->netcx.iseg2[j];

        if( ctx->netcx.ntyp[j] <= 1)
        {
          y11r= ctx->netcx.x11r[j];
          y11i= ctx->netcx.x11i[j];
          y12r= ctx->netcx.x12r[j];
          y12i= ctx->netcx.x12i[j];
          y22r= ctx->netcx.x22r[j];
          y22i= ctx->netcx.x22i[j];
        }
        else
        {
          y22r= TP* ctx->netcx.x11i[j]/ ctx->geometry.wlam;
          y12r=0.;
          y12i=1./( ctx->netcx.x11r[j]* sin( y22r));
          y11r= ctx->netcx.x12r[j];
          y11i= -y12i* cos( y22r);
          y22r= ctx->netcx.x22r[j];
          y22i= y11i+ ctx->netcx.x22i[j];
          y11i= y11i+ ctx->netcx.x12i[j];

          if( ctx->netcx.ntyp[j] != 2)
          {
            y12r= -y12r;
            y12i= -y12i;
          }

        } /* if( ctx->netcx.ntyp[j] <= 1) */

        jump1 = FALSE;
        if( ctx->vsorc.nsant != 0)
        {
          for( i = 0; i < ctx->vsorc.nsant; i++ )
            if( nseg1 == ctx->vsorc.isant[i])
            {
              isc1 = i;
              jump1 = TRUE;
              break;
            }
        } /* if( ctx->vsorc.nsant != 0) */

        jump2 = FALSE;
        if( ! jump1 )
        {
          isc1=-1;

          if( nteq != 0)
          {
            for( i = 0; i < nteq; i++ )
              if( nseg1 == nteqa[i])
              {
                irow1 = i;
                jump2 = TRUE;
                break;
              }

          } /* if( nteq != 0) */

          if( ! jump2 )
          {
            irow1= nteq;
            nteqa[nteq]= nseg1;
            nteq++;
          }

        } /* if( ! jump1 ) */
        else
        {
          if( ntsc != 0)
          {
            for( i = 0; i < ntsc; i++ )
            {
              if( nseg1 == ntsca[i])
              {
                irow1 = ndimn- (i+1);
                jump2 = TRUE;
                break;
              }
            }

          } /* if( ntsc != 0) */

          if( ! jump2 )
          {
            irow1= ndimn- (ntsc+1);
            ntsca[ntsc]= nseg1;
            vsrc[ntsc]= ctx->vsorc.vsant[isc1];
            ntsc++;
          }

        } /* if( ! jump1 ) */

        jump1 = FALSE;
        if( ctx->vsorc.nsant != 0)
        {
          for( i = 0; i < ctx->vsorc.nsant; i++ )
          {
            if( nseg2 == ctx->vsorc.isant[i])
            {
              isc2= i;
              jump1 = TRUE;
              break;
            }
          }

        } /* if( ctx->vsorc.nsant != 0) */

        jump2 = FALSE;
        if( ! jump1 )
        {
          isc2=-1;

          if( nteq != 0)
          {
            for( i = 0; i < nteq; i++ )
              if( nseg2 == nteqa[i])
              {
                irow2= i;
                jump2 = TRUE;
                break;
              }

          } /* if( nteq != 0) */

          if( ! jump2 )
          {
            irow2= nteq;
            nteqa[nteq]= nseg2;
            nteq++;
          }

        }  /* if( ! jump1 ) */
        else
        {
          if( ntsc != 0)
          {
            for( i = 0; i < ntsc; i++ )
              if( nseg2 == ntsca[i])
              {
                irow2 = ndimn- (i+1);
                jump2 = TRUE;
                break;
              }

          } /* if( ntsc != 0) */

          if( ! jump2 )
          {
            irow2= ndimn- (ntsc+1);
            ntsca[ntsc]= nseg2;
            vsrc[ntsc]= ctx->vsorc.vsant[isc2];
            ntsc++;
          }

        } /* if( ! jump1 ) */

        /* fill network equation matrix and right hand side vector with */
        /* network short-circuit admittance matrix coefficients. */
        if( isc1 == -1)
        {
          cmn[irow1+irow1*ndimn] -= cmplx( y11r, y11i)* ctx->geometry.si[nseg1-1];
          cmn[irow1+irow2*ndimn] -= cmplx( y12r, y12i)* ctx->geometry.si[nseg1-1];
        }
        else
        {
          rhnx[irow1] += cmplx( y11r, y11i)* ctx->vsorc.vsant[isc1]/ctx->geometry.wlam;
          rhnx[irow2] += cmplx( y12r, y12i)* ctx->vsorc.vsant[isc1]/ctx->geometry.wlam;
        }

        if( isc2 == -1)
        {
          cmn[irow2+irow2*ndimn] -= cmplx( y22r, y22i)* ctx->geometry.si[nseg2-1];
          cmn[irow2+irow1*ndimn] -= cmplx( y12r, y12i)* ctx->geometry.si[nseg2-1];
        }
        else
        {
          rhnx[irow1] += cmplx( y12r, y12i)* ctx->vsorc.vsant[isc2]/ctx->geometry.wlam;
          rhnx[irow2] += cmplx( y22r, y22i)* ctx->vsorc.vsant[isc2]/ctx->geometry.wlam;
        }

      } /* for( j = 0; j < ctx->netcx.nonet; j++ ) */

      /* add interaction matrix admittance */
      /* elements to network equation matrix */
      for( i = 0; i < nteq; i++ )
      {
        for( j = 0; j < neqt; j++ )
          rhs[j] = CPLX_00;

        irow1= nteqa[i]-1;
        rhs[irow1]=CPLX_10;
        solves(ctx, cm, ip, rhs, ctx->netcx.neq, 1, ctx->geometry.np, ctx->geometry.n, ctx->geometry.mp, ctx->geometry.m);
        cabc(ctx, rhs);

        for( j = 0; j < nteq; j++ )
        {
          irow1= nteqa[j]-1;
          cmn[i+j*ndimn] += rhs[irow1];
        }

      } /* for( i = 0; i < nteq; i++ ) */

      /* factor network equation matrix */
      factr(ctx, nteq, cmn, ipnt, ndimn);

    } /* if( ctx->netcx.nonet != 0) */

  } /* if( ctx->netcx.ntsol != 0) */

  if( ctx->netcx.nonet != 0)
  {
    /* add to network equation right hand side */
    /* the terms due to element interactions */
    for( i = 0; i < neqt; i++ )
      rhs[i]= einc[i];

    solves(ctx, cm, ip, rhs, ctx->netcx.neq, 1, ctx->geometry.np, ctx->geometry.n, ctx->geometry.mp, ctx->geometry.m);
    cabc(ctx, rhs);

    for( i = 0; i < nteq; i++ )
    {
      irow1= nteqa[i]-1;
      rhnt[i]= rhnx[i]+ rhs[irow1];
    }

    /* solve network equations */
    solve(ctx, nteq, cmn, ipnt, rhnt, ndimn);

    /* add fields due to network voltages to electric fields */
    /* applied to structure and solve for induced current */
    for( i = 0; i < nteq; i++ )
    {
      irow1= nteqa[i]-1;
      einc[irow1] -= rhnt[i];
    }

    solves(ctx, cm, ip, einc, ctx->netcx.neq, 1, ctx->geometry.np, ctx->geometry.n, ctx->geometry.mp, ctx->geometry.m);
    cabc(ctx, einc);

    /* Allocate arrays for network excitation data */
    ctx->netcx.nexc = nteq + ntsc;
    mem_realloc(ctx, (void **)&ctx->netcx.exc_tag, ctx->netcx.nexc * sizeof(int));
    mem_realloc(ctx, (void **)&ctx->netcx.exc_seg, ctx->netcx.nexc * sizeof(int));
    mem_realloc(ctx, (void **)&ctx->netcx.exc_v, ctx->netcx.nexc * sizeof(complex double));
    mem_realloc(ctx, (void **)&ctx->netcx.exc_i, ctx->netcx.nexc * sizeof(complex double));
    mem_realloc(ctx, (void **)&ctx->netcx.exc_z, ctx->netcx.nexc * sizeof(complex double));
    mem_realloc(ctx, (void **)&ctx->netcx.exc_y, ctx->netcx.nexc * sizeof(complex double));
    mem_realloc(ctx, (void **)&ctx->netcx.exc_pwr, ctx->netcx.nexc * sizeof(double));

    /* Store network excitation data */
    for( i = 0; i < nteq; i++ )
    {
      irow1= nteqa[i]-1;
      vlt= rhnt[i]* ctx->geometry.si[irow1]* ctx->geometry.wlam;
      cux= einc[irow1]* ctx->geometry.wlam;
      ymit= cux/ vlt;
      ctx->netcx.zped= vlt/ cux;
      irow2= ctx->geometry.tag_nums[irow1];
      pwr=.5* creal( vlt* conj( cux));
      ctx->netcx.pnls= ctx->netcx.pnls- pwr;

      /* Store in arrays */
      ctx->netcx.exc_tag[i] = irow2;
      ctx->netcx.exc_seg[i] = irow1 + 1;
      ctx->netcx.exc_v[i] = vlt;
      ctx->netcx.exc_i[i] = cux;
      ctx->netcx.exc_z[i] = ctx->netcx.zped;
      ctx->netcx.exc_y[i] = ymit;
      ctx->netcx.exc_pwr[i] = pwr;
    }

    if( ntsc != 0)
    {
      for( i = 0; i < ntsc; i++ )
      {
        irow1= ntsca[i]-1;
        vlt= vsrc[i];
        cux= einc[irow1]* ctx->geometry.wlam;
        ymit= cux/ vlt;
        ctx->netcx.zped= vlt/ cux;
        irow2= ctx->geometry.tag_nums[irow1];
        pwr=.5* creal( vlt* conj( cux));
        ctx->netcx.pnls= ctx->netcx.pnls- pwr;

        /* Store in arrays (offset by nteq) */
        int idx = nteq + i;
        ctx->netcx.exc_tag[idx] = irow2;
        ctx->netcx.exc_seg[idx] = irow1 + 1;
        ctx->netcx.exc_v[idx] = vlt;
        ctx->netcx.exc_i[idx] = cux;
        ctx->netcx.exc_z[idx] = ctx->netcx.zped;
        ctx->netcx.exc_y[idx] = ymit;
        ctx->netcx.exc_pwr[idx] = pwr;

      } /* for( i = 0; i < ntsc; i++ ) */

    } /* if( ntsc != 0) */

  } /* if( ctx->netcx.nonet != 0) */
  else
  {
    /* solve for currents when no networks are present */
    solves(ctx, cm, ip, einc, ctx->netcx.neq, 1, ctx->geometry.np, ctx->geometry.n, ctx->geometry.mp, ctx->geometry.m);
    cabc(ctx, einc);
    ntsc=0;
  }

  if( (ctx->vsorc.nsant+ctx->vsorc.nvqd) == 0)
  {
    mem_free( ctx, (void *)&ipnt );
    return;
  }

  /* Allocate arrays for antenna input parameters */
  ctx->netcx.ninp = ctx->vsorc.nsant + ctx->vsorc.nvqd;
  mem_realloc(ctx, (void **)&ctx->netcx.inp_tag, ctx->netcx.ninp * sizeof(int));
  mem_realloc(ctx, (void **)&ctx->netcx.inp_seg, ctx->netcx.ninp * sizeof(int));
  mem_realloc(ctx, (void **)&ctx->netcx.inp_v, ctx->netcx.ninp * sizeof(complex double));
  mem_realloc(ctx, (void **)&ctx->netcx.inp_i, ctx->netcx.ninp * sizeof(complex double));
  mem_realloc(ctx, (void **)&ctx->netcx.inp_z, ctx->netcx.ninp * sizeof(complex double));
  mem_realloc(ctx, (void **)&ctx->netcx.inp_y, ctx->netcx.ninp * sizeof(complex double));
  mem_realloc(ctx, (void **)&ctx->netcx.inp_pwr, ctx->netcx.ninp * sizeof(double));

  if( ctx->vsorc.nsant != 0)
  {
    for( i = 0; i < ctx->vsorc.nsant; i++ )
    {
      isc1= ctx->vsorc.isant[i]-1;
      vlt= ctx->vsorc.vsant[i];

      if( ntsc == 0)
      {
        cux= einc[isc1]* ctx->geometry.wlam;
        irow1=0;
      }
      else
      {
        for( j = 0; j < ntsc; j++ )
          if( ntsca[j] == isc1+1)
            break;

        irow1= ndimn- (j+1);
        cux= rhnx[irow1];
        for( j = 0; j < nteq; j++ )
          cux -= cmn[j+irow1*ndimn]*rhnt[j];
        cux=(einc[isc1]+ cux)* ctx->geometry.wlam;
        irow1++;

      } /* if( ntsc == 0) */

      ymit= cux/ vlt;
      ctx->netcx.zped= vlt/ cux;
      pwr=.5* creal( vlt* conj( cux));
      ctx->netcx.pin= ctx->netcx.pin+ pwr;

      if( irow1 != 0)
        ctx->netcx.pnls= ctx->netcx.pnls+ pwr;

      irow2= ctx->geometry.tag_nums[isc1];
      
      /* Store in arrays */
      ctx->netcx.inp_tag[i] = irow2;
      ctx->netcx.inp_seg[i] = isc1 + 1;
      ctx->netcx.inp_v[i] = vlt;
      ctx->netcx.inp_i[i] = cux;
      ctx->netcx.inp_z[i] = ctx->netcx.zped;
      ctx->netcx.inp_y[i] = ymit;
      ctx->netcx.inp_pwr[i] = pwr;

    } /* for( i = 0; i < ctx->vsorc.nsant; i++ ) */

  } /* if( ctx->vsorc.nsant != 0) */

  if( ctx->vsorc.nvqd != 0)
    for( i = 0; i < ctx->vsorc.nvqd; i++ )
    {
      isc1= ctx->vsorc.ivqd[i]-1;
      vlt= ctx->vsorc.vqd[i];
      cux= cmplx( ctx->crnt.air[isc1], ctx->crnt.aii[isc1]);
      ymit= cmplx( ctx->crnt.bir[isc1], ctx->crnt.bii[isc1]);
      ctx->netcx.zped= cmplx( ctx->crnt.cir[isc1], ctx->crnt.cii[isc1]);
      pwr= ctx->geometry.si[isc1]* TP*.5;
      cux=( cux- ymit* sin( pwr)+ ctx->netcx.zped* cos( pwr))* ctx->geometry.wlam;
      ymit= cux/ vlt;
      ctx->netcx.zped= vlt/ cux;
      pwr=.5* creal( vlt* conj( cux));
      ctx->netcx.pin= ctx->netcx.pin+ pwr;
      irow2= ctx->geometry.tag_nums[isc1];

      /* Store in arrays (offset by nsant) */
      int idx = ctx->vsorc.nsant + i;
      ctx->netcx.inp_tag[idx] = irow2;
      ctx->netcx.inp_seg[idx] = isc1 + 1;
      ctx->netcx.inp_v[idx] = vlt;
      ctx->netcx.inp_i[idx] = cux;
      ctx->netcx.inp_z[idx] = ctx->netcx.zped;
      ctx->netcx.inp_y[idx] = ymit;
      ctx->netcx.inp_pwr[idx] = pwr;

    } /* for( i = 0; i < ctx->vsorc.nvqd; i++ ) */

  /* Free network buffers */
  mem_free( ctx, (void *)&ipnt );
  mem_free( ctx, (void *)&nteqa );
  mem_free( ctx, (void *)&ntsca );
  mem_free( ctx, (void *)&vsrc );
  mem_free( ctx, (void *)&rhs );
  mem_free( ctx, (void *)&cmn );
  mem_free( ctx, (void *)&rhnt );
  mem_free( ctx, (void *)&rhnx );

  return;
}

/*-----------------------------------------------------------------------*/

