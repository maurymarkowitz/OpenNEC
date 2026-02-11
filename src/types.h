/******************************************************************************
 *
 * types.h
 *
 * types.h defines the many data structures that are used to pass
 * data around the calculation system. In nec2c these were defined
 * in nec2c.h, and have been moved here for clarity. OpenNEC also
 * adds new types for the deck_t and card_t, so they can be passed
 * instead of using globals. Other new types include Error and
 * Errors, key_value_t, and various definitions of measurements and such.
 *
 *****************************************************************************/

#pragma once

#ifndef TYPES_H
#define TYPES_H

#include <complex.h>
#include <stdio.h>
#include <stdbool.h>  // we will use the bool type!
#include <time.h>     // for clock_t and timing

// NEC has 4 int fields
#ifndef MAX_INT_FIELDS_DEF
#define MAX_INT_FIELDS_DEF
#define MAX_INT_FIELDS 4
#endif

// NEC has 7 float fields
#ifndef MAX_FLT_FIELDS_DEF
#define MAX_FLT_FIELDS_DEF
#define MAX_FLT_FIELDS 7
#endif

// OpenNEC generally allows commas or any whitespace between fields
#ifndef ONEC_WHITESPACE_DEF
#define ONEC_WHITESPACE_DEF
#define ONEC_WHITESPACE ", \t\n\r\v\f\0"
#endif

// these are the markers for *inline* comments
// does not include #, which is used by nec2c, but that can only
// appear at the start of the line, not inline, because of AWG measurements
#ifndef ONEC_COMMENTS_DEF
#define ONEC_COMMENTS_DEF
#define ONEC_COMMENTS "!'"
#endif

// these are the separators within an OpenNEC extension list
#ifndef ONEC_SEPARATORS_DEF
#define ONEC_SEPARATORS_DEF
#define ONEC_SEPARATORS ";,"
#endif

// these are the delimeters between the keys and values
#ifndef ONEC_DELIMETERS_DEF
#define ONEC_DELIMETERS_DEF
#define ONEC_DELIMETERS "=:"
#endif

/* card field names, like "I1" of "F4" */
#ifndef FIELD_NAMES_DEF
#define FIELD_NAMES_DEF
#define NUM_FIELD_NAMES 11
extern char *field_names[NUM_FIELD_NAMES];
#endif

/* input card mnemonic list */
/* "XT" stands for "exit", added for testing, not included in these lists */
#ifndef COMMENT_CODES_DEF
#define COMMENT_CODES_DEF
#define NUM_COMMENT_CODES  5
extern char *comment_codes[NUM_COMMENT_CODES];
#endif

#ifndef CONTROL_CODES_DEF
#define CONTROL_CODES_DEF
#define NUM_CONTROL_CODES  20
extern char *control_codes[NUM_CONTROL_CODES];
#endif

#ifndef GEOMETRY_CODES_DEF
#define GEOMETRY_CODES_DEF
#define NUM_GEOMETRY_CODES  13
extern char *geometry_codes[NUM_GEOMETRY_CODES];
#endif

#ifndef ONEC_CODES_DEF
#define ONEC_CODES_DEF
#define NUM_ONEC_CODES 4
extern char *onec_codes[NUM_ONEC_CODES];
#endif

/* tinyexpr variable names for field bindings */
#ifndef ONEC_FIELD_VAR_NAMES_DEF
#define ONEC_FIELD_VAR_NAMES_DEF
/*
 * The first entry is an empty string to align with 1-based indexing
 * used throughout the codebase for NEC fields (F1..F7, I1..I4).
 * Index 0 is intentionally unused.
 */
extern const char *fnames[MAX_FLT_FIELDS + 1];
extern const char *inames[MAX_INT_FIELDS + 1];
#endif

/*** Structs encapsulating global ("common") variables */

/*** Error levels are used internally, external software should use negatives ***/
typedef enum { NONE, WARNING, PROBLEM, FATAL } error_level;    // 1 = warning, 2 = error, 3 = fatal, <0 informational

/*** error_t has information about a single error or warning */
typedef struct
{
  int severity;
  char *message;  // the error string
} error_t;

/*** errors_list_t is a list generated during a particular stage, typically
there will be different error lists for import, sanity checks, running
and export
 ***/
typedef struct
{
  int num_errors; // total number of errors in this list
  error_t *errors;  // pointer to a list of errors
} errors_list_t;

/*** outputs_list_t is a list of informational messages generated during
processing, to be output later by output.c instead of direct fprintf
 ***/
typedef struct
{
  int num_messages; // total number of messages in this list
  char **messages;  // pointer to a list of message strings
} outputs_list_t;

/*** key_value_t is a key:value pair used to store an OpenNEC extension on a card */
typedef struct key_value_t
{
	unsigned int magic;
	char *key;
	char *value;
	double fv; // new field for storing a float value
	char separator; // what separator was used, a colon or an equals?
	struct key_value_t* next;
} key_value_t;

/*** card_t encapsulates a single card ***/
typedef struct card_t
{
  // used to track whether this card has been edited since being read
  bool edited;
  
  // raw data from the original card
  char *orig_str;     // the original line, as read from the file in raw format
  char *card_str;     // the "card part" of the string, everything in front of the inline comment (if one exists)
  
  // processed NEC2 data
  char card_code[3];  // the two-letter code for this card, or one letter for some comment formats
  
  // NEC uses i1 through i1 and f1 through f7. We'll put these in an
  // array to ease access when we're looping: f[i]. This could lead
  // to confusion because normally C would be zero-indexed, like f[0].
  // To avoid this we'll make the array one larger than it has to be
  // and just leave the zeroth entry empty.
  int i[5];           // i1 is normally the tag, etc.
  double f[8];        // geometery and so forth
  
  // the values above are the raw inputs, they may include units and/or
  // formulas that need to be calculated. these arrays hold the final
  // values that will be fed into the calculation engines. v for "value"
  int iv[5];
  double fv[8];

  // different cards have different numbers of inputs, so these are
  // used to track how many we actually read in
  int ints_used;      // the number of int parameters
  int flts_used;      // ...and floats
  
  // tags and segments are normally printed as they are calculated,
  // but onec only does that once, so we'll store them here so we
  // can print them out later
  int tag;
  int num_segments;
  int start_segment;
  int end_segment;
  bool int_form_inline[4];// was this formula found inline, or in a comment?
  bool flt_form_inline[8];
  
  // onec extensions
  char extn_code[1];  // the one-letter code that marked the extension or inline comment, if any
  char *extn_str;     // the entire inline comment, anything after the comment marker
  char *comment;      // if a comment was found, it's placed here, this is *not* the same
                      //    as extn_str, it might be found in a 'comment:' key/value pair
  key_value_t *extensns; // pairs of name:value key/value entries, this will **not** include a comment if there was one
  key_value_t *formulas; // pairs of name:value key/value entries for any formulas, inline or in the extension area

  // onec flags - only this one needs to be known during processing
  bool ignore;        // cards can be marked to be deliberately ignored
} card_t;

/*** deck_t encapsulates a single deck of cards ***/
typedef struct deck_t
{
  // input data
  card_t *cards;        // array of cards
  int num_cards;      // total number of cards read in, including any trailing lines
  int comment_start;  // card number of the start of the comments section, normally 0. -1 if there are no CM or CE cards
  int comment_end;    // card number of the last continuous CM card, or the CE card if present. -1 if there are no CM or CE cards
  int geometry_start; // card number of the first geometry card, which definitely should exist. -1 if not found
  int geometry_end;   // card number of the GE card, which also has to exist. -1 if not found
  int deck_end;       // card number of the EN card or the last card in the deck otherwise. -1 if not found
  char cmt_code;      // the default marker to use for inline comments, !, $ or '
  int unit_val;       // if there is a single GS, this is the f1 value, otherwise 1
  int unit_typ;       // if there is a single GS, and we recognize the value, put our index here
  key_value_t **symbols;  // array of pointers to key_value_t nodes in the cards (not owned)
  int num_symbols;        // number of symbols in the array
} deck_t;

/* Forward declaration for Opaque Handle support */
typedef struct nec_context_t nec_context_t;

/* Context Lifecycle - Opaque Handle style */
nec_context_t* nec_create_context(void);
void nec_destroy_context(nec_context_t *ctx);

// typedefs for backward compatibility
typedef card_t Card;
typedef deck_t Deck;

#endif /* TYPES_H */
