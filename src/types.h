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

/**
 * @enum field_sep_t
 * @brief Describes the field separator style used in a card's raw input.
 *
 * Captured during parsing to allow round-trip output to preserve the
 * original file's formatting style. At the deck level, only set when
 * all geometry and control cards agree on the same style.
 */
typedef enum {
  FSEP_UNKNOWN = 0,     /**< Not detected or cards disagree */
  FSEP_SPACE,           /**< Single space between fields */
  FSEP_COLUMN_ALIGNED,  /**< Two or more spaces (column-aligned style) */
  FSEP_TAB,             /**< Tab character between fields */
  FSEP_COMMA,           /**< Comma between fields */
} field_sep_t;

/*** card_t encapsulates a single card ***/
/**
 * @struct card_t
 * @brief Represents a single line (card) in a NEC deck.
 *
 * This structure holds the raw string data, processed field values (I1..I4, F1..F7),
 * and any OpenNEC-specific extensions or formulas.
 */
typedef struct card_t
{
  bool edited;        /**< true if the card has been modified since being read */
  field_sep_t field_sep; /**< Field separator style detected during parsing */

  char *orig_str;     /**< The original raw string line from the file */
  char *card_str;     /**< The card content minus any inline comments */
  
  char card_code[3];  /**< Two-letter NEC card mnemonic (e.g., "GW", "LD") */
  
  int i[5];           /**< Raw integer input fields (1-based: i[1]..i[4]) */
  double f[8];        /**< Raw float input fields (1-based: f[1]..f[7]) */
  
  int iv[5];          /**< Calculated integer values after formula/unit evaluation */
  double fv[8];       /**< Calculated float values after formula/unit evaluation */

  int ints_used;      /**< Number of integer parameters found on this card */
  int flts_used;      /**< Number of float parameters found on this card */
  
  int tag;            /**< Geometry tag number associated with this card */
  int num_segments;   /**< Number of segments generated by this card */
  int start_segment;  /**< Index of the first segment in the global list */
  int end_segment;    /**< Index of the last segment in the global list */
  bool int_form_inline[5]; /**< true if the integer field was defined by an inline formula (1-based) */
  bool flt_form_inline[8]; /**< true if the float field was defined by an inline formula (1-based) */
  
  char extn_code[1];  /**< Code character for the OpenNEC extension (if any) */
  char *extn_str;     /**< Full string of the OpenNEC extension */
  char *comment;      /**< Captured comment string */
  key_value_t *extensns; /**< Linked list of key-value extensions */
  key_value_t *formulas; /**< Linked list of formulas for substitution */

  bool ignore;        /**< true if this card should be skipped during processing */
} card_t;

/**
 * @struct deck_t
 * @brief Represents a collection of cards forming a complete simulation input.
 */
typedef struct deck_t
{
  card_t *cards;      /**< Array of cards in the deck */
  int num_cards;      /**< Total number of cards */
  
  int comment_start;  /**< Index of the first continuous CM card */
  int comment_end;    /**< Index of the last CM or the CE card */
  int symbol_start;   /**< Index of the first SY card after CE (-1 if none) */
  int symbol_end;     /**< Index of the last SY card before geometry (-1 if none) */
  int geometry_start; /**< Index of the first geometry card (usually GW) */
  int geometry_end;   /**< Index of the GE (Geometry End) card */
  int deck_end;       /**< Index of the EN (Execution End) card */
  char cmt_code;      /**< Default marker used for inline comments ('!', '$', or ''') */
  int unit_val;       /**< GS card scaling value (default 1) */
  int unit_typ;       /**< Recognized index for GS unit type */
  key_value_t **symbols; /**< Array of symbols (SY) found in the deck */
  int num_symbols;    /**< Total number of symbols */
  field_sep_t field_sep; /**< Separator style shared by all geo/control cards, or FSEP_UNKNOWN if mixed */
} deck_t;

/** @brief Opaque handle to the internal simulation state. 
 *  Forward declared to maintain ABI stability.
 */
typedef struct nec_context_t nec_context_t;

/** @name Context Lifecycle
 *  Functions for creating and destroying the simulation context.
 *  @{
 */

/** @brief Creates and initializes a new NEC simulation context.
 *  @return Pointer to the new context, or NULL if allocation fails.
 */
nec_context_t* nec_create_context(void);

/** @brief Safely destroys a context and frees all associated memory.
 *  @param ctx The context to destroy.
 */
void nec_destroy_context(nec_context_t *ctx);
/** @} */

/** @name Logging and Callbacks
 *  Modern interfaces for capturing simulation output and errors.
 *  @{
 */

/** @brief Severity levels for log messages */
typedef enum {
    ONEC_SEV_INFO = 0,    /**< Information regarding progress or state */
    ONEC_SEV_WARNING = 1, /**< Non-fatal issues (e.g., slightly overlapping segments) */
    ONEC_SEV_ERROR = 2,   /**< Calculation issues that may invalidate results */
    ONEC_SEV_FATAL = 3    /**< Unrecoverable errors that halt simulation */
} nec_severity_t;

/** 
 * @brief Callback signature for capturing log messages.
 * @param user_data User-provided context pointer.
 * @param level Severity level of the message.
 * @param message The null-terminated message string.
 */
typedef void (*nec_log_callback_t)(void *user_data, int level, const char *message);

/** 
 * @brief Registers a callback to receive real-time log updates.
 * @param ctx The simulation context.
 * @param callback The function to call when a log occurs.
 * @param user_data Pointer passed back to the callback.
 */
void nec_set_log_callback(nec_context_t *ctx, nec_log_callback_t callback, void *user_data);
/** @} */

// typedefs for backward compatibility
typedef card_t Card;
typedef deck_t Deck;

#endif /* TYPES_H */
