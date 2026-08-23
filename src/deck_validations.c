/******************************************************************************
 * deck_validations.c
 *
 * deck_validations.c contains code for a number of sanity-checking routines that look
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

#include "internals.h"
#include "deck_validations.h"

// local structs for structural geometry validations
typedef struct
{
  int tag;
  int segs;
  int line;
  double x1, y1, z1, x2, y2, z2;
  double radius;
} wire_info_t;
typedef struct
{
  int line;
  int tag;
  int segStart;
  int segEnd;
} ref_info_t;
typedef struct
{
  int line;
  int tag1;
  int seg1;
  int tag2;
  int seg2;
} tl_ref_t;

typedef struct
{
  int line;
  int segs;
  double total_len;
  double radius;
  char code[3];
} geom_seg_info_t;

// Forward declarations for helper functions
static double point_to_segment_distance(double px, double py, double pz,
                                        double qx1, double qy1, double qz1,
                                        double qx2, double qy2, double qz2);

// Percent-segment resolver duplicated from control.c. 4nec2 allows an input
// integer field to be a percentage of the total segment count, e.g. "50%",
// in which case the value should be rounded within [1,count]. The deck
// validation phase must apply the same logic so range checks see the
// canonical segment index.
static int resolve_pct_segment_local(const context_t *ctx, const card_t *card,
                                     int field_idx, int tag)
{
    if (!card->int_form_inline[field_idx])
        return card->i[field_idx];

    char key[3] = { 'I', (char)('0' + field_idx), '\0' };
    const key_value_t *kv = card->formulas;
    while (kv) {
        if (kv->key && strcmp(kv->key, key) == 0 && kv->value) {
            size_t vlen = strlen(kv->value);
            if (vlen > 1 && kv->value[vlen - 1] == '%') {
                double pct = strtod(kv->value, NULL);
                int count = 0;
                if (tag == 0) {
                    count = ctx->geometry.num_segs;
                } else {
                    for (int i = 0; i < ctx->geometry.num_segs; i++) {
                        if (ctx->geometry.tag_nums[i] == tag)
                            count++;
                    }
                }
                if (count <= 0)
                    return card->i[field_idx];
                int seg = (int)round(pct / 100.0 * (double)count);
                if (seg < 1)
                    seg = 1;
                if (seg > count)
                    seg = count;
                return seg;
            }
            break;
        }
        kv = kv->next;
    }
    return card->i[field_idx];
}
static void check_parallel_wire_segmentation(const context_t *ctx, errors_list_t *errors,
                                             const wire_info_t *wires, int wire_count,
                                             double freq_mhz);
static void check_segment_length_and_radius(const context_t *ctx, errors_list_t *errors,
                                            const wire_info_t *wires, int wire_count,
                                            double freq_mhz, int ek_enabled);
static void check_ge_low_height_hazard(const context_t *ctx, errors_list_t *errors,
                                       const wire_info_t *wires, int wire_count,
                                       int GEType);
static void check_junction_segmentation_consistency(const context_t *ctx, errors_list_t *errors,
                                                    const wire_info_t *wires, int wire_count);
static void check_connected_wire_radius_consistency(const context_t *ctx, errors_list_t *errors,
                                                    const wire_info_t *wires, int wire_count);
/* 4nec2 patch-area sanity rule: A (in lambda^2) should not exceed 1/25 */
static void check_patch_area(const context_t *ctx, errors_list_t *errors);
static void check_connected_wire_radius_consistency(const context_t *ctx, errors_list_t *errors,
                                                    const wire_info_t *wires, int wire_count);
static bool is_geometry_tag_ignored(const deck_t *deck, int tag);
static double estimate_helix_length(double s, double hl, double a1, double b1, double a2, double b2);
static void warn_segment_rules(const context_t *ctx, errors_list_t *errors,
                               const char *code, int line, int segs,
                               double total_len, double radius, double wavelength);
static void validate_geom_seg_info_list(const context_t *ctx, errors_list_t *errors,
                                        const geom_seg_info_t *items, int item_count,
                                        const char *trigger_code, int trigger_line,
                                        double wavelength);

/******************************************************************************
 * is_geometry_tag_ignored
 *
 * Helper to check if a geometry tag is marked as ignored in the deck
 *
 * @param deck the deck to search
 * @param tag the tag number to check
 * @return true if the tag exists and is marked ignored, false otherwise
 */
static bool is_geometry_tag_ignored(const deck_t *deck, int tag)
{
  if (tag <= 0 || deck->geometry_start < 0 || deck->geometry_end < 0)
    return false;

  for (int i = deck->geometry_start; i <= deck->geometry_end; i++)
  {
    if (deck->cards[i].i[1] == tag)
    {
      return deck->cards[i].ignore;
    }
  }
  return false; // tag not found
}

/******************************************************************************
 * validate_section
 *
 * Validates a single section of the deck. Each section must have proper
 * structure: comments (optional), geometry (required), GE card (required),
 * and proper termination (NX or EN).
 *
 * @param ctx the context_t
 * @param deck the deck_t containing this section
 * @param section the section_t to validate
 * @param section_num the section number (1-based) for error messages
 * @param errors the errors_list_t to add new messages to
 */
void validate_section(const context_t *ctx, const deck_t *deck, const section_t *section,
                     int section_num, errors_list_t *errors)
{
  char msg[MAX_ERROR_LEN];
  
  if (section == NULL)
  {
    snprintf(msg, sizeof(msg), "Section %d: NULL section pointer (internal error)", section_num);
    add_error(ctx, errors, msg, FATAL);
    return;
  }
  
  // Validate section has geometry
  if (section->geometry_start < 0 || section->geometry_end < 0)
  {
    snprintf(msg, sizeof(msg), "Section %d (cards %d-%d): missing geometry section",
             section_num, section->global_start + 1, section->global_end + 1);
    add_error(ctx, errors, msg, FATAL);
    return;
  }
  
  // Validate GE card exists
  if (section->geometry_end < 0)
  {
    snprintf(msg, sizeof(msg), "Section %d: missing GE (Geometry End) card",
             section_num);
    add_error(ctx, errors, msg, FATAL);
  }
  else
  {
    // Verify the geometry_end card is actually a GE card
    if (section->geometry_end < deck->num_cards)
    {
      const card_t *ge_card = &deck->cards[section->geometry_end];
      if (strcmp(ge_card->card_code, "GE") != 0)
      {
        snprintf(msg, sizeof(msg), "Section %d: geometry_end points to card %d (%s), not GE (internal error)",
                 section_num, section->geometry_end + 1, ge_card->card_code);
        add_error(ctx, errors, msg, FATAL);
      }
    }
  }
  
  // Validate proper section termination
  if (section->global_end >= 0 && section->global_end < deck->num_cards)
  {
    const card_t *end_card = &deck->cards[section->global_end];
    if (section->ends_with_nx)
    {
      if (strcmp(end_card->card_code, "NX") != 0)
      {
        snprintf(msg, sizeof(msg), "Section %d: marked as ending with NX but card %d is %s (internal error)",
                 section_num, section->global_end + 1, end_card->card_code);
        add_error(ctx, errors, msg, FATAL);
      }
    }
    else
    {
      if (strcmp(end_card->card_code, "EN") != 0)
      {
        snprintf(msg, sizeof(msg), "Section %d: marked as ending with EN but card %d is %s (internal error)",
                 section_num, section->global_end + 1, end_card->card_code);
        add_error(ctx, errors, msg, FATAL);
      }
    }
  }
  
  // Validate control section exists (should start after GE)
  if (section->control_start < 0 && section->geometry_end >= 0)
  {
    // It's possible to have no control cards between GE and NX/EN (unusual but not fatal)
    // Check if there's at least one card between GE and section end
    int cards_after_ge = section->global_end - section->geometry_end;
    if (cards_after_ge > 1)  // More than just the NX/EN card
    {
      // There are cards but control_start wasn't set - this might be a warning
      snprintf(msg, sizeof(msg), "Section %d: has %d cards between GE and %s but no control section detected",
               section_num, cards_after_ge - 1, section->ends_with_nx ? "NX" : "EN");
      add_error(ctx, errors, msg, WARNING);
    }
  }
  
  // Validate section boundaries are consistent
  if (section->global_start > section->global_end)
  {
    snprintf(msg, sizeof(msg), "Section %d: start (%d) > end (%d) (internal error)",
             section_num, section->global_start + 1, section->global_end + 1);
    add_error(ctx, errors, msg, FATAL);
  }
  
  // Validate geometry boundaries within section
  if (section->geometry_start >= 0 && section->geometry_end >= 0)
  {
    if (section->geometry_start > section->geometry_end)
    {
      snprintf(msg, sizeof(msg), "Section %d: geometry start (%d) > geometry end (%d) (internal error)",
               section_num, section->geometry_start + 1, section->geometry_end + 1);
      add_error(ctx, errors, msg, FATAL);
    }
    if (section->geometry_start < section->global_start || section->geometry_end > section->global_end)
    {
      snprintf(msg, sizeof(msg), "Section %d: geometry boundaries outside section range (internal error)",
               section_num);
      add_error(ctx, errors, msg, FATAL);
    }
  }
}

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
void test_deck_structure(const context_t *ctx, const deck_t *deck, errors_list_t *errors)
{
  // A short list of the minimum structure is found in the 4nec2 documentation:
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
  // There are a number of issues with this list:
  //
  // 1) some decks lack any comments, although we consider that fatal
  // 2) you don't need a GW card specifically, any geometry will do
  // 3) the EN is not really required, many decks lack it
  // 4) the FR and EX are not really required, there are other cards that can
  //    trigger the output, and decks producing GF don't need them at all
  //
  // For now, this code demands a minimum deck of five cards, at least one
  // comment, two geometry cards, an FX, and an EX. This will be better tuned
  // as we see more decks in the wild, but it should be enough to catch most
  // of the really broken ones that would cause problems in the processing.
  //
  // TODO: do you need an EX? what about transmission?
  //
  // There are also a number of additional tests performed below for other
  // issues like duplicates of cards that should only exist once, cards in the
  // wrong section of the deck, and similar issues.

  // although these look like they should be bools, we use int
  // so we can report the card number where the duplicate was seen
  int sawCE = 0, sawGx = 0, sawGE = 0, sawEN = 0, sawGF = 0;
  int sawFR = 0, sawSC = 0, sawSP = 0, sawGN = 0, sawGD = 0;
  int sawRP = 0;
  double freq_mhz = 0.0; // first FR base frequency for wavelength-based checks
  int ek_enabled = 0;
  int sawGS = 0, sawLD = 0, sawEX = 0, sawSY = 0;
  int pendingSM = 0; // track SM that must be immediately followed by SC
  int GEType = 0;
  // and some temps
  char *code, *last_code;
  char msg[MAX_ERROR_LEN];
  // track geometry tags seen so far (simple fixed-size set)
  int geom_tags[512];
  int geom_tag_count = 0;
  // capture wire geometries for endpoint connectivity checks
  wire_info_t wires[512];
  int wire_count = 0;
  // capture EX/LD/TL references to validate after geometry collection
  ref_info_t ex_refs[512];
  int ex_ref_count = 0;
  ref_info_t ld_refs[512];
  int ld_ref_count = 0;
  tl_ref_t tl_refs[512];
  int tl_ref_count = 0;
  geom_seg_info_t geom_segs[1024];
  int geom_seg_count = 0;

  // start with some obvious ones
  if (deck->num_cards == 0)
  {
    snprintf(msg, sizeof(msg), "The deck has no cards.");
    add_error(ctx, errors, msg, 2); // this is a critical error, this deck will not process
    return;
  }
  if (deck->num_cards < 5)
  {
    snprintf(msg, sizeof(msg), "A deck has to have at least five cards; one or more comments, one or more Gx cards, a GE, an FX, and an EX.");
    add_error(ctx, errors, msg, 2); // same here, there is no way this will calculate property
    return;
  }

  // Phase 3: Validate all sections
  if (deck->num_sections > 0)
  {
    for (int s = 0; s < deck->num_sections; s++)
    {
      validate_section(ctx, deck, deck->sections[s], s + 1, errors);
    }
  }
  else
  {
    // No sections created - this is an error (should have been created during parse_deck)
    snprintf(msg, sizeof(msg), "Deck has no sections (internal error - deck_create_sections not called)");
    add_error(ctx, errors, msg, FATAL);
  }

  // make sure we can find all the required cards
  last_code = "";
  for (int i = 0; i < deck->num_cards; i++)
  {
    // cache this
    code = deck->cards[i].card_code;

    // start with the checks for the cards we *have* to have, while also looking for duplicates

    // it's legal to have multiple GS cards, but that might be confusing
    if (strcmp(code, "GS") == 0)
    {
      if (sawGS == false)
      {
        sawGS = i;
      }
      else
      {
        snprintf(msg, sizeof(msg), "GS on line %d: appears before the GE; no single measurement type can be defined.", i + 1);
        add_error(ctx, errors, msg, 0); // this will calculate fine, so this is merely a warning
      }
    }
    // NOTE: nec4 does not require a CE or CM, but we'll demand them here for compatibility
    if (strcmp(code, "CE") == 0)
    {
      if (sawCE == false)
      {
        sawCE = i;
      }
      else
      {
        snprintf(msg, sizeof(msg), "CE on line %d: appears before the GE; control cards should follow geometry.", i + 1);
        add_error(ctx, errors, msg, 0);
      }
    }
    if (strcmp(code, "GE") == 0)
    {
      if (sawGE == false)
      {
        sawGE = i;
        GEType = deck->cards[i].i[1];
        // GE should typically follow at least one geometry card
        if (sawGx == false)
        {
          snprintf(msg, sizeof(msg), "GE on line %d: no geometry cards were seen before it.", i + 1);
          add_error(ctx, errors, msg, 0);
        }
      }
      else
      {
        snprintf(msg, sizeof(msg), "GE on line %d: appears before the GE; geometry should follow geometry.", i + 1);
        add_error(ctx, errors, msg, 0);
      }
    }
    if (strcmp(code, "EN") == 0)
    {
      if (sawEN == false)
      {
        sawEN = i;
      }
      else
      {
        snprintf(msg, sizeof(msg), "EN on line %d: appears before the GE; end cards should follow geometry.", i + 1);
        add_error(ctx, errors, msg, 0);
      }
    }

    if (strcmp(code, "NX") == 0)
    {
      // NX must not carry numeric parameters (ONEC comment is allowed)
      bool nx_has_params = false;
      for (int j = 1; j <= MAX_INT_FIELDS; j++)
        if (deck->cards[i].i[j] != 0)
        {
          nx_has_params = true;
          break;
        }
      if (!nx_has_params)
        for (int j = 1; j <= MAX_FLT_FIELDS; j++)
          if (deck->cards[i].f[j] != 0.0)
          {
            nx_has_params = true;
            break;
          }
      if (nx_has_params)
      {
        snprintf(msg, sizeof(msg),
                 "NX on line %d: has numeric parameters; NX takes no parameters (they will be ignored).", i + 1);
        add_error(ctx, errors, msg, WARNING);
      }

      // the first non-ignored card after NX must be CM or CE
      bool found_next_cm = false;
      for (int j = i + 1; j < deck->num_cards; j++)
      {
        if (deck->cards[j].ignore)
          continue;
        if (is_comment(&deck->cards[j]))
        {
          found_next_cm = true;
        }
        break; /* first non-ignored card, comment or not */
      }
      if (!found_next_cm)
      {
        snprintf(msg, sizeof(msg),
                 "NX on line %d: must be immediately followed by a CM card to start the next section.", i + 1);
        add_error(ctx, errors, msg, FATAL);
      }

      /* reset per-section tracking so the next section is validated independently. */
      sawCE = 0;
      sawGx = 0;
      sawGE = 0;
      sawEN = 0;
      sawGF = 0;
      sawFR = 0;
      sawSC = 0;
      sawSP = 0;
      sawGN = 0;
      sawGD = 0;
      sawRP = 0;
      sawGS = 0;
      sawLD = 0;
      sawEX = 0;
      sawSY = 0;
    }

    // NOTE: does a deck really need a EX?

    // and also look for other cards where there can only be one
    if (strcmp(code, "GF") == 0)
    {
      if (sawGF == false)
      {
        sawGF = i;
      }
      else
      {
        snprintf(msg, sizeof(msg), "GF on line %d: appears before the GE; ground settings should follow geometry.", i + 1);
        add_error(ctx, errors, msg, 0);
      }
    }
    if (strcmp(code, "FR") == 0)
    {
      if (sawFR == false)
      {
        sawFR = i;

        if (deck->cards[i].f[1] > 0.0)
        {
          freq_mhz = deck->cards[i].f[1];
        }
      }
      else
      {
        snprintf(msg, sizeof(msg), "FR on line %d: appears before the GE; frequency setup should follow geometry.", i + 1);
        add_error(ctx, errors, msg, 0);
      }
    }
    if (strcmp(code, "EK") == 0)
    {
      // any non-zero I1 enables extended thin-wire kernel
      if (deck->cards[i].ints_used >= 1 && deck->cards[i].i[1] != 0)
      {
        ek_enabled = 1;
      }
    }

    // warn if control cards appear before GE (except CE and cards with specific messages below)
    if (sawGE == false)
    {
      if (is_control(&deck->cards[i]) && strcmp(code, "CE") != 0 &&
          strcmp(code, "EX") != 0 && strcmp(code, "TL") != 0 && strcmp(code, "LD") != 0 &&
          strcmp(code, "FR") != 0 && strcmp(code, "RP") != 0 && strcmp(code, "GN") != 0 && strcmp(code, "GD") != 0 && strcmp(code, "EK") != 0)
      {
        snprintf(msg, sizeof(msg), "%s on line %d: control card that appears before the GE, control cards should follow geometry.", code, i + 1);
        add_error(ctx, errors, msg, 0);
      }
    }

    // specific placement warnings for common control cards
    if (sawGE == false)
    {
      if (strcmp(code, "EX") == 0)
      {
        snprintf(msg, sizeof(msg), "EX on line %d: appears before the GE; excitations should follow geometry.", i + 1);
        add_error(ctx, errors, msg, 0);
      }
      if (strcmp(code, "TL") == 0)
      {
        snprintf(msg, sizeof(msg), "TL on line %d: appears before the GE; transmission lines should follow geometry.", i + 1);
        add_error(ctx, errors, msg, 0);
      }
      if (strcmp(code, "LD") == 0)
      {
        snprintf(msg, sizeof(msg), "LD on line %d: appears before the GE; loading should follow geometry.", i + 1);
        add_error(ctx, errors, msg, 0);
      }
      if (strcmp(code, "FR") == 0)
      {
        snprintf(msg, sizeof(msg), "FR on line %d: appears before the GE; frequency setup should follow geometry.", i + 1);
        add_error(ctx, errors, msg, 0);
      }
      if (strcmp(code, "RP") == 0)
      {
        snprintf(msg, sizeof(msg), "RP on line %d: appears before the GE; pattern requests should follow geometry.", i + 1);
        add_error(ctx, errors, msg, 0);
      }
      if (strcmp(code, "GN") == 0)
      {
        snprintf(msg, sizeof(msg), "GN on line %d: appears before the GE; ground settings should follow geometry.", i + 1);
        add_error(ctx, errors, msg, 0);
      }
      if (strcmp(code, "GD") == 0)
      {
        snprintf(msg, sizeof(msg), "GD on line %d: appears before the GE; ground parameters should follow geometry.", i + 1);
        add_error(ctx, errors, msg, 0);
      }
      if (strcmp(code, "EK") == 0)
      {
        snprintf(msg, sizeof(msg), "EK on line %d: appears before the GE; EK should follow geometry to apply the extended kernel.", i + 1);
        add_error(ctx, errors, msg, 0);
      }
    }

    // GE: optional I1 in {-1,0,1,2}; no floats expected
    if (strcmp(code, "GE") == 0)
    {
      if (deck->cards[i].ints_used > 1)
      {
        snprintf(msg, sizeof(msg), "GE on line %d: has more than one integer input.", i);
        add_error(ctx, errors, msg, 0);
      }
      int gei1 = deck->cards[i].i[1];
      if (!(gei1 == -1 || gei1 == 0 || gei1 == 1 || gei1 == 2))
      {
        snprintf(msg, sizeof(msg), "GE on line %d: I1=%d is outside the typical range {-1,0,1,2}.", i, gei1);
        add_error(ctx, errors, msg, 0);
      }
      if (deck->cards[i].flts_used > 0)
      {
        snprintf(msg, sizeof(msg), "GE on line %d: has floating-point inputs, which are not expected.", i);
        add_error(ctx, errors, msg, 0);
      }
    }

    // TL expects 4 integer locators (tags/segments) and Z0 in F1
    if (strcmp(code, "TL") == 0)
    {
      if (deck->cards[i].ints_used < 4)
      {
        snprintf(msg, sizeof(msg), "TL on line %d: fewer than 4 integer inputs (tag/segment locators).", i);
        add_error(ctx, errors, msg, 0);
      }
      // basic sanity: locators positive
      if (deck->cards[i].i[1] <= 0 || deck->cards[i].i[2] <= 0 || deck->cards[i].i[3] <= 0 || deck->cards[i].i[4] <= 0)
      {
        snprintf(msg, sizeof(msg), "TL on line %d: non-positive tag/segment locator(s).", i);
        add_error(ctx, errors, msg, 0);
      }
      if (deck->cards[i].flts_used < 1)
      {
        snprintf(msg, sizeof(msg), "TL on line %d: no characteristic impedance in F1.", i);
        add_error(ctx, errors, msg, 0);
      }
      else if (deck->cards[i].f[1] == 0.0)
      {
        snprintf(msg, sizeof(msg), "TL on line %d: Z0 = 0 in F1, which is invalid.", i);
        add_error(ctx, errors, msg, 0);
      }
      // record TL endpoints for segment-bounds validation later
      if (deck->cards[i].ints_used >= 4 && tl_ref_count < (int)(sizeof(tl_refs) / sizeof(tl_refs[0])))
      {
        tl_refs[tl_ref_count].line = i + 1;
        tl_refs[tl_ref_count].tag1 = deck->cards[i].i[1];
        tl_refs[tl_ref_count].seg1 = resolve_pct_segment_local(ctx, &deck->cards[i], 2, deck->cards[i].i[1]);
        tl_refs[tl_ref_count].tag2 = deck->cards[i].i[3];
        tl_refs[tl_ref_count].seg2 = resolve_pct_segment_local(ctx, &deck->cards[i], 4, deck->cards[i].i[3]);
        tl_ref_count++;
      }
    }

    // EX, typical voltage source requires 4 integers and non-zero amplitude in F1
    if (strcmp(code, "EX") == 0)
    {
      if (deck->cards[i].ints_used < 4)
      {
        snprintf(msg, sizeof(msg), "EX on line %d: fewer than 4 integer inputs.", i);
        add_error(ctx, errors, msg, 0);
      }
      if (deck->cards[i].flts_used < 1)
      {
        snprintf(msg, sizeof(msg), "EX on line %d: no amplitude in F1.", i);
        add_error(ctx, errors, msg, 0);
      }
      else if (deck->cards[i].f[1] == 0.0)
      {
        snprintf(msg, sizeof(msg), "EX on line %d: zero amplitude in F1.", i);
        add_error(ctx, errors, msg, 0);
      }
      // basic locator sanity: tag and segment positive
      if (deck->cards[i].ints_used >= 3)
      {
        if (deck->cards[i].i[2] <= 0 || deck->cards[i].i[3] <= 0)
        {
          snprintf(msg, sizeof(msg), "EX on line %d: non-positive tag or segment locator.", i);
          add_error(ctx, errors, msg, 0);
        }
      }
      // check for unsupported EX types
      if (deck->cards[i].ints_used >= 1)
      {
        int ex_type = deck->cards[i].i[1];
        if (ex_type == 7)
        {
          snprintf(msg, sizeof(msg), "EX on line %d: type %d is not supported by OpenNEC.", i, ex_type);
          add_error(ctx, errors, msg, 0);
        }
      }
      // record for open-end placement validation later
      if (deck->cards[i].ints_used >= 3)
      {
        ref_info_t r = {
            .line = i + 1,
            .tag = deck->cards[i].i[2],
            .segStart = deck->cards[i].i[3],
            .segEnd = 0};
        if (ex_ref_count < (int)(sizeof(ex_refs) / sizeof(ex_refs[0])))
          ex_ref_count++;
        ex_refs[ex_ref_count - 1] = r;
      }
    }

    // GN: minimal check — at least the ground type integer should be present
    if (strcmp(code, "GN") == 0)
    {
      if (deck->cards[i].ints_used < 1)
      {
        snprintf(msg, sizeof(msg), "GN on line %d: no integer ground type specified.", i);
        add_error(ctx, errors, msg, 0);
      }
      else
      {
        int gn_type = deck->cards[i].i[1];
        if (gn_type < -1 || gn_type > 3)
        {
          snprintf(msg, sizeof(msg), "GN on line %d: type %d is not supported (valid: -1..3).", i, gn_type);
          add_error(ctx, errors, msg, 0);
        }
        /* GN 3 (MiniNec) requires a positive dielectric constant */
        if (gn_type == 3 && (deck->cards[i].flts_used < 1 || deck->cards[i].f[1] <= 0.0))
        {
          snprintf(msg, sizeof(msg), "GN on line %d (MiniNec ground): F1 (dielectric constant) must be positive.", i);
          add_error(ctx, errors, msg, 0);
        }
      }
    }

    // LD: loading — require type, tag, segment and at least one non-zero value
    if (strcmp(code, "LD") == 0)
    {
      // at minimum we need I1 (type), I2 (tag), I3 (segment)
      if (deck->cards[i].ints_used < 3)
      {
        snprintf(msg, sizeof(msg), "LD on line %d: fewer than 3 integer inputs, needs type, tag, segment.", i);
        add_error(ctx, errors, msg, 0);
      }
      else
      {
        int type = deck->cards[i].i[1];
        int tag = deck->cards[i].i[2];
        int seg1 = deck->cards[i].i[3];
        int seg2 = deck->cards[i].i[4];
        if (type < -1 || (type > 7 && type != -1))
        {
          snprintf(msg, sizeof(msg), "LD on line %d: unexpected type I1=%d.", i, type);
          add_error(ctx, errors, msg, 0);
        }
        if (tag <= 0 || seg1 <= 0)
        {
          snprintf(msg, sizeof(msg), "LD on line %d: non-positive tag or segment locator.", i);
          add_error(ctx, errors, msg, 0);
        }
        if (seg2 != 0 && seg2 < seg1)
        {
          snprintf(msg, sizeof(msg), "LD on line %d: end segment I4 < start segment I3.", i);
          add_error(ctx, errors, msg, 0);
        }
      }
      // Require at least one float and encourage non-zero values
      if (deck->cards[i].flts_used < 1)
      {
        snprintf(msg, sizeof(msg), "LD on line %d: has no floating-point load value (e.g., resistance).", i);
        add_error(ctx, errors, msg, 0);
      }
      else if (deck->cards[i].f[1] == 0.0 && deck->cards[i].f[2] == 0.0 && deck->cards[i].f[3] == 0.0)
      {
        snprintf(msg, sizeof(msg), "LD on line %d: load values F1..F3 are all zero.", i);
        add_error(ctx, errors, msg, 0);
      }
      // LD type 6 (LC-trap): L and C are required
      if (deck->cards[i].ints_used >= 1 && deck->cards[i].i[1] == 6)
      {
        if (deck->cards[i].flts_used < 3 || deck->cards[i].f[2] == 0.0 || deck->cards[i].f[3] == 0.0)
        {
          snprintf(msg, sizeof(msg), "LD on line %d (LC-trap): F2 (L) and F3 (C) must be non-zero.", i);
          add_error(ctx, errors, msg, 0);
        }
      }
      // LD type 7 (insulated wire): dielectric constant and coat radius are required
      if (deck->cards[i].ints_used >= 1 && deck->cards[i].i[1] == 7)
      {
        if (deck->cards[i].flts_used < 2 || deck->cards[i].f[1] <= 0.0 || deck->cards[i].f[2] <= 0.0)
        {
          snprintf(msg, sizeof(msg), "LD on line %d (insulated wire): F1 (dielectric constant) and F2 (coat radius) must be positive.", i);
          add_error(ctx, errors, msg, 0);
        }
      }
    }
    // record for open-end placement validation later (only for LD cards)
    if (strcmp(code, "LD") == 0 && deck->cards[i].ints_used >= 3)
    {
      int segStart = resolve_pct_segment_local(ctx, &deck->cards[i], 3, deck->cards[i].i[2]);
      int segEnd   = resolve_pct_segment_local(ctx, &deck->cards[i], 4, deck->cards[i].i[2]);
      ref_info_t r = {
          .line = i + 1,
          .tag = deck->cards[i].i[2],
          .segStart = segStart,
          .segEnd = segEnd};
      if (ld_ref_count < (int)(sizeof(ld_refs) / sizeof(ld_refs[0])))
        ld_ref_count++;
      ld_refs[ld_ref_count - 1] = r;
    }

    // along with some others we want to keep track of

    // we want to see if there are any SY's at all
    if (strcmp(code, "SY") == 0)
    {
      sawSY = true;
    }
    // you can have multiple GN cards, but only the last one is used for a given execution
    if (strcmp(code, "GN") == 0)
    {
      if (sawGN == false)
        sawGN = i;
    }
    // you can have multiple GC cards, but there has to be a GN somewhere
    if (strcmp(code, "GD") == 0)
    {
      if (sawGD == false)
      {
        sawGD = i;
      }
    }
    // you can have multiple SCs, but they have to follow a SP or SM
    if (strcmp(code, "SC") == 0)
    {
      if (sawSC == false)
        sawSC = i;
    }
    if (strcmp(code, "RP") == 0)
    {
      // RP should generally follow FR; warn if FR not yet seen
      if (sawFR == false)
      {
        snprintf(msg, sizeof(msg), "RP on line %d: but no FR has been seen earlier.", i + 1);
        add_error(ctx, errors, msg, 0);
      }
      if (sawSP == false)
        sawSP = i;
      if (sawRP == 0)
        sawRP = i + 1; // mark that we have at least one RP (store 1-based index)
    }
    // you need an EX or LD
    if (strcmp(code, "EX") == 0)
    {
      if (sawEX == false)
        sawEX = i;
    }
    // you should have an EX?
    if (strcmp(code, "LD") == 0)
    {
      if (sawLD == false)
        sawLD = i;
    }

    // geometry cards are a little harder because there are many of them
    for (int j = 0; j < NUM_GEOMETRY_CODES; j++)
    {
      if (strcmp(code, geometry_codes[j]) == 0 && strcmp(code, "GE") != 0)
      {
        if (sawGx == false)
        {
          sawGx = i;
        }
        // there's no else in this case, multiple Gx cards are fine, however
        // we do have a potential problem when we find Gx cards after a GE
        if (sawGx > 0 && sawGE > 0)
        {
          snprintf(msg, sizeof(msg), "%s on line %d: this is a geometry card, but we already saw the GE on card %d.", code, i + 1, sawGE + 1);
          add_error(ctx, errors, msg, 1);
        }
        // record geometry tags seen (I1) for later reference by control cards
        if (deck->cards[i].i[1] > 0)
        {
          int tag_i = deck->cards[i].i[1];
          int found = 0;
          for (int t = 0; t < geom_tag_count; t++)
          {
            if (geom_tags[t] == tag_i)
            {
              found = 1;
              break;
            }
          }
          if (!found && geom_tag_count < (int)(sizeof(geom_tags) / sizeof(geom_tags[0])))
          {
            geom_tags[geom_tag_count++] = tag_i;
          }
        }

        // If this is a GW, capture its endpoints and segment count
        if (strcmp(code, "GW") == 0 && wire_count < (int)(sizeof(wires) / sizeof(wires[0])))
        {
          wire_info_t w;
          w.tag = deck->cards[i].i[1];
          w.segs = deck->cards[i].i[2];
          w.line = i + 1;
          w.x1 = deck->cards[i].f[1];
          w.y1 = deck->cards[i].f[2];
          w.z1 = deck->cards[i].f[3];
          w.x2 = deck->cards[i].f[4];
          w.y2 = deck->cards[i].f[5];
          w.z2 = deck->cards[i].f[6];
          w.radius = deck->cards[i].f[7];
          wires[wire_count++] = w;
        }

        // Per-geometry-card segmentation/radius validation inputs (GW/GA/GH)
        if (geom_seg_count < (int)(sizeof(geom_segs) / sizeof(geom_segs[0])))
        {
          geom_seg_info_t g;
          int captured = 0;
          memset(&g, 0, sizeof(g));
          g.line = i + 1;

          if (strcmp(code, "GW") == 0)
          {
            double dx = deck->cards[i].f[4] - deck->cards[i].f[1];
            double dy = deck->cards[i].f[5] - deck->cards[i].f[2];
            double dz = deck->cards[i].f[6] - deck->cards[i].f[3];
            g.total_len = sqrt(dx * dx + dy * dy + dz * dz);
            g.segs = deck->cards[i].i[2];
            g.radius = deck->cards[i].f[7];
            strncpy(g.code, "GW", sizeof(g.code));
            captured = 1;
          }
          else if (strcmp(code, "GA") == 0)
          {
            double radius = fabs(deck->cards[i].f[1]);
            double theta = fabs(deck->cards[i].f[3] - deck->cards[i].f[2]) * (PI / 180.0);
            g.total_len = radius * theta;
            g.segs = deck->cards[i].i[2];
            g.radius = deck->cards[i].f[4];
            strncpy(g.code, "GA", sizeof(g.code));
            captured = 1;
          }
          else if (strcmp(code, "GH") == 0)
          {
            g.total_len = estimate_helix_length(deck->cards[i].f[1], deck->cards[i].f[2], deck->cards[i].f[3], deck->cards[i].f[4], deck->cards[i].f[5], deck->cards[i].f[6]);
            g.segs = deck->cards[i].i[2];
            g.radius = deck->cards[i].f[7];
            strncpy(g.code, "GH", sizeof(g.code));
            captured = 1;
          }

          if (captured)
          {
            geom_segs[geom_seg_count++] = g;
            if (freq_mhz > 0.0)
            {
              warn_segment_rules(ctx, errors, g.code, g.line, g.segs, g.total_len, g.radius, CVEL / freq_mhz);
            }
          }
        }

        // Transforms that can affect or duplicate geometry; validate current transformed set.
        if (strcmp(code, "GS") == 0)
        {
          double sf = fabs(deck->cards[i].f[1]);
          if (sf > 0.0)
          {
            for (int gi = 0; gi < geom_seg_count; gi++)
            {
              geom_segs[gi].total_len *= sf;
              geom_segs[gi].radius *= sf;
            }
          }
          if (freq_mhz > 0.0)
          {
            validate_geom_seg_info_list(ctx, errors, geom_segs, geom_seg_count, "GS", i + 1, CVEL / freq_mhz);
          }
        }
        else if (strcmp(code, "GM") == 0 || strcmp(code, "GR") == 0 || strcmp(code, "GX") == 0)
        {
          if (freq_mhz > 0.0)
          {
            validate_geom_seg_info_list(ctx, errors, geom_segs, geom_seg_count, code, i + 1, CVEL / freq_mhz);
          }
        }
        break;
      }
    } /* loop over geometry codes */

    // unique one here - it's possible to have any number of GS cards, but
    // it appears you can have multiple GS's, although why you would ever do that is unclear
    if (strcmp(code, "GS") == 0)
    {
      if (sawGS == false)
        sawGS = i;
    }

    // now we look for card pairs, where one card has to follow another

    // GC cards have to follow GW cards
    if (strcmp(code, "GC") == 0 && strcmp(last_code, "GW") != 0)
    {
      snprintf(msg, sizeof(msg), "GC on line %d: but the card above it is not a GW.", i + 1);
      add_error(ctx, errors, msg, 1);
    }

    // FIXME: we should do this, but it has to be calculated first because it might be a formula or units
    //    // GW cards with zero radius have to have a GC after it
    //    if(strcmp(code, "GW") == 0 && deck->cards[i].f[7] == 0.0 && strcmp(deck->cards[i+1].card_code, "GC") != 0) {
    //      snprintf(msg, sizeof(msg), "GW on line %d: with a zero radius, but the card after it is not a GC.", i + 1);
    //      add_error(errors, msg, 1);
    //    }

    // GD cards have to follow GN cards
    if (strcmp(code, "GD") == 0 && strcmp(last_code, "GN") != 0)
    {
      snprintf(msg, sizeof(msg), "GD on line %d: but the card above it is not a GN.", i + 1);
      add_error(ctx, errors, msg, 1);
    }
    if (strcmp(code, "GN") == 0)
    {
      if (i + 1 >= deck->num_cards || strcmp(deck->cards[i + 1].card_code, "GD") != 0)
      {
        snprintf(msg, sizeof(msg), "GN on line %d: but the card after it is not a GD.", i + 1);
        add_error(ctx, errors, msg, 1);
      }
    }

    // GF cards have to be the first item in the geometry section, which
    // means they must follow CE cards, or in an onec deck, an SY
    // FIXME: it could also follow onec comment cards, so this is somewhat complex
    if (strcmp(code, "GF") == 0 && !(strcmp(last_code, "CE") == 0 || strcmp(last_code, "SY") == 0))
    {
      snprintf(msg, sizeof(msg), "GF on line %d: but the card above it is not a CE or SY.", i + 1);
      add_error(ctx, errors, msg, 1);
    }

    // modifiers must follow normal geometry: GM/GR/GX after GA/GH/GW/SP/CW
    if (strcmp(code, "GM") == 0 || strcmp(code, "GR") == 0 || strcmp(code, "GX") == 0)
    {
      if (!(strcmp(last_code, "GA") == 0 || strcmp(last_code, "GH") == 0 || strcmp(last_code, "GW") == 0 ||
            strcmp(last_code, "SP") == 0 || strcmp(last_code, "CW") == 0))
      {
        snprintf(msg, sizeof(msg), "%s on line %d: but the card above it is not a GA, GH, GW, SP or CW.", code, i + 1);
        add_error(ctx, errors, msg, 1);
      }
    }

    // SC cards have to follow an SP or SM, or another SC. this is not an exhaustive
    // test, it should really roll backward until it finds an SP or SM, but...
    if (strcmp(code, "SC") == 0 && !(strcmp(last_code, "SP") == 0 || strcmp(last_code, "SM") == 0 || strcmp(last_code, "SC") == 0))
    {
      snprintf(msg, sizeof(msg), "SC on line %d: the card above it is not an SP, SM or another SC.", i + 1);
      add_error(ctx, errors, msg, 1);
    }
    // if SC follows another SC, ensure there was an SP or SM earlier in the deck
    if (strcmp(code, "SC") == 0 && strcmp(last_code, "SC") == 0)
    {
      int hasAncestor = 0;
      for (int j = i - 2; j >= 0; j--)
      {
        if (strcmp(deck->cards[j].card_code, "SP") == 0 || strcmp(deck->cards[j].card_code, "SM") == 0)
        {
          hasAncestor = 1;
          break;
        }
        // stop if we reach a GE; SC ancestry should be within geometry section
        if (strcmp(deck->cards[j].card_code, "GE") == 0)
          break;
      }
      if (!hasAncestor)
      {
        snprintf(msg, sizeof(msg), "SC on line %d: following an SC, but no earlier SP or SM was found before this SC chain.", i + 1);
        add_error(ctx, errors, msg, 1);
      }
    }
    // SM cards should follow an SP or another SM
    if (strcmp(code, "SM") == 0 && !(strcmp(last_code, "SP") == 0 || strcmp(last_code, "SM") == 0))
    {
      snprintf(msg, sizeof(msg), "SM on line %d: but the card above it is not an SP or another SM.", i + 1);
      add_error(ctx, errors, msg, 1);
    }
    // SM must be immediately followed by SC (set pending on SM, clear on next non-SC)
    if (strcmp(code, "SM") == 0)
    {
      pendingSM = i + 1; // store 1-based line number of SM
    }
    else if (pendingSM)
    {
      if (strcmp(code, "SC") != 0)
      {
        snprintf(msg, sizeof(msg), "SM on line %d: is not followed by an SC.", pendingSM);
        add_error(ctx, errors, msg, 1);
      }
      pendingSM = 0;
    }

    // QUESTION: it appears GR cards have to follow some other sort of geometry card, but it's not clear what

    // FR cards have to have either one input or three
    if (strcmp(code, "FR") == 0)
    {
      if (!(deck->cards[i].ints_used == 1 || deck->cards[i].ints_used == 3))
      {
        snprintf(msg, sizeof(msg), "FR on line %d: does not have 1 or 3 integer inputs.", i + 1);
        add_error(ctx, errors, msg, 0);
      }
    }
    // advance last_code to current card for next-iteration pair checks
    last_code = code;

    // warn about unsupported cards
    if (strcmp(code, "WG") == 0)
    {
      snprintf(msg, sizeof(msg), "WG on line %d: Wire Grid, which is not supported by OpenNEC.", i + 1);
      add_error(ctx, errors, msg, 0);
    }
    if (strcmp(code, "IT") == 0)
    {
      snprintf(msg, sizeof(msg), "IT on line %d: ITeration, which is not supported by OpenNEC.", i + 1);
      add_error(ctx, errors, msg, 0);
    }
    if (strcmp(code, "OP") == 0)
    {
      snprintf(msg, sizeof(msg), "OP on line %d: OPtimization, which is not supported by OpenNEC.", i + 1);
      add_error(ctx, errors, msg, 0);
    }
    // Also warn for any other extension cards that are not supported
    if (is_extension(&deck->cards[i]) && strcmp(code, "SY") != 0 && strcmp(code, "XT") != 0)
    {
      snprintf(msg, sizeof(msg), "%s on line %d: which is not supported by OpenNEC.", code, i + 1);
      add_error(ctx, errors, msg, 0);
    }

    // after recording last_code, validate that certain control cards reference existing geometry tags seen so far
    if (strcmp(code, "EX") == 0 && deck->cards[i].ints_used >= 3)
    {
      int tag = deck->cards[i].i[2];
      if (tag > 0)
      {
        int seen = 0;
        for (int t = 0; t < geom_tag_count; t++)
        {
          if (geom_tags[t] == tag)
          {
            seen = 1;
            break;
          }
        }
        if (!seen)
        {
          snprintf(msg, sizeof(msg), "EX on line %d (tag %d): referencing tag %d, which was not found.", i + 1, tag, tag);
          add_error(ctx, errors, msg, 0);
        }
        else if (is_geometry_tag_ignored(deck, tag))
        {
          snprintf(msg, sizeof(msg), "EX on line %d (tag %d): is marked as ignored.", i + 1, tag);
          add_error(ctx, errors, msg, 0);
        }
      }
    }
    if (strcmp(code, "TL") == 0 && deck->cards[i].ints_used >= 4)
    {
      int tag1 = deck->cards[i].i[1];
      int tag2 = deck->cards[i].i[3];
      if (tag1 > 0)
      {
        int seen1 = 0;
        for (int t = 0; t < geom_tag_count; t++)
        {
          if (geom_tags[t] == tag1)
          {
            seen1 = 1;
            break;
          }
        }
        if (!seen1)
        {
          snprintf(msg, sizeof(msg), "TL on line %d (tag %d): references tag %d (first endpoint), which was not found.", i + 1, tag1, tag1);
          add_error(ctx, errors, msg, 0);
        }
        else if (is_geometry_tag_ignored(deck, tag1))
        {
          snprintf(msg, sizeof(msg), "TL on line %d (tag %d): references tag %d (first endpoint), which is marked as ignored.", i + 1, tag1, tag1);
          add_error(ctx, errors, msg, 0);
        }
      }
      if (tag2 > 0)
      {
        int seen2 = 0;
        for (int t = 0; t < geom_tag_count; t++)
        {
          if (geom_tags[t] == tag2)
          {
            seen2 = 1;
            break;
          }
        }
        if (!seen2)
        {
          snprintf(msg, sizeof(msg), "TL on line %d (tag %d): references tag %d (second endpoint), which was not found.", i + 1, tag2, tag2);
          add_error(ctx, errors, msg, 0);
        }
        else if (is_geometry_tag_ignored(deck, tag2))
        {
          snprintf(msg, sizeof(msg), "TL on line %d (tag %d): references tag %d (second endpoint), which is marked as ignored.", i + 1, tag2, tag2);
          add_error(ctx, errors, msg, 0);
        }
      }
    }
    if (strcmp(code, "LD") == 0 && deck->cards[i].ints_used >= 3)
    {
      int tag = deck->cards[i].i[2];
      if (tag > 0)
      {
        int seen = 0;
        for (int t = 0; t < geom_tag_count; t++)
        {
          if (geom_tags[t] == tag)
          {
            seen = 1;
            break;
          }
        }
        if (!seen)
        {
          snprintf(msg, sizeof(msg), "LD on line %d (tag %d): referencing tag %d, which was not found.", i + 1, tag, tag);
          add_error(ctx, errors, msg, 0);
        }
        else if (is_geometry_tag_ignored(deck, tag))
        {
          snprintf(msg, sizeof(msg), "LD on line %d (tag %d): is marked as ignored.", i + 1, tag);
          add_error(ctx, errors, msg, 0);
        }
      }
    }
  } /* loop over cards */

  // validate EX/LD not located at open wire ends
  for (int wi = 0; wi < wire_count; wi++)
  {
    wire_info_t w = wires[wi];
    int end1_connected = 0, end2_connected = 0;
    for (int wj = 0; wj < wire_count; wj++)
    {
      if (wj == wi)
        continue;
      wire_info_t v = wires[wj];
      if ((w.x1 == v.x1 && w.y1 == v.y1 && w.z1 == v.z1) || (w.x1 == v.x2 && w.y1 == v.y2 && w.z1 == v.z2))
      {
        end1_connected = 1;
      }
      if ((w.x2 == v.x1 && w.y2 == v.y1 && w.z2 == v.z1) || (w.x2 == v.x2 && w.y2 == v.y2 && w.z2 == v.z2))
      {
        end2_connected = 1;
      }
    }
    // check EX references to this wire
    for (int er = 0; er < ex_ref_count; er++)
    {
      if (ex_refs[er].tag == w.tag)
      {
        int seg = ex_refs[er].segStart;
        if (seg == 1 && w.segs > 1 && !end1_connected)
        {
          snprintf(msg, sizeof(msg), "EX on line %d (tag %d): placed on segment 1, which is an open wire end.", ex_refs[er].line, w.tag);
          add_error(ctx, errors, msg, 0);
        }
        if (seg == w.segs && w.segs > 1 && !end2_connected)
        {
          snprintf(msg, sizeof(msg), "EX on line %d (tag %d): placed on segment %d, which is an open wire end.", ex_refs[er].line, w.tag, w.segs);
          add_error(ctx, errors, msg, 0);
        }
      }
    }
    // check LD references to this wire (consider start/end segments)
    for (int lr = 0; lr < ld_ref_count; lr++)
    {
      if (ld_refs[lr].tag == w.tag)
      {
        int s = ld_refs[lr].segStart;
        int e = ld_refs[lr].segEnd;
        if (s == 1 && w.segs > 1 && !end1_connected)
        {
          snprintf(msg, sizeof(msg), "LD on line %d (tag %d): starts at segment 1, which is an open wire end.", ld_refs[lr].line, w.tag);
          add_error(ctx, errors, msg, 0);
        }
        if (e == 0)
          e = s; // single-segment load
        if (e == w.segs && w.segs > 1 && !end2_connected)
        {
          snprintf(msg, sizeof(msg), "LD on line %d (tag %d): ends at segment %d, which is an open wire end.", ld_refs[lr].line, w.tag, w.segs);
          add_error(ctx, errors, msg, 0);
        }
      }
    }
  }

  // parallel wire segmentation check (0.05 wavelengths threshold)
  if (freq_mhz > 0.0 && wire_count > 1)
  {
    check_parallel_wire_segmentation(ctx, errors, wires, wire_count, freq_mhz);
  }
  if (freq_mhz > 0.0 && wire_count > 0)
  {
      check_segment_length_and_radius(ctx, errors, wires, wire_count, freq_mhz, ek_enabled);
  }

  /* 4nec2 patch area rule applies regardless of wires */
  if (ctx->geometry.num_patches > 0)
    check_patch_area(ctx, errors);

  if (wire_count > 0)
  {
    check_ge_low_height_hazard(ctx, errors, wires, wire_count, GEType);
    check_junction_segmentation_consistency(ctx, errors, wires, wire_count);
    check_connected_wire_radius_consistency(ctx, errors, wires, wire_count);
  }

  // TL segment bounds validation: ensure referenced segments exist on the given tags
  if (tl_ref_count > 0 && wire_count > 0)
  {
    for (int r = 0; r < tl_ref_count; r++)
    {
      int segs1 = -1, segs2 = -1;
      for (int w = 0; w < wire_count; w++)
      {
        if (wires[w].tag == tl_refs[r].tag1)
          segs1 = wires[w].segs;
        if (wires[w].tag == tl_refs[r].tag2)
          segs2 = wires[w].segs;
      }
      if (segs1 > 0 && (tl_refs[r].seg1 <= 0 || tl_refs[r].seg1 > segs1))
      {
        snprintf(msg, sizeof(msg), "TL on line %d (tag %d): references an invalid segment on the first endpoint; segment index out of range for the wire tag %d.", tl_refs[r].line, tl_refs[r].tag1, tl_refs[r].tag1);
        add_error(ctx, errors, msg, 0);
      }
      if (segs2 > 0 && (tl_refs[r].seg2 <= 0 || tl_refs[r].seg2 > segs2))
      {
        snprintf(msg, sizeof(msg), "TL on line %d (tag %d): references an invalid segment on the second endpoint; segment index out of range for the wire tag %d.", tl_refs[r].line, tl_refs[r].tag2, tl_refs[r].tag2);
        add_error(ctx, errors, msg, 0);
      }
      // TL self-loop: same tag+segment on both ends
      if (tl_refs[r].tag1 == tl_refs[r].tag2 && tl_refs[r].seg1 == tl_refs[r].seg2)
      {
        snprintf(msg, sizeof(msg), "TL on line %d: connects the same tag and segment on both ends; this is a no-op or invalid connection.", tl_refs[r].line);
        add_error(ctx, errors, msg, 0);
      }
    }
  }

  // LD segment bounds validation: ensure referenced segment range falls within the wire's segment count
  if (ld_ref_count > 0 && wire_count > 0)
  {
    for (int r = 0; r < ld_ref_count; r++)
    {
      int segs = -1;
      for (int w = 0; w < wire_count; w++)
      {
        if (wires[w].tag == ld_refs[r].tag)
        {
          segs = wires[w].segs;
          break;
        }
      }
      if (segs > 0)
      {
        int s = ld_refs[r].segStart;
        int e = ld_refs[r].segEnd == 0 ? ld_refs[r].segStart : ld_refs[r].segEnd;
        if (s <= 0 || s > segs)
        {
          snprintf(msg, sizeof(msg), "LD on line %d (tag %d): references an invalid start segment; out of range for wire tag %d.", ld_refs[r].line, ld_refs[r].tag, ld_refs[r].tag);
          add_error(ctx, errors, msg, 0);
        }
        if (e <= 0 || e > segs)
        {
          snprintf(msg, sizeof(msg), "LD on line %d (tag %d): references an invalid end segment; out of range for wire tag %d.", ld_refs[r].line, ld_refs[r].tag, ld_refs[r].tag);
          add_error(ctx, errors, msg, 0);
        }
      }
    }
  }

  // duplicate wire endpoints: overlapping wires
  for (int i = 0; i < wire_count; i++)
  {
    for (int j = i + 1; j < wire_count; j++)
    {
      wire_info_t a = wires[i];
      wire_info_t b = wires[j];
      int same_dir = (a.x1 == b.x1 && a.y1 == b.y1 && a.z1 == b.z1 && a.x2 == b.x2 && a.y2 == b.y2 && a.z2 == b.z2);
      int reversed = (a.x1 == b.x2 && a.y1 == b.y2 && a.z1 == b.z2 && a.x2 == b.x1 && a.y2 == b.y1 && a.z2 == b.z1);
      if (same_dir || reversed)
      {
        snprintf(msg, sizeof(msg), "GW on line %d (tag %d): has the same endpoints as the wire on line %d (tag %d). Overlapping wires can cause issues.", a.line, a.tag, b.line, b.tag);
        add_error(ctx, errors, msg, 0);
      }
    }
  }

  // Ground intersection: warn if wires cross or extend below z=0 when ground is enabled
  int ground_enabled = ((GEType == 1 || GEType == 2) || sawGN);
  if (ground_enabled)
  {
    for (int wi = 0; wi < wire_count; wi++)
    {
      wire_info_t w = wires[wi];
      double z1 = w.z1, z2 = w.z2;
      if (z1 < 0.0 && z2 < 0.0)
      {
        snprintf(msg, sizeof(msg), "GW on line %d (tag %d): lies below the ground plane (z<0).", w.line, w.tag);
        add_error(ctx, errors, msg, 0);
      }
      else if (z1 * z2 < 0.0)
      {
        snprintf(msg, sizeof(msg), "GW on line %d (tag %d): crosses the ground plane (z=0), which is typically invalid.", w.line, w.tag);
        add_error(ctx, errors, msg, 0);
      }
    }
  }

  // wires that are connected must contact at segment ends (connection separation < len/1000)
  for (int ai = 0; ai < wire_count; ai++)
  {
    wire_info_t a = wires[ai];
    double ax[2] = {a.x1, a.x2};
    double ay[2] = {a.y1, a.y2};
    double az[2] = {a.z1, a.z2};
    for (int end = 0; end < 2; end++)
    {
      double px = ax[end], py = ay[end], pz = az[end];
      for (int bi = 0; bi < wire_count; bi++)
      {
        if (bi == ai)
          continue;
        wire_info_t b = wires[bi];
        double vx = b.x2 - b.x1, vy = b.y2 - b.y1, vz = b.z2 - b.z1;
        double L2 = vx * vx + vy * vy + vz * vz;
        if (L2 == 0.0 || b.segs <= 0)
          continue;
        double L = sqrt(L2);
        double segLen = L / (double)b.segs;
        double tol = segLen / 1000.0;
        // project point onto line b
        double wx = px - b.x1, wy = py - b.y1, wz = pz - b.z1;
        double t = (wx * vx + wy * vy + wz * vz) / L2;
        if (t < 0.0 || t > 1.0)
          continue; // closest point lies outside the wire extent
        double fx = b.x1 + t * vx, fy = b.y1 + t * vy, fz = b.z1 + t * vz;
        double dx = px - fx, dy = py - fy, dz = pz - fz;
        double dist = sqrt(dx * dx + dy * dy + dz * dz);
        if (dist <= tol)
        {
          // near-connected; ensure this footpoint is at a segment endpoint of b
          int isEndpoint = 0;
          for (int k = 0; k <= b.segs; k++)
          {
            double ek = (double)k / (double)b.segs;
            double ex = b.x1 + ek * vx, ey = b.y1 + ek * vy, ez = b.z1 + ek * vz;
            double edx = fx - ex, edy = fy - ey, edz = fz - ez;
            double ed = sqrt(edx * edx + edy * edy + edz * edz);
            if (ed <= tol)
            {
              isEndpoint = 1;
              break;
            }
          }
          if (!isEndpoint)
          {
            snprintf(msg, sizeof(msg), "GW on line %d (tag %d): endpoint is connected near the middle of the wire on line %d (tag %d). Connections must be at segment ends.", a.line, a.tag, b.line, b.tag);
            add_error(ctx, errors, msg, 0);
          }
        }
      }
    }
  }

  // and with the entire deck tested, make sure we got the key cards
  if (!sawCE)
  {
    snprintf(msg, sizeof(msg), "A deck should have a CE card.");
    add_error(ctx, errors, msg, 0);
  }
  if (!sawGx)
  {
    snprintf(msg, sizeof(msg), "A deck has to have at least one geometry card.");
    add_error(ctx, errors, msg, 1);
  }
  if (!sawGE)
  {
    snprintf(msg, sizeof(msg), "A deck has to have a GE card.");
    add_error(ctx, errors, msg, 1);
  }
  if (!sawEN)
  {
    snprintf(msg, sizeof(msg), "A deck should contain an EN card.");
    add_error(ctx, errors, msg, 0);
  }
  // EN should be the last *active* card in the deck. Any subsequent
  // non-comment, non-ignored cards (i.e., real geometry/control/extension
  // cards that are not explicitly commented out) are likely user errors.
  // Free-form text after EN is allowed, and commented-out cards (leading
  // '!', '#', or '\'') are permitted. Iterate any cards after deck->deck_end
  // and report an error if a valid, non-commented card is found.
  if (deck->deck_end >= 0)
  {
    for (int ci = deck->deck_end + 1; ci < deck->num_cards; ci++)
    {
      const card_t *c = &deck->cards[ci];
      // skip cards that are explicitly ignored (commented-out or invisible)
      if (c->ignore)
        continue;
      // skip explicit comment cards
      if (is_comment(c))
        continue;
      // if this looks like a geometry, control, or onec-extension card,
      // report it as an error: valid active cards should not appear after EN
      if (is_geometry(c) || is_control(c) || is_extension(c))
      {
        snprintf(msg, sizeof(msg), "Card on line %d: '%s' appears after EN; non-comment cards after EN are not allowed.", ci + 1, c->card_code);
        add_error(ctx, errors, msg, 1);
      }
      // otherwise treat as freeform text (allowed)
    }
  }
  if (!sawFR && !sawRP)
  {
    snprintf(msg, sizeof(msg), "A deck has to have an FR card.");
    add_error(ctx, errors, msg, 1);
  }
  if (!sawEX && !sawLD)
  {
    snprintf(msg, sizeof(msg), "A deck has to have at least one EX or LD card.");
    add_error(ctx, errors, msg, 1);
  }
  if (sawSY && !sawCE)
  {
    snprintf(msg, sizeof(msg), "SY on line %d: there is no CE in the deck. SYs should follow the CE.", sawSY);
    add_error(ctx, errors, msg, 0);
  }
  // warn if GD appears without any preceding GN
  if (sawGD && !sawGN)
  {
    snprintf(msg, sizeof(msg), "GD on line %d: no GN card in the deck.", sawGD);
    add_error(ctx, errors, msg, 1);
  }

  // if the GE card was -1, there has to be a GN
  if (sawGE && GEType == -1 && !sawGN)
  {
    snprintf(msg, sizeof(msg), "GE on line %d; set to -1, but there is no GN card in the deck.", sawGE);
    add_error(ctx, errors, msg, 1);
  }
}

static double estimate_helix_length(double s, double hl, double a1, double b1, double a2, double b2)
{
  double abs_hl = fabs(hl);
  if (abs_hl <= 0.0)
    return 0.0;
  if (fabs(s) < 1.0e-12)
    return abs_hl;

  double turns = fabs(hl / s);
  double b1_eff = (b1 == 0.0) ? a1 : b1;
  double b2_eff = (b2 == 0.0) ? a2 : b2;
  double a = 0.5 * (fabs(a1) + fabs(a2));
  double b = 0.5 * (fabs(b1_eff) + fabs(b2_eff));

  double perim;
  if (fabs(a - b) <= 1.0e-12)
  {
    perim = 2.0 * PI * a;
  }
  else
  {
    perim = PI * (3.0 * (a + b) - sqrt((3.0 * a + b) * (a + 3.0 * b)));
  }

  {
    double circum_path = perim * turns;
    return sqrt(circum_path * circum_path + abs_hl * abs_hl);
  }
}

static void warn_segment_rules(const context_t *ctx, errors_list_t *errors,
                               const char *code, int line, int segs,
                               double total_len, double radius, double wavelength)
{
  char msg[MAX_ERROR_LEN];
  if (segs <= 0 || total_len <= 0.0 || wavelength <= 0.0)
    return;

  {
    double seg_len = total_len / (double)segs;
    double min_seg = 1.0e-4 * wavelength;
    if (seg_len < min_seg)
    {
      snprintf(msg, sizeof(msg), "%s on line %d: segment length %.6g is smaller than 1e-4 wavelength (%.6g).", code, line, seg_len, min_seg);
      add_error(ctx, errors, msg, WARNING);
    }

    if (radius > 0.0)
    {
      double min_r_by_seg = 0.5 * seg_len;
      double min_r_by_wav = 0.1 * wavelength;
      if (radius < min_r_by_seg || radius < min_r_by_wav)
      {
        snprintf(msg, sizeof(msg), "%s on line %d: wire radius %.6g is smaller than 0.5*segment length (%.6g) or 0.1*wavelength (%.6g).", code, line, radius, min_r_by_seg, min_r_by_wav);
        add_error(ctx, errors, msg, WARNING);
      }
    }
  }
}

static void validate_geom_seg_info_list(const context_t *ctx, errors_list_t *errors,
                                        const geom_seg_info_t *items, int item_count,
                                        const char *trigger_code, int trigger_line,
                                        double wavelength)
{
  char msg[MAX_ERROR_LEN];
  for (int i = 0; i < item_count; i++)
  {
    const geom_seg_info_t *g = &items[i];
    if (g->segs <= 0 || g->total_len <= 0.0 || wavelength <= 0.0)
      continue;

    {
      double seg_len = g->total_len / (double)g->segs;
      double min_seg = 1.0e-4 * wavelength;
      double min_r_by_seg = 0.5 * seg_len;
      double min_r_by_wav = 0.1 * wavelength;

      if (seg_len < min_seg)
      {
        snprintf(msg, sizeof(msg), "%s on line %d: transformed geometry from %s line %d has segment length %.6g < 1e-4 wavelength (%.6g).", trigger_code, trigger_line, g->code, g->line, seg_len, min_seg);
        add_error(ctx, errors, msg, WARNING);
      }
      if (g->radius > 0.0 && (g->radius < min_r_by_seg || g->radius < min_r_by_wav))
      {
        snprintf(msg, sizeof(msg), "%s on line %d: transformed geometry from %s line %d has radius %.6g smaller than 0.5*segment (%.6g) or 0.1*wavelength (%.6g).", trigger_code, trigger_line, g->code, g->line, g->radius, min_r_by_seg, min_r_by_wav);
        add_error(ctx, errors, msg, WARNING);
      }
    }
  }
}

// Helper: point-to-segment distance in 3D
static double point_to_segment_distance(double px, double py, double pz,
                                        double qx1, double qy1, double qz1,
                                        double qx2, double qy2, double qz2)
{
  double vx = qx2 - qx1, vy = qy2 - qy1, vz = qz2 - qz1;
  double L2 = vx * vx + vy * vy + vz * vz;
  if (L2 == 0.0)
    return 1e9;
  double wx = px - qx1, wy = py - qy1, wz = pz - qz1;
  double t = (wx * vx + wy * vy + wz * vz) / L2;
  if (t < 0.0)
    t = 0.0;
  else if (t > 1.0)
    t = 1.0;
  double fx = qx1 + t * vx, fy = qy1 + t * vy, fz = qz1 + t * vz;
  double dx = px - fx, dy = py - fy, dz = pz - fz;
  return sqrt(dx * dx + dy * dy + dz * dz);
}

// Helper: warn if parallel wires closer than 0.05 wavelengths have different segmentation
static void check_parallel_wire_segmentation(const context_t *ctx, errors_list_t *errors,
                                             const wire_info_t *wires, int wire_count,
                                             double freq_mhz)
{
  char msg[MAX_ERROR_LEN];
  double wlam_m = (freq_mhz > 0.0) ? (CVEL / freq_mhz) : 0.0; // CVEL in m/us vs MHz => meters
  if (wlam_m <= 0.0)
  {
    return;
  }
  double thr = 0.05 * wlam_m;

  for (int i = 0; i < wire_count; i++)
  {
    for (int j = i + 1; j < wire_count; j++)
    {
      const wire_info_t *a = &wires[i];
      const wire_info_t *b = &wires[j];
      // direction vectors
      double ax = a->x2 - a->x1, ay = a->y2 - a->y1, az = a->z2 - a->z1;
      double bx = b->x2 - b->x1, by = b->y2 - b->y1, bz = b->z2 - b->z1;
      double al = sqrt(ax * ax + ay * ay + az * az);
      double bl = sqrt(bx * bx + by * by + bz * bz);
      if (al == 0.0 || bl == 0.0)
        continue;
      // unit direction vectors
      ax /= al;
      ay /= al;
      az /= al;
      bx /= bl;
      by /= bl;
      bz /= bl;
      // parallel if |cross| small or |dot| close to 1
      double cx = ay * bz - az * by;
      double cy = az * bx - ax * bz;
      double cz = ax * by - ay * bx;
      double cross_mag = sqrt(cx * cx + cy * cy + cz * cz);
      double dot = ax * bx + ay * by + az * bz;
      if (cross_mag > 1e-3 && fabs(dot) < 0.999)
        continue; // not parallel enough

      // minimal distance between segments (approx): sample endpoints to other segment lines
      // point-to-line distance from a->x1 to b, and a->x2 to b, and vice versa; take min
      double min_dist = thr * 10.0; // init larger than thr
      double d1 = point_to_segment_distance(a->x1, a->y1, a->z1, b->x1, b->y1, b->z1, b->x2, b->y2, b->z2);
      double d2 = point_to_segment_distance(a->x2, a->y2, a->z2, b->x1, b->y1, b->z1, b->x2, b->y2, b->z2);
      double d3 = point_to_segment_distance(b->x1, b->y1, b->z1, a->x1, a->y1, a->z1, a->x2, a->y2, a->z2);
      double d4 = point_to_segment_distance(b->x2, b->y2, b->z2, a->x1, a->y1, a->z1, a->x2, a->y2, a->z2);
      min_dist = fmin(fmin(d1, d2), fmin(d3, d4));

      if (min_dist < thr)
      {
        double segA = al / (double)a->segs;
        double segB = bl / (double)b->segs;
        // different segmentation: either segment counts differ, or segment lengths differ >10%
        double rel = fabs(segA - segB) / fmax(segA, segB);
        if (a->segs != b->segs || rel > 0.10)
        {
          snprintf(msg, sizeof(msg), "Parallel wires (lines %d and %d, tags %d and %d) are closer than 0.05 wavelengths but have different segmentation.", a->line, b->line, a->tag, b->tag);
          add_error(ctx, errors, msg, 0);
        }
      }
    }
  }
}

// Helper: junction segmentation consistency — connected wire endpoints should have similar segment lengths
static void check_junction_segmentation_consistency(const context_t *ctx, errors_list_t *errors,
                                                    const wire_info_t *wires, int wire_count)
{
  char msg[MAX_ERROR_LEN];
  for (int i = 0; i < wire_count; i++)
  {
    const wire_info_t *a = &wires[i];
    if (a->segs <= 0)
      continue;
    double aL = sqrt(pow(a->x2 - a->x1, 2) + pow(a->y2 - a->y1, 2) + pow(a->z2 - a->z1, 2));
    double aSeg = aL / (double)a->segs;
    for (int j = i + 1; j < wire_count; j++)
    {
      const wire_info_t *b = &wires[j];
      if (b->segs <= 0)
        continue;
      double bL = sqrt(pow(b->x2 - b->x1, 2) + pow(b->y2 - b->y1, 2) + pow(b->z2 - b->z1, 2));
      double bSeg = bL / (double)b->segs;
      double tol = fmax(fmin(aSeg, bSeg) / 1000.0, 1e-9);
      // check direct endpoint-to-endpoint distances for proximity
      double d11 = sqrt(pow(a->x1 - b->x1, 2) + pow(a->y1 - b->y1, 2) + pow(a->z1 - b->z1, 2));
      double d12 = sqrt(pow(a->x1 - b->x2, 2) + pow(a->y1 - b->y2, 2) + pow(a->z1 - b->z2, 2));
      double d21 = sqrt(pow(a->x2 - b->x1, 2) + pow(a->y2 - b->y1, 2) + pow(a->z2 - b->z1, 2));
      double d22 = sqrt(pow(a->x2 - b->x2, 2) + pow(a->y2 - b->y2, 2) + pow(a->z2 - b->z2, 2));
      int connected = (d11 <= tol) || (d12 <= tol) || (d21 <= tol) || (d22 <= tol);
      if (connected)
      {
        double rel = fabs(aSeg - bSeg) / fmax(aSeg, bSeg);
        
        if (rel > 0.20)
        {
          snprintf(msg, sizeof(msg), "Connected wires (lines %d and %d, tags %d and %d) have very different segment lengths near the junction (%.4g m vs %.4g m); consider harmonizing segmentation.", a->line, b->line, a->tag, b->tag, aSeg, bSeg);
          add_error(ctx, errors, msg, 0);
        }
        /* 4nec2: warn if one segment length is more than 5× the other */
        if (aSeg < 0.2 * bSeg || bSeg < 0.2 * aSeg)
        {
          snprintf(msg, sizeof(msg), "Connected wires (lines %d and %d, tags %d and %d) have a junction length ratio >5:1 (%.4g vs %.4g); this may cause accuracy issues.",
                   a->line, b->line, a->tag, b->tag, aSeg, bSeg);
          add_error(ctx, errors, msg, 0);
        }
      }
    }
  }
}

// Helper: connected wires should use the same radius at the junction
static void check_connected_wire_radius_consistency(const context_t *ctx, errors_list_t *errors,
                                                    const wire_info_t *wires, int wire_count)
{
  char msg[MAX_ERROR_LEN];
  for (int i = 0; i < wire_count; i++)
  {
    const wire_info_t *a = &wires[i];
    if (a->segs <= 0)
      continue;

    double aL = sqrt(pow(a->x2 - a->x1, 2) + pow(a->y2 - a->y1, 2) + pow(a->z2 - a->z1, 2));
    double aSeg = aL / (double)a->segs;

    for (int j = i + 1; j < wire_count; j++)
    {
      const wire_info_t *b = &wires[j];
      if (b->segs <= 0)
        continue;

      double bL = sqrt(pow(b->x2 - b->x1, 2) + pow(b->y2 - b->y1, 2) + pow(b->z2 - b->z1, 2));
      double bSeg = bL / (double)b->segs;
      double tol = fmax(fmin(aSeg, bSeg) / 1000.0, 1e-9);

      double d11 = sqrt(pow(a->x1 - b->x1, 2) + pow(a->y1 - b->y1, 2) + pow(a->z1 - b->z1, 2));
      double d12 = sqrt(pow(a->x1 - b->x2, 2) + pow(a->y1 - b->y2, 2) + pow(a->z1 - b->z2, 2));
      double d21 = sqrt(pow(a->x2 - b->x1, 2) + pow(a->y2 - b->y1, 2) + pow(a->z2 - b->z1, 2));
      double d22 = sqrt(pow(a->x2 - b->x2, 2) + pow(a->y2 - b->y2, 2) + pow(a->z2 - b->z2, 2));
      int connected = (d11 <= tol) || (d12 <= tol) || (d21 <= tol) || (d22 <= tol);
      if (!connected)
        continue;

      if (a->radius <= 0.0 || b->radius <= 0.0)
        continue;

      /* 4nec2: radius ratio warnings/errors */
      {
        double r_big = fmax(a->radius, b->radius);
        double r_small = fmin(a->radius, b->radius);
        if (r_big > 10.0 * r_small)
        {
          snprintf(msg, sizeof(msg), "Connected wires (lines %d and %d, tags %d and %d) have a radius ratio >10:1 (%.6g vs %.6g); this is an error.",
                   a->line, b->line, a->tag, b->tag, r_big, r_small);
          add_error(ctx, errors, msg, 2);
        }
        else if (r_big > 5.0 * r_small)
        {
          snprintf(msg, sizeof(msg), "Connected wires (lines %d and %d, tags %d and %d) have a radius ratio >5:1 (%.6g vs %.6g); consider matching radii.",
                   a->line, b->line, a->tag, b->tag, r_big, r_small);
          add_error(ctx, errors, msg, 0);
        }
      }

      /* 4nec2: length/radius at junction (apply to each wire separately) */
      {
        if (aSeg < 6.0 * a->radius)
        {
          snprintf(msg, sizeof(msg), "Wire on line %d (tag %d) has L/R %.6g at a junction; 4nec2 warns if L<6R.", a->line, a->tag, aSeg / a->radius);
          add_error(ctx, errors, msg, 0);
        }
        if (aSeg < 2.0 * a->radius)
        {
          snprintf(msg, sizeof(msg), "Wire on line %d (tag %d) has L/R %.6g < 2; 4nec2 treats this as an error.", a->line, a->tag, aSeg / a->radius);
          add_error(ctx, errors, msg, 2);
        }
        if (bSeg < 6.0 * b->radius)
        {
          snprintf(msg, sizeof(msg), "Wire on line %d (tag %d) has L/R %.6g at a junction; 4nec2 warns if L<6R.", b->line, b->tag, bSeg / b->radius);
          add_error(ctx, errors, msg, 0);
        }
        if (bSeg < 2.0 * b->radius)
        {
          snprintf(msg, sizeof(msg), "Wire on line %d (tag %d) has L/R %.6g < 2; 4nec2 treats this as an error.", b->line, b->tag, bSeg / b->radius);
          add_error(ctx, errors, msg, 2);
        }
      }

      {
        double diff = fabs(a->radius - b->radius);
        double maxr = fmax(a->radius, b->radius);
        double rtol = fmax(1.0e-12, 1.0e-6 * maxr);
        if (diff > rtol)
        {
          snprintf(msg, sizeof(msg), "Connected wires (lines %d and %d, tags %d and %d) have different radii at the junction (%.6g m vs %.6g m).", a->line, b->line, a->tag, b->tag, a->radius, b->radius);
          add_error(ctx, errors, msg, WARNING);
        }
      }
    }
  }
}

// Helper: GE I1=1 connects segment ends if wire height < 1e-3 * segment length; suggest GE -1
static void check_ge_low_height_hazard(const context_t *ctx, errors_list_t *errors,
                                       const wire_info_t *wires, int wire_count,
                                       int GEType)
{
  if (GEType != 1)
    return;
  char msg[MAX_ERROR_LEN];
  for (int i = 0; i < wire_count; i++)
  {
    const wire_info_t *w = &wires[i];
    if (w->segs <= 0)
      continue;
    double L = sqrt(pow(w->x2 - w->x1, 2) + pow(w->y2 - w->y1, 2) + pow(w->z2 - w->z1, 2));
    double segLen = L / (double)w->segs;
    double dz = fabs(w->z2 - w->z1);
    double h = fmin(fabs(w->z1), fabs(w->z2));
    if (dz < 1e-9 && h < (1e-3 * segLen))
    {
      snprintf(msg, sizeof(msg), "GW on line %d (tag %d): has height %.4g m < 1e-3 x segment length (%.4g m) with GE I1=1; segment ends may be connected to ground. Consider GE -1.", w->line, w->tag, h, segLen);
      add_error(ctx, errors, msg, 0);
    }
  }
}

// Helper: segment length vs wavelength and radius sanity
static void check_segment_length_and_radius(const context_t *ctx, errors_list_t *errors,
                                            const wire_info_t *wires, int wire_count,
                                            double freq_mhz, int ek_enabled)
{
  char msg[MAX_ERROR_LEN];
  double wlam_m = (freq_mhz > 0.0) ? (CVEL / freq_mhz) : 0.0;
  if (wlam_m <= 0.0)
  {
    return;
  }

  for (int i = 0; i < wire_count; i++)
  {
    const wire_info_t *w = &wires[i];
    double L = sqrt(pow(w->x2 - w->x1, 2) + pow(w->y2 - w->y1, 2) + pow(w->z2 - w->z1, 2));
    if (w->segs > 0)
    {
      double segLen = L / (double)w->segs;
      /* wavelength fraction (lambda units) */
      double segment_fraction = segLen / wlam_m;


      /* ---- Cebik-derived rules ---- */
      /* warn if segment >= 0.1 lambda (Cebik) */
      if (segment_fraction >= 0.10)
      {
        snprintf(msg, sizeof(msg), "GW on line %d (tag %d): has segment length %.4g m (%.4g lambda) which is >= 0.1 lambda; consider increasing segmentation.",
                 w->line, w->tag, segLen, segment_fraction);
        add_error(ctx, errors, msg, 0);
      }
      /* informational note for critical regions (extra) */
      else if (segment_fraction >= 0.05)
      {
        snprintf(msg, sizeof(msg), "GW on line %d (tag %d): has segment length %.4g m (%.4g lambda); in critical regions, aim for < 0.05 lambda.",
                 w->line, w->tag, segLen, segment_fraction);
        add_error(ctx, errors, msg, 0);
      }
      /* Cebik: very small segments warning */
      if (segment_fraction < 0.001)
      {
        snprintf(msg, sizeof(msg), "GW on line %d (tag %d): has very small segment length %.4g m (%.4g lambda); this may cause excessive segmentation.",
                 w->line, w->tag, segLen, segment_fraction);
        add_error(ctx, errors, msg, 0);
      }

      /* ---- 4nec2 rules ---- */
      /* 4nec2: error if segment >= 0.2 lambda (lambda/5) */
      if (segment_fraction >= 0.20)
      {
        snprintf(msg, sizeof(msg), "GW on line %d (tag %d): segment length %.4g m (%.4g lambda) exceeds 0.2 lambda (lambda/5); segmentation too coarse.",
                 w->line, w->tag, segLen, segment_fraction);
        add_error(ctx, errors, msg, 2);
      }
      /* 4nec2: error if segment < 0.001 lambda */
      if (segment_fraction < 0.001)
      {
        snprintf(msg, sizeof(msg), "GW on line %d (tag %d): segment length %.4g m (%.4g lambda) is below 0.001 lambda; segmentation excessively fine.",
                 w->line, w->tag, segLen, segment_fraction);
        add_error(ctx, errors, msg, 2);
      }
      /* 4nec2: radius vs wavelength checks */
      if (w->radius > 0.0)
      {
        double rad_frac = w->radius / wlam_m;
        if (rad_frac > 0.01) /* lambda/100 */
        {
          snprintf(msg, sizeof(msg), "GW on line %d (tag %d): radius %.4g m (%.4g lambda) exceeds lambda/100; consider reducing radius.",
                   w->line, w->tag, w->radius, rad_frac);
          add_error(ctx, errors, msg, 0);
        }
        if (rad_frac > (1.0 / 30.0)) /* lambda/30 */
        {
          snprintf(msg, sizeof(msg), "GW on line %d (tag %d): radius %.4g m (%.4g lambda) exceeds lambda/30; this violates 4nec2 rules.",
                   w->line, w->tag, w->radius, rad_frac);
          add_error(ctx, errors, msg, 2);
        }
      }

      /* radius sanity relative to segment length */
      if (w->radius > 0.0)
      {
        if (ek_enabled)
        {
          /* EK errors: 4nec2 warns at L<2R, errors at L<0.5R */
          if (w->radius >= (2.0 * segLen))
          {
            snprintf(msg, sizeof(msg), "GW on line %d (tag %d): has radius %.4g m >= 2*len (%.4g m) with extended kernel; reduce radius or increase segmentation.",
                     w->line, w->tag, w->radius, 2.0 * segLen);
            add_error(ctx, errors, msg, 0);
          }
          if (w->radius >= (segLen * 2.0)) /* same as above, but apply warning if >=2R (L<=0.5R) */
          {
            /* already warned above; essentially duplicate, no extra message */
          }
          /* add missing warning for EK: L < 2*R (radius >= segLen/2) */
          if (w->radius >= (segLen / 2.0))
          {
            snprintf(msg, sizeof(msg), "GW on line %d (tag %d): radius %.4g m >= 0.5*len (%.4g m) with extended kernel; this is near the 4nec2 warning threshold.",
                     w->line, w->tag, w->radius, segLen / 2.0);
            add_error(ctx, errors, msg, 0);
          }
        }
        else
        {
          /* non-EK: Cebik rules already covered; adjust warning threshold to 8*R */
          if (w->radius >= (segLen / 2.0))
          {
            snprintf(msg, sizeof(msg), "GW on line %d (tag %d): has radius %.4g m >= len/2 (%.4g m); this violates thin-wire assumptions.",
                     w->line, w->tag, w->radius, segLen / 2.0);
            add_error(ctx, errors, msg, 2);
          }
          else if (w->radius >= (segLen / 8.0))
          {
            snprintf(msg, sizeof(msg), "GW on line %d (tag %d): has radius %.4g m which gives L/R <= 8; 4nec2 suggests warning when L<8R.",
                     w->line, w->tag, w->radius);
            add_error(ctx, errors, msg, 0);
          }
        }
      }
    }
  }
}

// Helper: check patch area against 4nec2 limit (A > lambda^2/25)
static void check_patch_area(const context_t *ctx, errors_list_t *errors)
{
  char msg[MAX_ERROR_LEN];
  for (int i = 0; i < ctx->geometry.num_patches; i++)
  {
    double area = ctx->geometry.patch_area[i];
    /* patch_area is already expressed in wavelengths^2 */
    if (area > (1.0 / 25.0))
    {
      snprintf(msg, sizeof(msg), "Patch %d area %.6g lambda^2 exceeds 1/25 lambda^2; consider refining mesh.", i + 1, area);
      add_error(ctx, errors, msg, 2);
    }
  }
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
void test_duplicate_tags(const context_t *ctx, const deck_t *deck, errors_list_t *errors)
{
  // we will also check to see if there are duplicate tags
  char msg[MAX_ERROR_LEN];

  // Only consider duplicates within the geometry section
  int gstart = deck->geometry_start;
  int gend = deck->geometry_end; // index of GE card
  if (gstart < 0 || gend < 0 || gend <= gstart)
  {
    // Fallback: search all cards but restrict to geometry types for both sides
    gstart = 0;
    gend = deck->num_cards;
  }

  // now check if there are any duplicate tags in the geometry
  // NOTE: this doesn't test for new tags generated by GM or similar
  for (int i = gstart; i < gend; i++)
  {
    if (is_geometry(&deck->cards[i]) && card_has_itag(&deck->cards[i]) && deck->cards[i].i[1] > 0)
    {
      int tag_i = deck->cards[i].i[1];
      for (int j = i + 1; j < gend; j++)
      {
        if (is_geometry(&deck->cards[j]) && card_has_itag(&deck->cards[j]) && deck->cards[j].i[1] > 0)
        {
          if (deck->cards[j].i[1] == tag_i)
          {
            snprintf(msg, sizeof(msg), "The tag number %d is found on card %d and card %d.", tag_i, i + 1, j + 1);
            add_error(ctx, errors, msg, 1);
          }
        }
      }
    }
  }
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
void test_card_inputs(const context_t *ctx, const deck_t *deck, errors_list_t *errors)
{
  const char *code;
  char msg[MAX_ERROR_LEN];

  for (int i = 0; i < deck->num_cards; i++)
  {
    code = deck->cards[i].card_code;

    // CE: comment end — should not have numeric inputs
    if (strcmp(code, "CE") == 0)
    {
      if (deck->cards[i].ints_used > 0 || deck->cards[i].flts_used > 0)
      {
        snprintf(msg, sizeof(msg), "CE on line %d: has numeric inputs.", i);
        add_error(ctx, errors, msg, 0);
      }
    }

    // EN: end of deck — should not have numeric inputs
    if (strcmp(code, "EN") == 0)
    {
      if (deck->cards[i].ints_used > 0 || deck->cards[i].flts_used > 0)
      {
        snprintf(msg, sizeof(msg), "EN on line %d: has numeric inputs.", i);
        add_error(ctx, errors, msg, 0);
      }
    }

    // FRs: allow single-frequency (I2=0) or stepped (I2>0)
    if (strcmp(code, "FR") == 0)
    {
      // there must be a value in F1
      if (deck->cards[i].f[1] == 0)
      {
        snprintf(msg, sizeof(msg), "FR on line %d: no base frequency in F1.", i);
        add_error(ctx, errors, msg, 0);
      }
      // Single-frequency: I2==0 should have F2==0
      if (deck->cards[i].i[2] == 0 && deck->cards[i].f[2] != 0)
      {
        snprintf(msg, sizeof(msg), "FR on line %d: has I2 = 0 (single frequency), but has a non-zero F2.", i);
        add_error(ctx, errors, msg, 0);
      }
      // Stepped: I2>0 requires positive step in F2
      else if (deck->cards[i].i[2] > 0 && deck->cards[i].f[2] <= 0)
      {
        snprintf(msg, sizeof(msg), "FR on line %d: has I2 > 0 (stepped), but F2 is not a positive step.", i);
        add_error(ctx, errors, msg, 0);
      }
    }

    // GW: wire geometry — require positive segment count and radius
    if (strcmp(code, "GW") == 0)
    {
      // At least tag and segment count
      if (deck->cards[i].ints_used < 2)
      {
        snprintf(msg, sizeof(msg), "GW on line %d: has fewer than 2 integer inputs (tag, segments).", i);
        add_error(ctx, errors, msg, 0);
      }
      else
      {
        int segs = deck->cards[i].i[2];
        if (segs <= 0)
        {
          snprintf(msg, sizeof(msg), "GW on line %d: has non-positive segment count.", i);
          add_error(ctx, errors, msg, 0);
        }
      }
      // Endpoints should not be identical (zero-length wire)
      double x1 = deck->cards[i].f[1], y1 = deck->cards[i].f[2], z1 = deck->cards[i].f[3];
      double x2 = deck->cards[i].f[4], y2 = deck->cards[i].f[5], z2 = deck->cards[i].f[6];
      if (x1 == x2 && y1 == y2 && z1 == z2)
      {
        snprintf(msg, sizeof(msg), "GW on line %d: has identical endpoints (zero-length wire).", i);
        add_error(ctx, errors, msg, 0);
      }
      // Radius must be present and positive (F7).
      double gw_radius = deck->cards[i].f[7];
      if (deck->cards[i].flts_used < 7 && !deck->cards[i].flt_form_inline[7])
      {
        snprintf(msg, sizeof(msg), "GW on line %d: has no radius specified in F7.", i);
        add_error(ctx, errors, msg, 0);
      }
      else if (gw_radius <= 0.0)
      {
        snprintf(msg, sizeof(msg), "GW on line %d: has non-positive radius in F7.", i);
        add_error(ctx, errors, msg, 0);
      }
      // If radius is zero (and not a formula), next card should be a GC with tapering info
      if ((deck->cards[i].flts_used >= 7 || deck->cards[i].flt_form_inline[7]) && gw_radius == 0.0)
      {
        if (i + 1 < deck->num_cards)
        {
          if (strcmp(deck->cards[i + 1].card_code, "GC") != 0)
          {
            snprintf(msg, sizeof(msg), "GW on line %d: has zero radius, but the next card is not a GC.", i + 1);
            add_error(ctx, errors, msg, 1);
          }
        }
        else
        {
          snprintf(msg, sizeof(msg), "GW on line %d: has zero radius and is the last card; a following GC is required.", i + 1);
          add_error(ctx, errors, msg, 1);
        }
      }
    }

    // RP: radiation pattern — counts, steps, and basic range sanity
    if (strcmp(code, "RP") == 0)
    {
      // Typical RP uses at least 4 integers and 4 floats
      if (deck->cards[i].ints_used < 4)
      {
        snprintf(msg, sizeof(msg), "RP on line %d: has fewer than 4 integer inputs.", i);
        add_error(ctx, errors, msg, 0);
      }
      if (deck->cards[i].flts_used < 4)
      {
        snprintf(msg, sizeof(msg), "RP on line %d: has fewer than 4 floating-point inputs.", i);
        add_error(ctx, errors, msg, 0);
      }
      // Number of theta/phi points should be positive
      int ntheta = deck->cards[i].i[2];
      int nphi = deck->cards[i].i[3];
      if (ntheta <= 0)
      {
        snprintf(msg, sizeof(msg), "RP on line %d: has non-positive NTHETA (I2).", i);
        add_error(ctx, errors, msg, 0);
      }
      if (nphi <= 0)
      {
        snprintf(msg, sizeof(msg), "RP on line %d: has non-positive NPHI (I3).", i);
        add_error(ctx, errors, msg, 0);
      }
      // Steps must be non-zero when requesting multiple points
      double th_start = deck->cards[i].f[1];
      double ph_start = deck->cards[i].f[2];
      double th_step = deck->cards[i].f[3];
      double ph_step = deck->cards[i].f[4];
      if (ntheta > 1 && th_step == 0.0)
      {
        snprintf(msg, sizeof(msg), "RP on line %d: has NTHETA > 1 but zero theta step (F3).", i);
        add_error(ctx, errors, msg, 0);
      }
      if (nphi > 1 && ph_step == 0.0)
      {
        snprintf(msg, sizeof(msg), "RP on line %d: has NPHI > 1 but zero phi step (F4).", i);
        add_error(ctx, errors, msg, 0);
      }
      // Basic angle sanity: starts within typical ranges
      if (!(th_start >= -180.0 && th_start <= 180.0))
      {
        snprintf(msg, sizeof(msg), "RP on line %d: has theta start (F1) outside [-180,180].", i);
        add_error(ctx, errors, msg, 0);
      }
      if (!(ph_start >= -360.0 && ph_start <= 360.0))
      {
        snprintf(msg, sizeof(msg), "RP on line %d: has phi start (F2) outside [-360,360].", i);
        add_error(ctx, errors, msg, 0);
      }
      // Step magnitudes should be reasonable
      if (fabs(th_step) > 180.0)
      {
        snprintf(msg, sizeof(msg), "RP on line %d: has an excessively large theta step (F3).", i);
        add_error(ctx, errors, msg, 0);
      }
      if (fabs(ph_step) > 360.0)
      {
        snprintf(msg, sizeof(msg), "RP on line %d: has an excessively large phi step (F4).", i);
        add_error(ctx, errors, msg, 0);
      }
      // Derived end angles should remain within sensible bounds and match step direction
      if (ntheta > 1)
      {
        double th_end = th_start + (ntheta - 1) * th_step;
        if (!(th_end >= -180.0 && th_end <= 180.0))
        {
          snprintf(msg, sizeof(msg), "RP on line %d: theta sweep leaves the [-180,180] range.", i);
          add_error(ctx, errors, msg, 0);
        }
        if (th_step > 0.0 && th_end < th_start)
        {
          snprintf(msg, sizeof(msg), "RP on line %d: has NTHETA>1 and positive theta step but decreasing sweep.", i);
          add_error(ctx, errors, msg, 0);
        }
        if (th_step < 0.0 && th_end > th_start)
        {
          snprintf(msg, sizeof(msg), "RP on line %d: has NTHETA>1 and negative theta step but increasing sweep.", i);
          add_error(ctx, errors, msg, 0);
        }
      }
      if (nphi > 1)
      {
        double ph_end = ph_start + (nphi - 1) * ph_step;
        if (!(ph_end >= -720.0 && ph_end <= 720.0))
        {
          snprintf(msg, sizeof(msg), "RP on line %d: phi sweep leaves a reasonable range.", i);
          add_error(ctx, errors, msg, 0);
        }
        if (ph_step > 0.0 && ph_end < ph_start)
        {
          snprintf(msg, sizeof(msg), "RP on line %d: has NPHI>1 and positive phi step but decreasing sweep.", i);
          add_error(ctx, errors, msg, 0);
        }
        if (ph_step < 0.0 && ph_end > ph_start)
        {
          snprintf(msg, sizeof(msg), "RP on line %d: has NPHI>1 and negative phi step but increasing sweep.", i);
          add_error(ctx, errors, msg, 0);
        }
      }
    }
  }
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
void test_bad_symbols(const context_t *ctx, deck_t *deck, errors_list_t *errors)
{
  char msg[MAX_ERROR_LEN];

  // List of default symbol names (pi, c, and unit constants)
  // Note: AWG symbols (awg0-awg40) are also defaults but checked separately below
  const char *default_symbols[] = {
      "pi", "c", "m", "cm", "mm", "ft", "in", "mil",
      "pf", "nf", "uf", "nh", "uh", "mh"};
  int num_defaults = sizeof(default_symbols) / sizeof(default_symbols[0]);

  // Check if any user-defined symbols override defaults
  for (int i = 0; i < deck->num_cards; i++)
  {
    card_t *card = &deck->cards[i];
    if (strcmp(card->card_code, "SY") == 0 && card->formulas)
    {
      key_value_t *kv = card->formulas;
      while (kv)
      {
        // Check against standard defaults
        for (int d = 0; d < num_defaults; d++)
        {
          if (strcasecmp(kv->key, default_symbols[d]) == 0)
          {
            snprintf(msg, sizeof(msg), "SY on line %d: symbol '%s' overrides a default symbol. This may cause unexpected results.", i, kv->key);
            add_error(ctx, errors, msg, WARNING);
          }
        }

        // Check against AWG symbols (awg0-awg40)
        if (strlen(kv->key) >= 4 && strlen(kv->key) <= 5)
        {
          if ((kv->key[0] == 'a' || kv->key[0] == 'A') &&
              (kv->key[1] == 'w' || kv->key[1] == 'W') &&
              (kv->key[2] == 'g' || kv->key[2] == 'G'))
          {
            int awg_num = atoi(&kv->key[3]);
            if (awg_num >= 0 && awg_num <= 40)
            {
              snprintf(msg, sizeof(msg), "SY on line %d: symbol '%s' overrides a default AWG constant. This may cause unexpected results.", i, kv->key);
              add_error(ctx, errors, msg, WARNING);
            }
          }
        }

        kv = kv->next;
      }
    }
  }

  // Check for duplicate symbol names
  for (int i = 0; i < deck->num_symbols; i++)
  {
    key_value_t *outer = deck->symbols[i];
    if (outer == NULL)
      continue;

    // Check if any other symbol has the same name
    for (int k = i + 1; k < deck->num_symbols; k++)
    {
      key_value_t *inner = deck->symbols[k];
      if (inner == NULL)
        continue;

      if (strcasecmp(outer->key, inner->key) == 0)
      {
        snprintf(msg, sizeof(msg), "SY on line %d: symbol '%s' has been defined more than once.", i, outer->key);
        add_error(ctx, errors, msg, 0);
      }
    }
  }
} /* end of test_bad_symbols */

/******************************************************************************
 * test_field_separators
 *
 * Checks whether all cards in the geometry section use the same field
 * separator style, and likewise for the control section.  Mixed separators
 * within a section produce a warning — the deck will still calculate, but
 * it suggests the file was edited inconsistently and may cause problems
 * for any output code attempting to preserve the original formatting.
 *
 * @param ctx  the context_t (used for error reporting)
 * @param deck the deck_t to test
 * @param errors the errors_list_t to append warnings to
 */
void test_field_separators(const context_t *ctx, const deck_t *deck, errors_list_t *errors)
{
  char msg[MAX_ERROR_LEN];

  // helper: scan a range of cards for separator consistency
  // returns false (and adds a warning) if mixed separators are found
  int geo_end = (deck->geometry_end >= 0) ? deck->geometry_end : deck->num_cards - 1;
  int ctrl_end = (deck->deck_end >= 0) ? deck->deck_end : deck->num_cards - 1;
  int ctrl_start = geo_end + 1;

  // --- geometry section ---
  if (deck->geometry_start >= 0)
  {
    field_sep_t first_sep = FSEP_UNKNOWN;
    int first_sep_idx = -1;
    for (int i = deck->geometry_start; i <= geo_end; i++)
    {
      const card_t *c = &deck->cards[i];
      if (!is_geometry(c))
        continue;
      if (c->field_sep == FSEP_UNKNOWN)
        continue;
      if (first_sep == FSEP_UNKNOWN)
      {
        first_sep = c->field_sep;
        first_sep_idx = i + 1; // 1-based for message
      }
      else if (c->field_sep != first_sep)
      {
        snprintf(msg, sizeof(msg),
                 "Geometry section has mixed field separators: card %d uses a different style "
                 "from card %d. Output formatting may not preserve the original file style.",
                 i + 1, first_sep_idx);
        add_error(ctx, errors, msg, WARNING);
        break; // one warning per section is enough
      }
    }
  }

  // --- control section ---
  if (ctrl_start < deck->num_cards)
  {
    field_sep_t first_sep = FSEP_UNKNOWN;
    int first_sep_idx = -1;
    for (int i = ctrl_start; i <= ctrl_end; i++)
    {
      const card_t *c = &deck->cards[i];
      if (!is_control(c))
        continue;
      if (c->field_sep == FSEP_UNKNOWN)
        continue;
      if (first_sep == FSEP_UNKNOWN)
      {
        first_sep = c->field_sep;
        first_sep_idx = i + 1;
      }
      else if (c->field_sep != first_sep)
      {
        snprintf(msg, sizeof(msg),
                 "Control section has mixed field separators: card %d uses a different style "
                 "from card %d. Output formatting may not preserve the original file style.",
                 i + 1, first_sep_idx);
        add_error(ctx, errors, msg, WARNING);
        break;
      }
    }
  }
} /* end of test_field_separators */

/* end of tests.c */
