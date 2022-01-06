/*******************************************************************
 * types.c
 *
 * types.c defines the various enums
 *
 *******************************************************************/
 
#include "types.h"

char *comment_codes[NUM_COMMENT_CODES] = {
  "CM", "CE", "!", "'", "#"
};

char *control_codes[NUM_CONTROL_CODES] = {
  "FR", "LD", "GN", "EX", "NT", "TL", \
  "XQ", "GD", "RP", "NX", "PT", "KH", \
  "NE", "NH", "PQ", "EK", "CP", "PL", \
  "EN", "WG"
};

// note that the continuation cards like GC are not here, but SC is,
// this is because you can have multiple SC's in a row so they need
// to show up in the list.
// FIXME: they don't, the SP handler should read forward until it
//  finds all the SCs
char *geometry_codes[NUM_GEOMETRY_CODES] = {
  "GW", "GX", "GR", "GS", "GE", "GM", \
  "SP", "SM", "GA", "SC", "GH", "GF"
};

// XT = "eXiT"     - from nec2c
// SY = "SY"mbol   - from 4nec2
// IT = "ITerate"  - new code, runs the output several times after changing SYs by a step
// OP = "OPtimize" - attempts to maximize or minimize a selected output value, like gain
char *onec_codes[NUM_ONEC_CODES] = {
  "XT", "SY", "IT", "OP"
};

// the unit_codes and unit_mult are in the same order,
// so if you change one, change the other!
char *unit_codes[NUM_UNIT_CODES] = {
  "", "m", "cm", "mm", "ft", "in", "ftin", "awg"
};
//// the last two units, ftin and awg, require special conversions
double unit_mult[NUM_UNIT_CODES] = {
  0, 1.0, 0.01, 0.001, 0.30480, 0.0254, 0, 0
};

