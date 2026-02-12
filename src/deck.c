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


#include "internals.h"
#include "deck.h"
#include "tinyexpr.h"

/* Forward declarations for internal functions */
static void update_symbol_values(nec_context_t *ctx, deck_t *deck, errors_list_t *errors);
static void update_card_values(deck_t *deck);
static void add_default_symbols(deck_t *deck);
static void update_symbol_list(deck_t *deck, errors_list_t *errors);

static bool references(const char *expr, const char *symname);
static int eval_symbol(int i, int sym_count, key_value_t **syms, bool *evaluated, deck_t *deck, nec_context_t *ctx, errors_list_t *errors);
static char *preprocess_feet_inches(const char *formula);
static char *preprocess_implicit_multiplication(const char *formula);

/******************************************************************************
 * new_card
 *
 * Creates and returns a new empty card_t.
 *
 * calloc'ing a card will set it up as wanted, but we'll use this explicit
 * constructor mostly as a form of documentation and possible future changes.
 *
 */
card_t* new_card(void) {
  card_t *card = calloc(1, sizeof(card_t));
  card->edited = false;     // new cards are not edited, by default. this only applies to USER edits!
  card->ignore = false;     // cards should not be ignored by default. should apply to geometry and commands?
  card->extn_code[0] = 0;   // this will be applied if there is a code found on the line or the user adds one
  return card;
}

/******************************************************************************
 * free_card
 *
 * deletes an existing card and frees its various structures
 *
 * @param card the card_t to free
 *
 */
void free_card(card_t *card) {
  if(card == NULL) return;
  
  // start with the various strings
  if(card->orig_str != NULL) free(card->orig_str);
  if(card->card_str != NULL) free(card->card_str);
  if(card->extn_str != NULL) free(card->extn_str);
  if(card->comment != NULL) free(card->comment);

  // now the two lists
  key_value_t *head, *temp;
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
  
  // Do NOT free(card) here; cards are part of an array and freed in free_deck
}

/******************************************************************************
 * recalculate_sections
 *
 * Loops over the deck and finds the start and end of the various sections,
 * like comments and geometry.
 *
 * @param deck the deck_t to recalculate
 *
 */
void recalculate_sections(deck_t *deck)
{
  card_t *card;
  
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
    bool isCmt = is_comment(card);
    if(isCmt) {
      if(deck->comment_start == -1)
        deck->comment_start = i;
      if(deck->comment_start != -1)
        deck->comment_end = i;
      continue;
    }
    bool isGeo = is_geometry(card);
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
deck_t* new_deck(void) {
  deck_t *deck = (deck_t *)calloc(1, sizeof(deck_t));
  return deck;
}

/******************************************************************************
 * free_deck
 *
 * deletes all the Cards in this deck_t and then any local bits
 *
 * @param deck the deck_t to free
 *
 */
void free_deck(deck_t *deck) {
  if(deck == NULL) return;

  // free all of the cards first
  for(int i = 0; i < deck->num_cards; i++) {
    free_card(&deck->cards[i]);
  }

  // then free the cards array itself
  if (deck->cards) free(deck->cards);

  // Only free the symbols array (array of pointers), not the nodes themselves
  // INVARIANT: All key_value_t nodes referenced by deck->symbols are owned and
  //            freed by the cards, this is a list of them for easy access.
  if (deck->symbols) {
    free(deck->symbols);
  }
}

/******************************************************************************
 * append_card
 *
 * append_card adds a card_t to the end of the Deck
 *
 * @param deck the deck_t to add a new card to
 * @param card the card_t to add
 *
 */
int append_card(deck_t *deck, card_t *card) {
  // calloc/realloc the deck and add this card to it
  // there may be performance improvements possible by allocing blocks of 10 or 20 cards at a time
  if(deck->num_cards == 0) {
    deck->num_cards++;
    deck->cards = calloc(1, sizeof(card_t));
  } else {
    deck->num_cards++;
    deck->cards = realloc(deck->cards, deck->num_cards * sizeof(card_t));
  }
  deck->cards[deck->num_cards - 1] = *card;
  
  // appending a card changes the deck and requires a recalc of that section
  // so we need to make the card edited so this will be noticed
  card->edited = true;

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
 * @param deck the deck_t to add a new card to
 * @param card the card_t to add
 * @param location the index to add it at
 * @return 0 for success, 1 for any problem (currently only bad index)
 *
 */
int insert_card(deck_t *deck, card_t *card, int location) {
  // sanity check the location
  if(location < 0 || location > deck->num_cards) return 1;
  
  // calloc/realloc space for another card
  if(deck->num_cards == 0) {
    deck->num_cards++;
    deck->cards = calloc(1, sizeof(card_t));
  } else {
    deck->num_cards++;
    deck->cards = realloc(deck->cards, deck->num_cards * sizeof(card_t));
  }

  // copy everything below the point down one space
  for(int i = location + 1; i < deck->num_cards; i++) {
    deck->cards[i + 1] = deck->cards[i];
  }
  
  // then insert the new one
  deck->cards[location] = *card;
  
  // appending a card changes the deck and requires a recalc of that section
  // so we need to make the card edited so this will be noticed
  card->edited = true;

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
 * @param deck the deck_t to delete the card from
 * @param location the index of the item to delete
 * @return 0 for success, 1 for any problem (currently only bad index)
 *
 */
int remove_card(deck_t *deck, int location) {
  // sanity check the location
  if(location < 0 || location > deck->num_cards) return 1;
  
  // get a handle to the card for future references
  card_t temp = deck->cards[location];
  
  // don't calloc/realloc the deck_t smaller, see
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
 * @param card the card to add the key/value pair to
 * @param list which list we're adding it to
 * @param key a string for the key
 * @param value a string for the value
 * @param separator the char to use as a separator when writing the deck
 *
 */
void add_key_value(const card_t *card, key_value_t **list, char *key, char *value, char separator)
{
  // ...removed add_key_value copied key debug print...
  key_value_t *pair = (key_value_t *)malloc(sizeof(key_value_t));
  if(pair != NULL) {
    pair->key = (char *)calloc(strlen(key) + 1, sizeof(char));
    strcpy(pair->key, key);
    pair->value = (char *)calloc(strlen(value) + 1, sizeof(char));
    strcpy(pair->value, value);
    if(separator != '\n') {
      pair->separator = separator;
    } else if(*list == card->formulas) {
      pair->separator = '=';
    } else {
      pair->separator = ':';
    }
    pair->next = NULL;
    if(*list == NULL) {
      *list = pair;
    } else {
      key_value_t *tail = *list;
      while(tail->next != NULL) tail = tail->next;
      tail->next = pair;
    }
  }
}

/******************************************************************************
 * lookup_formula
 *
 * Look up a formula value in the card's formulas list by key (e.g., "F3")
 * Returns the original formula string if found, NULL otherwise.
 * 
 */
const char* lookup_formula(const card_t *card, const char *key) {
  if (!card || !key) return NULL;
  
  key_value_t *formula = card->formulas;
  while (formula != NULL) {
    if (strcmp(formula->key, key) == 0) {
      return formula->value;
    }
    formula = formula->next;
  }
  return NULL;
}

/******************************************************************************
 * add_symbol/remove_symbol
 *
 * Add or remove a symbol from the deck's symbol list and update num_symbols.
 * 
 */
void add_symbol(deck_t *deck, key_value_t *new_sym) {
  if (!deck || !new_sym) return;
  // INVARIANT: Only add pointers to key_value_t nodes owned by cards (e.g., from card->formulas)
  // Never add separately allocated nodes (e.g., from add_default_symbols) to deck->symbols.
  deck->symbols = realloc(deck->symbols, sizeof(key_value_t*) * (deck->num_symbols + 1));
  deck->symbols[deck->num_symbols] = new_sym;
  deck->num_symbols++;
}

void remove_symbol(deck_t *deck, const char *key) {
  if (!deck || !deck->symbols || !key) return;
  for (int i = 0; i < deck->num_symbols; ++i) {
    key_value_t *cur = deck->symbols[i];
    if (cur && cur->key && strcmp(cur->key, key) == 0) {
      // Shift the rest of the array down
      for (int j = i; j < deck->num_symbols - 1; ++j) {
        deck->symbols[j] = deck->symbols[j + 1];
      }
      deck->num_symbols--;
      // Optionally shrink the array
      if (deck->num_symbols > 0) {
        deck->symbols = realloc(deck->symbols, sizeof(key_value_t*) * deck->num_symbols);
      } else {
        free(deck->symbols);
        deck->symbols = NULL;
      }
      return;
    }
  }
}

/******************************************************************************
 * is_comment/is_geometry/is_control/is_extension
 *
 * The series of "is" functions test a card code against the mnemonic
 * lists and return boolean true if the card belongs to that class,
 * like "isComment" which returns true for any comment card.
 *
 */
bool is_comment(const card_t *card)
{
  bool isCmt = false;
  for(int i = 0; i < NUM_COMMENT_CODES; i++) {
    if(strcmp(card->card_code, comment_codes[i]) == 0) {
      isCmt = true;
      break;
    }
  }
  return isCmt;
}

bool is_geometry(const card_t *card)
{
  bool isGeo = false;
  for(int i = 0; i < NUM_GEOMETRY_CODES; i++) {
    if(strcmp(card->card_code, geometry_codes[i]) == 0) {
      isGeo = true;
      break;
    }
  }
  return isGeo;
}

bool is_control(const card_t *card)
{
  bool isCtl = false;
  for(int i = 0; i < NUM_CONTROL_CODES; i++) {
    if(strcmp(card->card_code, control_codes[i]) == 0) {
      isCtl = true;
      break;
    }
  }
  return isCtl;
}

bool is_extension(const card_t *card)
{
  bool isExt = false;
  for(int i = 0; i < NUM_ONEC_CODES; i++) {
    if(strcmp(card->card_code, onec_codes[i]) == 0) {
      isExt = true;
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
int min_int_fields(const card_t* card)
{
  // GE has zero minimum fields
  if(strcmp(card->card_code, "GE") == 0) return 0; // the ground type is optional
  if(strcmp(card->card_code, "GF") == 0) return 0; // there is an option to print extra data
  if(strcmp(card->card_code, "GC") == 0) return 0; // tapers use I's from previous GW
  // SP/SC uses only one int or none, but it's in position 2, so nothing to do here
  
  if(strcmp(card->card_code, "EK") == 0) return 1; // flag for on or off
  if(strcmp(card->card_code, "EN") == 0) return 0;

  // now the default cases
  if(is_geometry(card)) {
    return 2;
  } else if (is_control(card)) {
    return 4;
  } else {
    return 0; // need to check this!
  }
}

int max_int_fields(const card_t* card)
{
  if(strcmp(card->card_code, "GF") == 0) return 1; // there is an option to print extra data
  if(strcmp(card->card_code, "GC") == 0) return 2; // tapers use I's from previous GW
  // SP/SC uses only one int or none, but it's in position 2, so nothing to do here

  if(strcmp(card->card_code, "EK") == 0) return 1; // flag for on or off
  if(strcmp(card->card_code, "EN") == 0) return 0;

  // now the default cases
  if(is_geometry(card)) {
    return 2;
  } else if (is_control(card)) {
    return 4;
  } else {
    return 0; // need to check this!
  }
}

int min_flt_fields(const card_t* card)
{
  // these are taken from the NEC-2 dox unless noted otherwise
  // the dox indicate optional parameters with () around the name
  if(strcmp(card->card_code, "GA") == 0) return 4; // arcs have four inputs
  if(strcmp(card->card_code, "GE") == 0) return 0; // no floats
  if(strcmp(card->card_code, "GR") == 0) return 0; // no floats
  if(strcmp(card->card_code, "GS") == 0) return 1; // scale
  if(strcmp(card->card_code, "GC") == 0) return 3; // tapers have three inputs
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
  if(is_geometry(card)) {
    return 7;
  } else if (is_control(card)) {
    return 4;
  } else {
    return 0; // need to check this!
  }
}

int max_flt_fields(const card_t* card)
{
  if(strcmp(card->card_code, "GA") == 0) return 4; // arcs have four inputs
  if(strcmp(card->card_code, "GE") == 0) return 0; // no floats
  if(strcmp(card->card_code, "GR") == 0) return 0; // no floats
  if(strcmp(card->card_code, "GS") == 0) return 1; // scale
  if(strcmp(card->card_code, "GC") == 0) return 3; // tapers have three inputs
  if(strcmp(card->card_code, "GX") == 0) return 0; // uses only the ints
  if(strcmp(card->card_code, "SP") == 0) return 6; // last field unused
  if(strcmp(card->card_code, "SC") == 0) return 6; // even in the three-input case, zeros are used

  // EX format depends on the value in I1
  if(strcmp(card->card_code, "EX") == 0) {
    if(card->i[1] == 0 || card->i[1] == 5)
      return 3;
    else
      return 6;
  }
  
  if(strcmp(card->card_code, "FR") == 0) return 2; // can be stepped

  // now the default cases
  if(is_geometry(card)) {
    return 7;
  } else if (is_control(card)) {
    return 6;
  } else {
    return 0; // need to check this!
  }
}

/******************************************************************************
 * isGeometryEdited
 *
 * isGeometryEdited loops through the geometry section of the deck and looks
 * for any edited (or new) card, which means we need to re-run the geometry
 * creation code. It's the only section that needs this, there's nothing
 * to recalculate for the comments, and the command section regenerates its
 * output every time.
 *
 */
bool isGeometryEdited(deck_t *deck)
{
  bool isEdited = false;
  for(int i = deck->geometry_start; i < deck->geometry_end; i++) {
    if(deck->cards[i].edited) {
      isEdited = true;
      break;
    }
  }
  return isEdited;
}

/******************************************************************************
 * convert_awg_to_meters
 *
 * convert_awg_to_meters returns the radius in meters for a given AWG value.
 * Supports standard gauges (0-40) and large wire gauges (4/0 through 1/0).
 * Large wire gauges are represented as negative values: 4/0=-3, 3/0=-2, 2/0=-1, 1/0=0
 */
double convert_awg_to_meters(double awg_value)
{
  int awg_code = floor(awg_value);

  // any decimal part is bad!
  if(awg_value != awg_code) {
    return -1.0;
  }

  switch(awg_code) {
    // Large wire gauges (negative values represent N/0 format)
    case -3: return 0.11684;   // 4/0 or 0000
    case -2: return 0.104049;  // 3/0 or 000
    case -1: return 0.092658;  // 2/0 or 00
    case 0: return 0.082515;   // 1/0 or 0
    // Standard AWG gauges
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
 * initialize_symbol_table
 *
 * Collects all SY symbols from the deck, adds default symbols (pi, c),
 * and evaluates symbols in the comment section sequentially.
 * This should be called after parse_deck() to prepare the symbol table
 * for sequential evaluation during geometry and control processing.
 *
 * @param deck     The deck to initialize
 * @param errors   Error list for reporting evaluation errors
 */
void initialize_symbol_table(deck_t *deck, errors_list_t *errors)
{
  // Step 1: Initialize symbols array and add default symbols (pi, c, units)
  if (deck->symbols) { 
    free(deck->symbols); 
    deck->symbols = NULL; 
  }
  deck->num_symbols = 0;
  add_default_symbols(deck);
  
  // Step 2: Collect all SY symbols from deck (warns on override attempts)
  update_symbol_list(deck, errors);
  
  // Step 3: Evaluate symbols in comment section sequentially
  // evaluate_symbols_in_comments(deck, errors);
}

/******************************************************************************
 * update_deck_values
 *
 * update_deck_values loops through the entire deck and calls
 * update_card_values on any card that has a formula or unit. Normally called
 * after making a change to any of the SY cards, or just before any deck-wide
 * actions like saving it out or running a calculation.
 */
void update_deck_values(nec_context_t *ctx, deck_t *deck)
{
  // Reinitialize with defaults first
  if (deck->symbols) { 
    free(deck->symbols); 
    deck->symbols = NULL; 
  }
  deck->num_symbols = 0;
  add_default_symbols(deck);
  
  // then add symbols from the
  update_symbol_list(deck, &ctx->errors);
  
  // Evaluate and update
  update_symbol_values(ctx, deck, &ctx->errors);
  update_card_values(deck);
}

/******************************************************************************
 * update_symbol_list
 *
 * update_symbol_list looks for any SY cards in the deck and adds their
 * key/value pairs to the deck's symbol list. Assumes default symbols have
 * already been added, and warns if a deck symbol tries to override a default.
 */
static void update_symbol_list(deck_t *deck, errors_list_t *errors) {
  // Count how many default symbols exist before adding user symbols
  int num_defaults = deck->num_symbols;
  
  // INVARIANT: only add pointers to key_value_t nodes owned by cards (e.g., from card->formulas)
  for (int i = 0; i < deck->num_cards; i++) {
    card_t *card = &deck->cards[i];
    if (strcmp(card->card_code, "SY") == 0 && card->formulas) {
      key_value_t *kv = card->formulas;
      while (kv) {
        // Check if this symbol name conflicts with any existing symbol (case-insensitive)
        for (int j = 0; j < num_defaults; j++) {
          if (deck->symbols[j] && strcasecmp(deck->symbols[j]->key, kv->key) == 0) {
            // if (errors) {
            //   char msg[256];
            //   snprintf(msg, sizeof(msg), "The symbol '%s' conflicts with existing symbol '%s'. The user symbol will take precedence.", kv->key, deck->symbols[j]->key);
            //   add_error(NULL, errors, msg, WARNING);
            // }
            // remove the conflicting default symbol
            remove_symbol(deck, deck->symbols[j]->key);
            num_defaults--; // Adjust count since we removed one
            break;
          }
        }
        
        add_symbol(deck, kv);
        kv = kv->next;
      }
    }
  }
}

/******************************************************************************
 * add_unit_constants
 *
 * Helper function to add unit conversion constants to the symbol table.
 * All unit constants use proper case (e.g., uF, nH) but are converted to
 * lowercase when building tinyexpr variable arrays.
 */
static void add_unit_constants(deck_t *deck)
{
  const struct {
    const char *name;
    double value;
  } units[] = {
    // length units (meters)
    {"m", 1.0},
    {"cm", 0.01},
    {"mm", 0.001},
    {"ft", 0.3048},
    {"in", 0.0254},
    {"mil", 0.0000254},  // 1 mil = 0.001 inch
    
    // capacitance units (farads)
    {"pF", 1e-12},
    {"nF", 1e-9},
    {"uF", 1e-6},
    
    // inductance units (henries)
    {"nH", 1e-9},
    {"uH", 1e-6},
    {"mH", 1e-3}
  };
  
  const int num_units = sizeof(units) / sizeof(units[0]);
  
  for (int u = 0; u < num_units; ++u) {
    bool found = false;
    for (int s = 0; s < deck->num_symbols; ++s) {
      key_value_t *sym = deck->symbols[s];
      if (sym && sym->key && strcasecmp(sym->key, units[u].name) == 0) {
        found = true;
        break;
      }
    }
    if (!found) {
      key_value_t *unit_sym = (key_value_t *)malloc(sizeof(key_value_t));
      unit_sym->key = strdup(units[u].name);
      char value_str[32];
      snprintf(value_str, sizeof(value_str), "%.12g", units[u].value);
      unit_sym->value = strdup(value_str);
      unit_sym->fv = units[u].value;
      unit_sym->separator = '=';
      unit_sym->next = NULL;
      add_symbol(deck, unit_sym);
    }
  }
  
  // add AWG wire gauge constants (awg0 through awg40)
  for (int awg = 0; awg <= 40; ++awg) {
    char name[8];
    snprintf(name, sizeof(name), "awg%d", awg);
    
    bool found = false;
    for (int s = 0; s < deck->num_symbols; ++s) {
      key_value_t *sym = deck->symbols[s];
      if (sym && sym->key && strcasecmp(sym->key, name) == 0) {
        found = true;
        break;
      }
    }
    if (!found) {
      double radius = convert_awg_to_meters((double)awg);
      if (radius > 0) {
        key_value_t *awg_sym = (key_value_t *)malloc(sizeof(key_value_t));
        awg_sym->key = strdup(name);
        char value_str[32];
        snprintf(value_str, sizeof(value_str), "%.12g", radius);
        awg_sym->value = strdup(value_str);
        awg_sym->fv = radius;
        awg_sym->separator = '=';
        awg_sym->next = NULL;
        add_symbol(deck, awg_sym);
      }
    }
  }
}

/******************************************************************************
 * add_default_symbols
 *
 * After adding the user-defined symbols from SY cards, this looks to see if
 * pi and c are defined, and if not, adds them with default values.
 * Also adds unit conversion constants (m, cm, mm, ft, in, pF, nF, uF, nH, uH, awg0-awg40).
 */
void add_default_symbols(deck_t *deck)
{
  /* Ensure 'pi' and 'c' are defined as symbols if not already present */
  const struct
  {
    const char *name;
    const char *value;
  } defaults[] = {
      {"pi", "3.141592653589793"},
      {"c", "299792458"}};
  for (int d = 0; d < 2; ++d)
  {
    bool found = false;
    for (int s = 0; s < deck->num_symbols; ++s) {
      key_value_t *sym = deck->symbols[s];
      if (sym && sym->key && strcasecmp(sym->key, defaults[d].name) == 0) {
        found = true;
        break;
      }
    }
    if (!found)
    {
      key_value_t *def_sym = (key_value_t *)malloc(sizeof(key_value_t));
      def_sym->key = strdup(defaults[d].name);
      def_sym->value = strdup(defaults[d].value);
      def_sym->separator = '=';
      def_sym->next = NULL;
       add_symbol(deck, def_sym);
    }
  }
  
  // add unit conversion constants
  add_unit_constants(deck);
} 

/******************************************************************************
 * update_symbol_values
 *
 * Evaluates formulas for each symbol in the deck and stores the result in fv.
 * Symbols are calculated in dependency order: if a symbol's formula references
 * other symbols, those referenced symbols are evaluated first.
 */
void update_symbol_values(nec_context_t *ctx, deck_t *deck, errors_list_t *errors) 
{
  key_value_t **syms = deck->symbols;
  bool *evaluated = calloc(deck->num_symbols, sizeof(bool));
  if (ctx) ctx->eval_depth = 0;
  for (int i = 0; i < deck->num_symbols; i++) if (eval_symbol(i, deck->num_symbols, syms, evaluated, deck, ctx, errors) != 0) {
    // error already added
  }
  free(evaluated);
}

// helper: check if formula references a symbol
static bool references(const char *expr, const char *symname) {
    if (!expr || !symname) return false;
    size_t len = strlen(symname);
    const char *p = expr;
    while ((p = strstr(p, symname))) {
        if ((p == expr || !isalnum((unsigned char)p[-1])) && (!isalnum((unsigned char)p[len]))) {
            return true;
        }
        p += len;
    }
    return false;
}

// Helper function for better formula error messages
static char *get_formula_error_description(const char *formula, int error_pos) {
  if (!formula || error_pos < 0 || error_pos >= (int)strlen(formula)) {
    return NULL;
  }
  
  // look for function-like patterns around the error position
  const char *pos = formula + error_pos;
  
  // look backwards from the error position to find the start of a potential function name
  const char *start = pos - 1;  // Start from the character before the error
  while (start >= formula && (isalnum((unsigned char)*start) || *start == '_')) {
    start--;
  }
  start++; // Move past the non-alphanumeric character we stopped at
  
  // check if this looks like a function call (we have a function name followed by '(')
  if (pos > start && *pos == '(') {
    // extract the function name
    size_t name_len = pos - start;
    if (name_len > 0 && name_len < 50) {
      char func_name[64];
      strncpy(func_name, start, name_len);
      func_name[name_len] = '\0';
      
      // check if it looks like a valid identifier
      if (isalpha((unsigned char)func_name[0]) || func_name[0] == '_') {
        char error_msg[128];
        snprintf(error_msg, sizeof(error_msg), "unknown function '%s'", func_name);
        return strdup(error_msg);
      }
    }
  }
  
  return NULL; // no specific error description available
}

/******************************************************************************
 * preprocess_awg
 *
 * Preprocesses AWG syntax in formulas, converting #14 or 14awg to radius in meters.
 * Also handles special large wire gauges: 4/0, 3/0, 2/0, 1/0, 0, 00, 000, 0000
 */
static char *preprocess_awg(const char *formula) {
  char *result = strdup(formula);
  char *p = result;
  
  while (*p) {
    if (*p == '#') {
      // Found #, check if followed by digits or special gauge notation
      char *start = p;
      char *num_start = p + 1;
      double gauge_value = 0;
      bool valid_awg = false;
      
      // Check for special large wire gauges
      if (strncasecmp(num_start, "4/0", 3) == 0 || strncasecmp(num_start, "0000", 4) == 0) {
        gauge_value = -3;
        valid_awg = true;
        num_start += (strncasecmp(num_start, "4/0", 3) == 0) ? 3 : 4;
      } else if (strncasecmp(num_start, "3/0", 3) == 0 || strncasecmp(num_start, "000", 3) == 0) {
        gauge_value = -2;
        valid_awg = true;
        num_start += (strncasecmp(num_start, "3/0", 3) == 0) ? 3 : 3;
      } else if (strncasecmp(num_start, "2/0", 3) == 0 || strncasecmp(num_start, "00", 2) == 0) {
        gauge_value = -1;
        valid_awg = true;
        num_start += (strncasecmp(num_start, "2/0", 3) == 0) ? 3 : 2;
      } else if (strncasecmp(num_start, "1/0", 3) == 0) {
        gauge_value = 0;
        valid_awg = true;
        num_start += 3;
      } else {
        // Try to parse as regular number
        char *endptr;
        long gauge = strtol(num_start, &endptr, 10);
        if (endptr > num_start && gauge >= 0 && gauge <= 40) {
          gauge_value = (double)gauge;
          valid_awg = true;
          num_start = endptr;
        }
      }
      
      if (valid_awg) {
        // Convert AWG gauge to radius in meters
        double radius = convert_awg_to_meters(gauge_value);
        
        // Replace #NN with the numerical value
        char replacement[32];
        snprintf(replacement, sizeof(replacement), "%.10f", radius);
        
        // Calculate lengths
        size_t prefix_len = start - result;
        size_t replacement_len = strlen(replacement);
        size_t suffix_len = strlen(num_start);
        
        // Allocate new string
        char *new_result = malloc(prefix_len + replacement_len + suffix_len + 1);
        memcpy(new_result, result, prefix_len);
        memcpy(new_result + prefix_len, replacement, replacement_len);
        memcpy(new_result + prefix_len + replacement_len, num_start, suffix_len + 1);
        
        free(result);
        result = new_result;
        p = result + prefix_len + replacement_len;
      } else {
        p++;
      }
    } else if (isdigit(*p) || *p == '-') {
      // Check for "NNawg" format
      char *num_start = p;
      char *endptr;
      
      // Try to parse the number
      long gauge = strtol(num_start, &endptr, 10);
      
      // Check if followed by "awg" (case insensitive)
      if (endptr > num_start && strncasecmp(endptr, "awg", 3) == 0) {
        double gauge_value = (double)gauge;
        
        // Handle negative values for large wire gauges
        // The parser may have already converted 4/0 to -3, etc.
        double radius = convert_awg_to_meters(gauge_value);
        
        // Replace NNawg with the numerical value
        char replacement[32];
        snprintf(replacement, sizeof(replacement), "%.10f", radius);
        
        // Calculate lengths
        size_t prefix_len = num_start - result;
        size_t replacement_len = strlen(replacement);
        char *suffix_start = endptr + 3; // skip "awg"
        size_t suffix_len = strlen(suffix_start);
        
        // Allocate new string
        char *new_result = malloc(prefix_len + replacement_len + suffix_len + 1);
        memcpy(new_result, result, prefix_len);
        memcpy(new_result + prefix_len, replacement, replacement_len);
        memcpy(new_result + prefix_len + replacement_len, suffix_start, suffix_len + 1);
        
        free(result);
        result = new_result;
        p = result + prefix_len + replacement_len;
      } else {
        p++;
      }
    } else {
      p++;
    }
  }
  
  return result;
}

/******************************************************************************
 * preprocess_feet_inches
 *
 * Preprocesses feet/inches syntax in formulas:
 * - "N ft / M in" -> "N*ft + M*in" (feet and inches combined)
 * - "Nft/in" -> "N*ft/in" (feet divided by inches for unit conversion)
 */
static char *preprocess_feet_inches(const char *formula) {
  char *result = strdup(formula);
  char *p = result;
  
  while (*p) {
    // Look for "ft/" pattern
    if (strncasecmp(p, "ft/", 3) == 0) {
      char *ft_pos = p;
      char *slash_pos = p + 2; // position of '/'
      char *after_slash = slash_pos + 1;
      
      // Skip whitespace after /
      while (*after_slash == ' ' || *after_slash == '\t') after_slash++;
      
      // Check what comes after the slash
      if (strncasecmp(after_slash, "in", 2) == 0) {
        // Pattern: Nft/in (without a number after slash)
        // Find the feet number before "ft"
        char *before_ft = ft_pos - 1;
        while (before_ft >= result && (*before_ft == ' ' || *before_ft == '\t')) before_ft--;
        
        // Find start of feet number
        char *feet_start = before_ft;
        while (feet_start > result && (isdigit(*(feet_start-1)) || *(feet_start-1) == '.' || *(feet_start-1) == '-')) feet_start--;
        
        if (feet_start <= before_ft && (isdigit(*feet_start) || *feet_start == '-')) {
          // Found a number before ft
          // Replace: Nft/in -> N*ft/in
          char *endptr;
          double feet_value = strtod(feet_start, &endptr);
          if (endptr >= ft_pos - 1) { // number extends to ft
            char replacement[128];
            snprintf(replacement, sizeof(replacement), "%.10f*ft/in", feet_value);
            
            // Calculate lengths
            size_t prefix_len = feet_start - result;
            size_t replacement_len = strlen(replacement);
            char *suffix_start = after_slash + 2; // skip "in"
            size_t suffix_len = strlen(suffix_start);
            
            // Allocate new string
            char *new_result = malloc(prefix_len + replacement_len + suffix_len + 1);
            memcpy(new_result, result, prefix_len);
            memcpy(new_result + prefix_len, replacement, replacement_len);
            memcpy(new_result + prefix_len + replacement_len, suffix_start, suffix_len + 1);
            
            free(result);
            result = new_result;
            p = result + prefix_len + replacement_len;
            continue;
          }
        }
      } else if ((isdigit(*after_slash) || (*after_slash == '-' && isdigit(*(after_slash+1))))) {
        // Original pattern: N ft / M in
        char *inches_start = after_slash;
        char *endptr;
        
        // Parse inches number
        double inches_value = strtod(inches_start, &endptr);
        if (endptr > inches_start) {
          // Skip whitespace
          char *after_inches_num = endptr;
          while (*after_inches_num == ' ' || *after_inches_num == '\t') after_inches_num++;
          
          // Check for "in"
          if (strncasecmp(after_inches_num, "in", 2) == 0) {
            // Found "ft/ number in" pattern
            // Now find the feet number before "ft"
            char *before_ft = ft_pos - 1;
            while (before_ft >= result && (*before_ft == ' ' || *before_ft == '\t')) before_ft--;
            
            // Find start of feet number
            char *feet_start = before_ft;
            while (feet_start >= result && (isdigit(*feet_start) || *feet_start == '.' || *feet_start == '-')) feet_start--;
            feet_start++; // move past the non-digit
            
            // Parse feet number
            double feet_value = strtod(feet_start, &endptr);
            if (endptr == before_ft + 1) { // should end at the space before ft
              // Replace the entire pattern: feet_number ft / inches_number in -> feet_value*ft + inches_value*in
              char replacement[128];
              snprintf(replacement, sizeof(replacement), "%.10f*ft+%.10f*in", feet_value, inches_value);
              
              // Calculate lengths
              size_t prefix_len = feet_start - result;
              size_t replacement_len = strlen(replacement);
              char *suffix_start = after_inches_num + 2; // skip "in"
              size_t suffix_len = strlen(suffix_start);
              
              // Allocate new string
              char *new_result = malloc(prefix_len + replacement_len + suffix_len + 1);
              memcpy(new_result, result, prefix_len);
              memcpy(new_result + prefix_len, replacement, replacement_len);
              memcpy(new_result + prefix_len + replacement_len, suffix_start, suffix_len + 1);
              
              free(result);
              result = new_result;
              p = result + prefix_len + replacement_len;
              continue;
            }
          }
        }
      }
    }
    p++;
  }
  
  return result;
}

/******************************************************************************
 * preprocess_implicit_multiplication
 *
 * Inserts '*' between numbers and unit identifiers to handle implicit multiplication
 * like "135 ft" -> "135*ft", but avoids function calls like "sin(30)".
 * Only processes known unit names to avoid false matches.
 */
static char *preprocess_implicit_multiplication(const char *formula) {
  // Known unit suffixes
  static const char *units[] = {
    "m", "cm", "mm", "ft", "in",
    "pF", "nF", "uF", "mF",
    "pH", "nH", "uH", "mH",
    NULL
  };
  
  char *result = strdup(formula);
  char *p = result;
  
  while (*p) {
    // Look for digit (or end of number) followed by optional spaces, then a unit
    if (isdigit(*p) || (*p == '.' && p > result && isdigit(*(p-1)))) {
      // Find the end of the number
      char *num_end = p;
      while (*num_end && (isdigit(*num_end) || *num_end == '.' || (*num_end == '-' && num_end == p))) num_end++;
      
      // Skip whitespace
      char *after_num = num_end;
      while (*after_num == ' ' || *after_num == '\t') after_num++;
      
      // Check if next chars match a known unit
      if (isalpha(*after_num)) {
        for (int i = 0; units[i] != NULL; i++) {
          size_t unit_len = strlen(units[i]);
          if (strncasecmp(after_num, units[i], unit_len) == 0) {
            // Make sure unit is followed by non-alphanumeric (or end of string)
            char after_unit = after_num[unit_len];
            if (!isalnum(after_unit) && after_unit != '_') {
              // Insert '*' between number and unit
              size_t prefix_len = num_end - result;
              //size_t spaces_len = after_num - num_end;
              size_t suffix_len = strlen(after_num);
              
              char *new_result = malloc(prefix_len + 1 + suffix_len + 1); // +1 for '*'
              memcpy(new_result, result, prefix_len);
              new_result[prefix_len] = '*';
              memcpy(new_result + prefix_len + 1, after_num, suffix_len + 1);
              
              free(result);
              result = new_result;
              p = result + prefix_len + 1 + unit_len;
              goto next_iteration;
            }
          }
        }
      }
      p = num_end;
      continue;
    }
    next_iteration:
    if (*p) p++;
  }
  
  return result;
}

// Recursive evaluation for symbol dependencies
static int eval_symbol(int i, int sym_count, key_value_t **syms, bool *evaluated, deck_t *deck, nec_context_t *ctx, errors_list_t *errors) {
    // Check recursion depth to prevent infinite loops using ctx->eval_depth
    if (ctx) ctx->eval_depth++;
    if (ctx && ctx->eval_depth > 100) {
        char err_msg[256];
        snprintf(err_msg, sizeof(err_msg), "Maximum recursion depth exceeded evaluating symbol '%s'", syms[i]->key);
        add_error(ctx, errors, err_msg, FATAL);
        ctx->eval_depth--;
        return -1;
    }
    
    // Check if already evaluated to prevent infinite recursion
    if (evaluated[i]) {
      if (ctx) ctx->eval_depth--;
      return 0;
    }
    
    // Mark as evaluated immediately to prevent infinite recursion on circular dependencies
    evaluated[i] = true;
    
    key_value_t *sym = syms[i];
    
    // Recursively evaluate all referenced symbols first
    for (int j = 0; j < sym_count; j++) {
      if (i == j) continue;
      if (references(sym->value, syms[j]->key)) {
        if (eval_symbol(j, sym_count, syms, evaluated, deck, ctx, errors) != 0)
          return -1;
      }
    }
    
    // Now evaluate this symbol's formula
    if (sym->value && sym->value[0] != '\0') {
      // Use original case for tinyexpr variables to avoid conflicts
      te_variable *vars = calloc(sym_count, sizeof(te_variable));
      for (int k = 0; k < sym_count; k++) {
        vars[k].name = syms[k]->key;
        vars[k].address = &syms[k]->fv;
        vars[k].type = TE_VARIABLE;
        vars[k].context = NULL;
      }
      
      // Preprocess AWG syntax (#14 -> awg value)
      char *temp_formula = preprocess_awg(sym->value);
      
      // Preprocess feet/inches syntax (10 ft / 2 in -> 10*ft + 2*in)
      char *temp_formula2 = preprocess_feet_inches(temp_formula);
      free(temp_formula);
      
      // Preprocess implicit multiplication (135 ft -> 135*ft)
      char *processed_formula = preprocess_implicit_multiplication(temp_formula2);
      free(temp_formula2);
      
      int err = 0;
      te_expr *expr = te_compile(processed_formula, vars, sym_count, &err);
      if (expr) {
        sym->fv = te_eval(expr);
        te_free(expr);
        
        // DEBUG: Show symbol evaluation
        // fprintf(stderr, "DEBUG: Evaluated symbol '%s' = '%s' -> %g\n", sym->key, processed_formula, sym->fv);
        
      } else {
        // Find which card this symbol belongs to
        int card_num = -1;
        for (int c = 0; c < deck->num_cards; c++) {
          card_t *card = &deck->cards[c];
          if (strcmp(card->card_code, "SY") == 0 && card->formulas) {
            key_value_t *kv = card->formulas;
            while (kv) {
              if (kv == sym) {
                card_num = c + 1;
                break;
              }
              kv = kv->next;
            }
            if (card_num > 0) break;
          }
        }
        char err_msg[256];
        // Try to provide a more descriptive error message
        char *error_desc = get_formula_error_description(processed_formula, err);
        if (card_num > 0) {
          if (error_desc) {
            snprintf(err_msg, sizeof(err_msg), "Error evaluating formula '%s' on card %d: %s", processed_formula, card_num, error_desc);
            free(error_desc);
          } else {
            snprintf(err_msg, sizeof(err_msg), "Error evaluating formula '%s' at position %d on card %d", processed_formula, err, card_num);
          }
        } else {
          if (error_desc) {
            snprintf(err_msg, sizeof(err_msg), "Error evaluating formula '%s': %s", processed_formula, error_desc);
            free(error_desc);
          } else {
            snprintf(err_msg, sizeof(err_msg), "Error evaluating formula '%s' at position %d", processed_formula, err);
          }
        }
        add_error(ctx, errors, err_msg, FATAL);
        free(processed_formula);
        free(vars);
        if (ctx) ctx->eval_depth--;
        return -1;
      }
      free(vars);
    }
    if (ctx) ctx->eval_depth--;
    return 0;
}

/******************************************************************************
 * update_card_values
 *
 * update_card_values looks for any formulas or units on the cards and updates
 * their values. Generally called after any changes to the card or as part
 * of update_deck_values
 *
 */
void update_card_values(deck_t *deck)
{
  for(int c = 0; c < deck->num_cards; c++) {
    card_t *card = &deck->cards[c];
    
    // Skip SY cards - their formulas define symbols, not field values
    // Symbol formulas are evaluated separately in update_symbol_values
    if(strcmp(card->card_code, "SY") == 0) {
      continue;
    }
    
    // first, copy any original input values into the value fields
    for(int i = 1; i <= MAX_INT_FIELDS; i++) {
      card->iv[i] = card->i[i];
    }
    for(int i = 1; i <= MAX_FLT_FIELDS; i++) {
      card->fv[i] = card->f[i];
    }

    // now run any calculations on the fields and copy those in instead
    if(card->formulas != NULL) {
      // Prepare variable bindings for tinyexpr: F1..F8, I1..I4, and all deck symbols
      const int icount = MAX_INT_FIELDS;    // 4
      const int fcount = MAX_FLT_FIELDS;    // number of float fields (NEC: 7)
      double fvals[MAX_FLT_FIELDS + 1];     // 1-based
      double ivals[MAX_INT_FIELDS + 1];     // 1-based
      for(int i = 1; i <= fcount; i++) fvals[i] = card->fv[i];
      for(int i = 1; i <= icount; i++) ivals[i] = (double)card->iv[i];

      int num_syms = deck ? deck->num_symbols : 0;

      // Allocate enough space for all variables
      int max_vars = MAX_FLT_FIELDS + MAX_INT_FIELDS + num_syms;
      te_variable *vars = calloc(max_vars, sizeof(te_variable));
      int v = 0;
      for(int i = 1; i <= fcount; i++) {
        vars[v].name = fnames[i];
        vars[v].address = &fvals[i];
        vars[v].type = TE_VARIABLE;
        v++;
      }
      for(int i = 1; i <= icount; i++) {
        vars[v].name = inames[i];
        vars[v].address = &ivals[i];
        vars[v].type = TE_VARIABLE;
        v++;
      }

      for(int i = 0; i < num_syms; i++) {
        vars[v].name = deck->symbols[i]->key;
        vars[v].address = &deck->symbols[i]->fv;
        vars[v].type = TE_VARIABLE;
        v++;
      }

      // Iterate formulas and evaluate each assignment (float/int targets)
      key_value_t *kv = card->formulas;
      while(kv != NULL) {
        const char *key = kv->key;
        const char *expr_str = kv->value;
        if(key != NULL && expr_str != NULL && key[0] != '\0') {
          char kind = key[0];
          int idx = atoi(key + 1);
          int err = 0;
          
          // Preprocess AWG syntax in the expression
          char *temp_expr = preprocess_awg(expr_str);
          
          // Preprocess feet/inches syntax
          char *temp_expr2 = preprocess_feet_inches(temp_expr);
          free(temp_expr);
          
          // Preprocess implicit multiplication
          char *processed_expr = preprocess_implicit_multiplication(temp_expr2);
          free(temp_expr2);
          
          te_expr *expr = te_compile(processed_expr, vars, v, &err);
          
          if(expr != NULL) {
            double val = te_eval(expr);
            te_free(expr);
            
            // DEBUG: Show formula evaluation results
            //fprintf(stderr, "DEBUG: Card %d (%s) formula %s='%s' -> %g\n", 
            //        c + 1, card->card_code, key, expr_str, val);
            
            if(kind == 'F' && idx >= 1 && idx <= MAX_FLT_FIELDS) {
              card->fv[idx] = val;
              fvals[idx] = val; // keep variables in sync for subsequent formulas
            } else if(kind == 'I' && idx >= 1 && idx <= MAX_INT_FIELDS) {
              int ival = (int)val; // truncate; can switch to rounding if desired
              card->iv[idx] = ival;
              ivals[idx] = (double)ival;
            }
          }
          free(processed_expr);
        }
        kv = kv->next;
      }
      free(vars);
    }
    
    // Unit conversions are now handled through formula evaluation
    // with unit constants in the symbol table (mm=0.001, ft=0.3048, etc.)
    // No post-processing needed here.
  }
} /* update_card_values() */

/******************************************************************************
 * evaluate_formula
 *
 * Evaluates a single formula (key_value_t) using currently-defined symbols
 * in deck->symbols[]. Updates the key_value_t->fv field with the result.
 * Reports errors if symbols are undefined or if there are syntax errors.
 *
 * @param ctx      The context (for add_error)
 * @param formula  The formula to evaluate (its value string will be compiled)
 * @param deck     The deck containing the symbol table
 * @param errors   Error list for reporting undefined symbols or syntax errors
 */
void evaluate_formula(nec_context_t *ctx, key_value_t *formula, deck_t *deck, errors_list_t *errors)
{
  // Prepare variables for tinyexpr - bind all current symbols (original case)
  int num_syms = deck ? deck->num_symbols : 0;
  te_variable *vars = calloc(num_syms, sizeof(te_variable));
  
  for (int k = 0; k < num_syms; k++) {
    vars[k].name = deck->symbols[k]->key;
    vars[k].address = &deck->symbols[k]->fv;
    vars[k].type = TE_VARIABLE;
    vars[k].context = NULL;
  }
  
  // Preprocess AWG syntax (#14 -> awg value)
  char *temp_formula = preprocess_awg(formula->value);
  
  // Preprocess feet/inches syntax (10 ft / 2 in -> 10*ft + 2*in)
  char *temp_formula2 = preprocess_feet_inches(temp_formula);
  free(temp_formula);
  
  // Preprocess implicit multiplication (135 ft -> 135*ft)
  char *processed_formula = preprocess_implicit_multiplication(temp_formula2);
  free(temp_formula2);
  
  // Compile and evaluate
  int err = 0;
  te_expr *expr = te_compile(processed_formula, vars, num_syms, &err);
  
  if (expr) {
    formula->fv = te_eval(expr);
    te_free(expr);
  } else {
    // Report error if compilation failed
    if (errors) {
      char msg[MAX_ERROR_LEN];
      // Try to provide a more descriptive error message
      char *error_desc = get_formula_error_description(processed_formula, err);
      if (error_desc) {
        snprintf(msg, sizeof(msg), "Error evaluating formula '%s = %s': %s", 
                 formula->key, formula->value, error_desc);
        free(error_desc);
      } else {
        snprintf(msg, sizeof(msg), "Error evaluating formula '%s = %s' at position %d", 
                 formula->key, formula->value, err);
      }
      add_error(ctx, errors, msg, WARNING);
    }
  }
  
  free(processed_formula);
  free(vars);
}

/******************************************************************************
 * evaluate_symbols_in_comments
 *
 * Evaluates all SY card formulas in the comment/header section sequentially.
 * This establishes initial symbol values before geometry processing begins.
 *
 * @param ctx      The context
 * @param deck     The deck containing cards and symbols
 * @param errors   Error list for reporting evaluation errors
 */
void evaluate_symbols_in_comments(nec_context_t *ctx, deck_t *deck, errors_list_t *errors)
{
  if (!deck || deck->num_cards == 0) {
    return;
  }
  
  // SY symbols are evaluated separately in update_symbol_values
  // No need to evaluate them here
}