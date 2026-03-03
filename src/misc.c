/******************************************************************************
 * misc.c
 *
 * Miscellaneous support functions for OpenNEC. This module provides utility
 * and helper routines used throughout the codebase, including error handling
 * and general-purpose helpers.
 *
 *****************************************************************************/

#include "internals.h"
#include "geometry.h"
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

/* Resolve path relative to source file's directory (see misc.h for docs). */
void resolve_path_relative_to_input(const char *path, const char *source_filename,
                                    char *buf, size_t bufsz)
{
  if (!path || *path == '\0') {
    buf[0] = '\0';
    return;
  }
  /* Absolute path: use as-is */
  if (path[0] == '/') {
    strncpy(buf, path, bufsz - 1);
    buf[bufsz - 1] = '\0';
    return;
  }
  /* Relative path with a source file that has a directory component */
  if (source_filename && *source_filename != '\0') {
    const char *slash = strrchr(source_filename, '/');
    if (slash) {
      size_t dir_len = (size_t)(slash - source_filename) + 1; /* include trailing / */
      if (dir_len >= bufsz) dir_len = bufsz - 1;
      strncpy(buf, source_filename, dir_len);
      buf[dir_len] = '\0';
      strncat(buf, path, bufsz - dir_len - 1);
      return;
    }
  }
  /* No directory component or no source: use path relative to CWD */
  strncpy(buf, path, bufsz - 1);
  buf[bufsz - 1] = '\0';
}

/* Transfer already-logged errors from src into dst without re-emitting them via nec_report. */
void transfer_errors(errors_list_t *src, errors_list_t *dst)
{
  for (int i = 0; i < src->num_errors; i++) {
    error_t *e = &src->errors[i];
    error_t copy;
    copy.severity = e->severity;
    copy.message = calloc(strlen(e->message) + 1, sizeof(char));
    strcpy(copy.message, e->message);
    if (dst->num_errors == 0)
      dst->errors = calloc(1, sizeof(error_t));
    else
      dst->errors = realloc(dst->errors, (dst->num_errors + 1) * sizeof(error_t));
    dst->errors[dst->num_errors] = copy;
    dst->num_errors++;
  }
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

	case -6: /* No convergence in shanks_integration() */
	  nec_report(ctx, ONEC_SEV_FATAL, "onec: No convergence in shanks_integration() - aborting");
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

    // Scan past the first field token to check if the inter-field separator
    // is a comma.  This catches "GW 1,299,..." style (space after mnemonic,
    // commas between fields) which is common in hand-edited NEC files.
    const char *q = p;
    while (*q && *q != ',' && *q != ' ' && *q != '\t' && *q != '\0') q++;
    if (*q == ',') return FSEP_SPACE_COMMA;

    return (count >= 2) ? FSEP_COLUMN_ALIGNED : FSEP_SPACE;
  }
  return FSEP_UNKNOWN;
}



/******************************************************************************
 * convert_awg_to_meters
 *
 * convert_awg_to_meters returns the wire *radius* in meters for a given AWG
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

  // These are wire *RADIUS* in meters, computed as (diameter_mm / 2) / 1000.
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

/*------------------------------------------------------------------------*/

/******************************************************************************
 * nec_estimate_setup  (private)
 *
 * Scans the parsed deck to extract the scalar parameters needed by the
 * NEC-2 Part III performance formula.  No geometry computation is performed.
 *
 * @param deck    Parsed deck (symbols evaluated, geometry not yet built)
 * @param ns      OUT: total wire segments in one symmetry sector
 * @param np      OUT: total surface patches in one symmetry sector
 * @param nf      OUT: total radiation-pattern evaluation points (NTH×NPH)
 * @param m_sym   OUT: symmetry multiplier from GR/GX (1 if none)
 * @param k_gnd   OUT: ground complexity factor (1=none, 2=approx/perfect, 4=Sommerfeld)
 * @param nfreq   OUT: number of frequencies from FR card (1 if absent)
 */
static void nec_estimate_setup(const deck_t *deck,
    int *ns, int *np, int *nf, int *m_sym, int *k_gnd, int *nfreq)
{
  *ns    = 0;
  *np    = 0;
  *nf    = 0;
  *m_sym = 1;
  *k_gnd = 2;  /* default: approximate ground, k=2 */
  *nfreq = 1;  /* default: single frequency */

  if (!deck || !deck->cards) return;

  /* ---- geometry pass ---- */
  for (int i = deck->geometry_start; i <= deck->geometry_end; i++) {
    const card_t *c = &deck->cards[i];
    if (c->ignore) continue;
    const char *code = c->card_code;

    if (strcmp(code, "GW") == 0 ||
        strcmp(code, "GA") == 0 ||
        strcmp(code, "GH") == 0) {
      /* i[2] = I2 = number of segments */
      *ns += c->i[2];
    }
    else if (strcmp(code, "SP") == 0) {
      /* Each SP card (any shape flag) contributes 1 patch */
      *np += 1;
    }
    else if (strcmp(code, "SM") == 0) {
      /* i[1]=I1 = patches along first axis, i[2]=I2 = patches along second */
      int m = c->i[1], n = c->i[2];
      if (m > 0 && n > 0) *np += m * n;
    }
    else if (strcmp(code, "GR") == 0) {
      /* i[2] = number of rotational sectors */
      int sectors = c->i[2];
      if (sectors > 1) *m_sym *= sectors;
    }
    else if (strcmp(code, "GX") == 0) {
      /* i[2] = three-digit bitmask: hundreds=x, tens=y, units=z */
      int bitmask = c->i[2];
      int ix = (bitmask / 100) % 10 ? 1 : 0;
      int iy = (bitmask / 10)  % 10 ? 1 : 0;
      int iz =  bitmask        % 10 ? 1 : 0;
      *m_sym *= (1 << (ix + iy + iz));
    }
  }

  /* ---- control pass (cards after GE up to EN) ---- */
  int ctrl_start = deck->geometry_end + 1;
  int ctrl_end   = (deck->deck_end >= 0) ? deck->deck_end : deck->num_cards - 1;

  for (int i = ctrl_start; i <= ctrl_end; i++) {
    const card_t *c = &deck->cards[i];
    if (c->ignore) continue;
    const char *code = c->card_code;

    if (strcmp(code, "GN") == 0) {
      int iperf = c->i[1];
      if      (iperf == -1) *k_gnd = 1;  /* no ground */
      else if (iperf ==  2) *k_gnd = 4;  /* Sommerfeld */
      else                  *k_gnd = 2;  /* perfect / approximate */
    }
    else if (strcmp(code, "FR") == 0) {
      /* i[2] = I2 = NFRQ; 0 on card means 1 frequency */
      int n = (c->i[2] > 0) ? c->i[2] : 1;
      /* Take the maximum across multiple FR cards (conservative) */
      if (n > *nfreq) *nfreq = n;
    }
    else if (strcmp(code, "RP") == 0) {
      int nth = (c->i[2] > 0) ? c->i[2] : 1;
      int nph = (c->i[3] > 0) ? c->i[3] : 1;
      *nf += nth * nph;
    }
  }
}

/*------------------------------------------------------------------------*/

/******************************************************************************
 * nec_estimate_time
 *
 * Returns a dimensionless complexity value T proportional to the expected
 * run time, based on the NEC-2 Part III performance formula:
 *
 *   T  =  Nfreq * (T1 + T2 + T3 + T4)
 *
 *   T1 = k * (Ns^2 + Ns*Np) / M         (impedance matrix fill)
 *   T2 = (Ns + 2*Np)^3 / M^2            (matrix factorisation — dominates)
 *   T3 = (Ns + 2*Np)^2 / M              (back-substitution for currents)
 *   T4 = k * Nf * (Ns + 2*Np)           (far-field summation)
 *
 * where:
 *   Ns    = wire segments in one symmetry sector
 *   Np    = surface patches in one symmetry sector
 *   M     = symmetry multiplier from GR / GX (1 if no symmetry)
 *   k     = ground complexity (1 = no ground, 2 = approx/perfect, 4 = Sommerfeld)
 *   Nf    = total radiation-pattern points (NTH * NPH from RP card)
 *   Nfreq = number of frequencies from FR card (1 if absent)
 *
 * All coefficients are 1 (unit coefficients).  T is not in seconds; it is
 * a dimensionless complexity number.  A GUI applies a platform-calibrated
 * threshold to classify a run as "fast" or "slow".
 *
 * Design notes:
 *   - T4 = 0 when no RP card is present.  No worst-case Nf is assumed.
 *   - The Nc (wire-to-surface junction) correction term is omitted because
 *     Nc cannot be determined from deck cards alone without running the full
 *     geometry engine.
 *   - The NEC-2 impedance matrix is frequency-dependent, so all four terms
 *     (fill, factorisation, solve, far-field) repeat for each frequency.
 *     Nfreq therefore multiplies the entire T, not just T4.
 *
 * @param  deck   Parsed deck (symbols evaluated; calculate_geometry() need
 *                NOT have been called)
 * @return        Dimensionless complexity estimate T >= 0.0, or 0.0 if the
 *                deck pointer is NULL.
 */
double nec_estimate_time(nec_context_t *ctx, deck_t *deck)
{
  if (!ctx || !deck) return 0.0;

  /* Ensure geometry is expanded so we get the post-GM segment count.
   * calculate_geometry() is coordinate math only — no matrix work.
   * If it was already called (e.g. by a preceding nec_run_simulation,
   * or by a GUI that built the geometry view), we reuse the result. */
  if (ctx->geometry.num_segs == 0 && ctx->geometry.num_patches == 0) {
    errors_list_t tmp_errs = {0};
    calculate_geometry(ctx, deck, &tmp_errs, &ctx->outputs);
    for (int i = 0; i < tmp_errs.num_errors; i++) free(tmp_errs.errors[i].message);
    free(tmp_errs.errors);
  }

  /* Scan control cards for FR / RP / GN — deck-scan is still needed for
   * these because they live outside the geometry section. The geometry
   * portion of nec_estimate_setup is now superseded by ctx->geometry. */
  int ns_scan, np_scan, nf, m_sym_scan, k_gnd, nfreq;
  nec_estimate_setup(deck, &ns_scan, &np_scan, &nf, &m_sym_scan, &k_gnd, &nfreq);

  /* Derive ns / np / m_sym from the expanded geometry.
   *  ctx->geometry.num_segs_sym  = segments in one symmetry cell (< .n when GR is used)
   *  ctx->geometry.num_patches_sym  = patches   in one symmetry cell
   *  m_sym             = n / np  (>1 for GR; ==1 after GM expansion) */
  int ns, np, m_sym;
  if (ctx->geometry.num_segs > 0 || ctx->geometry.num_patches > 0) {
    ns    = ctx->geometry.num_segs_sym;
    np    = ctx->geometry.num_patches_sym;
    m_sym = (ctx->geometry.num_segs_sym > 0) ? ctx->geometry.num_segs / ctx->geometry.num_segs_sym : 1;
  } else {
    /* Empty or failed geometry — fall back to deck-scan values. */
    ns    = ns_scan;
    np    = np_scan;
    m_sym = m_sym_scan;
  }

  if (m_sym  <= 0) m_sym  = 1;
  if (nfreq  <= 0) nfreq  = 1;

  double N     = (double)(ns + 2 * np);
  double M     = (double)m_sym;
  double k     = (double)k_gnd;
  double Ns    = (double)ns;
  double Np    = (double)np;
  double Nf    = (double)nf;
  double Nfreq = (double)nfreq;

  double T1 = k * (Ns * Ns + Ns * Np) / M;
  double T2 = (N * N * N) / (M * M);
  double T3 = (N * N) / M;
  double T4 = (nf > 0) ? k * Nf * N : 0.0;

  return Nfreq * (T1 + T2 + T3 + T4);
}

