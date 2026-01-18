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

#include "opennec.h"
#include "shared.h"

/*common  /tmh/ */
static tmh_t tmh;

/*-------------------------------------------------------------------*/

/* compute near e fields of a segment with sine, cosine, and */
/* constant currents.  ground effect included. */
void efld(nec_context_t *ctx, double xi, double yi, double zi, double ai, int ij )
{
#define	txk	egnd[0]
#define	tyk	egnd[1]
#define	tzk	egnd[2]
#define	txs	egnd[3]
#define	tys	egnd[4]
#define	tzs	egnd[5]
#define	txc	egnd[6]
#define	tyc	egnd[7]
#define	tzc	egnd[8]
  
  int ip, ijx;
  double xij, yij, rfl, salpr, zij, zp, rhox;
  double rhoy, rhoz, rh, r, rmag, cth, px, py;
  double xymag, xspec, yspec, rhospc, dmin;
  complex double epx, epy, refs, refps, zrsin, zratx, zscrn;
  complex double tezs, ters, tezc, terc, tezk, terk, egnd[9];
  
  xij= xi- ctx->dataj.xj;
  yij= yi- ctx->dataj.yj;
  ijx= ij;
  rfl=-1.;
  
  for( ip = 0; ip < ctx->gnd.ksymp; ip++ )
  {
    if( ip == 1)
      ijx=1;
    rfl= -rfl;
    salpr= ctx->dataj.salpj* rfl;
    zij= zi- rfl* ctx->dataj.zj;
    zp= xij* ctx->dataj.cabj+ yij* ctx->dataj.sabj+ zij* salpr;
    rhox= xij- ctx->dataj.cabj* zp;
    rhoy= yij- ctx->dataj.sabj* zp;
    rhoz= zij- salpr* zp;
    
    rh= sqrt( rhox* rhox+ rhoy* rhoy+ rhoz* rhoz+ ai* ai);
    if( rh <= 1.e-10)
    {
      rhox=0.;
      rhoy=0.;
      rhoz=0.;
    }
    else
    {
      rhox= rhox/ rh;
      rhoy= rhoy/ rh;
      rhoz= rhoz/ rh;
    }
    
    /* lumped current element approx. for large separations */
    r= sqrt( zp* zp+ rh* rh);
    if( r >= ctx->dataj.rkh)
    {
      rmag= TP* r;
      cth= zp/ r;
      px= rh/ r;
      txk= cmplx( cos( rmag), -sin( rmag));
      py= TP* r* r;
      tyk= ETA* cth* txk* cmplx(1.0,-1.0/ rmag)/ py;
      tzk= ETA* px* txk* cmplx(1.0, rmag-1.0/ rmag)/(2.* py);
      tezk= tyk* cth- tzk* px;
      terk= tyk* px+ tzk* cth;
      rmag= sin( PI* ctx->dataj.s)/ PI;
      tezc= tezk* rmag;
      terc= terk* rmag;
      tezk= tezk* ctx->dataj.s;
      terk= terk* ctx->dataj.s;
      txs=CPLX_00;
      tys=CPLX_00;
      tzs=CPLX_00;
      
    } /* if( r >= ctx->dataj.rkh) */
    else
    {
      /* eksc for thin wire approx. or ekscx for extended t.w. approx. */
      if( ctx->dataj.iexk != 1)
        eksc(ctx, ctx->dataj.s, zp, rh, TP, ijx, &tezs, &ters,
             &tezc, &terc, &tezk, &terk );
      else
        ekscx(ctx, ctx->dataj.b, ctx->dataj.s, zp, rh, TP, ijx, ctx->dataj.ind1, ctx->dataj.ind2,
              &tezs, &ters, &tezc, &terc, &tezk, &terk);
      
      txs= tezs* ctx->dataj.cabj+ ters* rhox;
      tys= tezs* ctx->dataj.sabj+ ters* rhoy;
      tzs= tezs* salpr+ ters* rhoz;
      
    } /* if( r < ctx->dataj.rkh) */
    
    txk= tezk* ctx->dataj.cabj+ terk* rhox;
    tyk= tezk* ctx->dataj.sabj+ terk* rhoy;
    tzk= tezk* salpr+ terk* rhoz;
    txc= tezc* ctx->dataj.cabj+ terc* rhox;
    tyc= tezc* ctx->dataj.sabj+ terc* rhoy;
    tzc= tezc* salpr+ terc* rhoz;
    
    if( ip == 1)
    {
      if( ctx->gnd.iperf <= 0)
      {
        zratx= ctx->gnd.zrati;
        rmag= r;
        xymag= sqrt( xij* xij+ yij* yij);
        
        /* set parameters for radial wire ground screen. */
        if( ctx->gnd.nradl != 0)
        {
          xspec=( xi* ctx->dataj.zj+ zi* ctx->dataj.xj)/( zi+ ctx->dataj.zj);
          yspec=( yi* ctx->dataj.zj+ zi* ctx->dataj.yj)/( zi+ ctx->dataj.zj);
          rhospc= sqrt( xspec* xspec+ yspec* yspec+ ctx->gnd.t2* ctx->gnd.t2);
          
          if( rhospc <= ctx->gnd.scrwl)
          {
            zscrn= ctx->gnd.t1* rhospc* log( rhospc/ ctx->gnd.t2);
            zratx=( zscrn* ctx->gnd.zrati)/( ETA* ctx->gnd.zrati+ zscrn);
          }
        } /* if( ctx->gnd.nradl != 0) */
        
        /* calculation of reflection coefficients when ground is specified. */
        if( xymag <= 1.0e-6)
        {
          px=0.;
          py=0.;
          cth=1.;
          zrsin=CPLX_10;
        }
        else
        {
          px= -yij/ xymag;
          py= xij/ xymag;
          cth= zij/ rmag;
          zrsin= csqrt(1.0 - zratx*zratx*(1.0 - cth*cth) );
          
        } /* if( xymag <= 1.0e-6) */
        
        refs=( cth- zratx* zrsin)/( cth+ zratx* zrsin);
        refps=-( zratx* cth- zrsin)/( zratx* cth+ zrsin);
        refps= refps- refs;
        epy= px* txk+ py* tyk;
        epx= px* epy;
        epy= py* epy;
        txk= refs* txk+ refps* epx;
        tyk= refs* tyk+ refps* epy;
        tzk= refs* tzk;
        epy= px* txs+ py* tys;
        epx= px* epy;
        epy= py* epy;
        txs= refs* txs+ refps* epx;
        tys= refs* tys+ refps* epy;
        tzs= refs* tzs;
        epy= px* txc+ py* tyc;
        epx= px* epy;
        epy= py* epy;
        txc= refs* txc+ refps* epx;
        tyc= refs* tyc+ refps* epy;
        tzc= refs* tzc;
        
      } /* if( ctx->gnd.iperf <= 0) */
      
      ctx->dataj.exk= ctx->dataj.exk- txk* ctx->gnd.frati;
      ctx->dataj.eyk= ctx->dataj.eyk- tyk* ctx->gnd.frati;
      ctx->dataj.ezk= ctx->dataj.ezk- tzk* ctx->gnd.frati;
      ctx->dataj.exs= ctx->dataj.exs- txs* ctx->gnd.frati;
      ctx->dataj.eys= ctx->dataj.eys- tys* ctx->gnd.frati;
      ctx->dataj.ezs= ctx->dataj.ezs- tzs* ctx->gnd.frati;
      ctx->dataj.exc= ctx->dataj.exc- txc* ctx->gnd.frati;
      ctx->dataj.eyc= ctx->dataj.eyc- tyc* ctx->gnd.frati;
      ctx->dataj.ezc= ctx->dataj.ezc- tzc* ctx->gnd.frati;
      continue;
      
    } /* if( ip == 1) */
    
    ctx->dataj.exk= txk;
    ctx->dataj.eyk= tyk;
    ctx->dataj.ezk= tzk;
    ctx->dataj.exs= txs;
    ctx->dataj.eys= tys;
    ctx->dataj.ezs= tzs;
    ctx->dataj.exc= txc;
    ctx->dataj.eyc= tyc;
    ctx->dataj.ezc= tzc;
    
  } /* for( ip = 0; ip < ctx->gnd.ksymp; ip++ ) */
  
  if( ctx->gnd.iperf != 2)
    return;
  
  /* field due to ground using sommerfeld/norton */
  ctx->incom.sn= sqrt( ctx->dataj.cabj* ctx->dataj.cabj+ ctx->dataj.sabj* ctx->dataj.sabj);
  if( ctx->incom.sn >= 1.0e-5)
  {
    ctx->incom.xsn= ctx->dataj.cabj/ ctx->incom.sn;
    ctx->incom.ysn= ctx->dataj.sabj/ ctx->incom.sn;
  }
  else
  {
    ctx->incom.sn=0.;
    ctx->incom.xsn=1.;
    ctx->incom.ysn=0.;
  }
  
  /* displace observation point for thin wire approximation */
  zij= zi+ ctx->dataj.zj;
  salpr= -ctx->dataj.salpj;
  rhox= ctx->dataj.sabj* zij- salpr* yij;
  rhoy= salpr* xij- ctx->dataj.cabj* zij;
  rhoz= ctx->dataj.cabj* yij- ctx->dataj.sabj* xij;
  rh= rhox* rhox+ rhoy* rhoy+ rhoz* rhoz;
  
  if( rh <= 1.e-10)
  {
    ctx->incom.xo= xi- ai* ctx->incom.ysn;
    ctx->incom.yo= yi+ ai* ctx->incom.xsn;
    ctx->incom.zo= zi;
  }
  else
  {
    rh= ai/ sqrt( rh);
    if( rhoz < 0.)
      rh= -rh;
    ctx->incom.xo= xi+ rh* rhox;
    ctx->incom.yo= yi+ rh* rhoy;
    ctx->incom.zo= zi+ rh* rhoz;
    
  } /* if( rh <= 1.e-10) */
  
  r= xij* xij+ yij* yij+ zij* zij;
  if( r <= .95)
  {
    double shaf;
    /* field from interpolation is integrated over segment */
    ctx->incom.isnor=1;
    dmin= creal(ctx->dataj.exk*conj(ctx->dataj.exk)) +
    creal(ctx->dataj.eyk*conj(ctx->dataj.eyk)) +
    creal(ctx->dataj.ezk*conj(ctx->dataj.ezk) );
    dmin=.01* sqrt( dmin);
    shaf=.5* ctx->dataj.s;
    rom2(ctx, - shaf, shaf, egnd, dmin);
  }
  else
  {
    /* norton field equations and lumped current element approximation */
    ctx->incom.isnor=2;
    sflds(ctx, 0., egnd);
  } /* if( r <= .95) */
  
  if( r > .95)
  {
    zp= xij* ctx->dataj.cabj+ yij* ctx->dataj.sabj+ zij* salpr;
    rh= r- zp* zp;
    if( rh <= 1.e-10)
      dmin=0.;
    else
      dmin= sqrt( rh/( rh+ ai* ai));
    
    if( dmin <= .95)
    {
      px=1.- dmin;
      terk=( txk* ctx->dataj.cabj+ tyk* ctx->dataj.sabj+ tzk* salpr)* px;
      txk= dmin* txk+ terk* ctx->dataj.cabj;
      tyk= dmin* tyk+ terk* ctx->dataj.sabj;
      tzk= dmin* tzk+ terk* salpr;
      ters=( txs* ctx->dataj.cabj+ tys* ctx->dataj.sabj+ tzs* salpr)* px;
      txs= dmin* txs+ ters* ctx->dataj.cabj;
      tys= dmin* tys+ ters* ctx->dataj.sabj;
      tzs= dmin* tzs+ ters* salpr;
      terc=( txc* ctx->dataj.cabj+ tyc* ctx->dataj.sabj+ tzc* salpr)* px;
      txc= dmin* txc+ terc* ctx->dataj.cabj;
      tyc= dmin* tyc+ terc* ctx->dataj.sabj;
      tzc= dmin* tzc+ terc* salpr;
      
    } /* if( dmin <= .95) */
    
  } /* if( r > .95) */
  
  ctx->dataj.exk= ctx->dataj.exk+ txk;
  ctx->dataj.eyk= ctx->dataj.eyk+ tyk;
  ctx->dataj.ezk= ctx->dataj.ezk+ tzk;
  ctx->dataj.exs= ctx->dataj.exs+ txs;
  ctx->dataj.eys= ctx->dataj.eys+ tys;
  ctx->dataj.ezs= ctx->dataj.ezs+ tzs;
  ctx->dataj.exc= ctx->dataj.exc+ txc;
  ctx->dataj.eyc= ctx->dataj.eyc+ tyc;
  ctx->dataj.ezc= ctx->dataj.ezc+ tzc;
  
  return;
}

/*-----------------------------------------------------------------------*/

/* compute e field of sine, cosine, and constant */
/* current filaments by thin wire approximation. */
void eksc(nec_context_t *ctx, double s, double z, double rh, double xk, int ij,
          complex double *ezs, complex double *ers, complex double *ezc,
          complex double *erc, complex double *ezk, complex double *erk )
{
  double rhk, sh, shk, ss, cs, z1a, z2a, cint, sint;
  complex double gz1, gz2, gp1, gp2, gzp1, gzp2;
  
  ctx->tmi.ij= ij;
  ctx->tmi.zpk= xk* z;
  rhk= xk* rh;
  ctx->tmi.rkb2= rhk* rhk;
  sh=.5* s;
  shk= xk* sh;
  ss= sin( shk);
  cs= cos( shk);
  z2a= sh- z;
  z1a=-( sh+ z);
  gx(ctx, z1a, rh, xk, &gz1, &gp1);
  gx(ctx, z2a, rh, xk, &gz2, &gp2);
  gzp1= gp1* z1a;
  gzp2= gp2* z2a;
  *ezs=  CONST1*(( gz2- gz1)* cs* xk-( gzp2+ gzp1)* ss);
  *ezc= -CONST1*(( gz2+ gz1)* ss* xk+( gzp2- gzp1)* cs);
  *erk= CONST1*( gp2- gp1)* rh;
  intx(ctx, - shk, shk, rhk, ij, &cint, &sint);
  *ezk= -CONST1*( gzp2- gzp1+ xk* xk* cmplx( cint,- sint));
  gzp1= gzp1* z1a;
  gzp2= gzp2* z2a;
  
  if( rh >= 1.0e-10)
  {
    *ers= -CONST1*(( gzp2+ gzp1+ gz2+ gz1)*
                   ss-( z2a* gz2- z1a* gz1)* cs*xk)/ rh;
    *erc= -CONST1*(( gzp2- gzp1+ gz2- gz1)*
                   cs+( z2a* gz2+ z1a* gz1)* ss*xk)/ rh;
    return;
  }
  
  *ers = CPLX_00;
  *erc = CPLX_00;
  
  return;
}

/*-----------------------------------------------------------------------*/

/* compute e field of sine, cosine, and constant current */
/* filaments by extended thin wire approximation. */
void ekscx(nec_context_t *ctx, double bx, double s, double z,
           double rhx, double xk, int ij, int inx1, int inx2,
           complex double *ezs, complex double *ers, complex double *ezc,
           complex double *erc, complex double *ezk, complex double *erk )
{
  int ira;
  double b, rh, sh, rhk, shk, ss, cs, z1a;
  double z2a, a2, bk, bk2, cint, sint;
  complex double gz1, gz2, gzp1, gzp2, gr1, gr2;
  complex double grp1, grp2, grk1, grk2, gzz1, gzz2;
  
  if( rhx >= bx)
  {
    rh= rhx;
    b= bx;
    ira=0;
  }
  else
  {
    rh= bx;
    b= rhx;
    ira=1;
  }
  
  sh=.5* s;
  ctx->tmi.ij= ij;
  ctx->tmi.zpk= xk* z;
  rhk= xk* rh;
  ctx->tmi.rkb2= rhk* rhk;
  shk= xk* sh;
  ss= sin( shk);
  cs= cos( shk);
  z2a= sh- z;
  z1a=-( sh+ z);
  a2= b* b;
  
  if( inx1 != 2)
    gxx(ctx, z1a, rh, b, a2, xk, ira, &gz1, &gzp1, &gr1, &grp1, &grk1, &gzz1);
  else
  {
    gx(ctx, z1a, rhx, xk, &gz1, &grk1);
    gzp1= grk1* z1a;
    gr1= gz1/ rhx;
    grp1= gzp1/ rhx;
    grk1= grk1* rhx;
    gzz1= CPLX_00;
  }
  
  if( inx2 != 2)
    gxx(ctx, z2a, rh, b, a2, xk, ira, &gz2, &gzp2, &gr2, &grp2, &grk2, &gzz2);
  else
  {
    gx(ctx, z2a, rhx, xk, &gz2, &grk2);
    gzp2= grk2* z2a;
    gr2= gz2/ rhx;
    grp2= gzp2/ rhx;
    grk2= grk2* rhx;
    gzz2= CPLX_00;
  }
  
  *ezs= CONST1*(( gz2- gz1)* cs* xk-( gzp2+ gzp1)* ss);
  *ezc= -CONST1*(( gz2+ gz1)* ss* xk+( gzp2- gzp1)* cs);
  *ers= -CONST1*(( z2a* grp2+ z1a* grp1+ gr2+ gr1)*ss
                 -( z2a* gr2- z1a* gr1)* cs* xk);
  *erc= -CONST1*(( z2a* grp2- z1a* grp1+ gr2- gr1)*cs
                 +( z2a* gr2+ z1a* gr1)* ss* xk);
  *erk= CONST1*( grk2- grk1);
  intx(ctx, - shk, shk, rhk, ij, &cint, &sint);
  bk= b* xk;
  bk2= bk* bk*.25;
  *ezk= -CONST1*( gzp2- gzp1+ xk* xk*(1.- bk2)*
                 cmplx( cint,- sint)-bk2*( gzz2- gzz1));
  
  return;
}

/*-----------------------------------------------------------------------*/

/* integrand for h field of a wire */
void gh(nec_context_t *ctx, double zk, double *hr, double *hi)
{
  double rs, r, ckr, skr, rr2, rr3;
  
  rs= zk- tmh.zpka;
  rs= tmh.rhks+ rs* rs;
  r= sqrt( rs);
  ckr= cos( r);
  skr= sin( r);
  rr2=1./ rs;
  rr3= rr2/ r;
  *hr= skr* rr2+ ckr* rr3;
  *hi= ckr* rr2- skr* rr3;
  
  return;
}

/*-----------------------------------------------------------------------*/

/* gwave computes the electric field, including ground wave, of a */
/* current element over a ground plane using formulas of k.a. norton */
/* (proc. ire, sept., 1937, pp.1203,1236.) */

void gwave(nec_context_t *ctx, complex double *erv, complex double *ezv,
           complex double *erh, complex double *ezh, complex double *eph )
{
  double sppp, sppp2, cppp2, cppp, spp, spp2, cpp2, cpp;
  complex double rk1, rk2, t1, t2, t3, t4, p1, rv;
  complex double omr, w, f, q1, rh, v, g, xr1, xr2;
  complex double x1, x2, x3, x4, x5, x6, x7;
  
  sppp= ctx->gwav.zmh/ ctx->gwav.r1;
  sppp2= sppp* sppp;
  cppp2=1.- sppp2;
  
  if( cppp2 < 1.0e-20)
    cppp2=1.0e-20;
  
  cppp= sqrt( cppp2);
  spp= ctx->gwav.zph/ ctx->gwav.r2;
  spp2= spp* spp;
  cpp2=1.- spp2;
  
  if( cpp2 < 1.0e-20)
    cpp2=1.0e-20;
  
  cpp= sqrt( cpp2);
  rk1= -TPJ* ctx->gwav.r1;
  rk2= -TPJ* ctx->gwav.r2;
  t1=1. -ctx->gwav.u2* cpp2;
  t2= csqrt( t1);
  t3=(1. -1./ rk1)/ rk1;
  t4=(1. -1./ rk2)/ rk2;
  p1= rk2* ctx->gwav.u2* t1/(2.* cpp2);
  rv=( spp- ctx->gwav.u* t2)/( spp+ ctx->gwav.u* t2);
  omr=1.- rv;
  w=1./ omr;
  w=(4.0 + I*0.0)* p1* w* w;
  fbar(ctx,  w, &f );
  q1= rk2* t1/(2.* ctx->gwav.u2* cpp2);
  rh=( t2- ctx->gwav.u* spp)/( t2+ ctx->gwav.u* spp);
  v=1./(1.+ rh);
  v=(4.0 + I*0.0)* q1* v* v;
  fbar(ctx,  v, &g );
  xr1= ctx->gwav.xx1/ ctx->gwav.r1;
  xr2= ctx->gwav.xx2/ ctx->gwav.r2;
  x1= cppp2* xr1;
  x2= rv* cpp2* xr2;
  x3= omr* cpp2* f* xr2;
  x4= ctx->gwav.u* t2* spp*2.* xr2/ rk2;
  x5= xr1* t3*(1.-3.* sppp2);
  x6= xr2* t4*(1.-3.* spp2);
  *ezv=( x1+ x2+ x3- x4- x5- x6)* (-CONST4);
  x1= sppp* cppp* xr1;
  x2= rv* spp* cpp* xr2;
  x3= cpp* omr* ctx->gwav.u* t2* f* xr2;
  x4= spp* cpp* omr* xr2/ rk2;
  x5=3.* sppp* cppp* t3* xr1;
  x6= cpp* ctx->gwav.u* t2* omr* xr2/ rk2*.5;
  x7=3.* spp* cpp* t4* xr2;
  *erv=-( x1+ x2- x3+ x4- x5+ x6- x7)* (-CONST4);
  *ezh=-( x1- x2+ x3- x4- x5- x6+ x7)* (-CONST4);
  x1= sppp2* xr1;
  x2= rv* spp2* xr2;
  x4= ctx->gwav.u2* t1* omr* f* xr2;
  x5= t3*(1.-3.* cppp2)* xr1;
  x6= t4*(1.-3.* cpp2)*(1.- ctx->gwav.u2*(1.+ rv)- ctx->gwav.u2* omr* f)* xr2;
  x7= ctx->gwav.u2* cpp2* omr*(1.-1./ rk2)*( f*( ctx->gwav.u2* t1- spp2-1./ rk2)+1./rk2)* xr2;
  *erh=( x1- x2- x4- x5+ x6+ x7)* (-CONST4);
  x1= xr1;
  x2= rh* xr2;
  x3=( rh+1.)* g* xr2;
  x4= t3* xr1;
  x5= t4*(1.- ctx->gwav.u2*(1.+ rv)- ctx->gwav.u2* omr* f)* xr2;
  x6=.5* ctx->gwav.u2* omr*( f*( ctx->gwav.u2* t1- spp2-1./ rk2)+1./ rk2)* xr2/ rk2;
  *eph=-( x1- x2+ x3- x4+ x5+ x6)* (-CONST4);
  
  return;
}

/*-----------------------------------------------------------------------*/

/* segment end contributions for thin wire approx. */
void gx(nec_context_t *ctx, double zz, double rh, double xk,
        complex double *gz, complex double *gzp)
{
  double r, r2, rkz;
  
  r2= zz* zz+ rh* rh;
  r= sqrt( r2);
  rkz= xk* r;
  *gz= cmplx( cos( rkz),- sin( rkz))/ r;
  *gzp= -cmplx(1.0, rkz)* *gz/ r2;
  
  return;
}

/*-----------------------------------------------------------------------*/

/* segment end contributions for ext. thin wire approx. */
void gxx(nec_context_t *ctx, double zz, double rh, double a,
         double a2, double xk, int ira, complex double *g1,
         complex double *g1p, complex double *g2,
         complex double *g2p, complex double *g3, complex double *gzp )
{
  double r, r2, r4, rk, rk2, rh2, t1, t2;
  complex double  gz, c1, c2, c3;
  
  r2= zz* zz+ rh* rh;
  r= sqrt( r2);
  r4= r2* r2;
  rk= xk* r;
  rk2= rk* rk;
  rh2= rh* rh;
  t1=.25* a2* rh2/ r4;
  t2=.5* a2/ r2;
  c1= cmplx(1.0, rk);
  c2=3.* c1- rk2;
  c3= cmplx(6.0, rk)* rk2-15.* c1;
  gz= cmplx( cos( rk),- sin( rk))/ r;
  *g2= gz*(1.+ t1* c2);
  *g1= *g2- t2* c1* gz;
  gz= gz/ r2;
  *g2p= gz*( t1* c3- c1);
  *gzp= t2* c2* gz;
  *g3= *g2p+ *gzp;
  *g1p= *g3* zz;
  
  if( ira != 1)
  {
    *g3=( *g3+ *gzp)* rh;
    *gzp= -zz* c1* gz;
    
    if( rh <= 1.0e-10)
    {
      *g2=0.;
      *g2p=0.;
      return;
    }
    
    *g2= *g2/ rh;
    *g2p= *g2p* zz/ rh;
    return;
    
  } /* if( ira != 1) */
  
  t2=.5* a;
  *g2= -t2* c1* gz;
  *g2p= t2* gz* c2/ r2;
  *g3= rh2* *g2p- a* gz* c1;
  *g2p= *g2p* zz;
  *gzp= -zz* c1* gz;
  
  return;
}

/*-----------------------------------------------------------------------*/

/* hfk computes the h field of a uniform current */
/* filament by numerical integration */
void hfk(nec_context_t *ctx, double el1, double el2, double rhk,
         double zpkx, double *sgr, double *sgi )
{
  int nx = 1, nma = 65536, nts = 4;
  int ns, nt;
  int flag = TRUE;
  double rx = 1.0e-4;
  double z, ze, s, ep, zend, dz=0., zp, dzot=0., t00r, g1r, g5r=0, t00i;
  double g1i, g5i=0., t01r, g3r=0, t01i, g3i=0, t10r, t10i, te1i, te1r, t02r;
  double g2r, g4r, t02i, g2i, g4i, t11r, t11i, t20r, t20i, te2i, te2r;
  
  tmh.zpka= zpkx;
  tmh.rhks= rhk* rhk;
  z= el1;
  ze= el2;
  s= ze- z;
  ep= s/(10.* nma);
  zend= ze- ep;
  *sgr=0.0;
  *sgi=0.0;
  ns= nx;
  nt=0;
  gh(ctx, z, &g1r, &g1i);
  
  while( TRUE )
  {
    if( flag )
    {
      dz= s/ ns;
      zp= z+ dz;
      
      if( zp > ze )
      {
        dz= ze- z;
        if( fabs(dz) <= ep )
        {
          *sgr= *sgr* rhk*.5;
          *sgi= *sgi* rhk*.5;
          return;
        }
      }
      
      dzot= dz*.5;
      zp= z+ dzot;
      gh(ctx, zp, &g3r, &g3i);
      zp= z+ dz;
      gh(ctx, zp, &g5r, &g5i);
      
    } /* if( flag ) */
    
    t00r=( g1r+ g5r)* dzot;
    t00i=( g1i+ g5i)* dzot;
    t01r=( t00r+ dz* g3r)*0.5;
    t01i=( t00i+ dz* g3i)*0.5;
    t10r=(4.0* t01r- t00r)/3.0;
    t10i=(4.0* t01i- t00i)/3.0;
    
    test(ctx, t01r, t10r, &te1r, t01i, t10i, &te1i, 0.);
    if( (te1i <= rx) && (te1r <= rx) )
    {
      *sgr= *sgr+ t10r;
      *sgi= *sgi+ t10i;
      nt += 2;
      
      z += dz;
      if( z >= zend)
      {
        *sgr= *sgr* rhk*.5;
        *sgi= *sgi* rhk*.5;
        return;
      }
      
      g1r= g5r;
      g1i= g5i;
      if( nt >= nts)
        if( ns > nx)
        {
          ns= ns/2;
          nt=1;
        }
      flag = TRUE;
      continue;
      
    } /* if( (te1i <= rx) && (te1r <= rx) ) */
    
    zp= z+ dz*0.25;
    gh(ctx, zp, &g2r, &g2i);
    zp= z+ dz*0.75;
    gh(ctx, zp, &g4r, &g4i);
    t02r=( t01r+ dzot*( g2r+ g4r))*0.5;
    t02i=( t01i+ dzot*( g2i+ g4i))*0.5;
    t11r=(4.0* t02r- t01r)/3.0;
    t11i=(4.0* t02i- t01i)/3.0;
    t20r=(16.0* t11r- t10r)/15.0;
    t20i=(16.0* t11i- t10i)/15.0;
    
    test(ctx, t11r, t20r, &te2r, t11i, t20i, &te2i, 0.);
    if( (te2i > rx) || (te2r > rx) )
    {
      nt=0;
      if( ns >= nma)
        fprintf( ctx->output_fp, "\n  STEP SIZE LIMITED AT Z= %10.5f", z );
      else
      {
        ns= ns*2;
        dz= s/ ns;
        dzot= dz*0.5;
        g5r= g3r;
        g5i= g3i;
        g3r= g2r;
        g3i= g2i;
        
        flag = FALSE;
        continue;
      }
      
    } /* if( (te2i > rx) || (te2r > rx) ) */
    
    *sgr= *sgr+ t20r;
    *sgi= *sgi+ t20i;
    nt++;
    
    z += dz;
    if( z >= zend)
    {
      *sgr= *sgr* rhk*.5;
      *sgi= *sgi* rhk*.5;
      return;
    }
    
    g1r= g5r;
    g1i= g5i;
    if( nt >= nts)
      if( ns > nx)
      {
        ns= ns/2;
        nt=1;
      }
    flag = TRUE;
    
  } /* while( TRUE ) */
  
}

/*-----------------------------------------------------------------------*/

/* hintg computes the h field of a patch current */
void hintg(nec_context_t *ctx, double xi, double yi, double zi )
{
  int ip;
  double rx, ry, rfl, xymag, pxx, pyy, cth;
  double rz, rsq, r, rk, cr, sr, t1zr, t2zr;
  complex double  gam, f1x, f1y, f1z, f2x, f2y, f2z, rrv, rrh;
  
  rx= xi- ctx->dataj.xj;
  ry= yi- ctx->dataj.yj;
  rfl=-1.;
  ctx->dataj.exk=CPLX_00;
  ctx->dataj.eyk=CPLX_00;
  ctx->dataj.ezk=CPLX_00;
  ctx->dataj.exs=CPLX_00;
  ctx->dataj.eys=CPLX_00;
  ctx->dataj.ezs=CPLX_00;
  
  for( ip = 1; ip <= ctx->gnd.ksymp; ip++ )
  {
    rfl= -rfl;
    rz= zi- ctx->dataj.zj* rfl;
    rsq= rx* rx+ ry* ry+ rz* rz;
    
    if( rsq < 1.0e-20)
      continue;
    
    r = sqrt( rsq );
    rk= TP* r;
    cr= cos( rk);
    sr= sin( rk);
    gam=-( cmplx(cr,-sr)+rk*cmplx(sr,cr) )/( FPI*rsq*r )* ctx->dataj.s;
    ctx->dataj.exc= gam* rx;
    ctx->dataj.eyc= gam* ry;
    ctx->dataj.ezc= gam* rz;
    t1zr= ctx->dataj.t1zj* rfl;
    t2zr= ctx->dataj.t2zj* rfl;
    f1x= ctx->dataj.eyc* t1zr- ctx->dataj.ezc* ctx->dataj.t1yj;
    f1y= ctx->dataj.ezc* ctx->dataj.t1xj- ctx->dataj.exc* t1zr;
    f1z= ctx->dataj.exc* ctx->dataj.t1yj- ctx->dataj.eyc* ctx->dataj.t1xj;
    f2x= ctx->dataj.eyc* t2zr- ctx->dataj.ezc* ctx->dataj.t2yj;
    f2y= ctx->dataj.ezc* ctx->dataj.t2xj- ctx->dataj.exc* t2zr;
    f2z= ctx->dataj.exc* ctx->dataj.t2yj- ctx->dataj.eyc* ctx->dataj.t2xj;
    
    if( ip != 1)
    {
      if( ctx->gnd.iperf == 1)
      {
        f1x= -f1x;
        f1y= -f1y;
        f1z= -f1z;
        f2x= -f2x;
        f2y= -f2y;
        f2z= -f2z;
      }
      else
      {
        xymag= sqrt( rx* rx+ ry* ry);
        if( xymag <= 1.0e-6)
        {
          pxx=0.;
          pyy=0.;
          cth=1.;
          rrv=CPLX_10;
        }
        else
        {
          pxx= -ry/ xymag;
          pyy= rx/ xymag;
          cth= rz/ r;
          rrv= csqrt(1.- ctx->gnd.zrati* ctx->gnd.zrati*(1.- cth* cth));
          
        } /* if( xymag <= 1.0e-6) */
        
        rrh= ctx->gnd.zrati* cth;
        rrh=( rrh- rrv)/( rrh+ rrv);
        rrv= ctx->gnd.zrati* rrv;
        rrv=-( cth- rrv)/( cth+ rrv);
        gam=( f1x* pxx+ f1y* pyy)*( rrv- rrh);
        f1x= f1x* rrh+ gam* pxx;
        f1y= f1y* rrh+ gam* pyy;
        f1z= f1z* rrh;
        gam=( f2x* pxx+ f2y* pyy)*( rrv- rrh);
        f2x= f2x* rrh+ gam* pxx;
        f2y= f2y* rrh+ gam* pyy;
        f2z= f2z* rrh;
        
      } /* if( ctx->gnd.iperf == 1) */
      
    } /* if( ip != 1) */
    
    ctx->dataj.exk += f1x;
    ctx->dataj.eyk += f1y;
    ctx->dataj.ezk += f1z;
    ctx->dataj.exs += f2x;
    ctx->dataj.eys += f2y;
    ctx->dataj.ezs += f2z;
    
  } /* for( ip = 1; ip <= ctx->gnd.ksymp; ip++ ) */
  
  return;
}

/*-----------------------------------------------------------------------*/

/* hsfld computes the h field for constant, sine, and */
/* cosine current on a segment including ground effects. */
void hsfld(nec_context_t *ctx,  double xi, double yi, double zi, double ai )
{
  int ip;
  double xij, yij, rfl, salpr, zij, zp, rhox, rhoy, rhoz, rh, phx;
  double phy, phz, rmag, xymag, xspec, yspec, rhospc, px, py, cth;
  complex double hpk, hps, hpc, qx, qy, qz, rrv, rrh, zratx;
  
  xij= xi- ctx->dataj.xj;
  yij= yi- ctx->dataj.yj;
  rfl=-1.;
  
  for( ip = 0; ip < ctx->gnd.ksymp; ip++ )
  {
    rfl= -rfl;
    salpr= ctx->dataj.salpj* rfl;
    zij= zi- rfl* ctx->dataj.zj;
    zp= xij* ctx->dataj.cabj+ yij* ctx->dataj.sabj+ zij* salpr;
    rhox= xij- ctx->dataj.cabj* zp;
    rhoy= yij- ctx->dataj.sabj* zp;
    rhoz= zij- salpr* zp;
    rh= sqrt( rhox* rhox+ rhoy* rhoy+ rhoz* rhoz+ ai* ai);
    
    if( rh <= 1.0e-10)
    {
      ctx->dataj.exk=0.;
      ctx->dataj.eyk=0.;
      ctx->dataj.ezk=0.;
      ctx->dataj.exs=0.;
      ctx->dataj.eys=0.;
      ctx->dataj.ezs=0.;
      ctx->dataj.exc=0.;
      ctx->dataj.eyc=0.;
      ctx->dataj.ezc=0.;
      continue;
    }
    
    rhox= rhox/ rh;
    rhoy= rhoy/ rh;
    rhoz= rhoz/ rh;
    phx= ctx->dataj.sabj* rhoz- salpr* rhoy;
    phy= salpr* rhox- ctx->dataj.cabj* rhoz;
    phz= ctx->dataj.cabj* rhoy- ctx->dataj.sabj* rhox;
    
    hsflx(ctx, ctx->dataj.s, rh, zp, &hpk, &hps, &hpc);
    
    if( ip == 1 )
    {
      if( ctx->gnd.iperf != 1 )
      {
        zratx= ctx->gnd.zrati;
        rmag= sqrt( zp* zp+ rh* rh);
        xymag= sqrt( xij* xij+ yij* yij);
        
        /* set parameters for radial wire ground screen. */
        if( ctx->gnd.nradl != 0)
        {
          xspec=( xi* ctx->dataj.zj+ zi* ctx->dataj.xj)/( zi+ ctx->dataj.zj);
          yspec=( yi* ctx->dataj.zj+ zi* ctx->dataj.yj)/( zi+ ctx->dataj.zj);
          rhospc= sqrt( xspec* xspec+ yspec* yspec+ ctx->gnd.t2* ctx->gnd.t2);
          
          if( rhospc <= ctx->gnd.scrwl)
          {
            rrv= ctx->gnd.t1* rhospc* log( rhospc/ ctx->gnd.t2);
            zratx=( rrv* ctx->gnd.zrati)/( ETA* ctx->gnd.zrati+ rrv);
          }
        }
        
        /* calculation of reflection coefficients when ground is specified. */
        if( xymag <= 1.0e-6)
        {
          px=0.;
          py=0.;
          cth=1.;
          rrv=CPLX_10;
        }
        else
        {
          px= -yij/ xymag;
          py= xij/ xymag;
          cth= zij/ rmag;
          rrv= csqrt(1.- zratx* zratx*(1.- cth* cth));
        }
        
        rrh= zratx* cth;
        rrh=-( rrh- rrv)/( rrh+ rrv);
        rrv= zratx* rrv;
        rrv=( cth- rrv)/( cth+ rrv);
        qy=( phx* px+ phy* py)*( rrv- rrh);
        qx= qy* px+ phx* rrh;
        qy= qy* py+ phy* rrh;
        qz= phz* rrh;
        ctx->dataj.exk= ctx->dataj.exk- hpk* qx;
        ctx->dataj.eyk= ctx->dataj.eyk- hpk* qy;
        ctx->dataj.ezk= ctx->dataj.ezk- hpk* qz;
        ctx->dataj.exs= ctx->dataj.exs- hps* qx;
        ctx->dataj.eys= ctx->dataj.eys- hps* qy;
        ctx->dataj.ezs= ctx->dataj.ezs- hps* qz;
        ctx->dataj.exc= ctx->dataj.exc- hpc* qx;
        ctx->dataj.eyc= ctx->dataj.eyc- hpc* qy;
        ctx->dataj.ezc= ctx->dataj.ezc- hpc* qz;
        continue;
        
      } /* if( ctx->gnd.iperf != 1 ) */
      
      ctx->dataj.exk= ctx->dataj.exk- hpk* phx;
      ctx->dataj.eyk= ctx->dataj.eyk- hpk* phy;
      ctx->dataj.ezk= ctx->dataj.ezk- hpk* phz;
      ctx->dataj.exs= ctx->dataj.exs- hps* phx;
      ctx->dataj.eys= ctx->dataj.eys- hps* phy;
      ctx->dataj.ezs= ctx->dataj.ezs- hps* phz;
      ctx->dataj.exc= ctx->dataj.exc- hpc* phx;
      ctx->dataj.eyc= ctx->dataj.eyc- hpc* phy;
      ctx->dataj.ezc= ctx->dataj.ezc- hpc* phz;
      continue;
      
    } /* if( ip == 1 ) */
    
    ctx->dataj.exk= hpk* phx;
    ctx->dataj.eyk= hpk* phy;
    ctx->dataj.ezk= hpk* phz;
    ctx->dataj.exs= hps* phx;
    ctx->dataj.eys= hps* phy;
    ctx->dataj.ezs= hps* phz;
    ctx->dataj.exc= hpc* phx;
    ctx->dataj.eyc= hpc* phy;
    ctx->dataj.ezc= hpc* phz;
    
  } /* for( ip = 0; ip < ctx->gnd.ksymp; ip++ ) */
  
  return;
}

/*-----------------------------------------------------------------------*/

/* calculates h field of sine cosine, and constant current of segment */
void hsflx(nec_context_t *ctx, double s, double rh, double zpx,
           complex double *hpk, complex double *hps,
           complex double *hpc )
{
  complex double fjk, ekr1, ekr2, t1, t2, cons;
  
  fjk = -TPJ;
  if( rh >= 1.0e-10)
  {
    double zp, z2a, hss, dh, z1;
    double rhz, dk, cdk, sdk, hkr, hki;
    
    if( zpx >= 0.)
    {
      zp= zpx;
      hss=1.;
    }
    else
    {
      zp= -zpx;
      hss=-1.;
    }
    
    dh=.5* s;
    z1= zp+ dh;
    z2a= zp- dh;
    if( z2a >= 1.0e-7)
      rhz= rh/ z2a;
    else
      rhz=1.;
    
    dk= TP* dh;
    cdk= cos( dk);
    sdk= sin( dk);
    hfk(ctx, -dk, dk, rh* TP, zp* TP, &hkr, &hki);
    *hpk= cmplx( hkr, hki);
    
    if( rhz >= 1.0e-3)
    {
      double rh2, r1, r2;
      rh2= rh* rh;
      r1= sqrt( rh2+ z1* z1);
      r2= sqrt( rh2+ z2a* z2a);
      ekr1= cexp( fjk* r1);
      ekr2= cexp( fjk* r2);
      t1= z1* ekr1/ r1;
      t2= z2a* ekr2/ r2;
      *hps=( cdk*( ekr2- ekr1)- CPLX_01* sdk*( t2+ t1))* hss;
      *hpc= -sdk*( ekr2+ ekr1)- CPLX_01* cdk*( t2- t1);
      cons= -CPLX_01/(2.* TP* rh);
      *hps= cons* *hps;
      *hpc= cons* *hpc;
      return;
      
    } /* if( rhz >= 1.0e-3) */
    
    ekr1= cmplx( cdk, sdk)/( z2a* z2a);
    ekr2= cmplx( cdk,- sdk)/( z1* z1);
    t1= TP*(1./ z1-1./ z2a);
    t2= cexp( fjk* zp)* rh/ PI8;
    *hps= t2*( t1+( ekr1+ ekr2)* sdk)* hss;
    *hpc= t2*(- CPLX_01* t1+( ekr1- ekr2)* cdk);
    return;
    
  } /* if( rh >= 1.0e-10) */
  
  *hps=CPLX_00;
  *hpc=CPLX_00;
  *hpk=CPLX_00;
  
  return;
}

/*-----------------------------------------------------------------------*/

/* nefld computes the near field at specified points in space after */
/* the structure currents have been computed. */
void nefld(nec_context_t *ctx, double xob, double yob, double zob,
           complex double *ex, complex double *ey, complex double *ez )
{
  int i, ix, ipr, iprx, jc, ipa;
  double zp, xi, ax;
  complex double acx, bcx, ccx;
  
  *ex=CPLX_00;
  *ey=CPLX_00;
  *ez=CPLX_00;
  ax=0.;
  
  if( ctx->geometry.n != 0)
  {
    for( i = 0; i < ctx->geometry.n; i++ )
    {
      ctx->dataj.xj= xob- ctx->geometry.x[i];
      ctx->dataj.yj= yob- ctx->geometry.y[i];
      ctx->dataj.zj= zob- ctx->geometry.z[i];
      zp= ctx->geometry.cab[i]* ctx->dataj.xj+ ctx->geometry.sab[i]* ctx->dataj.yj+ ctx->geometry.salp[i]* ctx->dataj.zj;
      
      if( fabs( zp) > 0.5001* ctx->geometry.si[i])
        continue;
      
      zp= ctx->dataj.xj* ctx->dataj.xj+ ctx->dataj.yj* ctx->dataj.yj+ ctx->dataj.zj* ctx->dataj.zj- zp* zp;
      ctx->dataj.xj= ctx->geometry.bi[i];
      
      if( zp > 0.9* ctx->dataj.xj* ctx->dataj.xj)
        continue;
      
      ax= ctx->dataj.xj;
      break;
      
    } /* for( i = 0; i < n; i++ ) */
    
    for( i = 0; i < ctx->geometry.n; i++ )
    {
      ix = i+1;
      ctx->dataj.s= ctx->geometry.si[i];
      ctx->dataj.b= ctx->geometry.bi[i];
      ctx->dataj.xj= ctx->geometry.x[i];
      ctx->dataj.yj= ctx->geometry.y[i];
      ctx->dataj.zj= ctx->geometry.z[i];
      ctx->dataj.cabj= ctx->geometry.cab[i];
      ctx->dataj.sabj= ctx->geometry.sab[i];
      ctx->dataj.salpj= ctx->geometry.salp[i];
      
      if( ctx->dataj.iexk != 0)
      {
        ipr= ctx->geometry.icon1[i];
        
        if(ipr > PCHCON) ctx->dataj.ind1 = 2;
        else if( ipr < 0 )
        {
          ipr = -ipr;
          iprx = ipr-1;
          
          if( -ctx->geometry.icon1[iprx] != ix )
            ctx->dataj.ind1=2;
          else
          {
            xi= fabs( ctx->dataj.cabj* ctx->geometry.cab[iprx]+ ctx->dataj.sabj*
                     ctx->geometry.sab[iprx]+ ctx->dataj.salpj* ctx->geometry.salp[iprx]);
            if( (xi < 0.999999) || (fabs(ctx->geometry.bi[iprx]/ctx->dataj.b-1.) > 1.0e-6) )
              ctx->dataj.ind1=2;
            else
              ctx->dataj.ind1=0;
          }
        } /* if( ipr < 0 ) */
        else
          if( ipr == 0 )
            ctx->dataj.ind1=1;
          else
          {
            iprx = ipr-1;
            
            if( ipr != ix )
            {
              if( ctx->geometry.icon2[iprx] != ix )
                ctx->dataj.ind1=2;
              else
              {
                xi= fabs( ctx->dataj.cabj* ctx->geometry.cab[iprx]+ ctx->dataj.sabj*
                         ctx->geometry.sab[iprx]+ ctx->dataj.salpj* ctx->geometry.salp[iprx]);
                if( (xi < 0.999999) || (fabs(ctx->geometry.bi[iprx]/ctx->dataj.b-1.) > 1.0e-6) )
                  ctx->dataj.ind1=2;
                else
                  ctx->dataj.ind1=0;
              }
            } /* if( ipr != ix ) */
            else
            {
              if( ctx->dataj.cabj* ctx->dataj.cabj+ ctx->dataj.sabj* ctx->dataj.sabj > 1.0e-8)
                ctx->dataj.ind1=2;
              else
                ctx->dataj.ind1=0;
            }
          } /* else */
        
        ipr= ctx->geometry.icon2[i];
        
        if (ipr > PCHCON) ctx->dataj.ind2 = 2;
        else if( ipr < 0 )
        {
          ipr = -ipr;
          iprx = ipr-1;
          
          if( -ctx->geometry.icon2[iprx] != ix )
            ctx->dataj.ind1=2;
          else
          {
            xi= fabs( ctx->dataj.cabj* ctx->geometry.cab[iprx]+ ctx->dataj.sabj*
                     ctx->geometry.sab[iprx]+ ctx->dataj.salpj* ctx->geometry.salp[iprx]);
            if( (xi < 0.999999) || (fabs(ctx->geometry.bi[iprx]/ctx->dataj.b-1.) > 1.0e-6) )
              ctx->dataj.ind1=2;
            else
              ctx->dataj.ind1=0;
          }
        } /* if( ipr < 0 ) */
        else
          if( ipr == 0 )
            ctx->dataj.ind2=1;
          else
          {
            iprx = ipr-1;
            
            if( ipr != ix )
            {
              if( ctx->geometry.icon1[iprx] != ix )
                ctx->dataj.ind2=2;
              else
              {
                xi= fabs( ctx->dataj.cabj* ctx->geometry.cab[iprx]+ ctx->dataj.sabj*
                         ctx->geometry.sab[iprx]+ ctx->dataj.salpj* ctx->geometry.salp[iprx]);
                if( (xi < 0.999999) || (fabs(ctx->geometry.bi[iprx]/ctx->dataj.b-1.) > 1.0e-6) )
                  ctx->dataj.ind2=2;
                else
                  ctx->dataj.ind2=0;
              }
            } /* if( ipr != (i+1) ) */
            else
            {
              if( ctx->dataj.cabj* ctx->dataj.cabj+ ctx->dataj.sabj* ctx->dataj.sabj > 1.0e-8)
                ctx->dataj.ind1=2;
              else
                ctx->dataj.ind1=0;
            }
            
          } /* else */
        
      } /* if( ctx->dataj.iexk != 0) */
      
      efld(ctx,  xob, yob, zob, ax,1);
      acx= cmplx( ctx->crnt.air[i], ctx->crnt.aii[i]);
      bcx= cmplx( ctx->crnt.bir[i], ctx->crnt.bii[i]);
      ccx= cmplx( ctx->crnt.cir[i], ctx->crnt.cii[i]);
      *ex += ctx->dataj.exk* acx+ ctx->dataj.exs* bcx+ ctx->dataj.exc* ccx;
      *ey += ctx->dataj.eyk* acx+ ctx->dataj.eys* bcx+ ctx->dataj.eyc* ccx;
      *ez += ctx->dataj.ezk* acx+ ctx->dataj.ezs* bcx+ ctx->dataj.ezc* ccx;
      
    } /* for( i = 0; i < n; i++ ) */
    
    if( ctx->geometry.m == 0)
      return;
    
  } /* if( n != 0) */
  
  jc= ctx->geometry.n-1;
  for( i = 0; i < ctx->geometry.m; i++ )
  {
    ctx->dataj.s= ctx->geometry.pbi[i];
    ctx->dataj.xj= ctx->geometry.px[i];
    ctx->dataj.yj= ctx->geometry.py[i];
    ctx->dataj.zj= ctx->geometry.pz[i];
    ctx->dataj.t1xj= ctx->geometry.t1x[i];
    ctx->dataj.t1yj= ctx->geometry.t1y[i];
    ctx->dataj.t1zj= ctx->geometry.t1z[i];
    ctx->dataj.t2xj= ctx->geometry.t2x[i];
    ctx->dataj.t2yj= ctx->geometry.t2y[i];
    ctx->dataj.t2zj= ctx->geometry.t2z[i];
    jc += 3;
    acx= ctx->dataj.t1xj* ctx->crnt.cur[jc-2]+ ctx->dataj.t1yj* ctx->crnt.cur[jc-1]+ ctx->dataj.t1zj* ctx->crnt.cur[jc];
    bcx= ctx->dataj.t2xj* ctx->crnt.cur[jc-2]+ ctx->dataj.t2yj* ctx->crnt.cur[jc-1]+ ctx->dataj.t2zj* ctx->crnt.cur[jc];
    
    for( ipa = 0; ipa < ctx->gnd.ksymp; ipa++ )
    {
      ctx->dataj.ipgnd= ipa+1;
      unere(ctx,  xob, yob, zob);
      *ex= *ex+ acx* ctx->dataj.exk+ bcx* ctx->dataj.exs;
      *ey= *ey+ acx* ctx->dataj.eyk+ bcx* ctx->dataj.eys;
      *ez= *ez+ acx* ctx->dataj.ezk+ bcx* ctx->dataj.ezs;
    }
    
  } /* for( i = 0; i < m; i++ ) */
  
  return;
}

/*-------------------------------------------------------------------*/
/* fill incident field array for charge discontinuity voltage source */
//for some reason this was in input.c in nec2c
void qdsrc(nec_context_t *ctx, int is, complex double v, complex double *e )
{
  int i, jx, j, jp1, ipr, ij, i1;
  double xi, yi, zi, ai, cabi, sabi, salpi, tx, ty, tz;
  complex double curd, etk, ets, etc;
  
  is--;
  i= ctx->geometry.icon1[is];
  ctx->geometry.icon1[is] = 0;
  tbf(ctx, is+1, 0);
  ctx->geometry.icon1[is]= i;
  ctx->dataj.s= ctx->geometry.si[is]*.5;
  curd= CCJ* v/(( log(2.* ctx->dataj.s/ ctx->geometry.bi[is])-1.)*( ctx->segj.bx[ctx->segj.jsno-1]*
                                                         cos( TP* ctx->dataj.s)+ ctx->segj.cx[ctx->segj.jsno-1]* sin( TP* ctx->dataj.s))* ctx->geometry.wlam);
  ctx->vsorc.vqds[ctx->vsorc.nqds]= v;
  ctx->vsorc.iqds[ctx->vsorc.nqds]= is+1;
  ctx->vsorc.nqds++;
  
  for( jx = 0; jx < ctx->segj.jsno; jx++ )
  {
    j= ctx->segj.jco[jx]-1;
    jp1 = j+1;
    ctx->dataj.s= ctx->geometry.si[j];
    ctx->dataj.b= ctx->geometry.bi[j];
    ctx->dataj.xj= ctx->geometry.x[j];
    ctx->dataj.yj= ctx->geometry.y[j];
    ctx->dataj.zj= ctx->geometry.z[j];
    ctx->dataj.cabj= ctx->geometry.cab[j];
    ctx->dataj.sabj= ctx->geometry.sab[j];
    ctx->dataj.salpj= ctx->geometry.salp[j];
    
    if( ctx->dataj.iexk != 0)
    {
      ipr= ctx->geometry.icon1[j];
      
      if (ipr > PCHCON) ctx->dataj.ind1=2;
      else if( ipr < 0 )
      {
        ipr= -ipr;
        ipr--;
        if( -ctx->geometry.icon1[ipr-1] != jp1 )
          ctx->dataj.ind1=2;
        else
        {
          xi= fabs( ctx->dataj.cabj* ctx->geometry.cab[ipr]+ ctx->dataj.sabj*
                   ctx->geometry.sab[ipr]+ ctx->dataj.salpj* ctx->geometry.salp[ipr]);
          if( (xi < 0.999999) || (fabs(ctx->geometry.bi[ipr]/ctx->dataj.b-1.) > 1.0e-6) )
            ctx->dataj.ind1=2;
          else
            ctx->dataj.ind1=0;
        }
      }  /* if( ipr < 0 ) */
      else
        if( ipr == 0 )
          ctx->dataj.ind1=1;
        else /* ipr > 0 */
        {
          ipr--;
          if( ipr != j )
          {
            if( ctx->geometry.icon2[ipr] != jp1)
              ctx->dataj.ind1=2;
            else
            {
              xi= fabs( ctx->dataj.cabj* ctx->geometry.cab[ipr]+ ctx->dataj.sabj*
                       ctx->geometry.sab[ipr]+ ctx->dataj.salpj* ctx->geometry.salp[ipr]);
              if( (xi < 0.999999) || (fabs(ctx->geometry.bi[ipr]/ctx->dataj.b-1.) > 1.0e-6) )
                ctx->dataj.ind1=2;
              else
                ctx->dataj.ind1=0;
            }
          } /* if( ipr != j ) */
          else
          {
            if( ctx->dataj.cabj* ctx->dataj.cabj+ ctx->dataj.sabj* ctx->dataj.sabj > 1.0e-8)
              ctx->dataj.ind1=2;
            else
              ctx->dataj.ind1=0;
          }
        } /* else */
      
      ipr= ctx->geometry.icon2[j];
      if (ipr > PCHCON) ctx->dataj.ind2=2;
      else if( ipr < 0 )
      {
        ipr = -ipr;
        ipr--;
        if( -ctx->geometry.icon2[ipr] != jp1 )
          ctx->dataj.ind1=2;
        else
        {
          xi= fabs( ctx->dataj.cabj* ctx->geometry.cab[ipr]+ ctx->dataj.sabj*
                   ctx->geometry.sab[ipr]+ ctx->dataj.salpj* ctx->geometry.salp[ipr]);
          if( (xi < 0.999999) || (fabs(ctx->geometry.bi[ipr]/ctx->dataj.b-1.) > 1.0e-6) )
            ctx->dataj.ind1=2;
          else
            ctx->dataj.ind1=0;
        }
      } /* if( ipr < 0 ) */
      else
        if( ipr == 0 )
          ctx->dataj.ind2=1;
        else /* ipr > 0 */
        {
          ipr--;
          if( ipr != j )
          {
            if( ctx->geometry.icon1[ipr] != jp1)
              ctx->dataj.ind2=2;
            else
            {
              xi= fabs( ctx->dataj.cabj* ctx->geometry.cab[ipr]+ ctx->dataj.sabj*
                       ctx->geometry.sab[ipr]+ ctx->dataj.salpj* ctx->geometry.salp[ipr]);
              if( (xi < 0.999999) || (fabs(ctx->geometry.bi[ipr]/ctx->dataj.b-1.) > 1.0e-6) )
                ctx->dataj.ind2=2;
              else
                ctx->dataj.ind2=0;
            }
          } /* if( ipr != j )*/
          else
          {
            if( ctx->dataj.cabj* ctx->dataj.cabj+ ctx->dataj.sabj* ctx->dataj.sabj > 1.0e-8)
              ctx->dataj.ind1=2;
            else
              ctx->dataj.ind1=0;
          }
        } /* else */
      
    } /* if( ctx->dataj.iexk != 0) */
    
    for( i = 0; i < ctx->geometry.n; i++ )
    {
      ij= i- j;
      xi= ctx->geometry.x[i];
      yi= ctx->geometry.y[i];
      zi= ctx->geometry.z[i];
      ai= ctx->geometry.bi[i];
      efld(ctx,  xi, yi, zi, ai, ij);
      cabi= ctx->geometry.cab[i];
      sabi= ctx->geometry.sab[i];
      salpi= ctx->geometry.salp[i];
      etk= ctx->dataj.exk* cabi+ ctx->dataj.eyk* sabi+ ctx->dataj.ezk* salpi;
      ets= ctx->dataj.exs* cabi+ ctx->dataj.eys* sabi+ ctx->dataj.ezs* salpi;
      etc= ctx->dataj.exc* cabi+ ctx->dataj.eyc* sabi+ ctx->dataj.ezc* salpi;
      e[i]= e[i]-( etk* ctx->segj.ax[jx]+ ets* ctx->segj.bx[jx]+ etc* ctx->segj.cx[jx])* curd;
    }
    
    if( ctx->geometry.m != 0)
    {
      i1= ctx->geometry.n-1;
      for( i = 0; i < ctx->geometry.m; i++ )
      {
        xi= ctx->geometry.px[i];
        yi= ctx->geometry.py[i];
        zi= ctx->geometry.pz[i];
        hsfld(ctx,  xi, yi, zi,0.);
        i1++;
        tx= ctx->geometry.t2x[i];
        ty= ctx->geometry.t2y[i];
        tz= ctx->geometry.t2z[i];
        etk= ctx->dataj.exk* tx+ ctx->dataj.eyk* ty+ ctx->dataj.ezk* tz;
        ets= ctx->dataj.exs* tx+ ctx->dataj.eys* ty+ ctx->dataj.ezs* tz;
        etc= ctx->dataj.exc* tx+ ctx->dataj.eyc* ty+ ctx->dataj.ezc* tz;
        e[i1] += ( etk* ctx->segj.ax[jx]+ ets* ctx->segj.bx[jx]+
                  etc* ctx->segj.cx[jx] )* curd* ctx->geometry.psalp[i];
        i1++;
        tx= ctx->geometry.t1x[i];
        ty= ctx->geometry.t1y[i];
        tz= ctx->geometry.t1z[i];
        etk= ctx->dataj.exk* tx+ ctx->dataj.eyk* ty+ ctx->dataj.ezk* tz;
        ets= ctx->dataj.exs* tx+ ctx->dataj.eys* ty+ ctx->dataj.ezs* tz;
        etc= ctx->dataj.exc* tx+ ctx->dataj.eyc* ty+ ctx->dataj.ezc* tz;
        e[i1] += ( etk* ctx->segj.ax[jx]+ ets* ctx->segj.bx[jx]+
                  etc* ctx->segj.cx[jx])* curd* ctx->geometry.psalp[i];
      }
      
    } /* if( m != 0) */
    
    if( ctx->zload.nload > 0 )
      e[j] += ctx->zload.zarray[j]* curd*(ctx->segj.ax[jx]+ ctx->segj.cx[jx]);
    
  } /* for( jx = 0; jx < ctx->segj.jsno; jx++ ) */
  
  return;
}

/*-----------------------------------------------------------------------*/

/* compute near e or h fields over a range of points */
void nfpat(nec_context_t *ctx)
{
  int i, j, kk;
  double znrt, cth=0., sth=0., ynrt, cph=0., sph=0., xnrt, xob, yob;
  double zob, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, xxx;
  complex double ex, ey, ez;
  
  if( ctx->fpat.nfeh != 1)
  {
    fprintf( ctx->output_fp,	"\n\n\n"
            "                             "
            "-------- NEAR ELECTRIC FIELDS --------\n"
            "     ------- LOCATION -------     ------- EX ------    ------- EY ------    ------- EZ ------\n"
            "      X         Y         Z       MAGNITUDE   PHASE    MAGNITUDE   PHASE    MAGNITUDE   PHASE\n"
            "    METERS    METERS    METERS     VOLTS/M  DEGREES    VOLTS/M   DEGREES     VOLTS/M  DEGREES" );
  }
  else
  {
    fprintf( ctx->output_fp,	"\n\n\n"
            "                                   "
            "-------- NEAR MAGNETIC FIELDS ---------\n\n"
            "     ------- LOCATION -------     ------- HX ------    ------- HY ------    ------- HZ ------\n"
            "      X         Y         Z       MAGNITUDE   PHASE    MAGNITUDE   PHASE    MAGNITUDE   PHASE\n"
            "    METERS    METERS    METERS      AMPS/M  DEGREES      AMPS/M  DEGREES      AMPS/M  DEGREES" );
  }
  
  znrt= ctx->fpat.znr- ctx->fpat.dznr;
  for( i = 0; i < ctx->fpat.nrz; i++ )
  {
    znrt += ctx->fpat.dznr;
    if( ctx->fpat.near != 0)
    {
      cth= cos( TA* znrt);
      sth= sin( TA* znrt);
    }
    
    ynrt= ctx->fpat.ynr- ctx->fpat.dynr;
    for( j = 0; j < ctx->fpat.nry; j++ )
    {
      ynrt += ctx->fpat.dynr;
      if( ctx->fpat.near != 0)
      {
        cph= cos( TA* ynrt);
        sph= sin( TA* ynrt);
      }
      
      xnrt= ctx->fpat.xnr- ctx->fpat.dxnr;
      for( kk = 0; kk < ctx->fpat.nrx; kk++ )
      {
        xnrt += ctx->fpat.dxnr;
        if( ctx->fpat.near != 0)
        {
          xob= xnrt* sth* cph;
          yob= xnrt* sth* sph;
          zob= xnrt* cth;
        }
        else
        {
          xob= xnrt;
          yob= ynrt;
          zob= znrt;
        }
        
        tmp1= xob/ ctx->geometry.wlam;
        tmp2= yob/ ctx->geometry.wlam;
        tmp3= zob/ ctx->geometry.wlam;
        
        if( ctx->fpat.nfeh != 1)
          nefld(ctx,  tmp1, tmp2, tmp3, &ex, &ey, &ez);
        else
          nhfld(ctx,  tmp1, tmp2, tmp3, &ex, &ey, &ez);
        
        tmp1= cabs( ex);
        tmp2= cang(ctx, ex);
        tmp3= cabs( ey);
        tmp4= cang(ctx, ey);
        tmp5= cabs( ez);
        tmp6= cang(ctx, ez);

        fprintf( ctx->output_fp, "\n"
                " %9.4f %9.4f %9.4f  %11.4E %7.2f  %11.4E %7.2f  %11.4E %7.2f",
                xob, yob, zob, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6 );
        
        if( ctx->plot.iplp1 != 2)
          continue;
        
        if( ctx->plot.iplp4 < 0 )
          xxx= xob;
        else
          if( ctx->plot.iplp4 == 0 )
            xxx= yob;
          else
            xxx= zob;
        
        if( ctx->plot.iplp2 == 2)
        {
          switch( ctx->plot.iplp3 )
          {
            case 1:
              fprintf( ctx->plot_fp, "%12.4E %12.4E %12.4E\n", xxx, tmp1, tmp2 );
              break;
            case 2:
              fprintf( ctx->plot_fp, "%12.4E %12.4E %12.4E\n", xxx, tmp3, tmp4 );
              break;
            case 3:
              fprintf( ctx->plot_fp, "%12.4E %12.4E %12.4E\n", xxx, tmp5, tmp6 );
              break;
            case 4:
              fprintf( ctx->plot_fp, "%12.4E %12.4E %12.4E %12.4E %12.4E %12.4E %12.4E\n",
                      xxx, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6 );
          }
          continue;
        }
        
        if( ctx->plot.iplp2 != 1)
          continue;
        
        switch( ctx->plot.iplp3 )
        {
          case 1:
            fprintf( ctx->plot_fp, "%12.4E %12.4E %12.4E\n", xxx, creal(ex), cimag(ex) );
            break;
          case 2:
            fprintf( ctx->plot_fp, "%12.4E %12.4E %12.4E\n", xxx, creal(ey), cimag(ey) );
            break;
          case 3:
            fprintf( ctx->plot_fp, "%12.4E %12.4E %12.4E\n", xxx, creal(ez), cimag(ez) );
            break;
          case 4:
            fprintf( ctx->plot_fp, "%12.4E %12.4E %12.4E %12.4E %12.4E %12.4E %12.4E\n",
                    xxx,creal(ex),cimag(ex),creal(ey),cimag(ey),creal(ez),cimag(ez) );
        }
      } /* for( kk = 0; kk < ctx->fpat.nrx; kk++ ) */
      
    } /* for( j = 0; j < ctx->fpat.nry; j++ ) */
    
  } /* for( i = 0; i < ctx->fpat.nrz; i++ ) */
  
  return;
}

/*-----------------------------------------------------------------------*/

/* nhfld computes the near field at specified points in space after */
/* the structure currents have been computed. */

void nhfld(nec_context_t *ctx,  double xob, double yob, double zob,
           complex double *hx, complex double *hy, complex double *hz )
{
  int i, jc;
  double ax, zp;
  complex double acx, bcx, ccx;
  
  *hx=CPLX_00;
  *hy=CPLX_00;
  *hz=CPLX_00;
  ax=0.;
  
  if(ctx->geometry.n != 0) {
    for(i = 0; i < ctx->geometry.n; i++) {
      ctx->dataj.xj= xob- ctx->geometry.x[i];
      ctx->dataj.yj= yob- ctx->geometry.y[i];
      ctx->dataj.zj= zob- ctx->geometry.z[i];
      zp= ctx->geometry.cab[i]* ctx->dataj.xj+ ctx->geometry.sab[i]* ctx->dataj.yj+ ctx->geometry.salp[i]* ctx->dataj.zj;
      
      if( fabs( zp) > 0.5001* ctx->geometry.si[i])
        continue;
      
      zp= ctx->dataj.xj* ctx->dataj.xj+ ctx->dataj.yj* ctx->dataj.yj+ ctx->dataj.zj* ctx->dataj.zj- zp* zp;
      ctx->dataj.xj= ctx->geometry.bi[i];
      
      if( zp > 0.9* ctx->dataj.xj* ctx->dataj.xj)
        continue;
      
      ax= ctx->dataj.xj;
      break;
    }
    
    for(i = 0; i < ctx->geometry.n; i++) {
      ctx->dataj.s= ctx->geometry.si[i];
      ctx->dataj.b= ctx->geometry.bi[i];
      ctx->dataj.xj= ctx->geometry.x[i];
      ctx->dataj.yj= ctx->geometry.y[i];
      ctx->dataj.zj= ctx->geometry.z[i];
      ctx->dataj.cabj= ctx->geometry.cab[i];
      ctx->dataj.sabj= ctx->geometry.sab[i];
      ctx->dataj.salpj= ctx->geometry.salp[i];
      hsfld(ctx,  xob, yob, zob, ax);
      acx= cmplx( ctx->crnt.air[i], ctx->crnt.aii[i]);
      bcx= cmplx( ctx->crnt.bir[i], ctx->crnt.bii[i]);
      ccx= cmplx( ctx->crnt.cir[i], ctx->crnt.cii[i]);
      *hx += ctx->dataj.exk* acx+ ctx->dataj.exs* bcx+ ctx->dataj.exc* ccx;
      *hy += ctx->dataj.eyk* acx+ ctx->dataj.eys* bcx+ ctx->dataj.eyc* ccx;
      *hz += ctx->dataj.ezk* acx+ ctx->dataj.ezs* bcx+ ctx->dataj.ezc* ccx;
    }
    
    if(ctx->geometry.m == 0)
      return;
    
  } /* if( data.n != 0) */
  
  jc = ctx->geometry.n - 1;
  for(i = 0; i < ctx->geometry.m; i++) {
    ctx->dataj.s= ctx->geometry.pbi[i];
    ctx->dataj.xj= ctx->geometry.px[i];
    ctx->dataj.yj= ctx->geometry.py[i];
    ctx->dataj.zj= ctx->geometry.pz[i];
    ctx->dataj.t1xj= ctx->geometry.t1x[i];
    ctx->dataj.t1yj= ctx->geometry.t1y[i];
    ctx->dataj.t1zj= ctx->geometry.t1z[i];
    ctx->dataj.t2xj= ctx->geometry.t2x[i];
    ctx->dataj.t2yj= ctx->geometry.t2y[i];
    ctx->dataj.t2zj= ctx->geometry.t2z[i];
    hintg(ctx,  xob, yob, zob);
    jc += 3;
    acx= ctx->dataj.t1xj* ctx->crnt.cur[jc-2]+ ctx->dataj.t1yj* ctx->crnt.cur[jc-1]+ ctx->dataj.t1zj* ctx->crnt.cur[jc];
    bcx= ctx->dataj.t2xj* ctx->crnt.cur[jc-2]+ ctx->dataj.t2yj* ctx->crnt.cur[jc-1]+ ctx->dataj.t2zj* ctx->crnt.cur[jc];
    *hx= *hx+ acx* ctx->dataj.exk+ bcx* ctx->dataj.exs;
    *hy= *hy+ acx* ctx->dataj.eyk+ bcx* ctx->dataj.eys;
    *hz= *hz+ acx* ctx->dataj.ezk+ bcx* ctx->dataj.ezs;
  }
  
  return;
}

/*-----------------------------------------------------------------------*/

/* integrate over patches at wire connection point */
void pcint(nec_context_t *ctx,  double xi, double yi, double zi, double cabi,
           double sabi, double salpi, complex double *e )
{
  int nint, i1, i2;
  double d, ds, da, gcon, fcon, xxj, xyj, xzj, xs, s1;
  double xss, yss, zss, s2x, s2, g1, g2, g3, g4, f2, f1;
  complex double e1, e2, e3, e4, e5, e6, e7, e8, e9;
  
  nint = 10;
  d= sqrt( ctx->dataj.s)*.5;
  ds=4.* d/ (double) nint;
  da= ds* ds;
  gcon=1./ ctx->dataj.s;
  fcon=1./(2.* TP* d);
  xxj= ctx->dataj.xj;
  xyj= ctx->dataj.yj;
  xzj= ctx->dataj.zj;
  xs= ctx->dataj.s;
  ctx->dataj.s= da;
  s1= d+ ds*.5;
  xss= ctx->dataj.xj+ s1*( ctx->dataj.t1xj+ ctx->dataj.t2xj);
  yss= ctx->dataj.yj+ s1*( ctx->dataj.t1yj+ ctx->dataj.t2yj);
  zss= ctx->dataj.zj+ s1*( ctx->dataj.t1zj+ ctx->dataj.t2zj);
  s1= s1+ d;
  s2x= s1;
  e1=CPLX_00;
  e2=CPLX_00;
  e3=CPLX_00;
  e4=CPLX_00;
  e5=CPLX_00;
  e6=CPLX_00;
  e7=CPLX_00;
  e8=CPLX_00;
  e9=CPLX_00;
  
  for( i1 = 0; i1 < nint; i1++ )
  {
    s1= s1- ds;
    s2= s2x;
    xss= xss- ds* ctx->dataj.t1xj;
    yss= yss- ds* ctx->dataj.t1yj;
    zss= zss- ds* ctx->dataj.t1zj;
    ctx->dataj.xj= xss;
    ctx->dataj.yj= yss;
    ctx->dataj.zj= zss;
    
    for( i2 = 0; i2 < nint; i2++ )
    {
      s2= s2- ds;
      ctx->dataj.xj= ctx->dataj.xj- ds* ctx->dataj.t2xj;
      ctx->dataj.yj= ctx->dataj.yj- ds* ctx->dataj.t2yj;
      ctx->dataj.zj= ctx->dataj.zj- ds* ctx->dataj.t2zj;
      unere(ctx,  xi, yi, zi);
      ctx->dataj.exk= ctx->dataj.exk* cabi+ ctx->dataj.eyk* sabi+ ctx->dataj.ezk* salpi;
      ctx->dataj.exs= ctx->dataj.exs* cabi+ ctx->dataj.eys* sabi+ ctx->dataj.ezs* salpi;
      g1=( d+ s1)*( d+ s2)* gcon;
      g2=( d- s1)*( d+ s2)* gcon;
      g3=( d- s1)*( d- s2)* gcon;
      g4=( d+ s1)*( d- s2)* gcon;
      f2=( s1* s1+ s2* s2)* TP;
      f1= s1/ f2-( g1- g2- g3+ g4)* fcon;
      f2= s2/ f2-( g1+ g2- g3- g4)* fcon;
      e1= e1+ ctx->dataj.exk* g1;
      e2= e2+ ctx->dataj.exk* g2;
      e3= e3+ ctx->dataj.exk* g3;
      e4= e4+ ctx->dataj.exk* g4;
      e5= e5+ ctx->dataj.exs* g1;
      e6= e6+ ctx->dataj.exs* g2;
      e7= e7+ ctx->dataj.exs* g3;
      e8= e8+ ctx->dataj.exs* g4;
      e9= e9+ ctx->dataj.exk* f1+ ctx->dataj.exs* f2;
      
    } /* for( i2 = 0; i2 < nint; i2++ ) */
    
  } /* for( i1 = 0; i1 < nint; i1++ ) */
  
  e[0]= e1;
  e[1]= e2;
  e[2]= e3;
  e[3]= e4;
  e[4]= e5;
  e[5]= e6;
  e[6]= e7;
  e[7]= e8;
  e[8]= e9;
  ctx->dataj.xj= xxj;
  ctx->dataj.yj= xyj;
  ctx->dataj.zj= xzj;
  ctx->dataj.s= xs;
  
  return;
}

/*-----------------------------------------------------------------------*/

/* calculates the electric field due to unit current */
/* in the t1 and t2 directions on a patch */
void unere(nec_context_t *ctx,  double xob, double yob, double zob )
{
  double zr, t1zr, t2zr, rx, ry, rz, r, tt1;
  double tt2, rt, xymag, px, py, cth, r2;
  complex double er, q1, q2, rrv, rrh, edp;
  
  zr= ctx->dataj.zj;
  t1zr= ctx->dataj.t1zj;
  t2zr= ctx->dataj.t2zj;
  
  if( ctx->dataj.ipgnd == 2)
  {
    zr= -zr;
    t1zr= -t1zr;
    t2zr= -t2zr;
  }
  
  rx= xob- ctx->dataj.xj;
  ry= yob- ctx->dataj.yj;
  rz= zob- zr;
  r2= rx* rx+ ry* ry+ rz* rz;
  
  if( r2 <= 1.0e-20)
  {
    ctx->dataj.exk=CPLX_00;
    ctx->dataj.eyk=CPLX_00;
    ctx->dataj.ezk=CPLX_00;
    ctx->dataj.exs=CPLX_00;
    ctx->dataj.eys=CPLX_00;
    ctx->dataj.ezs=CPLX_00;
    return;
  }
  
  r= sqrt( r2);
  tt1= -TP* r;
  tt2= tt1* tt1;
  rt= r2* r;
  er= cmplx( sin( tt1),- cos( tt1))*( CONST2* ctx->dataj.s);
  q1= cmplx( tt2-1., tt1)* er/ rt;
  q2= cmplx(3.- tt2,-3.* tt1)* er/( rt* r2);
  er = q2*( ctx->dataj.t1xj* rx+ ctx->dataj.t1yj* ry+ t1zr* rz);
  ctx->dataj.exk= q1* ctx->dataj.t1xj+ er* rx;
  ctx->dataj.eyk= q1* ctx->dataj.t1yj+ er* ry;
  ctx->dataj.ezk= q1* t1zr+ er* rz;
  er= q2*( ctx->dataj.t2xj* rx+ ctx->dataj.t2yj* ry+ t2zr* rz);
  ctx->dataj.exs= q1* ctx->dataj.t2xj+ er* rx;
  ctx->dataj.eys= q1* ctx->dataj.t2yj+ er* ry;
  ctx->dataj.ezs= q1* t2zr+ er* rz;
  
  if( ctx->dataj.ipgnd == 1)
    return;
  
  if( ctx->gnd.iperf == 1)
  {
    ctx->dataj.exk= -ctx->dataj.exk;
    ctx->dataj.eyk= -ctx->dataj.eyk;
    ctx->dataj.ezk= -ctx->dataj.ezk;
    ctx->dataj.exs= -ctx->dataj.exs;
    ctx->dataj.eys= -ctx->dataj.eys;
    ctx->dataj.ezs= -ctx->dataj.ezs;
    return;
  }
  
  xymag= sqrt( rx* rx+ ry* ry);
  if( xymag <= 1.0e-6)
  {
    px=0.;
    py=0.;
    cth=1.;
    rrv=CPLX_10;
  }
  else
  {
    px= -ry/ xymag;
    py= rx/ xymag;
    cth= rz/ sqrt( xymag* xymag+ rz* rz);
    rrv= csqrt(1.- ctx->gnd.zrati* ctx->gnd.zrati*(1.- cth* cth));
  }
  
  rrh= ctx->gnd.zrati* cth;
  rrh=( rrh- rrv)/( rrh+ rrv);
  rrv= ctx->gnd.zrati* rrv;
  rrv=-( cth- rrv)/( cth+ rrv);
  edp=( ctx->dataj.exk* px+ ctx->dataj.eyk* py)*( rrh- rrv);
  ctx->dataj.exk= ctx->dataj.exk* rrv+ edp* px;
  ctx->dataj.eyk= ctx->dataj.eyk* rrv+ edp* py;
  ctx->dataj.ezk= ctx->dataj.ezk* rrv;
  edp=( ctx->dataj.exs* px+ ctx->dataj.eys* py)*( rrh- rrv);
  ctx->dataj.exs= ctx->dataj.exs* rrv+ edp* px;
  ctx->dataj.eys= ctx->dataj.eys* rrv+ edp* py;
  ctx->dataj.ezs= ctx->dataj.ezs* rrv;
  
  return;
}

/*-----------------------------------------------------------------------*/

