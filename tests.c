/*******************************************************************
 * tests.c
 *
 * tests.c contains code for a number of santity-checking routines
 * that look for problems in the structure of the deck as a whole,
 * or commonly found issues in the design.
 *
 * An example of the former is a series of CM cards but no CE,
 * SM cards not following an SC, or a taper wire that's missing
 * its GC card. This set of tests is deliberately picky, it will
 * report issues that won't actually cause problems in most
 * systems. These can simply be ignored if not wanted, but they
 * are things that are easy to fix in the deck and should be.
 *
 * Examples of problems in the actual data include more complex
 * issues like crossed wires or wires that cross the ground line,
 * both of which will cause the calculations to fail.
 *
 * Non-critical errors like a missing CE card will have a value
 * of 0. Oes that will cause evaluation to fail, like a missing GE,
 * have a value of 1, and critical errors are 2. Users can add their
 * own errors using these values or any value less than 0.
 *
 *******************************************************************/

#include "opennec.h"
#include "shared.h"

/*----------------------------------------------------------------------*/

/** test_deck_structure runs various tests on the deck and returns a
 * list of errors and warnings.
 *
 */
void test_deck_structure(Deck *deck, Errors *errors) //, int level
{
  int sawCE = 0, sawGx = 0, sawGE = 0, sawEN = 0, sawGF = 0;
  int sawFR = 0, sawSC = 0, sawSP = 0, sawGN = 0, sawGD = 0;
  int sawGS = 0, sawLD = 0, sawEX = 0, sawSY = 0;
  char *code, *last_code;
  char *msg = calloc(1, MAX_ERROR_LEN);
  
  // start with some obvious ones
  if(deck->num_cards == 0) {
    sprintf(msg, "A deck has to have at least one card.");
    add_error(errors, msg, 2);  // this is a fatal error
    return;
  }
  if(deck->num_cards < 3) {
    sprintf(msg, "A deck has to have at least three cards; one or more Gx cards, a GE, and one or more EN or LD.");
    add_error(errors, msg, 0);
    return;
  }
  
  // now let's make sure we can find all the required cards
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
        sprintf(msg, "Card %d is a GS, but we already saw one on card %d. No single measurement type can be defined.", i, sawGS + 1);
        add_error(errors, msg, 0);
      }
    }
    // NOTE: nec4 does not require a CE or CM, but we'll demand them here for compatibility
    if(strcmp(code, "CE") == 0) {
      if(sawCE == FALSE) {
        sawCE = i;
      } else {
        sprintf(msg, "Card %d is a CE, but we already saw one on card %d.", i, sawCE + 1);
        add_error(errors, msg, 0);
      }
    }
    if(strcmp(code, "GE") == 0) {
      if(sawGE == FALSE) {
        sawGE = i;
      } else {
        sprintf(msg, "Card %d is a GE, but we already saw one on card %d.", i, sawGE + 1);
        add_error(errors, msg, 0);
      }
    }
    if(strcmp(code, "EN") == 0) {
      if(sawEN == FALSE) {
        sawEN = i;
      } else {
        sprintf(msg, "Card %d is an EN, but we already saw one on card %d.", i, sawEN + 1);
        add_error(errors, msg, 0);
      }
    }
    
    // NOTE: does a deck really need a EX?
    
    // and also look for other cards where there can only be one
    if(strcmp(code, "GF") == 0) {
      if(sawGF == FALSE) {
        sawGF = i;
      } else {
        sprintf(msg, "Card %d is an GF, but we already saw one on card %d.", i, sawGF + 1);
        add_error(errors, msg, 0);
      }
    }
    if(strcmp(code, "FR") == 0) {
      if(sawFR == FALSE) {
        sawFR = i;
      } else {
        sprintf(msg, "Card %d is an FR, but we already saw one on card %d.", i, sawFR + 1);
        add_error(errors, msg, 0);
      }
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
    // you can have multiple SCs, but they have to follow a SP
    if(strcmp(code, "SC") == 0) {
      if(sawSC == FALSE) sawSC = i;
    }
    if(strcmp(code, "SP") == 0) {
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
      if(strcmp(code, geometry_codes[j]) == 0 && !strcmp(code, "GE")) {
        if(sawGx == FALSE) {
          sawGx = i;
          break;
        }
        // there's no else in this case, multiple Gx cards are fine, however
        // we do have a potential problem when we find Gx cards after a GE
        if(sawGx > 0 && sawGE > 0) {
          sprintf(msg, "Card %d has geometry code %s, but we already saw the GE on card %d.", i, code, sawGE + 1);
          add_error(errors, msg, 1);
        }
      }
    } /* loop over geometry codes */
    
    // unique one here - it's possible to have any number of GS cards, but
    // it appears you can have multiple GS's, although why you would ever do that is unclear
    if(strcmp(code, "GS") == 0) {
      if(sawGS == FALSE) sawGS = i;
    }

    // now we look for card pairs, where one card has to follow another
    
    // GC cards have to follow GW cards
    if(strcmp(code, "GC") && !strcmp(last_code, "GW")) {
      sprintf(msg, "The card on line %d is a GC, but the line above it is not a GW.", i);
      add_error(errors, msg, 1);
    }
    
    // GD cards have to follow GN cards
    if(strcmp(code, "GD") && !strcmp(last_code, "GN")) {
      sprintf(msg, "The card on line %d is a GD, but the line above it is not a GN.", i);
      add_error(errors, msg, 1);
    }
    
    // GF cards have to be the first item in the geometry section,
    // which means they must follow CE cards, or in an onec deck, an SY
    if(strcmp(code, "GF") && !(strcmp(last_code, "CE") || strcmp(last_code, "SY"))) {
      sprintf(msg, "The card on line %d is a GF, but the line above it is not a CE or SY.", i);
      add_error(errors, msg, 1);
    }

    // SC cards have to follow an SP or SM, or another SC. this is not an exhaustive
    // test, it should really roll backward until it finds an SP or SM, but...
    if(strcmp(code, "SC") && !(strcmp(last_code, "SP") || strcmp(last_code, "SM") || strcmp(last_code, "SC"))) {
      sprintf(msg, "The card on line %d is an SC, but the line above it is not an SP, SM or another SC.", i);
      add_error(errors, msg, 1);
    }
    // SM cards *must* be followed by a SC
    if(strcmp(last_code, "SM") && !strcmp(code, "SC")) {
      sprintf(msg, "The card on line %d is an SM, but the line after it is not an SC.", i - 1);
      add_error(errors, msg, 1);
    }
    
    // QUESTION: it appears GR cards have to follow some other sort of geometry card, but it's not clear what

    //TODO: modifiers have to follow normal geometry, not another modifier or some other card
    //      if (["GM", "GR", "GX"] && !["GA", "GH", "GW", "SP", "CW"].contains(lastCard)) {

  } /* loop over cards */
  
  // and with the entire deck tested, make sure we got the key cards
  if(!sawCE) {
    sprintf(msg, "A NEC-2 deck should have a CE card.");
    add_error(errors, msg, 0);
  }
  if(!sawGx) {
    sprintf(msg, "A deck has to have at least one geometry card.");
    add_error(errors, msg, 1);
  }
  if(!sawGE) {
    sprintf(msg, "A deck has to have a GE card.");
    add_error(errors, msg, 1);
  }
  if(!sawFR) {
    sprintf(msg, "A deck has to have a FR card.");
    add_error(errors, msg, 0);
  }
  if(!sawEN) {
    sprintf(msg, "A deck should end with a EN card.");
    add_error(errors, msg, 0);
  }
  if(!sawEX && !sawLD) {
    sprintf(msg, "A deck has to have at least one EX or LD card.");
    add_error(errors, msg, 1);
  }
  if(sawSY && !sawCE) {
    sprintf(msg, "We found SY cards in the deck, but there is no CE in the deck. SYs should follow the CE.");
    add_error(errors, msg, 1);
  }

  // and get rid of the error
  free(msg);
}

// TODO: MISSING TESTS
// GE -1 requires a GN
// LDs and/or EXs should not be at open ends of wires
// look for SY formulas that override system-wide items like mm or awg
//   but overriding user-entered system variables is ok
// also look for SY's that define the same formula more than once
//   but this is OK, simply use the last definition
// look for EX or LD cards and check that they are connected to wires with more than one segment
//   wires that are connected must contact at segment ends (connection separation < len/1000)
// look for wires that have the same endpoints, or are parallel and have different segmentation
// look for wires that extend into the ground
// 4nec2 warns if parallel wires closer than 0.05 waves have different segmentation
// segment length > .0001 WL in all cases
// check for segment length is greater than 0.1 wavelengths, which NEC-4 says lead to bad results
//    more generally:
//    len < WL/10 in most cases
//    len < WL/20 in critical regions
//    len < WL/5 on long straight segments
// 4nec2 also warns if the segment length is smaller than 0.001 waves, which is simply too many
// check that the radius makes sense:
//    rad < len/2 with default thin wire kernel
//    rad < 2*len with extended thin wire kernel
//    rad < len/10 default usage

