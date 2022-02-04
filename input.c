/*******************************************************************
 * input.c
 *
 * input.c contains the routines needed to read and parse a card
 * deck file, normally ".nec" or ".deck". The process starts with
 * read_deck(), which uses read_line() to get the data from one
 * line in the file and then turn it into a Card. When all the
 * lines are read into Cards, read_deck() then calls a series of
 * functions to parse the deck into parts, using the logic
 * found in parse_command_card(), parse_geometry_card(), etc.
 *
 * The original nec2c, like the NEC2 code it was based on, is very
 * "modal". It reads a card, processes it, and then forgets it.
 * Any card that cannot be processed, like comments, are skipped
 * entirely. In contrast, OpenNEC reads the entire deck into memory
 * first. This allows it to perform whole-deck checks, like looking
 * for a missing EN, or a GW that's missing its GC. To do this,
 * OpenNEC also keeps every card it finds, even blank lines and
 * inline comments.
 *
 * You will note that the code for reading lines is more complex
 * than you might see in most C programs - it reads char by char
 * instead of doing a read-line. This is because NEC decks are
 * sometimes edited by hand in editors that insert hard breaks
 * when saved, and this are relatively common on the 'net. So this
 * code is somewhat more complex and tries to merge broken lines
 * of this sort. This is an area that might be improved, but
 * performance on modern machines makes this a non-issue in all the
 * examples I fed it.
 *
 * One lingering problem is when such a break has been inserted
 * precisely where an inline comment occurs. This causes the parser
 * to consider the second line to be a comment card, instead of
 * a comment on the previous card. This is a TODO.
 *
 * Other changes to this code include a wider set of comment
 * markers, including CM, !, # and ', whereas nec2c only accepted
 * # outside the comment header. Additionally, this code looks for
 * comment markers *in* a line, and splits that data out to a
 * separate buffer for processing out the (potential) OpenNEC
 * extensions. It keeps track of what the original comment marker
 * was so it can save it back out in the same format.
 *
 *******************************************************************/

#include "opennec.h"
#include "shared.h"

/* forward declares */
void parse_card(Card *card, int card_num, Errors *errors);

/*----------------------------------------------------------------------*/
/* read_deck()
 *
 * Reads the entire deck line by line and fills out the deck's cards[]
 * array with the resulting data. After this completes, the caller
 * should call parse_deck() to process the data.
 *
 */
void read_deck(Deck *deck, FILE *input_fp)
{
  Card *card = NULL;  // the card we're working on, have to null it or the Free() below may fail when this contains garbage

  char line_buf[MAX_LINE_LEN];  // make it large enough to hold any possible line
  size_t line_len;              // actual length of the current card being read
    
  /* set the card count to 0, it might have !=0 default */
  deck->num_cards = 0;

  /* loop and read lines one-by-one until we hit the EOF */
  while(TRUE) {
    /* read a line from input file and exit if it's the end of the stack */
    if(read_line(line_buf, input_fp) == EOF) break;
    
    /* store the line length because we do lots of comparisons against it */
    line_len = strlen(line_buf);
    
    /* make a new card and copy in the line */
    /* we save every line to a card, even blank lines */
    card = calloc(1, sizeof(Card));
    card->orig_str = calloc(line_len +1, sizeof(char));
    card->edited = FALSE;
    strcpy(card->orig_str, line_buf);
    
    /* calloc/realloc the deck and add this card to it */
    /* there may be performance improvements possible by allocing blocks of 10 or 20 cards at a time */
    if(deck->num_cards == 0) {
      deck->num_cards++;
      deck->cards = calloc(1, sizeof(Card));
    } else {
      deck->num_cards++;
      deck->cards = realloc(deck->cards, deck->num_cards * sizeof(Card));
    }
    deck->cards[deck->num_cards - 1] = *card;
  }
  
  /* card is temp, free it */
  free(card);
}

/*----------------------------------------------------------------------*/
/* read_line()
 *
 * reads a line from a file, aborts on failure
 *
 * Formerly known as "load_line", this version does not skip empty
 * lines or those that are pure comments. OpenNEC wants to save these
 * to separate cards so that it can perform whole-stack syntax checking
 * and similar tests.
 *
 * You might expect this to use c's built-in fgets or getline or similar.
 * It doesn't because of the infrequent but seen-in-the-wild problem
 * of hard returns being inserted in the middle of lines when users
 * edit their decks using some text editors. This code reads until
 * is has seen the required number of fields even if they cross
 * multiple lines, thereby fixing such damage by merging the lines
 * back together.
 *
 * This means that if one simply loads and then saves the deck, the
 * split lines will be removed.
 *
 * This can likely be improved by using scanf to read in one line,
 * process as much of it as possible, and the  deciding whether or
 * not the line is complete. But it seems unlikely this would have a
 * real-world impact given the size of typical decks.
 *
 * This code also automatically capitalizes the first two characters
 * on the line.
 *
 */
int read_line(char *buff, FILE *file)
{
  int
    num_chr = 0,  // number of characters read, excluding lf/cr
    eof = 0,      // EOF flag
    chr;          // character read by getc
  
  /* clear buffer at start */
  buff[0] = '\0';
  
  // if we're at the end of the file, return that, we're done
  if((chr = getc(file)) == EOF) {
    return(EOF);
  }
  
  // the line parser below stops and returns as soon as it sees a single cr or lf
  // that means that when we re-enter the routine, the file might have leading
  // cr's or lf's left over. this code eats them. note that this also eats totally
  // empty lines, and it's not clear that's what we want, we might want to save those
  // in order to report a warning. if that's the case, it would seem we should do this
  // eating at the end of the routine?
  while((chr == CR) || (chr == LF)) {
    // eat the next char, and return if that's the eof
    if((chr = getc(file)) == EOF) {
      return(EOF);
    }
    
    // eat any remaining line-ends
    while((chr == CR) || (chr == LF)) {
      if((chr = getc(file)) == EOF) {
        return(EOF);
      }
    }
  } /* end of while( (chr == CR) || ... */
  
  // read the line until you pick up any trailing cr's or lfs.
  while(num_chr < MAX_LINE_LEN) {
    /* if lf/cr reached before filling buffer, return */
    if((chr == CR) || (chr == LF))
      break;
    
    /* enter new char to buffer */
    buff[num_chr++] = (char)chr;
    
    /* if we get the EOF, end the string at that point by replacing it with a null */
    if((chr = getc(file)) == EOF) {
      buff[num_chr] = '\0';
      eof = EOF;
    }
  } /* end of while( num_chr < MAX_LINE_LEN ) */

  /* terminate buffer as a string */
  buff[num_chr] = '\0';
  
  return(eof);
} /* end of read_line() */

/*----------------------------------------------------------------------*/
/* parse_deck()
 *
 * parses the original data from the file once it's all read in
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
void parse_deck(Deck *deck, Errors *errors)
{
  Card *card;
  
  size_t line_len; // length of the original string for this card
  char type_buff[3];
  char hidden_type_buff[3];
  bool isCmt, isGeo, isCtl, isExt; // cache these so we can do the string compare only once
  bool sawCM = FALSE, sawCE = FALSE, sawGx = FALSE, sawGE = FALSE, sawEN = FALSE; // keep track of where we are

  for(int i = 0; i < deck->num_cards; i++) {
    card = &deck->cards[i];
    
    // set the card number, which is used in the parsers below to report errors
    card->card_num = i;

    // get the card and the original string length
    line_len = strlen(card->orig_str);
    
    /* copy out the card code */
    // onec comment markers can be one char, so we have a special case here
    if(line_len == 1) {
      strncpy(type_buff, card->orig_str, 1);
    } else {
      strncpy(type_buff, card->orig_str, 2);
    }
    // capitalize it
    type_buff[0] = toupper(type_buff[0]);
    type_buff[1] = toupper(type_buff[1]);
    // null the end
    type_buff[2] = '\0';
	
    // the code might only be one char, but if there's a comment following it then strlen>1, so...
    if(isspace(type_buff[1])) {
      type_buff[1] = '\0';
    }
    // and copy it over
    strncpy(card->card_code, type_buff, 2);
    
    /* see if we can find out what sort of card it is */
    isCmt = isComment(card);
    isGeo = isGeometry(card);
    isCtl = isControl(card);
    isExt = isExtension(card);

    /* while we loop, we want to keep track of key points in the deck
     * which will make it easier to work with in other parts of the code.
     * for instance, we need to know where the EN card is, if we find
     * one, because anything after that is automatically a comment. it's
     * also handy to know where the geometry and comments sections
     * start and end.
     *
     * note that we can't simply update the pointers every time we see
     * one of the cards in a certain set, because you might have a CM
     * card outside the comment section, for instance. This is not the
     * case in a "real" NEC2 file, but widely allowed by practically
     * every system.
     */
    
    // so, for instance, if this is the first CM card we've seen, and we
    // have NOT seen a CE or any Gx card, then this appears to be the
    // start of the comment section. but it's not if we've seen anything
    // else, like geometry.
    if(strcasecmp(type_buff, "CM") == 0 && !sawCM && !sawCE && !sawGx && !sawGE && !sawEN) {
      deck->comment_start = i;
      sawCM = TRUE;
    }
    // the CE case is similar, it has to be above any geometry
    if(strcasecmp(type_buff, "CE") == 0 && !sawCE && !sawGx && !sawGE && !sawEN) {
      deck->comment_end = i;
      sawCE = TRUE;
    }
    // if this is the first geo card, including GE...
    if(isGeo && !sawGx && !sawEN) {
      deck->geometry_start = i;
      sawGx = TRUE;
    }
    // the GE just has to be above the end of the deck
    if(strcasecmp(type_buff, "GE") == 0 && !sawGE && !sawEN) {
      deck->geometry_end = i;
      sawGE = TRUE;
    }
    // and finally, the first EN that we see
    if(strcasecmp(type_buff, "EN") == 0) {
      deck->deck_end = i;
      sawEN = TRUE;
    }
    
    /* another special case: if this is a comment card but the next two characters
     * are one of the extensions, this is really a "hidden extension" that is being
     * used to make the deck compatible with older NEC programs. In that case, we
     * want to change the card to an extension type, make it not a comment, and save
     * the comment marker in the extn. note that this could only be the case if the
     * card has at least three characters, which would assume something like !SY
     */
    if(isCmt && line_len > 3) {
      // skip forward to find anything after the comment marker
      int pos = 0;
      if(strcasecmp(type_buff, "CM") == 0 || strcasecmp(type_buff, "CE") == 0)  {
        pos = 2;
      } else if (strcasecmp(type_buff, "!") == 0 || strcasecmp(type_buff, "#") == 0|| strcasecmp(type_buff, "'") == 0) {
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
      
      // get the rest of the string after the comment marker
      
      // get the two characters *after* the comment marker
      if((strcasecmp(type_buff, "CM") == 0 || strcasecmp(type_buff, "CE") == 0) && line_len > 4) {
        strncpy(hidden_type_buff, &card->orig_str[2], 2);
      } else if (strcasecmp(type_buff, "!") == 0 || strcasecmp(type_buff, "#") == 0|| strcasecmp(type_buff, "'") == 0) {
        strncpy(hidden_type_buff, &card->orig_str[1], 2);
      } else {
        // we didn't find anything interesting
        strcpy(hidden_type_buff, "");
      }
      // now we see if those two characters are one of the extensions
      bool isHidden = FALSE;
      for(int i = 0; i < NUM_ONEC_CODES; i++) {
        if(strcasecmp(hidden_type_buff, onec_codes[i]) == 0) { // was card->card_code in the front?
          isHidden = TRUE;
          break;
        }
      }
      if(isHidden) {
        isCmt = FALSE;
        isExt = TRUE;
        card->extn_code[0] = '!';
      }
    } // checking for hidden info
    
    /* if we're past the end of the deck, everything that appears is a comment,
     * and we'll just copy it into the comment. but if we are not past the end,
     * and we didn't recognize the type then we want to report an error
     */
    if (!sawEN && !isCmt && !isCtl && !isGeo && !isExt) {
      // make a string for the message
      char *msg = calloc(1, MAX_ERROR_LEN);
      sprintf(msg, "Unknown card type '%s' encountered on card %d. Card will not be processed.", type_buff, i);
      add_error(errors, msg, 0);
      free(msg);
    }
    
    /* if we did figure out the card type, then we want to put something in the card_str,
     * but first we want to see if there is a comment inside the line ( > 0, < len ) and
     * clip that part out separately into extn_str
     */
    if(isCmt || isCtl || isGeo || isExt) {
      size_t len; // this is the length of the main card text
      // look for a comment marker, adjust length of card text based on that
      const char *sep = strpbrk(card->orig_str, ONEC_COMMENTS);
      if(sep == NULL) {
        // no comment was found, put everything into the string
        len = strlen(card->orig_str);
      } else {
        // comment was found at location sep
        len = sep - card->orig_str;
      }
      // malloc room for the card part, copy that in, and close the string
      card->card_str = malloc((len * sizeof(char)) + 1);
      strncpy(card->card_str, card->orig_str, len);
      card->card_str[len] = '\0';
      // and if there was any leftover, copy it into the extension, otherwise make sure its empty
      if(sep == NULL) {
        card->extn_str = NULL;
        card->extn_code[0] = '\0';
      } else {
        card->extn_str = strdup(sep + 1);
        card->extn_code[0] = sep[0];
      }
    }
    
    /* if we did find a comment marker in this line, and the deck doesn't have a
     * default marker set, assume this is the one used in the entire file and make
     * it the default. You can still use other markers on other lines, but if you
     * add a new card programmatically and set a comment, it should default to using
     * this marker
     */
    if(card->extn_code[0] != 0 && deck->cmt_code[0] == 0) {
      deck->cmt_code[0] = card->extn_code[0];
    }
    
    // now call the card parsers on the different card types
    if(isCmt) {
      parse_comment_card(card, errors);
    }
    if(isGeo) {
      parse_geometry_card(card, errors);
    }
    if(isCtl) {
      parse_command_card(card, errors);
    }
    if(isExt) {
      parse_onec_card(card, errors);
    }
    
    // process inline comments to look for key/value pairs
    // this can apply to any card, even comments
    if(card->extn_code[0] != '\0') {
      parse_key_values(card, errors);
    }
  } // foreach card
} /* end of parse_deck() */

/*----------------------------------------------------------------------*/
/* parse_comment_card()
 *
 * copies the comment from the card_str into the comment string so that
 * it can be updated there. This is not used to process inline comments,
 * that happened in the extension parser before we got here
 */
void parse_comment_card(Card *card, Errors *errors)
{
  // look for the different comment markers in the card_code and then
  // just copy everything else on the line to the comment. this
  // includes any leading whitespace, or lack of it
  int code_end;
  if(strcasecmp(card->card_code, "CM") == 0) {
    code_end = 2;
  } else if(strcasecmp(card->card_code, "CE") == 0) {
    code_end = 2;
  } else if(strcasecmp(card->card_code, "!") == 0) {
    code_end = 1;
  } else if(strcasecmp(card->card_code, "#") == 0) {
    code_end = 1;
  } else if(strcasecmp(card->card_code, "\'") == 0) {
    code_end = 1;
  } else {
    code_end = 0; // error case, shouldn't be able to happen
  }
  card->comment = calloc(strlen(card->orig_str) - code_end, sizeof(char));
  strcpy(card->comment, &card->card_str[code_end]);
}

/*----------------------------------------------------------------------*/
/* parse_command_card()
 *
 * parses the contents of one command card. formerly readem()
 *
 * The original nec2C code was invoked with a big list of byref variables,
 * read the card, parsed it, and passed everything back through the byrefs
 * to the main loop for processing. This version takes the card, parses
 * the card_str part, and then stores everything in the card's internal
 * variables.
 *
 */
void parse_command_card(Card *card, Errors *errors)
{
  int MAX_INT = 4, MAX_FLOAT = 6; // maximum number of integers on a line, max number of floats
  char* end_ptr;

	// get line length of the card part of the line
	size_t line_len = strlen(card->card_str);

  // calloc has zeroed everything, so if this card doesn't have any parameters,
  // like a CE or GM, just return now
  if(line_len <= 2) return;

  // skip the first two chars, the mnemonic is still there
  char *str = card->card_str + 2; //strdup(card->card_str + 2);

  // process up to four ints at the start of the line
  end_ptr = NULL;
  int ints_processed = 0;
  while(str <= card->card_str + line_len && ints_processed < MAX_INT) {
    // try to process this as a number
    size_t value = strtol(str, &end_ptr, 10);
    
    // if that returned nothing and we've reached the end of the line, exit
    if(value == 0L && end_ptr == str) break;
    
    // otherwise move forward to the new point
    str = end_ptr ;

    // and put the value in the right place
    ints_processed++;
    switch(ints_processed) {
      case 1:
        card->i1 = (int)value;
        break;
      case 2:
        card->i2 = (int)value;
        break;
      case 3:
        card->i3 = (int)value;
        break;
      case 4:
        card->i4 = (int)value;
        break;
    }
  }
  
  // process up to six doubles following the ints
  end_ptr = NULL;
  int dbls_processed = 0;
  while(str <= card->card_str + line_len && dbls_processed < MAX_FLOAT) {
    // try to read another double on the line, and exit otherwise
    double value = strtod(str, &end_ptr);
    if(value == 0L && end_ptr == str) break;
    str = end_ptr ;

    // if we got a value, put it into the right slot
    dbls_processed++;
    switch(dbls_processed) {
      case 1:
        card->f1 = value;
        break;
      case 2:
        card->f3 = value;
        break;
      case 3:
        card->f3 = value;
        break;
      case 4:
        card->f4 = value;
        break;
      case 5:
        card->f5 = value;
        break;
      case 6:
        card->f6 = value;
        break;
    }
  }
} /* end of parse_command_card() */

/*----------------------------------------------------------------------*/
/* parse_geometry_card()
 *
 * parses the contents of one geometry card. formerly ???()
 *
 * The main difference between this code and the original nec2c code is
 * the addition of parsers for measurement units and formulas.
 *
 */
void parse_geometry_card(Card *card, Errors *errors)
{
  int MAX_INTS = 2, MAX_FLOATS = 7;
  int ints_processed = 0;
  int dbls_processed = 0;
  char *token;
  char *end_ptr;      // end point of a a number as we parse them
  size_t int_value;   // used to parse ints...
  double dbl_value;   // ... and doubles
  char unit_code[MAX_UNIT_LEN]; // the unit code string (if any) found on this line
  int unit;                     // ...and our internal code for that unit if we found it, or 0 for default
  bool isFormula;               // was the double actually a formula?

  // get line length of the card part of the line
  int line_len = (int)strlen(card->card_str);
  
  // calloc has zeroed everything we need to set, so if this card doesn't
  // have any parameters, like a GM, just return now
  if(line_len <= 2) return;
  
  // skip the first two chars, the mnemonic is still there
  // we'll use this as the pointer to the current start location
  char *str = card->card_str + 2;
  
  // and also skip any leading whitespace or separator characters
  str += strspn(str, ONEC_WHITESPACE);
  
  // tokenize the rest of the line on the remaining whitespace
  token = strtok(str, ONEC_WHITESPACE);
  while(token != NULL) {
    // we have a non-zero length token, which might be an int
    // or float depending on what we've seen before
    if(ints_processed < MAX_INTS) {
      ints_processed++;
      
      // parse a number if we can find it
      int_value = strtol(token, &end_ptr , 10);
      
      // if there was a number in there, end_ptr will no longer be at the
      // start and that means we found one and can store it in the right slot
      if(end_ptr != token) {
        switch(ints_processed) {
          case 1:
            card->i1 = (int)int_value;
            break;
          case 2:
            card->i2 = (int)int_value;
            break;
        }
      }
    }
    
    // doubles are more complicated because the fields may contain other
    // bits like measurement indicators like "mm" or have formulas in them
    // the code assumes that it's a number until proven wrong. if it is, we
    // set isFormula
    //
    else if(dbls_processed < MAX_FLOATS) {
      dbls_processed++;
      
      isFormula = FALSE;  // assume it's a number until proven otherwise
      unit = 0;           // if we don't find a unit code, this will ensure it is set to "default"

      // look for a leading # indicating an awg measurement from 4nec2
      // FIXME: this doesn't currently work because we trim off everything after the # above
      if(token[0] == '#') {
        // see if we can find that code
        for(int i = 0; i < NUM_ONEC_UNIT_CODES; i++) {
          if(strcasecmp(unit_code, unit_codes[i]) == 0) {
            unit = i;
            break;
          }
        }
        // and then move forward to skip it
        str += 1;
      }
      
      // try to read a double in the token, which has to be at the start or
      // strtod fails - this is what we want, in case someone does 'freq/2'
      dbl_value = strtod(token , &end_ptr);
      if(end_ptr != token) {
        // we got a number at the front, but is that all we got?
        char *leftover = end_ptr;
        
        if(strlen(leftover) > 0) {
          // check to see if the leftover is one of our known units
          bool isUnit = false;
          for(int i = 0; i < NUM_ONEC_UNIT_CODES; i++) {
            if(strcasecmp(leftover, unit_codes[i]) == 0) {
              unit = i;
              isUnit = true;
              break;
            }
          }
          // if it was not a unit, and it wasn't zero length, we have to
          // assume the entire thing was a formula
          if(!isUnit) {
            isFormula = true;
          }
        }
      } else {
        // we did not get a number at the front, so it must be a formula one way or the other
        isFormula = true;
      }
      
      // now we decide where to put it all...
      if(!isFormula) {
        // if it's not a formula, set the values and any unit we found
        switch(dbls_processed) {
          case 1:
            card->f1 = dbl_value;
            card->m1 = unit;
            break;
          case 2:
            card->f2 = dbl_value;
            card->m2 = unit;
            break;
          case 3:
            card->f3 = dbl_value;
            card->m3 = unit;
            break;
          case 4:
            card->f4 = dbl_value;
            card->m4 = unit;
            break;
          case 5:
            card->f5 = dbl_value;
            card->m5 = unit;
            break;
          case 6:
            card->f6 = dbl_value;
            card->m6 = unit;
            break;
          case 7:
            card->f7 = dbl_value;
            card->m7 = unit;
            break;
        }
      } else {
        // it is a formula, copy the entire token into the right formula field
        switch(dbls_processed) {
          case 1:
            card->ff1 = token;
            break;
          case 2:
            card->ff2 = token;
            break;
          case 3:
            card->ff3 = token;
            break;
          case 4:
            card->ff4 = token;
            break;
          case 5:
            card->ff5 = token;
            break;
          case 6:
            card->ff6 = token;
            break;
          case 7:
            card->ff7 = token;
            break;
        }
      } // isFormula = true
    } // dbls_processed < MAX_FLOATS
    
    // and move on to the next bit
  NEXT_TOKEN:
    token = strtok(NULL, ONEC_WHITESPACE);
  } //token != NULL
} /* end of parse_geometry_card() */

/*----------------------------------------------------------------------*/
/* parse_onec_card()
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
void parse_onec_card(Card *card, Errors *errors)
{
  // see if this is an SY card, otherwise exit
  // TODO: add all onec_codes here, we currently only do SY
  if(strcasecmp(card->card_code, "SY") == 0) {
    // make a copy of the string so we can mangle it - DO WE NEED TO?
    char str[MAX_LINE_LEN];
    strcpy(str, card->card_str + 2);

    // SY allows only a comma as a delimeter
    char *token, *split;
    token = strtok(str, " ,");
    while(token != NULL) {
      // make sure there's a equals in it
      split = strpbrk(token, "=");
      if(split != NULL) {
        // remove any whitespace off the front, which might be trailing the comma
        while(isspace((unsigned char)*token)) token++;
        //TODO: do the same on the end of the string, if they put space comma

        // if the split was successful, meaning we found the = somewhere,
        // the = is still on the front so let's kill it
        if (split[0] == '=') split++;

        // now check that both sides are >0 len
        if(strlen(token) > 0 && strlen(split) > 0) {
          // if so, make a new keyvalue pair and add it to the card's collection
          KeyValue *pair = (KeyValue *)malloc(sizeof(KeyValue));
          if(pair != NULL) {
            // calloc the strings and store them...
            pair->key = calloc(split - token, sizeof(char));
            strncpy(pair->key, token, split - token - 1);
            pair->value = calloc(strlen(split) + 1, sizeof(char)); // add one for a trailing null
            strcpy(pair->value, split);
            // and store the original separator so we can recreate it on output
            pair->separator = token[split - token];
            // and null this out, as its going on the end
            pair->next = NULL;
            // and then add it to the end of the list
            KeyValue *tail = card->formulas;
            if(tail == NULL) {
              card->formulas = pair;
            } else {
              while(tail->next != NULL) tail = tail->next;
              tail->next = pair;
            }
          } // there should be else's for all the mallocs and callocs!
        }
      } // split != NULL
      // and get the next token
      token = strtok(NULL, ONEC_WHITESPACE ONEC_SEPARATORS);
    }
  }
} /* end of parse_onec_card() */

/*----------------------------------------------------------------------*/
/* parse_key_values()
 *
 * parses a string that may contain key/value pairs
 *
 * also looks for "comment:" markers within the string, and assumes
 * everything that marker is a comment
 *
 */
void parse_key_values(Card *card, Errors *errors)
{
  char str[MAX_LINE_LEN];
  char key[MAX_LINE_LEN], value[MAX_LINE_LEN];
  
  // track whether we found any onec extensions after the comment
  // marker. if we didn't, everything after the marker is a
  // comment all on its own
  bool hasExtensions = false;

  // make a copy of the string so we can mangle it - DO WE NEED TO?
  strcpy(str, card->extn_str);
  
  // strtok should be perfect for this one because we don't want the delimiters to be handed back
  // ...so start by priming the strtok pump
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
      strncpy(key, token, split - token - 1);
      strcpy(value, split);
      
      // now check that both sides are >0 len, otherwise skip this one
      if(strlen(key) > 0 && strlen(value) > 0)
        goto NEXT_TOKEN;

      // getting here means we did find a valid key/value of some
      // sort, so record that we got it
      hasExtensions = true;
      
      // there are a couple of cases here:
      // 1) the key is "name" or "group", which are stored separately
      // 2) the key is "comment" - copy everything after it into comment string and exit
      // 3) the key is a formula - make a KeyValue pair and add it to the formulas list
      // 4) the key is anything else - make a KeyValue pair and add it to the pairs list
      
      // handle "name" or "group"
      if(strcasecmp(key, "name") == 0) {
        card->name = value;
        goto NEXT_TOKEN;
      }
      if(strcasecmp(key, "group") == 0) {
        card->group = value;
        goto NEXT_TOKEN;
      }
      // and comments, which cause us to exit
      if(strcasecmp(key, "comment") == 0) {
        card->comment = value;
         return;
      }
      
      // now see if it's a formula
      bool isFormula = false;
      for(int i = 0; i < NUM_FIELD_NAMES; i++) {
        if(strcasestr(key, field_names[i]) != NULL) {
          isFormula = true;
          break;
        }
      }
      
      // formulas and any other tags not pulled out above are handled,
      // in the same fashion we make a KeyValue pair to hold it.
      // They differ only in where we put them in the end
      KeyValue *pair = (KeyValue *)malloc(sizeof(KeyValue));
      if(pair != NULL) {
        // calloc the strings and store them...
        pair->key = calloc(split - token, sizeof(char));
        strncpy(pair->key, token, split - token - 1);
        pair->value = calloc(strlen(split) + 1, sizeof(char)); // add one for a trailing null
        strcpy(pair->value, split);
        // and store the original separator so we can recreate it on output
        pair->separator = token[split - token];
        // and null this out, as its going on the end
        pair->next = NULL;
        
        // now decide which list to add it to
        KeyValue *head, *tail;
        if(isFormula) {
          head = card->formulas;
        } else {
          head = card->pairs;
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
      } // there should be else's for all the mallocs and callocs!
    } // split != NULL
    
  NEXT_TOKEN:
    // and get the next token
    token = strtok(NULL, ONEC_WHITESPACE ONEC_SEPARATORS);
  }
  
  // we are done processing this line, now see if there were any extensions
  // at all, if there weren't, then this is a pure comment line and we
  // know there's *something* here because otherwise we exited above
  if(!hasExtensions) {
    card->comment = str;
  }
  
} /* end of parse_key_values() */
