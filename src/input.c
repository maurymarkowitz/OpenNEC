/******************************************************************************
 * input.c
 *
 * input.c contains the routines needed to read and parse a card deck file,
 * normally ".nec" or ".deck". The process starts with read_deck(), which uses
 * read_line() to get the data from one line in the file and then turns it
 * it into a card. When all the lines are read into cards, read_deck()
 * then calls a series of functions to parse the deck into parts, using
 * the logic found in parse_command_card(), parse_geometry_card(), etc.
 *
 * The original nec2c, like the NEC2 code it was based on, is very "modal".
 * It reads a card, processes it, and then forgets it. Any cards that cannot
 * be processed, like comments, are skipped entirely. In contrast, OpenNEC
 * reads the entire deck into memory first. This allows it to easily perform
 * whole-deck checks, like looking for a missing EN, or a GW that's missing
 * its GC. To do this, OpenNEC also keeps every card it finds, even blank
 * lines and lines that contain only a comment. Keeping all of these also
 * makes it much easier to use as an editor, whereas NEC2 was designed solely
 * to read the deck and run it.
 *
 * Other changes to this code include a wider set of comment markers
 * including CM, !, # and ', whereas nec2c only accepted # outside the
 * comment header. Additionally, this code looks for comment markers *in*
 * a line, and splits that data out to a separate buffer for processing out
 * the (potential) OpenNEC extensions. It keeps track of what the original
 * comment marker was so it can save it back out in the same format.
 * 
 * NOTE: due the the # also being used as the marker for AWG wire gauges,
 *       the # character is only treated as a comment marker if it is the first
 *       non-whitespace character on the line. Elsewhere in the line it is treated as normal
 *       text.
 *
 *****************************************************************************/

#include "internals.h"
#include "input.h"

/* Forward declarations for internal functions */
static int read_line(context_t *ctx, char *buff, FILE *pfile, int line_num);
static void parse_comment_card(context_t *ctx, card_t *card, errors_list_t *errors);
static void parse_geometry_or_control_card(context_t *ctx, card_t *card, errors_list_t *errors);
static void parse_onec_card(context_t *ctx, card_t *card, errors_list_t *errors);
static void parse_key_values(context_t *ctx, card_t *card, errors_list_t *errors);

/* Module-level variables for line ending detection */
static int line_ending_crlf_count = 0;
static int line_ending_lf_count = 0;

/******************************************************************************
 * read_deck()
 *
 * Reads the entire deck line by line and fills out the deck's cards[]
 * array with the resulting data. After this completes, the caller
 * should call parse_deck() to process the data.
 *
 * @param deck deck_t structure that will hold the Cards
 * @param pfile file pointer to the file to be read, assumed
 *  to have been opened previous to this call
 *
 */
void read_deck(context_t *ctx, deck_t *deck, FILE *pfile)
{
  char line_buf[1024];  // make it large enough to hold any line
  size_t line_len;      // actual length of the current card being read
    
  // reset line ending detection for new read
  line_ending_crlf_count = 0;
  line_ending_lf_count = 0;
  // and default to unknown
  deck->line_endings = LINE_ENDING_UNDETERMINED;

  // set the card count to 0, it might have !=0 default
  deck->num_cards = 0;

  // and the symbols...
  deck->num_symbols = 0;
  
  // and set the default comment markers to empty
  deck->extn_code = 0;
  deck->cmt_code = 0;

  // loop and read lines one-by-one until we hit the EOF
  int line_num = 0;
  int last_line_nonempty = 0;
  do {
    line_num++;
    int read_result = read_line(ctx, line_buf, pfile, line_num);
    if(read_result == EOF) {
      if (strlen(line_buf) > 0) {
        last_line_nonempty = 1;
      } else {
        break;
      }
    }

    // Handle hard-newline continuation lines (e.g. 4nec2 split fields)
    // by appending numeric continuation lines to the previous card.
    size_t first_nonws = 0;
    while (line_buf[first_nonws] && isspace((unsigned char)line_buf[first_nonws])) {
      first_nonws++;
    }
    bool continuation_line = false;
    if (deck->num_cards > 0 && first_nonws < strlen(line_buf)) {
      char c = line_buf[first_nonws];
      if (c == '+' || c == '-' || c == '.' || isdigit((unsigned char)c)) {
        continuation_line = true;
      }
    }
    if (continuation_line) {
      card_t *prev_card = &deck->cards[deck->num_cards - 1];
      size_t prev_len = strlen(prev_card->orig_str);
      size_t add_len = strlen(line_buf);
      char *new_str = realloc(prev_card->orig_str, prev_len + 1 + add_len + 1);
      if (!new_str) {
        char msg[MAX_ERROR_LEN];
        snprintf(msg, MAX_ERROR_LEN, "[read_deck] ERROR: realloc failed for continuation line %d", line_num);
        add_error(ctx, &ctx->errors, msg, FATAL);
        return;
      }
      prev_card->orig_str = new_str;
      prev_card->orig_str[prev_len] = ' ';
      memcpy(prev_card->orig_str + prev_len + 1, line_buf, add_len + 1);
      // Do not add a new card for continuation lines.
      if (read_result == EOF && !last_line_nonempty) {
        break;
      }
      last_line_nonempty = 0;
      continue;
    }

    line_len = strlen(line_buf);
    if(deck->num_cards == 0) {
      deck->num_cards++;
      deck->cards = calloc(1, sizeof(card_t));
      if (!deck->cards) {
        char msg[MAX_ERROR_LEN];
        snprintf(msg, MAX_ERROR_LEN, "[read_deck] ERROR: calloc failed for deck->cards at line %d", line_num);
        add_error(ctx, &ctx->errors, msg, FATAL);
        return;
      }
    } else {
      deck->num_cards++;
      card_t *new_cards = realloc(deck->cards, deck->num_cards * sizeof(card_t));
      if (!new_cards) {
        char msg[MAX_ERROR_LEN];
        snprintf(msg, MAX_ERROR_LEN, "[read_deck] ERROR: realloc failed for deck->cards at line %d", line_num);
        add_error(ctx, &ctx->errors, msg, FATAL);
        return;
      }
      deck->cards = new_cards;
    }
    card_t *dest = &deck->cards[deck->num_cards - 1];
    *dest = (card_t){
      .edited = false,
      .ignore = false,
      .card_num = line_num
    };

    dest->orig_str = calloc(line_len + 1, sizeof(char));
    if (!dest->orig_str) {
      char msg[MAX_ERROR_LEN];
      snprintf(msg, MAX_ERROR_LEN, "[read_deck] ERROR: calloc failed for card->orig_str at line %d", line_num);
      add_error(ctx, &ctx->errors, msg, FATAL);
      return;
    }
    /* Safe: allocate size is line_len + 1, we copy line_len bytes + null */
    memcpy(dest->orig_str, line_buf, line_len);
    dest->orig_str[line_len] = '\0';
    if (read_result == EOF && !last_line_nonempty) {
      break;
    }
    last_line_nonempty = 0;
  } while(true);
  // do not free(card) here; its contents are now owned by deck->cards[]
  
  // detect and store the line ending style used in the file
   if(line_ending_crlf_count == 0 && line_ending_lf_count == 0) {
    deck->line_endings = LINE_ENDING_UNDETERMINED;
  } else if(line_ending_crlf_count > line_ending_lf_count) {
    deck->line_endings = LINE_ENDING_CRLF;
  } else if(line_ending_lf_count > line_ending_crlf_count) {
    deck->line_endings = LINE_ENDING_LF;
  } else {
    // equal counts - shouldn't happen unless exactly one of each.
    // default to CRLF in that case.
    deck->line_endings = LINE_ENDING_CRLF;
  }
} /* end read_deck */

/******************************************************************************
 * read_line()
 *
 * reads a line from a file, aborts on failure
 *
 * Formerly known as "load_line", this version does not skip empty
 * lines or those that are pure comments. OpenNEC wants to save these
 * to separate cards so that it can perform whole-stack syntax checking
 * and similar tests.
 *
 * You might expect this to use C's built-in fgets or getline or similar.
 * It doesn't because of the infrequent but seen-in-the-wild problem of
 * hard returns being inserted in the middle of lines when users edit
 * their decks using some text editors. This code reads until it has
 * seen the required number of fields even if they cross multiple lines,
 * thereby fixing such damage by merging the lines back together.
 *
 * This means that if one simply loads and then saves the deck, the
 * split lines will be removed and thus "fixed".
 *
 * One lingering problem is when such a break has been inserted precisely
 * where an inline comment occurs. This causes the parser to consider the
 * second line to be a separate comment card, instead of a comment on the
 * previous card. This is a TODO, which might be resolved by looking for
 * onec key/value pairs - if such are found it can be assumed to be a
 * comment on the previous line, if none are found then it is *likely* to
 * be a separate comment line.
 *
 * This code also automatically capitalizes the first two characters
 * on the line, regardless of how they were entered originally.
 *
 * @param ctx The current context_t, used for error reporting
 * @param buff String containing the contents of one line
 * @param file file pointer to the file to be read, assumed
 * @param line_num The current line number in the file, for error reporting
 *  to have been opened previous to this call
 * 
*/
int read_line(context_t *ctx, char *buff, FILE *pfile, int line_num)
{
  int
    num_chr = 0,  // number of characters read, excluding lf/cr
    eof = 0,      // EOF flag
    chr;          // character read by getc
  
  // clear buffer
  buff[0] = '\0';
  
  // if we're at the end of the file, return that, we're done
  if((chr = getc(pfile)) == EOF) {
    return(EOF);
  }

  // the line parser below stops and returns as soon as it sees a single
  // cr or lf. That means that when we re-enter the routine, the file might
  // have leading cr's or lf's left over. OpenNEC now preserves blank lines
  // as separate cards for editor functionality, so return them as empty strings
  // rather than skipping them entirely.
  if((chr == CR) || (chr == LF)) {
    // Track line endings: check if this is CRLF or just LF
    if(chr == CR) {
      // Might be CRLF - peek at next char
      chr = getc(pfile);
      if(chr == LF) {
        line_ending_crlf_count++;
      } else {
        line_ending_lf_count++;  // Just CR, treat as LF for counting
        if(chr != EOF) ungetc(chr, pfile);
      }
    } else {
      // Just LF
      line_ending_lf_count++;
    }
    // This is a blank line. Return it as an empty string so read_deck
    // can create a blank card. Don't skip multiple newlines here - let
    // the next call handle them so each blank line is a separate card.
    buff[0] = '\0';  // Return empty string for blank line
    return(0);       // Success: blank line
  }

  // the line parser below stops and returns as soon as it sees a single
  // cr or lf. That means that when we re-enter the routine, the file might
  // have been left with leading content. Just continue reading the line normally.

  // read the line until you pick up any trailing cr's or lfs.
  while(num_chr < MAX_LINE_LEN - 1) {
    // if lf/cr reached before filling buffer, exit
    if((chr == CR) || (chr == LF))
      break;

    // enter new char to buffer
    buff[num_chr++] = (char)chr;

    // if we get the EOF, end the string at that point by replacing it
    // with a null and then setting the flag that we're done
    if((chr = getc(pfile)) == EOF) {
      buff[num_chr] = '\0';
      eof = EOF;
      break;
    }
  } /* end of while( num_chr < MAX_LINE_LEN - 1 ) */
  
  // Track line ending: if we exited due to CR/LF, record which type(s)
  if(eof != EOF && (chr == CR || chr == LF)) {
    if(chr == CR) {
      // Might be CRLF - peek at next char
      int next = getc(pfile);
      if(next == LF) {
        line_ending_crlf_count++;
      } else {
        line_ending_lf_count++;  // Just CR, treat as LF
        if(next != EOF) ungetc(next, pfile);
      }
    } else {
      // Just LF
      line_ending_lf_count++;
    }
  }
  
  // If we exited the loop because the buffer is full (not because of CR/LF or EOF),
  // we need to consume any remaining characters on this line so they don't become
  // part of the next line. Also check if any are non-whitespace.
  if(num_chr >= MAX_LINE_LEN - 1 && chr != CR && chr != LF && eof != EOF) {
    bool found_nonws = false;
    // Continue reading to end of line, checking for non-whitespace
    while((chr = getc(pfile)) != EOF && chr != CR && chr != LF) {
      if(!isspace((unsigned char)chr)) {
        found_nonws = true;
      }
    }
    // Report if we found non-whitespace beyond the limit
    if(found_nonws) {
      char msg[MAX_ERROR_LEN];
      snprintf(msg, MAX_ERROR_LEN, "The card on line %d has non-whitespace characters beyond %d characters, these have been removed.", 
               line_num, MAX_LINE_LEN);
      add_error(ctx, &ctx->errors, msg, WARNING);
    }
  }
  
  // terminate buffer as a string
  buff[num_chr] = '\0';
  
  return(eof);
} /* end of read_line() */

/******************************************************************************
 * parse_deck()
 *
 * parses the original data from the file once it's all read in by read_deck
 *
 * this starts by extracting the code from the front of the orig_str and
 * then looking for and clipping off any comment or extensions at the end
 * of the line. When this process is complete, the card_str and extn_str
 * are filled out and ready to process.
 *
 * After that, further processing functions are used to extract the
 * numeric values from the card_str and key-value pairs from the
 * extn_str.
 *
 */
void parse_deck(context_t *ctx, deck_t *deck, errors_list_t *errors)
{
  card_t *card;
  
  size_t line_len; // length of the original string for this card
  char type_buff[3];
  char hidden_type_buff[3];
  bool isCmt, isGeo, isCtl, isExt; // cache these so we can do the string compare only once
  bool sawCM = false, sawCE = false, sawSY = false, sawGx = false, sawGE = false, sawEN = false; // keep track of where we are

  // First pass: merge comment-only lines with onec extensions into the previous non-comment line
  for(int i = 1; i < deck->num_cards; i++) {
    card_t *prev_card = &deck->cards[i-1];
    card_t *curr_card = &deck->cards[i];
    
    // Check if current card is entirely a comment
    line_len = strlen(curr_card->orig_str);
    size_t first = 0;
    while (first < line_len && isspace((unsigned char)curr_card->orig_str[first])) first++;
    
    bool curr_is_comment = false;
    if (first < line_len) {
      if ((toupper(curr_card->orig_str[first]) == 'C' && toupper(curr_card->orig_str[first+1]) == 'M') ||
          (toupper(curr_card->orig_str[first]) == 'C' && toupper(curr_card->orig_str[first+1]) == 'E') ||
          curr_card->orig_str[first] == '!' ||
          curr_card->orig_str[first] == '#' ||
          curr_card->orig_str[first] == '\'') {
        curr_is_comment = true;
      }
    }
    
    // Check if previous card is not a comment and doesn't have a trailing comment
    line_len = strlen(prev_card->orig_str);
    first = 0;
    while (first < line_len && isspace((unsigned char)prev_card->orig_str[first])) first++;
    
    bool prev_is_comment = false;
    if (first < line_len) {
      if ((toupper(prev_card->orig_str[first]) == 'C' && toupper(prev_card->orig_str[first+1]) == 'M') ||
          (toupper(prev_card->orig_str[first]) == 'C' && toupper(prev_card->orig_str[first+1]) == 'E') ||
          prev_card->orig_str[first] == '!' ||
          prev_card->orig_str[first] == '#' ||
          prev_card->orig_str[first] == '\'') {
        prev_is_comment = true;
      }
    }
    
    // Check if current comment line has onec key/values
    bool has_onec_extensions = false;
    if (curr_is_comment) {
      // Parse the comment to see if it has extensions
      char temp_str[MAX_LINE_LEN];
      size_t orig_len = strlen(curr_card->orig_str);
      if (orig_len >= MAX_LINE_LEN) orig_len = MAX_LINE_LEN - 1;
      memcpy(temp_str, curr_card->orig_str, orig_len);
      temp_str[orig_len] = '\0';
      
      // Find the comment part (after the marker)
      char *comment_start = NULL;
      if (strstr(temp_str, "CM") == temp_str || strstr(temp_str, "CE") == temp_str) {
        comment_start = temp_str + 2;
      } else if (temp_str[0] == '!' || temp_str[0] == '#' || temp_str[0] == '\'') {
        comment_start = temp_str + 1;
      }
      
      if (comment_start && strstr(comment_start, "onec:")) {
        has_onec_extensions = true;
      }
    }
    
    // If conditions are met, merge the comment
    if (curr_is_comment && has_onec_extensions && !prev_is_comment && prev_card->comment == NULL) {
      // Add error message
      char msg[MAX_ERROR_LEN];
      snprintf(msg, MAX_ERROR_LEN, "Comment line on card %d contains onec extensions and is being merged into line %d", i+1, i);
      add_error(ctx, errors, msg, 0);
      
      // copy the comment from current card to previous card
      if (curr_card->comment) {
        prev_card->comment = strdup(curr_card->comment);
      } else {
        // Extract comment from the line
        char *comment_start = NULL;
        if (strstr(curr_card->orig_str, "CM") == curr_card->orig_str || 
            strstr(curr_card->orig_str, "CE") == curr_card->orig_str) {
          comment_start = curr_card->orig_str + 2;
        } else if (curr_card->orig_str[0] == '!' || curr_card->orig_str[0] == '#' || 
                   curr_card->orig_str[0] == '\'') {
          comment_start = curr_card->orig_str + 1;
        }
        if (comment_start) {
          // Skip leading whitespace
          while (*comment_start && isspace((unsigned char)*comment_start)) comment_start++;
          if (*comment_start) {
            prev_card->comment = strdup(comment_start);
          }
        }
      }
      
      // Remove the current card by shifting all subsequent cards
      for (int j = i; j < deck->num_cards - 1; j++) {
        deck->cards[j] = deck->cards[j + 1];
      }
      deck->num_cards--;
      i--; // Adjust index since we removed a card
    }
  }

  // initialize section markers that won't be set if their cards are absent
  deck->symbol_start = -1;
  deck->symbol_end   = -1;

  for(int i = 0; i < deck->num_cards; i++) {
    card = &deck->cards[i];
    // get the card and the original string length
    line_len = strlen(card->orig_str);
    // for hidden (commented-out) cards, card_str starts past the leading marker
    size_t card_str_offset = 0;
    
    // Skip leading whitespace
    size_t first = 0;
    while (first < line_len && isspace((unsigned char)card->orig_str[first])) first++;

    // If the line is empty or entirely whitespace, mark as comment
    if (first >= line_len) {
      strcpy(type_buff, "!");
      strncpy(card->card_code, type_buff, sizeof(card->card_code));
    }
    // Check for comment markers (CM, CE, !, #, ')
    else if (toupper(card->orig_str[first]) == 'C' && first + 1 < line_len && toupper(card->orig_str[first+1]) == 'M') {
      strcpy(type_buff, "CM");
      strncpy(card->card_code, type_buff, sizeof(card->card_code));
    } else if (toupper(card->orig_str[first]) == 'C' && first + 1 < line_len && toupper(card->orig_str[first+1]) == 'E') {
      strcpy(type_buff, "CE");
      strncpy(card->card_code, type_buff, sizeof(card->card_code));
    } else if (card->orig_str[first] == '!') {
      strcpy(type_buff, "!");
      strncpy(card->card_code, type_buff, sizeof(card->card_code));
    } else if (card->orig_str[first] == '#') {
      strcpy(type_buff, "#");
      strncpy(card->card_code, type_buff, sizeof(card->card_code));
    } else if (card->orig_str[first] == '\'') {
      strcpy(type_buff, "'");
      strncpy(card->card_code, type_buff, sizeof(card->card_code));
    } else {
      // not a comment marker, extract up to 2 chars for card code
      if (line_len - first == 1) {
        type_buff[0] = toupper(card->orig_str[first]);
        type_buff[1] = '\0';
      } else {
        type_buff[0] = toupper(card->orig_str[first]);
        type_buff[1] = toupper(card->orig_str[first+1]);
        type_buff[2] = '\0';
        if (isspace(type_buff[1])) type_buff[1] = '\0';
      }
      strncpy(card->card_code, type_buff, sizeof(card->card_code));
    }
    
    // see if we can find out what sort of card it is
    isCmt = is_comment(card);
    isGeo = is_geometry(card);
    isCtl = is_control(card);
    isExt = is_extension(card);

    // while we loop, we want to keep track of key points in the deck
    // which will make it easier to work with in other parts of the code.
    // for instance, we need to know where the EN card is, if we find
    // one, because anything after that is automatically a comment. it's
    // also handy to know where the geometry and comments sections
    // start and end.
    //
    // note that we can't simply update the pointers every time we see
    // one of the cards in a certain set, because you might have a CM
    // card outside the comment section, for instance. This is not the
    // case in a "real" NEC2 file, but widely allowed by practically
    // every system.
    //
    // so, for instance, if this is the first CM card we've seen, and we
    // have NOT seen a CE or any Gx card, then this would be the start
    // of the comment section. But if we saw a GW or something, then it's
    // just a comment in the deck and not part of the header.
    if(strcmp(type_buff, "CM") == 0 && !sawCM && !sawCE && !sawGx && !sawGE && !sawEN) {
      deck->comment_start = i;
      sawCM = true;
    }
    // the CE case is similar, it has to be above any geometry
    if(strcmp(type_buff, "CE") == 0 && !sawCE && !sawGx && !sawGE && !sawEN) {
      deck->comment_end = i;
      sawCE = true;
    }
    // SY (symbol) cards between the comment section and the first geometry card
    if(strcmp(type_buff, "SY") == 0 && !sawGx && !sawGE && !sawEN && !card->ignore) {
      if(!sawSY) {
        deck->symbol_start = i;
        sawSY = true;
      }
      deck->symbol_end = i;
    }
    // if this is the first geo card, including GE (but not commented-out cards)...
    if(isGeo && !sawGx && !sawEN && !card->ignore) {
      deck->geometry_start = i;
      sawGx = true;
    }
    // the GE only has to be above the end of the deck
    if(strcmp(type_buff, "GE") == 0 && !sawGE && !sawEN && !card->ignore) {
      deck->geometry_end = i;
      sawGE = true;
    }
    // and finally, the first EN that we see
    if(strcmp(type_buff, "EN") == 0) {
      deck->deck_end = i;
      sawEN = true;
    }
    
    // another special case: if this is a single-character comment marker (!, ', #)
    // followed by a valid NEC or OpenNEC card code, this is a "hidden" (commented-out)
    // card. Parse it normally and set ignore=true so it is skipped during
    // calculation, but fully available to the GUI for single-click toggling.
    // Note: CM/CE header lines are deliberately excluded — they are comment headers,
    // not commented-out cards.
    if(isCmt && line_len > 3 &&
       (strcmp(type_buff, "!") == 0 || strcmp(type_buff, "#") == 0 || strcmp(type_buff, "'") == 0)) {
      // advance pos past the single-character comment marker
      size_t pos = 1;
      // eat one whitespace character if present (handles both !GW and ! GW)
      if(pos < line_len && isspace((unsigned char)card->orig_str[pos])) {
        pos++;
      }
      
      // extract and uppercase the two characters at pos as a potential card code.
      // Also require that a valid separator (whitespace, comma, or end-of-string)
      // follows the two-character code — this prevents a plain comment line like
      // "' Except for..." from being mis-detected as a hidden EX card just because
      // its first two significant letters happen to match a known card mnemonic.
      if(pos + 1 < line_len) {
        hidden_type_buff[0] = toupper((unsigned char)card->orig_str[pos]);
        hidden_type_buff[1] = toupper((unsigned char)card->orig_str[pos + 1]);
        hidden_type_buff[2] = '\0';
        // verify the character after the 2-char code is a separator or end-of-string
        size_t after = pos + 2;
        if(after < line_len && !isspace((unsigned char)card->orig_str[after]) && card->orig_str[after] != ',') {
          hidden_type_buff[0] = '\0'; // not a valid card code boundary — treat as plain comment
        }
      } else {
        hidden_type_buff[0] = '\0';
      }
      
      // check all three code arrays: OpenNEC extensions, geometry, and control
      bool isHidden = false;
      bool hiddenIsExt = false, hiddenIsGeo = false, hiddenIsCtl = false;
      for(int j = 0; j < NUM_ONEC_CODES && !isHidden; j++) {
        if(strcmp(hidden_type_buff, onec_codes[j]) == 0) { isHidden = true; hiddenIsExt = true; }
      }
      for(int j = 0; j < NUM_GEOMETRY_CODES && !isHidden; j++) {
        if(strcmp(hidden_type_buff, geometry_codes[j]) == 0) { isHidden = true; hiddenIsGeo = true; }
      }
      for(int j = 0; j < NUM_CONTROL_CODES && !isHidden; j++) {
        if(strcmp(hidden_type_buff, control_codes[j]) == 0) { isHidden = true; hiddenIsCtl = true; }
      }
      
      if(isHidden) {
        // set card_code to the real code, not the comment marker
        strncpy(card->card_code, hidden_type_buff, sizeof(card->card_code));
        // save the leading comment marker character
        card->cmt_code[0] = card->orig_str[first];
        // mark as ignored (commented out); processing will skip it
        card->ignore = true;
        // update flags
        isCmt = false;
        isExt = hiddenIsExt;
        isGeo = hiddenIsGeo;
        isCtl = hiddenIsCtl;
        // card_str starts at the actual code position, not the comment marker
        card_str_offset = pos;
      }
    } // checking for hidden info
    
    // if we're past the end of the deck, everything that appears is a comment,
    // and we'll just copy it into the comment string. but if we are not past
    // the end, and we didn't recognize the code then we want to report
    // an error
    if (!sawEN && !isCmt && !isCtl && !isGeo && !isExt) {
      char msg[MAX_ERROR_LEN];
      snprintf(msg, MAX_ERROR_LEN, "The card on line %d has unknown type '%s'. Card skipped.", i+1, type_buff);
      add_error(ctx, errors, msg, PROBLEM);
    }
    
    // if we did figure out the card type, then we want to put something in the card_str,
    // but first we want to see if there is a comment inside the line ( > 0, < len ) and
    // clip that part out separately into extn_str.
    // For hidden (commented-out) cards, card_str_offset skips past the leading marker
    // so that card_str starts with the actual card code (e.g. "GW", "GN").
    if(isCmt || isCtl || isGeo || isExt) {
      size_t len; // this is the length of the main card text
      const char *base = card->orig_str + card_str_offset;
      // look for an inline comment marker, adjust length of card text based on that
      const char *sep = strpbrk(base, ONEC_COMMENTS);
      if(sep == NULL) {
        // no comment was found, put everything into the string
        len = strlen(base);
      } else {
        if (isCmt && sep == base) {
          // for comment cards that start with comment marker, include the whole line
          len = strlen(base);
        } else {
          len = sep - base;
        }
      }
      // malloc room for the card part, copy that in, and close the string
      card->card_str = (char *)malloc((len * sizeof(char)) + 1);
      memcpy(card->card_str, base, len);
      card->card_str[len] = '\0';

      // and if there was any leftover, copy it into the extension, otherwise make sure its empty
      if(sep == NULL || (isCmt && sep == base)) {
        card->extn_str = NULL;
        card->extn_code[0] = '\0';
      } else {
        card->extn_str = strdup(sep + 1);
        card->extn_code[0] = sep[0];
      }
    }
    
    // if we found an inline comment marker, set the deck default for inline markers
    if(card->extn_code[0] != 0 && deck->extn_code == 0) {
      deck->extn_code = card->extn_code[0];
    }
    // if we found a leading (prefix) comment marker on a hidden card, set that deck default
    if(card->cmt_code[0] != '\0' && deck->cmt_code == 0) {
      deck->cmt_code = card->cmt_code[0];
    }
    
    // now call the card parsers on the different card types
    if(isCmt) {
      parse_comment_card(ctx, card, errors);
    }
    if(isGeo || isCtl) {
      parse_geometry_or_control_card(ctx, card, errors);
    }
    if(isExt) {
      parse_onec_card(ctx, card, errors);
    }
    
    // process inline comments to look for key/value pairs
    // this can apply to any card, even comments
    // EXCEPT for SY cards where formulas only appear in the main card part
    if(card->extn_code[0] != '\0' && strcmp(card->card_code, "SY") != 0) {
      parse_key_values(ctx, card, errors);
    }
  } // foreach card

  // Determine deck-level separator: scan geometry and control cards only;
  // set deck->field_sep if they all agree, otherwise leave as FSEP_UNKNOWN.
  {
    field_sep_t consensus = FSEP_UNKNOWN;
    bool conflict = false;
    for (int i = 0; i < deck->num_cards && !conflict; i++) {
      card_t *c = &deck->cards[i];
      if (!is_geometry(c) && !is_control(c)) continue;
      if (c->field_sep == FSEP_UNKNOWN) continue;
      if (consensus == FSEP_UNKNOWN) {
        consensus = c->field_sep;
      } else if (c->field_sep != consensus) {
        conflict = true;
      }
    }
    deck->field_sep = conflict ? FSEP_UNKNOWN : consensus;
  }

   // add invisible=true for geometry cards with tag >9000, from 4nec2
  mark_4nec2_cards_invisible(ctx, deck);
} /* end of parse_deck() */

/******************************************************************************
 * parse_comment_card()
 *
 * copies the comment from the card_str into the comment string so that
 * it can be updated there. This is not used to process inline comments,
 * that happened in the extension parser before we got here. This function
 * only applies to whole-line comments.
 *
 */
void parse_comment_card(context_t *ctx, card_t *card, errors_list_t *errors)
{
  // look for the different comment markers in the card_code and then
  // just copy everything else on the line to the comment. this
  // includes any leading whitespace, or lack of it
  int code_end;
  if(strcmp(card->card_code, "CM") == 0) {
    code_end = 2;
  } else if(strcmp(card->card_code, "CE") == 0) {
    code_end = 2;
  } else if(strcmp(card->card_code, "!") == 0) {
    code_end = 1;
  } else if(strcmp(card->card_code, "#") == 0) {
    code_end = 1;
  } else if(strcmp(card->card_code, "\'") == 0) {
    code_end = 1;
  } else {
    code_end = 0; // error case, shouldn't be able to happen
  }
  size_t comment_len = strlen(&card->card_str[code_end]);
  card->comment = (char *)calloc(comment_len + 1, sizeof(char));
  if (card->comment) {
    strcpy(card->comment, &card->card_str[code_end]);
    // Trim trailing whitespace from comment field
    trim_end(card->comment);
  }
}

/* Formula tokens are stored verbatim from the source so that round-trip output
 * preserves the user's original notation. preprocess_implicit_multiplication
 * and preprocess_awg are applied by the calculation layer in deck.c immediately
 * before formula evaluation with tinyexpr. */

/******************************************************************************
 * parse_geometry_or_command_card()
 *
 * parses the contents of one geometry card. formerly ???()
 *  *or*
 * parses the contents of one command card. formerly readem()
 *
 * The main difference between this code and the original nec2c code is
 * the addition of parsers for measurement units and formulas.
 *
 */
void parse_geometry_or_control_card(context_t *ctx, card_t *card, errors_list_t *errors)
{
  int ints_processed = 0;
  int flts_processed = 0;
  char *token;
  char *end_ptr;      // end point of a a number as we parse them
  size_t int_value;   // used to parse ints...
  double dbl_value;   // ... and doubles
  bool isFormula;     // was the double actually a formula?
  
  int MAX_INTS = max_int_fields(card);
  int MAX_FLTS = max_flt_fields(card);
  
  // get line length of the card part of the line
  int line_len = (int)strlen(card->card_str);
  
  // calloc has zeroed everything we need to set, so if this card doesn't
  // have any parameters, like a GM, just return now
  if(line_len <= 2) return;
  
  // skip the first two chars, the mnemonic is still there and it
  // can't be a single-char comment market, which was handled above
  // we'll use this as the pointer to the current start location
  card->field_sep = detect_field_separator(card->card_str);
  char *trimmed = trim_start(card->card_str + 2);
  char *preprocessed = strdup(trimmed);

  // WG and GF cards carry a filename that cannot be tokenized as a number.
  // Extract the filename from the raw card string and store it in card->comment,
  // then return immediately so the normal numeric field parser is not invoked.
  if (strcmp(card->card_code, "WG") == 0) {
    // WG FILENAME  — filename is the entire content after the mnemonic
    if (trimmed && *trimmed != '\0') {
      card->comment = strdup(trimmed);
      // strip NEC apostrophe-style inline comment (e.g. ' this is a comment)
      char *apos = strchr(card->comment, '\'');
      if (apos) *apos = '\0';
      // trim trailing whitespace
      size_t flen = strlen(card->comment);
      while (flen > 0 && isspace((unsigned char)card->comment[flen - 1]))
        card->comment[--flen] = '\0';
    }
    free(preprocessed);
    return;
  }
  if (strcmp(card->card_code, "GF") == 0) {
    // GF [I1] FILENAME  — I1 is the optional NGF continuation flag (0 or 1).
    // If the token after GF is not a number, treat the whole field as the filename.
    if (trimmed && *trimmed != '\0') {
      char *end_ptr2;
      long i1_val = strtol(trimmed, &end_ptr2, 10);
      char *fname_start;
      if (end_ptr2 != trimmed) {
        // Leading integer found — store it and advance past it
        card->i[1] = (int)i1_val;
        fname_start = trim_start(end_ptr2);
      } else {
        // No leading integer — filename starts immediately
        fname_start = trimmed;
      }
      if (fname_start && *fname_start != '\0') {
        card->comment = strdup(fname_start);
        // strip NEC apostrophe-style inline comment
        char *apos = strchr(card->comment, '\'');
        if (apos) *apos = '\0';
        // trim trailing whitespace
        size_t flen = strlen(card->comment);
        while (flen > 0 && isspace((unsigned char)card->comment[flen - 1]))
          card->comment[--flen] = '\0';
      }
    }
    free(preprocessed);
    return;
  }

  // tokenize the rest of the line on the remaining whitespace
  token = strtok(preprocessed, ONEC_WHITESPACE);

  while(token != NULL) {
    isFormula = false;  // assume it's a number until proven otherwise

    // we have a non-zero length token, which might be an int
    // or float depending on what we've seen before
    if(ints_processed < MAX_INTS) {
      ints_processed++;

      // parse a number if we can find it
      int_value = strtol(token, &end_ptr, 10);
      
      // if there was a number in there, end_ptr will no longer be at the
      // start and that means we found one and can store it in the right slot
      if(end_ptr != token) {
        card->i[ints_processed] = (int)int_value;
        
        // we got a number at the front, but is that all we got?
        // no units in these fields, so it would have to be a formula
        char *leftover = end_ptr;
        if(strlen(leftover) > 0) {
          isFormula = true;
        }
        card->i[ints_processed] = (int)int_value;
      }
      // if end_ptr = token, then we didn't find any number at the start
      else {
        isFormula = true;
      }
      
      // if there was a formula, save it
      if(isFormula) {
        card->int_form_inline[ints_processed] = true;  // indicate that we did have a formula inline
        char fld_name[3];
        fld_name[0] = 'I';
        fld_name[1] = ints_processed +'0';
        fld_name[2] = '\0';
        add_key_value(card, &card->formulas, fld_name, token, '=');
      }
    } // end integer part
    
    // doubles are more complicated because the fields may contain other
    // bits like measurement indicators like "mm" or have formulas in them
    // the code assumes that it's a number until proven wrong. if it is, we
    // set isFormula and/or isUnit
    //
    else if(flts_processed < MAX_FLTS) {
      flts_processed++;
      
      // Check for AWG wire gauge notation (#14 or 14awg)
      // These are now handled as formulas and processed by preprocess_awg()
      bool is_awg = false;
      if(token[0] == '#') {
        // 4nec2 format: #14
        is_awg = true;
      } else if(str_ends_with(ctx, token, "awg") == 0) {
        // OpenNEC format: 14awg
        is_awg = true;
      }
      
      if(is_awg) {
        // Store AWG notation as a formula to be processed later
        card->flt_form_inline[flts_processed] = true;
        char fld_name[3];
        fld_name[0] = 'F';
        fld_name[1] = flts_processed + '0';
        fld_name[2] = '\0';
        add_key_value(card, &card->formulas, fld_name, token, '=');
        goto NEXT_TOKEN;
      }

      // try to read a double in the token, which has to be at the start or
      // strtod fails - this is what we want, in case someone does 'freq/2'
      dbl_value = strtod(token, &end_ptr);
      if(end_ptr != token) {
        // we got a number at the front, but is that all we got?
        char *leftover = end_ptr;
        
        if(strlen(leftover) > 0) {
          isFormula = true;
        }
      } else {
        isFormula = true;
      }
            
      // now we decide where to put it all...
      if(!isFormula) {
        card->f[flts_processed] = dbl_value;
      } else {
        card->flt_form_inline[flts_processed] = true;
        char fld_name[3];
        fld_name[0] = 'F';
        fld_name[1] = flts_processed +'0';
        fld_name[2] = '\0';
        add_key_value(card, &card->formulas, fld_name, token, '=');

      } // isFormula = true
    } // dbls_processed < MAX_FLOATS
    
    // and move on to the next bit
  NEXT_TOKEN:
    token = strtok(NULL, ONEC_WHITESPACE);
  } //token != NULL
  
  card->ints_used = ints_processed;
  card->flts_used = flts_processed;
  
  free(preprocessed);
} /* end of parse_geometry_or_control_card() */

/******************************************************************************
 * parse_onec_card()
 *
 * parses cards only understood by onec, which at this point is only
 * the SY. Although XT is also part of the onec list, there's nothing to
 * do in that case as it has no parameters so it has no code here.
 *
 * TODO:
 *
 * - add handlers for IT(eration) and OP(timization) cards, which
 *   produce multiple output runs
 *
 */
void parse_onec_card(context_t *ctx, card_t *card, errors_list_t *errors)
{
  // see if this is an SY card, otherwise exit
  // TODO: add all onec_codes here, we currently only do SY
  if(strcmp(card->card_code, "SY") == 0) {
    // make a copy of the string so we can mangle it
    char str[MAX_LINE_LEN];
    strcpy(str, card->card_str + 2);

    // Split on commas that are NOT inside parentheses, so mod(10,3) stays intact.
    char *p = str;
    char *split;
    while (p != NULL) {
      // Find the next top-level comma (depth == 0)
      char *tok_start = p;
      char *tok_end = NULL;
      int depth = 0;
      char *scan = p;
      while (*scan) {
        if (*scan == '(') depth++;
        else if (*scan == ')') depth--;
        else if (*scan == ',' && depth == 0) {
          tok_end = scan;
          break;
        }
        scan++;
      }
      if (tok_end) {
        *tok_end = '\0';   // terminate this token
        p = tok_end + 1;   // next search starts after the comma
      } else {
        p = NULL;          // last token
      }

      char *token = tok_start;
      // trim leading whitespace
      while(isspace((unsigned char)*token)) token++;
      // skip empty tokens
      if(strlen(token) == 0) continue;

      // split on '='
      split = strchr(token, '=');
      if(split != NULL) {
        // separate key and value
        *split = '\0';
        char *key = token;
        char *value = split + 1;
        // trim whitespace from key and value
        key = trim(key);
        value = trim(value);

        // parse it if there's anything left
        if(strlen(key) > 0 && strlen(value) > 0) {
          key_value_t *pair = (key_value_t *)malloc(sizeof(key_value_t));
          if(pair != NULL) {
            pair->key = strdup(key);
            pair->value = strdup(value);
            pair->separator = '=';
            pair->next = NULL;
            // add to end of formulas list
            key_value_t *tail = card->formulas;
            if(tail == NULL) {
              card->formulas = pair;
            } else {
              while(tail->next != NULL) tail = tail->next;
              tail->next = pair;
            }
          }
        }
      }
    }
  }
} /* end of parse_onec_card() */

/******************************************************************************
 * parse_key_values()
 *
 * parses a string that may contain key/value pairs
 *
 * also looks for "comment:" markers within the string, and assumes
 * everything after the marker is a comment
 *
 */
void parse_key_values(context_t *ctx, card_t *card, errors_list_t *errors)
{
  char str[MAX_LINE_LEN];
  
  // track whether we found any onec extensions after the comment
  // marker. if we didn't, everything after the marker is a
  // comment all on its own
  bool hasExtensions = false;
  
  // make a copy of the string so we can mangle it
  size_t extn_len = strlen(card->extn_str);
  if (extn_len >= MAX_LINE_LEN) {
    // Truncate if too long, but this is a rare case
    extn_len = MAX_LINE_LEN - 1;
  }
  memcpy(str, card->extn_str, extn_len);
  str[extn_len] = '\0';
  
  // We'll parse tokens manually to support quoted values (single or
  // double-quoted) so extension values can contain spaces. Tokens are
  // separated by commas/semicolons or whitespace when not quoted.
  char *p = str;
  while (*p != '\0') {
    // skip leading whitespace and separators (comma/semicolon)
    while (*p != '\0' && (isspace((unsigned char)*p) || *p == ',' || *p == ';'))
      p++;
    if (*p == '\0')
      break;

    // extract one token (respecting quotes)
    char token[MAX_LINE_LEN];
    int ti = 0;
    if (*p == '"' || *p == '\'') {
      char q = *p++;
      while (*p != '\0' && *p != q && ti < (int)sizeof(token) - 1) {
        // allow escaped quotes inside quoted string (\" or \\')
        if (*p == '\\' && p[1] != '\0') {
          token[ti++] = *p++;
          token[ti++] = *p++;
          continue;
        }
        token[ti++] = *p++;
      }
      if (*p == q) p++; // skip closing quote
    } else {
      while (*p != '\0' && !isspace((unsigned char)*p) && *p != ',' && *p != ';' && ti < (int)sizeof(token) - 1) {
        token[ti++] = *p++;
      }
    }
    token[ti] = '\0';

    if (ti == 0)
      continue;

    // find delimiter = or : inside token
    char *split = strpbrk(token, "=:");
    if (split != NULL) {
      // separate key and value
      *split = '\0';
      char *keyptr = token;
      char *valptr = split + 1;
      // trim whitespace around key and value
      char *k = trim(keyptr);
      char *v = trim(valptr);

      // strip surrounding quotes on value if present
      size_t vlen = strlen(v);
      if ((vlen >= 2) && ((v[0] == '"' && v[vlen - 1] == '"') || (v[0] == '\'' && v[vlen - 1] == '\''))) {
        v[vlen - 1] = '\0';
        v = v + 1;
      }

      if (strlen(k) == 0 && strlen(v) == 0)
        continue;

      hasExtensions = true;

      if (strcasecmp(k, "ignore") == 0) {
        card->ignore = (strcasecmp(v, "true") == 0 || strcasecmp(v, "yes") == 0 || strcasecmp(v, "1") == 0);
        continue;
      }
      if (strcasecmp(k, "invisible") == 0) {
        card->invisible = (strcasecmp(v, "true") == 0 || strcasecmp(v, "yes") == 0 || strcasecmp(v, "1") == 0);
        continue;
      }
      if (strcasecmp(k, "comment") == 0) {
        char *leftover = strstr(card->extn_str, "comment");
        if (leftover) {
          leftover += 7; // point to delimiter or following char
          // skip a delimiter if present
          if (*leftover == ':' || *leftover == '=') leftover++;
          while (*leftover && isspace((unsigned char)*leftover)) leftover++;
          size_t len = strlen(leftover);
          card->comment = (char *)calloc(len + 1, sizeof(char));
          if (card->comment) {
            strcpy(card->comment, leftover);
          }
        }
        return;
      }

      // now decide if key is a formula name
      bool isFormula = false;
      for (int i = 0; i < NUM_FIELD_NAMES; i++) {
        if (strcasestr(k, field_names[i]) == 0) {
          isFormula = true;
          break;
        }
      }

      key_value_t *pair = (key_value_t *)malloc(sizeof(key_value_t));
      if (pair != NULL) {
        pair->key = strdup(k);
        pair->value = strdup(v);
        pair->separator = (strchr("=:", token[strlen(k)]) != NULL) ? token[strlen(k)] : '=';
        pair->next = NULL;

        key_value_t **headp = isFormula ? &card->formulas : &card->extensns;
        if (*headp == NULL) {
          *headp = pair;
        } else {
          key_value_t *tail = *headp;
          while (tail->next) tail = tail->next;
          tail->next = pair;
        }
      }
    }
  }
  
  // if we entered this func we had to have *some* sort of trailing
  // comment. by this point we've processed out all the extensions
  // that might have been in there. if there were none, then the
  // entire comment section was a comment text, so save that out
  if(!hasExtensions) {
    if (card->extn_str) {


//       size_t len = strlen(card->extn_str);
//       card->comment = (char *)calloc(len + 1, sizeof(char));
//       if (card->comment) {
//         strcpy(card->comment, card->extn_str);
//       }
//     } else {
//       card->comment = NULL;
     }
  }
} /* end of parse_key_values() */

/******************************************************************************
 * mark_4nec2_cards_invisible
 *
 * Called near the end of deck parsing to look for geometry cards with a tag
 * number >=9800 <9900, which 4nec2 uses to indicate invisible geometry.
 * If such a card is found, and it does not already have an "invisible"
 * extension, one is added with the value "true".
 */
void mark_4nec2_cards_invisible(context_t *ctx, deck_t *deck)
{
  const int INV_MIN = 9800;
  const int INV_MAX = 9900; // 9900 and up are current sources

  if (!deck || deck->num_cards <= 0) return;
  int start = deck->geometry_start >= 0 ? deck->geometry_start : 0;
  int end = deck->geometry_end >= 0 ? deck->geometry_end : deck->num_cards - 1;

  for (int i = start; i <= end; i++) {
    card_t *card = &deck->cards[i];
    if (!is_geometry(card)) continue;

    int tag = card->tag; /* populated during geometry build input parsing */
    if (tag < INV_MIN || tag >= INV_MAX) continue;

    /* tags in this range indicate invisible geometry in 4nec2, so set the
     * flag.  We do not need to append an extension here; the write path will
     * generate one if the flag is true. */
    card->invisible = true;
  }
} /* end of input.c */
