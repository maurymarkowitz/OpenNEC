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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "nec2c.h"
#include "shared.h"

/* signal handler */
static void sig_handler( int signal );

/*-------------------------------------------------------------------*/

int main( int argc, char **argv )
{
  char infile[81] = "", otfile[81] = "";
  char ain[3], line_buf[81];

/* input card mnemonic list */
#define NUM_CMNDS  20
  char *atst[NUM_CMNDS] =
  {
	"FR", "LD", "GN", "EX", "NT", "TL", \
	  "XQ", "GD", "RP", "NX", "PT", "KH", \
	  "NE", "NH", "PQ", "EK", "CP", "PL", \
	  "EN", "WG"
  };

  char *hpol[3] = { "LINEAR", "RIGHT", "LEFT" };
  char *pnet[3] = { "        ", "STRAIGHT", " CROSSED" };

  int *ldtyp, *ldtag, *ldtagf, *ldtagt;
  int ifrtmw, ifrtmp, mpcnt, igo, nfrq;
  int iexk, iptflg, iptflq, iped, iflow, itmp1, iresrv;
  int itmp3, itmp2, itmp4, nthi=0, nphi=0, iptag=0, iptagf=0, iptagt=0;
  int iptaq=0, iptaqf=0, iptaqt=0, nphic=0, inc=0;
  int i, j, itmp5, nthic=0, mhz=0, ifrq=0, isave=0;

  int
	igox,        /* used in place of "igo" in freq loop */
	next_job,    /* start next job (next sructure) flag */
	ain_num,     /* ain mnemonic as a number */
	jmp_iloop,   /* jump to input loop flag  */
	jmp_floop=0; /* jump to freq. loop flag  */
	size_t mreq; /* Size req. for malloc's   */

  double *zlr, *zli, *zlc, *fnorm;
  double *xtemp, *ytemp, *ztemp, *sitemp, *bitemp;
  double rkh, tmp1, delfrq=0., tmp2, tmp3, tmp4, tmp5, tmp6;
  double xpr1=0., xpr2=0., xpr3=0., xpr4=0., xpr5=0.;
  double zpnorm=0., thetis=0., phiss=0., extim;
  double tim1, tim, tim2, etha, fr, fr2, cmag, ph, ethm, ephm, epha;
  complex double eth, eph, curi, ex, ey, ez, epsc, *cm = NULL;

  /* getopt() variables */
  int option;

  /*** signal handler related code ***/
  /* new and old actions for sigaction() */
  struct sigaction sa_new, sa_old;


  /* initialize new actions */
  sa_new.sa_handler = sig_handler;
  sigemptyset( &sa_new.sa_mask );
  sa_new.sa_flags = 0;

  /* register function to handle signals */
  sigaction( SIGINT,  &sa_new, &sa_old );
  sigaction( SIGSEGV, &sa_new, 0 );
  sigaction( SIGFPE,  &sa_new, 0 );
  sigaction( SIGTERM, &sa_new, 0 );
  sigaction( SIGABRT, &sa_new, 0 );

  /*** command line arguments handler ***/
  if( argc == 1 )
  {
	usage();
	exit(-1);
  }

  /* process command line options */
  while( (option = getopt(argc, argv, "i:o:hv") ) != -1 )
  {
	switch( option )
	{
	  case 'i' : /* specify input file name */
		if( strlen(optarg) > 75 )
		  abort_on_error(-1);
		strcpy( infile, optarg );
		break;

	  case 'o' : /* specify output file name */
		if( strlen(optarg) > 75 )
		  abort_on_error(-2);
		strcpy( otfile, optarg );
		break;

	  case 'h' : /* print usage and exit */
		usage();
		exit(0);

	  case 'v' : /* print nec2c version */
		puts( PACKAGE_STRING );
		exit(0);

	  default: /* print usage and exit */
		usage();
		exit(-1);

	} /* end of switch( option ) */

  } /* while( (option = getopt(argc, argv, "i:o:hv") ) != -1 ) */

  /*** open input file ***/
  if( (input_fp = fopen(infile, "r")) == NULL )
  {
	char mesg[88] = "nec2c: ";

	strcat( mesg, infile );
	perror( mesg );
	exit(-1);
  }

  /* make an output file name if not */
  /* specified by user on invocation */
  if( strlen( otfile ) == 0 )
  {
	/* strip file name extension if there is one */
	int idx = 0;
	while( (infile[++idx] != '.') && (infile[idx] != '\0') );
	infile[idx] = '\0';

	/* make the output file name from input file */
	strcpy( otfile, infile );
	strcat( otfile, ".out" ); /* add extension */
  }

  /* open output file */
  if( (output_fp = fopen(otfile, "w")) == NULL )
  {
	char mesg[88] = "nec2c: ";

	strcat( mesg, otfile );
	perror( mesg );
	exit(-1);
  }

  /*** here we had code to read interactively input/output ***/
  /*** file names. this is done non-interactively above.   ***/

  secnds( &extim );

  /* Null local buffer pointers */
  /* type int */
  ldtyp = ldtag = ldtagf = ldtagt = NULL;
  /* type double */
  zlr = zli = zlc = fnorm = NULL;
  xtemp = ytemp = ztemp = sitemp = bitemp = NULL;
  /* type complex double */
  cm = NULL;

  /* Null global pointers */
  Null_Pointers();

  /* Allocate some buffers */
  mreq = sizeof(complex double) * 11 * 10 * 4;
  mem_alloc( (void *)&ggrid.ar1, mreq );
  mreq = sizeof(complex double) * 17 * 5 * 4;
  mem_alloc( (void *)&ggrid.ar2, mreq );
  mreq = sizeof(complex double) * 9 * 8 * 4;
  mem_alloc( (void *)&ggrid.ar3, mreq );

  /* Initialize ground grid parameters for somnec */
  ggrid.nxa[0] = 11;
  ggrid.nxa[1] = 17;
  ggrid.nxa[2] = 9;

  ggrid.nya[0] = 10;
  ggrid.nya[1] = 5;
  ggrid.nya[2] = 8;

  ggrid.dxa[0] = .02;
  ggrid.dxa[1] = .05;
  ggrid.dxa[2] = .1;

  ggrid.dya[0] = .1745329252;
  ggrid.dya[1] = .0872664626;
  ggrid.dya[2] = .1745329252;

  ggrid.xsa[0] = 0.;
  ggrid.xsa[1] = .2;
  ggrid.xsa[2] = .2;

  ggrid.ysa[0] = 0.;
  ggrid.ysa[1] = 0.;
  ggrid.ysa[2] = .3490658504;

  /* l_1: */
  /* main execution loop, exits at various points */
  /* depending on error conditions or end of jobs */
  while( TRUE )
  {
	ifrtmw=0;
	ifrtmp=0;

	/* print the nec2c header to output file */
	fprintf( output_fp,	"\n\n\n"
		"                              "
		" __________________________________________\n"
		"                              "
		"|                                          |\n"
		"                              "
		"|  NUMERICAL ELECTROMAGNETICS CODE (nec2c) |\n"
		"                              "
		"|   Translated to 'C' in Double Precision  |\n"
		"                              "
		"|__________________________________________|\n" );

	/* read a line from input file */
	if( load_line(line_buf, input_fp) == EOF )
	  abort_on_error(-3);

	/* separate card's id mnemonic */
	strncpy( ain, line_buf, 2 );
	ain[2] = '\0';

	/* if its a "cm" or "ce" card start reading comments */
	if( (strcmp(ain, "CM") == 0) ||
		(strcmp(ain, "CE") == 0) )
	{
	  fprintf( output_fp, "\n\n\n"
		  "                               "
		  "---------------- COMMENTS ----------------\n" );

	  /* write comment to output file */
	  fprintf( output_fp,
		  "                              %s\n",
		  &line_buf[2] );

	  /* Keep reading till a non "CM" card */
	  while( strcmp(ain, "CM") == 0 )
	  {
		/* read a line from input file */
		if( load_line(line_buf, input_fp) == EOF )
		  abort_on_error(-3);

		/* separate card's id mnemonic */
		strncpy( ain, line_buf, 2 );
		ain[2] = '\0';

		/* write comment to output file */
		fprintf( output_fp,
			"                              %s\n",
			&line_buf[2] );

	  } /* while( strcmp(ain, "CM") == 0 ) */

	  /* no "ce" card at end of comments */
	  if( strcmp(ain, "CE") != 0 )
	  {
		fprintf( output_fp,
			"\n\n  ERROR: INCORRECT LABEL FOR A COMMENT CARD" );
		abort_on_error(-4);
	  }

	} /* if( strcmp(ain, "CM") == 0 ... */
	else
	  rewind( input_fp );

	/* initializations etc from original fortran code */
	mpcnt=0;
	matpar.imat=0;

	/* set up geometry data in subroutine datagn */
	datagn();
	iflow=1;

	/* Allocate some buffers */
	mreq = (size_t)data.npm;
	mreq *= sizeof(double);
	mem_realloc( (void *)&crnt.air, mreq );
	mem_realloc( (void *)&crnt.aii, mreq );
	mem_realloc( (void *)&crnt.bir, mreq );
	mem_realloc( (void *)&crnt.bii, mreq );
	mem_realloc( (void *)&crnt.cir, mreq );
	mem_realloc( (void *)&crnt.cii, mreq );
	mem_realloc( (void *)&xtemp,  mreq );
	mem_realloc( (void *)&ytemp,  mreq );
	mem_realloc( (void *)&ztemp,  mreq );
	mem_realloc( (void *)&sitemp, mreq );
	mem_realloc( (void *)&bitemp, mreq );

	mreq = (size_t)data.np2m;
	mreq *= sizeof(int);
	mem_realloc( (void *)&save.ip, mreq );

	mreq = (size_t)data.np3m;
	mreq *= sizeof( complex double);
	mem_realloc( (void *)&crnt.cur, mreq );

	/* Matrix parameters */
	if( matpar.imat == 0)
	{
	  netcx.neq= data.n+2*data.m;
	  netcx.neq2=0;
	}

	fprintf( output_fp, "\n\n\n" );

	/* default values for input parameters and flags */
	netcx.npeq= data.np+2*data.mp;
	plot.iplp1=0;
	plot.iplp2=0;
	plot.iplp3=0;
	plot.iplp4=0;
	igo=1;
	nfrq=1;
	rkh=1.;
	iexk=0;
	fpat.ixtyp=0;
	zload.nload=0;
	netcx.nonet=0;
	fpat.near=-1;
	iptflg=-2;
	iptflq=-1;
	gnd.ifar=-1;
	gnd.zrati=CPLX_10;
	iped=0;
	yparm.ncoup=0;
	yparm.icoup=0;
	save.fmhz= CVEL;
	gnd.ksymp=1;
	gnd.nradl=0;
	gnd.iperf=0;

	/* l_14: */

	/* main input section, exits at various points */
	/* depending on error conditions or end of job */
	next_job = FALSE;
	while( ! next_job )
	{
	  jmp_iloop = FALSE;

	  /* main input section - standard read statement - jumps */
	  /* to appropriate section for specific parameter set up */
	  readmn( ain, &itmp1, &itmp2, &itmp3, &itmp4,
		  &tmp1, &tmp2, &tmp3, &tmp4, &tmp5, &tmp6 );

	  /* If its an "XT" card, exit */
	  if( strcmp(ain, "XT" ) == 0 )
	  {
		fprintf( stderr,
			"\nnec2c: Exiting after an \"XT\" command in main()\n" );
		fprintf( output_fp,
			"\n\n  nec2c: Exiting after an \"XT\" command in main()" );
		stop(0);
	  }

	  mpcnt++;
	  fprintf( output_fp,
		  "\n  DATA CARD No: %3d "
		  "%s %3d %5d %5d %5d %12.5E %12.5E %12.5E %12.5E %12.5E %12.5E",
		  mpcnt, ain, itmp1, itmp2, itmp3, itmp4,
		  tmp1, tmp2, tmp3, tmp4, tmp5, tmp6 );

	  /* identify card id mnemonic (except "ce" and "cm") */
	  for( ain_num = 0; ain_num < NUM_CMNDS; ain_num++ )
		if( strncmp( ain, atst[ain_num], 2) == 0 )
		  break;

	  /* take action according to card id mnemonic */
	  switch( ain_num )
	  {
		case 0: /* "fr" card, frequency parameters */

		  ifrq= itmp1;
		  nfrq= itmp2;
		  if( nfrq == 0)
			nfrq=1;
		  save.fmhz= tmp1;
		  delfrq= tmp2;
		  if( iped == 1)
			zpnorm=0.;
		  igo=1;
		  iflow=1;

		  continue; /* continue card input loop */

		case 1: /* "ld" card, loading parameters */
		  {
			int idx;

			if( iflow != 3 )
			{
			  iflow=3;
			  /* Free loading buffers */
			  zload.nload=0;
			  free_ptr( (void *)&ldtyp );
			  free_ptr( (void *)&ldtag );
			  free_ptr( (void *)&ldtagf );
			  free_ptr( (void *)&ldtagt );
			  free_ptr( (void *)&zlr );
			  free_ptr( (void *)&zli );
			  free_ptr( (void *)&zlc );

			  if( igo > 2 )
				igo=2;
			  if( itmp1 == -1 )
				continue; /* continue card input loop */
			}

			/* Reallocate loading buffers */
			zload.nload++;
			mreq = (size_t)zload.nload;
			mreq *= sizeof(int);
			mem_realloc( (void *)&ldtyp,  mreq );
			mem_realloc( (void *)&ldtag,  mreq );
			mem_realloc( (void *)&ldtagf, mreq );
			mem_realloc( (void *)&ldtagt, mreq );
			mreq = (size_t)zload.nload;
			mreq *= sizeof(double);
			mem_realloc( (void *)&zlr, mreq );
			mem_realloc( (void *)&zli, mreq );
			mem_realloc( (void *)&zlc, mreq );

			idx = zload.nload-1;
			ldtyp[idx]= itmp1;
			ldtag[idx]= itmp2;
			if( itmp4 == 0)
			  itmp4= itmp3;
			ldtagf[idx]= itmp3;
			ldtagt[idx]= itmp4;

			if( itmp4 < itmp3 )
			{
			  fprintf( output_fp,
				  "\n\n  DATA FAULT ON LOADING CARD No: %d: ITAG "
				  "STEP1: %d IS GREATER THAN ITAG STEP2: %d",
				  zload.nload, itmp3, itmp4 );
			  stop(-1);
			}

			zlr[idx]= tmp1;
			zli[idx]= tmp2;
			zlc[idx]= tmp3;
		  }

		  continue; /* continue card input loop */

		case 2: /* "gn" card, ground parameters under the antenna */

		  iflow=4;

		  if( igo > 2)
			igo=2;

		  if( itmp1 == -1 )
		  {
			gnd.ksymp=1;
			gnd.nradl=0;
			gnd.iperf=0;
			continue; /* continue card input loop */
		  }

		  gnd.iperf= itmp1;
		  gnd.nradl= itmp2;
		  gnd.ksymp=2;
		  save.epsr= tmp1;
		  save.sig= tmp2;

		  if( gnd.nradl != 0)
		  {
			if( gnd.iperf == 2)
			{
			  fprintf( output_fp,
				  "\n\n  RADIAL WIRE G.S. APPROXIMATION MAY "
				  "NOT BE USED WITH SOMMERFELD GROUND OPTION" );
			  stop(-1);
			}

			save.scrwlt= tmp3;
			save.scrwrt= tmp4;
			continue; /* continue card input loop */
		  }

		  fpat.epsr2= tmp3;
		  fpat.sig2= tmp4;
		  fpat.clt= tmp5;
		  fpat.cht= tmp6;

		  continue; /* continue card input loop */

		case 3: /* "ex" card, excitation parameters */

		  if( iflow != 5)
		  {
			/* Free vsource buffers */
			free_ptr( (void *)&vsorc.ivqd );
			free_ptr( (void *)&vsorc.iqds );
			free_ptr( (void *)&vsorc.vqd );
			free_ptr( (void *)&vsorc.vqds );
			free_ptr( (void *)&vsorc.isant );
			free_ptr( (void *)&vsorc.vsant );

			vsorc.nsant=0;
			vsorc.nvqd=0;
			iped=0;
			iflow=5;
			if( igo > 3)
			  igo=3;
		  }

		  fpat.ixtyp= itmp1;
		  netcx.masym= itmp4/10;
		  if( (itmp1 == 0) || (itmp1 == 5) )
		  {
			netcx.ntsol=0;

			if( fpat.ixtyp == 5)
			{
			  vsorc.nvqd++;
			  mreq = (size_t)vsorc.nvqd;
			  mreq *= sizeof(int);
			  mem_realloc( (void *)&vsorc.ivqd, mreq );
			  mem_realloc( (void *)&vsorc.iqds, mreq );
			  mreq = (size_t)vsorc.nvqd;
			  mreq *= sizeof(complex double);
			  mem_realloc( (void *)&vsorc.vqd, mreq );
			  mem_realloc( (void *)&vsorc.vqds, mreq );

			  {
				int idx = vsorc.nvqd-1;

				vsorc.ivqd[idx]= isegno( itmp2, itmp3);
				vsorc.vqd[idx]= cmplx( tmp1, tmp2);
				if( cabs( vsorc.vqd[idx]) < 1.e-20)
				  vsorc.vqd[idx] = CPLX_10;

				iped= itmp4- netcx.masym*10;
				zpnorm= tmp3;
				if( (iped == 1) && (zpnorm > 0.0) )
				  iped=2;
				continue; /* continue card input loop */
			  }

			} /* if( fpat.ixtyp == 5) */

			vsorc.nsant++;
			mreq = (size_t)vsorc.nsant;
			mreq *= sizeof(int);
			mem_realloc( (void *)&vsorc.isant, mreq );
			mreq = (size_t)vsorc.nsant;
			mreq *= sizeof(complex double);
			mem_realloc( (void *)&vsorc.vsant, mreq );

			{
			  int idx = vsorc.nsant-1;

			  vsorc.isant[idx]= isegno( itmp2, itmp3);
			  vsorc.vsant[idx]= cmplx( tmp1, tmp2);
			  if( cabs( vsorc.vsant[idx]) < 1.e-20)
				vsorc.vsant[idx] = CPLX_10;

			  iped= itmp4- netcx.masym*10;
			  zpnorm= tmp3;
			  if( (iped == 1) && (zpnorm > 0.0) )
				iped=2;
			  continue; /* continue card input loop */
			}

		  } /* if( (itmp1 <= 0) || (itmp1 == 5) ) */

		  nthi= itmp2;
		  nphi= itmp3;
		  xpr1= tmp1;
		  xpr2= tmp2;
		  xpr3= tmp3;
		  xpr4= tmp4;
		  xpr5= tmp5;
		  fpat.xpr6= tmp6;
		  vsorc.nsant=0;
		  vsorc.nvqd=0;
		  thetis= xpr1;
		  phiss= xpr2;

		  continue; /* continue card input loop */

		case 4: case 5: /* "nt" & "tl" cards, network parameters */
		  {
			int idx;

			if( iflow != 6)
			{
			  netcx.nonet=0;
			  netcx.ntsol=0;
			  iflow=6;

			  /* Free network buffers */
			  free_ptr( (void *)&netcx.ntyp );
			  free_ptr( (void *)&netcx.iseg1 );
			  free_ptr( (void *)&netcx.iseg2 );
			  free_ptr( (void *)&netcx.x11r );
			  free_ptr( (void *)&netcx.x11i );
			  free_ptr( (void *)&netcx.x12r );
			  free_ptr( (void *)&netcx.x12i );
			  free_ptr( (void *)&netcx.x22r );
			  free_ptr( (void *)&netcx.x22i );

			  if( igo > 3)
				igo=3;

			  if( itmp2 == -1 )
				continue; /* continue card input loop */
			}

			/* Re-allocate network buffers */
			netcx.nonet++;
			mreq = (size_t)netcx.nonet;
			mreq *= sizeof(int);
			mem_realloc( (void *)&netcx.ntyp, mreq );
			mem_realloc( (void *)&netcx.iseg1, mreq );
			mem_realloc( (void *)&netcx.iseg2, mreq );
			mreq = (size_t)netcx.nonet;
			mreq *= sizeof(double);
			mem_realloc( (void *)&netcx.x11r, mreq );
			mem_realloc( (void *)&netcx.x11i, mreq );
			mem_realloc( (void *)&netcx.x12r, mreq );
			mem_realloc( (void *)&netcx.x12i, mreq );
			mem_realloc( (void *)&netcx.x22r, mreq );
			mem_realloc( (void *)&netcx.x22i, mreq );

			idx = netcx.nonet-1;
			if( ain_num == 4 )
			  netcx.ntyp[idx]=1;
			else
			  netcx.ntyp[idx]=2;

			netcx.iseg1[idx]= isegno( itmp1, itmp2);
			netcx.iseg2[idx]= isegno( itmp3, itmp4);
			netcx.x11r[idx]= tmp1;
			netcx.x11i[idx]= tmp2;
			netcx.x12r[idx]= tmp3;
			netcx.x12i[idx]= tmp4;
			netcx.x22r[idx]= tmp5;
			netcx.x22i[idx]= tmp6;

			if( (netcx.ntyp[idx] == 1) || (tmp1 > 0.) )
			  continue; /* continue card input loop */

			netcx.ntyp[idx]=3;
			netcx.x11r[idx]= -tmp1;

			continue; /* continue card input loop */
		  }

		case 6: /* "xq" execute card - calc. including radiated fields */

		  if( ((iflow == 10) && (itmp1 == 0)) ||
			  ((nfrq  ==  1) && (itmp1 == 0) && (iflow > 7)) )
			continue; /* continue card input loop */

		  if( itmp1 == 0)
		  {
			if( iflow > 7)
			  iflow=11;
			else
			  iflow=7;
		  }
		  else
		  {
			gnd.ifar=0;
			fpat.rfld=0.;
			fpat.ipd=0;
			fpat.iavp=0;
			fpat.inor=0;
			fpat.iax=0;
			fpat.nth=91;
			fpat.nph=1;
			fpat.thets=0.;
			fpat.phis=0.;
			fpat.dth=1.0;
			fpat.dph=0.;

			if( itmp1 == 2)
			  fpat.phis=90.;

			if( itmp1 == 3)
			{
			  fpat.nph=2;
			  fpat.dph=90.;
			}

		  } /* if( itmp1 == 0) */

		  break;

		case 7: /* "gd" card, ground representation */

		  fpat.epsr2= tmp1;
		  fpat.sig2= tmp2;
		  fpat.clt= tmp3;
		  fpat.cht= tmp4;
		  iflow=9;

		  continue; /* continue card input loop */

		case 8: /* "rp" card, standard observation angle parameters */

		  gnd.ifar= itmp1;
		  fpat.nth= itmp2;
		  fpat.nph= itmp3;

		  if( fpat.nth == 0)
			fpat.nth=1;
		  if( fpat.nph == 0)
			fpat.nph=1;

		  fpat.ipd= itmp4/10;
		  fpat.iavp= itmp4- fpat.ipd*10;
		  fpat.inor= fpat.ipd/10;
		  fpat.ipd= fpat.ipd- fpat.inor*10;
		  fpat.iax= fpat.inor/10;
		  fpat.inor= fpat.inor- fpat.iax*10;

		  if( fpat.iax != 0)
			fpat.iax=1;
		  if( fpat.ipd != 0)
			fpat.ipd=1;
		  if( (fpat.nth < 2) || (fpat.nph < 2) || (gnd.ifar == 1) )
			fpat.iavp=0;

		  fpat.thets= tmp1;
		  fpat.phis= tmp2;
		  fpat.dth= tmp3;
		  fpat.dph= tmp4;
		  fpat.rfld= tmp5;
		  fpat.gnor= tmp6;
		  iflow=10;

		  break;

		case 9: /* "nx" card, do next job */
		  next_job = TRUE;
		  continue; /* continue card input loop */

		case 10: /* "pt" card, print control for current */

		  iptflg= itmp1;
		  iptag= itmp2;
		  iptagf= itmp3;
		  iptagt= itmp4;

		  if( (itmp3 == 0) && (iptflg != -1) )
			iptflg=-2;
		  if( itmp4 == 0)
			iptagt= iptagf;

		  continue; /* continue card input loop */

		case 11: /* "kh" card, matrix integration limit */

		  rkh= tmp1;
		  if( igo > 2)
			igo=2;
		  iflow=1;

		  continue; /* continue card input loop */

		case 12: case 13:  /* "ne"/"nh" cards, near field calculation parameters */

		  if( ain_num == 13 )
			fpat.nfeh=1;
		  else
			fpat.nfeh=0;

		  if( (iflow == 8) && (nfrq != 1) )
		  {
			fprintf( output_fp,
				"\n\n  WHEN MULTIPLE FREQUENCIES ARE REQUESTED, "
				"ONLY ONE NEAR FIELD CARD CAN BE USED -"
				"\n  LAST CARD READ WILL BE USED" );
		  }

		  fpat.near= itmp1;
		  fpat.nrx= itmp2;
		  fpat.nry= itmp3;
		  fpat.nrz= itmp4;
		  fpat.xnr= tmp1;
		  fpat.ynr= tmp2;
		  fpat.znr= tmp3;
		  fpat.dxnr= tmp4;
		  fpat.dynr= tmp5;
		  fpat.dznr= tmp6;
		  iflow=8;

		  if( nfrq != 1)
			continue; /* continue card input loop */

		  break;

		case 14: /* "pq" card, write control for charge */

		  iptflq= itmp1;
		  iptaq= itmp2;
		  iptaqf= itmp3;
		  iptaqt= itmp4;

		  if( (itmp3 == 0) && (iptflq != -1) )
			iptflq=-2;
		  if( itmp4 == 0)
			iptaqt= iptaqf;

		  continue; /* continue card input loop */

		case 15: /* "ek" card,  extended thin wire kernel option */

		  iexk=1;
		  if( itmp1 == -1)
			iexk=0;
		  if( igo > 2)
			igo=2;
		  iflow=1;

		  continue; /* continue card input loop */

		case 16: /* "cp" card, maximum coupling between antennas */

		  if( iflow != 2)
		  {
			yparm.ncoup=0;
			free_ptr( (void *)&yparm.nctag );
			free_ptr( (void *)&yparm.ncseg );
			free_ptr( (void *)&yparm.y11a );
			free_ptr( (void *)&yparm.y12a );
		  }

		  yparm.icoup=0;
		  iflow=2;

		  if( itmp2 == 0)
			continue; /* continue card input loop */

		  yparm.ncoup++;
		  mreq = (size_t)yparm.ncoup;
		  mreq *= sizeof(int);
		  mem_realloc( (void *)&yparm.nctag, mreq );
		  mem_realloc( (void *)&yparm.ncseg, mreq );
		  yparm.nctag[yparm.ncoup-1]= itmp1;
		  yparm.ncseg[yparm.ncoup-1]= itmp2;

		  if( itmp4 == 0)
			continue; /* continue card input loop */

		  yparm.ncoup++;
		  mreq = (size_t)yparm.ncoup;
		  mreq *= sizeof(int);
		  mem_realloc( (void *)&yparm.nctag, mreq );
		  mem_realloc( (void *)&yparm.ncseg, mreq );
		  yparm.nctag[yparm.ncoup-1]= itmp3;
		  yparm.ncseg[yparm.ncoup-1]= itmp4;

		  continue; /* continue card input loop */

		case 17: /* "pl" card, plot flags */

		  plot.iplp1= itmp1;
		  plot.iplp2= itmp2;
		  plot.iplp3= itmp3;
		  plot.iplp4= itmp4;

		  if( plot_fp == NULL )
		  {
			char plotfile[81];

			/* Make a plot file name */
			strcpy( plotfile, infile );
			strcat( plotfile, ".plt" );

			/* Open plot file */
			if( (plot_fp = fopen(plotfile, "w")) == NULL )
			{
			  char mesg[88] = "nec2c: ";

			  strcat( mesg, plotfile );
			  perror( mesg );
			  exit(-1);
			}
		  }

		  continue; /* continue card input loop */

		case 19: /* "wg" card, not supported */
		  abort_on_error(-5);

		default:
		  if( ain_num != 18 )
		  {
			fprintf( output_fp,
				"\n\n  FAULTY DATA CARD LABEL AFTER GEOMETRY SECTION" );
			stop(-1);
		  }

		  /******************************************************
		   *** normal exit of nec2c when all jobs complete ok ***
		   ******************************************************/

		  /* time the process */
		  secnds( &tmp1 );
		  tmp1 -= extim;
		  fprintf( output_fp, "\n\n  TOTAL RUN TIME: %d msec", (int)tmp1 );
		  stop(0);

	  } /* switch( ain_num ) */

	  /**************************************
	   *** end of the main input section. ***
	   *** beginning of frequency do loop ***
	   **************************************/

	  /* Allocate to normalization buffer */
	  {
		size_t mreq1, mreq2;

		mreq1 = mreq2 = 0;
		if( iped )
		{
		  mreq1 = (size_t)(4 * nfrq);
		  mreq1 *= sizeof(double);
		}
		if( iptflg >= 2 )
		{
		  mreq2 = (size_t)(nthi * nphi);
		  mreq2 *= sizeof(double);
		}

		if( (mreq1 > 0) || (mreq2 > 0) )
		{
		  if( mreq1 > mreq2 )
			mem_realloc( (void *)&fnorm, mreq1 );
		  else
			mem_realloc( (void *)&fnorm, mreq2 );
		}
	  }

	  /* igox is used in place of "igo" in the   */
	  /* freq loop. below is a special igox case */
	  if( ((ain_num == 6) || (ain_num == 8)) && (igo == 5) )
		igox = 6;
	  else
		igox = igo;

	  switch( igox )
	  {
		case 1: /* label 41 */
		  /* Memory allocation for primary interacton matrix. */
		  iresrv = data.np2m * (data.np + 2 * data.mp);
		  mreq = (size_t)iresrv;
		  mreq *= sizeof(complex double);
		  mem_realloc( (void *)&cm, mreq );

		  /* Memory allocation for symmetry array */
		  smat.nop = netcx.neq/netcx.npeq;
		  mreq = (size_t)(smat.nop * smat.nop);
		  mreq *= sizeof(complex double);
		  mem_realloc( (void *)&smat.ssx, mreq );

		  mhz=1;

		  if( (data.n != 0) && (ifrtmw != 1) )
		  {
			ifrtmw=1;
			for( i = 0; i < data.n; i++ )
			{
			  xtemp[i]= data.x[i];
			  ytemp[i]= data.y[i];
			  ztemp[i]= data.z[i];
			  sitemp[i]= data.si[i];
			  bitemp[i]= data.bi[i];
			}
		  }

		  if( (data.m != 0) && (ifrtmp != 1) )
		  {
			ifrtmp=1;
			for( i = 0; i < data.m; i++ )
			{
			  j = i+data.n;
			  xtemp[j]= data.px[i];
			  ytemp[j]= data.py[i];
			  ztemp[j]= data.pz[i];
			  bitemp[j]= data.pbi[i];
			}
		  }

		  /* irngf is not used (NGF function not implemented) */
		  if( matpar.imat == 0)
			fblock( netcx.npeq, netcx.neq, iresrv, data.ipsym);

		  /* label 42 */
		  /* frequency do loop */
		  do
		  {
			jmp_floop = FALSE;

			if( mhz != 1)
			{
			  if( ifrq == 1)
				save.fmhz *= delfrq;
			  else
				save.fmhz += delfrq;
			}

			fr= save.fmhz/ CVEL;
			data.wlam= CVEL/ save.fmhz;
			fprintf( output_fp, "\n\n\n"
				"                               "
				"--------- FREQUENCY --------\n"
				"                                "
				"FREQUENCY :%11.4E MHz\n"
				"                                "
				"WAVELENGTH:%11.4E Mtr", save.fmhz, data.wlam );

			fprintf( output_fp, "\n\n"
				"                        "
				"APPROXIMATE INTEGRATION EMPLOYED FOR SEGMENTS \n"
				"                        "
				"THAT ARE MORE THAN %.3f WAVELENGTHS APART", rkh );

			if( iexk == 1)
			  fprintf( output_fp, "\n"
				  "                        "
				  "THE EXTENDED THIN WIRE KERNEL WILL BE USED" );

			/* frequency scaling of geometric parameters */
			if( data.n != 0)
			{
			  for( i = 0; i < data.n; i++ )
			  {
				data.x[i]= xtemp[i]* fr;
				data.y[i]= ytemp[i]* fr;
				data.z[i]= ztemp[i]* fr;
				data.si[i]= sitemp[i]* fr;
				data.bi[i]= bitemp[i]* fr;
			  }
			}

			if( data.m != 0)
			{
			  fr2= fr* fr;
			  for( i = 0; i < data.m; i++ )
			  {
				j = i+data.n;
				data.px[i]= xtemp[j]* fr;
				data.py[i]= ytemp[j]* fr;
				data.pz[i]= ztemp[j]* fr;
				data.pbi[i]= bitemp[j]* fr2;
			  }
			}

			//igo = 2;

			/* label 46 */
			case 2: /* structure segment loading */
			fprintf( output_fp, "\n\n\n"
				"                          "
				"------ STRUCTURE IMPEDANCE LOADING ------" );

			if( zload.nload != 0)
			  load( ldtyp, ldtag, ldtagf, ldtagt, zlr, zli, zlc );

			if( zload.nload == 0 )
			  fprintf( output_fp, "\n"
				  "                                 "
				  "THIS STRUCTURE IS NOT LOADED" );

			fprintf( output_fp, "\n\n\n"
				"                            "
				"-------- ANTENNA ENVIRONMENT --------" );

			if( gnd.ksymp != 1)
			{
			  gnd.frati=CPLX_10;

			  if( gnd.iperf != 1)
			  {
				if( save.sig < 0.)
				  save.sig= -save.sig/(59.96*data.wlam);

				epsc= cmplx( save.epsr, -save.sig*data.wlam*59.96);
				gnd.zrati=1./ csqrt( epsc);
				gwav.u= gnd.zrati;
				gwav.u2= gwav.u* gwav.u;

				if( gnd.nradl != 0)
				{
				  gnd.scrwl= save.scrwlt/ data.wlam;
				  gnd.scrwr= save.scrwrt/ data.wlam;
				  gnd.t1= CPLX_01*2367.067/ (double)gnd.nradl;
				  gnd.t2= gnd.scrwr* (double)gnd.nradl;

				  fprintf( output_fp, "\n"
					  "                            "
					  "RADIAL WIRE GROUND SCREEN\n"
					  "                            "
					  "%d WIRES\n"
					  "                            "
					  "WIRE LENGTH: %8.2f METERS\n"
					  "                            "
					  "WIRE RADIUS: %10.3E METERS",
					  gnd.nradl, save.scrwlt, save.scrwrt );

				  fprintf( output_fp, "\n"
					  "                            "
					  "MEDIUM UNDER SCREEN -" );

				} /* if( gnd.nradl != 0) */

				if( gnd.iperf != 2)
				  fprintf( output_fp, "\n"
					  "                            "
					  "FINITE GROUND - REFLECTION COEFFICIENT APPROXIMATION" );
				else
				{
				  somnec( save.epsr, save.sig, save.fmhz );
				  gnd.frati=( epsc-1.)/( epsc+1.);
				  if( cabs(( ggrid.epscf- epsc)/ epsc) >= 1.0e-3 )
				  {
					fprintf( output_fp,
						"\n ERROR IN GROUND PARAMETERS -"
						"\n COMPLEX DIELECTRIC CONSTANT FROM FILE IS: %12.5E%+12.5Ej"
						"\n                                REQUESTED: %12.5E%+12.5Ej",
						creal(ggrid.epscf), cimag(ggrid.epscf),
						creal(epsc), cimag(epsc) );
					stop(-1);
				  }

				  fprintf( output_fp, "\n"
					  "                            "
					  "FINITE GROUND - SOMMERFELD SOLUTION" );

				} /* if( gnd.iperf != 2) */

				fprintf( output_fp, "\n"
					"                            "
					"RELATIVE DIELECTRIC CONST: %.3f\n"
					"                            "
					"CONDUCTIVITY: %10.3E MHOS/METER\n"
					"                            "
					"COMPLEX DIELECTRIC CONSTANT: %11.4E%+11.4Ej",
					save.epsr, save.sig, creal(epsc), cimag(epsc) );

			  } /* if( gnd.iperf != 1) */
			  else
				fprintf( output_fp, "\n"
					"                            "
					"PERFECT GROUND" );

			} /* if( gnd.ksymp != 1) */
			else
			  fprintf( output_fp, "\n"
				  "                            "
				  "FREE SPACE" );

			/* label 50 */
			/* fill and factor primary interaction matrix */
			secnds( &tim1 );
			cmset( netcx.neq, cm, rkh, iexk );
			secnds( &tim2 );
			tim= tim2- tim1;
			factrs( netcx.npeq, netcx.neq, cm, save.ip );
			secnds( &tim1 );
			tim2= tim1- tim2;
			fprintf( output_fp, "\n\n\n"
				"                             "
				"---------- MATRIX TIMING ----------\n"
				"                               "
				"FILL: %d msec  FACTOR: %d msec",
				(int)tim, (int)tim2 );

			//igo=3;
			netcx.ntsol=0;

			/* label 53 */
			case 3: /* excitation set up (right hand side, -e inc.) */

			nthic=1;
			nphic=1;
			inc=1;
			netcx.nprint=0;

			/* l_54 */
			do
			{
			  if( (fpat.ixtyp != 0) && (fpat.ixtyp != 5) )
			  {
				if( (iptflg <= 0) || (fpat.ixtyp == 4) )
				  fprintf( output_fp, "\n\n\n"
					  "                             "
					  "---------- EXCITATION ----------" );

				tmp5= TA* xpr5;
				tmp4= TA* xpr4;

				if( fpat.ixtyp == 4)
				{
				  tmp1= xpr1/ data.wlam;
				  tmp2= xpr2/ data.wlam;
				  tmp3= xpr3/ data.wlam;
				  tmp6= fpat.xpr6/( data.wlam* data.wlam);

				  fprintf( output_fp, "\n"
					  "                                  "
					  "    CURRENT SOURCE\n"
					  "                     -- POSITION (METERS) -- "
					  "      ORIENTATION (DEG)\n"
					  "                     X          Y          Z "
					  "      ALPHA        BETA   DIPOLE MOMENT\n"
					  "               %10.5f %10.5f %10.5f "
					  " %7.2f     %7.2f    %8.3f",
					  xpr1, xpr2, xpr3, xpr4, xpr5, fpat.xpr6 );
				}
				else
				{
				  tmp1= TA* xpr1;
				  tmp2= TA* xpr2;
				  tmp3= TA* xpr3;
				  tmp6= fpat.xpr6;

				  if( iptflg <= 0)
					fprintf( output_fp,
						"\n  PLANE WAVE - THETA: %7.2f deg, PHI: %7.2f deg,"
						" ETA=%7.2f DEG, TYPE - %s  AXIAL RATIO: %6.3f",
						xpr1, xpr2, xpr3, hpol[fpat.ixtyp-1], fpat.xpr6 );

				} /* if( fpat.ixtyp == 4) */

			  } /* if( (fpat.ixtyp  != 0) && (fpat.ixtyp <= 4) ) */

			  /* fills e field right-hand matrix */
			  etmns( tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, fpat.ixtyp, crnt.cur);

			  /* matrix solving  (netwk calls solves) */
			  if( (netcx.nonet != 0) && (inc <= 1) )
			  {
				fprintf( output_fp, "\n\n\n"
					"                                            "
					"---------- NETWORK DATA ----------" );

				itmp3=0;
				itmp1= netcx.ntyp[0];

				for( i = 0; i < 2; i++ )
				{
				  if( itmp1 == 3)
					itmp1=2;

				  if( itmp1 == 2)
					fprintf( output_fp, "\n"
						"  -- FROM -  --- TO --      TRANSMISSION LINE       "
						" --------- SHUNT ADMITTANCES (MHOS) ---------   LINE\n"
						"  TAG   SEG  TAG   SEG    IMPEDANCE      LENGTH    "
						" ----- END ONE -----      ----- END TWO -----   TYPE\n"
						"  No:   No:  No:   No:         OHMS      METERS     "
						" REAL      IMAGINARY      REAL      IMAGINARY" );
				  else
					if( itmp1 == 1)
					  fprintf( output_fp, "\n"
						  "  -- FROM -  --- TO --            --------"
						  " ADMITTANCE MATRIX ELEMENTS (MHOS) ---------\n"
						  "  TAG   SEG  TAG   SEG   ----- (ONE,ONE) ------  "
						  " ----- (ONE,TWO) -----   ----- (TWO,TWO) -------\n"
						  "  No:   No:  No:   No:      REAL      IMAGINARY     "
						  " REAL     IMAGINARY       REAL      IMAGINARY" );

				  for( j = 0; j < netcx.nonet; j++)
				  {
					itmp2= netcx.ntyp[j];

					if( (itmp2/itmp1) != 1 )
					  itmp3 = itmp2;
					else
					{
					  int idx4, idx5;

					  itmp4= netcx.iseg1[j];
					  itmp5= netcx.iseg2[j];
					  idx4 = itmp4-1;
					  idx5 = itmp5-1;

					  if( (itmp2 >= 2) && (netcx.x11i[j] <= 0.) )
					  {
						double xx, yy, zz;

						xx = data.x[idx5]- data.x[idx4];
						yy = data.y[idx5]- data.y[idx4];
						zz = data.z[idx5]- data.z[idx4];
						netcx.x11i[j]= data.wlam* sqrt( xx*xx + yy*yy + zz*zz );
					  }

					  fprintf( output_fp, "\n"
						  " %4d %5d %4d %5d  %11.4E %11.4E  "
						  "%11.4E %11.4E  %11.4E %11.4E %s",
						  data.itag[idx4], itmp4, data.itag[idx5], itmp5,
						  netcx.x11r[j], netcx.x11i[j], netcx.x12r[j], netcx.x12i[j],
						  netcx.x22r[j], netcx.x22i[j], pnet[itmp2-1] );

					} /* if(( itmp2/ itmp1) == 1) */

				  } /* for( j = 0; j < netcx.nonet; j++) */

				  if( itmp3 == 0)
					break;

				  itmp1= itmp3;

				} /* for( j = 0; j < netcx.nonet; j++) */

			  } /* if( (netcx.nonet != 0) && (inc <= 1) ) */

			  if( (inc > 1) && (iptflg > 0) )
				netcx.nprint=1;

			  netwk( cm, save.ip, crnt.cur );
			  netcx.ntsol=1;

			  if( iped != 0)
			  {
				itmp1= 4*( mhz-1);

				fnorm[itmp1  ]= creal( netcx.zped);
				fnorm[itmp1+1]= cimag( netcx.zped);
				fnorm[itmp1+2]= cabs( netcx.zped);
				fnorm[itmp1+3]= cang( netcx.zped);

				if( iped != 2 )
				{
				  if( fnorm[itmp1+2] > zpnorm)
					zpnorm= fnorm[itmp1+2];
				}

			  } /* if( iped != 0) */

			  /* printing structure currents */
			  if( data.n != 0)
			  {
				if( iptflg != -1)
				{
				  if( iptflg <= 0)
				  {
					fprintf( output_fp, "\n\n\n"
						"                           "
						"-------- CURRENTS AND LOCATION --------\n"
						"                                  "
						"DISTANCES IN WAVELENGTHS" );

					fprintf( output_fp,	"\n\n"
						"   SEG  TAG    COORDINATES OF SEGM CENTER     SEGM"
						"    ------------- CURRENT (AMPS) -------------\n"
						"   No:  No:       X         Y         Z      LENGTH"
						"     REAL      IMAGINARY    MAGN        PHASE" );
				  }
				  else
				  {
					if( (iptflg != 3) && (inc <= 1) )
					  fprintf( output_fp, "\n\n\n"
						  "                        "
						  "-------- RECEIVING PATTERN PARAMETERS --------\n"
						  "                        "
						  "         ETA: %7.2f DEGREES\n"
						  "                        "
						  "         TYPE: %s\n"
						  "                        "
						  "         AXIAL RATIO: %6.3f\n\n"
						  "                        "
						  "THETA     PHI      ----- CURRENT ----    SEG\n"
						  "                        "
						  "(DEG)    (DEG)     MAGNITUDE    PHASE    No:",
						  xpr3, hpol[fpat.ixtyp-1], fpat.xpr6 );

				  } /* if( iptflg <= 0) */

				} /* if( iptflg != -1) */

				fpat.ploss=0.;
				itmp1=0;

				for( i = 0; i < data.n; i++ )
				{
				  curi= crnt.cur[i]* data.wlam;
				  cmag= cabs( curi);
				  ph= cang( curi);

				  if( (zload.nload != 0) && (fabs(creal(zload.zarray[i])) >= 1.e-20) )
					fpat.ploss += 0.5* cmag* cmag* creal( zload.zarray[i])* data.si[i];

				  if( iptflg == -1 )
					continue;

				  if( iptflg >= 0 )
				  {
					if( (iptag != 0) && (data.itag[i] != iptag) )
					  continue;

					itmp1++;
					if( (itmp1 < iptagf) || (itmp1 > iptagt) )
					  continue;

					if( iptflg != 0)
					{
					  if( iptflg >= 2 )
					  {
						fnorm[inc-1]= cmag;
						isave= (i+1);
					  }

					  if( iptflg != 3)
					  {
						fprintf( output_fp, "\n"
							"                      "
							"%7.2f  %7.2f   %11.4E  %7.2f  %5d",
							xpr1, xpr2, cmag, ph, i+1 );
						continue;
					  }

					} /* if( iptflg != 0) */
					else
					  fprintf( output_fp, "\n"
						  " %5d %4d %9.4f %9.4f %9.4f %9.5f"
						  " %11.4E %11.4E %11.4E %8.3f",
						  i+1, data.itag[i], data.x[i], data.y[i], data.z[i],
						  data.si[i], creal(curi), cimag(curi), cmag, ph );

				  } /* if( iptflg >= 0 ) */
				  else
				  {
					fprintf( output_fp, "\n"
						" %5d %4d %9.4f %9.4f %9.4f %9.5f"
						" %11.4E %11.4E %11.4E %8.3f",
						i+1, data.itag[i], data.x[i], data.y[i], data.z[i],
						data.si[i], creal(curi), cimag(curi), cmag, ph );

					if( plot.iplp1 != 1 )
					  continue;

					if( plot.iplp2 == 1)
					  fprintf( plot_fp, "%12.4E %12.4E\n", creal(curi), cimag(curi) );
					else
					  if( plot.iplp2 == 2)
						fprintf( plot_fp, "%12.4E %12.4E\n", cmag, ph );
				  }

				} /* for( i = 0; i < n; i++ ) */

				if( iptflq != -1)
				{
				  fprintf( output_fp, "\n\n\n"
					  "                                  "
					  "------ CHARGE DENSITIES ------\n"
					  "                                  "
					  "   DISTANCES IN WAVELENGTHS\n\n"
					  "   SEG   TAG    COORDINATES OF SEG CENTER     SEG        "
					  "  CHARGE DENSITY (COULOMBS/METER)\n"
					  "   No:   No:     X         Y         Z       LENGTH   "
					  "  REAL      IMAGINARY     MAGN       PHASE" );

				  itmp1 = 0;
				  fr = 1.e-6/save.fmhz;

				  for( i = 0; i < data.n; i++ )
				  {
					if( iptflq != -2 )
					{
					  if( (iptaq != 0) && (data.itag[i] != iptaq) )
						continue;

					  itmp1++;
					  if( (itmp1 < iptaqf) || (itmp1 > iptaqt) )
						continue;

					} /* if( iptflq == -2) */

					curi= fr* cmplx(- crnt.bii[i], crnt.bir[i]);
					cmag= cabs( curi);
					ph= cang( curi);

					fprintf( output_fp, "\n"
						" %5d %4d %9.4f %9.4f %9.4f %9.5f"
						" %11.4E %11.4E %11.4E %8.3f",
						i+1, data.itag[i], data.x[i], data.y[i], data.z[i],
						data.si[i], creal(curi), cimag(curi), cmag, ph );

				  } /* for( i = 0; i < n; i++ ) */

				} /* if( iptflq != -1) */

			  } /* if( n != 0) */

			  if( data.m != 0)
			  {
				fprintf( output_fp, "\n\n\n"
					"                                      "
					" --------- SURFACE PATCH CURRENTS ---------\n"
					"                                                "
					" DISTANCE IN WAVELENGTHS\n"
					"                                                "
					" CURRENT IN AMPS/METER\n\n"
					"                                 ---------"
					" SURFACE COMPONENTS --------   "
					" ---------------- RECTANGULAR COMPONENTS ----------------\n"
					"  PCH   --- PATCH CENTER ---     TANGENT VECTOR 1    "
					" TANGENT VECTOR 2    ------- X ------    ------- Y ------   "
					" ------- Z ------\n  No:    X       Y       Z       MAG.    "
					"   PHASE     MAG.       PHASE    REAL   IMAGINARY    REAL  "
					" IMAGINARY    REAL   IMAGINARY" );

				j= data.n-3;
				itmp1= -1;

				for( i = 0; i <data. m; i++ )
				{
				  j += 3;
				  itmp1++;
				  ex= crnt.cur[j];
				  ey= crnt.cur[j+1];
				  ez= crnt.cur[j+2];
				  eth= ex* data.t1x[itmp1]+ ey* data.t1y[itmp1]+ ez* data.t1z[itmp1];
				  eph= ex* data.t2x[itmp1]+ ey* data.t2y[itmp1]+ ez* data.t2z[itmp1];
				  ethm= cabs( eth);
				  etha= cang( eth);
				  ephm= cabs( eph);
				  epha= cang( eph);

				  fprintf( output_fp, "\n"
					  " %4d %7.3f %7.3f %7.3f %11.4E %8.2f %11.4E %8.2f"
					  " %9.2E %9.2E %9.2E %9.2E %9.2E %9.2E",
					  i+1, data.px[itmp1], data.py[itmp1], data.pz[itmp1],
					  ethm, etha, ephm, epha, creal(ex), cimag(ex),
					  creal(ey), cimag(ey), creal(ez), cimag(ez) );

				  if( plot.iplp1 != 1)
					continue;

				  if( plot.iplp3 == 1)
					fprintf( plot_fp, "%12.4E %12.4E\n", creal(ex), cimag(ex) );
				  if( plot.iplp3 == 2)
					fprintf( plot_fp, "%12.4E %12.4E\n", creal(ey), cimag(ey) );
				  if( plot.iplp3 == 3)
					fprintf( plot_fp, "%12.4E %12.4E\n", creal(ez), cimag(ez) );
				  if( plot.iplp3 == 4)
					fprintf(
						plot_fp, "%12.4E %12.4E %12.4E %12.4E %12.4E %12.4E\n",
						creal(ex),cimag(ex),creal(ey),
						cimag(ey),creal(ez),cimag(ez) );

				} /* for( i=0; i<m; i++ ) */

			  } /* if( m != 0) */

			  if( (fpat.ixtyp == 0) || (fpat.ixtyp == 5) )
			  {
				tmp1= netcx.pin- netcx.pnls- fpat.ploss;
				tmp2= 100.* tmp1/ netcx.pin;

				fprintf( output_fp, "\n\n\n"
					"                               "
					"---------- POWER BUDGET ---------\n"
					"                               "
					"INPUT POWER   = %11.4E Watts\n"
					"                               "
					"RADIATED POWER= %11.4E Watts\n"
					"                               "
					"STRUCTURE LOSS= %11.4E Watts\n"
					"                               "
					"NETWORK LOSS  = %11.4E Watts\n"
					"                               "
					"EFFICIENCY    = %7.2f Percent",
					netcx.pin, tmp1, fpat.ploss, netcx.pnls, tmp2 );

			  } /* if( (fpat.ixtyp == 0) || (fpat.ixtyp == 5) ) */

			  igo = 4;

			  if( yparm.ncoup > 0)
				couple( crnt.cur, data.wlam );

			  if( iflow == 7)
			  {
				if( (fpat.ixtyp > 0) && (fpat.ixtyp < 4) )
				{
				  nthic++;
				  inc++;
				  xpr1 += xpr4;

				  if( nthic <= nthi )
					continue; /* continue excitation loop */

				  nthic=1;
				  xpr1= thetis;
				  xpr2= xpr2+ xpr5;
				  nphic++;

				  if( nphic <= nphi )
					continue; /* continue excitation loop */

				  break;

				} /* if( (fpat.ixtyp >= 1) && (fpat.ixtyp <= 3) ) */

				if( nfrq != 1)
				{
				  jmp_floop = TRUE;
				  break; /* continue the freq loop */
				}

				fprintf( output_fp, "\n\n\n" );
				jmp_iloop = TRUE;

				break; /* continue card input loop */

			  } /*if( iflow == 7) */


			  case 4: /* label_71 */
			  igo = 5;

			  /* label_72 */
			  case 5: /* near field calculation */

			  if( fpat.near != -1)
			  {
				nfpat();

				if( mhz == nfrq)
				  fpat.near=-1;

				if( nfrq == 1)
				{
				  fprintf( output_fp, "\n\n\n" );
				  jmp_iloop = TRUE;
				  break; /* continue card input loop */
				}

			  } /* if( fpat.near != -1) */


			  /* label_78 */
			  case 6: /* standard far field calculation */

			  if( gnd.ifar != -1)
			  {
				fpat.pinr= netcx.pin;
				fpat.pnlr= netcx.pnls;
				rdpat();
			  }

			  if( (fpat.ixtyp == 0) || (fpat.ixtyp >= 4) )
			  {
				if( mhz == nfrq )
				  gnd.ifar=-1;

				if( nfrq != 1)
				{
				  jmp_floop = TRUE;
				  break;
				}

				fprintf( output_fp, "\n\n\n" );
				jmp_iloop = TRUE;
				break;

			  } /* if( (fpat.ixtyp == 0) || (fpat.ixtyp >= 4) ) */

			  nthic++;
			  inc++;
			  xpr1 += xpr4;

			  if( nthic <= nthi )
				continue; /* continue excitation loop */

			  nthic = 1;
			  xpr1  = thetis;
			  xpr2 += xpr5;
			  nphic++;

			  if( nphic > nphi )
				break;

			} /* do (l_54) */
			while( TRUE );

			/* jump to freq. or input loop */
			if( jmp_iloop )
			  break;

			if( jmp_floop )
			  continue;

			nphic = 1;
			xpr2  = phiss;

			/* normalized receiving pattern printed */
			if( iptflg >= 2)
			{
			  itmp1= nthi* nphi;

			  tmp1= fnorm[0];
			  for( j = 1; j < itmp1; j++ )
				if( fnorm[j] > tmp1)
				  tmp1= fnorm[j];

			  fprintf( output_fp, "\n\n\n"
				  "                     "
				  "---- NORMALIZED RECEIVING PATTERN ----\n"
				  "                      "
				  "NORMALIZATION FACTOR: %11.4E\n"
				  "                      "
				  "ETA: %7.2f DEGREES\n"
				  "                      "
				  "TYPE: %s\n"
				  "                      AXIAL RATIO: %6.3f\n"
				  "                      SEGMENT No: %d\n\n"
				  "                      "
				  "THETA     PHI       ---- PATTERN ----\n"
				  "                      "
				  "(DEG)    (DEG)       DB     MAGNITUDE",
				  tmp1, xpr3, hpol[fpat.ixtyp-1], fpat.xpr6, isave );

			  for( j = 0; j < nphi; j++ )
			  {
				itmp2= nthi*j;

				for( i = 0; i < nthi; i++ )
				{
				  itmp3= i + itmp2;

				  if( itmp3 < itmp1)
				  {
					tmp2= fnorm[itmp3]/ tmp1;
					tmp3= db20( tmp2);

					fprintf( output_fp, "\n"
						"                    %7.2f  %7.2f   %7.2f  %11.4E",
						xpr1, xpr2, tmp3, tmp2 );

					xpr1 += xpr4;
				  }

				} /* for( i = 0; i < nthi; i++ ) */

				xpr1= thetis;
				xpr2 += xpr5;

			  } /* for( j = 0; j < nphi; j++ ) */

			  xpr2= phiss;

			} /* if( iptflg >= 2) */

			if( mhz == nfrq)
			  gnd.ifar=-1;

			if( nfrq == 1)
			{
			  fprintf( output_fp, "\n\n\n" );
			  jmp_iloop = TRUE;
			  break; /* continue card input loop */
			}

		  } /*** do (frequency loop) (l_42) ***/
		  while( (++mhz <= nfrq) );

		  /* Jump to card input loop */
		  if( jmp_iloop )
			break;

		  if( iped != 0)
		  {
			int iss;

			if( vsorc.nvqd > 0)
			  iss = vsorc.ivqd[vsorc.nvqd-1];
			else
			  iss = vsorc.isant[vsorc.nsant-1];

			fprintf( output_fp, "\n\n\n"
				"                            "
				" -------- INPUT IMPEDANCE DATA --------\n"
				"                                     "
				" SOURCE SEGMENT No: %d\n"
				"                                  "
				" NORMALIZATION FACTOR:%12.5E\n\n"
				"              ----------- UNNORMALIZED IMPEDANCE ----------  "
				"  ------------ NORMALIZED IMPEDANCE -----------\n"
				"      FREQ    RESISTANCE    REACTANCE    MAGNITUDE    PHASE  "
				"  RESISTANCE    REACTANCE    MAGNITUDE    PHASE\n"
				"       MHz       OHMS         OHMS         OHMS     DEGREES  "
				"     OHMS         OHMS         OHMS     DEGREES",
				iss, zpnorm );

			itmp1= nfrq;
			if( ifrq == 0)
			  tmp1= save.fmhz-( nfrq-1)* delfrq;
			else
			  if( ifrq == 1)
				tmp1= save.fmhz/( pow(delfrq, (nfrq-1)) );

			for( i = 0; i < itmp1; i++ )
			{
			  itmp2= 4*i;
			  tmp2= fnorm[itmp2  ]/ zpnorm;
			  tmp3= fnorm[itmp2+1]/ zpnorm;
			  tmp4= fnorm[itmp2+2]/ zpnorm;
			  tmp5= fnorm[itmp2+3];

			  fprintf( output_fp, "\n"
				  " %9.3f   %11.4E  %11.4E  %11.4E  %7.2f  "
				  " %11.4E  %11.4E  %11.4E  %7.2f",
				  tmp1, fnorm[itmp2], fnorm[itmp2+1], fnorm[itmp2+2],
				  fnorm[itmp2+3], tmp2, tmp3, tmp4, tmp5 );

			  if( ifrq == 0)
				tmp1 += delfrq;
			  else
				if( ifrq == 1)
				  tmp1 *= delfrq;

			} /* for( i = 0; i < itmp1; i++ ) */

			fprintf( output_fp, "\n\n\n" );

		  } /* if( iped != 0) */

		  nfrq=1;
		  mhz=1;

	  } /* switch( igox ) */

	} /* while( ! next_job ): Main input section (l_14) */

  } /* while(TRUE): Main execution loop (l_1) */

} /* end of main() */

/*-----------------------------------------------------------------------*/

/*  Null_Pointers()
 *
 *  Nulls pointers used in mem_realloc
 */
  void
Null_Pointers( void )
{
  crnt.air = crnt.aii = NULL;
  crnt.bir = crnt.bii = NULL;
  crnt.cir = crnt.cii = NULL;
  crnt.cur = NULL;

  data.x = data.y = data.z = NULL;
  data.x1 = data.y1 = data.z1 = NULL;
  data.x2 = data.y2 = data.z2 = NULL;
  data.si = data.bi = data.sab = NULL;
  data.cab = data.salp = NULL;
  data.itag = data.icon1 = data.icon2 = NULL;
  data.px = data.py = data.pz = NULL;
  data.t1x = data.t1y = data.t1z = NULL;
  data.t2x = data.t2y = data.t2z = NULL;
  data.pbi = data.psalp = NULL;

  netcx.ntyp = netcx.iseg1 = netcx.iseg2 = NULL;
  netcx.x11r = netcx.x11i = NULL;
  netcx.x12r = netcx.x12i = NULL;
  netcx.x22r = netcx.x22i = NULL;

  save.ip = NULL;

  segj.jco = NULL;
  segj.ax = segj.bx = segj.cx = NULL;

  smat.ssx = NULL;

  vsorc.isant = vsorc.ivqd = vsorc.iqds = NULL;
  vsorc.vqd = vsorc.vqds = vsorc.vsant = NULL;

  yparm.y11a = yparm.y12a = NULL;
  yparm.ncseg = yparm.nctag = NULL;

  zload.zarray = NULL;

} /* Null_Pointers() */

/*-----------------------------------------------------------------------*/

/* prnt sets up the print formats for impedance loading */
void prnt( int in1, int in2, int in3, double fl1, double fl2,
		   double fl3, double fl4, double fl5,
		   double fl6, char *ia, int ichar )
{
  /* record to be output and buffer used to make it */
  char record[101+ichar*4], buff[16];
  int in[3], i1, i;
  double fl[6];

  in[0]= in1;
  in[1]= in2;
  in[2]= in3;
  fl[0]= fl1;
  fl[1]= fl2;
  fl[2]= fl3;
  fl[3]= fl4;
  fl[4]= fl5;
  fl[5]= fl6;

  /* integer format */
  i1=0;
  strcpy( record, "\n " );

  if( (in1 == 0) && (in2 == 0) && (in3 == 0) )
  {
	strcat( record, " ALL" );
	i1=1;
  }

  for( i = i1; i < 3; i++ )
  {
	if( in[i] == 0)
	  strcat( record, "     " );
	else
	{
	  snprintf( buff, 6, "%5d", in[i] );
	  strcat( record, buff );
	}
  }

  /* floating point format */
  for( i = 0; i < 6; i++ )
  {
	if( fabs( fl[i]) >= 1.0e-20 )
	{
	  snprintf( buff, 15, " %11.4E", fl[i] );
	  strcat( record, buff );
	}
	else
	  strcat( record, "            " );
  }

  strcat( record, "   " );
  strcat( record, ia );
  fprintf( output_fp, "%s", record );

  return;
}

/*-----------------------------------------------------------------------*/

static void sig_handler( int signal )
{
  fprintf( stderr, "\n" );
  switch( signal )
  {
	case SIGINT :
	  fprintf( stderr, "%s\n", "nec2c: exiting via user interrupt" );
	  exit( signal );

	case SIGSEGV :
	  fprintf( stderr, "%s\n", "nec2c: segmentation fault" );
	  exit( signal );

	case SIGFPE :
	  fprintf( stderr, "%s\n", "nec2c: floating point exception" );
	  exit( signal );

	case SIGABRT :
	  fprintf( stderr, "%s\n", "nec2c: abort signal received" );
	  exit( signal );

	case SIGTERM :
	  fprintf( stderr, "%s\n", "nec2c: termination request received" );

	  stop( signal );
  }

} /* end of sig_handler() */

/*------------------------------------------------------------------------*/

