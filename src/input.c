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

#include "opennec.h"
#include "input.h"

/* Forward declarations for internal functions */
static int read_line(nec_context_t *ctx, char *buff, FILE *pfile, int line_num);
static void parse_comment_card(nec_context_t *ctx, card_t *card, errors_list_t *errors);
static void parse_geometry_or_control_card(nec_context_t *ctx, card_t *card, errors_list_t *errors);
static void parse_onec_card(nec_context_t *ctx, card_t *card, errors_list_t *errors);
static void parse_key_values(nec_context_t *ctx, card_t *card, errors_list_t *errors);

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
void read_deck(nec_context_t *ctx, deck_t *deck, FILE *pfile)
{
  char line_buf[1024];  // make it large enough to hold any line
  size_t line_len;      // actual length of the current card being read
    
  // set the card count to 0, it might have !=0 default
  deck->num_cards = 0;

  // and the symbols...
  deck->num_symbols = 0;
  
  // and set the default comment marker to empty
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
    line_len = strlen(line_buf);
    if(deck->num_cards == 0) {
      deck->num_cards++;
      deck->cards = calloc(1, sizeof(card_t));
      if (!deck->cards) {
        char *msg = calloc(1, MAX_ERROR_LEN);
        snprintf(msg, MAX_ERROR_LEN, "[read_deck] ERROR: calloc failed for deck->cards at line %d", line_num);
        add_error(ctx, &ctx->errors, msg, FATAL);
        free(msg);
        return;
      }
    } else {
      deck->num_cards++;
      card_t *new_cards = realloc(deck->cards, deck->num_cards * sizeof(card_t));
      if (!new_cards) {
        char *msg = calloc(1, MAX_ERROR_LEN);
        snprintf(msg, MAX_ERROR_LEN, "[read_deck] ERROR: realloc failed for deck->cards at line %d", line_num);
        add_error(ctx, &ctx->errors, msg, FATAL);
        free(msg);
        return;
      }
      deck->cards = new_cards;
    }
    card_t *dest = &deck->cards[deck->num_cards - 1];
    memset(dest, 0, sizeof(card_t));
    dest->orig_str = calloc(line_len + 1, sizeof(char));
    if (!dest->orig_str) {
      char *msg = calloc(1, MAX_ERROR_LEN);
      snprintf(msg, MAX_ERROR_LEN, "[read_deck] ERROR: calloc failed for card->orig_str at line %d", line_num);
      add_error(ctx, &ctx->errors, msg, FATAL);
      free(msg);
      return;
    }
    dest->edited = false;
    dest->ignore = false;
    strncpy(dest->orig_str, line_buf, line_len);
    dest->orig_str[line_len] = '\0';
    if (read_result == EOF && !last_line_nonempty) {
      break;
    }
    last_line_nonempty = 0;
  } while(true);
  // do not free(card) here; its contents are now owned by deck->cards[]
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
 * @param ctx The current nec_context_t, used for error reporting
 * @param buff String containing the contents of one line
 * @param file file pointer to the file to be read, assumed
 * @param line_num The current line number in the file, for error reporting
 *  to have been opened previous to this call
 * 
*/
int read_line(nec_context_t *ctx, char *buff, FILE *pfile, int line_num)
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
  // have leading cr's or lf's left over. this code eats them. note that this
  // also eats totally empty lines, and it's not clear that's what we want,
  // we might want to save those in order to report a warning. if that's the
  // case, it would seem we should do this eating at the end of the routine?
  while((chr == CR) || (chr == LF)) {
    // eat the next char, and return if that's the eof
    if((chr = getc(pfile)) == EOF) {
      return(EOF);
    }

    // eat any remaining line-ends
    while((chr == CR) || (chr == LF)) {
      if((chr = getc(pfile)) == EOF) {
        return(EOF);
      }
    }
  } /* end of while( (chr == CR) || ... */

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
      char *msg = calloc(1, MAX_ERROR_LEN);
      snprintf(msg, MAX_ERROR_LEN, "The card on line %d has non-whitespace characters beyond %d characters, these have been removed.", 
               line_num, MAX_LINE_LEN);
      add_error(ctx, &ctx->errors, msg, WARNING);
      free(msg);
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
void parse_deck(nec_context_t *ctx, deck_t *deck, errors_list_t *errors)
{
  card_t *card;
  
  size_t line_len; // length of the original string for this card
  char type_buff[3];
  char hidden_type_buff[3];
  bool isCmt, isGeo, isCtl, isExt; // cache these so we can do the string compare only once
  bool sawCM = false, sawCE = false, sawGx = false, sawGE = false, sawEN = false; // keep track of where we are

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
      strncpy(temp_str, curr_card->orig_str, orig_len);
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

  for(int i = 0; i < deck->num_cards; i++) {
    card = &deck->cards[i];
    // get the card and the original string length
    line_len = strlen(card->orig_str);
    
    // Skip leading whitespace
    size_t first = 0;
    while (first < line_len && isspace((unsigned char)card->orig_str[first])) first++;
    
    // Check for comment markers (CM, CE, !, #, ')
    if (toupper(card->orig_str[first]) == 'C' && toupper(card->orig_str[first+1]) == 'M') {
      strcpy(type_buff, "CM");
    } else if (toupper(card->orig_str[first]) == 'C' && toupper(card->orig_str[first+1]) == 'E') {
      strcpy(type_buff, "CE");
    } else if (card->orig_str[first] == '!') {
      strcpy(type_buff, "!");
    } else if (card->orig_str[first] == '#') {
      strcpy(type_buff, "#");
    } else if (card->orig_str[first] == '\'') {
      strcpy(type_buff, "'");
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
    }
    strncpy(card->card_code, type_buff, 2);
    card->card_code[2] = '\0';  // Ensure null termination
    
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
    // if this is the first geo card, including GE...
    if(isGeo && !sawGx && !sawEN) {
      deck->geometry_start = i;
      sawGx = true;
    }
    // the GE only has to be above the end of the deck
    if(strcmp(type_buff, "GE") == 0 && !sawGE && !sawEN) {
      deck->geometry_end = i;
      sawGE = true;
    }
    // and finally, the first EN that we see
    if(strcmp(type_buff, "EN") == 0) {
      deck->deck_end = i;
      sawEN = true;
    }
    
    // another special case: if this is a comment card but the next two characters
    // are one of the extensions, this is really a "hidden extension" that is being
    // used to make the deck compatible with older NEC programs. In that case, we
    // want to change the card to an extension type, make it not a comment, and save
    // the comment marker in the extn. note that this could only be the case if the
    // card has at least three characters, which would assume something like !SY
    if(isCmt && line_len > 3) {
      // skip forward to find anything after the comment marker
      size_t pos = 0;
      if(strcmp(type_buff, "CM") == 0 || strcmp(type_buff, "CE") == 0)  {
        pos = 2;
      } else if (strcmp(type_buff, "!") == 0 || strcmp(type_buff, "#") == 0|| strcmp(type_buff, "'") == 0) {
        pos = 1;
      } else {
        // how would this even happen?
      }
      // only continue if we didn't eat the entire line
      if(pos < line_len) {
        // eat any whitespace
        if(isspace(card->orig_str[pos])) {
          pos++;
        }
      }
      
      // get the two characters *after* the comment marker
      if((strcmp(type_buff, "CM") == 0 || strcmp(type_buff, "CE") == 0) && line_len > 4) {
        strncpy(hidden_type_buff, &card->orig_str[2], 2);
      } else if (strcmp(type_buff, "!") == 0 || strcmp(type_buff, "#") == 0|| strcmp(type_buff, "'") == 0) {
        strncpy(hidden_type_buff, &card->orig_str[1], 2);
      } else {
        // we didn't find anything interesting
        strcpy(hidden_type_buff, "");
      }
      // now we see if those two characters are one of the extensions
      bool isHidden = false;
      for(int i = 0; i < NUM_ONEC_CODES; i++) {
        if(strcmp(hidden_type_buff, onec_codes[i]) == 0) { // was card->card_code in the front?
          isHidden = true;
          break;
        }
      }
      if(isHidden) {
        isCmt = false;
        isExt = true;
        card->extn_code[0] = '!';
      }
    } // checking for hidden info
    
    // if we're past the end of the deck, everything that appears is a comment,
    // and we'll just copy it into the comment string. but if we are not past
    // the end, and we didn't recognize the code then we want to report
    // an error
    if (!sawEN && !isCmt && !isCtl && !isGeo && !isExt) {
      char *msg = calloc(1, MAX_ERROR_LEN);
      snprintf(msg, MAX_ERROR_LEN, "The card on line %d has unknown type '%s'. Card skipped.", i+1, type_buff);
      add_error(ctx, errors, msg, PROBLEM);
      free(msg);
    }
    
    // if we did figure out the card type, then we want to put something in the card_str,
    // but first we want to see if there is a comment inside the line ( > 0, < len ) and
    // clip that part out separately into extn_str
    if(isCmt || isCtl || isGeo || isExt) {
      size_t len; // this is the length of the main card text
      // look for a comment marker, adjust length of card text based on that
      const char *sep = strpbrk(card->orig_str, ONEC_COMMENTS);
      if(sep == NULL) {
        // no comment was found, put everything into the string
        len = strlen(card->orig_str);
      } else {
        if (isCmt && sep == card->orig_str) {
          // for comment cards that start with comment marker, include the whole line
          len = strlen(card->orig_str);
        } else {
          len = sep - card->orig_str;
        }
      }
      // malloc room for the card part, copy that in, and close the string
      card->card_str = (char *)malloc((len * sizeof(char)) + 1);
      strncpy(card->card_str, card->orig_str, len);
      card->card_str[len] = '\0';

      // and if there was any leftover, copy it into the extension, otherwise make sure its empty
      if(sep == NULL || (isCmt && sep == card->orig_str)) {
        card->extn_str = NULL;
        card->extn_code[0] = '\0';
      } else {
        card->extn_str = strdup(sep + 1);
        card->extn_code[0] = sep[0];
      }
    }
    
    // if we did find a comment marker in this line, and the deck doesn't have a
    // a default marker set, assume this is the one used in the entire file and
    // make it the default. You can still use other markers on other lines, but
    // if you add a new card programmatically and set a comment, it should
    // default to using this marker
    if(card->extn_code[0] != 0 && deck->cmt_code == 0) {
      deck->cmt_code = card->extn_code[0];
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
void parse_comment_card(nec_context_t *ctx, card_t *card, errors_list_t *errors)
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
  }
}

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
void parse_geometry_or_control_card(nec_context_t *ctx, card_t *card, errors_list_t *errors)
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
  char str[MAX_LINE_LEN];
  strcpy(str, trim_start(card->card_str + 2)); // skip the card code and remove whitespace
  
  // tokenize the rest of the line on the remaining whitespace
  token = strtok(str, ONEC_WHITESPACE);
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
        // ...removed pre-add_key_value debug print...
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
        // This preserves the original notation and allows formulas like "#14*2"
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
          // Any leftover text (like "mm", "ft", "uF") means this is a formula
          // The parser treats tokens like "10mm" as formulas to be evaluated
          // with unit constants from the symbol table (e.g., mm=0.001)
          isFormula = true;
        }
      } else {
        // we did not get a number at the front, so it must be a formula one way or the other
        isFormula = true;
      }
            
      // now we decide where to put it all...
      if(!isFormula) {
        // if it's not a formula, just store the numeric value
        card->f[flts_processed] = dbl_value;
        // Unit conversion now handled via formula constants
      } else {
        // it is a formula, copy the entire token into the right formula field
        // This preserves the original capitalization (e.g., "10MM" stays "10MM")
        card->flt_form_inline[flts_processed] = true;  // indicate that we did have a formula inline
        char fld_name[3];
        fld_name[0] = 'F';
        fld_name[1] = flts_processed +'0';
        fld_name[2] = '\0';
        // ...removed pre-add_key_value debug print...
        add_key_value(card, &card->formulas, fld_name, token, '=');

      } // isFormula = true
    } // dbls_processed < MAX_FLOATS
    
    // and move on to the next bit
  NEXT_TOKEN:
    token = strtok(NULL, ONEC_WHITESPACE);
  } //token != NULL
  
  // now we copy down the number of ints and floats we actually saw
  card->ints_used = ints_processed;
  card->flts_used = flts_processed;
} /* end of parse_geometry_card() */

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
void parse_onec_card(nec_context_t *ctx, card_t *card, errors_list_t *errors)
{
  // see if this is an SY card, otherwise exit
  // TODO: add all onec_codes here, we currently only do SY
  if(strcmp(card->card_code, "SY") == 0) {
    // make a copy of the string so we can mangle it
    char str[MAX_LINE_LEN];
    strcpy(str, card->card_str + 2);

    // Accept both comma and whitespace as delimiters, but also handle single key=value with no comma
    char *token, *split;
    // First, try to split by comma. If no comma, treat as single token
    token = strtok(str, ",");
    while(token != NULL) {
      // Now trim leading whitespace
      while(isspace((unsigned char)*token)) token++;

      // if token is empty, skip
      if(strlen(token) == 0) {
        token = strtok(NULL, ",");
        continue;
      }

      // now split on '='
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
      // next token
      token = strtok(NULL, ",");
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
void parse_key_values(nec_context_t *ctx, card_t *card, errors_list_t *errors)
{
  char str[MAX_LINE_LEN];
  char key[MAX_LINE_LEN], value[MAX_LINE_LEN];
  
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
  strncpy(str, card->extn_str, extn_len);
  str[extn_len] = '\0';
  
  // strtok should be perfect for this one because we don't want the delimiters
  // to be handed back ...but this will split up comments on their whitespace,
  // which is why comments have to be at the end because they're the only thing
  // that can have spaces inside them
  char *token, *split;
  token = strtok(str, ONEC_WHITESPACE ONEC_SEPARATORS); // note the "concat the string literals" trick
  while(token != NULL) {
    // make sure there's a equals or colon in it - note you can't nest another strtok!
    split = strpbrk(token, "=:");
    if(split != NULL) {
      // if the split was successful, meaning we found an = or : somewhere,
      // the split char is still on the front so let's kill it
      if (split[0] == '=') split++;
      if (split[0] == ':') split++;
      
      // and split these out for ease of use
      strncpy(key, token, split - token - 1); // move back over the delimiter
      key[split - token - 1] = '\0'; // add the missing null, strncpy doesn't
      strcpy(value, split); // this has a null at the end so we're ok
      
      // now check that both sides are >0 len, otherwise skip this one
      if(strlen(key) == 0 && strlen(value) == 0)
        goto NEXT_TOKEN;
      
      // getting here means we did find a valid key/value of some
      // sort, so record that we got it
      hasExtensions = true;
      
      // there are a couple of cases here:
      // 1) the key is "name", "group", "material", "visible" or "ignore", which are stored separately
      // 2) the key is "comment" - copy everything after it into comment string and exit
      // 3) the key is a formula - make a key_value_t pair and add it to the formulas list
      // 4) the key is anything else - make a key_value_t pair and add it to the pairs list
      
      // handle known
      if(strcasecmp(key, "ignore") == 0) {
        card->ignore = (strcasecmp(value, "true") == 0 || strcasecmp(value, "yes") == 0 || strcasecmp(value, "1") == 0);
        goto NEXT_TOKEN;
      }
      // and comments, which cause us to exit because we have to be at the end of the line
      // we can't just loop again because the comment might have whitespace and would still
      // generate more tokens which are just bits of the comment
      if(strcasecmp(key, "comment") == 0) {
        char *leftover = strstr(card->extn_str, "comment");
        leftover += 8; // 7 chars for 'comment' and 1 for the delimiter
        size_t len = strlen(leftover);
        card->comment = (char *)calloc(len + 1, sizeof(char));
        if (card->comment) {
          strcpy(card->comment, leftover);
        }
        return;
      }
      
      // now see if it's a formula
      bool isFormula = false;
      for(int i = 0; i < NUM_FIELD_NAMES; i++) {
        if(strcasestr(key, field_names[i]) == 0) {
          isFormula = true;
          break;
        }
      }
      
      // formulas and any other tags not pulled out above are handled
      // in the same fashion - we make a key_value_t pair to hold it.
      // They differ only in where we put them in the end
      key_value_t *pair = (key_value_t *)malloc(sizeof(key_value_t));
      if(pair != NULL) {
        // calloc the strings and store them...
        pair->key = calloc(split - token, sizeof(char));
        strncpy(pair->key, token, split - token - 1);
        pair->value = calloc(strlen(split) + 1, sizeof(char)); // add one for a trailing null
        strcpy(pair->value, split);
        // and store the original separator so we can recreate it on output
        pair->separator = token[split - token - 1];
        // and null this out, as its going on the end
        pair->next = NULL;
        
        // now decide which list to add it to
        key_value_t *head, *tail;
        if(isFormula) {
          head = card->formulas;
        } else {
          head = card->extensns;
        }
        tail = head;
        
        // and then add it to the end of the list
        if(tail == NULL) {
          head = pair;
        } else {
          while(tail->next != NULL)
            tail = tail->next;
          tail->next = pair;
        }
        if(isFormula) {
          card->formulas = head;
        } else {
          card->extensns = head;
        }

      } // there should be else's for all the mallocs and callocs!
    } // split != NULL
    
  NEXT_TOKEN:
    // and get the next token
    token = strtok(NULL, ONEC_WHITESPACE ONEC_SEPARATORS);
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
 * number >=9800 <=9900, which 4nec2 uses to indicate invisible geometry.
 * If such a card is found, and it does not already have an "invisible"
 * extension, one is added with the value "true".
 */
void mark_4nec2_cards_invisible(nec_context_t *ctx, deck_t *deck)
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

    // check if an "invisible" extension already exists
    bool has_invisible = false;
    key_value_t *kv = card->extensns;
    while (kv) {
      if (kv->key && strcasecmp(kv->key, "invisible") == 0) {
        has_invisible = true;
        break;
      }
      kv = kv->next;
    }
    if (has_invisible) continue;

    // append invisible=true to card->extensns
    key_value_t *pair = (key_value_t *)malloc(sizeof(key_value_t));
    if (!pair) {
      add_error(ctx, &ctx->errors, "Memory allocation failed for 'invisible' key/value", FATAL);
      return;
    }
    pair->key = (char *)calloc(strlen("invisible") + 1, sizeof(char));
    pair->value = (char *)calloc(strlen("true") + 1, sizeof(char));
    if (!pair->key || !pair->value) {
      free(pair->key); free(pair->value); free(pair);
      add_error(ctx, &ctx->errors, "Memory allocation failed for 'invisible' strings", FATAL);
      return;
    }
    strcpy(pair->key, "invisible");
    strcpy(pair->value, "true");
    pair->separator = ':'; // default separator for extensions
    pair->next = NULL;

    if (card->extensns == NULL) {
      card->extensns = pair;
    } else {
      key_value_t *tail = card->extensns;
      while (tail->next) tail = tail->next;
      tail->next = pair;
    }
  }
} /* end of input.c */
