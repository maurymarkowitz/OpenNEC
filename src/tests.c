/******************************************************************************
 * tests.c
 *
 * tests.c contains code for a number of santity-checking routines that look
 * for problems in the structure of the deck as a whole, or commonly found
 * issues in the design.
 *
 * An example of the former is a series of CM cards but no CE, SM cards not
 * followed by an SC, or a tapered wire that's missing its GC card. The tests
 * are deliberately picky, it will report issues that won't actually cause
 * problems in most systems. These can simply be ignored if not wanted, but
 * they are things that are generally easy to fix in the deck, and should be.
 *
 * Examples of problems in the actual data include more complex issues like
 * crossed wires or wires that cross the ground line, both of which will cause
 * the calculations to fail.
 *
 * Non-critical errors like a missing CE card will have a value of 0. Ones
 * that will cause evaluation to fail, like a missing GE, have a value of 1,
 * and critical errors like a missing file are 2. Users can add their own
 * errors using these values or any value less than 0.
 *
 *****************************************************************************/

#include "opennec.h"
#include "shared.h"

/******************************************************************************
 * test_deck_structure
 *
 * test_deck_structure runs various tests on the deck and returns a list of
 * errors and warnings. This looks only for structure problems, like missing
 * or duplicated cards, it does not look for logical problems or missing data
 * that's handled in other functions.
 *
 * @param deck the Deck to be tested
 * @param errors the Errors list to add new messages to
 *
 */
void test_deck_structure(Deck *deck, Errors *errors)
{
  // A short list of the minimum structure is found in the 4nec2
  // documentation:
  //
  // Zero or more CM (comment) cards
  // One CE (comment end) card
  // One or more GW (wire geometry) cards
  // One GE (geometry end) card
  // One or more FR (design frequency) cards
  // One or more EX (excitation point) cards
  // Zero or one GN (Ground condition) card
  // Zero or more LD (loading) cards
  // One EN (end of file) card
  //
  // There are minor issues with this list:
  //
  // 1) some decks lack any comments, although we consider that fatal
  // 2) you don't need a GW card specifically, any geometery will do
  // 3) the EN is not really required, many decks lack it
  //
  // as a result, this code demands a minimum deck of five cards,
  // one comment, two geometry cards, an FX, and a EX.
  //
  // TODO: do you need an EX? what about transmission?
  //
  // There are also a number of additional tests performed
  // below for other issues like duplicates of cards that should
  // only exist once, cards in the wrong section of the deck, and
  // similiar issues.

  // although these look like they should be bools, we use int
  // so we can report the card number where the duplicate was seen
  int sawCE = 0, sawGx = 0, sawGE = 0, sawEN = 0, sawGF = 0;
  int sawFR = 0, sawSC = 0, sawSP = 0, sawGN = 0, sawGD = 0;
  int sawGS = 0, sawLD = 0, sawEX = 0, sawSY = 0;
  int GEType = 0;
  // and some temps
  char *code, *last_code;
  char *msg = calloc(1, MAX_ERROR_LEN);
  
  // start with some obvious ones
  if(deck->num_cards == 0) {
    sprintf(msg, "The deck has no cards.");
    add_error(errors, msg, 2);  // this is a critical error, this deck will not process
    return;
  }
  if(deck->num_cards < 5) {
    sprintf(msg, "A deck has to have at least five cards; one or more comments, one or more Gx cards, a GE, an FX, and an EX.");
    add_error(errors, msg, 2);  // same here, there is no way this will calculate property
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
        add_error(errors, msg, 0);  // this will calculate fine, so this is merely a warning
      }
    }
    // NOTE: nec4 does not require a CE or CM, but we'll demand them here for compatibility
    if(strcmp(code, "CE") == 0) {
      if(sawCE == FALSE) {
        sawCE = i;
      } else {
        sprintf(msg, "The card on line %d is a CE, but we already saw one on card %d.", i, sawCE + 1);
        add_error(errors, msg, 0);
      }
    }
    if(strcmp(code, "GE") == 0) {
      if(sawGE == FALSE) {
        sawGE = i;
        GEType = deck->cards[i].i[1];
      } else {
        sprintf(msg, "The card on line %d is a GE, but we already saw one on card %d.", i, sawGE + 1);
        add_error(errors, msg, 0);
      }
    }
    if(strcmp(code, "EN") == 0) {
      if(sawEN == FALSE) {
        sawEN = i;
      } else {
        sprintf(msg, "The card on line %d is an EN, but we already saw one on card %d.", i, sawEN + 1);
        add_error(errors, msg, 0);
      }
    }
    
    // NOTE: does a deck really need a EX?
    
    // and also look for other cards where there can only be one
    if(strcmp(code, "GF") == 0) {
      if(sawGF == FALSE) {
        sawGF = i;
      } else {
        sprintf(msg, "The card on line %d is a GF, but we already saw one on card %d.", i, sawGF + 1);
        add_error(errors, msg, 0);
      }
    }
    if(strcmp(code, "FR") == 0) {
      if(sawFR == FALSE) {
        sawFR = i;
      } else {
        sprintf(msg, "The card on line %d is an FR, but we already saw one on card %d.", i, sawFR + 1);
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
    // you can have multiple SCs, but they have to follow a SP or SM
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
      if(strcmp(code, geometry_codes[j]) == 0 && strcmp(code, "GE") != 0) {
        if(sawGx == FALSE) {
          sawGx = i;
          break;
        }
        // there's no else in this case, multiple Gx cards are fine, however
        // we do have a potential problem when we find Gx cards after a GE
        if(sawGx > 0 && sawGE > 0) {
          sprintf(msg, "The card on line %d has geometry code %s, but we already saw the GE on card %d.", i, code, sawGE + 1);
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
    if(strcmp(code, "GC") == 0 && strcmp(last_code, "GW") != 0) {
      sprintf(msg, "The card on line %d is a GC, but the card above it is not a GW.", i + 1);
      add_error(errors, msg, 1);
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
      add_error(errors, msg, 1);
    }
    if(strcmp(code, "GN") == 0 && strcmp(deck->cards[i+1].card_code, "GD") != 0) {
      sprintf(msg, "The card on line %d is a GN, but the card after it is not a GN.", i + 1);
      add_error(errors, msg, 1);
    }

    // GF cards have to be the first item in the geometry section, which
    // means they must follow CE cards, or in an onec deck, an SY
    // FIXME: it could also follow onec comment cards, so this is somewhat complex
    if(strcmp(code, "GF") == 0 && !(strcmp(last_code, "CE") == 0 || strcmp(last_code, "SY") == 0)) {
      sprintf(msg, "The card on line %d is a GF, but the card above it is not a CE or SY.", i + 1);
      add_error(errors, msg, 1);
    }

    // SC cards have to follow an SP or SM, or another SC. this is not an exhaustive
    // test, it should really roll backward until it finds an SP or SM, but...
    if(strcmp(code, "SC") == 0 && !(strcmp(last_code, "SP") || strcmp(last_code, "SM") || strcmp(last_code, "SC"))) {
      sprintf(msg, "The card on line %d is an SC, but the card above it is not an SP, SM or another SC.", i + 1);
      add_error(errors, msg, 1);
    }
    // SM cards *must* be followed by a SC
    if(strcmp(last_code, "SM") && !strcmp(code, "SC")) {
      sprintf(msg, "The card on line %d is an SM, but the card after it is not an SC.", i);
      add_error(errors, msg, 1);
    }
    
    // QUESTION: it appears GR cards have to follow some other sort of geometry card, but it's not clear what
    
    // FR cards have to have either one input or three
    if(strcmp(code, "SC") == 0) {
      sprintf(msg, "The card on line %d is an SM, but the card after it is not an SC.", i);
      add_error(errors, msg, 1);
    }

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
    sprintf(msg, "A deck has to have an FR card.");
    add_error(errors, msg, 1);
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
    add_error(errors, msg, 0);
  }
  
  // if the GE card was -1, there has to be a GN
  if(sawGE && GEType == -1 && !sawGN) {
    sprintf(msg, "The GE is set to -1, but there is no GN card in the deck.");
    add_error(errors, msg, 1);
  }
  
  // and get rid of the local string
  free(msg);
}

/*******************************************************************
 * test_duplicate_tags
 *
 * test_duplicate_tags checks to see if there is more than one card
 * with the same tag on it. this will not notice problems if there
 * is a GM or similar card that creates new tags, that only happens
 * when the geometery is segmented
 *
 * @param deck the Deck to be tested
 * @param errors the Errors list to add new messages to
 *
 */
void test_duplicate_tags(Deck *deck, Errors *errors)
{
  // we will also check to see if there are duplicate tags
  char *msg = calloc(1, MAX_ERROR_LEN);
 
  // now check if there are any duplicate tags in the geometry
  // NOTE: this doesn't test for new tags generated by GM or similar
  // FIXME: we could do that by calculating geometry and then comparing
  //        card and tag numbers
  for(int i = 0; i < deck->num_cards; i++) {
    if(isGeometry(&deck->cards[i]) && deck->cards[i].i[1] > 0) {
      for(int j = i + 1; j < deck->num_cards; j++) {
        if(deck->cards[j].i[1] == deck->cards[i].i[1]) {
          sprintf(msg, "The tag number %d is found on card %d and card %d.", i, j, deck->cards[i].i[1]);
          add_error(errors, msg, 1);
        }
      }
    }
  }
  
  free(msg);
}

/******************************************************************************
 * test_card_inputs
 *
 * test_card_inputs looks at each card to ensure it has the right number
 * and type of inputs. For instance, an FR card has two forms; if I1 is 0
 * then it has to have no other values, if I1 is non-zero, it has to have
 * F1 and F2.
 *
 * @param deck the Deck to be tested
 * @param errors the Errors list to add new messages to
 *
 */
void test_card_inputs(Deck *deck, Errors *errors)
{
  char *code;
  char *msg = calloc(1, MAX_ERROR_LEN);

  for(int i = 0; i < deck->num_cards; i++) {
    code = deck->cards[i].card_code;

    // FRs come in two forms, I2=1 and I2>1
    if(strcmp(code, "FR") == 0) {
      // there must be a value in F1
      if(deck->cards[i].f[1] == 0) {
        sprintf(msg, "The card on line %d is a FR but has no base frequency in F1.", i);
        add_error(errors, msg, 0);
      }
      // I2 has to be >= 1
      if(deck->cards[i].i[2] < 1) {
        sprintf(msg, "The card on line %d is a FR with I2 < 1, which is illegal.", i);
        add_error(errors, msg, 0);
      }
      // if I2=1, then F2 should be 0
      else if(deck->cards[i].i[2] == 1 && deck->cards[i].f[2] != 0) {
        sprintf(msg, "The card on line %d is a FR with I2 = 1 (no steps), but has a step value in F2.", i);
        add_error(errors, msg, 0);
      }
      // but if I2 > 1 then F2 has to be > 0
      else if(deck->cards[i].i[2] > 1 && deck->cards[i].f[2] == 0) {
          sprintf(msg, "The card on line %d is a FR with I2 > 1 (steps), but has no step value in F2.", i);
          add_error(errors, msg, 0);
      }
    }
  }
    
  free(msg);
}

/******************************************************************************
 * test_bad_symbols
 *
 * looks at all the SYmbol cards, if any, and warns if they override one of
 * the system-wide symbols like "mm" or "awg".
 *
 * also warns about duplicate definitions, as only the last value will be used
 * NOTE: is this correct? can you define HEIGHT=7 and then 14 lower in the deck?
 *
 * @param deck the Deck to be tested
 * @param errors the Errors list to add new messages to
 *
 */
void test_bad_symbols(Deck *deck, Errors *errors)
{
  KeyValue *outer, *inner;
  char *msg = calloc(1, MAX_ERROR_LEN);
  
  // first we'll check that they aren't overriding a measurement
  outer = deck->symbols;
  while(outer != NULL) {
    for(int i = 0; i < NUM_ONEC_UNIT_CODES; i++) {
      if(strcasecmp(outer->key, unit_codes[i]) == 0) {
        sprintf(msg, "The symbol '%s' has been defined and overrides a system-wide symbol of the same name.", unit_codes[i]);
        add_error(errors, msg, 0);
      }
    }
    // and now see if any other symbol has the same name
    // TODO: need to see if this is actually used, should SY's only be at the top or can they be redefined in the body?
    inner = outer->next;
    while(inner != NULL) {
      if(strcasecmp(outer->key, inner->key)  == 0) {
        sprintf(msg, "The symbol '%s' has been defined more than once.", outer->key);
        add_error(errors, msg, 0);
      }
    }
    outer = outer->next;
  } /* while loop over cards */
    
  free(msg);
}

// TODO: MISSING TESTS
// LDs and/or EXs should not be at open ends of wires
// look for EX or LD cards and check that they are connected to wires with more than one segment
//   wires that are connected must contact at segment ends (connection separation < len/1000)
// look for wires that have the same endpoints
//   or are parallel and have different segmentation
// look for wires that extend into the ground
// 4nec2 warns if parallel wires closer than 0.05 waves have different segmentation
// segment length > .0001 WL in all cases
// NEC2 dox part 2 page 30 says wavelenth ~=0.1 or 0.05 in critical sections
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
// GE card - from nec dox, If the height of a horizontal wire is less than 10^-3 times the segment length, I1 equal to 1 will connect the end of every segment in the wire to ground. I1 should be -1 to avoid this disaster.
