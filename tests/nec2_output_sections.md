# NEC2 Fortran Output Sections (tests/nec2-1.2.1.2.f)
This document lists every Fortran `WRITE (2,...)` output statement in `tests/nec2-1.2.1.2.f`, grouped by the output section or routine that produces it.

> Note: In Fortran, numeric format labels are local to each program unit. The format definition shown for each `WRITE (2,...)` uses the routine-local label in the same subroutine or function as the write statement.

## Main program input/output and startup diagnostics
- Routine: `PROGRAM MAIN`
- Source lines: 1-1067

### `WRITE (2,126)` (label 126)
  - line 140; condition: `none`; statement: `          WRITE (2,126)`
  - format:
```fortran
  126 FORMAT('1')
```

### `WRITE (2,127)` (label 127)
  - line 141; condition: `none`; statement: `          WRITE (2,127)`
  - format:
```fortran
  127 FORMAT(///,33X,'************************************',//,36X,
     &'NUMERICAL ELECTROMAGNETICS CODE',//,33X,
     &'************************************')
```

### `WRITE (2,128)` (label 128)
  - line 142; condition: `none`; statement: `          WRITE (2,128)`
  - format:
```fortran
  128 FORMAT(////,37X,'- - - - COMMENTS - - - -',//)
```

### `WRITE (2,129)` (label 129)
  - line 145; condition: `none`; statement: `      WRITE (2,129) ( COM( I, KCOM), I=1,19)`
  - format:
```fortran
  129 FORMAT(' ', 20A4)
```

### `WRITE (2,130)` (label 130)
  - line 150; condition: `none`; statement: `          WRITE (2,130)`
  - format:
```fortran
  130 FORMAT(///,10X,'INCORRECT LABEL FOR A COMMENT CARD')
```

### `WRITE (2,135)` (label 135)
- Occurrences: 5
  - line 185; condition: `none`; statement: `      WRITE (2,135)`
  - line 851; condition: `none`; statement: `      WRITE (2,135)`
  - line 861; condition: `none`; statement: `      WRITE (2,135)`
  - line 909; condition: `none`; statement: `      WRITE (2,135)`
  - line 935; condition: `none`; statement: `      WRITE (2,135)`
  - format:
```fortran
  135 FORMAT(/////)
```

### `WRITE (2,137)` (label 137)
  - line 225; condition: `none`; statement: `      WRITE (2,137)  MPCNT, AIN, ITMP1, ITMP2, ITMP3, ITMP4, TMP1, TMP2`
  - format:
```fortran
  137 FORMAT(1X,'***** DATA CARD NO.',I3,3X,A2,1X,I3,3(1X,I5),6(1X,1P,E
     &12.5))
```

### `WRITE (2,138)` (label 138)
  - line 253; condition: `none`; statement: `   15 WRITE (2,138)`
  - format:
```fortran
  138 FORMAT(///,10X,'FAULTY DATA CARD LABEL AFTER GEOMETRY SECTION')
```

### `WRITE (2,139)` (label 139)
  - line 314; condition: `none`; statement: `      WRITE (2,139)`
  - format:
```fortran
  139 FORMAT(///,10X,'NUMBER OF LOADING CARDS EXCEEDS STORAGE ALLOTTED'
     &)
```

### `WRITE (2,140)` (label 140)
  - line 322; condition: `none`; statement: `      WRITE (2,140)  NLOAD, ITMP3, ITMP4`
  - format:
```fortran
  140 FORMAT(///,10X,'DATA FAULT ON LOADING CARD NO.=',I5,5X,'ITAG S',
     &'TEP1=',I5,'  IS GREATER THAN ITAG STEP2=',I5)
```

### `WRITE (2,141)` (label 141)
  - line 380; condition: `none`; statement: `  206 WRITE (2,141)`
  - format:
```fortran
  141 FORMAT(///,10X,'NUMBER OF EXCITATION CARDS EXCEEDS STORAGE ALLO',
     &'TTED')
```

### `WRITE (2,142)` (label 142)
  - line 415; condition: `none`; statement: `      WRITE (2,142)`
  - format:
```fortran
  142 FORMAT(///,10X,'NUMBER OF NETWORK CARDS EXCEEDS STORAGE ALLOTTED'
     &)
```

### `WRITE (2,143)` (label 143)
  - line 467; condition: `none`; statement: `      WRITE (2,143)`
  - format:
```fortran
  143 FORMAT(///,10X,'WHEN MULTIPLE FREQUENCIES ARE REQUESTED, ONLY ONE
     & NEAR FIELD CARD CAN BE USED -',/,10X,'LAST CARD READ IS USED')
```

### `WRITE (2,145)` (label 145)
  - line 594; condition: `none`; statement: `      WRITE (2,145)  FMHZ, WLAM`
  - format:
```fortran
  145 FORMAT(////,33X,'- - - - - - FREQUENCY - - - - - -',//,36X,'FR',
     &'EQUENCY=',1P,E11.4,' MHZ',/,36X,'WAVELENGTH=',E11.4,' METERS')
```

### `WRITE (2,146)` (label 146)
  - line 621; condition: `none`; statement: `   46 WRITE (2,146)`
  - format:
```fortran
  146 FORMAT(///,30X,' - - - STRUCTURE IMPEDANCE LOADING - - -')
```

### `WRITE (2,147)` (label 147)
  - line 624; condition: `NLOAD.EQ.0.AND. NLODF.EQ.0`; statement: `      IF( NLOAD.EQ.0.AND. NLODF.EQ.0) WRITE (2,147)`
  - format:
```fortran
  147 FORMAT(/,35X,'THIS STRUCTURE IS NOT LOADED')
```

### `WRITE (2,148)` (label 148)
  - line 627; condition: `none`; statement: `      WRITE (2,148)`
  - format:
```fortran
  148 FORMAT(///,34X,'- - - ANTENNA ENVIRONMENT - - -',/)
```

### `WRITE (2,149)` (label 149)
  - line 642; condition: `none`; statement: `      WRITE (2,149)`
  - format:
```fortran
  149 FORMAT(40X,'MEDIUM UNDER SCREEN -')
```

### `WRITE (2,150)` (label 150)
  - line 653; condition: `none`; statement: `  329 WRITE (2,150)  EPSR, SIG, EPSC`
  - format:
```fortran
  150 FORMAT(40X,'RELATIVE DIELECTRIC CONST.=',F7.3,/,40X,'CONDUCTIV',
     &'ITY=',1P,E10.3,' MHOS/METER',/,40X,
     &'COMPLEX DIELECTRIC CONSTANT=',2E12.5)
```

### `WRITE (2,151)` (label 151)
  - line 655; condition: `none`; statement: `   48 WRITE (2,151)`
  - format:
```fortran
  151 FORMAT(42X,'PERFECT GROUND')
```

### `WRITE (2,152)` (label 152)
  - line 657; condition: `none`; statement: `   49 WRITE (2,152)`
  - format:
```fortran
  152 FORMAT(44X,'FREE SPACE')
```

### `WRITE (2,153)` (label 153)
  - line 683; condition: `none`; statement: `      WRITE (2,153)  TIM, TIM2`
  - format:
```fortran
  153 FORMAT(///,32X,'- - - MATRIX TIMING - - -',//,24X,'FILL=',F9.3,
     &' SEC.,  FACTOR=',F9.3,' SEC.')
```

### `WRITE (2,154)` (label 154)
  - line 698; condition: `IPTFLG.LE.0.OR. IXTYP.EQ.4`; statement: `      IF( IPTFLG.LE.0.OR. IXTYP.EQ.4) WRITE (2,154)`
  - format:
```fortran
  154 FORMAT(///,40X,'- - - EXCITATION - - -')
```

### `WRITE (2,155)` (label 155)
  - line 712; condition: `IPTFLG.LE.0`; statement: `      IF( IPTFLG.LE.0) WRITE (2,155)  XPR1, XPR2, XPR3, HPOL( IXTYP),`
  - format:
```fortran
  155 FORMAT(/,4X,'PLANE WAVE',4X,'THETA=',F7.2,' DEG,  PHI=',F7.2,
     &' DEG,  ETA=',F7.2,' DEG,  TYPE -',A6,'=  AXIAL RATIO=',F6.3)
```

### `WRITE (2,156)` (label 156)
  - line 706; condition: `none`; statement: `      WRITE (2,156)  XPR1, XPR2, XPR3, XPR4, XPR5, XPR6`
  - format:
```fortran
  156 FORMAT(/,31X,'POSITION (METERS)',14X,'ORIENTATION (DEG)=/',28X,
     &'X',12X,'Y',12X,'Z',10X,'ALPHA',5X,'BETA',4X,'DIPOLE MOMENT',//,4
     &X,'CURRENT SOURCE',1X,3(3X,F10.5),1X,2(3X,F7.2),4X,F8.3)
```

### `WRITE (2,157)` (label 157)
  - line 736; condition: `none`; statement: `      WRITE (2,157)  ITAG( ITMP4), ITMP4, ITAG( ITMP5), ITMP5, X11R( J)`
  - format:
```fortran
  157 FORMAT(4X,4(I5,1X),1P,6(3X,E11.4),3X,A6,A2)
```

### `WRITE (2,158)` (label 158)
  - line 719; condition: `none`; statement: `      WRITE (2,158)`
  - format:
```fortran
  158 FORMAT(///,44X,'- - - NETWORK DATA - - -')
```

### `WRITE (2,159)` (label 159)
  - line 724; condition: `ITMP1.EQ.2`; statement: `      IF( ITMP1.EQ.2) WRITE (2,159)`
  - format:
```fortran
  159 FORMAT(/,6X,'- FROM -    - TO -',11X,'TRANSMISSION LINE',15X,
     &'-  -  SHUNT ADMITTANCES (MHOS)  -  -',14X,'LINE',/,6X,
     &'TAG  SEG.','   TAG  SEG.',6X,'IMPEDANCE',6X,'LENGTH',12X,
     &'- END ONE -',17X,'- END TWO -',12X,'TYPE',/,6X,
     &'NO.   NO.   NO.   NO.',9X,'OHM''S',8X,'METERS',9X,'REAL',10X,
     &'IMAG.',9X,'REAL',10X,'IMAG.')
```

### `WRITE (2,160)` (label 160)
  - line 725; condition: `ITMP1.EQ.1`; statement: `      IF( ITMP1.EQ.1) WRITE (2,160)`
  - format:
```fortran
  160 FORMAT(/,6X,'- FROM -',4X,'- TO -',26X,'-  -  ADMITTANCE MATRIX',
     &' ELEMENTS (MHOS)  -  -',/,6X,'TAG  SEG.   TAG  SEG.',13X,'(ON',
     &'E,ONE)',19X,'(ONE,TWO)',19X,'(TWO,TWO)',/,6X,'NO.   NO.   NO.',
     &'   NO.',8X,'REAL',10X,'IMAG.',9X,'REAL',10X,'IMAG.',9X,'REAL',10
     &X,'IMAG.')
```

### `WRITE (2,161)` (label 161)
  - line 763; condition: `none`; statement: `      WRITE (2,161)`
  - format:
```fortran
  161 FORMAT(///,29X,'- - - CURRENTS AND LOCATION - - -',//,33X,'DIS',
     &'TANCES IN WAVELENGTHS')
```

### `WRITE (2,162)` (label 162)
  - line 764; condition: `none`; statement: `      WRITE (2,162)`
  - format:
```fortran
  162 FORMAT(//,2X,'SEG.',2X,'TAG',4X,'COORD. OF SEG. CENTER',5X,'SEG.'
     &,12X,'- - - CURRENT (AMPS) - - -',/,2X,'NO.',3X,'NO.',5X,'X',8X,
     &'Y',8X,'Z',6X,'LENGTH',5X,'REAL',8X,'IMAG.',7X,'MAG.',8X,'PHASE')
```

### `WRITE (2,163)` (label 163)
  - line 767; condition: `none`; statement: `      WRITE (2,163)  XPR3, HPOL( IXTYP), XPR6`
  - format:
```fortran
  163 FORMAT(///,33X,'- - - RECEIVING PATTERN PARAMETERS - - -',/,43X,
     &'ETA=',F7.2,' DEGREES',/,43X,'TYPE -',A6,/,43X,'AXIAL RATIO=',F6.
     &3,//,11X,'THETA',6X,'PHI',10X,'-  CURRENT  -',9X,'SEG',/,11X,
     &'(DEG)',5X,'(DEG)',7X,'MAGNITUDE',4X,'PHASE',6X,'NO.',/)
```

### `WRITE (2,164)` (label 164)
  - line 787; condition: `IPTFLG.NE.3`; statement: `   67 IF( IPTFLG.NE.3) WRITE (2,164)  XPR1, XPR2, CMAG, PH, I`
  - format:
```fortran
  164 FORMAT(10X,2(F7.2,3X),1X,1P,E11.4,3X,0P,F7.2,4X,I5)
```

### `WRITE (2,165)` (label 165)
- Occurrences: 2
  - line 790; condition: `none`; statement: `   68 WRITE (2,165)  I, ITAG( I), X( I), Y( I), Z( I), SI( I), CURI,`
  - line 810; condition: `none`; statement: `      WRITE (2,165)  I, ITAG( I), X( I), Y( I), Z( I), SI( I), CURI,`
  - format:
```fortran
  165 FORMAT(1X,2I5,3F9.4,F9.5,1X,1P,3E12.4,0P,F9.3)
```

### `WRITE (2,166)` (label 166)
  - line 844; condition: `none`; statement: `      WRITE (2,166)  PIN, TMP1, PLOSS, PNLS, TMP2`
  - format:
```fortran
  166 FORMAT(///,40X,'- - - POWER BUDGET - - -',//,43X,'INPUT PO',
     &'WER   =',1P,E11.4,' WATTS',/,43X,'RADIATED POWER=',E11.4,
     &' WATTS',/,43X,'STRUCTURE LOSS=',E11.4,' WATTS',/,43X,
     &'NETWORK LOSS  =',E11.4,' WATTS',/,43X,'EFFICIENCY    =',0P,F7.2,
     &' PERCENT')
```

### `WRITE (2,170)` (label 170)
  - line 641; condition: `none`; statement: `      WRITE (2,170)  NRADL, SCRWLT, SCRWRT`
  - format:
```fortran
  170 FORMAT(40X,'RADIAL WIRE GROUND SCREEN',/,40X,I5,' WIRES',/,40X,
     &'WIRE LENGTH=',F8.2,' METERS',/,40X,'WIRE RADIUS=',1P,E10.3,
     &' METERS')
```

### `WRITE (2,181)` (label 181)
  - line 887; condition: `none`; statement: `      WRITE (2,181)`
  - format:
```fortran
  181 FORMAT(///,4X,'RECEIVING PATTERN STORAGE TOO SMALL,ARRAY TRUNCA',
     &'TED')
```

### `WRITE (2,182)` (label 182)
  - line 892; condition: `none`; statement: `      WRITE (2,182)  TMP1, XPR3, HPOL( IXTYP), XPR6, ISAVE`
  - format:
```fortran
  182 FORMAT(///,32X,'- - - NORMALIZED RECEIVING PATTERN - - -',/,41X,
     &'NORMALIZATION FACTOR=',1P,E11.4,/,41X,'ETA=',0P,F7.2,' DEGREES',
     &/,41X,'TYPE -',A6,/,41X,'AXIAL RATIO=',F6.3,/,41X,'SEGMENT NO.=',
     &I5,//,21X,'THETA',6X,'PHI',9X,'-  PATTERN  -',/,21X,'(DEG)',5X,
     &'(DEG)',8X,'DB',8X,'MAGNITUDE',/)
```

### `WRITE (2,183)` (label 183)
  - line 900; condition: `none`; statement: `      WRITE (2,183)  XPR1, XPR2, TMP3, TMP2`
  - format:
```fortran
  183 FORMAT(20X,2(F7.2,3X),1X,F7.2,4X,1P,E11.4)
```

### `WRITE (2,184)` (label 184)
- Occurrences: 2
  - line 915; condition: `none`; statement: `      WRITE (2,184)  IVQD( NVQD), ZPNORM`
  - line 917; condition: `none`; statement: `  199 WRITE (2,184)  ISANT( NSANT), ZPNORM`
  - format:
```fortran
  184 FORMAT(///,36X,'- - - INPUT IMPEDANCE DATA - - -',/,45X,'SO',
     &'URCE SEGMENT NO.',I4,/,45X,'NORMALIZATION FACTOR=',1P,E12.5,//,7
     &X,'FREQ.',13X,'-  -  UNNORMALIZED IMPEDANCE  -  -',21X,'-',
     &' -  NORMALIZED IMPEDANCE  -  -',/,19X,'RESISTANCE',4X,'REACTA',
     &'NCE',6X,'MAGNITUDE',4X,'PHASE',7X,'RESISTANCE',4X,'REACTANCE',6X
     &,'MAGNITUDE',4X,'PHASE',/,8X,'MHZ',11X,'OHMS',10X,'OHMS',11X,
     &'OHMS',5X,'DEGREES',47X,'DEGREES',/)
```

### `WRITE (2,185)` (label 185)
  - line 921; condition: `none`; statement: `      WRITE (2,185)`
  - format:
```fortran
  185 FORMAT(///,4X,'STORAGE FOR IMPEDANCE NORMALIZATION TOO SMALL, A',
     &'RRAY TRUNCATED')
```

### `WRITE (2,186)` (label 186)
  - line 930; condition: `none`; statement: `      WRITE (2,186)  TMP1, FNORM( ITMP2), FNORM( ITMP2+1), FNORM( ITMP2`
  - format:
```fortran
  186 FORMAT(3X,F9.3,2X,1P,2(2X,E12.5),3X,E12.5,2X,0P,F7.2,2X,1P,2(2X,E
     &12.5),3X,E12.5,2X,0P,F7.2)
```

### `WRITE (2,196)` (label 196)
  - line 595; condition: `none`; statement: `      WRITE (2,196)  RKH`
  - format:
```fortran
  196 FORMAT(////,20X,'APPROXIMATE INTEGRATION EMPLOYED FOR SEGMENT',
     &'S MORE THAN',F8.3,' WAVELENGTHS APART')
```

### `WRITE (2,197)` (label 197)
  - line 814; condition: `none`; statement: `      WRITE (2,197)`
  - format:
```fortran
  197 FORMAT(////,41X,'- - - - SURFACE PATCH CURRENTS - - - -',//,50X,
     &'DISTANCE IN WAVELENGTHS',/,50X,'CURRENT IN AMPS/METER',//,28X,
     &'- - SURFACE COMPONENTS - -',19X,'- - - RECTANGULAR COM',
     &'PONENTS - - -',/,6X,'PATCH CENTER',6X,'TANGENT VECTOR 1',3X,
     &'TANGENT VECTOR 2',11X,'X',19X,'Y',19X,'Z',/,5X,'X',6X,'Y',6X,'Z'
     &,5X,'MAG.',7X,'PHASE',3X,'MAG.',7X,'PHASE',3(4X,'REAL',6X,'IMAG.'
     &))
```

### `WRITE (2,198)` (label 198)
  - line 832; condition: `none`; statement: `      WRITE (2,198)  I, X( ITMP1), Y( ITMP1), Z( ITMP1), ETHM, ETHA,`
  - format:
```fortran
  198 FORMAT(1X,I4,/,1X,3F7.3,2(1P,E11.4,0P,F8.2),1P,6E10.2)
```

### `WRITE (2,201)` (label 201)
  - line 251; condition: `none`; statement: `      WRITE (2,201)  TMP1`
  - format:
```fortran
  201 FORMAT(/,' RUN TIME =',F10.3)
```

### `WRITE (2,302)` (label 302)
  - line 521; condition: `none`; statement: `      WRITE (2,302)`
  - format:
```fortran
  302 FORMAT(' ERROR - N.G.F. IN USE.  CANNOT WRITE NEW N.G.F.')
```

### `WRITE (2,303)` (label 303)
- Occurrences: 2
  - line 260; condition: `none`; statement: `      WRITE (2,303)  AIN`
  - line 333; condition: `none`; statement: `      WRITE (2,303)  AIN`
  - format:
```fortran
  303 FORMAT(/,' ERROR - ',A2,' CARD IS NOT ALLOWED WITH N.G.F.')
```

### `WRITE (2,313)` (label 313)
  - line 302; condition: `none`; statement: `  312 WRITE (2,313)`
  - format:
```fortran
  313 FORMAT(/,' NUMBER OF SEGMENTS IN COUPLING CALCULATION (CP) EXCEE'
     &,'DS LIMIT')
```

### `WRITE (2,315)` (label 315)
  - line 798; condition: `none`; statement: `      WRITE (2,315)`
  - format:
```fortran
  315 FORMAT(///,34X,'- - - CHARGE DENSITIES - - -',//,36X,
     &'DISTANCES IN WAVELENGTHS',///,2X,'SEG.',2X,'TAG',4X,
     &'COORD. OF SEG. CENTER',5X,'SEG.',10X,
     &'CHARGE DENSITY (COULOMBS/METER)',/,2X,'NO.',3X,'NO.',5X,'X',8X,
     &'Y',8X,'Z',6X,'LENGTH',5X,'REAL',8X,'IMAG.',7X,'MAG.',8X,'PHASE')
```

### `WRITE (2,321)` (label 321)
  - line 598; condition: `IEXK.EQ.1`; statement: `      IF( IEXK.EQ.1) WRITE (2,321)`
  - format:
```fortran
  321 FORMAT(/,20X,'THE EXTENDED THIN WIRE KERNEL WILL BE USED')
```

### `WRITE (2,327)` (label 327)
  - line 626; condition: `NLOAD.EQ.0.AND. NLODF.NE.0`; statement: `      IF( NLOAD.EQ.0.AND. NLODF.NE.0) WRITE (2,327)`
  - format:
```fortran
  327 FORMAT(/,35X,' LOADING ONLY IN N.G.F. SECTION')
```

### `WRITE (2,390)` (label 390)
  - line 348; condition: `none`; statement: `      WRITE (2,390)`
  - format:
```fortran
  390 FORMAT(' RADIAL WIRE G. S. APPROXIMATION MAY NOT BE USED WITH SO'
     &,'MMERFELD GROUND OPTION')
```

### `WRITE (2,391)` (label 391)
  - line 644; condition: `none`; statement: `      WRITE (2,391)`
  - format:
```fortran
  391 FORMAT(40X,'FINITE GROUND.  REFLECTION COEFFICIENT APPROXIMATION'
     &)
```

### `WRITE (2,392)` (label 392)
  - line 652; condition: `none`; statement: `  400 WRITE (2,392)`
  - format:
```fortran
  392 FORMAT(40X,'FINITE GROUND.  SOMMERFELD SOLUTION')
```

### `WRITE (2,393)` (label 393)
  - line 650; condition: `none`; statement: `      WRITE (2,393)  EPSCF, EPSC`
  - format:
```fortran
  393 FORMAT(/,' ERROR IN GROUND PARAMETERS -',/,' COMPLEX DIELECTRIC',
     &' CONSTANT FROM FILE IS',1P,2E12.5,/,32X,'REQUESTED',2E12.5)
```

---
## Arc/angle error output
- Routine: `SUBROUTINE ARC`
- Source lines: 1068-1114

### `WRITE (2,3)` (label 3)
  - line 1088; condition: `none`; statement: `      WRITE (2,3)`
  - format:
```fortran
    3 FORMAT(' ERROR -- ARC ANGLE EXCEEDS 360. DEGREES')
```

---
## EOF and file closure output
- Routine: `SUBROUTINE BLCKOT`
- Source lines: 1131-1162

### `WRITE (2,4)` (label 4)
  - line 1153; condition: `none`; statement: `    3 WRITE (2,4)  NUNIT, NBLKS, NEOF`
  - format:
```fortran
    4 FORMAT('  EOF ON UNIT',I3,'  NBLKS= ',I3,'  NEOF= ',I5)
```

---
## Geometry connectivity and multiple-wire junctions
- Routine: `SUBROUTINE CONECT`
- Source lines: 2108-2418

### `WRITE (2,50)` (label 50)
  - line 2292; condition: `none`; statement: `      WRITE (2,50)`
  - format:
```fortran
   50 FORMAT(//,9X,'- MULTIPLE WIRE JUNCTIONS -',/,1X,'JUNCTION',4X,
     &'SEGMENTS  (- FOR END 1, + FOR END 2)')
```

### `WRITE (2,51)` (label 51)
  - line 2361; condition: `none`; statement: `      WRITE (2,51)  ISEG,( JCO( I), I=1, IC)`
  - format:
```fortran
   51 FORMAT(1X,I5,5X,20I5,/,(11X,20I5))
```

### `WRITE (2,52)` (label 52)
  - line 2373; condition: `ISEG.EQ.0`; statement: `      IF( ISEG.EQ.0) WRITE (2,52)`
  - format:
```fortran
   52 FORMAT(2X,'NONE')
```

### `WRITE (2,53)` (label 53)
  - line 2392; condition: `none`; statement: `   49 WRITE (2,53)  IX`
  - format:
```fortran
   53 FORMAT(' CONNECT - SEGMENT CONNECTION ERROR FOR SEGMENT',I5)
```

### `WRITE (2,54)` (label 54)
  - line 2127; condition: `none`; statement: `      WRITE (2,54)`
  - format:
```fortran
   54 FORMAT(/,3X,'GROUND PLANE SPECIFIED.')
```

### `WRITE (2,55)` (label 55)
  - line 2128; condition: `IGND.GT.0`; statement: `      IF( IGND.GT.0) WRITE (2,55)`
  - format:
```fortran
   55 FORMAT(/,3X,'WHERE WIRE ENDS TOUCH GROUND, CURRENT WILL BE ',
     &'INTERPOLATED TO IMAGE IN GROUND PLANE.',/)
```

### `WRITE (2,56)` (label 56)
- Occurrences: 2
  - line 2152; condition: `none`; statement: `      WRITE (2,56)  I`
  - line 2178; condition: `none`; statement: `      WRITE (2,56)  I`
  - format:
```fortran
   56 FORMAT(' GEOMETRY DATA ERROR-- SEGMENT',I5,' EXTENDS BELOW GRO',
     &'UND')
```

### `WRITE (2,57)` (label 57)
  - line 2182; condition: `none`; statement: `      WRITE (2,57)  I`
  - format:
```fortran
   57 FORMAT(' GEOMETRY DATA ERROR--SEGMENT',I5,' LIES IN GROUND ',
     &'PLANE.')
```

### `WRITE (2,58)` (label 58)
  - line 2280; condition: `none`; statement: `   26 WRITE (2,58)  N, NP, IPSYM`
  - format:
```fortran
   58 FORMAT(/,3X,'TOTAL SEGMENTS USED=',I5,5X,'NO. SEG. IN ','A SY',
     &'MMETRIC CELL=',I5,5X,'SYMMETRY FLAG=',I3)
```

### `WRITE (2,59)` (label 59)
  - line 2286; condition: `none`; statement: `   28 WRITE (2,59)  ISEG`
  - format:
```fortran
   59 FORMAT(' STRUCTURE HAS',I4,' FOLD ROTATIONAL SYMMETRY',/)
```

### `WRITE (2,60)` (label 60)
  - line 2290; condition: `none`; statement: `      WRITE (2,60)  IC`
  - format:
```fortran
   60 FORMAT(' STRUCTURE HAS',I2,' PLANES OF SYMMETRY',/)
```

### `WRITE (2,61)` (label 61)
  - line 2281; condition: `M.GT.0`; statement: `      IF( M.GT.0) WRITE (2,61)  M, MP`
  - format:
```fortran
   61 FORMAT(3X,'TOTAL PATCHES USED=',I5,6X,'NO. PATCHES IN A SYMMET',
     &'RIC CELL=',I5)
```

### `WRITE (2,62)` (label 62)
- Occurrences: 2
  - line 2278; condition: `none`; statement: `      WRITE (2,62)  NPMAX`
  - line 2354; condition: `none`; statement: `      WRITE (2,62)  NSMAX`
  - format:
```fortran
   62 FORMAT(' ERROR - NO. NEW SEGMENTS CONNECTED TO N.G.F. SEGMENTS',
     &'OR PATCHES EXCEEDS LIMIT OF',I5)
```

---
## Isolation and coupling data
- Routine: `SUBROUTINE COUPLE`
- Source lines: 2419-2495

### `WRITE (2,6)` (label 6)
  - line 2447; condition: `none`; statement: `      WRITE (2,6)`
  - format:
```fortran
    6 FORMAT(///,36X,'- - - ISOLATION DATA - - -',//,6X,'- - COUPLIN',
     &'G BETWEEN - -',8X,'MAXIMUM',15X,'- - - FOR MAXIMUM COUPLING - ',
     &'- -',/,12X,'SEG.',14X,'SEG.',3X,'COUPLING',4X,'LOAD IMPEDANCE ',
     &'(2ND SEG.)',7X,'INPUT IMPEDANCE',/,2X,'TAG/SEG.',3X,'NO.',4X,
     &'TAG/''SEG.',3X,'NO.',6X,'(DB)',8X,'REAL',9X,'IMAG.',9X,'REAL',9X
     &,'IMAG.')
```

### `WRITE (2,7)` (label 7)
  - line 2477; condition: `none`; statement: `      WRITE (2,7)  ITT1, ITS1, ISG1, ITT2, ITS2, ISG2, DBC, ZL, ZIN`
  - format:
```fortran
    7 FORMAT(2(1X,I4,1X,I4,1X,I5,2X),F9.3,2X,1P,2(2X,E12.5,1X,E12.5))
```

### `WRITE (2,8)` (label 8)
  - line 2479; condition: `none`; statement: `    4 WRITE (2,8)  ITT1, ITS1, ISG1, ITT2, ITS2, ISG2, C`
  - format:
```fortran
    8 FORMAT(2(1X,I4,1X,I4,1X,I5,2X),'**ERROR** COUPLING IS NOT BETWE',
     &'EN 0 AND 1. (=',1P,E12.5,')')
```

---
## Geometry data, segmentation, and surface patch output
- Routine: `SUBROUTINE DATAGN`
- Source lines: 2496-2866

### `WRITE (2,38)` (label 38)
  - line 2604; condition: `none`; statement: `      WRITE (2,38)  NWIRE, XW1, YW1, ZW1, XW2, NS, I1, I2, ITG`
  - format:
```fortran
   38 FORMAT(1X,I5,2X,'ARC RADIUS =',F9.5,2X,'FROM',F8.3,' TO',F8.3,
     &' DEGREES',11X,F11.5,2X,I5,4X,I5,1X,I5,3X,I5)
```

### `WRITE (2,39)` (label 39)
- Occurrences: 2
  - line 2664; condition: `none`; statement: `      WRITE (2,39)  X3, Y3, Z3, X4, Y4, Z4`
  - line 2680; condition: `none`; statement: `   15 WRITE (2,39)  X3, Y3, Z3, X4, Y4, Z4`
  - format:
```fortran
   39 FORMAT(6X,3F11.5,1X,3F11.5)
```

### `WRITE (2,40)` (label 40)
  - line 2553; condition: `none`; statement: `      WRITE (2,40)`
  - format:
```fortran
   40 FORMAT(////,33X,'- - - STRUCTURE SPECIFICATION - - -',//,37X,
     &'COORDINATES MUST BE INPUT IN',/,37X,
     &'METERS OR BE SCALED TO METERS',/,37X,
     &'BEFORE STRUCTURE INPUT IS ENDED',//)
```

### `WRITE (2,41)` (label 41)
  - line 2554; condition: `none`; statement: `      WRITE (2,41)`
  - format:
```fortran
   41 FORMAT(2X,'WIRE',79X,'NO. OF',4X,'FIRST',2X,'LAST',5X,'TAG',/,2X,
     &'NO.',8X,'X1',9X,'Y1',9X,'Z1',10X,'X2',9X,'Y2',9X,'Z2',6X,
     &'RADIUS',3X,'SEG.',5X,'SEG.',3X,'SEG.',5X,'NO.')
```

### `WRITE (2,43)` (label 43)
  - line 2578; condition: `none`; statement: `      WRITE (2,43)  NWIRE, XW1, YW1, ZW1, XW2, YW2, ZW2, RAD, NS, I1,`
  - format:
```fortran
   43 FORMAT(1X,I5,3F11.5,1X,4F11.5,2X,I5,4X,I5,1X,I5,3X,I5)
```

### `WRITE (2,44)` (label 44)
  - line 2697; condition: `none`; statement: `      WRITE (2,44)  IFX( IX+1), IFY( IY+1), IFZ( IZ+1), ITG`
  - format:
```fortran
   44 FORMAT(6X,'STRUCTURE REFLECTED ALONG THE AXES',3(1X,A1),'.  TA',
     &'GS INCREMENTED BY',I5)
```

### `WRITE (2,45)` (label 45)
  - line 2699; condition: `none`; statement: `   19 WRITE (2,45)  NS, ITG`
  - format:
```fortran
   45 FORMAT(6X,'STRUCTURE ROTATED ABOUT Z-AXIS',I3,' TIMES.  LABELS',
     &' INCREMENTED BY',I5)
```

### `WRITE (2,46)` (label 46)
  - line 2724; condition: `none`; statement: `   25 WRITE (2,46)  XW1`
  - format:
```fortran
   46 FORMAT(6X,'STRUCTURE SCALED BY FACTOR',F10.5)
```

### `WRITE (2,47)` (label 47)
  - line 2729; condition: `none`; statement: `   26 WRITE (2,47)  ITG, NS, XW1, YW1, ZW1, XW2, YW2, ZW2, RAD`
  - format:
```fortran
   47 FORMAT(6X,'THE STRUCTURE HAS BEEN MOVED, MOVE DATA CARD IS -/6X',
     &I3,I5,7F10.5)
```

### `WRITE (2,48)` (label 48)
- Occurrences: 2
  - line 2590; condition: `none`; statement: `    5 WRITE (2,48)`
  - line 2811; condition: `none`; statement: `   36 WRITE (2,48)`
  - format:
```fortran
   48 FORMAT(' GEOMETRY DATA CARD ERROR')
```

### `WRITE (2,49)` (label 49)
  - line 2812; condition: `none`; statement: `      WRITE (2,49)  GM, ITG, NS, XW1, YW1, ZW1, XW2, YW2, ZW2, RAD`
  - format:
```fortran
   49 FORMAT(1X,A2,I3,I5,7F10.5)
```

### `WRITE (2,50)` (label 50)
  - line 2814; condition: `none`; statement: `   37 WRITE (2,50)`
  - format:
```fortran
   50 FORMAT(' NUMBER OF WIRE SEGMENTS AND SURFACE PATCHES EXCEEDS DI',
     &'MENSION LIMIT.')
```

### `WRITE (2,51)` (label 51)
- Occurrences: 2
  - line 2629; condition: `none`; statement: `      WRITE (2,51)  I1, IPT( NS), XW1, YW1, ZW1, XW2, YW2, ZW2`
  - line 2663; condition: `none`; statement: `   12 WRITE (2,51)  I1, IPT( NS), XW1, YW1, ZW1, XW2, YW2, ZW2`
  - format:
```fortran
   51 FORMAT(1X,I5,A1,F10.5,2F11.5,1X,3F11.5)
```

### `WRITE (2,52)` (label 52)
  - line 2739; condition: `none`; statement: `      WRITE (2,52)`
  - format:
```fortran
   52 FORMAT(' ERROR - GF MUST BE FIRST GEOMETRY DATA CARD')
```

### `WRITE (2,53)` (label 53)
  - line 2766; condition: `none`; statement: `      WRITE (2,53)`
  - format:
```fortran
   53 FORMAT(////33X,'- - - - SEGMENTATION DATA - - - -',//,40X,'COO',
     &'RDINATES IN METERS',//,25X,
     &'I+ AND I- INDICATE THE SEGMENTS BEFORE AND AFTER I',//)
```

### `WRITE (2,54)` (label 54)
  - line 2767; condition: `none`; statement: `      WRITE (2,54)`
  - format:
```fortran
   54 FORMAT(2X,'SEG.',3X,'COORDINATES OF SEG. CENTER',5X,'SEG.',5X,
     &'ORIENTATION ANGLES',4X,'WIRE',4X,'CONNECTION DATA',3X,'TAG',/,2X
     &,'NO.',7X,'X',9X,'Y',9X,'Z',7X,'LENGTH',5X,'ALPHA',5X,'BETA',6X,
     &'RADIUS',4X,'I-',3X,'I',4X,'I+',4X,'NO.')
```

### `WRITE (2,55)` (label 55)
  - line 2788; condition: `none`; statement: `      WRITE (2,55)  I, X( I), Y( I), Z( I), SI( I), XW2, YW2, BI( I),`
  - format:
```fortran
   55 FORMAT(1X,I5,4F10.5,1X,3F10.5,1X,3I5,2X,I5)
```

### `WRITE (2,56)` (label 56)
  - line 2796; condition: `none`; statement: `      WRITE (2,56)`
  - format:
```fortran
   56 FORMAT(' SEGMENT DATA ERROR')
```

### `WRITE (2,57)` (label 57)
  - line 2800; condition: `none`; statement: `      WRITE (2,57)`
  - format:
```fortran
   57 FORMAT(////,44X,'- - - SURFACE PATCH DATA - - -',//,49X,'COORD',
     &'INATES IN METERS',//,1X,'PATCH',5X,'COORD. OF PATCH CENTER',7X,
     &'UNIT NORMAL VECTOR',6X,'PATCH',12X,
     &'COMPONENTS OF UNIT TANGENT V''ECTORS',/,2X,'NO.',6X,'X',9X,'Y',9
     &X,'Z',9X,'X',7X,'Y',7X,'Z',7X,'AREA',7X,'X1',6X,'Y1',6X,'Z1',7X,
     &'X2',6X,'Y2',6X,'Z2')
```

### `WRITE (2,58)` (label 58)
  - line 2807; condition: `none`; statement: `      WRITE (2,58)  I, X( J), Y( J), Z( J), XW1, YW1, ZW1, BI( J), T1X(`
  - format:
```fortran
   58 FORMAT(1X,I4,3F10.5,1X,3F8.4,F10.5,1X,3F8.4,1X,3F8.4)
```

### `WRITE (2,59)` (label 59)
  - line 2670; condition: `none`; statement: `      WRITE (2,59)  I1, IPT(2), XW1, YW1, ZW1, XW2, YW2, ZW2, ITG, NS`
  - format:
```fortran
   59 FORMAT(1X,I5,A1,F10.5,2F11.5,1X,3F11.5,5X,'SURFACE -',I4,' BY',I3
     &,' PATCHES')
```

### `WRITE (2,60)` (label 60)
  - line 2685; condition: `none`; statement: `   17 WRITE (2,60)`
  - format:
```fortran
   60 FORMAT(' PATCH DATA ERROR')
```

### `WRITE (2,61)` (label 61)
  - line 2592; condition: `none`; statement: `    6 WRITE (2,61)  XS1, YS1, ZS1`
  - format:
```fortran
   61 FORMAT(9X,'ABOVE WIRE IS TAPERED.  SEG. LENGTH RATIO =',F9.5,/,33
     &X,'RADIUS FROM',F9.5,' TO',F9.5)
```

### `WRITE (2,124)` (label 124)
  - line 2614; condition: `none`; statement: `      WRITE (2,124)  XW1, YW1, NWIRE, ZW1, XW2, YW2, ZW2, RAD, NS, I1,`
  - format:
```fortran
  124 FORMAT(5X,'HELIX STRUCTURE-   AXIAL SPACING BETWEEN TURNS =',F8.3
     &,' TOTAL AXIAL LENGTH =',F8.3/1X,I5,2X,'RADIUS OF HELIX =',4(2X,F
     &8.3),7X,F11.5,I8,4X,I5,1X,I5,3X,I5)
```

---
## Factorization timing output
- Routine: `SUBROUTINE FACIO`
- Source lines: 3600-3656

### `WRITE (2,4)` (label 4)
  - line 3649; condition: `none`; statement: `      WRITE (2,4)  TIME`
  - format:
```fortran
    4 FORMAT(' CP TIME TAKEN FOR FACTORIZATION = ',1P,E12.5)
```

---
## Pivot output and matrix factorization
- Routine: `SUBROUTINE FACTR`
- Source lines: 3657-3733

### `WRITE (2,10)` (label 10)
  - line 3724; condition: `none`; statement: `      WRITE (2,10)  R, DMAX`
  - format:
```fortran
   10 FORMAT(1H ,'PIVOT(',I3,')=',1P,E16.8)
```

---
## Matrix storage and N.G.F. diagnostics
- Routine: `SUBROUTINE FBLOCK`
- Source lines: 3872-3979

### `WRITE (2,14)` (label 14)
- Occurrences: 2
  - line 3906; condition: `none`; statement: `      WRITE (2,14)  NBLOKS, NPBLK, NLAST`
  - line 3914; condition: `none`; statement: `      WRITE (2,14)  NBLOKS, NPBLK, NLAST`
  - format:
```fortran
   14 FORMAT(//' MATRIX FILE STORAGE -  NO. BLOCKS=',I5,' COLUMNS PE',
     &'R BLOCK=',I5,' COLUMNS IN LAST BLOCK=',I5)
```

### `WRITE (2,15)` (label 15)
  - line 3921; condition: `none`; statement: `      WRITE (2,15)`
  - format:
```fortran
   15 FORMAT(' SUBMATRICIES FIT IN CORE')
```

### `WRITE (2,16)` (label 16)
  - line 3931; condition: `none`; statement: `      WRITE (2,16)  NBLSYM, NPSYM, NLSYM`
  - format:
```fortran
   16 FORMAT(' SUBMATRIX PARTITIONING -  NO. BLOCKS=',I5,' COLUMNS P',
     &'ER BLOCK=',I5,' COLUMNS IN LAST BLOCK=',I5)
```

### `WRITE (2,17)` (label 17)
  - line 3964; condition: `none`; statement: `   12 WRITE (2,17)  NROW, NCOL`
  - format:
```fortran
   17 FORMAT(' ERROR - INSUFFICIENT STORAGE FOR MATRIX',2I5)
```

### `WRITE (2,18)` (label 18)
  - line 3966; condition: `none`; statement: `   13 WRITE (2,18)  NROW, NCOL`
  - format:
```fortran
   18 FORMAT(' SYMMETRY ERROR - NROW,NCOL=',2I5)
```

---
## Numerical Green’s Function and ground parameter messages
- Routine: `SUBROUTINE FBNGF`
- Source lines: 3980-4054

### `WRITE (2,7)` (label 7)
  - line 4040; condition: `none`; statement: `    6 WRITE (2,7)  IRESRV, IMAT, NEQ, NEQ2`
  - format:
```fortran
    7 FORMAT(55H ERROR - INSUFFICIENT STORAGE FOR INTERACTION MATRICIES
     &,'  IRESRV,IMAT,NEQ,NEQ2 =',4I5)
```

### `WRITE (2,8)` (label 8)
  - line 4036; condition: `none`; statement: `      WRITE (2,8)  ICASX`
  - format:
```fortran
    8 FORMAT(' FILE STORAGE FOR NEW MATRIX SECTIONS -  ICASX =', I2)
```

### `WRITE (2,9)` (label 9)
  - line 4037; condition: `none`; statement: `      WRITE (2,9)  NBBX, NPBX, NLBX`
  - format:
```fortran
    9 FORMAT(' B FILLED BY ROWS -',15X,'NO. BLOCKS =',I3,3X,
     &  'ROWS PER BLOCK =', I3, '   ROWS IN LAST BLOCK =', I3)
```

### `WRITE (2,10)` (label 10)
  - line 4038; condition: `none`; statement: `      WRITE (2,10)  NBBL, NPBL, NLBL`
  - format:
```fortran
   10 FORMAT(' B BY COLUMNS, C AND D BY ROWS -  NO. BLOCKS =',I3,
     & '    R/C PER BLOCK =', I3, '    R/C IN LAST BLOCK =', I3)
```

### `WRITE (2,11)` (label 11)
  - line 4034; condition: `none`; statement: `      WRITE (2,11)  NEQ2`
  - format:
```fortran
   11 FORMAT(//,' N.G.F. - NUMBER OF NEW UNKNOWNS IS', I4)
```

---
## Numerical Green’s Function file header output
- Routine: `SUBROUTINE GFIL`
- Source lines: 4331-4487

### `WRITE (2,14)` (label 14)
- Occurrences: 4
  - line 4434; condition: `none`; statement: `      WRITE (2,14)`
  - line 4435; condition: `none`; statement: `      WRITE (2,14)`
  - line 4452; condition: `none`; statement: `      WRITE (2,14)`
  - line 4453; condition: `none`; statement: `      WRITE (2,14)`
  - format:
```fortran
   14 FORMAT(5X,'**************************************************',
     &'**********************************')
```

### `WRITE (2,15)` (label 15)
  - line 4450; condition: `none`; statement: `   12 WRITE (2,15) ( COM( I, J), I=1,19)`
  - format:
```fortran
   15 FORMAT(5X,3H** ,19A4,3H **)
```

### `WRITE (2,16)` (label 16)
- Occurrences: 2
  - line 4433; condition: `none`; statement: `      WRITE (2,16)`
  - line 4454; condition: `none`; statement: `      WRITE (2,16)`
  - format:
```fortran
   16 FORMAT(////)
```

### `WRITE (2,17)` (label 17)
- Occurrences: 3
  - line 4436; condition: `none`; statement: `      WRITE (2,17)`
  - line 4448; condition: `none`; statement: `      WRITE (2,17)`
  - line 4451; condition: `none`; statement: `      WRITE (2,17)`
  - format:
```fortran
   17 FORMAT(5X,2H**,80X,2H**)
```

### `WRITE (2,18)` (label 18)
  - line 4437; condition: `none`; statement: `      WRITE (2,18)  N1, M1`
  - format:
```fortran
   18 FORMAT(5X,'** NUMERICAL GREEN S FUNCTION',53X,2H**,/,5X,'** NO',
     &'. SEGMENTS =',I4,10X,'NO. PATCHES =',I4,34X,2H**)
```

### `WRITE (2,19)` (label 19)
  - line 4438; condition: `NOP.GT.1`; statement: `      IF( NOP.GT.1) WRITE (2,19)  NOP`
  - format:
```fortran
   19 FORMAT(5X,'** NO. SYMMETRIC SECTIONS =',I4,51X,2H**)
```

### `WRITE (2,20)` (label 20)
  - line 4439; condition: `none`; statement: `      WRITE (2,20)  IMAT, ICASE`
  - format:
```fortran
   20 FORMAT(5X,'** N.G.F. MATRIX -  CORE STORAGE =',I7,' COMPLEX NU',
     &'MBERS,  CASE',I2,16X,2H**)
```

### `WRITE (2,21)` (label 21)
  - line 4442; condition: `none`; statement: `      WRITE (2,21)  NBL2`
  - format:
```fortran
   21 FORMAT(5X,2H**,19X,'MATRIX SIZE =',I7,' COMPLEX NUMBERS',25X,'**')
```

### `WRITE (2,22)` (label 22)
  - line 4443; condition: `none`; statement: `   11 WRITE (2,22)  FMHZ`
  - format:
```fortran
   22 FORMAT(5X,'** FREQUENCY =',1P,E12.5,' MHZ.',51X,2H**)
```

### `WRITE (2,23)` (label 23)
  - line 4444; condition: `KSYMP.EQ.2.AND. IPERF.EQ.1`; statement: `      IF( KSYMP.EQ.2.AND. IPERF.EQ.1) WRITE (2,23)`
  - format:
```fortran
   23 FORMAT(5X,'** PERFECT GROUND',65X,2H**)
```

### `WRITE (2,24)` (label 24)
  - line 4447; condition: `KSYMP.EQ.2.AND. IPERF.NE.1`; statement: `      IF( KSYMP.EQ.2.AND. IPERF.NE.1) WRITE (2,24)  EPSR, SIG`
  - format:
```fortran
   24 FORMAT(5X,'** GROUND PARAMETERS - DIELECTRIC CONSTANT =',1P,E12.5,
     &26X,'**',/,5X,'**',21X,'CONDUCTIVITY =',E12.5,' MHOS/M.',25X,'**')
```

### `WRITE (2,25)` (label 25)
  - line 4456; condition: `none`; statement: `      WRITE (2,25)`
  - format:
```fortran
   25 FORMAT(39X,'NUMERICAL GREEN S FUNCTION DATA',/,41X,'COORDINATES',
     &' OF SEGMENT ENDS',/,51X,'(METERS)',/,5X,'SEG.',11X,
     &'- - - END ON''E - - -',26X,'- - - END TWO - - -',/,6X,3HNO.,6X,1
     &HX,14X,1HY,14X,1HZ,14X,1HX,14X,1HY,14X,1HZ)
```

### `WRITE (2,26)` (label 26)
  - line 4458; condition: `none`; statement: `   13 WRITE (2,26)  I, X( I), Y( I), Z( I), SI( I), ALP( I), BET( I)`
  - format:
```fortran
   26 FORMAT(1X,I7,1P,6E15.6)
```

### `WRITE (2,27)` (label 27)
  - line 4445; condition: `KSYMP.EQ.2.AND. IPERF.EQ.0`; statement: `      IF( KSYMP.EQ.2.AND. IPERF.EQ.0) WRITE (2,27)`
  - format:
```fortran
   27 FORMAT(5X,'** FINITE GROUND.  REFLECTION COEFFICIENT APPROXIMAT',
     &'ION',27X,2H**)
```

### `WRITE (2,28)` (label 28)
  - line 4446; condition: `KSYMP.EQ.2.AND. IPERF.EQ.2`; statement: `      IF( KSYMP.EQ.2.AND. IPERF.EQ.2) WRITE (2,28)`
  - format:
```fortran
   28 FORMAT(5X,'** FINITE GROUND.  SOMMERFELD SOLUTION',44X,'**')
```

---
## N.G.F. tape file output
- Routine: `SUBROUTINE GFOUT`
- Source lines: 4642-4739

### `WRITE (2,13)` (label 13)
  - line 4731; condition: `none`; statement: `      WRITE (2,13)  IGFL, IMAT`
  - format:
```fortran
   13 FORMAT(///,' ****NUMERICAL GREEN S FUNCTION FILE ON TAPE',I3,
     &'****',/,5X,'MATRIX STORAGE -',I7,' COMPLEX NUMBERS',///)
```

---
## Helix structure parameters
- Routine: `SUBROUTINE HELIX`
- Source lines: 4903-4977

### `WRITE (2,104)` (label 104)
  - line 4951; condition: `none`; statement: `      WRITE (2,104)  SANGLE`
  - format:
```fortran
  104 FORMAT(5X,'THE CONE ANGLE OF THE SPIRAL IS',F10.4)
```

### `WRITE (2,105)` (label 105)
  - line 4970; condition: `none`; statement: `   40 WRITE (2,105)  PITCH, TURN`
  - format:
```fortran
  105 FORMAT(5X,'THE PITCH ANGLE IS',F10.4/5X,
     &'THE LENGTH OF WIRE/TURN ''IS',F10.4)
```

---
## ROM2 integration and ground kernel warnings
- Routine: `SUBROUTINE HFK`
- Source lines: 4978-5066

### `WRITE (2,18)` (label 18)
  - line 5047; condition: `none`; statement: `   15 WRITE (2,18)  Z`
  - format:
```fortran
   18 FORMAT(' STEP SIZE LIMITED AT Z=',F10.5)
```

---
## Segment position validation
- Routine: `SUBROUTINE INTX`
- Source lines: 5488-5597

### `WRITE (2,20)` (label 20)
  - line 5569; condition: `none`; statement: `   15 WRITE (2,20)  Z`
  - format:
```fortran
   20 FORMAT(' STEP SIZE LIMITED AT Z=',F10.5)
```

---
## ITAG validation output
- Routine: `FUNCTION ISEGNO`
- Source lines: 5598-5633

### `WRITE (2,6)` (label 6)
  - line 5610; condition: `none`; statement: `      WRITE (2,6)`
  - format:
```fortran
    6 FORMAT(4X,'CHECK DATA, PARAMETER SPECIFYING SEGMENT POSITION IN',
     &' A GROUP OF EQUAL TAGS MUST NOT BE ZERO')
```

### `WRITE (2,7)` (label 7)
  - line 5622; condition: `none`; statement: `    4 WRITE (2,7)  ITAGI`
  - format:
```fortran
    7 FORMAT(///,10X,'NO SEGMENT HAS AN ITAG OF ',I5)
```

---
## Matrix partitioning/storage diagnostics
- Routine: `SUBROUTINE LFACTR`
- Source lines: 5634-5744

### `WRITE (2,17)` (label 17)
  - line 5735; condition: `none`; statement: `      WRITE (2,17)  J2, DMAX`
  - format:
```fortran
   17 FORMAT(' ','PIVOT(,I3,2H)=',1P,E16.8)
```

---
## Loading card and segment loading errors
- Routine: `SUBROUTINE LOAD`
- Source lines: 5745-5888

### `WRITE (2,25)` (label 25)
  - line 5769; condition: `none`; statement: `      WRITE (2,25)`
  - format:
```fortran
   25 FORMAT(//,7X,'LOCATION',10X,'RESISTANCE',3X,'INDUCTANCE',2X,
     &'CAPACITANCE',7X,'IMPEDANCE (OHMS)',5X,'CONDUCTIVITY',4X,'TYPE',/
     &,4X,'ITAG',' FROM THRU',10X,'OHMS',8X,'HENRYS',7X,'FARADS',8X,
     &'REAL',6X,'IMAGINARY',4X,'MHOS/METER')
```

### `WRITE (2,26)` (label 26)
  - line 5779; condition: `IWARN.EQ.1`; statement: `      IF( IWARN.EQ.1) WRITE (2,26)`
  - format:
```fortran
   26 FORMAT(/,10X,'NOTE, SOME OF THE ABOVE SEGMENTS HAVE BEEN LOADED',
     &' TWICE - IMPEDANCES ADDED')
```

### `WRITE (2,27)` (label 27)
  - line 5791; condition: `none`; statement: `      WRITE (2,27)  LDTYP( ISTEP)`
  - format:
```fortran
   27 FORMAT(/,10X,'IMPROPER LOAD TYPE CHOOSEN, REQUESTED TYPE IS ',I3)
```

### `WRITE (2,28)` (label 28)
  - line 5848; condition: `none`; statement: `      WRITE (2,28)  LDTAGS`
  - format:
```fortran
   28 FORMAT(/,10X,'LOADING DATA CARD ERROR, NO SEGMENT HAS AN ITAG =',
     &I5)
```

### `WRITE (2,29)` (label 29)
  - line 5806; condition: `none`; statement: `      WRITE (2,29)`
  - format:
```fortran
   29 FORMAT(' ERROR - LOADING MAY NOT BE ADDED TO SEGMENTS IN N.G.F.',
     &' SECTION')
```

---
## Network/excitation/antenna input parameters
- Routine: `SUBROUTINE NETWK`
- Source lines: 6262-6609

### `WRITE (2,58)` (label 58)
  - line 6351; condition: `none`; statement: `      WRITE (2,58)  ASM, NTEQ, NTSC, ASA`
  - format:
```fortran
   58 FORMAT(///,3X,'MAXIMUM RELATIVE ASYMMETRY OF THE DRIVING POINT',
     &' ADMITTANCE MATRIX IS',1P,E10.3,' FOR SEGMENTS',I5,4H AND,I5,/,3
     &X,'RMS RELATIVE ASYMMETRY IS',E10.3)
```

### `WRITE (2,59)` (label 59)
- Occurrences: 2
  - line 6322; condition: `none`; statement: `      WRITE (2,59)`
  - line 6443; condition: `none`; statement: `      WRITE (2,59)`
  - format:
```fortran
   59 FORMAT(1X,'ERROR - - NETWORK ARRAY DIMENSIONS TOO SMALL')
```

### `WRITE (2,60)` (label 60)
- Occurrences: 2
  - line 6518; condition: `NPRINT.EQ.0`; statement: `      IF( NPRINT.EQ.0) WRITE (2,60)`
  - line 6552; condition: `none`; statement: `      WRITE (2,60)`
  - format:
```fortran
   60 FORMAT(/,3X,'TAG',3X,'SEG.',4X,'VOLTAGE (VOLTS)',9X,'CURRENT (',
     &'AMPS)',9X,'IMPEDANCE (OHMS)',8X,'ADMITTANCE (MHOS)',6X,'POWER',/
     &,3X,'NO.',3X,'NO.',4X,'REAL',8X,'IMAG.',3(7X,'REAL',8X,'IMAG.'),5
     &X,'(WATTS)')
```

### `WRITE (2,61)` (label 61)
  - line 6517; condition: `NPRINT.EQ.0`; statement: `      IF( NPRINT.EQ.0) WRITE (2,61)`
  - format:
```fortran
   61 FORMAT(///,27X,'- - - STRUCTURE EXCITATION DATA AT NETWORK CONN',
     &'ECTION POINTS - - -')
```

### `WRITE (2,62)` (label 62)
- Occurrences: 3
  - line 6528; condition: `NPRINT.EQ.0`; statement: `   46 IF( NPRINT.EQ.0) WRITE (2,62)  IROW2, IROW1, VLT, CUX, ZPED, YMIT`
  - line 6540; condition: `NPRINT.EQ.0`; statement: `   47 IF( NPRINT.EQ.0) WRITE (2,62)  IROW2, IROW1, VLT, CUX, ZPED, YMIT`
  - line 6575; condition: `none`; statement: `   55 WRITE (2,62)  IROW2, ISC1, VLT, CUX, ZPED, YMIT, PWR`
  - format:
```fortran
   62 FORMAT(2(1X,I5),1P,9E12.5)
```

### `WRITE (2,63)` (label 63)
  - line 6551; condition: `none`; statement: `      WRITE (2,63)`
  - format:
```fortran
   63 FORMAT(///,42X,'- - - ANTENNA INPUT PARAMETERS - - -')
```

### `WRITE (2,64)` (label 64)
  - line 6590; condition: `none`; statement: `   57 WRITE (2,64)  IROW2, ISC1, VLT, CUX, ZPED, YMIT, PWR`
  - format:
```fortran
   64 FORMAT(1X,I5,' *',I4,1P,9E12.5)
```

---
## Near-field electric and magnetic field headers
- Routine: `SUBROUTINE NFPAT`
- Source lines: 6610-6709

### `WRITE (2,10)` (label 10)
  - line 6628; condition: `none`; statement: `      WRITE (2,10)`
  - format:
```fortran
   10 FORMAT(///,35X,'- - - NEAR ELECTRIC FIELDS - - -',//,12X,'-  L',
     &'OCATION  -',21X,'-  EX  -',15X,'-  EY  -',15X,'-  EZ  -',/,8X,
     &'X',10X,'Y',10X,'Z',10X,'MAGNITUDE',3X,'PHASE',6X,'MAGNITUDE',3X,
     &'PHASE',6X,'MAGNITUDE',3X,'PHASE',/,6X,'METERS',5X,'METERS',5X,
     &'METERS',8X,'VOLTS/M',3X,'DEGREES',6X,'VOLTS/M',3X,'DEGREES',6X
     &,'VOLTS/M',3X,'DEGREES')
```

### `WRITE (2,11)` (label 11)
  - line 6668; condition: `none`; statement: `      WRITE (2,11)  XOB, YOB, ZOB, TMP1, TMP2, TMP3, TMP4, TMP5, TMP6`
  - format:
```fortran
   11 FORMAT(2X,3(2X,F9.4),1X,3(3X,1P,E11.4,2X,0P,F7.2))
```

### `WRITE (2,12)` (label 12)
  - line 6630; condition: `none`; statement: `    1 WRITE (2,12)`
  - format:
```fortran
   12 FORMAT(///,35X,'- - - NEAR MAGNETIC FIELDS - - -',//,12X,'-  L',
     &'OCATION  -',21X,'-  HX  -',15X,'-  HY  -',15X,'-  HZ  -',/,8X,
     &'X',10X,'Y',10X,'Z',10X,'MAGNITUDE',3X,'PHASE',6X,'MAGNITUDE',3X,
     &'PHASE',6X,'MAGNITUDE',3X,'PHASE',/,6X,'METERS',5X,'METERS',5X,
     &'METERS',9X,'AMPS/M',3X,'DEGREES',7X,'AMPS/M',3X,'DEGREES',7X,
     &'AMPS/M',3X,'DEGREES')
```

---
## Patch geometry errors
- Routine: `SUBROUTINE PATCH`
- Source lines: 6797-6998

### `WRITE (2,14)` (label 14)
  - line 6890; condition: `none`; statement: `      WRITE (2,14)`
  - format:
```fortran
   14 FORMAT(' ERROR -- CORNERS OF QUADRILATERAL PATCH DO NOT LIE IN ',
     &'A PLANE')
```

---
## Radiation and pattern output
- Routine: `SUBROUTINE RDPAT`
- Source lines: 7295-7573

### `WRITE (2,35)` (label 35)
  - line 7329; condition: `none`; statement: `      WRITE (2,35)`
  - format:
```fortran
   35 FORMAT(///,31X,'- - - FAR FIELD GROUND PARAMETERS - - -',//)
```

### `WRITE (2,36)` (label 36)
  - line 7331; condition: `none`; statement: `      WRITE (2,36)  NRADL, SCRWLT, SCRWRT`
  - format:
```fortran
   36 FORMAT(40X,'RADIAL WIRE GROUND SCREEN',/,40X,I5,' WIRES',/,40X,
     &'WIRE LENGTH=',F8.2,' METERS',/,40X,'WIRE RADIUS=',1P,E10.3,
     &' METERS')
```

### `WRITE (2,37)` (label 37)
  - line 7338; condition: `none`; statement: `      WRITE (2,37)  HCLIF, CLT, CHT, EPSR2, SIG2`
  - format:
```fortran
   37 FORMAT(40X,A6,' CLIFF',/,40X,'EDGE DISTANCE=',F9.2,' METERS',/,40
     &X,'HEIGHT=',F8.2,' METERS',/,40X,'SECOND MEDIUM -',/,40X,'RELA',
     &'TIVE DIELECTRIC CONST.=',F7.3,/,40X,'CONDUCTIVITY=',1P,E10.3,
     &' MHOS')
```

### `WRITE (2,38)` (label 38)
  - line 7346; condition: `none`; statement: `      WRITE (2,38)`
  - format:
```fortran
   38 FORMAT(///,48X,'- - - RADIATION PATTERNS - - -')
```

### `WRITE (2,39)` (label 39)
  - line 7351; condition: `none`; statement: `      WRITE (2,39)  RFLD, EXRM, EXRA`
  - format:
```fortran
   39 FORMAT(54X,'RANGE=',1P,E13.6,' METERS',/,54X,'EXP(-JKR)/R=',E12.5
     &,' AT PHASE',0P,F7.2,' DEGREES',/)
```

### `WRITE (2,40)` (label 40)
  - line 7352; condition: `none`; statement: `    4 WRITE (2,40)  IGTP( I), IGTP( J), IGAX( ITMP1), IGAX( ITMP2)`
  - format:
```fortran
   40 FORMAT(/,2X,'- - ANGLES - -',7X,2A6,'GAINS -',7X,'- - - POLARI',
     &'ZATION - - -',4X,'- - - E(THETA) - - -',4X,'- - - E(PHI) - -',
     &' -',/,2X,'THETA',5X,'PHI',7X,A6,2X,A6,3X,'TOTAL',6X,'AXIAL',5X,
     &'TILT',3X,'SENSE',2(5X,'MAGNITUDE',4X,'PHASE'),/,2(1X,'DEGREES',1
     &X),3(6X,'DB'),8X,'RATIO',5X,'DEG.',8X,2(6X,'VOLTS/M',4X,'DEGRE',
     &'ES'))
```

### `WRITE (2,41)` (label 41)
  - line 7340; condition: `none`; statement: `      WRITE (2,41)`
  - format:
```fortran
   41 FORMAT(///,28X,' - - - RADIATED FIELDS NEAR GROUND - - -',//,8X,
     &'- - - LOCATION - - -',10X,'- - E(THETA) - -',8X,'- - E(PHI) -',
     &' -',8X,'- - E(RADIAL) - -',/,7X,'RHO',6X,'PHI',9X,'Z',12X,'MAG',
     &6X,'PHASE',9X,'MAG',6X,'PHASE',9X,'MAG',6X,'PHASE',/,5X,'METERS',
     &3X,'DEGREES',4X,'METERS',8X,'VOLTS/M',3X,'DEGREES',6X,'VOLTS/M',3
     &X,'DEGREES',6X,'VOLTS/M',3X,'DEGREES',/)
```

### `WRITE (2,42)` (label 42)
  - line 7471; condition: `none`; statement: `   27 WRITE (2,42)  THET, PHI, TMP5, TMP6, GTOT, AXRAT, TILTA, ISENS,`
  - format:
```fortran
   42 FORMAT(1X,F7.2,F9.2,3X,3F8.2,F11.5,F9.2,2X,A6,2(1P,E15.5,0P,F9.2)
     &)
```

### `WRITE (2,43)` (label 43)
  - line 7487; condition: `none`; statement: `   28 WRITE (2,43)  RFLD, PHI, THET, ETHM, ETHA, EPHM, EPHA, ERDM, ERDA`
  - format:
```fortran
   43 FORMAT(3X,F9.2,2X,F7.2,2X,F9.2,1X,3(3X,1P,E11.4,2X,0P,F7.2))
```

### `WRITE (2,44)` (label 44)
  - line 7498; condition: `none`; statement: `      WRITE (2,44)  PINT, TMP3`
  - format:
```fortran
   44 FORMAT(//,3X,'AVERAGE POWER GAIN=',1P,E12.5,7X,'SOLID ANGLE U',
     &'SED IN AVERAGING=(',0P,F7.4,')*PI STERADIANS.',//)
```

### `WRITE (2,45)` (label 45)
  - line 7503; condition: `none`; statement: `      WRITE (2,45)  IGNTP( ITMP1), IGNTP( ITMP2), GMAX`
  - format:
```fortran
   45 FORMAT(//,37X,'- - - - NORMALIZED GAIN - - - -',//,37X,2A6,'GAI',
     &'N',/,38X,'NORMALIZATION FACTOR =',F9.2,' DB',//,3(4X,
     &'- - ANGLES'' - -',6X,'GAIN',7X),/,3(4X,'THETA',5X,'PHI',8X,'DB',
     &8X),/,3(3X,'DEGREES',2X,'DEGREES',16X))
```

### `WRITE (2,46)` (label 46)
- Occurrences: 3
  - line 7527; condition: `none`; statement: `   31 WRITE (2,46)  TMP1, TMP2, TSTOR1, TMP3, TMP4, TSTOR2, TMP5, TMP6,`
  - line 7532; condition: `none`; statement: `      WRITE (2,46)  TMP1, TMP2, TSTOR1, TMP3, TMP4, TSTOR2`
  - line 7534; condition: `none`; statement: `   33 WRITE (2,46)  TMP1, TMP2, TSTOR1`
  - format:
```fortran
   46 FORMAT(3(1X,2F9.2,1X,F9.2,6X))
```

---
## Geometry symmetry and patch connection errors
- Routine: `SUBROUTINE REFLC`
- Source lines: 7787-8003

### `WRITE (2,24)` (label 24)
- Occurrences: 3
  - line 7821; condition: `none`; statement: `      WRITE (2,24)  I`
  - line 7865; condition: `none`; statement: `      WRITE (2,24)  I`
  - line 7909; condition: `none`; statement: `      WRITE (2,24)  I`
  - format:
```fortran
   24 FORMAT(' GEOMETRY DATA ERROR--SEGMENT,I5,26H LIES IN PLANE OF S',
     &'YMMETRY')
```

### `WRITE (2,25)` (label 25)
- Occurrences: 3
  - line 7841; condition: `none`; statement: `      WRITE (2,25)  I`
  - line 7885; condition: `none`; statement: `      WRITE (2,25)  I`
  - line 7928; condition: `none`; statement: `      WRITE (2,25)  I`
  - format:
```fortran
   25 FORMAT(' GEOMETRY DATA ERROR--PATCH,I4,26H LIES IN PLANE OF SYM',
     &'METRY')
```

---
## ROM2 step-size error output
- Routine: `SUBROUTINE ROM2`
- Source lines: 8004-8117

### `WRITE (2,18)` (label 18)
  - line 8023; condition: `none`; statement: `      WRITE (2,18)`
  - format:
```fortran
   18 FORMAT(' ERROR - B LESS THAN A IN ROM2')
```

### `WRITE (2,19)` (label 19)
  - line 8100; condition: `none`; statement: `      WRITE (2,19)  Z`
  - format:
```fortran
   19 FORMAT(' ROM2 -- STEP SIZE LIMITED AT Z =',1P,E12.5)
```

---
## SBF segment connection errors
- Routine: `SUBROUTINE SBF`
- Source lines: 8118-8252

### `WRITE (2,25)` (label 25)
  - line 8245; condition: `none`; statement: `   24 WRITE (2,25)  I`
  - format:
```fortran
   25 FORMAT(' SBF - SEGMENT CONNECTION ERROR FOR SEGMENT',I5)
```

---
## TBF segment connection errors
- Routine: `SUBROUTINE TBF`
- Source lines: 8666-8805

### `WRITE (2,29)` (label 29)
  - line 8798; condition: `none`; statement: `   28 WRITE (2,29)  I`
  - format:
```fortran
   29 FORMAT(' TBF - SEGMENT CONNECTION ERROR FOR SEGMENT',I5)
```

---
## TRIO segment connection errors
- Routine: `SUBROUTINE TRIO`
- Source lines: 8828-8875

### `WRITE (2,10)` (label 10)
  - line 8868; condition: `none`; statement: `    9 WRITE (2,10)  J`
  - format:
```fortran
   10 FORMAT(' TRIO - SEGMENT CONNENTION ERROR FOR SEGMENT',I5)
```

---