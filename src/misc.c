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

    // Trigger callback if registered (always fire for callback)
    if (ctx && ctx->log_callback) {
        ctx->log_callback(ctx->log_user_data, level, buffer);
    }

    // Also write to error_fp if it exists and level is WARNING or higher
    // Informational messages (INFO) are suppressed from the console by default
    if (ctx && ctx->error_fp && level >= ONEC_SEV_WARNING) {
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

/******************************************************************************
 * preprocess_line
 *
 * Applies all 4nec2 preprocessing steps to a card line.
 * Returns a new allocated string that must be freed by the caller.
 */
char *preprocess_line(const char *line) {
  char *t1 = preprocess_awg(line);
  char *t2 = preprocess_feet_inches(t1);
  free(t1);
  char *t3 = preprocess_implicit_multiplication(t2);
  free(t2);
  return t3;
}

/******************************************************************************
 * convert_awg_to_meters
 *
 * convert_awg_to_meters returns the radius in meters for a given AWG value.
 * Supports standard gauges (0-40) and large wire gauges (4/0 through 1/0).
 * Large wire gauges are represented as negative values: 4/0=-3, 3/0=-2, 2/0=-1, 1/0=0
 */
double convert_awg_to_meters(double awg_value)
{
  int awg_code = (int)floor(awg_value);

  // any decimal part is bad!
  if(awg_value != awg_code) {
    return -1.0;
  }

  switch(awg_code) {
    // Large wire gauges (negative values represent N/0 format)
    case -3: return 0.11684;   // 4/0 or 0000
    case -2: return 0.104049;  // 3/0 or 000
    case -1: return 0.092658;  // 2/0 or 00
    case 0: return 0.082515;   // 1/0 or 0
    // Standard AWG gauges
    case 1: return 0.073481;
    case 2: return 0.065437;
    case 3: return 0.058273;
    case 4: return 0.051894;
    case 5: return 0.046213;
    case 6: return 0.041154;
    case 7: return 0.036649;
    case 8: return 0.032636;
    case 9: return 0.029064;
    case 10: return 0.025882;
    case 11: return 0.023048;
    case 12: return 0.020525;
    case 13: return 0.018278;
    case 14: return 0.016277;
    case 15: return 0.014495;
    case 16: return 0.012908;
    case 17: return 0.011495;
    case 18: return 0.010237;
    case 19: return 0.009116;
    case 20: return 0.008118;
    case 21: return 0.007229;
    case 22: return 0.006438;
    case 23: return 0.005733;
    case 24: return 0.005106;
    case 25: return 0.004547;
    case 26: return 0.004049;
    case 27: return 0.003606;
    case 28: return 0.003211;
    case 29: return 0.002859;
    case 30: return 0.002546;
    case 31: return 0.002268;
    case 32: return 0.002019;
    case 33: return 0.001798;
    case 34: return 0.001601;
    case 35: return 0.001426;
    case 36: return 0.00127;
    case 37: return 0.001131;
    case 38: return 0.001007;
    case 39: return 0.000897;
    case 40: return 0.000799;
    default: return -1.0;
  }
}

/******************************************************************************
 * preprocess_awg
 *
 * Preprocesses AWG wire gauge notation in formulas:
 * - "#14" -> radius in meters
 * - "14awg" -> radius in meters
 */
char *preprocess_awg(const char *formula) {
  char *result = strdup(formula);
  char *p = result;
  
  while (*p) {
    if (*p == '#') {
      char *start = p;
      char *num_start = p + 1;
      double gauge_value = 0;
      bool valid_awg = false;
      
      // Handle 4nec2 special cases: 00, 1/0, 2/0, 3/0, 4/0
      if (strncasecmp(num_start, "4/0", 3) == 0 || strncasecmp(num_start, "0000", 4) == 0) {
        gauge_value = -3;
        valid_awg = true;
        num_start += (strncasecmp(num_start, "4/0", 3) == 0) ? 3 : 4;
      } else if (strncasecmp(num_start, "3/0", 3) == 0 || strncasecmp(num_start, "000", 3) == 0) {
        gauge_value = -2;
        valid_awg = true;
        num_start += 3;
      } else if (strncasecmp(num_start, "2/0", 3) == 0 || strncasecmp(num_start, "00", 2) == 0) {
        gauge_value = -1;
        valid_awg = true;
        num_start += (strncasecmp(num_start, "2/0", 3) == 0) ? 3 : 2;
      } else if (strncasecmp(num_start, "1/0", 3) == 0) {
        gauge_value = 0;
        valid_awg = true;
        num_start += 3;
      } else {
        // Try to parse as regular number
        char *endptr;
        long gauge = strtol(num_start, &endptr, 10);
        if (endptr > num_start && gauge >= 0 && gauge <= 40) {
          gauge_value = (double)gauge;
          valid_awg = true;
          num_start = endptr;
        }
      }
      
      if (valid_awg) {
        // Convert AWG gauge to radius in meters
        double radius = convert_awg_to_meters(gauge_value);
        
        // Replace #NN with the numerical value
        char replacement[32];
        snprintf(replacement, sizeof(replacement), "%.10f", radius);
        
        // Calculate lengths
        size_t prefix_len = start - result;
        size_t replacement_len = strlen(replacement);
        size_t suffix_len = strlen(num_start);
        
        // Allocate new string
        char *new_result = malloc(prefix_len + replacement_len + suffix_len + 1);
        memcpy(new_result, result, prefix_len);
        memcpy(new_result + prefix_len, replacement, replacement_len);
        memcpy(new_result + prefix_len + replacement_len, num_start, suffix_len + 1);
        
        free(result);
        result = new_result;
        p = result + prefix_len + replacement_len;
      } else {
        p++;
      }
    } else if (isdigit((unsigned char)*p) || *p == '-') {
      // Check for "NNawg" format
      char *num_start = p;
      char *endptr;
      
      // Try to parse the number
      long gauge = strtol(num_start, &endptr, 10);
      
      // Check if followed by "awg" (case insensitive)
      if (endptr > num_start && strncasecmp(endptr, "awg", 3) == 0) {
        double gauge_value = (double)gauge;
        
        // Handle negative values for large wire gauges
        // The parser may have already converted 4/0 to -3, etc.
        double radius = convert_awg_to_meters(gauge_value);
        
        // Replace NNawg with the numerical value
        char replacement[32];
        snprintf(replacement, sizeof(replacement), "%.10f", radius);
        
        // Calculate lengths
        size_t prefix_len = num_start - result;
        size_t replacement_len = strlen(replacement);
        char *suffix_start = endptr + 3; // skip "awg"
        size_t suffix_len = strlen(suffix_start);
        
        // Allocate new string
        char *new_result = malloc(prefix_len + replacement_len + suffix_len + 1);
        memcpy(new_result, result, prefix_len);
        memcpy(new_result + prefix_len, replacement, replacement_len);
        memcpy(new_result + prefix_len + replacement_len, suffix_start, suffix_len + 1);
        
        free(result);
        result = new_result;
        p = result + prefix_len + replacement_len;
      } else {
        p++;
      }
    } else {
      p++;
    }
  }
  
  return result;
}

/******************************************************************************
 * preprocess_feet_inches
 *
 * Preprocesses feet/inches syntax in formulas:
 * - "N ft / M in" -> "N*ft + M*in" (feet and inches combined)
 * - "Nft/in" -> "N*ft/in" (feet divided by inches for unit conversion)
 */
char *preprocess_feet_inches(const char *formula) {
  char *result = strdup(formula);
  char *p = result;
  
  while (*p) {
    // Look for "ft/" pattern
    if (strncasecmp(p, "ft/", 3) == 0) {
      char *ft_pos = p;
      char *slash_pos = p + 2; // position of '/'
      char *after_slash = slash_pos + 1;
      
      // Skip whitespace after /
      while (*after_slash == ' ' || *after_slash == '\t') after_slash++;
      
      // Check what comes after the slash
      if (strncasecmp(after_slash, "in", 2) == 0) {
        // Pattern: Nft/in (without a number after slash)
        // Find the feet number before "ft"
        char *before_ft = ft_pos - 1;
        while (before_ft >= result && (*before_ft == ' ' || *before_ft == '\t')) before_ft--;
        
        // Find start of feet number
        char *feet_start = before_ft;
        while (feet_start > result && (isdigit((unsigned char)*(feet_start-1)) || *(feet_start-1) == '.' || *(feet_start-1) == '-')) feet_start--;
        
        if (feet_start <= before_ft && (isdigit((unsigned char)*feet_start) || *feet_start == '-')) {
          // Found a number before ft
          // Replace: Nft/in -> N*ft/in
          char *endptr;
          double feet_value = strtod(feet_start, &endptr);
          if (endptr >= ft_pos - 1) { // number extends to ft
            char replacement[128];
            snprintf(replacement, sizeof(replacement), "%.10f*ft/in", feet_value);
            
            // Calculate lengths
            size_t prefix_len = feet_start - result;
            size_t replacement_len = strlen(replacement);
            char *suffix_start = after_slash + 2; // skip "in"
            size_t suffix_len = strlen(suffix_start);
            
            // Allocate new string
            char *new_result = malloc(prefix_len + replacement_len + suffix_len + 1);
            memcpy(new_result, result, prefix_len);
            memcpy(new_result + prefix_len, replacement, replacement_len);
            memcpy(new_result + prefix_len + replacement_len, suffix_start, suffix_len + 1);
            
            free(result);
            result = new_result;
            p = result + prefix_len + replacement_len;
            continue;
          }
        }
      } else if ((isdigit((unsigned char)*after_slash) || (*after_slash == '-' && isdigit((unsigned char)*(after_slash+1))))) {
        // Original pattern: N ft / M in
        char *inches_start = after_slash;
        char *endptr;
        
        // Parse inches number
        double inches_value = strtod(inches_start, &endptr);
        if (endptr > inches_start) {
          // Skip whitespace
          char *after_inches_num = endptr;
          while (*after_inches_num == ' ' || *after_inches_num == '\t') after_inches_num++;
          
          // Check for "in"
          if (strncasecmp(after_inches_num, "in", 2) == 0) {
            // Found "ft/ number in" pattern
            // Now find the feet number before "ft"
            char *before_ft = ft_pos - 1;
            while (before_ft >= result && (*before_ft == ' ' || *before_ft == '\t')) before_ft--;
            
            // Find start of feet number
            char *feet_start = before_ft;
            while (feet_start >= result && (isdigit((unsigned char)*feet_start) || *feet_start == '.' || *feet_start == '-')) feet_start--;
            feet_start++; // move past the non-digit
            
            // Parse feet number
            double feet_value = strtod(feet_start, &endptr);
            if (endptr == before_ft + 1) { // should end at the space before ft
              // Replace the entire pattern: feet_number ft / inches_number in -> feet_value*ft + inches_value*in
              char replacement[128];
              snprintf(replacement, sizeof(replacement), "%.10f*ft+%.10f*in", feet_value, inches_value);
              
              // Calculate lengths
              size_t prefix_len = feet_start - result;
              size_t replacement_len = strlen(replacement);
              char *suffix_start = after_inches_num + 2; // skip "in"
              size_t suffix_len = strlen(suffix_start);
              
              // Allocate new string
              char *new_result = malloc(prefix_len + replacement_len + suffix_len + 1);
              memcpy(new_result, result, prefix_len);
              memcpy(new_result + prefix_len, replacement, replacement_len);
              memcpy(new_result + prefix_len + replacement_len, suffix_start, suffix_len + 1);
              
              free(result);
              result = new_result;
              p = result + prefix_len + replacement_len;
              continue;
            }
          }
        }
      }
    }
    p++;
  }
  
  return result;
}

/******************************************************************************
 * preprocess_implicit_multiplication
 *
 * Inserts '*' between numbers and unit identifiers to handle implicit multiplication
 * like "135 ft" -> "135*ft", but avoids function calls like "sin(30)".
 * Only processes known unit names to avoid false matches.
 */
char *preprocess_implicit_multiplication(const char *formula) {
  // Known unit suffixes
  static const char *units[] = {
    "m", "cm", "mm", "ft", "in",
    "pF", "nF", "uF", "mF",
    "pH", "nH", "uH", "mH",
    NULL
  };
  
  char *result = strdup(formula);
  char *p = result;
  
  while (*p) {
    // Look for digit (or end of number) followed by optional spaces, then a unit
    if (isdigit((unsigned char)*p) || *p == '-' || *p == '+' || (*p == '.' && p > result && isdigit((unsigned char)*(p-1)))) {
      // Find the end of the number
      char *num_end = p;
      while (*num_end && (isdigit((unsigned char)*num_end) || *num_end == '.' || ((*num_end == '-' || *num_end == '+') && num_end == p))) num_end++;
      
      // Skip whitespace
      char *after_num = num_end;
      while (*after_num == ' ' || *after_num == '\t') after_num++;
      
      // Check if next chars match a known unit
      if (isalpha((unsigned char)*after_num)) {
        for (int i = 0; units[i] != NULL; i++) {
          size_t unit_len = strlen(units[i]);
          if (strncasecmp(after_num, units[i], unit_len) == 0) {
            // Make sure unit is followed by non-alphanumeric (or end of string)
            char after_unit = after_num[unit_len];
            if (!isalnum((unsigned char)after_unit) && after_unit != '_') {
              // Insert '*' between number and unit
              size_t prefix_len = num_end - result;
              //size_t spaces_len = after_num - num_end;
              size_t suffix_len = strlen(after_num);
              
              char *new_result = malloc(prefix_len + 1 + suffix_len + 1); // +1 for '*'
              memcpy(new_result, result, prefix_len);
              new_result[prefix_len] = '*';
              memcpy(new_result + prefix_len + 1, after_num, suffix_len + 1);
              
              free(result);
              result = new_result;
              p = result + prefix_len + 1 + unit_len;
              goto next_iteration;
            }
          }
        }
      }
      p = num_end;
      continue;
    }
    next_iteration:
    if (*p) p++;
  }
  
  return result;
}

