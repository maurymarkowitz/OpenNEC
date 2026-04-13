C     PROGRAM NEC(INPUT,TAPE5=INPUT,OUTPUT,TAPE11,TAPE12,TAPE13,TAPE14, 
C    1TAPE15,TAPE16,TAPE20,TAPE21)                                      
C                                                                       
C     NUMERICAL ELECTROMAGNETICS CODE (NEC2)  DEVELOPED AT LAWRENCE     
C     LIVERMORE LAB., LIVERMORE, CA.  (CONTACT G. BURKE AT 415-422-8414 
C     FOR PROBLEMS WITH THE NEC CODE.  FOR PROBLEMS WITH THE VAX IMPLEM-
C     ENTATION, CONTACT J. BREAKALL AT 415-422-8196 OR E. DOMNING AT 415
C     422-5936)
C     FILE CREATED 4/11/80.                                             
C                                                                       
C                ***********NOTICE**********                            
C     THIS COMPUTER CODE MATERIAL WAS PREPARED AS AN ACCOUNT OF WORK    
C     SPONSORED BY THE UNITED STATES GOVERNMENT.  NEITHER THE UNITED    
C     STATES NOR THE UNITED STATES DEPARTMENT OF ENERGY, NOR ANY OF     
C     THEIR EMPLOYEES, NOR ANY OF THEIR CONTRACTORS, SUBCONTRACTORS, OR 
C     THEIR EMPLOYEES, MAKES ANY WARRANTY, EXPRESS OR IMPLIED, OR       
C     ASSUMES ANY LEGAL LIABILITY OR RESPONSIBILITY FOR THE ACCURACY,   
C     COMPLETENESS OR USEFULNESS OF ANY INFORMATION, APPARATUS, PRODUCT 
C     OR PROCESS DISCLOSED, OR REPRESENTS THAT ITS USE WOULD NOT        
C     INFRINGE PRIVATELY-OWNED RIGHTS.                                  
C                                                                       
C***
C ***
C     DOUBLE PRECISION 6/4/85
C
C ***
      IMPLICIT REAL (A-H,O-Z)
      PARAMETER ( NM=600, N2M=800, N3M=1000)

      CHARACTER   AIN*2, ATST*2, INFILE*80, OTFILE*80
C***
C     INTEGER  AIN,ATST,PNET
      integer*4 COM
      CHARACTER*6 HPOL,PNET
      COMPLEX  CM, FJ, VSANT, ETH, EPH, ZRATI, CUR, CURI, ZARRAY, 
     &ZRATI2
      COMPLEX  EX, EY, EZ, ZPED, VQD, VQDS, T1, Y11A, Y12A, EPSC, U,
     & U2, XX1, XX2
      COMPLEX  AR1, AR2, AR3, EPSCF, FRATI
      COMMON  /DATA/ LD, N1, N2, N, NP, M1, M2, M, MP, X( NM), Y( NM), 
     &Z( NM), SI( NM), BI( NM), ALP( NM), BET( NM), ICON1( N2M), ICON2(
     & N2M), ITAG( N2M), ICONX( NM), WLAM, IPSYM
      COMMON  /CMB/ CM(90000)
      COMMON  /MATPAR/ ICASE, NBLOKS, NPBLK, NLAST, NBLSYM, NPSYM, 
     &NLSYM, IMAT, ICASX, NBBX, NPBX, NLBX, NBBL, NPBL, NLBL
      COMMON  /SAVE/ IP( N2M), KCOM, COM(20,5), EPSR, SIG, SCRWLT, 
     &SCRWRT, FMHZ
      COMMON  /CRNT/ AIR( NM), AII( NM), BIR( NM), BII( NM), CIR( NM), 
     &CII( NM), CUR( N3M)
      COMMON  /GND/ ZRATI, ZRATI2, FRATI, CL, CH, SCRWL, SCRWR, NRADL, 
     &KSYMP, IFAR, IPERF, T1, T2
      COMMON  /ZLOAD/ ZARRAY( NM), NLOAD, NLODF
      COMMON  /YPARM/ NCOUP, ICOUP, NCTAG(5), NCSEG(5), Y11A(5), Y12A(
     &20)
      COMMON  /SEGJ/ AX(30), BX(30), CX(30), JCO(30), JSNO, ISCON(50), 
     &NSCON, IPCON(10), NPCON
      COMMON  /VSORC/ VQD(30), VSANT(30), VQDS(30), IVQD(30), ISANT(30)
     &, IQDS(30), NVQD, NSANT, NQDS
      COMMON  /NETCX/ ZPED, PIN, PNLS, NEQ, NPEQ, NEQ2, NONET, NTSOL, 
     &NPRINT, MASYM, ISEG1(150), ISEG2(150), X11R(150), X11I(150), 
     &X12R(150), X12I(150), X22R(150), X22I(150), NTYP(150)
      COMMON  /FPAT/ NTH, NPH, IPD, IAVP, INOR, IAX, THETS, PHIS, DTH, 
     &DPH, RFLD, GNOR, CLT, CHT, EPSR2, SIG2, IXTYP, XPR6, PINR, PNLR, 
     &PLOSS, NEAR, NFEH, NRX, NRY, NRZ, XNR, YNR, ZNR, DXNR, DYNR, DZNR
     &
      COMMON  /GGRID/ AR1(11,10,4), AR2(17,5,4), AR3(9,8,4), EPSCF, DXA
     &(3), DYA(3), XSA(3), YSA(3), NXA(3), NYA(3)
C***
      COMMON  /GWAV/ U, U2, XX1, XX2, R1, R2, ZMH, ZPH
C***
      COMMON  /PLOT/ IPLP1, IPLP2, IPLP3, IPLP4
      DIMENSION  CAB(1), SAB(1), X2(1), Y2(1), Z2(1)
      DIMENSION  LDTYP(200), LDTAG(200), LDTAGF(200), LDTAGT(200), 
     & ZLR(200), ZLI(200), ZLC(200)
      DIMENSION  ATST(22), PNET(6), HPOL(3), IX( N2M)
      DIMENSION  FNORM(200)
C***
      DIMENSION  T1X(1), T1Y(1), T1Z(1), T2X(1), T2Y(1), T2Z(1)
      DIMENSION  XTEMP( NM), YTEMP( NM), ZTEMP( NM), SITEMP( NM), 
     &BITEMP( NM)
      EQUIVALENCE(CAB,ALP),(SAB,BET),(X2,SI),(Y2,ALP),(Z2,BET)
      EQUIVALENCE(T1X,SI),(T1Y,ALP),(T1Z,BET),(T2X,ICON1),(T2Y,ICON2),(
     &T2Z,ITAG)
      DATA   ATST/'CE','FR','LD','GN','EX','NT','XQ','NE','GD','RP',
     &'CM','NX','EN','TL','PT','KH','NH','PQ','EK','WG','CP','PL'/
      DATA   HPOL/6HLINEAR,5HRIGHT,4HLEFT/
      DATA   PNET/6H      ,2H  ,6HSTRAIG,2HHT,6HCROSSE,1HD/
      DATA   TA/1.745329252D-02/, CVEL/299.8/
C***
      DATA   LOADMX, NSMAX, NETMX/200,150,150/, NORMF/200/
  706 CONTINUE
      PRINT 700 
  700 FORMAT('$ENTER DATA INPUT FILENAME [HIT RETURN FOR TERMINAL',
     &' INPUT] : ',/,'$     >')
  701 FORMAT(A)
      READ( *,701,ERR=702)  INFILE
      CALL STR0PC( INFILE, INFILE)

C      OPEN (UNIT=5,FILE=INFILE,STATUS='OLD',READONLY,ERR=702)
      IF( INFILE.NE.' ') THEN
      OPEN ( UNIT=1,FILE=INFILE,STATUS='OLD',ERR=702)
      ENDIF
  707 CONTINUE
      PRINT 703 
  703 FORMAT('$ENTER DATA OUTPUT FILENAME [HIT RETURN FOR TERMINAL',
     &' OUTPUT] : ',/,'$     >')
      READ( *,701,ERR=704)  OTFILE
      CALL STR0PC( OTFILE, OTFILE)

      IF( OTFILE.NE.' ') THEN
      OPEN ( UNIT=2,FILE=OTFILE,STATUS='NEW',ERR=704)
      ENDIF
      GOTO 705
  702 print *, 'error on terminal input'
      CALL ERROR
      GOTO 706
  704 CALL ERROR
      GOTO 707
C***
  705 CONTINUE
      CALL SECNDS(EXTIM)
      FJ=(0.,1.)
      LD=600
      NXA(1)=0
      IRESRV=90000
C***
    1 KCOM=0
      IFRTMW=0
C***
      IFRTMP=0
    2 KCOM= KCOM+1
      IF( KCOM.GT.5) KCOM=5
C***

      READ( 1,125)  AIN,( COM( I, KCOM), I=1,19)
C***
      CALL STR0PC( AIN, AIN)

      if (KCOM .le. 0) then
          WRITE (2,126) 
          WRITE (2,127) 
          WRITE (2,128) 
      endif

      WRITE (2,129) ( COM( I, KCOM), I=1,19)

      IF( AIN.EQ. ATST(11)) GOTO 2

      if (AIN .ne. ATST(1)) then
          WRITE (2,130) 
          STOP
      endif

C    4 CONTINUE

      DO 5  I=1, LD
    5 ZARRAY( I)=(0.,0.)
      MPCNT=0
C                                                                       
C     SET UP GEOMETRY DATA IN SUBROUTINE DATAGN                         
C                                                                       
      IMAT=0
      CALL DATAGN
      IFLOW=1
C                                                                       
C     CORE ALLOCATION FOR ARRAYS B, C, AND D FOR N.G.F. SOLUTION        
C                                                                       
      IF( IMAT.EQ.0) GOTO 326
      NEQ= N1+2* M1
      NEQ2= N- N1+2*( M- M1)+ NSCON+2* NPCON
      CALL FBNGF( NEQ, NEQ2, IRESRV, IB11, IC11, ID11, IX11)
      GOTO 6
  326 NEQ= N+2* M
      NEQ2=0
      IB11=1
      IC11=1
      ID11=1
      IX11=1
      ICASX=0
    6 NPEQ= NP+2* MP
C                                                                       
C     DEFAULT VALUES FOR INPUT PARAMETERS AND FLAGS                     
C                                                                       
C***
      WRITE (2,135) 
      IPLP1=0
      IPLP2=0
      IPLP3=0
C***
      IPLP4=0
      IGO=1
      FMHZS= CVEL
      NFRQ=1
      RKH=1.
      IEXK=0
      IXTYP=0
      NLOAD=0
      NONET=0
      NEAR=-1
      IPTFLG=-2
      IPTFLQ=-1
      IFAR=-1
      ZRATI=(1.,0.)
      IPED=0
      IRNGF=0
      NCOUP=0
      ICOUP=0
      IF( ICASX.GT.0) GOTO 14
      FMHZ= CVEL
      NLODF=0
      KSYMP=1
      NRADL=0
C                                                                       
C     MAIN INPUT SECTION - STANDARD READ STATEMENT - JUMPS TO APPRO-    
C     PRIATE SECTION FOR SPECIFIC PARAMETER SET UP                      
C                                                                       
C14    READ(1,136)AIN,ITMP1,ITMP2,ITMP3,ITMP4,TMP1,TMP2,TMP3,TMP4,TMP5, 
C     1TMP6                                                             
C***
      IPERF=0
C***
   14 CALL READMN( AIN, ITMP1, ITMP2, ITMP3, ITMP4, TMP1, TMP2, TMP3, 
     &TMP4, TMP5, TMP6)
      MPCNT= MPCNT+1
      WRITE (2,137)  MPCNT, AIN, ITMP1, ITMP2, ITMP3, ITMP4, TMP1, TMP2
     &, TMP3, TMP4, TMP5, TMP6
      IF( AIN.EQ. ATST(2)) GOTO 16
      IF( AIN.EQ. ATST(3)) GOTO 17
      IF( AIN.EQ. ATST(4)) GOTO 21
      IF( AIN.EQ. ATST(5)) GOTO 24
      IF( AIN.EQ. ATST(6)) GOTO 28
      IF( AIN.EQ. ATST(14)) GOTO 28
      IF( AIN.EQ. ATST(15)) GOTO 31
      IF( AIN.EQ. ATST(18)) GOTO 319
      IF( AIN.EQ. ATST(7)) GOTO 37
      IF( AIN.EQ. ATST(8)) GOTO 32
      IF( AIN.EQ. ATST(17)) GOTO 208
      IF( AIN.EQ. ATST(9)) GOTO 34
      IF( AIN.EQ. ATST(10)) GOTO 36
      IF( AIN.EQ. ATST(16)) GOTO 305
      IF( AIN.EQ. ATST(19)) GOTO 320
      IF( AIN.EQ. ATST(12)) GOTO 1
      IF( AIN.EQ. ATST(20)) GOTO 322
C***
      IF( AIN.EQ. ATST(21)) GOTO 304
C***
      IF( AIN.EQ. ATST(22)) GOTO 330
      IF( AIN.NE. ATST(13)) GOTO 15
      CALL SECNDS( TMP1)
      TMP1= TMP1- EXTIM
      WRITE (2,201)  TMP1
      STOP
   15 WRITE (2,138) 
C                                                                       
C     FREQUENCY PARAMETERS                                              
C                                                                       
      STOP
   16 IFRQ= ITMP1
      IF( ICASX.EQ.0) GOTO 8
      WRITE (2,303)  AIN
      STOP
    8 NFRQ= ITMP2
      IF( NFRQ.EQ.0) NFRQ=1
      FMHZ= TMP1
      DELFRQ= TMP2
      IF( IPED.EQ.1) ZPNORM=0.
      IGO=1
      IFLOW=1
C                                                                       
C     MATRIX INTEGRATION LIMIT                                          
C                                                                       
      GOTO 14
  305 RKH= TMP1
      IF( IGO.GT.2) IGO=2
      IFLOW=1
C                                                                       
C     EXTENDED THIN WIRE KERNEL OPTION                                  
C                                                                       
      GOTO 14
  320 IEXK=1
      IF( ITMP1.EQ.-1) IEXK=0
      IF( IGO.GT.2) IGO=2
      IFLOW=1
C                                                                       
C     MAXIMUM COUPLING BETWEEN ANTENNAS                                 
C                                                                       
      GOTO 14
  304 IF( IFLOW.NE.2) NCOUP=0
      ICOUP=0
      IFLOW=2
      IF( ITMP2.EQ.0) GOTO 14
      NCOUP= NCOUP+1
      IF( NCOUP.GT.5) GOTO 312
      NCTAG( NCOUP)= ITMP1
      NCSEG( NCOUP)= ITMP2
      IF( ITMP4.EQ.0) GOTO 14
      NCOUP= NCOUP+1
      IF( NCOUP.GT.5) GOTO 312
      NCTAG( NCOUP)= ITMP3
      NCSEG( NCOUP)= ITMP4
      GOTO 14
  312 WRITE (2,313) 
C                                                                       
C     LOADING PARAMETERS                                                
C                                                                       
      STOP
   17 IF( IFLOW.EQ.3) GOTO 18
      NLOAD=0
      IFLOW=3
      IF( IGO.GT.2) IGO=2
      IF( ITMP1.EQ.(-1)) GOTO 14
   18 NLOAD= NLOAD+1
      IF( NLOAD.LE. LOADMX) GOTO 19
      WRITE (2,139) 
      STOP
   19 LDTYP( NLOAD)= ITMP1
      LDTAG( NLOAD)= ITMP2
      IF( ITMP4.EQ.0) ITMP4= ITMP3
      LDTAGF( NLOAD)= ITMP3
      LDTAGT( NLOAD)= ITMP4
      IF( ITMP4.GE. ITMP3) GOTO 20
      WRITE (2,140)  NLOAD, ITMP3, ITMP4
      STOP
   20 ZLR( NLOAD)= TMP1
      ZLI( NLOAD)= TMP2
      ZLC( NLOAD)= TMP3
C                                                                       
C     GROUND PARAMETERS UNDER THE ANTENNA                               
C                                                                       
      GOTO 14
   21 IFLOW=4
      IF( ICASX.EQ.0) GOTO 10
      WRITE (2,303)  AIN
      STOP
   10 IF( IGO.GT.2) IGO=2
      IF( ITMP1.NE.(-1)) GOTO 22
      KSYMP=1
      NRADL=0
      IPERF=0
      GOTO 14
   22 IPERF= ITMP1
      NRADL= ITMP2
      KSYMP=2
      EPSR= TMP1
      SIG= TMP2
      IF( NRADL.EQ.0) GOTO 23
      IF( IPERF.NE.2) GOTO 314
      WRITE (2,390) 
      STOP
  314 SCRWLT= TMP3
      SCRWRT= TMP4
      GOTO 14
   23 EPSR2= TMP3
      SIG2= TMP4
      CLT= TMP5
      CHT= TMP6
C                                                                       
C     EXCITATION PARAMETERS                                             
C                                                                       
      GOTO 14
   24 IF( IFLOW.EQ.5) GOTO 25
      NSANT=0
      NVQD=0
      IPED=0
      IFLOW=5
      IF( IGO.GT.3) IGO=3
   25 MASYM= ITMP4/10
      IF( ITMP1.GT.0.AND. ITMP1.NE.5) GOTO 27
      IXTYP= ITMP1
      NTSOL=0
      IF( IXTYP.EQ.0) GOTO 205
      NVQD= NVQD+1
      IF( NVQD.GT. NSMAX) GOTO 206
      IVQD( NVQD)= ISEGNO( ITMP2, ITMP3)
      VQD( NVQD)= CMPLX( TMP1, TMP2)
      IF( ABS( VQD( NVQD)).LT.1.D-20) VQD( NVQD)=(1.,0.)
      GOTO 207
  205 NSANT= NSANT+1
      IF( NSANT.LE. NSMAX) GOTO 26
  206 WRITE (2,141) 
      STOP
   26 ISANT( NSANT)= ISEGNO( ITMP2, ITMP3)
      VSANT( NSANT)= CMPLX( TMP1, TMP2)
      IF( ABS( VSANT( NSANT)).LT.1.D-20) VSANT( NSANT)=(1.,0.)
  207 IPED= ITMP4- MASYM*10
      ZPNORM= TMP3
      IF( IPED.EQ.1.AND. ZPNORM.GT.0) IPED=2
      GOTO 14
   27 IF( IXTYP.EQ.0.OR. IXTYP.EQ.5) NTSOL=0
      IXTYP= ITMP1
      NTHI= ITMP2
      NPHI= ITMP3
      XPR1= TMP1
      XPR2= TMP2
      XPR3= TMP3
      XPR4= TMP4
      XPR5= TMP5
      XPR6= TMP6
      NSANT=0
      NVQD=0
      THETIS= XPR1
      PHISS= XPR2
C                                                                       
C     NETWORK PARAMETERS                                                
C                                                                       
      GOTO 14
   28 IF( IFLOW.EQ.6) GOTO 29
      NONET=0
      NTSOL=0
      IFLOW=6
      IF( IGO.GT.3) IGO=3
      IF( ITMP2.EQ.(-1)) GOTO 14
   29 NONET= NONET+1
      IF( NONET.LE. NETMX) GOTO 30
      WRITE (2,142) 
      STOP
   30 NTYP( NONET)=2
      IF( AIN.EQ. ATST(6)) NTYP( NONET)=1
      ISEG1( NONET)= ISEGNO( ITMP1, ITMP2)
      ISEG2( NONET)= ISEGNO( ITMP3, ITMP4)
      X11R( NONET)= TMP1
      X11I( NONET)= TMP2
      X12R( NONET)= TMP3
      X12I( NONET)= TMP4
      X22R( NONET)= TMP5
      X22I( NONET)= TMP6
      IF( NTYP( NONET).EQ.1.OR. TMP1.GT.0.) GOTO 14
      NTYP( NONET)=3
C***
C
C     PLOT FLAGS
C
      X11R( NONET)=- TMP1
  330 IPLP1= ITMP1
      IPLP2= ITMP2
      IPLP3= ITMP3
C***
      IPLP4= ITMP4
C                                                                       
C     PRINT CONTROL FOR CURRENT                                         
C                                                                       
      GOTO 14
   31 IPTFLG= ITMP1
      IPTAG= ITMP2
      IPTAGF= ITMP3
      IPTAGT= ITMP4
      IF( ITMP3.EQ.0.AND. IPTFLG.NE.-1) IPTFLG=-2
      IF( ITMP4.EQ.0) IPTAGT= IPTAGF
C                                                                       
C     WRITE CONTROL FOR CHARGE                                          
C                                                                       
      GOTO 14
  319 IPTFLQ= ITMP1
      IPTAQ= ITMP2
      IPTAQF= ITMP3
      IPTAQT= ITMP4
      IF( ITMP3.EQ.0.AND. IPTFLQ.NE.-1) IPTFLQ=-2
      IF( ITMP4.EQ.0) IPTAQT= IPTAQF
C                                                                       
C     NEAR FIELD CALCULATION PARAMETERS                                 
C                                                                       
      GOTO 14
  208 NFEH=1
      GOTO 209
   32 NFEH=0
  209 IF(.NOT.( IFLOW.EQ.8.AND. NFRQ.NE.1)) GOTO 33
      WRITE (2,143) 
   33 NEAR= ITMP1
      NRX= ITMP2
      NRY= ITMP3
      NRZ= ITMP4
      XNR= TMP1
      YNR= TMP2
      ZNR= TMP3
      DXNR= TMP4
      DYNR= TMP5
      DZNR= TMP6
      IFLOW=8
      IF( NFRQ.NE.1) GOTO 14
C                                                                       
C     GROUND REPRESENTATION                                             
C                                                                       
      GOTO (41,46,53,71,72), IGO
   34 EPSR2= TMP1
      SIG2= TMP2
      CLT= TMP3
      CHT= TMP4
      IFLOW=9
C                                                                       
C     STANDARD OBSERVATION ANGLE PARAMETERS                             
C                                                                       
      GOTO 14
   36 IFAR= ITMP1
      NTH= ITMP2
      NPH= ITMP3
      IF( NTH.EQ.0) NTH=1
      IF( NPH.EQ.0) NPH=1
      IPD= ITMP4/10
      IAVP= ITMP4- IPD*10
      INOR= IPD/10
      IPD= IPD- INOR*10
      IAX= INOR/10
      INOR= INOR- IAX*10
      IF( IAX.NE.0) IAX=1
      IF( IPD.NE.0) IPD=1
      IF( NTH.LT.2.OR. NPH.LT.2) IAVP=0
      IF( IFAR.EQ.1) IAVP=0
      THETS= TMP1
      PHIS= TMP2
      DTH= TMP3
      DPH= TMP4
      RFLD= TMP5
      GNOR= TMP6
      IFLOW=10
C                                                                       
C     WRITE NUMERICAL GREEN'S FUNCTION TAPE                             
C                                                                       
      GOTO (41,46,53,71,78), IGO
  322 IFLOW=12
      IF( ICASX.EQ.0) GOTO 301
      WRITE (2,302) 
      STOP
  301 IRNGF= IRESRV/2
C                                                                       
C     EXECUTE CARD  -  CALC. INCLUDING RADIATED FIELDS                  
C                                                                       
      GOTO (41,46,52,52,52), IGO
   37 IF( IFLOW.EQ.10.AND. ITMP1.EQ.0) GOTO 14
      IF( NFRQ.EQ.1.AND. ITMP1.EQ.0.AND. IFLOW.GT.7) GOTO 14
      IF( ITMP1.NE.0) GOTO 39
      IF( IFLOW.GT.7) GOTO 38
      IFLOW=7
      GOTO 40
   38 IFLOW=11
      GOTO 40
   39 IFAR=0
      RFLD=0.
      IPD=0
      IAVP=0
      INOR=0
      IAX=0
      NTH=91
      NPH=1
      THETS=0.
      PHIS=0.
      DTH=1.0
      DPH=0.
      IF( ITMP1.EQ.2) PHIS=90.
      IF( ITMP1.NE.3) GOTO 40
      NPH=2
      DPH=90.
C                                                                       
C     END OF THE MAIN INPUT SECTION                                     
C                                                                       
C     BEGINNING OF THE FREQUENCY DO LOOP                                
C                                                                       
   40 GOTO (41,46,53,71,78), IGO
C***
   41 MHZ=1
      IF( N.EQ.0.OR. IFRTMW.EQ.1) GOTO 406
      IFRTMW=1
      DO 445  I=1, N
      XTEMP( I)= X( I)
      YTEMP( I)= Y( I)
      ZTEMP( I)= Z( I)
      SITEMP( I)= SI( I)
      BITEMP( I)= BI( I)
  445 CONTINUE
  406 IF( M.EQ.0.OR. IFRTMP.EQ.1) GOTO 407
      IFRTMP=1
      J= LD+1
      DO 545  I=1, M
      J= J-1
      XTEMP( J)= X( J)
      YTEMP( J)= Y( J)
      ZTEMP( J)= Z( J)
      BITEMP( J)= BI( J)
  545 CONTINUE
  407 CONTINUE
C***
C     CORE ALLOCATION FOR PRIMARY INTERACTON MATRIX.  (A)               
      FMHZ1= FMHZ
      IF( IMAT.EQ.0) CALL FBLOCK( NPEQ, NEQ, IRESRV, IRNGF, IPSYM)
   42 IF( MHZ.EQ.1) GOTO 44
C      FMHZ=FMHZ+DELFRQ                                                 
C***
      IF( IFRQ.EQ.1) GOTO 43
      FMHZ= FMHZ1+( MHZ-1)* DELFRQ
      GOTO 44
   43 FMHZ= FMHZ* DELFRQ
C***
   44 FR= FMHZ/ CVEL
      WLAM= CVEL/ FMHZ
      WRITE (2,145)  FMHZ, WLAM
      WRITE (2,196)  RKH
C     FREQUENCY SCALING OF GEOMETRIC PARAMETERS                         
C***      FMHZS=FMHZ                                                    
      IF( IEXK.EQ.1) WRITE (2,321) 
      IF( N.EQ.0) GOTO 306
C***
      DO 45  I=1, N
      X( I)= XTEMP( I)* FR
      Y( I)= YTEMP( I)* FR
      Z( I)= ZTEMP( I)* FR
      SI( I)= SITEMP( I)* FR
C***
   45 BI( I)= BITEMP( I)* FR
  306 IF( M.EQ.0) GOTO 307
      FR2= FR* FR
      J= LD+1
      DO 245  I=1, M
C***
      J= J-1
      X( J)= XTEMP( J)* FR
      Y( J)= YTEMP( J)* FR
      Z( J)= ZTEMP( J)* FR
C***
  245 BI( J)= BITEMP( J)* FR2
C     STRUCTURE SEGMENT LOADING                                         
  307 IGO=2
   46 WRITE (2,146) 
      IF( NLOAD.NE.0) CALL LOAD( LDTYP, LDTAG, LDTAGF, LDTAGT, ZLR, ZLI
     &, ZLC)
      IF( NLOAD.EQ.0.AND. NLODF.EQ.0) WRITE (2,147) 
C     GROUND PARAMETER                                                  
      IF( NLOAD.EQ.0.AND. NLODF.NE.0) WRITE (2,327) 
      WRITE (2,148) 
      IF( KSYMP.EQ.1) GOTO 49
      FRATI=(1.,0.)
      IF( IPERF.EQ.1) GOTO 48
      IF( SIG.LT.0.) SIG=- SIG/(59.96* WLAM)
      EPSC= CMPLX( EPSR,- SIG* WLAM*59.96)
      ZRATI=1./ SQRT( EPSC)
      U= ZRATI
      U2= U* U
      IF( NRADL.EQ.0) GOTO 47
      SCRWL= SCRWLT/ WLAM
      SCRWR= SCRWRT/ WLAM
      T1= FJ*2367.067D+0/ DFLOAT( NRADL)
      T2= SCRWR* DFLOAT( NRADL)
      WRITE (2,170)  NRADL, SCRWLT, SCRWRT
      WRITE (2,149) 
   47 IF( IPERF.EQ.2) GOTO 328
      WRITE (2,391) 
      GOTO 329
  328 IF( NXA(1).EQ.0) READ( 21)  AR1, AR2, AR3, EPSCF, DXA, DYA, XSA, 
     &YSA, NXA, NYA
      FRATI=( EPSC-1.)/( EPSC+1.)
      IF( ABS(( EPSCF- EPSC)/ EPSC).LT.1.D-3) GOTO 400
      WRITE (2,393)  EPSCF, EPSC
      STOP
  400 WRITE (2,392) 
  329 WRITE (2,150)  EPSR, SIG, EPSC
      GOTO 50
   48 WRITE (2,151) 
      GOTO 50
   49 WRITE (2,152) 
C * * *                                                                 
C     FILL AND FACTOR PRIMARY INTERACTION MATRIX                        
C                                                                       
   50 CONTINUE
      CALL SECNDS( TIM1)
      IF( ICASX.NE.0) GOTO 324
      CALL CMSET( NEQ, CM, RKH, IEXK)
      CALL SECNDS( TIM2)
      TIM= TIM2- TIM1
      CALL FACTRS( NPEQ, NEQ, CM, IP, IX,11,12,13,14)
C                                                                       
C     N.G.F. - FILL B, C, AND D AND FACTOR D-C(INV(A)B)                 
C                                                                       
C ****
      GOTO 323
C ****
  324 IF( NEQ2.EQ.0) GOTO 333
      CALL CMNGF( CM( IB11), CM( IC11), CM( ID11), NPBX, NEQ, NEQ2, RKH
     &, IEXK)
      CALL SECNDS( TIM2)
      TIM= TIM2- TIM1
      CALL FACGF( CM, CM( IB11), CM( IC11), CM( ID11), CM( IX11), IP, 
     &IX, NP, N1, MP, M1, NEQ, NEQ2)
  323 CALL SECNDS( TIM1)
      TIM2= TIM1- TIM2
      WRITE (2,153)  TIM, TIM2
  333 IGO=3
      NTSOL=0
C     WRITE N.G.F. FILE                                                 
      IF( IFLOW.NE.12) GOTO 53
   52 CALL GFOUT
C                                                                       
C     EXCITATION SET UP (RIGHT HAND SIDE, -E INC.)                      
C                                                                       
      GOTO 14
   53 NTHIC=1
      NPHIC=1
      INC=1
      NPRINT=0
   54 IF( IXTYP.EQ.0.OR. IXTYP.EQ.5) GOTO 56
      IF( IPTFLG.LE.0.OR. IXTYP.EQ.4) WRITE (2,154) 
      TMP5= TA* XPR5
      TMP4= TA* XPR4
      IF( IXTYP.NE.4) GOTO 55
      TMP1= XPR1/ WLAM
      TMP2= XPR2/ WLAM
      TMP3= XPR3/ WLAM
      TMP6= XPR6/( WLAM* WLAM)
      WRITE (2,156)  XPR1, XPR2, XPR3, XPR4, XPR5, XPR6
      GOTO 56
   55 TMP1= TA* XPR1
      TMP2= TA* XPR2
      TMP3= TA* XPR3
      TMP6= XPR6
      IF( IPTFLG.LE.0) WRITE (2,155)  XPR1, XPR2, XPR3, HPOL( IXTYP), 
     &XPR6
C                                                                       
C     MATRIX SOLVING  (NETWK CALLS SOLVES)                              
C                                                                       
   56 CALL ETMNS( TMP1, TMP2, TMP3, TMP4, TMP5, TMP6, IXTYP, CUR)
      IF( NONET.EQ.0.OR. INC.GT.1) GOTO 60
      WRITE (2,158) 
      ITMP3=0
      ITMP1= NTYP(1)
      DO 59  I=1,2
      IF( ITMP1.EQ.3) ITMP1=2
      IF( ITMP1.EQ.2) WRITE (2,159) 
      IF( ITMP1.EQ.1) WRITE (2,160) 
      DO 58  J=1, NONET
      ITMP2= NTYP( J)
      IF(( ITMP2/ ITMP1).EQ.1) GOTO 57
      ITMP3= ITMP2
      GOTO 58
   57 ITMP4= ISEG1( J)
      ITMP5= ISEG2( J)
      IF( ITMP2.GE.2.AND. X11I( J).LE.0.) X11I( J)= WLAM* SQRT(( X( 
     &ITMP5)- X( ITMP4))**2+( Y( ITMP5)- Y( ITMP4))**2+( Z( ITMP5)- Z( 
     &ITMP4))**2)
      WRITE (2,157)  ITAG( ITMP4), ITMP4, ITAG( ITMP5), ITMP5, X11R( J)
     &, X11I( J), X12R( J), X12I( J), X22R( J), X22I( J), PNET(2* ITMP2
     &-1), PNET(2* ITMP2)
   58 CONTINUE
      IF( ITMP3.EQ.0) GOTO 60
      ITMP1= ITMP3
   59 CONTINUE
   60 CONTINUE
      IF( INC.GT.1.AND. IPTFLG.GT.0) NPRINT=1
      CALL NETWK( CM, CM( IB11), CM( IC11), CM( ID11), IP, CUR)
      NTSOL=1
      IF( IPED.EQ.0) GOTO 61
      ITMP1= MHZ+4*( MHZ-1)
      IF( ITMP1.GT.( NORMF-3)) GOTO 61
      FNORM( ITMP1)= REAL( ZPED)
      FNORM( ITMP1+1)= AIMAG( ZPED)
      FNORM( ITMP1+2)= ABS( ZPED)
      FNORM( ITMP1+3)= CANG( ZPED)
      IF( IPED.EQ.2) GOTO 61
      IF( FNORM( ITMP1+2).GT. ZPNORM) ZPNORM= FNORM( ITMP1+2)
C                                                                       
C     PRINTING STRUCTURE CURRENTS                                       
C                                                                       
   61 CONTINUE
      IF( N.EQ.0) GOTO 308
      IF( IPTFLG.EQ.(-1)) GOTO 63
      IF( IPTFLG.GT.0) GOTO 62
      WRITE (2,161) 
      WRITE (2,162) 
      GOTO 63
   62 IF( IPTFLG.EQ.3.OR. INC.GT.1) GOTO 63
      WRITE (2,163)  XPR3, HPOL( IXTYP), XPR6
   63 PLOSS=0.
      ITMP1=0
      JUMP= IPTFLG+1
      DO 69  I=1, N
      CURI= CUR( I)* WLAM
      CMAG= ABS( CURI)
      PH= CANG( CURI)
      IF( NLOAD.EQ.0.AND. NLODF.EQ.0) GOTO 64
      IF( ABS( REAL( ZARRAY( I))).LT.1.D-20) GOTO 64
      PLOSS= PLOSS+.5* CMAG* CMAG* REAL( ZARRAY( I))* SI( I)
   64 IF( JUMP) 68,69,65
   65 IF( IPTAG.EQ.0) GOTO 66
      IF( ITAG( I).NE. IPTAG) GOTO 69
   66 ITMP1= ITMP1+1
      IF( ITMP1.LT. IPTAGF.OR. ITMP1.GT. IPTAGT) GOTO 69
      IF( IPTFLG.EQ.0) GOTO 68
      IF( IPTFLG.LT.2.OR. INC.GT. NORMF) GOTO 67
      FNORM( INC)= CMAG
      ISAVE= I
   67 IF( IPTFLG.NE.3) WRITE (2,164)  XPR1, XPR2, CMAG, PH, I
      GOTO 69
C***
   68 WRITE (2,165)  I, ITAG( I), X( I), Y( I), Z( I), SI( I), CURI, 
     &CMAG, PH
      IF( IPLP1.NE.1) GOTO 69
      IF( IPLP2.EQ.1) WRITE( 8,*)  CURI
C***
      IF( IPLP2.EQ.2) WRITE( 8,*)  CMAG, PH
   69 CONTINUE
      IF( IPTFLQ.EQ.(-1)) GOTO 308
      WRITE (2,315) 
      ITMP1=0
      FR=1.D-6/ FMHZ
      DO 316  I=1, N
      IF( IPTFLQ.EQ.(-2)) GOTO 318
      IF( IPTAQ.EQ.0) GOTO 317
      IF( ITAG( I).NE. IPTAQ) GOTO 316
  317 ITMP1= ITMP1+1
      IF( ITMP1.LT. IPTAQF.OR. ITMP1.GT. IPTAQT) GOTO 316
  318 CURI= FR* CMPLX(- BII( I), BIR( I))
      CMAG= ABS( CURI)
      PH= CANG( CURI)
      WRITE (2,165)  I, ITAG( I), X( I), Y( I), Z( I), SI( I), CURI, 
     &CMAG, PH
  316 CONTINUE
  308 IF( M.EQ.0) GOTO 310
      WRITE (2,197) 
      J= N-2
      ITMP1= LD+1
      DO 309  I=1, M
      J= J+3
      ITMP1= ITMP1-1
      EX= CUR( J)
      EY= CUR( J+1)
      EZ= CUR( J+2)
      ETH= EX* T1X( ITMP1)+ EY* T1Y( ITMP1)+ EZ* T1Z( ITMP1)
      EPH= EX* T2X( ITMP1)+ EY* T2Y( ITMP1)+ EZ* T2Z( ITMP1)
      ETHM= ABS( ETH)
      ETHA= CANG( ETH)
      EPHM= ABS( EPH)
C309   WRITE(6,198) I,X(ITMP1),Y(ITMP1),Z(ITMP1),ETHM,ETHA,EPHM,EPHA,E  
C     1X,EY, EZ                                                         
C***
      EPHA= CANG( EPH)
      WRITE (2,198)  I, X( ITMP1), Y( ITMP1), Z( ITMP1), ETHM, ETHA, 
     &EPHM, EPHA, EX, EY, EZ
      IF( IPLP1.NE.1) GOTO 309
      IF( IPLP3.EQ.1) WRITE( 8,*)  EX
      IF( IPLP3.EQ.2) WRITE( 8,*)  EY
      IF( IPLP3.EQ.3) WRITE( 8,*)  EZ
      IF( IPLP3.EQ.4) WRITE( 8,*)  EX, EY, EZ
C***
  309 CONTINUE
  310 IF( IXTYP.NE.0.AND. IXTYP.NE.5) GOTO 70
      TMP1= PIN- PNLS- PLOSS
      TMP2=100.* TMP1/ PIN
      WRITE (2,166)  PIN, TMP1, PLOSS, PNLS, TMP2
   70 CONTINUE
      IGO=4
      IF( NCOUP.GT.0) CALL COUPLE( CUR, WLAM)
      IF( IFLOW.NE.7) GOTO 71
      IF( IXTYP.GT.0.AND. IXTYP.LT.4) GOTO 113
      IF( NFRQ.NE.1) GOTO 120
      WRITE (2,135) 
      GOTO 14
C                                                                       
C     NEAR FIELD CALCULATION                                            
C                                                                       
   71 IGO=5
   72 IF( NEAR.EQ.(-1)) GOTO 78
      CALL NFPAT
      IF( MHZ.EQ. NFRQ) NEAR=-1
      IF( NFRQ.NE.1) GOTO 78
      WRITE (2,135) 
C                                                                       
C     STANDARD FAR FIELD CALCULATION                                    
C                                                                       
      GOTO 14
   78 IF( IFAR.EQ.-1) GOTO 113
      PINR= PIN
      PNLR= PNLS
      CALL RDPAT
  113 IF( IXTYP.EQ.0.OR. IXTYP.GE.4) GOTO 119
      NTHIC= NTHIC+1
      INC= INC+1
      XPR1= XPR1+ XPR4
      IF( NTHIC.LE. NTHI) GOTO 54
      NTHIC=1
      XPR1= THETIS
      XPR2= XPR2+ XPR5
      NPHIC= NPHIC+1
      IF( NPHIC.LE. NPHI) GOTO 54
      NPHIC=1
      XPR2= PHISS
C     NORMALIZED RECEIVING PATTERN PRINTED                              
      IF( IPTFLG.LT.2) GOTO 119
      ITMP1= NTHI* NPHI
      IF( ITMP1.LE. NORMF) GOTO 114
      ITMP1= NORMF
      WRITE (2,181) 
  114 TMP1= FNORM(1)
      DO 115  J=2, ITMP1
      IF( FNORM( J).GT. TMP1) TMP1= FNORM( J)
  115 CONTINUE
      WRITE (2,182)  TMP1, XPR3, HPOL( IXTYP), XPR6, ISAVE
      DO 118  J=1, NPHI
      ITMP2= NTHI*( J-1)
      DO 116  I=1, NTHI
      ITMP3= I+ ITMP2
      IF( ITMP3.GT. ITMP1) GOTO 117
      TMP2= FNORM( ITMP3)/ TMP1
      TMP3= DB20( TMP2)
      WRITE (2,183)  XPR1, XPR2, TMP3, TMP2
      XPR1= XPR1+ XPR4
  116 CONTINUE
  117 XPR1= THETIS
      XPR2= XPR2+ XPR5
  118 CONTINUE
      XPR2= PHISS
  119 IF( MHZ.EQ. NFRQ) IFAR=-1
      IF( NFRQ.NE.1) GOTO 120
      WRITE (2,135) 
      GOTO 14
  120 MHZ= MHZ+1
      IF( MHZ.LE. NFRQ) GOTO 42
      IF( IPED.EQ.0) GOTO 123
      IF( NVQD.LT.1) GOTO 199
      WRITE (2,184)  IVQD( NVQD), ZPNORM
      GOTO 204
  199 WRITE (2,184)  ISANT( NSANT), ZPNORM
  204 ITMP1= NFRQ
      IF( ITMP1.LE.( NORMF/4)) GOTO 121
      ITMP1= NORMF/4
      WRITE (2,185) 
  121 IF( IFRQ.EQ.0) TMP1= FMHZ-( NFRQ-1)* DELFRQ
      IF( IFRQ.EQ.1) TMP1= FMHZ/( DELFRQ**( NFRQ-1))
      DO 122  I=1, ITMP1
      ITMP2= I+4*( I-1)
      TMP2= FNORM( ITMP2)/ ZPNORM
      TMP3= FNORM( ITMP2+1)/ ZPNORM
      TMP4= FNORM( ITMP2+2)/ ZPNORM
      TMP5= FNORM( ITMP2+3)
      WRITE (2,186)  TMP1, FNORM( ITMP2), FNORM( ITMP2+1), FNORM( ITMP2
     &+2), FNORM( ITMP2+3), TMP2, TMP3, TMP4, TMP5
      IF( IFRQ.EQ.0) TMP1= TMP1+ DELFRQ
      IF( IFRQ.EQ.1) TMP1= TMP1* DELFRQ
  122 CONTINUE
      WRITE (2,135) 
  123 CONTINUE
      NFRQ=1
      MHZ=1
      GOTO 14
  125 FORMAT(A2,19A4)
  126 FORMAT('1')
  127 FORMAT(///,33X,'************************************',//,36X,
     &'NUMERICAL ELECTROMAGNETICS CODE',//,33X,
     &'************************************')
  128 FORMAT(////,37X,'- - - - COMMENTS - - - -',//)
C  129 FORMAT(25X,20A4)
  129 FORMAT(' ', 20A4)
  130 FORMAT(///,10X,'INCORRECT LABEL FOR A COMMENT CARD')
  135 FORMAT(/////)
  136 FORMAT(A2,I3,3I5,6E10.3)
  137 FORMAT(1X,'***** DATA CARD NO.',I3,3X,A2,1X,I3,3(1X,I5),6(1X,1P,E
     &12.5))
  138 FORMAT(///,10X,'FAULTY DATA CARD LABEL AFTER GEOMETRY SECTION')
  139 FORMAT(///,10X,'NUMBER OF LOADING CARDS EXCEEDS STORAGE ALLOTTED'
     &)
  140 FORMAT(///,10X,'DATA FAULT ON LOADING CARD NO.=',I5,5X,'ITAG S',
     &'TEP1=',I5,'  IS GREATER THAN ITAG STEP2=',I5)
  141 FORMAT(///,10X,'NUMBER OF EXCITATION CARDS EXCEEDS STORAGE ALLO',
     &'TTED')
  142 FORMAT(///,10X,'NUMBER OF NETWORK CARDS EXCEEDS STORAGE ALLOTTED'
     &)
  143 FORMAT(///,10X,'WHEN MULTIPLE FREQUENCIES ARE REQUESTED, ONLY ONE 
     & NEAR FIELD CARD CAN BE USED -',/,10X,'LAST CARD READ IS USED')
  145 FORMAT(////,33X,'- - - - - - FREQUENCY - - - - - -',//,36X,'FR',
     &'EQUENCY=',1P,E11.4,' MHZ',/,36X,'WAVELENGTH=',E11.4,' METERS')
  146 FORMAT(///,30X,' - - - STRUCTURE IMPEDANCE LOADING - - -')
  147 FORMAT(/,35X,'THIS STRUCTURE IS NOT LOADED')
  148 FORMAT(///,34X,'- - - ANTENNA ENVIRONMENT - - -',/)
  149 FORMAT(40X,'MEDIUM UNDER SCREEN -')
  150 FORMAT(40X,'RELATIVE DIELECTRIC CONST.=',F7.3,/,40X,'CONDUCTIV',
     &'ITY=',1P,E10.3,' MHOS/METER',/,40X,
     &'COMPLEX DIELECTRIC CONSTANT=',2E12.5)
  151 FORMAT(42X,'PERFECT GROUND')
  152 FORMAT(44X,'FREE SPACE')
  153 FORMAT(///,32X,'- - - MATRIX TIMING - - -',//,24X,'FILL=',F9.3,
     &' SEC.,  FACTOR=',F9.3,' SEC.')
  154 FORMAT(///,40X,'- - - EXCITATION - - -')
  155 FORMAT(/,4X,'PLANE WAVE',4X,'THETA=',F7.2,' DEG,  PHI=',F7.2,
     &' DEG,  ETA=',F7.2,' DEG,  TYPE -',A6,'=  AXIAL RATIO=',F6.3)
  156 FORMAT(/,31X,'POSITION (METERS)',14X,'ORIENTATION (DEG)=/',28X,
     &'X',12X,'Y',12X,'Z',10X,'ALPHA',5X,'BETA',4X,'DIPOLE MOMENT',//,4
     &X,'CURRENT SOURCE',1X,3(3X,F10.5),1X,2(3X,F7.2),4X,F8.3)
  157 FORMAT(4X,4(I5,1X),1P,6(3X,E11.4),3X,A6,A2)
  158 FORMAT(///,44X,'- - - NETWORK DATA - - -')
  159 FORMAT(/,6X,'- FROM -    - TO -',11X,'TRANSMISSION LINE',15X,
     &'-  -  SHUNT ADMITTANCES (MHOS)  -  -',14X,'LINE',/,6X,
     &'TAG  SEG.','   TAG  SEG.',6X,'IMPEDANCE',6X,'LENGTH',12X,
     &'- END ONE -',17X,'- END TWO -',12X,'TYPE',/,6X,
     &'NO.   NO.   NO.   NO.',9X,'OHM''S',8X,'METERS',9X,'REAL',10X,
     &'IMAG.',9X,'REAL',10X,'IMAG.')
  160 FORMAT(/,6X,'- FROM -',4X,'- TO -',26X,'-  -  ADMITTANCE MATRIX',
     &' ELEMENTS (MHOS)  -  -',/,6X,'TAG  SEG.   TAG  SEG.',13X,'(ON',
     &'E,ONE)',19X,'(ONE,TWO)',19X,'(TWO,TWO)',/,6X,'NO.   NO.   NO.',
     &'   NO.',8X,'REAL',10X,'IMAG.',9X,'REAL',10X,'IMAG.',9X,'REAL',10
     &X,'IMAG.')
  161 FORMAT(///,29X,'- - - CURRENTS AND LOCATION - - -',//,33X,'DIS',
     &'TANCES IN WAVELENGTHS')
  162 FORMAT(//,2X,'SEG.',2X,'TAG',4X,'COORD. OF SEG. CENTER',5X,'SEG.'
     &,12X,'- - - CURRENT (AMPS) - - -',/,2X,'NO.',3X,'NO.',5X,'X',8X,
     &'Y',8X,'Z',6X,'LENGTH',5X,'REAL',8X,'IMAG.',7X,'MAG.',8X,'PHASE')
  163 FORMAT(///,33X,'- - - RECEIVING PATTERN PARAMETERS - - -',/,43X,
     &'ETA=',F7.2,' DEGREES',/,43X,'TYPE -',A6,/,43X,'AXIAL RATIO=',F6.
     &3,//,11X,'THETA',6X,'PHI',10X,'-  CURRENT  -',9X,'SEG',/,11X,
     &'(DEG)',5X,'(DEG)',7X,'MAGNITUDE',4X,'PHASE',6X,'NO.',/)
  164 FORMAT(10X,2(F7.2,3X),1X,1P,E11.4,3X,0P,F7.2,4X,I5)
  165 FORMAT(1X,2I5,3F9.4,F9.5,1X,1P,3E12.4,0P,F9.3)
  166 FORMAT(///,40X,'- - - POWER BUDGET - - -',//,43X,'INPUT PO',
     &'WER   =',1P,E11.4,' WATTS',/,43X,'RADIATED POWER=',E11.4,
     &' WATTS',/,43X,'STRUCTURE LOSS=',E11.4,' WATTS',/,43X,
     &'NETWORK LOSS  =',E11.4,' WATTS',/,43X,'EFFICIENCY    =',0P,F7.2,
     &' PERCENT')
  170 FORMAT(40X,'RADIAL WIRE GROUND SCREEN',/,40X,I5,' WIRES',/,40X,
     &'WIRE LENGTH=',F8.2,' METERS',/,40X,'WIRE RADIUS=',1P,E10.3,
     &' METERS')
  181 FORMAT(///,4X,'RECEIVING PATTERN STORAGE TOO SMALL,ARRAY TRUNCA',
     &'TED')
  182 FORMAT(///,32X,'- - - NORMALIZED RECEIVING PATTERN - - -',/,41X,
     &'NORMALIZATION FACTOR=',1P,E11.4,/,41X,'ETA=',0P,F7.2,' DEGREES',
     &/,41X,'TYPE -',A6,/,41X,'AXIAL RATIO=',F6.3,/,41X,'SEGMENT NO.=',
     &I5,//,21X,'THETA',6X,'PHI',9X,'-  PATTERN  -',/,21X,'(DEG)',5X,
     &'(DEG)',8X,'DB',8X,'MAGNITUDE',/)
  183 FORMAT(20X,2(F7.2,3X),1X,F7.2,4X,1P,E11.4)
  184 FORMAT(///,36X,'- - - INPUT IMPEDANCE DATA - - -',/,45X,'SO',
     &'URCE SEGMENT NO.',I4,/,45X,'NORMALIZATION FACTOR=',1P,E12.5,//,7
     &X,'FREQ.',13X,'-  -  UNNORMALIZED IMPEDANCE  -  -',21X,'-',
     &' -  NORMALIZED IMPEDANCE  -  -',/,19X,'RESISTANCE',4X,'REACTA',
     &'NCE',6X,'MAGNITUDE',4X,'PHASE',7X,'RESISTANCE',4X,'REACTANCE',6X
     &,'MAGNITUDE',4X,'PHASE',/,8X,'MHZ',11X,'OHMS',10X,'OHMS',11X,
     &'OHMS',5X,'DEGREES',47X,'DEGREES',/)
  185 FORMAT(///,4X,'STORAGE FOR IMPEDANCE NORMALIZATION TOO SMALL, A',
     &'RRAY TRUNCATED')
  186 FORMAT(3X,F9.3,2X,1P,2(2X,E12.5),3X,E12.5,2X,0P,F7.2,2X,1P,2(2X,E
     &12.5),3X,E12.5,2X,0P,F7.2)
  196 FORMAT(////,20X,'APPROXIMATE INTEGRATION EMPLOYED FOR SEGMENT',
     &'S MORE THAN',F8.3,' WAVELENGTHS APART')
  197 FORMAT(////,41X,'- - - - SURFACE PATCH CURRENTS - - - -',//,50X,
     &'DISTANCE IN WAVELENGTHS',/,50X,'CURRENT IN AMPS/METER',//,28X,
     &'- - SURFACE COMPONENTS - -',19X,'- - - RECTANGULAR COM',
     &'PONENTS - - -',/,6X,'PATCH CENTER',6X,'TANGENT VECTOR 1',3X,
     &'TANGENT VECTOR 2',11X,'X',19X,'Y',19X,'Z',/,5X,'X',6X,'Y',6X,'Z'
     &,5X,'MAG.',7X,'PHASE',3X,'MAG.',7X,'PHASE',3(4X,'REAL',6X,'IMAG.'
     &))
  198 FORMAT(1X,I4,/,1X,3F7.3,2(1P,E11.4,0P,F8.2),1P,6E10.2)
  201 FORMAT(/,' RUN TIME =',F10.3)
  315 FORMAT(///,34X,'- - - CHARGE DENSITIES - - -',//,36X,
     &'DISTANCES IN WAVELENGTHS',///,2X,'SEG.',2X,'TAG',4X,
     &'COORD. OF SEG. CENTER',5X,'SEG.',10X,
     &'CHARGE DENSITY (COULOMBS/METER)',/,2X,'NO.',3X,'NO.',5X,'X',8X,
     &'Y',8X,'Z',6X,'LENGTH',5X,'REAL',8X,'IMAG.',7X,'MAG.',8X,'PHASE')
     &
  321 FORMAT(/,20X,'THE EXTENDED THIN WIRE KERNEL WILL BE USED')
  303 FORMAT(/,' ERROR - ',A2,' CARD IS NOT ALLOWED WITH N.G.F.')
  327 FORMAT(/,35X,' LOADING ONLY IN N.G.F. SECTION')
  302 FORMAT(' ERROR - N.G.F. IN USE.  CANNOT WRITE NEW N.G.F.')
  313 FORMAT(/,' NUMBER OF SEGMENTS IN COUPLING CALCULATION (CP) EXCEE'
     &,'DS LIMIT')
  390 FORMAT(' RADIAL WIRE G. S. APPROXIMATION MAY NOT BE USED WITH SO'
     &,'MMERFELD GROUND OPTION')
  391 FORMAT(40X,'FINITE GROUND.  REFLECTION COEFFICIENT APPROXIMATION'
     &)
  392 FORMAT(40X,'FINITE GROUND.  SOMMERFELD SOLUTION')
  393 FORMAT(/,' ERROR IN GROUND PARAMETERS -',/,' COMPLEX DIELECTRIC',
     &' CONSTANT FROM FILE IS',1P,2E12.5,/,32X,'REQUESTED',2E12.5)
      END
C ***
C     DOUBLE PRECISION 6/4/85
C
      SUBROUTINE ARC( ITG, NS, RADA, ANG1, ANG2, RAD)
C ***
C                                                                       
C     ARC GENERATES SEGMENT GEOMETRY DATA FOR AN ARC OF NS SEGMENTS     
C                                                                       
      IMPLICIT REAL (A-H,O-Z)
      PARAMETER ( NM=600, N2M=800, N3M=1000)
      COMMON  /DATA/ LD, N1, N2, N, NP, M1, M2, M, MP, X( NM), Y( NM), 
     &Z( NM), SI( NM), BI( NM), ALP( NM), BET( NM), ICON1( N2M), ICON2(
     & N2M), ITAG( N2M), ICONX( NM), WLAM, IPSYM
      DIMENSION  X2(1), Y2(1), Z2(1)
      EQUIVALENCE(X2,SI),(Y2,ALP),(Z2,BET)
      DATA   TA/.01745329252D+0/
      IST= N+1
      N= N+ NS
      NP= N
      MP= M
      IPSYM=0
      IF( NS.LT.1) RETURN
      IF( ABS( ANG2- ANG1).LT.360.00001D+0) GOTO 1
      WRITE (2,3) 
      STOP
    1 ANG= ANG1* TA
      DANG=( ANG2- ANG1)* TA/ NS
      XS1= RADA* COS( ANG)
      ZS1= RADA* SIN( ANG)
      DO 2  I= IST, N
      ANG= ANG+ DANG
      XS2= RADA* COS( ANG)
      ZS2= RADA* SIN( ANG)
      X( I)= XS1
      Y( I)=0.
      Z( I)= ZS1
      X2( I)= XS2
      Y2( I)=0.
      Z2( I)= ZS2
      XS1= XS2
      ZS1= ZS2
      BI( I)= RAD
    2 ITAG( I)= ITG
C                                                                       
      RETURN
    3 FORMAT(' ERROR -- ARC ANGLE EXCEEDS 360. DEGREES')
      END
C ***
C     DOUBLE PRECISION 6/4/85
C
      FUNCTION ATGN2( X, Y)
C ***
C                                                                       
C     ATGN2 IS ARCTANGENT FUNCTION MODIFIED TO RETURN 0. WHEN X=Y=0.    
C                                                                       
      IMPLICIT REAL (A-H,O-Z)
      IF( X) 3,1,3
    1 IF( Y) 3,2,3
    2 ATGN2=0.
      RETURN
    3 ATGN2= ATAN2( X, Y)
      RETURN
      END
C ***
C     DOUBLE PRECISION 6/4/85
C
      SUBROUTINE BLCKOT( AR, NUNIT, IX1, IX2, NBLKS, NEOF)
C ***
C                                                                       
C     BLCKOT CONTROLS THE READING AND WRITING OF MATRIX BLOCKS ON FILES 
C     FOR THE OUT-OF-CORE MATRIX SOLUTION.                              
C                                                                       
      IMPLICIT REAL (A-H,O-Z)
C      LOGICAL  ENF
      COMPLEX  AR
      DIMENSION  AR(1000)
      I1=( IX1+1)/2
      I2=( IX2+1)/2
    1 WRITE( NUNIT) ( AR( J), J= I1, I2)
      RETURN
      ENTRY BLCKIN( AR, NUNIT, IX1, IX2, NBLKS, NEOF)
      I1=( IX1+1)/2
      I2=( IX2+1)/2
      DO 2  I=1, NBLKS
C     IF (ENF(NUNIT)) GO TO 3                                           
      READ( NUNIT,END=3) ( AR( J), J= I1, I2)
    2 CONTINUE
      RETURN
    3 WRITE (2,4)  NUNIT, NBLKS, NEOF
      IF( NEOF.NE.777) STOP
      NEOF=0
C                                                                       
      RETURN
    4 FORMAT('  EOF ON UNIT',I3,'  NBLKS= ',I3,'  NEOF= ',I5)
      END
C ***
C     DOUBLE PRECISION 6/4/85
C
      SUBROUTINE CABC( CURX)
C ***
C                                                                       
C     CABC COMPUTES COEFFICIENTS OF THE CONSTANT (A), SINE (B), AND     
C     COSINE (C) TERMS IN THE CURRENT INTERPOLATION FUNCTIONS FOR THE   
C     CURRENT VECTOR CUR.                                               
C                                                                       
      IMPLICIT REAL (A-H,O-Z)
      PARAMETER ( NM=600, N2M=800, N3M=1000)
      COMPLEX  CUR, CURX, VQDS, CURD, CCJ, VSANT, VQD, CS1, CS2
      COMMON  /DATA/ LD, N1, N2, N, NP, M1, M2, M, MP, X( NM), Y( NM), 
     &Z( NM), SI( NM), BI( NM), ALP( NM), BET( NM), ICON1( N2M), ICON2(
     & N2M), ITAG( N2M), ICONX( NM), WLAM, IPSYM
      COMMON  /CRNT/ AIR( NM), AII( NM), BIR( NM), BII( NM), CIR( NM), 
     &CII( NM), CUR( N3M)
      COMMON  /SEGJ/ AX(30), BX(30), CX(30), JCO(30), JSNO, ISCON(50), 
     &NSCON, IPCON(10), NPCON
      COMMON  /VSORC/ VQD(30), VSANT(30), VQDS(30), IVQD(30), ISANT(30)
     &, IQDS(30), NVQD, NSANT, NQDS
      COMMON  /ANGL/ SALP( NM)
      DIMENSION  T1X(1), T1Y(1), T1Z(1), T2X(1), T2Y(1), T2Z(1)
      DIMENSION  CURX(1), CCJX(2)
      EQUIVALENCE(T1X,SI),(T1Y,ALP),(T1Z,BET),(T2X,ICON1),(T2Y,ICON2),(
     &T2Z,ITAG)
      EQUIVALENCE(CCJ,CCJX)
      DATA   TP/6.283185308D+0/, CCJX/0.,-0.01666666667D+0/
      IF( N.EQ.0) GOTO 6
      DO 1  I=1, N
      AIR( I)=0.
      AII( I)=0.
      BIR( I)=0.
      BII( I)=0.
      CIR( I)=0.
    1 CII( I)=0.
      DO 2  I=1, N
      AR= REAL( CURX( I))
      AI= AIMAG( CURX( I))
      CALL TBF( I,1)
      DO 2  JX=1, JSNO
      J= JCO( JX)
      AIR( J)= AIR( J)+ AX( JX)* AR
      AII( J)= AII( J)+ AX( JX)* AI
      BIR( J)= BIR( J)+ BX( JX)* AR
      BII( J)= BII( J)+ BX( JX)* AI
      CIR( J)= CIR( J)+ CX( JX)* AR
    2 CII( J)= CII( J)+ CX( JX)* AI
      IF( NQDS.EQ.0) GOTO 4
      DO 3  IS=1, NQDS
      I= IQDS( IS)
      JX= ICON1( I)
      ICON1( I)=0
      CALL TBF( I,0)
      ICON1( I)= JX
      SH= SI( I)*.5
      CURD= CCJ* VQDS( IS)/(( LOG(2.* SH/ BI( I))-1.)*( BX( JSNO)* COS(
     & TP* SH)+ CX( JSNO)* SIN( TP* SH))* WLAM)
      AR= REAL( CURD)
      AI= AIMAG( CURD)
      DO 3  JX=1, JSNO
      J= JCO( JX)
      AIR( J)= AIR( J)+ AX( JX)* AR
      AII( J)= AII( J)+ AX( JX)* AI
      BIR( J)= BIR( J)+ BX( JX)* AR
      BII( J)= BII( J)+ BX( JX)* AI
      CIR( J)= CIR( J)+ CX( JX)* AR
    3 CII( J)= CII( J)+ CX( JX)* AI
    4 DO 5  I=1, N
    5 CURX( I)= CMPLX( AIR( I)+ CIR( I), AII( I)+ CII( I))
C     CONVERT SURFACE CURRENTS FROM T1,T2 COMPONENTS TO X,Y,Z COMPONENTS
    6 IF( M.EQ.0) RETURN
      K= LD- M
      JCO1= N+2* M+1
      JCO2= JCO1+ M
      DO 7  I=1, M
      K= K+1
      JCO1= JCO1-2
      JCO2= JCO2-3
      CS1= CURX( JCO1)
      CS2= CURX( JCO1+1)
      CURX( JCO2)= CS1* T1X( K)+ CS2* T2X( K)
      CURX( JCO2+1)= CS1* T1Y( K)+ CS2* T2Y( K)
    7 CURX( JCO2+2)= CS1* T1Z( K)+ CS2* T2Z( K)
      RETURN
      END
C ***
C     DOUBLE PRECISION 6/4/85
C
      FUNCTION CANG( Z)
C ***
C                                                                       
C     CANG RETURNS THE PHASE ANGLE OF A COMPLEX NUMBER IN DEGREES.      
C                                                                       
      IMPLICIT REAL (A-H,O-Z)
      COMPLEX  Z
      CANG= ATGN2( AIMAG( Z), REAL( Z))*57.29577951D+0
      RETURN
      END
C ***
C     DOUBLE PRECISION 6/4/85
C
      SUBROUTINE CMNGF( CB, CC, CD, NB, NC, ND, RKHX, IEXKX)
C ***
      IMPLICIT REAL (A-H,O-Z)
C     CMNGF FILLS INTERACTION MATRICIES B, C, AND D FOR N.G.F. SOLUTION 
      PARAMETER ( NM=600, N2M=800, N3M=1000)
      COMPLEX  CB, CC, CD, ZARRAY, EXK, EYK, EZK, EXS, EYS, EZS, EXC
     &, EYC, EZC
      COMMON  /DATA/ LD, N1, N2, N, NP, M1, M2, M, MP, X( NM), Y( NM), 
     &Z( NM), SI( NM), BI( NM), ALP( NM), BET( NM), ICON1( N2M), ICON2(
     & N2M), ITAG( N2M), ICONX( NM), WLAM, IPSYM
      COMMON  /ZLOAD/ ZARRAY( NM), NLOAD, NLODF
      COMMON  /SEGJ/ AX(30), BX(30), CX(30), JCO(30), JSNO, ISCON(50), 
     &NSCON, IPCON(10), NPCON
      COMMON  /DATAJ/ S, B, XJ, YJ, ZJ, CABJ, SABJ, SALPJ, EXK, EYK, 
     &EZK, EXS, EYS, EZS, EXC, EYC, EZC, RKH, IEXK, IND1, INDD1, IND2, 
     &INDD2, IPGND
      COMMON  /MATPAR/ ICASE, NBLOKS, NPBLK, NLAST, NBLSYM, NPSYM, 
     &NLSYM, IMAT, ICASX, NBBX, NPBX, NLBX, NBBL, NPBL, NLBL
      DIMENSION  CB( NB,1), CC( NC,1), CD( ND,1)
      RKH= RKHX
      IEXK= IEXKX
      M1EQ=2* M1
      M2EQ= M1EQ+1
      MEQ=2* M
      NEQP= ND- NPCON*2
      NEQS= NEQP- NSCON
      NEQSP= NEQS+ NC
      NEQN= NC+ N- N1
      ITX=1
      IF( NSCON.GT.0) ITX=2
      IF( ICASX.EQ.1) GOTO 1
      REWIND 12
      REWIND 14
      REWIND 15
      IF( ICASX.GT.2) GOTO 5
    1 DO 4  J=1, ND
      DO 2  I=1, ND
    2 CD( I, J)=(0.,0.)
      DO 3  I=1, NB
      CB( I, J)=(0.,0.)
    3 CC( I, J)=(0.,0.)
    4 CONTINUE
    5 IST= N- N1+1
      IT= NPBX
C     LOOP THRU 24 FILLS B.  FOR ICASX=1 OR 2 ALSO FILLS D(WW), D(WS)   
      ISV=- NPBX
      DO 24  IBLK=1, NBBX
      ISV= ISV+ NPBX
      IF( IBLK.EQ. NBBX) IT= NLBX
      IF( ICASX.LT.3) GOTO 7
      DO 6  J=1, ND
      DO 6  I=1, IT
    6 CB( I, J)=(0.,0.)
    7 I1= ISV+1
      I2= ISV+ IT
      IN2= I2
      IF( IN2.GT. N1) IN2= N1
      IM1= I1- N1
      IM2= I2- N1
      IF( IM1.LT.1) IM1=1
      IMX=1
      IF( I1.LE. N1) IMX= N1- I1+2
C     FILL B(WW),B(WS).  FOR ICASX=1,2 FILL D(WW),D(WS)                 
      IF( N2.GT. N) GOTO 12
      DO 11  J= N2, N
      CALL TRIO( J)
      DO 9  I=1, JSNO
      JSS= JCO( I)
C     SET JCO WHEN SOURCE IS NEW BASIS FUNCTION ON NEW SEGMENT          
      IF( JSS.LT. N2) GOTO 8
      JCO( I)= JSS- N1
C     SOURCE IS PORTION OF MODIFIED BASIS FUNCTION ON NEW SEGMENT       
      GOTO 9
    8 JCO( I)= NEQS+ ICONX( JSS)
    9 CONTINUE
      IF( I1.LE. IN2) CALL CMWW( J, I1, IN2, CB, NB, CB, NB,0)
      IF( IM1.LE. IM2) CALL CMWS( J, IM1, IM2, CB( IMX,1), NB, CB, NB,0
     &)
      IF( ICASX.GT.2) GOTO 11
      CALL CMWW( J, N2, N, CD, ND, CD, ND,1)
C     LOADING IN D(WW)                                                  
      IF( M2.LE. M) CALL CMWS( J, M2EQ, MEQ, CD(1, IST), ND, CD, ND,1)
      IF( NLOAD.EQ.0) GOTO 11
      IR= J- N1
      EXK= ZARRAY( J)
      DO 10  I=1, JSNO
      JSS= JCO( I)
   10 CD( JSS, IR)= CD( JSS, IR)-( AX( I)+ CX( I))* EXK
   11 CONTINUE
C     FILL B(WW)PRIME                                                   
   12 IF( NSCON.EQ.0) GOTO 20
      DO 19  I=1, NSCON
C     SOURCES ARE NEW OR MODIFIED BASIS FUNCTIONS ON OLD SEGMENTS WHICH 
C     CONNECT TO NEW SEGMENTS                                           
      J= ISCON( I)
      CALL TRIO( J)
      JSS=0
      DO 15  IX=1, JSNO
      IR= JCO( IX)
      IF( IR.LT. N2) GOTO 13
      IR= IR- N1
      GOTO 14
   13 IR= ICONX( IR)
      IF( IR.EQ.0) GOTO 15
      IR= NEQS+ IR
   14 JSS= JSS+1
      JCO( JSS)= IR
      AX( JSS)= AX( IX)
      BX( JSS)= BX( IX)
      CX( JSS)= CX( IX)
   15 CONTINUE
      JSNO= JSS
      IF( I1.LE. IN2) CALL CMWW( J, I1, IN2, CB, NB, CB, NB,0)
C     SOURCE IS SINGULAR COMPONENT OF PATCH CURRENT THAT IS PART OF     
C     MODIFIED BASIS FUNCTION FOR OLD SEGMENT THAT CONNECTS TO A NEW    
C     SEGMENT ON END OPPOSITE PATCH.                                    
      IF( IM1.LE. IM2) CALL CMWS( J, IM1, IM2, CB( IMX,1), NB, CB, NB,0
     &)
      IF( I1.LE. IN2) CALL CMSW( J, I, I1, IN2, CB, CB,0, NB,-1)
      IF( NLODF.EQ.0) GOTO 17
      JX= J- ISV
      IF( JX.LT.1.OR. JX.GT. IT) GOTO 17
      EXK= ZARRAY( J)
      DO 16  IX=1, JSNO
      JSS= JCO( IX)
C     SOURCES ARE PORTIONS OF MODIFIED BASIS FUNCTION J ON OLD SEGMENTS 
C     EXCLUDING OLD SEGMENTS THAT DIRECTLY CONNECT TO NEW SEGMENTS.     
   16 CB( JX, JSS)= CB( JX, JSS)-( AX( IX)+ CX( IX))* EXK
   17 CALL TBF( J,1)
      JSX= JSNO
      JSNO=1
      IR= JCO(1)
      JCO(1)= NEQS+ I
      DO 19  IX=1, JSX
      IF( IX.EQ.1) GOTO 18
      IR= JCO( IX)
      AX(1)= AX( IX)
      BX(1)= BX( IX)
      CX(1)= CX( IX)
   18 IF( IR.GT. N1) GOTO 19
      IF( ICONX( IR).NE.0) GOTO 19
      IF( I1.LE. IN2) CALL CMWW( IR, I1, IN2, CB, NB, CB, NB,0)
C     LOADING FOR B(WW)PRIME                                            
      IF( IM1.LE. IM2) CALL CMWS( IR, IM1, IM2, CB( IMX,1), NB, CB, NB,
     &0)
      IF( NLODF.EQ.0) GOTO 19
      JX= IR- ISV
      IF( JX.LT.1.OR. JX.GT. IT) GOTO 19
      EXK= ZARRAY( IR)
      JSS= JCO(1)
      CB( JX, JSS)= CB( JX, JSS)-( AX(1)+ CX(1))* EXK
   19 CONTINUE
   20 IF( NPCON.EQ.0) GOTO 22
C     FILL B(SS)PRIME TO SET OLD PATCH BASIS FUNCTIONS TO ZERO FOR      
C     PATCHES THAT CONNECT TO NEW SEGMENTS                              
      JSS= NEQP
      DO 21  I=1, NPCON
      IX= IPCON( I)*2+ N1- ISV
      IR= IX-1
      JSS= JSS+1
      IF( IR.GT.0.AND. IR.LE. IT) CB( IR, JSS)=(1.,0.)
      JSS= JSS+1
      IF( IX.GT.0.AND. IX.LE. IT) CB( IX, JSS)=(1.,0.)
   21 CONTINUE
C     FILL B(SW) AND B(SS)                                              
   22 IF( M2.GT. M) GOTO 23
      IF( I1.LE. IN2) CALL CMSW( M2, M, I1, IN2, CB(1, IST), CB, N1, NB
     &,0)
      IF( IM1.LE. IM2) CALL CMSS( M2, M, IM1, IM2, CB( IMX, IST), NB,0)
     &
   23 IF( ICASX.EQ.1) GOTO 24
      WRITE( 14) (( CB( I, J), I=1, IT), J=1, ND)
C     FILLING B COMPLETE.  START ON C AND D                             
   24 CONTINUE
      IT= NPBL
      ISV=- NPBL
      DO 43  IBLK=1, NBBL
      ISV= ISV+ NPBL
      ISVV= ISV+ NC
      IF( IBLK.EQ. NBBL) IT= NLBL
      IF( ICASX.LT.3) GOTO 27
      DO 26  J=1, IT
      DO 25  I=1, NC
   25 CC( I, J)=(0.,0.)
      DO 26  I=1, ND
   26 CD( I, J)=(0.,0.)
   27 I1= ISVV+1
      I2= ISVV+ IT
      IN1= I1- M1EQ
      IN2= I2- M1EQ
      IF( IN2.GT. N) IN2= N
      IM1= I1- N
      IM2= I2- N
      IF( IM1.LT. M2EQ) IM1= M2EQ
      IF( IM2.GT. MEQ) IM2= MEQ
      IMX=1
      IF( IN1.LE. IN2) IMX= NEQN- I1+2
      IF( ICASX.LT.3) GOTO 32
C     SAME AS DO 24 LOOP TO FILL D(WW) FOR ICASX GREATER THAN 2         
      IF( N2.GT. N) GOTO 32
      DO 31  J= N2, N
      CALL TRIO( J)
      DO 29  I=1, JSNO
      JSS= JCO( I)
      IF( JSS.LT. N2) GOTO 28
      JCO( I)= JSS- N1
      GOTO 29
   28 JCO( I)= NEQS+ ICONX( JSS)
   29 CONTINUE
      IF( IN1.LE. IN2) CALL CMWW( J, IN1, IN2, CD, ND, CD, ND,1)
      IF( IM1.LE. IM2) CALL CMWS( J, IM1, IM2, CD(1, IMX), ND, CD, ND,1
     &)
      IF( NLOAD.EQ.0) GOTO 31
      IR= J- N1- ISV
      IF( IR.LT.1.OR. IR.GT. IT) GOTO 31
      EXK= ZARRAY( J)
      DO 30  I=1, JSNO
      JSS= JCO( I)
   30 CD( JSS, IR)= CD( JSS, IR)-( AX( I)+ CX( I))* EXK
   31 CONTINUE
C     FILL D(SW) AND D(SS)                                              
   32 IF( M2.GT. M) GOTO 33
      IF( IN1.LE. IN2) CALL CMSW( M2, M, IN1, IN2, CD( IST,1), CD, N1, 
     &ND,1)
      IF( IM1.LE. IM2) CALL CMSS( M2, M, IM1, IM2, CD( IST, IMX), ND,1)
     &
C     FILL C(WW),C(WS), D(WW)PRIME, AND D(WS)PRIME.                     
   33 IF( N1.LT.1) GOTO 39
      DO 37  J=1, N1
      CALL TRIO( J)
      IF( NSCON.EQ.0) GOTO 36
      DO 35  IX=1, JSNO
      JSS= JCO( IX)
      IF( JSS.LT. N2) GOTO 34
      JCO( IX)= JSS+ M1EQ
      GOTO 35
   34 IR= ICONX( JSS)
      IF( IR.NE.0) JCO( IX)= NEQSP+ IR
   35 CONTINUE
   36 IF( IN1.LE. IN2) CALL CMWW( J, IN1, IN2, CC, NC, CD, ND, ITX)
      IF( IM1.LE. IM2) CALL CMWS( J, IM1, IM2, CC(1, IMX), NC, CD(1, 
     &IMX), ND, ITX)
   37 CONTINUE
C     FILL C(WW)PRIME                                                   
      IF( NSCON.EQ.0) GOTO 39
      DO 38  IX=1, NSCON
      IR= ISCON( IX)
      JSS= NEQS+ IX- ISV
      IF( JSS.GT.0.AND. JSS.LE. IT) CC( IR, JSS)=(1.,0.)
   38 CONTINUE
   39 IF( NPCON.EQ.0) GOTO 41
C     FILL C(SS)PRIME                                                   
      JSS= NEQP- ISV
      DO 40  I=1, NPCON
      IX= IPCON( I)*2+ N1
      IR= IX-1
      JSS= JSS+1
      IF( JSS.GT.0.AND. JSS.LE. IT) CC( IR, JSS)=(1.,0.)
      JSS= JSS+1
      IF( JSS.GT.0.AND. JSS.LE. IT) CC( IX, JSS)=(1.,0.)
   40 CONTINUE
C     FILL C(SW) AND C(SS)                                              
   41 IF( M1.LT.1) GOTO 42
      IF( IN1.LE. IN2) CALL CMSW(1, M1, IN1, IN2, CC( N2,1), CC,0, NC,1
     &)
      IF( IM1.LE. IM2) CALL CMSS(1, M1, IM1, IM2, CC( N2, IMX), NC,1)
   42 CONTINUE
      IF( ICASX.EQ.1) GOTO 43
      WRITE( 12) (( CD( J, I), J=1, ND), I=1, IT)
      WRITE( 15) (( CC( J, I), J=1, NC), I=1, IT)
   43 CONTINUE
      IF( ICASX.EQ.1) RETURN
      REWIND 12
      REWIND 14
      REWIND 15
      RETURN
      END
C ***
C     DOUBLE PRECISION 6/4/85
C
      SUBROUTINE CMSET( NROW, CM, RKHX, IEXKX)
C ***
      IMPLICIT REAL (A-H,O-Z)
C                                                                       
C     CMSET SETS UP THE COMPLEX STRUCTURE MATRIX IN THE ARRAY CM        
C                                                                       
      PARAMETER ( NM=600, N2M=800, N3M=1000)
      COMPLEX  CM, ZARRAY, ZAJ, EXK, EYK, EZK, EXS, 
     &EYS, EZS, EXC, EYC, EZC, SSX, D, DETER
      COMMON  /DATA/ LD, N1, N2, N, NP, M1, M2, M, MP, X( NM), Y( NM), 
     &Z( NM), SI( NM), BI( NM), ALP( NM), BET( NM), ICON1( N2M), ICON2(
     & N2M), ITAG( N2M), ICONX( NM), WLAM, IPSYM
      COMMON  /MATPAR/ ICASE, NBLOKS, NPBLK, NLAST, NBLSYM, NPSYM, 
     &NLSYM, IMAT, ICASX, NBBX, NPBX, NLBX, NBBL, NPBL, NLBL
      COMMON  /SMAT/ SSX(16,16)
      COMMON  /SCRATM/ D( N2M)
      COMMON  /ZLOAD/ ZARRAY( NM), NLOAD, NLODF
      COMMON  /SEGJ/ AX(30), BX(30), CX(30), JCO(30), JSNO, ISCON(50), 
     &NSCON, IPCON(10), NPCON
      COMMON  /DATAJ/ S, B, XJ, YJ, ZJ, CABJ, SABJ, SALPJ, EXK, EYK, 
     &EZK, EXS, EYS, EZS, EXC, EYC, EZC, RKH, IEXK, IND1, INDD1, IND2, 
     &INDD2, IPGND
      DIMENSION  CM( NROW,1)
      MP2=2* MP
      NPEQ= NP+ MP2
      NEQ= N+2* M
      NOP= NEQ/ NPEQ
      IF( ICASE.GT.2) REWIND 11
      RKH= RKHX
      IEXK= IEXKX
      IOUT=2* NPBLK* NROW
C                                                                       
C     CYCLE OVER MATRIX BLOCKS                                          
C                                                                       
      IT= NPBLK
      DO 13  IXBLK1=1, NBLOKS
      ISV=( IXBLK1-1)* NPBLK
      IF( IXBLK1.EQ. NBLOKS) IT= NLAST
      DO 1  I=1, NROW
      DO 1  J=1, IT
    1 CM( I, J)=(0.,0.)
      I1= ISV+1
      I2= ISV+ IT
      IN2= I2
      IF( IN2.GT. NP) IN2= NP
      IM1= I1- NP
      IM2= I2- NP
      IF( IM1.LT.1) IM1=1
      IST=1
      IF( I1.LE. NP) IST= NP- I1+2
C                                                                       
C     WIRE SOURCE LOOP                                                  
C                                                                       
      IF( N.EQ.0) GOTO 5
      DO 4  J=1, N
      CALL TRIO( J)
      DO 2  I=1, JSNO
      IJ= JCO( I)
    2 JCO( I)=(( IJ-1)/ NP)* MP2+ IJ
      IF( I1.LE. IN2) CALL CMWW( J, I1, IN2, CM, NROW, CM, NROW,1)
      IF( IM1.LE. IM2) CALL CMWS( J, IM1, IM2, CM(1, IST), NROW, CM, 
     &NROW,1)
C                                                                       
C     MATRIX ELEMENTS MODIFIED BY LOADING                               
C                                                                       
      IF( NLOAD.EQ.0) GOTO 4
      IF( J.GT. NP) GOTO 4
      IPR= J- ISV
      IF( IPR.LT.1.OR. IPR.GT. IT) GOTO 4
      ZAJ= ZARRAY( J)
      DO 3  I=1, JSNO
      JSS= JCO( I)
    3 CM( JSS, IPR)= CM( JSS, IPR)-( AX( I)+ CX( I))* ZAJ
    4 CONTINUE
C     MATRIX ELEMENTS FOR PATCH CURRENT SOURCES                         
    5 IF( M.EQ.0) GOTO 7
      JM1=1- MP
      JM2=0
      JST=1- MP2
      DO 6  I=1, NOP
      JM1= JM1+ MP
      JM2= JM2+ MP
      JST= JST+ NPEQ
      IF( I1.LE. IN2) CALL CMSW( JM1, JM2, I1, IN2, CM( JST,1), CM,0, 
     &NROW,1)
      IF( IM1.LE. IM2) CALL CMSS( JM1, JM2, IM1, IM2, CM( JST, IST), 
     &NROW,1)
    6 CONTINUE
    7 IF( ICASE.EQ.1) GOTO 13
C     COMBINE ELEMENTS FOR SYMMETRY MODES                               
      IF( ICASE.EQ.3) GOTO 12
      DO 11  I=1, IT
      DO 11  J=1, NPEQ
      DO 8  K=1, NOP
      KA= J+( K-1)* NPEQ
    8 D( K)= CM( KA, I)
      DETER= D(1)
      DO 9  KK=2, NOP
    9 DETER= DETER+ D( KK)
      CM( J, I)= DETER
      DO 11  K=2, NOP
      KA= J+( K-1)* NPEQ
      DETER= D(1)
      DO 10  KK=2, NOP
   10 DETER= DETER+ D( KK)* SSX( K, KK)
      CM( KA, I)= DETER
   11 CONTINUE
C     WRITE BLOCK FOR OUT-OF-CORE CASES.                                
      IF( ICASE.LT.3) GOTO 13
   12 CALL BLCKOT( CM,11,1, IOUT,1,31)
   13 CONTINUE
      IF( ICASE.GT.2) REWIND 11
      RETURN
      END
C ***
C     DOUBLE PRECISION 6/4/85
C
      SUBROUTINE CMSS( J1, J2, IM1, IM2, CM, NROW, ITRP)
C ***
      IMPLICIT REAL (A-H,O-Z)
C     CMSS COMPUTES MATRIX ELEMENTS FOR SURFACE-SURFACE INTERACTIONS.   
      PARAMETER ( NM=600, N2M=800, N3M=1000)
      COMPLEX  G11, G12, G21, G22, CM, EXK, EYK, EZK, EXS, EYS, EZS,
     & EXC, EYC, EZC
      COMMON  /DATA/ LD, N1, N2, N, NP, M1, M2, M, MP, X( NM), Y( NM), 
     &Z( NM), SI( NM), BI( NM), ALP( NM), BET( NM), ICON1( N2M), ICON2(
     & N2M), ITAG( N2M), ICONX( NM), WLAM, IPSYM
      COMMON  /ANGL/ SALP( NM)
      COMMON  /DATAJ/ S, B, XJ, YJ, ZJ, CABJ, SABJ, SALPJ, EXK, EYK, 
     &EZK, EXS, EYS, EZS, EXC, EYC, EZC, RKH, IEXK, IND1, INDD1, IND2, 
     &INDD2, IPGND
      DIMENSION  CM( NROW,1)
      DIMENSION  T1X(1), T1Y(1), T1Z(1), T2X(1), T2Y(1), T2Z(1)
      EQUIVALENCE(T1X,SI),(T1Y,ALP),(T1Z,BET),(T2X,ICON1),(T2Y,ICON2),(
     &T2Z,ITAG)
      EQUIVALENCE(T1XJ,CABJ),(T1YJ,SABJ),(T1ZJ,SALPJ),(T2XJ,B),(T2YJ,
     &IND1),(T2ZJ,IND2)
      LDP= LD+1
      I1=( IM1+1)/2
      I2=( IM2+1)/2
      ICOMP= I1*2-3
      II1=-1
C     LOOP OVER OBSERVATION PATCHES                                     
      IF( ICOMP+2.LT. IM1) II1=-2
      DO 5  I= I1, I2
      IL= LDP- I
      ICOMP= ICOMP+2
      II1= II1+2
      II2= II1+1
      T1XI= T1X( IL)* SALP( IL)
      T1YI= T1Y( IL)* SALP( IL)
      T1ZI= T1Z( IL)* SALP( IL)
      T2XI= T2X( IL)* SALP( IL)
      T2YI= T2Y( IL)* SALP( IL)
      T2ZI= T2Z( IL)* SALP( IL)
      XI= X( IL)
      YI= Y( IL)
      ZI= Z( IL)
C     LOOP OVER SOURCE PATCHES                                          
      JJ1=-1
      DO 5  J= J1, J2
      JL= LDP- J
      JJ1= JJ1+2
      JJ2= JJ1+1
      S= BI( JL)
      XJ= X( JL)
      YJ= Y( JL)
      ZJ= Z( JL)
      T1XJ= T1X( JL)
      T1YJ= T1Y( JL)
      T1ZJ= T1Z( JL)
      T2XJ= T2X( JL)
      T2YJ= T2Y( JL)
      T2ZJ= T2Z( JL)
      CALL HINTG( XI, YI, ZI)
      G11=-( T2XI* EXK+ T2YI* EYK+ T2ZI* EZK)
      G12=-( T2XI* EXS+ T2YI* EYS+ T2ZI* EZS)
      G21=-( T1XI* EXK+ T1YI* EYK+ T1ZI* EZK)
      G22=-( T1XI* EXS+ T1YI* EYS+ T1ZI* EZS)
      IF( I.NE. J) GOTO 1
      G11= G11-.5
      G22= G22+.5
C     NORMAL FILL                                                       
    1 IF( ITRP.NE.0) GOTO 3
      IF( ICOMP.LT. IM1) GOTO 2
      CM( II1, JJ1)= G11
      CM( II1, JJ2)= G12
    2 IF( ICOMP.GE. IM2) GOTO 5
      CM( II2, JJ1)= G21
      CM( II2, JJ2)= G22
C     TRANSPOSED FILL                                                   
      GOTO 5
    3 IF( ICOMP.LT. IM1) GOTO 4
      CM( JJ1, II1)= G11
      CM( JJ2, II1)= G12
    4 IF( ICOMP.GE. IM2) GOTO 5
      CM( JJ1, II2)= G21
      CM( JJ2, II2)= G22
    5 CONTINUE
      RETURN
      END
C ***
C     DOUBLE PRECISION 6/4/85
C
      SUBROUTINE CMSW( J1, J2, I1, I2, CM, CW, NCW, NROW, ITRP)
C ***
      IMPLICIT REAL (A-H,O-Z)
C     COMPUTES MATRIX ELEMENTS FOR E ALONG WIRES DUE TO PATCH CURRENT   
      PARAMETER ( NM=600, N2M=800, N3M=1000)
      COMPLEX  CM, ZRATI, ZRATI2, T1, EXK, EYK, EZK, EXS, EYS, EZS, 
     &EXC, EYC, EZC, EMEL, CW, FRATI
      COMMON  /DATA/ LD, N1, N2, N, NP, M1, M2, M, MP, X( NM), Y( NM), 
     &Z( NM), SI( NM), BI( NM), ALP( NM), BET( NM), ICON1( N2M), ICON2(
     & N2M), ITAG( N2M), ICONX( NM), WLAM, IPSYM
      COMMON  /ANGL/ SALP( NM)
      COMMON  /GND/ ZRATI, ZRATI2, FRATI, CL, CH, SCRWL, SCRWR, NRADL, 
     &KSYMP, IFAR, IPERF, T1, T2
      COMMON  /DATAJ/ S, B, XJ, YJ, ZJ, CABJ, SABJ, SALPJ, EXK, EYK, 
     &EZK, EXS, EYS, EZS, EXC, EYC, EZC, RKH, IEXK, IND1, INDD1, IND2, 
     &INDD2, IPGND
      COMMON  /SEGJ/ AX(30), BX(30), CX(30), JCO(30), JSNO, ISCON(50), 
     &NSCON, IPCON(10), NPCON
      DIMENSION  CAB(1), SAB(1), CM( NROW,1), CW( NROW,1)
      DIMENSION  T1X(1), T1Y(1), T1Z(1), T2X(1), T2Y(1), T2Z(1), EMEL(9
     &)
      EQUIVALENCE(T1X,SI),(T1Y,ALP),(T1Z,BET),(T2X,ICON1),(T2Y,ICON2),(
     &T2Z,ITAG),(CAB,ALP),(SAB,BET)
      EQUIVALENCE(T1XJ,CABJ),(T1YJ,SABJ),(T1ZJ,SALPJ),(T2XJ,B),(T2YJ,
     &IND1),(T2ZJ,IND2)
      DATA   PI/3.141592654D+0/
      LDP= LD+1
      NEQS= N- N1+2*( M- M1)
      IF( ITRP.LT.0) GOTO 13
      K=0
C     OBSERVATION LOOP                                                  
      ICGO=1
      DO 12  I= I1, I2
      K= K+1
      XI= X( I)
      YI= Y( I)
      ZI= Z( I)
      CABI= CAB( I)
      SABI= SAB( I)
      SALPI= SALP( I)
      IPCH=0
      IF( ICON1( I).LT.10000) GOTO 1
      IPCH= ICON1( I)-10000
      FSIGN=-1.
    1 IF( ICON2( I).LT.10000) GOTO 2
      IPCH= ICON2( I)-10000
      FSIGN=1.
C     SOURCE LOOP                                                       
    2 JL=0
      DO 12  J= J1, J2
      JS= LDP- J
      JL= JL+2
      T1XJ= T1X( JS)
      T1YJ= T1Y( JS)
      T1ZJ= T1Z( JS)
      T2XJ= T2X( JS)
      T2YJ= T2Y( JS)
      T2ZJ= T2Z( JS)
      XJ= X( JS)
      YJ= Y( JS)
      ZJ= Z( JS)
C     GROUND LOOP                                                       
      S= BI( JS)
      DO 12  IP=1, KSYMP
      IPGND= IP
      IF( IPCH.NE. J.AND. ICGO.EQ.1) GOTO 9
      IF( IP.EQ.2) GOTO 9
      IF( ICGO.GT.1) GOTO 6
      CALL PCINT( XI, YI, ZI, CABI, SABI, SALPI, EMEL)
      PY= PI* SI( I)* FSIGN
      PX= SIN( PY)
      PY= COS( PY)
      EXC= EMEL(9)* FSIGN
      CALL TRIO( I)
      IF( I.GT. N1) GOTO 3
      IL= NEQS+ ICONX( I)
      GOTO 4
    3 IL= I- NCW
      IF( I.LE. NP) IL=(( IL-1)/ NP)*2* MP+ IL
    4 IF( ITRP.NE.0) GOTO 5
      CW( K, IL)= CW( K, IL)+ EXC*( AX( JSNO)+ BX( JSNO)* PX+ CX( JSNO)
     &* PY)
      GOTO 6
    5 CW( IL, K)= CW( IL, K)+ EXC*( AX( JSNO)+ BX( JSNO)* PX+ CX( JSNO)
     &* PY)
    6 IF( ITRP.NE.0) GOTO 7
      CM( K, JL-1)= EMEL( ICGO)
      CM( K, JL)= EMEL( ICGO+4)
      GOTO 8
    7 CM( JL-1, K)= EMEL( ICGO)
      CM( JL, K)= EMEL( ICGO+4)
    8 ICGO= ICGO+1
      IF( ICGO.EQ.5) ICGO=1
      GOTO 11
    9 CALL UNERE( XI, YI, ZI)
C     NORMAL FILL                                                       
      IF( ITRP.NE.0) GOTO 10
      CM( K, JL-1)= CM( K, JL-1)+ EXK* CABI+ EYK* SABI+ EZK* SALPI
      CM( K, JL)= CM( K, JL)+ EXS* CABI+ EYS* SABI+ EZS* SALPI
C     TRANSPOSED FILL                                                   
      GOTO 11
   10 CM( JL-1, K)= CM( JL-1, K)+ EXK* CABI+ EYK* SABI+ EZK* SALPI
      CM( JL, K)= CM( JL, K)+ EXS* CABI+ EYS* SABI+ EZS* SALPI
   11 CONTINUE
   12 CONTINUE
C     FOR OLD SEG. CONNECTING TO OLD PATCH ON ONE END AND NEW SEG. ON   
C     OTHER END INTEGRATE SINGULAR COMPONENT (9) OF SURFACE CURRENT ONLY
      RETURN
   13 IF( J1.LT. I1.OR. J1.GT. I2) GOTO 16
      IPCH= ICON1( J1)
      IF( IPCH.LT.10000) GOTO 14
      IPCH= IPCH-10000
      FSIGN=-1.
      GOTO 15
   14 IPCH= ICON2( J1)
      IF( IPCH.LT.10000) GOTO 16
      IPCH= IPCH-10000
      FSIGN=1.
   15 IF( IPCH.GT. M1) GOTO 16
      JS= LDP- IPCH
      IPGND=1
      T1XJ= T1X( JS)
      T1YJ= T1Y( JS)
      T1ZJ= T1Z( JS)
      T2XJ= T2X( JS)
      T2YJ= T2Y( JS)
      T2ZJ= T2Z( JS)
      XJ= X( JS)
      YJ= Y( JS)
      ZJ= Z( JS)
      S= BI( JS)
      XI= X( J1)
      YI= Y( J1)
      ZI= Z( J1)
      CABI= CAB( J1)
      SABI= SAB( J1)
      SALPI= SALP( J1)
      CALL PCINT( XI, YI, ZI, CABI, SABI, SALPI, EMEL)
      PY= PI* SI( J1)* FSIGN
      PX= SIN( PY)
      PY= COS( PY)
      EXC= EMEL(9)* FSIGN
      IL= JCO( JSNO)
      K= J1- I1+1
      CW( K, IL)= CW( K, IL)+ EXC*( AX( JSNO)+ BX( JSNO)* PX+ CX( JSNO)
     &* PY)
   16 RETURN
      END
C ***
C     DOUBLE PRECISION 6/4/85
C
      SUBROUTINE CMWS( J, I1, I2, CM, NR, CW, NW, ITRP)
C ***
C                                                                       
C     CMWS COMPUTES MATRIX ELEMENTS FOR WIRE-SURFACE INTERACTIONS       
C                                                                       
      IMPLICIT REAL (A-H,O-Z)
      PARAMETER ( NM=600, N2M=800, N3M=1000)
      COMPLEX  CM, CW, ETK, ETS, ETC, EXK, EYK, EZK, EXS, EYS, EZS, 
     &EXC, EYC, EZC
      COMMON  /DATA/ LD, N1, N2, N, NP, M1, M2, M, MP, X( NM), Y( NM), 
     &Z( NM), SI( NM), BI( NM), ALP( NM), BET( NM), ICON1( N2M), ICON2(
     & N2M), ITAG( N2M), ICONX( NM), WLAM, IPSYM
      COMMON  /ANGL/ SALP( NM)
      COMMON  /SEGJ/ AX(30), BX(30), CX(30), JCO(30), JSNO, ISCON(50), 
     &NSCON, IPCON(10), NPCON
      COMMON  /DATAJ/ S, B, XJ, YJ, ZJ, CABJ, SABJ, SALPJ, EXK, EYK, 
     &EZK, EXS, EYS, EZS, EXC, EYC, EZC, RKH, IEXK, IND1, INDD1, IND2, 
     &INDD2, IPGND
      DIMENSION  CM( NR,1), CW( NW,1), CAB(1), SAB(1)
      DIMENSION  T1X(1), T1Y(1), T1Z(1), T2X(1), T2Y(1), T2Z(1)
      EQUIVALENCE(CAB,ALP),(SAB,BET),(T1X,SI),(T1Y,ALP),(T1Z,BET)
      EQUIVALENCE(T2X,ICON1),(T2Y,ICON2),(T2Z,ITAG)
      LDP= LD+1
      S= SI( J)
      B= BI( J)
      XJ= X( J)
      YJ= Y( J)
      ZJ= Z( J)
      CABJ= CAB( J)
      SABJ= SAB( J)
C                                                                       
C     OBSERVATION LOOP                                                  
C                                                                       
      SALPJ= SALP( J)
      IPR=0
      DO 9  I= I1, I2
      IPR= IPR+1
      IPATCH=( I+1)/2
      IK= I-( I/2)*2
      IF( IK.EQ.0.AND. IPR.NE.1) GOTO 1
      JS= LDP- IPATCH
      XI= X( JS)
      YI= Y( JS)
      ZI= Z( JS)
      CALL HSFLD( XI, YI, ZI,0.)
      IF( IK.EQ.0) GOTO 1
      TX= T2X( JS)
      TY= T2Y( JS)
      TZ= T2Z( JS)
      GOTO 2
    1 TX= T1X( JS)
      TY= T1Y( JS)
      TZ= T1Z( JS)
    2 ETK=-( EXK* TX+ EYK* TY+ EZK* TZ)* SALP( JS)
      ETS=-( EXS* TX+ EYS* TY+ EZS* TZ)* SALP( JS)
C                                                                       
C     FILL MATRIX ELEMENTS.  ELEMENT LOCATIONS DETERMINED BY CONNECTION 
C     DATA.                                                             
C                                                                       
      ETC=-( EXC* TX+ EYC* TY+ EZC* TZ)* SALP( JS)
C     NORMAL FILL                                                       
      IF( ITRP.NE.0) GOTO 4
      DO 3  IJ=1, JSNO
      JX= JCO( IJ)
    3 CM( IPR, JX)= CM( IPR, JX)+ ETK* AX( IJ)+ ETS* BX( IJ)+ ETC* CX( 
     &IJ)
      GOTO 9
C     TRANSPOSED FILL                                                   
    4 IF( ITRP.EQ.2) GOTO 6
      DO 5  IJ=1, JSNO
      JX= JCO( IJ)
    5 CM( JX, IPR)= CM( JX, IPR)+ ETK* AX( IJ)+ ETS* BX( IJ)+ ETC* CX( 
     &IJ)
C     TRANSPOSED FILL - C(WS) AND D(WS)PRIME (=CW)                      
      GOTO 9
    6 DO 8  IJ=1, JSNO
      JX= JCO( IJ)
      IF( JX.GT. NR) GOTO 7
      CM( JX, IPR)= CM( JX, IPR)+ ETK* AX( IJ)+ ETS* BX( IJ)+ ETC* CX( 
     &IJ)
      GOTO 8
    7 JX= JX- NR
      CW( JX, IPR)= CW( JX, IPR)+ ETK* AX( IJ)+ ETS* BX( IJ)+ ETC* CX( 
     &IJ)
    8 CONTINUE
    9 CONTINUE
      RETURN
      END
C ***
C     DOUBLE PRECISION 6/4/85
C
      SUBROUTINE CMWW( J, I1, I2, CM, NR, CW, NW, ITRP)
C ***
      IMPLICIT REAL (A-H,O-Z)
C                                                                       
C     CMWW COMPUTES MATRIX ELEMENTS FOR WIRE-WIRE INTERACTIONS          
C                                                                       
      PARAMETER ( NM=600, N2M=800, N3M=1000)
      COMPLEX  CM, CW, ETK, ETS, ETC, EXK, EYK, EZK, EXS, EYS, EZS, 
     &EXC, EYC, EZC
      COMMON  /DATA/ LD, N1, N2, N, NP, M1, M2, M, MP, X( NM), Y( NM), 
     &Z( NM), SI( NM), BI( NM), ALP( NM), BET( NM), ICON1( N2M), ICON2(
     & N2M), ITAG( N2M), ICONX( NM), WLAM, IPSYM
      COMMON  /ANGL/ SALP( NM)
      COMMON  /SEGJ/ AX(30), BX(30), CX(30), JCO(30), JSNO, ISCON(50), 
     &NSCON, IPCON(10), NPCON
      COMMON  /DATAJ/ S, B, XJ, YJ, ZJ, CABJ, SABJ, SALPJ, EXK, EYK, 
     &EZK, EXS, EYS, EZS, EXC, EYC, EZC, RKH, IEXK, IND1, INDD1, IND2, 
     &INDD2, IPGND
      DIMENSION  CM( NR,1), CW( NW,1), CAB(1), SAB(1)
C     SET SOURCE SEGMENT PARAMETERS                                     
      EQUIVALENCE(CAB,ALP),(SAB,BET)
      S= SI( J)
      B= BI( J)
      XJ= X( J)
      YJ= Y( J)
      ZJ= Z( J)
      CABJ= CAB( J)
      SABJ= SAB( J)
      SALPJ= SALP( J)
C     DECIDE WETHER EXT. T.W. APPROX. CAN BE USED                       
      IF( IEXK.EQ.0) GOTO 16
      IPR= ICON1( J)
      IF( IPR) 1,6,2
    1 IPR=- IPR
      IF(- ICON1( IPR).NE. J) GOTO 7
      GOTO 4
    2 IF( IPR.NE. J) GOTO 3
      IF( CABJ* CABJ+ SABJ* SABJ.GT.1.D-8) GOTO 7
      GOTO 5
    3 IF( ICON2( IPR).NE. J) GOTO 7
    4 XI= ABS( CABJ* CAB( IPR)+ SABJ* SAB( IPR)+ SALPJ* SALP( IPR))
      IF( XI.LT.0.999999D+0) GOTO 7
      IF( ABS( BI( IPR)/ B-1.).GT.1.D-6) GOTO 7
    5 IND1=0
      GOTO 8
    6 IND1=1
      GOTO 8
    7 IND1=2
    8 IPR= ICON2( J)
      IF( IPR) 9,14,10
    9 IPR=- IPR
      IF(- ICON2( IPR).NE. J) GOTO 15
      GOTO 12
   10 IF( IPR.NE. J) GOTO 11
      IF( CABJ* CABJ+ SABJ* SABJ.GT.1.D-8) GOTO 15
      GOTO 13
   11 IF( ICON1( IPR).NE. J) GOTO 15
   12 XI= ABS( CABJ* CAB( IPR)+ SABJ* SAB( IPR)+ SALPJ* SALP( IPR))
      IF( XI.LT.0.999999D+0) GOTO 15
      IF( ABS( BI( IPR)/ B-1.).GT.1.D-6) GOTO 15
   13 IND2=0
      GOTO 16
   14 IND2=1
      GOTO 16
   15 IND2=2
C                                                                       
C     OBSERVATION LOOP                                                  
C                                                                       
   16 CONTINUE
      IPR=0
      DO 23  I= I1, I2
      IPR= IPR+1
      IJ= I- J
      XI= X( I)
      YI= Y( I)
      ZI= Z( I)
      AI= BI( I)
      CABI= CAB( I)
      SABI= SAB( I)
      SALPI= SALP( I)
      CALL EFLD( XI, YI, ZI, AI, IJ)
      ETK= EXK* CABI+ EYK* SABI+ EZK* SALPI
      ETS= EXS* CABI+ EYS* SABI+ EZS* SALPI
C                                                                       
C     FILL MATRIX ELEMENTS.  ELEMENT LOCATIONS DETERMINED BY CONNECTION 
C     DATA.                                                             
C                                                                       
      ETC= EXC* CABI+ EYC* SABI+ EZC* SALPI
C     NORMAL FILL                                                       
      IF( ITRP.NE.0) GOTO 18
      DO 17  IJ=1, JSNO
      JX= JCO( IJ)
   17 CM( IPR, JX)= CM( IPR, JX)+ ETK* AX( IJ)+ ETS* BX( IJ)+ ETC* CX( 
     &IJ)
      GOTO 23
C     TRANSPOSED FILL                                                   
   18 IF( ITRP.EQ.2) GOTO 20
      DO 19  IJ=1, JSNO
      JX= JCO( IJ)
   19 CM( JX, IPR)= CM( JX, IPR)+ ETK* AX( IJ)+ ETS* BX( IJ)+ ETC* CX( 
     &IJ)
C     TRANS. FILL FOR C(WW) - TEST FOR ELEMENTS FOR D(WW)PRIME.  (=CW)  
      GOTO 23
   20 DO 22  IJ=1, JSNO
      JX= JCO( IJ)
      IF( JX.GT. NR) GOTO 21
      CM( JX, IPR)= CM( JX, IPR)+ ETK* AX( IJ)+ ETS* BX( IJ)+ ETC* CX( 
     &IJ)
      GOTO 22
   21 JX= JX- NR
      CW( JX, IPR)= CW( JX, IPR)+ ETK* AX( IJ)+ ETS* BX( IJ)+ ETC* CX( 
     &IJ)
   22 CONTINUE
   23 CONTINUE
      RETURN
      END
C ***
C     DOUBLE PRECISION 6/4/85
C
      SUBROUTINE CONECT( IGND)
C ***
      IMPLICIT REAL (A-H,O-Z)
C                                                                       
C     CONNECT SETS UP SEGMENT CONNECTION DATA IN ARRAYS ICON1 AND ICON2 
C     BY SEARCHING FOR SEGMENT ENDS THAT ARE IN CONTACT.                
C                                                                       
      PARAMETER ( NM=600, N2M=800, N3M=1000)
      COMMON  /DATA/ LD, N1, N2, N, NP, M1, M2, M, MP, X( NM), Y( NM), 
     &Z( NM), SI( NM), BI( NM), ALP( NM), BET( NM), ICON1( N2M), ICON2(
     & N2M), ITAG( N2M), ICONX( NM), WLAM, IPSYM
      COMMON  /SEGJ/ AX(30), BX(30), CX(30), JCO(30), JSNO, ISCON(50), 
     &NSCON, IPCON(10), NPCON
      DIMENSION  X2(1), Y2(1), Z2(1)
      EQUIVALENCE(X2,SI),(Y2,ALP),(Z2,BET)
      DATA   JMAX/30/, SMIN/1.D-3/, NSMAX/50/, NPMAX/10/
      NSCON=0
      NPCON=0
      IF( IGND.EQ.0) GOTO 3
      WRITE (2,54) 
      IF( IGND.GT.0) WRITE (2,55) 
      IF( IPSYM.NE.2) GOTO 1
      NP=2* NP
      MP=2* MP
    1 IF( IABS( IPSYM).LE.2) GOTO 2
      NP= N
      MP= M
    2 IF( NP.GT. N) STOP
      IF( NP.EQ. N.AND. MP.EQ. M) IPSYM=0
    3 IF( N.EQ.0) GOTO 26
      DO 15  I=1, N
      ICONX( I)=0
      XI1= X( I)
      YI1= Y( I)
      ZI1= Z( I)
      XI2= X2( I)
      YI2= Y2( I)
      ZI2= Z2( I)
C                                                                       
C     DETERMINE CONNECTION DATA FOR END 1 OF SEGMENT.                   
C                                                                       
      SLEN= SQRT(( XI2- XI1)**2+( YI2- YI1)**2+( ZI2- ZI1)**2)* SMIN
      IF( IGND.LT.1) GOTO 5
      IF( ZI1.GT.- SLEN) GOTO 4
      WRITE (2,56)  I
      STOP
    4 IF( ZI1.GT. SLEN) GOTO 5
      ICON1( I)= I
      Z( I)=0.
      GOTO 9
    5 IC= I
      DO 7  J=2, N
      IC= IC+1
      IF( IC.GT. N) IC=1
      SEP= ABS( XI1- X( IC))+ ABS( YI1- Y( IC))+ ABS( ZI1- Z( IC))
      IF( SEP.GT. SLEN) GOTO 6
      ICON1( I)=- IC
      GOTO 8
    6 SEP= ABS( XI1- X2( IC))+ ABS( YI1- Y2( IC))+ ABS( ZI1- Z2( IC))
      IF( SEP.GT. SLEN) GOTO 7
      ICON1( I)= IC
      GOTO 8
    7 CONTINUE
      IF( I.LT. N2.AND. ICON1( I).GT.10000) GOTO 8
C                                                                       
C     DETERMINE CONNECTION DATA FOR END 2 OF SEGMENT.                   
C                                                                       
      ICON1( I)=0
    8 IF( IGND.LT.1) GOTO 12
    9 IF( ZI2.GT.- SLEN) GOTO 10
      WRITE (2,56)  I
      STOP
   10 IF( ZI2.GT. SLEN) GOTO 12
      IF( ICON1( I).NE. I) GOTO 11
      WRITE (2,57)  I
      STOP
   11 ICON2( I)= I
      Z2( I)=0.
      GOTO 15
   12 IC= I
      DO 14  J=2, N
      IC= IC+1
      IF( IC.GT. N) IC=1
      SEP= ABS( XI2- X( IC))+ ABS( YI2- Y( IC))+ ABS( ZI2- Z( IC))
      IF( SEP.GT. SLEN) GOTO 13
      ICON2( I)= IC
      GOTO 15
   13 SEP= ABS( XI2- X2( IC))+ ABS( YI2- Y2( IC))+ ABS( ZI2- Z2( IC))
      IF( SEP.GT. SLEN) GOTO 14
      ICON2( I)=- IC
      GOTO 15
   14 CONTINUE
      IF( I.LT. N2.AND. ICON2( I).GT.10000) GOTO 15
      ICON2( I)=0
   15 CONTINUE
C     FIND WIRE-SURFACE CONNECTIONS FOR NEW PATCHES                     
      IF( M.EQ.0) GOTO 26
      IX= LD+1- M1
      I= M2
   16 IF( I.GT. M) GOTO 20
      IX= IX-1
      XS= X( IX)
      YS= Y( IX)
      ZS= Z( IX)
      DO 18  ISEG=1, N
      XI1= X( ISEG)
      YI1= Y( ISEG)
      ZI1= Z( ISEG)
      XI2= X2( ISEG)
      YI2= Y2( ISEG)
      ZI2= Z2( ISEG)
C     FOR FIRST END OF SEGMENT                                          
      SLEN=( ABS( XI2- XI1)+ ABS( YI2- YI1)+ ABS( ZI2- ZI1))* SMIN
      SEP= ABS( XI1- XS)+ ABS( YI1- YS)+ ABS( ZI1- ZS)
C     CONNECTION - DIVIDE PATCH INTO 4 PATCHES AT PRESENT ARRAY LOC.    
      IF( SEP.GT. SLEN) GOTO 17
      ICON1( ISEG)=10000+ I
      IC=0
      CALL SUBPH( I, IC, XI1, YI1, ZI1, XI2, YI2, ZI2, XA, YA, ZA, XS, 
     &YS, ZS)
      GOTO 19
   17 SEP= ABS( XI2- XS)+ ABS( YI2- YS)+ ABS( ZI2- ZS)
      IF( SEP.GT. SLEN) GOTO 18
      ICON2( ISEG)=10000+ I
      IC=0
      CALL SUBPH( I, IC, XI1, YI1, ZI1, XI2, YI2, ZI2, XA, YA, ZA, XS, 
     &YS, ZS)
      GOTO 19
   18 CONTINUE
   19 I= I+1
C     REPEAT SEARCH FOR NEW SEGMENTS CONNECTED TO NGF PATCHES.          
      GOTO 16
   20 IF( M1.EQ.0.OR. N2.GT. N) GOTO 26
      IX= LD+1
      I=1
   21 IF( I.GT. M1) GOTO 25
      IX= IX-1
      XS= X( IX)
      YS= Y( IX)
      ZS= Z( IX)
      DO 23  ISEG= N2, N
      XI1= X( ISEG)
      YI1= Y( ISEG)
      ZI1= Z( ISEG)
      XI2= X2( ISEG)
      YI2= Y2( ISEG)
      ZI2= Z2( ISEG)
      SLEN=( ABS( XI2- XI1)+ ABS( YI2- YI1)+ ABS( ZI2- ZI1))* SMIN
      SEP= ABS( XI1- XS)+ ABS( YI1- YS)+ ABS( ZI1- ZS)
      IF( SEP.GT. SLEN) GOTO 22
      ICON1( ISEG)=10001+ M
      IC=1
      NPCON= NPCON+1
      IPCON( NPCON)= I
      CALL SUBPH( I, IC, XI1, YI1, ZI1, XI2, YI2, ZI2, XA, YA, ZA, XS, 
     &YS, ZS)
      GOTO 24
   22 SEP= ABS( XI2- XS)+ ABS( YI2- YS)+ ABS( ZI2- ZS)
      IF( SEP.GT. SLEN) GOTO 23
      ICON2( ISEG)=10001+ M
      IC=1
      NPCON= NPCON+1
      IPCON( NPCON)= I
      CALL SUBPH( I, IC, XI1, YI1, ZI1, XI2, YI2, ZI2, XA, YA, ZA, XS, 
     &YS, ZS)
      GOTO 24
   23 CONTINUE
   24 I= I+1
      GOTO 21
   25 IF( NPCON.LE. NPMAX) GOTO 26
      WRITE (2,62)  NPMAX
      STOP
   26 WRITE (2,58)  N, NP, IPSYM
      IF( M.GT.0) WRITE (2,61)  M, MP
      ISEG=( N+ M)/( NP+ MP)
      IF( ISEG.EQ.1) GOTO 30
      IF( IPSYM) 28,27,29
   27 STOP
   28 WRITE (2,59)  ISEG
      GOTO 30
   29 IC= ISEG/2
      IF( ISEG.EQ.8) IC=3
      WRITE (2,60)  IC
   30 IF( N.EQ.0) GOTO 48
      WRITE (2,50) 
C     ADJUST CONNECTED SEG. ENDS TO EXACTLY COINCIDE.  PRINT JUNCTIONS  
C     OF 3 OR MORE SEG.  ALSO FIND OLD SEG. CONNECTING TO NEW SEG.      
      ISEG=0
      DO 44  J=1, N
      IEND=-1
      JEND=-1
      IX= ICON1( J)
      IC=1
      JCO(1)=- J
      XA= X( J)
      YA= Y( J)
      ZA= Z( J)
   31 IF( IX.EQ.0) GOTO 43
      IF( IX.EQ. J) GOTO 43
      IF( IX.GT.10000) GOTO 43
      NSFLG=0
   32 IF( IX) 33,49,34
   33 IX=- IX
      GOTO 35
   34 JEND=- JEND
   35 IF( IX.EQ. J) GOTO 37
      IF( IX.LT. J) GOTO 43
      IC= IC+1
      IF( IC.GT. JMAX) GOTO 49
      JCO( IC)= IX* JEND
      IF( IX.GT. N1) NSFLG=1
      IF( JEND.EQ.1) GOTO 36
      XA= XA+ X( IX)
      YA= YA+ Y( IX)
      ZA= ZA+ Z( IX)
      IX= ICON1( IX)
      GOTO 32
   36 XA= XA+ X2( IX)
      YA= YA+ Y2( IX)
      ZA= ZA+ Z2( IX)
      IX= ICON2( IX)
      GOTO 32
   37 SEP= IC
      XA= XA/ SEP
      YA= YA/ SEP
      ZA= ZA/ SEP
      DO 39  I=1, IC
      IX= JCO( I)
      IF( IX.GT.0) GOTO 38
      IX=- IX
      X( IX)= XA
      Y( IX)= YA
      Z( IX)= ZA
      GOTO 39
   38 X2( IX)= XA
      Y2( IX)= YA
      Z2( IX)= ZA
   39 CONTINUE
      IF( N1.EQ.0) GOTO 42
      IF( NSFLG.EQ.0) GOTO 42
      DO 41  I=1, IC
      IX= IABS( JCO( I))
      IF( IX.GT. N1) GOTO 41
      IF( ICONX( IX).NE.0) GOTO 41
      NSCON= NSCON+1
      IF( NSCON.LE. NSMAX) GOTO 40
      WRITE (2,62)  NSMAX
      STOP
   40 ISCON( NSCON)= IX
      ICONX( IX)= NSCON
   41 CONTINUE
   42 IF( IC.LT.3) GOTO 43
      ISEG= ISEG+1
      WRITE (2,51)  ISEG,( JCO( I), I=1, IC)
   43 IF( IEND.EQ.1) GOTO 44
      IEND=1
      JEND=1
      IX= ICON2( J)
      IC=1
      JCO(1)= J
      XA= X2( J)
      YA= Y2( J)
      ZA= Z2( J)
      GOTO 31
   44 CONTINUE
      IF( ISEG.EQ.0) WRITE (2,52) 
C     FIND OLD SEGMENTS THAT CONNECT TO NEW PATCHES                     
      IF( N1.EQ.0.OR. M1.EQ. M) GOTO 48
      DO 47  J=1, N1
      IX= ICON1( J)
      IF( IX.LT.10000) GOTO 45
      IX= IX-10000
      IF( IX.GT. M1) GOTO 46
   45 IX= ICON2( J)
      IF( IX.LT.10000) GOTO 47
      IX= IX-10000
      IF( IX.LT. M2) GOTO 47
   46 IF( ICONX( J).NE.0) GOTO 47
      NSCON= NSCON+1
      ISCON( NSCON)= J
      ICONX( J)= NSCON
   47 CONTINUE
   48 CONTINUE
      RETURN
   49 WRITE (2,53)  IX
C                                                                       
      STOP
   50 FORMAT(//,9X,'- MULTIPLE WIRE JUNCTIONS -',/,1X,'JUNCTION',4X,
     &'SEGMENTS  (- FOR END 1, + FOR END 2)')
   51 FORMAT(1X,I5,5X,20I5,/,(11X,20I5))
   52 FORMAT(2X,'NONE')
   53 FORMAT(' CONNECT - SEGMENT CONNECTION ERROR FOR SEGMENT',I5)
   54 FORMAT(/,3X,'GROUND PLANE SPECIFIED.')
   55 FORMAT(/,3X,'WHERE WIRE ENDS TOUCH GROUND, CURRENT WILL BE ',
     &'INTERPOLATED TO IMAGE IN GROUND PLANE.',/)
   56 FORMAT(' GEOMETRY DATA ERROR-- SEGMENT',I5,' EXTENDS BELOW GRO',
     &'UND')
   57 FORMAT(' GEOMETRY DATA ERROR--SEGMENT',I5,' LIES IN GROUND ',
     &'PLANE.')
   58 FORMAT(/,3X,'TOTAL SEGMENTS USED=',I5,5X,'NO. SEG. IN ','A SY',
     &'MMETRIC CELL=',I5,5X,'SYMMETRY FLAG=',I3)
   59 FORMAT(' STRUCTURE HAS',I4,' FOLD ROTATIONAL SYMMETRY',/)
   60 FORMAT(' STRUCTURE HAS',I2,' PLANES OF SYMMETRY',/)
   61 FORMAT(3X,'TOTAL PATCHES USED=',I5,6X,'NO. PATCHES IN A SYMMET',
     &'RIC CELL=',I5)
   62 FORMAT(' ERROR - NO. NEW SEGMENTS CONNECTED TO N.G.F. SEGMENTS',
     &'OR PATCHES EXCEEDS LIMIT OF',I5)
      END
C ***
C     DOUBLE PRECISION 6/4/85
C
      SUBROUTINE COUPLE( CUR, WLAM)
C ***
      IMPLICIT REAL (A-H,O-Z)
C                                                                       
C     COUPLE COMPUTES THE MAXIMUM COUPLING BETWEEN PAIRS OF SEGMENTS.   
C                                                                       
      PARAMETER ( NM=600, N2M=800, N3M=1000)
      COMPLEX  Y11A, Y12A, CUR, Y11, Y12, Y22, YL, YIN, ZL, ZIN, RHO
     &, VQD, VSANT, VQDS
      COMMON  /YPARM/ NCOUP, ICOUP, NCTAG(5), NCSEG(5), Y11A(5), Y12A(
     &20)
      COMMON  /VSORC/ VQD(30), VSANT(30), VQDS(30), IVQD(30), ISANT(30)
     &, IQDS(30), NVQD, NSANT, NQDS
      DIMENSION  CUR(1)
      IF( NSANT.NE.1.OR. NVQD.NE.0) RETURN
      J= ISEGNO( NCTAG( ICOUP+1), NCSEG( ICOUP+1))
      IF( J.NE. ISANT(1)) RETURN
      ICOUP= ICOUP+1
      ZIN= VSANT(1)
      Y11A( ICOUP)= CUR( J)* WLAM/ ZIN
      L1=( ICOUP-1)*( NCOUP-1)
      DO 1  I=1, NCOUP
      IF( I.EQ. ICOUP) GOTO 1
      K= ISEGNO( NCTAG( I), NCSEG( I))
      L1= L1+1
      Y12A( L1)= CUR( K)* WLAM/ ZIN
    1 CONTINUE
      IF( ICOUP.LT. NCOUP) RETURN
      WRITE (2,6) 
      NPM1= NCOUP-1
      DO 5  I=1, NPM1
      ITT1= NCTAG( I)
      ITS1= NCSEG( I)
      ISG1= ISEGNO( ITT1, ITS1)
      L1= I+1
      DO 5  J= L1, NCOUP
      ITT2= NCTAG( J)
      ITS2= NCSEG( J)
      ISG2= ISEGNO( ITT2, ITS2)
      J1= J+( I-1)* NPM1-1
      J2= I+( J-1)* NPM1
      Y11= Y11A( I)
      Y22= Y11A( J)
      Y12=.5*( Y12A( J1)+ Y12A( J2))
      YIN= Y12* Y12
      DBC= ABS( YIN)
      C= DBC/(2.* REAL( Y11)* REAL( Y22)- REAL( YIN))
      IF( C.LT.0..OR. C.GT.1.) GOTO 4
      IF( C.LT..01) GOTO 2
      GMAX=(1.- SQRT(1.- C* C))/ C
      GOTO 3
    2 GMAX=.5*( C+.25* C* C* C)
    3 RHO= GMAX* CONJG( YIN)/ DBC
      YL=((1.- RHO)/(1.+ RHO)+1.)* REAL( Y22)- Y22
      ZL=1./ YL
      YIN= Y11- YIN/( Y22+ YL)
      ZIN=1./ YIN
      DBC= DB10( GMAX)
      WRITE (2,7)  ITT1, ITS1, ISG1, ITT2, ITS2, ISG2, DBC, ZL, ZIN
      GOTO 5
    4 WRITE (2,8)  ITT1, ITS1, ISG1, ITT2, ITS2, ISG2, C
    5 CONTINUE
C                                                                       
      RETURN
    6 FORMAT(///,36X,'- - - ISOLATION DATA - - -',//,6X,'- - COUPLIN',
     &'G BETWEEN - -',8X,'MAXIMUM',15X,'- - - FOR MAXIMUM COUPLING - ',
     &'- -',/,12X,'SEG.',14X,'SEG.',3X,'COUPLING',4X,'LOAD IMPEDANCE ',
     &'(2ND SEG.)',7X,'INPUT IMPEDANCE',/,2X,'TAG/SEG.',3X,'NO.',4X,
     &'TAG/''SEG.',3X,'NO.',6X,'(DB)',8X,'REAL',9X,'IMAG.',9X,'REAL',9X
     &,'IMAG.')
    7 FORMAT(2(1X,I4,1X,I4,1X,I5,2X),F9.3,2X,1P,2(2X,E12.5,1X,E12.5))
    8 FORMAT(2(1X,I4,1X,I4,1X,I5,2X),'**ERROR** COUPLING IS NOT BETWE',
     &'EN 0 AND 1. (=',1P,E12.5,')')
      END
C ***
C     DOUBLE PRECISION 6/4/85
C
      SUBROUTINE DATAGN
C ***
      IMPLICIT REAL (A-H,O-Z)
C                                                                       
C     DATAGN IS THE MAIN ROUTINE FOR INPUT OF GEOMETRY DATA.            
C                                                                       
C***
      PARAMETER ( NM=600, N2M=800, N3M=1000)
C***
      CHARACTER *2  GM, ATST
      CHARACTER *1 IFX,IFY,IFZ,IPT

      COMMON  /DATA/ LD, N1, N2, N, NP, M1, M2, M, MP, X( NM), Y( NM), 
     &Z( NM), SI( NM), BI( NM), ALP( NM), BET( NM), ICON1( N2M), ICON2(
     & N2M), ITAG( N2M), ICONX( NM), WLAM, IPSYM
C***
      COMMON  /ANGL/ SALP( NM)
C***
      COMMON  /PLOT/ IPLP1, IPLP2, IPLP3, IPLP4
      DIMENSION  X2(1), Y2(1), Z2(1), T1X(1), T1Y(1), T1Z(1), T2X(1), 
     &T2Y(1), T2Z(1), ATST(13), IFX(2), IFY(2), IFZ(2), CAB(1), SAB(1),
     & IPT(4)
C***
      EQUIVALENCE(T1X,SI),(T1Y,ALP),(T1Z,BET),(T2X,ICON1),(T2Y,ICON2),(
     &T2Z,ITAG),(X2,SI),(Y2,ALP),(Z2,BET),(CAB,ALP),(SAB,BET)
C***
      data atst/'GW','GX','GR','GS','GE','GM','SP','SM','GF','GA',
     $          'SC','GC','GH'/
*      DATA   ATST/2HGW,2HGX,2HGR,2HGS,2HGE,2HGM,2HSP,2HSM,2HGF,2HGA,
*     &2HSC,2HGC,2HGH/
      DATA   IFX/1H ,1HX/, IFY/1H ,1HY/, IFZ/1H ,1HZ/
      DATA   TA/0.01745329252D+0/, TD/57.29577951D+0/, IPT/1HP,1HR,1HT,
     &1HQ/
      IPSYM=0
      NWIRE=0
      N=0
      NP=0
      M=0
      MP=0
      N1=0
      N2=1
      M1=0
      M2=1
      ISCT=0

C                                                                       
C     READ GEOMETRY DATA CARD AND BRANCH TO SECTION FOR OPERATION       
C     REQUESTED                                                         
C                                                                       
C***   
C 1     READ (5,42) GM,ITG,NS,XW1,YW1,ZW1,XW2,YW2,ZW2,RAD                
      IPHD=0
C***
    1 CALL READGM( GM, ITG, NS, XW1, YW1, ZW1, XW2, YW2, ZW2, RAD)
      IF( N+ M.GT. LD) GOTO 37
      IF( GM.EQ. ATST(9)) GOTO 27
      IF( IPHD.EQ.1) GOTO 2
      WRITE (2,40) 
      WRITE (2,41) 
      IPHD=1
    2 IF( GM.EQ. ATST(11)) GOTO 10
      ISCT=0
      IF( GM.EQ. ATST(1)) GOTO 3
      IF( GM.EQ. ATST(2)) GOTO 18
      IF( GM.EQ. ATST(3)) GOTO 19
      IF( GM.EQ. ATST(4)) GOTO 21
      IF( GM.EQ. ATST(7)) GOTO 9
      IF( GM.EQ. ATST(8)) GOTO 13
      IF( GM.EQ. ATST(5)) GOTO 29
      IF( GM.EQ. ATST(6)) GOTO 26
C***
      IF( GM.EQ. ATST(10)) GOTO 8
C***
      IF( GM.EQ. ATST(13)) GOTO 123
C                                                                       
C     GENERATE SEGMENT DATA FOR STRAIGHT WIRE.                          
C
      GOTO 36

    3 NWIRE= NWIRE+1
      I1= N+1
      I2= N+ NS
      WRITE (2,43)  NWIRE, XW1, YW1, ZW1, XW2, YW2, ZW2, RAD, NS, I1, 
     &I2, ITG
      IF( RAD.EQ.0) GOTO 4
      XS1=1.
      YS1=1.
C***
      GOTO 7
C 4     READ (5,42) GM,IX,IY,XS1,YS1,ZS1                                 
C***
    4 CALL READGM( GM, IX, IY, XS1, YS1, ZS1, DUMMY, DUMMY, DUMMY, 
     &DUMMY)
      IF( GM.EQ. ATST(12)) GOTO 6
    5 WRITE (2,48) 
      STOP
    6 WRITE (2,61)  XS1, YS1, ZS1
      IF( YS1.EQ.0.OR. ZS1.EQ.0) GOTO 5
      RAD= YS1
      YS1=( ZS1/ YS1)**(1./( NS-1.))
    7 CALL WIRE( XW1, YW1, ZW1, XW2, YW2, ZW2, RAD, XS1, YS1, NS, ITG)
C                                                                       
C     GENERATE SEGMENT DATA FOR WIRE ARC                                
C                                                                       
      GOTO 1
    8 NWIRE= NWIRE+1
      I1= N+1
      I2= N+ NS
      WRITE (2,38)  NWIRE, XW1, YW1, ZW1, XW2, NS, I1, I2, ITG
      CALL ARC( ITG, NS, XW1, YW1, ZW1, XW2)
C***
C
C     GENERATE HELIX
C
      GOTO 1
  123 NWIRE= NWIRE+1
      I1= N+1
      I2= N+ NS
      WRITE (2,124)  XW1, YW1, NWIRE, ZW1, XW2, YW2, ZW2, RAD, NS, I1, 
     &I2, ITG
      CALL HELIX( XW1, YW1, ZW1, XW2, YW2, ZW2, RAD, NS, ITG)
C
      GOTO 1
C***
C                                                                       
C     GENERATE SINGLE NEW PATCH                                         
C                                                                       
  124 FORMAT(5X,'HELIX STRUCTURE-   AXIAL SPACING BETWEEN TURNS =',F8.3
     &,' TOTAL AXIAL LENGTH =',F8.3/1X,I5,2X,'RADIUS OF HELIX =',4(2X,F
     &8.3),7X,F11.5,I8,4X,I5,1X,I5,3X,I5)
    9 I1= M+1
      NS= NS+1
      IF( ITG.NE.0) GOTO 17
      WRITE (2,51)  I1, IPT( NS), XW1, YW1, ZW1, XW2, YW2, ZW2
      IF( NS.EQ.2.OR. NS.EQ.4) ISCT=1
      IF( NS.GT.1) GOTO 14
      XW2= XW2* TA
      YW2= YW2* TA
      GOTO 16
   10 IF( ISCT.EQ.0) GOTO 17
      I1= M+1
      NS= NS+1
      IF( ITG.NE.0) GOTO 17
      IF( NS.NE.2.AND. NS.NE.4) GOTO 17
      XS1= X4
      YS1= Y4
      ZS1= Z4
      XS2= X3
      YS2= Y3
      ZS2= Z3
      X3= XW1
      Y3= YW1
      Z3= ZW1
      IF( NS.NE.4) GOTO 11
      X4= XW2
      Y4= YW2
      Z4= ZW2
   11 XW1= XS1
      YW1= YS1
      ZW1= ZS1
      XW2= XS2
      YW2= YS2
      ZW2= ZS2
      IF( NS.EQ.4) GOTO 12
      X4= XW1+ X3- XW2
      Y4= YW1+ Y3- YW2
      Z4= ZW1+ Z3- ZW2
   12 WRITE (2,51)  I1, IPT( NS), XW1, YW1, ZW1, XW2, YW2, ZW2
      WRITE (2,39)  X3, Y3, Z3, X4, Y4, Z4
C                                                                       
C     GENERATE MULTIPLE-PATCH SURFACE                                   
C                                                                       
      GOTO 16
   13 I1= M+1
      WRITE (2,59)  I1, IPT(2), XW1, YW1, ZW1, XW2, YW2, ZW2, ITG, NS
C***
      IF( ITG.LT.1.OR. NS.LT.1) GOTO 17
C 14    READ (5,42) GM,IX,IY,X3,Y3,Z3,X4,Y4,Z4                           
C***
   14 CALL READGM( GM, IX, IY, X3, Y3, Z3, X4, Y4, Z4, DUMMY)
      IF( NS.NE.2.AND. ITG.LT.1) GOTO 15
      X4= XW1+ X3- XW2
      Y4= YW1+ Y3- YW2
      Z4= ZW1+ Z3- ZW2
   15 WRITE (2,39)  X3, Y3, Z3, X4, Y4, Z4
      IF( GM.NE. ATST(11)) GOTO 17
   16 CALL PATCH( ITG, NS, XW1, YW1, ZW1, XW2, YW2, ZW2, X3, Y3, Z3, X4
     &, Y4, Z4)
      GOTO 1
   17 WRITE (2,60) 
C                                                                       
C     REFLECT STRUCTURE ALONG X,Y, OR Z AXES OR ROTATE TO FORM CYLINDER.
C                                                                       
      STOP
   18 IY= NS/10
      IZ= NS- IY*10
      IX= IY/10
      IY= IY- IX*10
      IF( IX.NE.0) IX=1
      IF( IY.NE.0) IY=1
      IF( IZ.NE.0) IZ=1
      WRITE (2,44)  IFX( IX+1), IFY( IY+1), IFZ( IZ+1), ITG
      GOTO 20
   19 WRITE (2,45)  NS, ITG
      IX=-1
   20 CALL REFLC( IX, IY, IZ, ITG, NS)
C                                                                       
C     SCALE STRUCTURE DIMENSIONS BY FACTOR XW1.                         
C                                                                       
      GOTO 1
   21 IF( N.LT. N2) GOTO 23
      DO 22  I= N2, N
      X( I)= X( I)* XW1
      Y( I)= Y( I)* XW1
      Z( I)= Z( I)* XW1
      X2( I)= X2( I)* XW1
      Y2( I)= Y2( I)* XW1
      Z2( I)= Z2( I)* XW1
   22 BI( I)= BI( I)* XW1
   23 IF( M.LT. M2) GOTO 25
      YW1= XW1* XW1
      IX= LD+1- M
      IY= LD- M1
      DO 24  I= IX, IY
      X( I)= X( I)* XW1
      Y( I)= Y( I)* XW1
      Z( I)= Z( I)* XW1
   24 BI( I)= BI( I)* YW1
   25 WRITE (2,46)  XW1
C                                                                       
C     MOVE STRUCTURE OR REPRODUCE ORIGINAL STRUCTURE IN NEW POSITIONS.  
C                                                                       
      GOTO 1
   26 WRITE (2,47)  ITG, NS, XW1, YW1, ZW1, XW2, YW2, ZW2, RAD
      XW1= XW1* TA
      YW1= YW1* TA
      ZW1= ZW1* TA
      CALL MOVE( XW1, YW1, ZW1, XW2, YW2, ZW2, INT( RAD+.5), NS, ITG)
C                                                                       
C     READ NUMERICAL GREEN'S FUNCTION TAPE                              
C                                                                       
      GOTO 1
   27 IF( N+ M.EQ.0) GOTO 28
      WRITE (2,52) 
      STOP
   28 CALL GFIL( ITG)
      NPSAV= NP
      MPSAV= MP
      IPSAV= IPSYM
C                                                                       
C     TERMINATE STRUCTURE GEOMETRY INPUT.                               
C                                                                       
C***
      GOTO 1
   29 IF( NS.EQ.0) GOTO 290
      IPLP1=1
      IPLP2=1
C***
  290 IX= N1+ M1
      IF( IX.EQ.0) GOTO 30
      NP= N
      MP= M
      IPSYM=0
   30 CALL CONECT( ITG)
      IF( IX.EQ.0) GOTO 31
      NP= NPSAV
      MP= MPSAV
      IPSYM= IPSAV
   31 IF( N+ M.GT. LD) GOTO 37
      IF( N.EQ.0) GOTO 33
      WRITE (2,53) 
      WRITE (2,54) 
      DO 32  I=1, N
      XW1= X2( I)- X( I)
      YW1= Y2( I)- Y( I)
      ZW1= Z2( I)- Z( I)
      X( I)=( X( I)+ X2( I))*.5
      Y( I)=( Y( I)+ Y2( I))*.5
      Z( I)=( Z( I)+ Z2( I))*.5
      XW2= XW1* XW1+ YW1* YW1+ ZW1* ZW1
      YW2= SQRT( XW2)
      YW2=( XW2/ YW2+ YW2)*.5
      SI( I)= YW2
      CAB( I)= XW1/ YW2
      SAB( I)= YW1/ YW2
      XW2= ZW1/ YW2
      IF( XW2.GT.1.) XW2=1.
      IF( XW2.LT.-1.) XW2=-1.
      SALP( I)= XW2
      XW2= ASIN( XW2)* TD
      YW2= ATGN2( YW1, XW1)* TD
C***
      WRITE (2,55)  I, X( I), Y( I), Z( I), SI( I), XW2, YW2, BI( I), 
     &ICON1( I), I, ICON2( I), ITAG( I)
      IF( IPLP1.NE.1) GOTO 320
      WRITE( 8,*)  X( I), Y( I), Z( I), SI( I), XW2, YW2, BI( I), ICON1
     &( I), I, ICON2( I)
C***
  320 CONTINUE
      IF( SI( I).GT.1.D-20.AND. BI( I).GT.0.) GOTO 32
      WRITE (2,56) 
      STOP
   32 CONTINUE
   33 IF( M.EQ.0) GOTO 35
      WRITE (2,57) 
      J= LD+1
      DO 34  I=1, M
      J= J-1
      XW1=( T1Y( J)* T2Z( J)- T1Z( J)* T2Y( J))* SALP( J)
      YW1=( T1Z( J)* T2X( J)- T1X( J)* T2Z( J))* SALP( J)
      ZW1=( T1X( J)* T2Y( J)- T1Y( J)* T2X( J))* SALP( J)
      WRITE (2,58)  I, X( J), Y( J), Z( J), XW1, YW1, ZW1, BI( J), T1X(
     & J), T1Y( J), T1Z( J), T2X( J), T2Y( J), T2Z( J)
   34 CONTINUE
   35 RETURN
   36 WRITE (2,48) 
      WRITE (2,49)  GM, ITG, NS, XW1, YW1, ZW1, XW2, YW2, ZW2, RAD
      STOP
   37 WRITE (2,50) 
C                                                                       
      STOP
   38 FORMAT(1X,I5,2X,'ARC RADIUS =',F9.5,2X,'FROM',F8.3,' TO',F8.3,
     &' DEGREES',11X,F11.5,2X,I5,4X,I5,1X,I5,3X,I5)
   39 FORMAT(6X,3F11.5,1X,3F11.5)
   40 FORMAT(////,33X,'- - - STRUCTURE SPECIFICATION - - -',//,37X,
     &'COORDINATES MUST BE INPUT IN',/,37X,
     &'METERS OR BE SCALED TO METERS',/,37X,
     &'BEFORE STRUCTURE INPUT IS ENDED',//)
   41 FORMAT(2X,'WIRE',79X,'NO. OF',4X,'FIRST',2X,'LAST',5X,'TAG',/,2X,
     &'NO.',8X,'X1',9X,'Y1',9X,'Z1',10X,'X2',9X,'Y2',9X,'Z2',6X,
     &'RADIUS',3X,'SEG.',5X,'SEG.',3X,'SEG.',5X,'NO.')
   42 FORMAT(A2, I3, I5, 7F10.5)
   43 FORMAT(1X,I5,3F11.5,1X,4F11.5,2X,I5,4X,I5,1X,I5,3X,I5)
   44 FORMAT(6X,'STRUCTURE REFLECTED ALONG THE AXES',3(1X,A1),'.  TA',
     &'GS INCREMENTED BY',I5)
   45 FORMAT(6X,'STRUCTURE ROTATED ABOUT Z-AXIS',I3,' TIMES.  LABELS',
     &' INCREMENTED BY',I5)
   46 FORMAT(6X,'STRUCTURE SCALED BY FACTOR',F10.5)
   47 FORMAT(6X,'THE STRUCTURE HAS BEEN MOVED, MOVE DATA CARD IS -/6X',
     &I3,I5,7F10.5)
   48 FORMAT(' GEOMETRY DATA CARD ERROR')
   49 FORMAT(1X,A2,I3,I5,7F10.5)
   50 FORMAT(' NUMBER OF WIRE SEGMENTS AND SURFACE PATCHES EXCEEDS DI',
     &'MENSION LIMIT.')
   51 FORMAT(1X,I5,A1,F10.5,2F11.5,1X,3F11.5)
   52 FORMAT(' ERROR - GF MUST BE FIRST GEOMETRY DATA CARD')
   53 FORMAT(////33X,'- - - - SEGMENTATION DATA - - - -',//,40X,'COO',
     &'RDINATES IN METERS',//,25X,
     &'I+ AND I- INDICATE THE SEGMENTS BEFORE AND AFTER I',//)
   54 FORMAT(2X,'SEG.',3X,'COORDINATES OF SEG. CENTER',5X,'SEG.',5X,
     &'ORIENTATION ANGLES',4X,'WIRE',4X,'CONNECTION DATA',3X,'TAG',/,2X
     &,'NO.',7X,'X',9X,'Y',9X,'Z',7X,'LENGTH',5X,'ALPHA',5X,'BETA',6X,
     &'RADIUS',4X,'I-',3X,'I',4X,'I+',4X,'NO.')
   55 FORMAT(1X,I5,4F10.5,1X,3F10.5,1X,3I5,2X,I5)
   56 FORMAT(' SEGMENT DATA ERROR')
   57 FORMAT(////,44X,'- - - SURFACE PATCH DATA - - -',//,49X,'COORD',
     &'INATES IN METERS',//,1X,'PATCH',5X,'COORD. OF PATCH CENTER',7X,
     &'UNIT NORMAL VECTOR',6X,'PATCH',12X,
     &'COMPONENTS OF UNIT TANGENT V''ECTORS',/,2X,'NO.',6X,'X',9X,'Y',9
     &X,'Z',9X,'X',7X,'Y',7X,'Z',7X,'AREA',7X,'X1',6X,'Y1',6X,'Z1',7X,
     &'X2',6X,'Y2',6X,'Z2')
   58 FORMAT(1X,I4,3F10.5,1X,3F8.4,F10.5,1X,3F8.4,1X,3F8.4)
   59 FORMAT(1X,I5,A1,F10.5,2F11.5,1X,3F11.5,5X,'SURFACE -',I4,' BY',I3
     &,' PATCHES')
   60 FORMAT(' PATCH DATA ERROR')
   61 FORMAT(9X,'ABOVE WIRE IS TAPERED.  SEG. LENGTH RATIO =',F9.5,/,33
     &X,'RADIUS FROM',F9.5,' TO',F9.5)
      END
C ***
C     DOUBLE PRECISION 6/4/85
C
      FUNCTION DB10( X)
C ***
C                                                                       
C     FUNCTION DB-- RETURNS DB FOR MAGNITUDE (FIELD) OR MAG**2 (POWER) I
C                                                                       
      IMPLICIT REAL (A-H,O-Z)
      F=10.
      GOTO 1
      ENTRY DB20 (x)
      F=20.
    1 IF( X.LT.1.D-20) GOTO 2
      DB10= F* LOG10( X)
      RETURN
    2 DB10=-999.99
      RETURN
      END
C ***
C     DOUBLE PRECISION 6/4/85
C
      SUBROUTINE EFLD( XI, YI, ZI, AI, IJ)
C ***
      IMPLICIT REAL (A-H,O-Z)
C                                                                       
C     COMPUTE NEAR E FIELDS OF A SEGMENT WITH SINE, COSINE, AND         
C     CONSTANT CURRENTS.  GROUND EFFECT INCLUDED.                       
C                                                                       
      PARAMETER ( NM=600, N2M=800, N3M=1000)
      COMPLEX  TXK, TYK, TZK, TXS, TYS, TZS, TXC, TYC, TZC, EXK, EYK
     &, EZK, EXS, EYS, EZS, EXC, EYC, EZC, EPX, EPY, ZRATI, REFS, REFPS
     &, ZRSIN, ZRATX, T1, ZSCRN, ZRATI2, TEZS, TERS, TEZC, TERC, TEZK, 
     &TERK, EGND, FRATI
      COMMON  /DATAJ/ S, B, XJ, YJ, ZJ, CABJ, SABJ, SALPJ, EXK, EYK, 
     &EZK, EXS, EYS, EZS, EXC, EYC, EZC, RKH, IEXK, IND1, INDD1, IND2, 
     &INDD2, IPGND
      COMMON  /GND/ ZRATI, ZRATI2, FRATI, CL, CH, SCRWL, SCRWR, NRADL, 
     &KSYMP, IFAR, IPERF, T1, T2
      COMMON  /INCOM/ XO, YO, ZO, SN, XSN, YSN, ISNOR
      DIMENSION  EGND(9)
      EQUIVALENCE(EGND(1),TXK),(EGND(2),TYK),(EGND(3),TZK),(EGND(4),TXS
     &),(EGND(5),TYS),(EGND(6),TZS),(EGND(7),TXC),(EGND(8),TYC),(EGND(9
     &),TZC)
      DATA   ETA/376.73/, PI/3.141592654D+0/, TP/6.283185308D+0/
      XIJ= XI- XJ
      YIJ= YI- YJ
      IJX= IJ
      RFL=-1.
      DO 12  IP=1, KSYMP
      IF( IP.EQ.2) IJX=1
      RFL=- RFL
      SALPR= SALPJ* RFL
      ZIJ= ZI- RFL* ZJ
      ZP= XIJ* CABJ+ YIJ* SABJ+ ZIJ* SALPR
      RHOX= XIJ- CABJ* ZP
      RHOY= YIJ- SABJ* ZP
      RHOZ= ZIJ- SALPR* ZP
      RH= SQRT( RHOX* RHOX+ RHOY* RHOY+ RHOZ* RHOZ+ AI* AI)
      IF( RH.GT.1.D-10) GOTO 1
      RHOX=0.
      RHOY=0.
      RHOZ=0.
      GOTO 2
    1 RHOX= RHOX/ RH
      RHOY= RHOY/ RH
      RHOZ= RHOZ/ RH
    2 R= SQRT( ZP* ZP+ RH* RH)
C                                                                       
C     LUMPED CURRENT ELEMENT APPROX. FOR LARGE SEPARATIONS              
C                                                                       
      IF( R.LT. RKH) GOTO 3
      RMAG= TP* R
      CTH= ZP/ R
      PX= RH/ R
      TXK= CMPLX( COS( RMAG),- SIN( RMAG))
      PY= TP* R* R
      TYK= ETA* CTH* TXK* CMPLX(1.D+0,-1.D+0/ RMAG)/ PY
      TZK= ETA* PX* TXK* CMPLX(1.D+0, RMAG-1.D+0/ RMAG)/(2.* PY)
      TEZK= TYK* CTH- TZK* PX
      TERK= TYK* PX+ TZK* CTH
      RMAG= SIN( PI* S)/ PI
      TEZC= TEZK* RMAG
      TERC= TERK* RMAG
      TEZK= TEZK* S
      TERK= TERK* S
      TXS=(0.,0.)
      TYS=(0.,0.)
      TZS=(0.,0.)
      GOTO 6
C                                                                       
C     EKSC FOR THIN WIRE APPROX. OR EKSCX FOR EXTENDED T.W. APPROX.     
C                                                                       
    3 IF( IEXK.EQ.1) GOTO 4
      CALL EKSC( S, ZP, RH, TP, IJX, TEZS, TERS, TEZC, TERC, TEZK, TERK
     &)
      GOTO 5
    4 CALL EKSCX( B, S, ZP, RH, TP, IJX, IND1, IND2, TEZS, TERS, TEZC, 
     &TERC, TEZK, TERK)
    5 TXS= TEZS* CABJ+ TERS* RHOX
      TYS= TEZS* SABJ+ TERS* RHOY
      TZS= TEZS* SALPR+ TERS* RHOZ
    6 TXK= TEZK* CABJ+ TERK* RHOX
      TYK= TEZK* SABJ+ TERK* RHOY
      TZK= TEZK* SALPR+ TERK* RHOZ
      TXC= TEZC* CABJ+ TERC* RHOX
      TYC= TEZC* SABJ+ TERC* RHOY
      TZC= TEZC* SALPR+ TERC* RHOZ
      IF( IP.NE.2) GOTO 11
      IF( IPERF.GT.0) GOTO 10
      ZRATX= ZRATI
      RMAG= R
C                                                                       
C     SET PARAMETERS FOR RADIAL WIRE GROUND SCREEN.                     
C                                                                       
      XYMAG= SQRT( XIJ* XIJ+ YIJ* YIJ)
      IF( NRADL.EQ.0) GOTO 7
      XSPEC=( XI* ZJ+ ZI* XJ)/( ZI+ ZJ)
      YSPEC=( YI* ZJ+ ZI* YJ)/( ZI+ ZJ)
      RHOSPC= SQRT( XSPEC* XSPEC+ YSPEC* YSPEC+ T2* T2)
      IF( RHOSPC.GT. SCRWL) GOTO 7
      ZSCRN= T1* RHOSPC* LOG( RHOSPC/ T2)
      ZRATX=( ZSCRN* ZRATI)/( ETA* ZRATI+ ZSCRN)
C                                                                       
C     CALCULATION OF REFLECTION COEFFICIENTS WHEN GROUND IS SPECIFIED.  
C                                                                       
    7 IF( XYMAG.GT.1.D-6) GOTO 8
      PX=0.
      PY=0.
      CTH=1.
      ZRSIN=(1.,0.)
      GOTO 9
    8 PX=- YIJ/ XYMAG
      PY= XIJ/ XYMAG
      CTH= ZIJ/ RMAG
      ZRSIN= SQRT(1.- ZRATX* ZRATX*(1.- CTH* CTH))
    9 REFS=( CTH- ZRATX* ZRSIN)/( CTH+ ZRATX* ZRSIN)
      REFPS=-( ZRATX* CTH- ZRSIN)/( ZRATX* CTH+ ZRSIN)
      REFPS= REFPS- REFS
      EPY= PX* TXK+ PY* TYK
      EPX= PX* EPY
      EPY= PY* EPY
      TXK= REFS* TXK+ REFPS* EPX
      TYK= REFS* TYK+ REFPS* EPY
      TZK= REFS* TZK
      EPY= PX* TXS+ PY* TYS
      EPX= PX* EPY
      EPY= PY* EPY
      TXS= REFS* TXS+ REFPS* EPX
      TYS= REFS* TYS+ REFPS* EPY
      TZS= REFS* TZS
      EPY= PX* TXC+ PY* TYC
      EPX= PX* EPY
      EPY= PY* EPY
      TXC= REFS* TXC+ REFPS* EPX
      TYC= REFS* TYC+ REFPS* EPY
      TZC= REFS* TZC
   10 EXK= EXK- TXK* FRATI
      EYK= EYK- TYK* FRATI
      EZK= EZK- TZK* FRATI
      EXS= EXS- TXS* FRATI
      EYS= EYS- TYS* FRATI
      EZS= EZS- TZS* FRATI
      EXC= EXC- TXC* FRATI
      EYC= EYC- TYC* FRATI
      EZC= EZC- TZC* FRATI
      GOTO 12
   11 EXK= TXK
      EYK= TYK
      EZK= TZK
      EXS= TXS
      EYS= TYS
      EZS= TZS
      EXC= TXC
      EYC= TYC
      EZC= TZC
   12 CONTINUE
      IF( IPERF.EQ.2) GOTO 13
C                                                                       
C     FIELD DUE TO GROUND USING SOMMERFELD/NORTON                       
C                                                                       
      RETURN
   13 SN= SQRT( CABJ* CABJ+ SABJ* SABJ)
      IF( SN.LT.1.D-5) GOTO 14
      XSN= CABJ/ SN
      YSN= SABJ/ SN
      GOTO 15
   14 SN=0.
      XSN=1.
C                                                                       
C     DISPLACE OBSERVATION POINT FOR THIN WIRE APPROXIMATION            
C                                                                       
      YSN=0.
   15 ZIJ= ZI+ ZJ
      SALPR=- SALPJ
      RHOX= SABJ* ZIJ- SALPR* YIJ
      RHOY= SALPR* XIJ- CABJ* ZIJ
      RHOZ= CABJ* YIJ- SABJ* XIJ
      RH= RHOX* RHOX+ RHOY* RHOY+ RHOZ* RHOZ
      IF( RH.GT.1.D-10) GOTO 16
      XO= XI- AI* YSN
      YO= YI+ AI* XSN
      ZO= ZI
      GOTO 17
   16 RH= AI/ SQRT( RH)
      IF( RHOZ.LT.0.) RH=- RH
      XO= XI+ RH* RHOX
      YO= YI+ RH* RHOY
      ZO= ZI+ RH* RHOZ
   17 R= XIJ* XIJ+ YIJ* YIJ+ ZIJ* ZIJ
C                                                                       
C     FIELD FROM INTERPOLATION IS INTEGRATED OVER SEGMENT               
C                                                                       
      IF( R.GT..95) GOTO 18
      ISNOR=1
      DMIN= EXK* CONJG( EXK)+ EYK* CONJG( EYK)+ EZK* CONJG( EZK)
      DMIN=.01* SQRT( DMIN)
      SHAF=.5* S
      CALL ROM2(- SHAF, SHAF, EGND, DMIN)
C                                                                       
C     NORTON FIELD EQUATIONS AND LUMPED CURRENT ELEMENT APPROXIMATION   
C                                                                       
      GOTO 19
   18 ISNOR=2
      CALL SFLDS(0., EGND)
      GOTO 22
   19 ZP= XIJ* CABJ+ YIJ* SABJ+ ZIJ* SALPR
      RH= R- ZP* ZP
      IF( RH.GT.1.D-10) GOTO 20
      DMIN=0.
      GOTO 21
   20 DMIN= SQRT( RH/( RH+ AI* AI))
   21 IF( DMIN.GT..95) GOTO 22
      PX=1.- DMIN
      TERK=( TXK* CABJ+ TYK* SABJ+ TZK* SALPR)* PX
      TXK= DMIN* TXK+ TERK* CABJ
      TYK= DMIN* TYK+ TERK* SABJ
      TZK= DMIN* TZK+ TERK* SALPR
      TERS=( TXS* CABJ+ TYS* SABJ+ TZS* SALPR)* PX
      TXS= DMIN* TXS+ TERS* CABJ
      TYS= DMIN* TYS+ TERS* SABJ
      TZS= DMIN* TZS+ TERS* SALPR
      TERC=( TXC* CABJ+ TYC* SABJ+ TZC* SALPR)* PX
      TXC= DMIN* TXC+ TERC* CABJ
      TYC= DMIN* TYC+ TERC* SABJ
      TZC= DMIN* TZC+ TERC* SALPR
   22 EXK= EXK+ TXK
      EYK= EYK+ TYK
      EZK= EZK+ TZK
      EXS= EXS+ TXS
      EYS= EYS+ TYS
      EZS= EZS+ TZS
      EXC= EXC+ TXC
      EYC= EYC+ TYC
      EZC= EZC+ TZC
      RETURN
      END
C ***
C     DOUBLE PRECISION 6/4/85
C
      SUBROUTINE EKSC( S, Z, RH, XK, IJ, EZS, ERS, EZC, ERC, EZK, ERK)
C ***
      IMPLICIT REAL (A-H,O-Z)
C     COMPUTE E FIELD OF SINE, COSINE, AND CONSTANT CURRENT FILAMENTS BY
C     THIN WIRE APPROXIMATION.                                          
      PARAMETER ( NM=600, N2M=800, N3M=1000)
      COMPLEX  CON, GZ1, GZ2, GP1, GP2, GZP1, GZP2, EZS, ERS, EZC, 
     &ERC, EZK, ERK
      COMMON  /TMI/ ZPK, RKB2, IJX
      DIMENSION  CONX(2)
      EQUIVALENCE(CONX,CON)
      DATA   CONX/0.,4.771341189D+0/
      IJX= IJ
      ZPK= XK* Z
      RHK= XK* RH
      RKB2= RHK* RHK
      SH=.5* S
      SHK= XK* SH
      SS= SIN( SHK)
      CS= COS( SHK)
      Z2= SH- Z
      Z1=-( SH+ Z)
      CALL GX( Z1, RH, XK, GZ1, GP1)
      CALL GX( Z2, RH, XK, GZ2, GP2)
      GZP1= GP1* Z1
      GZP2= GP2* Z2
      EZS= CON*(( GZ2- GZ1)* CS* XK-( GZP2+ GZP1)* SS)
      EZC=- CON*(( GZ2+ GZ1)* SS* XK+( GZP2- GZP1)* CS)
      ERK= CON*( GP2- GP1)* RH
      CALL INTX(- SHK, SHK, RHK, IJ, CINT, SINT)
      EZK=- CON*( GZP2- GZP1+ XK* XK* CMPLX( CINT,- SINT))
      GZP1= GZP1* Z1
      GZP2= GZP2* Z2
      IF( RH.LT.1.D-10) GOTO 1
      ERS=- CON*(( GZP2+ GZP1+ GZ2+ GZ1)* SS-( Z2* GZ2- Z1* GZ1)* CS* 
     &XK)/ RH
      ERC=- CON*(( GZP2- GZP1+ GZ2- GZ1)* CS+( Z2* GZ2+ Z1* GZ1)* SS* 
     &XK)/ RH
      RETURN
    1 ERS=(0.,0.)
      ERC=(0.,0.)
      RETURN
      END
C ***
C     DOUBLE PRECISION 6/4/85
C
      SUBROUTINE EKSCX( BX, S, Z, RHX, XK, IJ, INX1, INX2, EZS, ERS, 
     &EZC, ERC, EZK, ERK)
C ***
C     COMPUTE E FIELD OF SINE, COSINE, AND CONSTANT CURRENT FILAMENTS BY
C     EXTENDED THIN WIRE APPROXIMATION.                                 
      IMPLICIT REAL (A-H,O-Z)
      PARAMETER ( NM=600, N2M=800, N3M=1000)
      COMPLEX  CON, GZ1, GZ2, GZP1, GZP2, GR1, GR2, GRP1, GRP2, EZS,
     & EZC, ERS, ERC, GRK1, GRK2, EZK, ERK, GZZ1, GZZ2
      COMMON  /TMI/ ZPK, RKB2, IJX
      DIMENSION  CONX(2)
      EQUIVALENCE(CONX,CON)
      DATA   CONX/0.,4.771341189D+0/
      IF( RHX.LT. BX) GOTO 1
      RH= RHX
      B= BX
      IRA=0
      GOTO 2
    1 RH= BX
      B= RHX
      IRA=1
    2 SH=.5* S
      IJX= IJ
      ZPK= XK* Z
      RHK= XK* RH
      RKB2= RHK* RHK
      SHK= XK* SH
      SS= SIN( SHK)
      CS= COS( SHK)
      Z2= SH- Z
      Z1=-( SH+ Z)
      A2= B* B
      IF( INX1.EQ.2) GOTO 3
      CALL GXX( Z1, RH, B, A2, XK, IRA, GZ1, GZP1, GR1, GRP1, GRK1, 
     &GZZ1)
      GOTO 4
    3 CALL GX( Z1, RHX, XK, GZ1, GRK1)
      GZP1= GRK1* Z1
      GR1= GZ1/ RHX
      GRP1= GZP1/ RHX
      GRK1= GRK1* RHX
      GZZ1=(0.,0.)
    4 IF( INX2.EQ.2) GOTO 5
      CALL GXX( Z2, RH, B, A2, XK, IRA, GZ2, GZP2, GR2, GRP2, GRK2, 
     &GZZ2)
      GOTO 6
    5 CALL GX( Z2, RHX, XK, GZ2, GRK2)
      GZP2= GRK2* Z2
      GR2= GZ2/ RHX
      GRP2= GZP2/ RHX
      GRK2= GRK2* RHX
      GZZ2=(0.,0.)
    6 EZS= CON*(( GZ2- GZ1)* CS* XK-( GZP2+ GZP1)* SS)
      EZC=- CON*(( GZ2+ GZ1)* SS* XK+( GZP2- GZP1)* CS)
      ERS=- CON*(( Z2* GRP2+ Z1* GRP1+ GR2+ GR1)* SS-( Z2* GR2- Z1* GR1
     &)* CS* XK)
      ERC=- CON*(( Z2* GRP2- Z1* GRP1+ GR2- GR1)* CS+( Z2* GR2+ Z1* GR1
     &)* SS* XK)
      ERK= CON*( GRK2- GRK1)
      CALL INTX(- SHK, SHK, RHK, IJ, CINT, SINT)
      BK= B* XK
      BK2= BK* BK*.25
      EZK=- CON*( GZP2- GZP1+ XK* XK*(1.- BK2)* CMPLX( CINT,- SINT)- 
     &BK2*( GZZ2- GZZ1))
      RETURN
      END
C ***
C     DOUBLE PRECISION 6/4/85
C
C      LOGICAL FUNCTION ENF( NUNIT)
C ***
C*********** THIS ROUTINE NOT USED ON VAX **************
C      IF (EOF,NUNIT) 1,2                                                
C      IMPLICIT REAL (A-H,O-Z)
C    1 ENF=.TRUE.
C      RETURN
C    2 ENF=.FALSE.
C      RETURN
C      END
C ***
C     DOUBLE PRECISION 6/4/85
C
C     IMPLICIT REAL(A-H,O-Z)
C ***
      SUBROUTINE ERROR
      IMPLICIT INTEGER (A-Z)
      CHARACTER   MSG*80
C      CALL SYS$GETMSG(%VAL(RMSSTS),MSGLEN,MSG,,,)
C      CALL ERRSNS( FNUM, RMSSTS, RMSSTV, IUNIT, CNDVAL)
      CALL STR0PC( MSG, MSG)
      IND= INDEX( MSG,',')
      PRINT1 , MSG( IND+2: MSGLEN)
    1 FORMAT(//,'  ****  ERROR  ****   ',//,5X,A,//)
      RETURN
      END
C ***
C     DOUBLE PRECISION 6/4/85
C
      SUBROUTINE ETMNS( P1, P2, P3, P4, P5, P6, IPR, E)
      PARAMETER ( NM=600, N2M=800, N3M=1000)
C ***
C                                                                       
C     ETMNS FILLS THE ARRAY E WITH THE NEGATIVE OF THE ELECTRIC FIELD   
C     INCIDENT ON THE STRUCTURE.  E IS THE RIGHT HAND SIDE OF THE MATRIX
C     EQUATION.                                                         
C                                                                       
      IMPLICIT REAL (A-H,O-Z)
      COMPLEX  E, CX, CY, CZ, VSANT, ER, ET, EZH, ERH, VQD
     &, VQDS, ZRATI, ZRATI2, RRV, RRH, T1, TT1, TT2, FRATI
      COMMON  /DATA/ LD, N1, N2, N, NP, M1, M2, M, MP, X( NM), Y( NM), 
     &Z( NM), SI( NM), BI( NM), ALP( NM), BET( NM), ICON1( N2M), ICON2(
     & N2M), ITAG( N2M), ICONX( NM), WLAM, IPSYM
      COMMON  /ANGL/ SALP( NM)
      COMMON  /VSORC/ VQD(30), VSANT(30), VQDS(30), IVQD(30), ISANT(30)
     &, IQDS(30), NVQD, NSANT, NQDS
      COMMON  /GND/ ZRATI, ZRATI2, FRATI, CL, CH, SCRWL, SCRWR, NRADL, 
     &KSYMP, IFAR, IPERF, T1, T2
      DIMENSION  CAB(1), SAB(1), E( N2M)
      DIMENSION  T1X(1), T1Y(1), T1Z(1), T2X(1), T2Y(1), T2Z(1)
      EQUIVALENCE(CAB,ALP),(SAB,BET)
      EQUIVALENCE(T1X,SI),(T1Y,ALP),(T1Z,BET),(T2X,ICON1),(T2Y,ICON2),(
     &T2Z,ITAG)
      DATA   TP/6.283185308D+0/, RETA/2.654420938D-3/
      NEQ= N+2* M
      NQDS=0
C                                                                       
C     APPLIED FIELD OF VOLTAGE SOURCES FOR TRANSMITTING CASE            
C                                                                       
      IF( IPR.GT.0.AND. IPR.NE.5) GOTO 5
      DO 1  I=1, NEQ
    1 E( I)=(0.,0.)
      IF( NSANT.EQ.0) GOTO 3
      DO 2  I=1, NSANT
      IS= ISANT( I)
    2 E( IS)=- VSANT( I)/( SI( IS)* WLAM)
    3 IF( NVQD.EQ.0) RETURN
      DO 4  I=1, NVQD
      IS= IVQD( I)
    4 CALL QDSRC( IS, VQD( I), E)
      RETURN
C                                                                       
C     INCIDENT PLANE WAVE, LINEARLY POLARIZED.                          
C                                                                       
    5 IF( IPR.GT.3) GOTO 19
      CTH= COS( P1)
      STH= SIN( P1)
      CPH= COS( P2)
      SPH= SIN( P2)
      CET= COS( P3)
      SET= SIN( P3)
      PX= CTH* CPH* CET- SPH* SET
      PY= CTH* SPH* CET+ CPH* SET
      PZ=- STH* CET
      WX=- STH* CPH
      WY=- STH* SPH
      WZ=- CTH
      QX= WY* PZ- WZ* PY
      QY= WZ* PX- WX* PZ
      QZ= WX* PY- WY* PX
      IF( KSYMP.EQ.1) GOTO 7
      IF( IPERF.EQ.1) GOTO 6
      RRV= SQRT(1.- ZRATI* ZRATI* STH* STH)
      RRH= ZRATI* CTH
      RRH=( RRH- RRV)/( RRH+ RRV)
      RRV= ZRATI* RRV
      RRV=-( CTH- RRV)/( CTH+ RRV)
      GOTO 7
    6 RRV=-(1.,0.)
      RRH=-(1.,0.)
    7 IF( IPR.GT.1) GOTO 13
      IF( N.EQ.0) GOTO 10
      DO 8  I=1, N
      ARG=- TP*( WX* X( I)+ WY* Y( I)+ WZ* Z( I))
    8 E( I)=-( PX* CAB( I)+ PY* SAB( I)+ PZ* SALP( I))* CMPLX( COS( ARG
     &), SIN( ARG))
      IF( KSYMP.EQ.1) GOTO 10
      TT1=( PY* CPH- PX* SPH)*( RRH- RRV)
      CX= RRV* PX- TT1* SPH
      CY= RRV* PY+ TT1* CPH
      CZ=- RRV* PZ
      DO 9  I=1, N
      ARG=- TP*( WX* X( I)+ WY* Y( I)- WZ* Z( I))
    9 E( I)= E( I)-( CX* CAB( I)+ CY* SAB( I)+ CZ* SALP( I))* CMPLX( 
     &COS( ARG), SIN( ARG))
   10 IF( M.EQ.0) RETURN
      I= LD+1
      I1= N-1
      DO 11  IS=1, M
      I= I-1
      I1= I1+2
      I2= I1+1
      ARG=- TP*( WX* X( I)+ WY* Y( I)+ WZ* Z( I))
      TT1= CMPLX( COS( ARG), SIN( ARG))* SALP( I)* RETA
      E( I2)=( QX* T1X( I)+ QY* T1Y( I)+ QZ* T1Z( I))* TT1
   11 E( I1)=( QX* T2X( I)+ QY* T2Y( I)+ QZ* T2Z( I))* TT1
      IF( KSYMP.EQ.1) RETURN
      TT1=( QY* CPH- QX* SPH)*( RRV- RRH)
      CX=-( RRH* QX- TT1* SPH)
      CY=-( RRH* QY+ TT1* CPH)
      CZ= RRH* QZ
      I= LD+1
      I1= N-1
      DO 12  IS=1, M
      I= I-1
      I1= I1+2
      I2= I1+1
      ARG=- TP*( WX* X( I)+ WY* Y( I)- WZ* Z( I))
      TT1= CMPLX( COS( ARG), SIN( ARG))* SALP( I)* RETA
      E( I2)= E( I2)+( CX* T1X( I)+ CY* T1Y( I)+ CZ* T1Z( I))* TT1
   12 E( I1)= E( I1)+( CX* T2X( I)+ CY* T2Y( I)+ CZ* T2Z( I))* TT1
C                                                                       
C     INCIDENT PLANE WAVE, ELLIPTIC POLARIZATION.                       
C                                                                       
      RETURN
   13 TT1=-(0.,1.)* P6
      IF( IPR.EQ.3) TT1=- TT1
      IF( N.EQ.0) GOTO 16
      CX= PX+ TT1* QX
      CY= PY+ TT1* QY
      CZ= PZ+ TT1* QZ
      DO 14  I=1, N
      ARG=- TP*( WX* X( I)+ WY* Y( I)+ WZ* Z( I))
   14 E( I)=-( CX* CAB( I)+ CY* SAB( I)+ CZ* SALP( I))* CMPLX( COS( ARG
     &), SIN( ARG))
      IF( KSYMP.EQ.1) GOTO 16
      TT2=( CY* CPH- CX* SPH)*( RRH- RRV)
      CX= RRV* CX- TT2* SPH
      CY= RRV* CY+ TT2* CPH
      CZ=- RRV* CZ
      DO 15  I=1, N
      ARG=- TP*( WX* X( I)+ WY* Y( I)- WZ* Z( I))
   15 E( I)= E( I)-( CX* CAB( I)+ CY* SAB( I)+ CZ* SALP( I))* CMPLX( 
     &COS( ARG), SIN( ARG))
   16 IF( M.EQ.0) RETURN
      CX= QX- TT1* PX
      CY= QY- TT1* PY
      CZ= QZ- TT1* PZ
      I= LD+1
      I1= N-1
      DO 17  IS=1, M
      I= I-1
      I1= I1+2
      I2= I1+1
      ARG=- TP*( WX* X( I)+ WY* Y( I)+ WZ* Z( I))
      TT2= CMPLX( COS( ARG), SIN( ARG))* SALP( I)* RETA
      E( I2)=( CX* T1X( I)+ CY* T1Y( I)+ CZ* T1Z( I))* TT2
   17 E( I1)=( CX* T2X( I)+ CY* T2Y( I)+ CZ* T2Z( I))* TT2
      IF( KSYMP.EQ.1) RETURN
      TT1=( CY* CPH- CX* SPH)*( RRV- RRH)
      CX=-( RRH* CX- TT1* SPH)
      CY=-( RRH* CY+ TT1* CPH)
      CZ= RRH* CZ
      I= LD+1
      I1= N-1
      DO 18  IS=1, M
      I= I-1
      I1= I1+2
      I2= I1+1
      ARG=- TP*( WX* X( I)+ WY* Y( I)- WZ* Z( I))
      TT1= CMPLX( COS( ARG), SIN( ARG))* SALP( I)* RETA
      E( I2)= E( I2)+( CX* T1X( I)+ CY* T1Y( I)+ CZ* T1Z( I))* TT1
   18 E( I1)= E( I1)+( CX* T2X( I)+ CY* T2Y( I)+ CZ* T2Z( I))* TT1
C                                                                       
C     INCIDENT FIELD OF AN ELEMENTARY CURRENT SOURCE.                   
C                                                                       
      RETURN
   19 WZ= COS( P4)
      WX= WZ* COS( P5)
      WY= WZ* SIN( P5)
      WZ= SIN( P4)
      DS= P6*59.958
      DSH= P6/(2.* TP)
      NPM= N+ M
      IS= LD+1
      I1= N-1
      DO 24  I=1, NPM
      II= I
      IF( I.LE. N) GOTO 20
      IS= IS-1
      II= IS
      I1= I1+2
      I2= I1+1
   20 PX= X( II)- P1
      PY= Y( II)- P2
      PZ= Z( II)- P3
      RS= PX* PX+ PY* PY+ PZ* PZ
      IF( RS.LT.1.D-30) GOTO 24
      R= SQRT( RS)
      PX= PX/ R
      PY= PY/ R
      PZ= PZ/ R
      CTH= PX* WX+ PY* WY+ PZ* WZ
      STH= SQRT(1.- CTH* CTH)
      QX= PX- WX* CTH
      QY= PY- WY* CTH
      QZ= PZ- WZ* CTH
      ARG= SQRT( QX* QX+ QY* QY+ QZ* QZ)
      IF( ARG.LT.1.D-30) GOTO 21
      QX= QX/ ARG
      QY= QY/ ARG
      QZ= QZ/ ARG
      GOTO 22
   21 QX=1.
      QY=0.
      QZ=0.
   22 ARG=- TP* R
      TT1= CMPLX( COS( ARG), SIN( ARG))
      IF( I.GT. N) GOTO 23
      TT2= CMPLX(1.D+0,-1.D+0/( R* TP))/ RS
      ER= DS* TT1* TT2* CTH
      ET=.5* DS* TT1*((0.,1.)* TP/ R+ TT2)* STH
      EZH= ER* CTH- ET* STH
      ERH= ER* STH+ ET* CTH
      CX= EZH* WX+ ERH* QX
      CY= EZH* WY+ ERH* QY
      CZ= EZH* WZ+ ERH* QZ
      E( I)=-( CX* CAB( I)+ CY* SAB( I)+ CZ* SALP( I))
      GOTO 24
   23 PX= WY* QZ- WZ* QY
      PY= WZ* QX- WX* QZ
      PZ= WX* QY- WY* QX
      TT2= DSH* TT1* CMPLX(1./ R, TP)/ R* STH* SALP( II)
      CX= TT2* PX
      CY= TT2* PY
      CZ= TT2* PZ
      E( I2)= CX* T1X( II)+ CY* T1Y( II)+ CZ* T1Z( II)
      E( I1)= CX* T2X( II)+ CY* T2Y( II)+ CZ* T2Z( II)
   24 CONTINUE
      RETURN
      END
C ***
C     DOUBLE PRECISION 6/4/85
C
      SUBROUTINE FACGF( A, B, C, D, BX, IP, IX, NP, N1, MP, M1, N1C, 
     &N2C)
C ***
C     FACGF COMPUTES AND FACTORS D-C(INV(A)B).                          
      IMPLICIT REAL (A-H,O-Z)
      COMPLEX  A, B, C, D, BX, SUM
      COMMON  /MATPAR/ ICASE, NBLOKS, NPBLK, NLAST, NBLSYM, NPSYM, 
     &NLSYM, IMAT, ICASX, NBBX, NPBX, NLBX, NBBL, NPBL, NLBL
      DIMENSION  A(1), B( N1C,1), C( N1C,1), D( N2C,1), BX( N1C,1), IP(
     &1), IX(1)
      IF( N2C.EQ.0) RETURN
      IBFL=14
C     CONVERT B FROM BLOCKS OF ROWS ON T14 TO BLOCKS OF COL. ON T16     
      IF( ICASX.LT.3) GOTO 1
      CALL REBLK( B, C, N1C, NPBX, N2C)
      IBFL=16
    1 NPB= NPBL
C     COMPUTE INV(A)B AND WRITE ON TAPE14                               
      IF( ICASX.EQ.2) REWIND 14
      DO 2  IB=1, NBBL
      IF( IB.EQ. NBBL) NPB= NLBL
      IF( ICASX.GT.1) READ( IBFL) (( BX( I, J), I=1, N1C), J=1, NPB)
      CALL SOLVES( A, IP, BX, N1C, NPB, NP, N1, MP, M1,13,13)
      IF( ICASX.EQ.2) REWIND 14
      IF( ICASX.GT.1) WRITE( 14) (( BX( I, J), I=1, N1C), J=1, NPB)
    2 CONTINUE
      IF( ICASX.EQ.1) GOTO 3
      REWIND 11
      REWIND 12
      REWIND 15
      REWIND IBFL
C     COMPUTE D-C(INV(A)B) AND WRITE ON TAPE11                          
    3 NPC= NPBL
      DO 8  IC=1, NBBL
      IF( IC.EQ. NBBL) NPC= NLBL
      IF( ICASX.EQ.1) GOTO 4
      READ( 15) (( C( I, J), I=1, N1C), J=1, NPC)
      READ( 12) (( D( I, J), I=1, N2C), J=1, NPC)
      REWIND 14
    4 NPB= NPBL
      NIC=0
      DO 7  IB=1, NBBL
      IF( IB.EQ. NBBL) NPB= NLBL
      IF( ICASX.GT.1) READ( 14) (( B( I, J), I=1, N1C), J=1, NPB)
      DO 6  I=1, NPB
      II= I+ NIC
      DO 6  J=1, NPC
      SUM=(0.,0.)
      DO 5  K=1, N1C
    5 SUM= SUM+ B( K, I)* C( K, J)
    6 D( II, J)= D( II, J)- SUM
    7 NIC= NIC+ NPBL
      IF( ICASX.GT.1) WRITE( 11) (( D( I, J), I=1, N2C), J=1, NPBL)
    8 CONTINUE
      IF( ICASX.EQ.1) GOTO 9
      REWIND 11
      REWIND 12
      REWIND 14
      REWIND 15
C     FACTOR D-C(INV(A)B)                                               
    9 N1CP= N1C+1
      IF( ICASX.GT.1) GOTO 10
      CALL FACTR( N2C, D, IP( N1CP), N2C)
      GOTO 13
   10 IF( ICASX.EQ.4) GOTO 12
      NPB= NPBL
      IC=0
      DO 11  IB=1, NBBL
      IF( IB.EQ. NBBL) NPB= NLBL
      II= IC+1
      IC= IC+ N2C* NPB
   11 READ( 11) ( B( I,1), I= II, IC)
      REWIND 11
      CALL FACTR( N2C, B, IP( N1CP), N2C)
      NIC= N2C* N2C
      WRITE( 11) ( B( I,1), I=1, NIC)
      REWIND 11
      GOTO 13
   12 NBLSYS= NBLSYM
      NPSYS= NPSYM
      NLSYS= NLSYM
      ICASS= ICASE
      NBLSYM= NBBL
      NPSYM= NPBL
      NLSYM= NLBL
      ICASE=3
      CALL FACIO( B, N2C,1, IX( N1CP),11,12,16,11)
      CALL LUNSCR( B, N2C,1, IP( N1CP), IX( N1CP),12,11,16)
      NBLSYM= NBLSYS
      NPSYM= NPSYS
      NLSYM= NLSYS
      ICASE= ICASS
   13 RETURN
      END
C ***
C     DOUBLE PRECISION 6/4/85
C
      SUBROUTINE FACIO( A, NROW, NOP, IP, IU1, IU2, IU3, IU4)
C ***
C                                                                       
C     FACIO CONTROLS I/O FOR OUT-OF-CORE FACTORIZATION                  
C                                                                       
      IMPLICIT REAL (A-H,O-Z)
      COMPLEX  A
      COMMON  /MATPAR/ ICASE, NBLOKS, NPBLK, NLAST, NBLSYM, NPSYM, 
     &NLSYM, IMAT, ICASX, NBBX, NPBX, NLBX, NBBL, NPBL, NLBL
      DIMENSION  A( NROW,1), IP( NROW)
      IT=2* NPSYM* NROW
      NBM= NBLSYM-1
      I1=1
      I2= IT
      I3= I2+1
      I4=2* IT
      TIME=0.
      REWIND IU1
      REWIND IU2
      DO 3  KK=1, NOP
      KA=( KK-1)* NROW+1
      IFILE3= IU1
      IFILE4= IU3
      DO 2  IXBLK1=1, NBM
      REWIND IU3
      REWIND IU4
      CALL BLCKIN( A, IFILE3, I1, I2,1,17)
      IXBP= IXBLK1+1
      DO 1  IXBLK2= IXBP, NBLSYM
      CALL BLCKIN( A, IFILE3, I3, I4,1,18)
      CALL SECNDS( T1)
      CALL LFACTR( A, NROW, IXBLK1, IXBLK2, IP( KA))
      CALL SECNDS( T2)
      TIME= TIME+ T2- T1
      IF( IXBLK2.EQ. IXBP) CALL BLCKOT( A, IU2, I1, I2,1,19)
      IF( IXBLK1.EQ. NBM.AND. IXBLK2.EQ. NBLSYM) IFILE4= IU2
      CALL BLCKOT( A, IFILE4, I3, I4,1,20)
    1 CONTINUE
      IFILE3= IU3
      IFILE4= IU4
      IF(( IXBLK1/2)*2.NE. IXBLK1) GOTO 2
      IFILE3= IU4
      IFILE4= IU3
    2 CONTINUE
    3 CONTINUE
      REWIND IU1
      REWIND IU2
      REWIND IU3
      REWIND IU4
      WRITE (2,4)  TIME
C                                                                       
      RETURN
    4 FORMAT(' CP TIME TAKEN FOR FACTORIZATION = ',1P,E12.5)
      END
C ***
C     DOUBLE PRECISION 6/4/85
C
      SUBROUTINE FACTR( N, A, IP, NDIM)
C ***
C                                                                       
C     SUBROUTINE TO FACTOR A MATRIX INTO A UNIT LOWER TRIANGULAR MATRIX 
C     AND AN UPPER TRIANGULAR MATRIX USING THE GAUSS-DOOLITTLE ALGORITHM
C     PRESENTED ON PAGES 411-416 OF A. RALSTON--A FIRST COURSE IN       
C     NUMERICAL ANALYSIS.  COMMENTS BELOW REFER TO COMMENTS IN RALSTONS 
C     TEXT.    (MATRIX TRANSPOSED.                                      
C                                                                       
      IMPLICIT REAL (A-H,O-Z)
      PARAMETER ( NM=600, N2M=800, N3M=1000)
      COMPLEX  A, D, ARJ
      DIMENSION  A( NDIM, NDIM), IP( NDIM)
      COMMON  /SCRATM/ D( N2M)
      INTEGER  R, RM1, RP1, PJ, PR
      IFLG=0
C                                                                       
C     STEP 1                                                            
C                                                                       
      DO 9  R=1, N
      DO 1  K=1, N
      D( K)= A( R, K)
C                                                                       
C     STEPS 2 AND 3                                                     
C                                                                       
    1 CONTINUE
      RM1= R-1
      IF( RM1.LT.1) GOTO 4
      DO 3  J=1, RM1
      PJ= IP( J)
      ARJ= D( PJ)
      A( R, J)= ARJ
      D( PJ)= D( J)
      JP1= J+1
      DO 2  I= JP1, N
      D( I)= D( I)- A( J, I)* ARJ
    2 CONTINUE
    3 CONTINUE
C                                                                       
C     STEP 4                                                            
C                                                                       
    4 CONTINUE
      DMAX= REAL( D( R)* CONJG( D( R)))
      IP( R)= R
      RP1= R+1
      IF( RP1.GT. N) GOTO 6
      DO 5  I= RP1, N
      ELMAG= REAL( D( I)* CONJG( D( I)))
      IF( ELMAG.LT. DMAX) GOTO 5
      DMAX= ELMAG
      IP( R)= I
    5 CONTINUE
    6 CONTINUE
      IF( DMAX.LT.1.D-10) IFLG=1
      PR= IP( R)
      A( R, R)= D( PR)
C                                                                       
C     STEP 5                                                            
C                                                                       
      D( PR)= D( R)
      IF( RP1.GT. N) GOTO 8
      ARJ=1./ A( R, R)
      DO 7  I= RP1, N
      A( R, I)= D( I)* ARJ
    7 CONTINUE
    8 CONTINUE
      IF( IFLG.EQ.0) GOTO 9
      WRITE (2,10)  R, DMAX
      IFLG=0
    9 CONTINUE
C                                                                       
      RETURN
   10 FORMAT(1H ,'PIVOT(',I3,')=',1P,E16.8)
      END
C ***
C     DOUBLE PRECISION 6/4/85
C
      SUBROUTINE FACTRS( NP, NROW, A, IP, IX, IU1, IU2, IU3, IU4)
C ***
C                                                                       
C     FACTRS, FOR SYMMETRIC STRUCTURE, TRANSFORMS SUBMATRICIES TO FORM  
C     MATRICIES OF THE SYMMETRIC MODES AND CALLS ROUTINE TO FACTOR      
C     MATRICIES.  IF NO SYMMETRY, THE ROUTINE IS CALLED TO FACTOR THE   
C     COMPLETE MATRIX.                                                  
C                                                                       
      IMPLICIT REAL (A-H,O-Z)
      COMPLEX  A
      COMMON  /MATPAR/ ICASE, NBLOKS, NPBLK, NLAST, NBLSYM, NPSYM, 
     &NLSYM, IMAT, ICASX, NBBX, NPBX, NLBX, NBBL, NPBL, NLBL
      DIMENSION  A(1), IP( NROW), IX( NROW)
      NOP= NROW/ NP
      IF( ICASE.GT.2) GOTO 2
      DO 1  KK=1, NOP
      KA=( KK-1)* NP+1
    1 CALL FACTR( NP, A( KA), IP( KA), NROW)
      RETURN
C                                                                       
C     FACTOR SUBMATRICIES, OR FACTOR COMPLETE MATRIX IF NO SYMMETRY     
C     EXISTS.                                                           
C                                                                       
    2 IF( ICASE.GT.3) GOTO 3
      CALL FACIO( A, NROW, NOP, IX, IU1, IU2, IU3, IU4)
      CALL LUNSCR( A, NROW, NOP, IP, IX, IU2, IU3, IU4)
C                                                                       
C     REWRITE THE MATRICES BY COLUMNS ON TAPE 13                        
C                                                                       
      RETURN
    3 I2=2* NPBLK* NROW
      REWIND IU2
      DO 5  K=1, NOP
      REWIND IU1
      ICOLS= NPBLK
      IR2= K* NP
      IR1= IR2- NP+1
      DO 5  L=1, NBLOKS
      IF( NBLOKS.EQ.1.AND. K.GT.1) GOTO 4
      CALL BLCKIN( A, IU1,1, I2,1,602)
      IF( L.EQ. NBLOKS) ICOLS= NLAST
    4 IRR1= IR1
      IRR2= IR2
      DO 5  ICOLDX=1, ICOLS
      WRITE( IU2) ( A( I), I= IRR1, IRR2)
      IRR1= IRR1+ NROW
      IRR2= IRR2+ NROW
    5 CONTINUE
      REWIND IU1
      REWIND IU2
      IF( ICASE.EQ.5) GOTO 8
      REWIND IU3
      IRR1= NP* NP
      DO 7  KK=1, NOP
      IR1=1- NP
      IR2=0
      DO 6  I=1, NP
      IR1= IR1+ NP
      IR2= IR2+ NP
    6 READ( IU2) ( A( J), J= IR1, IR2)
      KA=( KK-1)* NP+1
      CALL FACTR( NP, A, IP( KA), NP)
      WRITE( IU3) ( A( I), I=1, IRR1)
    7 CONTINUE
      REWIND IU2
      REWIND IU3
      RETURN
    8 I2=2* NPSYM* NP
      DO 10  KK=1, NOP
      J2= NPSYM
      DO 10  L=1, NBLSYM
      IF( L.EQ. NBLSYM) J2= NLSYM
      IR1=1- NP
      IR2=0
      DO 9  J=1, J2
      IR1= IR1+ NP
      IR2= IR2+ NP
    9 READ( IU2) ( A( I), I= IR1, IR2)
   10 CALL BLCKOT( A, IU1,1, I2,1,193)
      REWIND IU1
      CALL FACIO( A, NP, NOP, IX, IU1, IU2, IU3, IU4)
      CALL LUNSCR( A, NP, NOP, IP, IX, IU2, IU3, IU4)
      RETURN
      END
C ***
C     DOUBLE PRECISION 6/4/85
C
C      COMPLEX FUNCTION FBAR( P)
      FUNCTION FBAR( P)
C ***
C                                                                       
C     FBAR IS SOMMERFELD ATTENUATION FUNCTION FOR NUMERICAL DISTANCE P  
C                                                                       
C      IMPLICIT REAL (A-H,O-Z)
      COMPLEX  Z, ZS, SUM, POW, TERM, P, FJ, FBAR
      DIMENSION  FJX(2)
      EQUIVALENCE(FJ,FJX)
      DATA   TOSP/1.128379167D+0/, ACCS/1.D-12/, SP/1.772453851D+0/, 
     &FJX/0.,1./
      Z= FJ* SQRT( P)
C                                                                       
C     SERIES EXPANSION                                                  
C                                                                       
      IF( ABS( Z).GT.3.) GOTO 3
      ZS= Z* Z
      SUM= Z
      POW= Z
      DO 1  I=1,100
      POW=- POW* ZS/ DFLOAT( I)
      TERM= POW/(2.* I+1.)
      SUM= SUM+ TERM
      TMS= REAL( TERM* CONJG( TERM))
      SMS= REAL( SUM* CONJG( SUM))
      IF( TMS/ SMS.LT. ACCS) GOTO 2
    1 CONTINUE
    2 FBAR=1.-(1.- SUM* TOSP)* Z* EXP( ZS)* SP
C                                                                       
C     ASYMPTOTIC EXPANSION                                              
C                                                                       
      RETURN
    3 IF( REAL( Z).GE.0.) GOTO 4
      MINUS=1
      Z=- Z
      GOTO 5
    4 MINUS=0
    5 ZS=.5/( Z* Z)
      SUM=(0.,0.)
      TERM=(1.,0.)
      DO 6  I=1,6
      TERM=- TERM*(2.* I-1.)* ZS
    6 SUM= SUM+ TERM
      IF( MINUS.EQ.1) SUM= SUM-2.* SP* Z* EXP( Z* Z)
      FBAR=- SUM
      RETURN
      END
C ***
C     DOUBLE PRECISION 6/4/85
C
      SUBROUTINE FBLOCK( NROW, NCOL, IMAX, IRNGF, IPSYM)
C ***
C     FBLOCK SETS PARAMETERS FOR OUT-OF-CORE SOLUTION FOR THE PRIMARY   
C     MATRIX (A)                                                        
      IMPLICIT REAL (A-H,O-Z)
      PARAMETER ( NM=600, N2M=800, N3M=1000)
      COMPLEX  SSX, DETER
      COMMON  /MATPAR/ ICASE, NBLOKS, NPBLK, NLAST, NBLSYM, NPSYM, 
     &NLSYM, IMAT, ICASX, NBBX, NPBX, NLBX, NBBL, NPBL, NLBL
      COMMON  /SMAT/ SSX(16,16)
      IMX1= IMAX- IRNGF
      IF( NROW* NCOL.GT. IMX1) GOTO 2
      NBLOKS=1
      NPBLK= NROW
      NLAST= NROW
      IMAT= NROW* NCOL
      IF( NROW.NE. NCOL) GOTO 1
      ICASE=1
      RETURN
    1 ICASE=2
      GOTO 5
    2 IF( NROW.NE. NCOL) GOTO 3
      ICASE=3
      NPBLK= IMAX/(2* NCOL)
      NPSYM= IMX1/ NCOL
      IF( NPSYM.LT. NPBLK) NPBLK= NPSYM
      IF( NPBLK.LT.1) GOTO 12
      NBLOKS=( NROW-1)/ NPBLK
      NLAST= NROW- NBLOKS* NPBLK
      NBLOKS= NBLOKS+1
      NBLSYM= NBLOKS
      NPSYM= NPBLK
      NLSYM= NLAST
      IMAT= NPBLK* NCOL
      WRITE (2,14)  NBLOKS, NPBLK, NLAST
      GOTO 11
    3 NPBLK= IMAX/ NCOL
      IF( NPBLK.LT.1) GOTO 12
      IF( NPBLK.GT. NROW) NPBLK= NROW
      NBLOKS=( NROW-1)/ NPBLK
      NLAST= NROW- NBLOKS* NPBLK
      NBLOKS= NBLOKS+1
      WRITE (2,14)  NBLOKS, NPBLK, NLAST
      IF( NROW* NROW.GT. IMX1) GOTO 4
      ICASE=4
      NBLSYM=1
      NPSYM= NROW
      NLSYM= NROW
      IMAT= NROW* NROW
      WRITE (2,15) 
      GOTO 5
    4 ICASE=5
      NPSYM= IMAX/(2* NROW)
      NBLSYM= IMX1/ NROW
      IF( NBLSYM.LT. NPSYM) NPSYM= NBLSYM
      IF( NPSYM.LT.1) GOTO 12
      NBLSYM=( NROW-1)/ NPSYM
      NLSYM= NROW- NBLSYM* NPSYM
      NBLSYM= NBLSYM+1
      WRITE (2,16)  NBLSYM, NPSYM, NLSYM
      IMAT= NPSYM* NROW
    5 NOP= NCOL/ NROW
      IF( NOP* NROW.NE. NCOL) GOTO 13
C                                                                       
C     SET UP SSX MATRIX FOR ROTATIONAL SYMMETRY.                        
C                                                                       
      IF( IPSYM.GT.0) GOTO 7
      PHAZ=6.2831853072D+0/ NOP
      DO 6  I=2, NOP
      DO 6  J= I, NOP
      ARG= PHAZ* DFLOAT( I-1)* DFLOAT( J-1)
      SSX( I, J)= CMPLX( COS( ARG), SIN( ARG))
    6 SSX( J, I)= SSX( I, J)
C                                                                       
C     SET UP SSX MATRIX FOR PLANE SYMMETRY                              
C                                                                       
      GOTO 11
    7 KK=1
      SSX(1,1)=(1.,0.)
      IF(( NOP.EQ.2).OR.( NOP.EQ.4).OR.( NOP.EQ.8)) GOTO 8
      STOP
    8 KA= NOP/2
      IF( NOP.EQ.8) KA=3
      DO 10  K=1, KA
      DO 9  I=1, KK
      DO 9  J=1, KK
      DETER= SSX( I, J)
      SSX( I, J+ KK)= DETER
      SSX( I+ KK, J+ KK)=- DETER
    9 SSX( I+ KK, J)= DETER
   10 KK= KK*2
   11 RETURN
   12 WRITE (2,17)  NROW, NCOL
      STOP
   13 WRITE (2,18)  NROW, NCOL
C                                                                       
      STOP
   14 FORMAT(//' MATRIX FILE STORAGE -  NO. BLOCKS=',I5,' COLUMNS PE',
     &'R BLOCK=',I5,' COLUMNS IN LAST BLOCK=',I5)
   15 FORMAT(' SUBMATRICIES FIT IN CORE')
   16 FORMAT(' SUBMATRIX PARTITIONING -  NO. BLOCKS=',I5,' COLUMNS P',
     &'ER BLOCK=',I5,' COLUMNS IN LAST BLOCK=',I5)
   17 FORMAT(' ERROR - INSUFFICIENT STORAGE FOR MATRIX',2I5)
   18 FORMAT(' SYMMETRY ERROR - NROW,NCOL=',2I5)
      END
C ***
C     DOUBLE PRECISION 6/4/85
C
      SUBROUTINE FBNGF( NEQ, NEQ2, IRESRV, IB11, IC11, ID11, IX11)
C ***
C     FBNGF SETS THE BLOCKING PARAMETERS FOR THE B, C, AND D ARRAYS FOR 
C     OUT-OF-CORE STORAGE.                                              
      IMPLICIT REAL (A-H,O-Z)
      PARAMETER ( NM=600, N2M=800, N3M=1000)
      COMMON  /MATPAR/ ICASE, NBLOKS, NPBLK, NLAST, NBLSYM, NPSYM, 
     &NLSYM, IMAT, ICASX, NBBX, NPBX, NLBX, NBBL, NPBL, NLBL
      IRESX= IRESRV- IMAT
      NBLN= NEQ* NEQ2
      NDLN= NEQ2* NEQ2
      NBCD=2* NBLN+ NDLN
      IF( NBCD.GT. IRESX) GOTO 1
      ICASX=1
      IB11= IMAT+1
      GOTO 2
    1 IF( ICASE.LT.3) GOTO 3
      IF( NBCD.GT. IRESRV.OR. NBLN.GT. IRESX) GOTO 3
      ICASX=2
      IB11=1
    2 NBBX=1
      NPBX= NEQ
      NLBX= NEQ
      NBBL=1
      NPBL= NEQ2
      NLBL= NEQ2
      GOTO 5
    3 IR= IRESRV
      IF( ICASE.LT.3) IR= IRESX
      ICASX=3
      IF( NDLN.GT. IR) ICASX=4
      NBCD=2* NEQ+ NEQ2
      NPBL= IR/ NBCD
      NLBL= IR/(2* NEQ2)
      IF( NLBL.LT. NPBL) NPBL= NLBL
      IF( ICASE.LT.3) GOTO 4
      NLBL= IRESX/ NEQ
      IF( NLBL.LT. NPBL) NPBL= NLBL
    4 IF( NPBL.LT.1) GOTO 6
      NBBL=( NEQ2-1)/ NPBL
      NLBL= NEQ2- NBBL* NPBL
      NBBL= NBBL+1
      NBLN= NEQ* NPBL
      IR= IR- NBLN
      NPBX= IR/ NEQ2
      IF( NPBX.GT. NEQ) NPBX= NEQ
      NBBX=( NEQ-1)/ NPBX
      NLBX= NEQ- NBBX* NPBX
      NBBX= NBBX+1
      IB11=1
      IF( ICASE.LT.3) IB11= IMAT+1
    5 IC11= IB11+ NBLN
      ID11= IC11+ NBLN
      IX11= IMAT+1
      WRITE (2,11)  NEQ2
      IF( ICASX.EQ.1) RETURN
      WRITE (2,8)  ICASX
      WRITE (2,9)  NBBX, NPBX, NLBX
      WRITE (2,10)  NBBL, NPBL, NLBL
      RETURN
    6 WRITE (2,7)  IRESRV, IMAT, NEQ, NEQ2
C                                                                       
      STOP
    7 FORMAT(55H ERROR - INSUFFICIENT STORAGE FOR INTERACTION MATRICIES
     &,'  IRESRV,IMAT,NEQ,NEQ2 =',4I5)
    8 FORMAT(' FILE STORAGE FOR NEW MATRIX SECTIONS -  ICASX =', I2)
    9 FORMAT(' B FILLED BY ROWS -',15X,'NO. BLOCKS =',I3,3X,
     &  'ROWS PER BLOCK =', I3, '   ROWS IN LAST BLOCK =', I3)
   10 FORMAT(' B BY COLUMNS, C AND D BY ROWS -  NO. BLOCKS =',I3,
     & '    R/C PER BLOCK =', I3, '    R/C IN LAST BLOCK =', I3)
   11 FORMAT(//,' N.G.F. - NUMBER OF NEW UNKNOWNS IS', I4)
      END
C ***
C     DOUBLE PRECISION 6/4/85
C
      SUBROUTINE FFLD( THET, PHI, ETH, EPH)
C ***
C                                                                       
C     FFLD CALCULATES THE FAR ZONE RADIATED ELECTRIC FIELDS,            
C     THE FACTOR EXP(J*K*R)/(R/LAMDA) NOT INCLUDED                      
C                                                                       
      IMPLICIT REAL (A-H,O-Z)
      PARAMETER ( NM=600, N2M=800, N3M=1000)
      COMPLEX  CIX, CIY, CIZ, EXA, ETH, EPH, CONST, CCX, CCY, CCZ, 
     &CDP, CUR
      COMPLEX  ZRATI, ZRSIN, RRV, RRH, RRV1, RRH1, RRV2, RRH2, 
     &ZRATI2, TIX, TIY, TIZ, T1, ZSCRN, EX, EY, EZ, GX, GY, GZ, FRATI
      COMMON  /DATA/ LD, N1, N2, N, NP, M1, M2, M, MP, X( NM), Y( NM), 
     &Z( NM), SI( NM), BI( NM), ALP( NM), BET( NM), ICON1( N2M), ICON2(
     & N2M), ITAG( N2M), ICONX( NM), WLAM, IPSYM
      COMMON  /ANGL/ SALP( NM)
      COMMON  /CRNT/ AIR( NM), AII( NM), BIR( NM), BII( NM), CIR( NM), 
     &CII( NM), CUR( N3M)
      COMMON  /GND/ ZRATI, ZRATI2, FRATI, CL, CH, SCRWL, SCRWR, NRADL, 
     &KSYMP, IFAR, IPERF, T1, T2
      DIMENSION  CAB(1), SAB(1), CONSX(2)
      EQUIVALENCE(CAB,ALP),(SAB,BET),(CONST,CONSX)
      DATA   PI, TP, ETA/3.141592654D+0,6.283185308D+0,376.73/
      DATA   CONSX/0.,-29.97922085D+0/
      PHX=- SIN( PHI)
      PHY= COS( PHI)
      ROZ= COS( THET)
      ROZS= ROZ
      THX= ROZ* PHY
      THY=- ROZ* PHX
      THZ=- SIN( THET)
      ROX=- THZ* PHY
      ROY= THZ* PHX
C                                                                       
C     LOOP FOR STRUCTURE IMAGE IF ANY                                   
C                                                                       
      IF( N.EQ.0) GOTO 20
C                                                                       
C     CALCULATION OF REFLECTION COEFFECIENTS                            
C                                                                       
      DO 19  K=1, KSYMP
      IF( K.EQ.1) GOTO 4
C                                                                       
C     FOR PERFECT GROUND                                                
C                                                                       
      IF( IPERF.NE.1) GOTO 1
      RRV=-(1.,0.)
      RRH=-(1.,0.)
C                                                                       
C     FOR INFINITE PLANAR GROUND                                        
C                                                                       
      GOTO 2
    1 ZRSIN= SQRT(1.- ZRATI* ZRATI* THZ* THZ)
      RRV=-( ROZ- ZRATI* ZRSIN)/( ROZ+ ZRATI* ZRSIN)
      RRH=( ZRATI* ROZ- ZRSIN)/( ZRATI* ROZ+ ZRSIN)
C                                                                       
C     FOR THE CLIFF PROBLEM, TWO REFLCTION COEFFICIENTS CALCULATED      
C                                                                       
    2 IF( IFAR.LE.1) GOTO 3
      RRV1= RRV
      RRH1= RRH
      TTHET= TAN( THET)
      IF( IFAR.EQ.4) GOTO 3
      ZRSIN= SQRT(1.- ZRATI2* ZRATI2* THZ* THZ)
      RRV2=-( ROZ- ZRATI2* ZRSIN)/( ROZ+ ZRATI2* ZRSIN)
      RRH2=( ZRATI2* ROZ- ZRSIN)/( ZRATI2* ROZ+ ZRSIN)
      DARG=- TP*2.* CH* ROZ
    3 ROZ=- ROZ
      CCX= CIX
      CCY= CIY
      CCZ= CIZ
    4 CIX=(0.,0.)
      CIY=(0.,0.)
C                                                                       
C     LOOP OVER STRUCTURE SEGMENTS                                      
C                                                                       
      CIZ=(0.,0.)
      DO 17  I=1, N
      OMEGA=-( ROX* CAB( I)+ ROY* SAB( I)+ ROZ* SALP( I))
      EL= PI* SI( I)
      SILL= OMEGA* EL
      TOP= EL+ SILL
      BOT= EL- SILL
      IF( ABS( OMEGA).LT.1.D-7) GOTO 5
      A=2.* SIN( SILL)/ OMEGA
      GOTO 6
    5 A=(2.- OMEGA* OMEGA* EL* EL/3.)* EL
    6 IF( ABS( TOP).LT.1.D-7) GOTO 7
      TOO= SIN( TOP)/ TOP
      GOTO 8
    7 TOO=1.- TOP* TOP/6.
    8 IF( ABS( BOT).LT.1.D-7) GOTO 9
      BOO= SIN( BOT)/ BOT
      GOTO 10
    9 BOO=1.- BOT* BOT/6.
   10 B= EL*( BOO- TOO)
      C= EL*( BOO+ TOO)
      RR= A* AIR( I)+ B* BII( I)+ C* CIR( I)
      RI= A* AII( I)- B* BIR( I)+ C* CII( I)
      ARG= TP*( X( I)* ROX+ Y( I)* ROY+ Z( I)* ROZ)
      IF( K.EQ.2.AND. IFAR.GE.2) GOTO 11
C                                                                       
C     SUMMATION FOR FAR FIELD INTEGRAL                                  
C                                                                       
      EXA= CMPLX( COS( ARG), SIN( ARG))* CMPLX( RR, RI)
      CIX= CIX+ EXA* CAB( I)
      CIY= CIY+ EXA* SAB( I)
      CIZ= CIZ+ EXA* SALP( I)
C                                                                       
C     CALCULATION OF IMAGE CONTRIBUTION IN CLIFF AND GROUND SCREEN      
C     PROBLEMS.                                                         
C                                                                       
      GOTO 17
C                                                                       
C     SPECULAR POINT DISTANCE                                           
C                                                                       
   11 DR= Z( I)* TTHET
      D= DR* PHY+ X( I)
      IF( IFAR.EQ.2) GOTO 13
      D= SQRT( D* D+( Y( I)- DR* PHX)**2)
      IF( IFAR.EQ.3) GOTO 13
C                                                                       
C     RADIAL WIRE GROUND SCREEN REFLECTION COEFFICIENT                  
C                                                                       
      IF(( SCRWL- D).LT.0.) GOTO 12
      D= D+ T2
      ZSCRN= T1* D* LOG( D/ T2)
      ZSCRN=( ZSCRN* ZRATI)/( ETA* ZRATI+ ZSCRN)
      ZRSIN= SQRT(1.- ZSCRN* ZSCRN* THZ* THZ)
      RRV=( ROZ+ ZSCRN* ZRSIN)/(- ROZ+ ZSCRN* ZRSIN)
      RRH=( ZSCRN* ROZ+ ZRSIN)/( ZSCRN* ROZ- ZRSIN)
      GOTO 16
   12 IF( IFAR.EQ.4) GOTO 14
      IF( IFAR.EQ.5) D= DR* PHY+ X( I)
   13 IF(( CL- D).LE.0.) GOTO 15
   14 RRV= RRV1
      RRH= RRH1
      GOTO 16
   15 RRV= RRV2
      RRH= RRH2
      ARG= ARG+ DARG
C                                                                       
C     CONTRIBUTION OF EACH IMAGE SEGMENT MODIFIED BY REFLECTION COEF. , 
C     FOR CLIFF AND GROUND SCREEN PROBLEMS                              
C                                                                       
   16 EXA= CMPLX( COS( ARG), SIN( ARG))* CMPLX( RR, RI)
      TIX= EXA* CAB( I)
      TIY= EXA* SAB( I)
      TIZ= EXA* SALP( I)
      CDP=( TIX* PHX+ TIY* PHY)*( RRH- RRV)
      CIX= CIX+ TIX* RRV+ CDP* PHX
      CIY= CIY+ TIY* RRV+ CDP* PHY
      CIZ= CIZ- TIZ* RRV
   17 CONTINUE
      IF( K.EQ.1) GOTO 19
C                                                                       
C     CALCULATION OF CONTRIBUTION OF STRUCTURE IMAGE FOR INFINITE GROUND
C                                                                       
      IF( IFAR.GE.2) GOTO 18
      CDP=( CIX* PHX+ CIY* PHY)*( RRH- RRV)
      CIX= CCX+ CIX* RRV+ CDP* PHX
      CIY= CCY+ CIY* RRV+ CDP* PHY
      CIZ= CCZ- CIZ* RRV
      GOTO 19
   18 CIX= CIX+ CCX
      CIY= CIY+ CCY
      CIZ= CIZ+ CCZ
   19 CONTINUE
      IF( M.GT.0) GOTO 21
      ETH=( CIX* THX+ CIY* THY+ CIZ* THZ)* CONST
      EPH=( CIX* PHX+ CIY* PHY)* CONST
      RETURN
   20 CIX=(0.,0.)
      CIY=(0.,0.)
      CIZ=(0.,0.)
C                                                                       
C     ELECTRIC FIELD COMPONENTS                                         
C                                                                       
   21 ROZ= ROZS
      RFL=-1.
      DO 25  IP=1, KSYMP
      RFL=- RFL
      RRZ= ROZ* RFL
      CALL FFLDS( ROX, ROY, RRZ, CUR( N+1), GX, GY, GZ)
      IF( IP.EQ.2) GOTO 22
      EX= GX
      EY= GY
      EZ= GZ
      GOTO 25
   22 IF( IPERF.NE.1) GOTO 23
      GX=- GX
      GY=- GY
      GZ=- GZ
      GOTO 24
   23 RRV= SQRT(1.- ZRATI* ZRATI* THZ* THZ)
      RRH= ZRATI* ROZ
      RRH=( RRH- RRV)/( RRH+ RRV)
      RRV= ZRATI* RRV
      RRV=-( ROZ- RRV)/( ROZ+ RRV)
      ETH=( GX* PHX+ GY* PHY)*( RRH- RRV)
      GX= GX* RRV+ ETH* PHX
      GY= GY* RRV+ ETH* PHY
      GZ= GZ* RRV
   24 EX= EX+ GX
      EY= EY+ GY
      EZ= EZ- GZ
   25 CONTINUE
      EX= EX+ CIX* CONST
      EY= EY+ CIY* CONST
      EZ= EZ+ CIZ* CONST
      ETH= EX* THX+ EY* THY+ EZ* THZ
      EPH= EX* PHX+ EY* PHY
      RETURN
      END
C ***
C     DOUBLE PRECISION 6/4/85
C
      SUBROUTINE FFLDS( ROX, ROY, ROZ, SCUR, EX, EY, EZ)
C ***
C     CALCULATES THE XYZ COMPONENTS OF THE ELECTRIC FIELD DUE TO        
C     SURFACE CURRENTS                                                  
      IMPLICIT REAL (A-H,O-Z)
      PARAMETER ( NM=600, N2M=800, N3M=1000)
      COMPLEX  CT, CONS, SCUR, EX, EY, EZ
      COMMON  /DATA/ LD, N1, N2, N, NP, M1, M2, M, MP, X( NM), Y( NM), 
     &Z( NM), SI( NM), BI( NM), ALP( NM), BET( NM), ICON1( N2M), ICON2(
     & N2M), ITAG( N2M), ICONX( NM), WLAM, IPSYM
      DIMENSION  XS(1), YS(1), ZS(1), S(1), SCUR(1), CONSX(2)
      EQUIVALENCE(XS,X),(YS,Y),(ZS,Z),(S,BI),(CONS,CONSX)
      DATA   TPI/6.283185308D+0/, CONSX/0.,188.365/
      EX=(0.,0.)
      EY=(0.,0.)
      EZ=(0.,0.)
      I= LD+1
      DO 1  J=1, M
      I= I-1
      ARG= TPI*( ROX* XS( I)+ ROY* YS( I)+ ROZ* ZS( I))
      CT= CMPLX( COS( ARG)* S( I), SIN( ARG)* S( I))
      K=3* J
      EX= EX+ SCUR( K-2)* CT
      EY= EY+ SCUR( K-1)* CT
      EZ= EZ+ SCUR( K)* CT
    1 CONTINUE
      CT= ROX* EX+ ROY* EY+ ROZ* EZ
      EX= CONS*( CT* ROX- EX)
      EY= CONS*( CT* ROY- EY)
      EZ= CONS*( CT* ROZ- EZ)
      RETURN
      END
C ***
C     DOUBLE PRECISION 6/4/85
C
      SUBROUTINE GF( ZK, CO, SI)
C ***
C                                                                       
C     GF COMPUTES THE INTEGRAND EXP(JKR)/(KR) FOR NUMERICAL INTEGRATION.
C                                                                       
      IMPLICIT REAL (A-H,O-Z)
      PARAMETER ( NM=600, N2M=800, N3M=1000)
      COMMON  /TMI/ ZPK, RKB2, IJ
      ZDK= ZK- ZPK
      RK= SQRT( RKB2+ ZDK* ZDK)
      SI= SIN( RK)/ RK
      IF( IJ) 1,2,1
    1 CO= COS( RK)/ RK
      RETURN
    2 IF( RK.LT..2) GOTO 3
      CO=( COS( RK)-1.)/ RK
      RETURN
    3 RKS= RK* RK
      CO=((-1.38888889D-3* RKS+4.16666667D-2)* RKS-.5)* RK
      RETURN
      END
C ***
C     DOUBLE PRECISION 6/4/85
C
      SUBROUTINE GFIL( IPRT)
C ***
C                                                                       
C     GFIL READS THE N.G.F. FILE                                        
C                                                                       
      IMPLICIT REAL (A-H,O-Z)
      PARAMETER ( NM=600, N2M=800, N3M=1000)
      integer*4 COM
      COMPLEX  CM, SSX, ZRATI, ZRATI2, T1, ZARRAY, AR1, AR2, AR3, 
     &EPSCF, FRATI
      COMMON  /DATA/ LD, N1, N2, N, NP, M1, M2, M, MP, X( NM), Y( NM), 
     &Z( NM), SI( NM), BI( NM), ALP( NM), BET( NM), ICON1( N2M), ICON2(
     & N2M), ITAG( N2M), ICONX( NM), WLAM, IPSYM
      COMMON  /CMB/ CM(90000)
      COMMON  /ANGL/ SALP( NM)
      COMMON  /GND/ ZRATI, ZRATI2, FRATI, CL, CH, SCRWL, SCRWR, NRADL, 
     &KSYMP, IFAR, IPERF, T1, T2
      COMMON  /GGRID/ AR1(11,10,4), AR2(17,5,4), AR3(9,8,4), EPSCF, DXA
     &(3), DYA(3), XSA(3), YSA(3), NXA(3), NYA(3)
      COMMON  /MATPAR/ ICASE, NBLOKS, NPBLK, NLAST, NBLSYM, NPSYM, 
     &NLSYM, IMAT, ICASX, NBBX, NPBX, NLBX, NBBL, NPBL, NLBL
      COMMON  /SMAT/ SSX(16,16)
      COMMON  /ZLOAD/ ZARRAY( NM), NLOAD, NLODF
      COMMON  /SAVE/ IP( N2M), KCOM, COM(20,5), EPSR, SIG, SCRWLT, 
     &SCRWRT, FMHZ
      DATA   IGFL/20/
      REWIND IGFL
      READ( IGFL)  N1, NP, M1, MP, WLAM, FMHZ, IPSYM, KSYMP, IPERF, 
     &NRADL, EPSR, SIG, SCRWLT, SCRWRT, NLODF, KCOM
      N= N1
      M= M1
      N2= N1+1
      M2= M1+1
C     READ SEG. DATA AND CONVERT BACK TO END COORD. IN UNITS OF METERS  
      IF( N1.EQ.0) GOTO 2
      READ( IGFL) ( X( I), I=1, N1),( Y( I), I=1, N1),( Z( I), I=1, N1)
     &
      READ( IGFL) ( SI( I), I=1, N1),( BI( I), I=1, N1),( ALP( I), I=1,
     & N1)
      READ( IGFL) ( BET( I), I=1, N1),( SALP( I), I=1, N1)
      READ( IGFL) ( ICON1( I), I=1, N1),( ICON2( I), I=1, N1)
      READ( IGFL) ( ITAG( I), I=1, N1)
      IF( NLODF.NE.0) READ( IGFL) ( ZARRAY( I), I=1, N1)
      DO 1  I=1, N1
      XI= X( I)* WLAM
      YI= Y( I)* WLAM
      ZI= Z( I)* WLAM
      DX= SI( I)*.5* WLAM
      X( I)= XI- ALP( I)* DX
      Y( I)= YI- BET( I)* DX
      Z( I)= ZI- SALP( I)* DX
      SI( I)= XI+ ALP( I)* DX
      ALP( I)= YI+ BET( I)* DX
      BET( I)= ZI+ SALP( I)* DX
      BI( I)= BI( I)* WLAM
    1 CONTINUE
    2 IF( M1.EQ.0) GOTO 4
C     READ PATCH DATA AND CONVERT TO METERS                             
      J= LD- M1+1
      READ( IGFL) ( X( I), I= J, LD),( Y( I), I= J, LD),( Z( I), I= J, 
     &LD)
      READ( IGFL) ( SI( I), I= J, LD),( BI( I), I= J, LD),( ALP( I), I=
     & J, LD)
      READ( IGFL) ( BET( I), I= J, LD),( SALP( I), I= J, LD)
      READ( IGFL) ( ICON1( I), I= J, LD),( ICON2( I), I= J, LD)
      READ( IGFL) ( ITAG( I), I= J, LD)
      DX= WLAM* WLAM
      DO 3  I= J, LD
      X( I)= X( I)* WLAM
      Y( I)= Y( I)* WLAM
      Z( I)= Z( I)* WLAM
    3 BI( I)= BI( I)* DX
    4 READ( IGFL)  ICASE, NBLOKS, NPBLK, NLAST, NBLSYM, NPSYM, NLSYM, 
     &IMAT
      IF( IPERF.EQ.2) READ( IGFL)  AR1, AR2, AR3, EPSCF, DXA, DYA, XSA,
     & YSA, NXA, NYA
      NEQ= N1+2* M1
      NPEQ= NP+2* MP
      NOP= NEQ/ NPEQ
      IF( NOP.GT.1) READ( IGFL) (( SSX( I, J), I=1, NOP), J=1, NOP)
C     READ MATRIX A AND WRITE TAPE13 FOR OUT OF CORE                    
      READ( IGFL) ( IP( I), I=1, NEQ), COM
      IF( ICASE.GT.2) GOTO 5
      IOUT= NEQ* NPEQ
      READ( IGFL) ( CM( I), I=1, IOUT)
      GOTO 10
    5 REWIND 13
      IF( ICASE.NE.4) GOTO 7
      IOUT= NPEQ* NPEQ
      DO 6  K=1, NOP
      READ( IGFL) ( CM( J), J=1, IOUT)
    6 WRITE( 13) ( CM( J), J=1, IOUT)
      GOTO 9
    7 IOUT= NPSYM* NPEQ*2
      NBL2=2* NBLSYM
      DO 8  IOP=1, NOP
      DO 8  I=1, NBL2
      CALL BLCKIN( CM, IGFL,1, IOUT,1,206)
    8 CALL BLCKOT( CM,13,1, IOUT,1,205)
    9 REWIND 13
C     WRITE(6,N) G.F. HEADING                                           
   10 REWIND IGFL
      WRITE (2,16) 
      WRITE (2,14) 
      WRITE (2,14) 
      WRITE (2,17) 
      WRITE (2,18)  N1, M1
      IF( NOP.GT.1) WRITE (2,19)  NOP
      WRITE (2,20)  IMAT, ICASE
      IF( ICASE.LT.3) GOTO 11
      NBL2= NEQ* NPEQ
      WRITE (2,21)  NBL2
   11 WRITE (2,22)  FMHZ
      IF( KSYMP.EQ.2.AND. IPERF.EQ.1) WRITE (2,23) 
      IF( KSYMP.EQ.2.AND. IPERF.EQ.0) WRITE (2,27) 
      IF( KSYMP.EQ.2.AND. IPERF.EQ.2) WRITE (2,28) 
      IF( KSYMP.EQ.2.AND. IPERF.NE.1) WRITE (2,24)  EPSR, SIG
      WRITE (2,17) 
      DO 12  J=1, KCOM
   12 WRITE (2,15) ( COM( I, J), I=1,19)
      WRITE (2,17) 
      WRITE (2,14) 
      WRITE (2,14) 
      WRITE (2,16) 
      IF( IPRT.EQ.0) RETURN
      WRITE (2,25) 
      DO 13  I=1, N1
   13 WRITE (2,26)  I, X( I), Y( I), Z( I), SI( I), ALP( I), BET( I)
C                                                                       
      RETURN
   14 FORMAT(5X,'**************************************************',
     &'**********************************')
   15 FORMAT(5X,3H** ,19A4,3H **)
   16 FORMAT(////)
   17 FORMAT(5X,2H**,80X,2H**)
   18 FORMAT(5X,'** NUMERICAL GREEN S FUNCTION',53X,2H**,/,5X,'** NO',
     &'. SEGMENTS =',I4,10X,'NO. PATCHES =',I4,34X,2H**)
   19 FORMAT(5X,'** NO. SYMMETRIC SECTIONS =',I4,51X,2H**)
   20 FORMAT(5X,'** N.G.F. MATRIX -  CORE STORAGE =',I7,' COMPLEX NU',
     &'MBERS,  CASE',I2,16X,2H**)
   21 FORMAT(5X,2H**,19X,'MATRIX SIZE =',I7,' COMPLEX NUMBERS',25X,'**')
   22 FORMAT(5X,'** FREQUENCY =',1P,E12.5,' MHZ.',51X,2H**)
   23 FORMAT(5X,'** PERFECT GROUND',65X,2H**)
   24 FORMAT(5X,'** GROUND PARAMETERS - DIELECTRIC CONSTANT =',1P,E12.5,
     &26X,'**',/,5X,'**',21X,'CONDUCTIVITY =',E12.5,' MHOS/M.',25X,'**')
   25 FORMAT(39X,'NUMERICAL GREEN S FUNCTION DATA',/,41X,'COORDINATES',
     &' OF SEGMENT ENDS',/,51X,'(METERS)',/,5X,'SEG.',11X,
     &'- - - END ON''E - - -',26X,'- - - END TWO - - -',/,6X,3HNO.,6X,1
     &HX,14X,1HY,14X,1HZ,14X,1HX,14X,1HY,14X,1HZ)
   26 FORMAT(1X,I7,1P,6E15.6)
   27 FORMAT(5X,'** FINITE GROUND.  REFLECTION COEFFICIENT APPROXIMAT',
     &'ION',27X,2H**)
   28 FORMAT(5X,'** FINITE GROUND.  SOMMERFELD SOLUTION',44X,'**')
      END
C ***
C     DOUBLE PRECISION 6/4/85
C
      SUBROUTINE GFLD( RHO, PHI, RZ, ETH, EPI, ERD, UX, KSYMP)
C ***
C                                                                       
C     GFLD COMPUTES THE RADIATED FIELD INCLUDING GROUND WAVE.           
C                                                                       
      IMPLICIT REAL (A-H,O-Z)
      PARAMETER ( NM=600, N2M=800, N3M=1000)
      COMPLEX  CUR, EPI, CIX, CIY, CIZ, EXA, XX1, XX2, U, U2, ERV, 
     &EZV, ERH, EPH
      COMPLEX  EZH, EX, EY, ETH, UX, ERD
      COMMON  /DATA/ LD, N1, N2, N, NP, M1, M2, M, MP, X( NM), Y( NM), 
     &Z( NM), SI( NM), BI( NM), ALP( NM), BET( NM), ICON1( N2M), ICON2(
     & N2M), ITAG( N2M), ICONX( NM), WLAM, IPSYM
      COMMON  /ANGL/ SALP( NM)
      COMMON  /CRNT/ AIR( NM), AII( NM), BIR( NM), BII( NM), CIR( NM), 
     &CII( NM), CUR( N3M)
      COMMON  /GWAV/ U, U2, XX1, XX2, R1, R2, ZMH, ZPH
      DIMENSION  CAB(1), SAB(1)
      EQUIVALENCE(CAB(1),ALP(1)),(SAB(1),BET(1))
      DATA   PI, TP/3.141592654D+0,6.283185308D+0/
      R= SQRT( RHO* RHO+ RZ* RZ)
      IF( KSYMP.EQ.1) GOTO 1
      IF( ABS( UX).GT..5) GOTO 1
      IF( R.GT.1.E5) GOTO 1
C                                                                       
C     COMPUTATION OF SPACE WAVE ONLY                                    
C                                                                       
      GOTO 4
    1 IF( RZ.LT.1.D-20) GOTO 2
      THET= ATAN( RHO/ RZ)
      GOTO 3
    2 THET= PI*.5
    3 CALL FFLD( THET, PHI, ETH, EPI)
      ARG=- TP* R
      EXA= CMPLX( COS( ARG), SIN( ARG))/ R
      ETH= ETH* EXA
      EPI= EPI* EXA
      ERD=(0.,0.)
C                                                                       
C     COMPUTATION OF SPACE AND GROUND WAVES.                            
C                                                                       
      RETURN
    4 U= UX
      U2= U* U
      PHX=- SIN( PHI)
      PHY= COS( PHI)
      RX= RHO* PHY
      RY=- RHO* PHX
      CIX=(0.,0.)
      CIY=(0.,0.)
C                                                                       
C     SUMMATION OF FIELD FROM INDIVIDUAL SEGMENTS                       
C                                                                       
      CIZ=(0.,0.)
      DO 17  I=1, N
      DX= CAB( I)
      DY= SAB( I)
      DZ= SALP( I)
      RIX= RX- X( I)
      RIY= RY- Y( I)
      RHS= RIX* RIX+ RIY* RIY
      RHP= SQRT( RHS)
      IF( RHP.LT.1.D-6) GOTO 5
      RHX= RIX/ RHP
      RHY= RIY/ RHP
      GOTO 6
    5 RHX=1.
      RHY=0.
    6 CALP=1.- DZ* DZ
      IF( CALP.LT.1.D-6) GOTO 7
      CALP= SQRT( CALP)
      CBET= DX/ CALP
      SBET= DY/ CALP
      CPH= RHX* CBET+ RHY* SBET
      SPH= RHY* CBET- RHX* SBET
      GOTO 8
    7 CPH= RHX
      SPH= RHY
    8 EL= PI* SI( I)
C                                                                       
C     INTEGRATION OF (CURRENT)*(PHASE FACTOR) OVER SEGMENT AND IMAGE FOR
C     CONSTANT, SINE, AND COSINE CURRENT DISTRIBUTIONS                  
C                                                                       
      RFL=-1.
      DO 16  K=1,2
      RFL=- RFL
      RIZ= RZ- Z( I)* RFL
      RXYZ= SQRT( RIX* RIX+ RIY* RIY+ RIZ* RIZ)
      RNX= RIX/ RXYZ
      RNY= RIY/ RXYZ
      RNZ= RIZ/ RXYZ
      OMEGA=-( RNX* DX+ RNY* DY+ RNZ* DZ* RFL)
      SILL= OMEGA* EL
      TOP= EL+ SILL
      BOT= EL- SILL
      IF( ABS( OMEGA).LT.1.D-7) GOTO 9
      A=2.* SIN( SILL)/ OMEGA
      GOTO 10
    9 A=(2.- OMEGA* OMEGA* EL* EL/3.)* EL
   10 IF( ABS( TOP).LT.1.D-7) GOTO 11
      TOO= SIN( TOP)/ TOP
      GOTO 12
   11 TOO=1.- TOP* TOP/6.
   12 IF( ABS( BOT).LT.1.D-7) GOTO 13
      BOO= SIN( BOT)/ BOT
      GOTO 14
   13 BOO=1.- BOT* BOT/6.
   14 B= EL*( BOO- TOO)
      C= EL*( BOO+ TOO)
      RR= A* AIR( I)+ B* BII( I)+ C* CIR( I)
      RI= A* AII( I)- B* BIR( I)+ C* CII( I)
      ARG= TP*( X( I)* RNX+ Y( I)* RNY+ Z( I)* RNZ* RFL)
      EXA= CMPLX( COS( ARG), SIN( ARG))* CMPLX( RR, RI)/ TP
      IF( K.EQ.2) GOTO 15
      XX1= EXA
      R1= RXYZ
      ZMH= RIZ
      GOTO 16
   15 XX2= EXA
      R2= RXYZ
      ZPH= RIZ
C                                                                       
C     CALL SUBROUTINE TO COMPUTE THE FIELD OF SEGMENT INCLUDING GROUND  
C     WAVE.                                                             
C                                                                       
   16 CONTINUE
      CALL GWAVE( ERV, EZV, ERH, EZH, EPH)
      ERH= ERH* CPH* CALP+ ERV* DZ
      EPH= EPH* SPH* CALP
      EZH= EZH* CPH* CALP+ EZV* DZ
      EX= ERH* RHX- EPH* RHY
      EY= ERH* RHY+ EPH* RHX
      CIX= CIX+ EX
      CIY= CIY+ EY
   17 CIZ= CIZ+ EZH
      ARG=- TP* R
      EXA= CMPLX( COS( ARG), SIN( ARG))
      CIX= CIX* EXA
      CIY= CIY* EXA
      CIZ= CIZ* EXA
      RNX= RX/ R
      RNY= RY/ R
      RNZ= RZ/ R
      THX= RNZ* PHY
      THY=- RNZ* PHX
      THZ=- RHO/ R
      ETH= CIX* THX+ CIY* THY+ CIZ* THZ
      EPI= CIX* PHX+ CIY* PHY
      ERD= CIX* RNX+ CIY* RNY+ CIZ* RNZ
      RETURN
      END
C ***
C     DOUBLE PRECISION 6/4/85
C
      SUBROUTINE GFOUT
C ***
C                                                                       
C     WRITE N.G.F. FILE                                                 
C                                                                       
      IMPLICIT REAL (A-H,O-Z)
      PARAMETER ( NM=600, N2M=800, N3M=1000)
      integer*4 COM
      COMPLEX  CM, SSX, ZRATI, ZRATI2, T1, ZARRAY, AR1, AR2, AR3, 
     &EPSCF, FRATI
      COMMON  /DATA/ LD, N1, N2, N, NP, M1, M2, M, MP, X( NM), Y( NM), 
     &Z( NM), SI( NM), BI( NM), ALP( NM), BET( NM), ICON1( N2M), ICON2(
     & N2M), ITAG( N2M), ICONX( NM), WLAM, IPSYM
      COMMON  /CMB/ CM(90000)
      COMMON  /ANGL/ SALP( NM)
      COMMON  /GND/ ZRATI, ZRATI2, FRATI, CL, CH, SCRWL, SCRWR, NRADL, 
     &KSYMP, IFAR, IPERF, T1, T2
      COMMON  /GGRID/ AR1(11,10,4), AR2(17,5,4), AR3(9,8,4), EPSCF, DXA
     &(3), DYA(3), XSA(3), YSA(3), NXA(3), NYA(3)
      COMMON  /MATPAR/ ICASE, NBLOKS, NPBLK, NLAST, NBLSYM, NPSYM, 
     &NLSYM, IMAT, ICASX, NBBX, NPBX, NLBX, NBBL, NPBL, NLBL
      COMMON  /SMAT/ SSX(16,16)
      COMMON  /ZLOAD/ ZARRAY( NM), NLOAD, NLODF
      COMMON  /SAVE/ IP( N2M), KCOM, COM(20,5), EPSR, SIG, SCRWLT, 
     &SCRWRT, FMHZ
      DATA   IGFL/20/
      NEQ= N+2* M
      NPEQ= NP+2* MP
      NOP= NEQ/ NPEQ
      WRITE( IGFL)  N, NP, M, MP, WLAM, FMHZ, IPSYM, KSYMP, IPERF, 
     &NRADL, EPSR, SIG, SCRWLT, SCRWRT, NLOAD, KCOM
      IF( N.EQ.0) GOTO 1
      WRITE( IGFL) ( X( I), I=1, N),( Y( I), I=1, N),( Z( I), I=1, N)
      WRITE( IGFL) ( SI( I), I=1, N),( BI( I), I=1, N),( ALP( I), I=1, 
     &N)
      WRITE( IGFL) ( BET( I), I=1, N),( SALP( I), I=1, N)
      WRITE( IGFL) ( ICON1( I), I=1, N),( ICON2( I), I=1, N)
      WRITE( IGFL) ( ITAG( I), I=1, N)
      IF( NLOAD.GT.0) WRITE( IGFL) ( ZARRAY( I), I=1, N)
    1 IF( M.EQ.0) GOTO 2
      J= LD- M+1
      WRITE( IGFL) ( X( I), I= J, LD),( Y( I), I= J, LD),( Z( I), I= J,
     & LD)
      WRITE( IGFL) ( SI( I), I= J, LD),( BI( I), I= J, LD),( ALP( I), I
     &= J, LD)
      WRITE( IGFL) ( BET( I), I= J, LD),( SALP( I), I= J, LD)
      WRITE( IGFL) ( ICON1( I), I= J, LD),( ICON2( I), I= J, LD)
      WRITE( IGFL) ( ITAG( I), I= J, LD)
    2 WRITE( IGFL)  ICASE, NBLOKS, NPBLK, NLAST, NBLSYM, NPSYM, NLSYM, 
     &IMAT
      IF( IPERF.EQ.2) WRITE( IGFL)  AR1, AR2, AR3, EPSCF, DXA, DYA, XSA
     &, YSA, NXA, NYA
      IF( NOP.GT.1) WRITE( IGFL) (( SSX( I, J), I=1, NOP), J=1, NOP)
      WRITE( IGFL) ( IP( I), I=1, NEQ), COM
      IF( ICASE.GT.2) GOTO 3
      IOUT= NEQ* NPEQ
      WRITE( IGFL) ( CM( I), I=1, IOUT)
      GOTO 12
    3 IF( ICASE.NE.4) GOTO 5
      REWIND 13
      I= NPEQ* NPEQ
      DO 4  K=1, NOP
      READ( 13) ( CM( J), J=1, I)
    4 WRITE( IGFL) ( CM( J), J=1, I)
      REWIND 13
      GOTO 12
    5 REWIND 13
      REWIND 14
      IF( ICASE.EQ.5) GOTO 8
      IOUT= NPBLK* NEQ*2
      DO 6  I=1, NBLOKS
      CALL BLCKIN( CM,13,1, IOUT,1,201)
    6 CALL BLCKOT( CM, IGFL,1, IOUT,1,202)
      DO 7  I=1, NBLOKS
      CALL BLCKIN( CM,14,1, IOUT,1,203)
    7 CALL BLCKOT( CM, IGFL,1, IOUT,1,204)
      GOTO 12
    8 IOUT= NPSYM* NPEQ*2
      DO 11  IOP=1, NOP
      DO 9  I=1, NBLSYM
      CALL BLCKIN( CM,13,1, IOUT,1,205)
    9 CALL BLCKOT( CM, IGFL,1, IOUT,1,206)
      DO 10  I=1, NBLSYM
      CALL BLCKIN( CM,14,1, IOUT,1,207)
   10 CALL BLCKOT( CM, IGFL,1, IOUT,1,208)
   11 CONTINUE
      REWIND 13
      REWIND 14
   12 REWIND IGFL
      WRITE (2,13)  IGFL, IMAT
C                                                                       
      RETURN
   13 FORMAT(///,' ****NUMERICAL GREEN S FUNCTION FILE ON TAPE',I3,
     &'****',/,5X,'MATRIX STORAGE -',I7,' COMPLEX NUMBERS',///)
      END
C ***
C     DOUBLE PRECISION 6/4/85
C
      SUBROUTINE GH( ZK, HR, HI)
C ***
C     INTEGRAND FOR H FIELD OF A WIRE                                   
      IMPLICIT REAL (A-H,O-Z)
      PARAMETER ( NM=600, N2M=800, N3M=1000)
      COMMON  /TMH/ ZPK, RHKS
      RS= ZK- ZPK
      RS= RHKS+ RS* RS
      R= SQRT( RS)
      CKR= COS( R)
      SKR= SIN( R)
      RR2=1./ RS
      RR3= RR2/ R
      HR= SKR* RR2+ CKR* RR3
      HI= CKR* RR2- SKR* RR3
      RETURN
      END
C ***
C     DOUBLE PRECISION 6/4/85
C
      SUBROUTINE GWAVE( ERV, EZV, ERH, EZH, EPH)
C ***
C                                                                       
C     GWAVE COMPUTES THE ELECTRIC FIELD, INCLUDING GROUND WAVE, OF A    
C     CURRENT ELEMENT OVER A GROUND PLANE USING FORMULAS OF K.A. NORTON 
C     (PROC. IRE, SEPT., 1937, PP.1203,1236.)                           
C                                                                       
      IMPLICIT REAL (A-H,O-Z)
      PARAMETER ( NM=600, N2M=800, N3M=1000)
      COMPLEX  FJ, TPJ, U2, U, RK1, RK2, T1, T2, T3, T4, P1, RV, OMR
     &, W, F, Q1, RH, V, G, XR1, XR2, X1, X2, X3, X4, X5, X6, X7, EZV, 
     &ERV, EZH, ERH, EPH, XX1, XX2, ECON, FBAR
      COMMON  /GWAV/ U, U2, XX1, XX2, R1, R2, ZMH, ZPH
      DIMENSION  FJX(2), TPJX(2), ECONX(2)
      EQUIVALENCE(FJ,FJX),(TPJ,TPJX),(ECON,ECONX)
      DATA   PI/3.141592654D+0/, FJX/0.,1./, TPJX/0.,6.283185308D+0/
      DATA   ECONX/0.,-188.367/
      SPPP= ZMH/ R1
      SPPP2= SPPP* SPPP
      CPPP2=1.- SPPP2
      IF( CPPP2.LT.1.D-20) CPPP2=1.D-20
      CPPP= SQRT( CPPP2)
      SPP= ZPH/ R2
      SPP2= SPP* SPP
      CPP2=1.- SPP2
      IF( CPP2.LT.1.D-20) CPP2=1.D-20
      CPP= SQRT( CPP2)
      RK1=- TPJ* R1
      RK2=- TPJ* R2
      T1=1.- U2* CPP2
      T2= SQRT( T1)
      T3=(1.-1./ RK1)/ RK1
      T4=(1.-1./ RK2)/ RK2
      P1= RK2* U2* T1/(2.* CPP2)
      RV=( SPP- U* T2)/( SPP+ U* T2)
      OMR=1.- RV
      W=1./ OMR
      W=(4.,0.)* P1* W* W
      F= FBAR( W)
      Q1= RK2* T1/(2.* U2* CPP2)
      RH=( T2- U* SPP)/( T2+ U* SPP)
      V=1./(1.+ RH)
      V=(4.,0.)* Q1* V* V
      G= FBAR( V)
      XR1= XX1/ R1
      XR2= XX2/ R2
      X1= CPPP2* XR1
      X2= RV* CPP2* XR2
      X3= OMR* CPP2* F* XR2
      X4= U* T2* SPP*2.* XR2/ RK2
      X5= XR1* T3*(1.-3.* SPPP2)
      X6= XR2* T4*(1.-3.* SPP2)
      EZV=( X1+ X2+ X3- X4- X5- X6)* ECON
      X1= SPPP* CPPP* XR1
      X2= RV* SPP* CPP* XR2
      X3= CPP* OMR* U* T2* F* XR2
      X4= SPP* CPP* OMR* XR2/ RK2
      X5=3.* SPPP* CPPP* T3* XR1
      X6= CPP* U* T2* OMR* XR2/ RK2*.5
      X7=3.* SPP* CPP* T4* XR2
      ERV=-( X1+ X2- X3+ X4- X5+ X6- X7)* ECON
      EZH=-( X1- X2+ X3- X4- X5- X6+ X7)* ECON
      X1= SPPP2* XR1
      X2= RV* SPP2* XR2
      X4= U2* T1* OMR* F* XR2
      X5= T3*(1.-3.* CPPP2)* XR1
      X6= T4*(1.-3.* CPP2)*(1.- U2*(1.+ RV)- U2* OMR* F)* XR2
      X7= U2* CPP2* OMR*(1.-1./ RK2)*( F*( U2* T1- SPP2-1./ RK2)+1./ 
     &RK2)* XR2
      ERH=( X1- X2- X4- X5+ X6+ X7)* ECON
      X1= XR1
      X2= RH* XR2
      X3=( RH+1.)* G* XR2
      X4= T3* XR1
      X5= T4*(1.- U2*(1.+ RV)- U2* OMR* F)* XR2
      X6=.5* U2* OMR*( F*( U2* T1- SPP2-1./ RK2)+1./ RK2)* XR2/ RK2
      EPH=-( X1- X2+ X3- X4+ X5+ X6)* ECON
      RETURN
      END
C ***
C     DOUBLE PRECISION 6/4/85
C
      SUBROUTINE GX( ZZ, RH, XK, GZ, GZP)
C ***
C     SEGMENT END CONTRIBUTIONS FOR THIN WIRE APPROX.                   
      PARAMETER ( NM=600, N2M=800, N3M=1000)
      COMPLEX  GZ, GZP
      R2= ZZ* ZZ+ RH* RH
      R= SQRT( R2)
      RKZ= XK* R
      GZ= CMPLX( COS( RKZ),- SIN( RKZ))/ R
      GZP=- CMPLX(1.0, RKZ)* GZ/ R2
      RETURN
      END
C ***
C     DOUBLE PRECISION 6/4/85
C
      SUBROUTINE GXX( ZZ, RH, A, A2, XK, IRA, G1, G1P, G2, G2P, G3, GZP
     &)
C ***
C     SEGMENT END CONTRIBUTIONS FOR EXT. THIN WIRE APPROX.              
      PARAMETER ( NM=600, N2M=800, N3M=1000)
      COMPLEX  GZ, C1, C2, C3, G1, G1P, G2, G2P, G3, GZP
      R2= ZZ* ZZ+ RH* RH
      R= SQRT( R2)
      R4= R2* R2
      RK= XK* R
      RK2= RK* RK
      RH2= RH* RH
      T1=.25* A2* RH2/ R4
      T2=.5* A2/ R2
      C1= CMPLX(1.0, RK)
      C2=3.* C1- RK2
      C3= CMPLX(6.0, RK)* RK2-15.* C1
      GZ= CMPLX( COS( RK),- SIN( RK))/ R
      G2= GZ*(1.+ T1* C2)
      G1= G2- T2* C1* GZ
      GZ= GZ/ R2
      G2P= GZ*( T1* C3- C1)
      GZP= T2* C2* GZ
      G3= G2P+ GZP
      G1P= G3* ZZ
      IF( IRA.EQ.1) GOTO 2
      G3=( G3+ GZP)* RH
      GZP=- ZZ* C1* GZ
      IF( RH.GT.1.D-10) GOTO 1
      G2=0.
      G2P=0.
      RETURN
    1 G2= G2/ RH
      G2P= G2P* ZZ/ RH
      RETURN
    2 T2=.5* A
      G2=- T2* C1* GZ
      G2P= T2* GZ* C2/ R2
      G3= RH2* G2P- A* GZ* C1
      G2P= G2P* ZZ
      GZP=- ZZ* C1* GZ
      RETURN
      END
C ***
C     DOUBLE PRECISION 6/4/85
C
      SUBROUTINE HELIX( S, HL, A1, B1, A2, B2, RAD, NS, ITG)
C ***
C     SUBROUTINE HELIX GENERATES SEGMENT GEOMETRY DATA FOR A HELIX OF NS
C     SEGMENTS
      IMPLICIT REAL (A-H,O-Z)
      PARAMETER ( NM=600, N2M=800, N3M=1000)
      COMMON  /DATA/ LD, N1, N2, N, NP, M1, M2, M, MP, X( NM), Y( NM), 
     &Z( NM), SI( NM), BI( NM), ALP( NM), BET( NM), ICON1( N2M), ICON2(
     & N2M), ITAG( N2M), ICONX( NM), WLAM, IPSYM
      DIMENSION  X2(1), Y2(1), Z2(1)
      EQUIVALENCE(X2(1),SI(1)),(Y2(1),ALP(1)),(Z2(1),BET(1))
      DATA   PI/3.1415926D+0/
      IST= N+1
      N= N+ NS
      NP= N
      MP= M
      IPSYM=0
      IF( NS.LT.1) RETURN
      TURNS= ABS( HL/ S)
      ZINC= ABS( HL/ NS)
      Z( IST)=0.
      DO 25  I= IST, N
      BI( I)= RAD
      ITAG( I)= ITG
      IF( I.NE. IST) Z( I)= Z( I-1)+ ZINC
      Z2( I)= Z( I)+ ZINC
      IF( A2.NE. A1) GOTO 10
      IF( B1.EQ.0) B1= A1
      X( I)= A1* COS(2.* PI* Z( I)/ S)
      Y( I)= B1* SIN(2.* PI* Z( I)/ S)
      X2( I)= A1* COS(2.* PI* Z2( I)/ S)
      Y2( I)= B1* SIN(2.* PI* Z2( I)/ S)
      GOTO 20
   10 IF( B2.EQ.0) B2= A2
      X( I)=( A1+( A2- A1)* Z( I)/ ABS( HL))* COS(2.* PI* Z( I)/ S)
      Y( I)=( B1+( B2- B1)* Z( I)/ ABS( HL))* SIN(2.* PI* Z( I)/ S)
      X2( I)=( A1+( A2- A1)* Z2( I)/ ABS( HL))* COS(2.* PI* Z2( I)/ S)
      Y2( I)=( B1+( B2- B1)* Z2( I)/ ABS( HL))* SIN(2.* PI* Z2( I)/ S)
   20 IF( HL.GT.0) GOTO 25
      COPY= X( I)
      X( I)= Y( I)
      Y( I)= COPY
      COPY= X2( I)
      X2( I)= Y2( I)
      Y2( I)= COPY
   25 CONTINUE
      IF( A2.EQ. A1) GOTO 21
      SANGLE= ATAN( A2/( ABS( HL)+( ABS( HL)* A1)/( A2- A1)))
      WRITE (2,104)  SANGLE
  104 FORMAT(5X,'THE CONE ANGLE OF THE SPIRAL IS',F10.4)
      RETURN
   21 IF( A1.NE. B1) GOTO 30
      HDIA=2.* A1
      TURN= HDIA* PI
      PITCH= ATAN( S/( PI* HDIA))
      TURN= TURN/ COS( PITCH)
      PITCH=180.* PITCH/ PI
      GOTO 40
   30 IF( A1.LT. B1) GOTO 34
      HMAJ=2.* A1
      HMIN=2.* B1
      GOTO 35
   34 HMAJ=2.* B1
      HMIN=2.* A1
   35 HDIA= SQRT(( HMAJ**2+ HMIN**2)/2* HMAJ)
      TURN=2.* PI* HDIA
      PITCH=(180./ PI)* ATAN( S/( PI* HDIA))
   40 WRITE (2,105)  PITCH, TURN
  105 FORMAT(5X,'THE PITCH ANGLE IS',F10.4/5X,
     &'THE LENGTH OF WIRE/TURN ''IS',F10.4)
      RETURN
      END
C ***
C     DOUBLE PRECISION 6/4/85
C
      SUBROUTINE HFK( EL1, EL2, RHK, ZPKX, SGR, SGI)
C ***
C     HFK COMPUTES THE H FIELD OF A UNIFORM CURRENT FILAMENT BY         
C     NUMERICAL INTEGRATION                                             
      IMPLICIT REAL (A-H,O-Z)
      COMMON  /TMH/ ZPK, RHKS
      DATA   NX, NM, NTS, RX/1,65536,4,1.D-4/
      ZPK= ZPKX
      RHKS= RHK* RHK
      Z= EL1
      ZE= EL2
      S= ZE- Z
      EP= S/(10.* NM)
      ZEND= ZE- EP
      SGR=0.0
      SGI=0.0
      NS= NX
      NT=0
      CALL GH( Z, G1R, G1I)
    1 DZ= S/ NS
      ZP= Z+ DZ
      IF( ZP- ZE) 3,3,2
    2 DZ= ZE- Z
      IF( ABS( DZ)- EP) 17,17,3
    3 DZOT= DZ*.5
      ZP= Z+ DZOT
      CALL GH( ZP, G3R, G3I)
      ZP= Z+ DZ
      CALL GH( ZP, G5R, G5I)
    4 T00R=( G1R+ G5R)* DZOT
      T00I=( G1I+ G5I)* DZOT
      T01R=( T00R+ DZ* G3R)*0.5
      T01I=( T00I+ DZ* G3I)*0.5
      T10R=(4.0* T01R- T00R)/3.0
      T10I=(4.0* T01I- T00I)/3.0
      CALL TEST( T01R, T10R, TE1R, T01I, T10I, TE1I,0.)
      IF( TE1I- RX) 5,5,6
    5 IF( TE1R- RX) 8,8,6
    6 ZP= Z+ DZ*0.25
      CALL GH( ZP, G2R, G2I)
      ZP= Z+ DZ*0.75
      CALL GH( ZP, G4R, G4I)
      T02R=( T01R+ DZOT*( G2R+ G4R))*0.5
      T02I=( T01I+ DZOT*( G2I+ G4I))*0.5
      T11R=(4.0* T02R- T01R)/3.0
      T11I=(4.0* T02I- T01I)/3.0
      T20R=(16.0* T11R- T10R)/15.0
      T20I=(16.0* T11I- T10I)/15.0
      CALL TEST( T11R, T20R, TE2R, T11I, T20I, TE2I,0.)
      IF( TE2I- RX) 7,7,14
    7 IF( TE2R- RX) 9,9,14
    8 SGR= SGR+ T10R
      SGI= SGI+ T10I
      NT= NT+2
      GOTO 10
    9 SGR= SGR+ T20R
      SGI= SGI+ T20I
      NT= NT+1
   10 Z= Z+ DZ
      IF( Z- ZEND) 11,17,17
   11 G1R= G5R
      G1I= G5I
      IF( NT- NTS) 1,12,12
   12 IF( NS- NX) 1,1,13
   13 NS= NS/2
      NT=1
      GOTO 1
   14 NT=0
      IF( NS- NM) 16,15,15
   15 WRITE (2,18)  Z
      GOTO 9
   16 NS= NS*2
      DZ= S/ NS
      DZOT= DZ*0.5
      G5R= G3R
      G5I= G3I
      G3R= G2R
      G3I= G2I
      GOTO 4
   17 CONTINUE
      SGR= SGR* RHK*.5
      SGI= SGI* RHK*.5
C                                                                       
      RETURN
   18 FORMAT(' STEP SIZE LIMITED AT Z=',F10.5)
      END
C ***
C     DOUBLE PRECISION 6/4/85
C
      SUBROUTINE HINTG( XI, YI, ZI)
C ***
C     HINTG COMPUTES THE H FIELD OF A PATCH CURRENT                     
      IMPLICIT REAL (A-H,O-Z)
      PARAMETER ( NM=600, N2M=800, N3M=1000)
      COMPLEX  EXK, EYK, EZK, EXS, EYS, EZS, EXC, EYC, EZC, ZRATI, 
     &ZRATI2, GAM, F1X, F1Y, F1Z, F2X, F2Y, F2Z, RRV, RRH, T1, FRATI
      COMMON  /DATAJ/ S, B, XJ, YJ, ZJ, CABJ, SABJ, SALPJ, EXK, EYK, 
     &EZK, EXS, EYS, EZS, EXC, EYC, EZC, RKH, IEXK, IND1, INDD1, IND2, 
     &INDD2, IPGND
      COMMON  /GND/ ZRATI, ZRATI2, FRATI, CL, CH, SCRWL, SCRWR, NRADL, 
     &KSYMP, IFAR, IPERF, T1, T2
      EQUIVALENCE(T1XJ,CABJ),(T1YJ,SABJ),(T1ZJ,SALPJ),(T2XJ,B),(T2YJ,
     &IND1),(T2ZJ,IND2)
      DATA   FPI/12.56637062D+0/, TP/6.283185308D+0/
      RX= XI- XJ
      RY= YI- YJ
      RFL=-1.
      EXK=(0.,0.)
      EYK=(0.,0.)
      EZK=(0.,0.)
      EXS=(0.,0.)
      EYS=(0.,0.)
      EZS=(0.,0.)
      DO 5  IP=1, KSYMP
      RFL=- RFL
      RZ= ZI- ZJ* RFL
      RSQ= RX* RX+ RY* RY+ RZ* RZ
      IF( RSQ.LT.1.D-20) GOTO 5
      R= SQRT( RSQ)
      RK= TP* R
      CR= COS( RK)
      SR= SIN( RK)
      GAM=-( CMPLX( CR,- SR)+ RK* CMPLX( SR, CR))/( FPI* RSQ* R)* S
      EXC= GAM* RX
      EYC= GAM* RY
      EZC= GAM* RZ
      T1ZR= T1ZJ* RFL
      T2ZR= T2ZJ* RFL
      F1X= EYC* T1ZR- EZC* T1YJ
      F1Y= EZC* T1XJ- EXC* T1ZR
      F1Z= EXC* T1YJ- EYC* T1XJ
      F2X= EYC* T2ZR- EZC* T2YJ
      F2Y= EZC* T2XJ- EXC* T2ZR
      F2Z= EXC* T2YJ- EYC* T2XJ
      IF( IP.EQ.1) GOTO 4
      IF( IPERF.NE.1) GOTO 1
      F1X=- F1X
      F1Y=- F1Y
      F1Z=- F1Z
      F2X=- F2X
      F2Y=- F2Y
      F2Z=- F2Z
      GOTO 4
    1 XYMAG= SQRT( RX* RX+ RY* RY)
      IF( XYMAG.GT.1.D-6) GOTO 2
      PX=0.
      PY=0.
      CTH=1.
      RRV=(1.,0.)
      GOTO 3
    2 PX=- RY/ XYMAG
      PY= RX/ XYMAG
      CTH= RZ/ R
      RRV= SQRT(1.- ZRATI* ZRATI*(1.- CTH* CTH))
    3 RRH= ZRATI* CTH
      RRH=( RRH- RRV)/( RRH+ RRV)
      RRV= ZRATI* RRV
      RRV=-( CTH- RRV)/( CTH+ RRV)
      GAM=( F1X* PX+ F1Y* PY)*( RRV- RRH)
      F1X= F1X* RRH+ GAM* PX
      F1Y= F1Y* RRH+ GAM* PY
      F1Z= F1Z* RRH
      GAM=( F2X* PX+ F2Y* PY)*( RRV- RRH)
      F2X= F2X* RRH+ GAM* PX
      F2Y= F2Y* RRH+ GAM* PY
      F2Z= F2Z* RRH
    4 EXK= EXK+ F1X
      EYK= EYK+ F1Y
      EZK= EZK+ F1Z
      EXS= EXS+ F2X
      EYS= EYS+ F2Y
      EZS= EZS+ F2Z
    5 CONTINUE
      RETURN
      END
C ***
C     DOUBLE PRECISION 6/4/85
C
      SUBROUTINE HSFLD( XI, YI, ZI, AI)
C ***
C     HSFLD COMPUTES THE H FIELD FOR CONSTANT, SINE, AND COSINE CURRENT 
C     ON A SEGMENT INCLUDING GROUND EFFECTS.                            
      IMPLICIT REAL (A-H,O-Z)
      PARAMETER ( NM=600, N2M=800, N3M=1000)
      COMPLEX  EXK, EYK, EZK, EXS, EYS, EZS, EXC, EYC, EZC, ZRATI, 
     &ZRATI2, T1, HPK, HPS, HPC, QX, QY, QZ, RRV, RRH, ZRATX, FRATI
      COMMON  /DATAJ/ S, B, XJ, YJ, ZJ, CABJ, SABJ, SALPJ, EXK, EYK, 
     &EZK, EXS, EYS, EZS, EXC, EYC, EZC, RKH, IEXK, IND1, INDD1, IND2, 
     &INDD2, IPGND
      COMMON  /GND/ ZRATI, ZRATI2, FRATI, CL, CH, SCRWL, SCRWR, NRADL, 
     &KSYMP, IFAR, IPERF, T1, T2
      DATA   ETA/376.73/
      XIJ= XI- XJ
      YIJ= YI- YJ
      RFL=-1.
      DO 7  IP=1, KSYMP
      RFL=- RFL
      SALPR= SALPJ* RFL
      ZIJ= ZI- RFL* ZJ
      ZP= XIJ* CABJ+ YIJ* SABJ+ ZIJ* SALPR
      RHOX= XIJ- CABJ* ZP
      RHOY= YIJ- SABJ* ZP
      RHOZ= ZIJ- SALPR* ZP
      RH= SQRT( RHOX* RHOX+ RHOY* RHOY+ RHOZ* RHOZ+ AI* AI)
      IF( RH.GT.1.D-10) GOTO 1
      EXK=0.
      EYK=0.
      EZK=0.
      EXS=0.
      EYS=0.
      EZS=0.
      EXC=0.
      EYC=0.
      EZC=0.
      GOTO 7
    1 RHOX= RHOX/ RH
      RHOY= RHOY/ RH
      RHOZ= RHOZ/ RH
      PHX= SABJ* RHOZ- SALPR* RHOY
      PHY= SALPR* RHOX- CABJ* RHOZ
      PHZ= CABJ* RHOY- SABJ* RHOX
      CALL HSFLX( S, RH, ZP, HPK, HPS, HPC)
      IF( IP.NE.2) GOTO 6
      IF( IPERF.EQ.1) GOTO 5
      ZRATX= ZRATI
      RMAG= SQRT( ZP* ZP+ RH* RH)
C                                                                       
C     SET PARAMETERS FOR RADIAL WIRE GROUND SCREEN.                     
C                                                                       
      XYMAG= SQRT( XIJ* XIJ+ YIJ* YIJ)
      IF( NRADL.EQ.0) GOTO 2
      XSPEC=( XI* ZJ+ ZI* XJ)/( ZI+ ZJ)
      YSPEC=( YI* ZJ+ ZI* YJ)/( ZI+ ZJ)
      RHOSPC= SQRT( XSPEC* XSPEC+ YSPEC* YSPEC+ T2* T2)
      IF( RHOSPC.GT. SCRWL) GOTO 2
      RRV= T1* RHOSPC* LOG( RHOSPC/ T2)
      ZRATX=( RRV* ZRATI)/( ETA* ZRATI+ RRV)
C                                                                       
C     CALCULATION OF REFLECTION COEFFICIENTS WHEN GROUND IS SPECIFIED.  
C                                                                       
    2 IF( XYMAG.GT.1.D-6) GOTO 3
      PX=0.
      PY=0.
      CTH=1.
      RRV=(1.,0.)
      GOTO 4
    3 PX=- YIJ/ XYMAG
      PY= XIJ/ XYMAG
      CTH= ZIJ/ RMAG
      RRV= SQRT(1.- ZRATX* ZRATX*(1.- CTH* CTH))
    4 RRH= ZRATX* CTH
      RRH=-( RRH- RRV)/( RRH+ RRV)
      RRV= ZRATX* RRV
      RRV=( CTH- RRV)/( CTH+ RRV)
      QY=( PHX* PX+ PHY* PY)*( RRV- RRH)
      QX= QY* PX+ PHX* RRH
      QY= QY* PY+ PHY* RRH
      QZ= PHZ* RRH
      EXK= EXK- HPK* QX
      EYK= EYK- HPK* QY
      EZK= EZK- HPK* QZ
      EXS= EXS- HPS* QX
      EYS= EYS- HPS* QY
      EZS= EZS- HPS* QZ
      EXC= EXC- HPC* QX
      EYC= EYC- HPC* QY
      EZC= EZC- HPC* QZ
      GOTO 7
    5 EXK= EXK- HPK* PHX
      EYK= EYK- HPK* PHY
      EZK= EZK- HPK* PHZ
      EXS= EXS- HPS* PHX
      EYS= EYS- HPS* PHY
      EZS= EZS- HPS* PHZ
      EXC= EXC- HPC* PHX
      EYC= EYC- HPC* PHY
      EZC= EZC- HPC* PHZ
      GOTO 7
    6 EXK= HPK* PHX
      EYK= HPK* PHY
      EZK= HPK* PHZ
      EXS= HPS* PHX
      EYS= HPS* PHY
      EZS= HPS* PHZ
      EXC= HPC* PHX
      EYC= HPC* PHY
      EZC= HPC* PHZ
    7 CONTINUE
      RETURN
      END
C ***
C     DOUBLE PRECISION 6/4/85
C
      SUBROUTINE HSFLX( S, RH, ZPX, HPK, HPS, HPC)
C ***
C     CALCULATES H FIELD OF SINE COSINE, AND CONSTANT CURRENT OF SEGMENT
      IMPLICIT REAL (A-H,O-Z)
      PARAMETER ( NM=600, N2M=800, N3M=1000)
      COMPLEX  FJ, FJK, EKR1, EKR2, T1, T2, CONS, HPS, HPC, HPK
      DIMENSION  FJX(2), FJKX(2)
      EQUIVALENCE(FJ,FJX),(FJK,FJKX)
      DATA   TP/6.283185308D+0/, FJX/0.,1./, FJKX/0.,-6.283185308D+0/
      DATA   PI8/25.13274123D+0/
      IF( RH.LT.1.D-10) GOTO 6
      IF( ZPX.LT.0.) GOTO 1
      ZP= ZPX
      HSS=1.
      GOTO 2
    1 ZP=- ZPX
      HSS=-1.
    2 DH=.5* S
      Z1= ZP+ DH
      Z2= ZP- DH
      IF( Z2.LT.1.D-7) GOTO 3
      RHZ= RH/ Z2
      GOTO 4
    3 RHZ=1.
    4 DK= TP* DH
      CDK= COS( DK)
      SDK= SIN( DK)
      CALL HFK(- DK, DK, RH* TP, ZP* TP, HKR, HKI)
      HPK= CMPLX( HKR, HKI)
      IF( RHZ.LT.1.D-3) GOTO 5
      RH2= RH* RH
      R1= SQRT( RH2+ Z1* Z1)
      R2= SQRT( RH2+ Z2* Z2)
      EKR1= EXP( FJK* R1)
      EKR2= EXP( FJK* R2)
      T1= Z1* EKR1/ R1
      T2= Z2* EKR2/ R2
      HPS=( CDK*( EKR2- EKR1)- FJ* SDK*( T2+ T1))* HSS
      HPC=- SDK*( EKR2+ EKR1)- FJ* CDK*( T2- T1)
      CONS=- FJ/(2.* TP* RH)
      HPS= CONS* HPS
      HPC= CONS* HPC
      RETURN
    5 EKR1= CMPLX( CDK, SDK)/( Z2* Z2)
      EKR2= CMPLX( CDK,- SDK)/( Z1* Z1)
      T1= TP*(1./ Z1-1./ Z2)
      T2= EXP( FJK* ZP)* RH/ PI8
      HPS= T2*( T1+( EKR1+ EKR2)* SDK)* HSS
      HPC= T2*(- FJ* T1+( EKR1- EKR2)* CDK)
      RETURN
    6 HPS=(0.,0.)
      HPC=(0.,0.)
      HPK=(0.,0.)
      RETURN
      END
C ***
C     DOUBLE PRECISION 6/4/85
C
      SUBROUTINE INTRP( X, Y, F1, F2, F3, F4)
C ***
C                                                                       
C     INTRP USES BIVARIATE CUBIC INTERPOLATION TO OBTAIN THE VALUES OF  
C     4 FUNCTIONS AT THE POINT (X,Y).                                   
C                                                                       
      IMPLICIT REAL (A-H,O-Z)
      PARAMETER ( NM=600, N2M=800, N3M=1000)
      COMPLEX  F1, F2, F3, F4, A, B, C, D, FX1, FX2, FX3, FX4, P1, 
     &P2, P3, P4, A11, A12, A13, A14, A21, A22, A23, A24, A31, A32, A33
     &, A34, A41, A42, A43, A44, B11, B12, B13, B14, B21, B22, B23, B24
     &, B31, B32, B33, B34, B41, B42, B43, B44, C11, C12, C13, C14, C21
     &, C22, C23, C24, C31, C32, C33, C34, C41, C42, C43, C44, D11, D12
     &, D13, D14, D21, D22, D23, D24, D31, D32, D33, D34, D41, D42, D43
     &, D44
      COMPLEX  AR1, AR2, AR3, ARL1, ARL2, ARL3, EPSCF
      COMMON  /GGRID/ AR1(11,10,4), AR2(17,5,4), AR3(9,8,4), EPSCF, DXA
     &(3), DYA(3), XSA(3), YSA(3), NXA(3), NYA(3)
      DIMENSION  NDA(3), NDPA(3)
      DIMENSION  A(4,4), B(4,4), C(4,4), D(4,4), ARL1(1), ARL2(1), ARL3
     &(1)
      EQUIVALENCE(A(1,1),A11),(A(1,2),A12),(A(1,3),A13),(A(1,4),A14)
      EQUIVALENCE(A(2,1),A21),(A(2,2),A22),(A(2,3),A23),(A(2,4),A24)
      EQUIVALENCE(A(3,1),A31),(A(3,2),A32),(A(3,3),A33),(A(3,4),A34)
      EQUIVALENCE(A(4,1),A41),(A(4,2),A42),(A(4,3),A43),(A(4,4),A44)
      EQUIVALENCE(B(1,1),B11),(B(1,2),B12),(B(1,3),B13),(B(1,4),B14)
      EQUIVALENCE(B(2,1),B21),(B(2,2),B22),(B(2,3),B23),(B(2,4),B24)
      EQUIVALENCE(B(3,1),B31),(B(3,2),B32),(B(3,3),B33),(B(3,4),B34)
      EQUIVALENCE(B(4,1),B41),(B(4,2),B42),(B(4,3),B43),(B(4,4),B44)
      EQUIVALENCE(C(1,1),C11),(C(1,2),C12),(C(1,3),C13),(C(1,4),C14)
      EQUIVALENCE(C(2,1),C21),(C(2,2),C22),(C(2,3),C23),(C(2,4),C24)
      EQUIVALENCE(C(3,1),C31),(C(3,2),C32),(C(3,3),C33),(C(3,4),C34)
      EQUIVALENCE(C(4,1),C41),(C(4,2),C42),(C(4,3),C43),(C(4,4),C44)
      EQUIVALENCE(D(1,1),D11),(D(1,2),D12),(D(1,3),D13),(D(1,4),D14)
      EQUIVALENCE(D(2,1),D21),(D(2,2),D22),(D(2,3),D23),(D(2,4),D24)
      EQUIVALENCE(D(3,1),D31),(D(3,2),D32),(D(3,3),D33),(D(3,4),D34)
      EQUIVALENCE(D(4,1),D41),(D(4,2),D42),(D(4,3),D43),(D(4,4),D44)
      EQUIVALENCE(ARL1,AR1),(ARL2,AR2),(ARL3,AR3),(XS2,XSA(2)),(YS3,YSA
     &(3))
      DATA   IXS, IYS, IGRS/-10,-10,-10/, DX, DY, XS, YS/1.,1.,0.,0./
      DATA   NDA/11,17,9/, NDPA/110,85,72/, IXEG, IYEG/0,0/
      IF( X.LT. XS.OR. Y.LT. YS) GOTO 1
      IX= INT(( X- XS)/ DX)+1
C                                                                       
C     IF POINT LIES IN SAME 4 BY 4 POINT REGION AS PREVIOUS POINT, OLD  
C     VALUES ARE REUSED                                                 
C                                                                       
      IY= INT(( Y- YS)/ DY)+1
      IF( IX.LT. IXEG.OR. IY.LT. IYEG) GOTO 1
C                                                                       
C     DETERMINE CORRECT GRID AND GRID REGION                            
C                                                                       
      IF( IABS( IX- IXS).LT.2.AND. IABS( IY- IYS).LT.2) GOTO 12
    1 IF( X.GT. XS2) GOTO 2
      IGR=1
      GOTO 3
    2 IGR=2
      IF( Y.GT. YS3) IGR=3
    3 IF( IGR.EQ. IGRS) GOTO 4
      IGRS= IGR
      DX= DXA( IGRS)
      DY= DYA( IGRS)
      XS= XSA( IGRS)
      YS= YSA( IGRS)
      NXM2= NXA( IGRS)-2
      NYM2= NYA( IGRS)-2
      NXMS=(( NXM2+1)/3)*3+1
      NYMS=(( NYM2+1)/3)*3+1
      ND= NDA( IGRS)
      NDP= NDPA( IGRS)
      IX= INT(( X- XS)/ DX)+1
      IY= INT(( Y- YS)/ DY)+1
    4 IXS=(( IX-1)/3)*3+2
      IF( IXS.LT.2) IXS=2
      IXEG=-10000
      IF( IXS.LE. NXM2) GOTO 5
      IXS= NXM2
      IXEG= NXMS
    5 IYS=(( IY-1)/3)*3+2
      IF( IYS.LT.2) IYS=2
      IYEG=-10000
      IF( IYS.LE. NYM2) GOTO 6
      IYS= NYM2
C                                                                       
C     COMPUTE COEFFICIENTS OF 4 CUBIC POLYNOMIALS IN X FOR THE 4 GRID   
C     VALUES OF Y FOR EACH OF THE 4 FUNCTIONS                           
C                                                                       
      IYEG= NYMS
    6 IADZ= IXS+( IYS-3)* ND- NDP
      DO 11  K=1,4
      IADZ= IADZ+ NDP
      IADD= IADZ
      DO 11  I=1,4
      IADD= IADD+ ND
C     P1=AR1(IXS-1,IYS-2+I,K)                                           
      GOTO (7,8,9), IGRS
    7 P1= ARL1( IADD-1)
      P2= ARL1( IADD)
      P3= ARL1( IADD+1)
      P4= ARL1( IADD+2)
      GOTO 10
    8 P1= ARL2( IADD-1)
      P2= ARL2( IADD)
      P3= ARL2( IADD+1)
      P4= ARL2( IADD+2)
      GOTO 10
    9 P1= ARL3( IADD-1)
      P2= ARL3( IADD)
      P3= ARL3( IADD+1)
      P4= ARL3( IADD+2)
   10 A( I, K)=( P4- P1+3.*( P2- P3))*.1666666667D+0
      B( I, K)=( P1-2.* P2+ P3)*.5
      C( I, K)= P3-(2.* P1+3.* P2+ P4)*.1666666667D+0
   11 D( I, K)= P2
      XZ=( IXS-1)* DX+ XS
C                                                                       
C     EVALUATE POLYMOMIALS IN X AND THEN USE CUBIC INTERPOLATION IN Y   
C     FOR EACH OF THE 4 FUNCTIONS.                                      
C                                                                       
      YZ=( IYS-1)* DY+ YS
   12 XX=( X- XZ)/ DX
      YY=( Y- YZ)/ DY
      FX1=(( A11* XX+ B11)* XX+ C11)* XX+ D11
      FX2=(( A21* XX+ B21)* XX+ C21)* XX+ D21
      FX3=(( A31* XX+ B31)* XX+ C31)* XX+ D31
      FX4=(( A41* XX+ B41)* XX+ C41)* XX+ D41
      P1= FX4- FX1+3.*( FX2- FX3)
      P2=3.*( FX1-2.* FX2+ FX3)
      P3=6.* FX3-2.* FX1-3.* FX2- FX4
      F1=(( P1* YY+ P2)* YY+ P3)* YY*.1666666667D+0+ FX2
      FX1=(( A12* XX+ B12)* XX+ C12)* XX+ D12
      FX2=(( A22* XX+ B22)* XX+ C22)* XX+ D22
      FX3=(( A32* XX+ B32)* XX+ C32)* XX+ D32
      FX4=(( A42* XX+ B42)* XX+ C42)* XX+ D42
      P1= FX4- FX1+3.*( FX2- FX3)
      P2=3.*( FX1-2.* FX2+ FX3)
      P3=6.* FX3-2.* FX1-3.* FX2- FX4
      F2=(( P1* YY+ P2)* YY+ P3)* YY*.1666666667D+0+ FX2
      FX1=(( A13* XX+ B13)* XX+ C13)* XX+ D13
      FX2=(( A23* XX+ B23)* XX+ C23)* XX+ D23
      FX3=(( A33* XX+ B33)* XX+ C33)* XX+ D33
      FX4=(( A43* XX+ B43)* XX+ C43)* XX+ D43
      P1= FX4- FX1+3.*( FX2- FX3)
      P2=3.*( FX1-2.* FX2+ FX3)
      P3=6.* FX3-2.* FX1-3.* FX2- FX4
      F3=(( P1* YY+ P2)* YY+ P3)* YY*.1666666667D+0+ FX2
      FX1=(( A14* XX+ B14)* XX+ C14)* XX+ D14
      FX2=(( A24* XX+ B24)* XX+ C24)* XX+ D24
      FX3=(( A34* XX+ B34)* XX+ C34)* XX+ D34
      FX4=(( A44* XX+ B44)* XX+ C44)* XX+ D44
      P1= FX4- FX1+3.*( FX2- FX3)
      P2=3.*( FX1-2.* FX2+ FX3)
      P3=6.* FX3-2.* FX1-3.* FX2- FX4
      F4=(( P1* YY+ P2)* YY+ P3)* YY*.1666666667D+0+ FX2
      RETURN
      END
C ***
C     DOUBLE PRECISION 6/4/85
C
      SUBROUTINE INTX( EL1, EL2, B, IJ, SGR, SGI)
C ***
C                                                                       
C     INTX PERFORMS NUMERICAL INTEGRATION OF EXP(JKR)/R BY THE METHOD OF
C     VARIABLE INTERVAL WIDTH ROMBERG INTEGRATION.  THE INTEGRAND VALUE 
C     IS SUPPLIED BY SUBROUTINE GF.                                     
C                                                                       
      IMPLICIT REAL (A-H,O-Z)
      DATA   NX, NM, NTS, RX/1,65536,4,1.D-4/
      Z= EL1
      ZE= EL2
      IF( IJ.EQ.0) ZE=0.
      S= ZE- Z
      FNM= NM
      EP= S/(10.* FNM)
      ZEND= ZE- EP
      SGR=0.
      SGI=0.
      NS= NX
      NT=0
      CALL GF( Z, G1R, G1I)
    1 FNS= NS
      DZ= S/ FNS
      ZP= Z+ DZ
      IF( ZP- ZE) 3,3,2
    2 DZ= ZE- Z
      IF( ABS( DZ)- EP) 17,17,3
    3 DZOT= DZ*.5
      ZP= Z+ DZOT
      CALL GF( ZP, G3R, G3I)
      ZP= Z+ DZ
      CALL GF( ZP, G5R, G5I)
    4 T00R=( G1R+ G5R)* DZOT
      T00I=( G1I+ G5I)* DZOT
      T01R=( T00R+ DZ* G3R)*0.5
      T01I=( T00I+ DZ* G3I)*0.5
      T10R=(4.0* T01R- T00R)/3.0
C                                                                       
C     TEST CONVERGENCE OF 3 POINT ROMBERG RESULT.                       
C                                                                       
      T10I=(4.0* T01I- T00I)/3.0
      CALL TEST( T01R, T10R, TE1R, T01I, T10I, TE1I,0.)
      IF( TE1I- RX) 5,5,6
    5 IF( TE1R- RX) 8,8,6
    6 ZP= Z+ DZ*0.25
      CALL GF( ZP, G2R, G2I)
      ZP= Z+ DZ*0.75
      CALL GF( ZP, G4R, G4I)
      T02R=( T01R+ DZOT*( G2R+ G4R))*0.5
      T02I=( T01I+ DZOT*( G2I+ G4I))*0.5
      T11R=(4.0* T02R- T01R)/3.0
      T11I=(4.0* T02I- T01I)/3.0
      T20R=(16.0* T11R- T10R)/15.0
C                                                                       
C     TEST CONVERGENCE OF 5 POINT ROMBERG RESULT.                       
C                                                                       
      T20I=(16.0* T11I- T10I)/15.0
      CALL TEST( T11R, T20R, TE2R, T11I, T20I, TE2I,0.)
      IF( TE2I- RX) 7,7,14
    7 IF( TE2R- RX) 9,9,14
    8 SGR= SGR+ T10R
      SGI= SGI+ T10I
      NT= NT+2
      GOTO 10
    9 SGR= SGR+ T20R
      SGI= SGI+ T20I
      NT= NT+1
   10 Z= Z+ DZ
      IF( Z- ZEND) 11,17,17
   11 G1R= G5R
      G1I= G5I
      IF( NT- NTS) 1,12,12
C                                                                       
C     DOUBLE STEP SIZE                                                  
C                                                                       
   12 IF( NS- NX) 1,1,13
   13 NS= NS/2
      NT=1
      GOTO 1
   14 NT=0
      IF( NS- NM) 16,15,15
   15 WRITE (2,20)  Z
C                                                                       
C     HALVE STEP SIZE                                                   
C                                                                       
      GOTO 9
   16 NS= NS*2
      FNS= NS
      DZ= S/ FNS
      DZOT= DZ*0.5
      G5R= G3R
      G5I= G3I
      G3R= G2R
      G3I= G2I
      GOTO 4
   17 CONTINUE
C                                                                       
C     ADD CONTRIBUTION OF NEAR SINGULARITY FOR DIAGONAL TERM            
C                                                                       
      IF( IJ) 19,18,19
   18 SGR=2.*( SGR+ LOG(( SQRT( B* B+ S* S)+ S)/ B))
      SGI=2.* SGI
   19 CONTINUE
C                                                                       
      RETURN
   20 FORMAT(' STEP SIZE LIMITED AT Z=',F10.5)
      END
C ***
C     DOUBLE PRECISION 6/4/85
C
      FUNCTION ISEGNO( ITAGI, MX)
C ***
C                                                                       
C     ISEGNO RETURNS THE SEGMENT NUMBER OF THE MTH SEGMENT HAVING THE   
C     TAG NUMBER ITAGI.  IF ITAGI=0 SEGMENT NUMBER M IS RETURNED.       
C                                                                       
      IMPLICIT REAL (A-H,O-Z)
      PARAMETER ( NM=600, N2M=800, N3M=1000)
      COMMON  /DATA/ LD, N1, N2, N, NP, M1, M2, M, MP, X( NM), Y( NM), 
     &Z( NM), SI( NM), BI( NM), ALP( NM), BET( NM), ICON1( N2M), ICON2(
     & N2M), ITAG( N2M), ICONX( NM), WLAM, IPSYM
      IF( MX.GT.0) GOTO 1
      WRITE (2,6) 
      STOP
    1 ICNT=0
      IF( ITAGI.NE.0) GOTO 2
      ISEGNO= MX
      RETURN
    2 IF( N.LT.1) GOTO 4
      DO 3  I=1, N
      IF( ITAG( I).NE. ITAGI) GOTO 3
      ICNT= ICNT+1
      IF( ICNT.EQ. MX) GOTO 5
    3 CONTINUE
    4 WRITE (2,7)  ITAGI
      STOP
    5 ISEGNO= I
C                                                                       
      RETURN
    6 FORMAT(4X,'CHECK DATA, PARAMETER SPECIFYING SEGMENT POSITION IN',
     &' A GROUP OF EQUAL TAGS MUST NOT BE ZERO')
    7 FORMAT(///,10X,'NO SEGMENT HAS AN ITAG OF ',I5)
      END
C ***
C     DOUBLE PRECISION 6/4/85
C
      SUBROUTINE LFACTR( A, NROW, IX1, IX2, IP)
C ***
C                                                                       
C     LFACTR PERFORMS GAUSS-DOOLITTLE MANIPULATIONS ON THE TWO BLOCKS OF
C     THE TRANSPOSED MATRIX IN CORE STORAGE.  THE GAUSS-DOOLITTLE       
C     ALGORITHM IS PRESENTED ON PAGES 411-416 OF A. RALSTON -- A FIRST  
C     COURSE IN NUMERICAL ANALYSIS.  COMMENTS BELOW REFER TO COMMENTS IN
C     RALSTONS TEXT.                                                    
C                                                                       
      IMPLICIT REAL (A-H,O-Z)
      PARAMETER ( NM=600, N2M=800, N3M=1000)
      COMPLEX  A, D, AJR
      INTEGER  R, R1, R2, PJ, PR
      LOGICAL  L1, L2, L3
      COMMON  /MATPAR/ ICASE, NBLOKS, NPBLK, NLAST, NBLSYM, NPSYM, 
     &NLSYM, IMAT, ICASX, NBBX, NPBX, NLBX, NBBL, NPBL, NLBL
      COMMON  /SCRATM/ D( N2M)
      DIMENSION  A( NROW,1), IP( NROW)
C                                                                       
C     INITIALIZE R1,R2,J1,J2                                            
C                                                                       
      IFLG=0
      L1= IX1.EQ.1.AND. IX2.EQ.2
      L2=( IX2-1).EQ. IX1
      L3= IX2.EQ. NBLSYM
      IF( L1) GOTO 1
      GOTO 2
    1 R1=1
      R2=2* NPSYM
      J1=1
      J2=-1
      GOTO 5
    2 R1= NPSYM+1
      R2=2* NPSYM
      J1=( IX1-1)* NPSYM+1
      IF( L2) GOTO 3
      GOTO 4
    3 J2= J1+ NPSYM-2
      GOTO 5
    4 J2= J1+ NPSYM-1
    5 IF( L3) R2= NPSYM+ NLSYM
C                                                                       
C     STEP 1                                                            
C                                                                       
      DO 16  R= R1, R2
      DO 6  K= J1, NROW
      D( K)= A( K, R)
C                                                                       
C     STEPS 2 AND 3                                                     
C                                                                       
    6 CONTINUE
      IF( L1.OR. L2) J2= J2+1
      IF( J1.GT. J2) GOTO 9
      IXJ=0
      DO 8  J= J1, J2
      IXJ= IXJ+1
      PJ= IP( J)
      AJR= D( PJ)
      A( J, R)= AJR
      D( PJ)= D( J)
      JP1= J+1
      DO 7  I= JP1, NROW
      D( I)= D( I)- A( I, IXJ)* AJR
    7 CONTINUE
    8 CONTINUE
C                                                                       
C     STEP 4                                                            
C                                                                       
    9 CONTINUE
      J2P1= J2+1
      IF( L1.OR. L2) GOTO 11
      IF( NROW.LT. J2P1) GOTO 16
      DO 10  I= J2P1, NROW
      A( I, R)= D( I)
   10 CONTINUE
      GOTO 16
   11 DMAX= REAL( D( J2P1)* CONJG( D( J2P1)))
      IP( J2P1)= J2P1
      J2P2= J2+2
      IF( J2P2.GT. NROW) GOTO 13
      DO 12  I= J2P2, NROW
      ELMAG= REAL( D( I)* CONJG( D( I)))
      IF( ELMAG.LT. DMAX) GOTO 12
      DMAX= ELMAG
      IP( J2P1)= I
   12 CONTINUE
   13 CONTINUE
      IF( DMAX.LT.1.D-10) IFLG=1
      PR= IP( J2P1)
      A( J2P1, R)= D( PR)
C                                                                       
C     STEP 5                                                            
C                                                                       
      D( PR)= D( J2P1)
      IF( J2P2.GT. NROW) GOTO 15
      AJR=1./ A( J2P1, R)
      DO 14  I= J2P2, NROW
      A( I, R)= D( I)* AJR
   14 CONTINUE
   15 CONTINUE
      IF( IFLG.EQ.0) GOTO 16
      WRITE (2,17)  J2, DMAX
      IFLG=0
   16 CONTINUE
C                                                                       
      RETURN
   17 FORMAT(' ','PIVOT(,I3,2H)=',1P,E16.8)
      END
C ***
C     DOUBLE PRECISION 6/4/85
C
      SUBROUTINE LOAD( LDTYP, LDTAG, LDTAGF, LDTAGT, ZLR, ZLI, ZLC)
C ***
C                                                                       
C     LOAD CALCULATES THE IMPEDANCE OF SPECIFIED SEGMENTS FOR VARIOUS   
C     TYPES OF LOADING                                                  
C                                                                       
      IMPLICIT REAL (A-H,O-Z)
      PARAMETER ( NM=600, N2M=800, N3M=1000)
      COMPLEX  ZARRAY, ZT, TPCJ, ZINT
      COMMON  /DATA/ LD, N1, N2, N, NP, M1, M2, M, MP, X( NM), Y( NM), 
     &Z( NM), SI( NM), BI( NM), ALP( NM), BET( NM), ICON1( N2M), ICON2(
     & N2M), ITAG( N2M), ICONX( NM), WLAM, IPSYM
      COMMON  /ZLOAD/ ZARRAY( NM), NLOAD, NLODF
      DIMENSION  LDTYP(1), LDTAG(1), LDTAGF(1), LDTAGT(1), ZLR(1), ZLI(
     &1), ZLC(1), TPCJX(2)
      EQUIVALENCE(TPCJ,TPCJX)
C                                                                       
C     WRITE(6,HEADING)                                                  
C                                                                       
      DATA   TPCJX/0.,1.883698955D+9/
C                                                                       
C     INITIALIZE D ARRAY, USED FOR TEMPORARY STORAGE OF LOADING         
C     INFORMATION.                                                      
C                                                                       
      WRITE (2,25) 
      DO 1  I= N2, N
    1 ZARRAY( I)=(0.,0.)
C                                                                       
C     CYCLE OVER LOADING CARDS                                          
C                                                                       
      IWARN=0
      ISTEP=0
    2 ISTEP= ISTEP+1
      IF( ISTEP.LE. NLOAD) GOTO 5
      IF( IWARN.EQ.1) WRITE (2,26) 
      IF( N1+2* M1.GT.0) GOTO 4
      NOP= N/ NP
      IF( NOP.EQ.1) GOTO 4
      DO 3  I=1, NP
      ZT= ZARRAY( I)
      L1= I
      DO 3  L2=2, NOP
      L1= L1+ NP
    3 ZARRAY( L1)= ZT
    4 RETURN
    5 IF( LDTYP( ISTEP).LE.5) GOTO 6
      WRITE (2,27)  LDTYP( ISTEP)
      STOP
    6 LDTAGS= LDTAG( ISTEP)
      JUMP= LDTYP( ISTEP)+1
C                                                                       
C     SEARCH SEGMENTS FOR PROPER ITAGS                                  
C                                                                       
      ICHK=0
      L1= N2
      L2= N
      IF( LDTAGS.NE.0) GOTO 7
      IF( LDTAGF( ISTEP).EQ.0.AND. LDTAGT( ISTEP).EQ.0) GOTO 7
      L1= LDTAGF( ISTEP)
      L2= LDTAGT( ISTEP)
      IF( L1.GT. N1) GOTO 7
      WRITE (2,29) 
      STOP
    7 DO 17  I= L1, L2
      IF( LDTAGS.EQ.0) GOTO 8
      IF( LDTAGS.NE. ITAG( I)) GOTO 17
      IF( LDTAGF( ISTEP).EQ.0) GOTO 8
      ICHK= ICHK+1
      IF( ICHK.GE. LDTAGF( ISTEP).AND. ICHK.LE. LDTAGT( ISTEP)) GOTO 9
      GOTO 17
C                                                                       
C     CALCULATION OF LAMDA*IMPED. PER UNIT LENGTH, JUMP TO APPROPRIATE  
C     SECTION FOR LOADING TYPE                                          
C                                                                       
    8 ICHK=1
    9 GOTO (10,11,12,13,14,15), JUMP
   10 ZT= ZLR( ISTEP)/ SI( I)+ TPCJ* ZLI( ISTEP)/( SI( I)* WLAM)
      IF( ABS( ZLC( ISTEP)).GT.1.D-20) ZT= ZT+ WLAM/( TPCJ* SI( I)* ZLC
     &( ISTEP))
      GOTO 16
   11 ZT= TPCJ* SI( I)* ZLC( ISTEP)/ WLAM
      IF( ABS( ZLI( ISTEP)).GT.1.D-20) ZT= ZT+ SI( I)* WLAM/( TPCJ* ZLI
     &( ISTEP))
      IF( ABS( ZLR( ISTEP)).GT.1.D-20) ZT= ZT+ SI( I)/ ZLR( ISTEP)
      ZT=1./ ZT
      GOTO 16
   12 ZT= ZLR( ISTEP)* WLAM+ TPCJ* ZLI( ISTEP)
      IF( ABS( ZLC( ISTEP)).GT.1.D-20) ZT= ZT+1./( TPCJ* SI( I)* SI( I)
     &* ZLC( ISTEP))
      GOTO 16
   13 ZT= TPCJ* SI( I)* SI( I)* ZLC( ISTEP)
      IF( ABS( ZLI( ISTEP)).GT.1.D-20) ZT= ZT+1./( TPCJ* ZLI( ISTEP))
      IF( ABS( ZLR( ISTEP)).GT.1.D-20) ZT= ZT+1./( ZLR( ISTEP)* WLAM)
      ZT=1./ ZT
      GOTO 16
   14 ZT= CMPLX( ZLR( ISTEP), ZLI( ISTEP))/ SI( I)
      GOTO 16
   15 ZT= ZINT( ZLR( ISTEP)* WLAM, BI( I))
   16 IF(( ABS( REAL( ZARRAY( I)))+ ABS( AIMAG( ZARRAY( I)))).GT.1.D-20
     &) IWARN=1
      ZARRAY( I)= ZARRAY( I)+ ZT
   17 CONTINUE
      IF( ICHK.NE.0) GOTO 18
      WRITE (2,28)  LDTAGS
C                                                                       
C     PRINTING THE SEGMENT LOADING DATA, JUMP TO PROPER PRINT           
C                                                                       
      STOP
   18 GOTO (19,20,21,22,23,24), JUMP
   19 CALL PRNT( LDTAGS, LDTAGF( ISTEP), LDTAGT( ISTEP), ZLR( ISTEP), 
     &ZLI( ISTEP), ZLC( ISTEP),0.,0.,0.,' SERIES ',2)
      GOTO 2
   20 CALL PRNT( LDTAGS, LDTAGF( ISTEP), LDTAGT( ISTEP), ZLR( ISTEP), 
     &ZLI( ISTEP), ZLC( ISTEP),0.,0.,0.,'PARALLEL',2)                
      GOTO 2
   21 CALL PRNT( LDTAGS, LDTAGF( ISTEP), LDTAGT( ISTEP), ZLR( ISTEP), 
     &ZLI( ISTEP), ZLC( ISTEP),0.,0.,0.,'SERIES (PER METER)',5)
      GOTO 2
   22 CALL PRNT( LDTAGS, LDTAGF( ISTEP), LDTAGT( ISTEP), ZLR( ISTEP), 
     &ZLI( ISTEP), ZLC( ISTEP),0.,0.,0.,'PARALLEL (PER METER)',5)
      GOTO 2
   23 CALL PRNT( LDTAGS, LDTAGF( ISTEP), LDTAGT( ISTEP),0.,0.,0., ZLR( 
     &ISTEP), ZLI( ISTEP),0.,'FIXED IMPEDANCE ',4)
      GOTO 2
   24 CALL PRNT( LDTAGS, LDTAGF( ISTEP), LDTAGT( ISTEP),0.,0.,0.,0.,0.,
     & ZLR( ISTEP),'  WIRE  ',2)
C                                                                       
      GOTO 2
   25 FORMAT(//,7X,'LOCATION',10X,'RESISTANCE',3X,'INDUCTANCE',2X,
     &'CAPACITANCE',7X,'IMPEDANCE (OHMS)',5X,'CONDUCTIVITY',4X,'TYPE',/
     &,4X,'ITAG',' FROM THRU',10X,'OHMS',8X,'HENRYS',7X,'FARADS',8X,
     &'REAL',6X,'IMAGINARY',4X,'MHOS/METER')
   26 FORMAT(/,10X,'NOTE, SOME OF THE ABOVE SEGMENTS HAVE BEEN LOADED',
     &' TWICE - IMPEDANCES ADDED')
   27 FORMAT(/,10X,'IMPROPER LOAD TYPE CHOOSEN, REQUESTED TYPE IS ',I3)
     &
   28 FORMAT(/,10X,'LOADING DATA CARD ERROR, NO SEGMENT HAS AN ITAG =',
     &I5)
   29 FORMAT(' ERROR - LOADING MAY NOT BE ADDED TO SEGMENTS IN N.G.F.',
     &' SECTION')
      END
C ***
C     DOUBLE PRECISION 6/4/85
C
      SUBROUTINE LTSOLV( A, NROW, IX, B, NEQ, NRH, IFL1, IFL2)
C ***
C                                                                       
C     LTSOLV SOLVES THE MATRIX EQ. Y(R)*LU(T)=B(R) WHERE (R) DENOTES ROW
C     VECTOR AND LU(T) DENOTES THE LU DECOMPOSITION OF THE TRANSPOSE OF 
C     THE ORIGINAL COEFFICIENT MATRIX.  THE LU(T) DECOMPOSITION IS      
C     STORED ON TAPE 5 IN BLOCKS IN ASCENDING ORDER AND ON FILE 3 IN    
C     BLOCKS OF DESCENDING ORDER.                                       
C                                                                       
      IMPLICIT REAL (A-H,O-Z)
      PARAMETER ( NM=600, N2M=800, N3M=1000)
      COMPLEX  A, B, Y, SUM
      COMMON  /MATPAR/ ICASE, NBLOKS, NPBLK, NLAST, NBLSYM, NPSYM, 
     &NLSYM, IMAT, ICASX, NBBX, NPBX, NLBX, NBBL, NPBL, NLBL
      COMMON  /SCRATM/ Y( N2M)
C                                                                       
C     FORWARD SUBSTITUTION                                              
C                                                                       
      DIMENSION  A( NROW, NROW), B( NEQ, NRH), IX( NEQ)
      I2=2* NPSYM* NROW
      DO 4  IXBLK1=1, NBLSYM
      CALL BLCKIN( A, IFL1,1, I2,1,121)
      K2= NPSYM
      IF( IXBLK1.EQ. NBLSYM) K2= NLSYM
      JST=( IXBLK1-1)* NPSYM
      DO 4  IC=1, NRH
      J= JST
      DO 3  K=1, K2
      JM1= J
      J= J+1
      SUM=(0.,0.)
      IF( JM1.LT.1) GOTO 2
      DO 1  I=1, JM1
    1 SUM= SUM+ A( I, K)* B( I, IC)
    2 B( J, IC)=( B( J, IC)- SUM)/ A( J, K)
    3 CONTINUE
C                                                                       
C     BACKWARD SUBSTITUTION                                             
C                                                                       
    4 CONTINUE
      JST= NROW+1
      DO 8  IXBLK1=1, NBLSYM
      CALL BLCKIN( A, IFL2,1, I2,1,122)
      K2= NPSYM
      IF( IXBLK1.EQ.1) K2= NLSYM
      DO 7  IC=1, NRH
      KP= K2+1
      J= JST
      DO 6  K=1, K2
      KP= KP-1
      JP1= J
      J= J-1
      SUM=(0.,0.)
      IF( NROW.LT. JP1) GOTO 6
      DO 5  I= JP1, NROW
    5 SUM= SUM+ A( I, KP)* B( I, IC)
      B( J, IC)= B( J, IC)- SUM
    6 CONTINUE
    7 CONTINUE
C                                                                       
C     UNSCRAMBLE SOLUTION                                               
C                                                                       
    8 JST= JST- K2
      DO 10  IC=1, NRH
      DO 9  I=1, NROW
      IXI= IX( I)
    9 Y( IXI)= B( I, IC)
      DO 10  I=1, NROW
   10 B( I, IC)= Y( I)
      RETURN
      END
C ***
C     DOUBLE PRECISION 6/4/85
C
      SUBROUTINE LUNSCR( A, NROW, NOP, IX, IP, IU2, IU3, IU4)
C ***
C                                                                       
C     S/R WHICH UNSCRAMBLES, SCRAMBLED FACTORED MATRIX                  
C                                                                       
      IMPLICIT REAL (A-H,O-Z)
      PARAMETER ( NM=600, N2M=800, N3M=1000)
      COMPLEX  A, TEMP
      COMMON  /MATPAR/ ICASE, NBLOKS, NPBLK, NLAST, NBLSYM, NPSYM, 
     &NLSYM, IMAT, ICASX, NBBX, NPBX, NLBX, NBBL, NPBL, NLBL
      DIMENSION  A( NROW,1), IP( NROW), IX( NROW)
      I1=1
      I2=2* NPSYM* NROW
      NM1= NROW-1
      REWIND IU2
      REWIND IU3
      REWIND IU4
      DO 9  KK=1, NOP
      KA=( KK-1)* NROW
      DO 4  IXBLK1=1, NBLSYM
      CALL BLCKIN( A, IU2, I1, I2,1,121)
      K1=( IXBLK1-1)* NPSYM+2
      IF( NM1.LT. K1) GOTO 3
      J2=0
      DO 2  K= K1, NM1
      IF( J2.LT. NPSYM) J2= J2+1
      IPK= IP( K+ KA)
      DO 1  J=1, J2
      TEMP= A( K, J)
      A( K, J)= A( IPK, J)
      A( IPK, J)= TEMP
    1 CONTINUE
    2 CONTINUE
    3 CONTINUE
      CALL BLCKOT( A, IU3, I1, I2,1,122)
    4 CONTINUE
      DO 5  IXBLK1=1, NBLSYM
      BACKSPACE IU3
      IF( IXBLK1.NE.1) BACKSPACE IU3
      CALL BLCKIN( A, IU3, I1, I2,1,123)
      CALL BLCKOT( A, IU4, I1, I2,1,124)
    5 CONTINUE
      DO 6  I=1, NROW
      IX( I+ KA)= I
    6 CONTINUE
      DO 7  I=1, NROW
      IPI= IP( I+ KA)
      IXT= IX( I+ KA)
      IX( I+ KA)= IX( IPI+ KA)
      IX( IPI+ KA)= IXT
    7 CONTINUE
      IF( NOP.EQ.1) GOTO 9
C     SKIP NB1 LOGICAL RECORDS FORWARD                                  
      NB1= NBLSYM-1
      DO 8  IXBLK1=1, NB1
      CALL BLCKIN( A, IU3, I1, I2,1,125)
    8 CONTINUE
    9 CONTINUE
      REWIND IU2
      REWIND IU3
      REWIND IU4
      RETURN
      END
C ***
C     DOUBLE PRECISION 6/4/85
C
      SUBROUTINE MOVE( ROX, ROY, ROZ, XS, YS, ZS, ITS, NRPT, ITGI)
C ***
C                                                                       
C     SUBROUTINE MOVE MOVES THE STRUCTURE WITH RESPECT TO ITS           
C     COORDINATE SYSTEM OR REPRODUCES STRUCTURE IN NEW POSITIONS.       
C     STRUCTURE IS ROTATED ABOUT X,Y,Z AXES BY ROX,ROY,ROZ              
C     RESPECTIVELY, THEN SHIFTED BY XS,YS,ZS                            
C                                                                       
      IMPLICIT REAL (A-H,O-Z)
      PARAMETER ( NM=600, N2M=800, N3M=1000)
      COMMON  /DATA/ LD, N1, N2, N, NP, M1, M2, M, MP, X( NM), Y( NM), 
     &Z( NM), SI( NM), BI( NM), ALP( NM), BET( NM), ICON1( N2M), ICON2(
     & N2M), ITAG( N2M), ICONX( NM), WLAM, IPSYM
      COMMON  /ANGL/ SALP( NM)
      DIMENSION  T1X(1), T1Y(1), T1Z(1), T2X(1), T2Y(1), T2Z(1), X2(1),
     & Y2(1), Z2(1)
      EQUIVALENCE(X2(1),SI(1)),(Y2(1),ALP(1)),(Z2(1),BET(1))
      EQUIVALENCE(T1X,SI),(T1Y,ALP),(T1Z,BET),(T2X,ICON1),(T2Y,ICON2),(
     &T2Z,ITAG)
      IF( ABS( ROX)+ ABS( ROY).GT.1.D-10) IPSYM= IPSYM*3
      SPS= SIN( ROX)
      CPS= COS( ROX)
      STH= SIN( ROY)
      CTH= COS( ROY)
      SPH= SIN( ROZ)
      CPH= COS( ROZ)
      XX= CPH* CTH
      XY= CPH* STH* SPS- SPH* CPS
      XZ= CPH* STH* CPS+ SPH* SPS
      YX= SPH* CTH
      YY= SPH* STH* SPS+ CPH* CPS
      YZ= SPH* STH* CPS- CPH* SPS
      ZX=- STH
      ZY= CTH* SPS
      ZZ= CTH* CPS
      NRP= NRPT
      IF( NRPT.EQ.0) NRP=1
      IX=1
      IF( N.LT. N2) GOTO 3
      I1= ISEGNO( ITS,1)
      IF( I1.LT. N2) I1= N2
      IX= I1
      K= N
      IF( NRPT.EQ.0) K= I1-1
      DO 2  IR=1, NRP
      DO 1  I= I1, N
      K= K+1
      XI= X( I)
      YI= Y( I)
      ZI= Z( I)
      X( K)= XI* XX+ YI* XY+ ZI* XZ+ XS
      Y( K)= XI* YX+ YI* YY+ ZI* YZ+ YS
      Z( K)= XI* ZX+ YI* ZY+ ZI* ZZ+ ZS
      XI= X2( I)
      YI= Y2( I)
      ZI= Z2( I)
      X2( K)= XI* XX+ YI* XY+ ZI* XZ+ XS
      Y2( K)= XI* YX+ YI* YY+ ZI* YZ+ YS
      Z2( K)= XI* ZX+ YI* ZY+ ZI* ZZ+ ZS
      BI( K)= BI( I)
      ITAG( K)= ITAG( I)
      IF( ITAG( I).NE.0) ITAG( K)= ITAG( I)+ ITGI
    1 CONTINUE
      I1= N+1
      N= K
    2 CONTINUE
    3 IF( M.LT. M2) GOTO 6
      I1= M2
      K= M
      LDI= LD+1
      IF( NRPT.EQ.0) K= M1
      DO 5  II=1, NRP
      DO 4  I= I1, M
      K= K+1
      IR= LDI- I
      KR= LDI- K
      XI= X( IR)
      YI= Y( IR)
      ZI= Z( IR)
      X( KR)= XI* XX+ YI* XY+ ZI* XZ+ XS
      Y( KR)= XI* YX+ YI* YY+ ZI* YZ+ YS
      Z( KR)= XI* ZX+ YI* ZY+ ZI* ZZ+ ZS
      XI= T1X( IR)
      YI= T1Y( IR)
      ZI= T1Z( IR)
      T1X( KR)= XI* XX+ YI* XY+ ZI* XZ
      T1Y( KR)= XI* YX+ YI* YY+ ZI* YZ
      T1Z( KR)= XI* ZX+ YI* ZY+ ZI* ZZ
      XI= T2X( IR)
      YI= T2Y( IR)
      ZI= T2Z( IR)
      T2X( KR)= XI* XX+ YI* XY+ ZI* XZ
      T2Y( KR)= XI* YX+ YI* YY+ ZI* YZ
      T2Z( KR)= XI* ZX+ YI* ZY+ ZI* ZZ
      SALP( KR)= SALP( IR)
    4 BI( KR)= BI( IR)
      I1= M+1
    5 M= K
    6 IF(( NRPT.EQ.0).AND.( IX.EQ.1)) RETURN
      NP= N
      MP= M
      IPSYM=0
      RETURN
      END
C ***
C     DOUBLE PRECISION 6/4/85
C
      SUBROUTINE NEFLD( XOB, YOB, ZOB, EX, EY, EZ)
C ***
C                                                                       
C     NEFLD COMPUTES THE NEAR FIELD AT SPECIFIED POINTS IN SPACE AFTER  
C     THE STRUCTURE CURRENTS HAVE BEEN COMPUTED.                        
C                                                                       
      IMPLICIT REAL (A-H,O-Z)
      PARAMETER ( NM=600, N2M=800, N3M=1000)
      COMPLEX  EX, EY, EZ, CUR, ACX, BCX, CCX, EXK, EYK, EZK, EXS, 
     &EYS, EZS, EXC, EYC, EZC, ZRATI, ZRATI2, T1, FRATI
      COMMON  /DATA/ LD, N1, N2, N, NP, M1, M2, M, MP, X( NM), Y( NM), 
     &Z( NM), SI( NM), BI( NM), ALP( NM), BET( NM), ICON1( N2M), ICON2(
     & N2M), ITAG( N2M), ICONX( NM), WLAM, IPSYM
      COMMON  /ANGL/ SALP( NM)
      COMMON  /CRNT/ AIR( NM), AII( NM), BIR( NM), BII( NM), CIR( NM), 
     &CII( NM), CUR( N3M)
      COMMON  /DATAJ/ S, B, XJ, YJ, ZJ, CABJ, SABJ, SALPJ, EXK, EYK, 
     &EZK, EXS, EYS, EZS, EXC, EYC, EZC, RKH, IEXK, IND1, INDD1, IND2, 
     &INDD2, IPGND
      COMMON  /GND/ ZRATI, ZRATI2, FRATI, CL, CH, SCRWL, SCRWR, NRADL, 
     &KSYMP, IFAR, IPERF, T1, T2
      DIMENSION  CAB(1), SAB(1), T1X(1), T1Y(1), T1Z(1), T2X(1), T2Y(1)
     &, T2Z(1)
      EQUIVALENCE(CAB,ALP),(SAB,BET)
      EQUIVALENCE(T1X,SI),(T1Y,ALP),(T1Z,BET),(T2X,ICON1),(T2Y,ICON2),(
     &T2Z,ITAG)
      EQUIVALENCE(T1XJ,CABJ),(T1YJ,SABJ),(T1ZJ,SALPJ),(T2XJ,B),(T2YJ,
     &IND1),(T2ZJ,IND2)
      EX=(0.,0.)
      EY=(0.,0.)
      EZ=(0.,0.)
      AX=0.
      IF( N.EQ.0) GOTO 20
      DO 1  I=1, N
      XJ= XOB- X( I)
      YJ= YOB- Y( I)
      ZJ= ZOB- Z( I)
      ZP= CAB( I)* XJ+ SAB( I)* YJ+ SALP( I)* ZJ
      IF( ABS( ZP).GT.0.5001* SI( I)) GOTO 1
      ZP= XJ* XJ+ YJ* YJ+ ZJ* ZJ- ZP* ZP
      XJ= BI( I)
      IF( ZP.GT.0.9* XJ* XJ) GOTO 1
      AX= XJ
      GOTO 2
    1 CONTINUE
    2 DO 19  I=1, N
      S= SI( I)
      B= BI( I)
      XJ= X( I)
      YJ= Y( I)
      ZJ= Z( I)
      CABJ= CAB( I)
      SABJ= SAB( I)
      SALPJ= SALP( I)
      IF( IEXK.EQ.0) GOTO 18
      IPR= ICON1( I)
      IF( IPR) 3,8,4
    3 IPR=- IPR
      IF(- ICON1( IPR).NE. I) GOTO 9
      GOTO 6
    4 IF( IPR.NE. I) GOTO 5
      IF( CABJ* CABJ+ SABJ* SABJ.GT.1.D-8) GOTO 9
      GOTO 7
    5 IF( ICON2( IPR).NE. I) GOTO 9
    6 XI= ABS( CABJ* CAB( IPR)+ SABJ* SAB( IPR)+ SALPJ* SALP( IPR))
      IF( XI.LT.0.999999D+0) GOTO 9
      IF( ABS( BI( IPR)/ B-1.).GT.1.D-6) GOTO 9
    7 IND1=0
      GOTO 10
    8 IND1=1
      GOTO 10
    9 IND1=2
   10 IPR= ICON2( I)
      IF( IPR) 11,16,12
   11 IPR=- IPR
      IF(- ICON2( IPR).NE. I) GOTO 17
      GOTO 14
   12 IF( IPR.NE. I) GOTO 13
      IF( CABJ* CABJ+ SABJ* SABJ.GT.1.D-8) GOTO 17
      GOTO 15
   13 IF( ICON1( IPR).NE. I) GOTO 17
   14 XI= ABS( CABJ* CAB( IPR)+ SABJ* SAB( IPR)+ SALPJ* SALP( IPR))
      IF( XI.LT.0.999999D+0) GOTO 17
      IF( ABS( BI( IPR)/ B-1.).GT.1.D-6) GOTO 17
   15 IND2=0
      GOTO 18
   16 IND2=1
      GOTO 18
   17 IND2=2
   18 CONTINUE
      CALL EFLD( XOB, YOB, ZOB, AX,1)
      ACX= CMPLX( AIR( I), AII( I))
      BCX= CMPLX( BIR( I), BII( I))
      CCX= CMPLX( CIR( I), CII( I))
      EX= EX+ EXK* ACX+ EXS* BCX+ EXC* CCX
      EY= EY+ EYK* ACX+ EYS* BCX+ EYC* CCX
   19 EZ= EZ+ EZK* ACX+ EZS* BCX+ EZC* CCX
      IF( M.EQ.0) RETURN
   20 JC= N
      JL= LD+1
      DO 21  I=1, M
      JL= JL-1
      S= BI( JL)
      XJ= X( JL)
      YJ= Y( JL)
      ZJ= Z( JL)
      T1XJ= T1X( JL)
      T1YJ= T1Y( JL)
      T1ZJ= T1Z( JL)
      T2XJ= T2X( JL)
      T2YJ= T2Y( JL)
      T2ZJ= T2Z( JL)
      JC= JC+3
      ACX= T1XJ* CUR( JC-2)+ T1YJ* CUR( JC-1)+ T1ZJ* CUR( JC)
      BCX= T2XJ* CUR( JC-2)+ T2YJ* CUR( JC-1)+ T2ZJ* CUR( JC)
      DO 21  IP=1, KSYMP
      IPGND= IP
      CALL UNERE( XOB, YOB, ZOB)
      EX= EX+ ACX* EXK+ BCX* EXS
      EY= EY+ ACX* EYK+ BCX* EYS
   21 EZ= EZ+ ACX* EZK+ BCX* EZS
      RETURN
      END
C ***
C     DOUBLE PRECISION 6/4/85
C
      SUBROUTINE NETWK( CM, CMB, CMC, CMD, IP, EINC) 
C *** 
C                                                                       
C     SUBROUTINE NETWK SOLVES FOR STRUCTURE CURRENTS FOR A GIVEN        
C     EXCITATION INCLUDING THE EFFECT OF NON-RADIATING NETWORKS IF      
C     PRESENT.                                                          
C                                                                       
      IMPLICIT REAL (A-H,O-Z)
      PARAMETER ( NM=600, N2M=800, N3M=1000)
      COMPLEX  CMN, RHNT, YMIT, RHS, ZPED, EINC, VSANT, VLT, CUR, 
     &VSRC, RHNX, VQD, VQDS, CUX, CM, CMB, CMC, CMD
      COMMON  /DATA/ LD, N1, N2, N, NP, M1, M2, M, MP, X( NM), Y( NM), 
     &Z( NM), SI( NM), BI( NM), ALP( NM), BET( NM), ICON1( N2M), ICON2(
     & N2M), ITAG( N2M), ICONX( NM), WLAM, IPSYM
      COMMON  /CRNT/ AIR( NM), AII( NM), BIR( NM), BII( NM), CIR( NM), 
     &CII( NM), CUR( N3M)
      COMMON  /VSORC/ VQD(30), VSANT(30), VQDS(30), IVQD(30), ISANT(30)
     &, IQDS(30), NVQD, NSANT, NQDS
      COMMON  /NETCX/ ZPED, PIN, PNLS, NEQ, NPEQ, NEQ2, NONET, NTSOL, 
     &NPRINT, MASYM, ISEG1(150), ISEG2(150), X11R(150), X11I(150), 
     &X12R(150), X12I(150), X22R(150), X22I(150), NTYP(150)
      DIMENSION  EINC(1), IP(1), CM(1), CMB(1), CMC(1), CMD(1)
      DIMENSION  CMN(150,150), RHNT(150), IPNT(150), NTEQA(150),  
     &NTSCA(150), RHS( N3M), VSRC(10), RHNX(150)
      DATA   NDIMN, NDIMNP/150,151/, TP/6.283185308D+0/
      NEQZ2= NEQ2
      IF( NEQZ2.EQ.0) NEQZ2=1
      PIN=0.
      PNLS=0.
      NEQT= NEQ+ NEQ2
      IF( NTSOL.NE.0) GOTO 42
      NOP= NEQ/ NPEQ
C                                                                       
C     COMPUTE RELATIVE MATRIX ASYMMETRY                                 
C                                                                       
      IF( MASYM.EQ.0) GOTO 14
      IROW1=0
      IF( NONET.EQ.0) GOTO 5
      DO 4  I=1, NONET
      NSEG1= ISEG1( I)
      DO 3  ISC1=1,2
      IF( IROW1.EQ.0) GOTO 2
      DO 1  J=1, IROW1
      IF( NSEG1.EQ. IPNT( J)) GOTO 3
    1 CONTINUE
    2 IROW1= IROW1+1
      IPNT( IROW1)= NSEG1
    3 NSEG1= ISEG2( I)
    4 CONTINUE
    5 IF( NSANT.EQ.0) GOTO 9
      DO 8  I=1, NSANT
      NSEG1= ISANT( I)
      IF( IROW1.EQ.0) GOTO 7
      DO 6  J=1, IROW1
      IF( NSEG1.EQ. IPNT( J)) GOTO 8
    6 CONTINUE
    7 IROW1= IROW1+1
      IPNT( IROW1)= NSEG1
    8 CONTINUE
    9 IF( IROW1.LT. NDIMNP) GOTO 10
      WRITE (2,59) 
      STOP
   10 IF( IROW1.LT.2) GOTO 14
      DO 12  I=1, IROW1
      ISC1= IPNT( I)
      ASM= SI( ISC1)
      DO 11  J=1, NEQT
   11 RHS( J)=(0.,0.)
      RHS( ISC1)=(1.,0.)
      CALL SOLGF( CM, CMB, CMC, CMD, RHS, IP, NP, N1, N, MP, M1, M, NEQ
     &, NEQ2, NEQZ2)
      CALL CABC( RHS)
      DO 12  J=1, IROW1
      ISC1= IPNT( J)
   12 CMN( J, I)= RHS( ISC1)/ ASM
      ASM=0.
      ASA=0.
      DO 13  I=2, IROW1
      ISC1= I-1
      DO 13  J=1, ISC1
      CUX= CMN( I, J)
      PWR= ABS(( CUX- CMN( J, I))/ CUX)
      ASA= ASA+ PWR* PWR
      IF( PWR.LT. ASM) GOTO 13
      ASM= PWR
      NTEQ= IPNT( I)
      NTSC= IPNT( J)
   13 CONTINUE
      ASA= SQRT( ASA*2./ DFLOAT( IROW1*( IROW1-1)))
      WRITE (2,58)  ASM, NTEQ, NTSC, ASA
C                                                                       
C     SOLUTION OF NETWORK EQUATIONS                                     
C                                                                       
   14 IF( NONET.EQ.0) GOTO 48
      DO 15  I=1, NDIMN
      RHNX( I)=(0.,0.)
      DO 15  J=1, NDIMN
   15 CMN( I, J)=(0.,0.)
      NTEQ=0
C                                                                       
C     SORT NETWORK AND SOURCE DATA AND ASSIGN EQUATION NUMBERS TO       
C     SEGMENTS.                                                         
C                                                                       
      NTSC=0
      DO 38  J=1, NONET
      NSEG1= ISEG1( J)
      NSEG2= ISEG2( J)
      IF( NTYP( J).GT.1) GOTO 16
      Y11R= X11R( J)
      Y11I= X11I( J)
      Y12R= X12R( J)
      Y12I= X12I( J)
      Y22R= X22R( J)
      Y22I= X22I( J)
      GOTO 17
   16 Y22R= TP* X11I( J)/ WLAM
      Y12R=0.
      Y12I=1./( X11R( J)* SIN( Y22R))
      Y11R= X12R( J)
      Y11I=- Y12I* COS( Y22R)
      Y22R= X22R( J)
      Y22I= Y11I+ X22I( J)
      Y11I= Y11I+ X12I( J)
      IF( NTYP( J).EQ.2) GOTO 17
      Y12R=- Y12R
      Y12I=- Y12I
   17 IF( NSANT.EQ.0) GOTO 19
      DO 18  I=1, NSANT
      IF( NSEG1.NE. ISANT( I)) GOTO 18
      ISC1= I
      GOTO 22
   18 CONTINUE
   19 ISC1=0
      IF( NTEQ.EQ.0) GOTO 21
      DO 20  I=1, NTEQ
      IF( NSEG1.NE. NTEQA( I)) GOTO 20
      IROW1= I
      GOTO 25
   20 CONTINUE
   21 NTEQ= NTEQ+1
      IROW1= NTEQ
      NTEQA( NTEQ)= NSEG1
      GOTO 25
   22 IF( NTSC.EQ.0) GOTO 24
      DO 23  I=1, NTSC
      IF( NSEG1.NE. NTSCA( I)) GOTO 23
      IROW1= NDIMNP- I
      GOTO 25
   23 CONTINUE
   24 NTSC= NTSC+1
      IROW1= NDIMNP- NTSC
      NTSCA( NTSC)= NSEG1
      VSRC( NTSC)= VSANT( ISC1)
   25 IF( NSANT.EQ.0) GOTO 27
      DO 26  I=1, NSANT
      IF( NSEG2.NE. ISANT( I)) GOTO 26
      ISC2= I
      GOTO 30
   26 CONTINUE
   27 ISC2=0
      IF( NTEQ.EQ.0) GOTO 29
      DO 28  I=1, NTEQ
      IF( NSEG2.NE. NTEQA( I)) GOTO 28
      IROW2= I
      GOTO 33
   28 CONTINUE
   29 NTEQ= NTEQ+1
      IROW2= NTEQ
      NTEQA( NTEQ)= NSEG2
      GOTO 33
   30 IF( NTSC.EQ.0) GOTO 32
      DO 31  I=1, NTSC
      IF( NSEG2.NE. NTSCA( I)) GOTO 31
      IROW2= NDIMNP- I
      GOTO 33
   31 CONTINUE
   32 NTSC= NTSC+1
      IROW2= NDIMNP- NTSC
      NTSCA( NTSC)= NSEG2
      VSRC( NTSC)= VSANT( ISC2)
   33 IF( NTSC+ NTEQ.LT. NDIMNP) GOTO 34
      WRITE (2,59) 
C                                                                       
C     FILL NETWORK EQUATION MATRIX AND RIGHT HAND SIDE VECTOR WITH      
C     NETWORK SHORT-CIRCUIT ADMITTANCE MATRIX COEFFICIENTS.             
C                                                                       
      STOP
   34 IF( ISC1.NE.0) GOTO 35
      CMN( IROW1, IROW1)= CMN( IROW1, IROW1)- CMPLX( Y11R, Y11I)* SI( 
     &NSEG1)
      CMN( IROW1, IROW2)= CMN( IROW1, IROW2)- CMPLX( Y12R, Y12I)* SI( 
     &NSEG1)
      GOTO 36
   35 RHNX( IROW1)= RHNX( IROW1)+ CMPLX( Y11R, Y11I)* VSANT( ISC1)/ 
     &WLAM
      RHNX( IROW2)= RHNX( IROW2)+ CMPLX( Y12R, Y12I)* VSANT( ISC1)/ 
     &WLAM
   36 IF( ISC2.NE.0) GOTO 37
      CMN( IROW2, IROW2)= CMN( IROW2, IROW2)- CMPLX( Y22R, Y22I)* SI( 
     &NSEG2)
      CMN( IROW2, IROW1)= CMN( IROW2, IROW1)- CMPLX( Y12R, Y12I)* SI( 
     &NSEG2)
      GOTO 38
   37 RHNX( IROW1)= RHNX( IROW1)+ CMPLX( Y12R, Y12I)* VSANT( ISC2)/ 
     &WLAM
      RHNX( IROW2)= RHNX( IROW2)+ CMPLX( Y22R, Y22I)* VSANT( ISC2)/ 
     &WLAM
C                                                                       
C     ADD INTERACTION MATRIX ADMITTANCE ELEMENTS TO NETWORK EQUATION    
C     MATRIX                                                            
C                                                                       
   38 CONTINUE
      DO 41  I=1, NTEQ
      DO 39  J=1, NEQT
   39 RHS( J)=(0.,0.)
      IROW1= NTEQA( I)
      RHS( IROW1)=(1.,0.)
      CALL SOLGF( CM, CMB, CMC, CMD, RHS, IP, NP, N1, N, MP, M1, M, NEQ
     &, NEQ2, NEQZ2)
      CALL CABC( RHS)
      DO 40  J=1, NTEQ
      IROW1= NTEQA( J)
   40 CMN( I, J)= CMN( I, J)+ RHS( IROW1)
C                                                                       
C     FACTOR NETWORK EQUATION MATRIX                                    
C                                                                       
   41 CONTINUE
C                                                                       
C     ADD TO NETWORK EQUATION RIGHT HAND SIDE THE TERMS DUE TO ELEMENT  
C     INTERACTIONS                                                      
C                                                                       
      CALL FACTR( NTEQ, CMN, IPNT, NDIMN)
   42 IF( NONET.EQ.0) GOTO 48
      DO 43  I=1, NEQT
   43 RHS( I)= EINC( I)
      CALL SOLGF( CM, CMB, CMC, CMD, RHS, IP, NP, N1, N, MP, M1, M, NEQ
     &, NEQ2, NEQZ2)
      CALL CABC( RHS)
      DO 44  I=1, NTEQ
      IROW1= NTEQA( I)
C                                                                       
C     SOLVE NETWORK EQUATIONS                                           
C                                                                       
   44 RHNT( I)= RHNX( I)+ RHS( IROW1)
C                                                                       
C     ADD FIELDS DUE TO NETWORK VOLTAGES TO ELECTRIC FIELDS APPLIED TO  
C     STRUCTURE AND SOLVE FOR INDUCED CURRENT                           
C                                                                       
      CALL SOLVE( NTEQ, CMN, IPNT, RHNT, NDIMN)
      DO 45  I=1, NTEQ
      IROW1= NTEQA( I)
   45 EINC( IROW1)= EINC( IROW1)- RHNT( I)
      CALL SOLGF( CM, CMB, CMC, CMD, EINC, IP, NP, N1, N, MP, M1, M, 
     &NEQ, NEQ2, NEQZ2)
      CALL CABC( EINC)
      IF( NPRINT.EQ.0) WRITE (2,61) 
      IF( NPRINT.EQ.0) WRITE (2,60) 
      DO 46  I=1, NTEQ
      IROW1= NTEQA( I)
      VLT= RHNT( I)* SI( IROW1)* WLAM
      CUX= EINC( IROW1)* WLAM
      YMIT= CUX/ VLT
      ZPED= VLT/ CUX
      IROW2= ITAG( IROW1)
      PWR=.5* REAL( VLT* CONJG( CUX))
      PNLS= PNLS- PWR
   46 IF( NPRINT.EQ.0) WRITE (2,62)  IROW2, IROW1, VLT, CUX, ZPED, YMIT
     &, PWR
      IF( NTSC.EQ.0) GOTO 49
      DO 47  I=1, NTSC
      IROW1= NTSCA( I)
      VLT= VSRC( I)
      CUX= EINC( IROW1)* WLAM
      YMIT= CUX/ VLT
      ZPED= VLT/ CUX
      IROW2= ITAG( IROW1)
      PWR=.5* REAL( VLT* CONJG( CUX))
      PNLS= PNLS- PWR
   47 IF( NPRINT.EQ.0) WRITE (2,62)  IROW2, IROW1, VLT, CUX, ZPED, YMIT
     &, PWR
C                                                                       
C     SOLVE FOR CURRENTS WHEN NO NETWORKS ARE PRESENT                   
C                                                                       
      GOTO 49
   48 CALL SOLGF( CM, CMB, CMC, CMD, EINC, IP, NP, N1, N, MP, M1, M, 
     &NEQ, NEQ2, NEQZ2)
      CALL CABC( EINC)
      NTSC=0
   49 IF( NSANT+ NVQD.EQ.0) RETURN
      WRITE (2,63) 
      WRITE (2,60) 
      IF( NSANT.EQ.0) GOTO 56
      DO 55  I=1, NSANT
      ISC1= ISANT( I)
      VLT= VSANT( I)
      IF( NTSC.EQ.0) GOTO 51
      DO 50  J=1, NTSC
      IF( NTSCA( J).EQ. ISC1) GOTO 52
   50 CONTINUE
   51 CUX= EINC( ISC1)* WLAM
      IROW1=0
      GOTO 54
   52 IROW1= NDIMNP- J
      CUX= RHNX( IROW1)
      DO 53  J=1, NTEQ
   53 CUX= CUX- CMN( J, IROW1)* RHNT( J)
      CUX=( EINC( ISC1)+ CUX)* WLAM
   54 YMIT= CUX/ VLT
      ZPED= VLT/ CUX
      PWR=.5* REAL( VLT* CONJG( CUX))
      PIN= PIN+ PWR
      IF( IROW1.NE.0) PNLS= PNLS+ PWR
      IROW2= ITAG( ISC1)
   55 WRITE (2,62)  IROW2, ISC1, VLT, CUX, ZPED, YMIT, PWR
   56 IF( NVQD.EQ.0) RETURN
      DO 57  I=1, NVQD
      ISC1= IVQD( I)
      VLT= VQD( I)
      CUX= CMPLX( AIR( ISC1), AII( ISC1))
      YMIT= CMPLX( BIR( ISC1), BII( ISC1))
      ZPED= CMPLX( CIR( ISC1), CII( ISC1))
      PWR= SI( ISC1)* TP*.5
      CUX=( CUX- YMIT* SIN( PWR)+ ZPED* COS( PWR))* WLAM
      YMIT= CUX/ VLT
      ZPED= VLT/ CUX
      PWR=.5* REAL( VLT* CONJG( CUX))
      PIN= PIN+ PWR
      IROW2= ITAG( ISC1)
   57 WRITE (2,64)  IROW2, ISC1, VLT, CUX, ZPED, YMIT, PWR
C                                                                       
      RETURN
   58 FORMAT(///,3X,'MAXIMUM RELATIVE ASYMMETRY OF THE DRIVING POINT',
     &' ADMITTANCE MATRIX IS',1P,E10.3,' FOR SEGMENTS',I5,4H AND,I5,/,3
     &X,'RMS RELATIVE ASYMMETRY IS',E10.3)
   59 FORMAT(1X,'ERROR - - NETWORK ARRAY DIMENSIONS TOO SMALL')
   60 FORMAT(/,3X,'TAG',3X,'SEG.',4X,'VOLTAGE (VOLTS)',9X,'CURRENT (',
     &'AMPS)',9X,'IMPEDANCE (OHMS)',8X,'ADMITTANCE (MHOS)',6X,'POWER',/
     &,3X,'NO.',3X,'NO.',4X,'REAL',8X,'IMAG.',3(7X,'REAL',8X,'IMAG.'),5
     &X,'(WATTS)')
   61 FORMAT(///,27X,'- - - STRUCTURE EXCITATION DATA AT NETWORK CONN',
     &'ECTION POINTS - - -')
   62 FORMAT(2(1X,I5),1P,9E12.5)
   63 FORMAT(///,42X,'- - - ANTENNA INPUT PARAMETERS - - -')
   64 FORMAT(1X,I5,' *',I4,1P,9E12.5)
      END
C ***
C     DOUBLE PRECISION 6/4/85
C
      SUBROUTINE NFPAT
C ***
C     COMPUTE NEAR E OR H FIELDS OVER A RANGE OF POINTS                 
      IMPLICIT REAL (A-H,O-Z)
      PARAMETER ( NM=600, N2M=800, N3M=1000)
      COMPLEX  EX, EY, EZ
      COMMON  /DATA/ LD, N1, N2, N, NP, M1, M2, M, MP, X( NM), Y( NM), 
     &Z( NM), SI( NM), BI( NM), ALP( NM), BET( NM), ICON1( N2M), ICON2(
     & N2M), ITAG( N2M), ICONX( NM), WLAM, IPSYM
C***
      COMMON  /FPAT/ NTH, NPH, IPD, IAVP, INOR, IAX, THETS, PHIS, DTH, 
     &DPH, RFLD, GNOR, CLT, CHT, EPSR2, SIG2, IXTYP, XPR6, PINR, PNLR, 
     &PLOSS, NEAR, NFEH, NRX, NRY, NRZ, XNR, YNR, ZNR, DXNR, DYNR, DZNR
     &
C***
      COMMON  /PLOT/ IPLP1, IPLP2, IPLP3, IPLP4
      DATA   TA/1.745329252D-02/
      IF( NFEH.EQ.1) GOTO 1
      WRITE (2,10) 
      GOTO 2
    1 WRITE (2,12) 
    2 ZNRT= ZNR- DZNR
      DO 9  I=1, NRZ
      ZNRT= ZNRT+ DZNR
      IF( NEAR.EQ.0) GOTO 3
      CTH= COS( TA* ZNRT)
      STH= SIN( TA* ZNRT)
    3 YNRT= YNR- DYNR
      DO 9  J=1, NRY
      YNRT= YNRT+ DYNR
      IF( NEAR.EQ.0) GOTO 4
      CPH= COS( TA* YNRT)
      SPH= SIN( TA* YNRT)
    4 XNRT= XNR- DXNR
      DO 9  KK=1, NRX
      XNRT= XNRT+ DXNR
      IF( NEAR.EQ.0) GOTO 5
      XOB= XNRT* STH* CPH
      YOB= XNRT* STH* SPH
      ZOB= XNRT* CTH
      GOTO 6
    5 XOB= XNRT
      YOB= YNRT
      ZOB= ZNRT
    6 TMP1= XOB/ WLAM
      TMP2= YOB/ WLAM
      TMP3= ZOB/ WLAM
      IF( NFEH.EQ.1) GOTO 7
      CALL NEFLD( TMP1, TMP2, TMP3, EX, EY, EZ)
      GOTO 8
    7 CALL NHFLD( TMP1, TMP2, TMP3, EX, EY, EZ)
    8 TMP1= ABS( EX)
      TMP2= CANG( EX)
      TMP3= ABS( EY)
      TMP4= CANG( EY)
      TMP5= ABS( EZ)
      TMP6= CANG( EZ)
C***
      WRITE (2,11)  XOB, YOB, ZOB, TMP1, TMP2, TMP3, TMP4, TMP5, TMP6
      IF( IPLP1.NE.2) GOTO 9
      GOTO (14,15,16), IPLP4
   14 XXX= XOB
      GOTO 17
   15 XXX= YOB
      GOTO 17
   16 XXX= ZOB
   17 CONTINUE
      IF( IPLP2.NE.2) GOTO 13
      IF( IPLP3.EQ.1) WRITE( 8,*)  XXX, TMP1, TMP2
      IF( IPLP3.EQ.2) WRITE( 8,*)  XXX, TMP3, TMP4
      IF( IPLP3.EQ.3) WRITE( 8,*)  XXX, TMP5, TMP6
      IF( IPLP3.EQ.4) WRITE( 8,*)  XXX, TMP1, TMP2, TMP3, TMP4, TMP5, 
     &TMP6
      GOTO 9
   13 IF( IPLP2.NE.1) GOTO 9
      IF( IPLP3.EQ.1) WRITE( 8,*)  XXX, EX
      IF( IPLP3.EQ.2) WRITE( 8,*)  XXX, EY
      IF( IPLP3.EQ.3) WRITE( 8,*)  XXX, EZ
C***
      IF( IPLP3.EQ.4) WRITE( 8,*)  XXX, EX, EY, EZ
    9 CONTINUE
C                                                                       
      RETURN
   10 FORMAT(///,35X,'- - - NEAR ELECTRIC FIELDS - - -',//,12X,'-  L',
     &'OCATION  -',21X,'-  EX  -',15X,'-  EY  -',15X,'-  EZ  -',/,8X,
     &'X',10X,'Y',10X,'Z',10X,'MAGNITUDE',3X,'PHASE',6X,'MAGNITUDE',3X,
     &'PHASE',6X,'MAGNITUDE',3X,'PHASE',/,6X,'METERS',5X,'METERS',5X,
     &'METERS',8X,'VOLTS/M',3X,'DEGREES',6X,'VOLTS/M',3X,'DEGREES',6X
     &,'VOLTS/M',3X,'DEGREES')
   11 FORMAT(2X,3(2X,F9.4),1X,3(3X,1P,E11.4,2X,0P,F7.2))
   12 FORMAT(///,35X,'- - - NEAR MAGNETIC FIELDS - - -',//,12X,'-  L',
     &'OCATION  -',21X,'-  HX  -',15X,'-  HY  -',15X,'-  HZ  -',/,8X,
     &'X',10X,'Y',10X,'Z',10X,'MAGNITUDE',3X,'PHASE',6X,'MAGNITUDE',3X,
     &'PHASE',6X,'MAGNITUDE',3X,'PHASE',/,6X,'METERS',5X,'METERS',5X,
     &'METERS',9X,'AMPS/M',3X,'DEGREES',7X,'AMPS/M',3X,'DEGREES',7X,
     &'AMPS/M',3X,'DEGREES')
      END
C ***
C     DOUBLE PRECISION 6/4/85
C
      SUBROUTINE NHFLD( XOB, YOB, ZOB, HX, HY, HZ)
C ***
C                                                                       
C     NHFLD COMPUTES THE NEAR FIELD AT SPECIFIED POINTS IN SPACE AFTER  
C     THE STRUCTURE CURRENTS HAVE BEEN COMPUTED.                        
C                                                                       
      IMPLICIT REAL (A-H,O-Z)
      PARAMETER ( NM=600, N2M=800, N3M=1000)
      COMPLEXHX,HY,HZ,CUR,ACX,  BCX, CCX, EXK, EYK, EZK, EXS, EYS, 
     &EZS, EXC, EYC, EZC
      COMMON  /DATA/ LD, N1, N2, N, NP, M1, M2, M, MP, X( NM), Y( NM), 
     &Z( NM), SI( NM), BI( NM), ALP( NM), BET( NM), ICON1( N2M), ICON2(
     & N2M), ITAG( N2M), ICONX( NM), WLAM, IPSYM
      COMMON  /ANGL/ SALP( NM)
      COMMON  /CRNT/ AIR( NM), AII( NM), BIR( NM), BII( NM), CIR( NM), 
     &CII( NM), CUR( N3M)
      COMMON  /DATAJ/ S, B, XJ, YJ, ZJ, CABJ, SABJ, SALPJ, EXK, EYK, 
     &EZK, EXS, EYS, EZS, EXC, EYC, EZC, RKH, IEXK, IND1, INDD1, IND2, 
     &INDD2, IPGND
      DIMENSION  CAB(1), SAB(1)
      DIMENSION  T1X(1), T1Y(1), T1Z(1), T2X(1), T2Y(1), T2Z(1), XS(1),
     & YS(1), ZS(1)
      EQUIVALENCE(T1X,SI),(T1Y,ALP),(T1Z,BET),(T2X,ICON1),(T2Y,ICON2),(
     &T2Z,ITAG),(XS,X),(YS,Y),(ZS,Z)
      EQUIVALENCE(T1XJ,CABJ),(T1YJ,SABJ),(T1ZJ,SALPJ),(T2XJ,B),(T2YJ,
     &IND1),(T2ZJ,IND2)
      EQUIVALENCE(CAB,ALP),(SAB,BET)
      HX=(0.,0.)
      HY=(0.,0.)
      HZ=(0.,0.)
      AX=0.
      IF( N.EQ.0) GOTO 4
      DO 1  I=1, N
      XJ= XOB- X( I)
      YJ= YOB- Y( I)
      ZJ= ZOB- Z( I)
      ZP= CAB( I)* XJ+ SAB( I)* YJ+ SALP( I)* ZJ
      IF( ABS( ZP).GT.0.5001* SI( I)) GOTO 1
      ZP= XJ* XJ+ YJ* YJ+ ZJ* ZJ- ZP* ZP
      XJ= BI( I)
      IF( ZP.GT.0.9* XJ* XJ) GOTO 1
      AX= XJ
      GOTO 2
    1 CONTINUE
    2 DO 3  I=1, N
      S= SI( I)
      B= BI( I)
      XJ= X( I)
      YJ= Y( I)
      ZJ= Z( I)
      CABJ= CAB( I)
      SABJ= SAB( I)
      SALPJ= SALP( I)
      CALL HSFLD( XOB, YOB, ZOB, AX)
      ACX= CMPLX( AIR( I), AII( I))
      BCX= CMPLX( BIR( I), BII( I))
      CCX= CMPLX( CIR( I), CII( I))
      HX= HX+ EXK* ACX+ EXS* BCX+ EXC* CCX
      HY= HY+ EYK* ACX+ EYS* BCX+ EYC* CCX
    3 HZ= HZ+ EZK* ACX+ EZS* BCX+ EZC* CCX
      IF( M.EQ.0) RETURN
    4 JC= N
      JL= LD+1
      DO 5  I=1, M
      JL= JL-1
      S= BI( JL)
      XJ= X( JL)
      YJ= Y( JL)
      ZJ= Z( JL)
      T1XJ= T1X( JL)
      T1YJ= T1Y( JL)
      T1ZJ= T1Z( JL)
      T2XJ= T2X( JL)
      T2YJ= T2Y( JL)
      T2ZJ= T2Z( JL)
      CALL HINTG( XOB, YOB, ZOB)
      JC= JC+3
      ACX= T1XJ* CUR( JC-2)+ T1YJ* CUR( JC-1)+ T1ZJ* CUR( JC)
      BCX= T2XJ* CUR( JC-2)+ T2YJ* CUR( JC-1)+ T2ZJ* CUR( JC)
      HX= HX+ ACX* EXK+ BCX* EXS
      HY= HY+ ACX* EYK+ BCX* EYS
    5 HZ= HZ+ ACX* EZK+ BCX* EZS
      RETURN
      END
C ***
C     DOUBLE PRECISION 6/4/85
C
      SUBROUTINE PATCH( NX, NY, X1, Y1, Z1, X2, Y2, Z2, X3, Y3, Z3, X4,
     & Y4, Z4)
C ***
C     PATCH GENERATES AND MODIFIES PATCH GEOMETRY DATA                  
      IMPLICIT REAL (A-H,O-Z)
      PARAMETER ( NM=600, N2M=800, N3M=1000)
      COMMON  /DATA/ LD, N1, N2, N, NP, M1, M2, M, MP, X( NM), Y( NM), 
     &Z( NM), SI( NM), BI( NM), ALP( NM), BET( NM), ICON1( N2M), ICON2(
     & N2M), ITAG( N2M), ICONX( NM), WLAM, IPSYM
      COMMON  /ANGL/ SALP( NM)
      DIMENSION  T1X(1), T1Y(1), T1Z(1), T2X(1), T2Y(1), T2Z(1)
C     NEW PATCHES.  FOR NX=0, NY=1,2,3,4 PATCH IS (RESPECTIVELY)        
C     ARBITRARY, RECTAGULAR, TRIANGULAR, OR QUADRILATERAL.              
C     FOR NX AND NY .GT. 0 A RECTANGULAR SURFACE IS PRODUCED WITH       
C     NX BY NY RECTANGULAR PATCHES.                                     
      EQUIVALENCE(T1X,SI),(T1Y,ALP),(T1Z,BET),(T2X,ICON1),(T2Y,ICON2),(
     &T2Z,ITAG)
      M= M+1
      MI= LD+1- M
      NTP= NY
      IF( NX.GT.0) NTP=2
      IF( NTP.GT.1) GOTO 2
      X( MI)= X1
      Y( MI)= Y1
      Z( MI)= Z1
      BI( MI)= Z2
      ZNV= COS( X2)
      XNV= ZNV* COS( Y2)
      YNV= ZNV* SIN( Y2)
      ZNV= SIN( X2)
      XA= SQRT( XNV* XNV+ YNV* YNV)
      IF( XA.LT.1.D-6) GOTO 1
      T1X( MI)=- YNV/ XA
      T1Y( MI)= XNV/ XA
      T1Z( MI)=0.
      GOTO 6
    1 T1X( MI)=1.
      T1Y( MI)=0.
      T1Z( MI)=0.
      GOTO 6
    2 S1X= X2- X1
      S1Y= Y2- Y1
      S1Z= Z2- Z1
      S2X= X3- X2
      S2Y= Y3- Y2
      S2Z= Z3- Z2
      IF( NX.EQ.0) GOTO 3
      S1X= S1X/ NX
      S1Y= S1Y/ NX
      S1Z= S1Z/ NX
      S2X= S2X/ NY
      S2Y= S2Y/ NY
      S2Z= S2Z/ NY
    3 XNV= S1Y* S2Z- S1Z* S2Y
      YNV= S1Z* S2X- S1X* S2Z
      ZNV= S1X* S2Y- S1Y* S2X
      XA= SQRT( XNV* XNV+ YNV* YNV+ ZNV* ZNV)
      XNV= XNV/ XA
      YNV= YNV/ XA
      ZNV= ZNV/ XA
      XST= SQRT( S1X* S1X+ S1Y* S1Y+ S1Z* S1Z)
      T1X( MI)= S1X/ XST
      T1Y( MI)= S1Y/ XST
      T1Z( MI)= S1Z/ XST
      IF( NTP.GT.2) GOTO 4
      X( MI)= X1+.5*( S1X+ S2X)
      Y( MI)= Y1+.5*( S1Y+ S2Y)
      Z( MI)= Z1+.5*( S1Z+ S2Z)
      BI( MI)= XA
      GOTO 6
    4 IF( NTP.EQ.4) GOTO 5
      X( MI)=( X1+ X2+ X3)/3.
      Y( MI)=( Y1+ Y2+ Y3)/3.
      Z( MI)=( Z1+ Z2+ Z3)/3.
      BI( MI)=.5* XA
      GOTO 6
    5 S1X= X3- X1
      S1Y= Y3- Y1
      S1Z= Z3- Z1
      S2X= X4- X1
      S2Y= Y4- Y1
      S2Z= Z4- Z1
      XN2= S1Y* S2Z- S1Z* S2Y
      YN2= S1Z* S2X- S1X* S2Z
      ZN2= S1X* S2Y- S1Y* S2X
      XST= SQRT( XN2* XN2+ YN2* YN2+ ZN2* ZN2)
      SALPN=1./(3.*( XA+ XST))
      X( MI)=( XA*( X1+ X2+ X3)+ XST*( X1+ X3+ X4))* SALPN
      Y( MI)=( XA*( Y1+ Y2+ Y3)+ XST*( Y1+ Y3+ Y4))* SALPN
      Z( MI)=( XA*( Z1+ Z2+ Z3)+ XST*( Z1+ Z3+ Z4))* SALPN
      BI( MI)=.5*( XA+ XST)
      S1X=( XNV* XN2+ YNV* YN2+ ZNV* ZN2)/ XST
      IF( S1X.GT.0.9998) GOTO 6
      WRITE (2,14) 
      STOP
    6 T2X( MI)= YNV* T1Z( MI)- ZNV* T1Y( MI)
      T2Y( MI)= ZNV* T1X( MI)- XNV* T1Z( MI)
      T2Z( MI)= XNV* T1Y( MI)- YNV* T1X( MI)
      SALP( MI)=1.
      IF( NX.EQ.0) GOTO 8
      M= M+ NX* NY-1
      XN2= X( MI)- S1X- S2X
      YN2= Y( MI)- S1Y- S2Y
      ZN2= Z( MI)- S1Z- S2Z
      XS= T1X( MI)
      YS= T1Y( MI)
      ZS= T1Z( MI)
      XT= T2X( MI)
      YT= T2Y( MI)
      ZT= T2Z( MI)
      MI= MI+1
      DO 7  IY=1, NY
      XN2= XN2+ S2X
      YN2= YN2+ S2Y
      ZN2= ZN2+ S2Z
      DO 7  IX=1, NX
      XST= IX
      MI= MI-1
      X( MI)= XN2+ XST* S1X
      Y( MI)= YN2+ XST* S1Y
      Z( MI)= ZN2+ XST* S1Z
      BI( MI)= XA
      SALP( MI)=1.
      T1X( MI)= XS
      T1Y( MI)= YS
      T1Z( MI)= ZS
      T2X( MI)= XT
      T2Y( MI)= YT
    7 T2Z( MI)= ZT
    8 IPSYM=0
      NP= N
      MP= M
C     DIVIDE PATCH FOR WIRE CONNECTION                                  
      RETURN
      ENTRY SUBPH( NX, NY, X1, Y1, Z1, X2, Y2, Z2, X3, Y3, Z3, X4, Y4, 
     &Z4)
      IF( NY.GT.0) GOTO 10
      IF( NX.EQ. M) GOTO 10
      NXP= NX+1
      IX= LD- M
      DO 9  IY= NXP, M
      IX= IX+1
      NYP= IX-3
      X( NYP)= X( IX)
      Y( NYP)= Y( IX)
      Z( NYP)= Z( IX)
      BI( NYP)= BI( IX)
      SALP( NYP)= SALP( IX)
      T1X( NYP)= T1X( IX)
      T1Y( NYP)= T1Y( IX)
      T1Z( NYP)= T1Z( IX)
      T2X( NYP)= T2X( IX)
      T2Y( NYP)= T2Y( IX)
    9 T2Z( NYP)= T2Z( IX)
   10 MI= LD+1- NX
      XS= X( MI)
      YS= Y( MI)
      ZS= Z( MI)
      XA= BI( MI)*.25
      XST= SQRT( XA)*.5
      S1X= T1X( MI)
      S1Y= T1Y( MI)
      S1Z= T1Z( MI)
      S2X= T2X( MI)
      S2Y= T2Y( MI)
      S2Z= T2Z( MI)
      SALN= SALP( MI)
      XT= XST
      YT= XST
      IF( NY.GT.0) GOTO 11
      MIA= MI
      GOTO 12
   11 M= M+1
      MP= MP+1
      MIA= LD+1- M
   12 DO 13  IX=1,4
      X( MIA)= XS+ XT* S1X+ YT* S2X
      Y( MIA)= YS+ XT* S1Y+ YT* S2Y
      Z( MIA)= ZS+ XT* S1Z+ YT* S2Z
      BI( MIA)= XA
      T1X( MIA)= S1X
      T1Y( MIA)= S1Y
      T1Z( MIA)= S1Z
      T2X( MIA)= S2X
      T2Y( MIA)= S2Y
      T2Z( MIA)= S2Z
      SALP( MIA)= SALN
      IF( IX.EQ.2) YT=- YT
      IF( IX.EQ.1.OR. IX.EQ.3) XT=- XT
      MIA= MIA-1
   13 CONTINUE
      M= M+3
      IF( NX.LE. MP) MP= MP+3
      IF( NY.GT.0) Z( MI)=10000.
C                                                                       
      RETURN
   14 FORMAT(' ERROR -- CORNERS OF QUADRILATERAL PATCH DO NOT LIE IN ',
     &'A PLANE')
      END
C ***
C     DOUBLE PRECISION 6/4/85
C
      SUBROUTINE PCINT( XI, YI, ZI, CABI, SABI, SALPI, E)
C ***
C     INTEGRATE OVER PATCHES AT WIRE CONNECTION POINT                   
      IMPLICIT REAL (A-H,O-Z)
      PARAMETER ( NM=600, N2M=800, N3M=1000)
      COMPLEX  EXK, EYK, EZK, EXS, EYS, EZS, EXC, EYC, EZC, E, E1, 
     &E2, E3, E4, E5, E6, E7, E8, E9
      COMMON  /DATAJ/ S, B, XJ, YJ, ZJ, CABJ, SABJ, SALPJ, EXK, EYK, 
     &EZK, EXS, EYS, EZS, EXC, EYC, EZC, RKH, IEXK, IND1, INDD1, IND2, 
     &INDD2, PGND
      DIMENSION  E(9)
      EQUIVALENCE(T1XJ,CABJ),(T1YJ,SABJ),(T1ZJ,SALPJ),(T2XJ,B),(T2YJ,
     &IND1),(T2ZJ,IND2)
      DATA   TPI/6.283185308D+0/, NINT/10/
      D= SQRT( S)*.5
      DS=4.* D/ DFLOAT( NINT)
      DA= DS* DS
      GCON=1./ S
      FCON=1./(2.* TPI* D)
      XXJ= XJ
      XYJ= YJ
      XZJ= ZJ
      XS= S
      S= DA
      S1= D+ DS*.5
      XSS= XJ+ S1*( T1XJ+ T2XJ)
      YSS= YJ+ S1*( T1YJ+ T2YJ)
      ZSS= ZJ+ S1*( T1ZJ+ T2ZJ)
      S1= S1+ D
      S2X= S1
      E1=(0.,0.)
      E2=(0.,0.)
      E3=(0.,0.)
      E4=(0.,0.)
      E5=(0.,0.)
      E6=(0.,0.)
      E7=(0.,0.)
      E8=(0.,0.)
      E9=(0.,0.)
      DO 1  I1=1, NINT
      S1= S1- DS
      S2= S2X
      XSS= XSS- DS* T1XJ
      YSS= YSS- DS* T1YJ
      ZSS= ZSS- DS* T1ZJ
      XJ= XSS
      YJ= YSS
      ZJ= ZSS
      DO 1  I2=1, NINT
      S2= S2- DS
      XJ= XJ- DS* T2XJ
      YJ= YJ- DS* T2YJ
      ZJ= ZJ- DS* T2ZJ
      CALL UNERE( XI, YI, ZI)
      EXK= EXK* CABI+ EYK* SABI+ EZK* SALPI
      EXS= EXS* CABI+ EYS* SABI+ EZS* SALPI
      G1=( D+ S1)*( D+ S2)* GCON
      G2=( D- S1)*( D+ S2)* GCON
      G3=( D- S1)*( D- S2)* GCON
      G4=( D+ S1)*( D- S2)* GCON
      F2=( S1* S1+ S2* S2)* TPI
      F1= S1/ F2-( G1- G2- G3+ G4)* FCON
      F2= S2/ F2-( G1+ G2- G3- G4)* FCON
      E1= E1+ EXK* G1
      E2= E2+ EXK* G2
      E3= E3+ EXK* G3
      E4= E4+ EXK* G4
      E5= E5+ EXS* G1
      E6= E6+ EXS* G2
      E7= E7+ EXS* G3
      E8= E8+ EXS* G4
    1 E9= E9+ EXK* F1+ EXS* F2
      E(1)= E1
      E(2)= E2
      E(3)= E3
      E(4)= E4
      E(5)= E5
      E(6)= E6
      E(7)= E7
      E(8)= E8
      E(9)= E9
      XJ= XXJ
      YJ= XYJ
      ZJ= XZJ
      S= XS
      RETURN
      END
C ***
C     DOUBLE PRECISION 6/4/85
C
      SUBROUTINE PRNT( IN1, IN2, IN3, FL1, FL2, FL3, FL4, FL5, FL6, IA,
     & ICHAR)
C ***
C                                                                       
C     PRNT SETS UP THE PRINT FORMATS FOR IMPEDANCE LOADING              
C                                                                       
      IMPLICIT REAL (A-H,O-Z)
      PARAMETER ( NM=600, N2M=800, N3M=1000)
      CHARACTER*6 IFORM, IVAR
      CHARACTER *(*) IA
      DIMENSION  IVAR(13), IA(1), IFORM(8), IN(3), INT(3), FL(6), FLT(6
     &)
      INTEGER  HALL
C                                                                       
C     NUMBER OF CHARACTERS PER COMPUTER WORD IS NCPW                    
C                                                                       
      DATA   IFORM/5H(/3X,,3HI5,,3H5X,,3HA5,,6HE13.4,,4H13X,,3H3X,,
     &4H5A4)/
      DATA   HALL/4H ALL/
      IN(1)= IN1
      IN(2)= IN2
      IN(3)= IN3
      FL(1)= FL1
      FL(2)= FL2
      FL(3)= FL3
      FL(4)= FL4
      FL(5)= FL5
C                                                                       
C     INTEGER FORMAT                                                    
C                                                                       
      FL(6)= FL6
      NINT=0
      IVAR(1)= IFORM(1)
      K=1
      I1=1
      IF(.NOT.( IN1.EQ.0.AND. IN2.EQ.0.AND. IN3.EQ.0)) GOTO 1
      INT(1)= HALL
      NINT=1
      I1=2
      K= K+1
      IVAR( K)= IFORM(4)
    1 DO 3  I= I1,3
      K= K+1
      IF( IN( I).EQ.0) GOTO 2
      NINT= NINT+1
      INT( NINT)= IN( I)
      IVAR( K)= IFORM(2)
      GOTO 3
    2 IVAR( K)= IFORM(3)
    3 CONTINUE
      K= K+1
C                                                                       
C     DFLOATING POINT FORMAT                                            
C                                                                       
      IVAR( K)= IFORM(7)
      NFLT=0
      DO 5  I=1,6
      K= K+1
      IF( ABS( FL( I)).LT.1.D-20) GOTO 4
      NFLT= NFLT+1
      FLT( NFLT)= FL( I)
      IVAR( K)= IFORM(5)
      GOTO 5
    4 IVAR( K)= IFORM(6)
    5 CONTINUE
      K= K+1
      IVAR( K)= IFORM(7)
      K= K+1
      IVAR( K)= IFORM(8)
      WRITE (2,IVAR) ( INT( I), I=1, NINT),( FLT( J), J=1, NFLT),
     *      ( IA( L), L=1, ICHAR)
      RETURN
      END
C ***
C     DOUBLE PRECISION 6/4/85
C
      SUBROUTINE QDSRC( IS, V, E)
C ***
C     FILL INCIDENT FIELD ARRAY FOR CHARGE DISCONTINUITY VOLTAGE SOURCE 
      IMPLICIT REAL (A-H,O-Z)
      PARAMETER ( NM=600, N2M=800, N3M=1000)
      COMPLEX  VQDS, CURD, CCJ, V, EXK, EYK, EZK, EXS, EYS, EZS, EXC
     &, EYC, EZC, ETK, ETS, ETC, VSANT, VQD, E, ZARRAY
      COMMON  /DATA/ LD, N1, N2, N, NP, M1, M2, M, MP, X( NM), Y( NM), 
     &Z( NM), SI( NM), BI( NM), ALP( NM), BET( NM), ICON1( N2M), ICON2(
     & N2M), ITAG( N2M), ICONX( NM), WLAM, IPSYM
      COMMON  /VSORC/ VQD(30), VSANT(30), VQDS(30), IVQD(30), ISANT(30)
     &, IQDS(30), NVQD, NSANT, NQDS
      COMMON  /SEGJ/ AX(30), BX(30), CX(30), JCO(30), JSNO, ISCON(50), 
     &NSCON, IPCON(10), NPCON
      COMMON  /DATAJ/ S, B, XJ, YJ, ZJ, CABJ, SABJ, SALPJ, EXK, EYK, 
     &EZK, EXS, EYS, EZS, EXC, EYC, EZC, RKH, IEXK, IND1, INDD1, IND2, 
     &INDD2, IPGND
      COMMON  /ANGL/ SALP( NM)
      COMMON  /ZLOAD/ ZARRAY( NM), NLOAD, NLODF
      DIMENSION  CCJX(2), E(1), CAB(1), SAB(1)
      DIMENSION  T1X(1), T1Y(1), T1Z(1), T2X(1), T2Y(1), T2Z(1)
      EQUIVALENCE(CCJ,CCJX),(CAB,ALP),(SAB,BET)
      EQUIVALENCE(T1X,SI),(T1Y,ALP),(T1Z,BET),(T2X,ICON1),(T2Y,ICON2),(
     &T2Z,ITAG)
      DATA   TP/6.283185308D+0/, CCJX/0.,-.01666666667D+0/
      I= ICON1( IS)
      ICON1( IS)=0
      CALL TBF( IS,0)
      ICON1( IS)= I
      S= SI( IS)*.5
      CURD= CCJ* V/(( LOG(2.* S/ BI( IS))-1.)*( BX( JSNO)* COS( TP* S)+
     & CX( JSNO)* SIN( TP* S))* WLAM)
      NQDS= NQDS+1
      VQDS( NQDS)= V
      IQDS( NQDS)= IS
      DO 20  JX=1, JSNO
      J= JCO( JX)
      S= SI( J)
      B= BI( J)
      XJ= X( J)
      YJ= Y( J)
      ZJ= Z( J)
      CABJ= CAB( J)
      SABJ= SAB( J)
      SALPJ= SALP( J)
      IF( IEXK.EQ.0) GOTO 16
      IPR= ICON1( J)
      IF( IPR) 1,6,2
    1 IPR=- IPR
      IF(- ICON1( IPR).NE. J) GOTO 7
      GOTO 4
    2 IF( IPR.NE. J) GOTO 3
      IF( CABJ* CABJ+ SABJ* SABJ.GT.1.D-8) GOTO 7
      GOTO 5
    3 IF( ICON2( IPR).NE. J) GOTO 7
    4 XI= ABS( CABJ* CAB( IPR)+ SABJ* SAB( IPR)+ SALPJ* SALP( IPR))
      IF( XI.LT.0.999999D+0) GOTO 7
      IF( ABS( BI( IPR)/ B-1.).GT.1.D-6) GOTO 7
    5 IND1=0
      GOTO 8
    6 IND1=1
      GOTO 8
    7 IND1=2
    8 IPR= ICON2( J)
      IF( IPR) 9,14,10
    9 IPR=- IPR
      IF(- ICON2( IPR).NE. J) GOTO 15
      GOTO 12
   10 IF( IPR.NE. J) GOTO 11
      IF( CABJ* CABJ+ SABJ* SABJ.GT.1.D-8) GOTO 15
      GOTO 13
   11 IF( ICON1( IPR).NE. J) GOTO 15
   12 XI= ABS( CABJ* CAB( IPR)+ SABJ* SAB( IPR)+ SALPJ* SALP( IPR))
      IF( XI.LT.0.999999D+0) GOTO 15
      IF( ABS( BI( IPR)/ B-1.).GT.1.D-6) GOTO 15
   13 IND2=0
      GOTO 16
   14 IND2=1
      GOTO 16
   15 IND2=2
   16 CONTINUE
      DO 17  I=1, N
      IJ= I- J
      XI= X( I)
      YI= Y( I)
      ZI= Z( I)
      AI= BI( I)
      CALL EFLD( XI, YI, ZI, AI, IJ)
      CABI= CAB( I)
      SABI= SAB( I)
      SALPI= SALP( I)
      ETK= EXK* CABI+ EYK* SABI+ EZK* SALPI
      ETS= EXS* CABI+ EYS* SABI+ EZS* SALPI
      ETC= EXC* CABI+ EYC* SABI+ EZC* SALPI
   17 E( I)= E( I)-( ETK* AX( JX)+ ETS* BX( JX)+ ETC* CX( JX))* CURD
      IF( M.EQ.0) GOTO 19
      IJ= LD+1
      I1= N
      DO 18  I=1, M
      IJ= IJ-1
      XI= X( IJ)
      YI= Y( IJ)
      ZI= Z( IJ)
      CALL HSFLD( XI, YI, ZI,0.)
      I1= I1+1
      TX= T2X( IJ)
      TY= T2Y( IJ)
      TZ= T2Z( IJ)
      ETK= EXK* TX+ EYK* TY+ EZK* TZ
      ETS= EXS* TX+ EYS* TY+ EZS* TZ
      ETC= EXC* TX+ EYC* TY+ EZC* TZ
      E( I1)= E( I1)+( ETK* AX( JX)+ ETS* BX( JX)+ ETC* CX( JX))* CURD*
     & SALP( IJ)
      I1= I1+1
      TX= T1X( IJ)
      TY= T1Y( IJ)
      TZ= T1Z( IJ)
      ETK= EXK* TX+ EYK* TY+ EZK* TZ
      ETS= EXS* TX+ EYS* TY+ EZS* TZ
      ETC= EXC* TX+ EYC* TY+ EZC* TZ
   18 E( I1)= E( I1)+( ETK* AX( JX)+ ETS* BX( JX)+ ETC* CX( JX))* CURD*
     & SALP( IJ)
   19 IF( NLOAD.GT.0.OR. NLODF.GT.0) E( J)= E( J)+ ZARRAY( J)* CURD*( 
     &AX( JX)+ CX( JX))
   20 CONTINUE
      RETURN
      END
C ***
C     DOUBLE PRECISION 6/4/85
C
      SUBROUTINE RDPAT
C ***
C     COMPUTE RADIATION PATTERN, GAIN, NORMALIZED GAIN                  
      IMPLICIT REAL (A-H,O-Z)
      PARAMETER ( NM=600, N2M=800, N3M=1000)
C     INTEGER HBLK,HCIR,HCLIF                                      
      CHARACTER*6 IGNTP, IGAX, IGTP, HPOL, HCIR, HCLIF, HBLK
      CHARACTER*6 ISENS
      integer*4 COM
      COMPLEX  ETH, EPH, ERD, ZRATI, ZRATI2, T1, FRATI
      COMMON  /DATA/ LD, N1, N2, N, NP, M1, M2, M, MP, X( NM), Y( NM), 
     &Z( NM), SI( NM), BI( NM), ALP( NM), BET( NM), ICON1( N2M), ICON2(
     & N2M), ITAG( N2M), ICONX( NM), WLAM, IPSYM
      COMMON  /SAVE/ IP( N2M), KCOM, COM(20,5), EPSR, SIG, SCRWLT, 
     &SCRWRT, FMHZ
      COMMON  /GND/ ZRATI, ZRATI2, FRATI, CL, CH, SCRWL, SCRWR, NRADL, 
     &KSYMP, IFAR, IPERF, T1, T2
      COMMON  /FPAT/ NTH, NPH, IPD, IAVP, INOR, IAX, THETS, PHIS, DTH, 
     &DPH, RFLD, GNOR, CLT, CHT, EPSR2, SIG2, IXTYP, XPR6, PINR, PNLR, 
     &PLOSS, NEAR, NFEH, NRX, NRY, NRZ, XNR, YNR, ZNR, DXNR, DYNR, DZNR
     &
C***
      COMMON  /SCRATM/ GAIN(2*N2M)
C***
      COMMON  /PLOT/ IPLP1, IPLP2, IPLP3, IPLP4
      DIMENSION  IGTP(4), IGAX(4), IGNTP(10), HPOL(3)
      DATA   HPOL/6HLINEAR,5HRIGHT,4HLEFT/, HBLK, HCIR/1H ,6HCIRCLE/
      DATA   IGTP/6H    - ,6HPOWER ,6H- DIRE,6HCTIVE /
      DATA   IGAX/6H MAJOR,6H MINOR,6H VERT.,6H HOR. /
      DATA   IGNTP/6H MAJOR,6H AXIS ,6H MINOR,6H AXIS ,6H   VER,
     &6HTICAL ,6H HORIZ,6HONTAL ,6H      ,6HTOTAL /
      DATA   PI, TA, TD/3.141592654D+0,1.745329252D-02,57.29577951D+0/
      DATA   NORMAX/1200/
      IF( IFAR.LT.2) GOTO 2
      WRITE (2,35) 
      IF( IFAR.LE.3) GOTO 1
      WRITE (2,36)  NRADL, SCRWLT, SCRWRT
      IF( IFAR.EQ.4) GOTO 2
    1 IF( IFAR.EQ.2.OR. IFAR.EQ.5) HCLIF= HPOL(1)
      IF( IFAR.EQ.3.OR. IFAR.EQ.6) HCLIF= HCIR
      CL= CLT/ WLAM
      CH= CHT/ WLAM
      ZRATI2= SQRT(1./ CMPLX( EPSR2,- SIG2* WLAM*59.96))
      WRITE (2,37)  HCLIF, CLT, CHT, EPSR2, SIG2
    2 IF( IFAR.NE.1) GOTO 3
      WRITE (2,41) 
      GOTO 5
    3 I=2* IPD+1
      J= I+1
      ITMP1=2* IAX+1
      ITMP2= ITMP1+1
      WRITE (2,38) 
      IF( RFLD.LT.1.D-20) GOTO 4
      EXRM=1./ RFLD
      EXRA= RFLD/ WLAM
      EXRA=-360.*( EXRA- AINT( EXRA))
      WRITE (2,39)  RFLD, EXRM, EXRA
    4 WRITE (2,40)  IGTP( I), IGTP( J), IGAX( ITMP1), IGAX( ITMP2)
    5 IF( IXTYP.EQ.0.OR. IXTYP.EQ.5) GOTO 7
      IF( IXTYP.EQ.4) GOTO 6
      PRAD=0.
      GCON=4.* PI/(1.+ XPR6* XPR6)
      GCOP= GCON
      GOTO 8
    6 PINR=394.51* XPR6* XPR6* WLAM* WLAM
    7 GCOP= WLAM* WLAM*2.* PI/(376.73* PINR)
      PRAD= PINR- PLOSS- PNLR
      GCON= GCOP
      IF( IPD.NE.0) GCON= GCON* PINR/ PRAD
    8 I=0
      GMAX=-1.E10
      PINT=0.
      TMP1= DPH* TA
      TMP2=.5* DTH* TA
      PHI= PHIS- DPH
      DO 29  KPH=1, NPH
      PHI= PHI+ DPH
      PHA= PHI* TA
      THET= THETS- DTH
      DO 29  KTH=1, NTH
      THET= THET+ DTH
      IF( KSYMP.EQ.2.AND. THET.GT.90.01.AND. IFAR.NE.1) GOTO 29
      THA= THET* TA
      IF( IFAR.EQ.1) GOTO 9
      CALL FFLD( THA, PHA, ETH, EPH)
      GOTO 10
    9 CALL GFLD( RFLD/ WLAM, PHA, THET/ WLAM, ETH, EPH, ERD, ZRATI, 
     &KSYMP)
      ERDM= ABS( ERD)
      ERDA= CANG( ERD)
   10 ETHM2= REAL( ETH* CONJG( ETH))
      ETHM= SQRT( ETHM2)
      ETHA= CANG( ETH)
      EPHM2= REAL( EPH* CONJG( EPH))
      EPHM= SQRT( EPHM2)
      EPHA= CANG( EPH)
C     ELLIPTICAL POLARIZATION CALC.                                     
      IF( IFAR.EQ.1) GOTO 28
      IF( ETHM2.GT.1.D-20.OR. EPHM2.GT.1.D-20) GOTO 11
      TILTA=0.
      EMAJR2=0.
      EMINR2=0.
      AXRAT=0.
      ISENS= HBLK
      GOTO 16
   11 DFAZ= EPHA- ETHA
      IF( EPHA.LT.0.) GOTO 12
      DFAZ2= DFAZ-360.
      GOTO 13
   12 DFAZ2= DFAZ+360.
   13 IF( ABS( DFAZ).GT. ABS( DFAZ2)) DFAZ= DFAZ2
      CDFAZ= COS( DFAZ* TA)
      TSTOR1= ETHM2- EPHM2
      TSTOR2=2.* EPHM* ETHM* CDFAZ
      TILTA=.5* ATGN2( TSTOR2, TSTOR1)
      STILTA= SIN( TILTA)
      TSTOR1= TSTOR1* STILTA* STILTA
      TSTOR2= TSTOR2* STILTA* COS( TILTA)
      EMAJR2=- TSTOR1+ TSTOR2+ ETHM2
      EMINR2= TSTOR1- TSTOR2+ EPHM2
      IF( EMINR2.LT.0.) EMINR2=0.
      AXRAT= SQRT( EMINR2/ EMAJR2)
      TILTA= TILTA* TD
      IF( AXRAT.GT.1.D-5) GOTO 14
      ISENS= HPOL(1)
      GOTO 16
   14 IF( DFAZ.GT.0.) GOTO 15
      ISENS= HPOL(2)
      GOTO 16
   15 ISENS= HPOL(3)
   16 GNMJ= DB10( GCON* EMAJR2)
      GNMN= DB10( GCON* EMINR2)
      GNV= DB10( GCON* ETHM2)
      GNH= DB10( GCON* EPHM2)
      GTOT= DB10( GCON*( ETHM2+ EPHM2))
      IF( INOR.LT.1) GOTO 23
      I= I+1
      IF( I.GT. NORMAX) GOTO 23
      GOTO (17,18,19,20,21), INOR
   17 TSTOR1= GNMJ
      GOTO 22
   18 TSTOR1= GNMN
      GOTO 22
   19 TSTOR1= GNV
      GOTO 22
   20 TSTOR1= GNH
      GOTO 22
   21 TSTOR1= GTOT
   22 GAIN( I)= TSTOR1
      IF( TSTOR1.GT. GMAX) GMAX= TSTOR1
   23 IF( IAVP.EQ.0) GOTO 24
      TSTOR1= GCOP*( ETHM2+ EPHM2)
      TMP3= THA- TMP2
      TMP4= THA+ TMP2
      IF( KTH.EQ.1) TMP3= THA
      IF( KTH.EQ. NTH) TMP4= THA
      DA= ABS( TMP1*( COS( TMP3)- COS( TMP4)))
      IF( KPH.EQ.1.OR. KPH.EQ. NPH) DA=.5* DA
      PINT= PINT+ TSTOR1* DA
      IF( IAVP.EQ.2) GOTO 29
   24 IF( IAX.EQ.1) GOTO 25
      TMP5= GNMJ
      TMP6= GNMN
      GOTO 26
   25 TMP5= GNV
      TMP6= GNH
   26 ETHM= ETHM* WLAM
      EPHM= EPHM* WLAM
      IF( RFLD.LT.1.D-20) GOTO 27
      ETHM= ETHM* EXRM
      ETHA= ETHA+ EXRA
      EPHM= EPHM* EXRM
      EPHA= EPHA+ EXRA
C      GO TO 29                                                         
C***
C28    WRITE(6,43)  RFLD,PHI,THET,ETHM,ETHA,EPHM,EPHA,ERDM,ERDA         
   27 WRITE (2,42)  THET, PHI, TMP5, TMP6, GTOT, AXRAT, TILTA, ISENS, 
     &ETHM, ETHA, EPHM, EPHA
      IF( IPLP1.NE.3) GOTO 299
      IF( IPLP3.EQ.0) GOTO 290
      IF( IPLP2.EQ.1.AND. IPLP3.EQ.1) WRITE( 8,*)  THET, ETHM, ETHA
      IF( IPLP2.EQ.1.AND. IPLP3.EQ.2) WRITE( 8,*)  THET, EPHM, EPHA
      IF( IPLP2.EQ.2.AND. IPLP3.EQ.1) WRITE( 8,*)  PHI, ETHM, ETHA
      IF( IPLP2.EQ.2.AND. IPLP3.EQ.2) WRITE( 8,*)  PHI, EPHM, EPHA
      IF( IPLP4.EQ.0) GOTO 299
  290 IF( IPLP2.EQ.1.AND. IPLP4.EQ.1) WRITE( 8,*)  THET, TMP5
      IF( IPLP2.EQ.1.AND. IPLP4.EQ.2) WRITE( 8,*)  THET, TMP6
      IF( IPLP2.EQ.1.AND. IPLP4.EQ.3) WRITE( 8,*)  THET, GTOT
      IF( IPLP2.EQ.2.AND. IPLP4.EQ.1) WRITE( 8,*)  PHI, TMP5
      IF( IPLP2.EQ.2.AND. IPLP4.EQ.2) WRITE( 8,*)  PHI, TMP6
      IF( IPLP2.EQ.2.AND. IPLP4.EQ.3) WRITE( 8,*)  PHI, GTOT
      GOTO 299
   28 WRITE (2,43)  RFLD, PHI, THET, ETHM, ETHA, EPHM, EPHA, ERDM, ERDA
     &
C***
  299 CONTINUE
   29 CONTINUE
      IF( IAVP.EQ.0) GOTO 30
      TMP3= THETS* TA
      TMP4= TMP3+ DTH* TA* DFLOAT( NTH-1)
      TMP3= ABS( DPH* TA* DFLOAT( NPH-1)*( COS( TMP3)- COS( TMP4)))
      PINT= PINT/ TMP3
      TMP3= TMP3/ PI
      WRITE (2,44)  PINT, TMP3
   30 IF( INOR.EQ.0) GOTO 34
      IF( ABS( GNOR).GT.1.D-20) GMAX= GNOR
      ITMP1=( INOR-1)*2+1
      ITMP2= ITMP1+1
      WRITE (2,45)  IGNTP( ITMP1), IGNTP( ITMP2), GMAX
      ITMP2= NPH* NTH
      IF( ITMP2.GT. NORMAX) ITMP2= NORMAX
      ITMP1=( ITMP2+2)/3
      ITMP2= ITMP1*3- ITMP2
      ITMP3= ITMP1
      ITMP4=2* ITMP1
      IF( ITMP2.EQ.2) ITMP4= ITMP4-1
      DO 31  I=1, ITMP1
      ITMP3= ITMP3+1
      ITMP4= ITMP4+1
      J=( I-1)/ NTH
      TMP1= THETS+ DFLOAT( I- J* NTH-1)* DTH
      TMP2= PHIS+ DFLOAT( J)* DPH
      J=( ITMP3-1)/ NTH
      TMP3= THETS+ DFLOAT( ITMP3- J* NTH-1)* DTH
      TMP4= PHIS+ DFLOAT( J)* DPH
      J=( ITMP4-1)/ NTH
      TMP5= THETS+ DFLOAT( ITMP4- J* NTH-1)* DTH
      TMP6= PHIS+ DFLOAT( J)* DPH
      TSTOR1= GAIN( I)- GMAX
      IF( I.EQ. ITMP1.AND. ITMP2.NE.0) GOTO 32
      TSTOR2= GAIN( ITMP3)- GMAX
      PINT= GAIN( ITMP4)- GMAX
   31 WRITE (2,46)  TMP1, TMP2, TSTOR1, TMP3, TMP4, TSTOR2, TMP5, TMP6,
     & PINT
      GOTO 34
   32 IF( ITMP2.EQ.2) GOTO 33
      TSTOR2= GAIN( ITMP3)- GMAX
      WRITE (2,46)  TMP1, TMP2, TSTOR1, TMP3, TMP4, TSTOR2
      GOTO 34
   33 WRITE (2,46)  TMP1, TMP2, TSTOR1
C                                                                       
   34 RETURN
   35 FORMAT(///,31X,'- - - FAR FIELD GROUND PARAMETERS - - -',//)
   36 FORMAT(40X,'RADIAL WIRE GROUND SCREEN',/,40X,I5,' WIRES',/,40X,
     &'WIRE LENGTH=',F8.2,' METERS',/,40X,'WIRE RADIUS=',1P,E10.3,
     &' METERS')
   37 FORMAT(40X,A6,' CLIFF',/,40X,'EDGE DISTANCE=',F9.2,' METERS',/,40
     &X,'HEIGHT=',F8.2,' METERS',/,40X,'SECOND MEDIUM -',/,40X,'RELA',
     &'TIVE DIELECTRIC CONST.=',F7.3,/,40X,'CONDUCTIVITY=',1P,E10.3,
     &' MHOS')
   38 FORMAT(///,48X,'- - - RADIATION PATTERNS - - -')
   39 FORMAT(54X,'RANGE=',1P,E13.6,' METERS',/,54X,'EXP(-JKR)/R=',E12.5
     &,' AT PHASE',0P,F7.2,' DEGREES',/)
   40 FORMAT(/,2X,'- - ANGLES - -',7X,2A6,'GAINS -',7X,'- - - POLARI',
     &'ZATION - - -',4X,'- - - E(THETA) - - -',4X,'- - - E(PHI) - -',
     &' -',/,2X,'THETA',5X,'PHI',7X,A6,2X,A6,3X,'TOTAL',6X,'AXIAL',5X,
     &'TILT',3X,'SENSE',2(5X,'MAGNITUDE',4X,'PHASE'),/,2(1X,'DEGREES',1
     &X),3(6X,'DB'),8X,'RATIO',5X,'DEG.',8X,2(6X,'VOLTS/M',4X,'DEGRE',
     &'ES'))
   41 FORMAT(///,28X,' - - - RADIATED FIELDS NEAR GROUND - - -',//,8X,
     &'- - - LOCATION - - -',10X,'- - E(THETA) - -',8X,'- - E(PHI) -',
     &' -',8X,'- - E(RADIAL) - -',/,7X,'RHO',6X,'PHI',9X,'Z',12X,'MAG',
     &6X,'PHASE',9X,'MAG',6X,'PHASE',9X,'MAG',6X,'PHASE',/,5X,'METERS',
     &3X,'DEGREES',4X,'METERS',8X,'VOLTS/M',3X,'DEGREES',6X,'VOLTS/M',3
     &X,'DEGREES',6X,'VOLTS/M',3X,'DEGREES',/)
   42 FORMAT(1X,F7.2,F9.2,3X,3F8.2,F11.5,F9.2,2X,A6,2(1P,E15.5,0P,F9.2)
     &)
   43 FORMAT(3X,F9.2,2X,F7.2,2X,F9.2,1X,3(3X,1P,E11.4,2X,0P,F7.2))
   44 FORMAT(//,3X,'AVERAGE POWER GAIN=',1P,E12.5,7X,'SOLID ANGLE U',
     &'SED IN AVERAGING=(',0P,F7.4,')*PI STERADIANS.',//)
   45 FORMAT(//,37X,'- - - - NORMALIZED GAIN - - - -',//,37X,2A6,'GAI',
     &'N',/,38X,'NORMALIZATION FACTOR =',F9.2,' DB',//,3(4X,
     &'- - ANGLES'' - -',6X,'GAIN',7X),/,3(4X,'THETA',5X,'PHI',8X,'DB',
     &8X),/,3(3X,'DEGREES',2X,'DEGREES',16X))
   46 FORMAT(3(1X,2F9.2,1X,F9.2,6X))
      END
C ***
C     DOUBLE PRECISION 6/4/85
C
      SUBROUTINE READGM( GM, I1, I2, X1, Y1, Z1, X2, Y2, Z2, RAD)
C ***
      IMPLICIT REAL (A-H,O-Z)
      PARAMETER ( NM=600, N2M=800, N3M=1000)
      INTEGER*4 NTOT
      INTEGER*4 NINT
      INTEGER*4 NFLT
      PARAMETER (NTOT=9, NINT=2, NFLT=7)
      INTEGER  IARR( NINT), BP( NTOT), EP( NTOT)
      DIMENSION  RARR( NFLT)
      CHARACTER   LINE*133, GM*2, BUFFER*132, BUFFER1*132
      READ (1, 10)  LINE
   10 FORMAT(A)

      NLIN= LEN(LINE)


      CALL STR0PC( LINE(1: NLIN), LINE(1: NLIN))
      IF( NLIN.LT.2) GOTO 110
      IF( NLIN.LE.132) GOTO 20
      NLIN=132
      LINE(133:133)=' '
   20 GM= LINE(1:2)
      NLIN= NLIN+1
      DO 30  I=1, NINT
   30 IARR( I)=0
      DO 40  I=1, NFLT
   40 RARR( I)=0.0
      IC=2
      IFOUND=0
      DO 70  I=1, NTOT
   50 IC= IC+1
      IF( IC.GE. NLIN) GOTO 80
      IF( LINE( IC: IC).EQ.' '.OR. LINE( IC: IC).EQ.',') GOTO 50
C BEGINNING OF I-TH NUMERICAL FIELD
      BP( I)= IC
   60 IC= IC+1
      IF( IC.GT. NLIN) GOTO 80
      IF( LINE( IC: IC).NE.' '.AND. LINE( IC: IC).NE.',') GOTO 60
C END OF I-TH NUMERICAL FIELD
      EP( I)= IC-1
      IFOUND= I
   70 CONTINUE
   80 CONTINUE
      DO 90  I=1, MIN( IFOUND, NINT)
      NLEN= EP( I)- BP( I)+1
      BUFFER= LINE( BP( I): EP( I))
      IND= INDEX( BUFFER(1: NLEN),'.')
      IF( IND.GT.0.AND. IND.LT. NLEN) GOTO 110
C USER PUT DECIMAL POINT FOR INTEGER
      IF( IND.EQ. NLEN) NLEN= NLEN-1
      READ( BUFFER(1: NLEN),111,ERR=110)  IARR( I)
111   format(i3)
   90 CONTINUE
      DO 100  I= NINT+1, IFOUND
      NLEN= EP( I)- BP( I)+1
      BUFFER= LINE( BP( I): EP( I))
      IND= INDEX( BUFFER(1: NLEN),'.')
C USER FORGOT DECIMAL POINT FOR REAL
      IF( IND.EQ.0) THEN
      IF( NLEN.GE.15) GOTO 110
      INDE= INDEX( BUFFER(1: NLEN),'E')
      NLEN= NLEN+1
      IF( INDE.EQ.0) THEN
      BUFFER( NLEN: NLEN)='.'
      ELSE
      BUFFER1= BUFFER(1: INDE-1)//'.'// BUFFER( INDE: NLEN-1)
      BUFFER= BUFFER1
      ENDIF
      ENDIF
      READ( BUFFER(1: NLEN),112,ERR=110)  RARR( I- NINT)
  112 format (F15.7)
  100 CONTINUE
      I1= IARR(1)
      I2= IARR(2)
      X1= RARR(1)
      Y1= RARR(2)
      Z1= RARR(3)
      X2= RARR(4)
      Y2= RARR(5)
      Z2= RARR(6)
      RAD= RARR(7)
      RETURN
  110 WRITE (2,*) ' GEOMETRY DATA CARD ERROR'
      WRITE (2,*)  LINE(1: MAX(1, NLIN-1))
      STOP
      END
C ***
C     DOUBLE PRECISION 6/4/85
C
      SUBROUTINE READMN( GM, I1, I2, I3, I4, F1, F2, F3, F4, F5, F6)
C ***
      IMPLICIT REAL (A-H,O-Z)
      PARAMETER ( NM=600, N2M=800, N3M=1000)
      INTEGER*4 NTOT
      INTEGER*4 NINT
      INTEGER*4 NFLT
      PARAMETER (NTOT=10, NINT=4, NFLT=6)
      INTEGER  IARR( NINT), BP( NTOT), EP( NTOT)
      DIMENSION  RARR( NFLT)
      CHARACTER   LINE*133, GM*2, BUFFER*132, BUFFER1*132
      READ (1,10)  LINE
   10 FORMAT(A)
      NLIN= LEN(LINE)
      CALL STR0PC( LINE(1: NLIN), LINE(1: NLIN))
      IF( NLIN.LT.2) GOTO 110
      IF( NLIN.LE.132) GOTO 20
      NLIN=132
      LINE(133:133)=' '
   20 GM= LINE(1:2)
      NLIN= NLIN+1
      DO 30  I=1, NINT
   30 IARR( I)=0
      DO 40  I=1, NFLT
   40 RARR( I)=0.0
      IC=2
      IFOUND=0
      DO 70  I=1, NTOT
   50 IC= IC+1
      IF( IC.GE. NLIN) GOTO 80
      IF( LINE( IC: IC).EQ.' '.OR. LINE( IC: IC).EQ.',') GOTO 50
C BEGINNING OF I-TH NUMERICAL FIELD
      BP( I)= IC
   60 IC= IC+1
      IF( IC.GT. NLIN) GOTO 80
      IF( LINE( IC: IC).NE.' '.AND. LINE( IC: IC).NE.',') GOTO 60
C END OF I-TH NUMERICAL FIELD
      EP( I)= IC-1
      IFOUND= I
   70 CONTINUE
   80 CONTINUE
      DO 90  I=1, MIN( IFOUND, NINT)
      NLEN= EP( I)- BP( I)+1
      BUFFER= LINE( BP( I): EP( I))
      IND= INDEX( BUFFER(1: NLEN),'.')
      IF( IND.GT.0.AND. IND.LT. NLEN) GOTO 110
C USER PUT DECIMAL POINT FOR INTEGER
      IF( IND.EQ. NLEN) NLEN= NLEN-1
      READ( BUFFER(1: NLEN),111,ERR=110)  IARR( I)
  111 format(I5)
   90 CONTINUE
      DO 100  I= NINT+1, IFOUND
      NLEN= EP( I)- BP( I)+1
      BUFFER= LINE( BP( I): EP( I))
      IND= INDEX( BUFFER(1: NLEN),'.')
C USER FORGOT DECIMAL POINT FOR REAL
      IF( IND.EQ.0) THEN
      IF( NLEN.GE.15) GOTO 110
      INDE= INDEX( BUFFER(1: NLEN),'E')
      NLEN= NLEN+1
      IF( INDE.EQ.0) THEN
      BUFFER( NLEN: NLEN)='.'
      ELSE
      BUFFER1= BUFFER(1: INDE-1)//'.'// BUFFER( INDE: NLEN-1)
      BUFFER= BUFFER1
      ENDIF
      ENDIF
      READ( BUFFER(1: NLEN),112,ERR=110)  RARR( I- NINT)
  112 format(F15.7)
  100 CONTINUE
      I1= IARR(1)
      I2= IARR(2)
      I3= IARR(3)
      I4= IARR(4)
      F1= RARR(1)
      F2= RARR(2)
      F3= RARR(3)
      F4= RARR(4)
      F5= RARR(5)
      F6= RARR(6)
      RETURN
  110 WRITE (2,*) '          FAULTY DATA CARD AFTER GEOMETRY SECTION'
      WRITE (2,*)  LINE(1: MAX(1, NLIN-1))
      STOP
      END
C ***
C     DOUBLE PRECISION 6/4/85
C
      SUBROUTINE REBLK( B, BX, NB, NBX, N2C)
C ***
C     REBLOCK ARRAY B IN N.G.F. SOLUTION FROM BLOCKS OF ROWS ON TAPE14  
C     TO BLOCKS OF COLUMNS ON TAPE16                                    
      IMPLICIT REAL (A-H,O-Z)
      PARAMETER ( NM=600, N2M=800, N3M=1000)
      COMPLEX  B, BX
      COMMON  /MATPAR/ ICASE, NBLOKS, NPBLK, NLAST, NBLSYM, NPSYM, 
     &NLSYM, IMAT, ICASX, NBBX, NPBX, NLBX, NBBL, NPBL, NLBL
      DIMENSION  B( NB,1), BX( NBX,1)
      REWIND 16
      NIB=0
      NPB= NPBL
      DO 3  IB=1, NBBL
      IF( IB.EQ. NBBL) NPB= NLBL
      REWIND 14
      NIX=0
      NPX= NPBX
      DO 2  IBX=1, NBBX
      IF( IBX.EQ. NBBX) NPX= NLBX
      READ( 14) (( BX( I, J), I=1, NPX), J=1, N2C)
      DO 1  I=1, NPX
      IX= I+ NIX
      DO 1  J=1, NPB
    1 B( IX, J)= BX( I, J+ NIB)
    2 NIX= NIX+ NPBX
      WRITE( 16) (( B( I, J), I=1, NB), J=1, NPB)
    3 NIB= NIB+ NPBL
      REWIND 14
      REWIND 16
      RETURN
      END
C ***
C     DOUBLE PRECISION 6/4/85
C
      SUBROUTINE REFLC( IX, IY, IZ, ITX, NOP)
C ***
C                                                                       
C     REFLC REFLECTS PARTIAL STRUCTURE ALONG X,Y, OR Z AXES OR ROTATES  
C     STRUCTURE TO COMPLETE A SYMMETRIC STRUCTURE.                      
C                                                                       
      IMPLICIT REAL (A-H,O-Z)
      PARAMETER ( NM=600, N2M=800, N3M=1000)
      COMMON  /DATA/ LD, N1, N2, N, NP, M1, M2, M, MP, X( NM), Y( NM), 
     &Z( NM), SI( NM), BI( NM), ALP( NM), BET( NM), ICON1( N2M), ICON2(
     & N2M), ITAG( N2M), ICONX( NM), WLAM, IPSYM
      COMMON  /ANGL/ SALP( NM)
      DIMENSION  T1X(1), T1Y(1), T1Z(1), T2X(1), T2Y(1), T2Z(1), X2(1),
     & Y2(1), Z2(1)
      EQUIVALENCE(T1X,SI),(T1Y,ALP),(T1Z,BET),(T2X,ICON1),(T2Y,ICON2),(
     &T2Z,ITAG),(X2,SI),(Y2,ALP),(Z2,BET)
      NP= N
      MP= M
      IPSYM=0
      ITI= ITX
      IF( IX.LT.0) GOTO 19
      IF( NOP.EQ.0) RETURN
      IPSYM=1
C                                                                       
C     REFLECT ALONG Z AXIS                                              
C                                                                       
      IF( IZ.EQ.0) GOTO 6
      IPSYM=2
      IF( N.LT. N2) GOTO 3
      DO 2  I= N2, N
      NX= I+ N- N1
      E1= Z( I)
      E2= Z2( I)
      IF( ABS( E1)+ ABS( E2).GT.1.D-5.AND. E1* E2.GE.-1.D-6) GOTO 1
      WRITE (2,24)  I
      STOP
    1 X( NX)= X( I)
      Y( NX)= Y( I)
      Z( NX)=- E1
      X2( NX)= X2( I)
      Y2( NX)= Y2( I)
      Z2( NX)=- E2
      ITAGI= ITAG( I)
      IF( ITAGI.EQ.0) ITAG( NX)=0
      IF( ITAGI.NE.0) ITAG( NX)= ITAGI+ ITI
    2 BI( NX)= BI( I)
      N= N*2- N1
      ITI= ITI*2
    3 IF( M.LT. M2) GOTO 6
      NXX= LD+1- M1
      DO 5  I= M2, M
      NXX= NXX-1
      NX= NXX- M+ M1
      IF( ABS( Z( NXX)).GT.1.D-10) GOTO 4
      WRITE (2,25)  I
      STOP
    4 X( NX)= X( NXX)
      Y( NX)= Y( NXX)
      Z( NX)=- Z( NXX)
      T1X( NX)= T1X( NXX)
      T1Y( NX)= T1Y( NXX)
      T1Z( NX)=- T1Z( NXX)
      T2X( NX)= T2X( NXX)
      T2Y( NX)= T2Y( NXX)
      T2Z( NX)=- T2Z( NXX)
      SALP( NX)=- SALP( NXX)
    5 BI( NX)= BI( NXX)
      M= M*2- M1
C                                                                       
C     REFLECT ALONG Y AXIS                                              
C                                                                       
    6 IF( IY.EQ.0) GOTO 12
      IF( N.LT. N2) GOTO 9
      DO 8  I= N2, N
      NX= I+ N- N1
      E1= Y( I)
      E2= Y2( I)
      IF( ABS( E1)+ ABS( E2).GT.1.D-5.AND. E1* E2.GE.-1.D-6) GOTO 7
      WRITE (2,24)  I
      STOP
    7 X( NX)= X( I)
      Y( NX)=- E1
      Z( NX)= Z( I)
      X2( NX)= X2( I)
      Y2( NX)=- E2
      Z2( NX)= Z2( I)
      ITAGI= ITAG( I)
      IF( ITAGI.EQ.0) ITAG( NX)=0
      IF( ITAGI.NE.0) ITAG( NX)= ITAGI+ ITI
    8 BI( NX)= BI( I)
      N= N*2- N1
      ITI= ITI*2
    9 IF( M.LT. M2) GOTO 12
      NXX= LD+1- M1
      DO 11  I= M2, M
      NXX= NXX-1
      NX= NXX- M+ M1
      IF( ABS( Y( NXX)).GT.1.D-10) GOTO 10
      WRITE (2,25)  I
      STOP
   10 X( NX)= X( NXX)
      Y( NX)=- Y( NXX)
      Z( NX)= Z( NXX)
      T1X( NX)= T1X( NXX)
      T1Y( NX)=- T1Y( NXX)
      T1Z( NX)= T1Z( NXX)
      T2X( NX)= T2X( NXX)
      T2Y( NX)=- T2Y( NXX)
      T2Z( NX)= T2Z( NXX)
      SALP( NX)=- SALP( NXX)
   11 BI( NX)= BI( NXX)
      M= M*2- M1
C                                                                       
C     REFLECT ALONG X AXIS                                              
C                                                                       
   12 IF( IX.EQ.0) GOTO 18
      IF( N.LT. N2) GOTO 15
      DO 14  I= N2, N
      NX= I+ N- N1
      E1= X( I)
      E2= X2( I)
      IF( ABS( E1)+ ABS( E2).GT.1.D-5.AND. E1* E2.GE.-1.D-6) GOTO 13
      WRITE (2,24)  I
      STOP
   13 X( NX)=- E1
      Y( NX)= Y( I)
      Z( NX)= Z( I)
      X2( NX)=- E2
      Y2( NX)= Y2( I)
      Z2( NX)= Z2( I)
      ITAGI= ITAG( I)
      IF( ITAGI.EQ.0) ITAG( NX)=0
      IF( ITAGI.NE.0) ITAG( NX)= ITAGI+ ITI
   14 BI( NX)= BI( I)
      N= N*2- N1
   15 IF( M.LT. M2) GOTO 18
      NXX= LD+1- M1
      DO 17  I= M2, M
      NXX= NXX-1
      NX= NXX- M+ M1
      IF( ABS( X( NXX)).GT.1.D-10) GOTO 16
      WRITE (2,25)  I
      STOP
   16 X( NX)=- X( NXX)
      Y( NX)= Y( NXX)
      Z( NX)= Z( NXX)
      T1X( NX)=- T1X( NXX)
      T1Y( NX)= T1Y( NXX)
      T1Z( NX)= T1Z( NXX)
      T2X( NX)=- T2X( NXX)
      T2Y( NX)= T2Y( NXX)
      T2Z( NX)= T2Z( NXX)
      SALP( NX)=- SALP( NXX)
   17 BI( NX)= BI( NXX)
      M= M*2- M1
C                                                                       
C     REPRODUCE STRUCTURE WITH ROTATION TO FORM CYLINDRICAL STRUCTURE   
C                                                                       
   18 RETURN
   19 FNOP= NOP
      IPSYM=-1
      SAM=6.283185308D+0/ FNOP
      CS= COS( SAM)
      SS= SIN( SAM)
      IF( N.LT. N2) GOTO 21
      N= N1+( N- N1)* NOP
      NX= NP+1
      DO 20  I= NX, N
      K= I- NP+ N1
      XK= X( K)
      YK= Y( K)
      X( I)= XK* CS- YK* SS
      Y( I)= XK* SS+ YK* CS
      Z( I)= Z( K)
      XK= X2( K)
      YK= Y2( K)
      X2( I)= XK* CS- YK* SS
      Y2( I)= XK* SS+ YK* CS
      Z2( I)= Z2( K)
      ITAGI= ITAG( K)
      IF( ITAGI.EQ.0) ITAG( I)=0
      IF( ITAGI.NE.0) ITAG( I)= ITAGI+ ITI
   20 BI( I)= BI( K)
   21 IF( M.LT. M2) GOTO 23
      M= M1+( M- M1)* NOP
      NX= MP+1
      K= LD+1- M1
      DO 22  I= NX, M
      K= K-1
      J= K- MP+ M1
      XK= X( K)
      YK= Y( K)
      X( J)= XK* CS- YK* SS
      Y( J)= XK* SS+ YK* CS
      Z( J)= Z( K)
      XK= T1X( K)
      YK= T1Y( K)
      T1X( J)= XK* CS- YK* SS
      T1Y( J)= XK* SS+ YK* CS
      T1Z( J)= T1Z( K)
      XK= T2X( K)
      YK= T2Y( K)
      T2X( J)= XK* CS- YK* SS
      T2Y( J)= XK* SS+ YK* CS
      T2Z( J)= T2Z( K)
      SALP( J)= SALP( K)
   22 BI( J)= BI( K)
C                                                                       
   23 RETURN
   24 FORMAT(' GEOMETRY DATA ERROR--SEGMENT,I5,26H LIES IN PLANE OF S',
     &'YMMETRY')
   25 FORMAT(' GEOMETRY DATA ERROR--PATCH,I4,26H LIES IN PLANE OF SYM',
     &'METRY')
      END
C ***
C     DOUBLE PRECISION 6/4/85
C
      SUBROUTINE ROM2( A, B, SUM, DMIN)
C ***
C                                                                       
C     FOR THE SOMMERFELD GROUND OPTION, ROM2 INTEGRATES OVER THE SOURCE 
C     SEGMENT TO OBTAIN THE TOTAL FIELD DUE TO GROUND.  THE METHOD OF   
C     VARIABLE INTERVAL WIDTH ROMBERG INTEGRATION IS USED.  THERE ARE 9 
C     FIELD COMPONENTS - THE X, Y, AND Z COMPONENTS DUE TO CONSTANT,    
C     SINE, AND COSINE CURRENT DISTRIBUTIONS.                           
C                                                                       
      IMPLICIT REAL (A-H,O-Z)
      COMPLEX  SUM, G1, G2, G3, G4, G5, T00, T01, T10, T02, T11, T20
     &
      DIMENSION  SUM(9), G1(9), G2(9), G3(9), G4(9), G5(9), T01(9), T10
     &(9), T20(9)
      DATA   NM, NTS, NX, N/65536,4,1,9/, RX/1.D-4/
      Z= A
      ZE= B
      S= B- A
      IF( S.GE.0.) GOTO 1
      WRITE (2,18) 
      STOP
    1 EP= S/(1.E4* NM)
      ZEND= ZE- EP
      DO 2  I=1, N
    2 SUM( I)=(0.,0.)
      NS= NX
      NT=0
      CALL SFLDS( Z, G1)
    3 DZ= S/ NS
      IF( Z+ DZ.LE. ZE) GOTO 4
      DZ= ZE- Z
      IF( DZ.LE. EP) GOTO 17
    4 DZOT= DZ*.5
      CALL SFLDS( Z+ DZOT, G3)
      CALL SFLDS( Z+ DZ, G5)
    5 TMAG1=0.
C                                                                       
C     EVALUATE 3 POINT ROMBERG RESULT AND TEST CONVERGENCE.             
C                                                                       
      TMAG2=0.
      DO 6  I=1, N
      T00=( G1( I)+ G5( I))* DZOT
      T01( I)=( T00+ DZ* G3( I))*.5
      T10( I)=(4.* T01( I)- T00)/3.
      IF( I.GT.3) GOTO 6
      TR= REAL( T01( I))
      TI= AIMAG( T01( I))
      TMAG1= TMAG1+ TR* TR+ TI* TI
      TR= REAL( T10( I))
      TI= AIMAG( T10( I))
      TMAG2= TMAG2+ TR* TR+ TI* TI
    6 CONTINUE
      TMAG1= SQRT( TMAG1)
      TMAG2= SQRT( TMAG2)
      CALL TEST( TMAG1, TMAG2, TR,0.,0., TI, DMIN)
      IF( TR.GT. RX) GOTO 8
      DO 7  I=1, N
    7 SUM( I)= SUM( I)+ T10( I)
      NT= NT+2
      GOTO 12
    8 CALL SFLDS( Z+ DZ*.25, G2)
      CALL SFLDS( Z+ DZ*.75, G4)
      TMAG1=0.
C                                                                       
C     EVALUATE 5 POINT ROMBERG RESULT AND TEST CONVERGENCE.             
C                                                                       
      TMAG2=0.
      DO 9  I=1, N
      T02=( T01( I)+ DZOT*( G2( I)+ G4( I)))*.5
      T11=(4.* T02- T01( I))/3.
      T20( I)=(16.* T11- T10( I))/15.
      IF( I.GT.3) GOTO 9
      TR= REAL( T11)
      TI= AIMAG( T11)
      TMAG1= TMAG1+ TR* TR+ TI* TI
      TR= REAL( T20( I))
      TI= AIMAG( T20( I))
      TMAG2= TMAG2+ TR* TR+ TI* TI
    9 CONTINUE
      TMAG1= SQRT( TMAG1)
      TMAG2= SQRT( TMAG2)
      CALL TEST( TMAG1, TMAG2, TR,0.,0., TI, DMIN)
      IF( TR.GT. RX) GOTO 14
   10 DO 11  I=1, N
   11 SUM( I)= SUM( I)+ T20( I)
      NT= NT+1
   12 Z= Z+ DZ
      IF( Z.GT. ZEND) GOTO 17
      DO 13  I=1, N
   13 G1( I)= G5( I)
      IF( NT.LT. NTS.OR. NS.LE. NX) GOTO 3
      NS= NS/2
      NT=1
      GOTO 3
   14 NT=0
      IF( NS.LT. NM) GOTO 15
      WRITE (2,19)  Z
      GOTO 10
   15 NS= NS*2
      DZ= S/ NS
      DZOT= DZ*.5
      DO 16  I=1, N
      G5( I)= G3( I)
   16 G3( I)= G2( I)
      GOTO 5
   17 CONTINUE
C                                                                       
      RETURN
   18 FORMAT(' ERROR - B LESS THAN A IN ROM2')
   19 FORMAT(' ROM2 -- STEP SIZE LIMITED AT Z =',1P,E12.5)
      END
C ***
C     DOUBLE PRECISION 6/4/85
C
      SUBROUTINE SBF( I, IS, AA, BB, CC)
C ***
C     COMPUTE COMPONENT OF BASIS FUNCTION I ON SEGMENT IS.              
      IMPLICIT REAL (A-H,O-Z)
      PARAMETER ( NM=600, N2M=800, N3M=1000)
      COMMON  /DATA/ LD, N1, N2, N, NP, M1, M2, M, MP, X( NM), Y( NM), 
     &Z( NM), SI( NM), BI( NM), ALP( NM), BET( NM), ICON1( N2M), ICON2(
     & N2M), ITAG( N2M), ICONX( NM), WLAM, IPSYM
      DATA   PI/3.141592654D+0/, JMAX/30/
      AA=0.
      BB=0.
      CC=0.
      JUNE=0
      JSNO=0
      PP=0.
      JCOX= ICON1( I)
      IF( JCOX.GT.10000) JCOX= I
      JEND=-1
      IEND=-1
      SIG=-1.
      IF( JCOX) 1,11,2
    1 JCOX=- JCOX
      GOTO 3
    2 SIG=- SIG
      JEND=- JEND
    3 JSNO= JSNO+1
      IF( JSNO.GE. JMAX) GOTO 24
      D= PI* SI( JCOX)
      SDH= SIN( D)
      CDH= COS( D)
      SD=2.* SDH* CDH
      IF( D.GT.0.015) GOTO 4
      OMC=4.* D* D
      OMC=((1.3888889D-3* OMC-4.1666666667D-2)* OMC+.5)* OMC
      GOTO 5
    4 OMC=1.- CDH* CDH+ SDH* SDH
    5 AJ=1./( LOG(1./( PI* BI( JCOX)))-.577215664D+0)
      PP= PP- OMC/ SD* AJ
      IF( JCOX.NE. IS) GOTO 6
      AA= AJ/ SD* SIG
      BB= AJ/(2.* CDH)
      CC=- AJ/(2.* SDH)* SIG
      JUNE= IEND
    6 IF( JCOX.EQ. I) GOTO 9
      IF( JEND.EQ.1) GOTO 7
      JCOX= ICON1( JCOX)
      GOTO 8
    7 JCOX= ICON2( JCOX)
    8 IF( IABS( JCOX).EQ. I) GOTO 10
      IF( JCOX) 1,24,2
    9 IF( JCOX.EQ. IS) BB=- BB
   10 IF( IEND.EQ.1) GOTO 12
   11 PM=- PP
      PP=0.
      NJUN1= JSNO
      JCOX= ICON2( I)
      IF( JCOX.GT.10000) JCOX= I
      JEND=1
      IEND=1
      SIG=-1.
      IF( JCOX) 1,12,2
   12 NJUN2= JSNO- NJUN1
      D= PI* SI( I)
      SDH= SIN( D)
      CDH= COS( D)
      SD=2.* SDH* CDH
      CD= CDH* CDH- SDH* SDH
      IF( D.GT.0.015) GOTO 13
      OMC=4.* D* D
      OMC=((1.3888889D-3* OMC-4.1666666667D-2)* OMC+.5)* OMC
      GOTO 14
   13 OMC=1.- CD
   14 AP=1./( LOG(1./( PI* BI( I)))-.577215664D+0)
      AJ= AP
      IF( NJUN1.EQ.0) GOTO 19
      IF( NJUN2.EQ.0) GOTO 21
      QP= SD*( PM* PP+ AJ* AP)+ CD*( PM* AP- PP* AJ)
      QM=( AP* OMC- PP* SD)/ QP
      QP=-( AJ* OMC+ PM* SD)/ QP
      IF( JUNE) 15,18,16
   15 AA= AA* QM
      BB= BB* QM
      CC= CC* QM
      GOTO 17
   16 AA=- AA* QP
      BB= BB* QP
      CC=- CC* QP
   17 IF( I.NE. IS) RETURN
   18 AA= AA-1.
      BB= BB+( AJ* QM+ AP* QP)* SDH/ SD
      CC= CC+( AJ* QM- AP* QP)* CDH/ SD
      RETURN
   19 IF( NJUN2.EQ.0) GOTO 23
      QP= PI* BI( I)
      XXI= QP* QP
      XXI= QP*(1.-.5* XXI)/(1.- XXI)
      QP=-( OMC+ XXI* SD)/( SD*( AP+ XXI* PP)+ CD*( XXI* AP- PP))
      IF( JUNE.NE.1) GOTO 20
      AA=- AA* QP
      BB= BB* QP
      CC=- CC* QP
      IF( I.NE. IS) RETURN
   20 AA= AA-1.
      D= CD- XXI* SD
      BB= BB+( SDH+ AP* QP*( CDH- XXI* SDH))/ D
      CC= CC+( CDH+ AP* QP*( SDH+ XXI* CDH))/ D
      RETURN
   21 QM= PI* BI( I)
      XXI= QM* QM
      XXI= QM*(1.-.5* XXI)/(1.- XXI)
      QM=( OMC+ XXI* SD)/( SD*( AJ- XXI* PM)+ CD*( PM+ XXI* AJ))
      IF( JUNE.NE.-1) GOTO 22
      AA= AA* QM
      BB= BB* QM
      CC= CC* QM
      IF( I.NE. IS) RETURN
   22 AA= AA-1.
      D= CD- XXI* SD
      BB= BB+( AJ* QM*( CDH- XXI* SDH)- SDH)/ D
      CC= CC+( CDH- AJ* QM*( SDH+ XXI* CDH))/ D
      RETURN
   23 AA=-1.
      QP= PI* BI( I)
      XXI= QP* QP
      XXI= QP*(1.-.5* XXI)/(1.- XXI)
      CC=1./( CDH- XXI* SDH)
      RETURN
   24 WRITE (2,25)  I
C                                                                       
      STOP
   25 FORMAT(' SBF - SEGMENT CONNECTION ERROR FOR SEGMENT',I5)
      END
C ***
C     DOUBLE PRECISION 6/4/85
C
      SUBROUTINE SFLDS( T, E)
C ***
C                                                                       
C     SFLDX RETURNS THE FIELD DUE TO GROUND FOR A CURRENT ELEMENT ON    
C     THE SOURCE SEGMENT AT T RELATIVE TO THE SEGMENT CENTER.           
C                                                                       
      IMPLICIT REAL (A-H,O-Z)
      PARAMETER ( NM=600, N2M=800, N3M=1000)
      COMPLEX  E, ERV, EZV, ERH, EZH, EPH, T1, EXK, EYK, EZK, EXS, 
     &EYS, EZS, EXC, EYC, EZC, XX1, XX2, U, U2, ZRATI, ZRATI2, FRATI, 
     &ER, ET, HRV, HZV, HRH
      COMMON  /DATAJ/ S, B, XJ, YJ, ZJ, CABJ, SABJ, SALPJ, EXK, EYK, 
     &EZK, EXS, EYS, EZS, EXC, EYC, EZC, RKH, IEXK, IND1, INDD1, IND2, 
     &INDD2, IPGND
      COMMON  /INCOM/ XO, YO, ZO, SN, XSN, YSN, ISNOR
      COMMON  /GWAV/ U, U2, XX1, XX2, R1, R2, ZMH, ZPH
      COMMON  /GND/ ZRATI, ZRATI2, FRATI, CL, CH, SCRWL, SCRWR, NRADL, 
     &KSYMP, IFAR, IPERF, T1, T2
      DIMENSION  E(9)
      DATA   PI/3.141592654D+0/, TP/6.283185308D+0/, POT/1.570796327D+0
     &/
      XT= XJ+ T* CABJ
      YT= YJ+ T* SABJ
      ZT= ZJ+ T* SALPJ
      RHX= XO- XT
      RHY= YO- YT
      RHS= RHX* RHX+ RHY* RHY
      RHO= SQRT( RHS)
      IF( RHO.GT.0.) GOTO 1
      RHX=1.
      RHY=0.
      PHX=0.
      PHY=1.
      GOTO 2
    1 RHX= RHX/ RHO
      RHY= RHY/ RHO
      PHX=- RHY
      PHY= RHX
    2 CPH= RHX* XSN+ RHY* YSN
      SPH= RHY* XSN- RHX* YSN
      IF( ABS( CPH).LT.1.D-10) CPH=0.
      IF( ABS( SPH).LT.1.D-10) SPH=0.
      ZPH= ZO+ ZT
      ZPHS= ZPH* ZPH
      R2S= RHS+ ZPHS
      R2= SQRT( R2S)
      RK= R2* TP
      XX2= CMPLX( COS( RK),- SIN( RK))
C                                                                       
C     USE NORTON APPROXIMATION FOR FIELD DUE TO GROUND.  CURRENT IS     
C     LUMPED AT SEGMENT CENTER WITH CURRENT MOMENT FOR CONSTANT, SINE,  
C     OR COSINE DISTRIBUTION.                                           
C                                                                       
      IF( ISNOR.EQ.1) GOTO 3
      ZMH=1.
      R1=1.
      XX1=0.
      CALL GWAVE( ERV, EZV, ERH, EZH, EPH)
      ET=-(0.,4.77134)* FRATI* XX2/( R2S* R2)
      ER=2.* ET* CMPLX(1.0, RK)
      ET= ET* CMPLX(1.0 - RK* RK, RK)
      HRV=( ER+ ET)* RHO* ZPH/ R2S
      HZV=( ZPHS* ER- RHS* ET)/ R2S
      HRH=( RHS* ER- ZPHS* ET)/ R2S
      ERV= ERV- HRV
      EZV= EZV- HZV
      ERH= ERH+ HRH
      EZH= EZH+ HRV
      EPH= EPH+ ET
      ERV= ERV* SALPJ
      EZV= EZV* SALPJ
      ERH= ERH* SN* CPH
      EZH= EZH* SN* CPH
      EPH= EPH* SN* SPH
      ERH= ERV+ ERH
      E(1)=( ERH* RHX+ EPH* PHX)* S
      E(2)=( ERH* RHY+ EPH* PHY)* S
      E(3)=( EZV+ EZH)* S
      E(4)=0.
      E(5)=0.
      E(6)=0.
      SFAC= PI* S
      SFAC= SIN( SFAC)/ SFAC
      E(7)= E(1)* SFAC
      E(8)= E(2)* SFAC
      E(9)= E(3)* SFAC
C                                                                       
C     INTERPOLATE IN SOMMERFELD FIELD TABLES                            
C                                                                       
      RETURN
    3 IF( RHO.LT.1.D-12) GOTO 4
      THET= ATAN( ZPH/ RHO)
      GOTO 5
    4 THET= POT
C     COMBINE VERTICAL AND HORIZONTAL COMPONENTS AND CONVERT TO X,Y,Z   
C     COMPONENTS.  MULTIPLY BY EXP(-JKR)/R.                             
    5 CALL INTRP( R2, THET, ERV, EZV, ERH, EPH)
      XX2= XX2/ R2
      SFAC= SN* CPH
      ERH= XX2*( SALPJ* ERV+ SFAC* ERH)
      EZH= XX2*( SALPJ* EZV- SFAC* ERV)
C     X,Y,Z FIELDS FOR CONSTANT CURRENT                                 
      EPH= SN* SPH* XX2* EPH
      E(1)= ERH* RHX+ EPH* PHX
      E(2)= ERH* RHY+ EPH* PHY
      E(3)= EZH
C     X,Y,Z FIELDS FOR SINE CURRENT                                     
      RK= TP* T
      SFAC= SIN( RK)
      E(4)= E(1)* SFAC
      E(5)= E(2)* SFAC
C     X,Y,Z FIELDS FOR COSINE CURRENT                                   
      E(6)= E(3)* SFAC
      SFAC= COS( RK)
      E(7)= E(1)* SFAC
      E(8)= E(2)* SFAC
      E(9)= E(3)* SFAC
      RETURN
      END
C ***
C     DOUBLE PRECISION 6/4/85
C
      SUBROUTINE SOLGF( A, B, C, D, XY, IP, NP, N1, N, MP, M1, M, N1C, 
     &N2C, N2CZ)
C ***
C     SOLVE FOR CURRENT IN N.G.F. PROCEDURE                             
      IMPLICIT REAL (A-H,O-Z)
      PARAMETER ( NM=600, N2M=800, N3M=1000)
      COMPLEX  A, B, C, D, SUM, XY, Y
      COMMON  /SCRATM/ Y( N2M)
      COMMON  /SEGJ/ AX(30), BX(30), CX(30), JCO(30), JSNO, ISCON(50), 
     &NSCON, IPCON(10), NPCON
      COMMON  /MATPAR/ ICASE, NBLOKS, NPBLK, NLAST, NBLSYM, NPSYM, 
     &NLSYM, IMAT, ICASX, NBBX, NPBX, NLBX, NBBL, NPBL, NLBL
      DIMENSION  A(1), B( N1C,1), C( N1C,1), D( N2CZ,1), IP(1), XY(1)
      IFL=14
      IF( ICASX.GT.0) IFL=13
C     NORMAL SOLUTION.  NOT N.G.F.                                      
      IF( N2C.GT.0) GOTO 1
      CALL SOLVES( A, IP, XY, N1C,1, NP, N, MP, M,13, IFL)
      GOTO 22
C     REORDER EXCITATION ARRAY                                          
    1 IF( N1.EQ. N.OR. M1.EQ.0) GOTO 5
      N2= N1+1
      JJ= N+1
      NPM= N+2* M1
      DO 2  I= N2, NPM
    2 Y( I)= XY( I)
      J= N1
      DO 3  I= JJ, NPM
      J= J+1
    3 XY( J)= Y( I)
      DO 4  I= N2, N
      J= J+1
    4 XY( J)= Y( I)
    5 NEQS= NSCON+2* NPCON
      IF( NEQS.EQ.0) GOTO 7
      NEQ= N1C+ N2C
C     COMPUTE INV(A)E1                                                  
      NEQS= NEQ- NEQS+1
      DO 6  I= NEQS, NEQ
    6 XY( I)=(0.,0.)
    7 CALL SOLVES( A, IP, XY, N1C,1, NP, N1, MP, M1,13, IFL)
      NI=0
C     COMPUTE E2-C(INV(A)E1)                                            
      NPB= NPBL
      DO 10  JJ=1, NBBL
      IF( JJ.EQ. NBBL) NPB= NLBL
      IF( ICASX.GT.1) READ( 15) (( C( I, J), I=1, N1C), J=1, NPB)
      II= N1C+ NI
      DO 9  I=1, NPB
      SUM=(0.,0.)
      DO 8  J=1, N1C
    8 SUM= SUM+ C( J, I)* XY( J)
      J= II+ I
    9 XY( J)= XY( J)- SUM
   10 NI= NI+ NPBL
      REWIND 15
C     COMPUTE INV(D)(E2-C(INV(A)E1)) = I2                               
      JJ= N1C+1
      IF( ICASX.GT.1) GOTO 11
      CALL SOLVE( N2C, D, IP( JJ), XY( JJ), N2C)
      GOTO 13
   11 IF( ICASX.EQ.4) GOTO 12
      NI= N2C* N2C
      READ( 11) ( B( J,1), J=1, NI)
      REWIND 11
      CALL SOLVE( N2C, B, IP( JJ), XY( JJ), N2C)
      GOTO 13
   12 NBLSYS= NBLSYM
      NPSYS= NPSYM
      NLSYS= NLSYM
      ICASS= ICASE
      NBLSYM= NBBL
      NPSYM= NPBL
      NLSYM= NLBL
      ICASE=3
      REWIND 11
      REWIND 16
      CALL LTSOLV( B, N2C, IP( JJ), XY( JJ), N2C,1,11,16)
      REWIND 11
      REWIND 16
      NBLSYM= NBLSYS
      NPSYM= NPSYS
      NLSYM= NLSYS
      ICASE= ICASS
   13 NI=0
C     COMPUTE INV(A)E1-(INV(A)B)I2 = I1                                 
      NPB= NPBL
      DO 16  JJ=1, NBBL
      IF( JJ.EQ. NBBL) NPB= NLBL
      IF( ICASX.GT.1) READ( 14) (( B( I, J), I=1, N1C), J=1, NPB)
      II= N1C+ NI
      DO 15  I=1, N1C
      SUM=(0.,0.)
      DO 14  J=1, NPB
      JP= II+ J
   14 SUM= SUM+ B( I, J)* XY( JP)
   15 XY( I)= XY( I)- SUM
   16 NI= NI+ NPBL
      REWIND 14
C     REORDER CURRENT ARRAY                                             
      IF( N1.EQ. N.OR. M1.EQ.0) GOTO 20
      DO 17  I= N2, NPM
   17 Y( I)= XY( I)
      JJ= N1C+1
      J= N1
      DO 18  I= JJ, NPM
      J= J+1
   18 XY( J)= Y( I)
      DO 19  I= N2, N1C
      J= J+1
   19 XY( J)= Y( I)
   20 IF( NSCON.EQ.0) GOTO 22
      J= NEQS-1
      DO 21  I=1, NSCON
      J= J+1
      JJ= ISCON( I)
   21 XY( JJ)= XY( J)
   22 RETURN
      END
C ***
C     DOUBLE PRECISION 6/4/85
C
      SUBROUTINE SOLVE( N, A, IP, B, NDIM)
C ***
C                                                                       
C     SUBROUTINE TO SOLVE THE MATRIX EQUATION LU*X=B WHERE L IS A UNIT  
C     LOWER TRIANGULAR MATRIX AND U IS AN UPPER TRIANGULAR MATRIX BOTH  
C     OF WHICH ARE STORED IN A.  THE RHS VECTOR B IS INPUT AND THE      
C     SOLUTION IS RETURNED THROUGH VECTOR B.    (MATRIX TRANSPOSED.     
C                                                                       
      IMPLICIT REAL (A-H,O-Z)
      PARAMETER ( NM=600, N2M=800, N3M=1000)
      COMPLEX  A, B, Y, SUM
      INTEGER  PI
      COMMON  /SCRATM/ Y( N2M)
C                                                                       
C     FORWARD SUBSTITUTION                                              
C                                                                       
      DIMENSION  A( NDIM, NDIM), IP( NDIM), B( NDIM)
      DO 3  I=1, N
      PI= IP( I)
      Y( I)= B( PI)
      B( PI)= B( I)
      IP1= I+1
      IF( IP1.GT. N) GOTO 2
      DO 1  J= IP1, N
      B( J)= B( J)- A( I, J)* Y( I)
    1 CONTINUE
    2 CONTINUE
C                                                                       
C     BACKWARD SUBSTITUTION                                             
C                                                                       
    3 CONTINUE
      DO 6  K=1, N
      I= N- K+1
      SUM=(0.,0.)
      IP1= I+1
      IF( IP1.GT. N) GOTO 5
      DO 4  J= IP1, N
      SUM= SUM+ A( J, I)* B( J)
    4 CONTINUE
    5 CONTINUE
      B( I)=( Y( I)- SUM)/ A( I, I)
    6 CONTINUE
      RETURN
      END
C ***
C     DOUBLE PRECISION 6/4/85
C
      SUBROUTINE SOLVES( A, IP, B, NEQ, NRH, NP, N, MP, M, IFL1, IFL2)
C ***
C                                                                       
C     SUBROUTINE SOLVES, FOR SYMMETRIC STRUCTURES, HANDLES THE          
C     TRANSFORMATION OF THE RIGHT HAND SIDE VECTOR AND SOLUTION OF THE  
C     MATRIX EQ.                                                        
C                                                                       
      IMPLICIT REAL (A-H,O-Z)
      PARAMETER ( NM=600, N2M=800, N3M=1000)
      COMPLEX  A, B, Y, SUM, SSX
      COMMON  /SMAT/ SSX(16,16)
      COMMON  /SCRATM/ Y( N2M)
      COMMON  /MATPAR/ ICASE, NBLOKS, NPBLK, NLAST, NBLSYM, NPSYM, 
     &NLSYM, IMAT, ICASX, NBBX, NPBX, NLBX, NBBL, NPBL, NLBL
      DIMENSION  A(1), IP(1), B( NEQ, NRH)
      NPEQ= NP+2* MP
      NOP= NEQ/ NPEQ
      FNOP= NOP
      FNORM=1./ FNOP
      NROW= NEQ
      IF( ICASE.GT.3) NROW= NPEQ
      IF( NOP.EQ.1) GOTO 11
      DO 10  IC=1, NRH
      IF( N.EQ.0.OR. M.EQ.0) GOTO 6
      DO 1  I=1, NEQ
    1 Y( I)= B( I, IC)
      KK=2* MP
      IA= NP
      IB= N
      J= NP
      DO 5  K=1, NOP
      IF( K.EQ.1) GOTO 3
      DO 2  I=1, NP
      IA= IA+1
      J= J+1
    2 B( J, IC)= Y( IA)
      IF( K.EQ. NOP) GOTO 5
    3 DO 4  I=1, KK
      IB= IB+1
      J= J+1
    4 B( J, IC)= Y( IB)
C                                                                       
C     TRANSFORM MATRIX EQ. RHS VECTOR ACCORDING TO SYMMETRY MODES       
C                                                                       
    5 CONTINUE
    6 DO 10  I=1, NPEQ
      DO 7  K=1, NOP
      IA= I+( K-1)* NPEQ
    7 Y( K)= B( IA, IC)
      SUM= Y(1)
      DO 8  K=2, NOP
    8 SUM= SUM+ Y( K)
      B( I, IC)= SUM* FNORM
      DO 10  K=2, NOP
      IA= I+( K-1)* NPEQ
      SUM= Y(1)
      DO 9  J=2, NOP
    9 SUM= SUM+ Y( J)* CONJG( SSX( K, J))
   10 B( IA, IC)= SUM* FNORM
   11 IF( ICASE.LT.3) GOTO 12
      REWIND IFL1
C                                                                       
C     SOLVE EACH MODE EQUATION                                          
C                                                                       
      REWIND IFL2
   12 DO 16  KK=1, NOP
      IA=( KK-1)* NPEQ+1
      IB= IA
      IF( ICASE.NE.4) GOTO 13
      I= NPEQ* NPEQ
      READ( IFL1) ( A( J), J=1, I)
      IB=1
   13 IF( ICASE.EQ.3.OR. ICASE.EQ.5) GOTO 15
      DO 14  IC=1, NRH
   14 CALL SOLVE( NPEQ, A( IB), IP( IA), B( IA, IC), NROW)
      GOTO 16
   15 CALL LTSOLV( A, NPEQ, IP( IA), B( IA,1), NEQ, NRH, IFL1, IFL2)
   16 CONTINUE
C                                                                       
C     INVERSE TRANSFORM THE MODE SOLUTIONS                              
C                                                                       
      IF( NOP.EQ.1) RETURN
      DO 26  IC=1, NRH
      DO 20  I=1, NPEQ
      DO 17  K=1, NOP
      IA= I+( K-1)* NPEQ
   17 Y( K)= B( IA, IC)
      SUM= Y(1)
      DO 18  K=2, NOP
   18 SUM= SUM+ Y( K)
      B( I, IC)= SUM
      DO 20  K=2, NOP
      IA= I+( K-1)* NPEQ
      SUM= Y(1)
      DO 19  J=2, NOP
   19 SUM= SUM+ Y( J)* SSX( K, J)
   20 B( IA, IC)= SUM
      IF( N.EQ.0.OR. M.EQ.0) GOTO 26
      DO 21  I=1, NEQ
   21 Y( I)= B( I, IC)
      KK=2* MP
      IA= NP
      IB= N
      J= NP
      DO 25  K=1, NOP
      IF( K.EQ.1) GOTO 23
      DO 22  I=1, NP
      IA= IA+1
      J= J+1
   22 B( IA, IC)= Y( J)
      IF( K.EQ. NOP) GOTO 25
   23 DO 24  I=1, KK
      IB= IB+1
      J= J+1
   24 B( IB, IC)= Y( J)
   25 CONTINUE
   26 CONTINUE
      RETURN
      END
C ***
C     DOUBLE PRECISION 6/4/85
C
      SUBROUTINE TBF( I, ICAP)
C ***
C     COMPUTE BASIS FUNCTION I                                          
      IMPLICIT REAL (A-H,O-Z)
      PARAMETER ( NM=600, N2M=800, N3M=1000)
      COMMON  /DATA/ LD, N1, N2, N, NP, M1, M2, M, MP, X( NM), Y( NM), 
     &Z( NM), SI( NM), BI( NM), ALP( NM), BET( NM), ICON1( N2M), ICON2(
     & N2M), ITAG( N2M), ICONX( NM), WLAM, IPSYM
      COMMON  /SEGJ/ AX(30), BX(30), CX(30), JCO(30), JSNO, ISCON(50), 
     &NSCON, IPCON(10), NPCON
      DATA   PI/3.141592654D+0/, JMAX/30/
      JSNO=0
      PP=0.
      JCOX= ICON1( I)
      IF( JCOX.GT.10000) JCOX= I
      JEND=-1
      IEND=-1
      SIG=-1.
      IF( JCOX) 1,10,2
    1 JCOX=- JCOX
      GOTO 3
    2 SIG=- SIG
      JEND=- JEND
    3 JSNO= JSNO+1
      IF( JSNO.GE. JMAX) GOTO 28
      JCO( JSNO)= JCOX
      D= PI* SI( JCOX)
      SDH= SIN( D)
      CDH= COS( D)
      SD=2.* SDH* CDH
      IF( D.GT.0.015) GOTO 4
      OMC=4.* D* D
      OMC=((1.3888889D-3* OMC-4.1666666667D-2)* OMC+.5)* OMC
      GOTO 5
    4 OMC=1.- CDH* CDH+ SDH* SDH
    5 AJ=1./( LOG(1./( PI* BI( JCOX)))-.577215664D+0)
      PP= PP- OMC/ SD* AJ
      AX( JSNO)= AJ/ SD* SIG
      BX( JSNO)= AJ/(2.* CDH)
      CX( JSNO)=- AJ/(2.* SDH)* SIG
      IF( JCOX.EQ. I) GOTO 8
      IF( JEND.EQ.1) GOTO 6
      JCOX= ICON1( JCOX)
      GOTO 7
    6 JCOX= ICON2( JCOX)
    7 IF( IABS( JCOX).EQ. I) GOTO 9
      IF( JCOX) 1,28,2
    8 BX( JSNO)=- BX( JSNO)
    9 IF( IEND.EQ.1) GOTO 11
   10 PM=- PP
      PP=0.
      NJUN1= JSNO
      JCOX= ICON2( I)
      IF( JCOX.GT.10000) JCOX= I
      JEND=1
      IEND=1
      SIG=-1.
      IF( JCOX) 1,11,2
   11 NJUN2= JSNO- NJUN1
      JSNOP= JSNO+1
      JCO( JSNOP)= I
      D= PI* SI( I)
      SDH= SIN( D)
      CDH= COS( D)
      SD=2.* SDH* CDH
      CD= CDH* CDH- SDH* SDH
      IF( D.GT.0.015) GOTO 12
      OMC=4.* D* D
      OMC=((1.3888889D-3* OMC-4.1666666667D-2)* OMC+.5)* OMC
      GOTO 13
   12 OMC=1.- CD
   13 AP=1./( LOG(1./( PI* BI( I)))-.577215664D+0)
      AJ= AP
      IF( NJUN1.EQ.0) GOTO 16
      IF( NJUN2.EQ.0) GOTO 20
      QP= SD*( PM* PP+ AJ* AP)+ CD*( PM* AP- PP* AJ)
      QM=( AP* OMC- PP* SD)/ QP
      QP=-( AJ* OMC+ PM* SD)/ QP
      BX( JSNOP)=( AJ* QM+ AP* QP)* SDH/ SD
      CX( JSNOP)=( AJ* QM- AP* QP)* CDH/ SD
      DO 14  IEND=1, NJUN1
      AX( IEND)= AX( IEND)* QM
      BX( IEND)= BX( IEND)* QM
   14 CX( IEND)= CX( IEND)* QM
      JEND= NJUN1+1
      DO 15  IEND= JEND, JSNO
      AX( IEND)=- AX( IEND)* QP
      BX( IEND)= BX( IEND)* QP
   15 CX( IEND)=- CX( IEND)* QP
      GOTO 27
   16 IF( NJUN2.EQ.0) GOTO 24
      IF( ICAP.NE.0) GOTO 17
      XXI=0.
      GOTO 18
   17 QP= PI* BI( I)
      XXI= QP* QP
      XXI= QP*(1.-.5* XXI)/(1.- XXI)
   18 QP=-( OMC+ XXI* SD)/( SD*( AP+ XXI* PP)+ CD*( XXI* AP- PP))
      D= CD- XXI* SD
      BX( JSNOP)=( SDH+ AP* QP*( CDH- XXI* SDH))/ D
      CX( JSNOP)=( CDH+ AP* QP*( SDH+ XXI* CDH))/ D
      DO 19  IEND=1, NJUN2
      AX( IEND)=- AX( IEND)* QP
      BX( IEND)= BX( IEND)* QP
   19 CX( IEND)=- CX( IEND)* QP
      GOTO 27
   20 IF( ICAP.NE.0) GOTO 21
      XXI=0.
      GOTO 22
   21 QM= PI* BI( I)
      XXI= QM* QM
      XXI= QM*(1.-.5* XXI)/(1.- XXI)
   22 QM=( OMC+ XXI* SD)/( SD*( AJ- XXI* PM)+ CD*( PM+ XXI* AJ))
      D= CD- XXI* SD
      BX( JSNOP)=( AJ* QM*( CDH- XXI* SDH)- SDH)/ D
      CX( JSNOP)=( CDH- AJ* QM*( SDH+ XXI* CDH))/ D
      DO 23  IEND=1, NJUN1
      AX( IEND)= AX( IEND)* QM
      BX( IEND)= BX( IEND)* QM
   23 CX( IEND)= CX( IEND)* QM
      GOTO 27
   24 BX( JSNOP)=0.
      IF( ICAP.NE.0) GOTO 25
      XXI=0.
      GOTO 26
   25 QP= PI* BI( I)
      XXI= QP* QP
      XXI= QP*(1.-.5* XXI)/(1.- XXI)
   26 CX( JSNOP)=1./( CDH- XXI* SDH)
   27 JSNO= JSNOP
      AX( JSNO)=-1.
      RETURN
   28 WRITE (2,29)  I
C                                                                       
      STOP
   29 FORMAT(' TBF - SEGMENT CONNECTION ERROR FOR SEGMENT',I5)
      END
C ***
C     DOUBLE PRECISION 6/4/85
C
      SUBROUTINE TEST( F1R, F2R, TR, F1I, F2I, TI, DMIN)
C ***
C                                                                       
C     TEST FOR CONVERGENCE IN NUMERICAL INTEGRATION                     
C                                                                       
      IMPLICIT REAL (A-H,O-Z)
      PARAMETER ( NM=600, N2M=800, N3M=1000)
      DEN= ABS( F2R)
      TR= ABS( F2I)
      IF( DEN.LT. TR) DEN= TR
      IF( DEN.LT. DMIN) DEN= DMIN
      IF( DEN.LT.1.D-37) GOTO 1
      TR= ABS(( F1R- F2R)/ DEN)
      TI= ABS(( F1I- F2I)/ DEN)
      RETURN
    1 TR=0.
      TI=0.
      RETURN
      END
C ***
C     DOUBLE PRECISION 6/4/85
C
      SUBROUTINE TRIO( J)
C ***
C     COMPUTE THE COMPONENTS OF ALL BASIS FUNCTIONS ON SEGMENT J        
      IMPLICIT REAL (A-H,O-Z)
      PARAMETER ( NM=600, N2M=800, N3M=1000)
      COMMON  /DATA/ LD, N1, N2, N, NP, M1, M2, M, MP, X( NM), Y( NM), 
     &Z( NM), SI( NM), BI( NM), ALP( NM), BET( NM), ICON1( N2M), ICON2(
     & N2M), ITAG( N2M), ICONX( NM), WLAM, IPSYM
      COMMON  /SEGJ/ AX(30), BX(30), CX(30), JCO(30), JSNO, ISCON(50), 
     &NSCON, IPCON(10), NPCON
      DATA   JMAX/30/
      JSNO=0
      JCOX= ICON1( J)
      IF( JCOX.GT.10000) GOTO 7
      JEND=-1
      IEND=-1
      IF( JCOX) 1,7,2
    1 JCOX=- JCOX
      GOTO 3
    2 JEND=- JEND
    3 IF( JCOX.EQ. J) GOTO 6
      JSNO= JSNO+1
      IF( JSNO.GE. JMAX) GOTO 9
      CALL SBF( JCOX, J, AX( JSNO), BX( JSNO), CX( JSNO))
      JCO( JSNO)= JCOX
      IF( JEND.EQ.1) GOTO 4
      JCOX= ICON1( JCOX)
      GOTO 5
    4 JCOX= ICON2( JCOX)
    5 IF( JCOX) 1,9,2
    6 IF( IEND.EQ.1) GOTO 8
    7 JCOX= ICON2( J)
      IF( JCOX.GT.10000) GOTO 8
      JEND=1
      IEND=1
      IF( JCOX) 1,8,2
    8 JSNO= JSNO+1
      CALL SBF( J, J, AX( JSNO), BX( JSNO), CX( JSNO))
      JCO( JSNO)= J
      RETURN
    9 WRITE (2,10)  J
C                                                                       
      STOP
   10 FORMAT(' TRIO - SEGMENT CONNENTION ERROR FOR SEGMENT',I5)
      END
C ***
C     DOUBLE PRECISION 6/4/85
C
      SUBROUTINE UNERE( XOB, YOB, ZOB)
C ***
C     CALCULATES THE ELECTRIC FIELD DUE TO UNIT CURRENT IN THE T1 AND T2
C     DIRECTIONS ON A PATCH                                             
      IMPLICIT REAL (A-H,O-Z)
      PARAMETER ( NM=600, N2M=800, N3M=1000)
      COMPLEX  EXK, EYK, EZK, EXS, EYS, EZS, EXC, EYC, EZC, ZRATI, 
     &ZRATI2, T1, ER, Q1, Q2, RRV, RRH, EDP, FRATI
      COMMON  /DATAJ/ S, B, XJ, YJ, ZJ, CABJ, SABJ, SALPJ, EXK, EYK, 
     &EZK, EXS, EYS, EZS, EXC, EYC, EZC, RKH, IEXK, IND1, INDD1, IND2, 
     &INDD2, IPGND
      COMMON  /GND/ ZRATI, ZRATI2, FRATI, CL, CH, SCRWL, SCRWR, NRADL, 
     &KSYMP, IFAR, IPERF, T1, T2
      EQUIVALENCE(T1XJ,CABJ),(T1YJ,SABJ),(T1ZJ,SALPJ),(T2XJ,B),(T2YJ,
     &IND1),(T2ZJ,IND2)
C     CONST=ETA/(8.*PI**2)                                              
      DATA   TPI, CONST/6.283185308D+0,4.771341188D+0/
      ZR= ZJ
      T1ZR= T1ZJ
      T2ZR= T2ZJ
      IF( IPGND.NE.2) GOTO 1
      ZR=- ZR
      T1ZR=- T1ZR
      T2ZR=- T2ZR
    1 RX= XOB- XJ
      RY= YOB- YJ
      RZ= ZOB- ZR
      R2= RX* RX+ RY* RY+ RZ* RZ
      IF( R2.GT.1.D-20) GOTO 2
      EXK=(0.,0.)
      EYK=(0.,0.)
      EZK=(0.,0.)
      EXS=(0.,0.)
      EYS=(0.,0.)
      EZS=(0.,0.)
      RETURN
    2 R= SQRT( R2)
      TT1=- TPI* R
      TT2= TT1* TT1
      RT= R2* R
      ER= CMPLX( SIN( TT1),- COS( TT1))*( CONST* S)
      Q1= CMPLX( TT2-1., TT1)* ER/ RT
      Q2= CMPLX(3.- TT2,-3.* TT1)* ER/( RT* R2)
      ER= Q2*( T1XJ* RX+ T1YJ* RY+ T1ZR* RZ)
      EXK= Q1* T1XJ+ ER* RX
      EYK= Q1* T1YJ+ ER* RY
      EZK= Q1* T1ZR+ ER* RZ
      ER= Q2*( T2XJ* RX+ T2YJ* RY+ T2ZR* RZ)
      EXS= Q1* T2XJ+ ER* RX
      EYS= Q1* T2YJ+ ER* RY
      EZS= Q1* T2ZR+ ER* RZ
      IF( IPGND.EQ.1) GOTO 6
      IF( IPERF.NE.1) GOTO 3
      EXK=- EXK
      EYK=- EYK
      EZK=- EZK
      EXS=- EXS
      EYS=- EYS
      EZS=- EZS
      GOTO 6
    3 XYMAG= SQRT( RX* RX+ RY* RY)
      IF( XYMAG.GT.1.D-6) GOTO 4
      PX=0.
      PY=0.
      CTH=1.
      RRV=(1.,0.)
      GOTO 5
    4 PX=- RY/ XYMAG
      PY= RX/ XYMAG
      CTH= RZ/ SQRT( XYMAG* XYMAG+ RZ* RZ)
      RRV= SQRT(1.- ZRATI* ZRATI*(1.- CTH* CTH))
    5 RRH= ZRATI* CTH
      RRH=( RRH- RRV)/( RRH+ RRV)
      RRV= ZRATI* RRV
      RRV=-( CTH- RRV)/( CTH+ RRV)
      EDP=( EXK* PX+ EYK* PY)*( RRH- RRV)
      EXK= EXK* RRV+ EDP* PX
      EYK= EYK* RRV+ EDP* PY
      EZK= EZK* RRV
      EDP=( EXS* PX+ EYS* PY)*( RRH- RRV)
      EXS= EXS* RRV+ EDP* PX
      EYS= EYS* RRV+ EDP* PY
      EZS= EZS* RRV
    6 RETURN
      END
C ***
C     DOUBLE PRECISION 6/4/85
C
      SUBROUTINE WIRE( XW1, YW1, ZW1, XW2, YW2, ZW2, RAD, RDEL, RRAD, 
     &NS, ITG)
C ***
C                                                                       
C     SUBROUTINE WIRE GENERATES SEGMENT GEOMETRY DATA FOR A STRAIGHT    
C     WIRE OF NS SEGMENTS.                                              
C                                                                       
      IMPLICIT REAL (A-H,O-Z)
      PARAMETER ( NM=600, N2M=800, N3M=1000)
      COMMON  /DATA/ LD, N1, N2, N, NP, M1, M2, M, MP, X( NM), Y( NM), 
     &Z( NM), SI( NM), BI( NM), ALP( NM), BET( NM), ICON1( N2M), ICON2(
     & N2M), ITAG( N2M), ICONX( NM), WLAM, IPSYM
      DIMENSION  X2(1), Y2(1), Z2(1)
      EQUIVALENCE(X2(1),SI(1)),(Y2(1),ALP(1)),(Z2(1),BET(1))
      IST= N+1
      N= N+ NS
      NP= N
      MP= M
      IPSYM=0
      IF( NS.LT.1) RETURN
      XD= XW2- XW1
      YD= YW2- YW1
      ZD= ZW2- ZW1
      IF( ABS( RDEL-1.).LT.1.D-6) GOTO 1
      DELZ= SQRT( XD* XD+ YD* YD+ ZD* ZD)
      XD= XD/ DELZ
      YD= YD/ DELZ
      ZD= ZD/ DELZ
      DELZ= DELZ*(1.- RDEL)/(1.- RDEL** NS)
      RD= RDEL
      GOTO 2
    1 FNS= NS
      XD= XD/ FNS
      YD= YD/ FNS
      ZD= ZD/ FNS
      DELZ=1.
      RD=1.
    2 RADZ= RAD
      XS1= XW1
      YS1= YW1
      ZS1= ZW1
      DO 3  I= IST, N
      ITAG( I)= ITG
      XS2= XS1+ XD* DELZ
      YS2= YS1+ YD* DELZ
      ZS2= ZS1+ ZD* DELZ
      X( I)= XS1
      Y( I)= YS1
      Z( I)= ZS1
      X2( I)= XS2
      Y2( I)= YS2
      Z2( I)= ZS2
      BI( I)= RADZ
      DELZ= DELZ* RD
      RADZ= RADZ* RRAD
      XS1= XS2
      YS1= YS2
    3 ZS1= ZS2
      X2( N)= XW2
      Y2( N)= YW2
      Z2( N)= ZW2
      RETURN
      END
C ***
C     DOUBLE PRECISION 6/4/85
C
      COMPLEX FUNCTION ZINT( SIGL, ROLAM)
C ***
C                                                                       
C     ZINT COMPUTES THE INTERNAL IMPEDANCE OF A CIRCULAR WIRE           
C                                                                       
C                                                                       
      IMPLICIT REAL (A-H,O-Z)
      COMPLEX  TH, PH, F, G, FJ, CN, BR1, BR2
      COMPLEX  CC1, CC2, CC3, CC4, CC5, CC6, CC7, CC8, CC9, CC10, 
     &CC11, CC12, CC13, CC14
      DIMENSION  FJX(2), CNX(2), CCN(28)
      EQUIVALENCE(FJ,FJX),(CN,CNX),(CC1,CCN(1)),(CC2,CCN(3)),(CC3,CCN(5
     &)),(CC4,CCN(7)),(CC5,CCN(9)),(CC6,CCN(11)),(CC7,CCN(13)),(CC8,CCN
     &(15)),(CC9,CCN(17)),(CC10,CCN(19)),(CC11,CCN(21)),(CC12,CCN(23)),
     &(CC13,CCN(25)),(CC14,CCN(27))
      DATA   PI, POT, TP, TPCMU/3.1415926D+0,1.5707963D+0,6.2831853D+0,
     &2.368705D+3/
      DATA   CMOTP/60.00/, FJX/0.,1./, CNX/.70710678D+0,.70710678D+0/
      DATA   CCN/6.D-7,1.9D-6,-3.4D-6,5.1D-6,-2.52D-5,0.,-9.06D-5,-
     &9.01D-5,0.,-9.765D-4,.0110486D+0,-.0110485D+0,0.,-.3926991D+0,
     &1.6D-6,-3.2D-6,1.17D-5,-2.4D-6,3.46D-5,3.38D-5,5.D-7,2.452D-4,-
     &1.3813D-3,1.3811D-3,-6.25001D-2,-1.D-7,.7071068D+0,.7071068D+0/
      TH( D)=((((( CC1* D+ CC2)* D+ CC3)* D+ CC4)* D+ CC5)* D+ CC6)* D+
     & CC7
      PH( D)=((((( CC8* D+ CC9)* D+ CC10)* D+ CC11)* D+ CC12)* D+ CC13)
     &* D+ CC14
      F( D)= SQRT( POT/ D)* EXP(- CN* D+ TH(-8./ X))
      G( D)= EXP( CN* D+ TH(8./ X))/ SQRT( TP* D)
      X= SQRT( TPCMU* SIGL)* ROLAM
      IF( X.GT.110.) GOTO 2
      IF( X.GT.8.) GOTO 1
      Y= X/8.
      Y= Y* Y
      S= Y* Y
      BER=((((((-9.01D-6* S+1.22552D-3)* S-.08349609D+0)* S+
     &2.6419140D+0)* S-32.363456D+0)* S+113.77778D+0)* S-64.)* S+1.
      BEI=((((((1.1346D-4* S-.01103667D+0)* S+.52185615D+0)* S-
     &10.567658D+0)* S+72.817777D+0)* S-113.77778D+0)* S+16.)* Y
      BR1= CMPLX( BER, BEI)
      BER=(((((((-3.94D-6* S+4.5957D-4)* S-.02609253D+0)* S+
     &.66047849D+0)* S-6.0681481D+0)* S+14.222222D+0)* S-4.)* Y)* X
      BEI=((((((4.609D-5* S-3.79386D-3)* S+.14677204D+0)* S-
     &2.3116751D+0)* S+11.377778D+0)* S-10.666667D+0)* S+.5)* X
      BR2= CMPLX( BER, BEI)
      BR1= BR1/ BR2
      GOTO 3
    1 BR2= FJ* F( X)/ PI
      BR1= G( X)+ BR2
      BR2= G( X)* PH(8./ X)- BR2* PH(-8./ X)
      BR1= BR1/ BR2
      GOTO 3
    2 BR1= CMPLX(.70710678D+0,-.70710678D+0)
    3 ZINT= FJ* SQRT( CMOTP/ SIGL)* BR1/ ROLAM
      RETURN
      END

      SUBROUTINE STR0PC( STRING, STRING1)
      CHARACTER *(*)  STRING, STRING1
      INTEGER*4  I, J, IC
      INTEGER IS_PC

      IS_PC = 0

      DO 150, I=1, LEN( STRING)
      IC= ICHAR( STRING( I: I))

      if (is_pc .ne. 0) then
         IF( IC.GE.97.AND. IC.LE.122) IC= IC-32
      endif

      STRING1( I: I)= CHAR( IC)
  150 CONTINUE

      RETURN
      END

