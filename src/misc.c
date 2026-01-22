/*
 * Miscellaneous support functions for onec
 */

#include "opennec.h"

/***  ONEC utils ***/

void add_error(nec_context_t *ctx, errors_list_t *errors, char *message, int severity)
{
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

void add_message(nec_context_t *ctx, outputs_list_t *outputs, char *message)
{
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
int str_ends_with(nec_context_t *ctx, const char *str, const char *suffix)
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
char* substr(nec_context_t *ctx, char* dest, char *src, int start, int len)
{
  strncpy(dest, src+start, len);
  dest[len] = '\0';
  return dest;
}

/*-------------------------------------------------------------------*/
char* trim(nec_context_t *ctx, char* str)
{
  trim_start(ctx, str);
  trim_end(ctx, str);
  return str;
}

/*-------------------------------------------------------------------*/
char* trim_start(nec_context_t *ctx, char* dest)
{
  while(isspace((unsigned char)*dest)) dest++;
  return dest;
}

/*-------------------------------------------------------------------*/
char* trim_end(nec_context_t *ctx, char* dest)
{
  char *end;
  end = dest + strlen(dest) - 1;
  while(end > dest && isspace((unsigned char)*end)) end--;
  *(end+1) = '\0'; // new trailing nul
  return dest;
}

/***  Various system/app utils ***/

/*------------------------------------------------------------------------*/
/*  abort_on_error()
 *
 *  prints an error message and exits
 */

void abort_on_error(nec_context_t *ctx, int why)
{
  switch(why)
  {
	case -1 : /* abort if input file name too long */
	  fprintf( ctx->error_fp, "%s\n",
		  "onec: Input file name too long - aborting" );
	  break;

	case -2 : /* abort if output file name too long */
	  fprintf( ctx->error_fp, "%s\n",
		  "onec: Output file name too long - aborting" );
	  break;

	case -3 : /* abort on input file read error */
	  fprintf( ctx->error_fp, "%s\n",
		  "onec: Error reading input file - aborting" );
	  break;

	case -4 : /* Abort on malloc failure */
	  fprintf( ctx->error_fp, "%s\n",
		  "onec: A memory allocation request has failed - aborting" );
	  break;

	case -5 : /* Abort if a GF card is read */
	  fprintf( ctx->error_fp, "%s\n",
		  "onec: NGF solution option not supported - aborting" );
	  break;

	case -6: /* No convergence in gshank() */
	  fprintf( ctx->error_fp, "%s\n",
		  "onec: No convergence in gshank() - aborting" );
	  break;

	case -7: /* Error in hankel() */
	  fprintf( ctx->error_fp, "%s\n",
		  "onec: Hankel not valid for z=0. - aborting" );

  }  /* switch( why ) */

  /* clean up and quit */
  stop(ctx, why);

} /* end of abort_on_error() */

/*------------------------------------------------------------------------*/

/* Returns process time (user+system) BUT in _msec_ */
void secnds(nec_context_t *ctx, double *x)
{
  struct tms buffer;
  double clk_tck;

  times(&buffer);
  clk_tck = sysconf( _SC_CLK_TCK );
  *x = 1000.0 * (double)(buffer.tms_utime + buffer.tms_stime) / clk_tck;

  return;
}

/*------------------------------------------------------------------------*/

/* Does the STOP function of fortran but with return value */
int stop( nec_context_t *ctx, int flag )
{
  if( ctx->input_fp != NULL )
    fclose( ctx->input_fp );
  if( ctx->output_fp != NULL )
    fclose( ctx->output_fp );
  if( ctx->plot_fp != NULL )
    fclose( ctx->plot_fp );

  exit( flag );
}

/***  Memory allocation/freeing utils ***/

/*------------------------------------------------------------------------*/

void mem_alloc( nec_context_t *ctx, void **ptr, size_t req )
{
  mem_free(ctx, ptr );
  *ptr = malloc( req );
  if( *ptr == NULL )
	abort_on_error(ctx, -4 );

} /* End of void mem_alloc() */

/*------------------------------------------------------------------------*/

void mem_realloc(nec_context_t *ctx, void **ptr, size_t req)
{
  *ptr = realloc(*ptr, req);
  if(*ptr == NULL)
	abort_on_error(ctx, -4 );

} /* End of void mem_realloc() */

/*------------------------------------------------------------------------*/

void mem_free( nec_context_t *ctx, void **ptr )
{
  if( *ptr != NULL )
	free( *ptr );
  *ptr = NULL;

} /* End of void free_ptr() */

/*------------------------------------------------------------------------*/

