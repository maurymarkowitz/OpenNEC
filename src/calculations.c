/*******************************************************************
 * calculations.c
 *
 * calculations.c contains the main calculation routines, which
 * handle both pure math like min() and cang(), as well as more
 * NEC-related calculations like zint() and load(). likely some
 * room for moving things around.
 *
 *******************************************************************/

#include "internals.h"
#include "calculations.h"
#include "geometry.h"

/* Forward declarations for internal functions */
static void gf(nec_context_t *ctx, double zk, double *co, double *si);
static int sbf(nec_context_t *ctx, int i, int is, double *aa, double *bb, double *cc);

/* Helper function to add loading output entries */
static void add_loading_output(nec_context_t *ctx, int tag, int tagf, int tagt, double conductivity, const char *type)
{
    if (ctx->loading_outputs.count >= ctx->loading_outputs.capacity) {
        ctx->loading_outputs.capacity = ctx->loading_outputs.capacity == 0 ? 16 : ctx->loading_outputs.capacity * 2;
        mem_realloc(ctx, (void **)&ctx->loading_outputs.entries, 
                   ctx->loading_outputs.capacity * sizeof(loading_output_t));
    }
    loading_output_t *entry = &ctx->loading_outputs.entries[ctx->loading_outputs.count++];
    entry->tag = tag;
    entry->tagf = tagf;
    entry->tagt = tagt;
    entry->conductivity = conductivity;
    strncpy(entry->type, type, sizeof(entry->type) - 1);
    entry->type[sizeof(entry->type) - 1] = '\0';
}

/*-----------------------------------------------------------------------*/

/* cabc computes coefficients of the constant (a), sine (b), and */
/* cosine (c) terms in the current interpolation functions for the */
/* current vector cur. */
void cabc(nec_context_t *restrict ctx, complex double *restrict curx)
{
  int i, is, j, jx, jco1, jco2;
  double ar, ai, sh;
  complex double  curd, cs1, cs2;
  
  if( ctx->geometry.n != 0)
  {
    for( i = 0; i < ctx->geometry.n; i++ )
    {
      ctx->crnt.air[i]=0.;
      ctx->crnt.aii[i]=0.;
      ctx->crnt.bir[i]=0.;
      ctx->crnt.bii[i]=0.;
      ctx->crnt.cir[i]=0.;
      ctx->crnt.cii[i]=0.;
    }
    
    for( i = 0; i < ctx->geometry.n; i++ )
    {
      ar= creal( curx[i]);
      ai= cimag( curx[i]);
      if (tbf(ctx,  i+1, 1) != 0)
        return;
      
      for( jx = 0; jx < ctx->segj.jsno; jx++ )
      {
        j= ctx->segj.jco[jx]-1;
        ctx->crnt.air[j] += ctx->segj.ax[jx]* ar;
        ctx->crnt.aii[j] += ctx->segj.ax[jx]* ai;
        ctx->crnt.bir[j] += ctx->segj.bx[jx]* ar;
        ctx->crnt.bii[j] += ctx->segj.bx[jx]* ai;
        ctx->crnt.cir[j] += ctx->segj.cx[jx]* ar;
        ctx->crnt.cii[j] += ctx->segj.cx[jx]* ai;
      }
      
    } /* for( i = 0; i < n; i++ ) */
    
    if( ctx->vsorc.nqds != 0)
    {
      for( is = 0; is < ctx->vsorc.nqds; is++ )
      {
        i= ctx->vsorc.iqds[is]-1;
        jx= ctx->geometry.icon1[i];
        ctx->geometry.icon1[i]=0;
        if (tbf(ctx, i+1,0) != 0)
          return;
        ctx->geometry.icon1[i]= jx;
        sh= ctx->geometry.si[i]*.5;
        curd= CCJ* ctx->vsorc.vqds[is]/( (log(2.* sh/ ctx->geometry.bi[i])-1.)*
                                   (ctx->segj.bx[ctx->segj.jsno-1]* cos(TP* sh)+ ctx->segj.cx[ctx->segj.jsno-1]*
                                    sin(TP* sh))* ctx->geometry.wlam );
        ar= creal( curd);
        ai= cimag( curd);
        
        for( jx = 0; jx < ctx->segj.jsno; jx++ )
        {
          j= ctx->segj.jco[jx]-1;
          ctx->crnt.air[j]= ctx->crnt.air[j]+ ctx->segj.ax[jx]* ar;
          ctx->crnt.aii[j]= ctx->crnt.aii[j]+ ctx->segj.ax[jx]* ai;
          ctx->crnt.bir[j]= ctx->crnt.bir[j]+ ctx->segj.bx[jx]* ar;
          ctx->crnt.bii[j]= ctx->crnt.bii[j]+ ctx->segj.bx[jx]* ai;
          ctx->crnt.cir[j]= ctx->crnt.cir[j]+ ctx->segj.cx[jx]* ar;
          ctx->crnt.cii[j]= ctx->crnt.cii[j]+ ctx->segj.cx[jx]* ai;
        }
        
      } /* for( is = 0; is < ctx->vsorc.nqds; is++ ) */
      
    } /* if( ctx->vsorc.nqds != 0) */
    
    for( i = 0; i < ctx->geometry.n; i++ )
      curx[i]= cmplx( ctx->crnt.air[i]+ctx->crnt.cir[i], ctx->crnt.aii[i]+ctx->crnt.cii[i] );
    
  } /* if( n != 0) */
  
  if( ctx->geometry.m == 0)
    return;
  
  /* convert surface currents from */
  /* t1,t2 components to x,y,z components */
  jco1= ctx->geometry.np2m;
  jco2= jco1+ ctx->geometry.m;
  for( i = 1; i <= ctx->geometry.m; i++ ) {
    jco1 -= 2;
    jco2 -= 3;
    cs1= curx[jco1];
    cs2= curx[jco1+1];
    curx[jco2]  = cs1* ctx->geometry.t1x[ctx->geometry.m-i]+ cs2* ctx->geometry.t2x[ctx->geometry.m-i];
    curx[jco2+1]= cs1* ctx->geometry.t1y[ctx->geometry.m-i]+ cs2* ctx->geometry.t2y[ctx->geometry.m-i];
    curx[jco2+2]= cs1* ctx->geometry.t1z[ctx->geometry.m-i]+ cs2* ctx->geometry.t2z[ctx->geometry.m-i];
  }
  
  return;
}

/*-----------------------------------------------------------------------*/

/* couple computes the maximum coupling between pairs of segments. */
void couple(nec_context_t *ctx, complex double *cur, double wlam )
{
  int j, j1, j2, l1, i, k, itt1, itt2, its1, its2, isg1, isg2, npm1;
  double dbc, c, gmax;
  complex double y11, y12, y22, yl, yin, zl, zin, rho;
  size_t mreq;
  
  if( (ctx->vsorc.nsant != 1) || (ctx->vsorc.nvqd != 0) )
    return;
  
  j= segment_number(ctx,  ctx->yparm.nctag[ctx->yparm.icoup], ctx->yparm.ncseg[ctx->yparm.icoup]);
  if( j != ctx->vsorc.isant[0] )
    return;
  
  zin= ctx->vsorc.vsant[0];
  ctx->yparm.icoup++;
  mreq = (size_t)ctx->yparm.icoup;
  mreq *= sizeof( complex double);
  mem_realloc(ctx,  (void *)&ctx->yparm.y11a, mreq );
  ctx->yparm.y11a[ctx->yparm.icoup-1]= cur[j-1]*wlam/zin;
  
  l1=(ctx->yparm.icoup-1)*(ctx->yparm.ncoup-1);
  for( i = 0; i < ctx->yparm.ncoup; i++ )
  {
    if( (i+1) == ctx->yparm.icoup)
      continue;
    
    l1++;
    mreq = (size_t)l1;
    mreq *= sizeof( complex double);
    mem_realloc(ctx,  (void *)&ctx->yparm.y12a, mreq );
    k= segment_number(ctx,  ctx->yparm.nctag[i], ctx->yparm.ncseg[i]);
    ctx->yparm.y12a[l1-1]= cur[k-1]* wlam/ zin;
  }
  
  if( ctx->yparm.icoup < ctx->yparm.ncoup)
    return;
  
  /* Accumulate coupling rows; write_nec_output() will render them. */
  npm1= ctx->yparm.ncoup-1;

  for( i = 0; i < npm1; i++ )
  {
    itt1= ctx->yparm.nctag[i];
    its1= ctx->yparm.ncseg[i];
    isg1= segment_number(ctx,  itt1, its1);
    l1= i+1;

    for( j = l1; j < ctx->yparm.ncoup; j++ )
    {
      itt2= ctx->yparm.nctag[j];
      its2= ctx->yparm.ncseg[j];
      isg2= segment_number(ctx,  itt2, its2);
      j1= j+ i* npm1-1;
      j2= i+ j* npm1;
      y11= ctx->yparm.y11a[i];
      y22= ctx->yparm.y11a[j];
      y12=.5*( ctx->yparm.y12a[j1]+ ctx->yparm.y12a[j2]);
      yin= y12* y12;
      dbc= cabs( yin);
      c= dbc/(2.* creal( y11)* creal( y22)- creal( yin));

      coupling_row_t row;
      memset(&row, 0, sizeof(row));
      row.tag1 = itt1; row.seg1 = its1; row.segno1 = isg1;
      row.tag2 = itt2; row.seg2 = its2; row.segno2 = isg2;

      if( (c >= 0.0) && (c <= 1.0) )
      {
        if( c >= .01 )
          gmax=(1.- sqrt(1.- c*c))/c;
        else
          gmax=.5*( c+.25* c* c* c);

        rho= gmax* conj( yin)/ dbc;
        yl=((1.- rho)/(1.+ rho)+1.)* creal( y22)- y22;
        zl=1./ yl;
        yin= y11- yin/( y22+ yl);
        zin=1./ yin;
        dbc= db10(ctx,  gmax);

        row.is_error   = false;
        row.coupling_db = dbc;
        row.zl_real    = creal(zl);  row.zl_imag  = cimag(zl);
        row.zin_real   = creal(zin); row.zin_imag = cimag(zin);
      }
      else
      {
        row.is_error = true;
        row.c_value  = c;
      }

      /* grow buffer if needed */
      if (ctx->yparm.num_coupling_rows >= ctx->yparm.coupling_rows_cap) {
        int newcap = ctx->yparm.coupling_rows_cap == 0 ? 8 : ctx->yparm.coupling_rows_cap * 2;
        ctx->yparm.coupling_rows = realloc(ctx->yparm.coupling_rows,
                                           (size_t)newcap * sizeof(coupling_row_t));
        ctx->yparm.coupling_rows_cap = newcap;
      }
      ctx->yparm.coupling_rows[ctx->yparm.num_coupling_rows++] = row;

    } /* for( j = l1; j < ctx->yparm.ncoup; j++ ) */

  } /* for( i = 0; i < npm1; i++ ) */

  return;
}

/*-----------------------------------------------------------------------*/

/* load calculates the impedance of specified */
/* segments for various types of loading */
int load(nec_context_t *ctx, int *ldtyp, int *ldtag, int *ldtagf, int *ldtagt,
          double *zlr, double *zli, double *zlc )
{
  int i, istep, istepx, l1, l2, ldtags, jump, ichk;
  bool iwarn;
  complex double zt=CPLX_00, tpcj;
  size_t mreq;
  
  tpcj = (0.0+I*1.883698955e+9);
  
  /* initialize d array, used for temporary */
  /* storage of loading information. */
  mreq = (size_t)ctx->geometry.npm;
  mreq *= sizeof(complex double);
  mem_realloc(ctx,  (void *)&ctx->zload.zarray, mreq );
  for( i = 0; i < ctx->geometry.n; i++ )
    ctx->zload.zarray[i]=CPLX_00;
  
  iwarn=false;
  istep=0;
  
  /* cycle over loading cards */
  while( true )
  {
    istepx = istep;
    istep++;
    
    if( istep > ctx->zload.nload)
    {
      if( iwarn == true )
        nec_report(ctx, ONEC_SEV_WARNING,
                   "Some segments have been loaded more than once; impedances added.");

      ctx->smat.nop = ctx->geometry.n/ctx->geometry.np;
      if( ctx->smat.nop == 1)
        return 0;
      
      for( i = 0; i < ctx->geometry.np; i++ )
      {
        zt= ctx->zload.zarray[i];
        l1= i;
        
        for( l2 = 1; l2 < ctx->smat.nop; l2++ )
        {
          l1 += ctx->geometry.np;
          ctx->zload.zarray[l1]= zt;
        }
      }
      return 0;
      
    } /* if( istep > ctx->zload.nload) */
    
    if( ldtyp[istepx] > 5 )
    {
      char err_msg[256];
      snprintf(err_msg, sizeof(err_msg), 
              "INTERNAL ERROR: IMPROPER LOAD TYPE %d processed in load().", ldtyp[istepx]);

      add_error(ctx, &ctx->errors, err_msg, FATAL);
      return -1;
    }
    
    /* search segments for proper itags */
    ldtags= ldtag[istepx];
    jump= ldtyp[istepx]+1;
    ichk=0;
    l1= 1;
    l2= ctx->geometry.n;
    
    if( ldtags == 0)
    {
      if( (ldtagf[istepx] != 0) || (ldtagt[istepx] != 0) )
      {
        l1= ldtagf[istepx];
        l2= ldtagt[istepx];
        
      } /* if( (ldtagf[istepx] != 0) || (ldtagt[istepx] != 0) ) */
      
    } /* if( ldtags == 0) */
    
    for(i = l1-1; i < l2; i++) {
      if(ldtags != 0) {
        if( ldtags != ctx->geometry.tag_nums[i])
          continue;
        
        if( ldtagf[istepx] != 0)
        {
          ichk++;
          if( (ichk < ldtagf[istepx]) || (ichk > ldtagt[istepx]) )
            continue;
        }
        else
          ichk=1;
        
      } /* if( ldtags != 0) */
      else
        ichk=1;
      
      /* calculation of lamda*imped. per unit length, */
      /* jump to appropriate section for loading type */
      switch( jump ) {
        case 1:
          zt= zlr[istepx]/ ctx->geometry.si[i]+ tpcj* zli[istepx]/( ctx->geometry.si[i]* ctx->geometry.wlam);
          if( fabs( zlc[istepx]) > 1.0e-20)
            zt += ctx->geometry.wlam/( tpcj* ctx->geometry.si[i]* zlc[istepx]);
          break;
          
        case 2:
          zt= tpcj* ctx->geometry.si[i]* zlc[istepx]/ ctx->geometry.wlam;
          if( fabs( zli[istepx]) > 1.0e-20)
            zt += ctx->geometry.si[i]* ctx->geometry.wlam/( tpcj* zli[istepx]);
          if( fabs( zlr[istepx]) > 1.0e-20)
            zt += ctx->geometry.si[i]/ zlr[istepx];
          zt=1./ zt;
          break;
          
        case 3:
          zt= zlr[istepx]* ctx->geometry.wlam+ tpcj* zli[istepx];
          if( fabs( zlc[istepx]) > 1.0e-20)
            zt += 1./( tpcj* ctx->geometry.si[i]* ctx->geometry.si[i]* zlc[istepx]);
          break;
          
        case 4:
          zt= tpcj* ctx->geometry.si[i]* ctx->geometry.si[i]* zlc[istepx];
          if( fabs( zli[istepx]) > 1.0e-20)
            zt += 1./( tpcj* zli[istepx]);
          if( fabs( zlr[istepx]) > 1.0e-20)
            zt += 1./( zlr[istepx]* ctx->geometry.wlam);
          zt=1./ zt;
          break;
          
        case 5:
          zt= cmplx( zlr[istepx], zli[istepx])/ ctx->geometry.si[i];
          break;
          
        case 6:
          zint( ctx, zlr[istepx]* ctx->geometry.wlam, ctx->geometry.bi[i], &zt );
          
      } /* switch( jump ) */
      
      if(( fabs( creal( ctx->zload.zarray[i]))+ fabs( cimag( ctx->zload.zarray[i]))) > 1.0e-20)
        iwarn=true;
      ctx->zload.zarray[i] += zt;
      
    } /* for( i = l1-1; i < l2; i++ ) */
    
    if( ichk == 0 )
    {
      char err_msg[256];
      snprintf(err_msg, sizeof(err_msg),
              "LOADING DATA CARD ERROR, NO SEGMENT HAS AN ITAG = %d", ldtags);
      add_error(ctx, &ctx->errors, err_msg, FATAL);
      return -1;
    }
    
    /* Store the segment loading data for output */
    switch( jump )
    {
      case 1:
           add_loading_output(ctx, ldtags, ldtagf[istepx], ldtagt[istepx], 0.0, "SERIES");
        break;
        
      case 2:
           add_loading_output(ctx, ldtags, ldtagf[istepx], ldtagt[istepx], 0.0, "PARALLEL");
        break;
        
      case 3:
           add_loading_output(ctx, ldtags, ldtagf[istepx], ldtagt[istepx], 0.0, "SERIES (PER METER)");
        break;
        
      case 4:
           add_loading_output(ctx, ldtags, ldtagf[istepx], ldtagt[istepx], 0.0, "PARALLEL (PER METER)");
        break;
        
      case 5:
           add_loading_output(ctx, ldtags, ldtagf[istepx], ldtagt[istepx], 0.0, "FIXED IMPEDANCE");
        break;
        
      case 6:
           add_loading_output(ctx, ldtags, ldtagf[istepx], ldtagt[istepx], zlr[istepx], "WIRE");
        
    } /* switch( jump ) */
  } /* while( true ) */
} /* while( true ) */

/*-----------------------------------------------------------------------*/

/* gf computes the integrand exp(jkr)/(kr) for numerical integration. */
void gf(nec_context_t *ctx, double zk, double *co, double *si )
{
  double zdk, rk, rks;
  
  zdk= zk- ctx->tmi.zpk;
  rk= sqrt( ctx->tmi.rkb2+ zdk* zdk);
  *si= sin( rk)/ rk;
  
  if( ctx->tmi.ij != 0 )
  {
    *co= cos( rk)/ rk;
    return;
  }
  
  if( rk >= .2)
  {
    *co=( cos( rk)-1.)/ rk;
    return;
  }
  
  rks= rk* rk;
  *co=((-1.38888889e-3* rks+4.16666667e-2)* rks-.5)* rk;
  
  return;
}

/*-----------------------------------------------------------------------*/

/* function db10 returns db for magnitude (field) */
double db10(const nec_context_t *ctx, double x )
{
  if( x < 1.e-20 )
    return( -999.99 );
  
  return( 10. * log10(x) );
}

/*-----------------------------------------------------------------------*/

/* function db20 returns db for mag**2 (power) i */
double db20(const nec_context_t *ctx, double x )
{
  if( x < 1.e-20 )
    return( -999.99 );
  
  return( 20. * log10(x) );
}

/*-----------------------------------------------------------------------*/

/* intrp uses bivariate cubic interpolation to obtain */
/* the values of 4 functions at the point (x,y). */
void intrp(nec_context_t *restrict ctx, double x, double y, complex double *restrict f1,
           complex double *restrict f2, complex double *restrict f3, complex double *restrict f4 )
{
  int nda[3]={11,17,9}, ndpa[3]={110,85,72};
  bool jump;
  double xx, yy;
  complex double p1=CPLX_00, p2=CPLX_00, p3=CPLX_00, p4=CPLX_00;
  complex double fx1, fx2, fx3, fx4;
  
  jump = false;
  if( (x < ctx->intrp.xs) || (y < ctx->intrp.ys) )
    jump = true;
  else
  {
    ctx->intrp.ix= (int)(( x- ctx->intrp.xs)/ ctx->intrp.dx)+1;
    ctx->intrp.iy= (int)(( y- ctx->intrp.ys)/ ctx->intrp.dy)+1;
  }
  
  /* if point lies in same 4 by 4 point region */
  /* as previous point, old values are reused. */
  if( (ctx->intrp.ix < ctx->intrp.ixeg) ||
     (ctx->intrp.iy < ctx->intrp.iyeg) ||
     (abs(ctx->intrp.ix- ctx->intrp.ixs) >= 2) ||
     (abs(ctx->intrp.iy- ctx->intrp.iys) >= 2) ||
     jump )
  {
    int igr, iadd, iadz, i, k;
    /* determine correct grid and grid region */
    if( x <= ctx->ggrid.xsa[1])
      igr=0;
    else
    {
      if( y > ctx->ggrid.ysa[2])
        igr=2;
      else
        igr=1;
    }
    
    if( igr != ctx->intrp.igrs)
    {
      ctx->intrp.igrs= igr;
      ctx->intrp.dx= ctx->ggrid.dxa[ctx->intrp.igrs];
      ctx->intrp.dy= ctx->ggrid.dya[ctx->intrp.igrs];
      ctx->intrp.xs= ctx->ggrid.xsa[ctx->intrp.igrs];
      ctx->intrp.ys= ctx->ggrid.ysa[ctx->intrp.igrs];
      ctx->intrp.nxm2= ctx->ggrid.nxa[ctx->intrp.igrs]-2;
      ctx->intrp.nym2= ctx->ggrid.nya[ctx->intrp.igrs]-2;
      ctx->intrp.nxms=(( ctx->intrp.nxm2+1)/3)*3+1;
      ctx->intrp.nyms=(( ctx->intrp.nym2+1)/3)*3+1;
      ctx->intrp.nd= nda[ctx->intrp.igrs];
      ctx->intrp.ndp= ndpa[ctx->intrp.igrs];
      ctx->intrp.ix= (int)(( x- ctx->intrp.xs)/ ctx->intrp.dx)+1;
      ctx->intrp.iy= (int)(( y- ctx->intrp.ys)/ ctx->intrp.dy)+1;
      
    } /* if( igr != ctx->intrp.igrs) */
    
    ctx->intrp.ixs=(( ctx->intrp.ix-1)/3)*3+2;
    if( ctx->intrp.ixs < 2)
      ctx->intrp.ixs=2;
    ctx->intrp.ixeg=-10000;
    
    if( ctx->intrp.ixs > ctx->intrp.nxm2)
    {
      ctx->intrp.ixs= ctx->intrp.nxm2;
      ctx->intrp.ixeg= ctx->intrp.nxms;
    }
    
    ctx->intrp.iys=(( ctx->intrp.iy-1)/3)*3+2;
    if( ctx->intrp.iys < 2)
      ctx->intrp.iys=2;
    ctx->intrp.iyeg=-10000;
    
    if( ctx->intrp.iys > ctx->intrp.nym2)
    {
      ctx->intrp.iys= ctx->intrp.nym2;
      ctx->intrp.iyeg= ctx->intrp.nyms;
    }
    
    /* compute coefficients of 4 cubic polynomials in x for */
    /* the 4 grid values of y for each of the 4 functions */
    iadz= ctx->intrp.ixs+( ctx->intrp.iys-3)* ctx->intrp.nd- ctx->intrp.ndp;
    for( k = 0; k < 4; k++ )
    {
      iadz += ctx->intrp.ndp;
      iadd = iadz;
      
      for( i = 0; i < 4; i++ )
      {
        iadd += ctx->intrp.nd;
        
        switch( ctx->intrp.igrs )
        {
          case 0:
            p1= ctx->ggrid.ar1[iadd-2];
            p2= ctx->ggrid.ar1[iadd-1];
            p3= ctx->ggrid.ar1[iadd];
            p4= ctx->ggrid.ar1[iadd+1];
            break;
            
          case 1:
            p1= ctx->ggrid.ar2[iadd-2];
            p2= ctx->ggrid.ar2[iadd-1];
            p3= ctx->ggrid.ar2[iadd];
            p4= ctx->ggrid.ar2[iadd+1];
            break;
            
          case 2:
            p1= ctx->ggrid.ar3[iadd-2];
            p2= ctx->ggrid.ar3[iadd-1];
            p3= ctx->ggrid.ar3[iadd];
            p4= ctx->ggrid.ar3[iadd+1];
            
        } /* switch( ctx->intrp.igrs ) */
        
        ctx->intrp.a[i][k]=( p4- p1+3.*( p2- p3))*.1666666667;
        ctx->intrp.b[i][k]=( p1-2.* p2+ p3)*.5;
        ctx->intrp.c[i][k]= p3-(2.* p1+3.* p2+ p4)*.1666666667;
        ctx->intrp.d[i][k]= p2;
        
      } /* for( i = 0; i < 4; i++ ) */
      
    } /* for( k = 0; k < 4; k++ ) */
    
    ctx->intrp.xz=( ctx->intrp.ixs-1)* ctx->intrp.dx+ ctx->intrp.xs;
    ctx->intrp.yz=( ctx->intrp.iys-1)* ctx->intrp.dy+ ctx->intrp.ys;
    
  } /* if( (abs(ctx->intrp.ix- ctx->intrp.ixs) >= 2) || */
  
  /* evaluate polymomials in x and use cubic */
  /* interpolation in y for each of the 4 functions. */
  xx=( x- ctx->intrp.xz)/ ctx->intrp.dx;
  yy=( y- ctx->intrp.yz)/ ctx->intrp.dy;
  fx1=(( ctx->intrp.a[0][0]* xx+ ctx->intrp.b[0][0])* xx+ ctx->intrp.c[0][0])* xx+ ctx->intrp.d[0][0];
  fx2=(( ctx->intrp.a[1][0]* xx+ ctx->intrp.b[1][0])* xx+ ctx->intrp.c[1][0])* xx+ ctx->intrp.d[1][0];
  fx3=(( ctx->intrp.a[2][0]* xx+ ctx->intrp.b[2][0])* xx+ ctx->intrp.c[2][0])* xx+ ctx->intrp.d[2][0];
  fx4=(( ctx->intrp.a[3][0]* xx+ ctx->intrp.b[3][0])* xx+ ctx->intrp.c[3][0])* xx+ ctx->intrp.d[3][0];
  p1= fx4- fx1+3.*( fx2- fx3);
  p2=3.*( fx1-2.* fx2+ fx3);
  p3=6.* fx3-2.* fx1-3.* fx2- fx4;
  *f1=(( p1* yy+ p2)* yy+ p3)* yy*.1666666667+ fx2;
  fx1=(( ctx->intrp.a[0][1]* xx+ ctx->intrp.b[0][1])* xx+ ctx->intrp.c[0][1])* xx+ ctx->intrp.d[0][1];
  fx2=(( ctx->intrp.a[1][1]* xx+ ctx->intrp.b[1][1])* xx+ ctx->intrp.c[1][1])* xx+ ctx->intrp.d[1][1];
  fx3=(( ctx->intrp.a[2][1]* xx+ ctx->intrp.b[2][1])* xx+ ctx->intrp.c[2][1])* xx+ ctx->intrp.d[2][1];
  fx4=(( ctx->intrp.a[3][1]* xx+ ctx->intrp.b[3][1])* xx+ ctx->intrp.c[3][1])* xx+ ctx->intrp.d[3][1];
  p1= fx4- fx1+3.*( fx2- fx3);
  p2=3.*( fx1-2.* fx2+ fx3);
  p3=6.* fx3-2.* fx1-3.* fx2- fx4;
  *f2=(( p1* yy+ p2)* yy+ p3)* yy*.1666666667+ fx2;
  fx1=(( ctx->intrp.a[0][2]* xx+ ctx->intrp.b[0][2])* xx+ ctx->intrp.c[0][2])* xx+ ctx->intrp.d[0][2];
  fx2=(( ctx->intrp.a[1][2]* xx+ ctx->intrp.b[1][2])* xx+ ctx->intrp.c[1][2])* xx+ ctx->intrp.d[1][2];
  fx3=(( ctx->intrp.a[2][2]* xx+ ctx->intrp.b[2][2])* xx+ ctx->intrp.c[2][2])* xx+ ctx->intrp.d[2][2];
  fx4=(( ctx->intrp.a[3][2]* xx+ ctx->intrp.b[3][2])* xx+ ctx->intrp.c[3][2])* xx+ ctx->intrp.d[3][2];
  p1= fx4- fx1+3.*( fx2- fx3);
  p2=3.*( fx1-2.* fx2+ fx3);
  p3=6.* fx3-2.* fx1-3.* fx2- fx4;
  *f3=(( p1* yy+ p2)* yy+ p3)* yy*.1666666667+ fx2;
  fx1=(( ctx->intrp.a[0][3]* xx+ ctx->intrp.b[0][3])* xx+ ctx->intrp.c[0][3])* xx+ ctx->intrp.d[0][3];
  fx2=(( ctx->intrp.a[1][3]* xx+ ctx->intrp.b[1][3])* xx+ ctx->intrp.c[1][3])* xx+ ctx->intrp.d[1][3];
  fx3=(( ctx->intrp.a[2][3]* xx+ ctx->intrp.b[2][3])* xx+ ctx->intrp.c[2][3])* xx+ ctx->intrp.d[2][3];
  fx4=(( ctx->intrp.a[3][3]* xx+ ctx->intrp.b[3][3])* xx+ ctx->intrp.c[3][3])* xx+ ctx->intrp.d[3][3];
  p1= fx4- fx1+3.*( fx2- fx3);
  p2=3.*( fx1-2.* fx2+ fx3);
  p3=6.* fx3-2.* fx1-3.* fx2- fx4;
  *f4=(( p1* yy+ p2)* yy+ p3)* yy*.16666666670+ fx2;
  
  return;
}

/*-----------------------------------------------------------------------*/

/* intx performs numerical integration of exp(jkr)/r by the method of */
/* variable interval width romberg integration.  the integrand value */
/* is supplied by subroutine gf. */
void intx(nec_context_t *ctx, double el1, double el2, double b,
          int ij, double *sgr, double *sgi)
{
  int ns, nt;
  int nx = 1, nma = 65536, nts = 4;
  bool flag = true;
  double z, s, ze, fnm, ep, zend, fns, dz=0., zp, dzot=0., t00r, g1r, g5r=0.0, t00i;
  double g1i, g5i=0.0, t01r, g3r=0.0, t01i, g3i=0.0, t10r, t10i, te1i, te1r, t02r;
  double g2r, g4r, t02i, g2i, g4i, t11r, t11i, t20r, t20i, te2i, te2r;
  double rx = 1.0e-4;
  
  z= el1;
  ze= el2;
  if( ij == 0)
    ze=0.;
  s= ze- z;
  fnm= nma;
  ep= s/(10.* fnm);
  zend= ze- ep;
  *sgr=0.;
  *sgi=0.;
  ns= nx;
  nt=0;
  gf(ctx,  z, &g1r, &g1i);

  /* Safety cap: the Romberg loop halves dz up to nma times then advances z;
   * worst-case iterations = nma (halvings) * nma (steps) — if we exceed that,
   * something has gone NaN and we would loop forever. */
  int intx_iters = 0;
  const int intx_max = 65536 * 64;

  while( true )
  {
    if( flag )
    {
      fns= ns;
      dz= s/ fns;
      zp= z+ dz;
      
      if( zp > ze)
      {
        dz= ze- z;
        if( fabs(dz) <= ep)
        {
          /* add contribution of near singularity for diagonal term */
          if(ij == 0)
          {
            *sgr=2.*( *sgr+ log(( sqrt( b* b+ s* s)+ s)/ b));
            *sgi=2.* *sgi;
          }
          return;
        }
        
      } /* if( zp > ze) */
      
      dzot= dz*.5;
      zp= z+ dzot;
      gf(ctx,  zp, &g3r, &g3i);
      zp= z+ dz;
      gf(ctx,  zp, &g5r, &g5i);
      
    } /* if( flag ) */
    
    t00r=( g1r+ g5r)* dzot;
    t00i=( g1i+ g5i)* dzot;
    t01r=( t00r+ dz* g3r)*0.5;
    t01i=( t00i+ dz* g3i)*0.5;
    t10r=(4.0* t01r- t00r)/3.0;
    t10i=(4.0* t01i- t00i)/3.0;
    
    /* test convergence of 3 point romberg result. */
    test(ctx,  t01r, t10r, &te1r, t01i, t10i, &te1i, 0.);
    if( (te1i <= rx) && (te1r <= rx) )
    {
      *sgr= *sgr+ t10r;
      *sgi= *sgi+ t10i;
      nt += 2;
      
      z += dz;
      if( z >= zend)
      {
        /* add contribution of near singularity for diagonal term */
        if(ij == 0)
        {
          *sgr=2.*( *sgr+ log(( sqrt( b* b+ s* s)+ s)/ b));
          *sgi=2.* *sgi;
        }
        return;
      }
      
      g1r= g5r;
      g1i= g5i;
      if( nt >= nts)
        if( ns > nx)
        {
          /* Double step size */
          ns= ns/2;
          nt=1;
        }
      flag = true;
      continue;
      
    } /* if( (te1i <= rx) && (te1r <= rx) ) */
    
    zp= z+ dz*0.25;
    gf(ctx,  zp, &g2r, &g2i);
    zp= z+ dz*0.75;
    gf(ctx,  zp, &g4r, &g4i);
    t02r=( t01r+ dzot*( g2r+ g4r))*0.5;
    t02i=( t01i+ dzot*( g2i+ g4i))*0.5;
    t11r=(4.0* t02r- t01r)/3.0;
    t11i=(4.0* t02i- t01i)/3.0;
    t20r=(16.0* t11r- t10r)/15.0;
    t20i=(16.0* t11i- t10i)/15.0;
    
    /* test convergence of 5 point romberg result. */
    test(ctx,  t11r, t20r, &te2r, t11i, t20i, &te2i, 0.);
    if( (te2i > rx) || (te2r > rx) )
    {
      nt=0;
      if( ns >= nma)
        nec_report(ctx, ONEC_SEV_WARNING, "Step size limited at Z= %10.5f", z);
      else
      {
        /* halve step size */
        ns= ns*2;
        fns= ns;
        dz= s/ fns;
        dzot= dz*0.5;
        g5r= g3r;
        g5i= g3i;
        g3r= g2r;
        g3i= g2i;
        
        flag = false;
        continue;
      }
      
    } /* if( (te2i > rx) || (te2r > rx) ) */
    
    *sgr= *sgr+ t20r;
    *sgi= *sgi+ t20i;
    nt++;
    
    z += dz;
    if( !isfinite(z) || ++intx_iters > intx_max )
    {
      nec_report(ctx, ONEC_SEV_WARNING,
        "intx: integration did not converge (degenerate geometry?); results may be inaccurate");
      return;
    }
    if( z >= zend)
    {
      /* add contribution of near singularity for diagonal term */
      if(ij == 0)
      {
        *sgr=2.*( *sgr+ log(( sqrt( b* b+ s* s)+ s)/ b));
        *sgi=2.* *sgi;
      }
      return;
    }
    
    g1r= g5r;
    g1i= g5i;
    if( nt >= nts)
      if( ns > nx)
      {
        /* Double step size */
        ns= ns/2;
        nt=1;
      }
    flag = true;
    
  } /* while( true ) */
  
}

/*-----------------------------------------------------------------------*/

/* returns smallest of two arguments */
int min(const nec_context_t *ctx, int a, int b )
{
  if( a < b )
    return(a);
  else
    return(b);
}

/*-----------------------------------------------------------------------*/

/* test for convergence in numerical integration */
void test(nec_context_t *ctx, double f1r, double f2r, double *tr,
          double f1i, double f2i, double *ti, double dmin )
{
  double den;
  
  den= fabs( f2r);
  *tr= fabs( f2i);
  
  if( den < *tr)
    den= *tr;
  if( den < dmin)
    den= dmin;
  
  if( den < 1.0e-37)
  {
    *tr=0.;
    *ti=0.;
    return;
  }
  
  *tr= fabs(( f1r- f2r)/ den);
  *ti= fabs(( f1i- f2i)/ den);
  
  return;
}

/*-----------------------------------------------------------------------*/

/* compute component of basis function i on segment is. */
int sbf(nec_context_t *ctx, int i, int is, double *aa, double *bb, double *cc )
{
  int ix, jsno, june, jcox, jcoxx, jend, iend, njun1=0, njun2;
  double d, sig, pp, sdh, cdh, sd, omc, aj, pm=0, cd, ap, qp, qm, xxi;
  
  *aa=0.;
  *bb=0.;
  *cc=0.;
  june=0;
  jsno=0;
  pp=0.;
  ix=i-1;
  
  jcox= ctx->geometry.icon1[ix];
  if( jcox > PCHCON) jcox= i;
  
  jend=-1;
  iend=-1;
  sig=-1.;
  int sbf_hops = 0;
  
  do
  {
    if( jcox != 0 )
    {
      if(++sbf_hops > ctx->geometry.n) {
        char err_msg[256];
        snprintf(err_msg, sizeof(err_msg),
            "Segment connection cycle detected at segment %d — geometry is degenerate", i);
        add_error(ctx, &ctx->errors, err_msg, FATAL);
        return -1;
      }

      if( jcox < 0 )
        jcox= -jcox;
      else
      {
        sig= -sig;
        jend= -jend;
      }
      jcoxx = jcox-1;
      
      jsno++;
      d= PI* ctx->geometry.si[jcoxx];
      sdh= sin( d);
      cdh= cos( d);
      sd=2.* sdh* cdh;
      
      if( d <= 0.015)
      {
        omc=4.* d* d;
        omc=((1.3888889e-3* omc -4.1666666667e-2)* omc +.5)* omc;
      }
      else
        omc=1.- cdh* cdh+ sdh* sdh;
      
      aj=1./( log(1./( PI* ctx->geometry.bi[jcoxx]))-.577215664);
      pp -= omc/ sd* aj;
      
      if( jcox == is)
      {
        *aa= aj/ sd* sig;
        *bb= aj/(2.* cdh);
        *cc= -aj/(2.* sdh)* sig;
        june= iend;
      }
      
      if( jcox != i )
      {
        if( jend != 1)
          jcox= ctx->geometry.icon1[jcoxx];
        else
          jcox= ctx->geometry.icon2[jcoxx];
        
        if( abs(jcox) != i )
        {
          if( jcox == 0 )
          {
            char err_msg[256];
            snprintf(err_msg, sizeof(err_msg),
                    "SBF - SEGMENT CONNECTION ERROR FOR SEGMENT %d", i);
            add_error(ctx, &ctx->errors, err_msg, FATAL);
            return -1;
          }
          else
            continue;
        }
        
      } /* if( jcox != i ) */
      else
        if( jcox == is)
          *bb= -*bb;
      
      if( iend == 1)
        break;
      
    } /* if( jcox != 0 ) */
    
    pm= -pp;
    pp=0.;
    njun1= jsno;
    
    jcox= ctx->geometry.icon2[ix];
    if( jcox > PCHCON) jcox= i;
    
    jend=1;
    iend=1;
    sig=-1.;
    sbf_hops = 0; /* reset for second pass (icon2 chain) */
    
  } /* do */
  while( jcox != 0 );
  
  njun2= jsno- njun1;
  d= PI* ctx->geometry.si[ix];
  sdh= sin( d);
  cdh= cos( d);
  sd=2.* sdh* cdh;
  cd= cdh* cdh- sdh* sdh;
  
  if( d <= 0.015)
  {
    omc=4.* d* d;
    omc=((1.3888889e-3* omc -4.1666666667e-2)* omc +.5)* omc;
  }
  else
    omc=1.- cd;
  
  ap=1./( log(1./( PI* ctx->geometry.bi[ix])) -.577215664);
  aj= ap;
  
  if( njun1 == 0)
  {
    if( njun2 == 0)
    {
      *aa =-1.;
      qp= PI* ctx->geometry.bi[ix];
      xxi= qp* qp;
      xxi= qp*(1.-.5* xxi)/(1.- xxi);
      *cc=1./( cdh- xxi* sdh);
      return 0;
    }
    
    qp= PI* ctx->geometry.bi[ix];
    xxi= qp* qp;
    xxi= qp*(1.-.5* xxi)/(1.- xxi);
    qp=-( omc+ xxi* sd)/( sd*( ap+ xxi* pp)+ cd*( xxi* ap- pp));
    
    if( june == 1)
    {
      *aa= -*aa* qp;
      *bb=  *bb* qp;
      *cc= -*cc* qp;
      if( i != is)
        return 0;
    }
    
    *aa -= 1.;
    d = cd - xxi * sd;
    *bb += (sdh + ap * qp * (cdh - xxi * sdh)) / d;
    *cc += (cdh + ap * qp * (sdh + xxi * cdh)) / d;
    return 0;
    
  } /* if( njun1 == 0) */
  
  if( njun2 == 0)
  {
    qm= PI* ctx->geometry.bi[ix];
    xxi= qm* qm;
    xxi= qm*(1.-.5* xxi)/(1.- xxi);
    qm=( omc+ xxi* sd)/( sd*( aj- xxi* pm)+ cd*( pm+ xxi* aj));
    
    if( june == -1)
    {
      *aa= *aa* qm;
      *bb= *bb* qm;
      *cc= *cc* qm;
      if( i != is)
        return 0;
    }
    
    *aa -= 1.;
    d= cd- xxi* sd;
    *bb += ( aj* qm*( cdh- xxi* sdh)- sdh)/ d;
    *cc += ( cdh- aj* qm*( sdh+ xxi* cdh))/ d;
    return 0;
    
  } /* if( njun2 == 0) */
  
  qp= sd*( pm* pp+ aj* ap)+ cd*( pm* ap- pp* aj);
  qm=( ap* omc- pp* sd)/ qp;
  qp=-( aj* omc+ pm* sd)/ qp;
  
  if( june != 0 )
  {
    if( june < 0 )
    {
      *aa= *aa* qm;
      *bb= *bb* qm;
      *cc= *cc* qm;
    }
    else
    {
      *aa= -*aa* qp;
      *bb= *bb* qp;
      *cc= -*cc* qp;
    }
    
    if( i != is)
      return 0;
    
  } /* if( june != 0 ) */
  
  *aa -= 1.;
  *bb += ( aj* qm+ ap* qp)* sdh/ sd;
  *cc += ( aj* qm- ap* qp)* cdh/ sd;
  
  return 0;
}

/*-----------------------------------------------------------------------*/

/* compute basis function i */
int tbf(nec_context_t *ctx, int i, int icap )
{
  int ix, jcox, jcoxx, jend, iend, njun1=0, njun2, jsnop, jsnox;
  double pp, sdh, cdh, sd, omc, aj, pm=0, cd, ap, qp, qm, xxi;
  double d, sig; /*** also global ***/
  
  ctx->segj.jsno=0;
  pp=0.;
  ix = i-1;
  jcox= ctx->geometry.icon1[ix];
  
  if( jcox > PCHCON) jcox= i;
  
  jend=-1;
  iend=-1;
  sig=-1.;
  
  do {
    if( jcox != 0 ) {
      if( jcox < 0 )
        jcox= -jcox;
      else
      {
        sig= -sig;
        jend= -jend;
      }
      
      jcoxx = jcox-1;
      ctx->segj.jsno++;
      jsnox = ctx->segj.jsno-1;
      ctx->segj.jco[jsnox]= jcox;
      d= PI* ctx->geometry.si[jcoxx];
      sdh= sin( d);
      cdh= cos( d);
      sd=2.* sdh* cdh;
      
      if( d <= 0.015)
      {
        omc=4.* d* d;
        omc=((1.3888889e-3* omc-4.1666666667e-2)* omc+.5)* omc;
      }
      else
        omc=1.- cdh* cdh+ sdh* sdh;
      
      aj=1./( log(1./( PI* ctx->geometry.bi[jcoxx]))-.577215664);
      pp= pp- omc/ sd* aj;
      ctx->segj.ax[jsnox]= aj/ sd* sig;
      ctx->segj.bx[jsnox]= aj/(2.* cdh);
      ctx->segj.cx[jsnox]= -aj/(2.* sdh)* sig;
      
      if( jcox != i)
      {
        if( jend == 1)
          jcox= ctx->geometry.icon2[jcoxx];
        else
          jcox= ctx->geometry.icon1[jcoxx];
        
        if( abs(jcox) != i )
        {
          if( jcox != 0 )
            continue;
          else
          {
            char err_msg[256];
            snprintf(err_msg, sizeof(err_msg),
                    "TBF - SEGMENT CONNECTION ERROR FOR SEGMENT %5d", i);
            add_error(ctx, &ctx->errors, err_msg, FATAL);
            return -1;
          }
        }
        
      } /* if( jcox != i) */
      else
        ctx->segj.bx[jsnox] = -ctx->segj.bx[jsnox];
      
      if( iend == 1)
        break;
      
    } /* if( jcox != 0 ) */
    
    pm= -pp;
    pp=0.;
    njun1= ctx->segj.jsno;
    
    jcox= ctx->geometry.icon2[ix];
    if( jcox > PCHCON) jcox= i;
    
    jend=1;
    iend=1;
    sig=-1.;
    
  } /* do */
  while( jcox != 0 );
  
  njun2= ctx->segj.jsno- njun1;
  jsnop= ctx->segj.jsno;
  ctx->segj.jco[jsnop]= i;
  d= PI* ctx->geometry.si[ix];
  sdh= sin( d);
  cdh= cos( d);
  sd=2.* sdh* cdh;
  cd= cdh* cdh- sdh* sdh;
  
  if( d <= 0.015)
  {
    omc=4.* d* d;
    omc=((1.3888889e-3* omc-4.1666666667e-2)* omc+.5)* omc;
  }
  else
    omc=1.- cd;
  
  ap=1./( log(1./( PI* ctx->geometry.bi[ix]))-.577215664);
  aj= ap;
  
  if( njun1 == 0)
  {
    if( njun2 == 0)
    {
      ctx->segj.bx[jsnop]=0.;
      
      if( icap == 0)
        xxi=0.;
      else
      {
        qp= PI* ctx->geometry.bi[ix];
        xxi= qp* qp;
        xxi= qp*(1.-.5* xxi)/(1.- xxi);
      }
      
      ctx->segj.cx[jsnop]=1./( cdh- xxi* sdh);
      ctx->segj.jsno= jsnop+1;
      ctx->segj.ax[jsnop]=-1.;
      return 0;
      
    } /* if( njun2 == 0) */
    
    if( icap == 0)
      xxi=0.;
    else
    {
      qp= PI* ctx->geometry.bi[ix];
      xxi= qp* qp;
      xxi= qp*(1.-.5* xxi)/(1.- xxi);
    }
    
    qp=-( omc+ xxi* sd)/( sd*( ap+ xxi* pp)+ cd*( xxi* ap- pp));
    d= cd- xxi* sd;
    ctx->segj.bx[jsnop]=( sdh+ ap* qp*( cdh- xxi* sdh))/ d;
    ctx->segj.cx[jsnop]=( cdh+ ap* qp*( sdh+ xxi* cdh))/ d;
    
    for( iend = 0; iend < njun2; iend++ )
    {
      ctx->segj.ax[iend]= -ctx->segj.ax[iend]* qp;
      ctx->segj.bx[iend]= ctx->segj.bx[iend]* qp;
      ctx->segj.cx[iend]= -ctx->segj.cx[iend]* qp;
    }
    
    ctx->segj.jsno= jsnop+1;
    ctx->segj.ax[jsnop]=-1.;
    return 0;
    
  } /* if( njun1 == 0) */
  
  if( njun2 == 0)
  {
    if( icap == 0)
      xxi=0.;
    else
    {
      qm= PI* ctx->geometry.bi[ix];
      xxi= qm* qm;
      xxi= qm*(1.-.5* xxi)/(1.- xxi);
    }
    
    qm=( omc+ xxi* sd)/( sd*( aj- xxi* pm)+ cd*( pm+ xxi* aj));
    d= cd- xxi* sd;
    ctx->segj.bx[jsnop]=( aj* qm*( cdh- xxi* sdh)- sdh)/ d;
    ctx->segj.cx[jsnop]=( cdh- aj* qm*( sdh+ xxi* cdh))/ d;
    
    for( iend = 0; iend < njun1; iend++ )
    {
      ctx->segj.ax[iend]= ctx->segj.ax[iend]* qm;
      ctx->segj.bx[iend]= ctx->segj.bx[iend]* qm;
      ctx->segj.cx[iend]= ctx->segj.cx[iend]* qm;
    }
    
    ctx->segj.jsno= jsnop+1;
    ctx->segj.ax[jsnop]=-1.;
    return 0;
    
  } /* if( njun2 == 0) */
  
  qp= sd*( pm* pp+ aj* ap)+ cd*( pm* ap- pp* aj);
  qm=( ap* omc- pp* sd)/ qp;
  qp=-( aj* omc+ pm* sd)/ qp;
  ctx->segj.bx[jsnop]=( aj* qm+ ap* qp)* sdh/ sd;
  ctx->segj.cx[jsnop]=( aj* qm- ap* qp)* cdh/ sd;
  
  for( iend = 0; iend < njun1; iend++ )
  {
    ctx->segj.ax[iend]= ctx->segj.ax[iend]* qm;
    ctx->segj.bx[iend]= ctx->segj.bx[iend]* qm;
    ctx->segj.cx[iend]= ctx->segj.cx[iend]* qm;
  }
  
  jend= njun1;
  for( iend = jend; iend < ctx->segj.jsno; iend++ )
  {
    ctx->segj.ax[iend]= -ctx->segj.ax[iend]* qp;
    ctx->segj.bx[iend]= ctx->segj.bx[iend]* qp;
    ctx->segj.cx[iend]= -ctx->segj.cx[iend]* qp;
  }
  
  ctx->segj.jsno= jsnop+1;
  ctx->segj.ax[jsnop]=-1.;
  return 0;
}

/*-----------------------------------------------------------------------*/

/* compute the components of all basis functions on segment j */
int trio(nec_context_t *ctx, int j )
{
  int jcox, jcoxx, jsnox, jx, jend=0, iend=0;
  
  ctx->segj.jsno=0;
  jx = j-1;
  jcox= ctx->geometry.icon1[jx];
  
  if( jcox <= PCHCON)
  {
    jend=-1;
    iend=-1;
  }
  
  if( (jcox == 0) || (jcox > PCHCON) )
  {
    jcox= ctx->geometry.icon2[jx];
    
    if( jcox <= PCHCON)
    {
      jend=1;
      iend=1;
    }
    
    if( jcox == 0 || (jcox > PCHCON) )
    {
      jsnox = ctx->segj.jsno;
      ctx->segj.jsno++;
      
      /* Allocate to connections buffers */
      if( ctx->segj.jsno >= ctx->segj.maxcon )
      {
        ctx->segj.maxcon = ctx->segj.jsno +1;
        size_t mreq = (size_t)ctx->segj.maxcon;
        mreq *= sizeof(int);
        mem_realloc(ctx,  (void *)&ctx->segj.jco, mreq );
        mreq = (size_t)ctx->segj.maxcon;
        mreq *= sizeof(double);
        mem_realloc(ctx,  (void *) &ctx->segj.ax, mreq );
        mem_realloc(ctx,  (void *) &ctx->segj.bx, mreq );
        mem_realloc(ctx,  (void *) &ctx->segj.cx, mreq );
      }
      
      if (sbf(ctx,  j, j, &ctx->segj.ax[jsnox], &ctx->segj.bx[jsnox], &ctx->segj.cx[jsnox]) != 0)
        return -1;
      ctx->segj.jco[jsnox]= j;
      return 0;
    }
    
  } /* if( (jcox == 0) || (jcox > PCHCON) ) */
  
  do
  {
    if( jcox < 0 )
      jcox= -jcox;
    else
      jend= -jend;
    jcoxx = jcox-1;
    
    if( jcox != j)
    {
      jsnox = ctx->segj.jsno;
      ctx->segj.jsno++;
      
      /* Allocate to connections buffers */
      if( ctx->segj.jsno >= ctx->segj.maxcon )
      {
        ctx->segj.maxcon = ctx->segj.jsno +1;
        size_t mreq = (size_t)ctx->segj.maxcon;
        mreq *= sizeof(int);
        mem_realloc(ctx,  (void *)&ctx->segj.jco, mreq );
        mreq = (size_t)ctx->segj.maxcon;
        mreq *= sizeof(double);
        mem_realloc(ctx,  (void *) &ctx->segj.ax, mreq );
        mem_realloc(ctx,  (void *) &ctx->segj.bx, mreq );
        mem_realloc(ctx,  (void *) &ctx->segj.cx, mreq );
      }
      
      if (sbf(ctx,  jcox, j, &ctx->segj.ax[jsnox], &ctx->segj.bx[jsnox], &ctx->segj.cx[jsnox]) != 0)
        return -1;
      ctx->segj.jco[jsnox]= jcox;
      
      if( jend != 1)
        jcox= ctx->geometry.icon1[jcoxx];
      else
        jcox= ctx->geometry.icon2[jcoxx];
      
      if( jcox == 0 )
      {
        char err_msg[256];
        snprintf(err_msg, sizeof(err_msg),
                "TRIO - SEGMENT CONNENTION ERROR FOR SEGMENT %5d", j);
        add_error(ctx, &ctx->errors, err_msg, FATAL);
        return -1;
      }
      else
        continue;
      
    } /* if( jcox != j) */
    
    if( iend == 1)
      break;
    
    jcox= ctx->geometry.icon2[jx];
    
    if( jcox > PCHCON ) break;
    
    jend=1;
    iend=1;
    
  } /* do */
  while( jcox != 0 );
  
  jsnox = ctx->segj.jsno;
  ctx->segj.jsno++;
  
  /* Allocate to connections buffers */
  if( ctx->segj.jsno >= ctx->segj.maxcon )
  {
    ctx->segj.maxcon = ctx->segj.jsno +1;
    size_t mreq = (size_t)ctx->segj.maxcon;
    mreq *= sizeof(int);
    mem_realloc(ctx,  (void *)&ctx->segj.jco, mreq );
    mreq = (size_t)ctx->segj.maxcon;
    mreq *= sizeof(double);
    mem_realloc(ctx,  (void *) &ctx->segj.ax, mreq );
    mem_realloc(ctx,  (void *) &ctx->segj.bx, mreq );
    mem_realloc(ctx,  (void *) &ctx->segj.cx, mreq );
  }
  
  if (sbf(ctx,  j, j, &ctx->segj.ax[jsnox], &ctx->segj.bx[jsnox], &ctx->segj.cx[jsnox]) != 0)
    return -1;
  ctx->segj.jco[jsnox]= j;
  
  return 0;
  
}

/*-----------------------------------------------------------------------*/

/* zint computes the internal impedance of a circular wire */
void zint(nec_context_t *restrict ctx, double sigl, double rolam, complex double *restrict zint )
{
#define cc1		( 6.0e-7     + I*1.9e-6)
#define cc2		(-3.4e-6     + I*5.1e-6)
#define cc3		(-2.52e-5    + I*0.0)
#define cc4		(-9.06e-5    - I*9.01e-5)
#define cc5		( 0.         - I*9.765e4)
#define cc6		(.0110486    - I*0.0110485)
#define cc7		( 0.         - I*0.3926991)
#define cc8		( 1.6e-6     - I*3.2e-6)
#define cc9		( 1.17e-5    - I*2.4e-6)
#define cc10	( 3.46e-5    + I*3.38e-5)
#define cc11	( 5.0e-7     + I*2.452e-4)
#define cc12	(-1.3813e-3  + I*1.3811e-3)
#define cc13	(-6.25001e-2 - I*1.0e-7)
#define cc14	(.7071068    + I*0.7071068)
#define cn	cc14
  
#define th(d) ( (((((cc1*(d)+cc2)*(d)+cc3)*(d)+cc4)*(d)+cc5)*(d)+cc6)*(d) + cc7 )
#define ph(d) ( (((((cc8*(d)+cc9)*(d)+cc10)*(d)+cc11)*(d)+cc12)*(d)+cc13)*(d)+cc14 )
#define f(d)  ( csqrt(POT/(d))*cexp(-cn*(d)+th(-8./x)) )
#define g(d)  ( cexp(cn*(d)+th(8./x))/csqrt(TP*(d)) )
  
  double x, tpcmu = 2.368705e+3, cmotp = 60.00;
  complex double br1, br2;
  
  x= sqrt( tpcmu* sigl)* rolam;
  if( x <= 110.)
  {
    if( x <= 8.)
    {
      double y, s, ber, bei;
      y= x/8.;
      y= y* y;
      s= y* y;
      
      ber=((((((-9.01e-6* s+1.22552e-3)* s-.08349609)* s+ 2.6419140)*
             s-32.363456)* s+113.77778)* s-64.)* s+1.;
      
      bei=((((((1.1346e-4* s-.01103667)* s+.52185615)* s-10.567658)*
             s+72.817777)* s-113.77778)* s+16.)* y;
      
      br1= cmplx( ber, bei);
      
      ber=(((((((-3.94e-6* s+4.5957e-4)* s-.02609253)* s+ .66047849)*
              s-6.0681481)* s+14.222222)* s-4.)* y)* x;
      
      bei=((((((4.609e-5* s-3.79386e-3)* s+.14677204)* s- 2.3116751)*
             s+11.377778)* s-10.666667)* s+.5)* x;
      
      br2= cmplx( ber, bei);
      br1= br1/ br2;
      *zint= CPLX_01* sqrt( cmotp/sigl )* br1/ rolam;
      
    } /* if( x <= 8.) */
    
    br2= I*f(x)/ PI;
    br1= g( x)+ br2;
    br2= g( x)* ph(8./ x)- br2* ph(-8./ x);
    br1= br1/ br2;
    *zint= CPLX_01* sqrt( cmotp/ sigl)* br1/ rolam;
    
  } /* if( x <= 110.) */
  
  br1= cmplx(.70710678,-.70710678);
  *zint= CPLX_01* sqrt( cmotp/ sigl)* br1/ rolam;
}

/*-----------------------------------------------------------------------*/

/* cang returns the phase angle of a complex number in degrees. */
double cang(const nec_context_t *ctx, complex double z )
{
  return( carg(z)*TD );
}

/*-----------------------------------------------------------------------*/
