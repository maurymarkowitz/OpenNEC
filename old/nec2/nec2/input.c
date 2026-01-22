/******* Translated to the C language by N. Kyriazis  20 Aug 2003 ******

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

/* fill incident field array for charge discontinuity voltage source */
void qdsrc( int is, complex double v, complex double *e )
{
  int i, jx, j, jp1, ipr, ij, i1;
  double xi, yi, zi, ai, cabi, sabi, salpi, tx, ty, tz;
  complex double curd, etk, ets, etc;

  is--;
  i= data.icon1[is];
  data.icon1[is]=0;
  tbf( is+1,0);
  data.icon1[is]= i;
  dataj.s= data.si[is]*.5;
  curd= CCJ* v/(( log(2.* dataj.s/ data.bi[is])-1.)*( segj.bx[segj.jsno-1]*
		cos( TP* dataj.s)+ segj.cx[segj.jsno-1]* sin( TP* dataj.s))* data.wlam);
  vsorc.vqds[vsorc.nqds]= v;
  vsorc.iqds[vsorc.nqds]= is+1;
  vsorc.nqds++;

  for( jx = 0; jx < segj.jsno; jx++ )
  {
	j= segj.jco[jx]-1;
	jp1 = j+1;
	dataj.s= data.si[j];
	dataj.b= data.bi[j];
	dataj.xj= data.x[j];
	dataj.yj= data.y[j];
	dataj.zj= data.z[j];
	dataj.cabj= data.cab[j];
	dataj.sabj= data.sab[j];
	dataj.salpj= data.salp[j];

	if( dataj.iexk != 0)
	{
	  ipr= data.icon1[j];

	  if (ipr > PCHCON) dataj.ind1=2;
	  else if( ipr < 0 )
	  {
		ipr= -ipr;
		ipr--;
		if( -data.icon1[ipr-1] != jp1 )
		  dataj.ind1=2;
		else
		{
		  xi= fabs( dataj.cabj* data.cab[ipr]+ dataj.sabj*
			  data.sab[ipr]+ dataj.salpj* data.salp[ipr]);
		  if( (xi < 0.999999) || (fabs(data.bi[ipr]/dataj.b-1.) > 1.0e-6) )
			dataj.ind1=2;
		  else
			dataj.ind1=0;
		}
	  }  /* if( ipr < 0 ) */
	  else
		if( ipr == 0 )
		  dataj.ind1=1;
		else /* ipr > 0 */
		{
		  ipr--;
		  if( ipr != j )
		  {
			if( data.icon2[ipr] != jp1)
			  dataj.ind1=2;
			else
			{
			  xi= fabs( dataj.cabj* data.cab[ipr]+ dataj.sabj*
				  data.sab[ipr]+ dataj.salpj* data.salp[ipr]);
			  if( (xi < 0.999999) || (fabs(data.bi[ipr]/dataj.b-1.) > 1.0e-6) )
				dataj.ind1=2;
			  else
				dataj.ind1=0;
			}
		  } /* if( ipr != j ) */
		  else
		  {
			if( dataj.cabj* dataj.cabj+ dataj.sabj* dataj.sabj > 1.0e-8)
			  dataj.ind1=2;
			else
			  dataj.ind1=0;
		  }
		} /* else */

	  ipr= data.icon2[j];
	  if (ipr > PCHCON) dataj.ind2=2;
	  else if( ipr < 0 )
	  {
		ipr = -ipr;
		ipr--;
		if( -data.icon2[ipr] != jp1 )
		  dataj.ind1=2;
		else
		{
		  xi= fabs( dataj.cabj* data.cab[ipr]+ dataj.sabj*
			  data.sab[ipr]+ dataj.salpj* data.salp[ipr]);
		  if( (xi < 0.999999) || (fabs(data.bi[ipr]/dataj.b-1.) > 1.0e-6) )
			dataj.ind1=2;
		  else
			dataj.ind1=0;
		}
	  } /* if( ipr < 0 ) */
	  else
		if( ipr == 0 )
		  dataj.ind2=1;
		else /* ipr > 0 */
		{
		  ipr--;
		  if( ipr != j )
		  {
			if( data.icon1[ipr] != jp1)
			  dataj.ind2=2;
			else
			{
			  xi= fabs( dataj.cabj* data.cab[ipr]+ dataj.sabj*
				  data.sab[ipr]+ dataj.salpj* data.salp[ipr]);
			  if( (xi < 0.999999) || (fabs(data.bi[ipr]/dataj.b-1.) > 1.0e-6) )
				dataj.ind2=2;
			  else
				dataj.ind2=0;
			}
		  } /* if( ipr != j )*/
		  else
		  {
			if( dataj.cabj* dataj.cabj+ dataj.sabj* dataj.sabj > 1.0e-8)
			  dataj.ind1=2;
			else
			  dataj.ind1=0;
		  }
		} /* else */

	} /* if( dataj.iexk != 0) */

	for( i = 0; i < data.n; i++ )
	{
	  ij= i- j;
	  xi= data.x[i];
	  yi= data.y[i];
	  zi= data.z[i];
	  ai= data.bi[i];
	  efld( xi, yi, zi, ai, ij);
	  cabi= data.cab[i];
	  sabi= data.sab[i];
	  salpi= data.salp[i];
	  etk= dataj.exk* cabi+ dataj.eyk* sabi+ dataj.ezk* salpi;
	  ets= dataj.exs* cabi+ dataj.eys* sabi+ dataj.ezs* salpi;
	  etc= dataj.exc* cabi+ dataj.eyc* sabi+ dataj.ezc* salpi;
	  e[i]= e[i]-( etk* segj.ax[jx]+ ets* segj.bx[jx]+ etc* segj.cx[jx])* curd;
	}

	if( data.m != 0)
	{
	  i1= data.n-1;
	  for( i = 0; i < data.m; i++ )
	  {
		xi= data.px[i];
		yi= data.py[i];
		zi= data.pz[i];
		hsfld( xi, yi, zi,0.);
		i1++;
		tx= data.t2x[i];
		ty= data.t2y[i];
		tz= data.t2z[i];
		etk= dataj.exk* tx+ dataj.eyk* ty+ dataj.ezk* tz;
		ets= dataj.exs* tx+ dataj.eys* ty+ dataj.ezs* tz;
		etc= dataj.exc* tx+ dataj.eyc* ty+ dataj.ezc* tz;
		e[i1] += ( etk* segj.ax[jx]+ ets* segj.bx[jx]+
			etc* segj.cx[jx] )* curd* data.psalp[i];
		i1++;
		tx= data.t1x[i];
		ty= data.t1y[i];
		tz= data.t1z[i];
		etk= dataj.exk* tx+ dataj.eyk* ty+ dataj.ezk* tz;
		ets= dataj.exs* tx+ dataj.eys* ty+ dataj.ezs* tz;
		etc= dataj.exc* tx+ dataj.eyc* ty+ dataj.ezc* tz;
		e[i1] += ( etk* segj.ax[jx]+ ets* segj.bx[jx]+
			etc* segj.cx[jx])* curd* data.psalp[i];
	  }

	} /* if( m != 0) */

	if( zload.nload > 0 )
	  e[j] += zload.zarray[j]* curd*(segj.ax[jx]+ segj.cx[jx]);

  } /* for( jx = 0; jx < segj.jsno; jx++ ) */

  return;
}

/*-----------------------------------------------------------------------*/

void readmn( char *gm, int *i1, int *i2, int *i3, int *i4,
	double *f1, double *f2, double *f3,
	double *f4, double *f5, double *f6 )
{
  char line_buf[134];
  int nlin, i, line_idx;
  int nint = 4, nflt = 6;
  int iarr[4] = { 0, 0, 0, 0 };
  double rarr[6] = { 0., 0., 0., 0., 0., 0. };

  /* read a line from input file */
  load_line( line_buf, input_fp );

  /* get line length */
  nlin= (int)strlen( line_buf );

  /* abort if card's mnemonic too short or missing */
  if( nlin < 2 )
  {
	fprintf( output_fp,
		"\n  COMMAND DATA CARD ERROR:"
		"\n  CARD'S MNEMONIC CODE TOO SHORT OR MISSING." );
	stop(-1);
  }

  /* extract card's mnemonic code */
  strncpy( gm, line_buf, 2 );
  gm[2] = '\0';

  /* Exit if "XT" command read (for testing) */
  if( strcmp( gm, "XT" ) == 0 )
  {
	fprintf( stderr,
		"\nnec2c: Exiting after an \"XT\" command in readgm()\n" );
	fprintf( output_fp,
		"\n\n  nec2c: Exiting after an \"XT\" command in readgm()" );
	stop(0);
  }

  /* Return if only mnemonic on card */
  if( nlin == 2 )
  {
	*i1 = *i2 = *i3 = *i4 = 0;
	*f1 = *f2 = *f3 = *f4 = *f5 = *f6 = 0.0;
	return;
  }

  /* read integers from line */
  line_idx = 1;
  for( i = 0; i < nint; i++ )
  {
	/* Find first numerical character */
	while( ((line_buf[++line_idx] <  '0')  ||
		  (line_buf[  line_idx] >  '9')) &&
		(line_buf[  line_idx] != '+')  &&
		(line_buf[  line_idx] != '-') )
	  if( line_buf[line_idx] == '\0' )
	  {
		*i1= iarr[0];
		*i2= iarr[1];
		*i3= iarr[2];
		*i4= iarr[3];
		*f1= rarr[0];
		*f2= rarr[1];
		*f3= rarr[2];
		*f4= rarr[3];
		*f5= rarr[4];
		*f6= rarr[5];
		return;
	  }

	/* read an integer from line */
	iarr[i] = atoi( &line_buf[line_idx] );

	/* traverse numerical field to next ' ' or ',' or '\0' */
	line_idx--;
	while(
		(line_buf[++line_idx] != ' ') &&
		(line_buf[  line_idx] != '	') &&
		(line_buf[  line_idx] != ',') &&
		(line_buf[  line_idx] != '\0') )
	{
	  /* test for non-numerical characters */
	  if( ((line_buf[line_idx] <  '0')  ||
			(line_buf[line_idx] >  '9')) &&
		  (line_buf[line_idx] != '+')  &&
		  (line_buf[line_idx] != '-') )
	  {
		fprintf( output_fp,
			"\n  COMMAND DATA CARD \"%s\" ERROR:"
			"\n  NON-NUMERICAL CHARACTER '%c' IN INTEGER FIELD AT CHAR. %d\n",
			gm, line_buf[line_idx], (line_idx+1) );
		stop(-1);
	  }

	} /* while( (line_buff[++line_idx] ... */

	/* Return on end of line */
	if( line_buf[line_idx] == '\0' )
	{
	  *i1= iarr[0];
	  *i2= iarr[1];
	  *i3= iarr[2];
	  *i4= iarr[3];
	  *f1= rarr[0];
	  *f2= rarr[1];
	  *f3= rarr[2];
	  *f4= rarr[3];
	  *f5= rarr[4];
	  *f6= rarr[5];
	  return;
	}

  } /* for( i = 0; i < nint; i++ ) */

  /* read doubles from line */
  for( i = 0; i < nflt; i++ )
  {
	/* Find first numerical character */
	while( ((line_buf[++line_idx] <  '0')  ||
		  (line_buf[  line_idx] >  '9')) &&
		(line_buf[  line_idx] != '+')  &&
		(line_buf[  line_idx] != '-')  &&
		(line_buf[  line_idx] != '.') )
	  if( line_buf[line_idx] == '\0' )
	  {
		*i1= iarr[0];
		*i2= iarr[1];
		*i3= iarr[2];
		*i4= iarr[3];
		*f1= rarr[0];
		*f2= rarr[1];
		*f3= rarr[2];
		*f4= rarr[3];
		*f5= rarr[4];
		*f6= rarr[5];
		return;
	  }

	/* read a double from line */
	rarr[i] = atof( &line_buf[line_idx] );

	/* traverse numerical field to next ' ' or ',' */
	line_idx--;
	while(
		(line_buf[++line_idx] != ' ') &&
		(line_buf[  line_idx] != '	') &&
		(line_buf[  line_idx] != ',') &&
		(line_buf[  line_idx] != '\0') )
	{
	  /* test for non-numerical characters */
	  if( ((line_buf[line_idx] <  '0')  ||
			(line_buf[line_idx] >  '9')) &&
		  (line_buf[line_idx] != '.')  &&
		  (line_buf[line_idx] != '+')  &&
		  (line_buf[line_idx] != '-')  &&
		  (line_buf[line_idx] != 'E')  &&
		  (line_buf[line_idx] != 'e') )
	  {
		fprintf( output_fp,
			"\n  COMMAND DATA CARD \"%s\" ERROR:"
			"\n  NON-NUMERICAL CHARACTER '%c' IN FLOAT FIELD AT CHAR. %d\n",
			gm, line_buf[line_idx], (line_idx+1) );
		stop(-1);
	  }

	} /* while( (line_buff[++line_idx] ... */

	/* Return on end of line */
	if( line_buf[line_idx] == '\0' )
	{
	  *i1= iarr[0];
	  *i2= iarr[1];
	  *i3= iarr[2];
	  *i4= iarr[3];
	  *f1= rarr[0];
	  *f2= rarr[1];
	  *f3= rarr[2];
	  *f4= rarr[3];
	  *f5= rarr[4];
	  *f6= rarr[5];
	  return;
	}

  } /* for( i = 0; i < nflt; i++ ) */

  *i1= iarr[0];
  *i2= iarr[1];
  *i3= iarr[2];
  *i4= iarr[3];
  *f1= rarr[0];
  *f2= rarr[1];
  *f3= rarr[2];
  *f4= rarr[3];
  *f5= rarr[4];
  *f6= rarr[5];

  return;
}

/*-----------------------------------------------------------------------*/

