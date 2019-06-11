/*******************************************************************
 * deck.c
 *
 * deck.c contains the code that actually runs the simulations and
 * creates results. It also allows new cards to be inserted or
 * existing ones to be deleted.
 *
 * Most of the functions here are simply wrappers for the original
 * nec2c code, but getting the data from the already-parsed deck
 * rather than reading it directly from the file. These outputs are
 * then used in the original nec2c code.
 *
 * OpenNEC adds the test_deck() function, which looks at the deck
 * trying to find common errors like a tapered wire missing a GC
 * card, as well as more difficult issues like crossed wires or
 * inputs that will cause the calculation to fail. There are a
 * variety of other functions for more mundane tasks.
 *
 *******************************************************************/

#include "opennec.h"
#include "shared.h"

/*----------------------------------------------------------------------*/

/** The series of "is" functions test the card code against the mnemonic
 * lists and return boolean TRUE if the card belongs to that class,
 * like "isComment" which returns TRUE for any comment card.
 */

int isComment(Card *card)
{
  int isCmt = FALSE;
  for(int i = 0; i < NUM_COMMENT_CODES; i++) {
    if(strcmp(card->card_code, comment_codes[i]) == 0) {
      isCmt = TRUE;
      break;
    }
  }
  return isCmt;
}

int isGeometry(Card *card)
{
  int isGeo = FALSE;
  for(int i = 0; i < NUM_GEOMETRY_CODES; i++) {
    if(strcmp(card->card_code, geometry_codes[i]) == 0) {
      isGeo = TRUE;
      break;
    }
  }
  return isGeo;
}

int isControl(Card *card)
{
  int isCtl = FALSE;
  for(int i = 0; i < NUM_CONTROL_CODES; i++) {
    if(strcmp(card->card_code, control_codes[i]) == 0) {
      isCtl = TRUE;
      break;
    }
  }
  return isCtl;
}

int isExtension(Card *card)
{
  int isExt = FALSE;
  for(int i = 0; i < NUM_ONEC_CODES; i++) {
    if(strcmp(card->card_code, onec_codes[i]) == 0) {
      isExt = TRUE;
      break;
    }
  }
  return isExt;
}

/*----------------------------------------------------------------------*/

/** update_deck_values()
 *
 * update_deck_values() loops through the entire deck and calls
 * update_card_values() on any card that has a formula. Normally called
 * after making a change to any of the SY cards, or just before any
 * deck-wide actions like saving it out or running a calculation
 */
void update_deck_values(Deck *deck)
{
  for(int i = 0; i < deck->num_cards; i++) {
    update_card_values(&deck->cards[0]);
  }
}

/*----------------------------------------------------------------------*/

/** update_card_values()
 *
 * update_card_values() looks for any formulas in the card and updates
 * their values. Generally called after any changes to the card or as
 * part of update_deck_values()
 */
void update_card_values(Card *card)
{
  // TODO do this!
}

/*----------------------------------------------------------------------*/

/** test_deck runs various tests on the deck and returns warnings or
 * errors. passing in a -1 for level means "return everything" while
 * passing in a positive number limits the results to items with that
 * severity.
 *
 */
void test_deck(Deck *deck, Errors *errors) //, int level
{
  int sawCE = 0, sawGx = 0, sawGE = 0, sawEN = 0, sawGF = 0;
  int sawFR = 0, sawSC = 0, sawSP = 0, sawGN = 0, sawGD = 0;
  int sawGS = 0, sawLD = 0, sawEX = 0;
  char *code, *last_code;
  char *msg = calloc(1, MAX_ERROR_LEN);
  
  // start with some obvious ones
  if(deck->num_cards == 0) {
    sprintf(msg, "A deck has to have at least one card.");
    add_error(errors, msg, 2);  // this is a fatal error
    return;
  }
  if(deck->num_cards < 3) {
    sprintf(msg, "A deck has to have at least three cards; one or more Gx cards and a GE, and one or more EN or LD.");
    add_error(errors, msg, 0);
    return;
  }
  
  // now let's make sure we can find all the required cards
  last_code = "";
  for(int i = 0; i < deck->num_cards; i++) {
    // cache this
    code = deck->cards[i].card_code;
    
    // start with the checks for the cards we HAVE to have
    
    // nec4 does not require a CE or CM
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
    // it appears you can have multiple GS's, although why you would ever do that is unclear
    if(strcmp(code, "GS") == 0) {
      if(sawGS == FALSE) sawGS = i;
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
    } /* for over geometry codes */
    
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
    
    // GF cards have to follow CE cards, or in an onec deck, an SY
    if(strcmp(code, "GF") && !(strcmp(last_code, "CE") || strcmp(last_code, "SY"))) {
      sprintf(msg, "The card on line %d is a GF, but the line above it is not a CE or SY.", i);
      add_error(errors, msg, 1);
    }
    
    // SY's also have to follow the CE or another SY
    if(strcmp(code, "SY") && !(strcmp(last_code, "CE") || strcmp(last_code, "SY"))) {
      sprintf(msg, "The card on line %d is a SY, but the line above it is not a CE or SY.", i);
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

  } /* for over cards */
  
  // and with the entire deck tested, make sure we got the key cards
  if(!sawCE) {
    sprintf(msg, "A NEC2 deck should have a CE card.");
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
  if(!sawEN) {
    sprintf(msg, "A deck should end with a EN card.");
    add_error(errors, msg, 0);
  }
  if(!sawEX && !sawLD) {
    sprintf(msg, "A deck has to have at least one EX or LD card.");
    add_error(errors, msg, 1);
  }
  
  // TODO: MISSING TESTS
  // look for EX or LD cards and check that they are connected to wires with more than one segment
  // look for wires that have the same endpoints, or are parallel and have different segmentation
  // look for wires that extend into the ground
  // 4nec2 warns if parallel wires closer than 0.05 waves have different segmentation
  // check for segment length is greater than 0.1 wavelengths, which NEC-4 says lead to bad results
  // 4nec2 also warns if the segment length is smaller than 0.001 waves, which is simply too many

  // and get rid of the error
  free(msg);
}

