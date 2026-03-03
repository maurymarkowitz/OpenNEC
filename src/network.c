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

#include "internals.h"
#include "network.h"
#include "matrix.h"
#include "calculations.h"

/*-------------------------------------------------------------------*/

/* network solves for structure currents for a given */
/* excitation including the effect of non-radiating networks if */
/* present. */
void network(nec_context_t *restrict ctx, complex double *restrict cm, int *restrict ip, complex double *restrict einc )
{
  int *ipnt = NULL, *nteqa = NULL, *ntsca = NULL;
  int nteq=0, ntsc=0, irow2=0, j, ndimn;
  int neqt, irow1=0, i, isc1=0;
  size_t mreq;
  double pwr;
  complex double *vsrc = NULL, *rhs = NULL, *cmn = NULL;
  complex double *rhnt = NULL, *rhnx = NULL, ymit, vlt, cux=CPLX_00;

  ctx->netcx.power_in=0.;
  ctx->netcx.power_net_loss=0.;
  neqt= ctx->netcx.num_eq+ ctx->netcx.num_eq_ngf;
  ndimn = (2*ctx->netcx.num_networks + ctx->vsorc.num_vsrcs);

  /* Calculate transmission line lengths from geometry if not specified */
  for (j = 0; j < ctx->netcx.num_networks; j++) {
    if ((ctx->netcx.net_types[j] >= 2) && (ctx->netcx.y11_imag[j] <= 0.0)) {
      int seg1_idx = ctx->netcx.net_seg1[j] - 1;
      int seg2_idx = ctx->netcx.net_seg2[j] - 1;
      double xx = ctx->geometry.x_center[seg2_idx] - ctx->geometry.x_center[seg1_idx];
      double yy = ctx->geometry.y_center[seg2_idx] - ctx->geometry.y_center[seg1_idx];
      double zz = ctx->geometry.z_center[seg2_idx] - ctx->geometry.z_center[seg1_idx];
      ctx->netcx.y11_imag[j] = ctx->geometry.wavelength * sqrt(xx*xx + yy*yy + zz*zz);
    }
  }

  /* Allocate network buffers */
  if( ctx->netcx.num_networks != 0 )
  {
    mreq = (size_t)ctx->geometry.num_segs_3xpatches;
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

    mreq = (size_t)ctx->vsorc.num_vsrcs;
    mreq *= sizeof(complex double);
    mem_alloc(ctx, (void *)&vsrc, mreq );
  }
  else
  if( ctx->netcx.check_asymmetry != 0)
  {
    mreq = (size_t)ndimn;
    mreq *= sizeof(int);
    mem_alloc(ctx, (void *)&ipnt, mreq );
  }

  if( ctx->netcx.network_type == 0)
  {
    int nseg1;
    /* compute relative matrix asymmetry */
    if( ctx->netcx.check_asymmetry != 0)
    {
      irow1=0;
      if( ctx->netcx.num_networks != 0)
      {
        for( i = 0; i < ctx->netcx.num_networks; i++ )
        {
          nseg1= ctx->netcx.net_seg1[i];
          for( isc1 = 0; isc1 < 2; isc1++ )
          {
            if( irow1 == 0)
            {
              ipnt[irow1]= nseg1;
              nseg1= ctx->netcx.net_seg2[i];
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

            nseg1= ctx->netcx.net_seg2[i];

          } /* for( isc1 = 0; isc1 < 2; isc1++ ) */

        } /* for( i = 0; i < ctx->netcx.num_networks; i++ ) */

      } /* if( ctx->netcx.num_networks != 0) */

      if( ctx->vsorc.num_vsrcs != 0)
      {
        for( i = 0; i < ctx->vsorc.num_vsrcs; i++ )
        {
          nseg1= ctx->vsorc.vsrc_segs[i];
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

        } /* for( i = 0; i < ctx->vsorc.num_vsrcs; i++ ) */

      } /* if( ctx->vsorc.num_vsrcs != 0) */

      if( irow1 >= 2)
      {
        double asmx, asa;
        for( i = 0; i < irow1; i++ )
        {
          isc1= ipnt[i]-1;
          asmx= ctx->geometry.half_len[isc1];

          for( j = 0; j < neqt; j++ )
            rhs[j] = CPLX_00;

          rhs[isc1] = CPLX_10;
          solve_symmetric(ctx, cm, ip, rhs, ctx->netcx.num_eq, 1, ctx->geometry.num_segs_sym, ctx->geometry.num_segs, ctx->geometry.num_patches_sym, ctx->geometry.num_patches);
          compute_current_coefficients(ctx, rhs);

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
        ctx->netcx.max_asymmetry = asmx;
        ctx->netcx.rms_asymmetry = asa;
        ctx->netcx.nteq_asym = nteq;
        ctx->netcx.ntsc_asym = ntsc;

      } /* if( irow1 >= 2) */

    } /* if( ctx->netcx.check_asymmetry != 0) */

    /* solution of network equations */
    if( ctx->netcx.num_networks != 0)
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

      for( j = 0; j < ctx->netcx.num_networks; j++ )
      {
        bool jump1, jump2;
        int isc2=0, nseg2;
        double y11r, y11i, y12r, y12i, y22r, y22i;
        int nseg1;

        nseg1= ctx->netcx.net_seg1[j];
        nseg2= ctx->netcx.net_seg2[j];

        if( ctx->netcx.net_types[j] <= 1)
        {
          y11r= ctx->netcx.y11_real[j];
          y11i= ctx->netcx.y11_imag[j];
          y12r= ctx->netcx.y12_real[j];
          y12i= ctx->netcx.y12_imag[j];
          y22r= ctx->netcx.y22_real[j];
          y22i= ctx->netcx.y22_imag[j];
        }
        else
        {
          y22r= TP* ctx->netcx.y11_imag[j]/ ctx->geometry.wavelength;
          y12r=0.;
          y12i=1./( ctx->netcx.y11_real[j]* sin( y22r));
          y11r= ctx->netcx.y12_real[j];
          y11i= -y12i* cos( y22r);
          y22r= ctx->netcx.y22_real[j];
          y22i= y11i+ ctx->netcx.y22_imag[j];
          y11i= y11i+ ctx->netcx.y12_imag[j];

          if( ctx->netcx.net_types[j] != 2)
          {
            y12r= -y12r;
            y12i= -y12i;
          }

        } /* if( ctx->netcx.net_types[j] <= 1) */

        jump1 = false;
        if( ctx->vsorc.num_vsrcs != 0)
        {
          for( i = 0; i < ctx->vsorc.num_vsrcs; i++ )
            if( nseg1 == ctx->vsorc.vsrc_segs[i])
            {
              isc1 = i;
              jump1 = true;
              break;
            }
        } /* if( ctx->vsorc.num_vsrcs != 0) */

        jump2 = false;
        if( ! jump1 )
        {
          isc1=-1;

          if( nteq != 0)
          {
            for( i = 0; i < nteq; i++ )
              if( nseg1 == nteqa[i])
              {
                irow1 = i;
                jump2 = true;
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
                jump2 = true;
                break;
              }
            }

          } /* if( ntsc != 0) */

          if( ! jump2 )
          {
            irow1= ndimn- (ntsc+1);
            ntsca[ntsc]= nseg1;
            vsrc[ntsc]= ctx->vsorc.vsrc_voltages[isc1];
            ntsc++;
          }

        } /* if( ! jump1 ) */

        jump1 = false;
        if( ctx->vsorc.num_vsrcs != 0)
        {
          for( i = 0; i < ctx->vsorc.num_vsrcs; i++ )
          {
            if( nseg2 == ctx->vsorc.vsrc_segs[i])
            {
              isc2= i;
              jump1 = true;
              break;
            }
          }

        } /* if( ctx->vsorc.num_vsrcs != 0) */

        jump2 = false;
        if( ! jump1 )
        {
          isc2=-1;

          if( nteq != 0)
          {
            for( i = 0; i < nteq; i++ )
              if( nseg2 == nteqa[i])
              {
                irow2= i;
                jump2 = true;
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
                jump2 = true;
                break;
              }

          } /* if( ntsc != 0) */

          if( ! jump2 )
          {
            irow2= ndimn- (ntsc+1);
            ntsca[ntsc]= nseg2;
            vsrc[ntsc]= ctx->vsorc.vsrc_voltages[isc2];
            ntsc++;
          }

        } /* if( ! jump1 ) */

        /* fill network equation matrix and right hand side vector with */
        /* network short-circuit admittance matrix coefficients. */
        if( isc1 == -1)
        {
          cmn[irow1+irow1*ndimn] -= cmplx( y11r, y11i)* ctx->geometry.half_len[nseg1-1];
          cmn[irow1+irow2*ndimn] -= cmplx( y12r, y12i)* ctx->geometry.half_len[nseg1-1];
        }
        else
        {
          rhnx[irow1] += cmplx( y11r, y11i)* ctx->vsorc.vsrc_voltages[isc1]/ctx->geometry.wavelength;
          rhnx[irow2] += cmplx( y12r, y12i)* ctx->vsorc.vsrc_voltages[isc1]/ctx->geometry.wavelength;
        }

        if( isc2 == -1)
        {
          cmn[irow2+irow2*ndimn] -= cmplx( y22r, y22i)* ctx->geometry.half_len[nseg2-1];
          cmn[irow2+irow1*ndimn] -= cmplx( y12r, y12i)* ctx->geometry.half_len[nseg2-1];
        }
        else
        {
          rhnx[irow1] += cmplx( y12r, y12i)* ctx->vsorc.vsrc_voltages[isc2]/ctx->geometry.wavelength;
          rhnx[irow2] += cmplx( y22r, y22i)* ctx->vsorc.vsrc_voltages[isc2]/ctx->geometry.wavelength;
        }

      } /* for( j = 0; j < ctx->netcx.num_networks; j++ ) */

      /* add interaction matrix admittance */
      /* elements to network equation matrix */
      for( i = 0; i < nteq; i++ )
      {
        for( j = 0; j < neqt; j++ )
          rhs[j] = CPLX_00;

        irow1= nteqa[i]-1;
        rhs[irow1]=CPLX_10;
        solve_symmetric(ctx, cm, ip, rhs, ctx->netcx.num_eq, 1, ctx->geometry.num_segs_sym, ctx->geometry.num_segs, ctx->geometry.num_patches_sym, ctx->geometry.num_patches);
        compute_current_coefficients(ctx, rhs);

        for( j = 0; j < nteq; j++ )
        {
          irow1= nteqa[j]-1;
          cmn[i+j*ndimn] += rhs[irow1];
        }

      } /* for( i = 0; i < nteq; i++ ) */

      /* factor network equation matrix */
      factor_matrix(ctx, nteq, cmn, ipnt, ndimn);

    } /* if( ctx->netcx.num_networks != 0) */

  } /* if( ctx->netcx.network_type != 0) */

  if( ctx->netcx.num_networks != 0)
  {
    /* add to network equation right hand side */
    /* the terms due to element interactions */
    for( i = 0; i < neqt; i++ )
      rhs[i]= einc[i];

    solve_symmetric(ctx, cm, ip, rhs, ctx->netcx.num_eq, 1, ctx->geometry.num_segs_sym, ctx->geometry.num_segs, ctx->geometry.num_patches_sym, ctx->geometry.num_patches);
    compute_current_coefficients(ctx, rhs);

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

    solve_symmetric(ctx, cm, ip, einc, ctx->netcx.num_eq, 1, ctx->geometry.num_segs_sym, ctx->geometry.num_segs, ctx->geometry.num_patches_sym, ctx->geometry.num_patches);
    compute_current_coefficients(ctx, einc);

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
      vlt= rhnt[i]* ctx->geometry.half_len[irow1]* ctx->geometry.wavelength;
      cux= einc[irow1]* ctx->geometry.wavelength;
      ymit= cux/ vlt;
      ctx->netcx.input_impedance= vlt/ cux;
      irow2= ctx->geometry.tag_nums[irow1];
      pwr=.5* creal( vlt* conj( cux));
      ctx->netcx.power_net_loss= ctx->netcx.power_net_loss- pwr;

      /* Store in arrays */
      ctx->netcx.exc_tag[i] = irow2;
      ctx->netcx.exc_seg[i] = irow1 + 1;
      ctx->netcx.exc_v[i] = vlt;
      ctx->netcx.exc_i[i] = cux;
      ctx->netcx.exc_z[i] = ctx->netcx.input_impedance;
      ctx->netcx.exc_y[i] = ymit;
      ctx->netcx.exc_pwr[i] = pwr;
    }

    if( ntsc != 0)
    {
      for( i = 0; i < ntsc; i++ )
      {
        irow1= ntsca[i]-1;
        vlt= vsrc[i];
        cux= einc[irow1]* ctx->geometry.wavelength;
        ymit= cux/ vlt;
        ctx->netcx.input_impedance= vlt/ cux;
        irow2= ctx->geometry.tag_nums[irow1];
        pwr=.5* creal( vlt* conj( cux));
        ctx->netcx.power_net_loss= ctx->netcx.power_net_loss- pwr;

        /* Store in arrays (offset by nteq) */
        int idx = nteq + i;
        ctx->netcx.exc_tag[idx] = irow2;
        ctx->netcx.exc_seg[idx] = irow1 + 1;
        ctx->netcx.exc_v[idx] = vlt;
        ctx->netcx.exc_i[idx] = cux;
        ctx->netcx.exc_z[idx] = ctx->netcx.input_impedance;
        ctx->netcx.exc_y[idx] = ymit;
        ctx->netcx.exc_pwr[idx] = pwr;

      } /* for( i = 0; i < ntsc; i++ ) */

    } /* if( ntsc != 0) */

  } /* if( ctx->netcx.num_networks != 0) */
  else
  {
    /* solve for currents when no networks are present */
    solve_symmetric(ctx, cm, ip, einc, ctx->netcx.num_eq, 1, ctx->geometry.num_segs_sym, ctx->geometry.num_segs, ctx->geometry.num_patches_sym, ctx->geometry.num_patches);
    compute_current_coefficients(ctx, einc);
    ntsc=0;
  }

  if( (ctx->vsorc.num_vsrcs+ctx->vsorc.num_qdsrcs) == 0)
  {
    mem_free( ctx, (void *)&ipnt );
    return;
  }

  /* Allocate arrays for antenna input parameters */
  ctx->netcx.ninp = ctx->vsorc.num_vsrcs + ctx->vsorc.num_qdsrcs;
  mem_realloc(ctx, (void **)&ctx->netcx.inp_tag, ctx->netcx.ninp * sizeof(int));
  mem_realloc(ctx, (void **)&ctx->netcx.inp_seg, ctx->netcx.ninp * sizeof(int));
  mem_realloc(ctx, (void **)&ctx->netcx.inp_v, ctx->netcx.ninp * sizeof(complex double));
  mem_realloc(ctx, (void **)&ctx->netcx.inp_i, ctx->netcx.ninp * sizeof(complex double));
  mem_realloc(ctx, (void **)&ctx->netcx.inp_z, ctx->netcx.ninp * sizeof(complex double));
  mem_realloc(ctx, (void **)&ctx->netcx.inp_y, ctx->netcx.ninp * sizeof(complex double));
  mem_realloc(ctx, (void **)&ctx->netcx.inp_pwr, ctx->netcx.ninp * sizeof(double));

  if( ctx->vsorc.num_vsrcs != 0)
  {
    for( i = 0; i < ctx->vsorc.num_vsrcs; i++ )
    {
      isc1= ctx->vsorc.vsrc_segs[i]-1;
      vlt= ctx->vsorc.vsrc_voltages[i];

      if( ntsc == 0)
      {
        cux= einc[isc1]* ctx->geometry.wavelength;
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
        cux=(einc[isc1]+ cux)* ctx->geometry.wavelength;
        irow1++;

      } /* if( ntsc == 0) */

      ymit= cux/ vlt;
      ctx->netcx.input_impedance= vlt/ cux;
      pwr=.5* creal( vlt* conj( cux));
      ctx->netcx.power_in= ctx->netcx.power_in+ pwr;

      if( irow1 != 0)
        ctx->netcx.power_net_loss= ctx->netcx.power_net_loss+ pwr;

      irow2= ctx->geometry.tag_nums[isc1];
      
      /* Store in arrays */
      ctx->netcx.inp_tag[i] = irow2;
      ctx->netcx.inp_seg[i] = isc1 + 1;
      ctx->netcx.inp_v[i] = vlt;
      ctx->netcx.inp_i[i] = cux;
      ctx->netcx.inp_z[i] = ctx->netcx.input_impedance;
      ctx->netcx.inp_y[i] = ymit;
      ctx->netcx.inp_pwr[i] = pwr;

    } /* for( i = 0; i < ctx->vsorc.num_vsrcs; i++ ) */

  } /* if( ctx->vsorc.num_vsrcs != 0) */

  if( ctx->vsorc.num_qdsrcs != 0)
    for( i = 0; i < ctx->vsorc.num_qdsrcs; i++ )
    {
      isc1= ctx->vsorc.qdsrc_segs[i]-1;
      vlt= ctx->vsorc.qdsrc_voltages[i];
      cux= cmplx( ctx->crnt.a_real[isc1], ctx->crnt.a_imag[isc1]);
      ymit= cmplx( ctx->crnt.b_real[isc1], ctx->crnt.b_imag[isc1]);
      ctx->netcx.input_impedance= cmplx( ctx->crnt.c_real[isc1], ctx->crnt.c_imag[isc1]);
      pwr= ctx->geometry.half_len[isc1]* TP*.5;
      cux=( cux- ymit* sin( pwr)+ ctx->netcx.input_impedance* cos( pwr))* ctx->geometry.wavelength;
      ymit= cux/ vlt;
      ctx->netcx.input_impedance= vlt/ cux;
      pwr=.5* creal( vlt* conj( cux));
      ctx->netcx.power_in= ctx->netcx.power_in+ pwr;
      irow2= ctx->geometry.tag_nums[isc1];

      /* Store in arrays (offset by nsant) */
      int idx = ctx->vsorc.num_vsrcs + i;
      ctx->netcx.inp_tag[idx] = irow2;
      ctx->netcx.inp_seg[idx] = isc1 + 1;
      ctx->netcx.inp_v[idx] = vlt;
      ctx->netcx.inp_i[idx] = cux;
      ctx->netcx.inp_z[idx] = ctx->netcx.input_impedance;
      ctx->netcx.inp_y[idx] = ymit;
      ctx->netcx.inp_pwr[idx] = pwr;

    } /* for( i = 0; i < ctx->vsorc.num_qdsrcs; i++ ) */

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

