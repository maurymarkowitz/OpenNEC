/******************************************************************************
 * ground.c
 *
 * ground.c contains routines for computing the contribution of the ground
 * (earth) to the electric field produced by wire segments. It implements
 * numerical integration and reflection models to account for various ground
 * configurations used in NEC, including perfect, Sommerfeld, and radial wire
 * ground screen approximations.
 *
 * Major responsibilities include:
 * - Integrating ground-influenced field components using variable-interval
 *   Romberg integration (rom2()).
 * - Computing field due to ground for a current element on the source segment
 *   at position t relative to the segment center (sflds()).
 * - Applying reflection coefficients and handling special cases like the
 *   radial wire ground screen when enabled.
 * - Producing the x, y, z components of the field for constant, sine, and
 *   cosine current distributions.
 *
 * These routines operate on the shared nec_context_t state, reading geometry,
 * ground parameters, and segment/source data from the common blocks to derive
 * coefficients and accumulate results for near-field and radiation calculations.
 *****************************************************************************/

#include <assert.h>
#include "internals.h"
#include "ground.h"
#include "calculations.h"
#include "fields.h"
#include "misc.h"

/*-------------------------------------------------------------------*/

/* segment to obtain the total field due to ground. the method of */
/* variable interval width romberg integration is used.  there are 9 */
/* field components - the x, y, and z components due to constant, */
/* sine, and cosine current distributions. */
int rom2(nec_context_t *restrict ctx, double a, double b, complex double *restrict sum, double dmin)
{
  int i, ns, nt;
  bool flag=true;
  int nts = 4, nx = 1, n = 9;
  double ze, ep, zend, dz=0., dzot=0., tmag1, tmag2, tr, ti;
  double z, s; /***also global***/
  double rx = 1.0e-4;
  complex double g1[9], g2[9], g3[9], g4[9], g5[9];
  complex double t00, t01[9], t10[9], t02, t11, t20[9];
  
  z= a;
  ze= b;
  s= b- a;
  
  assert(s >= 0. && "INTERNAL: rom2() called with b < a; segment length is negative (geometry corruption)");
  
  ep= s/(1.e4* ctx->geometry.npm);
  zend= ze- ep;
  
  for( i = 0; i < n; i++ )
    sum[i]=CPLX_00;
  
  ns= nx;
  nt=0;
  sflds(ctx, z, g1);
  
  while( true )
  {
    if( flag )
    {
      dz= s/ ns;
      if( z+ dz > ze)
      {
        dz= ze- z;
        if( dz <= ep)
          return 0;
      }
      
      dzot= dz*.5;
      sflds(ctx, z+ dzot, g3);
      sflds(ctx, z+ dz, g5);
      
    } /* if( flag ) */
    
    tmag1=0.;
    tmag2=0.;
    
    /* evaluate 3 point romberg result and test convergence. */
    for( i = 0; i < n; i++ )
    {
      t00=( g1[i]+ g5[i])* dzot;
      t01[i]=( t00+ dz* g3[i])*.5;
      t10[i]=(4.* t01[i]- t00)/3.;
      if( i > 2)
        continue;
      
      tr= creal( t01[i]);
      ti= cimag( t01[i]);
      tmag1= tmag1+ tr* tr+ ti* ti;
      tr= creal( t10[i]);
      ti= cimag( t10[i]);
      tmag2= tmag2+ tr* tr+ ti* ti;
      
    } /* for( i = 0; i < n; i++ ) */
    
    tmag1= sqrt( tmag1);
    tmag2= sqrt( tmag2);
    test(ctx, tmag1, tmag2, &tr, 0., 0., &ti, dmin);
    
    if( tr <= rx)
    {
      for( i = 0; i < n; i++ )
        sum[i] += t10[i];
      nt += 2;
      
      z += dz;
      if( z > zend)
        return 0;
      
      for( i = 0; i < n; i++ )
        g1[i]= g5[i];
      
      if( (nt >= nts) && (ns > nx) )
      {
        ns= ns/2;
        nt=1;
      }
      flag = true;
      continue;
      
    } /* if( tr <= rx) */
    
    sflds(ctx, z+ dz*.25, g2);
    sflds(ctx, z+ dz*.75, g4);
    tmag1=0.;
    tmag2=0.;
    
    /* evaluate 5 point romberg result and test convergence. */
    for( i = 0; i < n; i++ )
    {
      t02=( t01[i]+ dzot*( g2[i]+ g4[i]))*.5;
      t11=( 4.0 * t02- t01[i] )/3.;
      t20[i]=(16.* t11- t10[i])/15.;
      if( i > 2)
        continue;
      
      tr= creal( t11);
      ti= cimag( t11);
      tmag1= tmag1+ tr* tr+ ti* ti;
      tr= creal( t20[i]);
      ti= cimag( t20[i]);
      tmag2= tmag2+ tr* tr+ ti* ti;
      
    } /* for( i = 0; i < n; i++ ) */
    
    tmag1= sqrt( tmag1);
    tmag2= sqrt( tmag2);
    test(ctx, tmag1, tmag2, &tr, 0.,0., &ti, dmin);
    
    if( tr > rx)
    {
      nt=0;
      if( ns < ctx->geometry.npm )
      {
        ns= ns*2;
        dz= s/ ns;
        dzot= dz*.5;
        
        for( i = 0; i < n; i++ )
        {
          g5[i]= g3[i];
          g3[i]= g2[i];
        }
        
        flag=false;
        continue;
        
      } /* if( ns < npm) */
      
      if (!ctx->step_size_warned) {
        ctx->step_size_warned = true;
        nec_report(ctx, ONEC_SEV_WARNING,
          "Step size limited at Z= %12.5E near source segment (%.4g, %.4g, %.4g);"
          " further occurrences suppressed",
          z, ctx->dataj.src_x, ctx->dataj.src_y, ctx->dataj.src_z);
      }
      
    } /* if( tr > rx) */
    
    for( i = 0; i < n; i++ )
      sum[i]= sum[i]+ t20[i];
    nt= nt+1;
    
    z= z+ dz;
    if( z > zend)
      return 0;
    
    for( i = 0; i < n; i++ )
      g1[i]= g5[i];
    
    flag = true;
    if( (nt < nts) || (ns <= nx) )
      continue;
    
    ns= ns/2;
    nt=1;
  } /* while( true ) */
  
}

/*-----------------------------------------------------------------------*/

/* sfldx returns the field due to ground for a current element on */
/* the source segment at t relative to the segment center. */
void sflds(nec_context_t *restrict ctx, double t, complex double *restrict e )
{
  double xt, yt, zt, rhx, rhy, rhs, rho, phx, phy;
  double cph, sph, zphs, r2s, rk, sfac, thet;
  complex double  erv, ezv, erh, ezh, eph, er, et, hrv, hzv, hrh;
  
  xt= ctx->dataj.src_x+ t* ctx->dataj.src_dir_cos_x;
  yt= ctx->dataj.src_y+ t* ctx->dataj.src_dir_cos_y;
  zt= ctx->dataj.src_z+ t* ctx->dataj.src_dir_cos_z;
  rhx= ctx->incom.obs_x- xt;
  rhy= ctx->incom.obs_y- yt;
  rhs= rhx* rhx+ rhy* rhy;
  rho= sqrt( rhs);
  
  if( rho <= 0.)
  {
    rhx=1.;
    rhy=0.;
    phx=0.;
    phy=1.;
  }
  else
  {
    rhx= rhx/ rho;
    rhy= rhy/ rho;
    phx= -rhy;
    phy= rhx;
  }
  
  cph= rhx* ctx->incom.dir_cos_x+ rhy* ctx->incom.dir_cos_y;
  sph= rhy* ctx->incom.dir_cos_x- rhx* ctx->incom.dir_cos_y;
  
  if( fabs( cph) < 1.0e-10)
    cph=0.;
  if( fabs( sph) < 1.0e-10)
    sph=0.;
  
  ctx->gwav.z_img2= ctx->incom.obs_z+ zt;
  zphs= ctx->gwav.z_img2* ctx->gwav.z_img2;
  r2s= rhs+ zphs;
  ctx->gwav.range2= sqrt( r2s);
  rk= ctx->gwav.range2* TP;
  ctx->gwav.cur_phase2= cmplx( cos( rk),- sin( rk));
  
  /* use norton approximation for field due to ground.  current is */
  /* lumped at segment center with current moment for constant, sine, */
  /* or cosine distribution. */
  if( ctx->incom.use_sommerfeld != 1)
  {
    ctx->gwav.z_img1=1.;
    ctx->gwav.range1=1.;
    ctx->gwav.cur_phase1=0.;
    gwave(ctx, &erv, &ezv, &erh, &ezh, &eph);
    
    et=-CONST1* ctx->gnd.fresnel_ratio* ctx->gwav.cur_phase2/( r2s* ctx->gwav.range2);
    er=2.* et* cmplx(1.0, rk);
    et= et* cmplx(1.0 - rk* rk, rk);
    hrv=( er+ et)* rho* ctx->gwav.z_img2/ r2s;
    hzv=( zphs* er- rhs* et)/ r2s;
    hrh=( rhs* er- zphs* et)/ r2s;
    erv= erv- hrv;
    ezv= ezv- hzv;
    erh= erh+ hrh;
    ezh= ezh+ hrv;
    eph= eph+ et;
    erv= erv* ctx->dataj.src_dir_cos_z;
    ezv= ezv* ctx->dataj.src_dir_cos_z;
    erh= erh* ctx->incom.sin_alpha* cph;
    ezh= ezh* ctx->incom.sin_alpha* cph;
    eph= eph* ctx->incom.sin_alpha* sph;
    erh= erv+ erh;
    e[0]=( erh* rhx+ eph* phx)* ctx->dataj.seg_half_len;
    e[1]=( erh* rhy+ eph* phy)* ctx->dataj.seg_half_len;
    e[2]=( ezv+ ezh)* ctx->dataj.seg_half_len;
    e[3]=0.;
    e[4]=0.;
    e[5]=0.;
    sfac= PI* ctx->dataj.seg_half_len;
    sfac= sin( sfac)/ sfac;
    e[6]= e[0]* sfac;
    e[7]= e[1]* sfac;
    e[8]= e[2]* sfac;
    
    return;
  } /* if( smat.isnor != 1) */
  
  /* interpolate in sommerfeld field tables */
  if( rho >= 1.0e-12)
    thet= atan( ctx->gwav.z_img2/ rho);
  else
    thet= POT;
  
  /* combine vertical and horizontal components and convert */
  /* to x,y,z components. multiply by exp(-jkr)/r. */
  intrp(ctx, ctx->gwav.range2, thet, &erv, &ezv, &erh, &eph );
  ctx->gwav.cur_phase2= ctx->gwav.cur_phase2/ ctx->gwav.range2;
  sfac= ctx->incom.sin_alpha* cph;
  erh= ctx->gwav.cur_phase2*( ctx->dataj.src_dir_cos_z* erv+ sfac* erh);
  ezh= ctx->gwav.cur_phase2*( ctx->dataj.src_dir_cos_z* ezv- sfac* erv);
  /* x,y,z fields for constant current */
  eph= ctx->incom.sin_alpha* sph* ctx->gwav.cur_phase2* eph;
  e[0]= erh* rhx+ eph* phx;
  e[1]= erh* rhy+ eph* phy;
  e[2]= ezh;
  /* x,y,z fields for sine current */
  rk= TP* t;
  sfac= sin( rk);
  e[3]= e[0]* sfac;
  e[4]= e[1]* sfac;
  /* x,y,z fields for cosine current */
  e[5]= e[2]* sfac;
  sfac= cos( rk);
  e[6]= e[0]* sfac;
  e[7]= e[1]* sfac;
  e[8]= e[2]* sfac;
  
  return;
}

/*-----------------------------------------------------------------------*/

