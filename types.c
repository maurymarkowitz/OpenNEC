/******************************************************************************
 * types.c
 *
 * types.c defines the various enums
 *
 *****************************************************************************/
 
#include "types.h"

// NOTE: ordering of these lists is important! they are used in
//       various places to convert the code back to a number for
//       a switch statement

char *field_names[NUM_FIELD_NAMES] = {
  "I1", "I2", "I3", "I4", "F1", "F2", "F3", "F4", "F5", "F6", "F7"
};

// MSM 2022-02-12 "#" turning off hash for now until we actually find it somehere
char *comment_codes[NUM_COMMENT_CODES] = {
  "CM", "CE", "!", "'", "#"
};

char *control_codes[NUM_CONTROL_CODES] = {
  "FR", "LD", "GN", "EX", "NT", "TL", \
  "XQ", "GD", "RP", "NX", "PT", "KH", \
  "NE", "NH", "PQ", "EK", "CP", "PL", \
  "EN", "WG"
};

// this list is from the original nec2c but has been expanded with
// some that nec2c left off. The original code didn't need something
// like GC because it only follow a GW, so instead of decoding the
// code into a number and handling it, it triggered the decode right
// in the GW section. In this code the reading and decoding is separate
// so we need every code in here somewhere.
char *geometry_codes[NUM_GEOMETRY_CODES] = {
  "GW", "GX", "GR", "GS", "GE", "GM", \
  "SP", "SM", "GA", "SC", "GH", "GF", "GC"
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
// type zero means "none", or default, instead of using
// -1 or something so that it is set properly on calloc
char *unit_codes[NUM_ONEC_UNIT_CODES] = {
  "", "m", "cm", "mm", "ft", "in", "ftin", "awg", "#"
};
// the last three units, ftin and awg, require special conversions
double unit_mult[NUM_ONEC_UNIT_CODES] = {
  0, 1.0, 0.01, 0.001, 0.30480, 0.0254, 0, 0, 0
};
