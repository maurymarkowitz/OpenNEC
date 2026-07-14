/****************************************************************************
 * matrix.c
 *
 * matrix.c assembles and solves the complex interaction matrix used by NEC
 * to relate currents on wires and patches to applied excitations and fields.
 * It builds block structures for wire-wire, wire-surface, and surface-surface
 * interactions, applies loading modifications, and combines symmetric modes
 * when applicable. Factorization and solves are performed using platform
 * backends (Accelerate, OpenBLAS, Netlib LAPACK, MKL) through LU routines.
 *
 * Major responsibilities include:
 * - fill_interaction_matrix(): Assemble the primary NGF matrix A for the problem, setting up
 *   blocks, handling symmetry (n/p equations), and incorporating segment
 *   kernel choices and loading corrections.
 * - fill_wire_wire_matrix()/fill_wire_patch_matrix()/fill_patch_wire_matrix()/fill_patch_patch_matrix(): Compute interaction submatrices for
 *   wire-wire, wire-surface, surface-wire, and surface-surface terms.
 * - compute_all_basis_funcs_on_seg(): Prepare segment current expansion data used by interaction
 *   calculations.
 * - Factorization and solve: Perform LU factorization (zgetrf) and solve
 *   (zgetrs) on per-mode blocks with careful handling of storage layout and
 *   pivots to ensure numerical stability across backends.
 *
 * These routines operate on `context_t`, drawing geometry, material,
 * ground, and matrix parameters to build and solve the system.
 ****************************************************************************/

#include "internals.h"
#include "matrix.h"
#include "fields.h"
#include "calculations.h"

/* Forward declarations for internal functions */
/* Formerly nec2c: cmss */
static void fill_patch_patch_matrix(context_t *restrict ctx, int j1, int j2, int im1, int im2, complex double *restrict cm, int nrow, int itrp);
/* Formerly nec2c: cmsw */
static void fill_patch_wire_matrix(context_t *restrict ctx, int j1, int j2, int i1, int i2, complex double *restrict cm, complex double *restrict cw, int ncw, int nrow, int itrp);
/* Formerly nec2c: cmws */
static void fill_wire_patch_matrix(context_t *restrict ctx, int j, int i1, int i2, complex double *restrict cm, int nr, complex double *restrict cw, int itrp);
/* Formerly nec2c: cmww */
static void fill_wire_wire_matrix(context_t *restrict ctx, int j, int i1, int i2, complex double *restrict cm, int nr, complex double *cw, int nw, int itrp);
/* Formerly nec2c: qdsrc */
void charge_discontinuity_source(context_t *restrict ctx, int is, complex double v, complex double *restrict e);

#ifdef HAVE_ACCELERATE
#include <Accelerate/Accelerate.h>
#endif

#ifdef HAVE_OPENBLAS
/* Prototypes for Fortran LAPACK routines provided by OpenBLAS */
extern void zgetrf_(int*, int*, double _Complex*, int*, int*, int*);
extern void zgetrs_(char*, int*, int*, double _Complex*, int*, int*, double _Complex*, int*, int*);
#endif

#ifdef HAVE_BLAS
/* Prototypes for reference LAPACK (Netlib), common on Linux via liblapack */
extern void zgetrf_(int*, int*, double _Complex*, int*, int*, int*);
extern void zgetrs_(char*, int*, int*, double _Complex*, int*, int*, double _Complex*, int*, int*);
#endif

#ifdef HAVE_MKL
/* Prototypes for Intel MKL Fortran LAPACK routines */
extern void zgetrf_(int*, int*, double _Complex*, int*, int*, int*);
extern void zgetrs_(char*, int*, int*, double _Complex*, int*, int*, double _Complex*, int*, int*);
#endif

/*-------------------------------------------------------------------*/

/* cmset sets up the complex structure matrix in the array cm */
/* Formerly nec2c: cmset */
int fill_interaction_matrix(context_t *restrict ctx, int nrow, complex double *restrict cm, double rkhx, int iexkx)
{
  int mp2, neq, npeq, it, i, j, i1, i2, in2;
  int im1, im2, ist, ij, ipr, jss, jm1, jm2, jst, k, ka, kk;
  complex double zaj, deter, *scm = NULL;

  mp2 = 2 * ctx->geometry.num_patches_sym;
  npeq = ctx->geometry.num_segs_sym + mp2;
  neq = ctx->geometry.num_segs + 2 * ctx->geometry.num_patches;
  ctx->smat.num_sections = neq / npeq;

  ctx->dataj.k_half_len = rkhx;
  ctx->dataj.use_extended_kernel = iexkx;
  it = ctx->matpar.last_block_rows;

  for( i = 0; i < nrow; i++ )
	for( j = 0; j < it; j++ )
	  cm[i+j*nrow]= CPLX_00;

  i1= 1;
  i2= it;
  in2= i2;

  if( in2 > ctx->geometry.num_segs_sym)
	in2= ctx->geometry.num_segs_sym;

  im1= i1- ctx->geometry.num_segs_sym;
  im2= i2- ctx->geometry.num_segs_sym;

  if( im1 < 1)
	im1=1;

  ist=1;
  if( i1 <= ctx->geometry.num_segs_sym)
	ist= ctx->geometry.num_segs_sym- i1+2;

  /* wire source loop */
  if( ctx->geometry.num_segs != 0)
  {
	for( j = 1; j <= ctx->geometry.num_segs; j++ )
	{
	  if (compute_all_basis_funcs_on_seg(ctx, j) != 0)
	    return -1;
	  for( i = 0; i < ctx->segj.num_junction_segs; i++ )
	  {
		ij = ctx->segj.junction_segs[i];
		ctx->segj.junction_segs[i] = (( ij - 1 ) / ctx->geometry.num_segs_sym) * mp2 + ij;
	  }

	  if( i1 <= in2)
		fill_wire_wire_matrix(ctx, j, i1, in2, cm, nrow, cm, nrow, 1);

	  if( im1 <= im2)
		fill_wire_patch_matrix(ctx, j, im1, im2, &cm[(ist - 1) * nrow], nrow, cm, 1);

	  /* matrix elements modified by loading */
	  if( ctx->zload.num_loads == 0)
		continue;

	  if( j > ctx->geometry.num_segs_sym)
		continue;

	  ipr = j;
	  if( (ipr < 1) || (ipr > it) )
		continue;

	  zaj = ctx->zload.seg_impedance[j-1];

	  for( i = 0; i < ctx->segj.num_junction_segs; i++ )
	  {
		jss = ctx->segj.junction_segs[i];
		cm[(jss - 1) + (ipr - 1) * nrow] -= ( ctx->segj.coeff_const[i] + ctx->segj.coeff_cos[i] ) * zaj;
	  }

	} /* for( j = 1; j <= n; j++ ) */

  } /* if( n != 0) */

  if( ctx->geometry.num_patches != 0)
  {
	/* matrix elements for patch current sources */
	jm1 = 1 - ctx->geometry.num_patches_sym;
	jm2 = 0;
	jst = 1 - mp2;

	for( i = 0; i < ctx->smat.num_sections; i++ )
	{
	  jm1 += ctx->geometry.num_patches_sym;
	  jm2 += ctx->geometry.num_patches_sym;
	  jst += npeq;

	  if( i1 <= in2)
		fill_patch_wire_matrix(ctx, jm1, jm2, i1, in2, &cm[(jst - 1)], cm, 0, nrow, 1);

	  if( im1 <= im2)
		fill_patch_patch_matrix(ctx, jm1, jm2, im1, im2, &cm[(jst - 1) + (ist - 1) * nrow], nrow, 1);
	}

  } /* if( m != 0) */

  if( ctx->matpar.storage_case == 1)
	return 0;

  /* Allocate to scratch memory */
  size_t mreq = (size_t)ctx->geometry.num_segs_2xpatches;
  mreq *= sizeof(complex double);
  mem_alloc(ctx, (void *)&scm, mreq );

  /* combine elements for symmetry modes */
  for( i = 0; i < it; i++ )
  {
	for( j = 0; j < npeq; j++ )
	{
	  for( k = 0; k < ctx->smat.num_sections; k++ )
	  {
		ka= j+ k*npeq;
		scm[k]= cm[ka+i*nrow];
	  }

	  deter= scm[0];

	  for( kk = 1; kk < ctx->smat.num_sections; kk++ )
		deter += scm[kk];

	  cm[j+i*nrow]= deter;

	  for( k = 1; k < ctx->smat.num_sections; k++ )
	  {
		ka= j+ k*npeq;
		deter= scm[0];

		for( kk = 1; kk < ctx->smat.num_sections; kk++ )
		{
		  deter += scm[kk]* ctx->smat.mode_matrix[k+kk*ctx->smat.num_sections];
		  cm[ka+i*nrow]= deter;
		}

	  } /* for( k = 1; k < smat.num_sections; k++ ) */

	} /* for( j = 0; j < npeq; j++ ) */

  } /* for( i = 0; i < it; i++ ) */

  mem_free(ctx, (void *)&scm );

  return 0;
}

/*-----------------------------------------------------------------------*/

/* cmss computes matrix elements for surface-surface interactions. */
/* Formerly nec2c: cmss */
void fill_patch_patch_matrix(context_t *restrict ctx, int j1, int j2, int im1, int im2,
    complex double *restrict cm, int nrow, int itrp )
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

	t1xi = ctx->geometry.patch_t1x[il] * ctx->geometry.patch_normal_z[il];
	t1yi = ctx->geometry.patch_t1y[il] * ctx->geometry.patch_normal_z[il];
	t1zi = ctx->geometry.patch_t1z[il] * ctx->geometry.patch_normal_z[il];
	t2xi = ctx->geometry.patch_t2x[il] * ctx->geometry.patch_normal_z[il];
	t2yi = ctx->geometry.patch_t2y[il] * ctx->geometry.patch_normal_z[il];
	t2zi = ctx->geometry.patch_t2z[il] * ctx->geometry.patch_normal_z[il];
	xi = ctx->geometry.patch_x_center[il];
	yi = ctx->geometry.patch_y_center[il];
	zi = ctx->geometry.patch_z_center[il];

	/* loop over source patches */
	jj1=-2;
	for( j = j1; j <= j2; j++ )
	{
	  jl=j-1;
	  jj1 += 2;
	  jj2 = jj1+1;

	  ctx->dataj.seg_half_len = ctx->geometry.patch_area[jl];
	  ctx->dataj.src_x = ctx->geometry.patch_x_center[jl];
	  ctx->dataj.src_y = ctx->geometry.patch_y_center[jl];
	  ctx->dataj.src_z = ctx->geometry.patch_z_center[jl];
	  ctx->dataj.patch_t1x = ctx->geometry.patch_t1x[jl];
	  ctx->dataj.patch_t1y = ctx->geometry.patch_t1y[jl];
	  ctx->dataj.patch_t1z = ctx->geometry.patch_t1z[jl];
	  ctx->dataj.patch_t2x = ctx->geometry.patch_t2x[jl];
	  ctx->dataj.patch_t2y = ctx->geometry.patch_t2y[jl];
	  ctx->dataj.patch_t2z = ctx->geometry.patch_t2z[jl];

	  h_field_patch(ctx, xi, yi, zi);

	  g11 = -( t2xi * ctx->dataj.e_const_x + t2yi * ctx->dataj.e_const_y + t2zi * ctx->dataj.e_const_z );
	  g12 = -( t2xi * ctx->dataj.e_sin_x + t2yi * ctx->dataj.e_sin_y + t2zi * ctx->dataj.e_sin_z );
	  g21 = -( t1xi * ctx->dataj.e_const_x + t1yi * ctx->dataj.e_const_y + t1zi * ctx->dataj.e_const_z );
	  g22 = -( t1xi * ctx->dataj.e_sin_x + t1yi * ctx->dataj.e_sin_y + t1zi * ctx->dataj.e_sin_z );

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
/* Formerly nec2c: cmsw */
void fill_patch_wire_matrix(context_t *restrict ctx, int j1, int j2, int i1, int i2, complex double *restrict cm,
    complex double *restrict cw, int ncw, int nrow, int itrp )
{
  int jsnox; /* -1 offset to "jsno" for array indexing */
  complex double emel[9];

  jsnox = ctx->segj.num_junction_segs-1;

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
	  xi = ctx->geometry.x_center[i];
	  yi = ctx->geometry.y_center[i];
	  zi = ctx->geometry.z_center[i];
	  cabi = ctx->geometry.dir_cos_x[i];
	  sabi = ctx->geometry.dir_cos_y[i];
	  salpi = ctx->geometry.dir_cos_z[i];
	  ipch=0;

	  if( ctx->geometry.seg_end1_conn[i] >= PCHCON)
	  {
		ipch= ctx->geometry.seg_end1_conn[i]-PCHCON;
		fsign=-1.;
	  }

	  if( ctx->geometry.seg_end2_conn[i] >= PCHCON)
	  {
		ipch= ctx->geometry.seg_end2_conn[i]-PCHCON;
		fsign=1.;
	  }

	  /* source loop */
	  jl = -1;
	  for( j = j1; j <= j2; j++ )
	  {
		jl += 2;
		js = j-1;
		ctx->dataj.patch_t1x = ctx->geometry.patch_t1x[js];
		ctx->dataj.patch_t1y = ctx->geometry.patch_t1y[js];
		ctx->dataj.patch_t1z = ctx->geometry.patch_t1z[js];
		ctx->dataj.patch_t2x = ctx->geometry.patch_t2x[js];
		ctx->dataj.patch_t2y = ctx->geometry.patch_t2y[js];
		ctx->dataj.patch_t2z = ctx->geometry.patch_t2z[js];
		ctx->dataj.src_x = ctx->geometry.patch_x_center[js];
		ctx->dataj.src_y = ctx->geometry.patch_y_center[js];
		ctx->dataj.src_z = ctx->geometry.patch_z_center[js];
		ctx->dataj.seg_half_len = ctx->geometry.patch_area[js];

		/* ground loop */
		for( ip = 1; ip <= ctx->gnd.has_ground; ip++ )
		{
		  ctx->dataj.ground_image_pass= ip;

		  if( ((ipch == j) || (icgo != 0)) && (ip != 2) )
		  {
			if( icgo <= 0 )
			{
			  integrate_patch_at_junction(ctx, xi, yi, zi, cabi, sabi, salpi, emel);

			  pyl= PI* ctx->geometry.half_len[i]* fsign;
			  pxl= sin( pyl);
			  pyl= cos( pyl);
			  ctx->dataj.e_cos_x= emel[8]* fsign;

			  if (compute_all_basis_funcs_on_seg(ctx, i+1) != 0)
			    return;

			  il= i-ncw;
			  if( i < ctx->geometry.num_segs_sym)
				il += (il/ctx->geometry.num_segs_sym)*2*ctx->geometry.num_patches_sym;

			  if( itrp == 0 )
				cw[k+il*nrow] +=
				  ctx->dataj.e_cos_x * ( ctx->segj.coeff_const[jsnox] + ctx->segj.coeff_sine[jsnox] * pxl + ctx->segj.coeff_cos[jsnox] * pyl );
			  else
				cw[il+k*nrow] +=
				  ctx->dataj.e_cos_x * ( ctx->segj.coeff_const[jsnox] + ctx->segj.coeff_sine[jsnox] * pxl + ctx->segj.coeff_cos[jsnox] * pyl );

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

		  e_field_unit_patch_current(ctx, xi, yi, zi);

		  /* normal fill */
		  if( itrp == 0)
		  {
			cm[k+(jl-1)*nrow] += ctx->dataj.e_const_x* cabi+ ctx->dataj.e_const_y* sabi+ ctx->dataj.e_const_z* salpi;
			cm[k+jl*nrow]     += ctx->dataj.e_sin_x* cabi+ ctx->dataj.e_sin_y* sabi+ ctx->dataj.e_sin_z* salpi;
			continue;
		  }

		  /* transposed fill */
		  cm[(jl-1)+k*nrow] += ctx->dataj.e_const_x* cabi+ ctx->dataj.e_const_y* sabi+ ctx->dataj.e_const_z* salpi;
		  cm[jl+k*nrow]     += ctx->dataj.e_sin_x* cabi+ ctx->dataj.e_sin_y* sabi+ ctx->dataj.e_sin_z* salpi;

		} /* for( ip = 1; ip <= gnd.has_ground; ip++ ) */

	  } /* for( j = j1; j <= j2; j++ ) */

	} /* for( i = i1-1; i < i2; i++ ) */

  } /* if( itrp >= 0) */

  return;
}

/*-----------------------------------------------------------------------*/

/* cmws computes matrix elements for wire-surface interactions */
/* Formerly nec2c: cmws */
void fill_wire_patch_matrix(context_t *restrict ctx, int j, int i1, int i2, complex double *restrict cm,
    int nr, complex double *restrict cw, int itrp )
 {
  int ipr, i, ipatch, ik, js=0, ij, jx;
  double xi, yi, zi, tx, ty, tz;
  complex double etk, ets, etc;

  j--;
  ctx->dataj.seg_half_len = ctx->geometry.half_len[j];
  ctx->dataj.seg_radius = ctx->geometry.radius[j];
  ctx->dataj.src_x = ctx->geometry.x_center[j];
  ctx->dataj.src_y = ctx->geometry.y_center[j];
  ctx->dataj.src_z = ctx->geometry.z_center[j];
  ctx->dataj.src_dir_cos_x = ctx->geometry.dir_cos_x[j];
  ctx->dataj.src_dir_cos_y = ctx->geometry.dir_cos_y[j];
  ctx->dataj.src_dir_cos_z = ctx->geometry.dir_cos_z[j];

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
	  xi= ctx->geometry.patch_x_center[js];
	  yi= ctx->geometry.patch_y_center[js];
	  zi= ctx->geometry.patch_z_center[js];
	  h_field_segment(ctx, xi, yi, zi, 0.);

	  if( ik != 0 )
	  {
		tx= ctx->geometry.patch_t2x[js];
		ty= ctx->geometry.patch_t2y[js];
		tz= ctx->geometry.patch_t2z[js];
	  }
	  else
	  {
		tx= ctx->geometry.patch_t1x[js];
		ty= ctx->geometry.patch_t1y[js];
		tz= ctx->geometry.patch_t1z[js];
	  }

	} /* if( (ik != 0) || (ipr == 0) ) */
	else
	{
	  tx= ctx->geometry.patch_t1x[js];
	  ty= ctx->geometry.patch_t1y[js];
	  tz= ctx->geometry.patch_t1z[js];

	} /* if( (ik != 0) || (ipr == 0) ) */

	etk=-( ctx->dataj.e_const_x* tx+ ctx->dataj.e_const_y* ty+ ctx->dataj.e_const_z* tz)* ctx->geometry.patch_normal_z[js];
	ets=-( ctx->dataj.e_sin_x* tx+ ctx->dataj.e_sin_y* ty+ ctx->dataj.e_sin_z* tz)* ctx->geometry.patch_normal_z[js];
	etc=-( ctx->dataj.e_cos_x* tx+ ctx->dataj.e_cos_y* ty+ ctx->dataj.e_cos_z* tz)* ctx->geometry.patch_normal_z[js];

	/* fill matrix elements.  element locations */
	/* determined by connection data. */

	/* normal fill */
	if( itrp == 0)
	{
	  for( ij = 0; ij < ctx->segj.num_junction_segs; ij++ )
	  {
		jx= ctx->segj.junction_segs[ij]-1;
		cm[ipr+jx*nr] += etk* ctx->segj.coeff_const[ij]+ ets* ctx->segj.coeff_sine[ij]+ etc* ctx->segj.coeff_cos[ij];
	  }

	  continue;
	} /* if( itrp == 0) */

	/* transposed fill */
	if( itrp != 2)
	{
	  for( ij = 0; ij < ctx->segj.num_junction_segs; ij++ )
	  {
		jx= ctx->segj.junction_segs[ij]-1;
		cm[jx+ipr*nr] += etk* ctx->segj.coeff_const[ij]+ ets* ctx->segj.coeff_sine[ij]+ etc* ctx->segj.coeff_cos[ij];
	  }

	  continue;
	} /* if( itrp != 2) */

	/* transposed fill - c(ws) and d(ws)prime (=cw) */
	for( ij = 0; ij < ctx->segj.num_junction_segs; ij++ )
	{
	  jx= ctx->segj.junction_segs[ij]-1;
	  if( jx < nr)
		cm[jx+ipr*nr] += etk* ctx->segj.coeff_const[ij]+ ets* ctx->segj.coeff_sine[ij]+ etc* ctx->segj.coeff_cos[ij];
	  else
	  {
		jx -= nr;
		cw[jx+ipr*nr] += etk* ctx->segj.coeff_const[ij]+ ets* ctx->segj.coeff_sine[ij]+ etc* ctx->segj.coeff_cos[ij];
	  }
	} /* for( ij = 0; ij < segj.num_junction_segs; ij++ ) */

  } /* for( i = i1; i <= i2; i++ ) */

  return;
}

/*-----------------------------------------------------------------------*/

/* cmww computes matrix elements for wire-wire interactions */
/* Formerly nec2c: cmww */
void fill_wire_wire_matrix(context_t *restrict ctx, int j, int i1, int i2, complex double *restrict cm,
    int nr, complex double *cw, int nw, int itrp)
 {
  int ipr, iprx, i, ij, jx;
  double xi, yi, zi, ai, cabi, sabi, salpi;
  complex double etk, ets, etc;

  /* set source segment parameters */
  jx = j;
  j--;
  ctx->dataj.seg_half_len = ctx->geometry.half_len[j];
  ctx->dataj.seg_radius = ctx->geometry.radius[j];
  ctx->dataj.src_x = ctx->geometry.x_center[j];
  ctx->dataj.src_y = ctx->geometry.y_center[j];
  ctx->dataj.src_z = ctx->geometry.z_center[j];
  ctx->dataj.src_dir_cos_x = ctx->geometry.dir_cos_x[j];
  ctx->dataj.src_dir_cos_y = ctx->geometry.dir_cos_y[j];
  ctx->dataj.src_dir_cos_z = ctx->geometry.dir_cos_z[j];

  /* decide whether ext. t.w. approx. can be used */
  if( ctx->dataj.use_extended_kernel != 0)
  {
	ipr = ctx->geometry.seg_end1_conn[j];
	if (ipr > PCHCON) ctx->dataj.end1_kernel_type = 0;
	else if( ipr < 0 )
	{
	  ipr= -ipr;
	  iprx= ipr-1;

	  if( -ctx->geometry.seg_end1_conn[iprx] != jx )	ctx->dataj.end1_kernel_type = 2;
	  else
	  {
		xi= fabs( ctx->dataj.src_dir_cos_x* ctx->geometry.dir_cos_x[iprx]+ ctx->dataj.src_dir_cos_y*
			ctx->geometry.dir_cos_y[iprx]+ ctx->dataj.src_dir_cos_z* ctx->geometry.dir_cos_z[iprx]);
		if( (xi < 0.999999) || (fabs(ctx->geometry.radius[iprx]/ctx->dataj.seg_radius-1.) > 1.e-6) )
		  ctx->dataj.end1_kernel_type=2;
		else
		  ctx->dataj.end1_kernel_type=0;

	  } /* if( -data.icon1[iprx] != jx ) */

	} /* if( ipr < 0 ) */
	else
	{
	  iprx = ipr-1;
	  if( ipr == 0 ) ctx->dataj.end1_kernel_type=1;
	  else
	  {
		if( ipr != jx )
		{
		  if( ctx->geometry.seg_end2_conn[iprx] != jx ) ctx->dataj.end1_kernel_type=2;
		  else
		  {
			xi= fabs( ctx->dataj.src_dir_cos_x* ctx->geometry.dir_cos_x[iprx]+ ctx->dataj.src_dir_cos_y*
				ctx->geometry.dir_cos_y[iprx]+ ctx->dataj.src_dir_cos_z* ctx->geometry.dir_cos_z[iprx]);
			if( (xi < 0.999999) || (fabs(ctx->geometry.radius[iprx]/ctx->dataj.seg_radius-1.) > 1.e-6) )
			  ctx->dataj.end1_kernel_type=2;
			else
			  ctx->dataj.end1_kernel_type=0;

		  } /* if( data.icon2[iprx] != jx ) */

		} /* if( ipr != jx ) */
		else
		  if( ctx->dataj.src_dir_cos_x* ctx->dataj.src_dir_cos_x+ ctx->dataj.src_dir_cos_y* ctx->dataj.src_dir_cos_y > 1.e-8)
			ctx->dataj.end1_kernel_type=2;
		  else
			ctx->dataj.end1_kernel_type=0;

	  } /* if( ipr == 0 ) */

	} /* if( ipr < 0 ) */

	ipr = ctx->geometry.seg_end2_conn[j];
	if (ipr > PCHCON) ctx->dataj.end2_kernel_type = 2;
	else if( ipr < 0 )
	{
	  ipr= -ipr;
	  iprx = ipr-1;
	  if( -ctx->geometry.seg_end2_conn[iprx] != jx )
		ctx->dataj.end2_kernel_type=2;
	  else
	  {
		xi= fabs( ctx->dataj.src_dir_cos_x* ctx->geometry.dir_cos_x[iprx]+ ctx->dataj.src_dir_cos_y*
			ctx->geometry.dir_cos_y[iprx]+ ctx->dataj.src_dir_cos_z* ctx->geometry.dir_cos_z[iprx]);
		if( (xi < 0.999999) || (fabs(ctx->geometry.radius[iprx]/ctx->dataj.seg_radius-1.) > 1.e-6) )
		  ctx->dataj.end2_kernel_type=2;
		else
		  ctx->dataj.end2_kernel_type=0;

	  } /* if( -data.icon1[iprx] != jx ) */

	} /* if( ipr < 0 ) */
	else
	{
	  iprx = ipr-1;
	  if( ipr == 0 ) ctx->dataj.end2_kernel_type=1;
	  else
	  {
		if( ipr != jx )
		{
		  if( ctx->geometry.seg_end1_conn[iprx] != jx )
			ctx->dataj.end2_kernel_type=2;
		  else
		  {
			xi= fabs( ctx->dataj.src_dir_cos_x* ctx->geometry.dir_cos_x[iprx]+ ctx->dataj.src_dir_cos_y*
				ctx->geometry.dir_cos_y[iprx]+ ctx->dataj.src_dir_cos_z* ctx->geometry.dir_cos_z[iprx]);
			if( (xi < 0.999999) || (fabs(ctx->geometry.radius[iprx]/ctx->dataj.seg_radius-1.) > 1.e-6) )
			  ctx->dataj.end2_kernel_type=2;
			else
			  ctx->dataj.end2_kernel_type=0;

		  } /* if( data.icon2[iprx] != jx ) */

		} /* if( ipr != jx ) */
		else
		  if( ctx->dataj.src_dir_cos_x* ctx->dataj.src_dir_cos_x+ ctx->dataj.src_dir_cos_y* ctx->dataj.src_dir_cos_y > 1.e-8)
			ctx->dataj.end2_kernel_type=2;
		  else
			ctx->dataj.end2_kernel_type=0;

	  } /* if( ipr == 0 ) */

	} /* if( ipr < 0 ) */

  } /* if( dataj.use_extended_kernel != 0) */

  /* observation loop */
  ipr=-1;
  for( i = i1-1; i < i2; i++ )
  {
	ipr++;
	ij= i-j;
	xi= ctx->geometry.x_center[i];
	yi= ctx->geometry.y_center[i];
	zi= ctx->geometry.z_center[i];
	ai= ctx->geometry.radius[i];
	cabi= ctx->geometry.dir_cos_x[i];
	sabi= ctx->geometry.dir_cos_y[i];
	salpi= ctx->geometry.dir_cos_z[i];

	e_field_segment( ctx, xi, yi, zi, ai, ij);

	etk= ctx->dataj.e_const_x* cabi+ ctx->dataj.e_const_y* sabi+ ctx->dataj.e_const_z* salpi;
	ets= ctx->dataj.e_sin_x* cabi+ ctx->dataj.e_sin_y* sabi+ ctx->dataj.e_sin_z* salpi;
	etc= ctx->dataj.e_cos_x* cabi+ ctx->dataj.e_cos_y* sabi+ ctx->dataj.e_cos_z* salpi;

	/* fill matrix elements. element locations */
	/* determined by connection data. */

	/* normal fill */
	if( itrp == 0)
	{
	  for( ij = 0; ij < ctx->segj.num_junction_segs; ij++ )
	  {
		jx= ctx->segj.junction_segs[ij]-1;
		cm[ipr+jx*nr] += etk* ctx->segj.coeff_const[ij]+ ets* ctx->segj.coeff_sine[ij]+ etc* ctx->segj.coeff_cos[ij];
	  }
	  continue;
	}

	/* transposed fill */
	if( itrp != 2)
	{
	  for( ij = 0; ij < ctx->segj.num_junction_segs; ij++ )
	  {
		jx= ctx->segj.junction_segs[ij]-1;
		complex double elem = etk* ctx->segj.coeff_const[ij]+ ets* ctx->segj.coeff_sine[ij]+ etc* ctx->segj.coeff_cos[ij];
		cm[jx+ipr*nr] += elem;
	  }
	  continue;
	}

	/* trans. fill for c(ww) - test for elements for d(ww)prime.  (=cw) */
	for( ij = 0; ij < ctx->segj.num_junction_segs; ij++ )
	{
	  jx= ctx->segj.junction_segs[ij]-1;
	  if( jx < nr)
		cm[jx+ipr*nr] += etk* ctx->segj.coeff_const[ij]+ ets* ctx->segj.coeff_sine[ij]+ etc* ctx->segj.coeff_cos[ij];
	  else
	  {
		jx -= nr;
		cw[jx*ipr*nw] += etk* ctx->segj.coeff_const[ij]+ ets* ctx->segj.coeff_sine[ij]+ etc* ctx->segj.coeff_cos[ij];
	  }
	} /* for( ij = 0; ij < segj.num_junction_segs; ij++ ) */

  } /* for( i = i1-1; i < i2; i++ ) */

  return;
}

/*-----------------------------------------------------------------------*/

/* etmns fills the array e with the negative of the */
/* electric field incident on the structure. e is the */
/* right hand side of the matrix equation. */
/* Formerly nec2c: etmns */
void fill_excitation_vector(context_t *restrict ctx, double p1, double p2, double p3, double p4,
    double p5, double p6, int ipr, complex double *restrict e )
{
  int i, is, i1, i2=0, neq;
  double cth, sth, cph, sph, cet, set, pxl, pyl, pzl, wx;
  double wy, wz, qx, qy, qz, arg, ds, dsh, rs, r;
  complex double cx, cy, cz, er, et, ezh, erh, rrv=CPLX_00, rrh=CPLX_00, tt1, tt2;

  neq = ctx->geometry.num_segs + 2 * ctx->geometry.num_patches;
  ctx->vsorc.num_qdsrcs_used = 0;

  /* applied field of voltage sources for transmitting case */
  if( (ipr == 0) || (ipr == 5) )
  {
	for( i = 0; i < neq; i++ )
	  e[i] = CPLX_00;

	if( ctx->vsorc.num_vsrcs != 0)
	{
	  for( i = 0; i < ctx->vsorc.num_vsrcs; i++ )
	  {
		is = ctx->vsorc.vsrc_segs[i] - 1;
		e[is] = -ctx->vsorc.vsrc_voltages[i] / ( ctx->geometry.half_len[is] * ctx->geometry.wavelength );
	  }
	}

	if( ctx->vsorc.num_qdsrcs == 0)
	  return;

	for( i = 0; i < ctx->vsorc.num_qdsrcs; i++ )
	{
	  is= ctx->vsorc.qdsrc_segs[i];
	  charge_discontinuity_source( ctx, is, ctx->vsorc.qdsrc_voltages[i], e);
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

	if( ctx->gnd.has_ground != 1)
	{
	  if( ctx->gnd.is_perfect != 1)
	  {
		rrv = csqrt(1. - ctx->gnd.impedance_ratio * ctx->gnd.impedance_ratio * sth * sth);
		rrh = ctx->gnd.impedance_ratio * cth;
		rrh = ( rrh - rrv ) / ( rrh + rrv );
		rrv = ctx->gnd.impedance_ratio * rrv;
		rrv = -( cth - rrv ) / ( cth + rrv );
	  }
	  else
	  {
		rrv = -CPLX_10;
		rrh = -CPLX_10;
	  } /* if( gnd.is_perfect != 1) */

	} /* if( gnd.has_ground != 1) */

	if( ipr == 1)
	{
	  if( ctx->geometry.num_segs != 0)
	  {
		for( i = 0; i < ctx->geometry.num_segs; i++ )
		{
		  arg= -TP*( wx * ctx->geometry.x_center[i] + wy * ctx->geometry.y_center[i] + wz * ctx->geometry.z_center[i] );
		  e[i]=-( pxl * ctx->geometry.dir_cos_x[i] + pyl * ctx->geometry.dir_cos_y[i] + pzl * ctx->geometry.dir_cos_z[i] ) * cmplx( cos( arg ), sin( arg) );
		}

		if( ctx->gnd.has_ground != 1)
		{
		  tt1=( pyl* cph- pxl* sph)*( rrh- rrv);
		  cx= rrv* pxl- tt1* sph;
		  cy= rrv* pyl+ tt1* cph;
		  cz= -rrv* pzl;

		  for( i = 0; i < ctx->geometry.num_segs; i++ )
		  {
			arg= -TP*( wx* ctx->geometry.x_center[i]+ wy* ctx->geometry.y_center[i]- wz* ctx->geometry.z_center[i]);
			e[i]= e[i]-( cx* ctx->geometry.dir_cos_x[i]+ cy* ctx->geometry.dir_cos_y[i]+
				cz* ctx->geometry.dir_cos_z[i])* cmplx(cos( arg), sin( arg));
		  }

		} /* if( gnd.has_ground != 1) */

	  } /* if( data.n != 0) */

	  if( ctx->geometry.num_patches == 0)
		return;

	  i= -1;
	  i1= ctx->geometry.num_segs-2;
	  for( is = 0; is < ctx->geometry.num_patches; is++ )
	  {
		i++;
		i1 += 2;
		i2 = i1+1;
		arg= -TP*( wx* ctx->geometry.patch_x_center[i]+ wy* ctx->geometry.patch_y_center[i]+ wz* ctx->geometry.patch_z_center[i]);
		tt1= cmplx( cos( arg), sin( arg))* ctx->geometry.patch_normal_z[i]* RETA;
		e[i2]=( qx* ctx->geometry.patch_t1x[i]+ qy* ctx->geometry.patch_t1y[i]+ qz* ctx->geometry.patch_t1z[i])* tt1;
		e[i1]=( qx* ctx->geometry.patch_t2x[i]+ qy* ctx->geometry.patch_t2y[i]+ qz* ctx->geometry.patch_t2z[i])* tt1;
	  }

	  if( ctx->gnd.has_ground == 1)
		return;

	  tt1=( qy* cph- qx* sph)*( rrv- rrh);
	  cx=-( rrh* qx- tt1* sph);
	  cy=-( rrh* qy+ tt1* cph);
	  cz= rrh* qz;

	  i= -1;
	  i1= ctx->geometry.num_segs-2;
	  for( is = 0; is < ctx->geometry.num_patches; is++ )
	  {
		i++;
		i1 += 2;
		i2 = i1+1;
		arg= -TP*( wx* ctx->geometry.patch_x_center[i]+ wy* ctx->geometry.patch_y_center[i]- wz* ctx->geometry.patch_z_center[i]);
		tt1= cmplx( cos( arg), sin( arg))* ctx->geometry.patch_normal_z[i]* RETA;
		e[i2]= e[i2]+( cx* ctx->geometry.patch_t1x[i]+ cy* ctx->geometry.patch_t1y[i]+ cz* ctx->geometry.patch_t1z[i])* tt1;
		e[i1]= e[i1]+( cx* ctx->geometry.patch_t2x[i]+ cy* ctx->geometry.patch_t2y[i]+ cz* ctx->geometry.patch_t2z[i])* tt1;
	  }
	  return;

	} /* if( ipr == 1) */

	/* incident plane wave, elliptic polarization. */
	tt1=-(CPLX_01)* p6;
	if( ipr == 3)
	  tt1= -tt1;

	if( ctx->geometry.num_segs != 0)
	{
	  cx= pxl+ tt1* qx;
	  cy= pyl+ tt1* qy;
	  cz= pzl+ tt1* qz;

	  for( i = 0; i < ctx->geometry.num_segs; i++ )
	  {
		arg= -TP*( wx* ctx->geometry.x_center[i]+ wy* ctx->geometry.y_center[i]+ wz* ctx->geometry.z_center[i]);
		e[i]=-( cx* ctx->geometry.dir_cos_x[i]+ cy* ctx->geometry.dir_cos_y[i]+ cz*
			ctx->geometry.dir_cos_z[i])* cmplx( cos( arg), sin( arg));
	  }

	  if( ctx->gnd.has_ground != 1)
	  {
		tt2=( cy* cph- cx* sph)*( rrh- rrv);
		cx= rrv* cx- tt2* sph;
		cy= rrv* cy+ tt2* cph;
		cz= -rrv* cz;

		for( i = 0; i < ctx->geometry.num_segs; i++ )
		{
		  arg= -TP*( wx* ctx->geometry.x_center[i]+ wy* ctx->geometry.y_center[i]- wz* ctx->geometry.z_center[i]);
		  e[i]= e[i]-( cx* ctx->geometry.dir_cos_x[i]+ cy* ctx->geometry.dir_cos_y[i]+
			  cz* ctx->geometry.dir_cos_z[i])* cmplx(cos( arg), sin( arg));
		}

	  } /* if( gnd.has_ground != 1) */

	} /* if( n != 0) */

	if( ctx->geometry.num_patches == 0)
	  return;

	cx= qx- tt1* pxl;
	cy= qy- tt1* pyl;
	cz= qz- tt1* pzl;

	i= -1;
	i1= ctx->geometry.num_segs-2;
	for( is = 0; is < ctx->geometry.num_patches; is++ )
	{
	  i++;
	  i1 += 2;
	  i2 = i1+1;
	  arg= -TP*( wx* ctx->geometry.patch_x_center[i]+ wy* ctx->geometry.patch_y_center[i]+ wz* ctx->geometry.patch_z_center[i]);
	  tt2= cmplx( cos( arg), sin( arg))* ctx->geometry.patch_normal_z[i]* RETA;
	  e[i2]=( cx* ctx->geometry.patch_t1x[i]+ cy* ctx->geometry.patch_t1y[i]+ cz* ctx->geometry.patch_t1z[i])* tt2;
	  e[i1]=( cx* ctx->geometry.patch_t2x[i]+ cy* ctx->geometry.patch_t2y[i]+ cz* ctx->geometry.patch_t2z[i])* tt2;
	}

	if( ctx->gnd.has_ground == 1)
	  return;

	tt1=( cy* cph- cx* sph)*( rrv- rrh);
	cx=-( rrh* cx- tt1* sph);
	cy=-( rrh* cy+ tt1* cph);
	cz= rrh* cz;

	i= -1;
	i1= ctx->geometry.num_segs-2;
	for( is=0; is < ctx->geometry.num_patches; is++ )
	{
	  i++;
	  i1 += 2;
	  i2 = i1+1;
	  arg= -TP*( wx* ctx->geometry.patch_x_center[i]+ wy* ctx->geometry.patch_y_center[i]- wz* ctx->geometry.patch_z_center[i]);
	  tt1= cmplx( cos( arg), sin( arg))* ctx->geometry.patch_normal_z[i]* RETA;
	  e[i2]= e[i2]+( cx* ctx->geometry.patch_t1x[i]+ cy* ctx->geometry.patch_t1y[i]+ cz* ctx->geometry.patch_t1z[i])* tt1;
	  e[i1]= e[i1]+( cx* ctx->geometry.patch_t2x[i]+ cy* ctx->geometry.patch_t2y[i]+ cz* ctx->geometry.patch_t2z[i])* tt1;
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
  i1= ctx->geometry.num_segs-2;
  for( i = 0; i < ctx->geometry.num_segs_and_patches; i++ )
  {
	if( i >= ctx->geometry.num_segs )
	{
	  i1 += 2;
	  i2 = i1+1;
	  pxl= ctx->geometry.patch_x_center[is]- p1;
	  pyl= ctx->geometry.patch_y_center[is]- p2;
	  pzl= ctx->geometry.patch_z_center[is]- p3;
	}
	else
	{
	  pxl= ctx->geometry.x_center[i]- p1;
	  pyl= ctx->geometry.y_center[i]- p2;
	  pzl= ctx->geometry.z_center[i]- p3;
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

	if( i < ctx->geometry.num_segs )
	{
	  tt2= cmplx(1.0,-1.0/( r* TP))/ rs;
	  er= ds* tt1* tt2* cth;
	  et=.5* ds* tt1*((CPLX_01)* TP/ r+ tt2)* sth;
	  ezh= er* cth- et* sth;
	  erh= er* sth+ et* cth;
	  cx= ezh* wx+ erh* qx;
	  cy= ezh* wy+ erh* qy;
	  cz= ezh* wz+ erh* qz;
	  e[i]=-( cx* ctx->geometry.dir_cos_x[i]+ cy* ctx->geometry.dir_cos_y[i]+ cz* ctx->geometry.dir_cos_z[i]);
	}
	else
	{
	  pxl= wy* qz- wz* qy;
	  pyl= wz* qx- wx* qz;
	  pzl= wx* qy- wy* qx;
	  tt2= dsh* tt1* cmplx(1./ r, TP)/ r* sth* ctx->geometry.patch_normal_z[is];
	  cx= tt2* pxl;
	  cy= tt2* pyl;
	  cz= tt2* pzl;
	  e[i2]= cx* ctx->geometry.patch_t1x[is]+ cy* ctx->geometry.patch_t1y[is]+ cz* ctx->geometry.patch_t1z[is];
	  e[i1]= cx* ctx->geometry.patch_t2x[is]+ cy* ctx->geometry.patch_t2y[is]+ cz* ctx->geometry.patch_t2z[is];
	  is++;
	} /* if( i < ctx->geometry.num_segs) */

  } /* for( i = 0; i < ctx->geometry.num_segs_and_patches; i++ ) */

  return;
}

/*-----------------------------------------------------------------------*/

/* subroutine to factor a matrix into a unit lower triangular matrix */
/* and an upper triangular matrix using the gauss-doolittle algorithm */
/* presented on pages 411-416 of a. ralston--a first course in */
/* numerical analysis.  comments below refer to comments in ralstons */
/* text.    (matrix transposed.) */

/* Formerly nec2c: factr */
void factor_matrix(const context_t *restrict ctx, int n, complex double *restrict a, int *restrict ip, int ndim)
{
#if defined(HAVE_ACCELERATE) || defined(HAVE_OPENBLAS) || defined(HAVE_BLAS) || defined(HAVE_MKL)
	/* LAPACK-backed LU factorization using a local np×np buffer to honor layout. */
	int m = n;
	int lda = n; /* local buffer leading dimension */
	int info = 0;

	/* Allocate local buffer for the n x n submatrix and pivot array. */
	complex double *buf = NULL;
	int *ipiv = NULL;
	size_t buf_bytes = (size_t)n * (size_t)n * sizeof(complex double);
	mem_alloc(ctx, (void *)&buf, buf_bytes);
	mem_alloc(ctx, (void *)&ipiv, (size_t)n * sizeof(int));

	/* Copy from big matrix (column-major, leading dimension=ndim) into local buf (lda=n). */
	for (int j = 0; j < n; j++) {
		for (int i = 0; i < n; i++) {
			buf[i + j * lda] = a[i + j * ndim];
		}
	}

	/* Match custom semantics: the original routine "un-transposes" the matrix
		 before factorization by swapping across the diagonal. Do the same here. */
	for (int i = 1; i < n; i++) {
		for (int j = 0; j < i; j++) {
			complex double tmp = buf[i + j * lda];
			buf[i + j * lda] = buf[j + i * lda];
			buf[j + i * lda] = tmp;
		}
	}
	


	/* Factorize local buffer: buf = P*L*U. */
	zgetrf_(&m, &m, (double _Complex *)buf, &lda, ipiv, &info);

	if (info < 0) {
		report(ctx, ONEC_SEV_ERROR, "ZGETRF ERROR: Illegal argument %d", -info);
	} else if (info > 0) {
		report(ctx, ONEC_SEV_WARNING, "ZGETRF WARNING: U(%d,%d) is exactly zero", info, info);
	}

	/* Copy LU factors back into the big matrix block. */
	for (int j = 0; j < n; j++) {
		for (int i = 0; i < n; i++) {
			a[i + j * ndim] = buf[i + j * lda];
		}
	}

	/* Copy pivot indices (1-based) to caller's ip array. */
	for (int i = 0; i < n; i++) {
		ip[i] = ipiv[i];
	}

	/* Free local buffers. */
	mem_free(ctx, (void *)&buf);
	mem_free(ctx, (void *)&ipiv);

#else
  /* Custom implementation */
  int r, rm1, rp1, pj, pr, k, j, jp1, i;
  bool iflg;
  double dmax, elmag;
  complex double arj, *scm = NULL;

  /* Allocate to scratch memory */
  size_t mreq = (size_t)ctx->geometry.num_segs_2xpatches;
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

  iflg=false;
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
	  iflg=true;

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

	if( iflg == true )
	{
	  /* report(ctx, ONEC_SEV_INFO, "PIVOT(%d)= %16.8E", r, dmax); */
	  
	  /* Write PIVOT diagnostic to output file in ORIGINAL format mode */
	  /* if (ctx->output_format == OUTPUT_FORMAT_ORIGINAL && ctx->output_fp != NULL)
	  {
	    fprintf(ctx->output_fp, " PIVOT(%3d)=%16.8E\n", r+1, dmax);
	  } */
	  
	  iflg=false;
	}

  } /* for( r=0; r < n; r++ ) */

	mem_free(ctx, (void *)&scm );
#endif /* HAVE_ACCELERATE || HAVE_OPENBLAS || HAVE_BLAS || HAVE_MKL */

  return;
}

/*-----------------------------------------------------------------------*/

/* factrs, for symmetric structure, transforms submatricies to form */
/* matricies of the symmetric modes and calls routine to factor */
/* matricies.  if no symmetry, the routine is called to factor the */
/* complete matrix. */
/* Formerly nec2c: factrs */
void factor_matrix_symmetric(context_t *restrict ctx, int np, int nrow, complex double *restrict a, int *restrict ip )
{
  int kk, ka;

  ctx->smat.num_sections = nrow/np;
  for( kk = 0; kk < ctx->smat.num_sections; kk++ )
  {
	ka= kk* np;
	factor_matrix(ctx, np, &a[ka], &ip[ka], nrow );
  }
  return;
}

/*-----------------------------------------------------------------------*/

/* fblock sets parameters for out-of-core */
/* solution for the primary matrix (a) */
/* Formerly nec2c: fblock */
int factor_block_matrix(context_t *ctx, int nrow, int ncol, int imax, int ipsym )
{
  int i, j, k, ka, kk;
  double phaz, arg;
  complex double deter;

  if( nrow*ncol <= imax)
  {
	ctx->matpar.block_rows= nrow;
	ctx->matpar.last_block_rows= nrow;
	ctx->matpar.core_used= nrow* ncol;

	if( nrow == ncol)
	{
	  ctx->matpar.storage_case=1;
	  return 0;
	}
	else
	  ctx->matpar.storage_case=2;

  } /* if( nrow*ncol <= imax) */

  ctx->smat.num_sections = ncol/nrow;
  if( ctx->smat.num_sections*nrow != ncol)
  {
	char err_msg[256];
	snprintf(err_msg, sizeof(err_msg),
		"SYMMETRY ERROR - NROW: %d NCOL: %d", nrow, ncol);
	add_error(ctx, &ctx->errors, err_msg, FATAL);
	return -1;
  }

  /* set up smat.mode_matrix matrix for rotational symmetry. */
  if( ipsym <= 0)
  {
	phaz = TP / ctx->smat.num_sections;

	for( i = 1; i < ctx->smat.num_sections; i++ )
	{
	  for( j= i; j < ctx->smat.num_sections; j++ )
	  {
		arg = phaz * (double)i * (double)j;
		ctx->smat.mode_matrix[i + j * ctx->smat.num_sections]= cmplx( cos( arg ), sin( arg) );
		ctx->smat.mode_matrix[j + i * ctx->smat.num_sections]= ctx->smat.mode_matrix[i + j * ctx->smat.num_sections];
	  }
	}
	return 0;

  } /* if( ipsym <= 0) */

  /* set up smat.mode_matrix matrix for plane symmetry */
  kk=1;
  ctx->smat.mode_matrix[0]=CPLX_10;

  k = 2;
  for( ka = 1; k != ctx->smat.num_sections; ka++ )
	k *= 2;

  for( k = 0; k < ka; k++ )
  {
	for( i = 0; i < kk; i++ )
	{
	  for( j = 0; j < kk; j++ )
	  {
		deter= ctx->smat.mode_matrix[i+j*ctx->smat.num_sections];
		ctx->smat.mode_matrix[i+(j+kk)*ctx->smat.num_sections]= deter;
		ctx->smat.mode_matrix[i+kk+(j+kk)*ctx->smat.num_sections]= -deter;
		ctx->smat.mode_matrix[i+kk+j*ctx->smat.num_sections]= deter;
	  }
	}
	kk *= 2;

  } /* for( k = 0; k < ka; k++ ) */

  return 0;
}

/*-----------------------------------------------------------------------*/


/* subroutine to solve the matrix equation lu*x=b where l is a unit */
/* lower triangular matrix and u is an upper triangular matrix both */
/* of which are stored in a.  the rhs vector b is input and the */
/* solution is returned through vector b.   (matrix transposed. */
void solve(const context_t *restrict ctx, int n, complex double *restrict a, int *restrict ip,
		complex double *restrict b, int ndim )
{
#if defined(HAVE_ACCELERATE) || defined(HAVE_OPENBLAS) || defined(HAVE_BLAS) || defined(HAVE_MKL)
	/* LAPACK-backed solve using local buffers for matrix and RHS. */
	int m = n;
	int lda = n; /* local matrix leading dimension */
	int ldb = n; /* local rhs leading dimension */
	int nrhs = 1;
	int info = 0;
	char trans = 'N';

	/* Allocate local buffers */
	complex double *buf = NULL;
	complex double *rhs = NULL;
	int *ipiv = NULL;
	mem_alloc(ctx, (void *)&buf, (size_t)n * (size_t)n * sizeof(complex double));
	mem_alloc(ctx, (void *)&rhs, (size_t)n * sizeof(complex double));
	mem_alloc(ctx, (void *)&ipiv, (size_t)n * sizeof(int));

	/* Copy matrix block and RHS into local buffers */
	for (int j = 0; j < n; j++) {
		for (int i = 0; i < n; i++) {
			buf[i + j * lda] = a[i + j * ndim];
		}
	}
	for (int i = 0; i < n; i++) {
		rhs[i] = b[i];
		ipiv[i] = ip[i];
	}

	/* Solve buf * X = rhs using LU factors present in buf and pivots ipiv */
	zgetrs_(&trans, &m, &nrhs, (double _Complex *)buf, &lda,
					ipiv, (double _Complex *)rhs, &ldb, &info);

	if (info != 0) {
		report(ctx, ONEC_SEV_ERROR, "ZGETRS ERROR: Illegal argument %d", -info);
	}

	/* Copy solution back to b */
	for (int i = 0; i < n; i++) {
		b[i] = rhs[i];
	}

	/* Free local buffers */
	mem_free(ctx, (void *)&buf);
	mem_free(ctx, (void *)&rhs);
	mem_free(ctx, (void *)&ipiv);

#else
  /* Custom implementation */
  int i, ip1, j, k, pia;
  complex double sum, *scm = NULL;

  /* Allocate to scratch memory */
  size_t mreq = (size_t)ctx->geometry.num_segs_2xpatches;
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
#endif /* HAVE_ACCELERATE || HAVE_OPENBLAS || HAVE_BLAS || HAVE_MKL */

  return;
}

/*-----------------------------------------------------------------------*/

/* subroutine solves, for symmetric structures, handles the */
/* transformation of the right hand side vector and solution */
/* of the matrix eq. */
/* Formerly nec2c: solves */
void solve_symmetric(context_t *restrict ctx, complex double *restrict a, int *restrict ip, complex double *restrict b,
    int neq, int nrh, int np, int n, int mp, int m)
{
  int npeq, nrow, ic, i, kk, ia, ib, j, k;
  double fnop, fnorm;
  complex double  sum, *scm = NULL;

  npeq= np+ 2*mp;
  ctx->smat.num_sections = neq/npeq;
  fnop= ctx->smat.num_sections;
  fnorm=1./ fnop;
  nrow= neq;

  /* Allocate to scratch memory */
  size_t mreq = (size_t)ctx->geometry.num_segs_2xpatches;
  mreq *= sizeof(complex double);
  mem_alloc(ctx, (void *)&scm, mreq );

  if( ctx->smat.num_sections != 1)
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

		for( k = 0; k < ctx->smat.num_sections; k++ )
		{
		  if( k != 0 )
		  {
			for( i = 0; i < np; i++ )
			{
			  ia++;
			  j++;
			  b[j+ic*neq]= scm[ia];
			}

			if( k == (ctx->smat.num_sections-1) )
			  continue;

		  } /* if( k != 0 ) */

		  for( i = 0; i < kk; i++ )
		  {
			ib++;
			j++;
			b[j+ic*neq]= scm[ib];
		  }

		} /* for( k = 0; k < smat.num_sections; k++ ) */

	  } /* if( (n != 0) && (m != 0) ) */

	  /* transform matrix eq. rhs vector according to symmetry modes */
	  for( i = 0; i < npeq; i++ )
	  {
		for( k = 0; k < ctx->smat.num_sections; k++ )
		{
		  ia= i+ k* npeq;
		  scm[k]= b[ia+ic*neq];
		}

		sum= scm[0];
		for( k = 1; k < ctx->smat.num_sections; k++ )
		  sum += scm[k];

		b[i+ic*neq]= sum* fnorm;

		for( k = 1; k < ctx->smat.num_sections; k++ )
		{
		  ia= i+ k* npeq;
		  sum= scm[0];

		  for( j = 1; j < ctx->smat.num_sections; j++ )
			sum += scm[j]* conj( ctx->smat.mode_matrix[k+j*ctx->smat.num_sections]);

		  b[ia+ic*neq]= sum* fnorm;
		}

	  } /* for( i = 0; i < npeq; i++ ) */

	} /* for( ic = 0; ic < nrh; ic++ ) */

  } /* if( smat.num_sections != 1) */

  /* solve each mode equation */
  for( kk = 0; kk < ctx->smat.num_sections; kk++ )
  {
     ia= kk* npeq;
     ib= ia;

     for( ic = 0; ic < nrh; ic++ )
       solve(ctx, npeq, &a[ib], &ip[ia], &b[ia+ic*neq], nrow );

   } /* for( kk = 0; kk < smat.num_sections; kk++ ) */

  if( ctx->smat.num_sections == 1)
  {
	mem_free(ctx, (void *)&scm );
	return;
  }

  /* inverse transform the mode solutions */
  for( ic = 0; ic < nrh; ic++ )
  {
	for( i = 0; i < npeq; i++ )
	{
	  for( k = 0; k < ctx->smat.num_sections; k++ )
	  {
		ia= i+ k* npeq;
		scm[k]= b[ia+ic*neq];
	  }

	  sum= scm[0];
	  for( k = 1; k < ctx->smat.num_sections; k++ )
		sum += scm[k];

	  b[i+ic*neq]= sum;
	  for( k = 1; k < ctx->smat.num_sections; k++ )
	  {
		ia= i+ k* npeq;
		sum= scm[0];

		for( j = 1; j < ctx->smat.num_sections; j++ )
		  sum += scm[j]* ctx->smat.mode_matrix[k+j*ctx->smat.num_sections];

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

	for( k = 0; k < ctx->smat.num_sections; k++ )
	{
	  if( k != 0 )
	  {
		for( i = 0; i < np; i++ )
		{
		  ia++;
		  j++;
		  b[ia+ic*neq]= scm[j];
		}

		if( k == ctx->smat.num_sections)
		  continue;

	  } /* if( k != 0 ) */

	  for( i = 0; i < kk; i++ )
	  {
		ib++;
		j++;
		b[ib+ic*neq]= scm[j];
	  }

	} /* for( k = 0; k < smat.num_sections; k++ ) */

  } /* for( ic = 0; ic < nrh; ic++ ) */

  mem_free(ctx, (void *)&scm );

  return;
}

/*-----------------------------------------------------------------------*/
