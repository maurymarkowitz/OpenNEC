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
 * creates and returns a new empty Card
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
 * new_deck
 *
 * creates and returns a new empty Deck
 *
 */
Deck* new_deck(void) {
  Deck *deck = calloc(1, sizeof(Card));
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
  // now the two lists
  KeyValue *head, *temp;
  head = deck->symbols;
  while(head != NULL) {
    temp = head;
    head = head->next;
    free(temp);
  }
}

/******************************************************************************
 * append_card
 *
 * append_card adds the Card to the end of the Deck
 *
 * @param deck the Deck to add a new card to
 * @param card the Card to add
 *
 * TODO: all of these methods need to recalculate geometry_start etc.
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
  if(strcasecmp(card->card_code, "GE")) return 0; // the ground type is optional
  if(strcasecmp(card->card_code, "GF")) return 0; // there is an option to print extra data

  // now the default cases
  if(isComment(card)) {
    return 0;
  } else if(isGeometry(card)) {
    return 2;
  } else if (isControl(card)) {
    return 4;
  } else {
    return 0; // need to check this!
  }
}

int max_int_fields(Card* card)
{
  // now the default cases
  if(isComment(card)) {
    return 0;
  } else if(isGeometry(card)) {
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
  if(strcasecmp(card->card_code, "GA")) return 4;
  if(strcasecmp(card->card_code, "GE")) return 0;

  // now the default cases
  if(isComment(card)) {
    return 0;
  } else if(isGeometry(card)) {
    return 7;
  } else if (isControl(card)) {
    return 4;
  } else {
    return 0; // need to check this!
  }
}

int max_flt_fields(Card* card)
{
  // now the default cases
  if(isComment(card)) {
    return 0;
  } else if(isGeometry(card)) {
    return 7;
  } else if (isControl(card)) {
    return 6;
  } else {
    return 0; // need to check this!
  }
}

/******************************************************************************
 * update_deck_values
 *
 * update_deck_values loops through the entire deck and calls
 * update_card_values on any card that has a formula. Normally called
 * after making a change to any of the SY cards, or just before any
 * deck-wide actions like saving it out or running a calculation
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
 * update_card_values looks for any formulas in the card and updates
 * their values. Generally called after any changes to the card or as
 * part of update_deck_values
 */
void update_card_values(Card *card)
{
  // TODO: do this!
}
