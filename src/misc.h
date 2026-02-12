/*
 * misc.h - Core utility functions for OpenNEC
 * 
 * Memory management, error handling, and timing utilities
 * used throughout the codebase.
 */

#ifndef MISC_H
#define MISC_H

#include "types.h"

/* Memory management */
int mem_alloc(const nec_context_t *ctx, void **ptr, size_t req);
int mem_realloc(const nec_context_t *ctx, void **ptr, size_t req);
void mem_free(const nec_context_t *ctx, void **ptr);

/* Error and message handling */
void add_error(const nec_context_t *ctx, errors_list_t *errors, char *message, int severity);
void add_message(const nec_context_t *ctx, outputs_list_t *outputs, char *message);

/* Unified logging and reporting */
void nec_report(const nec_context_t *ctx, int level, const char *format, ...) __attribute__ ((format (printf, 3, 4)));

/* Timing */
/** @brief Returns high-resolution monotonic time in milliseconds */
void nec_get_time_ms(const nec_context_t *ctx, double *ms);

/* Error handling */
int stop(const nec_context_t *ctx, int flag);
void abort_on_error(const nec_context_t *ctx, int why);

/* String utilities (used internally by input/deck modules) */
char* substr(const nec_context_t *ctx, char* dest, char *src, int start, int len);
char* trim_start(char* dest);
char* trim_end(char* dest);
char* trim(char* dest);
int str_ends_with(const nec_context_t *ctx, const char *str, const char *suffix);

/* 4nec2 Preprocessing */
char *preprocess_line(const char *line);
char *preprocess_awg(const char *formula);
char *preprocess_feet_inches(const char *formula);
char *preprocess_implicit_multiplication(const char *formula);
double convert_awg_to_meters(double awg_value);

#endif /* MISC_H */
