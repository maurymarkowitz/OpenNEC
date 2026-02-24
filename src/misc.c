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
#include <time.h>
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

/* Returns high-resolution monotonic time in milliseconds */
void nec_get_time_ms(const nec_context_t *ctx, double *ms)
{
  struct timespec ts;
  if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
    *ms = (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1.0e6;
  } else {
    *ms = 0.0;
  }
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
 * detect_field_separator
 *
 * Examines the raw card string after the 2-char mnemonic to determine
 * the field separator style. Used during parsing to record the original
 * formatting for round-trip output.
 *
 * @param card_str the raw card string starting with the 2-char mnemonic
 * @return the detected field_sep_t style
 */
field_sep_t detect_field_separator(const char *card_str) {
  if (!card_str || strlen(card_str) <= 2) return FSEP_UNKNOWN;

  const char *p = card_str + 2; // skip 2-char mnemonic

  if (*p == '\t') return FSEP_TAB;
  if (*p == ',')  return FSEP_COMMA;
  if (*p == ' ') {
    int count = 0;
    while (*p == ' ') { count++; p++; }
    return (count >= 2) ? FSEP_COLUMN_ALIGNED : FSEP_SPACE;
  }
  return FSEP_UNKNOWN;
}

/******************************************************************************
 * preprocess_line
 *
 * Applies all 4nec2 preprocessing steps to a card line.
 * Returns a new allocated string that must be freed by the caller.
 */
char *preprocess_line(const char *line) {
  // NOTE: AWG notation (#12, 12awg) is intentionally NOT preprocessed here.
  // It is detected as a formula token in parse_geometry_or_control_card(),
  // stored as e.g. F7=#12 in card->formulas, and converted to a numeric
  // radius by preprocess_awg() during formula evaluation in update_card_values().
  // This preserves the original notation so the GUI can display and round-trip it.
  char *t1 = preprocess_feet_inches(line);
  char *t2 = preprocess_implicit_multiplication(t1);
  free(t1);
  return t2;
}

/******************************************************************************
 * convert_awg_to_meters
 *
 * convert_awg_to_meters returns the wire RADIUS in meters for a given AWG
 * gauge number. Supports standard gauges (0-40) and large wire gauges
 * (4/0 through 1/0). Large wire gauges are represented as negative values:
 * 4/0=-3, 3/0=-2, 2/0=-1, 1/0=0. Returns -1.0 for invalid gauge values.
 * Values are computed as (ASTM B258 diameter_mm / 2) / 1000.
 */
double convert_awg_to_meters(double awg_value)
{
  int awg_code = (int)floor(awg_value);

  // any decimal part is bad!
  if(awg_value != awg_code) {
    return -1.0;
  }

  // These are wire RADIUS in meters, computed as (diameter_mm / 2) / 1000.
  // Large gauge diameters are ASTM B258 standard values; AWG 1-40 use
  // the formula d_mm = 0.127 * 92^((36-n)/39).
  // NOTE: prior to this fix the table contained values 20x too large because
  // the source data (diameter in inches) was multiplied by 0.254 instead of
  // 0.0254, and was never halved to convert diameter → radius.
  switch(awg_code) {
    // Large wire gauges (negative values represent N/0 format)
    case -3: return 0.0058420;  // 4/0 or 0000
    case -2: return 0.0052025;  // 3/0 or 000
    case -1: return 0.0046328;  // 2/0 or 00
    case  0: return 0.0041258;  // 1/0 or 0
    // Standard AWG gauges
    case  1: return 0.0036741;
    case  2: return 0.0032719;
    case  3: return 0.0029137;
    case  4: return 0.0025947;
    case  5: return 0.0023106;
    case  6: return 0.0020577;
    case  7: return 0.0018324;
    case  8: return 0.0016318;
    case  9: return 0.0014532;
    case 10: return 0.0012941;
    case 11: return 0.0011524;
    case 12: return 0.0010263;
    case 13: return 0.0009139;
    case 14: return 0.0008139;
    case 15: return 0.0007248;
    case 16: return 0.0006454;
    case 17: return 0.0005748;
    case 18: return 0.0005118;
    case 19: return 0.0004558;
    case 20: return 0.0004059;
    case 21: return 0.0003615;
    case 22: return 0.0003219;
    case 23: return 0.0002867;
    case 24: return 0.0002553;
    case 25: return 0.0002273;
    case 26: return 0.0002024;
    case 27: return 0.0001803;
    case 28: return 0.0001605;
    case 29: return 0.0001430;
    case 30: return 0.0001273;
    case 31: return 0.0001134;
    case 32: return 0.0001010;
    case 33: return 0.0000899;
    case 34: return 0.0000801;
    case 35: return 0.0000713;
    case 36: return 0.0000635;
    case 37: return 0.0000565;
    case 38: return 0.0000504;
    case 39: return 0.0000448;
    case 40: return 0.0000399;
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
    // Look for "ft" pattern, ignoring case and optional spaces
    if (strncasecmp(p, "ft", 2) == 0) {
      char *ft_pos = p;
      char *slash_pos = p + 2;
      while (*slash_pos && isspace((unsigned char)*slash_pos)) slash_pos++;
      
      if (*slash_pos == '/') {
        char *after_slash = slash_pos + 1;
        while (*after_slash && isspace((unsigned char)*after_slash)) after_slash++;
        
        // Check what comes after the slash: "in" or a number
        if (strncasecmp(after_slash, "in", 2) == 0) {
          // Pattern: N ft / in -> N*ft/in
          char *feet_num_end = ft_pos;
          while (feet_num_end > result && isspace((unsigned char)*(feet_num_end-1))) feet_num_end--;
          
          char *feet_start = feet_num_end;
          while (feet_start > result && (isdigit((unsigned char)*(feet_start-1)) || *(feet_start-1) == '.' || *(feet_start-1) == '-')) feet_start--;
          
          if (feet_start < feet_num_end) {
            char *endptr;
            double feet_value = strtod(feet_start, &endptr);
            if (endptr >= feet_num_end) {
              char replacement[128];
              snprintf(replacement, sizeof(replacement), "%.10g*ft/in", feet_value);
              
              size_t prefix_len = feet_start - result;
              size_t replacement_len = strlen(replacement);
              const char *suffix_start = after_slash + 2; 
              size_t suffix_len = strlen(suffix_start);
              
              // Handle potential digit following "in"
              bool separate = isdigit((unsigned char)*suffix_start);
              
              char *new_result = malloc(prefix_len + replacement_len + suffix_len + (separate ? 1 : 0) + 1);
              memcpy(new_result, result, prefix_len);
              memcpy(new_result + prefix_len, replacement, replacement_len);
              if (separate) {
                new_result[prefix_len + replacement_len] = ' ';
                strcpy(new_result + prefix_len + replacement_len + 1, suffix_start);
              } else {
                strcpy(new_result + prefix_len + replacement_len, suffix_start);
              }
              
              free(result);
              result = new_result;
              p = result + prefix_len + replacement_len + (separate ? 1 : 0);
              continue;
            }
          }
        } else if (isdigit((unsigned char)*after_slash) || *after_slash == '.' || *after_slash == '-') {
          // Pattern: N ft / M in
          char *inches_start = after_slash;
          char *endptr;
          double inches_value = strtod(inches_start, &endptr);
          if (endptr > inches_start) {
            char *after_inches_num = endptr;
            while (*after_inches_num && isspace((unsigned char)*after_inches_num)) after_inches_num++;
            
            if (strncasecmp(after_inches_num, "in", 2) == 0) {
              char *feet_num_end = ft_pos;
              while (feet_num_end > result && isspace((unsigned char)*(feet_num_end-1))) feet_num_end--;
              
              char *feet_start = feet_num_end;
              while (feet_start > result && (isdigit((unsigned char)*(feet_start-1)) || *(feet_start-1) == '.' || *(feet_start-1) == '-')) feet_start--;
              
              if (feet_start < feet_num_end) {
                double feet_value = strtod(feet_start, &endptr);
                if (endptr >= feet_num_end) {
                  char replacement[128];
                  snprintf(replacement, sizeof(replacement), "%.10g*ft+%.10g*in", feet_value, inches_value);
                  
                  size_t prefix_len = feet_start - result;
                  size_t replacement_len = strlen(replacement);
                  const char *suffix_start = after_inches_num + 2;
                  size_t suffix_len = strlen(suffix_start);
                  
                  bool separate = isdigit((unsigned char)*suffix_start);
                  
                  char *new_result = malloc(prefix_len + replacement_len + suffix_len + (separate ? 1 : 0) + 1);
                  memcpy(new_result, result, prefix_len);
                  memcpy(new_result + prefix_len, replacement, replacement_len);
                  if (separate) {
                    new_result[prefix_len + replacement_len] = ' ';
                    strcpy(new_result + prefix_len + replacement_len + 1, suffix_start);
                  } else {
                    strcpy(new_result + prefix_len + replacement_len, suffix_start);
                  }
                  
                  free(result);
                  result = new_result;
                  p = result + prefix_len + replacement_len + (separate ? 1 : 0);
                  continue;
                }
              }
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
    "mil",
    NULL
  };
  
  // Strip outer spaces and check if the entire formula is just a unit name
  const char *trimmed = formula;
  while (*trimmed && isspace((unsigned char)*trimmed)) trimmed++;
  char *temp = strdup(trimmed);
  char *end = temp + strlen(temp) - 1;
  while (end > temp && isspace((unsigned char)*end)) *end-- = '\0';
  
  for (int i = 0; units[i] != NULL; i++) {
    if (strcasecmp(temp, units[i]) == 0) {
      char *new_result = malloc(strlen(units[i]) + 8);
      sprintf(new_result, "1.0*%s", temp);
      free(temp);
      return new_result;
    }
  }
  free(temp);

  char *result = strdup(formula);
  char *p = result;
  
  while (*p) {
    // Look for digit (or end of number)
    if (isdigit((unsigned char)*p) || (*p == '.' && p[1] != '\0' && isdigit((unsigned char)p[1]))) {
      // Find the end of the number
      char *num_end = p;
      if (*num_end == '.') num_end++;
      while (*num_end && (isdigit((unsigned char)*num_end) || *num_end == '.')) num_end++;
      
      // Check for optional spaces followed by a unit
      // We only skip spaces, not tabs, as tabs are field separators
      char *after_num = num_end;
      while (*after_num == ' ') after_num++;
      
      // Check if next chars match a known unit
      if (isalpha((unsigned char)*after_num)) {
        for (int i = 0; units[i] != NULL; i++) {
          size_t unit_len = strlen(units[i]);
          if (strncasecmp(after_num, units[i], unit_len) == 0) {
            // Make sure unit is followed by non-alphanumeric (or end of string)
            // or if it's followed by a digit, handle as a separate field
            char after_unit = after_num[unit_len];
            bool separation_needed = isdigit((unsigned char)after_unit);
            
            if (!isalnum((unsigned char)after_unit) || separation_needed || after_unit == '_') {
              // Insert '*' between number and unit
              size_t prefix_len = num_end - result;
              size_t suffix_len = strlen(after_num);
              
              char *new_result = malloc(prefix_len + 1 + suffix_len + (separation_needed ? 1 : 0) + 1); // +1 for '*'
              memcpy(new_result, result, prefix_len);
              new_result[prefix_len] = '*';
              
              if (separation_needed) {
                  // copy the unit, then a space, then the rest
                  memcpy(new_result + prefix_len + 1, after_num, unit_len);
                  new_result[prefix_len + 1 + unit_len] = ' ';
                  strcpy(new_result + prefix_len + 1 + unit_len + 1, after_num + unit_len);
              } else {
                  strcpy(new_result + prefix_len + 1, after_num);
              }
              
              free(result);
              result = new_result;
              p = result + prefix_len + 1 + unit_len + (separation_needed ? 1 : 0);
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

