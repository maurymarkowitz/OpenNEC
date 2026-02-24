/******************************************************************************
 * card_validation.c
 *
 * Per-field validation for individual NEC cards, intended for use by GUI
 * applications that want to highlight problem fields interactively as the
 * user edits a card.
 *
 * Each exported function takes only a single card_t pointer (no deck_t or
 * nec_context_t), so it can be called on any card in isolation. Validation
 * rules that require knowledge of other cards in the deck — such as whether
 * a referenced tag exists, or whether a wire end is open — are intentionally
 * out of scope here and remain in deck_validations.c.
 *
 * Severity convention (reuses error_level from types.h):
 *   NONE    = 0  No problem / no rule defined for this field
 *   WARNING = 1  Suspicious; simulation will likely proceed
 *   PROBLEM = 2  Likely to cause incorrect results or failure
 *   FATAL   = 3  Will definitely fail / deck cannot be processed
 *
 *****************************************************************************/

#include <string.h>
#include <math.h>
#include "internals.h"
#include "card_validation.h"

/* Convenience macro for returning a result with a formatted message */
#define RESULT(sev, ...) \
  do { \
    field_validation_t _r; \
    _r.severity = (sev); \
    snprintf(_r.message, MAX_ERROR_LEN, __VA_ARGS__); \
    return _r; \
  } while(0)

/* Return a clean "no problem" result */
static field_validation_t ok(void)
{
  field_validation_t r;
  r.severity = NONE;
  r.message[0] = '\0';
  return r;
}

/******************************************************************************
 * parse_field_name
 *
 * Converts a field name string ("I1".."I4", "F1".."F7") into a flag and
 * 1-based index. Returns false if the string is not a recognised field name.
 *
 * @param name    Field name string to parse
 * @param is_int  Set to 1 if the field is an integer (I-field), 0 if float
 * @param idx     Set to the 1-based field index (1..4 for I, 1..7 for F)
 * @return        1 on success, 0 if name is not recognised
 *****************************************************************************/
static int parse_field_name(const char *name, int *is_int, int *idx)
{
  if(!name || name[0] == '\0' || name[1] == '\0') return 0;
  if ((name[0] == 'I' || name[0] == 'i') && name[2] == '\0') {
    int n = name[1] - '0';
    if(n >= 1 && n <= MAX_INT_FIELDS) { *is_int = 1; *idx = n; return 1; }
  }
  if ((name[0] == 'F' || name[0] == 'f') && name[2] == '\0') {
    int n = name[1] - '0';
    if(n >= 1 && n <= MAX_FLT_FIELDS) { *is_int = 0; *idx = n; return 1; }
  }
  return 0;
}


/******************************************************************************
 * Control card validators
 *****************************************************************************/

/* FR — frequency sweep
 *   F1: base frequency — required, must be non-zero
 *   I2: step count — if > 1, F2 must be a positive step
 *   F2: frequency step — must be positive when I2 > 1; spurious if I2 <= 1
 */
static field_validation_t validate_FR_field(const card_t *c, int is_int, int idx)
{
  if(!is_int && idx == 1) {
    if(c->f[1] == 0.0)
      RESULT(PROBLEM, "FR F1: base frequency is required and must be non-zero.");
  }
  if(is_int && idx == 2) {
    if(c->i[2] > 1 && c->f[2] == 0.0)
      RESULT(WARNING, "FR I2: step count is I2 > 1,  but F2 (frequency step) is zero.");
  }
  if(!is_int && idx == 2) {
    if(c->i[2] > 1 && c->f[2] <= 0.0)
      RESULT(PROBLEM, "FR F2: step count I2 > 1 requires a positive frequency step.");
    if(c->i[2] == 0 && c->f[2] != 0.0)
      RESULT(WARNING, "FR F2: I2 is 0 (single frequency) but F2 is non-zero.");
  }
  return ok();
}

/* TL — transmission line
 *   I1..I4: tag/segment locators for both endpoints — all must be positive
 *   F1: characteristic impedance Z0 — required, must be non-zero
 */
static field_validation_t validate_TL_field(const card_t *c, int is_int, int idx)
{
  if(is_int && idx >= 1 && idx <= 4) {
    if(c->i[idx] <= 0)
      RESULT(PROBLEM, "TL I%d: tag/segment locators must be positive (got %d).", idx, c->i[idx]);
  }
  if(!is_int && idx == 1) {
    if(c->f[1] == 0.0)
      RESULT(PROBLEM, "TL F1: characteristic impedance Z0 is required and must be non-zero.");
  }
  return ok();
}

/* EX — excitation (voltage/current source)
 *   I1: excitation type — types 6 and 7 are not supported by OpenNEC
 *   I2: tag number — must be positive
 *   I3: segment number — must be positive
 *   F1: amplitude — required, must be non-zero
 */
static field_validation_t validate_EX_field(const card_t *c, int is_int, int idx)
{
  if(is_int && idx == 1) {
    if(c->i[1] == 6 || c->i[1] == 7)
      RESULT(WARNING, "EX I1: excitation type %d is not supported by OpenNEC.", c->i[1]);
  }
  if(is_int && (idx == 2 || idx == 3)) {
    if(c->i[idx] <= 0)
      RESULT(PROBLEM, "EX I%d: tag/segment locator must be positive (got %d).", idx, c->i[idx]);
  }
  if(!is_int && idx == 1) {
    if(c->f[1] == 0.0)
      RESULT(PROBLEM, "EX F1: excitation amplitude is required and must be non-zero.");
  }
  return ok();
}

/* LD — loading
 *   I1: load type — must be >= -1
 *   I2: tag number — must be positive
 *   I3: start segment — must be positive
 *   I4: end segment — if non-zero, must be >= I3
 *   F1..F3: load values — at least one should be non-zero
 */
static field_validation_t validate_LD_field(const card_t *c, int is_int, int idx)
{
  if(is_int && idx == 1) {
    if(c->i[1] < -1)
      RESULT(WARNING, "LD I1: unexpected load type %d.", c->i[1]);
  }
  if(is_int && (idx == 2 || idx == 3)) {
    if(c->i[idx] <= 0)
      RESULT(PROBLEM, "LD I%d: tag/segment locator must be positive (got %d).", idx, c->i[idx]);
  }
  if(is_int && idx == 4) {
    if(c->i[4] != 0 && c->i[4] < c->i[3])
      RESULT(WARNING, "LD I4: end segment (%d) is less than start segment I3 (%d).", c->i[4], c->i[3]);
  }
  if(!is_int && (idx == 1 || idx == 2 || idx == 3)) {
    if(c->f[1] == 0.0 && c->f[2] == 0.0 && c->f[3] == 0.0)
      RESULT(WARNING, "LD F1-F3: all load values are zero; at least one should be non-zero.");
  }
  return ok();
}

/* RP — radiation pattern
 *   I2: NTHETA — number of theta points, must be positive
 *   I3: NPHI   — number of phi points, must be positive
 *   F1: theta start — should be within [-180, 180]
 *   F2: phi start — should be within [-360, 360]
 *   F3: theta step — required when NTHETA > 1
 *   F4: phi step — required when NPHI > 1
 */
static field_validation_t validate_RP_field(const card_t *c, int is_int, int idx)
{
  if(is_int && idx == 2) {
    if(c->i[2] <= 0)
      RESULT(PROBLEM, "RP I2: NTHETA must be positive (got %d).", c->i[2]);
  }
  if(is_int && idx == 3) {
    if(c->i[3] <= 0)
      RESULT(PROBLEM, "RP I3: NPHI must be positive (got %d).", c->i[3]);
  }
  if(!is_int && idx == 1) {
    if(c->f[1] < -180.0 || c->f[1] > 180.0)
      RESULT(WARNING, "RP F1: theta start %.4g is outside [-180, 180].", c->f[1]);
  }
  if(!is_int && idx == 2) {
    if(c->f[2] < -360.0 || c->f[2] > 360.0)
      RESULT(WARNING, "RP F2: phi start %.4g is outside [-360, 360].", c->f[2]);
  }
  if(!is_int && idx == 3) {
    if(c->i[2] > 1 && c->f[3] == 0.0)
      RESULT(PROBLEM, "RP F3: NTHETA > 1 requires a non-zero theta step.");
  }
  if(!is_int && idx == 4) {
    if(c->i[3] > 1 && c->f[4] == 0.0)
      RESULT(PROBLEM, "RP F4: NPHI > 1 requires a non-zero phi step.");
  }
  return ok();
}

/* GN — ground parameters
 *   I1: ground type — must be present (ints_used >= 1)
 */
static field_validation_t validate_GN_field(const card_t *c, int is_int, int idx)
{
  if(is_int && idx == 1) {
    if(c->ints_used < 1)
      RESULT(PROBLEM, "GN I1: ground type is required.");
  }
  return ok();
}

/* EK — extended thin-wire kernel
 *   I1: flag — supported values are 0 (disable) and 1 (enable); other values unusual
 */
static field_validation_t validate_EK_field(const card_t *c, int is_int, int idx)
{
  if(is_int && idx == 1) {
    if(c->i[1] != 0 && c->i[1] != 1)
      RESULT(WARNING, "EK I1: value %d is unusual; expected 0 (disable) or 1 (enable).", c->i[1]);
  }
  return ok();
}

/* GD — ground supplementary data
 *   F1, F2: dielectric constant and conductivity — at least one should be non-zero
 */
static field_validation_t validate_GD_field(const card_t *c, int is_int, int idx)
{
  if(!is_int && (idx == 1 || idx == 2)) {
    if(c->f[1] == 0.0 && c->f[2] == 0.0)
      RESULT(WARNING, "GD F1/F2: both dielectric constant and conductivity are zero.");
  }
  return ok();
}

/* NE / NH — near electric / magnetic field
 *   I2, I3, I4: grid point counts — each must be positive
 *   F4, F5, F6: grid spacing — must be non-zero when corresponding count > 1
 */
static field_validation_t validate_NE_NH_field(const card_t *c, int is_int, int idx)
{
  if(is_int && idx >= 2 && idx <= 4) {
    if(c->i[idx] <= 0)
      RESULT(PROBLEM, "%s I%d: grid point count must be positive (got %d).",
             c->card_code, idx, c->i[idx]);
  }
  /* F4 corresponds to I2 count, F5 to I3, F6 to I4 */
  if(!is_int && idx >= 4 && idx <= 6) {
    int count_idx = idx - 2;  /* F4->I2, F5->I3, F6->I4 */
    if(c->i[count_idx] > 1 && c->f[idx] == 0.0)
      RESULT(PROBLEM, "%s F%d: grid spacing must be non-zero when count I%d > 1.",
             c->card_code, idx, count_idx);
  }
  return ok();
}


/******************************************************************************
 * Geometry card validators
 *****************************************************************************/

/* GA — wire arc
 *   I2: segment count — must be positive
 *   F1: arc radius — must be positive
 *   F7: wire radius — must be positive
 */
static field_validation_t validate_GA_field(const card_t *c, int is_int, int idx)
{
  if(is_int && idx == 2) {
    if(c->i[2] <= 0)
      RESULT(PROBLEM, "GA I2: segment count must be positive (got %d).", c->i[2]);
  }
  if(!is_int && idx == 1) {
    if(c->f[1] <= 0.0)
      RESULT(PROBLEM, "GA F1: arc radius must be positive (got %.4g).", c->f[1]);
  }
  if(!is_int && idx == 7) {
    if(c->f[7] <= 0.0)
      RESULT(PROBLEM, "GA F7: wire radius must be positive (got %.4g).", c->f[7]);
  }
  return ok();
}

/* GC — tapered wire (follows GW)
 *   F1: taper ratio — zero means no taper, which is unusual after a GC
 *   F2: radius at start — must be positive
 *   F3: radius at end   — must be positive
 */
static field_validation_t validate_GC_field(const card_t *c, int is_int, int idx)
{
  if(!is_int && idx == 1) {
    if(c->f[1] == 0.0)
      RESULT(WARNING, "GC F1: taper ratio is zero; no taper will be applied.");
  }
  if(!is_int && (idx == 2 || idx == 3)) {
    if(c->f[idx] <= 0.0)
      RESULT(PROBLEM, "GC F%d: wire radius must be positive (got %.4g).", idx, c->f[idx]);
  }
  return ok();
}

/* GE — geometry end
 *   I1: ground flag — recognised values are -1, 0, 1, 2
 */
static field_validation_t validate_GE_field(const card_t *c, int is_int, int idx)
{
  if(is_int && idx == 1) {
    int v = c->i[1];
    if(v != -1 && v != 0 && v != 1 && v != 2)
      RESULT(WARNING, "GE I1: value %d is not a recognised ground flag (-1, 0, 1, 2).", v);
  }
  return ok();
}

/* GH — helix
 *   I2: segment count — must be positive
 *   F1: total axial length — must be non-zero
 *   F7: wire radius — must be positive
 */
static field_validation_t validate_GH_field(const card_t *c, int is_int, int idx)
{
  if(is_int && idx == 2) {
    if(c->i[2] <= 0)
      RESULT(PROBLEM, "GH I2: segment count must be positive (got %d).", c->i[2]);
  }
  if(!is_int && idx == 1) {
    if(c->f[1] == 0.0)
      RESULT(PROBLEM, "GH F1: total axial length/spacing must be non-zero.");
  }
  if(!is_int && idx == 7) {
    if(c->f[7] <= 0.0)
      RESULT(PROBLEM, "GH F7: wire radius must be positive (got %.4g).", c->f[7]);
  }
  return ok();
}

/* GM — geometry move/rotate/scale
 *   All fields: warn if every field is zero (a no-op transformation)
 */
static field_validation_t validate_GM_field(const card_t *c, int is_int, int idx)
{
  /* Only report on F1 to avoid duplicating the warning on every field */
  if(!is_int && idx == 1) {
    int all_zero = 1;
    for(int n = 1; n <= MAX_INT_FIELDS; n++) if(c->i[n] != 0) { all_zero = 0; break; }
    for(int n = 1; n <= MAX_FLT_FIELDS && all_zero; n++) if(c->f[n] != 0.0) { all_zero = 0; break; }
    if(all_zero)
      RESULT(WARNING, "GM: all fields are zero; this transformation is a no-op.");
  }
  return ok();
}

/* GR — geometry rotate/duplicate
 *   I2: number of copies — must be positive
 */
static field_validation_t validate_GR_field(const card_t *c, int is_int, int idx)
{
  if(is_int && idx == 2) {
    if(c->i[2] <= 0)
      RESULT(PROBLEM, "GR I2: number of copies must be positive (got %d).", c->i[2]);
  }
  return ok();
}

/* GS — geometry scale
 *   F1: scale factor — must be positive
 */
static field_validation_t validate_GS_field(const card_t *c, int is_int, int idx)
{
  if(!is_int && idx == 1) {
    if(c->f[1] <= 0.0)
      RESULT(PROBLEM, "GS F1: scale factor must be positive (got %.4g).", c->f[1]);
  }
  return ok();
}

/* GW — wire
 *   I2: segment count — must be positive
 *   F4..F6: second endpoint — flagged if identical to first endpoint (zero-length wire)
 *   F7: wire radius — must be positive
 */
static field_validation_t validate_GW_field(const card_t *c, int is_int, int idx)
{
  if(is_int && idx == 2) {
    if(c->i[2] <= 0)
      RESULT(PROBLEM, "GW I2: segment count must be positive (got %d).", c->i[2]);
  }
  /* Report zero-length wire on the second endpoint fields (F4, F5, F6).
   * Use fv[] (evaluated values) so formula-based coordinates are handled
   * correctly — fv[] equals f[] when no formula is present. */
  if(!is_int && idx >= 4 && idx <= 6) {
    if(c->fv[1] == c->fv[4] && c->fv[2] == c->fv[5] && c->fv[3] == c->fv[6])
      RESULT(PROBLEM, "GW F4-F6: both endpoints are identical (zero-length wire).");
  }
  if(!is_int && idx == 7) {
    /* When the radius was given as a formula (e.g. AWG #12), f[7]==0 but
     * fv[7] holds the evaluated radius.  Use fv[7] when a formula is present. */
    double radius = (c->flt_form_inline[7] && c->fv[7] > 0.0) ? c->fv[7] : c->f[7];
    if(radius <= 0.0)
      RESULT(PROBLEM, "GW F7: wire radius must be positive (got %.4g).", radius);
  }
  return ok();
}

/* GX — geometry reflection
 *   I2: reflection axes bitmap — zero means no axes selected (likely unintentional)
 */
static field_validation_t validate_GX_field(const card_t *c, int is_int, int idx)
{
  if(is_int && idx == 2) {
    if(c->i[2] == 0)
      RESULT(WARNING, "GX I2: no reflection axes selected (value is 0); GX will have no effect.");
  }
  return ok();
}

/* SC — surface patch corner
 *   F1..F3: corner coordinates — at least one must be non-zero
 */
static field_validation_t validate_SC_field(const card_t *c, int is_int, int idx)
{
  if(!is_int && (idx >= 1 && idx <= 3)) {
    if(c->f[1] == 0.0 && c->f[2] == 0.0 && c->f[3] == 0.0)
      RESULT(PROBLEM, "SC F1-F3: all corner coordinates are zero; a non-zero position is required.");
  }
  return ok();
}

/* SM — surface multiple patches
 *   I1: patch count in first direction — must be positive
 *   I2: patch count in second direction — must be positive
 */
static field_validation_t validate_SM_field(const card_t *c, int is_int, int idx)
{
  if(is_int && (idx == 1 || idx == 2)) {
    if(c->i[idx] <= 0)
      RESULT(PROBLEM, "SM I%d: patch count must be positive (got %d).", idx, c->i[idx]);
  }
  return ok();
}

/* SP — single surface patch
 *   F4..F6: surface normal direction — at least one component must be non-zero
 */
static field_validation_t validate_SP_field(const card_t *c, int is_int, int idx)
{
  if(!is_int && (idx >= 4 && idx <= 6)) {
    if(c->f[4] == 0.0 && c->f[5] == 0.0 && c->f[6] == 0.0)
      RESULT(PROBLEM, "SP F4-F6: all normal direction components are zero; a direction is required.");
  }
  return ok();
}


/******************************************************************************
 * validate_card_field — public dispatch entry point
 *****************************************************************************/

field_validation_t validate_card_field(const card_t *card, const char *field_name)
{
  if(!card || !field_name) return ok();

  int is_int = 0, idx = 0;
  if(!parse_field_name(field_name, &is_int, &idx)) return ok();

  const char *code = card->card_code;

  /* Control cards */
  if(strcmp(code, "FR") == 0) return validate_FR_field(card, is_int, idx);
  if(strcmp(code, "TL") == 0) return validate_TL_field(card, is_int, idx);
  if(strcmp(code, "EX") == 0) return validate_EX_field(card, is_int, idx);
  if(strcmp(code, "LD") == 0) return validate_LD_field(card, is_int, idx);
  if(strcmp(code, "RP") == 0) return validate_RP_field(card, is_int, idx);
  if(strcmp(code, "GN") == 0) return validate_GN_field(card, is_int, idx);
  if(strcmp(code, "EK") == 0) return validate_EK_field(card, is_int, idx);
  if(strcmp(code, "GD") == 0) return validate_GD_field(card, is_int, idx);
  if(strcmp(code, "NE") == 0) return validate_NE_NH_field(card, is_int, idx);
  if(strcmp(code, "NH") == 0) return validate_NE_NH_field(card, is_int, idx);

  /* Geometry cards */
  if(strcmp(code, "GA") == 0) return validate_GA_field(card, is_int, idx);
  if(strcmp(code, "GC") == 0) return validate_GC_field(card, is_int, idx);
  if(strcmp(code, "GE") == 0) return validate_GE_field(card, is_int, idx);
  if(strcmp(code, "GH") == 0) return validate_GH_field(card, is_int, idx);
  if(strcmp(code, "GM") == 0) return validate_GM_field(card, is_int, idx);
  if(strcmp(code, "GR") == 0) return validate_GR_field(card, is_int, idx);
  if(strcmp(code, "GS") == 0) return validate_GS_field(card, is_int, idx);
  if(strcmp(code, "GW") == 0) return validate_GW_field(card, is_int, idx);
  if(strcmp(code, "GX") == 0) return validate_GX_field(card, is_int, idx);
  if(strcmp(code, "SC") == 0) return validate_SC_field(card, is_int, idx);
  if(strcmp(code, "SM") == 0) return validate_SM_field(card, is_int, idx);
  if(strcmp(code, "SP") == 0) return validate_SP_field(card, is_int, idx);

  /* GF: no field rules — filename lives in card_str, not i[]/f[] */
  /* CM, CE, EN, SY, and others: no per-field rules at this level */

  return ok();
}


/******************************************************************************
 * validate_card_all_fields — validate all 11 fields in one call
 *
 * Index mapping matches field_names[] in types.c:
 *   results[0..3]  => I1..I4  (I_idx = fieldN - 1)
 *   results[4..10] => F1..F7  (F_idx = fieldN + 3)
 *****************************************************************************/

void validate_card_all_fields(const card_t *card, field_validation_t results[11])
{
  /* I1..I4 at indices 0..3 */
  static const char *int_names[] = { "I1", "I2", "I3", "I4" };
  for(int n = 0; n < 4; n++)
    results[n] = validate_card_field(card, int_names[n]);

  /* F1..F7 at indices 4..10 */
  static const char *flt_names[] = { "F1", "F2", "F3", "F4", "F5", "F6", "F7" };
  for(int n = 0; n < 7; n++)
    results[4 + n] = validate_card_field(card, flt_names[n]);
}
