/******************************************************************************
 * deck.c
 *
 * deck.c contains various routines that build decks and the cards in them,
 * provides methods for checking the type of cards, and checking the basic
 * validity of the data during reads and writes like the number and type
 * of data fields for a particular card type.
 *
 * deck.c also contains the high-level connections with the underlying NEC
 * code, building the required NEC structures from the data in the card
 * rather than reading it directly from the input file. This is intended
 * to allow the deck to be easily modified and cards to be added or removed
 * before triggering a calculation run. It also has links to all of the
 * internal structures like the segmentation, which allows it to be used
 * interactively without having to read and write files.
 *
 *****************************************************************************/

#include "opennec.h"
#include "shared.h"

/******************************************************************************
 * new_card
 *
 * Creates and returns a new empty Card.
 *
 * calloc'ing a card will set it up as wanted, but we'll use this explicit
 * constructor mostly as a form of documentation and possible future changes.
 *
 */
Card* new_card(void) {
  Card *card = calloc(1, sizeof(Card));
  card->edited = FALSE;     // new cards are not edited, by default. this only applies to USER edits!
  card->ignore = FALSE;     // cards should not be ignored by default. should apply to geometery and commands?
  card->extn_code[0] = 0;   // this will be applied if there is a code found on the line or the user adds one
  return card;
}

/******************************************************************************
 * free_card
 *
 * deletes an existing card and frees its various structures
 *
 * @param card the Card to free
 *
 */
void free_card(Card *card) {
  // start with the various strings
  if(card->orig_str != NULL) free(card->orig_str);
  if(card->card_str != NULL) free(card->card_str);
  if(card->extn_str != NULL) free(card->extn_str);
  if(card->comment != NULL) free(card->comment);

  // now the two lists
  KeyValue *head, *temp;
  head = card->formulas;
  while(head != NULL) {
    temp = head;
    head = head->next;
    free(temp);
  }
  head = card->extensns;
  while(head != NULL) {
    temp = head;
    head = head->next;
    free(temp);
  }
  
  // and finally, the card itself
  free(card);
}

/******************************************************************************
 * recalculate_sections
 *
 * Loops over the deck and finds the start and end of the various sections,
 * like comments and geometry.
 *
 * @param deck the Deck to recalculate
 *
 */
void recalculate_sections(Deck *deck)
{
  Card *card;
  
  // reset the indexes
  deck->comment_start = -1;
  deck->comment_end = -1;
  deck->geometry_start = -1;
  deck->geometry_end = -1;
  deck->deck_end = -1;

  // re-calculate the section limits
  for(int i = 0; i < deck->num_cards; i++) {
    card = &deck->cards[i];
    
    // the logic here is pretty simple: get the section this card belongs to,
    // if we haven't see a card in that section yet then it's the start, but
    // if we have, keep updating the end until we stop seeing them. that way
    // things like a missing CE won't cause the _end to be -1
    bool isCmt = isComment(card);
    if(isCmt) {
      if(deck->comment_start == -1)
        deck->comment_start = i;
      if(deck->comment_start != -1)
        deck->comment_end = i;
      continue;
    }
    bool isGeo = isGeometry(card);
    if(isGeo) {
      if(deck->geometry_start == -1)
        deck->geometry_start = i;
      if(deck->geometry_end != -1)
        deck->geometry_end = i;
      continue;
    }
    // the oddball is the end, which is only at the EN card
    if(strcmp(card->card_code, "EN") == 0) {
      deck->deck_end = i;
    }
  } /* for loop over cards */
}


/******************************************************************************
 * new_deck
 *
 * creates and returns a new empty Deck
 *
 */
Deck* new_deck(void) {
  Deck *deck = (Deck *)calloc(1, sizeof(Deck));
  return deck;
}

/******************************************************************************
 * free_deck
 *
 * deletes all the Cards in this Deck and then any local bits
 *
 * @param deck the Deck to free
 *
 */
void free_deck(Deck *deck) {
  // free all of the cards
  for(int i = 0; i < deck->num_cards; i++) {
    free_card(&deck->cards[i]);
  }
  // now the list of symbols/formulas
  KeyValue *head, *temp;
  head = deck->symbols;
  while(head != NULL) {
    temp = head;
    head = head->next;
    free(temp);
  } /* while loop over cards */
}

/******************************************************************************
 * append_card
 *
 * append_card adds the Card to the end of the Deck
 *
 * @param deck the Deck to add a new card to
 * @param card the Card to add
 *
 */
int append_card(Deck *deck, Card *card) {
  // calloc/realloc the deck and add this card to it
  // there may be performance improvements possible by allocing blocks of 10 or 20 cards at a time
  if(deck->num_cards == 0) {
    deck->num_cards++;
    deck->cards = calloc(1, sizeof(Card));
  } else {
    deck->num_cards++;
    deck->cards = realloc(deck->cards, deck->num_cards * sizeof(Card));
  }
  deck->cards[deck->num_cards - 1] = *card;
  
  // appending a card changes the deck and requires a recalc of that section
  // so we need to make the card edited so this will be noticed
  card->edited = TRUE;

  // refresh the deck layout
  recalculate_sections(deck);

  return 0; // no error for now
}

/******************************************************************************
 * insert_card
 *
 * inserts a card into a deck at the given index in the Deck's card array.
 * may fail if the index is outside the bounds of the existing deck.
 *
 * @param deck the Deck to add a new card to
 * @param card the Card to add
 * @param location the index to add it at
 *
 */
int insert_card(Deck *deck, Card *card, int location) {
  // sanity check the location
  if(location < 0 || location > deck->num_cards) return 1;
  
  // calloc/realloc space for another card
  if(deck->num_cards == 0) {
    deck->num_cards++;
    deck->cards = calloc(1, sizeof(Card));
  } else {
    deck->num_cards++;
    deck->cards = realloc(deck->cards, deck->num_cards * sizeof(Card));
  }

  // copy everything below the point down one space
  for(int i = location + 1; i < deck->num_cards; i++) {
    deck->cards[i + 1] = deck->cards[i];
  }
  
  // then insert the new one
  deck->cards[location] = *card;
  
  // appending a card changes the deck and requires a recalc of that section
  // so we need to make the card edited so this will be noticed
  card->edited = TRUE;

  // refresh the deck layout
  recalculate_sections(deck);

  // and we're good to go
  return 0;
}

/******************************************************************************
 * remove_card
 *
 * removes the card at the given location from the deck and deletes its memory
 *
 * @param deck the Deck to delete the card from
 * @param location the index of the item to delete
 *
 */
int remove_card(Deck *deck, int location) {
  // sanity check the location
  if(location < 0 || location > deck->num_cards) return 1;
  
  // get a handle to the card for future references
  Card temp = deck->cards[location];
  
  // don't calloc/realloc the Deck smaller, see
  // https://stackoverflow.com/questions/7078019/using-realloc-to-shrink-the-allocated-memory
  // copy everything below it up one space
  for(int i = deck->num_cards - 1; i >= location; i--) {
    deck->cards[i - 1] = deck->cards[i];
  }
  
  // free the card
  free_card(&temp);
  
  // refresh the deck layout
  recalculate_sections(deck);
  
  // and we're good to go
  return 0;
}

/******************************************************************************
 * add_key_value
 *
 * adds a key/value pair at the end of a card's formula or extensions list
 *
 * @param card the Deck to delete the card from
 * @param list which list we're adding it to
 * @param key a string for the key
 * @param value a string for the value
 *
 */
void add_key_value(Card *card, KeyValue *list, char *key, char *value, char separator)
{
  KeyValue *pair = (KeyValue *)malloc(sizeof(KeyValue));
  if(pair != NULL) {
    // calloc the strings and store them...
    pair->key = (char *)calloc(strlen(key) + 1, sizeof(char));
    strcpy(pair->key, key);
    pair->value = (char *)calloc(strlen(value) + 1, sizeof(char));
    strcpy(pair->value, value);
    // now store the separator based on the list
    if(separator != '\n') {
      pair->separator = separator;
    } else if(list == card->formulas) {
      pair->separator = '=';
    } else {
      pair->separator = ':';
    }
    // and then add it to the end of the list
    KeyValue *tail = list;
    if(tail == NULL) {
      list = pair;
    } else {
      while(tail->next != NULL) tail = tail->next;
      tail->next = pair;
    }
  } // there should be else's for all the mallocs and callocs!
}

/******************************************************************************
 * isComment/isGeometry/isControl/isExtension
 *
 * The series of "is" functions test a card code against the mnemonic
 * lists and return boolean TRUE if the card belongs to that class,
 * like "isComment" which returns TRUE for any comment card.
 *
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

/******************************************************************************
 * min_int_fields/max_int_fields/min_flt_fields/max_flt_fields
 *
 * min|min_int|flt_fields returns the minimum or maximum number of
 * fields expected for a given card type. For instance, the "GE" card has
 * zero or one integer parameters, so min_int_fields would return 0 while
 * max_int_fields would return 1, and both of the flts would return 0.
 *
 * Most of the time this can be determined purely by the type of card,
 * for instance, almost every geometery card has two ints while command
 * cards have four. In contrast, the number of floating point inputs
 * is pretty much random.
 *
 */
int min_int_fields(Card* card)
{
  // GE has zero minimum fields
  if(strcmp(card->card_code, "GE") == 0) return 0; // the ground type is optional
  if(strcmp(card->card_code, "GF") == 0) return 0; // there is an option to print extra data
  if(strcmp(card->card_code, "GC") == 0) return 0; // tapers use I's from previous GW
  // SP/SC uses only one int or none, but it's in position 2, so nothing to do here
  
  if(strcmp(card->card_code, "EK") == 0) return 1; // flag for on or off
  if(strcmp(card->card_code, "EN") == 0) return 0;

  // now the default cases
  if(isGeometry(card)) {
    return 2;
  } else if (isControl(card)) {
    return 4;
  } else {
    return 0; // need to check this!
  }
}

int max_int_fields(Card* card)
{
  if(strcmp(card->card_code, "GF") == 0) return 1; // there is an option to print extra data
  if(strcmp(card->card_code, "GC") == 0) return 0; // tapers use I's from previous GW
  // SP/SC uses only one int or none, but it's in position 2, so nothing to do here

  if(strcmp(card->card_code, "EK") == 0) return 1; // flag for on or off
  if(strcmp(card->card_code, "EN") == 0) return 0;

  // now the default cases
  if(isGeometry(card)) {
    return 2;
  } else if (isControl(card)) {
    return 4;
  } else {
    return 0; // need to check this!
  }
}

int min_flt_fields(Card* card)
{
  // these are taken from the NEC-2 dox unless noted otherwise
  // the dox indicate optional parameters with () around the name
  if(strcmp(card->card_code, "GA") == 0) return 4; // arcs have four inputs
  if(strcmp(card->card_code, "GE") == 0) return 0; // no floats
  if(strcmp(card->card_code, "GR") == 0) return 0; // no floats
  if(strcmp(card->card_code, "GS") == 0) return 1; // scale
  if(strcmp(card->card_code, "GC") == 0) return 0; // tapers have three inputs
  if(strcmp(card->card_code, "GX") == 0) return 0; // uses only the ints
  if(strcmp(card->card_code, "SP") == 0) return 6; // last field unused
  if(strcmp(card->card_code, "SM") == 0) return 6; // last field unused
  if(strcmp(card->card_code, "SC") == 0) return 6; // this might only be three if it follows SM, but filled with zeros

  // this one is annoying as the format depends on the value in I1
  if(strcmp(card->card_code, "EX") == 0) {
    if(card->i[1] == 0 || card->i[1] == 5)
      return 3;
    else
      return 6;
  }
  
  if(strcmp(card->card_code, "RF")) return 1; // can be a single frequency

  // now the default cases
  if(isGeometry(card)) {
    return 7;
  } else if (isControl(card)) {
    return 4;
  } else {
    return 0; // need to check this!
  }
}

int max_flt_fields(Card* card)
{
  if(strcmp(card->card_code, "GA") == 0) return 4; // arcs have four inputs
  if(strcmp(card->card_code, "GE") == 0) return 0; // no floats
  if(strcmp(card->card_code, "GR") == 0) return 0; // no floats
  if(strcmp(card->card_code, "GS") == 0) return 1; // scale
  if(strcmp(card->card_code, "GC") == 0) return 0; // tapers have three inputs
  if(strcmp(card->card_code, "GX") == 0) return 0; // uses only the ints
  if(strcmp(card->card_code, "SP") == 0) return 6; // last field unused
  if(strcmp(card->card_code, "SC") == 0) return 6; // even in the three-used case, zeros are used

  // EX format depends on the value in I1
  if(strcmp(card->card_code, "EX") == 0) {
    if(card->i[1] == 0 || card->i[1] == 5)
      return 3;
    else
      return 6;
  }
  
  if(strcmp(card->card_code, "FR") == 0) return 2; // can be stepped

  // now the default cases
  if(isGeometry(card)) {
    return 7;
  } else if (isControl(card)) {
    return 6;
  } else {
    return 0; // need to check this!
  }
}

/******************************************************************************
 * isGeometryEdited
 *
 * isGeometryEdited loops through the geometry section of the deck and looks
 * for any new or edited card, which means we need to re-run the geometry
 * creation code. It's the only section that needs this, there's nothing
 * to recalculate for the comments, and the command section
 *
 */
bool isGeometryEdited(Deck *deck)
{
  bool isEdited = FALSE;
  for(int i = deck->geometry_start; i < deck->geometry_end; i++) {
    if(deck->cards[i].edited) {
      isEdited = TRUE;
      break;
    }
  }
  return isEdited;
}

/******************************************************************************
 * convert_awg_to_meters
 *
 * convert_awg_to_meters returns a value in meters for a given AWG value.
 *
 */
double convert_awg_to_meters(double awg_value)
{
  // TODO: how are we going to handle "4/0" and/or "0000"?
  int awg_code = floor(awg_value);

  // any decimal part is bad!
  if(awg_value != awg_code) {
    return -1.0;
  }

  switch(awg_code) {
//      0000 (4/0)  0.11684
 //     000 (3/0)  0.104049
//      00 (2/0)  0.092658
    case 0: return 0.082515;
    case 1: return 0.073481;
    case 2: return 0.065437;
    case 3: return 0.058273;
    case 4: return 0.051894;
    case 5: return 0.046213;
    case 6: return 0.041154;
    case 7: return 0.036649;
    case 8: return 0.032636;
    case 9: return 0.029064;
    case 10: return 0.025882;
    case 11: return 0.023048;
    case 12: return 0.020525;
    case 13: return 0.018278;
    case 14: return 0.016277;
    case 15: return 0.014495;
    case 16: return 0.012908;
    case 17: return 0.011495;
    case 18: return 0.010237;
    case 19: return 0.009116;
    case 20: return 0.008118;
    case 21: return 0.007229;
    case 22: return 0.006438;
    case 23: return 0.005733;
    case 24: return 0.005106;
    case 25: return 0.004547;
    case 26: return 0.004049;
    case 27: return 0.003606;
    case 28: return 0.003211;
    case 29: return 0.002859;
    case 30: return 0.002546;
    case 31: return 0.002268;
    case 32: return 0.002019;
    case 33: return 0.001798;
    case 34: return 0.001601;
    case 35: return 0.001426;
    case 36: return 0.00127;
    case 37: return 0.001131;
    case 38: return 0.001007;
    case 39: return 0.000897;
    case 40: return 0.000799;
    default: return -1.0;
  }
}


/******************************************************************************
 * update_deck_values
 *
 * update_deck_values loops through the entire deck and calls
 * update_card_values on any card that has a formula or unit. Normally called
 * after making a change to any of the SY cards, or just before any deck-wide
 * actions like saving it out or running a calculation.
 */
void update_deck_values(Deck *deck)
{
  for(int i = 0; i < deck->num_cards; i++) {
    update_card_values(&deck->cards[i]);
  }
}

/******************************************************************************
 * update_card_values
 *
 * update_card_values looks for any formulas or units in the card and updates
 * their values. Generally called after any changes to the card or as part
 * of update_deck_values
 *
 */
void update_card_values(Card *card)
{
  double ft, in;
  
  // first, copy any original input values into the value fields
  for(int i = 1; i <= MAX_INT_FIELDS; i++) {
    card->iv[i] = card->i[i];
  }
  for(int i = 1; i <= MAX_FLT_FIELDS; i++) {
    card->fv[i] = card->f[i];
  }

  // now run any calculations on the fields and copy those in instead
  // TODO: do this!
  
  // and finally, apply any unit conversions - which are only on the flts
  for(int i = 1; i <= MAX_FLT_FIELDS; i++) {
    if(card->units[i] == 0) continue;  // 0 means "no units"
    
      if(unit_mult[card->units[i]] != 0) {
        card->fv[i] = card->fv[i] * unit_mult[card->units[i]];
      } else {
        // units 6 through 8 are zeros and have to be converted case-by-case
        switch(card->units[i]) {
          case 6:  //ftin
            // the fraction part is inches, not decimal feet
            ft = floor(card->fv[i]);
            in = card->fv[i] - ft;
            card->fv[i] = ft + (in / 12.0);
            
            // now convert feet to meters
            card->fv[i] = card->fv[i] * unit_mult[card->units[4]];
            break;
            
          case 7:
          case 8:
            card->fv[i] = convert_awg_to_meters(card->fv[i]);
            break;
        } // switch
      } // if can be directly converted
  } // for
} /* update_card_values() */

