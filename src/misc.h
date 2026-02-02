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
int mem_alloc(nec_context_t *ctx, void **ptr, size_t req);
int mem_realloc(nec_context_t *ctx, void **ptr, size_t req);
void mem_free(nec_context_t *ctx, void **ptr);

/* Error and message handling */
void add_error(nec_context_t *ctx, errors_list_t *errors, char *message, int severity);
void add_message(nec_context_t *ctx, outputs_list_t *outputs, char *message);

/* Timing */
void secnds(nec_context_t *ctx, double *x);

/* Error handling */
int stop(nec_context_t *ctx, int flag);
void abort_on_error(nec_context_t *ctx, int why);

/* String utilities (used internally by input/deck modules) */
char* substr(nec_context_t *ctx, char* dest, char *src, int start, int len);
char* trim_start(char* dest);
char* trim_end(char* dest);
char* trim(char* dest);
int str_ends_with(nec_context_t *ctx, const char *str, const char *suffix);

#endif /* MISC_H */
