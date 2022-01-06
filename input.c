/*******************************************************************
 * input.c
 *
 * input.c contains the routines needed to read and parse a card
 * deck file, normally ".nec" or ".deck". The process starts with
 * read_deck(), which uses read_line() to get the data from one
 * line in the file and then turn it into a Card. When all the
 * lines are read into Cards, read_deck() then calls a series of
 * functions to parse the deck into parts, mostly using the logic
 * found in parse_command_card() or parse_geometry_card().
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
 * Other changes to this code include a wider set of comment
 * markers, including CM, !, # and ', whereas nec2c only accepted
 * # outside the comment header. Additionally, this code looks for
 * comment markers *in* a line, and splits that data out to a
 * separate buffer for processing out the (potential) OpenNEC
 * extensions. It keeps track of what the original comment marker
 * was so it can save it back out in the same format.
 *
 * There is one remaining problem. Because the .nec files are
 * normally edited by hand, it is not entirely unknown to find a
 * file where the text editor has inserted hard-line-wraps. This
 * causes some of the data to appear on a separate line, "splitting"
 * the card. There are a variety of ways to solve this in most
 * cases, but one requires some thought: if the split is inserted
 * at the point where a trailing comment appears, the comment will
 * be put onto a line by itself. If the line above it does not have
 * a comment of its own, its not immediately possible to tell if this
 * is a wrapped comment or one that was always on its own line.
 * Looking for colons, the onec field separator, may help. The
 * remaining case, a comment on a line by itself with no onec data
 * might be perfectly acceptable, and likely extremely rare.
 *
 *******************************************************************/

#include "opennec.h"
#include "shared.h"

// TODO:
// - add a case where if the leading char(s) is a comment marker, read the next two chars
//   and see if that's one of the extensions. this will allow the extensions to be hidden
//   in a comment
//
// - what do we do if there is more than one GS, or the GS isn't at the end of the geometry?
//   it seems this is a legal thing, and that we should rescale everything above that point?

/* forward declares */
void parse_card(Card *card, int card_num, Errors *errors);

/*----------------------------------------------------------------------*/
/* read_deck()
 *
 * Reads the entire deck line by line and fills out the decks's cards[]
 * array with the resulting data. After this completes, the caller should
 * call parse_deck() to process the data.
 *
 */
void read_deck(Deck *deck, FILE *pfile)
{
  Card *card = NULL;  // the card we're working on, have to null it or
                      // the Free() below may fail when this contains garbage

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
 * The only oddity here is some code that handles split lines. The
 * original NEC format was on 80-column punch cards, but that is no
 * longer recognized as a limit. Some decks spill over 80-chars, and
 * then the text editors insert a return at the 80-char mark. This is
 * clearly wrong, but seems like it might be common in the wild. This
 * code looks for such cases and merges the lines into a single card.
 * This means that if one simply loads and then saves the deck, the
 * split lines will be removed.
 *
 * This code also automatically capitalizes the first two characters
 * on the line.
 *
 */
int read_line(char *buff, FILE *file)
{
  int
    num_chr = 0, // number of characters read, excluding lf/cr
    eof = 0, // EOF flag
    chr;     // character read by getc
  
  /* clear buffer at start */
  buff[0] = '\0';
  
  // if we're at the end of the file, just return that
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
  } /* end of while( (chr == '#') || ... */
  
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
  } /* end of while( num_chr < max_chr ) */
  
  /* capitalize first two characters (mnemonic) */
  if((buff[0] > 0x60) && (buff[0] < 0x79))
    buff[0] -= 0x20;
  if((buff[1] > 0x60) && (buff[1] < 0x79))
    buff[1] -= 0x20;
  
  /* terminate buffer as a string */
  buff[num_chr] = '\0';
  
  return(eof);
} /* end of read_line() */

/*----------------------------------------------------------------------*/
/* parse_deck()
 *
 * parses the original data from the file once its all read in
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
  int isCmt, isGeo, isCtl, isExt; // boolean so we can do the string compare only once
  /* the following are used to keep track of various key points in the deck */
  int sawCM = FALSE, sawCE = FALSE, sawGx = FALSE, sawGE = FALSE, sawEN = FALSE;

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
    // null the end
    type_buff[2] = '\0';
	
    // the code might only be one char, but if there's a comment following it there strlen>1, so...
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
     * one, because anything after that is automatically a comment. its
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
    if(strcmp(type_buff, "CM") == 0 && !sawCM && !sawCE && !sawGx && !sawGE && !sawEN) {
      deck->comment_start = i;
      sawCM = TRUE;
    }
    // the CE case is similar, it has to be above any geometry
    if(strcmp(type_buff, "CE") == 0 && !sawCE && !sawGx && !sawGE && !sawEN) {
      deck->comment_end = i;
      sawCE = TRUE;
    }
    // if this is the first geo card, including GE...
    if(isGeo && !sawGx && !sawEN) {
      deck->geometry_start = i;
      sawGx = TRUE;
    }
    // the GE just has to be above the end of the deck
    if(strcmp(type_buff, "GE") == 0 && !sawGE && !sawEN) {
      deck->geometry_end = i;
      sawGE = TRUE;
    }
    // and finally, the first EN that we see
    if(strcmp(type_buff, "EN") == 0) {
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
    if (isCmt && line_len > 3) {
      // skip forward to find anything after the comment marker
      int pos = 0;
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
        //
      }

      //get the rest of the string after the comment marker
    
    
     // get the two characters *after* the comment marker
     if((strcmp(type_buff, "CM") == 0 || strcmp(type_buff, "CE") == 0) && line_len > 4) {
       strncpy(hidden_type_buff, &card->orig_str[2], 2);
     } else if (strcmp(type_buff, "!") == 0 || strcmp(type_buff, "#") == 0|| strcmp(type_buff, "'") == 0) {
       strncpy(hidden_type_buff, &card->orig_str[1], 2);
     } else {
       // we didn't find anything interesting
       strcpy(hidden_type_buff, "");
     }
     //now we see if those two characters are one of the extensions
     int isHidden = FALSE;
     for(int i = 0; i < NUM_ONEC_CODES; i++) {
       if(strcmp(hidden_type_buff, onec_codes[i]) == 0) { // was card->card_code in the front
         isHidden = TRUE;
         break;
       }
     }
     if (isHidden) {
       isCmt = FALSE;
       isExt = TRUE;
       card->extn_code[0] = '!';
     }
  }

    /* if we're past the end of the deck, don't do anything, just keep it in the orig_card
     * but if we're still inside the deck and we don't recognize the card, make an error
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
      const char *sep = strpbrk(card->orig_str, "!'#");
      if (sep == NULL) {
        // no comment was found
        card->card_str = malloc(strlen(card->orig_str) + 1);
        strcpy(card->card_str, card->orig_str);
        card->extn_str = NULL;
      } else {
        // comment was found at sep
        size_t len = sep - card->orig_str;
        char *p = malloc(len + 1);
        if (p != NULL) {
          memcpy(p, card->orig_str, len);
          p[len] = '\0';
          card->card_str = p;
          card->extn_str = strdup(sep);
          card->extn_code[0] = card->extn_str[0];
        }
      }
    }
    
    /* if we did find a comment marker in this line, and the deck doesn't have a
     * default marker set, assume this is the one used in the entire file and make
     * it the default. You can still use other codes on other lines, but if you
     * add a new card with a comment, it will default to using this marker on that card.
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
    
    // and finally, if there is an extension, parse it into a comment or key:value pairs
    // TODO

  } // foreach card
} /* end of parse_card() */

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
  // just copy everything else on the line to the comment
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
  card->comment = calloc(strlen(card->card_code) - code_end, sizeof(char));
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
	int nint = 4, nflt = 6; // maximum number of integers on a line, max number of floats
  char* end_ptr;

	// get line length of the card part of the line
	size_t line_len = strlen(card->card_str);

  // calloc has zeroed everything, so if this card doesn't have any parameters,
  // like a CE or GM, just return now.
  if(line_len <= 2) return;

  // skip the first two chars, the mnemonic is still there
  char *str = card->card_str + 2; //strdup(card->card_str + 2);

  // process up to four ints at the start of the line
  end_ptr = NULL;
  int ints_processed = 0;
  while(str <= card->card_str + line_len && ints_processed < nint) {
    size_t value = strtol(str , &end_ptr , 10);
    if(value == 0L && end_ptr == str) break;
    str = end_ptr ;

    ints_processed++;
    switch(ints_processed) {
      case 1:
        card->i1 = (int)value;
        break;
      case 2:
        card->i2 =  (int)value;
        break;
      case 3:
        card->i3 =  (int)value;
        break;
      case 4:
        card->i4 =  (int)value;
        break;
    }
  }
  
  // process up to six doubles following the ints
  end_ptr = NULL;
  int dbls_processed = 0;
  while(str <= card->card_str + line_len && dbls_processed < nflt) {
    // try to read another double on the line, and exit otherwise
    double value = strtod(str , &end_ptr);
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
  int nint = 2, nflt = 7; // maximum number of integers on a line, max number of floats
  char* end_ptr;
  
  /* get line length of the card part of the line */
  int line_len = (int)strlen(card->card_str);
  
  // calloc has zeroed everything, so if this card doesn't have any parameters,
  // like a GM, just return now.
  if(line_len <= 2) return;
  
  // skip the first two chars, the mnemonic is still there
  char *str = card->card_str + 2;//strdup(card->card_str + 2);

  // process up to four ints at the start of the line
  end_ptr = NULL;
  int ints_processed = 0;
  while(str <= card->card_str + line_len && ints_processed < nint) {
    // try to read an int on the line, and exit otherwise
    size_t value = strtol(str, &end_ptr , 10);
    if(value == 0L && end_ptr == str) break;
    str = end_ptr ;

    // if we got a value, put it into the right slot
    ints_processed++;
    switch(ints_processed) {
      case 1:
        card->i1 = (int)value;
        break;
      case 2:
        card->i2 = (int)value;
        break;
    }
  } // end while(str <= card->card_str...
  
  /* process up to seven doubles following the ints
   *
   * unlike cmd cards, geometry cards may also include a measurement unit
   * immediately after the number. There is code at the bottom of this loop
   * to test for this case.
   *
   */
  end_ptr = NULL;
  int dbls_processed = 0;
  int unit;
  char unit_code[MAX_UNIT_LEN];
  size_t pos;
  while(str <= card->card_str + line_len && dbls_processed < nflt) {
    // try to read another double on the line, and exit otherwise
    double value = strtod(str , &end_ptr);
    if(value == 0L && end_ptr == str) break;
    str = end_ptr;
    
    /* now see if there is a potential measurement unit in the line, which we do simply
     * by seeing if the next character is a whitespace or comma. If it's not, we read until
     * we find one of those, and then look at the resulting string to see if we can match
     * it with one of our units. Othewise report an error.
     */
    unit = 0; // if we don't find a unit code, this will ensure it is set to "default"
    if(!isspace(str[0])) {
      pos = strcspn(str, OUR_WHITESPACE); // you need the \0, but \n\r should have been pulled already
      if(pos > 0) {
        // copy out the code and then move up in the string
        memset(unit_code, '\0', sizeof(unit));
        strncpy(unit_code, str, pos);
        str += pos;
        
        // see if we can find that code
        for(int i = 0; i < NUM_UNIT_CODES; i++) {
          if(strcmp(unit_code, unit_codes[i]) == 0) {
            unit = i;
            break;
          }
        }
        
        /* test to see if we got a code we recognize */
        if(unit < 1) {
          char *msg = calloc(1, MAX_ERROR_LEN);
          sprintf(msg, "Unknown unit type '%s' encountered in float field %d of card %d. Units left as 'default'.", unit_code, dbls_processed, card->card_num);
          add_error(errors, msg, 0);
          free(msg);
        }
      }
    }

    // if we got a value, put it into the right slot, along with any units
    dbls_processed++;
    switch(dbls_processed) {
      case 1:
        card->f1 = value;
        card->m1 = unit;
        if(unit)
        break;
      case 2:
        card->f2 = value;
        card->m2 = unit;
        break;
      case 3:
        card->f3 = value;
        card->m3 = unit;
        break;
      case 4:
        card->f4 = value;
        card->m4 = unit;
        break;
      case 5:
        card->f5 = value;
        card->m5 = unit;
        break;
      case 6:
        card->f6 = value;
        card->m6 = unit;
        break;
      case 7:
        card->f7 = value;
        card->m7 = unit;
        break;
    }
  }
} /* end of parse_geometry_card() */

/*----------------------------------------------------------------------*/
/* parse_onec_card()
 *
 * parses cards only understood by onec, which at this point is only
 * the SY. Although XT is also part of the onec list, there's nothing to
 * do in that case as it has no parameters.
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
  if(strcmp(card->card_code, "SY") == 0) {
    char *token, *name, *value, *split;
    
    // make a copy of the string so we can mangle it, and cut off the card code at the front
    char *str = malloc(strlen(card->orig_str) * sizeof(char));
    strcpy(str, card->card_str + 2);
    
    // strtok should be perfect for this one, because we don't want the delimiters to be handed back
    // so start by priming the pump
    token = strtok(str, OUR_WHITESPACE);
    while(token != NULL) {
      // make sure there's a equals or colon in it - note you can't nest another strtok!
      split = strstr(token, "=");
      if(split != NULL) {
        // we found the equals, so we have a pair, so set it up
        
        printf("token=%s, split=%s\n", token, split);
        
      } else {
        char *msg = calloc(1, MAX_ERROR_LEN);
        sprintf(msg, "SY definition '%s' on card %d has no '=' separator. Variable definition ignored.", token, card->card_num);
        add_error(errors, msg, 0);
        free(msg);
      }
      
      // and get another token
      token = strtok(NULL, str);
    }
//    free(token);
//    free(name);
//    free(value);
//    free(split);
  }
} /* end of parse_onec_card() */


///*-----------------------------------------------------------------------*/
///* this is used to read a geometry card, as opposed to "general" cards
// * the main difference is the number of ints and floats
// */
//void read_geometry_card( char *gm, int *i1, int *i2, double *x1, double *y1,
//                        double *z1, double *x2, double *y2, double *z2, double *rad )
//{
//  char line_buf[MAX_LINE_LEN];
//  int line_length, i, line_idx;
//  int nint = 2, nflt = 7;
//  int iarr[2] = { 0, 0 };
//  double rarr[7] = { 0., 0., 0., 0., 0., 0., 0. };
//
//  /* read a line from input file */
//  read_line( line_buf, input_fp );
//
//  /* get line length */
//  line_length = (int)strlen( line_buf );
//
//  /* abort if card's mnemonic too short or missing */
//  if( line_length < 2 ) {
//    fprintf( output_fp,
//            "\n  GEOMETRY DATA CARD ERROR:"
//            "\n  CARD'S MNEMONIC CODE TOO SHORT OR MISSING." );
//    stop(-1);
//  }
//
//  /* extract card's mnemonic code */
//  strncpy( gm, line_buf, 2 );
//  gm[2] = '\0';
//
//  /* Exit if "XT" command read (for testing) */
//  if( strcmp( gm, "XT" ) == 0 ) {
//    fprintf( stderr,
//            "\nnec2c: Exiting after an \"XT\" command in read_geometry_card()\n" );
//    fprintf( output_fp,
//            "\n\n  nec2c: Exiting after an \"XT\" command in read_geometry_card()" );
//    stop(0);
//  }
//
//  /* Return if only mnemonic on card, like GS */
//  if( line_length == 2 ) {
//    *i1 = *i2 = 0;
//    *x1 = *y1 = *z1 = *x2 = *y2 = *z2 = *rad = 0.;
//    return;
//  }
//
//  /* read integers from line */
//  line_idx = 1;
//  for( i = 0; i < nint; i++ ) {
//    /* Find first numerical character */
//    while( ((line_buf[++line_idx] < '0')  ||  (line_buf[  line_idx] > '9')) &&
//          (line_buf[  line_idx] != '+')  &&
//          (line_buf[  line_idx] != '-') )
//      if( line_buf[line_idx] == '\0' ) {
//        *i1= iarr[0];
//        *i2= iarr[1];
//        *x1= rarr[0];
//        *y1= rarr[1];
//        *z1= rarr[2];
//        *x2= rarr[3];
//        *y2= rarr[4];
//        *z2= rarr[5];
//        *rad= rarr[6];
//        return;
//      }
//
//    /* read an integer from line */
//    iarr[i] = atoi( &line_buf[line_idx] );
//
//    /* traverse numerical field to next ' ' or ',' or '\0' */
//    line_idx--;
//    while(
//          (line_buf[++line_idx] != ' ') &&
//          (line_buf[  line_idx] != '\t') && // this was two spaces, which seems unlikely
//          (line_buf[  line_idx] != ',') &&
//          (line_buf[  line_idx] != '\0') ) {
//      /* test for non-numerical characters */
//      if( ((line_buf[line_idx] <  '0')  ||
//           (line_buf[line_idx] >  '9')) &&
//         (line_buf[line_idx] != '+')  &&
//         (line_buf[line_idx] != '-') ) {
//        fprintf( output_fp,
//                "\n  GEOMETRY DATA CARD \"%s\" ERROR:"
//                "\n  NON-NUMERICAL CHARACTER '%c' IN INTEGER FIELD AT CHAR. %d\n",
//                gm, line_buf[line_idx], (line_idx+1)  );
//        stop(-1);
//      }
//    } /* while( (line_buff[++line_idx] ... */
//
//    /* Return on end of line */
//    if( line_buf[line_idx] == '\0' ) {
//      *i1= iarr[0];
//      *i2= iarr[1];
//      *x1= rarr[0];
//      *y1= rarr[1];
//      *z1= rarr[2];
//      *x2= rarr[3];
//      *y2= rarr[4];
//      *z2= rarr[5];
//      *rad= rarr[6];
//      return;
//    }
//  } /* for( i = 0; i < nint; i++ ) */
//
//  /* read doubles from line */
//  for( i = 0; i < nflt; i++ )  {
//    /* Find first numerical character */
//    while( ((line_buf[++line_idx] <  '0')  || (line_buf[  line_idx] >  '9')) &&
//          (line_buf[  line_idx] != '+')  &&
//          (line_buf[  line_idx] != '-')  &&
//          (line_buf[  line_idx] != '.') )
//      if( line_buf[line_idx] == '\0' ) {
//        *i1= iarr[0];
//        *i2= iarr[1];
//        *x1= rarr[0];
//        *y1= rarr[1];
//        *z1= rarr[2];
//        *x2= rarr[3];
//        *y2= rarr[4];
//        *z2= rarr[5];
//        *rad= rarr[6];
//        return;
//      }
//
//    /* read a double from line */
//    rarr[i] = atof( &line_buf[line_idx] );
//
//    /* traverse numerical field to next ' ' or ',' or '\0' */
//    line_idx--;
//    while(
//          (line_buf[++line_idx] != ' ')  &&
//          (line_buf[  line_idx] != '\t') && // this was two spaces, but that is not correct
//          (line_buf[  line_idx] != ',')  &&
//          (line_buf[  line_idx] != '\0') ) {
//      /* test for non-numerical characters */
//      if( ((line_buf[line_idx] <  '0')  ||
//           (line_buf[line_idx] >  '9')) &&
//         (line_buf[line_idx] != '.')  &&
//         (line_buf[line_idx] != '+')  &&
//         (line_buf[line_idx] != '-')  &&
//         (line_buf[line_idx] != 'E')  &&
//         (line_buf[line_idx] != 'e') ) {
//        fprintf( output_fp,
//                "\n  GEOMETRY DATA CARD \"%s\" ERROR:"
//                "\n  NON-NUMERICAL CHARACTER '%c' IN FLOAT FIELD AT CHAR. %d.\n",
//                gm, line_buf[line_idx], (line_idx+1) );
//        stop(-1);
//      }
//    } /* while( (line_buff[++line_idx] ... */
//
//    /* Return on end of line */
//    if( line_buf[line_idx] == '\0' ) {
//      *i1= iarr[0];
//      *i2= iarr[1];
//      *x1= rarr[0];
//      *y1= rarr[1];
//      *z1= rarr[2];
//      *x2= rarr[3];
//      *y2= rarr[4];
//      *z2= rarr[5];
//      *rad= rarr[6];
//      return;
//    }
//  } /* for( i = 0; i < nflt; i++ ) */
//
//  *i1  = iarr[0];
//  *i2  = iarr[1];
//  *x1  = rarr[0];
//  *y1  = rarr[1];
//  *z1  = rarr[2];
//  *x2  = rarr[3];
//  *y2  = rarr[4];
//  *z2  = rarr[5];
//  *rad = rarr[6];
//
//  return;
//}
/*----------------------------------------------------------------------*/

