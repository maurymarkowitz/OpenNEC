/*
 * misc.h - Core utility functions for OpenNEC
 * 
 * Memory management, error handling, and timing utilities
 * used throughout the codebase.
 */

#ifndef MISC_H
#define MISC_H

#include "types.h"
#include "compat.h"

/* Memory management */
int mem_alloc(const context_t *ctx, void **ptr, size_t req);
int mem_realloc(const context_t *ctx, void **ptr, size_t req);
void mem_free(const context_t *ctx, void **ptr);

/* Error and message handling */
void add_error(const context_t *ctx, errors_list_t *errors, char *message, int severity);
void add_message(const context_t *ctx, outputs_list_t *outputs, char *message);
void transfer_errors(errors_list_t *src, errors_list_t *dst);

/* Path utilities */
/**
 * Resolve path relative to a source file's directory.
 * Absolute paths are returned as-is. Relative paths are anchored to the
 * directory of source_filename so that NEC deck files referencing NGF/WGF
 * files by bare name or relative path resolve to the same directory as the
 * deck, not the process CWD.  When source_filename has no directory component
 * (bare name) or is NULL/empty, path is used as-is (CWD-relative).
 */
void resolve_path_relative_to_input(const char *path, const char *source_filename,
                                    char *buf, size_t bufsz);

/* Unified logging and reporting */
void report(const context_t *ctx, int level, const char *format, ...) __attribute__ ((format (printf, 3, 4)));

/* Timing */
/** @brief Returns high-resolution monotonic time in milliseconds */
void get_time_ms(const context_t *ctx, double *ms);

/**
 * @brief Returns a dimensionless complexity estimate proportional to run time.
 *
 * Based on the NEC-2 Part III performance formula (T1+T2+T3+T4) with unit
 * coefficients. T is not in seconds; compare against a platform-calibrated
 * threshold to classify a run as fast or slow. On an M2 Mac using Accelerate,
 * anything with T < ~10^7 generally runs in about 1/10th of a second or less,
 * while T > ~10^9 takes multiple seconds.
 *
 * T4 = 0 when no RP card is present; Nc (wire-surface junctions) is omitted.
 * When a FR card is present, Nfreq multiplies the entire T (fill, factorisation,
 * solve, and far-field all repeat for each frequency in NEC-2).
 * See estimate_time() in misc.c for the full formula and design notes.
 *
 * @param  ctx   NEC context (geometry is populated as a side effect if not already done)
 * @param  deck  Parsed deck (symbols evaluated)
 * @return       T >= 0.0, or 0.0 if ctx or deck is NULL
 */
double estimate_time(context_t *ctx, deck_t *deck);

/* Error handling */
int stop(const context_t *ctx, int flag);
void abort_on_error(const context_t *ctx, int why);

/* String utilities (used internally by input/deck modules) */
char* substr(const context_t *ctx, char* dest, char *src, int start, int len);
char* trim_start(char* dest);
char* trim_end(char* dest);
char* trim(char* dest);
int str_ends_with(const context_t *ctx, const char *str, const char *suffix);

/* Field separator detection */
field_sep_t detect_field_separator(const char *card_str);

/* 4nec2 Preprocessing */
char *preprocess_awg(const char *formula);
char *preprocess_implicit_multiplication(const char *formula);
char *preprocess_max_min(const char *formula);  /* OpenNEC: convert max/min to max1-7/min1-7 */
double convert_awg_to_meters(double awg_value);

#endif /* MISC_H */
