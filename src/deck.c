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
static void update_symbol_values(context_t *ctx, deck_t *deck, errors_list_t *errors);
static void update_card_values(deck_t *deck);
static void add_default_symbols(deck_t *deck);
static void update_symbol_list(deck_t *deck, errors_list_t *errors);

static bool references(const char *expr, const char *symname);
static int eval_symbol(int i, int sym_count, key_value_t **syms, bool *evaluated, deck_t *deck, context_t *ctx, errors_list_t *errors);

/******************************************************************************
 * new_card
 *
 * Creates and returns a new empty card_t.
 *
 * calloc'ing a card will set it up as wanted, but we'll use this explicit
 * constructor mostly as a form of documentation and possible future changes.
 *
 */
card_t *new_card(void)
{
  card_t *card = calloc(1, sizeof(card_t));
  if (card)
  {
    *card = (card_t){
        .edited = false,
        .ignore = false,
        .invisible = false,
        .extn_code = {0}};
  }
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
void free_card(card_t *card)
{
  if (card == NULL)
    return;

  // start with the various strings
  if (card->orig_str != NULL)
    free(card->orig_str);
  if (card->card_str != NULL)
    free(card->card_str);
  if (card->extn_str != NULL)
    free(card->extn_str);
  if (card->comment != NULL)
    free(card->comment);

  // now the two lists
  key_value_t *head, *temp;
  head = card->formulas;
  while (head != NULL)
  {
    temp = head;
    head = head->next;
    free(temp);
  }
  head = card->extensns;
  while (head != NULL)
  {
    temp = head;
    head = head->next;
    free(temp);
  }

  // do NOT free(card) here; cards are part of an array and freed in destroy_deck
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
  deck->symbol_start = -1;
  deck->symbol_end = -1;
  deck->geometry_start = -1;
  deck->geometry_end = -1;
  deck->deck_end = -1;

  // re-calculate the section limits
  for (int i = 0; i < deck->num_cards; i++)
  {
    deck->cards[i].card_num = i + 1;
    card = &deck->cards[i];

    // commented-out (ignored) cards do not contribute to section boundaries
    if (card->ignore)
      continue;

    // the logic here is pretty simple: get the section this card belongs to,
    // if we haven't see a card in that section yet then it's the start, but
    // if we have, keep updating the end until we stop seeing them. that way
    // things like a missing CE won't cause the _end to be -1
    bool isCmt = is_comment(card);
    if (isCmt)
    {
      if (deck->comment_start == -1)
        deck->comment_start = i;
      deck->comment_end = i;
      continue;
    }
    // SY cards form an optional symbol section between CE and first geometry;
    // SY cards that appear elsewhere in the deck are not part of this section
    if (strcmp(card->card_code, "SY") == 0 && deck->geometry_start == -1)
    {
      if (deck->symbol_start == -1)
        deck->symbol_start = i;
      deck->symbol_end = i;
      continue;
    }
    bool isGeo = is_geometry(card);
    if (isGeo)
    {
      if (deck->geometry_start == -1)
        deck->geometry_start = i;
      deck->geometry_end = i;
      continue;
    }
    // the oddball is the end, which is only at the EN card
    if (strcmp(card->card_code, "EN") == 0)
    {
      deck->deck_end = i;
    }
    // NX ends the first section; geometry/section indexes stop here
    if (strcmp(card->card_code, "NX") == 0)
    {
      break;
    }
  } /* for loop over cards */
}

/******************************************************************************
 * Section Lifecycle Functions
 *****************************************************************************/

/******************************************************************************
 * new_section
 *
 * Creates and returns a new empty section_t.
 */
section_t *new_section(void)
{
  section_t *section = (section_t *)calloc(1, sizeof(section_t));
  if (section)
  {
    // Initialize with invalid/empty markers
    section->global_start = -1;
    section->global_end = -1;
    section->comment_start = -1;
    section->comment_end = -1;
    section->symbol_start = -1;
    section->symbol_end = -1;
    section->geometry_start = -1;
    section->geometry_end = -1;
    section->control_start = -1;
    section->control_end = -1;
    section->symbols = NULL;
    section->num_symbols = 0;
    section->ends_with_nx = false;
  }
  return section;
}

/******************************************************************************
 * init_section
 *
 * Initializes a section_t to an empty state.
 */
void init_section(section_t *section)
{
  if (section == NULL)
    return;
  memset(section, 0, sizeof(section_t));
  section->global_start = -1;
  section->global_end = -1;
  section->comment_start = -1;
  section->comment_end = -1;
  section->symbol_start = -1;
  section->symbol_end = -1;
  section->geometry_start = -1;
  section->geometry_end = -1;
  section->control_start = -1;
  section->control_end = -1;
}

/******************************************************************************
 * destroy_section
 *
 * Frees all heap memory owned by a section_t (symbols pointer array).
 * Does NOT free the section_t struct itself — the caller controls allocation.
 */
void destroy_section(section_t *section)
{
  if (section == NULL)
    return;

  // Only free the symbols array (array of pointers), not the nodes themselves
  // INVARIANT: All key_value_t nodes referenced by section->symbols are owned
  //            and freed by the cards, this is a list of them for easy access.
  if (section->symbols)
  {
    free(section->symbols);
    section->symbols = NULL;
  }
  section->num_symbols = 0;
}

/******************************************************************************
 * Deck Lifecycle Functions
 *****************************************************************************/

/******************************************************************************
 * new_deck
 *
 * creates and returns a new empty Deck
 *
 */
deck_t *new_deck(void)
{
  deck_t *deck = (deck_t *)calloc(1, sizeof(deck_t));
  if (deck)
  {
    // Initialize section-related fields
    deck->sections = NULL;
    deck->num_sections = 0;
    memset(&deck->deck_errors, 0, sizeof(errors_list_t));
  }
  return deck;
}

/******************************************************************************
 * init_deck
 *
 * Initializes a deck_t to an empty state. Works for both stack- and
 * heap-allocated deck_t instances. Must be called before any other deck
 * operation.
 *
 * @param deck the deck_t to initialize
 */
void init_deck(deck_t *deck)
{
  if (deck == NULL)
    return;
  memset(deck, 0, sizeof(deck_t));
}

/******************************************************************************
 * destroy_deck
 *
 * Frees all heap memory owned by a deck_t (cards and their contents, and the
 * symbols pointer array). Does NOT free the deck_t struct itself — the caller
 * controls allocation.
 *
 * @param deck the deck_t to destroy
 */
void destroy_deck(deck_t *deck)
{
  if (deck == NULL)
    return;

  // free all of the cards first
  for (int i = 0; i < deck->num_cards; i++)
  {
    free_card(&deck->cards[i]);
  }

  // then free the cards array itself
  if (deck->cards)
    free(deck->cards);

  // Free all sections
  if (deck->sections)
  {
    for (int i = 0; i < deck->num_sections; i++)
    {
      if (deck->sections[i])
      {
        destroy_section(deck->sections[i]);
        free(deck->sections[i]);
      }
    }
    free(deck->sections);
  }

  // Free deck-wide errors
  if (deck->deck_errors.errors)
  {
    for (int i = 0; i < deck->deck_errors.num_errors; i++)
    {
      if (deck->deck_errors.errors[i].message)
        free(deck->deck_errors.errors[i].message);
    }
    free(deck->deck_errors.errors);
  }

  // Only free the symbols array (array of pointers), not the nodes themselves
  // INVARIANT: All key_value_t nodes referenced by deck->symbols are owned and
  //            freed by the cards, this is a list of them for easy access.
  if (deck->symbols)
  {
    free(deck->symbols);
  }
}

/******************************************************************************
 * deck_create_sections
 *
 * Creates the sections array from the deck's cards by detecting NX (Next
 * Structure) card boundaries. Each NX card terminates one section and
 * begins a new independent section.
 * 
 * For backward compatibility, this also populates the legacy deck boundary
 * fields (comment_start, geometry_start, etc.) from the first section.
 * 
 * @param deck the deck_t to populate with sections
 * @return 0 on success, -1 on failure
 */
int deck_create_sections(deck_t *deck)
{
  if (deck == NULL || deck->num_cards == 0)
    return -1;

  // Free any existing sections
  if (deck->sections)
  {
    for (int i = 0; i < deck->num_sections; i++)
    {
      if (deck->sections[i])
      {
        destroy_section(deck->sections[i]);
        free(deck->sections[i]);
      }
    }
    free(deck->sections);
    deck->sections = NULL;
    deck->num_sections = 0;
  }

  // Phase 2: Scan for NX cards to determine section boundaries
  // Build a list of section end positions (NX or EN cards)
  int *section_ends = NULL;
  int num_section_ends = 0;
  
  for (int i = 0; i < deck->num_cards; i++)
  {
    card_t *card = &deck->cards[i];
    if (card->ignore) continue;
    
    // NX or EN marks a section boundary
    if (strcmp(card->card_code, "NX") == 0 || strcmp(card->card_code, "EN") == 0)
    {
      num_section_ends++;
      section_ends = (int *)realloc(section_ends, num_section_ends * sizeof(int));
      if (section_ends == NULL)
        return -1;
      section_ends[num_section_ends - 1] = i;
      
      // EN is terminal - no more sections after it
      if (strcmp(card->card_code, "EN") == 0)
        break;
    }
  }
  
  // If no EN or NX found, treat entire deck as one section
  if (num_section_ends == 0)
  {
    num_section_ends = 1;
    section_ends = (int *)malloc(sizeof(int));
    if (section_ends == NULL)
      return -1;
    section_ends[0] = deck->num_cards - 1;  // last card in deck
  }
  
  // Allocate sections array
  deck->num_sections = num_section_ends;
  deck->sections = (section_t **)malloc(deck->num_sections * sizeof(section_t *));
  if (deck->sections == NULL)
  {
    free(section_ends);
    return -1;
  }
  
  // Create and populate each section
  int section_start = 0;
  for (int s = 0; s < deck->num_sections; s++)
  {
    section_t *section = new_section();
    if (section == NULL)
    {
      free(section_ends);
      // Free already allocated sections
      for (int j = 0; j < s; j++)
      {
        destroy_section(deck->sections[j]);
        free(deck->sections[j]);
      }
      free(deck->sections);
      deck->sections = NULL;
      deck->num_sections = 0;
      return -1;
    }
    
    deck->sections[s] = section;
    section->global_start = section_start;
    section->global_end = section_ends[s];
    section->ends_with_nx = (strcmp(deck->cards[section_ends[s]].card_code, "NX") == 0);
    
    // Scan this section to find internal boundaries
    bool sawCM = false, sawCE = false, sawSY = false, sawGx = false, sawGE = false;
    
    for (int i = section_start; i <= section_ends[s]; i++)
    {
      card_t *card = &deck->cards[i];
      if (card->ignore) continue;
      
      const char *code = card->card_code;
      
      // Comment section (CM/CE must be before geometry)
      if (strcmp(code, "CM") == 0 && !sawCM && !sawCE && !sawGx && !sawGE)
      {
        section->comment_start = i;  // absolute index
        sawCM = true;
      }
      if (strcmp(code, "CE") == 0 && !sawCE && !sawGx && !sawGE)
      {
        section->comment_end = i;  // absolute index
        sawCE = true;
      }
      
      // Symbol cards (SY between CE and geometry)
      if (strcmp(code, "SY") == 0 && !sawGx && !sawGE)
      {
        if (!sawSY)
        {
          section->symbol_start = i;  // absolute index
          sawSY = true;
        }
        section->symbol_end = i;  // absolute index
      }
      
      // Geometry section
      if (is_geometry(card) && !sawGx)
      {
        section->geometry_start = i;  // absolute index
        sawGx = true;
      }
      if (strcmp(code, "GE") == 0 && !sawGE)
      {
        section->geometry_end = i;  // absolute index
        sawGE = true;
      }
    }
    
    // Control cards start after GE
    if (section->geometry_end >= 0)
    {
      section->control_start = section->geometry_end + 1;
      section->control_end = section_ends[s];
    }
    
    // Copy section-specific symbols (shallow copy - symbols owned by cards)
    // For now, just reference the deck-wide symbols in the first section
    // Phase 3 will properly scope symbols per-section
    if (s == 0)
    {
      section->symbols = deck->symbols;
      section->num_symbols = deck->num_symbols;
    }
    else
    {
      section->symbols = NULL;
      section->num_symbols = 0;
    }
    
    // Next section starts after current section end
    section_start = section_ends[s] + 1;
  }
  
  free(section_ends);
  
  // Populate legacy deck fields from first section for backward compatibility
  if (deck->num_sections > 0)
  {
    section_t *first = deck->sections[0];
    deck->comment_start = first->comment_start;
    deck->comment_end = first->comment_end;
    deck->symbol_start = first->symbol_start;
    deck->symbol_end = first->symbol_end;
    deck->geometry_start = first->geometry_start;
    deck->geometry_end = first->geometry_end;
    // deck_end stays as the global EN position (already set by parse_deck)
  }
  
  return 0;
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
int append_card(deck_t *deck, card_t *card)
{
  // calloc/realloc the deck and add this card to it
  // there may be performance improvements possible by allocing blocks of 10 or 20 cards at a time
  if (deck->num_cards == 0)
  {
    deck->num_cards++;
    deck->cards = calloc(1, sizeof(card_t));
  }
  else
  {
    deck->num_cards++;
    deck->cards = realloc(deck->cards, deck->num_cards * sizeof(card_t));
  }
  deck->cards[deck->num_cards - 1] = *card;

  // appending a card changes the deck and requires a recalc of that section
  // so we need to make the card edited so this will be noticed
  card->edited = true;

  // new cards inherit the deck's separator style if one has been established
  if (deck->field_sep != FSEP_UNKNOWN)
    deck->cards[deck->num_cards - 1].field_sep = deck->field_sep;

  // refresh the deck layout
  recalculate_sections(deck);

  return 0; // no error for now
}

/******************************************************************************
 * append_card_from_text
 *
 * Allocates and appends a card to the deck from a raw NEC text line.
 * Populates card_code, orig_str, card_str, and comment (for CM/CE/!).
 *
 * @param deck Target deck (non-NULL).
 * @param text Null-terminated card string.
 * @return 0 on success, -1 on allocation or insertion failure.
 */
int append_card_from_text(deck_t *deck, const char *text)
{
    card_t card = {0};
    size_t len = strlen(text);
    card.edited = false;
    card.ignore = false;
    card.card_num = deck->num_cards + 1;
    card.orig_str = calloc(len + 1, 1);
    card.card_str = calloc(len + 1, 1);
    if (!card.orig_str || !card.card_str) {
        free(card.orig_str);
        free(card.card_str);
        return -1;
    }
    memcpy(card.orig_str, text, len);
    memcpy(card.card_str, text, len);
    /* mnemonic: first two non-space chars */
    const char *p = text;
    while (*p && isspace((unsigned char)*p)) p++;
    card.card_code[0] = *p ? *p : '\0';
    card.card_code[1] = (*p && *(p+1)) ? *(p+1) : '\0';
    card.card_code[2] = '\0';
    /* populate comment field for CM / CE / '!' / '#' cards */
    {
        int code_end = 0;
        if (strcmp(card.card_code, "CM") == 0 || strcmp(card.card_code, "CE") == 0)
            code_end = 2;
        else if (card.card_code[0] == '!' || card.card_code[0] == '#')
            code_end = 1;
        if (code_end > 0) {
            const char *rest = p + code_end;
            card.comment = strdup(rest);
        }
    }
    if (insert_card(deck, &card, deck->num_cards) < 0)
        return -1;
    return 0;
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
int insert_card(deck_t *deck, card_t *card, int location)
{
  // sanity check the location
  if (location < 0 || location > deck->num_cards)
    return 1;

  // calloc/realloc space for another card
  if (deck->num_cards == 0)
  {
    deck->num_cards++;
    deck->cards = calloc(1, sizeof(card_t));
  }
  else
  {
    deck->num_cards++;
    deck->cards = realloc(deck->cards, deck->num_cards * sizeof(card_t));
  }

  // shift everything from location onward up one space to make room
  for (int i = deck->num_cards - 1; i > location; i--)
  {
    deck->cards[i] = deck->cards[i - 1];
  }

  // then insert the new one
  deck->cards[location] = *card;

  // appending a card changes the deck and requires a recalc of that section
  // so we need to make the card edited so this will be noticed
  card->edited = true;

  // new cards inherit the deck's separator style if one has been established
  if (deck->field_sep != FSEP_UNKNOWN)
    deck->cards[location].field_sep = deck->field_sep;

  // refresh the deck layout
  recalculate_sections(deck);

  // and we're good to go
  return 0;
}

/******************************************************************************
 * move_card
 *
 * Moves the card at index src to insert-before position dst within the deck's
 * card array.  dst is specified in the original (pre-move) coordinates.
 *
 * Pure in-place memmove: no allocation, no free_card, no ownership change.
 * All internal pointer fields (formulas, comment, extn_str, etc.) are intact.
 *
 * dst == src or dst == src+1 is a no-op (card is already in place).
 *
 * @param deck  the deck_t to operate on
 * @param src   current index of the card to move (0-based)
 * @param dst   insert-before position in the original array (0-based, 0…num_cards)
 * @return 0 on success, 1 if src or dst is out of range
 */
int move_card(deck_t *deck, int src, int dst)
{
  if (src < 0 || src >= deck->num_cards)
    return 1;
  if (dst < 0 || dst > deck->num_cards)
    return 1;
  if (dst == src || dst == src + 1)
    return 0; // already in place

  // dst > src → elements shift left  → card lands at dst-1
  // dst < src → elements shift right → card lands at dst
  int new_idx = (dst > src) ? dst - 1 : dst;

  card_t moving = deck->cards[src]; // shallow copy — all pointers preserved

  if (src < new_idx)
  {
    memmove(&deck->cards[src], &deck->cards[src + 1],
            (size_t)(new_idx - src) * sizeof(card_t));
  }
  else
  {
    memmove(&deck->cards[new_idx + 1], &deck->cards[new_idx],
            (size_t)(src - new_idx) * sizeof(card_t));
  }

  deck->cards[new_idx] = moving;
  recalculate_sections(deck);
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
int remove_card(deck_t *deck, int location)
{
  // sanity check the location
  if (location < 0 || location >= deck->num_cards)
    return 1;

  // get a handle to the card for future references
  card_t temp = deck->cards[location];

  // don't calloc/realloc the deck_t smaller, see
  // https://stackoverflow.com/questions/7078019/using-realloc-to-shrink-the-allocated-memory
  // shift everything after location down one space
  for (int i = location; i < deck->num_cards - 1; i++)
  {
    deck->cards[i] = deck->cards[i + 1];
  }
  deck->num_cards--;

  // free the card
  free_card(&temp);

  // refresh the deck layout
  recalculate_sections(deck);

  // and we're good to go
  return 0;
}

/******************************************************************************
 * card_is_toggleable
 *
 * Returns true if the card can be meaningfully commented out or uncommented.
 * Comment cards (CM, CE, !, ', #) cannot be toggled.
 *
 * @param card the card_t to check
 * @return true if the card is geometry or control (can be enabled/disabled)
 */
bool card_is_toggleable(const card_t *card)
{
  if (card == NULL)
    return false;
  return is_geometry(card) || is_control(card);
}

/******************************************************************************
 * card_disable
 *
 * Comments out a card by setting ignore=true and recording the leading comment
 * marker in card->cmt_code. Uses deck->cmt_code as the marker if set, otherwise
 * falls back to '!'. Calls recalculate_sections so section indices stay correct.
 *
 * @param deck the deck_t containing the card
 * @param card the card_t to disable
 */
void card_disable(deck_t *deck, card_t *card)
{
  if (card == NULL || !card_is_toggleable(card))
    return;
  if (card->ignore)
    return; // already disabled
  card->ignore = true;
  card->cmt_code[0] = (deck->cmt_code != 0) ? deck->cmt_code : '!';
  card->edited = true;
  recalculate_sections(deck);
}

/******************************************************************************
 * card_enable
 *
 * Uncomments a card by clearing ignore and cmt_code, making it active again.
 * Calls recalculate_sections so section indices stay correct.
 *
 * @param deck the deck_t containing the card
 * @param card the card_t to enable
 */
void card_enable(deck_t *deck, card_t *card)
{
  if (card == NULL)
    return;
  if (!card->ignore)
    return; // already enabled
  card->ignore = false;
  card->cmt_code[0] = '\0';
  card->edited = true;
  recalculate_sections(deck);
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
  if (pair != NULL)
  {
    pair->key = (char *)calloc(strlen(key) + 1, sizeof(char));
    strcpy(pair->key, key);
    pair->value = (char *)calloc(strlen(value) + 1, sizeof(char));
    strcpy(pair->value, value);
    if (separator != '\n')
    {
      pair->separator = separator;
    }
    else if (*list == card->formulas)
    {
      pair->separator = '=';
    }
    else
    {
      pair->separator = ':';
    }
    pair->next = NULL;
    if (*list == NULL)
    {
      *list = pair;
    }
    else
    {
      key_value_t *tail = *list;
      while (tail->next != NULL)
        tail = tail->next;
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
const char *lookup_formula(const card_t *card, const char *key)
{
  if (!card || !key)
    return NULL;

  key_value_t *formula = card->formulas;
  while (formula != NULL)
  {
    if (strcmp(formula->key, key) == 0)
    {
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
void add_symbol(deck_t *deck, key_value_t *new_sym)
{
  if (!deck || !new_sym)
    return;
  // INVARIANT: Only add pointers to key_value_t nodes owned by cards (e.g., from card->formulas)
  // Never add separately allocated nodes (e.g., from add_default_symbols) to deck->symbols.
  deck->symbols = realloc(deck->symbols, sizeof(key_value_t *) * (deck->num_symbols + 1));
  deck->symbols[deck->num_symbols] = new_sym;
  deck->num_symbols++;
}

void remove_symbol(deck_t *deck, const char *key)
{
  if (!deck || !deck->symbols || !key)
    return;
  for (int i = 0; i < deck->num_symbols; ++i)
  {
    key_value_t *cur = deck->symbols[i];
    if (cur && cur->key && strcmp(cur->key, key) == 0)
    {
      // Shift the rest of the array down
      for (int j = i; j < deck->num_symbols - 1; ++j)
      {
        deck->symbols[j] = deck->symbols[j + 1];
      }
      deck->num_symbols--;
      // Optionally shrink the array
      if (deck->num_symbols > 0)
      {
        deck->symbols = realloc(deck->symbols, sizeof(key_value_t *) * deck->num_symbols);
      }
      else
      {
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
  for (int i = 0; i < NUM_COMMENT_CODES; i++)
  {
    if (strcmp(card->card_code, comment_codes[i]) == 0)
    {
      isCmt = true;
      break;
    }
  }
  return isCmt;
}

bool is_geometry(const card_t *card)
{
  bool isGeo = false;
  for (int i = 0; i < NUM_GEOMETRY_CODES; i++)
  {
    if (strcmp(card->card_code, geometry_codes[i]) == 0)
    {
      isGeo = true;
      break;
    }
  }
  return isGeo;
}

bool is_control(const card_t *card)
{
  bool isCtl = false;
  for (int i = 0; i < NUM_CONTROL_CODES; i++)
  {
    if (strcmp(card->card_code, control_codes[i]) == 0)
    {
      isCtl = true;
      break;
    }
  }
  return isCtl;
}

bool is_extension(const card_t *card)
{
  bool isExt = false;
  for (int i = 0; i < NUM_ONEC_CODES; i++)
  {
    if (strcmp(card->card_code, onec_codes[i]) == 0)
    {
      isExt = true;
      break;
    }
  }
  return isExt;
}

/* card_has_itag
 *
 * Returns true if the given geometry card assigns an ITG (tag) to the segments
 * it generates. Only a subset of geometry cards actually set a tag: GW, GA and
 * GH per the NEC specification. Other geometry/control cards use I1 for other
 * purposes and should not be treated as assigning tags.
 */
bool card_has_itag(const card_t *card)
{
  if (card == NULL)
    return false;
  if (strcmp(card->card_code, "GW") == 0)
    return true;
  if (strcmp(card->card_code, "GA") == 0)
    return true;
  if (strcmp(card->card_code, "GH") == 0)
    return true;
  return false;
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
int min_int_fields(const card_t *card)
{
  // GE has zero minimum fields
  if (strcmp(card->card_code, "GE") == 0)
    return 0; // the ground type is optional
  if (strcmp(card->card_code, "GF") == 0)
    return 0; // there is an option to print extra data
  if (strcmp(card->card_code, "GC") == 0)
    return 0; // tapers use I's from previous GW
  // SP/SC uses only one int or none, but it's in position 2, so nothing to do here

  if (strcmp(card->card_code, "EK") == 0)
    return 1; // flag for on or off
  if (strcmp(card->card_code, "EN") == 0)
    return 0;

  // now the default cases
  if (is_geometry(card))
  {
    return 2;
  }
  else if (is_control(card))
  {
    return 4;
  }
  else
  {
    return 0; // need to check this!
  }
}

int max_int_fields(const card_t *card)
{
  if (strcmp(card->card_code, "GF") == 0)
    return 1; // there is an option to print extra data
  if (strcmp(card->card_code, "GC") == 0)
    return 2; // tapers use I's from previous GW
  // SP/SC uses only one int or none, but it's in position 2, so nothing to do here

  if (strcmp(card->card_code, "EK") == 0)
    return 1; // flag for on or off
  if (strcmp(card->card_code, "EN") == 0)
    return 0;

  // now the default cases
  if (is_geometry(card))
  {
    return 2;
  }
  else if (is_control(card))
  {
    return 4;
  }
  else
  {
    return 0; // need to check this!
  }
}

int min_flt_fields(const card_t *card)
{
  // these are taken from the NEC-2 dox unless noted otherwise
  // the dox indicate optional parameters with () around the name
  if (strcmp(card->card_code, "GA") == 0)
    return 4; // arcs have four inputs
  if (strcmp(card->card_code, "GE") == 0)
    return 0; // no floats
  if (strcmp(card->card_code, "GR") == 0)
    return 0; // no floats
  if (strcmp(card->card_code, "GS") == 0)
    return 1; // scale
  if (strcmp(card->card_code, "GC") == 0)
    return 3; // tapers have three inputs
  if (strcmp(card->card_code, "GX") == 0)
    return 0; // uses only the ints
  if (strcmp(card->card_code, "SP") == 0)
    return 6; // last field unused
  if (strcmp(card->card_code, "SM") == 0)
    return 6; // last field unused
  if (strcmp(card->card_code, "SC") == 0)
    return 6; // this might only be three if it follows SM, but filled with zeros

  // this one is annoying as the format depends on the value in I1
  if (strcmp(card->card_code, "EX") == 0)
  {
    if (card->i[1] == 0 || card->i[1] == 5)
      return 3;
    else
      return 6;
  }

  if (strcmp(card->card_code, "RF"))
    return 1; // can be a single frequency

  // now the default cases
  if (is_geometry(card))
  {
    return 7;
  }
  else if (is_control(card))
  {
    return 4;
  }
  else
  {
    return 0; // need to check this!
  }
}

int max_flt_fields(const card_t *card)
{
  if (strcmp(card->card_code, "GA") == 0)
    return 4; // arcs have four inputs
  if (strcmp(card->card_code, "GE") == 0)
    return 0; // no floats
  if (strcmp(card->card_code, "GR") == 0)
    return 0; // no floats
  if (strcmp(card->card_code, "GS") == 0)
    return 1; // scale
  if (strcmp(card->card_code, "GC") == 0)
    return 3; // tapers have three inputs
  if (strcmp(card->card_code, "GX") == 0)
    return 0; // uses only the ints
  if (strcmp(card->card_code, "SP") == 0)
    return 6; // last field unused
  if (strcmp(card->card_code, "SC") == 0)
    return 6; // even in the three-input case, zeros are used

  // EX format depends on the value in I1
  if (strcmp(card->card_code, "EX") == 0)
  {
    if (card->i[1] == 0 || card->i[1] == 5)
      return 3;
    else
      return 6;
  }

  if (strcmp(card->card_code, "FR") == 0)
    return 2; // can be stepped

  // now the default cases
  if (is_geometry(card))
  {
    return 7;
  }
  else if (is_control(card))
  {
    return 6;
  }
  else
  {
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
  for (int i = DECK_GEOMETRY_START(deck); i < DECK_GEOMETRY_END(deck); i++)
  {
    if (deck->cards[i].edited)
    {
      isEdited = true;
      break;
    }
  }
  return isEdited;
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
  if (deck->symbols)
  {
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
void update_deck_values(context_t *ctx, deck_t *deck)
{
  // Reinitialize with defaults first
  if (deck->symbols)
  {
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
static void update_symbol_list(deck_t *deck, errors_list_t *errors)
{
  // Count how many default symbols exist before adding user symbols
  int num_defaults = deck->num_symbols;

  // INVARIANT: only add pointers to key_value_t nodes owned by cards (e.g., from card->formulas)
  for (int i = 0; i < deck->num_cards; i++)
  {
    card_t *card = &deck->cards[i];
    // Ignored (commented-out) SY cards must NOT contribute to the symbol table.
    // If an ignored 'SY sg=6 appears before an active SY sg=1, both would be
    // added and tinyexpr would pick up the first (ignored) binding, giving the
    // wrong value. Skip ignored cards here to keep the table clean.
    if (strcmp(card->card_code, "SY") == 0 && card->formulas && !card->ignore)
    {
      key_value_t *kv = card->formulas;
      while (kv)
      {
        // Check if this symbol name conflicts with any existing symbol (case-insensitive)
        for (int j = 0; j < num_defaults; j++)
        {
          if (deck->symbols[j] && strcasecmp(deck->symbols[j]->key, kv->key) == 0)
          {
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
  const struct
  {
    const char *name;
    double value;
  } units[] = {
      // length units (meters)
      {"m", 1.0},
      {"cm", 0.01},
      {"mm", 0.001},
      {"ft", 0.3048},
      {"in", 0.0254},
      {"mil", 0.0000254}, // 1 mil = 0.001 inch

      // capacitance units (farads)
      {"pF", 1e-12},
      {"nF", 1e-9},
      {"uF", 1e-6},

      // inductance units (henries)
      {"nH", 1e-9},
      {"uH", 1e-6},
      {"mH", 1e-3}};

  const int num_units = sizeof(units) / sizeof(units[0]);

  for (int u = 0; u < num_units; ++u)
  {
    bool found = false;
    for (int s = 0; s < deck->num_symbols; ++s)
    {
      key_value_t *sym = deck->symbols[s];
      if (sym && sym->key && strcasecmp(sym->key, units[u].name) == 0)
      {
        found = true;
        break;
      }
    }
    if (!found)
    {
      key_value_t *unit_sym = (key_value_t *)malloc(sizeof(key_value_t));
      unit_sym->key = strdup(units[u].name);
      // Lowercase for case-insensitive matching
      for (char *k = unit_sym->key; *k; k++)
        *k = tolower((unsigned char)*k);
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
  for (int awg = 0; awg <= 40; ++awg)
  {
    char name[8];
    snprintf(name, sizeof(name), "awg%d", awg);

    bool found = false;
    for (int s = 0; s < deck->num_symbols; ++s)
    {
      key_value_t *sym = deck->symbols[s];
      if (sym && sym->key && strcasecmp(sym->key, name) == 0)
      {
        found = true;
        break;
      }
    }
    if (!found)
    {
      double radius = convert_awg_to_meters((double)awg);
      if (radius > 0)
      {
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
 * Also calls add_unit_constants to add the unit conversion constants
 * m, cm, mm, ft, in, pF, nF, uF, nH, uH and awg0-awg40.
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
    for (int s = 0; s < deck->num_symbols; ++s)
    {
      key_value_t *sym = deck->symbols[s];
      if (sym && sym->key && strcasecmp(sym->key, defaults[d].name) == 0)
      {
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
void update_symbol_values(context_t *ctx, deck_t *deck, errors_list_t *errors)
{
  key_value_t **syms = deck->symbols;
  bool *evaluated = calloc(deck->num_symbols, sizeof(bool));
  if (ctx)
    ctx->eval_depth = 0;
  for (int i = 0; i < deck->num_symbols; i++)
    if (eval_symbol(i, deck->num_symbols, syms, evaluated, deck, ctx, errors) != 0)
    {
      // error already added
    }
  free(evaluated);
}

// helper: check if formula references a symbol
static bool references(const char *expr, const char *symname)
{
  if (!expr || !symname)
    return false;
  size_t len = strlen(symname);
  const char *p = expr;
  while ((p = strstr(p, symname)))
  {
    if ((p == expr || !isalnum((unsigned char)p[-1])) && (!isalnum((unsigned char)p[len])))
    {
      return true;
    }
    p += len;
  }
  return false;
}

// Helper function for better formula error messages
static char *get_formula_error_description(const char *formula, int error_pos)
{
  if (!formula || error_pos < 0 || error_pos >= (int)strlen(formula))
  {
    return NULL;
  }

  // look for function-like patterns around the error position
  const char *pos = formula + error_pos;

  // look backwards from the error position to find the start of a potential function name
  const char *start = pos - 1; // Start from the character before the error
  while (start >= formula && (isalnum((unsigned char)*start) || *start == '_'))
  {
    start--;
  }
  start++; // Move past the non-alphanumeric character we stopped at

  // check if this looks like a function call (we have a function name followed by '(')
  if (pos > start && *pos == '(')
  {
    // extract the function name
    size_t name_len = pos - start;
    if (name_len > 0 && name_len < 50)
    {
      char func_name[64];
      strncpy(func_name, start, name_len);
      func_name[name_len] = '\0';

      // check if it looks like a valid identifier
      if (isalpha((unsigned char)func_name[0]) || func_name[0] == '_')
      {
        char error_msg[128];
        snprintf(error_msg, sizeof(error_msg), "unknown function '%s'", func_name);
        return strdup(error_msg);
      }
    }
  }

  return NULL; // no specific error description available
}

// Recursive evaluation for symbol dependencies
static int eval_symbol(int i, int sym_count, key_value_t **syms, bool *evaluated, deck_t *deck, context_t *ctx, errors_list_t *errors)
{
  // Check recursion depth to prevent infinite loops using ctx->eval_depth
  if (ctx)
    ctx->eval_depth++;
  if (ctx && ctx->eval_depth > 100)
  {
    char err_msg[256];
    snprintf(err_msg, sizeof(err_msg), "Maximum recursion depth exceeded evaluating symbol '%s'", syms[i]->key);
    add_error(ctx, errors, err_msg, FATAL);
    ctx->eval_depth--;
    return -1;
  }

  // Check if already evaluated to prevent infinite recursion
  if (evaluated[i])
  {
    if (ctx)
      ctx->eval_depth--;
    return 0;
  }

  // Mark as evaluated immediately to prevent infinite recursion on circular dependencies
  evaluated[i] = true;

  key_value_t *sym = syms[i];

  // Recursively evaluate all referenced symbols first
  for (int j = 0; j < sym_count; j++)
  {
    if (i == j)
      continue;
    if (references(sym->value, syms[j]->key))
    {
      if (eval_symbol(j, sym_count, syms, evaluated, deck, ctx, errors) != 0)
        return -1;
    }
  }

  // Now evaluate this symbol's formula
  if (sym->value && sym->value[0] != '\0')
  {
    // Use original case for tinyexpr variables to avoid conflicts
    te_variable *vars = calloc(sym_count, sizeof(te_variable));
    for (int k = 0; k < sym_count; k++)
    {
      vars[k].name = syms[k]->key;
      vars[k].address = &syms[k]->fv;
      vars[k].type = TE_VARIABLE;
      vars[k].context = NULL;
    }

    // Preprocess AWG syntax (#14 -> awg value)
    char *temp_formula = preprocess_awg(sym->value);

    // Preprocess implicit multiplication (135 ft -> 135*ft)
    char *processed_formula = preprocess_implicit_multiplication(temp_formula);
    free(temp_formula);

    int err = 0;
    te_expr *expr = te_compile(processed_formula, vars, sym_count, &err);
    if (expr)
    {
      sym->fv = te_eval(expr);
      te_free(expr);

      // DEBUG: Show symbol evaluation
      // fprintf(stderr, "DEBUG: Evaluated symbol '%s' = '%s' -> %g\n", sym->key, processed_formula, sym->fv);
    }
    else
    {
      // Find which card this symbol belongs to
      int card_num = -1;
      for (int c = 0; c < deck->num_cards; c++)
      {
        card_t *card = &deck->cards[c];
        if (strcmp(card->card_code, "SY") == 0 && card->formulas)
        {
          key_value_t *kv = card->formulas;
          while (kv)
          {
            if (kv == sym)
            {
              card_num = c + 1;
              break;
            }
            kv = kv->next;
          }
          if (card_num > 0)
            break;
        }
      }
      char err_msg[256];
      // Try to provide a more descriptive error message
      char *error_desc = get_formula_error_description(processed_formula, err);
      if (card_num > 0)
      {
        if (error_desc)
        {
          snprintf(err_msg, sizeof(err_msg), "Error evaluating formula '%s' on card %d: %s", processed_formula, card_num, error_desc);
          free(error_desc);
        }
        else
        {
          snprintf(err_msg, sizeof(err_msg), "Error evaluating formula '%s' at position %d on card %d", processed_formula, err, card_num);
        }
      }
      else
      {
        if (error_desc)
        {
          snprintf(err_msg, sizeof(err_msg), "Error evaluating formula '%s': %s", processed_formula, error_desc);
          free(error_desc);
        }
        else
        {
          snprintf(err_msg, sizeof(err_msg), "Error evaluating formula '%s' at position %d", processed_formula, err);
        }
      }
      add_error(ctx, errors, err_msg, FATAL);
      free(processed_formula);
      free(vars);
      if (ctx)
        ctx->eval_depth--;
      return -1;
    }
    free(vars);
  }
  if (ctx)
    ctx->eval_depth--;
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
  for (int c = 0; c < deck->num_cards; c++)
  {
    card_t *card = &deck->cards[c];

    // Skip SY cards - their formulas define symbols, not field values
    // Symbol formulas are evaluated separately in update_symbol_values
    if (strcmp(card->card_code, "SY") == 0)
    {
      continue;
    }

    // now run any calculations on the fields and copy those in instead
    if (card->formulas != NULL)
    {
      // Prepare variable bindings for tinyexpr: F1..F8, I1..I4, and all deck symbols
      const int icount = MAX_INT_FIELDS; // 4
      const int fcount = MAX_FLT_FIELDS; // number of float fields (NEC: 7)
      double fvals[MAX_FLT_FIELDS + 1];  // 1-based
      double ivals[MAX_INT_FIELDS + 1];  // 1-based
      for (int i = 1; i <= fcount; i++)
        fvals[i] = card->f[i];
      for (int i = 1; i <= icount; i++)
        ivals[i] = (double)card->i[i];

      int num_syms = deck ? deck->num_symbols : 0;

      // Allocate enough space for all variables
      int max_vars = MAX_FLT_FIELDS + MAX_INT_FIELDS + num_syms;
      te_variable *vars = calloc(max_vars, sizeof(te_variable));
      int v = 0;
      for (int i = 1; i <= fcount; i++)
      {
        vars[v].name = fnames[i];
        vars[v].address = &fvals[i];
        vars[v].type = TE_VARIABLE;
        v++;
      }
      for (int i = 1; i <= icount; i++)
      {
        vars[v].name = inames[i];
        vars[v].address = &ivals[i];
        vars[v].type = TE_VARIABLE;
        v++;
      }

      for (int i = 0; i < num_syms; i++)
      {
        vars[v].name = deck->symbols[i]->key;
        vars[v].address = &deck->symbols[i]->fv;
        vars[v].type = TE_VARIABLE;
        v++;
      }

      // Iterate formulas and evaluate each assignment (float/int targets)
      key_value_t *kv = card->formulas;
      while (kv != NULL)
      {
        const char *key = kv->key;
        const char *expr_str = kv->value;
        if (key != NULL && expr_str != NULL && key[0] != '\0')
        {
          char kind = key[0];
          int idx = atoi(key + 1);
          int err = 0;

          // Preprocess AWG syntax in the expression
          char *temp_expr = preprocess_awg(expr_str);

          // Preprocess implicit multiplication
          char *processed_expr = preprocess_implicit_multiplication(temp_expr);
          free(temp_expr);

          // Normalize to lowercase for case-insensitive symbol matching
          for (char *p = processed_expr; *p; p++)
          {
            *p = tolower((unsigned char)*p);
          }

          te_expr *expr = te_compile(processed_expr, vars, v, &err);

          if (expr != NULL)
          {
            double val = te_eval(expr);
            te_free(expr);

            if (kind == 'F' && idx >= 1 && idx <= MAX_FLT_FIELDS)
            {
              card->f[idx] = val;
              fvals[idx] = val; // keep variables in sync for subsequent formulas
            }
            else if (kind == 'I' && idx >= 1 && idx <= MAX_INT_FIELDS)
            {
              int ival = (int)val; // truncate; can switch to rounding if desired
              card->i[idx] = ival;
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
void evaluate_formula(context_t *ctx, key_value_t *formula, deck_t *deck, errors_list_t *errors)
{
  // Prepare variables for tinyexpr - bind all current symbols (original case)
  int num_syms = deck ? deck->num_symbols : 0;
  te_variable *vars = calloc(num_syms, sizeof(te_variable));

  for (int k = 0; k < num_syms; k++)
  {
    vars[k].name = deck->symbols[k]->key;
    vars[k].address = &deck->symbols[k]->fv;
    vars[k].type = TE_VARIABLE;
    vars[k].context = NULL;
  }

  // Preprocess AWG syntax (#14 -> awg value)
  char *temp_formula = preprocess_awg(formula->value);

  // Preprocess implicit multiplication (135 ft -> 135*ft)
  char *processed_formula = preprocess_implicit_multiplication(temp_formula);
  free(temp_formula);

  // Normalize to lowercase for case-insensitive symbol matching
  for (char *p = processed_formula; *p; p++)
  {
    *p = tolower((unsigned char)*p);
  }

  // (debug removed)

  // Compile and evaluate
  int err = 0;
  te_expr *expr = te_compile(processed_formula, vars, num_syms, &err);

  if (expr)
  {
    formula->fv = te_eval(expr);

    te_free(expr);
  }
  else
  {
    // Report error if compilation failed
    if (errors)
    {
      char msg[MAX_ERROR_LEN];
      // Try to provide a more descriptive error message
      char *error_desc = get_formula_error_description(processed_formula, err);
      if (error_desc)
      {
        snprintf(msg, sizeof(msg), "Error evaluating formula '%s = %s': %s",
                 formula->key, formula->value, error_desc);
        free(error_desc);
      }
      else
      {
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
void evaluate_symbols_in_comments(context_t *ctx, deck_t *deck, errors_list_t *errors)
{
  if (!deck || deck->num_cards == 0)
  {
    return;
  }

  // SY symbols are evaluated separately in update_symbol_values
  // No need to evaluate them here
}
