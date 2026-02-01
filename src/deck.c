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
#include "proto.h"
#include "tinyexpr.h"
#include <string.h>

void update_symbol_values(deck_t *deck);

static bool references(const char *expr, const char *symname);
static void eval_symbol(int i, int sym_count, key_value_t **syms, bool *evaluated);

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
  card->edited = FALSE;     // new cards are not edited, by default. this only applies to USER edits!
  card->ignore = FALSE;     // cards should not be ignored by default. should apply to geometry and commands?
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
 * lists and return boolean TRUE if the card belongs to that class,
 * like "isComment" which returns TRUE for any comment card.
 *
 */
int is_comment(const card_t *card)
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

int is_geometry(const card_t *card)
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

int is_control(const card_t *card)
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

int is_extension(const card_t *card)
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

int min_flt_fields(const card_t* card)
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
  if(strcmp(card->card_code, "GC") == 0) return 0; // tapers have three inputs
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
void update_deck_values(deck_t *deck)
{
  update_symbol_list(deck, NULL);
  add_default_symbols(deck); // TEMP: Disabled to test double free
  update_symbol_values(deck);

  // and now the formulas on each of the cards
  for(int i = 0; i < deck->num_cards; i++) {
    update_card_values(&deck->cards[i]);
  }
}

/******************************************************************************
 * update_symbol_list
 *
 * update_symbol_list looks for any SY cards in the deck and adds their
 * key/value pairs to the deck's symbol list. It also checks for redeclarations
 * of symbols and adds a warning to the errors list if found.
 */
void update_symbol_list(deck_t *deck, errors_list_t *errors) {
    if (deck->symbols) { free(deck->symbols); deck->symbols = NULL; }
    deck->num_symbols = 0; // Initialize num_symbols
    // INVARIANT: Only add pointers to key_value_t nodes owned by cards (e.g., from card->formulas)
  for (int i = 0; i < deck->num_cards; i++) {
    card_t *card = &deck->cards[i];
    if (strcmp(card->card_code, "SY") == 0 && card->formulas) {
      key_value_t *kv = card->formulas;
      while (kv) {
        // Check for redeclaration
        bool found = false;
        for (int s = 0; s < deck->num_symbols; ++s) {
          key_value_t *existing = deck->symbols[s];
          if (existing && existing->key && strcmp(existing->key, kv->key) == 0) {
            found = true;
            char msg[256];
            snprintf(msg, sizeof(msg), "Symbol '%s' redeclared in deck.", kv->key);
            add_error(NULL, errors, msg, WARNING);
            break;
          }
        }
        if (!found) {
          add_symbol(deck, kv);
        }
        kv = kv->next;
      }
    }
  }
}

/******************************************************************************
 * add_default_symbols
 *
 * After adding the user-defined symbols from SY cards, this looks to see if
 * pi and c are defined, and if not, adds them with default values.
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
} 

/******************************************************************************
 * update_symbol_values
 *
 * Evaluates formulas for each symbol in the deck and stores the result in fv.
 * Symbols are calculated in dependency order: if a symbol's formula references
 * other symbols, those referenced symbols are evaluated first.
 */
void update_symbol_values(deck_t *deck) 
{
  key_value_t **syms = deck->symbols;
  bool *evaluated = calloc(deck->num_symbols, sizeof(bool));
  for (int i = 0; i < deck->num_symbols; i++) eval_symbol(i, deck->num_symbols, syms, evaluated);
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

// Recursive evaluation for symbol dependencies
static void eval_symbol(int i, int sym_count, key_value_t **syms, bool *evaluated) {
    if (evaluated[i]) return;
    key_value_t *sym = syms[i];
    for (int j = 0; j < sym_count; j++) {
        if (i == j) continue;
        if (references(sym->value, syms[j]->key)) {
            eval_symbol(j, sym_count, syms, evaluated);
        }
    }
    sym->fv = 0.0;
    if (sym->value && sym->value[0] != '\0') {
        te_variable *vars = calloc(sym_count, sizeof(te_variable));
        for (int k = 0; k < sym_count; k++) {
            vars[k].name = syms[k]->key;
            vars[k].address = &syms[k]->fv;
            vars[k].type = TE_VARIABLE;
        }
        int err = 0;
        te_expr *expr = te_compile(sym->value, vars, sym_count, &err);
        if (expr) {
            sym->fv = te_eval(expr);
            te_free(expr);
        }
        free(vars);
    }
    evaluated[i] = true;
}

/******************************************************************************
 * update_card_values
 *
 * update_card_values looks for any formulas or units in the card and updates
 * their values. Generally called after any changes to the card or as part
 * of update_deck_values
 *
 */
void update_card_values(card_t *card)
{
  double ft, det;
  
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
    const int fcount = MAX_FLT_FIELDS;    // number of float fields (NEC: 7)
    const int icount = MAX_INT_FIELDS;    // 4
    double fvals[MAX_FLT_FIELDS + 1];     // 1-based
    double ivals[MAX_INT_FIELDS + 1];     // 1-based
    for(int i = 1; i <= fcount; i++) fvals[i] = card->fv[i];
    for(int i = 1; i <= icount; i++) ivals[i] = (double)card->iv[i];

    // Get deck pointer from card (assume card is part of deck->cards[])
    extern deck_t *current_deck_for_card;
    deck_t *deck = current_deck_for_card;
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
    // Add all deck symbols as variables
    for(int s = 0; s < num_syms; s++) {
      key_value_t *sym = deck->symbols[s];
      if(sym && sym->key && sym->key[0] != '\0') {
        vars[v].name = sym->key;
        vars[v].address = &sym->fv;
        vars[v].type = TE_VARIABLE;
        v++;
      }
    }

    // Iterate formulas and evaluate each assignment (float/int targets)
    key_value_t *kv = card->formulas;
    while(kv != NULL) {
      const char *key = kv->key;
      const char *expr_str = kv->value;
      if(key != NULL && expr_str != NULL && key[0] != '\0') {
        char kind = key[0];
        int idx = atoi(key + 1);
        if(kind == 'F' && idx >= 1 && idx <= MAX_FLT_FIELDS) {
          int err = 0;
          te_expr *expr = te_compile(expr_str, vars, v, &err);
          if(expr != NULL) {
            double val = te_eval(expr);
            te_free(expr);
            card->fv[idx] = val;
            fvals[idx] = val; // keep variables in sync for subsequent formulas
          }
        } else if(kind == 'I' && idx >= 1 && idx <= MAX_INT_FIELDS) {
          int err = 0;
          te_expr *expr = te_compile(expr_str, vars, v, &err);
          if(expr != NULL) {
            double val = te_eval(expr);
            te_free(expr);
            int ival = (int)val; // truncate; can switch to rounding if desired
            card->iv[idx] = ival;
            ivals[idx] = (double)ival; // keep variables in sync for subsequent formulas
          }
        }
      }
      kv = kv->next;
    }
    free(vars);
  }
  
  // and finally, apply any unit conversions - which are only on the flts
  for(int i = 1; i <= MAX_FLT_FIELDS; i++) {
    if(card->units[i] == 0) continue;  // 0 means "no units", so skip it
    
      if(unit_mult[card->units[i]] != 0) {
        card->fv[i] = card->fv[i] * unit_mult[card->units[i]];
      } else {
        // units 6 through 8 are zeros and have to be converted case-by-case
        switch(card->units[i]) {
          case 6:  // ftin: feet + inches (two-digit inches)
            ft = floor(card->fv[i]);
            {
              double frac = card->fv[i] - ft;
              int inches_code = (int)lround(frac * 100.0);
              if(inches_code >= 0 && inches_code <= 11) {
                // Interpret as inches digits (0..11)
                card->fv[i] = ft + (inches_code / 12.0);
              } else {
                // Fallback: treat fractional part as decimal feet (legacy behavior)
                card->fv[i] = ft + frac;
              }
            }
            // convert resulting feet to meters
            card->fv[i] = card->fv[i] * unit_mult[4];
            break;

          case 7:
          case 8: { // AWG
            // Allow non-integer gauges via formulas; round and clamp to valid range
            int gauge = (int)lround(card->fv[i]);
            if(gauge < -3) gauge = -3; // 4/0 -> -3
            if(gauge > 40) gauge = 40;
            det = (36.0 - (double)gauge) / 39.0;
            double mm_diam = 0.127 * pow(92.0, det); // diameter in mm
            // Convert to meters radius: mm -> m (÷1000), then /2
            card->fv[i] = (mm_diam * 0.001) * 0.5; // meters radius
            break;
          }
        } // switch
      } // if can be directly converted
  } // for
} /* update_card_values() */