/*
 * card_validation.h - Per-field card validation for OpenNEC
 *
 * Provides field-level validation suitable for GUI use, where caller needs
 * to know the state of individual fields (I1..I4, F1..F7) on a single card
 * so it can highlight problems interactively as users type.
 *
 * Unlike deck_validations.c, which sweeps the whole deck and returns a message
 * list, these functions primarily validate single cards and return a result by
 * value.  (Earlier versions exposed deck-aware helpers, but all current
 * rules operate on a single card without knowledge of other cards.)
 *
 * Severity uses the existing error_level enum from types.h:
 *   NONE    = 0  No problem detected, or no validation rule defined for field
 *   WARNING = 1  Suspicious but simulation will likely proceed
 *   PROBLEM = 2  Likely to cause incorrect results or failure
 *   FATAL   = 3  Will definitely fail
 *
 * The NONE value covers both "field is fine" and "no validation rule for this
 * field on this card type" — the GUI treats them identically (no indicator).
 */

#ifndef CARD_VALIDATION_H
#define CARD_VALIDATION_H

#include "types.h"

/* MAX_ERROR_LEN is normally defined in opennec.h; guard here in case this
 * header is included before opennec.h (e.g. from within opennec.h itself). */
#ifndef MAX_ERROR_LEN
#define MAX_ERROR_LEN 512
#endif

/*
 * field_validation_t — result of validating a single field on a card.
 * Returned by value; never heap-allocated.
 */
typedef struct {
  error_level severity;         /**< NONE=OK/no-rule, WARNING, PROBLEM, FATAL */
  char message[MAX_ERROR_LEN];  /**< Human-readable description; empty when severity==NONE */
} field_validation_t;

/*
 * validate_card_field
 *
 * Validates a single named field on a card, taking into account the values
 * of other fields on the same card where necessary (for example, FR's F2
 * result depends on I2).
 *
 * @param card        The card to validate (must not be NULL)
 * @param field_name  One of "I1".."I4" or "F1".."F7"
 * @return            field_validation_t by value; severity==NONE if the field
 *                    is valid or no rule is defined for this card/field pair.
 */
field_validation_t validate_card_field(const card_t *card, const char *field_name);


/*
 * validate_card_all_fields
 *
 * Validates all 11 fields on a card in one call. Useful when a card is first
 * displayed and the GUI needs to set the initial state of all field indicators.
 *
 * Results are stored at indices matching field_names[] in types.c:
 *   results[0..3]  => I1..I4
 *   results[4..10] => F1..F7
 *
 * Equivalently: I_idx = fieldN - 1,  F_idx = fieldN + 3
 *
 * @param card     The card to validate (must not be NULL)
 * @param results  Caller-allocated array of 11 field_validation_t values
 */
void validate_card_all_fields(const card_t *card, field_validation_t results[11]);


#endif /* CARD_VALIDATION_H */
