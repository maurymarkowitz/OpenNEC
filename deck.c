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

/** The series of "is" functions test a card code against the mnemonic
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
