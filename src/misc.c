/******************************************************************************
 * misc.c
 *
 * Miscellaneous support functions for OpenNEC. This module provides utility
 * and helper routines used throughout the codebase, including error handling
 * and general-purpose helpers.
 *
 *****************************************************************************/

#include "internals.h"
#include <unistd.h>
#include <sys/times.h>
#include <stdarg.h>
#include <stdio.h>

/***  ONEC utils ***/

void add_error(const nec_context_t *ctx, errors_list_t *errors, char *message, int severity)
{
  // Trigger callback and logging via unified helper
  nec_report(ctx, severity, "%s", message);

  // make a new error object and fill it out
  error_t newErr;
  newErr.severity = severity;
  newErr.message = calloc(strlen(message) + 1, sizeof(char));
  strcpy(newErr.message, message);
  // now put it into the error list
  if(errors->num_errors == 0) {
    errors->errors = calloc(1, sizeof(error_t));
  } else {
    errors->errors = realloc(errors->errors, (errors->num_errors + 1) * sizeof(error_t));
  }
  errors->errors[errors->num_errors] = newErr;
  errors->num_errors++;
}

void add_message(const nec_context_t *ctx, outputs_list_t *outputs, char *message)
{
  // Trigger callback and logging via unified helper
  nec_report(ctx, ONEC_SEV_INFO, "%s", message);

  // make a new message string
  char *newMsg = calloc(strlen(message) + 1, sizeof(char));
  strcpy(newMsg, message);
  // now put it into the message list
  if(outputs->num_messages == 0) {
    outputs->messages = calloc(1, sizeof(char *));
  } else {
    outputs->messages = realloc(outputs->messages, (outputs->num_messages + 1) * sizeof(char *));
  }
  outputs->messages[outputs->num_messages] = newMsg;
  outputs->num_messages++;
}

/***  String utils ***/

/*-------------------------------------------------------------------*/
int str_ends_with(const nec_context_t *ctx, const char *str, const char *suffix)
{
    if (!str || !suffix)
        return 1;
  
    size_t lenstr = strlen(str);
    size_t lensuffix = strlen(suffix);
  
    if (lensuffix > lenstr)
        return 1;
  
    return strcasecmp(str + lenstr - lensuffix, suffix);
}

/*-------------------------------------------------------------------*/
char* substr(const nec_context_t *ctx, char* dest, char *src, int start, int len)
{
  strncpy(dest, src+start, len);
  dest[len] = '\0';
  return dest;
}


/*-------------------------------------------------------------------*/
char* trim_start(char* dest)
{
  while(isspace((unsigned char)*dest)) dest++;
  return dest;
}

/*-------------------------------------------------------------------*/
char* trim_end(char* dest)
{
  char *end = dest + strlen(dest) - 1;
  while(end > dest && isspace((unsigned char)*end)) end--;
  *(end+1) = '\0'; // new trailing nul
  return dest;
}

/*-------------------------------------------------------------------*/
char* trim(char* str)
{
  char *start = trim_start(str);
  char *end = start + strlen(start) - 1;
  while(end > start && isspace((unsigned char)*end)) end--;
  *(end+1) = '\0';
  return start;
}

/*-------------------------------------------------------------------*/
void nec_report(const nec_context_t *ctx, int level, const char *format, ...)
{
    char buffer[2048];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    // Trigger callback if registered
    if (ctx && ctx->log_callback) {
        ctx->log_callback(ctx->log_user_data, level, buffer);
    }

    // Also write to error_fp if it exists
    if (ctx && ctx->error_fp) {
        fprintf(ctx->error_fp, "%s\n", buffer);
        fflush(ctx->error_fp);
    }
}

/***  Various system/app utils ***/

/*------------------------------------------------------------------------*/
/*  abort_on_error()
 *
 *  prints an error message and exits
 */

void abort_on_error(const nec_context_t *ctx, int why)
{
  switch(why)
  {
	case -1 : /* abort if input file name too long */
	  nec_report(ctx, ONEC_SEV_FATAL, "onec: Input file name too long - aborting");
	  break;

	case -2 : /* abort if output file name too long */
	  nec_report(ctx, ONEC_SEV_FATAL, "onec: Output file name too long - aborting");
	  break;

	case -3 : /* abort on input file read error */
	  nec_report(ctx, ONEC_SEV_FATAL, "onec: Error reading input file - aborting");
	  break;

	case -4 : /* Abort on malloc failure */
	  nec_report(ctx, ONEC_SEV_FATAL, "onec: A memory allocation request has failed - aborting");
	  break;

	case -5 : /* Abort if a GF card is read */
	  nec_report(ctx, ONEC_SEV_FATAL, "onec: NGF solution option not supported - aborting");
	  break;

	case -6: /* No convergence in gshank() */
	  nec_report(ctx, ONEC_SEV_FATAL, "onec: No convergence in gshank() - aborting");
	  break;

	case -7: /* Error in hankel() */
	  nec_report(ctx, ONEC_SEV_FATAL, "onec: Hankel not valid for z=0. - aborting");

  }  /* switch( why ) */

  /* clean up and quit */
  exit(why);

} /* end of abort_on_error() */

/*------------------------------------------------------------------------*/

/* Returns process time (user+system) BUT in _msec_ */
void secnds(const nec_context_t *ctx, double *x)
{
  struct tms buffer;
  double clk_tck;

  times(&buffer);
  clk_tck = sysconf( _SC_CLK_TCK );
  *x = 1000.0 * (double)(buffer.tms_utime + buffer.tms_stime) / clk_tck;

  return;
}

/*------------------------------------------------------------------------*/

/***  Memory allocation/freeing utils ***/

/*------------------------------------------------------------------------*/

int mem_alloc( const nec_context_t *ctx, void **ptr, size_t req )
{
  mem_free(ctx, ptr );
  *ptr = malloc( req );
  if( *ptr == NULL ) {
	add_error(ctx, (errors_list_t*)&ctx->errors, "Memory allocation failed", FATAL);
	return -1;
  }
  return 0;
} /* End of mem_alloc() */

/*------------------------------------------------------------------------*/

int mem_realloc(const nec_context_t *ctx, void **ptr, size_t req)
{
  *ptr = realloc(*ptr, req);
  if(*ptr == NULL) {
	add_error(ctx, (errors_list_t*)&ctx->errors, "Memory reallocation failed", FATAL);
	return -1;
  }
  return 0;
} /* End of mem_realloc() */

/*------------------------------------------------------------------------*/

void mem_free( const nec_context_t *ctx, void **ptr )
{
  if( *ptr != NULL )
	free( *ptr );
  *ptr = NULL;

} /* End of void free_ptr() */

/*------------------------------------------------------------------------*/

