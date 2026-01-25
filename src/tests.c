/******************************************************************************
 * tests.c
 *
 * tests.c contains code for a number of sanity-checking routines that look
 * for problems in the structure of the deck as a whole, or commonly found
 * issues in the design.
 *
 * An example of the former is a series of CM cards but no CE, SM cards not
 * followed by an SC, or a tapered wire that's missing its GC card. The tests
 * are deliberately picky, it will report issues that won't actually cause
 * problems in most systems. These can simply be ignored, but they are issues
 * that are generally easy to fix in the deck, and should be.
 *
 * Examples of problems in the actual data include more complex issues like
 * crossed wires or wires that cross the ground line, both of which will cause
 * the calculations to fail.
 *
 * Non-critical errors like a missing CE card will have a value of 0. Ones
 * that will cause evaluation to fail, like a missing GE, have a value of 1,
 * and critical errors like a missing file are 2. Users and applications can
 * add their own errors using these values, or any value less than 0.
 *
 *****************************************************************************/

#include "opennec.h"

// Local structs for structural geometry validations
typedef struct { int tag; int segs; int line; double x1,y1,z1,x2,y2,z2; double radius; } wire_info_t;
typedef struct { int line; int tag; int segStart; int segEnd; } ref_info_t;
typedef struct { int line; int tag1; int seg1; int tag2; int seg2; } tl_ref_t;

// Forward declarations for helper functions
static double point_to_segment_distance(double px, double py, double pz,
                                        double qx1, double qy1, double qz1,
                                        double qx2, double qy2, double qz2);
static void check_parallel_wire_segmentation(nec_context_t *ctx, errors_list_t *errors,
                                             const wire_info_t *wires, int wire_count,
                                             double freq_mhz);
static void check_segment_length_and_radius(nec_context_t *ctx, errors_list_t *errors,
                                            const wire_info_t *wires, int wire_count,
                                            double freq_mhz, int ek_enabled);
static void check_ge_low_height_hazard(nec_context_t *ctx, errors_list_t *errors,
                                       const wire_info_t *wires, int wire_count,
                                       int GEType);
static void check_junction_segmentation_consistency(nec_context_t *ctx, errors_list_t *errors,
                                                    const wire_info_t *wires, int wire_count);

/******************************************************************************
 * test_deck_structure
 *
 * test_deck_structure runs various tests on the deck and returns a list of
 * errors and warnings. This looks only for structure problems, like missing
 * or duplicated cards, it does not look for logical problems or missing data
 * that's handled in other functions.
 *
 * @param deck the deck_t to be tested
 * @param errors the errors_list_t to add new messages to
 *
 */
void test_deck_structure(nec_context_t *ctx, deck_t *deck, errors_list_t *errors)
{
  // A short list of the minimum structure is found in the 4nec2
  // documentation:
  //
  // zero or more CM (comment) cards
  // one CE (comment end) card
  // one or more GW (wire geometry) cards
  // one GE (geometry end) card
  // one or more FR (design frequency) cards
  // one or more EX (excitation point) cards
  // zero or one GN (ground condition) card
  // zero or more LD (loading) cards
  // one EN (end of file) card
  //
  // There are minor issues with this list:
  //
  // 1) some decks lack any comments, although we consider that fatal
  // 2) you don't need a GW card specifically, any geometry will do
  // 3) the EN is not really required, many decks lack it
  //
  // as a result, this code demands a minimum deck of five cards,
  // one comment, two geometry cards, an FX, and an EX.
  //
  // TODO: do you need an EX? what about transmission?
  //
  // There are also a number of additional tests performed
  // below for other issues like duplicates of cards that should
  // only exist once, cards in the wrong section of the deck, and
  // similar issues.

  // although these look like they should be bools, we use int
  // so we can report the card number where the duplicate was seen
  int sawCE = 0, sawGx = 0, sawGE = 0, sawEN = 0, sawGF = 0;
  int sawFR = 0, sawSC = 0, sawSP = 0, sawGN = 0, sawGD = 0;
  double freq_mhz = 0.0; // first FR base frequency for wavelength-based checks
  int ek_enabled = 0;
  int sawGS = 0, sawLD = 0, sawEX = 0, sawSY = 0;
  int pendingSM = 0; // track SM that must be immediately followed by SC
  int GEType = 0;
  // and some temps
  char *code, *last_code;
  char *msg = calloc(1, MAX_ERROR_LEN);
  // track geometry tags seen so far (simple fixed-size set)
  int geom_tags[512];
  int geom_tag_count = 0;
  // capture wire geometries for endpoint connectivity checks
  wire_info_t wires[512];
  int wire_count = 0;
  // capture EX/LD/TL references to validate after geometry collection
  ref_info_t ex_refs[512]; int ex_ref_count = 0;
  ref_info_t ld_refs[512]; int ld_ref_count = 0;
  tl_ref_t tl_refs[512]; int tl_ref_count = 0;
  
  // start with some obvious ones
  if(deck->num_cards == 0) {
    sprintf(msg, "The deck has no cards.");
    add_error(ctx, errors, msg, 2);  // this is a critical error, this deck will not process
    return;
  }
  if(deck->num_cards < 5) {
    sprintf(msg, "A deck has to have at least five cards; one or more comments, one or more Gx cards, a GE, an FX, and an EX.");
    add_error(ctx, errors, msg, 2);  // same here, there is no way this will calculate property
    return;
  }
  
  // make sure we can find all the required cards
  last_code = "";
  for(int i = 0; i < deck->num_cards; i++) {
    // cache this
    code = deck->cards[i].card_code;
    
    // start with the checks for the cards we *have* to have, while also looking for duplicates
    
    // it's legal to have multiple GS cards, but that might be confusing
    if(strcmp(code, "GS") == 0) {
      if(sawGS == FALSE) {
        sawGS = i;
      } else {
        sprintf(msg, "The card on line %d is a GS, but we already saw one on card %d. No single measurement type can be defined.", i, sawGS + 1);
        add_error(ctx, errors, msg, 0);  // this will calculate fine, so this is merely a warning
      }
    }
    // NOTE: nec4 does not require a CE or CM, but we'll demand them here for compatibility
    if(strcmp(code, "CE") == 0) {
      if(sawCE == FALSE) {
        sawCE = i;
      } else {
        sprintf(msg, "The card on line %d is a CE, but we already saw one on card %d.", i, sawCE + 1);
        add_error(ctx, errors, msg, 0);
      }
    }
    if(strcmp(code, "GE") == 0) {
      if(sawGE == FALSE) {
        sawGE = i;
        GEType = deck->cards[i].i[1];
        // GE should typically follow at least one geometry card
        if(sawGx == FALSE) {
          sprintf(msg, "The card on line %d is a GE, but no geometry cards were seen before it.", i + 1);
          add_error(ctx, errors, msg, 0);
        }
      } else {
        sprintf(msg, "The card on line %d is a GE, but we already saw one on card %d.", i, sawGE + 1);
        add_error(ctx, errors, msg, 0);
      }
    }
    if(strcmp(code, "EN") == 0) {
      if(sawEN == FALSE) {
        sawEN = i;
      } else {
        sprintf(msg, "The card on line %d is an EN, but we already saw one on card %d.", i, sawEN + 1);
        add_error(ctx, errors, msg, 0);
      }
    }
    
    // NOTE: does a deck really need a EX?
    
    // and also look for other cards where there can only be one
    if(strcmp(code, "GF") == 0) {
      if(sawGF == FALSE) {
        sawGF = i;
      } else {
        sprintf(msg, "The card on line %d is a GF, but we already saw one on card %d.", i, sawGF + 1);
        add_error(ctx, errors, msg, 0);
      }
    }
    if(strcmp(code, "FR") == 0) {
      if(sawFR == FALSE) {
        sawFR = i;
        if(freq_mhz == 0.0) {
          freq_mhz = deck->cards[i].f[1];
        }
      } else {
        sprintf(msg, "The card on line %d is an FR, but we already saw one on card %d.", i, sawFR + 1);
        add_error(ctx, errors, msg, 0);
      }
    }
    if(strcmp(code, "EK") == 0) {
      // Any non-zero I1 enables extended thin-wire kernel
      if(deck->cards[i].ints_used >= 1 && deck->cards[i].i[1] != 0) {
        ek_enabled = 1;
      }
    }

    // Warn if control cards appear before GE (except CE and cards with specific messages below)
    if(sawGE == FALSE) {
      if(is_control(&deck->cards[i]) && strcmp(code, "CE") != 0 &&
         strcmp(code, "EX") != 0 && strcmp(code, "TL") != 0 && strcmp(code, "LD") != 0 &&
         strcmp(code, "FR") != 0 && strcmp(code, "RP") != 0 && strcmp(code, "GN") != 0 && strcmp(code, "GD") != 0 && strcmp(code, "EK") != 0) {
        sprintf(msg, "The card on line %d is a %s, but it appears before the GE; control cards should follow geometry.", i + 1, code);
        add_error(ctx, errors, msg, 0);
      }
    }

    // Specific placement warnings for common control cards
    if(sawGE == FALSE) {
      if(strcmp(code, "EX") == 0) {
        sprintf(msg, "The card on line %d is an EX, but it appears before the GE; excitations should follow geometry.", i + 1);
        add_error(ctx, errors, msg, 0);
      }
      if(strcmp(code, "TL") == 0) {
        sprintf(msg, "The card on line %d is a TL, but it appears before the GE; transmission lines should follow geometry.", i + 1);
        add_error(ctx, errors, msg, 0);
      }
      if(strcmp(code, "LD") == 0) {
        sprintf(msg, "The card on line %d is an LD, but it appears before the GE; loading should follow geometry.", i + 1);
        add_error(ctx, errors, msg, 0);
      }
      if(strcmp(code, "FR") == 0) {
        sprintf(msg, "The card on line %d is an FR, but it appears before the GE; frequency setup should follow geometry.", i + 1);
        add_error(ctx, errors, msg, 0);
      }
      if(strcmp(code, "RP") == 0) {
        sprintf(msg, "The card on line %d is an RP, but it appears before the GE; pattern requests should follow geometry.", i + 1);
        add_error(ctx, errors, msg, 0);
      }
      if(strcmp(code, "GN") == 0) {
        sprintf(msg, "The card on line %d is a GN, but it appears before the GE; ground settings should follow geometry.", i + 1);
        add_error(ctx, errors, msg, 0);
      }
      if(strcmp(code, "GD") == 0) {
        sprintf(msg, "The card on line %d is a GD, but it appears before the GE; ground parameters should follow geometry.", i + 1);
        add_error(ctx, errors, msg, 0);
      }
      if(strcmp(code, "EK") == 0) {
        sprintf(msg, "The card on line %d is an EK, but it appears before the GE; EK should follow geometry to apply the extended kernel.", i + 1);
        add_error(ctx, errors, msg, 0);
      }
    }

    // GE: optional I1 in {-1,0,1,2}; no floats expected
    if(strcmp(code, "GE") == 0) {
      if(deck->cards[i].ints_used > 1) {
        sprintf(msg, "The card on line %d is a GE but has more than one integer input.", i);
        add_error(ctx, errors, msg, 0);
      }
      int gei1 = deck->cards[i].i[1];
      if(!(gei1 == -1 || gei1 == 0 || gei1 == 1 || gei1 == 2)) {
        sprintf(msg, "The card on line %d is a GE with I1=%d, which is outside the typical range {-1,0,1,2}.", i, gei1);
        add_error(ctx, errors, msg, 0);
      }
      if(deck->cards[i].flts_used > 0) {
        sprintf(msg, "The card on line %d is a GE but has floating-point inputs, which are not expected.", i);
        add_error(ctx, errors, msg, 0);
      }
    }

    // TL: expects 4 integer locators (tags/segments) and Z0 in F1
    if(strcmp(code, "TL") == 0) {
      if(deck->cards[i].ints_used < 4) {
        sprintf(msg, "The card on line %d is a TL but has fewer than 4 integer inputs (tag/segment locators).", i);
        add_error(ctx, errors, msg, 0);
      }
      // basic sanity: locators positive
      if(deck->cards[i].i[1] <= 0 || deck->cards[i].i[2] <= 0 || deck->cards[i].i[3] <= 0 || deck->cards[i].i[4] <= 0) {
        sprintf(msg, "The card on line %d is a TL with non-positive tag/segment locator(s).", i);
        add_error(ctx, errors, msg, 0);
      }
      if(deck->cards[i].flts_used < 1) {
        sprintf(msg, "The card on line %d is a TL but has no characteristic impedance in F1.", i);
        add_error(ctx, errors, msg, 0);
      } else if(deck->cards[i].f[1] == 0.0) {
        sprintf(msg, "The card on line %d is a TL with Z0 = 0 in F1, which is invalid.", i);
        add_error(ctx, errors, msg, 0);
      }
      // record TL endpoints for segment-bounds validation later
      if(deck->cards[i].ints_used >= 4 && tl_ref_count < (int)(sizeof(tl_refs)/sizeof(tl_refs[0]))) {
        tl_refs[tl_ref_count].line = i + 1;
        tl_refs[tl_ref_count].tag1 = deck->cards[i].i[1];
        tl_refs[tl_ref_count].seg1 = deck->cards[i].i[2];
        tl_refs[tl_ref_count].tag2 = deck->cards[i].i[3];
        tl_refs[tl_ref_count].seg2 = deck->cards[i].i[4];
        tl_ref_count++;
      }
    }

    // EX: typical voltage source requires 4 integers and non-zero amplitude in F1
    if(strcmp(code, "EX") == 0) {
      if(deck->cards[i].ints_used < 4) {
        sprintf(msg, "The card on line %d is an EX but has fewer than 4 integer inputs.", i);
        add_error(ctx, errors, msg, 0);
      }
      if(deck->cards[i].flts_used < 1) {
        sprintf(msg, "The card on line %d is an EX but has no amplitude in F1.", i);
        add_error(ctx, errors, msg, 0);
      } else if(deck->cards[i].f[1] == 0.0) {
        sprintf(msg, "The card on line %d is an EX with zero amplitude in F1.", i);
        add_error(ctx, errors, msg, 0);
      }
      // Basic locator sanity: tag and segment positive
      if(deck->cards[i].ints_used >= 3) {
        if(deck->cards[i].i[2] <= 0 || deck->cards[i].i[3] <= 0) {
          sprintf(msg, "The card on line %d is an EX with non-positive tag or segment locator.", i);
          add_error(ctx, errors, msg, 0);
        }
      }
      // Record for open-end placement validation later
      if(deck->cards[i].ints_used >= 3) {
        ref_info_t r = { i + 1, deck->cards[i].i[2], deck->cards[i].i[3], 0 };
        if(ex_ref_count < (int)(sizeof(ex_refs)/sizeof(ex_refs[0]))) ex_ref_count++;
        ex_refs[ex_ref_count - 1] = r;
      }
    }

    // GN: minimal check — at least the ground type integer should be present
    if(strcmp(code, "GN") == 0) {
      if(deck->cards[i].ints_used < 1) {
        sprintf(msg, "The card on line %d is a GN but has no integer ground type specified.", i);
        add_error(ctx, errors, msg, 0);
      }
    }

    // LD: loading — require type, tag, segment and at least one non-zero value
    if(strcmp(code, "LD") == 0) {
      // At minimum: I1 (type), I2 (tag), I3 (segment)
      if(deck->cards[i].ints_used < 3) {
        sprintf(msg, "The card on line %d is an LD but has fewer than 3 integer inputs (type, tag, segment).", i);
        add_error(ctx, errors, msg, 0);
      } else {
        int type = deck->cards[i].i[1];
        int tag  = deck->cards[i].i[2];
        int seg1 = deck->cards[i].i[3];
        int seg2 = deck->cards[i].i[4];
        if(type < -1) {
          sprintf(msg, "The card on line %d is an LD with unexpected type I1=%d.", i, type);
          add_error(ctx, errors, msg, 0);
        }
        if(tag <= 0 || seg1 <= 0) {
          sprintf(msg, "The card on line %d is an LD with non-positive tag or segment locator.", i);
          add_error(ctx, errors, msg, 0);
        }
        if(seg2 != 0 && seg2 < seg1) {
          sprintf(msg, "The card on line %d is an LD with end segment I4 < start segment I3.", i);
          add_error(ctx, errors, msg, 0);
        }
      }
      // Require at least one float and encourage non-zero values
      if(deck->cards[i].flts_used < 1) {
        sprintf(msg, "The card on line %d is an LD but has no floating-point load value (e.g., resistance).", i);
        add_error(ctx, errors, msg, 0);
      } else if(deck->cards[i].f[1] == 0.0 && deck->cards[i].f[2] == 0.0 && deck->cards[i].f[3] == 0.0) {
        sprintf(msg, "The card on line %d is an LD with zero load values (F1..F3 all zero).", i);
        add_error(ctx, errors, msg, 0);
      }
    }
    // Record for open-end placement validation later (only for LD cards)
    if(strcmp(code, "LD") == 0 && deck->cards[i].ints_used >= 3) {
      int segStart = deck->cards[i].i[3];
      int segEnd = deck->cards[i].i[4];
      ref_info_t r = { i + 1, deck->cards[i].i[2], segStart, segEnd };
      if(ld_ref_count < (int)(sizeof(ld_refs)/sizeof(ld_refs[0]))) ld_ref_count++;
      ld_refs[ld_ref_count - 1] = r;
    }
    
    // along with some others we want to keep track of
    
    // we want to see if there are any SY's at all
    if(strcmp(code, "SY") == 0) {
      sawSY = TRUE;
    }
    // you can have multiple GN cards, but only the last one is used for a given execution
    if(strcmp(code, "GN") == 0) {
      if(sawGN == FALSE) sawGN = i;
    }
    // you can have multiple GC cards, but there has to be a GN somewhere
    if(strcmp(code, "GD") == 0) {
      if(sawGD == FALSE) {
        sawGD = i;
      }
    }
    // you can have multiple SCs, but they have to follow a SP or SM
    if(strcmp(code, "SC") == 0) {
      if(sawSC == FALSE) sawSC = i;
    }
    if(strcmp(code, "RP") == 0) {
      // RP should generally follow FR; warn if FR not yet seen
      if(sawFR == FALSE) {
        sprintf(msg, "The card on line %d is an RP, but no FR has been seen earlier.", i + 1);
        add_error(ctx, errors, msg, 0);
      }
      if(sawSP == FALSE) sawSP = i;
    }
    // you need an EX or LD
    if(strcmp(code, "EX") == 0) {
      if(sawEX == FALSE) sawEX = i;
    }
    // you should have an EX?
    if(strcmp(code, "LD") == 0) {
      if(sawLD == FALSE) sawLD = i;
    }

    // geometry cards are a little harder because there are many of them
    for(int j = 0; j < NUM_GEOMETRY_CODES; j++) {
      if(strcmp(code, geometry_codes[j]) == 0 && strcmp(code, "GE") != 0) {
        if(sawGx == FALSE) {
          sawGx = i;
        }
        // there's no else in this case, multiple Gx cards are fine, however
        // we do have a potential problem when we find Gx cards after a GE
        if(sawGx > 0 && sawGE > 0) {
          sprintf(msg, "The card on line %d has geometry code %s, but we already saw the GE on card %d.", i, code, sawGE + 1);
          add_error(ctx, errors, msg, 1);
        }
        // record geometry tags seen (I1) for later reference by control cards
        if(deck->cards[i].i[1] > 0) {
          int tag_i = deck->cards[i].i[1];
          int found = 0;
          for(int t = 0; t < geom_tag_count; t++) {
            if(geom_tags[t] == tag_i) { found = 1; break; }
          }
          if(!found && geom_tag_count < (int)(sizeof(geom_tags)/sizeof(geom_tags[0]))) {
            geom_tags[geom_tag_count++] = tag_i;
          }
          // If this is a GW, capture its endpoints and segment count
          if(strcmp(code, "GW") == 0 && wire_count < (int)(sizeof(wires)/sizeof(wires[0]))) {
            wire_info_t w;
            w.tag = tag_i;
            w.segs = deck->cards[i].i[2];
            w.line = i + 1;
            w.x1 = deck->cards[i].f[1]; w.y1 = deck->cards[i].f[2]; w.z1 = deck->cards[i].f[3];
            w.x2 = deck->cards[i].f[4]; w.y2 = deck->cards[i].f[5]; w.z2 = deck->cards[i].f[6];
            w.radius = deck->cards[i].f[7];
            wires[wire_count++] = w;
          }
        }
        break;
      }
    } /* loop over geometry codes */
    
    // unique one here - it's possible to have any number of GS cards, but
    // it appears you can have multiple GS's, although why you would ever do that is unclear
    if(strcmp(code, "GS") == 0) {
      if(sawGS == FALSE) sawGS = i;
    }

    // now we look for card pairs, where one card has to follow another
    
    // GC cards have to follow GW cards
    if(strcmp(code, "GC") == 0 && strcmp(last_code, "GW") != 0) {
      sprintf(msg, "The card on line %d is a GC, but the card above it is not a GW.", i + 1);
      add_error(ctx, errors, msg, 1);
    }
    
    // FIXME: we should do this, but it has to be calculated first because it might be a formula or units
//    // GW cards with zero radius have to have a GC after it
//    if(strcmp(code, "GW") == 0 && deck->cards[i].f[7] == 0.0 && strcmp(deck->cards[i+1].card_code, "GC") != 0) {
//      sprintf(msg, "The card on line %d is a GW with a zero radius, but the card after it is not a GC.", i + 1);
//      add_error(errors, msg, 1);
//    }
    
    // GD cards have to follow GN cards
    if(strcmp(code, "GD") == 0 && strcmp(last_code, "GN") != 0) {
      sprintf(msg, "The card on line %d is a GD, but the card above it is not a GN.", i + 1);
      add_error(ctx, errors, msg, 1);
    }
    if(strcmp(code, "GN") == 0) {
      if(i + 1 >= deck->num_cards || strcmp(deck->cards[i+1].card_code, "GD") != 0) {
        sprintf(msg, "The card on line %d is a GN, but the card after it is not a GD.", i + 1);
        add_error(ctx, errors, msg, 1);
      }
    }

    // GF cards have to be the first item in the geometry section, which
    // means they must follow CE cards, or in an onec deck, an SY
    // FIXME: it could also follow onec comment cards, so this is somewhat complex
    if(strcmp(code, "GF") == 0 && !(strcmp(last_code, "CE") == 0 || strcmp(last_code, "SY") == 0)) {
      sprintf(msg, "The card on line %d is a GF, but the card above it is not a CE or SY.", i + 1);
      add_error(ctx, errors, msg, 1);
    }

    // Modifiers must follow normal geometry: GM/GR/GX after GA/GH/GW/SP/CW
    if(strcmp(code, "GM") == 0 || strcmp(code, "GR") == 0 || strcmp(code, "GX") == 0) {
      if(!(strcmp(last_code, "GA") == 0 || strcmp(last_code, "GH") == 0 || strcmp(last_code, "GW") == 0 ||
           strcmp(last_code, "SP") == 0 || strcmp(last_code, "CW") == 0)) {
        sprintf(msg, "The card on line %d is a %s, but the card above it is not a GA, GH, GW, SP or CW.", i + 1, code);
        add_error(ctx, errors, msg, 1);
      }
    }

    // SC cards have to follow an SP or SM, or another SC. this is not an exhaustive
    // test, it should really roll backward until it finds an SP or SM, but...
    if(strcmp(code, "SC") == 0 && !(strcmp(last_code, "SP") == 0 || strcmp(last_code, "SM") == 0 || strcmp(last_code, "SC") == 0)) {
      sprintf(msg, "The card on line %d is an SC, but the card above it is not an SP, SM or another SC.", i + 1);
      add_error(ctx, errors, msg, 1);
    }
    // If SC follows another SC, ensure there was an SP or SM earlier in the deck
    if(strcmp(code, "SC") == 0 && strcmp(last_code, "SC") == 0) {
      int hasAncestor = 0;
      for(int j = i - 2; j >= 0; j--) {
        if(strcmp(deck->cards[j].card_code, "SP") == 0 || strcmp(deck->cards[j].card_code, "SM") == 0) { hasAncestor = 1; break; }
        // Stop if we reach a GE; SC ancestry should be within geometry section
        if(strcmp(deck->cards[j].card_code, "GE") == 0) break;
      }
      if(!hasAncestor) {
        sprintf(msg, "The card on line %d is an SC following an SC, but no earlier SP or SM was found before this SC chain.", i + 1);
        add_error(ctx, errors, msg, 1);
      }
    }
    // SM cards should follow an SP or another SM
    if(strcmp(code, "SM") == 0 && !(strcmp(last_code, "SP") == 0 || strcmp(last_code, "SM") == 0)) {
      sprintf(msg, "The card on line %d is an SM, but the card above it is not an SP or another SM.", i + 1);
      add_error(ctx, errors, msg, 1);
    }
    // SM must be immediately followed by SC (set pending on SM, clear on next non-SC)
    if(strcmp(code, "SM") == 0) {
      pendingSM = i + 1; // store 1-based line number of SM
    } else if(pendingSM) {
      if(strcmp(code, "SC") != 0) {
        sprintf(msg, "The card on line %d is an SM, but the card after it is not an SC.", pendingSM);
        add_error(ctx, errors, msg, 1);
      }
      pendingSM = 0;
    }
    
    // QUESTION: it appears GR cards have to follow some other sort of geometry card, but it's not clear what
    
    // FR cards have to have either one input or three
    if(strcmp(code, "FR") == 0) {
      if(!(deck->cards[i].ints_used == 1 || deck->cards[i].ints_used == 3)) {
        sprintf(msg, "The card on line %d is an FR but does not have 1 or 3 integer inputs.", i + 1);
        add_error(ctx, errors, msg, 0);
      }
    }
    // advance last_code to current card for next-iteration pair checks
    last_code = code;

    // After recording last_code, validate that certain control cards reference existing geometry tags seen so far
    if(strcmp(code, "EX") == 0 && deck->cards[i].ints_used >= 3) {
      int tag = deck->cards[i].i[2];
      if(tag > 0) {
        int seen = 0;
        for(int t = 0; t < geom_tag_count; t++) { if(geom_tags[t] == tag) { seen = 1; break; } }
        if(!seen) {
          sprintf(msg, "The card on line %d is an EX referencing tag %d, which was not found in prior geometry.", i + 1, tag);
          add_error(ctx, errors, msg, 0);
        }
      }
    }
    if(strcmp(code, "TL") == 0 && deck->cards[i].ints_used >= 4) {
      int tag1 = deck->cards[i].i[1];
      int tag2 = deck->cards[i].i[3];
      if(tag1 > 0) {
        int seen1 = 0;
        for(int t = 0; t < geom_tag_count; t++) { if(geom_tags[t] == tag1) { seen1 = 1; break; } }
        if(!seen1) {
          sprintf(msg, "The TL on line %d references tag %d (first endpoint), which was not found in prior geometry.", i + 1, tag1);
          add_error(ctx, errors, msg, 0);
        }
      }
      if(tag2 > 0) {
        int seen2 = 0;
        for(int t = 0; t < geom_tag_count; t++) { if(geom_tags[t] == tag2) { seen2 = 1; break; } }
        if(!seen2) {
          sprintf(msg, "The TL on line %d references tag %d (second endpoint), which was not found in prior geometry.", i + 1, tag2);
          add_error(ctx, errors, msg, 0);
        }
      }
    }
    if(strcmp(code, "LD") == 0 && deck->cards[i].ints_used >= 3) {
      int tag = deck->cards[i].i[2];
      if(tag > 0) {
        int seen = 0;
        for(int t = 0; t < geom_tag_count; t++) { if(geom_tags[t] == tag) { seen = 1; break; } }
        if(!seen) {
          sprintf(msg, "The card on line %d is an LD referencing tag %d, which was not found in prior geometry.", i + 1, tag);
          add_error(ctx, errors, msg, 0);
        }
      }
    }
  } /* loop over cards */

  // Validate EX/LD not located at open wire ends
  for(int wi = 0; wi < wire_count; wi++) {
    wire_info_t w = wires[wi];
    int end1_connected = 0, end2_connected = 0;
    for(int wj = 0; wj < wire_count; wj++) {
      if(wj == wi) continue;
      wire_info_t v = wires[wj];
      if((w.x1 == v.x1 && w.y1 == v.y1 && w.z1 == v.z1) || (w.x1 == v.x2 && w.y1 == v.y2 && w.z1 == v.z2)) {
        end1_connected = 1;
      }
      if((w.x2 == v.x1 && w.y2 == v.y1 && w.z2 == v.z1) || (w.x2 == v.x2 && w.y2 == v.y2 && w.z2 == v.z2)) {
        end2_connected = 1;
      }
    }
    // Check EX references to this wire
    for(int er = 0; er < ex_ref_count; er++) {
      if(ex_refs[er].tag == w.tag) {
        int seg = ex_refs[er].segStart;
        if(seg == 1 && w.segs > 1 && !end1_connected) {
          sprintf(msg, "The EX on line %d is placed on segment 1 of tag %d, which is an open wire end.", ex_refs[er].line, w.tag);
          add_error(ctx, errors, msg, 0);
        }
        if(seg == w.segs && w.segs > 1 && !end2_connected) {
          sprintf(msg, "The EX on line %d is placed on segment %d of tag %d, which is an open wire end.", ex_refs[er].line, w.segs, w.tag);
          add_error(ctx, errors, msg, 0);
        }
      }
    }
    // Check LD references to this wire (consider start/end segments)
    for(int lr = 0; lr < ld_ref_count; lr++) {
      if(ld_refs[lr].tag == w.tag) {
        int s = ld_refs[lr].segStart;
        int e = ld_refs[lr].segEnd;
        if(s == 1 && w.segs > 1 && !end1_connected) {
          sprintf(msg, "The LD on line %d starts at segment 1 of tag %d, which is an open wire end.", ld_refs[lr].line, w.tag);
          add_error(ctx, errors, msg, 0);
        }
        if(e == 0) e = s; // single-segment load
        if(e == w.segs && w.segs > 1 && !end2_connected) {
          sprintf(msg, "The LD on line %d ends at segment %d of tag %d, which is an open wire end.", ld_refs[lr].line, w.segs, w.tag);
          add_error(ctx, errors, msg, 0);
        }
      }
    }
  }

  // Parallel wire segmentation check (0.05 wavelengths threshold)
  if(freq_mhz > 0.0 && wire_count > 1) {
    check_parallel_wire_segmentation(ctx, errors, wires, wire_count, freq_mhz);
  }
  if(freq_mhz > 0.0 && wire_count > 0) {
    check_segment_length_and_radius(ctx, errors, wires, wire_count, freq_mhz, ek_enabled);
  }
  if(wire_count > 0) {
    check_ge_low_height_hazard(ctx, errors, wires, wire_count, GEType);
    check_junction_segmentation_consistency(ctx, errors, wires, wire_count);
  }

  // TL segment bounds validation: ensure referenced segments exist on the given tags
  if(tl_ref_count > 0 && wire_count > 0) {
    for(int r = 0; r < tl_ref_count; r++) {
      int segs1 = -1, segs2 = -1;
      for(int w = 0; w < wire_count; w++) {
        if(wires[w].tag == tl_refs[r].tag1) segs1 = wires[w].segs;
        if(wires[w].tag == tl_refs[r].tag2) segs2 = wires[w].segs;
      }
      if(segs1 > 0 && (tl_refs[r].seg1 <= 0 || tl_refs[r].seg1 > segs1)) {
        sprintf(msg, "The TL on line %d references an invalid segment on the first endpoint; segment index out of range for the wire tag %d.", tl_refs[r].line, tl_refs[r].tag1);
        add_error(ctx, errors, msg, 0);
      }
      if(segs2 > 0 && (tl_refs[r].seg2 <= 0 || tl_refs[r].seg2 > segs2)) {
        sprintf(msg, "The TL on line %d references an invalid segment on the second endpoint; segment index out of range for the wire tag %d.", tl_refs[r].line, tl_refs[r].tag2);
        add_error(ctx, errors, msg, 0);
      }
      // TL self-loop: same tag+segment on both ends
      if(tl_refs[r].tag1 == tl_refs[r].tag2 && tl_refs[r].seg1 == tl_refs[r].seg2) {
        sprintf(msg, "The TL on line %d connects the same tag and segment on both ends; this is a no-op or invalid connection.", tl_refs[r].line);
        add_error(ctx, errors, msg, 0);
      }
    }
  }

  // LD segment bounds validation: ensure referenced segment range falls within the wire's segment count
  if(ld_ref_count > 0 && wire_count > 0) {
    for(int r = 0; r < ld_ref_count; r++) {
      int segs = -1;
      for(int w = 0; w < wire_count; w++) {
        if(wires[w].tag == ld_refs[r].tag) { segs = wires[w].segs; break; }
      }
      if(segs > 0) {
        int s = ld_refs[r].segStart;
        int e = ld_refs[r].segEnd == 0 ? ld_refs[r].segStart : ld_refs[r].segEnd;
        if(s <= 0 || s > segs) {
          sprintf(msg, "The LD on line %d references an invalid start segment; out of range for wire tag %d.", ld_refs[r].line, ld_refs[r].tag);
          add_error(ctx, errors, msg, 0);
        }
        if(e <= 0 || e > segs) {
          sprintf(msg, "The LD on line %d references an invalid end segment; out of range for wire tag %d.", ld_refs[r].line, ld_refs[r].tag);
          add_error(ctx, errors, msg, 0);
        }
      }
    }
  }

  // Duplicate wire endpoints: overlapping wires
  for(int i = 0; i < wire_count; i++) {
    for(int j = i + 1; j < wire_count; j++) {
      wire_info_t a = wires[i];
      wire_info_t b = wires[j];
      int same_dir = (a.x1==b.x1 && a.y1==b.y1 && a.z1==b.z1 && a.x2==b.x2 && a.y2==b.y2 && a.z2==b.z2);
      int reversed = (a.x1==b.x2 && a.y1==b.y2 && a.z1==b.z2 && a.x2==b.x1 && a.y2==b.y1 && a.z2==b.z1);
      if(same_dir || reversed) {
        sprintf(msg, "The wire on line %d (tag %d) has the same endpoints as the wire on line %d (tag %d). Overlapping wires can cause issues.", a.line, a.tag, b.line, b.tag);
        add_error(ctx, errors, msg, 0);
      }
    }
  }

  // Ground intersection: warn if wires cross or extend below z=0 when ground is enabled
  int ground_enabled = ((GEType == 1 || GEType == 2) || sawGN);
  if(ground_enabled) {
    for(int wi = 0; wi < wire_count; wi++) {
      wire_info_t w = wires[wi];
      double z1 = w.z1, z2 = w.z2;
      if(z1 < 0.0 && z2 < 0.0) {
        sprintf(msg, "The wire on line %d (tag %d) lies below the ground plane (z<0).", w.line, w.tag);
        add_error(ctx, errors, msg, 0);
      } else if(z1 * z2 < 0.0) {
        sprintf(msg, "The wire on line %d (tag %d) crosses the ground plane (z=0), which is typically invalid.", w.line, w.tag);
        add_error(ctx, errors, msg, 0);
      }
    }
  }

  // Wires that are connected must contact at segment ends (connection separation < len/1000)
  for(int ai = 0; ai < wire_count; ai++) {
    wire_info_t a = wires[ai];
    double ax[2] = {a.x1, a.x2};
    double ay[2] = {a.y1, a.y2};
    double az[2] = {a.z1, a.z2};
    for(int end = 0; end < 2; end++) {
      double px = ax[end], py = ay[end], pz = az[end];
      for(int bi = 0; bi < wire_count; bi++) {
        if(bi == ai) continue;
        wire_info_t b = wires[bi];
        double vx = b.x2 - b.x1, vy = b.y2 - b.y1, vz = b.z2 - b.z1;
        double L2 = vx*vx + vy*vy + vz*vz;
        if(L2 == 0.0 || b.segs <= 0) continue;
        double L = sqrt(L2);
        double segLen = L / (double)b.segs;
        double tol = segLen / 1000.0;
        // project point onto line b
        double wx = px - b.x1, wy = py - b.y1, wz = pz - b.z1;
        double t = (wx*vx + wy*vy + wz*vz) / L2;
        if(t < 0.0 || t > 1.0) continue; // closest point lies outside the wire extent
        double fx = b.x1 + t*vx, fy = b.y1 + t*vy, fz = b.z1 + t*vz;
        double dx = px - fx, dy = py - fy, dz = pz - fz;
        double dist = sqrt(dx*dx + dy*dy + dz*dz);
        if(dist <= tol) {
          // near-connected; ensure this footpoint is at a segment endpoint of b
          int isEndpoint = 0;
          for(int k = 0; k <= b.segs; k++) {
            double ek = (double)k / (double)b.segs;
            double ex = b.x1 + ek*vx, ey = b.y1 + ek*vy, ez = b.z1 + ek*vz;
            double edx = fx - ex, edy = fy - ey, edz = fz - ez;
            double ed = sqrt(edx*edx + edy*edy + edz*edz);
            if(ed <= tol) { isEndpoint = 1; break; }
          }
          if(!isEndpoint) {
            sprintf(msg, "The wire on line %d (tag %d) endpoint is connected near the middle of the wire on line %d (tag %d). Connections must be at segment ends.", a.line, a.tag, b.line, b.tag);
            add_error(ctx, errors, msg, 0);
          }
        }
      }
    }
  }
  
  // and with the entire deck tested, make sure we got the key cards
  if(!sawCE) {
    sprintf(msg, "A NEC-2 deck should have a CE card.");
    add_error(ctx, errors, msg, 0);
  }
  if(!sawGx) {
    sprintf(msg, "A deck has to have at least one geometry card.");
    add_error(ctx, errors, msg, 1);
  }
  if(!sawGE) {
    sprintf(msg, "A deck has to have a GE card.");
    add_error(ctx, errors, msg, 1);
  }
  if(!sawFR) {
    sprintf(msg, "A deck has to have an FR card.");
    add_error(ctx, errors, msg, 1);
  }
  if(!sawEN) {
    sprintf(msg, "A deck should end with a EN card.");
    add_error(ctx, errors, msg, 0);
  }
  // EN should be the last card when present
  if(sawEN && sawEN != deck->num_cards - 1) {
    sprintf(msg, "The EN card should be the final card in the deck (found earlier at card %d).", sawEN + 1);
    add_error(ctx, errors, msg, 0);
  }
  if(!sawEX && !sawLD) {
    sprintf(msg, "A deck has to have at least one EX or LD card.");
    add_error(ctx, errors, msg, 1);
  }
  if(sawSY && !sawCE) {
    sprintf(msg, "We found SY cards in the deck, but there is no CE in the deck. SYs should follow the CE.");
    add_error(ctx, errors, msg, 0);
  }
  // Warn if GD appears without any preceding GN
  if(sawGD && !sawGN) {
    sprintf(msg, "A GD card appears in the deck, but there is no GN card.");
    add_error(ctx, errors, msg, 1);
  }
  
  // if the GE card was -1, there has to be a GN
  if(sawGE && GEType == -1 && !sawGN) {
    sprintf(msg, "The GE is set to -1, but there is no GN card in the deck.");
    add_error(ctx, errors, msg, 1);
  }
  
  // and get rid of the local string
  free(msg);
}

// Helper: point-to-segment distance in 3D
static double point_to_segment_distance(double px, double py, double pz,
                                        double qx1, double qy1, double qz1,
                                        double qx2, double qy2, double qz2)
{
  double vx = qx2 - qx1, vy = qy2 - qy1, vz = qz2 - qz1;
  double L2 = vx*vx + vy*vy + vz*vz;
  if(L2 == 0.0) return 1e9;
  double wx = px - qx1, wy = py - qy1, wz = pz - qz1;
  double t = (wx*vx + wy*vy + wz*vz) / L2;
  if(t < 0.0) t = 0.0; else if(t > 1.0) t = 1.0;
  double fx = qx1 + t*vx, fy = qy1 + t*vy, fz = qz1 + t*vz;
  double dx = px - fx, dy = py - fy, dz = pz - fz;
  return sqrt(dx*dx + dy*dy + dz*dz);
}

// Helper: warn if parallel wires closer than 0.05 wavelengths have different segmentation
static void check_parallel_wire_segmentation(nec_context_t *ctx, errors_list_t *errors,
                                             const wire_info_t *wires, int wire_count,
                                             double freq_mhz)
{
  char *msg = calloc(1, MAX_ERROR_LEN);
  double wlam_m = (freq_mhz > 0.0) ? (CVEL / freq_mhz) : 0.0; // CVEL in m/us vs MHz => meters
  if(wlam_m <= 0.0) { free(msg); return; }
  double thr = 0.05 * wlam_m;

  for(int i = 0; i < wire_count; i++) {
    for(int j = i + 1; j < wire_count; j++) {
      const wire_info_t *a = &wires[i];
      const wire_info_t *b = &wires[j];
      // direction vectors
      double ax = a->x2 - a->x1, ay = a->y2 - a->y1, az = a->z2 - a->z1;
      double bx = b->x2 - b->x1, by = b->y2 - b->y1, bz = b->z2 - b->z1;
      double al = sqrt(ax*ax + ay*ay + az*az);
      double bl = sqrt(bx*bx + by*by + bz*bz);
      if(al == 0.0 || bl == 0.0) continue;
      // unit direction vectors
      ax /= al; ay /= al; az /= al;
      bx /= bl; by /= bl; bz /= bl;
      // parallel if |cross| small or |dot| close to 1
      double cx = ay*bz - az*by;
      double cy = az*bx - ax*bz;
      double cz = ax*by - ay*bx;
      double cross_mag = sqrt(cx*cx + cy*cy + cz*cz);
      double dot = ax*bx + ay*by + az*bz;
      if(cross_mag > 1e-3 && fabs(dot) < 0.999) continue; // not parallel enough

      // minimal distance between segments (approx): sample endpoints to other segment lines
      // point-to-line distance from a->x1 to b, and a->x2 to b, and vice versa; take min
      double min_dist = thr * 10.0; // init larger than thr
      double d1 = point_to_segment_distance(a->x1, a->y1, a->z1, b->x1, b->y1, b->z1, b->x2, b->y2, b->z2);
      double d2 = point_to_segment_distance(a->x2, a->y2, a->z2, b->x1, b->y1, b->z1, b->x2, b->y2, b->z2);
      double d3 = point_to_segment_distance(b->x1, b->y1, b->z1, a->x1, a->y1, a->z1, a->x2, a->y2, a->z2);
      double d4 = point_to_segment_distance(b->x2, b->y2, b->z2, a->x1, a->y1, a->z1, a->x2, a->y2, a->z2);
      min_dist = fmin(fmin(d1, d2), fmin(d3, d4));

      if(min_dist < thr) {
        double segA = al / (double)a->segs;
        double segB = bl / (double)b->segs;
        // different segmentation: either segment counts differ, or segment lengths differ >10%
        double rel = fabs(segA - segB) / fmax(segA, segB);
        if(a->segs != b->segs || rel > 0.10) {
          sprintf(msg, "Parallel wires (tags %d and %d; lines %d and %d) are closer than 0.05 wavelengths but have different segmentation.", a->tag, b->tag, a->line, b->line);
          add_error(ctx, errors, msg, 0);
        }
      }
    }
  }
  free(msg);
}

// Helper: junction segmentation consistency — connected wire endpoints should have similar segment lengths
static void check_junction_segmentation_consistency(nec_context_t *ctx, errors_list_t *errors,
                                                    const wire_info_t *wires, int wire_count)
{
  char *msg = calloc(1, MAX_ERROR_LEN);
  for(int i = 0; i < wire_count; i++) {
    const wire_info_t *a = &wires[i];
    if(a->segs <= 0) continue;
    double aL = sqrt(pow(a->x2 - a->x1, 2) + pow(a->y2 - a->y1, 2) + pow(a->z2 - a->z1, 2));
    double aSeg = aL / (double)a->segs;
    for(int j = i + 1; j < wire_count; j++) {
      const wire_info_t *b = &wires[j];
      if(b->segs <= 0) continue;
      double bL = sqrt(pow(b->x2 - b->x1, 2) + pow(b->y2 - b->y1, 2) + pow(b->z2 - b->z1, 2));
      double bSeg = bL / (double)b->segs;
      double tol = fmax(fmin(aSeg, bSeg) / 1000.0, 1e-9);
      // check direct endpoint-to-endpoint distances for proximity
      double d11 = sqrt(pow(a->x1 - b->x1, 2) + pow(a->y1 - b->y1, 2) + pow(a->z1 - b->z1, 2));
      double d12 = sqrt(pow(a->x1 - b->x2, 2) + pow(a->y1 - b->y2, 2) + pow(a->z1 - b->z2, 2));
      double d21 = sqrt(pow(a->x2 - b->x1, 2) + pow(a->y2 - b->y1, 2) + pow(a->z2 - b->z1, 2));
      double d22 = sqrt(pow(a->x2 - b->x2, 2) + pow(a->y2 - b->y2, 2) + pow(a->z2 - b->z2, 2));
      int connected = (d11 <= tol) || (d12 <= tol) || (d21 <= tol) || (d22 <= tol);
      if(connected) {
        double rel = fabs(aSeg - bSeg) / fmax(aSeg, bSeg);
        if(rel > 0.20) {
          sprintf(msg, "Connected wires (tags %d and %d; lines %d and %d) have very different segment lengths near the junction (%.4g m vs %.4g m); consider harmonizing segmentation.", a->tag, b->tag, a->line, b->line, aSeg, bSeg);
          add_error(ctx, errors, msg, 0);
        }
      }
    }
  }
  free(msg);
}

// Helper: GE I1=1 connects segment ends if wire height < 1e-3 * segment length; suggest GE -1
static void check_ge_low_height_hazard(nec_context_t *ctx, errors_list_t *errors,
                                       const wire_info_t *wires, int wire_count,
                                       int GEType)
{
  if(GEType != 1) return;
  char *msg = calloc(1, MAX_ERROR_LEN);
  for(int i = 0; i < wire_count; i++) {
    const wire_info_t *w = &wires[i];
    if(w->segs <= 0) continue;
    double L = sqrt(pow(w->x2 - w->x1, 2) + pow(w->y2 - w->y1, 2) + pow(w->z2 - w->z1, 2));
    double segLen = L / (double)w->segs;
    double dz = fabs(w->z2 - w->z1);
    double h = fmin(fabs(w->z1), fabs(w->z2));
    if(dz < 1e-9 && h < (1e-3 * segLen)) {
      sprintf(msg, "With GE I1=1, the wire on line %d (tag %d) has height %.4g m < 1e-3 x segment length (%.4g m); segment ends may be connected to ground. Consider GE -1.", w->line, w->tag, h, segLen);
      add_error(ctx, errors, msg, 0);
    }
  }
  free(msg);
}

// Helper: segment length vs wavelength and radius sanity
static void check_segment_length_and_radius(nec_context_t *ctx, errors_list_t *errors,
                                            const wire_info_t *wires, int wire_count,
                                            double freq_mhz, int ek_enabled)
{
  char *msg = calloc(1, MAX_ERROR_LEN);
  double wlam_m = (freq_mhz > 0.0) ? (CVEL / freq_mhz) : 0.0;
  if(wlam_m <= 0.0) { free(msg); return; }

  for(int i = 0; i < wire_count; i++) {
    const wire_info_t *w = &wires[i];
    double L = sqrt(pow(w->x2 - w->x1, 2) + pow(w->y2 - w->y1, 2) + pow(w->z2 - w->z1, 2));
    if(w->segs > 0) {
      double segLen = L / (double)w->segs;
      double segFrac = segLen / wlam_m; // in wavelengths
      if(segFrac >= 0.10) {
        sprintf(msg, "The wire on line %d (tag %d) has segment length %.4g m (%.4g lambda) which is >= 0.1 lambda; consider increasing segmentation.", w->line, w->tag, segLen, segFrac);
        add_error(ctx, errors, msg, 0);
      } else if(segFrac >= 0.05) {
        sprintf(msg, "The wire on line %d (tag %d) has segment length %.4g m (%.4g lambda); in critical regions, aim for < 0.05 lambda.", w->line, w->tag, segLen, segFrac);
        add_error(ctx, errors, msg, 0);
      }
      if(segFrac < 0.001) {
        sprintf(msg, "The wire on line %d (tag %d) has very small segment length %.4g m (%.4g lambda); this may cause excessive segmentation.", w->line, w->tag, segLen, segFrac);
        add_error(ctx, errors, msg, 0);
      }
      // radius sanity relative to segment length
      if(w->radius > 0.0) {
        if(ek_enabled) {
          if(w->radius >= (2.0 * segLen)) {
            sprintf(msg, "The wire on line %d (tag %d) has radius %.4g m >= 2*len (%.4g m) with extended kernel; reduce radius or increase segmentation.", w->line, w->tag, w->radius, 2.0*segLen);
            add_error(ctx, errors, msg, 0);
          }
        } else {
          if(w->radius >= (segLen / 2.0)) {
            sprintf(msg, "The wire on line %d (tag %d) has radius %.4g m >= len/2 (%.4g m); this violates thin-wire assumptions.", w->line, w->tag, w->radius, segLen/2.0);
            add_error(ctx, errors, msg, 0);
          } else if(w->radius >= (segLen / 10.0)) {
            sprintf(msg, "The wire on line %d (tag %d) has radius %.4g m; typical thin-wire usage prefers radius < len/10 (%.4g m).", w->line, w->tag, w->radius, segLen/10.0);
            add_error(ctx, errors, msg, 0);
          }
        }
      }
    }
  }
  free(msg);
}


/*******************************************************************
 * test_duplicate_tags
 *
 * test_duplicate_tags checks to see if there is more than one card
 * with the same tag on it. this will not notice problems if there
 * is a GM or similar card that creates new tags, that only happens
 * when the geometry is segmented
 *
 * @param deck the deck_t to be tested
 * @param errors the errors_list_t to add new messages to
 *
 */
void test_duplicate_tags(nec_context_t *ctx, deck_t *deck, errors_list_t *errors)
{
  // we will also check to see if there are duplicate tags
  char *msg = calloc(1, MAX_ERROR_LEN);
  
  // Only consider duplicates within the geometry section
  int gstart = deck->geometry_start;
  int gend = deck->geometry_end; // index of GE card
  if (gstart < 0 || gend < 0 || gend <= gstart) {
    // Fallback: search all cards but restrict to geometry types for both sides
    gstart = 0;
    gend = deck->num_cards;
  }

  // now check if there are any duplicate tags in the geometry
  // NOTE: this doesn't test for new tags generated by GM or similar
  for(int i = gstart; i < gend; i++) {
    if(is_geometry(&deck->cards[i]) && deck->cards[i].i[1] > 0) {
      int tag_i = deck->cards[i].i[1];
      for(int j = i + 1; j < gend; j++) {
        if(is_geometry(&deck->cards[j]) && deck->cards[j].i[1] > 0) {
          if(deck->cards[j].i[1] == tag_i) {
            sprintf(msg, "The tag number %d is found on card %d and card %d.", tag_i, i + 1, j + 1);
            add_error(ctx, errors, msg, 1);
          }
        }
      }
    }
  }
  
  free(msg);
}
/* end test_duplicate_tags */

/******************************************************************************
 * test_card_inputs
 *
 * test_card_inputs looks at each card to ensure it has the right number
 * and type of inputs. For instance, an FR card has two forms; if I1 is 0
 * then it has to have no other values, if I1 is non-zero, it has to have
 * F1 and F2.
 *
 * @param deck the deck_t to be tested
 * @param errors the errors_list_t to add new messages to
 *
 * TODO: this needs to be greatly expanded!
 *
 */
void test_card_inputs(nec_context_t *ctx, deck_t *deck, errors_list_t *errors)
{
  char *code;
  char *msg = calloc(1, MAX_ERROR_LEN);

  for(int i = 0; i < deck->num_cards; i++) {
    code = deck->cards[i].card_code;

    // CE: comment end — should not have numeric inputs
    if(strcmp(code, "CE") == 0) {
      if(deck->cards[i].ints_used > 0 || deck->cards[i].flts_used > 0) {
        sprintf(msg, "The card on line %d is a CE but has numeric inputs.", i);
        add_error(ctx, errors, msg, 0);
      }
    }

    // EN: end of deck — should not have numeric inputs
    if(strcmp(code, "EN") == 0) {
      if(deck->cards[i].ints_used > 0 || deck->cards[i].flts_used > 0) {
        sprintf(msg, "The card on line %d is an EN but has numeric inputs.", i);
        add_error(ctx, errors, msg, 0);
      }
    }

    // FRs: allow single-frequency (I2=0) or stepped (I2>0)
    if(strcmp(code, "FR") == 0) {
      // there must be a value in F1
      if(deck->cards[i].f[1] == 0) {
        sprintf(msg, "The card on line %d is a FR but has no base frequency in F1.", i);
        add_error(ctx, errors, msg, 0);
      }
      // Single-frequency: I2==0 should have F2==0
      if(deck->cards[i].i[2] == 0 && deck->cards[i].f[2] != 0) {
        sprintf(msg, "The card on line %d is a FR with I2 = 0 (single frequency), but has a non-zero F2.", i);
        add_error(ctx, errors, msg, 0);
      }
      // Stepped: I2>0 requires positive step in F2
      else if(deck->cards[i].i[2] > 0 && deck->cards[i].f[2] <= 0) {
        sprintf(msg, "The card on line %d is a FR with I2 > 0 (stepped), but F2 is not a positive step.", i);
        add_error(ctx, errors, msg, 0);
      }
    }

    // GW: wire geometry — require positive segment count and radius
    if(strcmp(code, "GW") == 0) {
      // At least tag and segment count
      if(deck->cards[i].ints_used < 2) {
        sprintf(msg, "The card on line %d is a GW but has fewer than 2 integer inputs (tag, segments).", i);
        add_error(ctx, errors, msg, 0);
      } else {
        int segs = deck->cards[i].i[2];
        if(segs <= 0) {
          sprintf(msg, "The card on line %d is a GW with non-positive segment count.", i);
          add_error(ctx, errors, msg, 0);
        }
      }
      // Endpoints should not be identical (zero-length wire)
      double x1 = deck->cards[i].f[1], y1 = deck->cards[i].f[2], z1 = deck->cards[i].f[3];
      double x2 = deck->cards[i].f[4], y2 = deck->cards[i].f[5], z2 = deck->cards[i].f[6];
      if(x1 == x2 && y1 == y2 && z1 == z2) {
        sprintf(msg, "The card on line %d is a GW with identical endpoints (zero-length wire).", i);
        add_error(ctx, errors, msg, 0);
      }
      // Radius must be present and positive (F7)
      if(deck->cards[i].flts_used < 7) {
        sprintf(msg, "The card on line %d is a GW but has no radius specified in F7.", i);
        add_error(ctx, errors, msg, 0);
      } else if(deck->cards[i].f[7] <= 0.0) {
        sprintf(msg, "The card on line %d is a GW with non-positive radius in F7.", i);
        add_error(ctx, errors, msg, 0);
      }
      // If radius is zero, next card should be a GC with tapering info
      if(deck->cards[i].flts_used >= 7 && deck->cards[i].f[7] == 0.0) {
        if(i + 1 < deck->num_cards) {
          if(strcmp(deck->cards[i+1].card_code, "GC") != 0) {
            sprintf(msg, "The card on line %d is a GW with a zero radius, but the next card is not a GC.", i + 1);
            add_error(ctx, errors, msg, 1);
          }
        } else {
          sprintf(msg, "The card on line %d is a GW with a zero radius and is the last card; a following GC is required.", i + 1);
          add_error(ctx, errors, msg, 1);
        }
      }
    }

    // RP: radiation pattern — counts, steps, and basic range sanity
    if(strcmp(code, "RP") == 0) {
      // Typical RP uses at least 4 integers and 4 floats
      if(deck->cards[i].ints_used < 4) {
        sprintf(msg, "The card on line %d is an RP but has fewer than 4 integer inputs.", i);
        add_error(ctx, errors, msg, 0);
      }
      if(deck->cards[i].flts_used < 4) {
        sprintf(msg, "The card on line %d is an RP but has fewer than 4 floating-point inputs.", i);
        add_error(ctx, errors, msg, 0);
      }
      // Number of theta/phi points should be positive
      int ntheta = deck->cards[i].i[2];
      int nphi   = deck->cards[i].i[3];
      if(ntheta <= 0) {
        sprintf(msg, "The card on line %d is an RP with non-positive NTHETA (I2).", i);
        add_error(ctx, errors, msg, 0);
      }
      if(nphi <= 0) {
        sprintf(msg, "The card on line %d is an RP with non-positive NPHI (I3).", i);
        add_error(ctx, errors, msg, 0);
      }
      // Steps must be non-zero when requesting multiple points
      double th_start = deck->cards[i].f[1];
      double ph_start = deck->cards[i].f[2];
      double th_step  = deck->cards[i].f[3];
      double ph_step  = deck->cards[i].f[4];
      if(ntheta > 1 && th_step == 0.0) {
        sprintf(msg, "The card on line %d is an RP with NTHETA > 1 but zero theta step (F3).", i);
        add_error(ctx, errors, msg, 0);
      }
      if(nphi > 1 && ph_step == 0.0) {
        sprintf(msg, "The card on line %d is an RP with NPHI > 1 but zero phi step (F4).", i);
        add_error(ctx, errors, msg, 0);
      }
      // Basic angle sanity: starts within typical ranges
      if(!(th_start >= -180.0 && th_start <= 180.0)) {
        sprintf(msg, "The card on line %d is an RP with theta start (F1) outside [-180,180].", i);
        add_error(ctx, errors, msg, 0);
      }
      if(!(ph_start >= -360.0 && ph_start <= 360.0)) {
        sprintf(msg, "The card on line %d is an RP with phi start (F2) outside [-360,360].", i);
        add_error(ctx, errors, msg, 0);
      }
      // Step magnitudes should be reasonable
      if(fabs(th_step) > 180.0) {
        sprintf(msg, "The card on line %d is an RP with an excessively large theta step (F3).", i);
        add_error(ctx, errors, msg, 0);
      }
      if(fabs(ph_step) > 360.0) {
        sprintf(msg, "The card on line %d is an RP with an excessively large phi step (F4).", i);
        add_error(ctx, errors, msg, 0);
      }
      // Derived end angles should remain within sensible bounds and match step direction
      if(ntheta > 1) {
        double th_end = th_start + (ntheta - 1) * th_step;
        if(!(th_end >= -180.0 && th_end <= 180.0)) {
          sprintf(msg, "The card on line %d is an RP whose theta sweep leaves the [-180,180] range.", i);
          add_error(ctx, errors, msg, 0);
        }
        if(th_step > 0.0 && th_end < th_start) {
          sprintf(msg, "The card on line %d is an RP with NTHETA>1 and positive theta step but decreasing sweep.", i);
          add_error(ctx, errors, msg, 0);
        }
        if(th_step < 0.0 && th_end > th_start) {
          sprintf(msg, "The card on line %d is an RP with NTHETA>1 and negative theta step but increasing sweep.", i);
          add_error(ctx, errors, msg, 0);
        }
      }
      if(nphi > 1) {
        double ph_end = ph_start + (nphi - 1) * ph_step;
        if(!(ph_end >= -720.0 && ph_end <= 720.0)) {
          sprintf(msg, "The card on line %d is an RP whose phi sweep leaves a reasonable range.", i);
          add_error(ctx, errors, msg, 0);
        }
        if(ph_step > 0.0 && ph_end < ph_start) {
          sprintf(msg, "The card on line %d is an RP with NPHI>1 and positive phi step but decreasing sweep.", i);
          add_error(ctx, errors, msg, 0);
        }
        if(ph_step < 0.0 && ph_end > ph_start) {
          sprintf(msg, "The card on line %d is an RP with NPHI>1 and negative phi step but increasing sweep.", i);
          add_error(ctx, errors, msg, 0);
        }
      }
    }
  }
  
  free(msg);
}
/* end test_card_inputs */

/******************************************************************************
 * test_bad_symbols
 *
 * looks at all the SYmbol cards, if any, and warns if they override one of
 * the system-wide symbols like "mm" or "awg".
 *
 * also warns about duplicate definitions, as only the last value will be used
 * NOTE: is this correct? can you define HEIGHT=7 and then 14 lower in the deck?
 *
 * @param deck the deck_t to be tested
 * @param errors the errors_list_t to add new messages to
 *
 */
void test_bad_symbols(nec_context_t *ctx, deck_t *deck, errors_list_t *errors)
{
  key_value_t *outer, *inner;
  char *msg = calloc(1, MAX_ERROR_LEN);
  
  // first we'll check that they aren't overriding a measurement
  outer = deck->symbols;
  while(outer != NULL) {
    for(int i = 0; i < NUM_ONEC_UNIT_CODES; i++) {
      if(strcasecmp(outer->key, unit_codes[i]) == 0) {
        sprintf(msg, "The symbol '%s' has been defined and overrides a system-wide symbol of the same name.", unit_codes[i]);
        add_error(ctx, errors, msg, 0);
      }
    }
    // and now see if any other symbol has the same name
    // TODO: need to see if this is actually used, should SY's only be at the top or can they be redefined in the body?
    inner = outer->next;
    while(inner != NULL) {
      if(strcasecmp(outer->key, inner->key)  == 0) {
        sprintf(msg, "The symbol '%s' has been defined more than once.", outer->key);
        add_error(ctx, errors, msg, 0);
      }
    }
    outer = outer->next;
  } /* while loop over cards */
  
  free(msg);
} /* end of test_bad_symbols */

/* end of tests.c */
