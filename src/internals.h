#ifndef OPENNEC_INTERNALS_H
#define OPENNEC_INTERNALS_H

#include "opennec.h"

/* common constants - Internal physics and grid parameters */
#define	POT		1.570796327
#define	PTP		.6283185308
#define	TPJ		(0.0+I*6.283185308)
#define PI8		25.13274123
#define PI10	31.41592654
#define FPI     12.56637062
#define	ETA		376.73
#define	RETA	2.654420938E-3
#define	TOSP	1.128379167
#define ACCS	1.E-12
#define	SP		1.772453851
#define	CCJ		(0.0-I*0.01666666667)
#define	CONST1	(0.0+I*4.771341189)
#define	CONST2	4.771341188
#define	CONST3	(0.0-I*29.97922085)
#define	CONST4	(0.0+I*188.365)
#define	GAMMA	.5772156649
#define C1		-.02457850915
#define C2		.3674669052
#define C3		.7978845608
#define P10		.0703125
#define P20		.1121520996
#define Q10		.125
#define Q20		.0732421875
#define P11		.1171875
#define P21		.1441955566
#define Q11		.375
#define Q21		.1025390625
#define POF		.7853981635
#define MAXH	20
#define CRIT	1.0E-4
#define NM		131072
#define NTS		4
#define	SMIN	1.e-3

/* Replaces the "10000" limit used to identiy segment/patch connections */
#define	PCHCON  100000

/* carriage return and line feed */
#define	CR	0x0d
#define	LF	0x0a

#endif
