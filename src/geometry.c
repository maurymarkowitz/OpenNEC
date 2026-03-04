/******************************************************************************
 * geometry.c
 *
 * geometry.c contains the code that parses the geometry section of
 * the deck and then generates a list of segments, patches, and
 * connections. These are collected into a geometry_t structure for
 * the deck.
 *
 ******************************************************************************/

#include "internals.h"
#include "geometry.h"
#include "output.h"

/* Forward declarations for internal functions */
static void wire(nec_context_t *ctx, geometry_t *geom, int card_num, int tag_num, int segs, double xw1, double yw1, double zw1, double xw2, double yw2, double zsw2, double rad, double rdel, double rrad);
static void arc(nec_context_t *ctx, geometry_t *geom, int card_num, int tag_num, int segs, double rada, double ang1, double ang2, double rad);
static void helix(nec_context_t *ctx, geometry_t *geom, int card_num, int tag_num, int segs, double s, double hl, double a1, double b1, double a2, double b2, double rad, outputs_list_t *outputs);
static void patch(nec_context_t *ctx, geometry_t *geom, int card_num, int nx, int ny, double ax1, double ay1, double az1, double ax2, double ay2, double az2, double ax3, double ay3, double az3, double ax4, double ay4, double az4);
static void calculate_patch(nec_context_t *ctx, int nx, int ny);
static void reproduce(nec_context_t *ctx, double rox, double roy, double roz, double xs, double ys, double zs, int its, int nrpt, int itgi);
static void reflect(nec_context_t *ctx, int card_num, int tag_increment, int ix, int iy, int iz);
static void rotate(nec_context_t *ctx, int card_num, int tag_increment, int num_copies);
static void scale(nec_context_t *ctx, double xw1);
static int connect_segments(nec_context_t *ctx, int ignd, outputs_list_t *outputs);
static void finish_geometry(nec_context_t *ctx);

/******************************************************************************
 * peek_next_geom
 * 
 * Looks ahead in the deck to find the next geometry card, skipping any
 * continuation cards like SC or similar.
 * 
 */
static int peek_next_geometry(deck_t *deck, int current) {
  for (int j = current + 1; j <= deck->geometry_end; j++) {
    if (!is_extension(&deck->cards[j]) && !is_comment(&deck->cards[j])) return j;
  }
  return -1;
}

/******************************************************************************
 * calculate_geometry
 *
 * calculate_geometry (formerly datagn) is the main routine for creation
 * of geometry data. It reads the geometry cards, builds segments and patches,
 * and returns various errors. The resulting data, in ctx->geometry, can then
 * be used to draw a diagram of the antenna as well as being used in the
 * calculations.
 *
 * The list of errors is local to geometry, as this allows the various work
 * methods to add new entries without having to pass around an errors object.
 * It's likely useful to create a new errors object for every geometry, but
 * it's equally usable by passing in a global errors.
 *
 * @param ctx nec_context_t structure that will be modified
 * @param deck deck_t structure that has the geometry cards
 * @param errors a list of errors to add to
 *
 */
void calculate_geometry(nec_context_t *ctx, deck_t *deck, errors_list_t *errors, outputs_list_t *outputs)
{
  //(void)outputs;// currently unused
  card_t *card;
  char msg[MAX_ERROR_LEN];
  
  int code_num;   // geometry card code as a number
  int tag, segs;  // tag number (or zero) and number of segments to be added
  //int isct, iphd; // no longer used
  double rad, xs1, ys1, zs1, x4 = 0.0, y4 = 0.0, z4 = 0.0;
  double x3 = 0, y3 = 0, z3 = 0, xw1, xw2, yw1, yw2, zw1, zw2;
  int ix, iy, iz; // only used for reflection
  
  // set up the counters and flags
  ctx->geometry.symmetry_flag = 0;
  ctx->geometry.num_segs = 0;
  ctx->geometry.num_segs_sym = 0;
  ctx->geometry.num_patches = 0;
  ctx->geometry.num_patches_sym = 0;
  //isct = 0;     // this is "I am looking for an SC card", which we no longer need
  //iphd = false;	// this is "I printed the header", also not used
  
  // make sure there's cards to process
  // TODO: should this be an error/warning? or just in test?
  if(deck->num_cards == 0 || deck->geometry_start == -1 || deck->geometry_end == -1) {
    return;
  }
  
  // Symbol table and defaults were already initialized after parse_deck
  // Now we evaluate card formulas sequentially as we encounter cards
  
  // loop over the geometry section of the deck, which should be correct by this point
  for(int i = deck->geometry_start; i <= deck->geometry_end; i++) {
    card = &deck->cards[i];
    
    // Check if this is a SY card - if so, evaluate its formulas and continue to next card.
    // Ignored (commented-out) SY cards are skipped: they must not update the symbol table,
    // otherwise an ignored 'SY X=old that precedes an active SY X=new would shadow the
    // correct value when tinyexpr resolves 'X' (it picks the first binding it finds).
    if(strcmp(card->card_code, "SY") == 0) {
      if(!card->ignore && card->formulas) {
        key_value_t *kv = card->formulas;
        while (kv) {
          evaluate_formula(ctx, kv, deck, errors);
          kv = kv->next;
        }
      }
      continue; // Skip to next card, SY cards don't generate geometry
    }
    
    // one of the few ways that onec modifies the original NEC code is by adding
    // a flag saying whether this card should be ignored. That makes it easy to
    // have a GUI with a switch to turn off a card during testing (for example)
    // without having to physically remove it from the deck. this is not the same
    // as commenting it out, because the card is still read and parsed, and the
    // segments are in the geometry and can still be used in a GUI
    //
    // Commented-out cards (leading marker like '!' or ''') are skipped entirely —
    // they produce no geometry at all, not even in ignored_geometry.
    if (card_is_commented_out(card)) continue;

    // Invisible cards (annotated ignore=true but no leading marker) still generate
    // geometry, routed to ignored_geometry so the GUI can display them.
    geometry_t *target_geom = card_is_invisible(card) ? &ctx->ignored_geometry : &ctx->geometry;
    
    // convert the code into its numeric value so we can switch on it
    for(code_num = 0; code_num < NUM_GEOMETRY_CODES; code_num++) {
      if(strncmp(card->card_code, geometry_codes[code_num], 2) == 0) break;
    }
    
    // ignore SY and other extension cards in the geometry section
    // they were already evaluated in update_deck_values()
    if (is_extension(card)) continue;

    // now read in the values that are the same for all the cards
    // NOTE: remember to read the VALUES, not the original inputs!
    tag = card->i[1];
    segs = card->i[2];
    xw1 = card->f[1];
    yw1 = card->f[2];
    zw1 = card->f[3];
    xw2 = card->f[4];
    yw2 = card->f[5];
    zw2 = card->f[6];
    rad = card->f[7];
    
    // set the card's tag number and number of segments
    // Only set card->tag for card types that actually assign an ITG (tag)
    // to generated segments. Some geometry-like cards (GC, GN, GE, etc.)
    // use I1 for other purposes and should not be treated as tags.
    if (card_has_itag(card)) {
      card->tag = tag;
    } else {
      card->tag = 0;
    }
    card->num_segments = segs;
    
    // and now the switch. basically all this does is call the appropriate
    // function to insert the segments for that card type, or complete
    // processing when it sees the GE
    switch(code_num) {
      case 0: // GW, make a wire
        // the radius can be in the f7 field, or it can be on the next card if its tapered
        if(rad != 0.0) {
          xs1 = 1.0;
          ys1 = 1.0;
        } else {
          // make sure the next card is a GC, although we should have already done that
          int next_idx = peek_next_geometry(deck, i);
          if(next_idx == -1 || strcmp(deck->cards[next_idx].card_code, "GC") != 0) {
            snprintf(msg, sizeof(msg), "GW on line %d: has zero radius, but the next card is not a GC with the tapering info.", i + 1);
            add_error(ctx, errors, msg, WARNING);
            continue;
          }
          // and also that the values in it are valid
          // Use the GC card tapering info.
          card_t *gc_card = &deck->cards[next_idx];

            double gc_x1 = gc_card->f[1];
            // In many decks a GC F1 value of 0 means "no tapering of spacing";
            // treat 0 the same as 1 (equal spacing) to avoid producing zero
            // rd values that collapse segment lengths to zero.
            if (gc_x1 == 0.0) gc_x1 = 1.0;
            double gc_y1 = gc_card->f[2];
            double gc_z1 = gc_card->f[3];


          if((gc_y1 == 0.0) || (gc_z1 == 0.0)) {
            snprintf(msg, sizeof(msg), "GC on line %d: has tapering info for GW in card %d, but there is a zero in Y1 or Z1.", next_idx + 1, i + 1);
            add_error(ctx, errors, msg, WARNING);
            i = next_idx; // skip the invalid GC card
            continue;
          }

            // override the original inputs with the ones from the GC
            xs1 = gc_x1;
            ys1 = gc_y1;
            zs1 = gc_z1;
            rad = ys1;
            ys1 = pow((zs1 / ys1), (1.0 / (segs - 1.0)));

          

            // move up a card so we don't process the GC separately
            i = next_idx;
        }
        
        // update the number of wires and the segment counts
        // Use target_geom->num_segs so ignored cards track against ignored_geometry, not live geometry
        card->start_segment = target_geom->num_segs + 1;
        // now we have all the data, so turn it into segments
        wire(ctx, target_geom, i, tag, segs, xw1, yw1, zw1, xw2, yw2, zw2, rad, xs1, ys1);
        // and cache the final number
        card->end_segment = target_geom->num_segs;
        continue;
        
      case 1: // GX, reflect structure along x, y, or z axes, or rotate to form cylinder
        // the gx puts a three-digit integer value in the I2 slot, and then uses its digits
        // as bit flags for the x, y and z axes. here were pull them out...
        iy = segs / 10;
        iz = segs - iy * 10;
        ix = iy / 10;
        iy = iy - ix * 10;
        if(ix != 0) ix = 1;
        if(iy != 0) iy = 1;
        if(iz != 0) iz = 1;
        if (!card->ignore) {
          card->start_segment = ctx->geometry.num_segs + 1;
          reflect(ctx, i, tag, ix, iy, iz);
          card->end_segment = ctx->geometry.num_segs;
        }
        if (ctx->ignored_geometry.num_segs > 0 || ctx->ignored_geometry.num_patches > 0) {
          geometry_t _live = ctx->geometry;
          ctx->geometry = ctx->ignored_geometry;
          reflect(ctx, i, tag, ix, iy, iz);
          ctx->ignored_geometry = ctx->geometry;
          ctx->geometry = _live;
        }
        continue;
        
      case 2: // GR, rotate the structure
        // I2 is the number of times to duplicate the structure as it rotates
        // ix is set to -1 to indicate this is a rotation, not reflection
        if (!card->ignore) {
          rotate(ctx, i, tag, segs);
        }
        if (ctx->ignored_geometry.num_segs > 0 || ctx->ignored_geometry.num_patches > 0) {
          geometry_t _live = ctx->geometry;
          ctx->geometry = ctx->ignored_geometry;
          rotate(ctx, i, tag, segs);
          ctx->ignored_geometry = ctx->geometry;
          ctx->geometry = _live;
        }
        continue;
        
      case 3: // GS, scale structure dimensions by factor xw1
        if (xw1 == 0.0) {
          /*
           * Special-case handling: sometimes unit tokens (e.g. "in", "ft")
           * are separated by spaces and the preprocessor merges them into
           * the previous integer field (producing "0*in"). In that case the
           * float field may be empty but the original card contains the unit
           * as the third token. Try to recover the scale by parsing the raw
           * `card->card_str` (which contains the un-preprocessed card contents
           * after the mnemonic) and evaluating the third token as a standalone
           * formula (e.g. "in" -> 0.0254).
           */
          char tmp[256];
          char *s = trim_start(card->card_str);
          strncpy(tmp, s, sizeof(tmp)-1);
          tmp[sizeof(tmp)-1] = '\0';
          /* Skip the mnemonic (first token) and then pick the third field
           * after the mnemonic (i.e., the float field). This handles lines
           * like: "GS 0 0 in" where tokens are [GS,0,0,in] and we want "in".
           */
          char *tok = strtok(tmp, " \t");
          int count = 0;
          char *third = NULL;
          while ((tok = strtok(NULL, " \t")) != NULL) {
            count++;
            if (count == 3) { /* third field after mnemonic */
              third = tok;
              break;
            }
          }
          if (third) {
            key_value_t temp_kv = {0};
            temp_kv.key = "GS_TMP";
            temp_kv.value = strdup(third);
            temp_kv.fv = 0.0;
            evaluate_formula(ctx, &temp_kv, deck, errors);
            if (temp_kv.fv != 0.0) {
              xw1 = temp_kv.fv;
            }
            free(temp_kv.value);
          }

          if (xw1 == 0.0) {
            snprintf(msg, sizeof(msg), "GS on line %d: scale factor is zero.", i + 1);
            add_error(ctx, errors, msg, FATAL);
            return; // Stops further geometry processing
          }
        }
        if (!card->ignore) {
          scale(ctx, xw1);
        }
        if (ctx->ignored_geometry.num_segs > 0 || ctx->ignored_geometry.num_patches > 0) {
          geometry_t _live = ctx->geometry;
          ctx->geometry = ctx->ignored_geometry;
          scale(ctx, xw1);
          ctx->ignored_geometry = ctx->geometry;
          ctx->geometry = _live;
        }
        continue;
        
      case 4: // GE, finish off the segments and patches, and calculate everything
        // FIXME: it's not clear what this is testing, on a GE card there shouldn't be an ns input
        //  perhaps it is  clearing out the ns from the previous line? but why bother when it's
        //  about to return anyway?
        if(segs != 0) {
          ctx->plot.plot_type = 1;
          ctx->plot.plot_axis = 1;
        }
        
        // if we're at the end of the geometry section, we have all the segments
        // so now is an opportune time to connect them together
        if (connect_segments(ctx, tag, outputs) != 0) {
          return; // Stop if there's a fatal geometry error (e.g. below ground)
        }
        
        // ... and calculate the midpoints and other bits
        finish_geometry(ctx);
        
        // and in this case, we're done
        return;
        
      case 5: // GM, move structure or reproduce/duplicate original structure in new positions
        xw1 = xw1 * TA;
        yw1 = yw1 * TA;
        zw1 = zw1 * TA;
        // convert the original float value in F7 to int
        int tag_increment = (int)(card->f[7] + .5);
        if (!card->ignore) {
          reproduce(ctx, xw1, yw1, zw1, xw2, yw2, zw2, tag_increment, segs, tag);
        }
        if (ctx->ignored_geometry.num_segs > 0 || ctx->ignored_geometry.num_patches > 0) {
          geometry_t _live = ctx->geometry;
          ctx->geometry = ctx->ignored_geometry;
          reproduce(ctx, xw1, yw1, zw1, xw2, yw2, zw2, tag_increment, segs, tag);
          ctx->ignored_geometry = ctx->geometry;
          ctx->geometry = _live;
        }
        continue;
        
      case 6: // SP, generate single new patch or a series of patches with SC
        //ns++;
        
        // SP cards have to have a blank in I1, but is this really an error?
        if (tag != 0) {
          snprintf(msg, sizeof(msg), "SP on line %d: has data in I1.", i + 1);
          add_error(ctx,errors, msg, WARNING);
        }
        
        // start with the simple case of a simple, single patch, no set shape
        if(segs == 0) {
          xw2 = xw2 * TA;
          yw2 = yw2 * TA;
          patch(ctx, target_geom, i, tag, segs + 1, xw1, yw1, zw1, xw2, yw2, zw2, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
        }
        // other shapes, segs=1,2,3, require more inputs and there will be additional SC cards
        else {
          // make sure the next card is an SC
          // TODO: we should test the sanity of the inputs based on the ns
          int next_idx = peek_next_geometry(deck, i);
          if(next_idx == -1 || strcmp(deck->cards[next_idx].card_code, "SC") != 0) {
            snprintf(msg, sizeof(msg), "SP on line %d: type %d requires the next card to be an SC.", i + 1, segs);
            add_error(ctx, errors, msg, WARNING);
            continue;
          }
          // if it's a triangle we just read one more point from the new card and go...
          if(segs == 2) {
            x3 = deck->cards[next_idx].f[1];
            y3 = deck->cards[next_idx].f[2];
            z3 = deck->cards[next_idx].f[3];
            i = next_idx; // skip the SC card next time through the main loop
            patch(ctx, target_geom, i, tag, segs, xw1, yw1, zw1, xw2, yw2, zw2, x3, y3, z3, 0.0, 0.0, 0.0);
          } /* ns == 2 */
          // if it's not a triangle, we have to loop over the following cards
          else {
            // there has to be at least one following...
            x3 = deck->cards[next_idx].f[1];
            y3 = deck->cards[next_idx].f[2];
            z3 = deck->cards[next_idx].f[3];
            x4 = deck->cards[next_idx].f[4];
            y4 = deck->cards[next_idx].f[5];
            z4 = deck->cards[next_idx].f[6];
            i = next_idx;
            patch(ctx, target_geom, i, tag, segs, xw1, yw1, zw1, xw2, yw2, zw2, x3, y3, z3, x4, y4, z4);
            
            // if it was segs=1 we are done at this point, for segs=3 there's more,
            // so loop until we run out of following SC's
            while((next_idx = peek_next_geometry(deck, i)) != -1 && strcmp(deck->cards[next_idx].card_code, "SC") == 0) {
              // copy the last set of end coords into this set's start coords
              xw1 = x3;
              yw1 = y3;
              zw1 = z3;
              xw2 = x4;
              yw2 = y4;
              zw2 = z4;
              // and then get the next set of end coords
              x3 = deck->cards[next_idx].f[1];
              y3 = deck->cards[next_idx].f[2];
              z3 = deck->cards[next_idx].f[3];
              x4 = deck->cards[next_idx].f[4];
              y4 = deck->cards[next_idx].f[5];
              z4 = deck->cards[next_idx].f[6];
              i = next_idx;
              patch(ctx, target_geom, i, tag, segs, xw1, yw1, zw1, xw2, yw2, zw2, x3, y3, z3, x4, y4, z4);
            } /* while cards are SC's */
          }/* ns = 2 */
        } /* ns > 0 */
        
        continue;
        
      case 7: // SM, generate multiple-patch rectangular surface
        if(tag < 1 || segs < 1) {
          snprintf(msg, sizeof(msg), "SM on line %d: number of patches in I1 or I2 is too small.", i + 1);
          add_error(ctx, errors, msg, 1);
          continue;
        }
        int sm_next = peek_next_geometry(deck, i);
        if(sm_next == -1 || strcmp(deck->cards[sm_next].card_code, "SC") != 0) {
          snprintf(msg, sizeof(msg), "SM on line %d: requires the next card to be an SC.", i + 1);
          add_error(ctx, errors, msg, 1);
          continue;
        }
        
        // read the sc and skip it
        x3 = deck->cards[sm_next].f[1];
        y3 = deck->cards[sm_next].f[2];
        z3 = deck->cards[sm_next].f[3];
        i = sm_next;
        
        // calculate corner 4
        if(segs == 2 || tag > 0) {
          x4 = xw1 + x3 - xw2;
          y4 = yw1 + y3 - yw2;
          z4 = zw1 + z3 - zw2;
        }
        
        patch(ctx, target_geom, i, tag, segs, xw1, yw1, zw1, xw2, yw2, zw2, x3, y3, z3, x4, y4, z4);
        continue;
        
      case 8: // GA, generate segment data for wire arc
        arc(ctx, target_geom, i, tag, segs, xw1, yw1, zw1, xw2);
        continue;
        
      case 9: // SC card, skip it - but it should never happen because SP/SM should have read it
        continue;
        
      case 10: // GH, generate helix
        // Detect 4NEC2's "NEC-4" GH format vs standard NEC-2 format.
        // NEC-2: F1=spacing, F2=length(signed), F3=a1, F4=b1, F5=a2, F6=b2, F7=rad
        // 4NEC2: F1=turns(signed), F2=length, F3=a1, F4=b1, F5=rad, F6=rad, F7=flag
        //   F7=0: log spiral, F7=1: Archimedes spiral (flag only, geometry is the same)
        //
        // Detection: if F7 is 0 or 1 AND F5 is much smaller than F3 (wire radius vs
        // helix radius), this is the 4NEC2 format. Convert turns to spacing and use
        // F5 as the wire radius.
        if((rad == 0.0 || rad == 1.0) && yw2 > 0.0 && zw1 > 0.0 && yw2 < zw1 * 0.5) {
          // 4NEC2 format: F1=turns, F5=wire radius, F7=spiral type flag
          double turns = xw1;      // F1 = number of turns (signed for handedness)
          double wire_rad = yw2;   // F5 = wire radius
          // Convert to NEC-2 parameters: spacing = length / turns
          xw1 = yw1 / turns;       // spacing = total_length / turns (sign carries handedness)
          // For 4NEC2 format, a2=a1 and b2=b1 (uniform helix assumed)
          yw2 = zw1;               // a2 = a1
          zw2 = xw2;               // b2 = b1
          rad = wire_rad;
          // Update card f[] so output display shows the converted NEC-2 values
          card->f[1] = xw1;
          card->f[5] = yw2;
          card->f[6] = zw2;
          card->f[7] = rad;
          snprintf(msg, sizeof(msg), "GH card on line %d: detected 4NEC2 format (%.0f turns, wire radius %.4g).", i + 1, fabs(turns), rad);
          add_message(ctx, outputs, msg);
        }
        helix(ctx, target_geom, i, tag, segs, xw1, yw1, zw1, xw2, yw2, zw2, rad, outputs);
        continue;
        
      case 11: { // GF - load Numerical Green's Function file
        const char *ngf_filename = card->comment;
        char gf_default[MAX_PATH_LEN + 1];
        char gf_resolved[MAX_PATH_LEN + 1];
        if (!ngf_filename || *ngf_filename == '\0') {
          if (ctx->source_filename) {
            strncpy(gf_default, ctx->source_filename, MAX_PATH_LEN);
            gf_default[MAX_PATH_LEN] = '\0';
            char *dot   = strrchr(gf_default, '.');
            char *slash = strrchr(gf_default, '/');
            if (dot && (!slash || dot > slash))
              *dot = '\0';
            strncat(gf_default, ".ngf", MAX_PATH_LEN - strlen(gf_default));
            ngf_filename = gf_default;
          } else {
            snprintf(msg, sizeof(msg), "GF on line %d: no filename and no input file to derive one from.", i + 1);
            add_error(ctx, errors, msg, FATAL);
            return;
          }
        } else {
          /* Explicit filename: resolve relative to input file's directory */
          resolve_path_relative_to_input(ngf_filename, ctx->source_filename,
                                         gf_resolved, sizeof(gf_resolved));
          ngf_filename = gf_resolved;
        }
        /* Determine whether to show full path or just the basename.
         * Only show the full path when the NGF file resides in a different
         * directory than the input deck; otherwise show the basename to
         * avoid repeating the deck's directory. */
        char ngf_display[MAX_PATH_LEN + 1];
        if (ctx->source_filename && ctx->source_filename[0]) {
          const char *deck_slash = strrchr(ctx->source_filename, '/');
          const char *ngf_slash  = strrchr(ngf_filename, '/');
          size_t deck_dir_len = deck_slash ? (size_t)(deck_slash - ctx->source_filename) : 0;
          size_t ngf_dir_len  = ngf_slash ? (size_t)(ngf_slash - ngf_filename) : 0;
          if (deck_dir_len == ngf_dir_len &&
              (deck_dir_len == 0 || strncmp(ctx->source_filename, ngf_filename, deck_dir_len) == 0)) {
            /* Same directory: show basename only */
            const char *base = ngf_slash ? ngf_slash + 1 : ngf_filename;
            strncpy(ngf_display, base, sizeof(ngf_display) - 1);
            ngf_display[sizeof(ngf_display) - 1] = '\0';
          } else {
            /* Different directory: show full resolved path */
            strncpy(ngf_display, ngf_filename, sizeof(ngf_display) - 1);
            ngf_display[sizeof(ngf_display) - 1] = '\0';
          }
        } else {
          strncpy(ngf_display, ngf_filename, sizeof(ngf_display) - 1);
          ngf_display[sizeof(ngf_display) - 1] = '\0';
        }

        FILE *gfp = fopen(ngf_filename, "rb");
        if (!gfp) {
          snprintf(msg, sizeof(msg), "GF on line %d: cannot open NGF file '%s'.", i + 1, ngf_display);
          add_error(ctx, errors, msg, FATAL);
          return;
        }
        bool ngf_ok = read_greens_binary(gfp, ctx);
        fclose(gfp);
        if (!ngf_ok) {
          /* error already recorded by read_greens_binary */
          return;
        }
        snprintf(msg, sizeof(msg),
           "GF card on line %d: loaded %d segments from '%s'.",
           i + 1, ctx->ngf_n_segs, ngf_display);
        add_message(ctx, outputs, msg);
        continue;
      }
        
      case 12: // GC, geometry continuation - should only appear after GW
        snprintf(msg, sizeof(msg), "GC on line %d: found outside of GW tapering context.", i + 1);
        add_error(ctx, errors, msg, WARNING);
        continue;
        
      default: // error message if this isn't a comment
        if(!is_comment(card)) {
          snprintf(msg, sizeof(msg), "Unknown card type '%s' on line %d: skipped.", card->card_code, i + 1);
          add_error(ctx, errors, msg, 1);
        }
    } /* switch on card type */
  } /* for loop over cards */
  
} /* calculate_geometry */

/******************************************************************************
 * segment_number
 *
 * segment_number (formerly isegno) returns the segment number for the @p m th
 * segment within the structure generated by the card with tag number @p tag.
 * For instance, the 5th segment within tag 7 might be segment_number 25.
 *
 * @param tag The tag number of the structure/card
 * @param m The segment number within that structure
 *
 */
int segment_number(nec_context_t *ctx, int tag, int seg)
{
  int icnt, iseg;
  char msg[MAX_ERROR_LEN]; // used for seg <= 0 error below
  
  if (seg <= 0) {
    snprintf(msg, sizeof(msg), "segment_number was called with a segment number less or equal to zero.");
    add_error(ctx, &ctx->geometry.errors, msg, 1);
  }
  
  // if the tag number is zero, then simply return the mth segment as the answer
  // FIXME: is there any point assigning iseg here?
  if (tag == 0) {
    iseg = seg;
    return(iseg);
  }
  
  // if the tag isn't zero, look for it in the segment collection
  icnt = 0;
  if (ctx->geometry.num_segs > 0) {
    for (int i = 0; i < ctx->geometry.num_segs; i++) {
      if (ctx->geometry.tag_nums[i] != tag)
        continue;
      
      icnt++;
      if (icnt == seg) {
        iseg = i + 1;
        return(iseg);
      }
    } /* for( i = 0; i < ctx->geometry.num_segs; i++ ) */
  } /* if( ctx->geometry.num_segs > 0) */
  
  // if we didn't find it, return 0 (caller is responsible for reporting)
  return(0);
} /* end of segment_number */

/******************************************************************************
 * connect_segments
 *
 * connect_segments (formerly CONECT) sets up segment connection data in
 * arrays icon1 and icon2 by searching for segment ends that are in contact.
 *
 * @param ignd If a ground plane is in use, checks if wires touch ground
 *
 */
int connect_segments(nec_context_t *ctx, int ignd, outputs_list_t *outputs)
{
  int i, iz, ic, j, jx, ix, ixx, iseg, iend, jend, jump, ipf;
  double sep=0., xi1, yi1, zi1, xi2, yi2, zi2;
  double slen, xa, ya, za, xs, ys, zs;
  size_t mreq;
  char msg[MAX_ERROR_LEN * 64];

  // Default: np/mp span the full geometry (symmetry commands may reduce them).
  // Matches nec2c conect() lines 39-41.
  ctx->geometry.num_segs_sym = ctx->geometry.num_segs;
  ctx->geometry.num_patches_sym = ctx->geometry.num_patches;
  ctx->geometry.symmetry_flag = 0;

  ctx->segj.max_connections = 1;
  
  if(ignd != 0) {
    add_message(ctx, outputs, "\n\n     GROUND PLANE SPECIFIED.");

    if( ignd > 0)
      add_message(ctx, outputs,
              "\n     WHERE WIRE ENDS TOUCH GROUND, CURRENT WILL"
              " BE INTERPOLATED TO IMAGE IN GROUND PLANE.\n" );

    if(ctx->geometry.symmetry_flag == 2) {
      ctx->geometry.num_segs_sym = 2 * ctx->geometry.num_segs_sym;
      ctx->geometry.num_patches_sym = 2 * ctx->geometry.num_patches_sym;
    }

    if(abs(ctx->geometry.symmetry_flag) > 2) {
      ctx->geometry.num_segs_sym = ctx->geometry.num_segs;
      ctx->geometry.num_patches_sym = ctx->geometry.num_patches;
    }
    
    /** possibly should be error condition?? **/
    if(ctx->geometry.num_segs_sym > ctx->geometry.num_segs) {
      char err_msg[256];
      snprintf(err_msg, sizeof(err_msg), "connect_segments was called np > n, %d > %d", ctx->geometry.num_segs_sym, ctx->geometry.num_segs);
      add_error(ctx, &ctx->errors, err_msg, FATAL);
      return -1;
    }
    
    if((ctx->geometry.num_segs_sym == ctx->geometry.num_segs) && (ctx->geometry.num_patches_sym == ctx->geometry.num_patches))
      ctx->geometry.symmetry_flag = 0;
    
  } /* if( ignd != 0) */
  
  if(ctx->geometry.num_segs != 0) {
    /* Allocate memory to connections */
    mreq = (size_t)(ctx->geometry.num_segs + ctx->geometry.num_patches);
    mreq *= sizeof(int);
    mem_realloc(ctx, (void *)&ctx->geometry.seg_end1_conn, mreq);
    mem_realloc(ctx, (void *)&ctx->geometry.seg_end2_conn, mreq);
    
    for(i = 0; i < ctx->geometry.num_segs; i++) {
      ctx->geometry.seg_end1_conn[i] = ctx->geometry.seg_end2_conn[i] = 0;
      iz = i+1;
      xi1 = ctx->geometry.end1_x[i];
      yi1 = ctx->geometry.end1_y[i];
      zi1 = ctx->geometry.end1_z[i];
      xi2 = ctx->geometry.end2_x[i];
      yi2 = ctx->geometry.end2_y[i];
      zi2 = ctx->geometry.end2_z[i];
      slen = sqrt( (xi2- xi1)*(xi2- xi1) + (yi2- yi1) *
                  (yi2- yi1) + (zi2- zi1)*(zi2- zi1) ) * SMIN;
      
      // determine connection data for end 1 of segment
      jump = false;
      if(ignd > 0) {
        if(zi1 <= -slen) {
          char l_msg[MAX_ERROR_LEN];
          snprintf(l_msg, sizeof(l_msg), "GEOMETRY DATA ERROR -- SEGMENT %d EXTENDS BELOW GROUND", iz);
          add_error(ctx, &ctx->geometry.errors, l_msg, 1);
          return -1;
        }
        
        if( zi1 <= slen) {
          ctx->geometry.seg_end1_conn[i]= iz;
          ctx->geometry.end1_z[i]=0.;
          jump = true;
        } /* if( zi1 <= slen) */
      } /* if( ignd > 0) */
      
      if( !jump ) {
        ic= i;
        for( j = 1; j < ctx->geometry.num_segs; j++) {
          ic++;
          if( ic >= ctx->geometry.num_segs)
            ic=0;
          
          sep= fabs( xi1- ctx->geometry.end1_x[ic])+ fabs(yi1- ctx->geometry.end1_y[ic])+ fabs(zi1- ctx->geometry.end1_z[ic]);
          if( sep <= slen) {
            ctx->geometry.seg_end1_conn[i]= -(ic+1);
            break;
          }
          
          sep= fabs( xi1- ctx->geometry.end2_x[ic])+ fabs(yi1- ctx->geometry.end2_y[ic])+ fabs(zi1- ctx->geometry.end2_z[ic]);
          if( sep <= slen) {
            ctx->geometry.seg_end1_conn[i]= (ic+1);
            break;
          }
        } /* for( j = 1; j < data.n; j++) */
      } /* if( ! jump ) */
      
      /* determine connection data for end 2 of segment. */
      if( (ignd > 0) || jump ) {
        if( zi2 <= -slen) {
          char err_msg[256];
          snprintf(err_msg, sizeof(err_msg), "GEOMETRY DATA ERROR -- SEGMENT %d EXTENDS BELOW GROUND", iz);
          add_error(ctx, &ctx->errors, err_msg, FATAL);
          return -1;
        }
        
        if( zi2 <= slen) {
          if( ctx->geometry.seg_end1_conn[i] == iz ) {
            char err_msg[256];
            snprintf(err_msg, sizeof(err_msg), "GEOMETRY DATA ERROR -- SEGMENT %d LIES IN GROUND PLANE", iz);
            add_error(ctx, &ctx->errors, err_msg, FATAL);
            return -1;
          }
          
          ctx->geometry.seg_end2_conn[i] = iz;
          ctx->geometry.end2_z[i] = 0.;
          continue;
          
        } /* if( zi2 <= slen) */
      } /* if( ignd > 0) */
      
      ic= i;
      for(j = 1; j < ctx->geometry.num_segs; j++) {
        ic++;
        if( ic >= ctx->geometry.num_segs)
          ic=0;
        
        sep= fabs(xi2- ctx->geometry.end1_x[ic])+ fabs(yi2- ctx->geometry.end1_y[ic])+ fabs(zi2- ctx->geometry.end1_z[ic]);
        if(sep <= slen) {
          ctx->geometry.seg_end2_conn[i]= (ic+1);
          break;
        }
        
        sep= fabs(xi2- ctx->geometry.end2_x[ic])+ fabs(yi2- ctx->geometry.end2_y[ic])+ fabs(zi2- ctx->geometry.end2_z[ic]);
        if(sep <= slen) {
          ctx->geometry.seg_end2_conn[i]= -(ic+1);
          break;
        }
        
      } /* for( j = 1; j < data.n; j++ ) */
    } /* for( i = 0; i < data.n; i++ ) */
    
    /* find wire-surface connections for new patches */
    if(ctx->geometry.num_patches != 0) {
      ix = -1;
      i = 0;
      while(++i <= ctx->geometry.num_patches) {
        ix++;
        xs = ctx->geometry.patch_x_center[ix];
        ys = ctx->geometry.patch_y_center[ix];
        zs = ctx->geometry.patch_z_center[ix];
        
        for(iseg = 0; iseg < ctx->geometry.num_segs; iseg++) {
          xi1 = ctx->geometry.end1_x[iseg];
          yi1 = ctx->geometry.end1_y[iseg];
          zi1 = ctx->geometry.end1_z[iseg];
          xi2 = ctx->geometry.end2_x[iseg];
          yi2 = ctx->geometry.end2_y[iseg];
          zi2 = ctx->geometry.end2_z[iseg];
          
          /* for first end of segment */
          slen = (fabs(xi2 - xi1) + fabs(yi2 - yi1) + fabs(zi2 - zi1))* SMIN;
          sep = fabs(xi1 - xs) + fabs(yi1 - ys) + fabs(zi1 - zs);
          
          /* connection - divide patch into 4 patches at present array loc. */
          if(sep <= slen) {
            ctx->geometry.seg_end1_conn[iseg] = PCHCON + i;
            ic=0;
            calculate_patch(ctx, i, ic);
            break;
          }
          
          sep = fabs(xi2- xs)+ fabs(yi2- ys)+ fabs(zi2- zs);
          if(sep <= slen) {
            ctx->geometry.seg_end2_conn[iseg] = PCHCON + i;
            ic = 0;
            calculate_patch(ctx, i, ic);
            break;
          }
          
        } /* for( iseg = 0; iseg < data.n; iseg++ ) */
      } /* while( ++i <= data.m ) */
    } /* if( data.m != 0) */
  } /* if( data.n != 0) */
  
  // if we have no geometry, we're done
  if(ctx->geometry.num_segs == 0) {
    return 0;
  }
  
  // allocate to connection buffers
  mreq = (size_t)ctx->segj.max_connections;
  mreq *= sizeof(int);
  mem_realloc(ctx, (void *)&ctx->segj.junction_segs, mreq);
  
  /* adjust connected segment ends to exactly coincide.  print junctions */
  /* of 3 or more seg.  also find old seg. connecting to new seg. */
  iseg = 0;
  ipf = false;
  for(j = 0; j < ctx->geometry.num_segs; j++) {
    jx = j + 1;
    iend = -1;
    jend = -1;
    ix = ctx->geometry.seg_end1_conn[j];
    ic = 1;
    ctx->segj.junction_segs[0] = -jx;
    xa = ctx->geometry.end1_x[j];
    ya = ctx->geometry.end1_y[j];
    za = ctx->geometry.end1_z[j];
    
    /* if( ix == 0 ) Not needed??
     {
     fprintf( output_fp,
     "\n  CONNECT - SEGMENT CONNECTION ERROR FOR SEGMENT: %d", ix );
     stop(ctx, -1);
     } */
    
    while(true) {
      if((ix != 0) && (ix != (j+1)) && (ix <= PCHCON)) {
        /* chain_limit: a valid connection chain can visit each segment at most
         * once before terminating (ix==0).  If we exceed ctx->geometry.num_segs hops
         * the graph has a cycle and we would loop forever. */
        int chain_hops = 0;
        do {
          if(++chain_hops > ctx->geometry.num_segs) {
            int other_seg = (ix < 0) ? -ix : ix;
            snprintf(msg, sizeof(msg),
              "Segment connection cycle: segment %d (card %d) chains into segment %d (card %d) — geometry is degenerate",
              j + 1, ctx->geometry.card_nums[j],
              other_seg, ctx->geometry.card_nums[other_seg - 1]);
            add_error(ctx, &ctx->geometry.errors, msg, FATAL);
            return -1;
          }

          if(ix < 0)
            ix = -ix;
          else
            jend = -jend;
          
          jump = false;
          
          if(ix == jx)
            break;
          
          if(ix < jx) {
            jump = true;
            break;
          }
          
          /* Record max. no. of connections */
          ic++;
          if(ic >= ctx->segj.max_connections) {
            ctx->segj.max_connections = ic + 1;
            mreq = (size_t)ctx->segj.max_connections;
            mreq *= sizeof(int);
            mem_realloc(ctx, (void *)&ctx->segj.junction_segs, mreq);
          }
          ctx->segj.junction_segs[ic-1]= ix* jend;
          
          ixx = ix-1;
          if(jend != 1) {
            xa = xa + ctx->geometry.end1_x[ixx];
            ya = ya + ctx->geometry.end1_y[ixx];
            za = za + ctx->geometry.end1_z[ixx];
            ix = ctx->geometry.seg_end1_conn[ixx];
            continue;
          }
          
          xa = xa + ctx->geometry.end2_x[ixx];
          ya = ya + ctx->geometry.end2_y[ixx];
          za = za + ctx->geometry.end2_z[ixx];
          ix = ctx->geometry.seg_end2_conn[ixx];
          
        } /* do */
        while(ix != 0);
        
        if(jump && (iend == 1))
          break;
        else
          if(jump) {
            iend = 1;
            jend = 1;
            ix = ctx->geometry.seg_end2_conn[j];
            ic = 1;
            ctx->segj.junction_segs[0] = jx;
            xa = ctx->geometry.end2_x[j];
            ya = ctx->geometry.end2_y[j];
            za = ctx->geometry.end2_z[j];
            continue;
          }
        
        sep= (double)ic;
        xa= xa / sep;
        ya= ya / sep;
        za= za / sep;
        
        for(i = 0; i < ic; i++) {
          ix = ctx->segj.junction_segs[i];
          if(ix <= 0) {
            ix = -ix;
            ixx = ix - 1;
            ctx->geometry.end1_x[ixx] = xa;
            ctx->geometry.end1_y[ixx] = ya;
            ctx->geometry.end1_z[ixx] = za;
            continue;
          }
          
          ixx = ix - 1;
          ctx->geometry.end2_x[ixx] = xa;
          ctx->geometry.end2_y[ixx] = ya;
          ctx->geometry.end2_z[ixx] = za;
        } /* for( i = 0; i < ic; i++ ) */
        
        if(ic >= 3) {
          if(!ipf) {
            snprintf(msg, sizeof(msg), "\n\n    ---------- MULTIPLE WIRE JUNCTIONS ----------\n    JUNCTION  SEGMENTS (- FOR END 1, + FOR END 2)");
            add_message(ctx, outputs, msg);
            ipf = true;
          }

          iseg++;
          snprintf(msg, sizeof(msg), "\n   %5d      ", iseg);

          for(i = 1; i <= ic; i++)  {
            size_t len = strlen(msg);
            if (len + 7 > sizeof(msg)) {
              snprintf(msg + len, sizeof(msg) - len, " ...");
              break;
            }
            snprintf(msg + len, sizeof(msg) - len, "%5d", ctx->segj.junction_segs[i-1]);
            if(!(i % 20) && (i < ic)) {
              len = strlen(msg);
              if (len + 16 > sizeof(msg)) {
                snprintf(msg + len, sizeof(msg) - len, " ...");
                break;
              }
              snprintf(msg + len, sizeof(msg) - len, "\n              ");
            }
          }
          add_message(ctx, outputs, msg);
          
        } /* if( ic >= 3) */
      } /*if( (ix != 0) && (ix != j) && (ix <= PCHCON) ) */
      
      if(iend == 1)
        break;
      
      iend = 1;
      jend = 1;
      ix = ctx->geometry.seg_end2_conn[j];
      ic = 1;
      ctx->segj.junction_segs[0] = jx;
      xa = ctx->geometry.end2_x[j];
      ya = ctx->geometry.end2_y[j];
      za = ctx->geometry.end2_z[j];
      
    } /* while( true ) */
  } /* for( j = 0; j < data.n; j++ ) */
  
  mreq = (size_t)ctx->segj.max_connections;
  mreq *= sizeof(double);
  mem_realloc(ctx, (void *)&ctx->segj.coeff_const, mreq);
  mem_realloc(ctx, (void *)&ctx->segj.coeff_sine, mreq);
  mem_realloc(ctx, (void *)&ctx->segj.coeff_cos, mreq);
  return 0;
} /* end of connect_segments */

/******************************************************************************
 * finish_geometry
 *
 * finish_geometry (formerly part of calculate_geometry) calculates midpoints
 * of wires and patches and similar values that run when the GE is seen.
 *
 * Some of the calculations it performed were used only for display in the
 * output files, including the angles of segments and the midpoints of patches.
 * These have been moved to output.c. As a result, this code no longer does
 * anything with the patches and it's possible that more of the values being
 * cached here may be removed entirely.
 *
 */
void finish_geometry(nec_context_t *ctx)
{
  size_t mreq;
  double xw1, yw1, zw1;
  double xw2, yw2;
  char msg[MAX_ERROR_LEN];
  
  // and now we calculate various geometry-related data for wires,
  // like the centerpoints and orientation
  if(ctx->geometry.num_segs != 0) {
    // reallocate the buffers
    mreq = (size_t)ctx->geometry.num_segs * sizeof(double);
    mem_realloc(ctx, (void *)&ctx->geometry.half_len, mreq);
    mem_realloc(ctx, (void *)&ctx->geometry.dir_cos_y, mreq);
    mem_realloc(ctx, (void *)&ctx->geometry.dir_cos_x, mreq);
    mem_realloc(ctx, (void *)&ctx->geometry.dir_cos_z, mreq);
    mem_realloc(ctx, (void *)&ctx->geometry.x_center, mreq);
    mem_realloc(ctx, (void *)&ctx->geometry.y_center, mreq);
    mem_realloc(ctx, (void *)&ctx->geometry.z_center, mreq);
    
    for(int i = 0; i < ctx->geometry.num_segs; i++) {
      // calculate the segment midpoints
      xw1 = ctx->geometry.end2_x[i] - ctx->geometry.end1_x[i];
      yw1 = ctx->geometry.end2_y[i] - ctx->geometry.end1_y[i];
      zw1 = ctx->geometry.end2_z[i] - ctx->geometry.end1_z[i];
      ctx->geometry.x_center[i] = (ctx->geometry.end1_x[i] + ctx->geometry.end2_x[i]) / 2.0;
      ctx->geometry.y_center[i] = (ctx->geometry.end1_y[i] + ctx->geometry.end2_y[i]) / 2.0;
      ctx->geometry.z_center[i] = (ctx->geometry.end1_z[i] + ctx->geometry.end2_z[i]) / 2.0;
      
      // and lengths
      xw2 = xw1 * xw1 + yw1 * yw1 + zw1 * zw1;
      yw2 = sqrt(xw2);
      yw2 = (xw2 / yw2 + yw2) * 0.5;
      ctx->geometry.half_len[i] = yw2;
      
      // and angles
      ctx->geometry.dir_cos_x[i] = xw1 / yw2;
      ctx->geometry.dir_cos_y[i] = yw1 / yw2;
      xw2 = zw1 / yw2;
      
      if(xw2 > 1.0)
        xw2 = 1.0;
      if(xw2 < -1.0)
        xw2 = -1.0;
      ctx->geometry.dir_cos_z[i] = xw2;
      
      if(ctx->geometry.half_len[i] <= 1.e-20) {
        snprintf(msg, sizeof(msg), "The length of segment %d is too small to process.", i + 1);
        add_error(ctx, &ctx->geometry.errors, msg, 1);
      }
      if(ctx->geometry.radius[i] <= 0.0) {
        snprintf(msg, sizeof(msg), "The radius of segment %d is too small to process.", i + 1);
        add_error(ctx, &ctx->geometry.errors, msg, 1);
      }
    } /* for( i = 0; i < ctx->geometry.num_segs; i++ ) */
  } /* if( ctx->geometry.num_segs != 0) */
  
  // update the counters that track the total number of segments and patches
  ctx->geometry.num_segs_and_patches = ctx->geometry.num_segs + ctx->geometry.num_patches;
  ctx->geometry.num_segs_2xpatches = ctx->geometry.num_segs + 2 * ctx->geometry.num_patches;
  ctx->geometry.num_segs_3xpatches = ctx->geometry.num_segs + 3 * ctx->geometry.num_patches;
  
}

/******************************************************************************
 * wire
 *
 * wire generates segment geometry data for a straight wire of @p segs segments.
 *
 * @param card_num card_t number for this set of segments
 * @param tag_num Tag number for this set of segments, maybe 0segs
 * @param segs Number of segments in the arc
 * @param xw1 Starting X point of one end of the wire
 * @param yw1 Starting Y point of one end of the wire
 * @param zw1 Starting Z point of one end of the wire
 * @param xw2 Ending X point of other end of the wire
 * @param yw2 Ending Y point of other end of the wire
 * @param zw2 Ending Z point of other end of the wire
 * @param wire_radius Radius of the wire
 * @param rdel Taper parameter length
 * @param rrad Taper parameter radius
 *
 */
void wire(nec_context_t *ctx, geometry_t *geom, int card_num, int tag_num, int segs,
          double xw1, double yw1, double zw1,
          double xw2, double yw2, double zw2,
          double rad, double rdel, double rrad)
{
  int first_segment_num;
  size_t mreq;
  double xd, yd, zd, delz, rd, fns, radz;
  double xs1, ys1, zs1, xs2, ys2, zs2;
  
  // only add this wire if it actually has segments
  // NOTE: in the original code  this was done below setting the n and np
  //       below, which would mean adding a wire with zero segments would
  //       reset geometry, which seems to make no sense
  if(segs < 1) return;
  
  // FIXME: should this also check if the length is zero?
  
  // copy down the starting segment number, and then move up all the segment counters
  first_segment_num = geom->num_segs;
  geom->num_segs += segs;
  
  // reset the symmetry
  geom->num_segs_sym = geom->num_segs;
  geom->num_patches_sym = geom->num_patches;
  geom->symmetry_flag = 0;
  
  // reallocate the cards and tags buffers
  mreq = (size_t)(geom->num_segs + geom->num_patches);
  mreq *= sizeof(int);
  mem_realloc(ctx, (void *)&geom->card_nums, mreq);
  mem_realloc(ctx, (void *)&geom->tag_nums, mreq);
  
  // reallocate wire buffers
  mreq = (size_t)geom->num_segs;  // this is the current number of wire segments, after adding the new segments
  mreq *= sizeof(double);
  mem_realloc(ctx, (void *)&geom->end1_x, mreq);
  mem_realloc(ctx, (void *)&geom->end1_y, mreq);
  mem_realloc(ctx, (void *)&geom->end1_z, mreq);
  mem_realloc(ctx, (void *)&geom->end2_x, mreq);
  mem_realloc(ctx, (void *)&geom->end2_y, mreq);
  mem_realloc(ctx, (void *)&geom->end2_z, mreq);
  mem_realloc(ctx, (void *)&geom->radius, mreq);
  
  // calculate a segment length based either on the rdels parameter from a GC,
  // or the number of segments in a normal GW
  xd = xw2 - xw1;
  yd = yw2 - yw1;
  zd = zw2 - zw1;
  
  if(fabs(rdel - 1) >= 1.0e-6) {
    delz = sqrt(xd * xd + yd * yd + zd * zd);
    xd /= delz;
    yd /= delz;
    zd /= delz;
    delz = delz * (1.0- rdel)/(1.0- pow(rdel, segs) );
    rd = rdel;
  } else {
    fns= (double)segs;
    xd /= fns;
    yd /= fns;
    zd /= fns;
    delz = 1.0;
    rd= 1.0;
  }
  
  // now start at one end...
  radz = rad;
  xs1 = xw1;
  ys1 = yw1;
  zs1 = zw1;
  
  // and for the rest of the segments, generate a segment end after moving
  // xd/yd/zd along the line, filling out the interior points
  for(int i = first_segment_num; i < geom->num_segs; i++) {
    // save these out
    geom->card_nums[i] = card_num;
    geom->tag_nums[i] = tag_num;
    
    // calculate the new locations
    xs2 = xs1 + xd * delz;
    ys2 = ys1 + yd * delz;
    zs2 = zs1 + zd * delz;
    
    // set the geometry
    geom->end1_x[i] = xs1;
    geom->end1_y[i] = ys1;
    geom->end1_z[i] = zs1;
    geom->end2_x[i] = xs2;
    geom->end2_y[i] = ys2;
    geom->end2_z[i] = zs2;
    geom->radius[i] = radz;
    
    // move to the other end and and re-taper
    delz = delz * rd;
    radz = radz * rrad;
    xs1 = xs2;
    ys1 = ys2;
    zs1 = zs2;
  } /* loop over remaining segments */
  
  // fill in the end of the line with the last point
  geom->end2_x[geom->num_segs-1] = xw2;
  geom->end2_y[geom->num_segs-1] = yw2;
  geom->end2_z[geom->num_segs-1] = zw2;
} /* end of wire() */

/******************************************************************************
 * arc
 *
 * arc generates segment geometry data for an arc of @p segs segments.
 *
 * @param card_num card_t number for this set of segments
 * @param tag_num Tag number for this set of segments, maybe 0
 * @param segs Number of segments in the arc
 * @param arc_radius Radius of the arc
 * @param ang1 Starting angle
 * @param ang2 Ending angle - ang2-ang1 <= 360
 * @param wire_radius Radius of the wire
 *
 */
void arc(nec_context_t *ctx, geometry_t *geom, int card_num, int tag_num, int segs, double rada, double ang1, double ang2, double rad)
{
  double ang, dang, xs1, xs2, zs1, zs2;
  int first_segment_num = geom->num_segs;
  
  // no point continuing if there are no segments
  if(segs < 1) return;
  
  // this test was previously performed at the end, which meant that
  // symmetry was removed even if it didn't actually build the arc.
  // as is the case in wire and helix, we will do the test now
  if(fabs(ang2- ang1) > 360.0000) {
    char msg[MAX_ERROR_LEN];
    snprintf(msg, sizeof(msg), "GA on line %d: angle >360 degrees.", card_num + 1);
    add_error(ctx, &ctx->geometry.errors, msg, 1);
    return;
  }
  
  // update the segment count
  geom->num_segs += segs;
  
  // reset symmetry
  geom->num_segs_sym = geom->num_segs;
  geom->num_patches_sym = geom->num_patches;
  geom->symmetry_flag = 0;
  
  // Reallocate card nums and tags buffer
  size_t mreq = (size_t)geom->num_segs;
  mreq *= sizeof(int);
  mem_realloc(ctx, (void *)&geom->card_nums, mreq);
  mem_realloc(ctx, (void *)&geom->tag_nums, mreq);
  
  // reallocate wire buffers
  mreq = (size_t)geom->num_segs;
  mreq *= sizeof(double);
  mem_realloc(ctx, (void *)&geom->end1_x, mreq);
  mem_realloc(ctx, (void *)&geom->end1_y, mreq);
  mem_realloc(ctx, (void *)&geom->end1_z, mreq);
  mem_realloc(ctx, (void *)&geom->end2_x, mreq);
  mem_realloc(ctx, (void *)&geom->end2_y, mreq);
  mem_realloc(ctx, (void *)&geom->end2_z, mreq);
  mem_realloc(ctx, (void *)&geom->radius, mreq);
  
  ang = ang1 * TA;
  dang = (ang2- ang1) * TA/ segs;
  xs1 = rada * cos(ang);
  zs1 = rada * sin(ang);
  
  for(int i = first_segment_num; i < geom->num_segs; i++) {
    // save these bits out
    geom->card_nums[i] = card_num;
    geom->tag_nums[i] = tag_num;
    geom->radius[i] = rad;
    
    // move around the arc by the delta angle
    ang += dang;
    xs2 = rada * cos(ang);
    zs2 = rada * sin(ang);
    
    // save that out
    geom->end1_x[i] = xs1;
    geom->end1_y[i] = 0.0;
    geom->end1_z[i] = zs1;
    geom->end2_x[i] = xs2;
    geom->end2_y[i] = 0.0;
    geom->end2_z[i] = zs2;
    
    // move up one stop
    xs1 = xs2;
    zs1 = zs2;
  } /* for( i = ist; i < data.n; i++ ) */
} /* end of arc */

/******************************************************************************
 * helix
 *
 * helix generates segment geometry data for an a helix of @p segs segments.
 *
 * @param tag_num Tag number for this set of segments
 * @param segs Number of segments in the arc
 * @param rad Radius of the wire
 *
 */
void helix(nec_context_t *ctx, geometry_t *geom, int card_num, int tag_num, int segs, double s, double hl,
           double a1, double b1, double a2, double b2, double rad, outputs_list_t *outputs)
{
  int first_seg_num;
  size_t mreq;
  double zinc, copy, sangle, hdia, turn, pitch, hmaj, hmin;
  
  // no point continuing if the number of segments is zero
  if(segs < 1) return;
  
  // update the counters
  first_seg_num = geom->num_segs;
  geom->num_segs += segs;
  
  // reset symmetry
  geom->num_segs_sym = geom->num_segs;
  geom->num_patches_sym = geom->num_patches;
  geom->symmetry_flag = 0;
  
  zinc = fabs(hl / segs);
  
  // reallocate card num and tags buffer
  mreq = (size_t)(geom->num_segs + geom->num_patches);
  mreq *= sizeof(int);
  mem_realloc(ctx, (void *)&geom->card_nums, mreq);
  mem_realloc(ctx, (void *)&geom->tag_nums, mreq);
  
  // reallocate wire buffers
  mreq = (size_t)geom->num_segs;
  mreq *= sizeof(double);
  mem_realloc(ctx, (void *)&geom->end1_x, mreq);
  mem_realloc(ctx, (void *)&geom->end1_y, mreq);
  mem_realloc(ctx, (void *)&geom->end1_z, mreq);
  mem_realloc(ctx, (void *)&geom->end2_x, mreq);
  mem_realloc(ctx, (void *)&geom->end2_y, mreq);
  mem_realloc(ctx, (void *)&geom->end2_z, mreq);
  mem_realloc(ctx, (void *)&geom->radius, mreq);
  
  geom->end1_z[first_seg_num] = 0.0;
  for(int i = first_seg_num; i < geom->num_segs; i++ ) {
    // save these out
    geom->card_nums[i] = card_num;
    geom->tag_nums[i] = tag_num;
    geom->radius[i] = rad;
    
    if(i != first_seg_num)
      geom->end1_z[i] = geom->end1_z[i-1] + zinc;
    
    geom->end2_z[i] = geom->end1_z[i] + zinc;
    
    if(a2 == a1) {
      if(b1 == 0.0)
        b1 = a1;
      
      geom->end1_x[i]= a1* cos(2.* PI* geom->end1_z[i]/ s);
      geom->end1_y[i]= b1* sin(2.* PI* geom->end1_z[i]/ s);
      geom->end2_x[i]= a1* cos(2.* PI* geom->end2_z[i]/ s);
      geom->end2_y[i]= b1* sin(2.* PI* geom->end2_z[i]/ s);
    }
    else
    {
      if(b2 == 0.0)
        b2= a2;
      
      geom->end1_x[i]=( a1+( a2- a1)* geom->end1_z[i]/ fabs( hl))* cos(2.* PI* geom->end1_z[i]/ s);
      geom->end1_y[i]=( b1+( b2- b1)* geom->end1_z[i]/ fabs( hl))* sin(2.* PI* geom->end1_z[i]/ s);
      geom->end2_x[i]=( a1+( a2- a1)* geom->end2_z[i]/ fabs( hl))* cos(2.* PI* geom->end2_z[i]/ s);
      geom->end2_y[i]=( b1+( b2- b1)* geom->end2_z[i]/ fabs( hl))* sin(2.* PI* geom->end2_z[i]/ s);
    } /* if( a2 == a1) */
    
    if(hl > 0.0)
      continue;
    
    copy= geom->end1_x[i];
    geom->end1_x[i]= geom->end1_y[i];
    geom->end1_y[i]= copy;
    copy= geom->end2_x[i];
    geom->end2_x[i]= geom->end2_y[i];
    geom->end2_y[i]= copy;
    
  } /* for( i = ist; i < data.n; i++ ) */
  
  if(a2 != a1) {
    sangle = atan( a2/( fabs( hl)+( fabs( hl)* a1)/( a2- a1)));
    char msg[MAX_ERROR_LEN];
    snprintf(msg, sizeof(msg), "\n       THE CONE ANGLE OF THE SPIRAL IS %10.4f", sangle);
    add_message(ctx, outputs, msg);
    return;
  }
  
  if(a1 == b1) {
    hdia=2.* a1;
    turn= hdia* PI;
    pitch= atan( s/( PI* hdia));
    turn= turn/ cos( pitch);
    pitch=180.* pitch/ PI;
  }
  else
  {
    if(a1 >= b1) {
      hmaj=2.* a1;
      hmin=2.* b1;
    } else {
      hmaj=2.* b1;
      hmin=2.* a1;
    }
    
    hdia = sqrt(( hmaj*hmaj+ hmin*hmin)/2* hmaj);
    turn = 2.0 * PI * hdia;
    pitch = (180.0/ PI)* atan( s/( PI* hdia));
  } /* if( a1 == b1) */
  
  {
    char msg[MAX_ERROR_LEN];
    snprintf(msg, sizeof(msg), "\n       THE PITCH ANGLE IS: %.4f    THE LENGTH OF WIRE/TURN IS: %.4f", pitch, turn);
    add_message(ctx, outputs, msg);
  }
} /* end of helix */

/******************************************************************************
 * scale
 *
 * scales all existing geometry by the given factor. As the overall geometry
 * is the same before and after, differing only in values, there are no changes
 * to the tag or card numbers.
 *
 * @param scale_factor the amount to scale by
 *
 */
void scale(nec_context_t *ctx, double xw1)
{
  /* GS card: NEC-2 spec says GS affects new structure only, not NGF segments.
   * Start from ngf_n_segs so frozen NGF segments are left untouched. */
  int scale_start = ctx->has_ngf ? ctx->ngf_n_segs : 0;

  // scale the wires
  if(ctx->geometry.num_segs > 0) {
    for(int i = scale_start; i < ctx->geometry.num_segs; i++) {
      ctx->geometry.end1_x[i] = ctx->geometry.end1_x[i] * xw1;
      ctx->geometry.end1_y[i] = ctx->geometry.end1_y[i] * xw1;
      ctx->geometry.end1_z[i] = ctx->geometry.end1_z[i] * xw1;
      ctx->geometry.end2_x[i] = ctx->geometry.end2_x[i] * xw1;
      ctx->geometry.end2_y[i] = ctx->geometry.end2_y[i] * xw1;
      ctx->geometry.end2_z[i] = ctx->geometry.end2_z[i] * xw1;
      ctx->geometry.radius[i] = ctx->geometry.radius[i] * xw1;
    }
  } /* if( data.n >= n2) */

  // and then the patches
  if(ctx->geometry.num_patches > 0) {
    double area_factor = xw1 * xw1;
    for (int i = 0; i < ctx->geometry.num_patches; i++) {
      ctx->geometry.patch_x_center[i] = ctx->geometry.patch_x_center[i] * xw1;
      ctx->geometry.patch_y_center[i] = ctx->geometry.patch_y_center[i] * xw1;
      ctx->geometry.patch_z_center[i] = ctx->geometry.patch_z_center[i] * xw1;
      ctx->geometry.patch_area[i] = ctx->geometry.patch_area[i] * area_factor;
    }
  } /* if( data.m >= m2) */
} /* end of scale */

/******************************************************************************
 * reproduce
 *
 * reproduce moves the structure with respect to its coordinate system or
 * reproduces/duplicates the structure in new positions. The structure is
 * rotated about x,y,z axes by rox,roy,roz respectively, and then shifted by
 * xs,ys,zs. Any new elements are given new tag numbers offset from their
 * original value by the number in tag_increment. Geometry with a tag of
 * zero will also be zero after duplication.
 *
 * formerly known as move(), but that conflicts with stdio
 *
 */
void reproduce(nec_context_t *ctx, double rox, double roy, double roz, double xs,
               double ys, double zs, int its, int nrpt, int tag_increment)
{
  int nrp, ix, i1, k, i;
  size_t mreq;
  double sps, cps, sth, cth, sph, cph, xx, xy;
  double xz, yx, yy, yz, zx, zy, zz, xi, yi, zi;
  
  // if we are rotating around X or Y the update the symmetry
  if(fabs(rox) + fabs(roy) > 1.0e-10)
    ctx->geometry.symmetry_flag = ctx->geometry.symmetry_flag * 3;
  
  sps = sin(rox);
  cps = cos(rox);
  sth = sin(roy);
  cth = cos(roy);
  sph = sin(roz);
  cph = cos(roz);
  xx = cph * cth;
  xy = cph * sth * sps - sph * cps;
  xz = cph * sth * cps + sph * sps;
  yx = sph * cth;
  yy = sph * sth * sps + cph * cps;
  yz = sph * sth * cps - cph * sps;
  zx = -sth;
  zy = cth * sps;
  zz = cth * cps;
  
  if(nrpt == 0)
    nrp = 1;
  else
    nrp = nrpt;
  
  // move the wires, if there are any
  ix = 1;
  if(ctx->geometry.num_segs > 0) {
    int ir;
    int original_n = ctx->geometry.num_segs;

    // get the first segment of this object
    i1 = segment_number(ctx, its, 1);
    if(i1 < 1)
      i1 = 1;

    /* GM card: NEC-2 spec says GM affects new structure only, not NGF segments.
     * Clamp i1 so we never touch frozen NGF segments (indices 0..ngf_n_segs-1). */
    if(ctx->has_ngf && i1 <= ctx->ngf_n_segs)
      i1 = ctx->ngf_n_segs + 1;

    ix = i1;
    if(nrpt == 0)
      k= i1-1;
    else {
      k = ctx->geometry.num_segs;
      /* Reallocate tags buffer */
      mreq = (size_t)(ctx->geometry.num_segs + ctx->geometry.num_patches + (ctx->geometry.num_segs + 1 - i1) * nrpt);
      mreq *= sizeof(int);
      mem_realloc(ctx, (void *)&ctx->geometry.tag_nums, mreq);

      /* Reallocate wire buffers */
      mreq = (size_t)(ctx->geometry.num_segs + (ctx->geometry.num_segs + 1 - i1) * nrpt);
      mreq *= sizeof(double);
      mem_realloc(ctx, (void *)&ctx->geometry.end1_x, mreq);
      mem_realloc(ctx, (void *)&ctx->geometry.end1_y, mreq);
      mem_realloc(ctx, (void *)&ctx->geometry.end1_z, mreq);
      mem_realloc(ctx, (void *)&ctx->geometry.end2_x, mreq);
      mem_realloc(ctx, (void *)&ctx->geometry.end2_y, mreq);
      mem_realloc(ctx, (void *)&ctx->geometry.end2_z, mreq);
      mem_realloc(ctx, (void *)&ctx->geometry.radius, mreq);
    }

    for(ir = 0; ir < nrp; ir++) {
      for(i = i1-1; i < original_n; i++)  {
        xi= ctx->geometry.end1_x[i];
        yi= ctx->geometry.end1_y[i];
        zi= ctx->geometry.end1_z[i];
        ctx->geometry.end1_x[k]= xi* xx+ yi* xy+ zi* xz+ xs;
        ctx->geometry.end1_y[k]= xi* yx+ yi* yy+ zi* yz+ ys;
        ctx->geometry.end1_z[k]= xi* zx+ yi* zy+ zi* zz+ zs;
        xi= ctx->geometry.end2_x[i];
        yi= ctx->geometry.end2_y[i];
        zi= ctx->geometry.end2_z[i];
        ctx->geometry.end2_x[k]= xi* xx+ yi* xy+ zi* xz+ xs;
        ctx->geometry.end2_y[k]= xi* yx+ yi* yy+ zi* yz+ ys;
        ctx->geometry.end2_z[k]= xi* zx+ yi* zy+ zi* zz+ zs;
        ctx->geometry.radius[k]= ctx->geometry.radius[i];
        ctx->geometry.tag_nums[k]= ctx->geometry.tag_nums[i];
        if(ctx->geometry.tag_nums[i] != 0)
          ctx->geometry.tag_nums[k]= ctx->geometry.tag_nums[i] + (ir + 1) * tag_increment;

        k++;
      } /* for( i = i1; i < data.n; i++ ) */

      ctx->geometry.num_segs = k;
    } /* for( ir = 0; ir < nrp; ir++ ) */
  } /* if( data.n >= n2) */
  
  // repeat the move for any patches
  if(ctx->geometry.num_patches > 0) {
    int ii;
    int original_m = ctx->geometry.num_patches;
    i1 = 0;
    if( nrpt == 0)
      k= 0;
    else
      k = ctx->geometry.num_patches;

    /* Reallocate patch buffers */
    mreq = (size_t)(ctx->geometry.num_patches * (nrpt + 1));
    mreq *= sizeof(double);
    mem_realloc(ctx, (void *)&ctx->geometry.patch_x_center, mreq);
    mem_realloc(ctx, (void *)&ctx->geometry.patch_y_center, mreq);
    mem_realloc(ctx, (void *)&ctx->geometry.patch_z_center, mreq);
    mem_realloc(ctx, (void *)&ctx->geometry.patch_t1x, mreq);
    mem_realloc(ctx, (void *)&ctx->geometry.patch_t1y, mreq);
    mem_realloc(ctx, (void *)&ctx->geometry.patch_t1z, mreq);
    mem_realloc(ctx, (void *)&ctx->geometry.patch_t2x, mreq);
    mem_realloc(ctx, (void *)&ctx->geometry.patch_t2y, mreq);
    mem_realloc(ctx, (void *)&ctx->geometry.patch_t2z, mreq);
    mem_realloc(ctx, (void *)&ctx->geometry.patch_area, mreq);
    mem_realloc(ctx, (void *)&ctx->geometry.patch_normal_z, mreq);

    for( ii = 0; ii < nrp; ii++ ) {
      for( i = i1; i < original_m; i++ ) {
        xi= ctx->geometry.patch_x_center[i];
        yi= ctx->geometry.patch_y_center[i];
        zi= ctx->geometry.patch_z_center[i];
        ctx->geometry.patch_x_center[k]= xi* xx+ yi* xy+ zi* xz+ xs;
        ctx->geometry.patch_y_center[k]= xi* yx+ yi* yy+ zi* yz+ ys;
        ctx->geometry.patch_z_center[k]= xi* zx+ yi* zy+ zi* zz+ zs;
        xi= ctx->geometry.patch_t1x[i];
        yi= ctx->geometry.patch_t1y[i];
        zi= ctx->geometry.patch_t1z[i];
        ctx->geometry.patch_t1x[k]= xi* xx+ yi* xy+ zi* xz;
        ctx->geometry.patch_t1y[k]= xi* yx+ yi* yy+ zi* yz;
        ctx->geometry.patch_t1z[k]= xi* zx+ yi* zy+ zi* zz;
        xi= ctx->geometry.patch_t2x[i];
        yi= ctx->geometry.patch_t2y[i];
        zi= ctx->geometry.patch_t2z[i];
        ctx->geometry.patch_t2x[k]= xi* xx+ yi* xy+ zi* xz;
        ctx->geometry.patch_t2y[k]= xi* yx+ yi* yy+ zi* yz;
        ctx->geometry.patch_t2z[k]= xi* zx+ yi* zy+ zi* zz;
        ctx->geometry.patch_normal_z[k]= ctx->geometry.patch_normal_z[i];
        ctx->geometry.patch_area[k]= ctx->geometry.patch_area[i];
        k++;
      } /* for( i = i1; i < data.m; i++ ) */

      ctx->geometry.num_patches = k;
    } /* for( ii = 0; ii < nrp; ii++ ) */

  } /* if( data.m >= m2) */
  
  // test whether we did a complete rotation/copy
  if((nrpt == 0) && (ix == 1))
    return;
  
  // otherwise, reset the symmetry flags to "none"
  ctx->geometry.num_segs_sym = ctx->geometry.num_segs;
  ctx->geometry.num_patches_sym = ctx->geometry.num_patches;
  ctx->geometry.symmetry_flag = 0;
} /* end of reproduce */

/******************************************************************************
 * reflect
 *
 * reflect (formerly reflc) creates new geometry entries for all existing
 * entries to create reflections across the selected axes. reflect can
 * duplicate across the X, Y and/or Z axes in a single operation. If the
 * original entries had a tag number, it will be updated by the tag_increment,
 * while those with a zero tag will remain zero.
 *
 * reflect formerly performed two separate functions, reflecting for GX cards
 * or rotating for GR cards. The code was entirely separate for these two
 * functions, controlled by a long if statement. It made no sense to leave
 * them combined, so the handler for the GR case has been split out into its
 * own function, rotate.
 *
 * @param card_num card_t number that contains this instruction
 * @param tag_increment the number to increment the tag by, see notes below
 * @param ix see iz
 * @param iy see iz
 * @param iz flags indicating whether to relect on this axis
 *
 */
void reflect(nec_context_t *ctx, int card_num, int tag_increment, int ix, int iy, int iz)
{
  int iti, i, nx, itagi;
  size_t mreq;
  double e1, e2;

  // sanity check, formerly used nop>0 but we no longer pass that in
  if(ix == 0 && iy == 0 && iz == 0) {
    char msg[MAX_ERROR_LEN];
    snprintf(msg, sizeof(msg), "GX on line %d: no reflection axes.", card_num + 1);
    add_error(ctx, &ctx->geometry.errors, msg, 1);
    return;
  }

  /* GX card: NEC-2 spec says GX affects new structure only.
   * n0 = number of frozen NGF segments at the start of the array.
   * All loops start from n0 so NGF segments are left untouched.
   * Per spec: "GX will not result in use of symmetry in the solution"
   * when an NGF file is in use — ipsym is forced to 0 at the end. */
  int n0 = ctx->has_ngf ? ctx->ngf_n_segs : 0;

  // we are going to create symmetry one way or the other,
  // so we copy down how much geometry is in the symmetry "cell"
  ctx->geometry.num_segs_sym = ctx->geometry.num_segs - n0;
  ctx->geometry.num_patches_sym = ctx->geometry.num_patches;
  iti = tag_increment;

  // both GR and GX cards use only the I1 and I2 inputs in the card. I1 is
  // passed in the tag_increment, and I2 in num_copies. However, the I2 value
  // means different things in the two cards, in the GR card is is the number
  // of times to make a copy of the wires, for the GX is is a flag saying which
  // axes to reflect along. Since the flag value is a value number of copies
  // value, the code that calls reflect copies the I2 value into the ix, iy and iz
  // so to indicate if we are performing

  // we are now symmetric
  // FIXME: the original code for this is confusing, this should be reviewed
  ctx->geometry.symmetry_flag = 1;

  // reflect along z axis
  if(iz != 0) {
    ctx->geometry.symmetry_flag = 2;

    // copy existing wires if there are any
    if(ctx->geometry.num_segs > n0) {
      int nn = ctx->geometry.num_segs;
      int new_count = nn - n0;

      // reallocate cards and tags buffers
      mreq = (size_t)(nn + new_count + ctx->geometry.num_patches);
      mreq *= sizeof(int);
      mem_realloc(ctx, (void *)&ctx->geometry.tag_nums, mreq);
      mem_realloc(ctx, (void *)&ctx->geometry.card_nums, mreq);

      // Reallocate wire buffers
      mreq = (size_t)(nn + new_count);
      mreq *= sizeof(double);
      mem_realloc(ctx, (void *)&ctx->geometry.end1_x, mreq);
      mem_realloc(ctx, (void *)&ctx->geometry.end1_y, mreq);
      mem_realloc(ctx, (void *)&ctx->geometry.end1_z, mreq);
      mem_realloc(ctx, (void *)&ctx->geometry.end2_x, mreq);
      mem_realloc(ctx, (void *)&ctx->geometry.end2_y, mreq);
      mem_realloc(ctx, (void *)&ctx->geometry.end2_z, mreq);
      mem_realloc(ctx, (void *)&ctx->geometry.radius, mreq);

      for(i = n0; i < nn; i++) {
        // pack copies right after existing segments
        nx = nn + (i - n0);

        // get the existing z end points and test them
        e1 = ctx->geometry.end1_z[i];
        e2 = ctx->geometry.end2_z[i];

        if((fabs(e1) + fabs(e2) <= 1.0e-12) || (e1 * e2 < -1.0e-12)) {
          char l_msg[MAX_ERROR_LEN];
          snprintf(l_msg, sizeof(l_msg),
                  "\n  GEOMETRY DATA ERROR--SEGMENT %d"
                  " LIES IN PLANE OF SYMMETRY",
                  i + 1);
          add_error(ctx, &ctx->geometry.errors, l_msg, 1);
          return;
        }

        ctx->geometry.end1_x[nx] = ctx->geometry.end1_x[i];
        ctx->geometry.end1_y[nx] = ctx->geometry.end1_y[i];
        ctx->geometry.end1_z[nx] = -e1;
        ctx->geometry.end2_x[nx] = ctx->geometry.end2_x[i];
        ctx->geometry.end2_y[nx] = ctx->geometry.end2_y[i];
        ctx->geometry.end2_z[nx] = -e2;

        // get the last used tag num
        itagi = ctx->geometry.tag_nums[i];

        // now set the tag of the new entries to zero or that offset
        if(itagi == 0)
          ctx->geometry.tag_nums[nx] = 0;
        if(itagi != 0)
          ctx->geometry.tag_nums[nx]= itagi + iti;

        ctx->geometry.radius[nx]= ctx->geometry.radius[i];
      } /* for( i = n0; i < nn; i++ ) */

      // new count doubles the new structure (not the frozen NGF segments)
      ctx->geometry.num_segs = nn + new_count;

      // and that if we make more entries they need to be
      // offset by a greater number
      iti = iti * 2;
    } /* if( geometry.num_segs > n0) */

    // and now the patches, if there are any (patches are never NGF)
    if(ctx->geometry.num_patches > 0) {
      /* Reallocate patch buffers */
      mreq = (size_t)(2 * ctx->geometry.num_patches);
      mreq *= sizeof(double);
      mem_realloc(ctx, (void *)&ctx->geometry.patch_x_center, mreq);
      mem_realloc(ctx, (void *)&ctx->geometry.patch_y_center, mreq);
      mem_realloc(ctx, (void *)&ctx->geometry.patch_z_center, mreq);
      mem_realloc(ctx, (void *)&ctx->geometry.patch_t1x, mreq);
      mem_realloc(ctx, (void *)&ctx->geometry.patch_t1y, mreq);
      mem_realloc(ctx, (void *)&ctx->geometry.patch_t1z, mreq);
      mem_realloc(ctx, (void *)&ctx->geometry.patch_t2x, mreq);
      mem_realloc(ctx, (void *)&ctx->geometry.patch_t2y, mreq);
      mem_realloc(ctx, (void *)&ctx->geometry.patch_t2z, mreq);
      mem_realloc(ctx, (void *)&ctx->geometry.patch_area, mreq);
      mem_realloc(ctx, (void *)&ctx->geometry.patch_normal_z, mreq);

      for(i = 0; i < ctx->geometry.num_patches; i++) {
        nx = i+ctx->geometry.num_patches;
        if(fabs(ctx->geometry.patch_z_center[i]) <= 1.0e-10) {
          char l_msg[MAX_ERROR_LEN];
          snprintf(l_msg, sizeof(l_msg),
                  "\n  GEOMETRY DATA ERROR--PATCH %d"
                  " LIES IN PLANE OF SYMMETRY",
                  i + 1);
          add_error(ctx, &ctx->geometry.errors, l_msg, 1);
          return;
        }

        ctx->geometry.patch_x_center[nx]= ctx->geometry.patch_x_center[i];
        ctx->geometry.patch_y_center[nx]= ctx->geometry.patch_y_center[i];
        ctx->geometry.patch_z_center[nx]= -ctx->geometry.patch_z_center[i];
        ctx->geometry.patch_t1x[nx]= ctx->geometry.patch_t1x[i];
        ctx->geometry.patch_t1y[nx]= ctx->geometry.patch_t1y[i];
        ctx->geometry.patch_t1z[nx]= -ctx->geometry.patch_t1z[i];
        ctx->geometry.patch_t2x[nx]= ctx->geometry.patch_t2x[i];
        ctx->geometry.patch_t2y[nx]= ctx->geometry.patch_t2y[i];
        ctx->geometry.patch_t2z[nx]= -ctx->geometry.patch_t2z[i];
        ctx->geometry.patch_normal_z[nx]= -ctx->geometry.patch_normal_z[i];
        ctx->geometry.patch_area[nx]= ctx->geometry.patch_area[i];
      }

      ctx->geometry.num_patches= ctx->geometry.num_patches*2;
    } /* if( data.m >= m2) */
  } /* if( iz != 0) */

  // now repeat all of that for the y-axis
  if(iy != 0) {
    if(ctx->geometry.num_segs > n0) {
      int nn = ctx->geometry.num_segs;
      int new_count = nn - n0;

      /* Reallocate tags buffer */
      mreq = (size_t)(nn + new_count + ctx->geometry.num_patches);
      mreq *= sizeof(int);
      mem_realloc(ctx, (void *)&ctx->geometry.tag_nums, mreq);

      /* Reallocate wire buffers */
      mreq = (size_t)(nn + new_count);
      mreq *= sizeof(double);
      mem_realloc(ctx, (void *)&ctx->geometry.end1_x, mreq);
      mem_realloc(ctx, (void *)&ctx->geometry.end1_y, mreq);
      mem_realloc(ctx, (void *)&ctx->geometry.end1_z, mreq);
      mem_realloc(ctx, (void *)&ctx->geometry.end2_x, mreq);
      mem_realloc(ctx, (void *)&ctx->geometry.end2_y, mreq);
      mem_realloc(ctx, (void *)&ctx->geometry.end2_z, mreq);
      mem_realloc(ctx, (void *)&ctx->geometry.radius, mreq);

      for(i = n0; i < nn; i++) {
        nx = nn + (i - n0);
        e1= ctx->geometry.end1_y[i];
        e2= ctx->geometry.end2_y[i];

        if((fabs(e1)+fabs(e2) <= 1.0e-12) || (e1*e2 < -1.0e-12)) {
          char l_msg[MAX_ERROR_LEN];
          snprintf(l_msg, sizeof(l_msg),
                  "\n  GEOMETRY DATA ERROR--SEGMENT %d"
                  " LIES IN PLANE OF SYMMETRY",
                  i + 1);
          add_error(ctx, &ctx->geometry.errors, l_msg, 1);
          return;
        }

        ctx->geometry.end1_x[nx] = ctx->geometry.end1_x[i];
        ctx->geometry.end1_y[nx] = -e1;
        ctx->geometry.end1_z[nx] = ctx->geometry.end1_z[i];
        ctx->geometry.end2_x[nx] = ctx->geometry.end2_x[i];
        ctx->geometry.end2_y[nx] = -e2;
        ctx->geometry.end2_z[nx] = ctx->geometry.end2_z[i];
        itagi = ctx->geometry.tag_nums[i];

        if( itagi == 0)
          ctx->geometry.tag_nums[nx]=0;
        if( itagi != 0)
          ctx->geometry.tag_nums[nx]= itagi+ iti;

        ctx->geometry.radius[nx]= ctx->geometry.radius[i];

      } /* for( i = n0; i < nn; i++ ) */

      ctx->geometry.num_segs = nn + new_count;
      iti= iti*2;

    } /* if( geometry.num_segs > n0) */

    // reflect any patches
    if(ctx->geometry.num_patches > 0)  {
      // reflection doubles the number of patches, so we start
      // by reallocating the patch list to hold the new ones
      mreq = (size_t)(2 * ctx->geometry.num_patches);
      mreq *= sizeof(double);
      mem_realloc(ctx, (void *)&ctx->geometry.patch_x_center, mreq);
      mem_realloc(ctx, (void *)&ctx->geometry.patch_y_center, mreq);
      mem_realloc(ctx, (void *)&ctx->geometry.patch_z_center, mreq);
      mem_realloc(ctx, (void *)&ctx->geometry.patch_t1x, mreq);
      mem_realloc(ctx, (void *)&ctx->geometry.patch_t1y, mreq);
      mem_realloc(ctx, (void *)&ctx->geometry.patch_t1z, mreq);
      mem_realloc(ctx, (void *)&ctx->geometry.patch_t2x, mreq);
      mem_realloc(ctx, (void *)&ctx->geometry.patch_t2y, mreq);
      mem_realloc(ctx, (void *)&ctx->geometry.patch_t2z, mreq);
      mem_realloc(ctx, (void *)&ctx->geometry.patch_area, mreq);
      mem_realloc(ctx, (void *)&ctx->geometry.patch_normal_z, mreq);

      for( i = 0; i < ctx->geometry.num_patches; i++ ) {
        nx= i+ctx->geometry.num_patches;
        if( fabs( ctx->geometry.patch_y_center[i]) <= 1.0e-10) {
          char l_msg[MAX_ERROR_LEN];
          snprintf(l_msg, sizeof(l_msg),
                  "\n  GEOMETRY DATA ERROR--PATCH %d"
                  " LIES IN PLANE OF SYMMETRY",
                  i + 1);
          add_error(ctx, &ctx->geometry.errors, l_msg, 1);
          return;
        }

        ctx->geometry.patch_x_center[nx]= ctx->geometry.patch_x_center[i];
        ctx->geometry.patch_y_center[nx]= -ctx->geometry.patch_y_center[i];
        ctx->geometry.patch_z_center[nx]= ctx->geometry.patch_z_center[i];
        ctx->geometry.patch_t1x[nx]= -ctx->geometry.patch_t1x[i];
        ctx->geometry.patch_t1y[nx]= ctx->geometry.patch_t1y[i];
        ctx->geometry.patch_t1z[nx]= ctx->geometry.patch_t1z[i];
        ctx->geometry.patch_t2x[nx]= -ctx->geometry.patch_t2x[i];
        ctx->geometry.patch_t2y[nx]= ctx->geometry.patch_t2y[i];
        ctx->geometry.patch_t2z[nx]= ctx->geometry.patch_t2z[i];
        ctx->geometry.patch_normal_z[nx]= -ctx->geometry.patch_normal_z[i];
        ctx->geometry.patch_area[nx]= ctx->geometry.patch_area[i];

      } /* for( i = m2; i <= ctx->geometry.num_patches; i++ ) */

      ctx->geometry.num_patches= ctx->geometry.num_patches * 2;
    } /* if( ctx->geometry.num_patches >= m2) */
  } /* if( iy != 0) */

  // and finally the x axis
  if(ix == 0) {
    /* When NGF is active, clear symmetry flag — per NEC-2 spec GX does not
     * result in use of symmetry in the solution when NGF is in use. */
    if(ctx->has_ngf) ctx->geometry.symmetry_flag = 0;
    return;
  }

  if( ctx->geometry.num_segs > n0 ) {
    int nn = ctx->geometry.num_segs;
    int new_count = nn - n0;

    /* Reallocate tags buffer */
    mreq = (size_t)(nn + new_count + ctx->geometry.num_patches);
    mreq *= sizeof(int);
    mem_realloc(ctx, (void *)&ctx->geometry.tag_nums, mreq);

    /* Reallocate wire buffers */
    mreq = (size_t)(nn + new_count);
    mreq *= sizeof(double);
    mem_realloc(ctx, (void *)&ctx->geometry.end1_x, mreq);
    mem_realloc(ctx, (void *)&ctx->geometry.end1_y, mreq);
    mem_realloc(ctx, (void *)&ctx->geometry.end1_z, mreq);
    mem_realloc(ctx, (void *)&ctx->geometry.end2_x, mreq);
    mem_realloc(ctx, (void *)&ctx->geometry.end2_y, mreq);
    mem_realloc(ctx, (void *)&ctx->geometry.end2_z, mreq);
    mem_realloc(ctx, (void *)&ctx->geometry.radius, mreq);

    for(i = n0; i < nn; i++) {
      nx = nn + (i - n0);
      e1= ctx->geometry.end1_x[i];
      e2= ctx->geometry.end2_x[i];

      if( (fabs(e1)+fabs(e2) <= 1.0e-12) || (e1*e2 < -1.0e-12) ) {
        char l_msg[MAX_ERROR_LEN];
        snprintf(l_msg, sizeof(l_msg),
                "\n  GEOMETRY DATA ERROR--SEGMENT %d"
                " LIES IN PLANE OF SYMMETRY",
                i + 1);
        add_error(ctx, &ctx->geometry.errors, l_msg, 1);
        return;
      }

      ctx->geometry.end1_x[nx]= -e1;
      ctx->geometry.end1_y[nx]= ctx->geometry.end1_y[i];
      ctx->geometry.end1_z[nx]= ctx->geometry.end1_z[i];
      ctx->geometry.end2_x[nx]= -e2;
      ctx->geometry.end2_y[nx]= ctx->geometry.end2_y[i];
      ctx->geometry.end2_z[nx]= ctx->geometry.end2_z[i];
      itagi= ctx->geometry.tag_nums[i];

      if(itagi == 0)
        ctx->geometry.tag_nums[nx]=0;
      if(itagi != 0)
        ctx->geometry.tag_nums[nx]= itagi + iti;

      ctx->geometry.radius[nx]= ctx->geometry.radius[i];
    }

    ctx->geometry.num_segs = nn + new_count;

  } /* if( data.n > n0) */

  if(ctx->geometry.num_patches == 0) {
    /* When NGF is active, clear symmetry flag. */
    if(ctx->has_ngf) ctx->geometry.symmetry_flag = 0;
    return;
  }

  /* Reallocate patch buffers */
  mreq = (size_t)(2 * ctx->geometry.num_patches);
  mreq *= sizeof(double);
  mem_realloc(ctx, (void *)&ctx->geometry.patch_x_center, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.patch_y_center, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.patch_z_center, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.patch_t1x, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.patch_t1y, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.patch_t1z, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.patch_t2x, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.patch_t2y, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.patch_t2z, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.patch_area, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.patch_normal_z, mreq);

  for( i = 0; i < ctx->geometry.num_patches; i++ ) {
    nx = i+ctx->geometry.num_patches;
    if(fabs(ctx->geometry.patch_x_center[i]) <= 1.0e-10) {
      char l_msg[MAX_ERROR_LEN];
      snprintf(l_msg, sizeof(l_msg),
              "\n  GEOMETRY DATA ERROR--PATCH %d"
              " LIES IN PLANE OF SYMMETRY",
              i + 1);
      add_error(ctx, &ctx->geometry.errors, l_msg, 1);
      return;
    }

    ctx->geometry.patch_x_center[nx]= -ctx->geometry.patch_x_center[i];
    ctx->geometry.patch_y_center[nx]= ctx->geometry.patch_y_center[i];
    ctx->geometry.patch_z_center[nx]= ctx->geometry.patch_z_center[i];
    ctx->geometry.patch_t1x[nx]= -ctx->geometry.patch_t1x[i];
    ctx->geometry.patch_t1y[nx]= ctx->geometry.patch_t1y[i];
    ctx->geometry.patch_t1z[nx]= ctx->geometry.patch_t1z[i];
    ctx->geometry.patch_t2x[nx]= -ctx->geometry.patch_t2x[i];
    ctx->geometry.patch_t2y[nx]= ctx->geometry.patch_t2y[i];
    ctx->geometry.patch_t2z[nx]= ctx->geometry.patch_t2z[i];
    ctx->geometry.patch_normal_z[nx]= -ctx->geometry.patch_normal_z[i];
    ctx->geometry.patch_area[nx]= ctx->geometry.patch_area[i];
  }

  ctx->geometry.num_patches= ctx->geometry.num_patches * 2;

  /* When NGF is active, clear symmetry flag. */
  if(ctx->has_ngf) ctx->geometry.symmetry_flag = 0;
} /* end of reflect */

/******************************************************************************
 * rotate
 *
 * rotate creates new geometry entries for all existing entries to create
 * a rotation around the Z axis. If the original entries had a tag number,
 * it will be updated by the tag_increment, while those with a zero tag will
 * remain zero.
 *
 * rotate was formerly part of reflect, although the code was entirely
 * separate, so it has been moved it its own function for clarity.
 *
 * @param card_num card_t number that contains this instruction
 * @param tag_increment the number to increment the tag by, see notes below
 * @param num_copies number of new copies to produce
 *
 */
void rotate(nec_context_t *ctx, int card_num, int tag_increment, int num_copies)
{
  int nx, itagi, k;
  size_t mreq;
  double fnop, sam, cs, ss, xk, yk;

  /* GR card: NEC-2 spec says GR affects new structure only, not NGF segments.
   * n0 = number of frozen NGF segments.  The symmetry period np is set to
   * new_count (non-NGF segments).  Per spec: "GR will not result in use of
   * symmetry in the solution" when NGF is in use — ipsym is forced to 0. */
  int n0 = ctx->has_ngf ? ctx->ngf_n_segs : 0;
  int new_count = ctx->geometry.num_segs - n0;

  // we are going to create symmetry around the Z axis
  ctx->geometry.num_segs_sym = new_count;
  ctx->geometry.num_patches_sym = ctx->geometry.num_patches;
  ctx->geometry.symmetry_flag = -1;      // rotational symmetry

  // reproduce structure with rotation to form cylindrical structure
  fnop = (double)num_copies;
  sam = TP / fnop;
  cs = cos(sam);
  ss = sin(sam);

  if(new_count > 0) {
    // total segments after rotation: n0 frozen + new_count * num_copies
    int n_new_total = n0 + new_count * num_copies;

    nx = n0 + new_count;   // first copy starts here

    //r eallocate cards and tags buffers
    mreq = (size_t)(n_new_total + ctx->geometry.num_patches);
    mreq *= sizeof(int);
    mem_realloc(ctx, (void *)&ctx->geometry.tag_nums, mreq);
    mem_realloc(ctx, (void *)&ctx->geometry.card_nums, mreq);

    // reallocate wire buffers
    mreq = (size_t)n_new_total;
    mreq *= sizeof(double);
    mem_realloc(ctx, (void *)&ctx->geometry.end1_x, mreq);
    mem_realloc(ctx, (void *)&ctx->geometry.end1_y, mreq);
    mem_realloc(ctx, (void *)&ctx->geometry.end1_z, mreq);
    mem_realloc(ctx, (void *)&ctx->geometry.end2_x, mreq);
    mem_realloc(ctx, (void *)&ctx->geometry.end2_y, mreq);
    mem_realloc(ctx, (void *)&ctx->geometry.end2_z, mreq);
    mem_realloc(ctx, (void *)&ctx->geometry.radius, mreq);

    for(int i = nx; i < n_new_total; i++ ) {
      // cycle through the original new segments
      k = n0 + (i - n0) % new_count;
      xk = ctx->geometry.end1_x[k];
      yk = ctx->geometry.end1_y[k];
      ctx->geometry.end1_x[i]= xk* cs- yk* ss;
      ctx->geometry.end1_y[i]= xk* ss+ yk* cs;
      ctx->geometry.end1_z[i]= ctx->geometry.end1_z[k];
      xk= ctx->geometry.end2_x[k];
      yk= ctx->geometry.end2_y[k];
      ctx->geometry.end2_x[i]= xk* cs- yk* ss;
      ctx->geometry.end2_y[i]= xk* ss+ yk* cs;
      ctx->geometry.end2_z[i]= ctx->geometry.end2_z[k];
      ctx->geometry.radius[i]= ctx->geometry.radius[k];
      itagi= ctx->geometry.tag_nums[k];

      if(itagi == 0)
        ctx->geometry.tag_nums[i] = 0;
      if( itagi != 0) {
        int copy_num = (i - n0) / new_count;  /* 1 for first copy, 2 for second, etc. */
        ctx->geometry.tag_nums[i] = itagi + copy_num * tag_increment;
      }

      ctx->geometry.card_nums[i] = card_num;
    }

    ctx->geometry.num_segs = n_new_total;
  } /* if( new_count > 0) */

  /* When NGF is active, clear symmetry flag per NEC-2 spec. */
  if(ctx->has_ngf) ctx->geometry.symmetry_flag = 0;
  
  // now do it all again for the patches if there are any
  // FIXME: this doesn't see to record tag or card numbers, did that happen above?
  if(ctx->geometry.num_patches == 0)
    return;
  
  ctx->geometry.num_patches *= num_copies;
  nx = ctx->geometry.num_patches_sym;
  
  /* Reallocate patch buffers */
  mreq = (size_t)ctx->geometry.num_patches;
  mreq *= sizeof(double);
  mem_realloc(ctx, (void *)&ctx->geometry.patch_x_center, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.patch_y_center, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.patch_z_center, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.patch_t1x, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.patch_t1y, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.patch_t1z, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.patch_t2x, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.patch_t2y, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.patch_t2z, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.patch_area, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.patch_normal_z, mreq);
  
  for(int i = nx; i < ctx->geometry.num_patches; i++) {
    k = i-ctx->geometry.num_patches_sym;
    xk= ctx->geometry.patch_x_center[k];
    yk= ctx->geometry.patch_y_center[k];
    ctx->geometry.patch_x_center[i]= xk* cs- yk* ss;
    ctx->geometry.patch_y_center[i]= xk* ss+ yk* cs;
    ctx->geometry.patch_z_center[i]= ctx->geometry.patch_z_center[k];
    xk= ctx->geometry.patch_t1x[k];
    yk= ctx->geometry.patch_t1y[k];
    ctx->geometry.patch_t1x[i]= xk* cs- yk* ss;
    ctx->geometry.patch_t1y[i]= xk* ss+ yk* cs;
    ctx->geometry.patch_t1z[i]= ctx->geometry.patch_t1z[k];
    xk= ctx->geometry.patch_t2x[k];
    yk= ctx->geometry.patch_t2y[k];
    ctx->geometry.patch_t2x[i]= xk* cs- yk* ss;
    ctx->geometry.patch_t2y[i]= xk* ss+ yk* cs;
    ctx->geometry.patch_t2z[i]= ctx->geometry.patch_t2z[k];
    ctx->geometry.patch_normal_z[i]= ctx->geometry.patch_normal_z[k];
    ctx->geometry.patch_area[i]= ctx->geometry.patch_area[k];
  } /* for( i = nx; i < data.m; i++ ) */
} /* end of rotate */

/******************************************************************************
 * patch
 *
 * patch creates a surface patch of one of three types, including a free-form
 * surface that is defined on multiple cards.
 *
 * FIXME: this should be broken into two methods, single_patch and multi_
 *
 * @param card_num card_t number that contains this instruction
 * @param nx the number of patches to generate in x...
 * @param ny ... and y.
 *
 */
void patch(nec_context_t *ctx, geometry_t *geom, int card_num, int nx, int ny,
           double ax1, double ay1, double az1,
           double ax2, double ay2, double az2,
           double ax3, double ay3, double az3,
           double ax4, double ay4, double az4)
{
  int mi, ntp;
  size_t mreq;
  double s1x=0.0, s1y=0.0, s1z=0.0, s2x=0.0, s2y=0.0, s2z=0.0, xst=0.0;
  double znv, xnv, ynv, xa, xn2, yn2, zn2;
  
  // new patches. for nx=0, ny=1,2,3,4 patch is (respectively)
  // arbitrary, rectangular, triangular, or quadrilateral.
  // for nx and ny > 0 a rectangular surface is produced with
  // nx by ny rectangular patches.
  
  geom->num_patches++;
  mi = geom->num_patches - 1;
  
  // reallocate patch buffers
  mreq = (size_t)geom->num_patches;
  mreq *= sizeof(double);
  mem_realloc(ctx, (void *)&geom->patch_x_center, mreq);
  mem_realloc(ctx, (void *)&geom->patch_y_center, mreq);
  mem_realloc(ctx, (void *)&geom->patch_z_center, mreq);
  mem_realloc(ctx, (void *)&geom->patch_t1x, mreq);
  mem_realloc(ctx, (void *)&geom->patch_t1y, mreq);
  mem_realloc(ctx, (void *)&geom->patch_t1z, mreq);
  mem_realloc(ctx, (void *)&geom->patch_t2x, mreq);
  mem_realloc(ctx, (void *)&geom->patch_t2y, mreq);
  mem_realloc(ctx, (void *)&geom->patch_t2z, mreq);
  mem_realloc(ctx, (void *)&geom->patch_area, mreq);
  mem_realloc(ctx, (void *)&geom->patch_normal_z, mreq);
  
  if(nx > 0)
    ntp = 2;
  else
    ntp = ny;
  
  if(ntp <= 1) {
    geom->patch_x_center[mi] = ax1;
    geom->patch_y_center[mi] = ay1;
    geom->patch_z_center[mi] = az1;
    geom->patch_area[mi] = az2;
    znv = cos(ax2);
    xnv = znv * cos(ay2);
    ynv = znv * sin(ay2);
    znv = sin(ax2);
    xa = sqrt(xnv * xnv+ ynv * ynv);
    
    if(xa >= 1.0e-6) {
      geom->patch_t1x[mi] = -ynv/ xa;
      geom->patch_t1y[mi] = xnv/ xa;
      geom->patch_t1z[mi] = 0.0;
    } else {
      geom->patch_t1x[mi]=1.;
      geom->patch_t1y[mi]=0.;
      geom->patch_t1z[mi]=0.;
    }
    
  } /* if( ntp <= 1) */
  else {
    s1x = ax2 - ax1;
    s1y = ay2 - ay1;
    s1z = az2 - az1;
    s2x = ax3 - ax2;
    s2y = ay3 - ay2;
    s2z = az3 - az2;
    
    if(nx != 0) {
      s1x = s1x / nx;
      s1y = s1y / nx;
      s1z = s1z / nx;
      s2x = s2x / ny;
      s2y = s2y / ny;
      s2z = s2z / ny;
    }
    
    xnv = s1y * s2z - s1z * s2y;
    ynv = s1z * s2x - s1x * s2z;
    znv = s1x * s2y - s1y * s2x;
    xa = sqrt(xnv * xnv + ynv * ynv + znv * znv);
    xnv = xnv/ xa;
    ynv = ynv/ xa;
    znv = znv/ xa;
    xst = sqrt( s1x* s1x+ s1y* s1y+ s1z* s1z);
    geom->patch_t1x[mi] = s1x / xst;
    geom->patch_t1y[mi] = s1y / xst;
    geom->patch_t1z[mi] = s1z / xst;
    
    if(ntp <= 2) {
      geom->patch_x_center[mi] = ax1 + 0.5 * (s1x + s2x);
      geom->patch_y_center[mi] = ay1 + 0.5 * (s1y + s2y);
      geom->patch_z_center[mi] = az1 + 0.5 * (s1z + s2z);
      geom->patch_area[mi] = xa;
    }
    else {
      if( ntp != 4) {
        geom->patch_x_center[mi] = (ax1 + ax2 + ax3) / 3.0;
        geom->patch_y_center[mi] = (ay1 + ay2 + ay3) / 3.0;
        geom->patch_z_center[mi] = (az1 + az2 + az3) / 3.0;
        geom->patch_area[mi] = 0.5 * xa;
      }
      else  {
        double salpn;
        s1x= ax3- ax1;
        s1y= ay3- ay1;
        s1z= az3- az1;
        s2x= ax4- ax1;
        s2y= ay4- ay1;
        s2z= az4- az1;
        xn2= s1y* s2z- s1z* s2y;
        yn2= s1z* s2x- s1x* s2z;
        zn2= s1x* s2y- s1y* s2x;
        xst= sqrt( xn2* xn2+ yn2* yn2+ zn2* zn2);
        salpn=1./(3.*( xa+ xst));
        geom->patch_x_center[mi]=( xa*( ax1+ ax2+ ax3)+ xst*( ax1+ ax3+ ax4))* salpn;
        geom->patch_y_center[mi]=( xa*( ay1+ ay2+ ay3)+ xst*( ay1+ ay3+ ay4))* salpn;
        geom->patch_z_center[mi]=( xa*( az1+ az2+ az3)+ xst*( az1+ az3+ az4))* salpn;
        geom->patch_area[mi]=.5*( xa+ xst);
        s1x=( xnv* xn2+ ynv* yn2+ znv* zn2)/ xst;
        
        if(s1x <= 0.9998) {
          char msg[MAX_ERROR_LEN];
          snprintf(msg, sizeof(msg),
                  "\n  ERROR -- CORNERS OF QUADRILATERAL"
                  " PATCH DO NOT LIE IN A PLANE" );
          add_error(ctx, &ctx->geometry.errors, msg, 1);
          return;
        }
      } /* if( ntp != 4) */
    } /* if( ntp <= 2) */
  } /* if( ntp <= 1) */
  
  geom->patch_t2x[mi] = ynv * geom->patch_t1z[mi] - znv * geom->patch_t1y[mi];
  geom->patch_t2y[mi] = znv * geom->patch_t1x[mi] - xnv * geom->patch_t1z[mi];
  geom->patch_t2z[mi] = xnv * geom->patch_t1y[mi] - ynv * geom->patch_t1x[mi];
  geom->patch_normal_z[mi] = 1.0;
  
  if(nx != 0) {
    int iy, ix;
    double xs, ys, zs, xt, yt, zt;
    
    geom->num_patches += nx * ny - 1;
    // reallocate patch buffers
    mreq = (size_t)geom->num_patches;
    mreq *= sizeof(double);
    mem_realloc(ctx, (void *)&geom->patch_x_center, mreq);
    mem_realloc(ctx, (void *)&geom->patch_y_center, mreq);
    mem_realloc(ctx, (void *)&geom->patch_z_center, mreq);
    mem_realloc(ctx, (void *)&geom->patch_t1x, mreq);
    mem_realloc(ctx, (void *)&geom->patch_t1y, mreq);
    mem_realloc(ctx, (void *)&geom->patch_t1z, mreq);
    mem_realloc(ctx, (void *)&geom->patch_t2x, mreq);
    mem_realloc(ctx, (void *)&geom->patch_t2y, mreq);
    mem_realloc(ctx, (void *)&geom->patch_t2z, mreq);
    mem_realloc(ctx, (void *)&geom->patch_area, mreq);
    mem_realloc(ctx, (void *)&geom->patch_normal_z, mreq);
    
    xn2 = geom->patch_x_center[mi] - s1x - s2x;
    yn2 = geom->patch_y_center[mi] - s1y - s2y;
    zn2 = geom->patch_z_center[mi] - s1z - s2z;
    xs = geom->patch_t1x[mi];
    ys = geom->patch_t1y[mi];
    zs = geom->patch_t1z[mi];
    xt = geom->patch_t2x[mi];
    yt = geom->patch_t2y[mi];
    zt = geom->patch_t2z[mi];
    
    for(iy = 0; iy < ny; iy++) {
      xn2 += s2x;
      yn2 += s2y;
      zn2 += s2z;
      
      for(ix = 1; ix <= nx; ix++) {
        xst= (double)ix;
        geom->patch_x_center[mi] = xn2+ xst* s1x;
        geom->patch_y_center[mi] = yn2+ xst* s1y;
        geom->patch_z_center[mi] = zn2+ xst* s1z;
        geom->patch_area[mi] = xa;
        geom->patch_normal_z[mi] =1.;
        geom->patch_t1x[mi] = xs;
        geom->patch_t1y[mi] = ys;
        geom->patch_t1z[mi] = zs;
        geom->patch_t2x[mi] = xt;
        geom->patch_t2y[mi] = yt;
        geom->patch_t2z[mi] = zt;
        mi++;
      } /* for( ix = 0; ix < nx; ix++ ) */
    } /* for( iy = 0; iy < ny; iy++ ) */
  } /* if( nx != 0) */
  
  // reset symmetry
  // TODO: why is this at the end? other methods have it at the top
  geom->symmetry_flag = 0;
  geom->num_segs_sym = geom->num_segs;
  geom->num_patches_sym = geom->num_patches;
} /* end of patch */

/******************************************************************************
 * calculate_patch (formerly subph) was an entry point (part of)
 * patch()
 *
 */
void calculate_patch(nec_context_t *ctx, int nx, int ny )
{
  int mia, ix, iy, mi;
  size_t mreq;
  double xs, ys, zs, xa, xst, s1x, s1y, s1z, s2x, s2y, s2z, saln, xt, yt;
  
  // reallocate patch buffers
  if(ny == 0) {
    ctx->geometry.num_patches += 3;
  } else {
    ctx->geometry.num_patches += 4;
  }
  
  mreq = (size_t)ctx->geometry.num_patches;
  mreq *= sizeof(double);
  mem_realloc(ctx, (void *)&ctx->geometry.patch_x_center, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.patch_y_center, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.patch_z_center, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.patch_t1x, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.patch_t1y, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.patch_t1z, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.patch_t2x, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.patch_t2y, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.patch_t2z, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.patch_area, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.patch_normal_z, mreq);
  mreq = (size_t)(ctx->geometry.num_segs + ctx->geometry.num_patches);
  mreq *= sizeof(int);
  mem_realloc(ctx, (void *)&ctx->geometry.seg_end1_conn, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.seg_end2_conn, mreq);
  
  // shift patches to make room for new ones
  if((ny == 0) && (nx != ctx->geometry.num_patches))  {
    for(iy = ctx->geometry.num_patches - 1; iy > nx+2; iy--) {
      ix = iy-3;
      ctx->geometry.patch_x_center[iy]= ctx->geometry.patch_x_center[ix];
      ctx->geometry.patch_y_center[iy]= ctx->geometry.patch_y_center[ix];
      ctx->geometry.patch_z_center[iy]= ctx->geometry.patch_z_center[ix];
      ctx->geometry.patch_area[iy]= ctx->geometry.patch_area[ix];
      ctx->geometry.patch_normal_z[iy]= ctx->geometry.patch_normal_z[ix];
      ctx->geometry.patch_t1x[iy]= ctx->geometry.patch_t1x[ix];
      ctx->geometry.patch_t1y[iy]= ctx->geometry.patch_t1y[ix];
      ctx->geometry.patch_t1z[iy]= ctx->geometry.patch_t1z[ix];
      ctx->geometry.patch_t2x[iy]= ctx->geometry.patch_t2x[ix];
      ctx->geometry.patch_t2y[iy]= ctx->geometry.patch_t2y[ix];
      ctx->geometry.patch_t2z[iy]= ctx->geometry.patch_t2z[ix];
    }
  } /* if( (ny == 0) || (nx != m) ) */
  
  /* divide patch for connection */
  mi = nx-1;
  xs = ctx->geometry.patch_x_center[mi];
  ys = ctx->geometry.patch_y_center[mi];
  zs = ctx->geometry.patch_z_center[mi];
  xa = ctx->geometry.patch_area[mi] / 4.0;
  xst = sqrt(xa) / 2.0;
  s1x = ctx->geometry.patch_t1x[mi];
  s1y = ctx->geometry.patch_t1y[mi];
  s1z = ctx->geometry.patch_t1z[mi];
  s2x = ctx->geometry.patch_t2x[mi];
  s2y = ctx->geometry.patch_t2y[mi];
  s2z = ctx->geometry.patch_t2z[mi];
  saln = ctx->geometry.patch_normal_z[mi];
  xt = xst;
  yt = xst;
  
  if(ny == 0)
    mia= mi;
  else {
    ctx->geometry.num_patches_sym++;
    mia = ctx->geometry.num_patches - 1;
  }
  
  for(ix = 1; ix <= 4; ix++) {
    ctx->geometry.patch_x_center[mia]= xs+ xt* s1x+ yt* s2x;
    ctx->geometry.patch_y_center[mia]= ys+ xt* s1y+ yt* s2y;
    ctx->geometry.patch_z_center[mia]= zs+ xt* s1z+ yt* s2z;
    ctx->geometry.patch_area[mia]= xa;
    ctx->geometry.patch_t1x[mia]= s1x;
    ctx->geometry.patch_t1y[mia]= s1y;
    ctx->geometry.patch_t1z[mia]= s1z;
    ctx->geometry.patch_t2x[mia]= s2x;
    ctx->geometry.patch_t2y[mia]= s2y;
    ctx->geometry.patch_t2z[mia]= s2z;
    ctx->geometry.patch_normal_z[mia]= saln;
    
    if(ix == 2)
      yt= -yt;
    
    if((ix == 1) || (ix == 3))
      xt= -xt;
    
    mia++;
  }
  
  if(nx <= ctx->geometry.num_patches_sym)
    ctx->geometry.num_patches_sym += 3;
  
  if(ny > 0)
    ctx->geometry.patch_z_center[mi] = 10000.0;
} /* end of calculate_patch */


/******************************************************************************
 * compute_segmentation()
 *
 * Compute the MMANA-GAL tapering segmentation plan for a single wire.
 *
 * Implements the algorithm described in the "Segmentation" and "How MMANA-GAL
 * Segmentation Process Operates" sections of the MMANA-GAL basic help:
 * https://hamsoft.ca/pages/mmana-gal/mmana-gal_basic_help/
 *
 * The MMANA SEG values:
 *   > 0  manual exact count  (caller's value used as-is, like NEC)
 *     0  automatic uniform   (λ/DM2 throughout)
 *    -1  taper at both ends  (recommended; default MMANA setting)
 *    -2  taper at start end only
 *    -3  taper at end end only
 *
 * Taper sequence from each wire end inward:
 *   [EC segments of length λ/DM1]  [λ/DM1·SC]  [λ/DM1·SC²]  ...
 *   ... (stepping by factor SC until the step exceeds λ/DM2)
 *   [remaining middle filled with λ/DM2 segments]
 *
 * The groups[] array in the optional plan is ordered from the start end to
 * the end end of the wire, providing the exact sub-wire breakdown needed to
 * emit a accurate sequence of NEC GW cards.
 *
 * @param wire_len   Physical length of the wire (metres).
 * @param wavelength Wavelength at the operating frequency (metres).
 * @param seg_mode   Segmentation mode (see seg_mode_t and above).
 * @param dm1        Finest-segment divisor; end-segment length = λ/DM1.
 * @param dm2        Coarsest-segment divisor; middle-segment length = λ/DM2.
 * @param sc         Geometric growth factor between taper steps (> 1).
 * @param ec         Count of finest segments at each tapered end (≥ 1).
 * @param plan       If non-NULL, filled with per-sub-wire breakdown.
 * @return Total segment count (≥ 1), or -1 for invalid input.
 *
 *****************************************************************************/
int compute_segmentation(double wire_len, double wavelength,
                         int seg_mode,
                         int dm1, int dm2, double sc, int ec,
                         seg_plan_t *plan)
{
    if (wire_len <= 0.0 || wavelength <= 0.0)
        return -1;

    /* ---- Manual: use the supplied count unchanged ---- */
    if (seg_mode > 0) {
        if (plan) {
            plan->total_segs  = seg_mode;
            plan->n_groups    = 1;
            plan->groups[0].segs = seg_mode;
            plan->groups[0].frac = 1.0;
        }
        return seg_mode;
    }

    /* ---- Sanitise taper parameters ---- */
    if (dm1 <= 0) dm1 = 200;
    if (dm2 <= 0) dm2 = 20;
    /* dm1 must be ≥ dm2 so that λ/dm1 ≤ λ/dm2 (fine ≤ coarse) */
    if (dm1 < dm2) { int tmp = dm1; dm1 = dm2; dm2 = tmp; }
    if (sc  < 1.01) sc = 2.0;
    if (sc  > 3.0)  sc = 3.0;
    if (ec  < 1)    ec = 1;

    double seg_fine   = wavelength / (double)dm1;   /* finest  (wire end) */
    double seg_coarse = wavelength / (double)dm2;   /* coarsest (middle) */

    /* Accuracy floor: segments < 0.001 λ are unreliable (MMANA-GAL guidance) */
    double seg_floor = wavelength * 0.001;
    if (seg_fine < seg_floor) seg_fine = seg_floor;
    /* coarse cannot be finer than fine */
    if (seg_coarse < seg_fine) seg_coarse = seg_fine;

    /* ---- Uniform (seg_mode == 0): fill with coarse segments ---- */
    if (seg_mode == 0) {
        int n = (int)ceil(wire_len / seg_coarse);
        if (n < 1) n = 1;
        if (plan) {
            plan->total_segs     = n;
            plan->n_groups       = 1;
            plan->groups[0].segs = n;
            plan->groups[0].frac = 1.0;
        }
        return n;
    }

    /* ---- Tapered segmentation (-1 / -2 / -3) ---- */

    /*
     * Build the geometric growth sequence for one tapered end.
     * step_lens[0] = seg_fine  (the EC-flat-segment length; EC segments)
     * step_lens[1] = seg_fine * sc   (one segment)
     * step_lens[2] = seg_fine * sc²  (one segment)
     * ...  until the step length first equals or exceeds seg_coarse.
     *
     * step_segs[i] is the segment count for that step:
     *   i == 0  →  ec  (flat block of finest segments)
     *   i >  0  →  1
     */
    double step_lens[MAX_SEG_GROUPS / 2];
    int    step_segs[MAX_SEG_GROUPS / 2];
    int n_steps = 0;
    int max_steps = MAX_SEG_GROUPS / 2 - 1;

    step_lens[n_steps] = seg_fine;
    step_segs[n_steps] = ec;
    n_steps++;

    double s = seg_fine * sc;
    while (s < seg_coarse - 1e-12 && n_steps < max_steps) {
        step_lens[n_steps] = s;
        step_segs[n_steps] = 1;
        n_steps++;
        s *= sc;
    }

    /* Total wire length consumed by one tapered end */
    double end_len = 0.0;
    for (int i = 0; i < n_steps; i++)
        end_len += step_lens[i] * step_segs[i];

    /* Number of tapered ends (1 or 2) */
    int n_ends = (seg_mode == -1) ? 2 : 1;

    /* Middle: whatever remains after the taper(s) */
    double taper_total = n_ends * end_len;
    double middle_len  = wire_len - taper_total;
    if (middle_len < 0.0) {
        /* Wire too short for the full taper — fall back to uniform fine */
        int n = (int)ceil(wire_len / seg_fine);
        if (n < 1) n = 1;
        if (plan) {
            plan->total_segs     = n;
            plan->n_groups       = 1;
            plan->groups[0].segs = n;
            plan->groups[0].frac = 1.0;
        }
        return n;
    }

    int middle_segs = (middle_len > 0.0) ? (int)ceil(middle_len / seg_coarse) : 0;

    /* Count segments in one end taper */
    int end_segs = 0;
    for (int i = 0; i < n_steps; i++)
        end_segs += step_segs[i];

    int total = n_ends * end_segs + middle_segs;
    if (total < 1) total = 1;

    /* ---- Optionally fill the plan ---- */
    if (plan) {
        plan->total_segs = total;
        plan->n_groups   = 0;

        /* Start-end taper (present for modes -1 and -2) */
        if (seg_mode == -1 || seg_mode == -2) {
            for (int i = 0; i < n_steps && plan->n_groups < MAX_SEG_GROUPS; i++) {
                plan->groups[plan->n_groups].segs = step_segs[i];
                plan->groups[plan->n_groups].frac = (step_lens[i] * step_segs[i]) / wire_len;
                plan->n_groups++;
            }
        }

        /* Middle uniform block */
        if (middle_segs > 0 && plan->n_groups < MAX_SEG_GROUPS) {
            plan->groups[plan->n_groups].segs = middle_segs;
            plan->groups[plan->n_groups].frac = middle_len / wire_len;
            plan->n_groups++;
        }

        /* End taper (present for modes -1 and -3) — mirror of start-end, reversed */
        if (seg_mode == -1 || seg_mode == -3) {
            for (int i = n_steps - 1; i >= 0 && plan->n_groups < MAX_SEG_GROUPS; i--) {
                plan->groups[plan->n_groups].segs = step_segs[i];
                plan->groups[plan->n_groups].frac = (step_lens[i] * step_segs[i]) / wire_len;
                plan->n_groups++;
            }
        }
    }

    return total;
}

/* end of geometry.c */
