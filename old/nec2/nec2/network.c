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

#include "nec2c.h"
#include "shared.h"

/*-------------------------------------------------------------------*/

/* subroutine netwk solves for structure currents for a given */
/* excitation including the effect of non-radiating networks if */
/* present. */
void netwk( complex double *cm, int *ip, complex double *einc )
{
  int *ipnt = NULL, *nteqa = NULL, *ntsca = NULL;
  int nteq=0, ntsc=0, irow2=0, j, ndimn;
  int neqt, irow1=0, i, isc1=0;
  size_t mreq;
  double pwr;
  complex double *vsrc = NULL, *rhs = NULL, *cmn = NULL;
  complex double *rhnt = NULL, *rhnx = NULL, ymit, vlt, cux=CPLX_00;

  netcx.pin=0.;
  netcx.pnls=0.;
  neqt= netcx.neq+ netcx.neq2;
  ndimn = j = (2*netcx.nonet + vsorc.nsant);

  /* Allocate network buffers */
  if( netcx.nonet != 0 )
  {
	mreq = (size_t)data.np3m;
	mreq *= sizeof(complex double);
	mem_alloc( (void *)&rhs, mreq );

	mreq = (size_t)j;
	mreq *= sizeof(complex double);
	mem_alloc( (void *)&rhnt, mreq );
	mem_alloc( (void *)&rhnx, mreq );
	mreq *= (size_t)j;
	mem_alloc( (void *)&cmn, mreq );

	mreq = (size_t)j;
	mreq *= sizeof(int);
	mem_alloc( (void *)&ntsca, mreq );
	mem_alloc( (void *)&nteqa, mreq );
	mem_alloc( (void *)&ipnt, mreq );

	mreq = (size_t)vsorc.nsant;
	mreq *= sizeof(complex double);
	mem_alloc( (void *)&vsrc, mreq );
  }
  else
	if( netcx.masym != 0)
	{
	  mreq = (size_t)j;
	  mreq *= sizeof(int);
	  mem_alloc( (void *)&ipnt, mreq );
	}

  if( netcx.ntsol == 0)
  {
	int nseg1;
	/* compute relative matrix asymmetry */
	if( netcx.masym != 0)
	{
	  irow1=0;
	  if( netcx.nonet != 0)
	  {
		for( i = 0; i < netcx.nonet; i++ )
		{
		  nseg1= netcx.iseg1[i];
		  for( isc1 = 0; isc1 < 2; isc1++ )
		  {
			if( irow1 == 0)
			{
			  ipnt[irow1]= nseg1;
			  nseg1= netcx.iseg2[i];
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

			nseg1= netcx.iseg2[i];

		  } /* for( isc1 = 0; isc1 < 2; isc1++ ) */

		} /* for( i = 0; i < netcx.nonet; i++ ) */

	  } /* if( netcx.nonet != 0) */

	  if( vsorc.nsant != 0)
	  {
		for( i = 0; i < vsorc.nsant; i++ )
		{
		  nseg1= vsorc.isant[i];
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

		} /* for( i = 0; i < vsorc.nsant; i++ ) */

	  } /* if( vsorc.nsant != 0) */

	  if( irow1 >= 2)
	  {
		double asmx, asa;
		for( i = 0; i < irow1; i++ )
		{
		  isc1= ipnt[i]-1;
		  asmx= data.si[isc1];

		  for( j = 0; j < neqt; j++ )
			rhs[j] = CPLX_00;

		  rhs[isc1] = CPLX_10;
		  solves( cm, ip, rhs, netcx.neq, 1, data.np, data.n, data.mp, data.m);
		  cabc( rhs);

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
		fprintf( output_fp, "\n\n"
			"   MAXIMUM RELATIVE ASYMMETRY OF THE DRIVING POINT ADMITTANCE\n"
			"   MATRIX IS %10.3E FOR SEGMENTS %d AND %d\n"
			"   RMS RELATIVE ASYMMETRY IS %10.3E",
			asmx, nteq, ntsc, asa );

	  } /* if( irow1 >= 2) */

	} /* if( netcx.masym != 0) */

	/* solution of network equations */
	if( netcx.nonet != 0)
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

	  for( j = 0; j < netcx.nonet; j++ )
	  {
		int jump1, jump2, isc2=0, nseg2;
		double y11r, y11i, y12r, y12i, y22r, y22i;

		nseg1= netcx.iseg1[j];
		nseg2= netcx.iseg2[j];

		if( netcx.ntyp[j] <= 1)
		{
		  y11r= netcx.x11r[j];
		  y11i= netcx.x11i[j];
		  y12r= netcx.x12r[j];
		  y12i= netcx.x12i[j];
		  y22r= netcx.x22r[j];
		  y22i= netcx.x22i[j];
		}
		else
		{
		  y22r= TP* netcx.x11i[j]/ data.wlam;
		  y12r=0.;
		  y12i=1./( netcx.x11r[j]* sin( y22r));
		  y11r= netcx.x12r[j];
		  y11i= -y12i* cos( y22r);
		  y22r= netcx.x22r[j];
		  y22i= y11i+ netcx.x22i[j];
		  y11i= y11i+ netcx.x12i[j];

		  if( netcx.ntyp[j] != 2)
		  {
			y12r= -y12r;
			y12i= -y12i;
		  }

		} /* if( netcx.ntyp[j] <= 1) */

		jump1 = FALSE;
		if( vsorc.nsant != 0)
		{
		  for( i = 0; i < vsorc.nsant; i++ )
			if( nseg1 == vsorc.isant[i])
			{
			  isc1 = i;
			  jump1 = TRUE;
			  break;
			}
		} /* if( vsorc.nsant != 0) */

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
			vsrc[ntsc]= vsorc.vsant[isc1];
			ntsc++;
		  }

		} /* if( ! jump1 ) */

		jump1 = FALSE;
		if( vsorc.nsant != 0)
		{
		  for( i = 0; i < vsorc.nsant; i++ )
		  {
			if( nseg2 == vsorc.isant[i])
			{
			  isc2= i;
			  jump1 = TRUE;
			  break;
			}
		  }

		} /* if( vsorc.nsant != 0) */

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
			vsrc[ntsc]= vsorc.vsant[isc2];
			ntsc++;
		  }

		} /* if( ! jump1 ) */

		/* fill network equation matrix and right hand side vector with */
		/* network short-circuit admittance matrix coefficients. */
		if( isc1 == -1)
		{
		  cmn[irow1+irow1*ndimn] -= cmplx( y11r, y11i)* data.si[nseg1-1];
		  cmn[irow1+irow2*ndimn] -= cmplx( y12r, y12i)* data.si[nseg1-1];
		}
		else
		{
		  rhnx[irow1] += cmplx( y11r, y11i)* vsorc.vsant[isc1]/data.wlam;
		  rhnx[irow2] += cmplx( y12r, y12i)* vsorc.vsant[isc1]/data.wlam;
		}

		if( isc2 == -1)
		{
		  cmn[irow2+irow2*ndimn] -= cmplx( y22r, y22i)* data.si[nseg2-1];
		  cmn[irow2+irow1*ndimn] -= cmplx( y12r, y12i)* data.si[nseg2-1];
		}
		else
		{
		  rhnx[irow1] += cmplx( y12r, y12i)* vsorc.vsant[isc2]/data.wlam;
		  rhnx[irow2] += cmplx( y22r, y22i)* vsorc.vsant[isc2]/data.wlam;
		}

	  } /* for( j = 0; j < netcx.nonet; j++ ) */

	  /* add interaction matrix admittance */
	  /* elements to network equation matrix */
	  for( i = 0; i < nteq; i++ )
	  {
		for( j = 0; j < neqt; j++ )
		  rhs[j] = CPLX_00;

		irow1= nteqa[i]-1;
		rhs[irow1]=CPLX_10;
		solves( cm, ip, rhs, netcx.neq, 1, data.np, data.n, data.mp, data.m);
		cabc( rhs);

		for( j = 0; j < nteq; j++ )
		{
		  irow1= nteqa[j]-1;
		  cmn[i+j*ndimn] += rhs[irow1];
		}

	  } /* for( i = 0; i < nteq; i++ ) */

	  /* factor network equation matrix */
	  factr( nteq, cmn, ipnt, ndimn);

	} /* if( netcx.nonet != 0) */

  } /* if( netcx.ntsol != 0) */

  if( netcx.nonet != 0)
  {
	/* add to network equation right hand side */
	/* the terms due to element interactions */
	for( i = 0; i < neqt; i++ )
	  rhs[i]= einc[i];

	solves( cm, ip, rhs, netcx.neq, 1, data.np, data.n, data.mp, data.m);
	cabc( rhs);

	for( i = 0; i < nteq; i++ )
	{
	  irow1= nteqa[i]-1;
	  rhnt[i]= rhnx[i]+ rhs[irow1];
	}

	/* solve network equations */
	solve( nteq, cmn, ipnt, rhnt, ndimn);

	/* add fields due to network voltages to electric fields */
	/* applied to structure and solve for induced current */
	for( i = 0; i < nteq; i++ )
	{
	  irow1= nteqa[i]-1;
	  einc[irow1] -= rhnt[i];
	}

	solves( cm, ip, einc, netcx.neq, 1, data.np, data.n, data.mp, data.m);
	cabc( einc);

	if( netcx.nprint == 0)
	{
	  fprintf( output_fp, "\n\n\n"
		  "                          "
		  "--------- STRUCTURE EXCITATION DATA AT NETWORK CONNECTION POINTS --------" );

	  fprintf( output_fp, "\n"
		  "  TAG   SEG       VOLTAGE (VOLTS)          CURRENT (AMPS)        "
		  " IMPEDANCE (OHMS)       ADMITTANCE (MHOS)     POWER\n"
		  "  No:   No:     REAL      IMAGINARY     REAL      IMAGINARY    "
		  " REAL      IMAGINARY     REAL      IMAGINARY   (WATTS)" );
	}

	for( i = 0; i < nteq; i++ )
	{
	  irow1= nteqa[i]-1;
	  vlt= rhnt[i]* data.si[irow1]* data.wlam;
	  cux= einc[irow1]* data.wlam;
	  ymit= cux/ vlt;
	  netcx.zped= vlt/ cux;
	  irow2= data.itag[irow1];
	  pwr=.5* creal( vlt* conj( cux));
	  netcx.pnls= netcx.pnls- pwr;

	  if( netcx.nprint == 0)
		fprintf( output_fp, "\n"
			" %4d %5d %11.4E %11.4E %11.4E %11.4E"
			" %11.4E %11.4E %11.4E %11.4E %11.4E",
			irow2, irow1+1, creal(vlt), cimag(vlt), creal(cux), cimag(cux),
			creal(netcx.zped), cimag(netcx.zped), creal(ymit), cimag(ymit), pwr );
	}

	if( ntsc != 0)
	{
	  for( i = 0; i < ntsc; i++ )
	  {
		irow1= ntsca[i]-1;
		vlt= vsrc[i];
		cux= einc[irow1]* data.wlam;
		ymit= cux/ vlt;
		netcx.zped= vlt/ cux;
		irow2= data.itag[irow1];
		pwr=.5* creal( vlt* conj( cux));
		netcx.pnls= netcx.pnls- pwr;

		if( netcx.nprint == 0)
		  fprintf( output_fp, "\n"
			  " %4d %5d %11.4E %11.4E %11.4E %11.4E"
			  " %11.4E %11.4E %11.4E %11.4E %11.4E",
			  irow2, irow1+1, creal(vlt), cimag(vlt), creal(cux), cimag(cux),
			  creal(netcx.zped), cimag(netcx.zped), creal(ymit), cimag(ymit), pwr );

	  } /* for( i = 0; i < ntsc; i++ ) */

	} /* if( ntsc != 0) */

  } /* if( netcx.nonet != 0) */
  else
  {
	/* solve for currents when no networks are present */
	solves( cm, ip, einc, netcx.neq, 1, data.np, data.n, data.mp, data.m);
	cabc( einc);
	ntsc=0;
  }

  if( (vsorc.nsant+vsorc.nvqd) == 0)
  {
	free_ptr( (void *)&ipnt );
	return;
  }

  fprintf( output_fp, "\n\n\n"
	  "                        "
	  "--------- ANTENNA INPUT PARAMETERS ---------" );

  fprintf( output_fp, "\n"
	  "  TAG   SEG       VOLTAGE (VOLTS)         "
	  "CURRENT (AMPS)         IMPEDANCE (OHMS)    "
	  "    ADMITTANCE (MHOS)     POWER\n"
	  "  No:   No:     REAL      IMAGINARY"
	  "     REAL      IMAGINARY     REAL      "
	  "IMAGINARY    REAL       IMAGINARY   (WATTS)" );

  if( vsorc.nsant != 0)
  {
	for( i = 0; i < vsorc.nsant; i++ )
	{
	  isc1= vsorc.isant[i]-1;
	  vlt= vsorc.vsant[i];

	  if( ntsc == 0)
	  {
		cux= einc[isc1]* data.wlam;
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
		cux=(einc[isc1]+ cux)* data.wlam;
		irow1++;

	  } /* if( ntsc == 0) */

	  ymit= cux/ vlt;
	  netcx.zped= vlt/ cux;
	  pwr=.5* creal( vlt* conj( cux));
	  netcx.pin= netcx.pin+ pwr;

	  if( irow1 != 0)
		netcx.pnls= netcx.pnls+ pwr;

	  irow2= data.itag[isc1];
	  fprintf( output_fp, "\n"
		  " %4d %5d %11.4E %11.4E %11.4E %11.4E"
		  " %11.4E %11.4E %11.4E %11.4E %11.4E",
		  irow2, isc1+1, creal(vlt), cimag(vlt), creal(cux), cimag(cux),
		  creal(netcx.zped), cimag(netcx.zped), creal(ymit), cimag(ymit), pwr );

	} /* for( i = 0; i < vsorc.nsant; i++ ) */

  } /* if( vsorc.nsant != 0) */

  if( vsorc.nvqd != 0)
	for( i = 0; i < vsorc.nvqd; i++ )
	{
	  isc1= vsorc.ivqd[i]-1;
	  vlt= vsorc.vqd[i];
	  cux= cmplx( crnt.air[isc1], crnt.aii[isc1]);
	  ymit= cmplx( crnt.bir[isc1], crnt.bii[isc1]);
	  netcx.zped= cmplx( crnt.cir[isc1], crnt.cii[isc1]);
	  pwr= data.si[isc1]* TP*.5;
	  cux=( cux- ymit* sin( pwr)+ netcx.zped* cos( pwr))* data.wlam;
	  ymit= cux/ vlt;
	  netcx.zped= vlt/ cux;
	  pwr=.5* creal( vlt* conj( cux));
	  netcx.pin= netcx.pin+ pwr;
	  irow2= data.itag[isc1];

	  fprintf( output_fp,	"\n"
		  " %4d %5d %11.4E %11.4E %11.4E %11.4E"
		  " %11.4E %11.4E %11.4E %11.4E %11.4E",
		  irow2, isc1+1, creal(vlt), cimag(vlt), creal(cux), cimag(cux),
		  creal(netcx.zped), cimag(netcx.zped), creal(ymit), cimag(ymit), pwr );

	} /* for( i = 0; i < vsorc.nvqd; i++ ) */

  /* Free network buffers */
  free_ptr( (void *)&ipnt );
  free_ptr( (void *)&nteqa );
  free_ptr( (void *)&ntsca );
  free_ptr( (void *)&vsrc );
  free_ptr( (void *)&rhs );
  free_ptr( (void *)&cmn );
  free_ptr( (void *)&rhnt );
  free_ptr( (void *)&rhnx );

  return;
}

/*-----------------------------------------------------------------------*/

