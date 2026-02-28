FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 170
CALL: add_error(ctx, errors, msg, 2); // this is a critical error, this deck will not process
--- Context (lines 110-175) ---
    110:   // zero or more CM (comment) cards
    111:   // one CE (comment end) card
    112:   // one or more GW (wire geometry) cards
    113:   // one GE (geometry end) card
    114:   // one or more FR (design frequency) cards
    115:   // one or more EX (excitation point) cards
    116:   // zero or one GN (ground condition) card
    117:   // zero or more LD (loading) cards
    118:   // one EN (end of file) card
    119:   //
    120:   // There are a number of issues with this list:
    121:   //
    122:   // 1) some decks lack any comments, although we consider that fatal
    123:   // 2) you don't need a GW card specifically, any geometry will do
    124:   // 3) the EN is not really required, many decks lack it
    125:   // 4) the FR and EX are not really required, there are other cards that can
    126:   //    trigger the output, and decks producing GF don't need them at all
    127:   //
    128:   // For now, this code demands a minimum deck of five cards, at least one
    129:   // comment, two geometry cards, an FX, and an EX. This will be better tuned
    130:   // as we see more decks in the wild, but it should be enough to catch most
    131:   // of the really broken ones that would cause problems in the processing.
    132:   //
    133:   // TODO: do you need an EX? what about transmission?
    134:   //
    135:   // There are also a number of additional tests performed below for other
    136:   // issues like duplicates of cards that should only exist once, cards in the
    137:   // wrong section of the deck, and similar issues.
    138: 
    139:   // although these look like they should be bools, we use int
    140:   // so we can report the card number where the duplicate was seen
    141:   int sawCE = 0, sawGx = 0, sawGE = 0, sawEN = 0, sawGF = 0;
    142:   int sawFR = 0, sawSC = 0, sawSP = 0, sawGN = 0, sawGD = 0;
    143:   int sawRP = 0;
    144:   double freq_mhz = 0.0; // first FR base frequency for wavelength-based checks
    145:   int ek_enabled = 0;
    146:   int sawGS = 0, sawLD = 0, sawEX = 0, sawSY = 0;
    147:   int pendingSM = 0; // track SM that must be immediately followed by SC
    148:   int GEType = 0;
    149:   // and some temps
    150:   char *code, *last_code;
    151:   char msg[MAX_ERROR_LEN];
    152:   // track geometry tags seen so far (simple fixed-size set)
    153:   int geom_tags[512];
    154:   int geom_tag_count = 0;
    155:   // capture wire geometries for endpoint connectivity checks
    156:   wire_info_t wires[512];
    157:   int wire_count = 0;
    158:   // capture EX/LD/TL references to validate after geometry collection
    159:   ref_info_t ex_refs[512];
    160:   int ex_ref_count = 0;
    161:   ref_info_t ld_refs[512];
    162:   int ld_ref_count = 0;
    163:   tl_ref_t tl_refs[512];
    164:   int tl_ref_count = 0;
    165: 
    166:   // start with some obvious ones
    167:   if (deck->num_cards == 0)
    168:   {
    169:     snprintf(msg, sizeof(msg), "The deck has no cards.");
>   170:     add_error(ctx, errors, msg, 2); // this is a critical error, this deck will not process
    171:     return;
    172:   }
    173:   if (deck->num_cards < 5)
    174:   {
    175:     snprintf(msg, sizeof(msg), "A deck has to have at least five cards; one or more comments, one or more Gx cards, a GE, an FX, and an EX.");

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L169:     snprintf(msg, sizeof(msg), "The deck has no cards.");

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 176
CALL: add_error(ctx, errors, msg, 2); // same here, there is no way this will calculate property
--- Context (lines 116-181) ---
    116:   // zero or one GN (ground condition) card
    117:   // zero or more LD (loading) cards
    118:   // one EN (end of file) card
    119:   //
    120:   // There are a number of issues with this list:
    121:   //
    122:   // 1) some decks lack any comments, although we consider that fatal
    123:   // 2) you don't need a GW card specifically, any geometry will do
    124:   // 3) the EN is not really required, many decks lack it
    125:   // 4) the FR and EX are not really required, there are other cards that can
    126:   //    trigger the output, and decks producing GF don't need them at all
    127:   //
    128:   // For now, this code demands a minimum deck of five cards, at least one
    129:   // comment, two geometry cards, an FX, and an EX. This will be better tuned
    130:   // as we see more decks in the wild, but it should be enough to catch most
    131:   // of the really broken ones that would cause problems in the processing.
    132:   //
    133:   // TODO: do you need an EX? what about transmission?
    134:   //
    135:   // There are also a number of additional tests performed below for other
    136:   // issues like duplicates of cards that should only exist once, cards in the
    137:   // wrong section of the deck, and similar issues.
    138: 
    139:   // although these look like they should be bools, we use int
    140:   // so we can report the card number where the duplicate was seen
    141:   int sawCE = 0, sawGx = 0, sawGE = 0, sawEN = 0, sawGF = 0;
    142:   int sawFR = 0, sawSC = 0, sawSP = 0, sawGN = 0, sawGD = 0;
    143:   int sawRP = 0;
    144:   double freq_mhz = 0.0; // first FR base frequency for wavelength-based checks
    145:   int ek_enabled = 0;
    146:   int sawGS = 0, sawLD = 0, sawEX = 0, sawSY = 0;
    147:   int pendingSM = 0; // track SM that must be immediately followed by SC
    148:   int GEType = 0;
    149:   // and some temps
    150:   char *code, *last_code;
    151:   char msg[MAX_ERROR_LEN];
    152:   // track geometry tags seen so far (simple fixed-size set)
    153:   int geom_tags[512];
    154:   int geom_tag_count = 0;
    155:   // capture wire geometries for endpoint connectivity checks
    156:   wire_info_t wires[512];
    157:   int wire_count = 0;
    158:   // capture EX/LD/TL references to validate after geometry collection
    159:   ref_info_t ex_refs[512];
    160:   int ex_ref_count = 0;
    161:   ref_info_t ld_refs[512];
    162:   int ld_ref_count = 0;
    163:   tl_ref_t tl_refs[512];
    164:   int tl_ref_count = 0;
    165: 
    166:   // start with some obvious ones
    167:   if (deck->num_cards == 0)
    168:   {
    169:     snprintf(msg, sizeof(msg), "The deck has no cards.");
    170:     add_error(ctx, errors, msg, 2); // this is a critical error, this deck will not process
    171:     return;
    172:   }
    173:   if (deck->num_cards < 5)
    174:   {
    175:     snprintf(msg, sizeof(msg), "A deck has to have at least five cards; one or more comments, one or more Gx cards, a GE, an FX, and an EX.");
>   176:     add_error(ctx, errors, msg, 2); // same here, there is no way this will calculate property
    177:     return;
    178:   }
    179: 
    180:   // make sure we can find all the required cards
    181:   last_code = "";

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L169:     snprintf(msg, sizeof(msg), "The deck has no cards.");
L175:     snprintf(msg, sizeof(msg), "A deck has to have at least five cards; one or more comments, one or more Gx cards, a GE, an FX, and an EX.");

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 199
CALL: add_error(ctx, errors, msg, 0); // this will calculate fine, so this is merely a warning
--- Context (lines 139-204) ---
    139:   // although these look like they should be bools, we use int
    140:   // so we can report the card number where the duplicate was seen
    141:   int sawCE = 0, sawGx = 0, sawGE = 0, sawEN = 0, sawGF = 0;
    142:   int sawFR = 0, sawSC = 0, sawSP = 0, sawGN = 0, sawGD = 0;
    143:   int sawRP = 0;
    144:   double freq_mhz = 0.0; // first FR base frequency for wavelength-based checks
    145:   int ek_enabled = 0;
    146:   int sawGS = 0, sawLD = 0, sawEX = 0, sawSY = 0;
    147:   int pendingSM = 0; // track SM that must be immediately followed by SC
    148:   int GEType = 0;
    149:   // and some temps
    150:   char *code, *last_code;
    151:   char msg[MAX_ERROR_LEN];
    152:   // track geometry tags seen so far (simple fixed-size set)
    153:   int geom_tags[512];
    154:   int geom_tag_count = 0;
    155:   // capture wire geometries for endpoint connectivity checks
    156:   wire_info_t wires[512];
    157:   int wire_count = 0;
    158:   // capture EX/LD/TL references to validate after geometry collection
    159:   ref_info_t ex_refs[512];
    160:   int ex_ref_count = 0;
    161:   ref_info_t ld_refs[512];
    162:   int ld_ref_count = 0;
    163:   tl_ref_t tl_refs[512];
    164:   int tl_ref_count = 0;
    165: 
    166:   // start with some obvious ones
    167:   if (deck->num_cards == 0)
    168:   {
    169:     snprintf(msg, sizeof(msg), "The deck has no cards.");
    170:     add_error(ctx, errors, msg, 2); // this is a critical error, this deck will not process
    171:     return;
    172:   }
    173:   if (deck->num_cards < 5)
    174:   {
    175:     snprintf(msg, sizeof(msg), "A deck has to have at least five cards; one or more comments, one or more Gx cards, a GE, an FX, and an EX.");
    176:     add_error(ctx, errors, msg, 2); // same here, there is no way this will calculate property
    177:     return;
    178:   }
    179: 
    180:   // make sure we can find all the required cards
    181:   last_code = "";
    182:   for (int i = 0; i < deck->num_cards; i++)
    183:   {
    184:     // cache this
    185:     code = deck->cards[i].card_code;
    186: 
    187:     // start with the checks for the cards we *have* to have, while also looking for duplicates
    188: 
    189:     // it's legal to have multiple GS cards, but that might be confusing
    190:     if (strcmp(code, "GS") == 0)
    191:     {
    192:       if (sawGS == false)
    193:       {
    194:         sawGS = i;
    195:       }
    196:       else
    197:       {
    198:         snprintf(msg, sizeof(msg), "The card on line %d is a GS, but we already saw one on card %d. No single measurement type can be defined.", i, sawGS + 1);
>   199:         add_error(ctx, errors, msg, 0); // this will calculate fine, so this is merely a warning
    200:       }
    201:     }
    202:     // NOTE: nec4 does not require a CE or CM, but we'll demand them here for compatibility
    203:     if (strcmp(code, "CE") == 0)
    204:     {

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L169:     snprintf(msg, sizeof(msg), "The deck has no cards.");
L175:     snprintf(msg, sizeof(msg), "A deck has to have at least five cards; one or more comments, one or more Gx cards, a GE, an FX, and an EX.");
L198:         snprintf(msg, sizeof(msg), "The card on line %d is a GS, but we already saw one on card %d. No single measurement type can be defined.", i, sawGS + 1);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 212
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 152-217) ---
    152:   // track geometry tags seen so far (simple fixed-size set)
    153:   int geom_tags[512];
    154:   int geom_tag_count = 0;
    155:   // capture wire geometries for endpoint connectivity checks
    156:   wire_info_t wires[512];
    157:   int wire_count = 0;
    158:   // capture EX/LD/TL references to validate after geometry collection
    159:   ref_info_t ex_refs[512];
    160:   int ex_ref_count = 0;
    161:   ref_info_t ld_refs[512];
    162:   int ld_ref_count = 0;
    163:   tl_ref_t tl_refs[512];
    164:   int tl_ref_count = 0;
    165: 
    166:   // start with some obvious ones
    167:   if (deck->num_cards == 0)
    168:   {
    169:     snprintf(msg, sizeof(msg), "The deck has no cards.");
    170:     add_error(ctx, errors, msg, 2); // this is a critical error, this deck will not process
    171:     return;
    172:   }
    173:   if (deck->num_cards < 5)
    174:   {
    175:     snprintf(msg, sizeof(msg), "A deck has to have at least five cards; one or more comments, one or more Gx cards, a GE, an FX, and an EX.");
    176:     add_error(ctx, errors, msg, 2); // same here, there is no way this will calculate property
    177:     return;
    178:   }
    179: 
    180:   // make sure we can find all the required cards
    181:   last_code = "";
    182:   for (int i = 0; i < deck->num_cards; i++)
    183:   {
    184:     // cache this
    185:     code = deck->cards[i].card_code;
    186: 
    187:     // start with the checks for the cards we *have* to have, while also looking for duplicates
    188: 
    189:     // it's legal to have multiple GS cards, but that might be confusing
    190:     if (strcmp(code, "GS") == 0)
    191:     {
    192:       if (sawGS == false)
    193:       {
    194:         sawGS = i;
    195:       }
    196:       else
    197:       {
    198:         snprintf(msg, sizeof(msg), "The card on line %d is a GS, but we already saw one on card %d. No single measurement type can be defined.", i, sawGS + 1);
    199:         add_error(ctx, errors, msg, 0); // this will calculate fine, so this is merely a warning
    200:       }
    201:     }
    202:     // NOTE: nec4 does not require a CE or CM, but we'll demand them here for compatibility
    203:     if (strcmp(code, "CE") == 0)
    204:     {
    205:       if (sawCE == false)
    206:       {
    207:         sawCE = i;
    208:       }
    209:       else
    210:       {
    211:         snprintf(msg, sizeof(msg), "The card on line %d is a CE, but we already saw one on card %d.", i, sawCE + 1);
>   212:         add_error(ctx, errors, msg, 0);
    213:       }
    214:     }
    215:     if (strcmp(code, "GE") == 0)
    216:     {
    217:       if (sawGE == false)

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L169:     snprintf(msg, sizeof(msg), "The deck has no cards.");
L175:     snprintf(msg, sizeof(msg), "A deck has to have at least five cards; one or more comments, one or more Gx cards, a GE, an FX, and an EX.");
L198:         snprintf(msg, sizeof(msg), "The card on line %d is a GS, but we already saw one on card %d. No single measurement type can be defined.", i, sawGS + 1);
L211:         snprintf(msg, sizeof(msg), "The card on line %d is a CE, but we already saw one on card %d.", i, sawCE + 1);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 225
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 165-230) ---
    165: 
    166:   // start with some obvious ones
    167:   if (deck->num_cards == 0)
    168:   {
    169:     snprintf(msg, sizeof(msg), "The deck has no cards.");
    170:     add_error(ctx, errors, msg, 2); // this is a critical error, this deck will not process
    171:     return;
    172:   }
    173:   if (deck->num_cards < 5)
    174:   {
    175:     snprintf(msg, sizeof(msg), "A deck has to have at least five cards; one or more comments, one or more Gx cards, a GE, an FX, and an EX.");
    176:     add_error(ctx, errors, msg, 2); // same here, there is no way this will calculate property
    177:     return;
    178:   }
    179: 
    180:   // make sure we can find all the required cards
    181:   last_code = "";
    182:   for (int i = 0; i < deck->num_cards; i++)
    183:   {
    184:     // cache this
    185:     code = deck->cards[i].card_code;
    186: 
    187:     // start with the checks for the cards we *have* to have, while also looking for duplicates
    188: 
    189:     // it's legal to have multiple GS cards, but that might be confusing
    190:     if (strcmp(code, "GS") == 0)
    191:     {
    192:       if (sawGS == false)
    193:       {
    194:         sawGS = i;
    195:       }
    196:       else
    197:       {
    198:         snprintf(msg, sizeof(msg), "The card on line %d is a GS, but we already saw one on card %d. No single measurement type can be defined.", i, sawGS + 1);
    199:         add_error(ctx, errors, msg, 0); // this will calculate fine, so this is merely a warning
    200:       }
    201:     }
    202:     // NOTE: nec4 does not require a CE or CM, but we'll demand them here for compatibility
    203:     if (strcmp(code, "CE") == 0)
    204:     {
    205:       if (sawCE == false)
    206:       {
    207:         sawCE = i;
    208:       }
    209:       else
    210:       {
    211:         snprintf(msg, sizeof(msg), "The card on line %d is a CE, but we already saw one on card %d.", i, sawCE + 1);
    212:         add_error(ctx, errors, msg, 0);
    213:       }
    214:     }
    215:     if (strcmp(code, "GE") == 0)
    216:     {
    217:       if (sawGE == false)
    218:       {
    219:         sawGE = i;
    220:         GEType = deck->cards[i].i[1];
    221:         // GE should typically follow at least one geometry card
    222:         if (sawGx == false)
    223:         {
    224:           snprintf(msg, sizeof(msg), "The card on line %d is a GE, but no geometry cards were seen before it.", i + 1);
>   225:           add_error(ctx, errors, msg, 0);
    226:         }
    227:       }
    228:       else
    229:       {
    230:         snprintf(msg, sizeof(msg), "The card on line %d is a GE, but we already saw one on card %d.", i, sawGE + 1);

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L169:     snprintf(msg, sizeof(msg), "The deck has no cards.");
L175:     snprintf(msg, sizeof(msg), "A deck has to have at least five cards; one or more comments, one or more Gx cards, a GE, an FX, and an EX.");
L198:         snprintf(msg, sizeof(msg), "The card on line %d is a GS, but we already saw one on card %d. No single measurement type can be defined.", i, sawGS + 1);
L211:         snprintf(msg, sizeof(msg), "The card on line %d is a CE, but we already saw one on card %d.", i, sawCE + 1);
L224:           snprintf(msg, sizeof(msg), "The card on line %d is a GE, but no geometry cards were seen before it.", i + 1);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 231
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 171-236) ---
    171:     return;
    172:   }
    173:   if (deck->num_cards < 5)
    174:   {
    175:     snprintf(msg, sizeof(msg), "A deck has to have at least five cards; one or more comments, one or more Gx cards, a GE, an FX, and an EX.");
    176:     add_error(ctx, errors, msg, 2); // same here, there is no way this will calculate property
    177:     return;
    178:   }
    179: 
    180:   // make sure we can find all the required cards
    181:   last_code = "";
    182:   for (int i = 0; i < deck->num_cards; i++)
    183:   {
    184:     // cache this
    185:     code = deck->cards[i].card_code;
    186: 
    187:     // start with the checks for the cards we *have* to have, while also looking for duplicates
    188: 
    189:     // it's legal to have multiple GS cards, but that might be confusing
    190:     if (strcmp(code, "GS") == 0)
    191:     {
    192:       if (sawGS == false)
    193:       {
    194:         sawGS = i;
    195:       }
    196:       else
    197:       {
    198:         snprintf(msg, sizeof(msg), "The card on line %d is a GS, but we already saw one on card %d. No single measurement type can be defined.", i, sawGS + 1);
    199:         add_error(ctx, errors, msg, 0); // this will calculate fine, so this is merely a warning
    200:       }
    201:     }
    202:     // NOTE: nec4 does not require a CE or CM, but we'll demand them here for compatibility
    203:     if (strcmp(code, "CE") == 0)
    204:     {
    205:       if (sawCE == false)
    206:       {
    207:         sawCE = i;
    208:       }
    209:       else
    210:       {
    211:         snprintf(msg, sizeof(msg), "The card on line %d is a CE, but we already saw one on card %d.", i, sawCE + 1);
    212:         add_error(ctx, errors, msg, 0);
    213:       }
    214:     }
    215:     if (strcmp(code, "GE") == 0)
    216:     {
    217:       if (sawGE == false)
    218:       {
    219:         sawGE = i;
    220:         GEType = deck->cards[i].i[1];
    221:         // GE should typically follow at least one geometry card
    222:         if (sawGx == false)
    223:         {
    224:           snprintf(msg, sizeof(msg), "The card on line %d is a GE, but no geometry cards were seen before it.", i + 1);
    225:           add_error(ctx, errors, msg, 0);
    226:         }
    227:       }
    228:       else
    229:       {
    230:         snprintf(msg, sizeof(msg), "The card on line %d is a GE, but we already saw one on card %d.", i, sawGE + 1);
>   231:         add_error(ctx, errors, msg, 0);
    232:       }
    233:     }
    234:     if (strcmp(code, "EN") == 0)
    235:     {
    236:       if (sawEN == false)

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L169:     snprintf(msg, sizeof(msg), "The deck has no cards.");
L175:     snprintf(msg, sizeof(msg), "A deck has to have at least five cards; one or more comments, one or more Gx cards, a GE, an FX, and an EX.");
L198:         snprintf(msg, sizeof(msg), "The card on line %d is a GS, but we already saw one on card %d. No single measurement type can be defined.", i, sawGS + 1);
L211:         snprintf(msg, sizeof(msg), "The card on line %d is a CE, but we already saw one on card %d.", i, sawCE + 1);
L224:           snprintf(msg, sizeof(msg), "The card on line %d is a GE, but no geometry cards were seen before it.", i + 1);
L230:         snprintf(msg, sizeof(msg), "The card on line %d is a GE, but we already saw one on card %d.", i, sawGE + 1);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 243
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 183-248) ---
    183:   {
    184:     // cache this
    185:     code = deck->cards[i].card_code;
    186: 
    187:     // start with the checks for the cards we *have* to have, while also looking for duplicates
    188: 
    189:     // it's legal to have multiple GS cards, but that might be confusing
    190:     if (strcmp(code, "GS") == 0)
    191:     {
    192:       if (sawGS == false)
    193:       {
    194:         sawGS = i;
    195:       }
    196:       else
    197:       {
    198:         snprintf(msg, sizeof(msg), "The card on line %d is a GS, but we already saw one on card %d. No single measurement type can be defined.", i, sawGS + 1);
    199:         add_error(ctx, errors, msg, 0); // this will calculate fine, so this is merely a warning
    200:       }
    201:     }
    202:     // NOTE: nec4 does not require a CE or CM, but we'll demand them here for compatibility
    203:     if (strcmp(code, "CE") == 0)
    204:     {
    205:       if (sawCE == false)
    206:       {
    207:         sawCE = i;
    208:       }
    209:       else
    210:       {
    211:         snprintf(msg, sizeof(msg), "The card on line %d is a CE, but we already saw one on card %d.", i, sawCE + 1);
    212:         add_error(ctx, errors, msg, 0);
    213:       }
    214:     }
    215:     if (strcmp(code, "GE") == 0)
    216:     {
    217:       if (sawGE == false)
    218:       {
    219:         sawGE = i;
    220:         GEType = deck->cards[i].i[1];
    221:         // GE should typically follow at least one geometry card
    222:         if (sawGx == false)
    223:         {
    224:           snprintf(msg, sizeof(msg), "The card on line %d is a GE, but no geometry cards were seen before it.", i + 1);
    225:           add_error(ctx, errors, msg, 0);
    226:         }
    227:       }
    228:       else
    229:       {
    230:         snprintf(msg, sizeof(msg), "The card on line %d is a GE, but we already saw one on card %d.", i, sawGE + 1);
    231:         add_error(ctx, errors, msg, 0);
    232:       }
    233:     }
    234:     if (strcmp(code, "EN") == 0)
    235:     {
    236:       if (sawEN == false)
    237:       {
    238:         sawEN = i;
    239:       }
    240:       else
    241:       {
    242:         snprintf(msg, sizeof(msg), "The card on line %d is an EN, but we already saw one on card %d.", i, sawEN + 1);
>   243:         add_error(ctx, errors, msg, 0);
    244:       }
    245:     }
    246: 
    247:     if (strcmp(code, "NX") == 0)
    248:     {

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L169:     snprintf(msg, sizeof(msg), "The deck has no cards.");
L175:     snprintf(msg, sizeof(msg), "A deck has to have at least five cards; one or more comments, one or more Gx cards, a GE, an FX, and an EX.");
L198:         snprintf(msg, sizeof(msg), "The card on line %d is a GS, but we already saw one on card %d. No single measurement type can be defined.", i, sawGS + 1);
L211:         snprintf(msg, sizeof(msg), "The card on line %d is a CE, but we already saw one on card %d.", i, sawCE + 1);
L224:           snprintf(msg, sizeof(msg), "The card on line %d is a GE, but no geometry cards were seen before it.", i + 1);
L230:         snprintf(msg, sizeof(msg), "The card on line %d is a GE, but we already saw one on card %d.", i, sawGE + 1);
L242:         snprintf(msg, sizeof(msg), "The card on line %d is an EN, but we already saw one on card %d.", i, sawEN + 1);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 268
CALL: add_error(ctx, errors, msg, WARNING);
--- Context (lines 208-273) ---
    208:       }
    209:       else
    210:       {
    211:         snprintf(msg, sizeof(msg), "The card on line %d is a CE, but we already saw one on card %d.", i, sawCE + 1);
    212:         add_error(ctx, errors, msg, 0);
    213:       }
    214:     }
    215:     if (strcmp(code, "GE") == 0)
    216:     {
    217:       if (sawGE == false)
    218:       {
    219:         sawGE = i;
    220:         GEType = deck->cards[i].i[1];
    221:         // GE should typically follow at least one geometry card
    222:         if (sawGx == false)
    223:         {
    224:           snprintf(msg, sizeof(msg), "The card on line %d is a GE, but no geometry cards were seen before it.", i + 1);
    225:           add_error(ctx, errors, msg, 0);
    226:         }
    227:       }
    228:       else
    229:       {
    230:         snprintf(msg, sizeof(msg), "The card on line %d is a GE, but we already saw one on card %d.", i, sawGE + 1);
    231:         add_error(ctx, errors, msg, 0);
    232:       }
    233:     }
    234:     if (strcmp(code, "EN") == 0)
    235:     {
    236:       if (sawEN == false)
    237:       {
    238:         sawEN = i;
    239:       }
    240:       else
    241:       {
    242:         snprintf(msg, sizeof(msg), "The card on line %d is an EN, but we already saw one on card %d.", i, sawEN + 1);
    243:         add_error(ctx, errors, msg, 0);
    244:       }
    245:     }
    246: 
    247:     if (strcmp(code, "NX") == 0)
    248:     {
    249:       // NX must not carry numeric parameters (ONEC comment is allowed)
    250:       bool nx_has_params = false;
    251:       for (int j = 1; j <= MAX_INT_FIELDS; j++)
    252:         if (deck->cards[i].i[j] != 0)
    253:         {
    254:           nx_has_params = true;
    255:           break;
    256:         }
    257:       if (!nx_has_params)
    258:         for (int j = 1; j <= MAX_FLT_FIELDS; j++)
    259:           if (deck->cards[i].f[j] != 0.0)
    260:           {
    261:             nx_has_params = true;
    262:             break;
    263:           }
    264:       if (nx_has_params)
    265:       {
    266:         snprintf(msg, sizeof(msg),
    267:                  "The NX card on line %d has numeric parameters; NX takes no parameters (they will be ignored).", i + 1);
>   268:         add_error(ctx, errors, msg, WARNING);
    269:       }
    270: 
    271:       // the first non-ignored card after NX must be CM or CE
    272:       bool found_next_cm = false;
    273:       for (int j = i + 1; j < deck->num_cards; j++)

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L169:     snprintf(msg, sizeof(msg), "The deck has no cards.");
L175:     snprintf(msg, sizeof(msg), "A deck has to have at least five cards; one or more comments, one or more Gx cards, a GE, an FX, and an EX.");
L198:         snprintf(msg, sizeof(msg), "The card on line %d is a GS, but we already saw one on card %d. No single measurement type can be defined.", i, sawGS + 1);
L211:         snprintf(msg, sizeof(msg), "The card on line %d is a CE, but we already saw one on card %d.", i, sawCE + 1);
L224:           snprintf(msg, sizeof(msg), "The card on line %d is a GE, but no geometry cards were seen before it.", i + 1);
L230:         snprintf(msg, sizeof(msg), "The card on line %d is a GE, but we already saw one on card %d.", i, sawGE + 1);
L242:         snprintf(msg, sizeof(msg), "The card on line %d is an EN, but we already saw one on card %d.", i, sawEN + 1);
L266:         snprintf(msg, sizeof(msg),

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 287
CALL: add_error(ctx, errors, msg, FATAL);
--- Context (lines 227-292) ---
    227:       }
    228:       else
    229:       {
    230:         snprintf(msg, sizeof(msg), "The card on line %d is a GE, but we already saw one on card %d.", i, sawGE + 1);
    231:         add_error(ctx, errors, msg, 0);
    232:       }
    233:     }
    234:     if (strcmp(code, "EN") == 0)
    235:     {
    236:       if (sawEN == false)
    237:       {
    238:         sawEN = i;
    239:       }
    240:       else
    241:       {
    242:         snprintf(msg, sizeof(msg), "The card on line %d is an EN, but we already saw one on card %d.", i, sawEN + 1);
    243:         add_error(ctx, errors, msg, 0);
    244:       }
    245:     }
    246: 
    247:     if (strcmp(code, "NX") == 0)
    248:     {
    249:       // NX must not carry numeric parameters (ONEC comment is allowed)
    250:       bool nx_has_params = false;
    251:       for (int j = 1; j <= MAX_INT_FIELDS; j++)
    252:         if (deck->cards[i].i[j] != 0)
    253:         {
    254:           nx_has_params = true;
    255:           break;
    256:         }
    257:       if (!nx_has_params)
    258:         for (int j = 1; j <= MAX_FLT_FIELDS; j++)
    259:           if (deck->cards[i].f[j] != 0.0)
    260:           {
    261:             nx_has_params = true;
    262:             break;
    263:           }
    264:       if (nx_has_params)
    265:       {
    266:         snprintf(msg, sizeof(msg),
    267:                  "The NX card on line %d has numeric parameters; NX takes no parameters (they will be ignored).", i + 1);
    268:         add_error(ctx, errors, msg, WARNING);
    269:       }
    270: 
    271:       // the first non-ignored card after NX must be CM or CE
    272:       bool found_next_cm = false;
    273:       for (int j = i + 1; j < deck->num_cards; j++)
    274:       {
    275:         if (deck->cards[j].ignore)
    276:           continue;
    277:         if (is_comment(&deck->cards[j]))
    278:         {
    279:           found_next_cm = true;
    280:         }
    281:         break; /* first non-ignored card, comment or not */
    282:       }
    283:       if (!found_next_cm)
    284:       {
    285:         snprintf(msg, sizeof(msg),
    286:                  "The NX card on line %d must be immediately followed by a CM card to start the next section.", i + 1);
>   287:         add_error(ctx, errors, msg, FATAL);
    288:       }
    289: 
    290:       /* reset per-section tracking so the next section is validated independently. */
    291:       sawCE = 0;
    292:       sawGx = 0;

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L169:     snprintf(msg, sizeof(msg), "The deck has no cards.");
L175:     snprintf(msg, sizeof(msg), "A deck has to have at least five cards; one or more comments, one or more Gx cards, a GE, an FX, and an EX.");
L198:         snprintf(msg, sizeof(msg), "The card on line %d is a GS, but we already saw one on card %d. No single measurement type can be defined.", i, sawGS + 1);
L211:         snprintf(msg, sizeof(msg), "The card on line %d is a CE, but we already saw one on card %d.", i, sawCE + 1);
L224:           snprintf(msg, sizeof(msg), "The card on line %d is a GE, but no geometry cards were seen before it.", i + 1);
L230:         snprintf(msg, sizeof(msg), "The card on line %d is a GE, but we already saw one on card %d.", i, sawGE + 1);
L242:         snprintf(msg, sizeof(msg), "The card on line %d is an EN, but we already saw one on card %d.", i, sawEN + 1);
L266:         snprintf(msg, sizeof(msg),
L285:         snprintf(msg, sizeof(msg),

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 320
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 260-325) ---
    260:           {
    261:             nx_has_params = true;
    262:             break;
    263:           }
    264:       if (nx_has_params)
    265:       {
    266:         snprintf(msg, sizeof(msg),
    267:                  "The NX card on line %d has numeric parameters; NX takes no parameters (they will be ignored).", i + 1);
    268:         add_error(ctx, errors, msg, WARNING);
    269:       }
    270: 
    271:       // the first non-ignored card after NX must be CM or CE
    272:       bool found_next_cm = false;
    273:       for (int j = i + 1; j < deck->num_cards; j++)
    274:       {
    275:         if (deck->cards[j].ignore)
    276:           continue;
    277:         if (is_comment(&deck->cards[j]))
    278:         {
    279:           found_next_cm = true;
    280:         }
    281:         break; /* first non-ignored card, comment or not */
    282:       }
    283:       if (!found_next_cm)
    284:       {
    285:         snprintf(msg, sizeof(msg),
    286:                  "The NX card on line %d must be immediately followed by a CM card to start the next section.", i + 1);
    287:         add_error(ctx, errors, msg, FATAL);
    288:       }
    289: 
    290:       /* reset per-section tracking so the next section is validated independently. */
    291:       sawCE = 0;
    292:       sawGx = 0;
    293:       sawGE = 0;
    294:       sawEN = 0;
    295:       sawGF = 0;
    296:       sawFR = 0;
    297:       sawSC = 0;
    298:       sawSP = 0;
    299:       sawGN = 0;
    300:       sawGD = 0;
    301:       sawRP = 0;
    302:       sawGS = 0;
    303:       sawLD = 0;
    304:       sawEX = 0;
    305:       sawSY = 0;
    306:     }
    307: 
    308:     // NOTE: does a deck really need a EX?
    309: 
    310:     // and also look for other cards where there can only be one
    311:     if (strcmp(code, "GF") == 0)
    312:     {
    313:       if (sawGF == false)
    314:       {
    315:         sawGF = i;
    316:       }
    317:       else
    318:       {
    319:         snprintf(msg, sizeof(msg), "The card on line %d is a GF, but we already saw one on card %d.", i, sawGF + 1);
>   320:         add_error(ctx, errors, msg, 0);
    321:       }
    322:     }
    323:     if (strcmp(code, "FR") == 0)
    324:     {
    325:       if (sawFR == false)

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L169:     snprintf(msg, sizeof(msg), "The deck has no cards.");
L175:     snprintf(msg, sizeof(msg), "A deck has to have at least five cards; one or more comments, one or more Gx cards, a GE, an FX, and an EX.");
L198:         snprintf(msg, sizeof(msg), "The card on line %d is a GS, but we already saw one on card %d. No single measurement type can be defined.", i, sawGS + 1);
L211:         snprintf(msg, sizeof(msg), "The card on line %d is a CE, but we already saw one on card %d.", i, sawCE + 1);
L224:           snprintf(msg, sizeof(msg), "The card on line %d is a GE, but no geometry cards were seen before it.", i + 1);
L230:         snprintf(msg, sizeof(msg), "The card on line %d is a GE, but we already saw one on card %d.", i, sawGE + 1);
L242:         snprintf(msg, sizeof(msg), "The card on line %d is an EN, but we already saw one on card %d.", i, sawEN + 1);
L266:         snprintf(msg, sizeof(msg),
L285:         snprintf(msg, sizeof(msg),
L319:         snprintf(msg, sizeof(msg), "The card on line %d is a GF, but we already saw one on card %d.", i, sawGF + 1);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 336
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 276-341) ---
    276:           continue;
    277:         if (is_comment(&deck->cards[j]))
    278:         {
    279:           found_next_cm = true;
    280:         }
    281:         break; /* first non-ignored card, comment or not */
    282:       }
    283:       if (!found_next_cm)
    284:       {
    285:         snprintf(msg, sizeof(msg),
    286:                  "The NX card on line %d must be immediately followed by a CM card to start the next section.", i + 1);
    287:         add_error(ctx, errors, msg, FATAL);
    288:       }
    289: 
    290:       /* reset per-section tracking so the next section is validated independently. */
    291:       sawCE = 0;
    292:       sawGx = 0;
    293:       sawGE = 0;
    294:       sawEN = 0;
    295:       sawGF = 0;
    296:       sawFR = 0;
    297:       sawSC = 0;
    298:       sawSP = 0;
    299:       sawGN = 0;
    300:       sawGD = 0;
    301:       sawRP = 0;
    302:       sawGS = 0;
    303:       sawLD = 0;
    304:       sawEX = 0;
    305:       sawSY = 0;
    306:     }
    307: 
    308:     // NOTE: does a deck really need a EX?
    309: 
    310:     // and also look for other cards where there can only be one
    311:     if (strcmp(code, "GF") == 0)
    312:     {
    313:       if (sawGF == false)
    314:       {
    315:         sawGF = i;
    316:       }
    317:       else
    318:       {
    319:         snprintf(msg, sizeof(msg), "The card on line %d is a GF, but we already saw one on card %d.", i, sawGF + 1);
    320:         add_error(ctx, errors, msg, 0);
    321:       }
    322:     }
    323:     if (strcmp(code, "FR") == 0)
    324:     {
    325:       if (sawFR == false)
    326:       {
    327:         sawFR = i;
    328:         if (freq_mhz == 0.0)
    329:         {
    330:           freq_mhz = deck->cards[i].f[1];
    331:         }
    332:       }
    333:       else
    334:       {
    335:         snprintf(msg, sizeof(msg), "The card on line %d is an FR, but we already saw one on card %d.", i, sawFR + 1);
>   336:         add_error(ctx, errors, msg, 0);
    337:       }
    338:     }
    339:     if (strcmp(code, "EK") == 0)
    340:     {
    341:       // any non-zero I1 enables extended thin-wire kernel

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L169:     snprintf(msg, sizeof(msg), "The deck has no cards.");
L175:     snprintf(msg, sizeof(msg), "A deck has to have at least five cards; one or more comments, one or more Gx cards, a GE, an FX, and an EX.");
L198:         snprintf(msg, sizeof(msg), "The card on line %d is a GS, but we already saw one on card %d. No single measurement type can be defined.", i, sawGS + 1);
L211:         snprintf(msg, sizeof(msg), "The card on line %d is a CE, but we already saw one on card %d.", i, sawCE + 1);
L224:           snprintf(msg, sizeof(msg), "The card on line %d is a GE, but no geometry cards were seen before it.", i + 1);
L230:         snprintf(msg, sizeof(msg), "The card on line %d is a GE, but we already saw one on card %d.", i, sawGE + 1);
L242:         snprintf(msg, sizeof(msg), "The card on line %d is an EN, but we already saw one on card %d.", i, sawEN + 1);
L266:         snprintf(msg, sizeof(msg),
L285:         snprintf(msg, sizeof(msg),
L319:         snprintf(msg, sizeof(msg), "The card on line %d is a GF, but we already saw one on card %d.", i, sawGF + 1);
L335:         snprintf(msg, sizeof(msg), "The card on line %d is an FR, but we already saw one on card %d.", i, sawFR + 1);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 356
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 296-361) ---
    296:       sawFR = 0;
    297:       sawSC = 0;
    298:       sawSP = 0;
    299:       sawGN = 0;
    300:       sawGD = 0;
    301:       sawRP = 0;
    302:       sawGS = 0;
    303:       sawLD = 0;
    304:       sawEX = 0;
    305:       sawSY = 0;
    306:     }
    307: 
    308:     // NOTE: does a deck really need a EX?
    309: 
    310:     // and also look for other cards where there can only be one
    311:     if (strcmp(code, "GF") == 0)
    312:     {
    313:       if (sawGF == false)
    314:       {
    315:         sawGF = i;
    316:       }
    317:       else
    318:       {
    319:         snprintf(msg, sizeof(msg), "The card on line %d is a GF, but we already saw one on card %d.", i, sawGF + 1);
    320:         add_error(ctx, errors, msg, 0);
    321:       }
    322:     }
    323:     if (strcmp(code, "FR") == 0)
    324:     {
    325:       if (sawFR == false)
    326:       {
    327:         sawFR = i;
    328:         if (freq_mhz == 0.0)
    329:         {
    330:           freq_mhz = deck->cards[i].f[1];
    331:         }
    332:       }
    333:       else
    334:       {
    335:         snprintf(msg, sizeof(msg), "The card on line %d is an FR, but we already saw one on card %d.", i, sawFR + 1);
    336:         add_error(ctx, errors, msg, 0);
    337:       }
    338:     }
    339:     if (strcmp(code, "EK") == 0)
    340:     {
    341:       // any non-zero I1 enables extended thin-wire kernel
    342:       if (deck->cards[i].ints_used >= 1 && deck->cards[i].i[1] != 0)
    343:       {
    344:         ek_enabled = 1;
    345:       }
    346:     }
    347: 
    348:     // warn if control cards appear before GE (except CE and cards with specific messages below)
    349:     if (sawGE == false)
    350:     {
    351:       if (is_control(&deck->cards[i]) && strcmp(code, "CE") != 0 &&
    352:           strcmp(code, "EX") != 0 && strcmp(code, "TL") != 0 && strcmp(code, "LD") != 0 &&
    353:           strcmp(code, "FR") != 0 && strcmp(code, "RP") != 0 && strcmp(code, "GN") != 0 && strcmp(code, "GD") != 0 && strcmp(code, "EK") != 0)
    354:       {
    355:         snprintf(msg, sizeof(msg), "The card on line %d is a %s, but it appears before the GE; control cards should follow geometry.", i + 1, code);
>   356:         add_error(ctx, errors, msg, 0);
    357:       }
    358:     }
    359: 
    360:     // specific placement warnings for common control cards
    361:     if (sawGE == false)

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L169:     snprintf(msg, sizeof(msg), "The deck has no cards.");
L175:     snprintf(msg, sizeof(msg), "A deck has to have at least five cards; one or more comments, one or more Gx cards, a GE, an FX, and an EX.");
L198:         snprintf(msg, sizeof(msg), "The card on line %d is a GS, but we already saw one on card %d. No single measurement type can be defined.", i, sawGS + 1);
L211:         snprintf(msg, sizeof(msg), "The card on line %d is a CE, but we already saw one on card %d.", i, sawCE + 1);
L224:           snprintf(msg, sizeof(msg), "The card on line %d is a GE, but no geometry cards were seen before it.", i + 1);
L230:         snprintf(msg, sizeof(msg), "The card on line %d is a GE, but we already saw one on card %d.", i, sawGE + 1);
L242:         snprintf(msg, sizeof(msg), "The card on line %d is an EN, but we already saw one on card %d.", i, sawEN + 1);
L266:         snprintf(msg, sizeof(msg),
L285:         snprintf(msg, sizeof(msg),
L319:         snprintf(msg, sizeof(msg), "The card on line %d is a GF, but we already saw one on card %d.", i, sawGF + 1);
L335:         snprintf(msg, sizeof(msg), "The card on line %d is an FR, but we already saw one on card %d.", i, sawFR + 1);
L355:         snprintf(msg, sizeof(msg), "The card on line %d is a %s, but it appears before the GE; control cards should follow geometry.", i + 1, code);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 366
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 306-371) ---
    306:     }
    307: 
    308:     // NOTE: does a deck really need a EX?
    309: 
    310:     // and also look for other cards where there can only be one
    311:     if (strcmp(code, "GF") == 0)
    312:     {
    313:       if (sawGF == false)
    314:       {
    315:         sawGF = i;
    316:       }
    317:       else
    318:       {
    319:         snprintf(msg, sizeof(msg), "The card on line %d is a GF, but we already saw one on card %d.", i, sawGF + 1);
    320:         add_error(ctx, errors, msg, 0);
    321:       }
    322:     }
    323:     if (strcmp(code, "FR") == 0)
    324:     {
    325:       if (sawFR == false)
    326:       {
    327:         sawFR = i;
    328:         if (freq_mhz == 0.0)
    329:         {
    330:           freq_mhz = deck->cards[i].f[1];
    331:         }
    332:       }
    333:       else
    334:       {
    335:         snprintf(msg, sizeof(msg), "The card on line %d is an FR, but we already saw one on card %d.", i, sawFR + 1);
    336:         add_error(ctx, errors, msg, 0);
    337:       }
    338:     }
    339:     if (strcmp(code, "EK") == 0)
    340:     {
    341:       // any non-zero I1 enables extended thin-wire kernel
    342:       if (deck->cards[i].ints_used >= 1 && deck->cards[i].i[1] != 0)
    343:       {
    344:         ek_enabled = 1;
    345:       }
    346:     }
    347: 
    348:     // warn if control cards appear before GE (except CE and cards with specific messages below)
    349:     if (sawGE == false)
    350:     {
    351:       if (is_control(&deck->cards[i]) && strcmp(code, "CE") != 0 &&
    352:           strcmp(code, "EX") != 0 && strcmp(code, "TL") != 0 && strcmp(code, "LD") != 0 &&
    353:           strcmp(code, "FR") != 0 && strcmp(code, "RP") != 0 && strcmp(code, "GN") != 0 && strcmp(code, "GD") != 0 && strcmp(code, "EK") != 0)
    354:       {
    355:         snprintf(msg, sizeof(msg), "The card on line %d is a %s, but it appears before the GE; control cards should follow geometry.", i + 1, code);
    356:         add_error(ctx, errors, msg, 0);
    357:       }
    358:     }
    359: 
    360:     // specific placement warnings for common control cards
    361:     if (sawGE == false)
    362:     {
    363:       if (strcmp(code, "EX") == 0)
    364:       {
    365:         snprintf(msg, sizeof(msg), "The card on line %d is an EX, but it appears before the GE; excitations should follow geometry.", i + 1);
>   366:         add_error(ctx, errors, msg, 0);
    367:       }
    368:       if (strcmp(code, "TL") == 0)
    369:       {
    370:         snprintf(msg, sizeof(msg), "The card on line %d is a TL, but it appears before the GE; transmission lines should follow geometry.", i + 1);
    371:         add_error(ctx, errors, msg, 0);

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L169:     snprintf(msg, sizeof(msg), "The deck has no cards.");
L175:     snprintf(msg, sizeof(msg), "A deck has to have at least five cards; one or more comments, one or more Gx cards, a GE, an FX, and an EX.");
L198:         snprintf(msg, sizeof(msg), "The card on line %d is a GS, but we already saw one on card %d. No single measurement type can be defined.", i, sawGS + 1);
L211:         snprintf(msg, sizeof(msg), "The card on line %d is a CE, but we already saw one on card %d.", i, sawCE + 1);
L224:           snprintf(msg, sizeof(msg), "The card on line %d is a GE, but no geometry cards were seen before it.", i + 1);
L230:         snprintf(msg, sizeof(msg), "The card on line %d is a GE, but we already saw one on card %d.", i, sawGE + 1);
L242:         snprintf(msg, sizeof(msg), "The card on line %d is an EN, but we already saw one on card %d.", i, sawEN + 1);
L266:         snprintf(msg, sizeof(msg),
L285:         snprintf(msg, sizeof(msg),
L319:         snprintf(msg, sizeof(msg), "The card on line %d is a GF, but we already saw one on card %d.", i, sawGF + 1);
L335:         snprintf(msg, sizeof(msg), "The card on line %d is an FR, but we already saw one on card %d.", i, sawFR + 1);
L355:         snprintf(msg, sizeof(msg), "The card on line %d is a %s, but it appears before the GE; control cards should follow geometry.", i + 1, code);
L365:         snprintf(msg, sizeof(msg), "The card on line %d is an EX, but it appears before the GE; excitations should follow geometry.", i + 1);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 371
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 311-376) ---
    311:     if (strcmp(code, "GF") == 0)
    312:     {
    313:       if (sawGF == false)
    314:       {
    315:         sawGF = i;
    316:       }
    317:       else
    318:       {
    319:         snprintf(msg, sizeof(msg), "The card on line %d is a GF, but we already saw one on card %d.", i, sawGF + 1);
    320:         add_error(ctx, errors, msg, 0);
    321:       }
    322:     }
    323:     if (strcmp(code, "FR") == 0)
    324:     {
    325:       if (sawFR == false)
    326:       {
    327:         sawFR = i;
    328:         if (freq_mhz == 0.0)
    329:         {
    330:           freq_mhz = deck->cards[i].f[1];
    331:         }
    332:       }
    333:       else
    334:       {
    335:         snprintf(msg, sizeof(msg), "The card on line %d is an FR, but we already saw one on card %d.", i, sawFR + 1);
    336:         add_error(ctx, errors, msg, 0);
    337:       }
    338:     }
    339:     if (strcmp(code, "EK") == 0)
    340:     {
    341:       // any non-zero I1 enables extended thin-wire kernel
    342:       if (deck->cards[i].ints_used >= 1 && deck->cards[i].i[1] != 0)
    343:       {
    344:         ek_enabled = 1;
    345:       }
    346:     }
    347: 
    348:     // warn if control cards appear before GE (except CE and cards with specific messages below)
    349:     if (sawGE == false)
    350:     {
    351:       if (is_control(&deck->cards[i]) && strcmp(code, "CE") != 0 &&
    352:           strcmp(code, "EX") != 0 && strcmp(code, "TL") != 0 && strcmp(code, "LD") != 0 &&
    353:           strcmp(code, "FR") != 0 && strcmp(code, "RP") != 0 && strcmp(code, "GN") != 0 && strcmp(code, "GD") != 0 && strcmp(code, "EK") != 0)
    354:       {
    355:         snprintf(msg, sizeof(msg), "The card on line %d is a %s, but it appears before the GE; control cards should follow geometry.", i + 1, code);
    356:         add_error(ctx, errors, msg, 0);
    357:       }
    358:     }
    359: 
    360:     // specific placement warnings for common control cards
    361:     if (sawGE == false)
    362:     {
    363:       if (strcmp(code, "EX") == 0)
    364:       {
    365:         snprintf(msg, sizeof(msg), "The card on line %d is an EX, but it appears before the GE; excitations should follow geometry.", i + 1);
    366:         add_error(ctx, errors, msg, 0);
    367:       }
    368:       if (strcmp(code, "TL") == 0)
    369:       {
    370:         snprintf(msg, sizeof(msg), "The card on line %d is a TL, but it appears before the GE; transmission lines should follow geometry.", i + 1);
>   371:         add_error(ctx, errors, msg, 0);
    372:       }
    373:       if (strcmp(code, "LD") == 0)
    374:       {
    375:         snprintf(msg, sizeof(msg), "The card on line %d is an LD, but it appears before the GE; loading should follow geometry.", i + 1);
    376:         add_error(ctx, errors, msg, 0);

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L175:     snprintf(msg, sizeof(msg), "A deck has to have at least five cards; one or more comments, one or more Gx cards, a GE, an FX, and an EX.");
L198:         snprintf(msg, sizeof(msg), "The card on line %d is a GS, but we already saw one on card %d. No single measurement type can be defined.", i, sawGS + 1);
L211:         snprintf(msg, sizeof(msg), "The card on line %d is a CE, but we already saw one on card %d.", i, sawCE + 1);
L224:           snprintf(msg, sizeof(msg), "The card on line %d is a GE, but no geometry cards were seen before it.", i + 1);
L230:         snprintf(msg, sizeof(msg), "The card on line %d is a GE, but we already saw one on card %d.", i, sawGE + 1);
L242:         snprintf(msg, sizeof(msg), "The card on line %d is an EN, but we already saw one on card %d.", i, sawEN + 1);
L266:         snprintf(msg, sizeof(msg),
L285:         snprintf(msg, sizeof(msg),
L319:         snprintf(msg, sizeof(msg), "The card on line %d is a GF, but we already saw one on card %d.", i, sawGF + 1);
L335:         snprintf(msg, sizeof(msg), "The card on line %d is an FR, but we already saw one on card %d.", i, sawFR + 1);
L355:         snprintf(msg, sizeof(msg), "The card on line %d is a %s, but it appears before the GE; control cards should follow geometry.", i + 1, code);
L365:         snprintf(msg, sizeof(msg), "The card on line %d is an EX, but it appears before the GE; excitations should follow geometry.", i + 1);
L370:         snprintf(msg, sizeof(msg), "The card on line %d is a TL, but it appears before the GE; transmission lines should follow geometry.", i + 1);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 376
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 316-381) ---
    316:       }
    317:       else
    318:       {
    319:         snprintf(msg, sizeof(msg), "The card on line %d is a GF, but we already saw one on card %d.", i, sawGF + 1);
    320:         add_error(ctx, errors, msg, 0);
    321:       }
    322:     }
    323:     if (strcmp(code, "FR") == 0)
    324:     {
    325:       if (sawFR == false)
    326:       {
    327:         sawFR = i;
    328:         if (freq_mhz == 0.0)
    329:         {
    330:           freq_mhz = deck->cards[i].f[1];
    331:         }
    332:       }
    333:       else
    334:       {
    335:         snprintf(msg, sizeof(msg), "The card on line %d is an FR, but we already saw one on card %d.", i, sawFR + 1);
    336:         add_error(ctx, errors, msg, 0);
    337:       }
    338:     }
    339:     if (strcmp(code, "EK") == 0)
    340:     {
    341:       // any non-zero I1 enables extended thin-wire kernel
    342:       if (deck->cards[i].ints_used >= 1 && deck->cards[i].i[1] != 0)
    343:       {
    344:         ek_enabled = 1;
    345:       }
    346:     }
    347: 
    348:     // warn if control cards appear before GE (except CE and cards with specific messages below)
    349:     if (sawGE == false)
    350:     {
    351:       if (is_control(&deck->cards[i]) && strcmp(code, "CE") != 0 &&
    352:           strcmp(code, "EX") != 0 && strcmp(code, "TL") != 0 && strcmp(code, "LD") != 0 &&
    353:           strcmp(code, "FR") != 0 && strcmp(code, "RP") != 0 && strcmp(code, "GN") != 0 && strcmp(code, "GD") != 0 && strcmp(code, "EK") != 0)
    354:       {
    355:         snprintf(msg, sizeof(msg), "The card on line %d is a %s, but it appears before the GE; control cards should follow geometry.", i + 1, code);
    356:         add_error(ctx, errors, msg, 0);
    357:       }
    358:     }
    359: 
    360:     // specific placement warnings for common control cards
    361:     if (sawGE == false)
    362:     {
    363:       if (strcmp(code, "EX") == 0)
    364:       {
    365:         snprintf(msg, sizeof(msg), "The card on line %d is an EX, but it appears before the GE; excitations should follow geometry.", i + 1);
    366:         add_error(ctx, errors, msg, 0);
    367:       }
    368:       if (strcmp(code, "TL") == 0)
    369:       {
    370:         snprintf(msg, sizeof(msg), "The card on line %d is a TL, but it appears before the GE; transmission lines should follow geometry.", i + 1);
    371:         add_error(ctx, errors, msg, 0);
    372:       }
    373:       if (strcmp(code, "LD") == 0)
    374:       {
    375:         snprintf(msg, sizeof(msg), "The card on line %d is an LD, but it appears before the GE; loading should follow geometry.", i + 1);
>   376:         add_error(ctx, errors, msg, 0);
    377:       }
    378:       if (strcmp(code, "FR") == 0)
    379:       {
    380:         snprintf(msg, sizeof(msg), "The card on line %d is an FR, but it appears before the GE; frequency setup should follow geometry.", i + 1);
    381:         add_error(ctx, errors, msg, 0);

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L198:         snprintf(msg, sizeof(msg), "The card on line %d is a GS, but we already saw one on card %d. No single measurement type can be defined.", i, sawGS + 1);
L211:         snprintf(msg, sizeof(msg), "The card on line %d is a CE, but we already saw one on card %d.", i, sawCE + 1);
L224:           snprintf(msg, sizeof(msg), "The card on line %d is a GE, but no geometry cards were seen before it.", i + 1);
L230:         snprintf(msg, sizeof(msg), "The card on line %d is a GE, but we already saw one on card %d.", i, sawGE + 1);
L242:         snprintf(msg, sizeof(msg), "The card on line %d is an EN, but we already saw one on card %d.", i, sawEN + 1);
L266:         snprintf(msg, sizeof(msg),
L285:         snprintf(msg, sizeof(msg),
L319:         snprintf(msg, sizeof(msg), "The card on line %d is a GF, but we already saw one on card %d.", i, sawGF + 1);
L335:         snprintf(msg, sizeof(msg), "The card on line %d is an FR, but we already saw one on card %d.", i, sawFR + 1);
L355:         snprintf(msg, sizeof(msg), "The card on line %d is a %s, but it appears before the GE; control cards should follow geometry.", i + 1, code);
L365:         snprintf(msg, sizeof(msg), "The card on line %d is an EX, but it appears before the GE; excitations should follow geometry.", i + 1);
L370:         snprintf(msg, sizeof(msg), "The card on line %d is a TL, but it appears before the GE; transmission lines should follow geometry.", i + 1);
L375:         snprintf(msg, sizeof(msg), "The card on line %d is an LD, but it appears before the GE; loading should follow geometry.", i + 1);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 381
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 321-386) ---
    321:       }
    322:     }
    323:     if (strcmp(code, "FR") == 0)
    324:     {
    325:       if (sawFR == false)
    326:       {
    327:         sawFR = i;
    328:         if (freq_mhz == 0.0)
    329:         {
    330:           freq_mhz = deck->cards[i].f[1];
    331:         }
    332:       }
    333:       else
    334:       {
    335:         snprintf(msg, sizeof(msg), "The card on line %d is an FR, but we already saw one on card %d.", i, sawFR + 1);
    336:         add_error(ctx, errors, msg, 0);
    337:       }
    338:     }
    339:     if (strcmp(code, "EK") == 0)
    340:     {
    341:       // any non-zero I1 enables extended thin-wire kernel
    342:       if (deck->cards[i].ints_used >= 1 && deck->cards[i].i[1] != 0)
    343:       {
    344:         ek_enabled = 1;
    345:       }
    346:     }
    347: 
    348:     // warn if control cards appear before GE (except CE and cards with specific messages below)
    349:     if (sawGE == false)
    350:     {
    351:       if (is_control(&deck->cards[i]) && strcmp(code, "CE") != 0 &&
    352:           strcmp(code, "EX") != 0 && strcmp(code, "TL") != 0 && strcmp(code, "LD") != 0 &&
    353:           strcmp(code, "FR") != 0 && strcmp(code, "RP") != 0 && strcmp(code, "GN") != 0 && strcmp(code, "GD") != 0 && strcmp(code, "EK") != 0)
    354:       {
    355:         snprintf(msg, sizeof(msg), "The card on line %d is a %s, but it appears before the GE; control cards should follow geometry.", i + 1, code);
    356:         add_error(ctx, errors, msg, 0);
    357:       }
    358:     }
    359: 
    360:     // specific placement warnings for common control cards
    361:     if (sawGE == false)
    362:     {
    363:       if (strcmp(code, "EX") == 0)
    364:       {
    365:         snprintf(msg, sizeof(msg), "The card on line %d is an EX, but it appears before the GE; excitations should follow geometry.", i + 1);
    366:         add_error(ctx, errors, msg, 0);
    367:       }
    368:       if (strcmp(code, "TL") == 0)
    369:       {
    370:         snprintf(msg, sizeof(msg), "The card on line %d is a TL, but it appears before the GE; transmission lines should follow geometry.", i + 1);
    371:         add_error(ctx, errors, msg, 0);
    372:       }
    373:       if (strcmp(code, "LD") == 0)
    374:       {
    375:         snprintf(msg, sizeof(msg), "The card on line %d is an LD, but it appears before the GE; loading should follow geometry.", i + 1);
    376:         add_error(ctx, errors, msg, 0);
    377:       }
    378:       if (strcmp(code, "FR") == 0)
    379:       {
    380:         snprintf(msg, sizeof(msg), "The card on line %d is an FR, but it appears before the GE; frequency setup should follow geometry.", i + 1);
>   381:         add_error(ctx, errors, msg, 0);
    382:       }
    383:       if (strcmp(code, "RP") == 0)
    384:       {
    385:         snprintf(msg, sizeof(msg), "The card on line %d is an RP, but it appears before the GE; pattern requests should follow geometry.", i + 1);
    386:         add_error(ctx, errors, msg, 0);

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L198:         snprintf(msg, sizeof(msg), "The card on line %d is a GS, but we already saw one on card %d. No single measurement type can be defined.", i, sawGS + 1);
L211:         snprintf(msg, sizeof(msg), "The card on line %d is a CE, but we already saw one on card %d.", i, sawCE + 1);
L224:           snprintf(msg, sizeof(msg), "The card on line %d is a GE, but no geometry cards were seen before it.", i + 1);
L230:         snprintf(msg, sizeof(msg), "The card on line %d is a GE, but we already saw one on card %d.", i, sawGE + 1);
L242:         snprintf(msg, sizeof(msg), "The card on line %d is an EN, but we already saw one on card %d.", i, sawEN + 1);
L266:         snprintf(msg, sizeof(msg),
L285:         snprintf(msg, sizeof(msg),
L319:         snprintf(msg, sizeof(msg), "The card on line %d is a GF, but we already saw one on card %d.", i, sawGF + 1);
L335:         snprintf(msg, sizeof(msg), "The card on line %d is an FR, but we already saw one on card %d.", i, sawFR + 1);
L355:         snprintf(msg, sizeof(msg), "The card on line %d is a %s, but it appears before the GE; control cards should follow geometry.", i + 1, code);
L365:         snprintf(msg, sizeof(msg), "The card on line %d is an EX, but it appears before the GE; excitations should follow geometry.", i + 1);
L370:         snprintf(msg, sizeof(msg), "The card on line %d is a TL, but it appears before the GE; transmission lines should follow geometry.", i + 1);
L375:         snprintf(msg, sizeof(msg), "The card on line %d is an LD, but it appears before the GE; loading should follow geometry.", i + 1);
L380:         snprintf(msg, sizeof(msg), "The card on line %d is an FR, but it appears before the GE; frequency setup should follow geometry.", i + 1);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 386
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 326-391) ---
    326:       {
    327:         sawFR = i;
    328:         if (freq_mhz == 0.0)
    329:         {
    330:           freq_mhz = deck->cards[i].f[1];
    331:         }
    332:       }
    333:       else
    334:       {
    335:         snprintf(msg, sizeof(msg), "The card on line %d is an FR, but we already saw one on card %d.", i, sawFR + 1);
    336:         add_error(ctx, errors, msg, 0);
    337:       }
    338:     }
    339:     if (strcmp(code, "EK") == 0)
    340:     {
    341:       // any non-zero I1 enables extended thin-wire kernel
    342:       if (deck->cards[i].ints_used >= 1 && deck->cards[i].i[1] != 0)
    343:       {
    344:         ek_enabled = 1;
    345:       }
    346:     }
    347: 
    348:     // warn if control cards appear before GE (except CE and cards with specific messages below)
    349:     if (sawGE == false)
    350:     {
    351:       if (is_control(&deck->cards[i]) && strcmp(code, "CE") != 0 &&
    352:           strcmp(code, "EX") != 0 && strcmp(code, "TL") != 0 && strcmp(code, "LD") != 0 &&
    353:           strcmp(code, "FR") != 0 && strcmp(code, "RP") != 0 && strcmp(code, "GN") != 0 && strcmp(code, "GD") != 0 && strcmp(code, "EK") != 0)
    354:       {
    355:         snprintf(msg, sizeof(msg), "The card on line %d is a %s, but it appears before the GE; control cards should follow geometry.", i + 1, code);
    356:         add_error(ctx, errors, msg, 0);
    357:       }
    358:     }
    359: 
    360:     // specific placement warnings for common control cards
    361:     if (sawGE == false)
    362:     {
    363:       if (strcmp(code, "EX") == 0)
    364:       {
    365:         snprintf(msg, sizeof(msg), "The card on line %d is an EX, but it appears before the GE; excitations should follow geometry.", i + 1);
    366:         add_error(ctx, errors, msg, 0);
    367:       }
    368:       if (strcmp(code, "TL") == 0)
    369:       {
    370:         snprintf(msg, sizeof(msg), "The card on line %d is a TL, but it appears before the GE; transmission lines should follow geometry.", i + 1);
    371:         add_error(ctx, errors, msg, 0);
    372:       }
    373:       if (strcmp(code, "LD") == 0)
    374:       {
    375:         snprintf(msg, sizeof(msg), "The card on line %d is an LD, but it appears before the GE; loading should follow geometry.", i + 1);
    376:         add_error(ctx, errors, msg, 0);
    377:       }
    378:       if (strcmp(code, "FR") == 0)
    379:       {
    380:         snprintf(msg, sizeof(msg), "The card on line %d is an FR, but it appears before the GE; frequency setup should follow geometry.", i + 1);
    381:         add_error(ctx, errors, msg, 0);
    382:       }
    383:       if (strcmp(code, "RP") == 0)
    384:       {
    385:         snprintf(msg, sizeof(msg), "The card on line %d is an RP, but it appears before the GE; pattern requests should follow geometry.", i + 1);
>   386:         add_error(ctx, errors, msg, 0);
    387:       }
    388:       if (strcmp(code, "GN") == 0)
    389:       {
    390:         snprintf(msg, sizeof(msg), "The card on line %d is a GN, but it appears before the GE; ground settings should follow geometry.", i + 1);
    391:         add_error(ctx, errors, msg, 0);

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L198:         snprintf(msg, sizeof(msg), "The card on line %d is a GS, but we already saw one on card %d. No single measurement type can be defined.", i, sawGS + 1);
L211:         snprintf(msg, sizeof(msg), "The card on line %d is a CE, but we already saw one on card %d.", i, sawCE + 1);
L224:           snprintf(msg, sizeof(msg), "The card on line %d is a GE, but no geometry cards were seen before it.", i + 1);
L230:         snprintf(msg, sizeof(msg), "The card on line %d is a GE, but we already saw one on card %d.", i, sawGE + 1);
L242:         snprintf(msg, sizeof(msg), "The card on line %d is an EN, but we already saw one on card %d.", i, sawEN + 1);
L266:         snprintf(msg, sizeof(msg),
L285:         snprintf(msg, sizeof(msg),
L319:         snprintf(msg, sizeof(msg), "The card on line %d is a GF, but we already saw one on card %d.", i, sawGF + 1);
L335:         snprintf(msg, sizeof(msg), "The card on line %d is an FR, but we already saw one on card %d.", i, sawFR + 1);
L355:         snprintf(msg, sizeof(msg), "The card on line %d is a %s, but it appears before the GE; control cards should follow geometry.", i + 1, code);
L365:         snprintf(msg, sizeof(msg), "The card on line %d is an EX, but it appears before the GE; excitations should follow geometry.", i + 1);
L370:         snprintf(msg, sizeof(msg), "The card on line %d is a TL, but it appears before the GE; transmission lines should follow geometry.", i + 1);
L375:         snprintf(msg, sizeof(msg), "The card on line %d is an LD, but it appears before the GE; loading should follow geometry.", i + 1);
L380:         snprintf(msg, sizeof(msg), "The card on line %d is an FR, but it appears before the GE; frequency setup should follow geometry.", i + 1);
L385:         snprintf(msg, sizeof(msg), "The card on line %d is an RP, but it appears before the GE; pattern requests should follow geometry.", i + 1);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 391
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 331-396) ---
    331:         }
    332:       }
    333:       else
    334:       {
    335:         snprintf(msg, sizeof(msg), "The card on line %d is an FR, but we already saw one on card %d.", i, sawFR + 1);
    336:         add_error(ctx, errors, msg, 0);
    337:       }
    338:     }
    339:     if (strcmp(code, "EK") == 0)
    340:     {
    341:       // any non-zero I1 enables extended thin-wire kernel
    342:       if (deck->cards[i].ints_used >= 1 && deck->cards[i].i[1] != 0)
    343:       {
    344:         ek_enabled = 1;
    345:       }
    346:     }
    347: 
    348:     // warn if control cards appear before GE (except CE and cards with specific messages below)
    349:     if (sawGE == false)
    350:     {
    351:       if (is_control(&deck->cards[i]) && strcmp(code, "CE") != 0 &&
    352:           strcmp(code, "EX") != 0 && strcmp(code, "TL") != 0 && strcmp(code, "LD") != 0 &&
    353:           strcmp(code, "FR") != 0 && strcmp(code, "RP") != 0 && strcmp(code, "GN") != 0 && strcmp(code, "GD") != 0 && strcmp(code, "EK") != 0)
    354:       {
    355:         snprintf(msg, sizeof(msg), "The card on line %d is a %s, but it appears before the GE; control cards should follow geometry.", i + 1, code);
    356:         add_error(ctx, errors, msg, 0);
    357:       }
    358:     }
    359: 
    360:     // specific placement warnings for common control cards
    361:     if (sawGE == false)
    362:     {
    363:       if (strcmp(code, "EX") == 0)
    364:       {
    365:         snprintf(msg, sizeof(msg), "The card on line %d is an EX, but it appears before the GE; excitations should follow geometry.", i + 1);
    366:         add_error(ctx, errors, msg, 0);
    367:       }
    368:       if (strcmp(code, "TL") == 0)
    369:       {
    370:         snprintf(msg, sizeof(msg), "The card on line %d is a TL, but it appears before the GE; transmission lines should follow geometry.", i + 1);
    371:         add_error(ctx, errors, msg, 0);
    372:       }
    373:       if (strcmp(code, "LD") == 0)
    374:       {
    375:         snprintf(msg, sizeof(msg), "The card on line %d is an LD, but it appears before the GE; loading should follow geometry.", i + 1);
    376:         add_error(ctx, errors, msg, 0);
    377:       }
    378:       if (strcmp(code, "FR") == 0)
    379:       {
    380:         snprintf(msg, sizeof(msg), "The card on line %d is an FR, but it appears before the GE; frequency setup should follow geometry.", i + 1);
    381:         add_error(ctx, errors, msg, 0);
    382:       }
    383:       if (strcmp(code, "RP") == 0)
    384:       {
    385:         snprintf(msg, sizeof(msg), "The card on line %d is an RP, but it appears before the GE; pattern requests should follow geometry.", i + 1);
    386:         add_error(ctx, errors, msg, 0);
    387:       }
    388:       if (strcmp(code, "GN") == 0)
    389:       {
    390:         snprintf(msg, sizeof(msg), "The card on line %d is a GN, but it appears before the GE; ground settings should follow geometry.", i + 1);
>   391:         add_error(ctx, errors, msg, 0);
    392:       }
    393:       if (strcmp(code, "GD") == 0)
    394:       {
    395:         snprintf(msg, sizeof(msg), "The card on line %d is a GD, but it appears before the GE; ground parameters should follow geometry.", i + 1);
    396:         add_error(ctx, errors, msg, 0);

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L198:         snprintf(msg, sizeof(msg), "The card on line %d is a GS, but we already saw one on card %d. No single measurement type can be defined.", i, sawGS + 1);
L211:         snprintf(msg, sizeof(msg), "The card on line %d is a CE, but we already saw one on card %d.", i, sawCE + 1);
L224:           snprintf(msg, sizeof(msg), "The card on line %d is a GE, but no geometry cards were seen before it.", i + 1);
L230:         snprintf(msg, sizeof(msg), "The card on line %d is a GE, but we already saw one on card %d.", i, sawGE + 1);
L242:         snprintf(msg, sizeof(msg), "The card on line %d is an EN, but we already saw one on card %d.", i, sawEN + 1);
L266:         snprintf(msg, sizeof(msg),
L285:         snprintf(msg, sizeof(msg),
L319:         snprintf(msg, sizeof(msg), "The card on line %d is a GF, but we already saw one on card %d.", i, sawGF + 1);
L335:         snprintf(msg, sizeof(msg), "The card on line %d is an FR, but we already saw one on card %d.", i, sawFR + 1);
L355:         snprintf(msg, sizeof(msg), "The card on line %d is a %s, but it appears before the GE; control cards should follow geometry.", i + 1, code);
L365:         snprintf(msg, sizeof(msg), "The card on line %d is an EX, but it appears before the GE; excitations should follow geometry.", i + 1);
L370:         snprintf(msg, sizeof(msg), "The card on line %d is a TL, but it appears before the GE; transmission lines should follow geometry.", i + 1);
L375:         snprintf(msg, sizeof(msg), "The card on line %d is an LD, but it appears before the GE; loading should follow geometry.", i + 1);
L380:         snprintf(msg, sizeof(msg), "The card on line %d is an FR, but it appears before the GE; frequency setup should follow geometry.", i + 1);
L385:         snprintf(msg, sizeof(msg), "The card on line %d is an RP, but it appears before the GE; pattern requests should follow geometry.", i + 1);
L390:         snprintf(msg, sizeof(msg), "The card on line %d is a GN, but it appears before the GE; ground settings should follow geometry.", i + 1);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 396
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 336-401) ---
    336:         add_error(ctx, errors, msg, 0);
    337:       }
    338:     }
    339:     if (strcmp(code, "EK") == 0)
    340:     {
    341:       // any non-zero I1 enables extended thin-wire kernel
    342:       if (deck->cards[i].ints_used >= 1 && deck->cards[i].i[1] != 0)
    343:       {
    344:         ek_enabled = 1;
    345:       }
    346:     }
    347: 
    348:     // warn if control cards appear before GE (except CE and cards with specific messages below)
    349:     if (sawGE == false)
    350:     {
    351:       if (is_control(&deck->cards[i]) && strcmp(code, "CE") != 0 &&
    352:           strcmp(code, "EX") != 0 && strcmp(code, "TL") != 0 && strcmp(code, "LD") != 0 &&
    353:           strcmp(code, "FR") != 0 && strcmp(code, "RP") != 0 && strcmp(code, "GN") != 0 && strcmp(code, "GD") != 0 && strcmp(code, "EK") != 0)
    354:       {
    355:         snprintf(msg, sizeof(msg), "The card on line %d is a %s, but it appears before the GE; control cards should follow geometry.", i + 1, code);
    356:         add_error(ctx, errors, msg, 0);
    357:       }
    358:     }
    359: 
    360:     // specific placement warnings for common control cards
    361:     if (sawGE == false)
    362:     {
    363:       if (strcmp(code, "EX") == 0)
    364:       {
    365:         snprintf(msg, sizeof(msg), "The card on line %d is an EX, but it appears before the GE; excitations should follow geometry.", i + 1);
    366:         add_error(ctx, errors, msg, 0);
    367:       }
    368:       if (strcmp(code, "TL") == 0)
    369:       {
    370:         snprintf(msg, sizeof(msg), "The card on line %d is a TL, but it appears before the GE; transmission lines should follow geometry.", i + 1);
    371:         add_error(ctx, errors, msg, 0);
    372:       }
    373:       if (strcmp(code, "LD") == 0)
    374:       {
    375:         snprintf(msg, sizeof(msg), "The card on line %d is an LD, but it appears before the GE; loading should follow geometry.", i + 1);
    376:         add_error(ctx, errors, msg, 0);
    377:       }
    378:       if (strcmp(code, "FR") == 0)
    379:       {
    380:         snprintf(msg, sizeof(msg), "The card on line %d is an FR, but it appears before the GE; frequency setup should follow geometry.", i + 1);
    381:         add_error(ctx, errors, msg, 0);
    382:       }
    383:       if (strcmp(code, "RP") == 0)
    384:       {
    385:         snprintf(msg, sizeof(msg), "The card on line %d is an RP, but it appears before the GE; pattern requests should follow geometry.", i + 1);
    386:         add_error(ctx, errors, msg, 0);
    387:       }
    388:       if (strcmp(code, "GN") == 0)
    389:       {
    390:         snprintf(msg, sizeof(msg), "The card on line %d is a GN, but it appears before the GE; ground settings should follow geometry.", i + 1);
    391:         add_error(ctx, errors, msg, 0);
    392:       }
    393:       if (strcmp(code, "GD") == 0)
    394:       {
    395:         snprintf(msg, sizeof(msg), "The card on line %d is a GD, but it appears before the GE; ground parameters should follow geometry.", i + 1);
>   396:         add_error(ctx, errors, msg, 0);
    397:       }
    398:       if (strcmp(code, "EK") == 0)
    399:       {
    400:         snprintf(msg, sizeof(msg), "The card on line %d is an EK, but it appears before the GE; EK should follow geometry to apply the extended kernel.", i + 1);
    401:         add_error(ctx, errors, msg, 0);

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L198:         snprintf(msg, sizeof(msg), "The card on line %d is a GS, but we already saw one on card %d. No single measurement type can be defined.", i, sawGS + 1);
L211:         snprintf(msg, sizeof(msg), "The card on line %d is a CE, but we already saw one on card %d.", i, sawCE + 1);
L224:           snprintf(msg, sizeof(msg), "The card on line %d is a GE, but no geometry cards were seen before it.", i + 1);
L230:         snprintf(msg, sizeof(msg), "The card on line %d is a GE, but we already saw one on card %d.", i, sawGE + 1);
L242:         snprintf(msg, sizeof(msg), "The card on line %d is an EN, but we already saw one on card %d.", i, sawEN + 1);
L266:         snprintf(msg, sizeof(msg),
L285:         snprintf(msg, sizeof(msg),
L319:         snprintf(msg, sizeof(msg), "The card on line %d is a GF, but we already saw one on card %d.", i, sawGF + 1);
L335:         snprintf(msg, sizeof(msg), "The card on line %d is an FR, but we already saw one on card %d.", i, sawFR + 1);
L355:         snprintf(msg, sizeof(msg), "The card on line %d is a %s, but it appears before the GE; control cards should follow geometry.", i + 1, code);
L365:         snprintf(msg, sizeof(msg), "The card on line %d is an EX, but it appears before the GE; excitations should follow geometry.", i + 1);
L370:         snprintf(msg, sizeof(msg), "The card on line %d is a TL, but it appears before the GE; transmission lines should follow geometry.", i + 1);
L375:         snprintf(msg, sizeof(msg), "The card on line %d is an LD, but it appears before the GE; loading should follow geometry.", i + 1);
L380:         snprintf(msg, sizeof(msg), "The card on line %d is an FR, but it appears before the GE; frequency setup should follow geometry.", i + 1);
L385:         snprintf(msg, sizeof(msg), "The card on line %d is an RP, but it appears before the GE; pattern requests should follow geometry.", i + 1);
L390:         snprintf(msg, sizeof(msg), "The card on line %d is a GN, but it appears before the GE; ground settings should follow geometry.", i + 1);
L395:         snprintf(msg, sizeof(msg), "The card on line %d is a GD, but it appears before the GE; ground parameters should follow geometry.", i + 1);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 401
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 341-406) ---
    341:       // any non-zero I1 enables extended thin-wire kernel
    342:       if (deck->cards[i].ints_used >= 1 && deck->cards[i].i[1] != 0)
    343:       {
    344:         ek_enabled = 1;
    345:       }
    346:     }
    347: 
    348:     // warn if control cards appear before GE (except CE and cards with specific messages below)
    349:     if (sawGE == false)
    350:     {
    351:       if (is_control(&deck->cards[i]) && strcmp(code, "CE") != 0 &&
    352:           strcmp(code, "EX") != 0 && strcmp(code, "TL") != 0 && strcmp(code, "LD") != 0 &&
    353:           strcmp(code, "FR") != 0 && strcmp(code, "RP") != 0 && strcmp(code, "GN") != 0 && strcmp(code, "GD") != 0 && strcmp(code, "EK") != 0)
    354:       {
    355:         snprintf(msg, sizeof(msg), "The card on line %d is a %s, but it appears before the GE; control cards should follow geometry.", i + 1, code);
    356:         add_error(ctx, errors, msg, 0);
    357:       }
    358:     }
    359: 
    360:     // specific placement warnings for common control cards
    361:     if (sawGE == false)
    362:     {
    363:       if (strcmp(code, "EX") == 0)
    364:       {
    365:         snprintf(msg, sizeof(msg), "The card on line %d is an EX, but it appears before the GE; excitations should follow geometry.", i + 1);
    366:         add_error(ctx, errors, msg, 0);
    367:       }
    368:       if (strcmp(code, "TL") == 0)
    369:       {
    370:         snprintf(msg, sizeof(msg), "The card on line %d is a TL, but it appears before the GE; transmission lines should follow geometry.", i + 1);
    371:         add_error(ctx, errors, msg, 0);
    372:       }
    373:       if (strcmp(code, "LD") == 0)
    374:       {
    375:         snprintf(msg, sizeof(msg), "The card on line %d is an LD, but it appears before the GE; loading should follow geometry.", i + 1);
    376:         add_error(ctx, errors, msg, 0);
    377:       }
    378:       if (strcmp(code, "FR") == 0)
    379:       {
    380:         snprintf(msg, sizeof(msg), "The card on line %d is an FR, but it appears before the GE; frequency setup should follow geometry.", i + 1);
    381:         add_error(ctx, errors, msg, 0);
    382:       }
    383:       if (strcmp(code, "RP") == 0)
    384:       {
    385:         snprintf(msg, sizeof(msg), "The card on line %d is an RP, but it appears before the GE; pattern requests should follow geometry.", i + 1);
    386:         add_error(ctx, errors, msg, 0);
    387:       }
    388:       if (strcmp(code, "GN") == 0)
    389:       {
    390:         snprintf(msg, sizeof(msg), "The card on line %d is a GN, but it appears before the GE; ground settings should follow geometry.", i + 1);
    391:         add_error(ctx, errors, msg, 0);
    392:       }
    393:       if (strcmp(code, "GD") == 0)
    394:       {
    395:         snprintf(msg, sizeof(msg), "The card on line %d is a GD, but it appears before the GE; ground parameters should follow geometry.", i + 1);
    396:         add_error(ctx, errors, msg, 0);
    397:       }
    398:       if (strcmp(code, "EK") == 0)
    399:       {
    400:         snprintf(msg, sizeof(msg), "The card on line %d is an EK, but it appears before the GE; EK should follow geometry to apply the extended kernel.", i + 1);
>   401:         add_error(ctx, errors, msg, 0);
    402:       }
    403:     }
    404: 
    405:     // GE: optional I1 in {-1,0,1,2}; no floats expected
    406:     if (strcmp(code, "GE") == 0)

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L211:         snprintf(msg, sizeof(msg), "The card on line %d is a CE, but we already saw one on card %d.", i, sawCE + 1);
L224:           snprintf(msg, sizeof(msg), "The card on line %d is a GE, but no geometry cards were seen before it.", i + 1);
L230:         snprintf(msg, sizeof(msg), "The card on line %d is a GE, but we already saw one on card %d.", i, sawGE + 1);
L242:         snprintf(msg, sizeof(msg), "The card on line %d is an EN, but we already saw one on card %d.", i, sawEN + 1);
L266:         snprintf(msg, sizeof(msg),
L285:         snprintf(msg, sizeof(msg),
L319:         snprintf(msg, sizeof(msg), "The card on line %d is a GF, but we already saw one on card %d.", i, sawGF + 1);
L335:         snprintf(msg, sizeof(msg), "The card on line %d is an FR, but we already saw one on card %d.", i, sawFR + 1);
L355:         snprintf(msg, sizeof(msg), "The card on line %d is a %s, but it appears before the GE; control cards should follow geometry.", i + 1, code);
L365:         snprintf(msg, sizeof(msg), "The card on line %d is an EX, but it appears before the GE; excitations should follow geometry.", i + 1);
L370:         snprintf(msg, sizeof(msg), "The card on line %d is a TL, but it appears before the GE; transmission lines should follow geometry.", i + 1);
L375:         snprintf(msg, sizeof(msg), "The card on line %d is an LD, but it appears before the GE; loading should follow geometry.", i + 1);
L380:         snprintf(msg, sizeof(msg), "The card on line %d is an FR, but it appears before the GE; frequency setup should follow geometry.", i + 1);
L385:         snprintf(msg, sizeof(msg), "The card on line %d is an RP, but it appears before the GE; pattern requests should follow geometry.", i + 1);
L390:         snprintf(msg, sizeof(msg), "The card on line %d is a GN, but it appears before the GE; ground settings should follow geometry.", i + 1);
L395:         snprintf(msg, sizeof(msg), "The card on line %d is a GD, but it appears before the GE; ground parameters should follow geometry.", i + 1);
L400:         snprintf(msg, sizeof(msg), "The card on line %d is an EK, but it appears before the GE; EK should follow geometry to apply the extended kernel.", i + 1);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 411
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 351-416) ---
    351:       if (is_control(&deck->cards[i]) && strcmp(code, "CE") != 0 &&
    352:           strcmp(code, "EX") != 0 && strcmp(code, "TL") != 0 && strcmp(code, "LD") != 0 &&
    353:           strcmp(code, "FR") != 0 && strcmp(code, "RP") != 0 && strcmp(code, "GN") != 0 && strcmp(code, "GD") != 0 && strcmp(code, "EK") != 0)
    354:       {
    355:         snprintf(msg, sizeof(msg), "The card on line %d is a %s, but it appears before the GE; control cards should follow geometry.", i + 1, code);
    356:         add_error(ctx, errors, msg, 0);
    357:       }
    358:     }
    359: 
    360:     // specific placement warnings for common control cards
    361:     if (sawGE == false)
    362:     {
    363:       if (strcmp(code, "EX") == 0)
    364:       {
    365:         snprintf(msg, sizeof(msg), "The card on line %d is an EX, but it appears before the GE; excitations should follow geometry.", i + 1);
    366:         add_error(ctx, errors, msg, 0);
    367:       }
    368:       if (strcmp(code, "TL") == 0)
    369:       {
    370:         snprintf(msg, sizeof(msg), "The card on line %d is a TL, but it appears before the GE; transmission lines should follow geometry.", i + 1);
    371:         add_error(ctx, errors, msg, 0);
    372:       }
    373:       if (strcmp(code, "LD") == 0)
    374:       {
    375:         snprintf(msg, sizeof(msg), "The card on line %d is an LD, but it appears before the GE; loading should follow geometry.", i + 1);
    376:         add_error(ctx, errors, msg, 0);
    377:       }
    378:       if (strcmp(code, "FR") == 0)
    379:       {
    380:         snprintf(msg, sizeof(msg), "The card on line %d is an FR, but it appears before the GE; frequency setup should follow geometry.", i + 1);
    381:         add_error(ctx, errors, msg, 0);
    382:       }
    383:       if (strcmp(code, "RP") == 0)
    384:       {
    385:         snprintf(msg, sizeof(msg), "The card on line %d is an RP, but it appears before the GE; pattern requests should follow geometry.", i + 1);
    386:         add_error(ctx, errors, msg, 0);
    387:       }
    388:       if (strcmp(code, "GN") == 0)
    389:       {
    390:         snprintf(msg, sizeof(msg), "The card on line %d is a GN, but it appears before the GE; ground settings should follow geometry.", i + 1);
    391:         add_error(ctx, errors, msg, 0);
    392:       }
    393:       if (strcmp(code, "GD") == 0)
    394:       {
    395:         snprintf(msg, sizeof(msg), "The card on line %d is a GD, but it appears before the GE; ground parameters should follow geometry.", i + 1);
    396:         add_error(ctx, errors, msg, 0);
    397:       }
    398:       if (strcmp(code, "EK") == 0)
    399:       {
    400:         snprintf(msg, sizeof(msg), "The card on line %d is an EK, but it appears before the GE; EK should follow geometry to apply the extended kernel.", i + 1);
    401:         add_error(ctx, errors, msg, 0);
    402:       }
    403:     }
    404: 
    405:     // GE: optional I1 in {-1,0,1,2}; no floats expected
    406:     if (strcmp(code, "GE") == 0)
    407:     {
    408:       if (deck->cards[i].ints_used > 1)
    409:       {
    410:         snprintf(msg, sizeof(msg), "The card on line %d is a GE but has more than one integer input.", i);
>   411:         add_error(ctx, errors, msg, 0);
    412:       }
    413:       int gei1 = deck->cards[i].i[1];
    414:       if (!(gei1 == -1 || gei1 == 0 || gei1 == 1 || gei1 == 2))
    415:       {
    416:         snprintf(msg, sizeof(msg), "The card on line %d is a GE with I1=%d, which is outside the typical range {-1,0,1,2}.", i, gei1);

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L211:         snprintf(msg, sizeof(msg), "The card on line %d is a CE, but we already saw one on card %d.", i, sawCE + 1);
L224:           snprintf(msg, sizeof(msg), "The card on line %d is a GE, but no geometry cards were seen before it.", i + 1);
L230:         snprintf(msg, sizeof(msg), "The card on line %d is a GE, but we already saw one on card %d.", i, sawGE + 1);
L242:         snprintf(msg, sizeof(msg), "The card on line %d is an EN, but we already saw one on card %d.", i, sawEN + 1);
L266:         snprintf(msg, sizeof(msg),
L285:         snprintf(msg, sizeof(msg),
L319:         snprintf(msg, sizeof(msg), "The card on line %d is a GF, but we already saw one on card %d.", i, sawGF + 1);
L335:         snprintf(msg, sizeof(msg), "The card on line %d is an FR, but we already saw one on card %d.", i, sawFR + 1);
L355:         snprintf(msg, sizeof(msg), "The card on line %d is a %s, but it appears before the GE; control cards should follow geometry.", i + 1, code);
L365:         snprintf(msg, sizeof(msg), "The card on line %d is an EX, but it appears before the GE; excitations should follow geometry.", i + 1);
L370:         snprintf(msg, sizeof(msg), "The card on line %d is a TL, but it appears before the GE; transmission lines should follow geometry.", i + 1);
L375:         snprintf(msg, sizeof(msg), "The card on line %d is an LD, but it appears before the GE; loading should follow geometry.", i + 1);
L380:         snprintf(msg, sizeof(msg), "The card on line %d is an FR, but it appears before the GE; frequency setup should follow geometry.", i + 1);
L385:         snprintf(msg, sizeof(msg), "The card on line %d is an RP, but it appears before the GE; pattern requests should follow geometry.", i + 1);
L390:         snprintf(msg, sizeof(msg), "The card on line %d is a GN, but it appears before the GE; ground settings should follow geometry.", i + 1);
L395:         snprintf(msg, sizeof(msg), "The card on line %d is a GD, but it appears before the GE; ground parameters should follow geometry.", i + 1);
L400:         snprintf(msg, sizeof(msg), "The card on line %d is an EK, but it appears before the GE; EK should follow geometry to apply the extended kernel.", i + 1);
L410:         snprintf(msg, sizeof(msg), "The card on line %d is a GE but has more than one integer input.", i);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 417
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 357-422) ---
    357:       }
    358:     }
    359: 
    360:     // specific placement warnings for common control cards
    361:     if (sawGE == false)
    362:     {
    363:       if (strcmp(code, "EX") == 0)
    364:       {
    365:         snprintf(msg, sizeof(msg), "The card on line %d is an EX, but it appears before the GE; excitations should follow geometry.", i + 1);
    366:         add_error(ctx, errors, msg, 0);
    367:       }
    368:       if (strcmp(code, "TL") == 0)
    369:       {
    370:         snprintf(msg, sizeof(msg), "The card on line %d is a TL, but it appears before the GE; transmission lines should follow geometry.", i + 1);
    371:         add_error(ctx, errors, msg, 0);
    372:       }
    373:       if (strcmp(code, "LD") == 0)
    374:       {
    375:         snprintf(msg, sizeof(msg), "The card on line %d is an LD, but it appears before the GE; loading should follow geometry.", i + 1);
    376:         add_error(ctx, errors, msg, 0);
    377:       }
    378:       if (strcmp(code, "FR") == 0)
    379:       {
    380:         snprintf(msg, sizeof(msg), "The card on line %d is an FR, but it appears before the GE; frequency setup should follow geometry.", i + 1);
    381:         add_error(ctx, errors, msg, 0);
    382:       }
    383:       if (strcmp(code, "RP") == 0)
    384:       {
    385:         snprintf(msg, sizeof(msg), "The card on line %d is an RP, but it appears before the GE; pattern requests should follow geometry.", i + 1);
    386:         add_error(ctx, errors, msg, 0);
    387:       }
    388:       if (strcmp(code, "GN") == 0)
    389:       {
    390:         snprintf(msg, sizeof(msg), "The card on line %d is a GN, but it appears before the GE; ground settings should follow geometry.", i + 1);
    391:         add_error(ctx, errors, msg, 0);
    392:       }
    393:       if (strcmp(code, "GD") == 0)
    394:       {
    395:         snprintf(msg, sizeof(msg), "The card on line %d is a GD, but it appears before the GE; ground parameters should follow geometry.", i + 1);
    396:         add_error(ctx, errors, msg, 0);
    397:       }
    398:       if (strcmp(code, "EK") == 0)
    399:       {
    400:         snprintf(msg, sizeof(msg), "The card on line %d is an EK, but it appears before the GE; EK should follow geometry to apply the extended kernel.", i + 1);
    401:         add_error(ctx, errors, msg, 0);
    402:       }
    403:     }
    404: 
    405:     // GE: optional I1 in {-1,0,1,2}; no floats expected
    406:     if (strcmp(code, "GE") == 0)
    407:     {
    408:       if (deck->cards[i].ints_used > 1)
    409:       {
    410:         snprintf(msg, sizeof(msg), "The card on line %d is a GE but has more than one integer input.", i);
    411:         add_error(ctx, errors, msg, 0);
    412:       }
    413:       int gei1 = deck->cards[i].i[1];
    414:       if (!(gei1 == -1 || gei1 == 0 || gei1 == 1 || gei1 == 2))
    415:       {
    416:         snprintf(msg, sizeof(msg), "The card on line %d is a GE with I1=%d, which is outside the typical range {-1,0,1,2}.", i, gei1);
>   417:         add_error(ctx, errors, msg, 0);
    418:       }
    419:       if (deck->cards[i].flts_used > 0)
    420:       {
    421:         snprintf(msg, sizeof(msg), "The card on line %d is a GE but has floating-point inputs, which are not expected.", i);
    422:         add_error(ctx, errors, msg, 0);

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L224:           snprintf(msg, sizeof(msg), "The card on line %d is a GE, but no geometry cards were seen before it.", i + 1);
L230:         snprintf(msg, sizeof(msg), "The card on line %d is a GE, but we already saw one on card %d.", i, sawGE + 1);
L242:         snprintf(msg, sizeof(msg), "The card on line %d is an EN, but we already saw one on card %d.", i, sawEN + 1);
L266:         snprintf(msg, sizeof(msg),
L285:         snprintf(msg, sizeof(msg),
L319:         snprintf(msg, sizeof(msg), "The card on line %d is a GF, but we already saw one on card %d.", i, sawGF + 1);
L335:         snprintf(msg, sizeof(msg), "The card on line %d is an FR, but we already saw one on card %d.", i, sawFR + 1);
L355:         snprintf(msg, sizeof(msg), "The card on line %d is a %s, but it appears before the GE; control cards should follow geometry.", i + 1, code);
L365:         snprintf(msg, sizeof(msg), "The card on line %d is an EX, but it appears before the GE; excitations should follow geometry.", i + 1);
L370:         snprintf(msg, sizeof(msg), "The card on line %d is a TL, but it appears before the GE; transmission lines should follow geometry.", i + 1);
L375:         snprintf(msg, sizeof(msg), "The card on line %d is an LD, but it appears before the GE; loading should follow geometry.", i + 1);
L380:         snprintf(msg, sizeof(msg), "The card on line %d is an FR, but it appears before the GE; frequency setup should follow geometry.", i + 1);
L385:         snprintf(msg, sizeof(msg), "The card on line %d is an RP, but it appears before the GE; pattern requests should follow geometry.", i + 1);
L390:         snprintf(msg, sizeof(msg), "The card on line %d is a GN, but it appears before the GE; ground settings should follow geometry.", i + 1);
L395:         snprintf(msg, sizeof(msg), "The card on line %d is a GD, but it appears before the GE; ground parameters should follow geometry.", i + 1);
L400:         snprintf(msg, sizeof(msg), "The card on line %d is an EK, but it appears before the GE; EK should follow geometry to apply the extended kernel.", i + 1);
L410:         snprintf(msg, sizeof(msg), "The card on line %d is a GE but has more than one integer input.", i);
L416:         snprintf(msg, sizeof(msg), "The card on line %d is a GE with I1=%d, which is outside the typical range {-1,0,1,2}.", i, gei1);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 422
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 362-427) ---
    362:     {
    363:       if (strcmp(code, "EX") == 0)
    364:       {
    365:         snprintf(msg, sizeof(msg), "The card on line %d is an EX, but it appears before the GE; excitations should follow geometry.", i + 1);
    366:         add_error(ctx, errors, msg, 0);
    367:       }
    368:       if (strcmp(code, "TL") == 0)
    369:       {
    370:         snprintf(msg, sizeof(msg), "The card on line %d is a TL, but it appears before the GE; transmission lines should follow geometry.", i + 1);
    371:         add_error(ctx, errors, msg, 0);
    372:       }
    373:       if (strcmp(code, "LD") == 0)
    374:       {
    375:         snprintf(msg, sizeof(msg), "The card on line %d is an LD, but it appears before the GE; loading should follow geometry.", i + 1);
    376:         add_error(ctx, errors, msg, 0);
    377:       }
    378:       if (strcmp(code, "FR") == 0)
    379:       {
    380:         snprintf(msg, sizeof(msg), "The card on line %d is an FR, but it appears before the GE; frequency setup should follow geometry.", i + 1);
    381:         add_error(ctx, errors, msg, 0);
    382:       }
    383:       if (strcmp(code, "RP") == 0)
    384:       {
    385:         snprintf(msg, sizeof(msg), "The card on line %d is an RP, but it appears before the GE; pattern requests should follow geometry.", i + 1);
    386:         add_error(ctx, errors, msg, 0);
    387:       }
    388:       if (strcmp(code, "GN") == 0)
    389:       {
    390:         snprintf(msg, sizeof(msg), "The card on line %d is a GN, but it appears before the GE; ground settings should follow geometry.", i + 1);
    391:         add_error(ctx, errors, msg, 0);
    392:       }
    393:       if (strcmp(code, "GD") == 0)
    394:       {
    395:         snprintf(msg, sizeof(msg), "The card on line %d is a GD, but it appears before the GE; ground parameters should follow geometry.", i + 1);
    396:         add_error(ctx, errors, msg, 0);
    397:       }
    398:       if (strcmp(code, "EK") == 0)
    399:       {
    400:         snprintf(msg, sizeof(msg), "The card on line %d is an EK, but it appears before the GE; EK should follow geometry to apply the extended kernel.", i + 1);
    401:         add_error(ctx, errors, msg, 0);
    402:       }
    403:     }
    404: 
    405:     // GE: optional I1 in {-1,0,1,2}; no floats expected
    406:     if (strcmp(code, "GE") == 0)
    407:     {
    408:       if (deck->cards[i].ints_used > 1)
    409:       {
    410:         snprintf(msg, sizeof(msg), "The card on line %d is a GE but has more than one integer input.", i);
    411:         add_error(ctx, errors, msg, 0);
    412:       }
    413:       int gei1 = deck->cards[i].i[1];
    414:       if (!(gei1 == -1 || gei1 == 0 || gei1 == 1 || gei1 == 2))
    415:       {
    416:         snprintf(msg, sizeof(msg), "The card on line %d is a GE with I1=%d, which is outside the typical range {-1,0,1,2}.", i, gei1);
    417:         add_error(ctx, errors, msg, 0);
    418:       }
    419:       if (deck->cards[i].flts_used > 0)
    420:       {
    421:         snprintf(msg, sizeof(msg), "The card on line %d is a GE but has floating-point inputs, which are not expected.", i);
>   422:         add_error(ctx, errors, msg, 0);
    423:       }
    424:     }
    425: 
    426:     // TL expects 4 integer locators (tags/segments) and Z0 in F1
    427:     if (strcmp(code, "TL") == 0)

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L224:           snprintf(msg, sizeof(msg), "The card on line %d is a GE, but no geometry cards were seen before it.", i + 1);
L230:         snprintf(msg, sizeof(msg), "The card on line %d is a GE, but we already saw one on card %d.", i, sawGE + 1);
L242:         snprintf(msg, sizeof(msg), "The card on line %d is an EN, but we already saw one on card %d.", i, sawEN + 1);
L266:         snprintf(msg, sizeof(msg),
L285:         snprintf(msg, sizeof(msg),
L319:         snprintf(msg, sizeof(msg), "The card on line %d is a GF, but we already saw one on card %d.", i, sawGF + 1);
L335:         snprintf(msg, sizeof(msg), "The card on line %d is an FR, but we already saw one on card %d.", i, sawFR + 1);
L355:         snprintf(msg, sizeof(msg), "The card on line %d is a %s, but it appears before the GE; control cards should follow geometry.", i + 1, code);
L365:         snprintf(msg, sizeof(msg), "The card on line %d is an EX, but it appears before the GE; excitations should follow geometry.", i + 1);
L370:         snprintf(msg, sizeof(msg), "The card on line %d is a TL, but it appears before the GE; transmission lines should follow geometry.", i + 1);
L375:         snprintf(msg, sizeof(msg), "The card on line %d is an LD, but it appears before the GE; loading should follow geometry.", i + 1);
L380:         snprintf(msg, sizeof(msg), "The card on line %d is an FR, but it appears before the GE; frequency setup should follow geometry.", i + 1);
L385:         snprintf(msg, sizeof(msg), "The card on line %d is an RP, but it appears before the GE; pattern requests should follow geometry.", i + 1);
L390:         snprintf(msg, sizeof(msg), "The card on line %d is a GN, but it appears before the GE; ground settings should follow geometry.", i + 1);
L395:         snprintf(msg, sizeof(msg), "The card on line %d is a GD, but it appears before the GE; ground parameters should follow geometry.", i + 1);
L400:         snprintf(msg, sizeof(msg), "The card on line %d is an EK, but it appears before the GE; EK should follow geometry to apply the extended kernel.", i + 1);
L410:         snprintf(msg, sizeof(msg), "The card on line %d is a GE but has more than one integer input.", i);
L416:         snprintf(msg, sizeof(msg), "The card on line %d is a GE with I1=%d, which is outside the typical range {-1,0,1,2}.", i, gei1);
L421:         snprintf(msg, sizeof(msg), "The card on line %d is a GE but has floating-point inputs, which are not expected.", i);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 432
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 372-437) ---
    372:       }
    373:       if (strcmp(code, "LD") == 0)
    374:       {
    375:         snprintf(msg, sizeof(msg), "The card on line %d is an LD, but it appears before the GE; loading should follow geometry.", i + 1);
    376:         add_error(ctx, errors, msg, 0);
    377:       }
    378:       if (strcmp(code, "FR") == 0)
    379:       {
    380:         snprintf(msg, sizeof(msg), "The card on line %d is an FR, but it appears before the GE; frequency setup should follow geometry.", i + 1);
    381:         add_error(ctx, errors, msg, 0);
    382:       }
    383:       if (strcmp(code, "RP") == 0)
    384:       {
    385:         snprintf(msg, sizeof(msg), "The card on line %d is an RP, but it appears before the GE; pattern requests should follow geometry.", i + 1);
    386:         add_error(ctx, errors, msg, 0);
    387:       }
    388:       if (strcmp(code, "GN") == 0)
    389:       {
    390:         snprintf(msg, sizeof(msg), "The card on line %d is a GN, but it appears before the GE; ground settings should follow geometry.", i + 1);
    391:         add_error(ctx, errors, msg, 0);
    392:       }
    393:       if (strcmp(code, "GD") == 0)
    394:       {
    395:         snprintf(msg, sizeof(msg), "The card on line %d is a GD, but it appears before the GE; ground parameters should follow geometry.", i + 1);
    396:         add_error(ctx, errors, msg, 0);
    397:       }
    398:       if (strcmp(code, "EK") == 0)
    399:       {
    400:         snprintf(msg, sizeof(msg), "The card on line %d is an EK, but it appears before the GE; EK should follow geometry to apply the extended kernel.", i + 1);
    401:         add_error(ctx, errors, msg, 0);
    402:       }
    403:     }
    404: 
    405:     // GE: optional I1 in {-1,0,1,2}; no floats expected
    406:     if (strcmp(code, "GE") == 0)
    407:     {
    408:       if (deck->cards[i].ints_used > 1)
    409:       {
    410:         snprintf(msg, sizeof(msg), "The card on line %d is a GE but has more than one integer input.", i);
    411:         add_error(ctx, errors, msg, 0);
    412:       }
    413:       int gei1 = deck->cards[i].i[1];
    414:       if (!(gei1 == -1 || gei1 == 0 || gei1 == 1 || gei1 == 2))
    415:       {
    416:         snprintf(msg, sizeof(msg), "The card on line %d is a GE with I1=%d, which is outside the typical range {-1,0,1,2}.", i, gei1);
    417:         add_error(ctx, errors, msg, 0);
    418:       }
    419:       if (deck->cards[i].flts_used > 0)
    420:       {
    421:         snprintf(msg, sizeof(msg), "The card on line %d is a GE but has floating-point inputs, which are not expected.", i);
    422:         add_error(ctx, errors, msg, 0);
    423:       }
    424:     }
    425: 
    426:     // TL expects 4 integer locators (tags/segments) and Z0 in F1
    427:     if (strcmp(code, "TL") == 0)
    428:     {
    429:       if (deck->cards[i].ints_used < 4)
    430:       {
    431:         snprintf(msg, sizeof(msg), "The card on line %d is a TL but has fewer than 4 integer inputs (tag/segment locators).", i);
>   432:         add_error(ctx, errors, msg, 0);
    433:       }
    434:       // basic sanity: locators positive
    435:       if (deck->cards[i].i[1] <= 0 || deck->cards[i].i[2] <= 0 || deck->cards[i].i[3] <= 0 || deck->cards[i].i[4] <= 0)
    436:       {
    437:         snprintf(msg, sizeof(msg), "The card on line %d is a TL with non-positive tag/segment locator(s).", i);

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L242:         snprintf(msg, sizeof(msg), "The card on line %d is an EN, but we already saw one on card %d.", i, sawEN + 1);
L266:         snprintf(msg, sizeof(msg),
L285:         snprintf(msg, sizeof(msg),
L319:         snprintf(msg, sizeof(msg), "The card on line %d is a GF, but we already saw one on card %d.", i, sawGF + 1);
L335:         snprintf(msg, sizeof(msg), "The card on line %d is an FR, but we already saw one on card %d.", i, sawFR + 1);
L355:         snprintf(msg, sizeof(msg), "The card on line %d is a %s, but it appears before the GE; control cards should follow geometry.", i + 1, code);
L365:         snprintf(msg, sizeof(msg), "The card on line %d is an EX, but it appears before the GE; excitations should follow geometry.", i + 1);
L370:         snprintf(msg, sizeof(msg), "The card on line %d is a TL, but it appears before the GE; transmission lines should follow geometry.", i + 1);
L375:         snprintf(msg, sizeof(msg), "The card on line %d is an LD, but it appears before the GE; loading should follow geometry.", i + 1);
L380:         snprintf(msg, sizeof(msg), "The card on line %d is an FR, but it appears before the GE; frequency setup should follow geometry.", i + 1);
L385:         snprintf(msg, sizeof(msg), "The card on line %d is an RP, but it appears before the GE; pattern requests should follow geometry.", i + 1);
L390:         snprintf(msg, sizeof(msg), "The card on line %d is a GN, but it appears before the GE; ground settings should follow geometry.", i + 1);
L395:         snprintf(msg, sizeof(msg), "The card on line %d is a GD, but it appears before the GE; ground parameters should follow geometry.", i + 1);
L400:         snprintf(msg, sizeof(msg), "The card on line %d is an EK, but it appears before the GE; EK should follow geometry to apply the extended kernel.", i + 1);
L410:         snprintf(msg, sizeof(msg), "The card on line %d is a GE but has more than one integer input.", i);
L416:         snprintf(msg, sizeof(msg), "The card on line %d is a GE with I1=%d, which is outside the typical range {-1,0,1,2}.", i, gei1);
L421:         snprintf(msg, sizeof(msg), "The card on line %d is a GE but has floating-point inputs, which are not expected.", i);
L431:         snprintf(msg, sizeof(msg), "The card on line %d is a TL but has fewer than 4 integer inputs (tag/segment locators).", i);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 438
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 378-443) ---
    378:       if (strcmp(code, "FR") == 0)
    379:       {
    380:         snprintf(msg, sizeof(msg), "The card on line %d is an FR, but it appears before the GE; frequency setup should follow geometry.", i + 1);
    381:         add_error(ctx, errors, msg, 0);
    382:       }
    383:       if (strcmp(code, "RP") == 0)
    384:       {
    385:         snprintf(msg, sizeof(msg), "The card on line %d is an RP, but it appears before the GE; pattern requests should follow geometry.", i + 1);
    386:         add_error(ctx, errors, msg, 0);
    387:       }
    388:       if (strcmp(code, "GN") == 0)
    389:       {
    390:         snprintf(msg, sizeof(msg), "The card on line %d is a GN, but it appears before the GE; ground settings should follow geometry.", i + 1);
    391:         add_error(ctx, errors, msg, 0);
    392:       }
    393:       if (strcmp(code, "GD") == 0)
    394:       {
    395:         snprintf(msg, sizeof(msg), "The card on line %d is a GD, but it appears before the GE; ground parameters should follow geometry.", i + 1);
    396:         add_error(ctx, errors, msg, 0);
    397:       }
    398:       if (strcmp(code, "EK") == 0)
    399:       {
    400:         snprintf(msg, sizeof(msg), "The card on line %d is an EK, but it appears before the GE; EK should follow geometry to apply the extended kernel.", i + 1);
    401:         add_error(ctx, errors, msg, 0);
    402:       }
    403:     }
    404: 
    405:     // GE: optional I1 in {-1,0,1,2}; no floats expected
    406:     if (strcmp(code, "GE") == 0)
    407:     {
    408:       if (deck->cards[i].ints_used > 1)
    409:       {
    410:         snprintf(msg, sizeof(msg), "The card on line %d is a GE but has more than one integer input.", i);
    411:         add_error(ctx, errors, msg, 0);
    412:       }
    413:       int gei1 = deck->cards[i].i[1];
    414:       if (!(gei1 == -1 || gei1 == 0 || gei1 == 1 || gei1 == 2))
    415:       {
    416:         snprintf(msg, sizeof(msg), "The card on line %d is a GE with I1=%d, which is outside the typical range {-1,0,1,2}.", i, gei1);
    417:         add_error(ctx, errors, msg, 0);
    418:       }
    419:       if (deck->cards[i].flts_used > 0)
    420:       {
    421:         snprintf(msg, sizeof(msg), "The card on line %d is a GE but has floating-point inputs, which are not expected.", i);
    422:         add_error(ctx, errors, msg, 0);
    423:       }
    424:     }
    425: 
    426:     // TL expects 4 integer locators (tags/segments) and Z0 in F1
    427:     if (strcmp(code, "TL") == 0)
    428:     {
    429:       if (deck->cards[i].ints_used < 4)
    430:       {
    431:         snprintf(msg, sizeof(msg), "The card on line %d is a TL but has fewer than 4 integer inputs (tag/segment locators).", i);
    432:         add_error(ctx, errors, msg, 0);
    433:       }
    434:       // basic sanity: locators positive
    435:       if (deck->cards[i].i[1] <= 0 || deck->cards[i].i[2] <= 0 || deck->cards[i].i[3] <= 0 || deck->cards[i].i[4] <= 0)
    436:       {
    437:         snprintf(msg, sizeof(msg), "The card on line %d is a TL with non-positive tag/segment locator(s).", i);
>   438:         add_error(ctx, errors, msg, 0);
    439:       }
    440:       if (deck->cards[i].flts_used < 1)
    441:       {
    442:         snprintf(msg, sizeof(msg), "The card on line %d is a TL but has no characteristic impedance in F1.", i);
    443:         add_error(ctx, errors, msg, 0);

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L242:         snprintf(msg, sizeof(msg), "The card on line %d is an EN, but we already saw one on card %d.", i, sawEN + 1);
L266:         snprintf(msg, sizeof(msg),
L285:         snprintf(msg, sizeof(msg),
L319:         snprintf(msg, sizeof(msg), "The card on line %d is a GF, but we already saw one on card %d.", i, sawGF + 1);
L335:         snprintf(msg, sizeof(msg), "The card on line %d is an FR, but we already saw one on card %d.", i, sawFR + 1);
L355:         snprintf(msg, sizeof(msg), "The card on line %d is a %s, but it appears before the GE; control cards should follow geometry.", i + 1, code);
L365:         snprintf(msg, sizeof(msg), "The card on line %d is an EX, but it appears before the GE; excitations should follow geometry.", i + 1);
L370:         snprintf(msg, sizeof(msg), "The card on line %d is a TL, but it appears before the GE; transmission lines should follow geometry.", i + 1);
L375:         snprintf(msg, sizeof(msg), "The card on line %d is an LD, but it appears before the GE; loading should follow geometry.", i + 1);
L380:         snprintf(msg, sizeof(msg), "The card on line %d is an FR, but it appears before the GE; frequency setup should follow geometry.", i + 1);
L385:         snprintf(msg, sizeof(msg), "The card on line %d is an RP, but it appears before the GE; pattern requests should follow geometry.", i + 1);
L390:         snprintf(msg, sizeof(msg), "The card on line %d is a GN, but it appears before the GE; ground settings should follow geometry.", i + 1);
L395:         snprintf(msg, sizeof(msg), "The card on line %d is a GD, but it appears before the GE; ground parameters should follow geometry.", i + 1);
L400:         snprintf(msg, sizeof(msg), "The card on line %d is an EK, but it appears before the GE; EK should follow geometry to apply the extended kernel.", i + 1);
L410:         snprintf(msg, sizeof(msg), "The card on line %d is a GE but has more than one integer input.", i);
L416:         snprintf(msg, sizeof(msg), "The card on line %d is a GE with I1=%d, which is outside the typical range {-1,0,1,2}.", i, gei1);
L421:         snprintf(msg, sizeof(msg), "The card on line %d is a GE but has floating-point inputs, which are not expected.", i);
L431:         snprintf(msg, sizeof(msg), "The card on line %d is a TL but has fewer than 4 integer inputs (tag/segment locators).", i);
L437:         snprintf(msg, sizeof(msg), "The card on line %d is a TL with non-positive tag/segment locator(s).", i);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 443
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 383-448) ---
    383:       if (strcmp(code, "RP") == 0)
    384:       {
    385:         snprintf(msg, sizeof(msg), "The card on line %d is an RP, but it appears before the GE; pattern requests should follow geometry.", i + 1);
    386:         add_error(ctx, errors, msg, 0);
    387:       }
    388:       if (strcmp(code, "GN") == 0)
    389:       {
    390:         snprintf(msg, sizeof(msg), "The card on line %d is a GN, but it appears before the GE; ground settings should follow geometry.", i + 1);
    391:         add_error(ctx, errors, msg, 0);
    392:       }
    393:       if (strcmp(code, "GD") == 0)
    394:       {
    395:         snprintf(msg, sizeof(msg), "The card on line %d is a GD, but it appears before the GE; ground parameters should follow geometry.", i + 1);
    396:         add_error(ctx, errors, msg, 0);
    397:       }
    398:       if (strcmp(code, "EK") == 0)
    399:       {
    400:         snprintf(msg, sizeof(msg), "The card on line %d is an EK, but it appears before the GE; EK should follow geometry to apply the extended kernel.", i + 1);
    401:         add_error(ctx, errors, msg, 0);
    402:       }
    403:     }
    404: 
    405:     // GE: optional I1 in {-1,0,1,2}; no floats expected
    406:     if (strcmp(code, "GE") == 0)
    407:     {
    408:       if (deck->cards[i].ints_used > 1)
    409:       {
    410:         snprintf(msg, sizeof(msg), "The card on line %d is a GE but has more than one integer input.", i);
    411:         add_error(ctx, errors, msg, 0);
    412:       }
    413:       int gei1 = deck->cards[i].i[1];
    414:       if (!(gei1 == -1 || gei1 == 0 || gei1 == 1 || gei1 == 2))
    415:       {
    416:         snprintf(msg, sizeof(msg), "The card on line %d is a GE with I1=%d, which is outside the typical range {-1,0,1,2}.", i, gei1);
    417:         add_error(ctx, errors, msg, 0);
    418:       }
    419:       if (deck->cards[i].flts_used > 0)
    420:       {
    421:         snprintf(msg, sizeof(msg), "The card on line %d is a GE but has floating-point inputs, which are not expected.", i);
    422:         add_error(ctx, errors, msg, 0);
    423:       }
    424:     }
    425: 
    426:     // TL expects 4 integer locators (tags/segments) and Z0 in F1
    427:     if (strcmp(code, "TL") == 0)
    428:     {
    429:       if (deck->cards[i].ints_used < 4)
    430:       {
    431:         snprintf(msg, sizeof(msg), "The card on line %d is a TL but has fewer than 4 integer inputs (tag/segment locators).", i);
    432:         add_error(ctx, errors, msg, 0);
    433:       }
    434:       // basic sanity: locators positive
    435:       if (deck->cards[i].i[1] <= 0 || deck->cards[i].i[2] <= 0 || deck->cards[i].i[3] <= 0 || deck->cards[i].i[4] <= 0)
    436:       {
    437:         snprintf(msg, sizeof(msg), "The card on line %d is a TL with non-positive tag/segment locator(s).", i);
    438:         add_error(ctx, errors, msg, 0);
    439:       }
    440:       if (deck->cards[i].flts_used < 1)
    441:       {
    442:         snprintf(msg, sizeof(msg), "The card on line %d is a TL but has no characteristic impedance in F1.", i);
>   443:         add_error(ctx, errors, msg, 0);
    444:       }
    445:       else if (deck->cards[i].f[1] == 0.0)
    446:       {
    447:         snprintf(msg, sizeof(msg), "The card on line %d is a TL with Z0 = 0 in F1, which is invalid.", i);
    448:         add_error(ctx, errors, msg, 0);

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L266:         snprintf(msg, sizeof(msg),
L285:         snprintf(msg, sizeof(msg),
L319:         snprintf(msg, sizeof(msg), "The card on line %d is a GF, but we already saw one on card %d.", i, sawGF + 1);
L335:         snprintf(msg, sizeof(msg), "The card on line %d is an FR, but we already saw one on card %d.", i, sawFR + 1);
L355:         snprintf(msg, sizeof(msg), "The card on line %d is a %s, but it appears before the GE; control cards should follow geometry.", i + 1, code);
L365:         snprintf(msg, sizeof(msg), "The card on line %d is an EX, but it appears before the GE; excitations should follow geometry.", i + 1);
L370:         snprintf(msg, sizeof(msg), "The card on line %d is a TL, but it appears before the GE; transmission lines should follow geometry.", i + 1);
L375:         snprintf(msg, sizeof(msg), "The card on line %d is an LD, but it appears before the GE; loading should follow geometry.", i + 1);
L380:         snprintf(msg, sizeof(msg), "The card on line %d is an FR, but it appears before the GE; frequency setup should follow geometry.", i + 1);
L385:         snprintf(msg, sizeof(msg), "The card on line %d is an RP, but it appears before the GE; pattern requests should follow geometry.", i + 1);
L390:         snprintf(msg, sizeof(msg), "The card on line %d is a GN, but it appears before the GE; ground settings should follow geometry.", i + 1);
L395:         snprintf(msg, sizeof(msg), "The card on line %d is a GD, but it appears before the GE; ground parameters should follow geometry.", i + 1);
L400:         snprintf(msg, sizeof(msg), "The card on line %d is an EK, but it appears before the GE; EK should follow geometry to apply the extended kernel.", i + 1);
L410:         snprintf(msg, sizeof(msg), "The card on line %d is a GE but has more than one integer input.", i);
L416:         snprintf(msg, sizeof(msg), "The card on line %d is a GE with I1=%d, which is outside the typical range {-1,0,1,2}.", i, gei1);
L421:         snprintf(msg, sizeof(msg), "The card on line %d is a GE but has floating-point inputs, which are not expected.", i);
L431:         snprintf(msg, sizeof(msg), "The card on line %d is a TL but has fewer than 4 integer inputs (tag/segment locators).", i);
L437:         snprintf(msg, sizeof(msg), "The card on line %d is a TL with non-positive tag/segment locator(s).", i);
L442:         snprintf(msg, sizeof(msg), "The card on line %d is a TL but has no characteristic impedance in F1.", i);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 448
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 388-453) ---
    388:       if (strcmp(code, "GN") == 0)
    389:       {
    390:         snprintf(msg, sizeof(msg), "The card on line %d is a GN, but it appears before the GE; ground settings should follow geometry.", i + 1);
    391:         add_error(ctx, errors, msg, 0);
    392:       }
    393:       if (strcmp(code, "GD") == 0)
    394:       {
    395:         snprintf(msg, sizeof(msg), "The card on line %d is a GD, but it appears before the GE; ground parameters should follow geometry.", i + 1);
    396:         add_error(ctx, errors, msg, 0);
    397:       }
    398:       if (strcmp(code, "EK") == 0)
    399:       {
    400:         snprintf(msg, sizeof(msg), "The card on line %d is an EK, but it appears before the GE; EK should follow geometry to apply the extended kernel.", i + 1);
    401:         add_error(ctx, errors, msg, 0);
    402:       }
    403:     }
    404: 
    405:     // GE: optional I1 in {-1,0,1,2}; no floats expected
    406:     if (strcmp(code, "GE") == 0)
    407:     {
    408:       if (deck->cards[i].ints_used > 1)
    409:       {
    410:         snprintf(msg, sizeof(msg), "The card on line %d is a GE but has more than one integer input.", i);
    411:         add_error(ctx, errors, msg, 0);
    412:       }
    413:       int gei1 = deck->cards[i].i[1];
    414:       if (!(gei1 == -1 || gei1 == 0 || gei1 == 1 || gei1 == 2))
    415:       {
    416:         snprintf(msg, sizeof(msg), "The card on line %d is a GE with I1=%d, which is outside the typical range {-1,0,1,2}.", i, gei1);
    417:         add_error(ctx, errors, msg, 0);
    418:       }
    419:       if (deck->cards[i].flts_used > 0)
    420:       {
    421:         snprintf(msg, sizeof(msg), "The card on line %d is a GE but has floating-point inputs, which are not expected.", i);
    422:         add_error(ctx, errors, msg, 0);
    423:       }
    424:     }
    425: 
    426:     // TL expects 4 integer locators (tags/segments) and Z0 in F1
    427:     if (strcmp(code, "TL") == 0)
    428:     {
    429:       if (deck->cards[i].ints_used < 4)
    430:       {
    431:         snprintf(msg, sizeof(msg), "The card on line %d is a TL but has fewer than 4 integer inputs (tag/segment locators).", i);
    432:         add_error(ctx, errors, msg, 0);
    433:       }
    434:       // basic sanity: locators positive
    435:       if (deck->cards[i].i[1] <= 0 || deck->cards[i].i[2] <= 0 || deck->cards[i].i[3] <= 0 || deck->cards[i].i[4] <= 0)
    436:       {
    437:         snprintf(msg, sizeof(msg), "The card on line %d is a TL with non-positive tag/segment locator(s).", i);
    438:         add_error(ctx, errors, msg, 0);
    439:       }
    440:       if (deck->cards[i].flts_used < 1)
    441:       {
    442:         snprintf(msg, sizeof(msg), "The card on line %d is a TL but has no characteristic impedance in F1.", i);
    443:         add_error(ctx, errors, msg, 0);
    444:       }
    445:       else if (deck->cards[i].f[1] == 0.0)
    446:       {
    447:         snprintf(msg, sizeof(msg), "The card on line %d is a TL with Z0 = 0 in F1, which is invalid.", i);
>   448:         add_error(ctx, errors, msg, 0);
    449:       }
    450:       // record TL endpoints for segment-bounds validation later
    451:       if (deck->cards[i].ints_used >= 4 && tl_ref_count < (int)(sizeof(tl_refs) / sizeof(tl_refs[0])))
    452:       {
    453:         tl_refs[tl_ref_count].line = i + 1;

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L266:         snprintf(msg, sizeof(msg),
L285:         snprintf(msg, sizeof(msg),
L319:         snprintf(msg, sizeof(msg), "The card on line %d is a GF, but we already saw one on card %d.", i, sawGF + 1);
L335:         snprintf(msg, sizeof(msg), "The card on line %d is an FR, but we already saw one on card %d.", i, sawFR + 1);
L355:         snprintf(msg, sizeof(msg), "The card on line %d is a %s, but it appears before the GE; control cards should follow geometry.", i + 1, code);
L365:         snprintf(msg, sizeof(msg), "The card on line %d is an EX, but it appears before the GE; excitations should follow geometry.", i + 1);
L370:         snprintf(msg, sizeof(msg), "The card on line %d is a TL, but it appears before the GE; transmission lines should follow geometry.", i + 1);
L375:         snprintf(msg, sizeof(msg), "The card on line %d is an LD, but it appears before the GE; loading should follow geometry.", i + 1);
L380:         snprintf(msg, sizeof(msg), "The card on line %d is an FR, but it appears before the GE; frequency setup should follow geometry.", i + 1);
L385:         snprintf(msg, sizeof(msg), "The card on line %d is an RP, but it appears before the GE; pattern requests should follow geometry.", i + 1);
L390:         snprintf(msg, sizeof(msg), "The card on line %d is a GN, but it appears before the GE; ground settings should follow geometry.", i + 1);
L395:         snprintf(msg, sizeof(msg), "The card on line %d is a GD, but it appears before the GE; ground parameters should follow geometry.", i + 1);
L400:         snprintf(msg, sizeof(msg), "The card on line %d is an EK, but it appears before the GE; EK should follow geometry to apply the extended kernel.", i + 1);
L410:         snprintf(msg, sizeof(msg), "The card on line %d is a GE but has more than one integer input.", i);
L416:         snprintf(msg, sizeof(msg), "The card on line %d is a GE with I1=%d, which is outside the typical range {-1,0,1,2}.", i, gei1);
L421:         snprintf(msg, sizeof(msg), "The card on line %d is a GE but has floating-point inputs, which are not expected.", i);
L431:         snprintf(msg, sizeof(msg), "The card on line %d is a TL but has fewer than 4 integer inputs (tag/segment locators).", i);
L437:         snprintf(msg, sizeof(msg), "The card on line %d is a TL with non-positive tag/segment locator(s).", i);
L442:         snprintf(msg, sizeof(msg), "The card on line %d is a TL but has no characteristic impedance in F1.", i);
L447:         snprintf(msg, sizeof(msg), "The card on line %d is a TL with Z0 = 0 in F1, which is invalid.", i);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 468
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 408-473) ---
    408:       if (deck->cards[i].ints_used > 1)
    409:       {
    410:         snprintf(msg, sizeof(msg), "The card on line %d is a GE but has more than one integer input.", i);
    411:         add_error(ctx, errors, msg, 0);
    412:       }
    413:       int gei1 = deck->cards[i].i[1];
    414:       if (!(gei1 == -1 || gei1 == 0 || gei1 == 1 || gei1 == 2))
    415:       {
    416:         snprintf(msg, sizeof(msg), "The card on line %d is a GE with I1=%d, which is outside the typical range {-1,0,1,2}.", i, gei1);
    417:         add_error(ctx, errors, msg, 0);
    418:       }
    419:       if (deck->cards[i].flts_used > 0)
    420:       {
    421:         snprintf(msg, sizeof(msg), "The card on line %d is a GE but has floating-point inputs, which are not expected.", i);
    422:         add_error(ctx, errors, msg, 0);
    423:       }
    424:     }
    425: 
    426:     // TL expects 4 integer locators (tags/segments) and Z0 in F1
    427:     if (strcmp(code, "TL") == 0)
    428:     {
    429:       if (deck->cards[i].ints_used < 4)
    430:       {
    431:         snprintf(msg, sizeof(msg), "The card on line %d is a TL but has fewer than 4 integer inputs (tag/segment locators).", i);
    432:         add_error(ctx, errors, msg, 0);
    433:       }
    434:       // basic sanity: locators positive
    435:       if (deck->cards[i].i[1] <= 0 || deck->cards[i].i[2] <= 0 || deck->cards[i].i[3] <= 0 || deck->cards[i].i[4] <= 0)
    436:       {
    437:         snprintf(msg, sizeof(msg), "The card on line %d is a TL with non-positive tag/segment locator(s).", i);
    438:         add_error(ctx, errors, msg, 0);
    439:       }
    440:       if (deck->cards[i].flts_used < 1)
    441:       {
    442:         snprintf(msg, sizeof(msg), "The card on line %d is a TL but has no characteristic impedance in F1.", i);
    443:         add_error(ctx, errors, msg, 0);
    444:       }
    445:       else if (deck->cards[i].f[1] == 0.0)
    446:       {
    447:         snprintf(msg, sizeof(msg), "The card on line %d is a TL with Z0 = 0 in F1, which is invalid.", i);
    448:         add_error(ctx, errors, msg, 0);
    449:       }
    450:       // record TL endpoints for segment-bounds validation later
    451:       if (deck->cards[i].ints_used >= 4 && tl_ref_count < (int)(sizeof(tl_refs) / sizeof(tl_refs[0])))
    452:       {
    453:         tl_refs[tl_ref_count].line = i + 1;
    454:         tl_refs[tl_ref_count].tag1 = deck->cards[i].i[1];
    455:         tl_refs[tl_ref_count].seg1 = deck->cards[i].i[2];
    456:         tl_refs[tl_ref_count].tag2 = deck->cards[i].i[3];
    457:         tl_refs[tl_ref_count].seg2 = deck->cards[i].i[4];
    458:         tl_ref_count++;
    459:       }
    460:     }
    461: 
    462:     // EX, typical voltage source requires 4 integers and non-zero amplitude in F1
    463:     if (strcmp(code, "EX") == 0)
    464:     {
    465:       if (deck->cards[i].ints_used < 4)
    466:       {
    467:         snprintf(msg, sizeof(msg), "The card on line %d is an EX but has fewer than 4 integer inputs.", i);
>   468:         add_error(ctx, errors, msg, 0);
    469:       }
    470:       if (deck->cards[i].flts_used < 1)
    471:       {
    472:         snprintf(msg, sizeof(msg), "The card on line %d is an EX but has no amplitude in F1.", i);
    473:         add_error(ctx, errors, msg, 0);

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L285:         snprintf(msg, sizeof(msg),
L319:         snprintf(msg, sizeof(msg), "The card on line %d is a GF, but we already saw one on card %d.", i, sawGF + 1);
L335:         snprintf(msg, sizeof(msg), "The card on line %d is an FR, but we already saw one on card %d.", i, sawFR + 1);
L355:         snprintf(msg, sizeof(msg), "The card on line %d is a %s, but it appears before the GE; control cards should follow geometry.", i + 1, code);
L365:         snprintf(msg, sizeof(msg), "The card on line %d is an EX, but it appears before the GE; excitations should follow geometry.", i + 1);
L370:         snprintf(msg, sizeof(msg), "The card on line %d is a TL, but it appears before the GE; transmission lines should follow geometry.", i + 1);
L375:         snprintf(msg, sizeof(msg), "The card on line %d is an LD, but it appears before the GE; loading should follow geometry.", i + 1);
L380:         snprintf(msg, sizeof(msg), "The card on line %d is an FR, but it appears before the GE; frequency setup should follow geometry.", i + 1);
L385:         snprintf(msg, sizeof(msg), "The card on line %d is an RP, but it appears before the GE; pattern requests should follow geometry.", i + 1);
L390:         snprintf(msg, sizeof(msg), "The card on line %d is a GN, but it appears before the GE; ground settings should follow geometry.", i + 1);
L395:         snprintf(msg, sizeof(msg), "The card on line %d is a GD, but it appears before the GE; ground parameters should follow geometry.", i + 1);
L400:         snprintf(msg, sizeof(msg), "The card on line %d is an EK, but it appears before the GE; EK should follow geometry to apply the extended kernel.", i + 1);
L410:         snprintf(msg, sizeof(msg), "The card on line %d is a GE but has more than one integer input.", i);
L416:         snprintf(msg, sizeof(msg), "The card on line %d is a GE with I1=%d, which is outside the typical range {-1,0,1,2}.", i, gei1);
L421:         snprintf(msg, sizeof(msg), "The card on line %d is a GE but has floating-point inputs, which are not expected.", i);
L431:         snprintf(msg, sizeof(msg), "The card on line %d is a TL but has fewer than 4 integer inputs (tag/segment locators).", i);
L437:         snprintf(msg, sizeof(msg), "The card on line %d is a TL with non-positive tag/segment locator(s).", i);
L442:         snprintf(msg, sizeof(msg), "The card on line %d is a TL but has no characteristic impedance in F1.", i);
L447:         snprintf(msg, sizeof(msg), "The card on line %d is a TL with Z0 = 0 in F1, which is invalid.", i);
L467:         snprintf(msg, sizeof(msg), "The card on line %d is an EX but has fewer than 4 integer inputs.", i);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 473
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 413-478) ---
    413:       int gei1 = deck->cards[i].i[1];
    414:       if (!(gei1 == -1 || gei1 == 0 || gei1 == 1 || gei1 == 2))
    415:       {
    416:         snprintf(msg, sizeof(msg), "The card on line %d is a GE with I1=%d, which is outside the typical range {-1,0,1,2}.", i, gei1);
    417:         add_error(ctx, errors, msg, 0);
    418:       }
    419:       if (deck->cards[i].flts_used > 0)
    420:       {
    421:         snprintf(msg, sizeof(msg), "The card on line %d is a GE but has floating-point inputs, which are not expected.", i);
    422:         add_error(ctx, errors, msg, 0);
    423:       }
    424:     }
    425: 
    426:     // TL expects 4 integer locators (tags/segments) and Z0 in F1
    427:     if (strcmp(code, "TL") == 0)
    428:     {
    429:       if (deck->cards[i].ints_used < 4)
    430:       {
    431:         snprintf(msg, sizeof(msg), "The card on line %d is a TL but has fewer than 4 integer inputs (tag/segment locators).", i);
    432:         add_error(ctx, errors, msg, 0);
    433:       }
    434:       // basic sanity: locators positive
    435:       if (deck->cards[i].i[1] <= 0 || deck->cards[i].i[2] <= 0 || deck->cards[i].i[3] <= 0 || deck->cards[i].i[4] <= 0)
    436:       {
    437:         snprintf(msg, sizeof(msg), "The card on line %d is a TL with non-positive tag/segment locator(s).", i);
    438:         add_error(ctx, errors, msg, 0);
    439:       }
    440:       if (deck->cards[i].flts_used < 1)
    441:       {
    442:         snprintf(msg, sizeof(msg), "The card on line %d is a TL but has no characteristic impedance in F1.", i);
    443:         add_error(ctx, errors, msg, 0);
    444:       }
    445:       else if (deck->cards[i].f[1] == 0.0)
    446:       {
    447:         snprintf(msg, sizeof(msg), "The card on line %d is a TL with Z0 = 0 in F1, which is invalid.", i);
    448:         add_error(ctx, errors, msg, 0);
    449:       }
    450:       // record TL endpoints for segment-bounds validation later
    451:       if (deck->cards[i].ints_used >= 4 && tl_ref_count < (int)(sizeof(tl_refs) / sizeof(tl_refs[0])))
    452:       {
    453:         tl_refs[tl_ref_count].line = i + 1;
    454:         tl_refs[tl_ref_count].tag1 = deck->cards[i].i[1];
    455:         tl_refs[tl_ref_count].seg1 = deck->cards[i].i[2];
    456:         tl_refs[tl_ref_count].tag2 = deck->cards[i].i[3];
    457:         tl_refs[tl_ref_count].seg2 = deck->cards[i].i[4];
    458:         tl_ref_count++;
    459:       }
    460:     }
    461: 
    462:     // EX, typical voltage source requires 4 integers and non-zero amplitude in F1
    463:     if (strcmp(code, "EX") == 0)
    464:     {
    465:       if (deck->cards[i].ints_used < 4)
    466:       {
    467:         snprintf(msg, sizeof(msg), "The card on line %d is an EX but has fewer than 4 integer inputs.", i);
    468:         add_error(ctx, errors, msg, 0);
    469:       }
    470:       if (deck->cards[i].flts_used < 1)
    471:       {
    472:         snprintf(msg, sizeof(msg), "The card on line %d is an EX but has no amplitude in F1.", i);
>   473:         add_error(ctx, errors, msg, 0);
    474:       }
    475:       else if (deck->cards[i].f[1] == 0.0)
    476:       {
    477:         snprintf(msg, sizeof(msg), "The card on line %d is an EX with zero amplitude in F1.", i);
    478:         add_error(ctx, errors, msg, 0);

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L285:         snprintf(msg, sizeof(msg),
L319:         snprintf(msg, sizeof(msg), "The card on line %d is a GF, but we already saw one on card %d.", i, sawGF + 1);
L335:         snprintf(msg, sizeof(msg), "The card on line %d is an FR, but we already saw one on card %d.", i, sawFR + 1);
L355:         snprintf(msg, sizeof(msg), "The card on line %d is a %s, but it appears before the GE; control cards should follow geometry.", i + 1, code);
L365:         snprintf(msg, sizeof(msg), "The card on line %d is an EX, but it appears before the GE; excitations should follow geometry.", i + 1);
L370:         snprintf(msg, sizeof(msg), "The card on line %d is a TL, but it appears before the GE; transmission lines should follow geometry.", i + 1);
L375:         snprintf(msg, sizeof(msg), "The card on line %d is an LD, but it appears before the GE; loading should follow geometry.", i + 1);
L380:         snprintf(msg, sizeof(msg), "The card on line %d is an FR, but it appears before the GE; frequency setup should follow geometry.", i + 1);
L385:         snprintf(msg, sizeof(msg), "The card on line %d is an RP, but it appears before the GE; pattern requests should follow geometry.", i + 1);
L390:         snprintf(msg, sizeof(msg), "The card on line %d is a GN, but it appears before the GE; ground settings should follow geometry.", i + 1);
L395:         snprintf(msg, sizeof(msg), "The card on line %d is a GD, but it appears before the GE; ground parameters should follow geometry.", i + 1);
L400:         snprintf(msg, sizeof(msg), "The card on line %d is an EK, but it appears before the GE; EK should follow geometry to apply the extended kernel.", i + 1);
L410:         snprintf(msg, sizeof(msg), "The card on line %d is a GE but has more than one integer input.", i);
L416:         snprintf(msg, sizeof(msg), "The card on line %d is a GE with I1=%d, which is outside the typical range {-1,0,1,2}.", i, gei1);
L421:         snprintf(msg, sizeof(msg), "The card on line %d is a GE but has floating-point inputs, which are not expected.", i);
L431:         snprintf(msg, sizeof(msg), "The card on line %d is a TL but has fewer than 4 integer inputs (tag/segment locators).", i);
L437:         snprintf(msg, sizeof(msg), "The card on line %d is a TL with non-positive tag/segment locator(s).", i);
L442:         snprintf(msg, sizeof(msg), "The card on line %d is a TL but has no characteristic impedance in F1.", i);
L447:         snprintf(msg, sizeof(msg), "The card on line %d is a TL with Z0 = 0 in F1, which is invalid.", i);
L467:         snprintf(msg, sizeof(msg), "The card on line %d is an EX but has fewer than 4 integer inputs.", i);
L472:         snprintf(msg, sizeof(msg), "The card on line %d is an EX but has no amplitude in F1.", i);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 478
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 418-483) ---
    418:       }
    419:       if (deck->cards[i].flts_used > 0)
    420:       {
    421:         snprintf(msg, sizeof(msg), "The card on line %d is a GE but has floating-point inputs, which are not expected.", i);
    422:         add_error(ctx, errors, msg, 0);
    423:       }
    424:     }
    425: 
    426:     // TL expects 4 integer locators (tags/segments) and Z0 in F1
    427:     if (strcmp(code, "TL") == 0)
    428:     {
    429:       if (deck->cards[i].ints_used < 4)
    430:       {
    431:         snprintf(msg, sizeof(msg), "The card on line %d is a TL but has fewer than 4 integer inputs (tag/segment locators).", i);
    432:         add_error(ctx, errors, msg, 0);
    433:       }
    434:       // basic sanity: locators positive
    435:       if (deck->cards[i].i[1] <= 0 || deck->cards[i].i[2] <= 0 || deck->cards[i].i[3] <= 0 || deck->cards[i].i[4] <= 0)
    436:       {
    437:         snprintf(msg, sizeof(msg), "The card on line %d is a TL with non-positive tag/segment locator(s).", i);
    438:         add_error(ctx, errors, msg, 0);
    439:       }
    440:       if (deck->cards[i].flts_used < 1)
    441:       {
    442:         snprintf(msg, sizeof(msg), "The card on line %d is a TL but has no characteristic impedance in F1.", i);
    443:         add_error(ctx, errors, msg, 0);
    444:       }
    445:       else if (deck->cards[i].f[1] == 0.0)
    446:       {
    447:         snprintf(msg, sizeof(msg), "The card on line %d is a TL with Z0 = 0 in F1, which is invalid.", i);
    448:         add_error(ctx, errors, msg, 0);
    449:       }
    450:       // record TL endpoints for segment-bounds validation later
    451:       if (deck->cards[i].ints_used >= 4 && tl_ref_count < (int)(sizeof(tl_refs) / sizeof(tl_refs[0])))
    452:       {
    453:         tl_refs[tl_ref_count].line = i + 1;
    454:         tl_refs[tl_ref_count].tag1 = deck->cards[i].i[1];
    455:         tl_refs[tl_ref_count].seg1 = deck->cards[i].i[2];
    456:         tl_refs[tl_ref_count].tag2 = deck->cards[i].i[3];
    457:         tl_refs[tl_ref_count].seg2 = deck->cards[i].i[4];
    458:         tl_ref_count++;
    459:       }
    460:     }
    461: 
    462:     // EX, typical voltage source requires 4 integers and non-zero amplitude in F1
    463:     if (strcmp(code, "EX") == 0)
    464:     {
    465:       if (deck->cards[i].ints_used < 4)
    466:       {
    467:         snprintf(msg, sizeof(msg), "The card on line %d is an EX but has fewer than 4 integer inputs.", i);
    468:         add_error(ctx, errors, msg, 0);
    469:       }
    470:       if (deck->cards[i].flts_used < 1)
    471:       {
    472:         snprintf(msg, sizeof(msg), "The card on line %d is an EX but has no amplitude in F1.", i);
    473:         add_error(ctx, errors, msg, 0);
    474:       }
    475:       else if (deck->cards[i].f[1] == 0.0)
    476:       {
    477:         snprintf(msg, sizeof(msg), "The card on line %d is an EX with zero amplitude in F1.", i);
>   478:         add_error(ctx, errors, msg, 0);
    479:       }
    480:       // basic locator sanity: tag and segment positive
    481:       if (deck->cards[i].ints_used >= 3)
    482:       {
    483:         if (deck->cards[i].i[2] <= 0 || deck->cards[i].i[3] <= 0)

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L285:         snprintf(msg, sizeof(msg),
L319:         snprintf(msg, sizeof(msg), "The card on line %d is a GF, but we already saw one on card %d.", i, sawGF + 1);
L335:         snprintf(msg, sizeof(msg), "The card on line %d is an FR, but we already saw one on card %d.", i, sawFR + 1);
L355:         snprintf(msg, sizeof(msg), "The card on line %d is a %s, but it appears before the GE; control cards should follow geometry.", i + 1, code);
L365:         snprintf(msg, sizeof(msg), "The card on line %d is an EX, but it appears before the GE; excitations should follow geometry.", i + 1);
L370:         snprintf(msg, sizeof(msg), "The card on line %d is a TL, but it appears before the GE; transmission lines should follow geometry.", i + 1);
L375:         snprintf(msg, sizeof(msg), "The card on line %d is an LD, but it appears before the GE; loading should follow geometry.", i + 1);
L380:         snprintf(msg, sizeof(msg), "The card on line %d is an FR, but it appears before the GE; frequency setup should follow geometry.", i + 1);
L385:         snprintf(msg, sizeof(msg), "The card on line %d is an RP, but it appears before the GE; pattern requests should follow geometry.", i + 1);
L390:         snprintf(msg, sizeof(msg), "The card on line %d is a GN, but it appears before the GE; ground settings should follow geometry.", i + 1);
L395:         snprintf(msg, sizeof(msg), "The card on line %d is a GD, but it appears before the GE; ground parameters should follow geometry.", i + 1);
L400:         snprintf(msg, sizeof(msg), "The card on line %d is an EK, but it appears before the GE; EK should follow geometry to apply the extended kernel.", i + 1);
L410:         snprintf(msg, sizeof(msg), "The card on line %d is a GE but has more than one integer input.", i);
L416:         snprintf(msg, sizeof(msg), "The card on line %d is a GE with I1=%d, which is outside the typical range {-1,0,1,2}.", i, gei1);
L421:         snprintf(msg, sizeof(msg), "The card on line %d is a GE but has floating-point inputs, which are not expected.", i);
L431:         snprintf(msg, sizeof(msg), "The card on line %d is a TL but has fewer than 4 integer inputs (tag/segment locators).", i);
L437:         snprintf(msg, sizeof(msg), "The card on line %d is a TL with non-positive tag/segment locator(s).", i);
L442:         snprintf(msg, sizeof(msg), "The card on line %d is a TL but has no characteristic impedance in F1.", i);
L447:         snprintf(msg, sizeof(msg), "The card on line %d is a TL with Z0 = 0 in F1, which is invalid.", i);
L467:         snprintf(msg, sizeof(msg), "The card on line %d is an EX but has fewer than 4 integer inputs.", i);
L472:         snprintf(msg, sizeof(msg), "The card on line %d is an EX but has no amplitude in F1.", i);
L477:         snprintf(msg, sizeof(msg), "The card on line %d is an EX with zero amplitude in F1.", i);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 486
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 426-491) ---
    426:     // TL expects 4 integer locators (tags/segments) and Z0 in F1
    427:     if (strcmp(code, "TL") == 0)
    428:     {
    429:       if (deck->cards[i].ints_used < 4)
    430:       {
    431:         snprintf(msg, sizeof(msg), "The card on line %d is a TL but has fewer than 4 integer inputs (tag/segment locators).", i);
    432:         add_error(ctx, errors, msg, 0);
    433:       }
    434:       // basic sanity: locators positive
    435:       if (deck->cards[i].i[1] <= 0 || deck->cards[i].i[2] <= 0 || deck->cards[i].i[3] <= 0 || deck->cards[i].i[4] <= 0)
    436:       {
    437:         snprintf(msg, sizeof(msg), "The card on line %d is a TL with non-positive tag/segment locator(s).", i);
    438:         add_error(ctx, errors, msg, 0);
    439:       }
    440:       if (deck->cards[i].flts_used < 1)
    441:       {
    442:         snprintf(msg, sizeof(msg), "The card on line %d is a TL but has no characteristic impedance in F1.", i);
    443:         add_error(ctx, errors, msg, 0);
    444:       }
    445:       else if (deck->cards[i].f[1] == 0.0)
    446:       {
    447:         snprintf(msg, sizeof(msg), "The card on line %d is a TL with Z0 = 0 in F1, which is invalid.", i);
    448:         add_error(ctx, errors, msg, 0);
    449:       }
    450:       // record TL endpoints for segment-bounds validation later
    451:       if (deck->cards[i].ints_used >= 4 && tl_ref_count < (int)(sizeof(tl_refs) / sizeof(tl_refs[0])))
    452:       {
    453:         tl_refs[tl_ref_count].line = i + 1;
    454:         tl_refs[tl_ref_count].tag1 = deck->cards[i].i[1];
    455:         tl_refs[tl_ref_count].seg1 = deck->cards[i].i[2];
    456:         tl_refs[tl_ref_count].tag2 = deck->cards[i].i[3];
    457:         tl_refs[tl_ref_count].seg2 = deck->cards[i].i[4];
    458:         tl_ref_count++;
    459:       }
    460:     }
    461: 
    462:     // EX, typical voltage source requires 4 integers and non-zero amplitude in F1
    463:     if (strcmp(code, "EX") == 0)
    464:     {
    465:       if (deck->cards[i].ints_used < 4)
    466:       {
    467:         snprintf(msg, sizeof(msg), "The card on line %d is an EX but has fewer than 4 integer inputs.", i);
    468:         add_error(ctx, errors, msg, 0);
    469:       }
    470:       if (deck->cards[i].flts_used < 1)
    471:       {
    472:         snprintf(msg, sizeof(msg), "The card on line %d is an EX but has no amplitude in F1.", i);
    473:         add_error(ctx, errors, msg, 0);
    474:       }
    475:       else if (deck->cards[i].f[1] == 0.0)
    476:       {
    477:         snprintf(msg, sizeof(msg), "The card on line %d is an EX with zero amplitude in F1.", i);
    478:         add_error(ctx, errors, msg, 0);
    479:       }
    480:       // basic locator sanity: tag and segment positive
    481:       if (deck->cards[i].ints_used >= 3)
    482:       {
    483:         if (deck->cards[i].i[2] <= 0 || deck->cards[i].i[3] <= 0)
    484:         {
    485:           snprintf(msg, sizeof(msg), "The card on line %d is an EX with non-positive tag or segment locator.", i);
>   486:           add_error(ctx, errors, msg, 0);
    487:         }
    488:       }
    489:       // check for unsupported EX types
    490:       if (deck->cards[i].ints_used >= 1)
    491:       {

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L319:         snprintf(msg, sizeof(msg), "The card on line %d is a GF, but we already saw one on card %d.", i, sawGF + 1);
L335:         snprintf(msg, sizeof(msg), "The card on line %d is an FR, but we already saw one on card %d.", i, sawFR + 1);
L355:         snprintf(msg, sizeof(msg), "The card on line %d is a %s, but it appears before the GE; control cards should follow geometry.", i + 1, code);
L365:         snprintf(msg, sizeof(msg), "The card on line %d is an EX, but it appears before the GE; excitations should follow geometry.", i + 1);
L370:         snprintf(msg, sizeof(msg), "The card on line %d is a TL, but it appears before the GE; transmission lines should follow geometry.", i + 1);
L375:         snprintf(msg, sizeof(msg), "The card on line %d is an LD, but it appears before the GE; loading should follow geometry.", i + 1);
L380:         snprintf(msg, sizeof(msg), "The card on line %d is an FR, but it appears before the GE; frequency setup should follow geometry.", i + 1);
L385:         snprintf(msg, sizeof(msg), "The card on line %d is an RP, but it appears before the GE; pattern requests should follow geometry.", i + 1);
L390:         snprintf(msg, sizeof(msg), "The card on line %d is a GN, but it appears before the GE; ground settings should follow geometry.", i + 1);
L395:         snprintf(msg, sizeof(msg), "The card on line %d is a GD, but it appears before the GE; ground parameters should follow geometry.", i + 1);
L400:         snprintf(msg, sizeof(msg), "The card on line %d is an EK, but it appears before the GE; EK should follow geometry to apply the extended kernel.", i + 1);
L410:         snprintf(msg, sizeof(msg), "The card on line %d is a GE but has more than one integer input.", i);
L416:         snprintf(msg, sizeof(msg), "The card on line %d is a GE with I1=%d, which is outside the typical range {-1,0,1,2}.", i, gei1);
L421:         snprintf(msg, sizeof(msg), "The card on line %d is a GE but has floating-point inputs, which are not expected.", i);
L431:         snprintf(msg, sizeof(msg), "The card on line %d is a TL but has fewer than 4 integer inputs (tag/segment locators).", i);
L437:         snprintf(msg, sizeof(msg), "The card on line %d is a TL with non-positive tag/segment locator(s).", i);
L442:         snprintf(msg, sizeof(msg), "The card on line %d is a TL but has no characteristic impedance in F1.", i);
L447:         snprintf(msg, sizeof(msg), "The card on line %d is a TL with Z0 = 0 in F1, which is invalid.", i);
L467:         snprintf(msg, sizeof(msg), "The card on line %d is an EX but has fewer than 4 integer inputs.", i);
L472:         snprintf(msg, sizeof(msg), "The card on line %d is an EX but has no amplitude in F1.", i);
L477:         snprintf(msg, sizeof(msg), "The card on line %d is an EX with zero amplitude in F1.", i);
L485:           snprintf(msg, sizeof(msg), "The card on line %d is an EX with non-positive tag or segment locator.", i);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 496
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 436-501) ---
    436:       {
    437:         snprintf(msg, sizeof(msg), "The card on line %d is a TL with non-positive tag/segment locator(s).", i);
    438:         add_error(ctx, errors, msg, 0);
    439:       }
    440:       if (deck->cards[i].flts_used < 1)
    441:       {
    442:         snprintf(msg, sizeof(msg), "The card on line %d is a TL but has no characteristic impedance in F1.", i);
    443:         add_error(ctx, errors, msg, 0);
    444:       }
    445:       else if (deck->cards[i].f[1] == 0.0)
    446:       {
    447:         snprintf(msg, sizeof(msg), "The card on line %d is a TL with Z0 = 0 in F1, which is invalid.", i);
    448:         add_error(ctx, errors, msg, 0);
    449:       }
    450:       // record TL endpoints for segment-bounds validation later
    451:       if (deck->cards[i].ints_used >= 4 && tl_ref_count < (int)(sizeof(tl_refs) / sizeof(tl_refs[0])))
    452:       {
    453:         tl_refs[tl_ref_count].line = i + 1;
    454:         tl_refs[tl_ref_count].tag1 = deck->cards[i].i[1];
    455:         tl_refs[tl_ref_count].seg1 = deck->cards[i].i[2];
    456:         tl_refs[tl_ref_count].tag2 = deck->cards[i].i[3];
    457:         tl_refs[tl_ref_count].seg2 = deck->cards[i].i[4];
    458:         tl_ref_count++;
    459:       }
    460:     }
    461: 
    462:     // EX, typical voltage source requires 4 integers and non-zero amplitude in F1
    463:     if (strcmp(code, "EX") == 0)
    464:     {
    465:       if (deck->cards[i].ints_used < 4)
    466:       {
    467:         snprintf(msg, sizeof(msg), "The card on line %d is an EX but has fewer than 4 integer inputs.", i);
    468:         add_error(ctx, errors, msg, 0);
    469:       }
    470:       if (deck->cards[i].flts_used < 1)
    471:       {
    472:         snprintf(msg, sizeof(msg), "The card on line %d is an EX but has no amplitude in F1.", i);
    473:         add_error(ctx, errors, msg, 0);
    474:       }
    475:       else if (deck->cards[i].f[1] == 0.0)
    476:       {
    477:         snprintf(msg, sizeof(msg), "The card on line %d is an EX with zero amplitude in F1.", i);
    478:         add_error(ctx, errors, msg, 0);
    479:       }
    480:       // basic locator sanity: tag and segment positive
    481:       if (deck->cards[i].ints_used >= 3)
    482:       {
    483:         if (deck->cards[i].i[2] <= 0 || deck->cards[i].i[3] <= 0)
    484:         {
    485:           snprintf(msg, sizeof(msg), "The card on line %d is an EX with non-positive tag or segment locator.", i);
    486:           add_error(ctx, errors, msg, 0);
    487:         }
    488:       }
    489:       // check for unsupported EX types
    490:       if (deck->cards[i].ints_used >= 1)
    491:       {
    492:         int ex_type = deck->cards[i].i[1];
    493:         if (ex_type == 6 || ex_type == 7)
    494:         {
    495:           snprintf(msg, sizeof(msg), "The card on line %d is an EX with type %d, which is not supported by OpenNEC.", i, ex_type);
>   496:           add_error(ctx, errors, msg, 0);
    497:         }
    498:       }
    499:       // record for open-end placement validation later
    500:       if (deck->cards[i].ints_used >= 3)
    501:       {

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L319:         snprintf(msg, sizeof(msg), "The card on line %d is a GF, but we already saw one on card %d.", i, sawGF + 1);
L335:         snprintf(msg, sizeof(msg), "The card on line %d is an FR, but we already saw one on card %d.", i, sawFR + 1);
L355:         snprintf(msg, sizeof(msg), "The card on line %d is a %s, but it appears before the GE; control cards should follow geometry.", i + 1, code);
L365:         snprintf(msg, sizeof(msg), "The card on line %d is an EX, but it appears before the GE; excitations should follow geometry.", i + 1);
L370:         snprintf(msg, sizeof(msg), "The card on line %d is a TL, but it appears before the GE; transmission lines should follow geometry.", i + 1);
L375:         snprintf(msg, sizeof(msg), "The card on line %d is an LD, but it appears before the GE; loading should follow geometry.", i + 1);
L380:         snprintf(msg, sizeof(msg), "The card on line %d is an FR, but it appears before the GE; frequency setup should follow geometry.", i + 1);
L385:         snprintf(msg, sizeof(msg), "The card on line %d is an RP, but it appears before the GE; pattern requests should follow geometry.", i + 1);
L390:         snprintf(msg, sizeof(msg), "The card on line %d is a GN, but it appears before the GE; ground settings should follow geometry.", i + 1);
L395:         snprintf(msg, sizeof(msg), "The card on line %d is a GD, but it appears before the GE; ground parameters should follow geometry.", i + 1);
L400:         snprintf(msg, sizeof(msg), "The card on line %d is an EK, but it appears before the GE; EK should follow geometry to apply the extended kernel.", i + 1);
L410:         snprintf(msg, sizeof(msg), "The card on line %d is a GE but has more than one integer input.", i);
L416:         snprintf(msg, sizeof(msg), "The card on line %d is a GE with I1=%d, which is outside the typical range {-1,0,1,2}.", i, gei1);
L421:         snprintf(msg, sizeof(msg), "The card on line %d is a GE but has floating-point inputs, which are not expected.", i);
L431:         snprintf(msg, sizeof(msg), "The card on line %d is a TL but has fewer than 4 integer inputs (tag/segment locators).", i);
L437:         snprintf(msg, sizeof(msg), "The card on line %d is a TL with non-positive tag/segment locator(s).", i);
L442:         snprintf(msg, sizeof(msg), "The card on line %d is a TL but has no characteristic impedance in F1.", i);
L447:         snprintf(msg, sizeof(msg), "The card on line %d is a TL with Z0 = 0 in F1, which is invalid.", i);
L467:         snprintf(msg, sizeof(msg), "The card on line %d is an EX but has fewer than 4 integer inputs.", i);
L472:         snprintf(msg, sizeof(msg), "The card on line %d is an EX but has no amplitude in F1.", i);
L477:         snprintf(msg, sizeof(msg), "The card on line %d is an EX with zero amplitude in F1.", i);
L485:           snprintf(msg, sizeof(msg), "The card on line %d is an EX with non-positive tag or segment locator.", i);
L495:           snprintf(msg, sizeof(msg), "The card on line %d is an EX with type %d, which is not supported by OpenNEC.", i, ex_type);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 519
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 459-524) ---
    459:       }
    460:     }
    461: 
    462:     // EX, typical voltage source requires 4 integers and non-zero amplitude in F1
    463:     if (strcmp(code, "EX") == 0)
    464:     {
    465:       if (deck->cards[i].ints_used < 4)
    466:       {
    467:         snprintf(msg, sizeof(msg), "The card on line %d is an EX but has fewer than 4 integer inputs.", i);
    468:         add_error(ctx, errors, msg, 0);
    469:       }
    470:       if (deck->cards[i].flts_used < 1)
    471:       {
    472:         snprintf(msg, sizeof(msg), "The card on line %d is an EX but has no amplitude in F1.", i);
    473:         add_error(ctx, errors, msg, 0);
    474:       }
    475:       else if (deck->cards[i].f[1] == 0.0)
    476:       {
    477:         snprintf(msg, sizeof(msg), "The card on line %d is an EX with zero amplitude in F1.", i);
    478:         add_error(ctx, errors, msg, 0);
    479:       }
    480:       // basic locator sanity: tag and segment positive
    481:       if (deck->cards[i].ints_used >= 3)
    482:       {
    483:         if (deck->cards[i].i[2] <= 0 || deck->cards[i].i[3] <= 0)
    484:         {
    485:           snprintf(msg, sizeof(msg), "The card on line %d is an EX with non-positive tag or segment locator.", i);
    486:           add_error(ctx, errors, msg, 0);
    487:         }
    488:       }
    489:       // check for unsupported EX types
    490:       if (deck->cards[i].ints_used >= 1)
    491:       {
    492:         int ex_type = deck->cards[i].i[1];
    493:         if (ex_type == 6 || ex_type == 7)
    494:         {
    495:           snprintf(msg, sizeof(msg), "The card on line %d is an EX with type %d, which is not supported by OpenNEC.", i, ex_type);
    496:           add_error(ctx, errors, msg, 0);
    497:         }
    498:       }
    499:       // record for open-end placement validation later
    500:       if (deck->cards[i].ints_used >= 3)
    501:       {
    502:         ref_info_t r = {
    503:             .line = i + 1,
    504:             .tag = deck->cards[i].i[2],
    505:             .segStart = deck->cards[i].i[3],
    506:             .segEnd = 0};
    507:         if (ex_ref_count < (int)(sizeof(ex_refs) / sizeof(ex_refs[0])))
    508:           ex_ref_count++;
    509:         ex_refs[ex_ref_count - 1] = r;
    510:       }
    511:     }
    512: 
    513:     // GN: minimal check — at least the ground type integer should be present
    514:     if (strcmp(code, "GN") == 0)
    515:     {
    516:       if (deck->cards[i].ints_used < 1)
    517:       {
    518:         snprintf(msg, sizeof(msg), "The card on line %d is a GN but has no integer ground type specified.", i);
>   519:         add_error(ctx, errors, msg, 0);
    520:       }
    521:     }
    522: 
    523:     // LD: loading — require type, tag, segment and at least one non-zero value
    524:     if (strcmp(code, "LD") == 0)

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L319:         snprintf(msg, sizeof(msg), "The card on line %d is a GF, but we already saw one on card %d.", i, sawGF + 1);
L335:         snprintf(msg, sizeof(msg), "The card on line %d is an FR, but we already saw one on card %d.", i, sawFR + 1);
L355:         snprintf(msg, sizeof(msg), "The card on line %d is a %s, but it appears before the GE; control cards should follow geometry.", i + 1, code);
L365:         snprintf(msg, sizeof(msg), "The card on line %d is an EX, but it appears before the GE; excitations should follow geometry.", i + 1);
L370:         snprintf(msg, sizeof(msg), "The card on line %d is a TL, but it appears before the GE; transmission lines should follow geometry.", i + 1);
L375:         snprintf(msg, sizeof(msg), "The card on line %d is an LD, but it appears before the GE; loading should follow geometry.", i + 1);
L380:         snprintf(msg, sizeof(msg), "The card on line %d is an FR, but it appears before the GE; frequency setup should follow geometry.", i + 1);
L385:         snprintf(msg, sizeof(msg), "The card on line %d is an RP, but it appears before the GE; pattern requests should follow geometry.", i + 1);
L390:         snprintf(msg, sizeof(msg), "The card on line %d is a GN, but it appears before the GE; ground settings should follow geometry.", i + 1);
L395:         snprintf(msg, sizeof(msg), "The card on line %d is a GD, but it appears before the GE; ground parameters should follow geometry.", i + 1);
L400:         snprintf(msg, sizeof(msg), "The card on line %d is an EK, but it appears before the GE; EK should follow geometry to apply the extended kernel.", i + 1);
L410:         snprintf(msg, sizeof(msg), "The card on line %d is a GE but has more than one integer input.", i);
L416:         snprintf(msg, sizeof(msg), "The card on line %d is a GE with I1=%d, which is outside the typical range {-1,0,1,2}.", i, gei1);
L421:         snprintf(msg, sizeof(msg), "The card on line %d is a GE but has floating-point inputs, which are not expected.", i);
L431:         snprintf(msg, sizeof(msg), "The card on line %d is a TL but has fewer than 4 integer inputs (tag/segment locators).", i);
L437:         snprintf(msg, sizeof(msg), "The card on line %d is a TL with non-positive tag/segment locator(s).", i);
L442:         snprintf(msg, sizeof(msg), "The card on line %d is a TL but has no characteristic impedance in F1.", i);
L447:         snprintf(msg, sizeof(msg), "The card on line %d is a TL with Z0 = 0 in F1, which is invalid.", i);
L467:         snprintf(msg, sizeof(msg), "The card on line %d is an EX but has fewer than 4 integer inputs.", i);
L472:         snprintf(msg, sizeof(msg), "The card on line %d is an EX but has no amplitude in F1.", i);
L477:         snprintf(msg, sizeof(msg), "The card on line %d is an EX with zero amplitude in F1.", i);
L485:           snprintf(msg, sizeof(msg), "The card on line %d is an EX with non-positive tag or segment locator.", i);
L495:           snprintf(msg, sizeof(msg), "The card on line %d is an EX with type %d, which is not supported by OpenNEC.", i, ex_type);
L518:         snprintf(msg, sizeof(msg), "The card on line %d is a GN but has no integer ground type specified.", i);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 530
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 470-535) ---
    470:       if (deck->cards[i].flts_used < 1)
    471:       {
    472:         snprintf(msg, sizeof(msg), "The card on line %d is an EX but has no amplitude in F1.", i);
    473:         add_error(ctx, errors, msg, 0);
    474:       }
    475:       else if (deck->cards[i].f[1] == 0.0)
    476:       {
    477:         snprintf(msg, sizeof(msg), "The card on line %d is an EX with zero amplitude in F1.", i);
    478:         add_error(ctx, errors, msg, 0);
    479:       }
    480:       // basic locator sanity: tag and segment positive
    481:       if (deck->cards[i].ints_used >= 3)
    482:       {
    483:         if (deck->cards[i].i[2] <= 0 || deck->cards[i].i[3] <= 0)
    484:         {
    485:           snprintf(msg, sizeof(msg), "The card on line %d is an EX with non-positive tag or segment locator.", i);
    486:           add_error(ctx, errors, msg, 0);
    487:         }
    488:       }
    489:       // check for unsupported EX types
    490:       if (deck->cards[i].ints_used >= 1)
    491:       {
    492:         int ex_type = deck->cards[i].i[1];
    493:         if (ex_type == 6 || ex_type == 7)
    494:         {
    495:           snprintf(msg, sizeof(msg), "The card on line %d is an EX with type %d, which is not supported by OpenNEC.", i, ex_type);
    496:           add_error(ctx, errors, msg, 0);
    497:         }
    498:       }
    499:       // record for open-end placement validation later
    500:       if (deck->cards[i].ints_used >= 3)
    501:       {
    502:         ref_info_t r = {
    503:             .line = i + 1,
    504:             .tag = deck->cards[i].i[2],
    505:             .segStart = deck->cards[i].i[3],
    506:             .segEnd = 0};
    507:         if (ex_ref_count < (int)(sizeof(ex_refs) / sizeof(ex_refs[0])))
    508:           ex_ref_count++;
    509:         ex_refs[ex_ref_count - 1] = r;
    510:       }
    511:     }
    512: 
    513:     // GN: minimal check — at least the ground type integer should be present
    514:     if (strcmp(code, "GN") == 0)
    515:     {
    516:       if (deck->cards[i].ints_used < 1)
    517:       {
    518:         snprintf(msg, sizeof(msg), "The card on line %d is a GN but has no integer ground type specified.", i);
    519:         add_error(ctx, errors, msg, 0);
    520:       }
    521:     }
    522: 
    523:     // LD: loading — require type, tag, segment and at least one non-zero value
    524:     if (strcmp(code, "LD") == 0)
    525:     {
    526:       // at minimum we need I1 (type), I2 (tag), I3 (segment)
    527:       if (deck->cards[i].ints_used < 3)
    528:       {
    529:         snprintf(msg, sizeof(msg), "The card on line %d is an LD but has fewer than 3 integer inputs (type, tag, segment).", i);
>   530:         add_error(ctx, errors, msg, 0);
    531:       }
    532:       else
    533:       {
    534:         int type = deck->cards[i].i[1];
    535:         int tag = deck->cards[i].i[2];

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L335:         snprintf(msg, sizeof(msg), "The card on line %d is an FR, but we already saw one on card %d.", i, sawFR + 1);
L355:         snprintf(msg, sizeof(msg), "The card on line %d is a %s, but it appears before the GE; control cards should follow geometry.", i + 1, code);
L365:         snprintf(msg, sizeof(msg), "The card on line %d is an EX, but it appears before the GE; excitations should follow geometry.", i + 1);
L370:         snprintf(msg, sizeof(msg), "The card on line %d is a TL, but it appears before the GE; transmission lines should follow geometry.", i + 1);
L375:         snprintf(msg, sizeof(msg), "The card on line %d is an LD, but it appears before the GE; loading should follow geometry.", i + 1);
L380:         snprintf(msg, sizeof(msg), "The card on line %d is an FR, but it appears before the GE; frequency setup should follow geometry.", i + 1);
L385:         snprintf(msg, sizeof(msg), "The card on line %d is an RP, but it appears before the GE; pattern requests should follow geometry.", i + 1);
L390:         snprintf(msg, sizeof(msg), "The card on line %d is a GN, but it appears before the GE; ground settings should follow geometry.", i + 1);
L395:         snprintf(msg, sizeof(msg), "The card on line %d is a GD, but it appears before the GE; ground parameters should follow geometry.", i + 1);
L400:         snprintf(msg, sizeof(msg), "The card on line %d is an EK, but it appears before the GE; EK should follow geometry to apply the extended kernel.", i + 1);
L410:         snprintf(msg, sizeof(msg), "The card on line %d is a GE but has more than one integer input.", i);
L416:         snprintf(msg, sizeof(msg), "The card on line %d is a GE with I1=%d, which is outside the typical range {-1,0,1,2}.", i, gei1);
L421:         snprintf(msg, sizeof(msg), "The card on line %d is a GE but has floating-point inputs, which are not expected.", i);
L431:         snprintf(msg, sizeof(msg), "The card on line %d is a TL but has fewer than 4 integer inputs (tag/segment locators).", i);
L437:         snprintf(msg, sizeof(msg), "The card on line %d is a TL with non-positive tag/segment locator(s).", i);
L442:         snprintf(msg, sizeof(msg), "The card on line %d is a TL but has no characteristic impedance in F1.", i);
L447:         snprintf(msg, sizeof(msg), "The card on line %d is a TL with Z0 = 0 in F1, which is invalid.", i);
L467:         snprintf(msg, sizeof(msg), "The card on line %d is an EX but has fewer than 4 integer inputs.", i);
L472:         snprintf(msg, sizeof(msg), "The card on line %d is an EX but has no amplitude in F1.", i);
L477:         snprintf(msg, sizeof(msg), "The card on line %d is an EX with zero amplitude in F1.", i);
L485:           snprintf(msg, sizeof(msg), "The card on line %d is an EX with non-positive tag or segment locator.", i);
L495:           snprintf(msg, sizeof(msg), "The card on line %d is an EX with type %d, which is not supported by OpenNEC.", i, ex_type);
L518:         snprintf(msg, sizeof(msg), "The card on line %d is a GN but has no integer ground type specified.", i);
L529:         snprintf(msg, sizeof(msg), "The card on line %d is an LD but has fewer than 3 integer inputs (type, tag, segment).", i);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 541
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 481-546) ---
    481:       if (deck->cards[i].ints_used >= 3)
    482:       {
    483:         if (deck->cards[i].i[2] <= 0 || deck->cards[i].i[3] <= 0)
    484:         {
    485:           snprintf(msg, sizeof(msg), "The card on line %d is an EX with non-positive tag or segment locator.", i);
    486:           add_error(ctx, errors, msg, 0);
    487:         }
    488:       }
    489:       // check for unsupported EX types
    490:       if (deck->cards[i].ints_used >= 1)
    491:       {
    492:         int ex_type = deck->cards[i].i[1];
    493:         if (ex_type == 6 || ex_type == 7)
    494:         {
    495:           snprintf(msg, sizeof(msg), "The card on line %d is an EX with type %d, which is not supported by OpenNEC.", i, ex_type);
    496:           add_error(ctx, errors, msg, 0);
    497:         }
    498:       }
    499:       // record for open-end placement validation later
    500:       if (deck->cards[i].ints_used >= 3)
    501:       {
    502:         ref_info_t r = {
    503:             .line = i + 1,
    504:             .tag = deck->cards[i].i[2],
    505:             .segStart = deck->cards[i].i[3],
    506:             .segEnd = 0};
    507:         if (ex_ref_count < (int)(sizeof(ex_refs) / sizeof(ex_refs[0])))
    508:           ex_ref_count++;
    509:         ex_refs[ex_ref_count - 1] = r;
    510:       }
    511:     }
    512: 
    513:     // GN: minimal check — at least the ground type integer should be present
    514:     if (strcmp(code, "GN") == 0)
    515:     {
    516:       if (deck->cards[i].ints_used < 1)
    517:       {
    518:         snprintf(msg, sizeof(msg), "The card on line %d is a GN but has no integer ground type specified.", i);
    519:         add_error(ctx, errors, msg, 0);
    520:       }
    521:     }
    522: 
    523:     // LD: loading — require type, tag, segment and at least one non-zero value
    524:     if (strcmp(code, "LD") == 0)
    525:     {
    526:       // at minimum we need I1 (type), I2 (tag), I3 (segment)
    527:       if (deck->cards[i].ints_used < 3)
    528:       {
    529:         snprintf(msg, sizeof(msg), "The card on line %d is an LD but has fewer than 3 integer inputs (type, tag, segment).", i);
    530:         add_error(ctx, errors, msg, 0);
    531:       }
    532:       else
    533:       {
    534:         int type = deck->cards[i].i[1];
    535:         int tag = deck->cards[i].i[2];
    536:         int seg1 = deck->cards[i].i[3];
    537:         int seg2 = deck->cards[i].i[4];
    538:         if (type < -1)
    539:         {
    540:           snprintf(msg, sizeof(msg), "The card on line %d is an LD with unexpected type I1=%d.", i, type);
>   541:           add_error(ctx, errors, msg, 0);
    542:         }
    543:         if (tag <= 0 || seg1 <= 0)
    544:         {
    545:           snprintf(msg, sizeof(msg), "The card on line %d is an LD with non-positive tag or segment locator.", i);
    546:           add_error(ctx, errors, msg, 0);

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L355:         snprintf(msg, sizeof(msg), "The card on line %d is a %s, but it appears before the GE; control cards should follow geometry.", i + 1, code);
L365:         snprintf(msg, sizeof(msg), "The card on line %d is an EX, but it appears before the GE; excitations should follow geometry.", i + 1);
L370:         snprintf(msg, sizeof(msg), "The card on line %d is a TL, but it appears before the GE; transmission lines should follow geometry.", i + 1);
L375:         snprintf(msg, sizeof(msg), "The card on line %d is an LD, but it appears before the GE; loading should follow geometry.", i + 1);
L380:         snprintf(msg, sizeof(msg), "The card on line %d is an FR, but it appears before the GE; frequency setup should follow geometry.", i + 1);
L385:         snprintf(msg, sizeof(msg), "The card on line %d is an RP, but it appears before the GE; pattern requests should follow geometry.", i + 1);
L390:         snprintf(msg, sizeof(msg), "The card on line %d is a GN, but it appears before the GE; ground settings should follow geometry.", i + 1);
L395:         snprintf(msg, sizeof(msg), "The card on line %d is a GD, but it appears before the GE; ground parameters should follow geometry.", i + 1);
L400:         snprintf(msg, sizeof(msg), "The card on line %d is an EK, but it appears before the GE; EK should follow geometry to apply the extended kernel.", i + 1);
L410:         snprintf(msg, sizeof(msg), "The card on line %d is a GE but has more than one integer input.", i);
L416:         snprintf(msg, sizeof(msg), "The card on line %d is a GE with I1=%d, which is outside the typical range {-1,0,1,2}.", i, gei1);
L421:         snprintf(msg, sizeof(msg), "The card on line %d is a GE but has floating-point inputs, which are not expected.", i);
L431:         snprintf(msg, sizeof(msg), "The card on line %d is a TL but has fewer than 4 integer inputs (tag/segment locators).", i);
L437:         snprintf(msg, sizeof(msg), "The card on line %d is a TL with non-positive tag/segment locator(s).", i);
L442:         snprintf(msg, sizeof(msg), "The card on line %d is a TL but has no characteristic impedance in F1.", i);
L447:         snprintf(msg, sizeof(msg), "The card on line %d is a TL with Z0 = 0 in F1, which is invalid.", i);
L467:         snprintf(msg, sizeof(msg), "The card on line %d is an EX but has fewer than 4 integer inputs.", i);
L472:         snprintf(msg, sizeof(msg), "The card on line %d is an EX but has no amplitude in F1.", i);
L477:         snprintf(msg, sizeof(msg), "The card on line %d is an EX with zero amplitude in F1.", i);
L485:           snprintf(msg, sizeof(msg), "The card on line %d is an EX with non-positive tag or segment locator.", i);
L495:           snprintf(msg, sizeof(msg), "The card on line %d is an EX with type %d, which is not supported by OpenNEC.", i, ex_type);
L518:         snprintf(msg, sizeof(msg), "The card on line %d is a GN but has no integer ground type specified.", i);
L529:         snprintf(msg, sizeof(msg), "The card on line %d is an LD but has fewer than 3 integer inputs (type, tag, segment).", i);
L540:           snprintf(msg, sizeof(msg), "The card on line %d is an LD with unexpected type I1=%d.", i, type);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 546
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 486-551) ---
    486:           add_error(ctx, errors, msg, 0);
    487:         }
    488:       }
    489:       // check for unsupported EX types
    490:       if (deck->cards[i].ints_used >= 1)
    491:       {
    492:         int ex_type = deck->cards[i].i[1];
    493:         if (ex_type == 6 || ex_type == 7)
    494:         {
    495:           snprintf(msg, sizeof(msg), "The card on line %d is an EX with type %d, which is not supported by OpenNEC.", i, ex_type);
    496:           add_error(ctx, errors, msg, 0);
    497:         }
    498:       }
    499:       // record for open-end placement validation later
    500:       if (deck->cards[i].ints_used >= 3)
    501:       {
    502:         ref_info_t r = {
    503:             .line = i + 1,
    504:             .tag = deck->cards[i].i[2],
    505:             .segStart = deck->cards[i].i[3],
    506:             .segEnd = 0};
    507:         if (ex_ref_count < (int)(sizeof(ex_refs) / sizeof(ex_refs[0])))
    508:           ex_ref_count++;
    509:         ex_refs[ex_ref_count - 1] = r;
    510:       }
    511:     }
    512: 
    513:     // GN: minimal check — at least the ground type integer should be present
    514:     if (strcmp(code, "GN") == 0)
    515:     {
    516:       if (deck->cards[i].ints_used < 1)
    517:       {
    518:         snprintf(msg, sizeof(msg), "The card on line %d is a GN but has no integer ground type specified.", i);
    519:         add_error(ctx, errors, msg, 0);
    520:       }
    521:     }
    522: 
    523:     // LD: loading — require type, tag, segment and at least one non-zero value
    524:     if (strcmp(code, "LD") == 0)
    525:     {
    526:       // at minimum we need I1 (type), I2 (tag), I3 (segment)
    527:       if (deck->cards[i].ints_used < 3)
    528:       {
    529:         snprintf(msg, sizeof(msg), "The card on line %d is an LD but has fewer than 3 integer inputs (type, tag, segment).", i);
    530:         add_error(ctx, errors, msg, 0);
    531:       }
    532:       else
    533:       {
    534:         int type = deck->cards[i].i[1];
    535:         int tag = deck->cards[i].i[2];
    536:         int seg1 = deck->cards[i].i[3];
    537:         int seg2 = deck->cards[i].i[4];
    538:         if (type < -1)
    539:         {
    540:           snprintf(msg, sizeof(msg), "The card on line %d is an LD with unexpected type I1=%d.", i, type);
    541:           add_error(ctx, errors, msg, 0);
    542:         }
    543:         if (tag <= 0 || seg1 <= 0)
    544:         {
    545:           snprintf(msg, sizeof(msg), "The card on line %d is an LD with non-positive tag or segment locator.", i);
>   546:           add_error(ctx, errors, msg, 0);
    547:         }
    548:         if (seg2 != 0 && seg2 < seg1)
    549:         {
    550:           snprintf(msg, sizeof(msg), "The card on line %d is an LD with end segment I4 < start segment I3.", i);
    551:           add_error(ctx, errors, msg, 0);

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L355:         snprintf(msg, sizeof(msg), "The card on line %d is a %s, but it appears before the GE; control cards should follow geometry.", i + 1, code);
L365:         snprintf(msg, sizeof(msg), "The card on line %d is an EX, but it appears before the GE; excitations should follow geometry.", i + 1);
L370:         snprintf(msg, sizeof(msg), "The card on line %d is a TL, but it appears before the GE; transmission lines should follow geometry.", i + 1);
L375:         snprintf(msg, sizeof(msg), "The card on line %d is an LD, but it appears before the GE; loading should follow geometry.", i + 1);
L380:         snprintf(msg, sizeof(msg), "The card on line %d is an FR, but it appears before the GE; frequency setup should follow geometry.", i + 1);
L385:         snprintf(msg, sizeof(msg), "The card on line %d is an RP, but it appears before the GE; pattern requests should follow geometry.", i + 1);
L390:         snprintf(msg, sizeof(msg), "The card on line %d is a GN, but it appears before the GE; ground settings should follow geometry.", i + 1);
L395:         snprintf(msg, sizeof(msg), "The card on line %d is a GD, but it appears before the GE; ground parameters should follow geometry.", i + 1);
L400:         snprintf(msg, sizeof(msg), "The card on line %d is an EK, but it appears before the GE; EK should follow geometry to apply the extended kernel.", i + 1);
L410:         snprintf(msg, sizeof(msg), "The card on line %d is a GE but has more than one integer input.", i);
L416:         snprintf(msg, sizeof(msg), "The card on line %d is a GE with I1=%d, which is outside the typical range {-1,0,1,2}.", i, gei1);
L421:         snprintf(msg, sizeof(msg), "The card on line %d is a GE but has floating-point inputs, which are not expected.", i);
L431:         snprintf(msg, sizeof(msg), "The card on line %d is a TL but has fewer than 4 integer inputs (tag/segment locators).", i);
L437:         snprintf(msg, sizeof(msg), "The card on line %d is a TL with non-positive tag/segment locator(s).", i);
L442:         snprintf(msg, sizeof(msg), "The card on line %d is a TL but has no characteristic impedance in F1.", i);
L447:         snprintf(msg, sizeof(msg), "The card on line %d is a TL with Z0 = 0 in F1, which is invalid.", i);
L467:         snprintf(msg, sizeof(msg), "The card on line %d is an EX but has fewer than 4 integer inputs.", i);
L472:         snprintf(msg, sizeof(msg), "The card on line %d is an EX but has no amplitude in F1.", i);
L477:         snprintf(msg, sizeof(msg), "The card on line %d is an EX with zero amplitude in F1.", i);
L485:           snprintf(msg, sizeof(msg), "The card on line %d is an EX with non-positive tag or segment locator.", i);
L495:           snprintf(msg, sizeof(msg), "The card on line %d is an EX with type %d, which is not supported by OpenNEC.", i, ex_type);
L518:         snprintf(msg, sizeof(msg), "The card on line %d is a GN but has no integer ground type specified.", i);
L529:         snprintf(msg, sizeof(msg), "The card on line %d is an LD but has fewer than 3 integer inputs (type, tag, segment).", i);
L540:           snprintf(msg, sizeof(msg), "The card on line %d is an LD with unexpected type I1=%d.", i, type);
L545:           snprintf(msg, sizeof(msg), "The card on line %d is an LD with non-positive tag or segment locator.", i);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 551
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 491-556) ---
    491:       {
    492:         int ex_type = deck->cards[i].i[1];
    493:         if (ex_type == 6 || ex_type == 7)
    494:         {
    495:           snprintf(msg, sizeof(msg), "The card on line %d is an EX with type %d, which is not supported by OpenNEC.", i, ex_type);
    496:           add_error(ctx, errors, msg, 0);
    497:         }
    498:       }
    499:       // record for open-end placement validation later
    500:       if (deck->cards[i].ints_used >= 3)
    501:       {
    502:         ref_info_t r = {
    503:             .line = i + 1,
    504:             .tag = deck->cards[i].i[2],
    505:             .segStart = deck->cards[i].i[3],
    506:             .segEnd = 0};
    507:         if (ex_ref_count < (int)(sizeof(ex_refs) / sizeof(ex_refs[0])))
    508:           ex_ref_count++;
    509:         ex_refs[ex_ref_count - 1] = r;
    510:       }
    511:     }
    512: 
    513:     // GN: minimal check — at least the ground type integer should be present
    514:     if (strcmp(code, "GN") == 0)
    515:     {
    516:       if (deck->cards[i].ints_used < 1)
    517:       {
    518:         snprintf(msg, sizeof(msg), "The card on line %d is a GN but has no integer ground type specified.", i);
    519:         add_error(ctx, errors, msg, 0);
    520:       }
    521:     }
    522: 
    523:     // LD: loading — require type, tag, segment and at least one non-zero value
    524:     if (strcmp(code, "LD") == 0)
    525:     {
    526:       // at minimum we need I1 (type), I2 (tag), I3 (segment)
    527:       if (deck->cards[i].ints_used < 3)
    528:       {
    529:         snprintf(msg, sizeof(msg), "The card on line %d is an LD but has fewer than 3 integer inputs (type, tag, segment).", i);
    530:         add_error(ctx, errors, msg, 0);
    531:       }
    532:       else
    533:       {
    534:         int type = deck->cards[i].i[1];
    535:         int tag = deck->cards[i].i[2];
    536:         int seg1 = deck->cards[i].i[3];
    537:         int seg2 = deck->cards[i].i[4];
    538:         if (type < -1)
    539:         {
    540:           snprintf(msg, sizeof(msg), "The card on line %d is an LD with unexpected type I1=%d.", i, type);
    541:           add_error(ctx, errors, msg, 0);
    542:         }
    543:         if (tag <= 0 || seg1 <= 0)
    544:         {
    545:           snprintf(msg, sizeof(msg), "The card on line %d is an LD with non-positive tag or segment locator.", i);
    546:           add_error(ctx, errors, msg, 0);
    547:         }
    548:         if (seg2 != 0 && seg2 < seg1)
    549:         {
    550:           snprintf(msg, sizeof(msg), "The card on line %d is an LD with end segment I4 < start segment I3.", i);
>   551:           add_error(ctx, errors, msg, 0);
    552:         }
    553:       }
    554:       // Require at least one float and encourage non-zero values
    555:       if (deck->cards[i].flts_used < 1)
    556:       {

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L355:         snprintf(msg, sizeof(msg), "The card on line %d is a %s, but it appears before the GE; control cards should follow geometry.", i + 1, code);
L365:         snprintf(msg, sizeof(msg), "The card on line %d is an EX, but it appears before the GE; excitations should follow geometry.", i + 1);
L370:         snprintf(msg, sizeof(msg), "The card on line %d is a TL, but it appears before the GE; transmission lines should follow geometry.", i + 1);
L375:         snprintf(msg, sizeof(msg), "The card on line %d is an LD, but it appears before the GE; loading should follow geometry.", i + 1);
L380:         snprintf(msg, sizeof(msg), "The card on line %d is an FR, but it appears before the GE; frequency setup should follow geometry.", i + 1);
L385:         snprintf(msg, sizeof(msg), "The card on line %d is an RP, but it appears before the GE; pattern requests should follow geometry.", i + 1);
L390:         snprintf(msg, sizeof(msg), "The card on line %d is a GN, but it appears before the GE; ground settings should follow geometry.", i + 1);
L395:         snprintf(msg, sizeof(msg), "The card on line %d is a GD, but it appears before the GE; ground parameters should follow geometry.", i + 1);
L400:         snprintf(msg, sizeof(msg), "The card on line %d is an EK, but it appears before the GE; EK should follow geometry to apply the extended kernel.", i + 1);
L410:         snprintf(msg, sizeof(msg), "The card on line %d is a GE but has more than one integer input.", i);
L416:         snprintf(msg, sizeof(msg), "The card on line %d is a GE with I1=%d, which is outside the typical range {-1,0,1,2}.", i, gei1);
L421:         snprintf(msg, sizeof(msg), "The card on line %d is a GE but has floating-point inputs, which are not expected.", i);
L431:         snprintf(msg, sizeof(msg), "The card on line %d is a TL but has fewer than 4 integer inputs (tag/segment locators).", i);
L437:         snprintf(msg, sizeof(msg), "The card on line %d is a TL with non-positive tag/segment locator(s).", i);
L442:         snprintf(msg, sizeof(msg), "The card on line %d is a TL but has no characteristic impedance in F1.", i);
L447:         snprintf(msg, sizeof(msg), "The card on line %d is a TL with Z0 = 0 in F1, which is invalid.", i);
L467:         snprintf(msg, sizeof(msg), "The card on line %d is an EX but has fewer than 4 integer inputs.", i);
L472:         snprintf(msg, sizeof(msg), "The card on line %d is an EX but has no amplitude in F1.", i);
L477:         snprintf(msg, sizeof(msg), "The card on line %d is an EX with zero amplitude in F1.", i);
L485:           snprintf(msg, sizeof(msg), "The card on line %d is an EX with non-positive tag or segment locator.", i);
L495:           snprintf(msg, sizeof(msg), "The card on line %d is an EX with type %d, which is not supported by OpenNEC.", i, ex_type);
L518:         snprintf(msg, sizeof(msg), "The card on line %d is a GN but has no integer ground type specified.", i);
L529:         snprintf(msg, sizeof(msg), "The card on line %d is an LD but has fewer than 3 integer inputs (type, tag, segment).", i);
L540:           snprintf(msg, sizeof(msg), "The card on line %d is an LD with unexpected type I1=%d.", i, type);
L545:           snprintf(msg, sizeof(msg), "The card on line %d is an LD with non-positive tag or segment locator.", i);
L550:           snprintf(msg, sizeof(msg), "The card on line %d is an LD with end segment I4 < start segment I3.", i);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 558
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 498-563) ---
    498:       }
    499:       // record for open-end placement validation later
    500:       if (deck->cards[i].ints_used >= 3)
    501:       {
    502:         ref_info_t r = {
    503:             .line = i + 1,
    504:             .tag = deck->cards[i].i[2],
    505:             .segStart = deck->cards[i].i[3],
    506:             .segEnd = 0};
    507:         if (ex_ref_count < (int)(sizeof(ex_refs) / sizeof(ex_refs[0])))
    508:           ex_ref_count++;
    509:         ex_refs[ex_ref_count - 1] = r;
    510:       }
    511:     }
    512: 
    513:     // GN: minimal check — at least the ground type integer should be present
    514:     if (strcmp(code, "GN") == 0)
    515:     {
    516:       if (deck->cards[i].ints_used < 1)
    517:       {
    518:         snprintf(msg, sizeof(msg), "The card on line %d is a GN but has no integer ground type specified.", i);
    519:         add_error(ctx, errors, msg, 0);
    520:       }
    521:     }
    522: 
    523:     // LD: loading — require type, tag, segment and at least one non-zero value
    524:     if (strcmp(code, "LD") == 0)
    525:     {
    526:       // at minimum we need I1 (type), I2 (tag), I3 (segment)
    527:       if (deck->cards[i].ints_used < 3)
    528:       {
    529:         snprintf(msg, sizeof(msg), "The card on line %d is an LD but has fewer than 3 integer inputs (type, tag, segment).", i);
    530:         add_error(ctx, errors, msg, 0);
    531:       }
    532:       else
    533:       {
    534:         int type = deck->cards[i].i[1];
    535:         int tag = deck->cards[i].i[2];
    536:         int seg1 = deck->cards[i].i[3];
    537:         int seg2 = deck->cards[i].i[4];
    538:         if (type < -1)
    539:         {
    540:           snprintf(msg, sizeof(msg), "The card on line %d is an LD with unexpected type I1=%d.", i, type);
    541:           add_error(ctx, errors, msg, 0);
    542:         }
    543:         if (tag <= 0 || seg1 <= 0)
    544:         {
    545:           snprintf(msg, sizeof(msg), "The card on line %d is an LD with non-positive tag or segment locator.", i);
    546:           add_error(ctx, errors, msg, 0);
    547:         }
    548:         if (seg2 != 0 && seg2 < seg1)
    549:         {
    550:           snprintf(msg, sizeof(msg), "The card on line %d is an LD with end segment I4 < start segment I3.", i);
    551:           add_error(ctx, errors, msg, 0);
    552:         }
    553:       }
    554:       // Require at least one float and encourage non-zero values
    555:       if (deck->cards[i].flts_used < 1)
    556:       {
    557:         snprintf(msg, sizeof(msg), "The card on line %d is an LD but has no floating-point load value (e.g., resistance).", i);
>   558:         add_error(ctx, errors, msg, 0);
    559:       }
    560:       else if (deck->cards[i].f[1] == 0.0 && deck->cards[i].f[2] == 0.0 && deck->cards[i].f[3] == 0.0)
    561:       {
    562:         snprintf(msg, sizeof(msg), "The card on line %d is an LD with zero load values (F1..F3 all zero).", i);
    563:         add_error(ctx, errors, msg, 0);

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L365:         snprintf(msg, sizeof(msg), "The card on line %d is an EX, but it appears before the GE; excitations should follow geometry.", i + 1);
L370:         snprintf(msg, sizeof(msg), "The card on line %d is a TL, but it appears before the GE; transmission lines should follow geometry.", i + 1);
L375:         snprintf(msg, sizeof(msg), "The card on line %d is an LD, but it appears before the GE; loading should follow geometry.", i + 1);
L380:         snprintf(msg, sizeof(msg), "The card on line %d is an FR, but it appears before the GE; frequency setup should follow geometry.", i + 1);
L385:         snprintf(msg, sizeof(msg), "The card on line %d is an RP, but it appears before the GE; pattern requests should follow geometry.", i + 1);
L390:         snprintf(msg, sizeof(msg), "The card on line %d is a GN, but it appears before the GE; ground settings should follow geometry.", i + 1);
L395:         snprintf(msg, sizeof(msg), "The card on line %d is a GD, but it appears before the GE; ground parameters should follow geometry.", i + 1);
L400:         snprintf(msg, sizeof(msg), "The card on line %d is an EK, but it appears before the GE; EK should follow geometry to apply the extended kernel.", i + 1);
L410:         snprintf(msg, sizeof(msg), "The card on line %d is a GE but has more than one integer input.", i);
L416:         snprintf(msg, sizeof(msg), "The card on line %d is a GE with I1=%d, which is outside the typical range {-1,0,1,2}.", i, gei1);
L421:         snprintf(msg, sizeof(msg), "The card on line %d is a GE but has floating-point inputs, which are not expected.", i);
L431:         snprintf(msg, sizeof(msg), "The card on line %d is a TL but has fewer than 4 integer inputs (tag/segment locators).", i);
L437:         snprintf(msg, sizeof(msg), "The card on line %d is a TL with non-positive tag/segment locator(s).", i);
L442:         snprintf(msg, sizeof(msg), "The card on line %d is a TL but has no characteristic impedance in F1.", i);
L447:         snprintf(msg, sizeof(msg), "The card on line %d is a TL with Z0 = 0 in F1, which is invalid.", i);
L467:         snprintf(msg, sizeof(msg), "The card on line %d is an EX but has fewer than 4 integer inputs.", i);
L472:         snprintf(msg, sizeof(msg), "The card on line %d is an EX but has no amplitude in F1.", i);
L477:         snprintf(msg, sizeof(msg), "The card on line %d is an EX with zero amplitude in F1.", i);
L485:           snprintf(msg, sizeof(msg), "The card on line %d is an EX with non-positive tag or segment locator.", i);
L495:           snprintf(msg, sizeof(msg), "The card on line %d is an EX with type %d, which is not supported by OpenNEC.", i, ex_type);
L518:         snprintf(msg, sizeof(msg), "The card on line %d is a GN but has no integer ground type specified.", i);
L529:         snprintf(msg, sizeof(msg), "The card on line %d is an LD but has fewer than 3 integer inputs (type, tag, segment).", i);
L540:           snprintf(msg, sizeof(msg), "The card on line %d is an LD with unexpected type I1=%d.", i, type);
L545:           snprintf(msg, sizeof(msg), "The card on line %d is an LD with non-positive tag or segment locator.", i);
L550:           snprintf(msg, sizeof(msg), "The card on line %d is an LD with end segment I4 < start segment I3.", i);
L557:         snprintf(msg, sizeof(msg), "The card on line %d is an LD but has no floating-point load value (e.g., resistance).", i);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 563
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 503-568) ---
    503:             .line = i + 1,
    504:             .tag = deck->cards[i].i[2],
    505:             .segStart = deck->cards[i].i[3],
    506:             .segEnd = 0};
    507:         if (ex_ref_count < (int)(sizeof(ex_refs) / sizeof(ex_refs[0])))
    508:           ex_ref_count++;
    509:         ex_refs[ex_ref_count - 1] = r;
    510:       }
    511:     }
    512: 
    513:     // GN: minimal check — at least the ground type integer should be present
    514:     if (strcmp(code, "GN") == 0)
    515:     {
    516:       if (deck->cards[i].ints_used < 1)
    517:       {
    518:         snprintf(msg, sizeof(msg), "The card on line %d is a GN but has no integer ground type specified.", i);
    519:         add_error(ctx, errors, msg, 0);
    520:       }
    521:     }
    522: 
    523:     // LD: loading — require type, tag, segment and at least one non-zero value
    524:     if (strcmp(code, "LD") == 0)
    525:     {
    526:       // at minimum we need I1 (type), I2 (tag), I3 (segment)
    527:       if (deck->cards[i].ints_used < 3)
    528:       {
    529:         snprintf(msg, sizeof(msg), "The card on line %d is an LD but has fewer than 3 integer inputs (type, tag, segment).", i);
    530:         add_error(ctx, errors, msg, 0);
    531:       }
    532:       else
    533:       {
    534:         int type = deck->cards[i].i[1];
    535:         int tag = deck->cards[i].i[2];
    536:         int seg1 = deck->cards[i].i[3];
    537:         int seg2 = deck->cards[i].i[4];
    538:         if (type < -1)
    539:         {
    540:           snprintf(msg, sizeof(msg), "The card on line %d is an LD with unexpected type I1=%d.", i, type);
    541:           add_error(ctx, errors, msg, 0);
    542:         }
    543:         if (tag <= 0 || seg1 <= 0)
    544:         {
    545:           snprintf(msg, sizeof(msg), "The card on line %d is an LD with non-positive tag or segment locator.", i);
    546:           add_error(ctx, errors, msg, 0);
    547:         }
    548:         if (seg2 != 0 && seg2 < seg1)
    549:         {
    550:           snprintf(msg, sizeof(msg), "The card on line %d is an LD with end segment I4 < start segment I3.", i);
    551:           add_error(ctx, errors, msg, 0);
    552:         }
    553:       }
    554:       // Require at least one float and encourage non-zero values
    555:       if (deck->cards[i].flts_used < 1)
    556:       {
    557:         snprintf(msg, sizeof(msg), "The card on line %d is an LD but has no floating-point load value (e.g., resistance).", i);
    558:         add_error(ctx, errors, msg, 0);
    559:       }
    560:       else if (deck->cards[i].f[1] == 0.0 && deck->cards[i].f[2] == 0.0 && deck->cards[i].f[3] == 0.0)
    561:       {
    562:         snprintf(msg, sizeof(msg), "The card on line %d is an LD with zero load values (F1..F3 all zero).", i);
>   563:         add_error(ctx, errors, msg, 0);
    564:       }
    565:     }
    566:     // record for open-end placement validation later (only for LD cards)
    567:     if (strcmp(code, "LD") == 0 && deck->cards[i].ints_used >= 3)
    568:     {

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L365:         snprintf(msg, sizeof(msg), "The card on line %d is an EX, but it appears before the GE; excitations should follow geometry.", i + 1);
L370:         snprintf(msg, sizeof(msg), "The card on line %d is a TL, but it appears before the GE; transmission lines should follow geometry.", i + 1);
L375:         snprintf(msg, sizeof(msg), "The card on line %d is an LD, but it appears before the GE; loading should follow geometry.", i + 1);
L380:         snprintf(msg, sizeof(msg), "The card on line %d is an FR, but it appears before the GE; frequency setup should follow geometry.", i + 1);
L385:         snprintf(msg, sizeof(msg), "The card on line %d is an RP, but it appears before the GE; pattern requests should follow geometry.", i + 1);
L390:         snprintf(msg, sizeof(msg), "The card on line %d is a GN, but it appears before the GE; ground settings should follow geometry.", i + 1);
L395:         snprintf(msg, sizeof(msg), "The card on line %d is a GD, but it appears before the GE; ground parameters should follow geometry.", i + 1);
L400:         snprintf(msg, sizeof(msg), "The card on line %d is an EK, but it appears before the GE; EK should follow geometry to apply the extended kernel.", i + 1);
L410:         snprintf(msg, sizeof(msg), "The card on line %d is a GE but has more than one integer input.", i);
L416:         snprintf(msg, sizeof(msg), "The card on line %d is a GE with I1=%d, which is outside the typical range {-1,0,1,2}.", i, gei1);
L421:         snprintf(msg, sizeof(msg), "The card on line %d is a GE but has floating-point inputs, which are not expected.", i);
L431:         snprintf(msg, sizeof(msg), "The card on line %d is a TL but has fewer than 4 integer inputs (tag/segment locators).", i);
L437:         snprintf(msg, sizeof(msg), "The card on line %d is a TL with non-positive tag/segment locator(s).", i);
L442:         snprintf(msg, sizeof(msg), "The card on line %d is a TL but has no characteristic impedance in F1.", i);
L447:         snprintf(msg, sizeof(msg), "The card on line %d is a TL with Z0 = 0 in F1, which is invalid.", i);
L467:         snprintf(msg, sizeof(msg), "The card on line %d is an EX but has fewer than 4 integer inputs.", i);
L472:         snprintf(msg, sizeof(msg), "The card on line %d is an EX but has no amplitude in F1.", i);
L477:         snprintf(msg, sizeof(msg), "The card on line %d is an EX with zero amplitude in F1.", i);
L485:           snprintf(msg, sizeof(msg), "The card on line %d is an EX with non-positive tag or segment locator.", i);
L495:           snprintf(msg, sizeof(msg), "The card on line %d is an EX with type %d, which is not supported by OpenNEC.", i, ex_type);
L518:         snprintf(msg, sizeof(msg), "The card on line %d is a GN but has no integer ground type specified.", i);
L529:         snprintf(msg, sizeof(msg), "The card on line %d is an LD but has fewer than 3 integer inputs (type, tag, segment).", i);
L540:           snprintf(msg, sizeof(msg), "The card on line %d is an LD with unexpected type I1=%d.", i, type);
L545:           snprintf(msg, sizeof(msg), "The card on line %d is an LD with non-positive tag or segment locator.", i);
L550:           snprintf(msg, sizeof(msg), "The card on line %d is an LD with end segment I4 < start segment I3.", i);
L557:         snprintf(msg, sizeof(msg), "The card on line %d is an LD but has no floating-point load value (e.g., resistance).", i);
L562:         snprintf(msg, sizeof(msg), "The card on line %d is an LD with zero load values (F1..F3 all zero).", i);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 614
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 554-619) ---
    554:       // Require at least one float and encourage non-zero values
    555:       if (deck->cards[i].flts_used < 1)
    556:       {
    557:         snprintf(msg, sizeof(msg), "The card on line %d is an LD but has no floating-point load value (e.g., resistance).", i);
    558:         add_error(ctx, errors, msg, 0);
    559:       }
    560:       else if (deck->cards[i].f[1] == 0.0 && deck->cards[i].f[2] == 0.0 && deck->cards[i].f[3] == 0.0)
    561:       {
    562:         snprintf(msg, sizeof(msg), "The card on line %d is an LD with zero load values (F1..F3 all zero).", i);
    563:         add_error(ctx, errors, msg, 0);
    564:       }
    565:     }
    566:     // record for open-end placement validation later (only for LD cards)
    567:     if (strcmp(code, "LD") == 0 && deck->cards[i].ints_used >= 3)
    568:     {
    569:       int segStart = deck->cards[i].i[3];
    570:       int segEnd = deck->cards[i].i[4];
    571:       ref_info_t r = {
    572:           .line = i + 1,
    573:           .tag = deck->cards[i].i[2],
    574:           .segStart = segStart,
    575:           .segEnd = segEnd};
    576:       if (ld_ref_count < (int)(sizeof(ld_refs) / sizeof(ld_refs[0])))
    577:         ld_ref_count++;
    578:       ld_refs[ld_ref_count - 1] = r;
    579:     }
    580: 
    581:     // along with some others we want to keep track of
    582: 
    583:     // we want to see if there are any SY's at all
    584:     if (strcmp(code, "SY") == 0)
    585:     {
    586:       sawSY = true;
    587:     }
    588:     // you can have multiple GN cards, but only the last one is used for a given execution
    589:     if (strcmp(code, "GN") == 0)
    590:     {
    591:       if (sawGN == false)
    592:         sawGN = i;
    593:     }
    594:     // you can have multiple GC cards, but there has to be a GN somewhere
    595:     if (strcmp(code, "GD") == 0)
    596:     {
    597:       if (sawGD == false)
    598:       {
    599:         sawGD = i;
    600:       }
    601:     }
    602:     // you can have multiple SCs, but they have to follow a SP or SM
    603:     if (strcmp(code, "SC") == 0)
    604:     {
    605:       if (sawSC == false)
    606:         sawSC = i;
    607:     }
    608:     if (strcmp(code, "RP") == 0)
    609:     {
    610:       // RP should generally follow FR; warn if FR not yet seen
    611:       if (sawFR == false)
    612:       {
    613:         snprintf(msg, sizeof(msg), "The card on line %d is an RP, but no FR has been seen earlier.", i + 1);
>   614:         add_error(ctx, errors, msg, 0);
    615:       }
    616:       if (sawSP == false)
    617:         sawSP = i;
    618:       if (sawRP == 0)
    619:         sawRP = i + 1; // mark that we have at least one RP (store 1-based index)

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L416:         snprintf(msg, sizeof(msg), "The card on line %d is a GE with I1=%d, which is outside the typical range {-1,0,1,2}.", i, gei1);
L421:         snprintf(msg, sizeof(msg), "The card on line %d is a GE but has floating-point inputs, which are not expected.", i);
L431:         snprintf(msg, sizeof(msg), "The card on line %d is a TL but has fewer than 4 integer inputs (tag/segment locators).", i);
L437:         snprintf(msg, sizeof(msg), "The card on line %d is a TL with non-positive tag/segment locator(s).", i);
L442:         snprintf(msg, sizeof(msg), "The card on line %d is a TL but has no characteristic impedance in F1.", i);
L447:         snprintf(msg, sizeof(msg), "The card on line %d is a TL with Z0 = 0 in F1, which is invalid.", i);
L467:         snprintf(msg, sizeof(msg), "The card on line %d is an EX but has fewer than 4 integer inputs.", i);
L472:         snprintf(msg, sizeof(msg), "The card on line %d is an EX but has no amplitude in F1.", i);
L477:         snprintf(msg, sizeof(msg), "The card on line %d is an EX with zero amplitude in F1.", i);
L485:           snprintf(msg, sizeof(msg), "The card on line %d is an EX with non-positive tag or segment locator.", i);
L495:           snprintf(msg, sizeof(msg), "The card on line %d is an EX with type %d, which is not supported by OpenNEC.", i, ex_type);
L518:         snprintf(msg, sizeof(msg), "The card on line %d is a GN but has no integer ground type specified.", i);
L529:         snprintf(msg, sizeof(msg), "The card on line %d is an LD but has fewer than 3 integer inputs (type, tag, segment).", i);
L540:           snprintf(msg, sizeof(msg), "The card on line %d is an LD with unexpected type I1=%d.", i, type);
L545:           snprintf(msg, sizeof(msg), "The card on line %d is an LD with non-positive tag or segment locator.", i);
L550:           snprintf(msg, sizeof(msg), "The card on line %d is an LD with end segment I4 < start segment I3.", i);
L557:         snprintf(msg, sizeof(msg), "The card on line %d is an LD but has no floating-point load value (e.g., resistance).", i);
L562:         snprintf(msg, sizeof(msg), "The card on line %d is an LD with zero load values (F1..F3 all zero).", i);
L613:         snprintf(msg, sizeof(msg), "The card on line %d is an RP, but no FR has been seen earlier.", i + 1);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 648
CALL: add_error(ctx, errors, msg, 1);
--- Context (lines 588-653) ---
    588:     // you can have multiple GN cards, but only the last one is used for a given execution
    589:     if (strcmp(code, "GN") == 0)
    590:     {
    591:       if (sawGN == false)
    592:         sawGN = i;
    593:     }
    594:     // you can have multiple GC cards, but there has to be a GN somewhere
    595:     if (strcmp(code, "GD") == 0)
    596:     {
    597:       if (sawGD == false)
    598:       {
    599:         sawGD = i;
    600:       }
    601:     }
    602:     // you can have multiple SCs, but they have to follow a SP or SM
    603:     if (strcmp(code, "SC") == 0)
    604:     {
    605:       if (sawSC == false)
    606:         sawSC = i;
    607:     }
    608:     if (strcmp(code, "RP") == 0)
    609:     {
    610:       // RP should generally follow FR; warn if FR not yet seen
    611:       if (sawFR == false)
    612:       {
    613:         snprintf(msg, sizeof(msg), "The card on line %d is an RP, but no FR has been seen earlier.", i + 1);
    614:         add_error(ctx, errors, msg, 0);
    615:       }
    616:       if (sawSP == false)
    617:         sawSP = i;
    618:       if (sawRP == 0)
    619:         sawRP = i + 1; // mark that we have at least one RP (store 1-based index)
    620:     }
    621:     // you need an EX or LD
    622:     if (strcmp(code, "EX") == 0)
    623:     {
    624:       if (sawEX == false)
    625:         sawEX = i;
    626:     }
    627:     // you should have an EX?
    628:     if (strcmp(code, "LD") == 0)
    629:     {
    630:       if (sawLD == false)
    631:         sawLD = i;
    632:     }
    633: 
    634:     // geometry cards are a little harder because there are many of them
    635:     for (int j = 0; j < NUM_GEOMETRY_CODES; j++)
    636:     {
    637:       if (strcmp(code, geometry_codes[j]) == 0 && strcmp(code, "GE") != 0)
    638:       {
    639:         if (sawGx == false)
    640:         {
    641:           sawGx = i;
    642:         }
    643:         // there's no else in this case, multiple Gx cards are fine, however
    644:         // we do have a potential problem when we find Gx cards after a GE
    645:         if (sawGx > 0 && sawGE > 0)
    646:         {
    647:           snprintf(msg, sizeof(msg), "The card on line %d has geometry code %s, but we already saw the GE on card %d.", i, code, sawGE + 1);
>   648:           add_error(ctx, errors, msg, 1);
    649:         }
    650:         // record geometry tags seen (I1) for later reference by control cards
    651:         if (deck->cards[i].i[1] > 0)
    652:         {
    653:           int tag_i = deck->cards[i].i[1];

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L467:         snprintf(msg, sizeof(msg), "The card on line %d is an EX but has fewer than 4 integer inputs.", i);
L472:         snprintf(msg, sizeof(msg), "The card on line %d is an EX but has no amplitude in F1.", i);
L477:         snprintf(msg, sizeof(msg), "The card on line %d is an EX with zero amplitude in F1.", i);
L485:           snprintf(msg, sizeof(msg), "The card on line %d is an EX with non-positive tag or segment locator.", i);
L495:           snprintf(msg, sizeof(msg), "The card on line %d is an EX with type %d, which is not supported by OpenNEC.", i, ex_type);
L518:         snprintf(msg, sizeof(msg), "The card on line %d is a GN but has no integer ground type specified.", i);
L529:         snprintf(msg, sizeof(msg), "The card on line %d is an LD but has fewer than 3 integer inputs (type, tag, segment).", i);
L540:           snprintf(msg, sizeof(msg), "The card on line %d is an LD with unexpected type I1=%d.", i, type);
L545:           snprintf(msg, sizeof(msg), "The card on line %d is an LD with non-positive tag or segment locator.", i);
L550:           snprintf(msg, sizeof(msg), "The card on line %d is an LD with end segment I4 < start segment I3.", i);
L557:         snprintf(msg, sizeof(msg), "The card on line %d is an LD but has no floating-point load value (e.g., resistance).", i);
L562:         snprintf(msg, sizeof(msg), "The card on line %d is an LD with zero load values (F1..F3 all zero).", i);
L613:         snprintf(msg, sizeof(msg), "The card on line %d is an RP, but no FR has been seen earlier.", i + 1);
L647:           snprintf(msg, sizeof(msg), "The card on line %d has geometry code %s, but we already saw the GE on card %d.", i, code, sawGE + 1);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 702
CALL: add_error(ctx, errors, msg, 1);
--- Context (lines 642-707) ---
    642:         }
    643:         // there's no else in this case, multiple Gx cards are fine, however
    644:         // we do have a potential problem when we find Gx cards after a GE
    645:         if (sawGx > 0 && sawGE > 0)
    646:         {
    647:           snprintf(msg, sizeof(msg), "The card on line %d has geometry code %s, but we already saw the GE on card %d.", i, code, sawGE + 1);
    648:           add_error(ctx, errors, msg, 1);
    649:         }
    650:         // record geometry tags seen (I1) for later reference by control cards
    651:         if (deck->cards[i].i[1] > 0)
    652:         {
    653:           int tag_i = deck->cards[i].i[1];
    654:           int found = 0;
    655:           for (int t = 0; t < geom_tag_count; t++)
    656:           {
    657:             if (geom_tags[t] == tag_i)
    658:             {
    659:               found = 1;
    660:               break;
    661:             }
    662:           }
    663:           if (!found && geom_tag_count < (int)(sizeof(geom_tags) / sizeof(geom_tags[0])))
    664:           {
    665:             geom_tags[geom_tag_count++] = tag_i;
    666:           }
    667:           // If this is a GW, capture its endpoints and segment count
    668:           if (strcmp(code, "GW") == 0 && wire_count < (int)(sizeof(wires) / sizeof(wires[0])))
    669:           {
    670:             wire_info_t w;
    671:             w.tag = tag_i;
    672:             w.segs = deck->cards[i].i[2];
    673:             w.line = i + 1;
    674:             w.x1 = deck->cards[i].f[1];
    675:             w.y1 = deck->cards[i].f[2];
    676:             w.z1 = deck->cards[i].f[3];
    677:             w.x2 = deck->cards[i].f[4];
    678:             w.y2 = deck->cards[i].f[5];
    679:             w.z2 = deck->cards[i].f[6];
    680:             w.radius = deck->cards[i].f[7];
    681:             wires[wire_count++] = w;
    682:           }
    683:         }
    684:         break;
    685:       }
    686:     } /* loop over geometry codes */
    687: 
    688:     // unique one here - it's possible to have any number of GS cards, but
    689:     // it appears you can have multiple GS's, although why you would ever do that is unclear
    690:     if (strcmp(code, "GS") == 0)
    691:     {
    692:       if (sawGS == false)
    693:         sawGS = i;
    694:     }
    695: 
    696:     // now we look for card pairs, where one card has to follow another
    697: 
    698:     // GC cards have to follow GW cards
    699:     if (strcmp(code, "GC") == 0 && strcmp(last_code, "GW") != 0)
    700:     {
    701:       snprintf(msg, sizeof(msg), "The card on line %d is a GC, but the card above it is not a GW.", i + 1);
>   702:       add_error(ctx, errors, msg, 1);
    703:     }
    704: 
    705:     // FIXME: we should do this, but it has to be calculated first because it might be a formula or units
    706:     //    // GW cards with zero radius have to have a GC after it
    707:     //    if(strcmp(code, "GW") == 0 && deck->cards[i].f[7] == 0.0 && strcmp(deck->cards[i+1].card_code, "GC") != 0) {

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L518:         snprintf(msg, sizeof(msg), "The card on line %d is a GN but has no integer ground type specified.", i);
L529:         snprintf(msg, sizeof(msg), "The card on line %d is an LD but has fewer than 3 integer inputs (type, tag, segment).", i);
L540:           snprintf(msg, sizeof(msg), "The card on line %d is an LD with unexpected type I1=%d.", i, type);
L545:           snprintf(msg, sizeof(msg), "The card on line %d is an LD with non-positive tag or segment locator.", i);
L550:           snprintf(msg, sizeof(msg), "The card on line %d is an LD with end segment I4 < start segment I3.", i);
L557:         snprintf(msg, sizeof(msg), "The card on line %d is an LD but has no floating-point load value (e.g., resistance).", i);
L562:         snprintf(msg, sizeof(msg), "The card on line %d is an LD with zero load values (F1..F3 all zero).", i);
L613:         snprintf(msg, sizeof(msg), "The card on line %d is an RP, but no FR has been seen earlier.", i + 1);
L647:           snprintf(msg, sizeof(msg), "The card on line %d has geometry code %s, but we already saw the GE on card %d.", i, code, sawGE + 1);
L701:       snprintf(msg, sizeof(msg), "The card on line %d is a GC, but the card above it is not a GW.", i + 1);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 709
CALL: //      add_error(errors, msg, 1);
--- Context (lines 649-714) ---
    649:         }
    650:         // record geometry tags seen (I1) for later reference by control cards
    651:         if (deck->cards[i].i[1] > 0)
    652:         {
    653:           int tag_i = deck->cards[i].i[1];
    654:           int found = 0;
    655:           for (int t = 0; t < geom_tag_count; t++)
    656:           {
    657:             if (geom_tags[t] == tag_i)
    658:             {
    659:               found = 1;
    660:               break;
    661:             }
    662:           }
    663:           if (!found && geom_tag_count < (int)(sizeof(geom_tags) / sizeof(geom_tags[0])))
    664:           {
    665:             geom_tags[geom_tag_count++] = tag_i;
    666:           }
    667:           // If this is a GW, capture its endpoints and segment count
    668:           if (strcmp(code, "GW") == 0 && wire_count < (int)(sizeof(wires) / sizeof(wires[0])))
    669:           {
    670:             wire_info_t w;
    671:             w.tag = tag_i;
    672:             w.segs = deck->cards[i].i[2];
    673:             w.line = i + 1;
    674:             w.x1 = deck->cards[i].f[1];
    675:             w.y1 = deck->cards[i].f[2];
    676:             w.z1 = deck->cards[i].f[3];
    677:             w.x2 = deck->cards[i].f[4];
    678:             w.y2 = deck->cards[i].f[5];
    679:             w.z2 = deck->cards[i].f[6];
    680:             w.radius = deck->cards[i].f[7];
    681:             wires[wire_count++] = w;
    682:           }
    683:         }
    684:         break;
    685:       }
    686:     } /* loop over geometry codes */
    687: 
    688:     // unique one here - it's possible to have any number of GS cards, but
    689:     // it appears you can have multiple GS's, although why you would ever do that is unclear
    690:     if (strcmp(code, "GS") == 0)
    691:     {
    692:       if (sawGS == false)
    693:         sawGS = i;
    694:     }
    695: 
    696:     // now we look for card pairs, where one card has to follow another
    697: 
    698:     // GC cards have to follow GW cards
    699:     if (strcmp(code, "GC") == 0 && strcmp(last_code, "GW") != 0)
    700:     {
    701:       snprintf(msg, sizeof(msg), "The card on line %d is a GC, but the card above it is not a GW.", i + 1);
    702:       add_error(ctx, errors, msg, 1);
    703:     }
    704: 
    705:     // FIXME: we should do this, but it has to be calculated first because it might be a formula or units
    706:     //    // GW cards with zero radius have to have a GC after it
    707:     //    if(strcmp(code, "GW") == 0 && deck->cards[i].f[7] == 0.0 && strcmp(deck->cards[i+1].card_code, "GC") != 0) {
    708:     //      snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius, but the card after it is not a GC.", i + 1);
>   709:     //      add_error(errors, msg, 1);
    710:     //    }
    711: 
    712:     // GD cards have to follow GN cards
    713:     if (strcmp(code, "GD") == 0 && strcmp(last_code, "GN") != 0)
    714:     {

-- Message variable name candidates --
VAR_EXPR: 1  VAR_NAME: 1
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L509:         ex_refs[ex_ref_count - 1] = r;
L534:         int type = deck->cards[i].i[1];
L536:         int seg1 = deck->cards[i].i[3];
L540:           snprintf(msg, sizeof(msg), "The card on line %d is an LD with unexpected type I1=%d.", i, type);
L543:         if (tag <= 0 || seg1 <= 0)
L548:         if (seg2 != 0 && seg2 < seg1)
L560:       else if (deck->cards[i].f[1] == 0.0 && deck->cards[i].f[2] == 0.0 && deck->cards[i].f[3] == 0.0)
L562:         snprintf(msg, sizeof(msg), "The card on line %d is an LD with zero load values (F1..F3 all zero).", i);
L572:           .line = i + 1,
L578:       ld_refs[ld_ref_count - 1] = r;
L613:         snprintf(msg, sizeof(msg), "The card on line %d is an RP, but no FR has been seen earlier.", i + 1);
L619:         sawRP = i + 1; // mark that we have at least one RP (store 1-based index)
L647:           snprintf(msg, sizeof(msg), "The card on line %d has geometry code %s, but we already saw the GE on card %d.", i, code, sawGE + 1);
L653:           int tag_i = deck->cards[i].i[1];
L659:               found = 1;
L673:             w.line = i + 1;
L674:             w.x1 = deck->cards[i].f[1];
L675:             w.y1 = deck->cards[i].f[2];
L676:             w.z1 = deck->cards[i].f[3];
L701:       snprintf(msg, sizeof(msg), "The card on line %d is a GC, but the card above it is not a GW.", i + 1);
L707:     //    if(strcmp(code, "GW") == 0 && deck->cards[i].f[7] == 0.0 && strcmp(deck->cards[i+1].card_code, "GC") != 0) {
L708:     //      snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius, but the card after it is not a GC.", i + 1);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 716
CALL: add_error(ctx, errors, msg, 1);
--- Context (lines 656-721) ---
    656:           {
    657:             if (geom_tags[t] == tag_i)
    658:             {
    659:               found = 1;
    660:               break;
    661:             }
    662:           }
    663:           if (!found && geom_tag_count < (int)(sizeof(geom_tags) / sizeof(geom_tags[0])))
    664:           {
    665:             geom_tags[geom_tag_count++] = tag_i;
    666:           }
    667:           // If this is a GW, capture its endpoints and segment count
    668:           if (strcmp(code, "GW") == 0 && wire_count < (int)(sizeof(wires) / sizeof(wires[0])))
    669:           {
    670:             wire_info_t w;
    671:             w.tag = tag_i;
    672:             w.segs = deck->cards[i].i[2];
    673:             w.line = i + 1;
    674:             w.x1 = deck->cards[i].f[1];
    675:             w.y1 = deck->cards[i].f[2];
    676:             w.z1 = deck->cards[i].f[3];
    677:             w.x2 = deck->cards[i].f[4];
    678:             w.y2 = deck->cards[i].f[5];
    679:             w.z2 = deck->cards[i].f[6];
    680:             w.radius = deck->cards[i].f[7];
    681:             wires[wire_count++] = w;
    682:           }
    683:         }
    684:         break;
    685:       }
    686:     } /* loop over geometry codes */
    687: 
    688:     // unique one here - it's possible to have any number of GS cards, but
    689:     // it appears you can have multiple GS's, although why you would ever do that is unclear
    690:     if (strcmp(code, "GS") == 0)
    691:     {
    692:       if (sawGS == false)
    693:         sawGS = i;
    694:     }
    695: 
    696:     // now we look for card pairs, where one card has to follow another
    697: 
    698:     // GC cards have to follow GW cards
    699:     if (strcmp(code, "GC") == 0 && strcmp(last_code, "GW") != 0)
    700:     {
    701:       snprintf(msg, sizeof(msg), "The card on line %d is a GC, but the card above it is not a GW.", i + 1);
    702:       add_error(ctx, errors, msg, 1);
    703:     }
    704: 
    705:     // FIXME: we should do this, but it has to be calculated first because it might be a formula or units
    706:     //    // GW cards with zero radius have to have a GC after it
    707:     //    if(strcmp(code, "GW") == 0 && deck->cards[i].f[7] == 0.0 && strcmp(deck->cards[i+1].card_code, "GC") != 0) {
    708:     //      snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius, but the card after it is not a GC.", i + 1);
    709:     //      add_error(errors, msg, 1);
    710:     //    }
    711: 
    712:     // GD cards have to follow GN cards
    713:     if (strcmp(code, "GD") == 0 && strcmp(last_code, "GN") != 0)
    714:     {
    715:       snprintf(msg, sizeof(msg), "The card on line %d is a GD, but the card above it is not a GN.", i + 1);
>   716:       add_error(ctx, errors, msg, 1);
    717:     }
    718:     if (strcmp(code, "GN") == 0)
    719:     {
    720:       if (i + 1 >= deck->num_cards || strcmp(deck->cards[i + 1].card_code, "GD") != 0)
    721:       {

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L518:         snprintf(msg, sizeof(msg), "The card on line %d is a GN but has no integer ground type specified.", i);
L529:         snprintf(msg, sizeof(msg), "The card on line %d is an LD but has fewer than 3 integer inputs (type, tag, segment).", i);
L540:           snprintf(msg, sizeof(msg), "The card on line %d is an LD with unexpected type I1=%d.", i, type);
L545:           snprintf(msg, sizeof(msg), "The card on line %d is an LD with non-positive tag or segment locator.", i);
L550:           snprintf(msg, sizeof(msg), "The card on line %d is an LD with end segment I4 < start segment I3.", i);
L557:         snprintf(msg, sizeof(msg), "The card on line %d is an LD but has no floating-point load value (e.g., resistance).", i);
L562:         snprintf(msg, sizeof(msg), "The card on line %d is an LD with zero load values (F1..F3 all zero).", i);
L613:         snprintf(msg, sizeof(msg), "The card on line %d is an RP, but no FR has been seen earlier.", i + 1);
L647:           snprintf(msg, sizeof(msg), "The card on line %d has geometry code %s, but we already saw the GE on card %d.", i, code, sawGE + 1);
L701:       snprintf(msg, sizeof(msg), "The card on line %d is a GC, but the card above it is not a GW.", i + 1);
L708:     //      snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius, but the card after it is not a GC.", i + 1);
L715:       snprintf(msg, sizeof(msg), "The card on line %d is a GD, but the card above it is not a GN.", i + 1);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 723
CALL: add_error(ctx, errors, msg, 1);
--- Context (lines 663-728) ---
    663:           if (!found && geom_tag_count < (int)(sizeof(geom_tags) / sizeof(geom_tags[0])))
    664:           {
    665:             geom_tags[geom_tag_count++] = tag_i;
    666:           }
    667:           // If this is a GW, capture its endpoints and segment count
    668:           if (strcmp(code, "GW") == 0 && wire_count < (int)(sizeof(wires) / sizeof(wires[0])))
    669:           {
    670:             wire_info_t w;
    671:             w.tag = tag_i;
    672:             w.segs = deck->cards[i].i[2];
    673:             w.line = i + 1;
    674:             w.x1 = deck->cards[i].f[1];
    675:             w.y1 = deck->cards[i].f[2];
    676:             w.z1 = deck->cards[i].f[3];
    677:             w.x2 = deck->cards[i].f[4];
    678:             w.y2 = deck->cards[i].f[5];
    679:             w.z2 = deck->cards[i].f[6];
    680:             w.radius = deck->cards[i].f[7];
    681:             wires[wire_count++] = w;
    682:           }
    683:         }
    684:         break;
    685:       }
    686:     } /* loop over geometry codes */
    687: 
    688:     // unique one here - it's possible to have any number of GS cards, but
    689:     // it appears you can have multiple GS's, although why you would ever do that is unclear
    690:     if (strcmp(code, "GS") == 0)
    691:     {
    692:       if (sawGS == false)
    693:         sawGS = i;
    694:     }
    695: 
    696:     // now we look for card pairs, where one card has to follow another
    697: 
    698:     // GC cards have to follow GW cards
    699:     if (strcmp(code, "GC") == 0 && strcmp(last_code, "GW") != 0)
    700:     {
    701:       snprintf(msg, sizeof(msg), "The card on line %d is a GC, but the card above it is not a GW.", i + 1);
    702:       add_error(ctx, errors, msg, 1);
    703:     }
    704: 
    705:     // FIXME: we should do this, but it has to be calculated first because it might be a formula or units
    706:     //    // GW cards with zero radius have to have a GC after it
    707:     //    if(strcmp(code, "GW") == 0 && deck->cards[i].f[7] == 0.0 && strcmp(deck->cards[i+1].card_code, "GC") != 0) {
    708:     //      snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius, but the card after it is not a GC.", i + 1);
    709:     //      add_error(errors, msg, 1);
    710:     //    }
    711: 
    712:     // GD cards have to follow GN cards
    713:     if (strcmp(code, "GD") == 0 && strcmp(last_code, "GN") != 0)
    714:     {
    715:       snprintf(msg, sizeof(msg), "The card on line %d is a GD, but the card above it is not a GN.", i + 1);
    716:       add_error(ctx, errors, msg, 1);
    717:     }
    718:     if (strcmp(code, "GN") == 0)
    719:     {
    720:       if (i + 1 >= deck->num_cards || strcmp(deck->cards[i + 1].card_code, "GD") != 0)
    721:       {
    722:         snprintf(msg, sizeof(msg), "The card on line %d is a GN, but the card after it is not a GD.", i + 1);
>   723:         add_error(ctx, errors, msg, 1);
    724:       }
    725:     }
    726: 
    727:     // GF cards have to be the first item in the geometry section, which
    728:     // means they must follow CE cards, or in an onec deck, an SY

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L529:         snprintf(msg, sizeof(msg), "The card on line %d is an LD but has fewer than 3 integer inputs (type, tag, segment).", i);
L540:           snprintf(msg, sizeof(msg), "The card on line %d is an LD with unexpected type I1=%d.", i, type);
L545:           snprintf(msg, sizeof(msg), "The card on line %d is an LD with non-positive tag or segment locator.", i);
L550:           snprintf(msg, sizeof(msg), "The card on line %d is an LD with end segment I4 < start segment I3.", i);
L557:         snprintf(msg, sizeof(msg), "The card on line %d is an LD but has no floating-point load value (e.g., resistance).", i);
L562:         snprintf(msg, sizeof(msg), "The card on line %d is an LD with zero load values (F1..F3 all zero).", i);
L613:         snprintf(msg, sizeof(msg), "The card on line %d is an RP, but no FR has been seen earlier.", i + 1);
L647:           snprintf(msg, sizeof(msg), "The card on line %d has geometry code %s, but we already saw the GE on card %d.", i, code, sawGE + 1);
L701:       snprintf(msg, sizeof(msg), "The card on line %d is a GC, but the card above it is not a GW.", i + 1);
L708:     //      snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius, but the card after it is not a GC.", i + 1);
L715:       snprintf(msg, sizeof(msg), "The card on line %d is a GD, but the card above it is not a GN.", i + 1);
L722:         snprintf(msg, sizeof(msg), "The card on line %d is a GN, but the card after it is not a GD.", i + 1);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 733
CALL: add_error(ctx, errors, msg, 1);
--- Context (lines 673-738) ---
    673:             w.line = i + 1;
    674:             w.x1 = deck->cards[i].f[1];
    675:             w.y1 = deck->cards[i].f[2];
    676:             w.z1 = deck->cards[i].f[3];
    677:             w.x2 = deck->cards[i].f[4];
    678:             w.y2 = deck->cards[i].f[5];
    679:             w.z2 = deck->cards[i].f[6];
    680:             w.radius = deck->cards[i].f[7];
    681:             wires[wire_count++] = w;
    682:           }
    683:         }
    684:         break;
    685:       }
    686:     } /* loop over geometry codes */
    687: 
    688:     // unique one here - it's possible to have any number of GS cards, but
    689:     // it appears you can have multiple GS's, although why you would ever do that is unclear
    690:     if (strcmp(code, "GS") == 0)
    691:     {
    692:       if (sawGS == false)
    693:         sawGS = i;
    694:     }
    695: 
    696:     // now we look for card pairs, where one card has to follow another
    697: 
    698:     // GC cards have to follow GW cards
    699:     if (strcmp(code, "GC") == 0 && strcmp(last_code, "GW") != 0)
    700:     {
    701:       snprintf(msg, sizeof(msg), "The card on line %d is a GC, but the card above it is not a GW.", i + 1);
    702:       add_error(ctx, errors, msg, 1);
    703:     }
    704: 
    705:     // FIXME: we should do this, but it has to be calculated first because it might be a formula or units
    706:     //    // GW cards with zero radius have to have a GC after it
    707:     //    if(strcmp(code, "GW") == 0 && deck->cards[i].f[7] == 0.0 && strcmp(deck->cards[i+1].card_code, "GC") != 0) {
    708:     //      snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius, but the card after it is not a GC.", i + 1);
    709:     //      add_error(errors, msg, 1);
    710:     //    }
    711: 
    712:     // GD cards have to follow GN cards
    713:     if (strcmp(code, "GD") == 0 && strcmp(last_code, "GN") != 0)
    714:     {
    715:       snprintf(msg, sizeof(msg), "The card on line %d is a GD, but the card above it is not a GN.", i + 1);
    716:       add_error(ctx, errors, msg, 1);
    717:     }
    718:     if (strcmp(code, "GN") == 0)
    719:     {
    720:       if (i + 1 >= deck->num_cards || strcmp(deck->cards[i + 1].card_code, "GD") != 0)
    721:       {
    722:         snprintf(msg, sizeof(msg), "The card on line %d is a GN, but the card after it is not a GD.", i + 1);
    723:         add_error(ctx, errors, msg, 1);
    724:       }
    725:     }
    726: 
    727:     // GF cards have to be the first item in the geometry section, which
    728:     // means they must follow CE cards, or in an onec deck, an SY
    729:     // FIXME: it could also follow onec comment cards, so this is somewhat complex
    730:     if (strcmp(code, "GF") == 0 && !(strcmp(last_code, "CE") == 0 || strcmp(last_code, "SY") == 0))
    731:     {
    732:       snprintf(msg, sizeof(msg), "The card on line %d is a GF, but the card above it is not a CE or SY.", i + 1);
>   733:       add_error(ctx, errors, msg, 1);
    734:     }
    735: 
    736:     // modifiers must follow normal geometry: GM/GR/GX after GA/GH/GW/SP/CW
    737:     if (strcmp(code, "GM") == 0 || strcmp(code, "GR") == 0 || strcmp(code, "GX") == 0)
    738:     {

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L540:           snprintf(msg, sizeof(msg), "The card on line %d is an LD with unexpected type I1=%d.", i, type);
L545:           snprintf(msg, sizeof(msg), "The card on line %d is an LD with non-positive tag or segment locator.", i);
L550:           snprintf(msg, sizeof(msg), "The card on line %d is an LD with end segment I4 < start segment I3.", i);
L557:         snprintf(msg, sizeof(msg), "The card on line %d is an LD but has no floating-point load value (e.g., resistance).", i);
L562:         snprintf(msg, sizeof(msg), "The card on line %d is an LD with zero load values (F1..F3 all zero).", i);
L613:         snprintf(msg, sizeof(msg), "The card on line %d is an RP, but no FR has been seen earlier.", i + 1);
L647:           snprintf(msg, sizeof(msg), "The card on line %d has geometry code %s, but we already saw the GE on card %d.", i, code, sawGE + 1);
L701:       snprintf(msg, sizeof(msg), "The card on line %d is a GC, but the card above it is not a GW.", i + 1);
L708:     //      snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius, but the card after it is not a GC.", i + 1);
L715:       snprintf(msg, sizeof(msg), "The card on line %d is a GD, but the card above it is not a GN.", i + 1);
L722:         snprintf(msg, sizeof(msg), "The card on line %d is a GN, but the card after it is not a GD.", i + 1);
L732:       snprintf(msg, sizeof(msg), "The card on line %d is a GF, but the card above it is not a CE or SY.", i + 1);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 743
CALL: add_error(ctx, errors, msg, 1);
--- Context (lines 683-748) ---
    683:         }
    684:         break;
    685:       }
    686:     } /* loop over geometry codes */
    687: 
    688:     // unique one here - it's possible to have any number of GS cards, but
    689:     // it appears you can have multiple GS's, although why you would ever do that is unclear
    690:     if (strcmp(code, "GS") == 0)
    691:     {
    692:       if (sawGS == false)
    693:         sawGS = i;
    694:     }
    695: 
    696:     // now we look for card pairs, where one card has to follow another
    697: 
    698:     // GC cards have to follow GW cards
    699:     if (strcmp(code, "GC") == 0 && strcmp(last_code, "GW") != 0)
    700:     {
    701:       snprintf(msg, sizeof(msg), "The card on line %d is a GC, but the card above it is not a GW.", i + 1);
    702:       add_error(ctx, errors, msg, 1);
    703:     }
    704: 
    705:     // FIXME: we should do this, but it has to be calculated first because it might be a formula or units
    706:     //    // GW cards with zero radius have to have a GC after it
    707:     //    if(strcmp(code, "GW") == 0 && deck->cards[i].f[7] == 0.0 && strcmp(deck->cards[i+1].card_code, "GC") != 0) {
    708:     //      snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius, but the card after it is not a GC.", i + 1);
    709:     //      add_error(errors, msg, 1);
    710:     //    }
    711: 
    712:     // GD cards have to follow GN cards
    713:     if (strcmp(code, "GD") == 0 && strcmp(last_code, "GN") != 0)
    714:     {
    715:       snprintf(msg, sizeof(msg), "The card on line %d is a GD, but the card above it is not a GN.", i + 1);
    716:       add_error(ctx, errors, msg, 1);
    717:     }
    718:     if (strcmp(code, "GN") == 0)
    719:     {
    720:       if (i + 1 >= deck->num_cards || strcmp(deck->cards[i + 1].card_code, "GD") != 0)
    721:       {
    722:         snprintf(msg, sizeof(msg), "The card on line %d is a GN, but the card after it is not a GD.", i + 1);
    723:         add_error(ctx, errors, msg, 1);
    724:       }
    725:     }
    726: 
    727:     // GF cards have to be the first item in the geometry section, which
    728:     // means they must follow CE cards, or in an onec deck, an SY
    729:     // FIXME: it could also follow onec comment cards, so this is somewhat complex
    730:     if (strcmp(code, "GF") == 0 && !(strcmp(last_code, "CE") == 0 || strcmp(last_code, "SY") == 0))
    731:     {
    732:       snprintf(msg, sizeof(msg), "The card on line %d is a GF, but the card above it is not a CE or SY.", i + 1);
    733:       add_error(ctx, errors, msg, 1);
    734:     }
    735: 
    736:     // modifiers must follow normal geometry: GM/GR/GX after GA/GH/GW/SP/CW
    737:     if (strcmp(code, "GM") == 0 || strcmp(code, "GR") == 0 || strcmp(code, "GX") == 0)
    738:     {
    739:       if (!(strcmp(last_code, "GA") == 0 || strcmp(last_code, "GH") == 0 || strcmp(last_code, "GW") == 0 ||
    740:             strcmp(last_code, "SP") == 0 || strcmp(last_code, "CW") == 0))
    741:       {
    742:         snprintf(msg, sizeof(msg), "The card on line %d is a %s, but the card above it is not a GA, GH, GW, SP or CW.", i + 1, code);
>   743:         add_error(ctx, errors, msg, 1);
    744:       }
    745:     }
    746: 
    747:     // SC cards have to follow an SP or SM, or another SC. this is not an exhaustive
    748:     // test, it should really roll backward until it finds an SP or SM, but...

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L545:           snprintf(msg, sizeof(msg), "The card on line %d is an LD with non-positive tag or segment locator.", i);
L550:           snprintf(msg, sizeof(msg), "The card on line %d is an LD with end segment I4 < start segment I3.", i);
L557:         snprintf(msg, sizeof(msg), "The card on line %d is an LD but has no floating-point load value (e.g., resistance).", i);
L562:         snprintf(msg, sizeof(msg), "The card on line %d is an LD with zero load values (F1..F3 all zero).", i);
L613:         snprintf(msg, sizeof(msg), "The card on line %d is an RP, but no FR has been seen earlier.", i + 1);
L647:           snprintf(msg, sizeof(msg), "The card on line %d has geometry code %s, but we already saw the GE on card %d.", i, code, sawGE + 1);
L701:       snprintf(msg, sizeof(msg), "The card on line %d is a GC, but the card above it is not a GW.", i + 1);
L708:     //      snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius, but the card after it is not a GC.", i + 1);
L715:       snprintf(msg, sizeof(msg), "The card on line %d is a GD, but the card above it is not a GN.", i + 1);
L722:         snprintf(msg, sizeof(msg), "The card on line %d is a GN, but the card after it is not a GD.", i + 1);
L732:       snprintf(msg, sizeof(msg), "The card on line %d is a GF, but the card above it is not a CE or SY.", i + 1);
L742:         snprintf(msg, sizeof(msg), "The card on line %d is a %s, but the card above it is not a GA, GH, GW, SP or CW.", i + 1, code);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 752
CALL: add_error(ctx, errors, msg, 1);
--- Context (lines 692-757) ---
    692:       if (sawGS == false)
    693:         sawGS = i;
    694:     }
    695: 
    696:     // now we look for card pairs, where one card has to follow another
    697: 
    698:     // GC cards have to follow GW cards
    699:     if (strcmp(code, "GC") == 0 && strcmp(last_code, "GW") != 0)
    700:     {
    701:       snprintf(msg, sizeof(msg), "The card on line %d is a GC, but the card above it is not a GW.", i + 1);
    702:       add_error(ctx, errors, msg, 1);
    703:     }
    704: 
    705:     // FIXME: we should do this, but it has to be calculated first because it might be a formula or units
    706:     //    // GW cards with zero radius have to have a GC after it
    707:     //    if(strcmp(code, "GW") == 0 && deck->cards[i].f[7] == 0.0 && strcmp(deck->cards[i+1].card_code, "GC") != 0) {
    708:     //      snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius, but the card after it is not a GC.", i + 1);
    709:     //      add_error(errors, msg, 1);
    710:     //    }
    711: 
    712:     // GD cards have to follow GN cards
    713:     if (strcmp(code, "GD") == 0 && strcmp(last_code, "GN") != 0)
    714:     {
    715:       snprintf(msg, sizeof(msg), "The card on line %d is a GD, but the card above it is not a GN.", i + 1);
    716:       add_error(ctx, errors, msg, 1);
    717:     }
    718:     if (strcmp(code, "GN") == 0)
    719:     {
    720:       if (i + 1 >= deck->num_cards || strcmp(deck->cards[i + 1].card_code, "GD") != 0)
    721:       {
    722:         snprintf(msg, sizeof(msg), "The card on line %d is a GN, but the card after it is not a GD.", i + 1);
    723:         add_error(ctx, errors, msg, 1);
    724:       }
    725:     }
    726: 
    727:     // GF cards have to be the first item in the geometry section, which
    728:     // means they must follow CE cards, or in an onec deck, an SY
    729:     // FIXME: it could also follow onec comment cards, so this is somewhat complex
    730:     if (strcmp(code, "GF") == 0 && !(strcmp(last_code, "CE") == 0 || strcmp(last_code, "SY") == 0))
    731:     {
    732:       snprintf(msg, sizeof(msg), "The card on line %d is a GF, but the card above it is not a CE or SY.", i + 1);
    733:       add_error(ctx, errors, msg, 1);
    734:     }
    735: 
    736:     // modifiers must follow normal geometry: GM/GR/GX after GA/GH/GW/SP/CW
    737:     if (strcmp(code, "GM") == 0 || strcmp(code, "GR") == 0 || strcmp(code, "GX") == 0)
    738:     {
    739:       if (!(strcmp(last_code, "GA") == 0 || strcmp(last_code, "GH") == 0 || strcmp(last_code, "GW") == 0 ||
    740:             strcmp(last_code, "SP") == 0 || strcmp(last_code, "CW") == 0))
    741:       {
    742:         snprintf(msg, sizeof(msg), "The card on line %d is a %s, but the card above it is not a GA, GH, GW, SP or CW.", i + 1, code);
    743:         add_error(ctx, errors, msg, 1);
    744:       }
    745:     }
    746: 
    747:     // SC cards have to follow an SP or SM, or another SC. this is not an exhaustive
    748:     // test, it should really roll backward until it finds an SP or SM, but...
    749:     if (strcmp(code, "SC") == 0 && !(strcmp(last_code, "SP") == 0 || strcmp(last_code, "SM") == 0 || strcmp(last_code, "SC") == 0))
    750:     {
    751:       snprintf(msg, sizeof(msg), "The card on line %d is an SC, but the card above it is not an SP, SM or another SC.", i + 1);
>   752:       add_error(ctx, errors, msg, 1);
    753:     }
    754:     // if SC follows another SC, ensure there was an SP or SM earlier in the deck
    755:     if (strcmp(code, "SC") == 0 && strcmp(last_code, "SC") == 0)
    756:     {
    757:       int hasAncestor = 0;

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L557:         snprintf(msg, sizeof(msg), "The card on line %d is an LD but has no floating-point load value (e.g., resistance).", i);
L562:         snprintf(msg, sizeof(msg), "The card on line %d is an LD with zero load values (F1..F3 all zero).", i);
L613:         snprintf(msg, sizeof(msg), "The card on line %d is an RP, but no FR has been seen earlier.", i + 1);
L647:           snprintf(msg, sizeof(msg), "The card on line %d has geometry code %s, but we already saw the GE on card %d.", i, code, sawGE + 1);
L701:       snprintf(msg, sizeof(msg), "The card on line %d is a GC, but the card above it is not a GW.", i + 1);
L708:     //      snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius, but the card after it is not a GC.", i + 1);
L715:       snprintf(msg, sizeof(msg), "The card on line %d is a GD, but the card above it is not a GN.", i + 1);
L722:         snprintf(msg, sizeof(msg), "The card on line %d is a GN, but the card after it is not a GD.", i + 1);
L732:       snprintf(msg, sizeof(msg), "The card on line %d is a GF, but the card above it is not a CE or SY.", i + 1);
L742:         snprintf(msg, sizeof(msg), "The card on line %d is a %s, but the card above it is not a GA, GH, GW, SP or CW.", i + 1, code);
L751:       snprintf(msg, sizeof(msg), "The card on line %d is an SC, but the card above it is not an SP, SM or another SC.", i + 1);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 772
CALL: add_error(ctx, errors, msg, 1);
--- Context (lines 712-777) ---
    712:     // GD cards have to follow GN cards
    713:     if (strcmp(code, "GD") == 0 && strcmp(last_code, "GN") != 0)
    714:     {
    715:       snprintf(msg, sizeof(msg), "The card on line %d is a GD, but the card above it is not a GN.", i + 1);
    716:       add_error(ctx, errors, msg, 1);
    717:     }
    718:     if (strcmp(code, "GN") == 0)
    719:     {
    720:       if (i + 1 >= deck->num_cards || strcmp(deck->cards[i + 1].card_code, "GD") != 0)
    721:       {
    722:         snprintf(msg, sizeof(msg), "The card on line %d is a GN, but the card after it is not a GD.", i + 1);
    723:         add_error(ctx, errors, msg, 1);
    724:       }
    725:     }
    726: 
    727:     // GF cards have to be the first item in the geometry section, which
    728:     // means they must follow CE cards, or in an onec deck, an SY
    729:     // FIXME: it could also follow onec comment cards, so this is somewhat complex
    730:     if (strcmp(code, "GF") == 0 && !(strcmp(last_code, "CE") == 0 || strcmp(last_code, "SY") == 0))
    731:     {
    732:       snprintf(msg, sizeof(msg), "The card on line %d is a GF, but the card above it is not a CE or SY.", i + 1);
    733:       add_error(ctx, errors, msg, 1);
    734:     }
    735: 
    736:     // modifiers must follow normal geometry: GM/GR/GX after GA/GH/GW/SP/CW
    737:     if (strcmp(code, "GM") == 0 || strcmp(code, "GR") == 0 || strcmp(code, "GX") == 0)
    738:     {
    739:       if (!(strcmp(last_code, "GA") == 0 || strcmp(last_code, "GH") == 0 || strcmp(last_code, "GW") == 0 ||
    740:             strcmp(last_code, "SP") == 0 || strcmp(last_code, "CW") == 0))
    741:       {
    742:         snprintf(msg, sizeof(msg), "The card on line %d is a %s, but the card above it is not a GA, GH, GW, SP or CW.", i + 1, code);
    743:         add_error(ctx, errors, msg, 1);
    744:       }
    745:     }
    746: 
    747:     // SC cards have to follow an SP or SM, or another SC. this is not an exhaustive
    748:     // test, it should really roll backward until it finds an SP or SM, but...
    749:     if (strcmp(code, "SC") == 0 && !(strcmp(last_code, "SP") == 0 || strcmp(last_code, "SM") == 0 || strcmp(last_code, "SC") == 0))
    750:     {
    751:       snprintf(msg, sizeof(msg), "The card on line %d is an SC, but the card above it is not an SP, SM or another SC.", i + 1);
    752:       add_error(ctx, errors, msg, 1);
    753:     }
    754:     // if SC follows another SC, ensure there was an SP or SM earlier in the deck
    755:     if (strcmp(code, "SC") == 0 && strcmp(last_code, "SC") == 0)
    756:     {
    757:       int hasAncestor = 0;
    758:       for (int j = i - 2; j >= 0; j--)
    759:       {
    760:         if (strcmp(deck->cards[j].card_code, "SP") == 0 || strcmp(deck->cards[j].card_code, "SM") == 0)
    761:         {
    762:           hasAncestor = 1;
    763:           break;
    764:         }
    765:         // stop if we reach a GE; SC ancestry should be within geometry section
    766:         if (strcmp(deck->cards[j].card_code, "GE") == 0)
    767:           break;
    768:       }
    769:       if (!hasAncestor)
    770:       {
    771:         snprintf(msg, sizeof(msg), "The card on line %d is an SC following an SC, but no earlier SP or SM was found before this SC chain.", i + 1);
>   772:         add_error(ctx, errors, msg, 1);
    773:       }
    774:     }
    775:     // SM cards should follow an SP or another SM
    776:     if (strcmp(code, "SM") == 0 && !(strcmp(last_code, "SP") == 0 || strcmp(last_code, "SM") == 0))
    777:     {

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L613:         snprintf(msg, sizeof(msg), "The card on line %d is an RP, but no FR has been seen earlier.", i + 1);
L647:           snprintf(msg, sizeof(msg), "The card on line %d has geometry code %s, but we already saw the GE on card %d.", i, code, sawGE + 1);
L701:       snprintf(msg, sizeof(msg), "The card on line %d is a GC, but the card above it is not a GW.", i + 1);
L708:     //      snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius, but the card after it is not a GC.", i + 1);
L715:       snprintf(msg, sizeof(msg), "The card on line %d is a GD, but the card above it is not a GN.", i + 1);
L722:         snprintf(msg, sizeof(msg), "The card on line %d is a GN, but the card after it is not a GD.", i + 1);
L732:       snprintf(msg, sizeof(msg), "The card on line %d is a GF, but the card above it is not a CE or SY.", i + 1);
L742:         snprintf(msg, sizeof(msg), "The card on line %d is a %s, but the card above it is not a GA, GH, GW, SP or CW.", i + 1, code);
L751:       snprintf(msg, sizeof(msg), "The card on line %d is an SC, but the card above it is not an SP, SM or another SC.", i + 1);
L771:         snprintf(msg, sizeof(msg), "The card on line %d is an SC following an SC, but no earlier SP or SM was found before this SC chain.", i + 1);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 779
CALL: add_error(ctx, errors, msg, 1);
--- Context (lines 719-784) ---
    719:     {
    720:       if (i + 1 >= deck->num_cards || strcmp(deck->cards[i + 1].card_code, "GD") != 0)
    721:       {
    722:         snprintf(msg, sizeof(msg), "The card on line %d is a GN, but the card after it is not a GD.", i + 1);
    723:         add_error(ctx, errors, msg, 1);
    724:       }
    725:     }
    726: 
    727:     // GF cards have to be the first item in the geometry section, which
    728:     // means they must follow CE cards, or in an onec deck, an SY
    729:     // FIXME: it could also follow onec comment cards, so this is somewhat complex
    730:     if (strcmp(code, "GF") == 0 && !(strcmp(last_code, "CE") == 0 || strcmp(last_code, "SY") == 0))
    731:     {
    732:       snprintf(msg, sizeof(msg), "The card on line %d is a GF, but the card above it is not a CE or SY.", i + 1);
    733:       add_error(ctx, errors, msg, 1);
    734:     }
    735: 
    736:     // modifiers must follow normal geometry: GM/GR/GX after GA/GH/GW/SP/CW
    737:     if (strcmp(code, "GM") == 0 || strcmp(code, "GR") == 0 || strcmp(code, "GX") == 0)
    738:     {
    739:       if (!(strcmp(last_code, "GA") == 0 || strcmp(last_code, "GH") == 0 || strcmp(last_code, "GW") == 0 ||
    740:             strcmp(last_code, "SP") == 0 || strcmp(last_code, "CW") == 0))
    741:       {
    742:         snprintf(msg, sizeof(msg), "The card on line %d is a %s, but the card above it is not a GA, GH, GW, SP or CW.", i + 1, code);
    743:         add_error(ctx, errors, msg, 1);
    744:       }
    745:     }
    746: 
    747:     // SC cards have to follow an SP or SM, or another SC. this is not an exhaustive
    748:     // test, it should really roll backward until it finds an SP or SM, but...
    749:     if (strcmp(code, "SC") == 0 && !(strcmp(last_code, "SP") == 0 || strcmp(last_code, "SM") == 0 || strcmp(last_code, "SC") == 0))
    750:     {
    751:       snprintf(msg, sizeof(msg), "The card on line %d is an SC, but the card above it is not an SP, SM or another SC.", i + 1);
    752:       add_error(ctx, errors, msg, 1);
    753:     }
    754:     // if SC follows another SC, ensure there was an SP or SM earlier in the deck
    755:     if (strcmp(code, "SC") == 0 && strcmp(last_code, "SC") == 0)
    756:     {
    757:       int hasAncestor = 0;
    758:       for (int j = i - 2; j >= 0; j--)
    759:       {
    760:         if (strcmp(deck->cards[j].card_code, "SP") == 0 || strcmp(deck->cards[j].card_code, "SM") == 0)
    761:         {
    762:           hasAncestor = 1;
    763:           break;
    764:         }
    765:         // stop if we reach a GE; SC ancestry should be within geometry section
    766:         if (strcmp(deck->cards[j].card_code, "GE") == 0)
    767:           break;
    768:       }
    769:       if (!hasAncestor)
    770:       {
    771:         snprintf(msg, sizeof(msg), "The card on line %d is an SC following an SC, but no earlier SP or SM was found before this SC chain.", i + 1);
    772:         add_error(ctx, errors, msg, 1);
    773:       }
    774:     }
    775:     // SM cards should follow an SP or another SM
    776:     if (strcmp(code, "SM") == 0 && !(strcmp(last_code, "SP") == 0 || strcmp(last_code, "SM") == 0))
    777:     {
    778:       snprintf(msg, sizeof(msg), "The card on line %d is an SM, but the card above it is not an SP or another SM.", i + 1);
>   779:       add_error(ctx, errors, msg, 1);
    780:     }
    781:     // SM must be immediately followed by SC (set pending on SM, clear on next non-SC)
    782:     if (strcmp(code, "SM") == 0)
    783:     {
    784:       pendingSM = i + 1; // store 1-based line number of SM

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L613:         snprintf(msg, sizeof(msg), "The card on line %d is an RP, but no FR has been seen earlier.", i + 1);
L647:           snprintf(msg, sizeof(msg), "The card on line %d has geometry code %s, but we already saw the GE on card %d.", i, code, sawGE + 1);
L701:       snprintf(msg, sizeof(msg), "The card on line %d is a GC, but the card above it is not a GW.", i + 1);
L708:     //      snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius, but the card after it is not a GC.", i + 1);
L715:       snprintf(msg, sizeof(msg), "The card on line %d is a GD, but the card above it is not a GN.", i + 1);
L722:         snprintf(msg, sizeof(msg), "The card on line %d is a GN, but the card after it is not a GD.", i + 1);
L732:       snprintf(msg, sizeof(msg), "The card on line %d is a GF, but the card above it is not a CE or SY.", i + 1);
L742:         snprintf(msg, sizeof(msg), "The card on line %d is a %s, but the card above it is not a GA, GH, GW, SP or CW.", i + 1, code);
L751:       snprintf(msg, sizeof(msg), "The card on line %d is an SC, but the card above it is not an SP, SM or another SC.", i + 1);
L771:         snprintf(msg, sizeof(msg), "The card on line %d is an SC following an SC, but no earlier SP or SM was found before this SC chain.", i + 1);
L778:       snprintf(msg, sizeof(msg), "The card on line %d is an SM, but the card above it is not an SP or another SM.", i + 1);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 791
CALL: add_error(ctx, errors, msg, 1);
--- Context (lines 731-796) ---
    731:     {
    732:       snprintf(msg, sizeof(msg), "The card on line %d is a GF, but the card above it is not a CE or SY.", i + 1);
    733:       add_error(ctx, errors, msg, 1);
    734:     }
    735: 
    736:     // modifiers must follow normal geometry: GM/GR/GX after GA/GH/GW/SP/CW
    737:     if (strcmp(code, "GM") == 0 || strcmp(code, "GR") == 0 || strcmp(code, "GX") == 0)
    738:     {
    739:       if (!(strcmp(last_code, "GA") == 0 || strcmp(last_code, "GH") == 0 || strcmp(last_code, "GW") == 0 ||
    740:             strcmp(last_code, "SP") == 0 || strcmp(last_code, "CW") == 0))
    741:       {
    742:         snprintf(msg, sizeof(msg), "The card on line %d is a %s, but the card above it is not a GA, GH, GW, SP or CW.", i + 1, code);
    743:         add_error(ctx, errors, msg, 1);
    744:       }
    745:     }
    746: 
    747:     // SC cards have to follow an SP or SM, or another SC. this is not an exhaustive
    748:     // test, it should really roll backward until it finds an SP or SM, but...
    749:     if (strcmp(code, "SC") == 0 && !(strcmp(last_code, "SP") == 0 || strcmp(last_code, "SM") == 0 || strcmp(last_code, "SC") == 0))
    750:     {
    751:       snprintf(msg, sizeof(msg), "The card on line %d is an SC, but the card above it is not an SP, SM or another SC.", i + 1);
    752:       add_error(ctx, errors, msg, 1);
    753:     }
    754:     // if SC follows another SC, ensure there was an SP or SM earlier in the deck
    755:     if (strcmp(code, "SC") == 0 && strcmp(last_code, "SC") == 0)
    756:     {
    757:       int hasAncestor = 0;
    758:       for (int j = i - 2; j >= 0; j--)
    759:       {
    760:         if (strcmp(deck->cards[j].card_code, "SP") == 0 || strcmp(deck->cards[j].card_code, "SM") == 0)
    761:         {
    762:           hasAncestor = 1;
    763:           break;
    764:         }
    765:         // stop if we reach a GE; SC ancestry should be within geometry section
    766:         if (strcmp(deck->cards[j].card_code, "GE") == 0)
    767:           break;
    768:       }
    769:       if (!hasAncestor)
    770:       {
    771:         snprintf(msg, sizeof(msg), "The card on line %d is an SC following an SC, but no earlier SP or SM was found before this SC chain.", i + 1);
    772:         add_error(ctx, errors, msg, 1);
    773:       }
    774:     }
    775:     // SM cards should follow an SP or another SM
    776:     if (strcmp(code, "SM") == 0 && !(strcmp(last_code, "SP") == 0 || strcmp(last_code, "SM") == 0))
    777:     {
    778:       snprintf(msg, sizeof(msg), "The card on line %d is an SM, but the card above it is not an SP or another SM.", i + 1);
    779:       add_error(ctx, errors, msg, 1);
    780:     }
    781:     // SM must be immediately followed by SC (set pending on SM, clear on next non-SC)
    782:     if (strcmp(code, "SM") == 0)
    783:     {
    784:       pendingSM = i + 1; // store 1-based line number of SM
    785:     }
    786:     else if (pendingSM)
    787:     {
    788:       if (strcmp(code, "SC") != 0)
    789:       {
    790:         snprintf(msg, sizeof(msg), "The card on line %d is an SM, but the card after it is not an SC.", pendingSM);
>   791:         add_error(ctx, errors, msg, 1);
    792:       }
    793:       pendingSM = 0;
    794:     }
    795: 
    796:     // QUESTION: it appears GR cards have to follow some other sort of geometry card, but it's not clear what

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L613:         snprintf(msg, sizeof(msg), "The card on line %d is an RP, but no FR has been seen earlier.", i + 1);
L647:           snprintf(msg, sizeof(msg), "The card on line %d has geometry code %s, but we already saw the GE on card %d.", i, code, sawGE + 1);
L701:       snprintf(msg, sizeof(msg), "The card on line %d is a GC, but the card above it is not a GW.", i + 1);
L708:     //      snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius, but the card after it is not a GC.", i + 1);
L715:       snprintf(msg, sizeof(msg), "The card on line %d is a GD, but the card above it is not a GN.", i + 1);
L722:         snprintf(msg, sizeof(msg), "The card on line %d is a GN, but the card after it is not a GD.", i + 1);
L732:       snprintf(msg, sizeof(msg), "The card on line %d is a GF, but the card above it is not a CE or SY.", i + 1);
L742:         snprintf(msg, sizeof(msg), "The card on line %d is a %s, but the card above it is not a GA, GH, GW, SP or CW.", i + 1, code);
L751:       snprintf(msg, sizeof(msg), "The card on line %d is an SC, but the card above it is not an SP, SM or another SC.", i + 1);
L771:         snprintf(msg, sizeof(msg), "The card on line %d is an SC following an SC, but no earlier SP or SM was found before this SC chain.", i + 1);
L778:       snprintf(msg, sizeof(msg), "The card on line %d is an SM, but the card above it is not an SP or another SM.", i + 1);
L790:         snprintf(msg, sizeof(msg), "The card on line %d is an SM, but the card after it is not an SC.", pendingSM);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 804
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 744-809) ---
    744:       }
    745:     }
    746: 
    747:     // SC cards have to follow an SP or SM, or another SC. this is not an exhaustive
    748:     // test, it should really roll backward until it finds an SP or SM, but...
    749:     if (strcmp(code, "SC") == 0 && !(strcmp(last_code, "SP") == 0 || strcmp(last_code, "SM") == 0 || strcmp(last_code, "SC") == 0))
    750:     {
    751:       snprintf(msg, sizeof(msg), "The card on line %d is an SC, but the card above it is not an SP, SM or another SC.", i + 1);
    752:       add_error(ctx, errors, msg, 1);
    753:     }
    754:     // if SC follows another SC, ensure there was an SP or SM earlier in the deck
    755:     if (strcmp(code, "SC") == 0 && strcmp(last_code, "SC") == 0)
    756:     {
    757:       int hasAncestor = 0;
    758:       for (int j = i - 2; j >= 0; j--)
    759:       {
    760:         if (strcmp(deck->cards[j].card_code, "SP") == 0 || strcmp(deck->cards[j].card_code, "SM") == 0)
    761:         {
    762:           hasAncestor = 1;
    763:           break;
    764:         }
    765:         // stop if we reach a GE; SC ancestry should be within geometry section
    766:         if (strcmp(deck->cards[j].card_code, "GE") == 0)
    767:           break;
    768:       }
    769:       if (!hasAncestor)
    770:       {
    771:         snprintf(msg, sizeof(msg), "The card on line %d is an SC following an SC, but no earlier SP or SM was found before this SC chain.", i + 1);
    772:         add_error(ctx, errors, msg, 1);
    773:       }
    774:     }
    775:     // SM cards should follow an SP or another SM
    776:     if (strcmp(code, "SM") == 0 && !(strcmp(last_code, "SP") == 0 || strcmp(last_code, "SM") == 0))
    777:     {
    778:       snprintf(msg, sizeof(msg), "The card on line %d is an SM, but the card above it is not an SP or another SM.", i + 1);
    779:       add_error(ctx, errors, msg, 1);
    780:     }
    781:     // SM must be immediately followed by SC (set pending on SM, clear on next non-SC)
    782:     if (strcmp(code, "SM") == 0)
    783:     {
    784:       pendingSM = i + 1; // store 1-based line number of SM
    785:     }
    786:     else if (pendingSM)
    787:     {
    788:       if (strcmp(code, "SC") != 0)
    789:       {
    790:         snprintf(msg, sizeof(msg), "The card on line %d is an SM, but the card after it is not an SC.", pendingSM);
    791:         add_error(ctx, errors, msg, 1);
    792:       }
    793:       pendingSM = 0;
    794:     }
    795: 
    796:     // QUESTION: it appears GR cards have to follow some other sort of geometry card, but it's not clear what
    797: 
    798:     // FR cards have to have either one input or three
    799:     if (strcmp(code, "FR") == 0)
    800:     {
    801:       if (!(deck->cards[i].ints_used == 1 || deck->cards[i].ints_used == 3))
    802:       {
    803:         snprintf(msg, sizeof(msg), "The card on line %d is an FR but does not have 1 or 3 integer inputs.", i + 1);
>   804:         add_error(ctx, errors, msg, 0);
    805:       }
    806:     }
    807:     // advance last_code to current card for next-iteration pair checks
    808:     last_code = code;
    809: 

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L613:         snprintf(msg, sizeof(msg), "The card on line %d is an RP, but no FR has been seen earlier.", i + 1);
L647:           snprintf(msg, sizeof(msg), "The card on line %d has geometry code %s, but we already saw the GE on card %d.", i, code, sawGE + 1);
L701:       snprintf(msg, sizeof(msg), "The card on line %d is a GC, but the card above it is not a GW.", i + 1);
L708:     //      snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius, but the card after it is not a GC.", i + 1);
L715:       snprintf(msg, sizeof(msg), "The card on line %d is a GD, but the card above it is not a GN.", i + 1);
L722:         snprintf(msg, sizeof(msg), "The card on line %d is a GN, but the card after it is not a GD.", i + 1);
L732:       snprintf(msg, sizeof(msg), "The card on line %d is a GF, but the card above it is not a CE or SY.", i + 1);
L742:         snprintf(msg, sizeof(msg), "The card on line %d is a %s, but the card above it is not a GA, GH, GW, SP or CW.", i + 1, code);
L751:       snprintf(msg, sizeof(msg), "The card on line %d is an SC, but the card above it is not an SP, SM or another SC.", i + 1);
L771:         snprintf(msg, sizeof(msg), "The card on line %d is an SC following an SC, but no earlier SP or SM was found before this SC chain.", i + 1);
L778:       snprintf(msg, sizeof(msg), "The card on line %d is an SM, but the card above it is not an SP or another SM.", i + 1);
L790:         snprintf(msg, sizeof(msg), "The card on line %d is an SM, but the card after it is not an SC.", pendingSM);
L803:         snprintf(msg, sizeof(msg), "The card on line %d is an FR but does not have 1 or 3 integer inputs.", i + 1);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 814
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 754-819) ---
    754:     // if SC follows another SC, ensure there was an SP or SM earlier in the deck
    755:     if (strcmp(code, "SC") == 0 && strcmp(last_code, "SC") == 0)
    756:     {
    757:       int hasAncestor = 0;
    758:       for (int j = i - 2; j >= 0; j--)
    759:       {
    760:         if (strcmp(deck->cards[j].card_code, "SP") == 0 || strcmp(deck->cards[j].card_code, "SM") == 0)
    761:         {
    762:           hasAncestor = 1;
    763:           break;
    764:         }
    765:         // stop if we reach a GE; SC ancestry should be within geometry section
    766:         if (strcmp(deck->cards[j].card_code, "GE") == 0)
    767:           break;
    768:       }
    769:       if (!hasAncestor)
    770:       {
    771:         snprintf(msg, sizeof(msg), "The card on line %d is an SC following an SC, but no earlier SP or SM was found before this SC chain.", i + 1);
    772:         add_error(ctx, errors, msg, 1);
    773:       }
    774:     }
    775:     // SM cards should follow an SP or another SM
    776:     if (strcmp(code, "SM") == 0 && !(strcmp(last_code, "SP") == 0 || strcmp(last_code, "SM") == 0))
    777:     {
    778:       snprintf(msg, sizeof(msg), "The card on line %d is an SM, but the card above it is not an SP or another SM.", i + 1);
    779:       add_error(ctx, errors, msg, 1);
    780:     }
    781:     // SM must be immediately followed by SC (set pending on SM, clear on next non-SC)
    782:     if (strcmp(code, "SM") == 0)
    783:     {
    784:       pendingSM = i + 1; // store 1-based line number of SM
    785:     }
    786:     else if (pendingSM)
    787:     {
    788:       if (strcmp(code, "SC") != 0)
    789:       {
    790:         snprintf(msg, sizeof(msg), "The card on line %d is an SM, but the card after it is not an SC.", pendingSM);
    791:         add_error(ctx, errors, msg, 1);
    792:       }
    793:       pendingSM = 0;
    794:     }
    795: 
    796:     // QUESTION: it appears GR cards have to follow some other sort of geometry card, but it's not clear what
    797: 
    798:     // FR cards have to have either one input or three
    799:     if (strcmp(code, "FR") == 0)
    800:     {
    801:       if (!(deck->cards[i].ints_used == 1 || deck->cards[i].ints_used == 3))
    802:       {
    803:         snprintf(msg, sizeof(msg), "The card on line %d is an FR but does not have 1 or 3 integer inputs.", i + 1);
    804:         add_error(ctx, errors, msg, 0);
    805:       }
    806:     }
    807:     // advance last_code to current card for next-iteration pair checks
    808:     last_code = code;
    809: 
    810:     // warn about unsupported cards
    811:     if (strcmp(code, "WG") == 0)
    812:     {
    813:       snprintf(msg, sizeof(msg), "The card on line %d is a WG (Wire Grid), which is not supported by OpenNEC.", i + 1);
>   814:       add_error(ctx, errors, msg, 0);
    815:     }
    816:     if (strcmp(code, "IT") == 0)
    817:     {
    818:       snprintf(msg, sizeof(msg), "The card on line %d is an IT (ITeration), which is not supported by OpenNEC.", i + 1);
    819:       add_error(ctx, errors, msg, 0);

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L647:           snprintf(msg, sizeof(msg), "The card on line %d has geometry code %s, but we already saw the GE on card %d.", i, code, sawGE + 1);
L701:       snprintf(msg, sizeof(msg), "The card on line %d is a GC, but the card above it is not a GW.", i + 1);
L708:     //      snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius, but the card after it is not a GC.", i + 1);
L715:       snprintf(msg, sizeof(msg), "The card on line %d is a GD, but the card above it is not a GN.", i + 1);
L722:         snprintf(msg, sizeof(msg), "The card on line %d is a GN, but the card after it is not a GD.", i + 1);
L732:       snprintf(msg, sizeof(msg), "The card on line %d is a GF, but the card above it is not a CE or SY.", i + 1);
L742:         snprintf(msg, sizeof(msg), "The card on line %d is a %s, but the card above it is not a GA, GH, GW, SP or CW.", i + 1, code);
L751:       snprintf(msg, sizeof(msg), "The card on line %d is an SC, but the card above it is not an SP, SM or another SC.", i + 1);
L771:         snprintf(msg, sizeof(msg), "The card on line %d is an SC following an SC, but no earlier SP or SM was found before this SC chain.", i + 1);
L778:       snprintf(msg, sizeof(msg), "The card on line %d is an SM, but the card above it is not an SP or another SM.", i + 1);
L790:         snprintf(msg, sizeof(msg), "The card on line %d is an SM, but the card after it is not an SC.", pendingSM);
L803:         snprintf(msg, sizeof(msg), "The card on line %d is an FR but does not have 1 or 3 integer inputs.", i + 1);
L813:       snprintf(msg, sizeof(msg), "The card on line %d is a WG (Wire Grid), which is not supported by OpenNEC.", i + 1);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 819
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 759-824) ---
    759:       {
    760:         if (strcmp(deck->cards[j].card_code, "SP") == 0 || strcmp(deck->cards[j].card_code, "SM") == 0)
    761:         {
    762:           hasAncestor = 1;
    763:           break;
    764:         }
    765:         // stop if we reach a GE; SC ancestry should be within geometry section
    766:         if (strcmp(deck->cards[j].card_code, "GE") == 0)
    767:           break;
    768:       }
    769:       if (!hasAncestor)
    770:       {
    771:         snprintf(msg, sizeof(msg), "The card on line %d is an SC following an SC, but no earlier SP or SM was found before this SC chain.", i + 1);
    772:         add_error(ctx, errors, msg, 1);
    773:       }
    774:     }
    775:     // SM cards should follow an SP or another SM
    776:     if (strcmp(code, "SM") == 0 && !(strcmp(last_code, "SP") == 0 || strcmp(last_code, "SM") == 0))
    777:     {
    778:       snprintf(msg, sizeof(msg), "The card on line %d is an SM, but the card above it is not an SP or another SM.", i + 1);
    779:       add_error(ctx, errors, msg, 1);
    780:     }
    781:     // SM must be immediately followed by SC (set pending on SM, clear on next non-SC)
    782:     if (strcmp(code, "SM") == 0)
    783:     {
    784:       pendingSM = i + 1; // store 1-based line number of SM
    785:     }
    786:     else if (pendingSM)
    787:     {
    788:       if (strcmp(code, "SC") != 0)
    789:       {
    790:         snprintf(msg, sizeof(msg), "The card on line %d is an SM, but the card after it is not an SC.", pendingSM);
    791:         add_error(ctx, errors, msg, 1);
    792:       }
    793:       pendingSM = 0;
    794:     }
    795: 
    796:     // QUESTION: it appears GR cards have to follow some other sort of geometry card, but it's not clear what
    797: 
    798:     // FR cards have to have either one input or three
    799:     if (strcmp(code, "FR") == 0)
    800:     {
    801:       if (!(deck->cards[i].ints_used == 1 || deck->cards[i].ints_used == 3))
    802:       {
    803:         snprintf(msg, sizeof(msg), "The card on line %d is an FR but does not have 1 or 3 integer inputs.", i + 1);
    804:         add_error(ctx, errors, msg, 0);
    805:       }
    806:     }
    807:     // advance last_code to current card for next-iteration pair checks
    808:     last_code = code;
    809: 
    810:     // warn about unsupported cards
    811:     if (strcmp(code, "WG") == 0)
    812:     {
    813:       snprintf(msg, sizeof(msg), "The card on line %d is a WG (Wire Grid), which is not supported by OpenNEC.", i + 1);
    814:       add_error(ctx, errors, msg, 0);
    815:     }
    816:     if (strcmp(code, "IT") == 0)
    817:     {
    818:       snprintf(msg, sizeof(msg), "The card on line %d is an IT (ITeration), which is not supported by OpenNEC.", i + 1);
>   819:       add_error(ctx, errors, msg, 0);
    820:     }
    821:     if (strcmp(code, "OP") == 0)
    822:     {
    823:       snprintf(msg, sizeof(msg), "The card on line %d is an OP (OPtimization), which is not supported by OpenNEC.", i + 1);
    824:       add_error(ctx, errors, msg, 0);

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L647:           snprintf(msg, sizeof(msg), "The card on line %d has geometry code %s, but we already saw the GE on card %d.", i, code, sawGE + 1);
L701:       snprintf(msg, sizeof(msg), "The card on line %d is a GC, but the card above it is not a GW.", i + 1);
L708:     //      snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius, but the card after it is not a GC.", i + 1);
L715:       snprintf(msg, sizeof(msg), "The card on line %d is a GD, but the card above it is not a GN.", i + 1);
L722:         snprintf(msg, sizeof(msg), "The card on line %d is a GN, but the card after it is not a GD.", i + 1);
L732:       snprintf(msg, sizeof(msg), "The card on line %d is a GF, but the card above it is not a CE or SY.", i + 1);
L742:         snprintf(msg, sizeof(msg), "The card on line %d is a %s, but the card above it is not a GA, GH, GW, SP or CW.", i + 1, code);
L751:       snprintf(msg, sizeof(msg), "The card on line %d is an SC, but the card above it is not an SP, SM or another SC.", i + 1);
L771:         snprintf(msg, sizeof(msg), "The card on line %d is an SC following an SC, but no earlier SP or SM was found before this SC chain.", i + 1);
L778:       snprintf(msg, sizeof(msg), "The card on line %d is an SM, but the card above it is not an SP or another SM.", i + 1);
L790:         snprintf(msg, sizeof(msg), "The card on line %d is an SM, but the card after it is not an SC.", pendingSM);
L803:         snprintf(msg, sizeof(msg), "The card on line %d is an FR but does not have 1 or 3 integer inputs.", i + 1);
L813:       snprintf(msg, sizeof(msg), "The card on line %d is a WG (Wire Grid), which is not supported by OpenNEC.", i + 1);
L818:       snprintf(msg, sizeof(msg), "The card on line %d is an IT (ITeration), which is not supported by OpenNEC.", i + 1);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 824
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 764-829) ---
    764:         }
    765:         // stop if we reach a GE; SC ancestry should be within geometry section
    766:         if (strcmp(deck->cards[j].card_code, "GE") == 0)
    767:           break;
    768:       }
    769:       if (!hasAncestor)
    770:       {
    771:         snprintf(msg, sizeof(msg), "The card on line %d is an SC following an SC, but no earlier SP or SM was found before this SC chain.", i + 1);
    772:         add_error(ctx, errors, msg, 1);
    773:       }
    774:     }
    775:     // SM cards should follow an SP or another SM
    776:     if (strcmp(code, "SM") == 0 && !(strcmp(last_code, "SP") == 0 || strcmp(last_code, "SM") == 0))
    777:     {
    778:       snprintf(msg, sizeof(msg), "The card on line %d is an SM, but the card above it is not an SP or another SM.", i + 1);
    779:       add_error(ctx, errors, msg, 1);
    780:     }
    781:     // SM must be immediately followed by SC (set pending on SM, clear on next non-SC)
    782:     if (strcmp(code, "SM") == 0)
    783:     {
    784:       pendingSM = i + 1; // store 1-based line number of SM
    785:     }
    786:     else if (pendingSM)
    787:     {
    788:       if (strcmp(code, "SC") != 0)
    789:       {
    790:         snprintf(msg, sizeof(msg), "The card on line %d is an SM, but the card after it is not an SC.", pendingSM);
    791:         add_error(ctx, errors, msg, 1);
    792:       }
    793:       pendingSM = 0;
    794:     }
    795: 
    796:     // QUESTION: it appears GR cards have to follow some other sort of geometry card, but it's not clear what
    797: 
    798:     // FR cards have to have either one input or three
    799:     if (strcmp(code, "FR") == 0)
    800:     {
    801:       if (!(deck->cards[i].ints_used == 1 || deck->cards[i].ints_used == 3))
    802:       {
    803:         snprintf(msg, sizeof(msg), "The card on line %d is an FR but does not have 1 or 3 integer inputs.", i + 1);
    804:         add_error(ctx, errors, msg, 0);
    805:       }
    806:     }
    807:     // advance last_code to current card for next-iteration pair checks
    808:     last_code = code;
    809: 
    810:     // warn about unsupported cards
    811:     if (strcmp(code, "WG") == 0)
    812:     {
    813:       snprintf(msg, sizeof(msg), "The card on line %d is a WG (Wire Grid), which is not supported by OpenNEC.", i + 1);
    814:       add_error(ctx, errors, msg, 0);
    815:     }
    816:     if (strcmp(code, "IT") == 0)
    817:     {
    818:       snprintf(msg, sizeof(msg), "The card on line %d is an IT (ITeration), which is not supported by OpenNEC.", i + 1);
    819:       add_error(ctx, errors, msg, 0);
    820:     }
    821:     if (strcmp(code, "OP") == 0)
    822:     {
    823:       snprintf(msg, sizeof(msg), "The card on line %d is an OP (OPtimization), which is not supported by OpenNEC.", i + 1);
>   824:       add_error(ctx, errors, msg, 0);
    825:     }
    826:     // Also warn for any other extension cards that are not supported
    827:     if (is_extension(&deck->cards[i]) && strcmp(code, "SY") != 0 && strcmp(code, "XT") != 0)
    828:     {
    829:       snprintf(msg, sizeof(msg), "The card on line %d is a %s, which is not supported by OpenNEC.", i + 1, code);

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L647:           snprintf(msg, sizeof(msg), "The card on line %d has geometry code %s, but we already saw the GE on card %d.", i, code, sawGE + 1);
L701:       snprintf(msg, sizeof(msg), "The card on line %d is a GC, but the card above it is not a GW.", i + 1);
L708:     //      snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius, but the card after it is not a GC.", i + 1);
L715:       snprintf(msg, sizeof(msg), "The card on line %d is a GD, but the card above it is not a GN.", i + 1);
L722:         snprintf(msg, sizeof(msg), "The card on line %d is a GN, but the card after it is not a GD.", i + 1);
L732:       snprintf(msg, sizeof(msg), "The card on line %d is a GF, but the card above it is not a CE or SY.", i + 1);
L742:         snprintf(msg, sizeof(msg), "The card on line %d is a %s, but the card above it is not a GA, GH, GW, SP or CW.", i + 1, code);
L751:       snprintf(msg, sizeof(msg), "The card on line %d is an SC, but the card above it is not an SP, SM or another SC.", i + 1);
L771:         snprintf(msg, sizeof(msg), "The card on line %d is an SC following an SC, but no earlier SP or SM was found before this SC chain.", i + 1);
L778:       snprintf(msg, sizeof(msg), "The card on line %d is an SM, but the card above it is not an SP or another SM.", i + 1);
L790:         snprintf(msg, sizeof(msg), "The card on line %d is an SM, but the card after it is not an SC.", pendingSM);
L803:         snprintf(msg, sizeof(msg), "The card on line %d is an FR but does not have 1 or 3 integer inputs.", i + 1);
L813:       snprintf(msg, sizeof(msg), "The card on line %d is a WG (Wire Grid), which is not supported by OpenNEC.", i + 1);
L818:       snprintf(msg, sizeof(msg), "The card on line %d is an IT (ITeration), which is not supported by OpenNEC.", i + 1);
L823:       snprintf(msg, sizeof(msg), "The card on line %d is an OP (OPtimization), which is not supported by OpenNEC.", i + 1);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 830
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 770-835) ---
    770:       {
    771:         snprintf(msg, sizeof(msg), "The card on line %d is an SC following an SC, but no earlier SP or SM was found before this SC chain.", i + 1);
    772:         add_error(ctx, errors, msg, 1);
    773:       }
    774:     }
    775:     // SM cards should follow an SP or another SM
    776:     if (strcmp(code, "SM") == 0 && !(strcmp(last_code, "SP") == 0 || strcmp(last_code, "SM") == 0))
    777:     {
    778:       snprintf(msg, sizeof(msg), "The card on line %d is an SM, but the card above it is not an SP or another SM.", i + 1);
    779:       add_error(ctx, errors, msg, 1);
    780:     }
    781:     // SM must be immediately followed by SC (set pending on SM, clear on next non-SC)
    782:     if (strcmp(code, "SM") == 0)
    783:     {
    784:       pendingSM = i + 1; // store 1-based line number of SM
    785:     }
    786:     else if (pendingSM)
    787:     {
    788:       if (strcmp(code, "SC") != 0)
    789:       {
    790:         snprintf(msg, sizeof(msg), "The card on line %d is an SM, but the card after it is not an SC.", pendingSM);
    791:         add_error(ctx, errors, msg, 1);
    792:       }
    793:       pendingSM = 0;
    794:     }
    795: 
    796:     // QUESTION: it appears GR cards have to follow some other sort of geometry card, but it's not clear what
    797: 
    798:     // FR cards have to have either one input or three
    799:     if (strcmp(code, "FR") == 0)
    800:     {
    801:       if (!(deck->cards[i].ints_used == 1 || deck->cards[i].ints_used == 3))
    802:       {
    803:         snprintf(msg, sizeof(msg), "The card on line %d is an FR but does not have 1 or 3 integer inputs.", i + 1);
    804:         add_error(ctx, errors, msg, 0);
    805:       }
    806:     }
    807:     // advance last_code to current card for next-iteration pair checks
    808:     last_code = code;
    809: 
    810:     // warn about unsupported cards
    811:     if (strcmp(code, "WG") == 0)
    812:     {
    813:       snprintf(msg, sizeof(msg), "The card on line %d is a WG (Wire Grid), which is not supported by OpenNEC.", i + 1);
    814:       add_error(ctx, errors, msg, 0);
    815:     }
    816:     if (strcmp(code, "IT") == 0)
    817:     {
    818:       snprintf(msg, sizeof(msg), "The card on line %d is an IT (ITeration), which is not supported by OpenNEC.", i + 1);
    819:       add_error(ctx, errors, msg, 0);
    820:     }
    821:     if (strcmp(code, "OP") == 0)
    822:     {
    823:       snprintf(msg, sizeof(msg), "The card on line %d is an OP (OPtimization), which is not supported by OpenNEC.", i + 1);
    824:       add_error(ctx, errors, msg, 0);
    825:     }
    826:     // Also warn for any other extension cards that are not supported
    827:     if (is_extension(&deck->cards[i]) && strcmp(code, "SY") != 0 && strcmp(code, "XT") != 0)
    828:     {
    829:       snprintf(msg, sizeof(msg), "The card on line %d is a %s, which is not supported by OpenNEC.", i + 1, code);
>   830:       add_error(ctx, errors, msg, 0);
    831:     }
    832: 
    833:     // after recording last_code, validate that certain control cards reference existing geometry tags seen so far
    834:     if (strcmp(code, "EX") == 0 && deck->cards[i].ints_used >= 3)
    835:     {

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L647:           snprintf(msg, sizeof(msg), "The card on line %d has geometry code %s, but we already saw the GE on card %d.", i, code, sawGE + 1);
L701:       snprintf(msg, sizeof(msg), "The card on line %d is a GC, but the card above it is not a GW.", i + 1);
L708:     //      snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius, but the card after it is not a GC.", i + 1);
L715:       snprintf(msg, sizeof(msg), "The card on line %d is a GD, but the card above it is not a GN.", i + 1);
L722:         snprintf(msg, sizeof(msg), "The card on line %d is a GN, but the card after it is not a GD.", i + 1);
L732:       snprintf(msg, sizeof(msg), "The card on line %d is a GF, but the card above it is not a CE or SY.", i + 1);
L742:         snprintf(msg, sizeof(msg), "The card on line %d is a %s, but the card above it is not a GA, GH, GW, SP or CW.", i + 1, code);
L751:       snprintf(msg, sizeof(msg), "The card on line %d is an SC, but the card above it is not an SP, SM or another SC.", i + 1);
L771:         snprintf(msg, sizeof(msg), "The card on line %d is an SC following an SC, but no earlier SP or SM was found before this SC chain.", i + 1);
L778:       snprintf(msg, sizeof(msg), "The card on line %d is an SM, but the card above it is not an SP or another SM.", i + 1);
L790:         snprintf(msg, sizeof(msg), "The card on line %d is an SM, but the card after it is not an SC.", pendingSM);
L803:         snprintf(msg, sizeof(msg), "The card on line %d is an FR but does not have 1 or 3 integer inputs.", i + 1);
L813:       snprintf(msg, sizeof(msg), "The card on line %d is a WG (Wire Grid), which is not supported by OpenNEC.", i + 1);
L818:       snprintf(msg, sizeof(msg), "The card on line %d is an IT (ITeration), which is not supported by OpenNEC.", i + 1);
L823:       snprintf(msg, sizeof(msg), "The card on line %d is an OP (OPtimization), which is not supported by OpenNEC.", i + 1);
L829:       snprintf(msg, sizeof(msg), "The card on line %d is a %s, which is not supported by OpenNEC.", i + 1, code);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 851
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 791-856) ---
    791:         add_error(ctx, errors, msg, 1);
    792:       }
    793:       pendingSM = 0;
    794:     }
    795: 
    796:     // QUESTION: it appears GR cards have to follow some other sort of geometry card, but it's not clear what
    797: 
    798:     // FR cards have to have either one input or three
    799:     if (strcmp(code, "FR") == 0)
    800:     {
    801:       if (!(deck->cards[i].ints_used == 1 || deck->cards[i].ints_used == 3))
    802:       {
    803:         snprintf(msg, sizeof(msg), "The card on line %d is an FR but does not have 1 or 3 integer inputs.", i + 1);
    804:         add_error(ctx, errors, msg, 0);
    805:       }
    806:     }
    807:     // advance last_code to current card for next-iteration pair checks
    808:     last_code = code;
    809: 
    810:     // warn about unsupported cards
    811:     if (strcmp(code, "WG") == 0)
    812:     {
    813:       snprintf(msg, sizeof(msg), "The card on line %d is a WG (Wire Grid), which is not supported by OpenNEC.", i + 1);
    814:       add_error(ctx, errors, msg, 0);
    815:     }
    816:     if (strcmp(code, "IT") == 0)
    817:     {
    818:       snprintf(msg, sizeof(msg), "The card on line %d is an IT (ITeration), which is not supported by OpenNEC.", i + 1);
    819:       add_error(ctx, errors, msg, 0);
    820:     }
    821:     if (strcmp(code, "OP") == 0)
    822:     {
    823:       snprintf(msg, sizeof(msg), "The card on line %d is an OP (OPtimization), which is not supported by OpenNEC.", i + 1);
    824:       add_error(ctx, errors, msg, 0);
    825:     }
    826:     // Also warn for any other extension cards that are not supported
    827:     if (is_extension(&deck->cards[i]) && strcmp(code, "SY") != 0 && strcmp(code, "XT") != 0)
    828:     {
    829:       snprintf(msg, sizeof(msg), "The card on line %d is a %s, which is not supported by OpenNEC.", i + 1, code);
    830:       add_error(ctx, errors, msg, 0);
    831:     }
    832: 
    833:     // after recording last_code, validate that certain control cards reference existing geometry tags seen so far
    834:     if (strcmp(code, "EX") == 0 && deck->cards[i].ints_used >= 3)
    835:     {
    836:       int tag = deck->cards[i].i[2];
    837:       if (tag > 0)
    838:       {
    839:         int seen = 0;
    840:         for (int t = 0; t < geom_tag_count; t++)
    841:         {
    842:           if (geom_tags[t] == tag)
    843:           {
    844:             seen = 1;
    845:             break;
    846:           }
    847:         }
    848:         if (!seen)
    849:         {
    850:           snprintf(msg, sizeof(msg), "The card on line %d is an EX referencing tag %d, which was not found.", i + 1, tag);
>   851:           add_error(ctx, errors, msg, 0);
    852:         }
    853:         else if (is_geometry_tag_ignored(deck, tag))
    854:         {
    855:           snprintf(msg, sizeof(msg), "The card on line %d is an EX referencing tag %d, which is marked as ignored.", i + 1, tag);
    856:           add_error(ctx, errors, msg, 0);

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L701:       snprintf(msg, sizeof(msg), "The card on line %d is a GC, but the card above it is not a GW.", i + 1);
L708:     //      snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius, but the card after it is not a GC.", i + 1);
L715:       snprintf(msg, sizeof(msg), "The card on line %d is a GD, but the card above it is not a GN.", i + 1);
L722:         snprintf(msg, sizeof(msg), "The card on line %d is a GN, but the card after it is not a GD.", i + 1);
L732:       snprintf(msg, sizeof(msg), "The card on line %d is a GF, but the card above it is not a CE or SY.", i + 1);
L742:         snprintf(msg, sizeof(msg), "The card on line %d is a %s, but the card above it is not a GA, GH, GW, SP or CW.", i + 1, code);
L751:       snprintf(msg, sizeof(msg), "The card on line %d is an SC, but the card above it is not an SP, SM or another SC.", i + 1);
L771:         snprintf(msg, sizeof(msg), "The card on line %d is an SC following an SC, but no earlier SP or SM was found before this SC chain.", i + 1);
L778:       snprintf(msg, sizeof(msg), "The card on line %d is an SM, but the card above it is not an SP or another SM.", i + 1);
L790:         snprintf(msg, sizeof(msg), "The card on line %d is an SM, but the card after it is not an SC.", pendingSM);
L803:         snprintf(msg, sizeof(msg), "The card on line %d is an FR but does not have 1 or 3 integer inputs.", i + 1);
L813:       snprintf(msg, sizeof(msg), "The card on line %d is a WG (Wire Grid), which is not supported by OpenNEC.", i + 1);
L818:       snprintf(msg, sizeof(msg), "The card on line %d is an IT (ITeration), which is not supported by OpenNEC.", i + 1);
L823:       snprintf(msg, sizeof(msg), "The card on line %d is an OP (OPtimization), which is not supported by OpenNEC.", i + 1);
L829:       snprintf(msg, sizeof(msg), "The card on line %d is a %s, which is not supported by OpenNEC.", i + 1, code);
L850:           snprintf(msg, sizeof(msg), "The card on line %d is an EX referencing tag %d, which was not found.", i + 1, tag);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 856
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 796-861) ---
    796:     // QUESTION: it appears GR cards have to follow some other sort of geometry card, but it's not clear what
    797: 
    798:     // FR cards have to have either one input or three
    799:     if (strcmp(code, "FR") == 0)
    800:     {
    801:       if (!(deck->cards[i].ints_used == 1 || deck->cards[i].ints_used == 3))
    802:       {
    803:         snprintf(msg, sizeof(msg), "The card on line %d is an FR but does not have 1 or 3 integer inputs.", i + 1);
    804:         add_error(ctx, errors, msg, 0);
    805:       }
    806:     }
    807:     // advance last_code to current card for next-iteration pair checks
    808:     last_code = code;
    809: 
    810:     // warn about unsupported cards
    811:     if (strcmp(code, "WG") == 0)
    812:     {
    813:       snprintf(msg, sizeof(msg), "The card on line %d is a WG (Wire Grid), which is not supported by OpenNEC.", i + 1);
    814:       add_error(ctx, errors, msg, 0);
    815:     }
    816:     if (strcmp(code, "IT") == 0)
    817:     {
    818:       snprintf(msg, sizeof(msg), "The card on line %d is an IT (ITeration), which is not supported by OpenNEC.", i + 1);
    819:       add_error(ctx, errors, msg, 0);
    820:     }
    821:     if (strcmp(code, "OP") == 0)
    822:     {
    823:       snprintf(msg, sizeof(msg), "The card on line %d is an OP (OPtimization), which is not supported by OpenNEC.", i + 1);
    824:       add_error(ctx, errors, msg, 0);
    825:     }
    826:     // Also warn for any other extension cards that are not supported
    827:     if (is_extension(&deck->cards[i]) && strcmp(code, "SY") != 0 && strcmp(code, "XT") != 0)
    828:     {
    829:       snprintf(msg, sizeof(msg), "The card on line %d is a %s, which is not supported by OpenNEC.", i + 1, code);
    830:       add_error(ctx, errors, msg, 0);
    831:     }
    832: 
    833:     // after recording last_code, validate that certain control cards reference existing geometry tags seen so far
    834:     if (strcmp(code, "EX") == 0 && deck->cards[i].ints_used >= 3)
    835:     {
    836:       int tag = deck->cards[i].i[2];
    837:       if (tag > 0)
    838:       {
    839:         int seen = 0;
    840:         for (int t = 0; t < geom_tag_count; t++)
    841:         {
    842:           if (geom_tags[t] == tag)
    843:           {
    844:             seen = 1;
    845:             break;
    846:           }
    847:         }
    848:         if (!seen)
    849:         {
    850:           snprintf(msg, sizeof(msg), "The card on line %d is an EX referencing tag %d, which was not found.", i + 1, tag);
    851:           add_error(ctx, errors, msg, 0);
    852:         }
    853:         else if (is_geometry_tag_ignored(deck, tag))
    854:         {
    855:           snprintf(msg, sizeof(msg), "The card on line %d is an EX referencing tag %d, which is marked as ignored.", i + 1, tag);
>   856:           add_error(ctx, errors, msg, 0);
    857:         }
    858:       }
    859:     }
    860:     if (strcmp(code, "TL") == 0 && deck->cards[i].ints_used >= 4)
    861:     {

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L701:       snprintf(msg, sizeof(msg), "The card on line %d is a GC, but the card above it is not a GW.", i + 1);
L708:     //      snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius, but the card after it is not a GC.", i + 1);
L715:       snprintf(msg, sizeof(msg), "The card on line %d is a GD, but the card above it is not a GN.", i + 1);
L722:         snprintf(msg, sizeof(msg), "The card on line %d is a GN, but the card after it is not a GD.", i + 1);
L732:       snprintf(msg, sizeof(msg), "The card on line %d is a GF, but the card above it is not a CE or SY.", i + 1);
L742:         snprintf(msg, sizeof(msg), "The card on line %d is a %s, but the card above it is not a GA, GH, GW, SP or CW.", i + 1, code);
L751:       snprintf(msg, sizeof(msg), "The card on line %d is an SC, but the card above it is not an SP, SM or another SC.", i + 1);
L771:         snprintf(msg, sizeof(msg), "The card on line %d is an SC following an SC, but no earlier SP or SM was found before this SC chain.", i + 1);
L778:       snprintf(msg, sizeof(msg), "The card on line %d is an SM, but the card above it is not an SP or another SM.", i + 1);
L790:         snprintf(msg, sizeof(msg), "The card on line %d is an SM, but the card after it is not an SC.", pendingSM);
L803:         snprintf(msg, sizeof(msg), "The card on line %d is an FR but does not have 1 or 3 integer inputs.", i + 1);
L813:       snprintf(msg, sizeof(msg), "The card on line %d is a WG (Wire Grid), which is not supported by OpenNEC.", i + 1);
L818:       snprintf(msg, sizeof(msg), "The card on line %d is an IT (ITeration), which is not supported by OpenNEC.", i + 1);
L823:       snprintf(msg, sizeof(msg), "The card on line %d is an OP (OPtimization), which is not supported by OpenNEC.", i + 1);
L829:       snprintf(msg, sizeof(msg), "The card on line %d is a %s, which is not supported by OpenNEC.", i + 1, code);
L850:           snprintf(msg, sizeof(msg), "The card on line %d is an EX referencing tag %d, which was not found.", i + 1, tag);
L855:           snprintf(msg, sizeof(msg), "The card on line %d is an EX referencing tag %d, which is marked as ignored.", i + 1, tag);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 878
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 818-883) ---
    818:       snprintf(msg, sizeof(msg), "The card on line %d is an IT (ITeration), which is not supported by OpenNEC.", i + 1);
    819:       add_error(ctx, errors, msg, 0);
    820:     }
    821:     if (strcmp(code, "OP") == 0)
    822:     {
    823:       snprintf(msg, sizeof(msg), "The card on line %d is an OP (OPtimization), which is not supported by OpenNEC.", i + 1);
    824:       add_error(ctx, errors, msg, 0);
    825:     }
    826:     // Also warn for any other extension cards that are not supported
    827:     if (is_extension(&deck->cards[i]) && strcmp(code, "SY") != 0 && strcmp(code, "XT") != 0)
    828:     {
    829:       snprintf(msg, sizeof(msg), "The card on line %d is a %s, which is not supported by OpenNEC.", i + 1, code);
    830:       add_error(ctx, errors, msg, 0);
    831:     }
    832: 
    833:     // after recording last_code, validate that certain control cards reference existing geometry tags seen so far
    834:     if (strcmp(code, "EX") == 0 && deck->cards[i].ints_used >= 3)
    835:     {
    836:       int tag = deck->cards[i].i[2];
    837:       if (tag > 0)
    838:       {
    839:         int seen = 0;
    840:         for (int t = 0; t < geom_tag_count; t++)
    841:         {
    842:           if (geom_tags[t] == tag)
    843:           {
    844:             seen = 1;
    845:             break;
    846:           }
    847:         }
    848:         if (!seen)
    849:         {
    850:           snprintf(msg, sizeof(msg), "The card on line %d is an EX referencing tag %d, which was not found.", i + 1, tag);
    851:           add_error(ctx, errors, msg, 0);
    852:         }
    853:         else if (is_geometry_tag_ignored(deck, tag))
    854:         {
    855:           snprintf(msg, sizeof(msg), "The card on line %d is an EX referencing tag %d, which is marked as ignored.", i + 1, tag);
    856:           add_error(ctx, errors, msg, 0);
    857:         }
    858:       }
    859:     }
    860:     if (strcmp(code, "TL") == 0 && deck->cards[i].ints_used >= 4)
    861:     {
    862:       int tag1 = deck->cards[i].i[1];
    863:       int tag2 = deck->cards[i].i[3];
    864:       if (tag1 > 0)
    865:       {
    866:         int seen1 = 0;
    867:         for (int t = 0; t < geom_tag_count; t++)
    868:         {
    869:           if (geom_tags[t] == tag1)
    870:           {
    871:             seen1 = 1;
    872:             break;
    873:           }
    874:         }
    875:         if (!seen1)
    876:         {
    877:           snprintf(msg, sizeof(msg), "The TL on line %d references tag %d (first endpoint), which was not found.", i + 1, tag1);
>   878:           add_error(ctx, errors, msg, 0);
    879:         }
    880:         else if (is_geometry_tag_ignored(deck, tag1))
    881:         {
    882:           snprintf(msg, sizeof(msg), "The TL on line %d references tag %d (first endpoint), which is marked as ignored.", i + 1, tag1);
    883:           add_error(ctx, errors, msg, 0);

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L701:       snprintf(msg, sizeof(msg), "The card on line %d is a GC, but the card above it is not a GW.", i + 1);
L708:     //      snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius, but the card after it is not a GC.", i + 1);
L715:       snprintf(msg, sizeof(msg), "The card on line %d is a GD, but the card above it is not a GN.", i + 1);
L722:         snprintf(msg, sizeof(msg), "The card on line %d is a GN, but the card after it is not a GD.", i + 1);
L732:       snprintf(msg, sizeof(msg), "The card on line %d is a GF, but the card above it is not a CE or SY.", i + 1);
L742:         snprintf(msg, sizeof(msg), "The card on line %d is a %s, but the card above it is not a GA, GH, GW, SP or CW.", i + 1, code);
L751:       snprintf(msg, sizeof(msg), "The card on line %d is an SC, but the card above it is not an SP, SM or another SC.", i + 1);
L771:         snprintf(msg, sizeof(msg), "The card on line %d is an SC following an SC, but no earlier SP or SM was found before this SC chain.", i + 1);
L778:       snprintf(msg, sizeof(msg), "The card on line %d is an SM, but the card above it is not an SP or another SM.", i + 1);
L790:         snprintf(msg, sizeof(msg), "The card on line %d is an SM, but the card after it is not an SC.", pendingSM);
L803:         snprintf(msg, sizeof(msg), "The card on line %d is an FR but does not have 1 or 3 integer inputs.", i + 1);
L813:       snprintf(msg, sizeof(msg), "The card on line %d is a WG (Wire Grid), which is not supported by OpenNEC.", i + 1);
L818:       snprintf(msg, sizeof(msg), "The card on line %d is an IT (ITeration), which is not supported by OpenNEC.", i + 1);
L823:       snprintf(msg, sizeof(msg), "The card on line %d is an OP (OPtimization), which is not supported by OpenNEC.", i + 1);
L829:       snprintf(msg, sizeof(msg), "The card on line %d is a %s, which is not supported by OpenNEC.", i + 1, code);
L850:           snprintf(msg, sizeof(msg), "The card on line %d is an EX referencing tag %d, which was not found.", i + 1, tag);
L855:           snprintf(msg, sizeof(msg), "The card on line %d is an EX referencing tag %d, which is marked as ignored.", i + 1, tag);
L877:           snprintf(msg, sizeof(msg), "The TL on line %d references tag %d (first endpoint), which was not found.", i + 1, tag1);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 883
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 823-888) ---
    823:       snprintf(msg, sizeof(msg), "The card on line %d is an OP (OPtimization), which is not supported by OpenNEC.", i + 1);
    824:       add_error(ctx, errors, msg, 0);
    825:     }
    826:     // Also warn for any other extension cards that are not supported
    827:     if (is_extension(&deck->cards[i]) && strcmp(code, "SY") != 0 && strcmp(code, "XT") != 0)
    828:     {
    829:       snprintf(msg, sizeof(msg), "The card on line %d is a %s, which is not supported by OpenNEC.", i + 1, code);
    830:       add_error(ctx, errors, msg, 0);
    831:     }
    832: 
    833:     // after recording last_code, validate that certain control cards reference existing geometry tags seen so far
    834:     if (strcmp(code, "EX") == 0 && deck->cards[i].ints_used >= 3)
    835:     {
    836:       int tag = deck->cards[i].i[2];
    837:       if (tag > 0)
    838:       {
    839:         int seen = 0;
    840:         for (int t = 0; t < geom_tag_count; t++)
    841:         {
    842:           if (geom_tags[t] == tag)
    843:           {
    844:             seen = 1;
    845:             break;
    846:           }
    847:         }
    848:         if (!seen)
    849:         {
    850:           snprintf(msg, sizeof(msg), "The card on line %d is an EX referencing tag %d, which was not found.", i + 1, tag);
    851:           add_error(ctx, errors, msg, 0);
    852:         }
    853:         else if (is_geometry_tag_ignored(deck, tag))
    854:         {
    855:           snprintf(msg, sizeof(msg), "The card on line %d is an EX referencing tag %d, which is marked as ignored.", i + 1, tag);
    856:           add_error(ctx, errors, msg, 0);
    857:         }
    858:       }
    859:     }
    860:     if (strcmp(code, "TL") == 0 && deck->cards[i].ints_used >= 4)
    861:     {
    862:       int tag1 = deck->cards[i].i[1];
    863:       int tag2 = deck->cards[i].i[3];
    864:       if (tag1 > 0)
    865:       {
    866:         int seen1 = 0;
    867:         for (int t = 0; t < geom_tag_count; t++)
    868:         {
    869:           if (geom_tags[t] == tag1)
    870:           {
    871:             seen1 = 1;
    872:             break;
    873:           }
    874:         }
    875:         if (!seen1)
    876:         {
    877:           snprintf(msg, sizeof(msg), "The TL on line %d references tag %d (first endpoint), which was not found.", i + 1, tag1);
    878:           add_error(ctx, errors, msg, 0);
    879:         }
    880:         else if (is_geometry_tag_ignored(deck, tag1))
    881:         {
    882:           snprintf(msg, sizeof(msg), "The TL on line %d references tag %d (first endpoint), which is marked as ignored.", i + 1, tag1);
>   883:           add_error(ctx, errors, msg, 0);
    884:         }
    885:       }
    886:       if (tag2 > 0)
    887:       {
    888:         int seen2 = 0;

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L701:       snprintf(msg, sizeof(msg), "The card on line %d is a GC, but the card above it is not a GW.", i + 1);
L708:     //      snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius, but the card after it is not a GC.", i + 1);
L715:       snprintf(msg, sizeof(msg), "The card on line %d is a GD, but the card above it is not a GN.", i + 1);
L722:         snprintf(msg, sizeof(msg), "The card on line %d is a GN, but the card after it is not a GD.", i + 1);
L732:       snprintf(msg, sizeof(msg), "The card on line %d is a GF, but the card above it is not a CE or SY.", i + 1);
L742:         snprintf(msg, sizeof(msg), "The card on line %d is a %s, but the card above it is not a GA, GH, GW, SP or CW.", i + 1, code);
L751:       snprintf(msg, sizeof(msg), "The card on line %d is an SC, but the card above it is not an SP, SM or another SC.", i + 1);
L771:         snprintf(msg, sizeof(msg), "The card on line %d is an SC following an SC, but no earlier SP or SM was found before this SC chain.", i + 1);
L778:       snprintf(msg, sizeof(msg), "The card on line %d is an SM, but the card above it is not an SP or another SM.", i + 1);
L790:         snprintf(msg, sizeof(msg), "The card on line %d is an SM, but the card after it is not an SC.", pendingSM);
L803:         snprintf(msg, sizeof(msg), "The card on line %d is an FR but does not have 1 or 3 integer inputs.", i + 1);
L813:       snprintf(msg, sizeof(msg), "The card on line %d is a WG (Wire Grid), which is not supported by OpenNEC.", i + 1);
L818:       snprintf(msg, sizeof(msg), "The card on line %d is an IT (ITeration), which is not supported by OpenNEC.", i + 1);
L823:       snprintf(msg, sizeof(msg), "The card on line %d is an OP (OPtimization), which is not supported by OpenNEC.", i + 1);
L829:       snprintf(msg, sizeof(msg), "The card on line %d is a %s, which is not supported by OpenNEC.", i + 1, code);
L850:           snprintf(msg, sizeof(msg), "The card on line %d is an EX referencing tag %d, which was not found.", i + 1, tag);
L855:           snprintf(msg, sizeof(msg), "The card on line %d is an EX referencing tag %d, which is marked as ignored.", i + 1, tag);
L877:           snprintf(msg, sizeof(msg), "The TL on line %d references tag %d (first endpoint), which was not found.", i + 1, tag1);
L882:           snprintf(msg, sizeof(msg), "The TL on line %d references tag %d (first endpoint), which is marked as ignored.", i + 1, tag1);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 900
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 840-905) ---
    840:         for (int t = 0; t < geom_tag_count; t++)
    841:         {
    842:           if (geom_tags[t] == tag)
    843:           {
    844:             seen = 1;
    845:             break;
    846:           }
    847:         }
    848:         if (!seen)
    849:         {
    850:           snprintf(msg, sizeof(msg), "The card on line %d is an EX referencing tag %d, which was not found.", i + 1, tag);
    851:           add_error(ctx, errors, msg, 0);
    852:         }
    853:         else if (is_geometry_tag_ignored(deck, tag))
    854:         {
    855:           snprintf(msg, sizeof(msg), "The card on line %d is an EX referencing tag %d, which is marked as ignored.", i + 1, tag);
    856:           add_error(ctx, errors, msg, 0);
    857:         }
    858:       }
    859:     }
    860:     if (strcmp(code, "TL") == 0 && deck->cards[i].ints_used >= 4)
    861:     {
    862:       int tag1 = deck->cards[i].i[1];
    863:       int tag2 = deck->cards[i].i[3];
    864:       if (tag1 > 0)
    865:       {
    866:         int seen1 = 0;
    867:         for (int t = 0; t < geom_tag_count; t++)
    868:         {
    869:           if (geom_tags[t] == tag1)
    870:           {
    871:             seen1 = 1;
    872:             break;
    873:           }
    874:         }
    875:         if (!seen1)
    876:         {
    877:           snprintf(msg, sizeof(msg), "The TL on line %d references tag %d (first endpoint), which was not found.", i + 1, tag1);
    878:           add_error(ctx, errors, msg, 0);
    879:         }
    880:         else if (is_geometry_tag_ignored(deck, tag1))
    881:         {
    882:           snprintf(msg, sizeof(msg), "The TL on line %d references tag %d (first endpoint), which is marked as ignored.", i + 1, tag1);
    883:           add_error(ctx, errors, msg, 0);
    884:         }
    885:       }
    886:       if (tag2 > 0)
    887:       {
    888:         int seen2 = 0;
    889:         for (int t = 0; t < geom_tag_count; t++)
    890:         {
    891:           if (geom_tags[t] == tag2)
    892:           {
    893:             seen2 = 1;
    894:             break;
    895:           }
    896:         }
    897:         if (!seen2)
    898:         {
    899:           snprintf(msg, sizeof(msg), "The TL on line %d references tag %d (second endpoint), which was not found.", i + 1, tag2);
>   900:           add_error(ctx, errors, msg, 0);
    901:         }
    902:         else if (is_geometry_tag_ignored(deck, tag2))
    903:         {
    904:           snprintf(msg, sizeof(msg), "The TL on line %d references tag %d (second endpoint), which is marked as ignored.", i + 1, tag2);
    905:           add_error(ctx, errors, msg, 0);

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L701:       snprintf(msg, sizeof(msg), "The card on line %d is a GC, but the card above it is not a GW.", i + 1);
L708:     //      snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius, but the card after it is not a GC.", i + 1);
L715:       snprintf(msg, sizeof(msg), "The card on line %d is a GD, but the card above it is not a GN.", i + 1);
L722:         snprintf(msg, sizeof(msg), "The card on line %d is a GN, but the card after it is not a GD.", i + 1);
L732:       snprintf(msg, sizeof(msg), "The card on line %d is a GF, but the card above it is not a CE or SY.", i + 1);
L742:         snprintf(msg, sizeof(msg), "The card on line %d is a %s, but the card above it is not a GA, GH, GW, SP or CW.", i + 1, code);
L751:       snprintf(msg, sizeof(msg), "The card on line %d is an SC, but the card above it is not an SP, SM or another SC.", i + 1);
L771:         snprintf(msg, sizeof(msg), "The card on line %d is an SC following an SC, but no earlier SP or SM was found before this SC chain.", i + 1);
L778:       snprintf(msg, sizeof(msg), "The card on line %d is an SM, but the card above it is not an SP or another SM.", i + 1);
L790:         snprintf(msg, sizeof(msg), "The card on line %d is an SM, but the card after it is not an SC.", pendingSM);
L803:         snprintf(msg, sizeof(msg), "The card on line %d is an FR but does not have 1 or 3 integer inputs.", i + 1);
L813:       snprintf(msg, sizeof(msg), "The card on line %d is a WG (Wire Grid), which is not supported by OpenNEC.", i + 1);
L818:       snprintf(msg, sizeof(msg), "The card on line %d is an IT (ITeration), which is not supported by OpenNEC.", i + 1);
L823:       snprintf(msg, sizeof(msg), "The card on line %d is an OP (OPtimization), which is not supported by OpenNEC.", i + 1);
L829:       snprintf(msg, sizeof(msg), "The card on line %d is a %s, which is not supported by OpenNEC.", i + 1, code);
L850:           snprintf(msg, sizeof(msg), "The card on line %d is an EX referencing tag %d, which was not found.", i + 1, tag);
L855:           snprintf(msg, sizeof(msg), "The card on line %d is an EX referencing tag %d, which is marked as ignored.", i + 1, tag);
L877:           snprintf(msg, sizeof(msg), "The TL on line %d references tag %d (first endpoint), which was not found.", i + 1, tag1);
L882:           snprintf(msg, sizeof(msg), "The TL on line %d references tag %d (first endpoint), which is marked as ignored.", i + 1, tag1);
L899:           snprintf(msg, sizeof(msg), "The TL on line %d references tag %d (second endpoint), which was not found.", i + 1, tag2);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 905
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 845-910) ---
    845:             break;
    846:           }
    847:         }
    848:         if (!seen)
    849:         {
    850:           snprintf(msg, sizeof(msg), "The card on line %d is an EX referencing tag %d, which was not found.", i + 1, tag);
    851:           add_error(ctx, errors, msg, 0);
    852:         }
    853:         else if (is_geometry_tag_ignored(deck, tag))
    854:         {
    855:           snprintf(msg, sizeof(msg), "The card on line %d is an EX referencing tag %d, which is marked as ignored.", i + 1, tag);
    856:           add_error(ctx, errors, msg, 0);
    857:         }
    858:       }
    859:     }
    860:     if (strcmp(code, "TL") == 0 && deck->cards[i].ints_used >= 4)
    861:     {
    862:       int tag1 = deck->cards[i].i[1];
    863:       int tag2 = deck->cards[i].i[3];
    864:       if (tag1 > 0)
    865:       {
    866:         int seen1 = 0;
    867:         for (int t = 0; t < geom_tag_count; t++)
    868:         {
    869:           if (geom_tags[t] == tag1)
    870:           {
    871:             seen1 = 1;
    872:             break;
    873:           }
    874:         }
    875:         if (!seen1)
    876:         {
    877:           snprintf(msg, sizeof(msg), "The TL on line %d references tag %d (first endpoint), which was not found.", i + 1, tag1);
    878:           add_error(ctx, errors, msg, 0);
    879:         }
    880:         else if (is_geometry_tag_ignored(deck, tag1))
    881:         {
    882:           snprintf(msg, sizeof(msg), "The TL on line %d references tag %d (first endpoint), which is marked as ignored.", i + 1, tag1);
    883:           add_error(ctx, errors, msg, 0);
    884:         }
    885:       }
    886:       if (tag2 > 0)
    887:       {
    888:         int seen2 = 0;
    889:         for (int t = 0; t < geom_tag_count; t++)
    890:         {
    891:           if (geom_tags[t] == tag2)
    892:           {
    893:             seen2 = 1;
    894:             break;
    895:           }
    896:         }
    897:         if (!seen2)
    898:         {
    899:           snprintf(msg, sizeof(msg), "The TL on line %d references tag %d (second endpoint), which was not found.", i + 1, tag2);
    900:           add_error(ctx, errors, msg, 0);
    901:         }
    902:         else if (is_geometry_tag_ignored(deck, tag2))
    903:         {
    904:           snprintf(msg, sizeof(msg), "The TL on line %d references tag %d (second endpoint), which is marked as ignored.", i + 1, tag2);
>   905:           add_error(ctx, errors, msg, 0);
    906:         }
    907:       }
    908:     }
    909:     if (strcmp(code, "LD") == 0 && deck->cards[i].ints_used >= 3)
    910:     {

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L708:     //      snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius, but the card after it is not a GC.", i + 1);
L715:       snprintf(msg, sizeof(msg), "The card on line %d is a GD, but the card above it is not a GN.", i + 1);
L722:         snprintf(msg, sizeof(msg), "The card on line %d is a GN, but the card after it is not a GD.", i + 1);
L732:       snprintf(msg, sizeof(msg), "The card on line %d is a GF, but the card above it is not a CE or SY.", i + 1);
L742:         snprintf(msg, sizeof(msg), "The card on line %d is a %s, but the card above it is not a GA, GH, GW, SP or CW.", i + 1, code);
L751:       snprintf(msg, sizeof(msg), "The card on line %d is an SC, but the card above it is not an SP, SM or another SC.", i + 1);
L771:         snprintf(msg, sizeof(msg), "The card on line %d is an SC following an SC, but no earlier SP or SM was found before this SC chain.", i + 1);
L778:       snprintf(msg, sizeof(msg), "The card on line %d is an SM, but the card above it is not an SP or another SM.", i + 1);
L790:         snprintf(msg, sizeof(msg), "The card on line %d is an SM, but the card after it is not an SC.", pendingSM);
L803:         snprintf(msg, sizeof(msg), "The card on line %d is an FR but does not have 1 or 3 integer inputs.", i + 1);
L813:       snprintf(msg, sizeof(msg), "The card on line %d is a WG (Wire Grid), which is not supported by OpenNEC.", i + 1);
L818:       snprintf(msg, sizeof(msg), "The card on line %d is an IT (ITeration), which is not supported by OpenNEC.", i + 1);
L823:       snprintf(msg, sizeof(msg), "The card on line %d is an OP (OPtimization), which is not supported by OpenNEC.", i + 1);
L829:       snprintf(msg, sizeof(msg), "The card on line %d is a %s, which is not supported by OpenNEC.", i + 1, code);
L850:           snprintf(msg, sizeof(msg), "The card on line %d is an EX referencing tag %d, which was not found.", i + 1, tag);
L855:           snprintf(msg, sizeof(msg), "The card on line %d is an EX referencing tag %d, which is marked as ignored.", i + 1, tag);
L877:           snprintf(msg, sizeof(msg), "The TL on line %d references tag %d (first endpoint), which was not found.", i + 1, tag1);
L882:           snprintf(msg, sizeof(msg), "The TL on line %d references tag %d (first endpoint), which is marked as ignored.", i + 1, tag1);
L899:           snprintf(msg, sizeof(msg), "The TL on line %d references tag %d (second endpoint), which was not found.", i + 1, tag2);
L904:           snprintf(msg, sizeof(msg), "The TL on line %d references tag %d (second endpoint), which is marked as ignored.", i + 1, tag2);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 926
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 866-931) ---
    866:         int seen1 = 0;
    867:         for (int t = 0; t < geom_tag_count; t++)
    868:         {
    869:           if (geom_tags[t] == tag1)
    870:           {
    871:             seen1 = 1;
    872:             break;
    873:           }
    874:         }
    875:         if (!seen1)
    876:         {
    877:           snprintf(msg, sizeof(msg), "The TL on line %d references tag %d (first endpoint), which was not found.", i + 1, tag1);
    878:           add_error(ctx, errors, msg, 0);
    879:         }
    880:         else if (is_geometry_tag_ignored(deck, tag1))
    881:         {
    882:           snprintf(msg, sizeof(msg), "The TL on line %d references tag %d (first endpoint), which is marked as ignored.", i + 1, tag1);
    883:           add_error(ctx, errors, msg, 0);
    884:         }
    885:       }
    886:       if (tag2 > 0)
    887:       {
    888:         int seen2 = 0;
    889:         for (int t = 0; t < geom_tag_count; t++)
    890:         {
    891:           if (geom_tags[t] == tag2)
    892:           {
    893:             seen2 = 1;
    894:             break;
    895:           }
    896:         }
    897:         if (!seen2)
    898:         {
    899:           snprintf(msg, sizeof(msg), "The TL on line %d references tag %d (second endpoint), which was not found.", i + 1, tag2);
    900:           add_error(ctx, errors, msg, 0);
    901:         }
    902:         else if (is_geometry_tag_ignored(deck, tag2))
    903:         {
    904:           snprintf(msg, sizeof(msg), "The TL on line %d references tag %d (second endpoint), which is marked as ignored.", i + 1, tag2);
    905:           add_error(ctx, errors, msg, 0);
    906:         }
    907:       }
    908:     }
    909:     if (strcmp(code, "LD") == 0 && deck->cards[i].ints_used >= 3)
    910:     {
    911:       int tag = deck->cards[i].i[2];
    912:       if (tag > 0)
    913:       {
    914:         int seen = 0;
    915:         for (int t = 0; t < geom_tag_count; t++)
    916:         {
    917:           if (geom_tags[t] == tag)
    918:           {
    919:             seen = 1;
    920:             break;
    921:           }
    922:         }
    923:         if (!seen)
    924:         {
    925:           snprintf(msg, sizeof(msg), "The card on line %d is an LD referencing tag %d, which was not found.", i + 1, tag);
>   926:           add_error(ctx, errors, msg, 0);
    927:         }
    928:         else if (is_geometry_tag_ignored(deck, tag))
    929:         {
    930:           snprintf(msg, sizeof(msg), "The card on line %d is an LD referencing tag %d, which is marked as ignored.", i + 1, tag);
    931:           add_error(ctx, errors, msg, 0);

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L732:       snprintf(msg, sizeof(msg), "The card on line %d is a GF, but the card above it is not a CE or SY.", i + 1);
L742:         snprintf(msg, sizeof(msg), "The card on line %d is a %s, but the card above it is not a GA, GH, GW, SP or CW.", i + 1, code);
L751:       snprintf(msg, sizeof(msg), "The card on line %d is an SC, but the card above it is not an SP, SM or another SC.", i + 1);
L771:         snprintf(msg, sizeof(msg), "The card on line %d is an SC following an SC, but no earlier SP or SM was found before this SC chain.", i + 1);
L778:       snprintf(msg, sizeof(msg), "The card on line %d is an SM, but the card above it is not an SP or another SM.", i + 1);
L790:         snprintf(msg, sizeof(msg), "The card on line %d is an SM, but the card after it is not an SC.", pendingSM);
L803:         snprintf(msg, sizeof(msg), "The card on line %d is an FR but does not have 1 or 3 integer inputs.", i + 1);
L813:       snprintf(msg, sizeof(msg), "The card on line %d is a WG (Wire Grid), which is not supported by OpenNEC.", i + 1);
L818:       snprintf(msg, sizeof(msg), "The card on line %d is an IT (ITeration), which is not supported by OpenNEC.", i + 1);
L823:       snprintf(msg, sizeof(msg), "The card on line %d is an OP (OPtimization), which is not supported by OpenNEC.", i + 1);
L829:       snprintf(msg, sizeof(msg), "The card on line %d is a %s, which is not supported by OpenNEC.", i + 1, code);
L850:           snprintf(msg, sizeof(msg), "The card on line %d is an EX referencing tag %d, which was not found.", i + 1, tag);
L855:           snprintf(msg, sizeof(msg), "The card on line %d is an EX referencing tag %d, which is marked as ignored.", i + 1, tag);
L877:           snprintf(msg, sizeof(msg), "The TL on line %d references tag %d (first endpoint), which was not found.", i + 1, tag1);
L882:           snprintf(msg, sizeof(msg), "The TL on line %d references tag %d (first endpoint), which is marked as ignored.", i + 1, tag1);
L899:           snprintf(msg, sizeof(msg), "The TL on line %d references tag %d (second endpoint), which was not found.", i + 1, tag2);
L904:           snprintf(msg, sizeof(msg), "The TL on line %d references tag %d (second endpoint), which is marked as ignored.", i + 1, tag2);
L925:           snprintf(msg, sizeof(msg), "The card on line %d is an LD referencing tag %d, which was not found.", i + 1, tag);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 931
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 871-936) ---
    871:             seen1 = 1;
    872:             break;
    873:           }
    874:         }
    875:         if (!seen1)
    876:         {
    877:           snprintf(msg, sizeof(msg), "The TL on line %d references tag %d (first endpoint), which was not found.", i + 1, tag1);
    878:           add_error(ctx, errors, msg, 0);
    879:         }
    880:         else if (is_geometry_tag_ignored(deck, tag1))
    881:         {
    882:           snprintf(msg, sizeof(msg), "The TL on line %d references tag %d (first endpoint), which is marked as ignored.", i + 1, tag1);
    883:           add_error(ctx, errors, msg, 0);
    884:         }
    885:       }
    886:       if (tag2 > 0)
    887:       {
    888:         int seen2 = 0;
    889:         for (int t = 0; t < geom_tag_count; t++)
    890:         {
    891:           if (geom_tags[t] == tag2)
    892:           {
    893:             seen2 = 1;
    894:             break;
    895:           }
    896:         }
    897:         if (!seen2)
    898:         {
    899:           snprintf(msg, sizeof(msg), "The TL on line %d references tag %d (second endpoint), which was not found.", i + 1, tag2);
    900:           add_error(ctx, errors, msg, 0);
    901:         }
    902:         else if (is_geometry_tag_ignored(deck, tag2))
    903:         {
    904:           snprintf(msg, sizeof(msg), "The TL on line %d references tag %d (second endpoint), which is marked as ignored.", i + 1, tag2);
    905:           add_error(ctx, errors, msg, 0);
    906:         }
    907:       }
    908:     }
    909:     if (strcmp(code, "LD") == 0 && deck->cards[i].ints_used >= 3)
    910:     {
    911:       int tag = deck->cards[i].i[2];
    912:       if (tag > 0)
    913:       {
    914:         int seen = 0;
    915:         for (int t = 0; t < geom_tag_count; t++)
    916:         {
    917:           if (geom_tags[t] == tag)
    918:           {
    919:             seen = 1;
    920:             break;
    921:           }
    922:         }
    923:         if (!seen)
    924:         {
    925:           snprintf(msg, sizeof(msg), "The card on line %d is an LD referencing tag %d, which was not found.", i + 1, tag);
    926:           add_error(ctx, errors, msg, 0);
    927:         }
    928:         else if (is_geometry_tag_ignored(deck, tag))
    929:         {
    930:           snprintf(msg, sizeof(msg), "The card on line %d is an LD referencing tag %d, which is marked as ignored.", i + 1, tag);
>   931:           add_error(ctx, errors, msg, 0);
    932:         }
    933:       }
    934:     }
    935:   } /* loop over cards */
    936: 

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L732:       snprintf(msg, sizeof(msg), "The card on line %d is a GF, but the card above it is not a CE or SY.", i + 1);
L742:         snprintf(msg, sizeof(msg), "The card on line %d is a %s, but the card above it is not a GA, GH, GW, SP or CW.", i + 1, code);
L751:       snprintf(msg, sizeof(msg), "The card on line %d is an SC, but the card above it is not an SP, SM or another SC.", i + 1);
L771:         snprintf(msg, sizeof(msg), "The card on line %d is an SC following an SC, but no earlier SP or SM was found before this SC chain.", i + 1);
L778:       snprintf(msg, sizeof(msg), "The card on line %d is an SM, but the card above it is not an SP or another SM.", i + 1);
L790:         snprintf(msg, sizeof(msg), "The card on line %d is an SM, but the card after it is not an SC.", pendingSM);
L803:         snprintf(msg, sizeof(msg), "The card on line %d is an FR but does not have 1 or 3 integer inputs.", i + 1);
L813:       snprintf(msg, sizeof(msg), "The card on line %d is a WG (Wire Grid), which is not supported by OpenNEC.", i + 1);
L818:       snprintf(msg, sizeof(msg), "The card on line %d is an IT (ITeration), which is not supported by OpenNEC.", i + 1);
L823:       snprintf(msg, sizeof(msg), "The card on line %d is an OP (OPtimization), which is not supported by OpenNEC.", i + 1);
L829:       snprintf(msg, sizeof(msg), "The card on line %d is a %s, which is not supported by OpenNEC.", i + 1, code);
L850:           snprintf(msg, sizeof(msg), "The card on line %d is an EX referencing tag %d, which was not found.", i + 1, tag);
L855:           snprintf(msg, sizeof(msg), "The card on line %d is an EX referencing tag %d, which is marked as ignored.", i + 1, tag);
L877:           snprintf(msg, sizeof(msg), "The TL on line %d references tag %d (first endpoint), which was not found.", i + 1, tag1);
L882:           snprintf(msg, sizeof(msg), "The TL on line %d references tag %d (first endpoint), which is marked as ignored.", i + 1, tag1);
L899:           snprintf(msg, sizeof(msg), "The TL on line %d references tag %d (second endpoint), which was not found.", i + 1, tag2);
L904:           snprintf(msg, sizeof(msg), "The TL on line %d references tag %d (second endpoint), which is marked as ignored.", i + 1, tag2);
L925:           snprintf(msg, sizeof(msg), "The card on line %d is an LD referencing tag %d, which was not found.", i + 1, tag);
L930:           snprintf(msg, sizeof(msg), "The card on line %d is an LD referencing tag %d, which is marked as ignored.", i + 1, tag);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 965
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 905-970) ---
    905:           add_error(ctx, errors, msg, 0);
    906:         }
    907:       }
    908:     }
    909:     if (strcmp(code, "LD") == 0 && deck->cards[i].ints_used >= 3)
    910:     {
    911:       int tag = deck->cards[i].i[2];
    912:       if (tag > 0)
    913:       {
    914:         int seen = 0;
    915:         for (int t = 0; t < geom_tag_count; t++)
    916:         {
    917:           if (geom_tags[t] == tag)
    918:           {
    919:             seen = 1;
    920:             break;
    921:           }
    922:         }
    923:         if (!seen)
    924:         {
    925:           snprintf(msg, sizeof(msg), "The card on line %d is an LD referencing tag %d, which was not found.", i + 1, tag);
    926:           add_error(ctx, errors, msg, 0);
    927:         }
    928:         else if (is_geometry_tag_ignored(deck, tag))
    929:         {
    930:           snprintf(msg, sizeof(msg), "The card on line %d is an LD referencing tag %d, which is marked as ignored.", i + 1, tag);
    931:           add_error(ctx, errors, msg, 0);
    932:         }
    933:       }
    934:     }
    935:   } /* loop over cards */
    936: 
    937:   // validate EX/LD not located at open wire ends
    938:   for (int wi = 0; wi < wire_count; wi++)
    939:   {
    940:     wire_info_t w = wires[wi];
    941:     int end1_connected = 0, end2_connected = 0;
    942:     for (int wj = 0; wj < wire_count; wj++)
    943:     {
    944:       if (wj == wi)
    945:         continue;
    946:       wire_info_t v = wires[wj];
    947:       if ((w.x1 == v.x1 && w.y1 == v.y1 && w.z1 == v.z1) || (w.x1 == v.x2 && w.y1 == v.y2 && w.z1 == v.z2))
    948:       {
    949:         end1_connected = 1;
    950:       }
    951:       if ((w.x2 == v.x1 && w.y2 == v.y1 && w.z2 == v.z1) || (w.x2 == v.x2 && w.y2 == v.y2 && w.z2 == v.z2))
    952:       {
    953:         end2_connected = 1;
    954:       }
    955:     }
    956:     // check EX references to this wire
    957:     for (int er = 0; er < ex_ref_count; er++)
    958:     {
    959:       if (ex_refs[er].tag == w.tag)
    960:       {
    961:         int seg = ex_refs[er].segStart;
    962:         if (seg == 1 && w.segs > 1 && !end1_connected)
    963:         {
    964:           snprintf(msg, sizeof(msg), "The EX on line %d is placed on segment 1 of tag %d, which is an open wire end.", ex_refs[er].line, w.tag);
>   965:           add_error(ctx, errors, msg, 0);
    966:         }
    967:         if (seg == w.segs && w.segs > 1 && !end2_connected)
    968:         {
    969:           snprintf(msg, sizeof(msg), "The EX on line %d is placed on segment %d of tag %d, which is an open wire end.", ex_refs[er].line, w.segs, w.tag);
    970:           add_error(ctx, errors, msg, 0);

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L771:         snprintf(msg, sizeof(msg), "The card on line %d is an SC following an SC, but no earlier SP or SM was found before this SC chain.", i + 1);
L778:       snprintf(msg, sizeof(msg), "The card on line %d is an SM, but the card above it is not an SP or another SM.", i + 1);
L790:         snprintf(msg, sizeof(msg), "The card on line %d is an SM, but the card after it is not an SC.", pendingSM);
L803:         snprintf(msg, sizeof(msg), "The card on line %d is an FR but does not have 1 or 3 integer inputs.", i + 1);
L813:       snprintf(msg, sizeof(msg), "The card on line %d is a WG (Wire Grid), which is not supported by OpenNEC.", i + 1);
L818:       snprintf(msg, sizeof(msg), "The card on line %d is an IT (ITeration), which is not supported by OpenNEC.", i + 1);
L823:       snprintf(msg, sizeof(msg), "The card on line %d is an OP (OPtimization), which is not supported by OpenNEC.", i + 1);
L829:       snprintf(msg, sizeof(msg), "The card on line %d is a %s, which is not supported by OpenNEC.", i + 1, code);
L850:           snprintf(msg, sizeof(msg), "The card on line %d is an EX referencing tag %d, which was not found.", i + 1, tag);
L855:           snprintf(msg, sizeof(msg), "The card on line %d is an EX referencing tag %d, which is marked as ignored.", i + 1, tag);
L877:           snprintf(msg, sizeof(msg), "The TL on line %d references tag %d (first endpoint), which was not found.", i + 1, tag1);
L882:           snprintf(msg, sizeof(msg), "The TL on line %d references tag %d (first endpoint), which is marked as ignored.", i + 1, tag1);
L899:           snprintf(msg, sizeof(msg), "The TL on line %d references tag %d (second endpoint), which was not found.", i + 1, tag2);
L904:           snprintf(msg, sizeof(msg), "The TL on line %d references tag %d (second endpoint), which is marked as ignored.", i + 1, tag2);
L925:           snprintf(msg, sizeof(msg), "The card on line %d is an LD referencing tag %d, which was not found.", i + 1, tag);
L930:           snprintf(msg, sizeof(msg), "The card on line %d is an LD referencing tag %d, which is marked as ignored.", i + 1, tag);
L964:           snprintf(msg, sizeof(msg), "The EX on line %d is placed on segment 1 of tag %d, which is an open wire end.", ex_refs[er].line, w.tag);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 970
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 910-975) ---
    910:     {
    911:       int tag = deck->cards[i].i[2];
    912:       if (tag > 0)
    913:       {
    914:         int seen = 0;
    915:         for (int t = 0; t < geom_tag_count; t++)
    916:         {
    917:           if (geom_tags[t] == tag)
    918:           {
    919:             seen = 1;
    920:             break;
    921:           }
    922:         }
    923:         if (!seen)
    924:         {
    925:           snprintf(msg, sizeof(msg), "The card on line %d is an LD referencing tag %d, which was not found.", i + 1, tag);
    926:           add_error(ctx, errors, msg, 0);
    927:         }
    928:         else if (is_geometry_tag_ignored(deck, tag))
    929:         {
    930:           snprintf(msg, sizeof(msg), "The card on line %d is an LD referencing tag %d, which is marked as ignored.", i + 1, tag);
    931:           add_error(ctx, errors, msg, 0);
    932:         }
    933:       }
    934:     }
    935:   } /* loop over cards */
    936: 
    937:   // validate EX/LD not located at open wire ends
    938:   for (int wi = 0; wi < wire_count; wi++)
    939:   {
    940:     wire_info_t w = wires[wi];
    941:     int end1_connected = 0, end2_connected = 0;
    942:     for (int wj = 0; wj < wire_count; wj++)
    943:     {
    944:       if (wj == wi)
    945:         continue;
    946:       wire_info_t v = wires[wj];
    947:       if ((w.x1 == v.x1 && w.y1 == v.y1 && w.z1 == v.z1) || (w.x1 == v.x2 && w.y1 == v.y2 && w.z1 == v.z2))
    948:       {
    949:         end1_connected = 1;
    950:       }
    951:       if ((w.x2 == v.x1 && w.y2 == v.y1 && w.z2 == v.z1) || (w.x2 == v.x2 && w.y2 == v.y2 && w.z2 == v.z2))
    952:       {
    953:         end2_connected = 1;
    954:       }
    955:     }
    956:     // check EX references to this wire
    957:     for (int er = 0; er < ex_ref_count; er++)
    958:     {
    959:       if (ex_refs[er].tag == w.tag)
    960:       {
    961:         int seg = ex_refs[er].segStart;
    962:         if (seg == 1 && w.segs > 1 && !end1_connected)
    963:         {
    964:           snprintf(msg, sizeof(msg), "The EX on line %d is placed on segment 1 of tag %d, which is an open wire end.", ex_refs[er].line, w.tag);
    965:           add_error(ctx, errors, msg, 0);
    966:         }
    967:         if (seg == w.segs && w.segs > 1 && !end2_connected)
    968:         {
    969:           snprintf(msg, sizeof(msg), "The EX on line %d is placed on segment %d of tag %d, which is an open wire end.", ex_refs[er].line, w.segs, w.tag);
>   970:           add_error(ctx, errors, msg, 0);
    971:         }
    972:       }
    973:     }
    974:     // check LD references to this wire (consider start/end segments)
    975:     for (int lr = 0; lr < ld_ref_count; lr++)

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L771:         snprintf(msg, sizeof(msg), "The card on line %d is an SC following an SC, but no earlier SP or SM was found before this SC chain.", i + 1);
L778:       snprintf(msg, sizeof(msg), "The card on line %d is an SM, but the card above it is not an SP or another SM.", i + 1);
L790:         snprintf(msg, sizeof(msg), "The card on line %d is an SM, but the card after it is not an SC.", pendingSM);
L803:         snprintf(msg, sizeof(msg), "The card on line %d is an FR but does not have 1 or 3 integer inputs.", i + 1);
L813:       snprintf(msg, sizeof(msg), "The card on line %d is a WG (Wire Grid), which is not supported by OpenNEC.", i + 1);
L818:       snprintf(msg, sizeof(msg), "The card on line %d is an IT (ITeration), which is not supported by OpenNEC.", i + 1);
L823:       snprintf(msg, sizeof(msg), "The card on line %d is an OP (OPtimization), which is not supported by OpenNEC.", i + 1);
L829:       snprintf(msg, sizeof(msg), "The card on line %d is a %s, which is not supported by OpenNEC.", i + 1, code);
L850:           snprintf(msg, sizeof(msg), "The card on line %d is an EX referencing tag %d, which was not found.", i + 1, tag);
L855:           snprintf(msg, sizeof(msg), "The card on line %d is an EX referencing tag %d, which is marked as ignored.", i + 1, tag);
L877:           snprintf(msg, sizeof(msg), "The TL on line %d references tag %d (first endpoint), which was not found.", i + 1, tag1);
L882:           snprintf(msg, sizeof(msg), "The TL on line %d references tag %d (first endpoint), which is marked as ignored.", i + 1, tag1);
L899:           snprintf(msg, sizeof(msg), "The TL on line %d references tag %d (second endpoint), which was not found.", i + 1, tag2);
L904:           snprintf(msg, sizeof(msg), "The TL on line %d references tag %d (second endpoint), which is marked as ignored.", i + 1, tag2);
L925:           snprintf(msg, sizeof(msg), "The card on line %d is an LD referencing tag %d, which was not found.", i + 1, tag);
L930:           snprintf(msg, sizeof(msg), "The card on line %d is an LD referencing tag %d, which is marked as ignored.", i + 1, tag);
L964:           snprintf(msg, sizeof(msg), "The EX on line %d is placed on segment 1 of tag %d, which is an open wire end.", ex_refs[er].line, w.tag);
L969:           snprintf(msg, sizeof(msg), "The EX on line %d is placed on segment %d of tag %d, which is an open wire end.", ex_refs[er].line, w.segs, w.tag);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 984
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 924-989) ---
    924:         {
    925:           snprintf(msg, sizeof(msg), "The card on line %d is an LD referencing tag %d, which was not found.", i + 1, tag);
    926:           add_error(ctx, errors, msg, 0);
    927:         }
    928:         else if (is_geometry_tag_ignored(deck, tag))
    929:         {
    930:           snprintf(msg, sizeof(msg), "The card on line %d is an LD referencing tag %d, which is marked as ignored.", i + 1, tag);
    931:           add_error(ctx, errors, msg, 0);
    932:         }
    933:       }
    934:     }
    935:   } /* loop over cards */
    936: 
    937:   // validate EX/LD not located at open wire ends
    938:   for (int wi = 0; wi < wire_count; wi++)
    939:   {
    940:     wire_info_t w = wires[wi];
    941:     int end1_connected = 0, end2_connected = 0;
    942:     for (int wj = 0; wj < wire_count; wj++)
    943:     {
    944:       if (wj == wi)
    945:         continue;
    946:       wire_info_t v = wires[wj];
    947:       if ((w.x1 == v.x1 && w.y1 == v.y1 && w.z1 == v.z1) || (w.x1 == v.x2 && w.y1 == v.y2 && w.z1 == v.z2))
    948:       {
    949:         end1_connected = 1;
    950:       }
    951:       if ((w.x2 == v.x1 && w.y2 == v.y1 && w.z2 == v.z1) || (w.x2 == v.x2 && w.y2 == v.y2 && w.z2 == v.z2))
    952:       {
    953:         end2_connected = 1;
    954:       }
    955:     }
    956:     // check EX references to this wire
    957:     for (int er = 0; er < ex_ref_count; er++)
    958:     {
    959:       if (ex_refs[er].tag == w.tag)
    960:       {
    961:         int seg = ex_refs[er].segStart;
    962:         if (seg == 1 && w.segs > 1 && !end1_connected)
    963:         {
    964:           snprintf(msg, sizeof(msg), "The EX on line %d is placed on segment 1 of tag %d, which is an open wire end.", ex_refs[er].line, w.tag);
    965:           add_error(ctx, errors, msg, 0);
    966:         }
    967:         if (seg == w.segs && w.segs > 1 && !end2_connected)
    968:         {
    969:           snprintf(msg, sizeof(msg), "The EX on line %d is placed on segment %d of tag %d, which is an open wire end.", ex_refs[er].line, w.segs, w.tag);
    970:           add_error(ctx, errors, msg, 0);
    971:         }
    972:       }
    973:     }
    974:     // check LD references to this wire (consider start/end segments)
    975:     for (int lr = 0; lr < ld_ref_count; lr++)
    976:     {
    977:       if (ld_refs[lr].tag == w.tag)
    978:       {
    979:         int s = ld_refs[lr].segStart;
    980:         int e = ld_refs[lr].segEnd;
    981:         if (s == 1 && w.segs > 1 && !end1_connected)
    982:         {
    983:           snprintf(msg, sizeof(msg), "The LD on line %d starts at segment 1 of tag %d, which is an open wire end.", ld_refs[lr].line, w.tag);
>   984:           add_error(ctx, errors, msg, 0);
    985:         }
    986:         if (e == 0)
    987:           e = s; // single-segment load
    988:         if (e == w.segs && w.segs > 1 && !end2_connected)
    989:         {

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L790:         snprintf(msg, sizeof(msg), "The card on line %d is an SM, but the card after it is not an SC.", pendingSM);
L803:         snprintf(msg, sizeof(msg), "The card on line %d is an FR but does not have 1 or 3 integer inputs.", i + 1);
L813:       snprintf(msg, sizeof(msg), "The card on line %d is a WG (Wire Grid), which is not supported by OpenNEC.", i + 1);
L818:       snprintf(msg, sizeof(msg), "The card on line %d is an IT (ITeration), which is not supported by OpenNEC.", i + 1);
L823:       snprintf(msg, sizeof(msg), "The card on line %d is an OP (OPtimization), which is not supported by OpenNEC.", i + 1);
L829:       snprintf(msg, sizeof(msg), "The card on line %d is a %s, which is not supported by OpenNEC.", i + 1, code);
L850:           snprintf(msg, sizeof(msg), "The card on line %d is an EX referencing tag %d, which was not found.", i + 1, tag);
L855:           snprintf(msg, sizeof(msg), "The card on line %d is an EX referencing tag %d, which is marked as ignored.", i + 1, tag);
L877:           snprintf(msg, sizeof(msg), "The TL on line %d references tag %d (first endpoint), which was not found.", i + 1, tag1);
L882:           snprintf(msg, sizeof(msg), "The TL on line %d references tag %d (first endpoint), which is marked as ignored.", i + 1, tag1);
L899:           snprintf(msg, sizeof(msg), "The TL on line %d references tag %d (second endpoint), which was not found.", i + 1, tag2);
L904:           snprintf(msg, sizeof(msg), "The TL on line %d references tag %d (second endpoint), which is marked as ignored.", i + 1, tag2);
L925:           snprintf(msg, sizeof(msg), "The card on line %d is an LD referencing tag %d, which was not found.", i + 1, tag);
L930:           snprintf(msg, sizeof(msg), "The card on line %d is an LD referencing tag %d, which is marked as ignored.", i + 1, tag);
L964:           snprintf(msg, sizeof(msg), "The EX on line %d is placed on segment 1 of tag %d, which is an open wire end.", ex_refs[er].line, w.tag);
L969:           snprintf(msg, sizeof(msg), "The EX on line %d is placed on segment %d of tag %d, which is an open wire end.", ex_refs[er].line, w.segs, w.tag);
L983:           snprintf(msg, sizeof(msg), "The LD on line %d starts at segment 1 of tag %d, which is an open wire end.", ld_refs[lr].line, w.tag);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 991
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 931-996) ---
    931:           add_error(ctx, errors, msg, 0);
    932:         }
    933:       }
    934:     }
    935:   } /* loop over cards */
    936: 
    937:   // validate EX/LD not located at open wire ends
    938:   for (int wi = 0; wi < wire_count; wi++)
    939:   {
    940:     wire_info_t w = wires[wi];
    941:     int end1_connected = 0, end2_connected = 0;
    942:     for (int wj = 0; wj < wire_count; wj++)
    943:     {
    944:       if (wj == wi)
    945:         continue;
    946:       wire_info_t v = wires[wj];
    947:       if ((w.x1 == v.x1 && w.y1 == v.y1 && w.z1 == v.z1) || (w.x1 == v.x2 && w.y1 == v.y2 && w.z1 == v.z2))
    948:       {
    949:         end1_connected = 1;
    950:       }
    951:       if ((w.x2 == v.x1 && w.y2 == v.y1 && w.z2 == v.z1) || (w.x2 == v.x2 && w.y2 == v.y2 && w.z2 == v.z2))
    952:       {
    953:         end2_connected = 1;
    954:       }
    955:     }
    956:     // check EX references to this wire
    957:     for (int er = 0; er < ex_ref_count; er++)
    958:     {
    959:       if (ex_refs[er].tag == w.tag)
    960:       {
    961:         int seg = ex_refs[er].segStart;
    962:         if (seg == 1 && w.segs > 1 && !end1_connected)
    963:         {
    964:           snprintf(msg, sizeof(msg), "The EX on line %d is placed on segment 1 of tag %d, which is an open wire end.", ex_refs[er].line, w.tag);
    965:           add_error(ctx, errors, msg, 0);
    966:         }
    967:         if (seg == w.segs && w.segs > 1 && !end2_connected)
    968:         {
    969:           snprintf(msg, sizeof(msg), "The EX on line %d is placed on segment %d of tag %d, which is an open wire end.", ex_refs[er].line, w.segs, w.tag);
    970:           add_error(ctx, errors, msg, 0);
    971:         }
    972:       }
    973:     }
    974:     // check LD references to this wire (consider start/end segments)
    975:     for (int lr = 0; lr < ld_ref_count; lr++)
    976:     {
    977:       if (ld_refs[lr].tag == w.tag)
    978:       {
    979:         int s = ld_refs[lr].segStart;
    980:         int e = ld_refs[lr].segEnd;
    981:         if (s == 1 && w.segs > 1 && !end1_connected)
    982:         {
    983:           snprintf(msg, sizeof(msg), "The LD on line %d starts at segment 1 of tag %d, which is an open wire end.", ld_refs[lr].line, w.tag);
    984:           add_error(ctx, errors, msg, 0);
    985:         }
    986:         if (e == 0)
    987:           e = s; // single-segment load
    988:         if (e == w.segs && w.segs > 1 && !end2_connected)
    989:         {
    990:           snprintf(msg, sizeof(msg), "The LD on line %d ends at segment %d of tag %d, which is an open wire end.", ld_refs[lr].line, w.segs, w.tag);
>   991:           add_error(ctx, errors, msg, 0);
    992:         }
    993:       }
    994:     }
    995:   }
    996: 

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L803:         snprintf(msg, sizeof(msg), "The card on line %d is an FR but does not have 1 or 3 integer inputs.", i + 1);
L813:       snprintf(msg, sizeof(msg), "The card on line %d is a WG (Wire Grid), which is not supported by OpenNEC.", i + 1);
L818:       snprintf(msg, sizeof(msg), "The card on line %d is an IT (ITeration), which is not supported by OpenNEC.", i + 1);
L823:       snprintf(msg, sizeof(msg), "The card on line %d is an OP (OPtimization), which is not supported by OpenNEC.", i + 1);
L829:       snprintf(msg, sizeof(msg), "The card on line %d is a %s, which is not supported by OpenNEC.", i + 1, code);
L850:           snprintf(msg, sizeof(msg), "The card on line %d is an EX referencing tag %d, which was not found.", i + 1, tag);
L855:           snprintf(msg, sizeof(msg), "The card on line %d is an EX referencing tag %d, which is marked as ignored.", i + 1, tag);
L877:           snprintf(msg, sizeof(msg), "The TL on line %d references tag %d (first endpoint), which was not found.", i + 1, tag1);
L882:           snprintf(msg, sizeof(msg), "The TL on line %d references tag %d (first endpoint), which is marked as ignored.", i + 1, tag1);
L899:           snprintf(msg, sizeof(msg), "The TL on line %d references tag %d (second endpoint), which was not found.", i + 1, tag2);
L904:           snprintf(msg, sizeof(msg), "The TL on line %d references tag %d (second endpoint), which is marked as ignored.", i + 1, tag2);
L925:           snprintf(msg, sizeof(msg), "The card on line %d is an LD referencing tag %d, which was not found.", i + 1, tag);
L930:           snprintf(msg, sizeof(msg), "The card on line %d is an LD referencing tag %d, which is marked as ignored.", i + 1, tag);
L964:           snprintf(msg, sizeof(msg), "The EX on line %d is placed on segment 1 of tag %d, which is an open wire end.", ex_refs[er].line, w.tag);
L969:           snprintf(msg, sizeof(msg), "The EX on line %d is placed on segment %d of tag %d, which is an open wire end.", ex_refs[er].line, w.segs, w.tag);
L983:           snprintf(msg, sizeof(msg), "The LD on line %d starts at segment 1 of tag %d, which is an open wire end.", ld_refs[lr].line, w.tag);
L990:           snprintf(msg, sizeof(msg), "The LD on line %d ends at segment %d of tag %d, which is an open wire end.", ld_refs[lr].line, w.segs, w.tag);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 1028
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 968-1033) ---
    968:         {
    969:           snprintf(msg, sizeof(msg), "The EX on line %d is placed on segment %d of tag %d, which is an open wire end.", ex_refs[er].line, w.segs, w.tag);
    970:           add_error(ctx, errors, msg, 0);
    971:         }
    972:       }
    973:     }
    974:     // check LD references to this wire (consider start/end segments)
    975:     for (int lr = 0; lr < ld_ref_count; lr++)
    976:     {
    977:       if (ld_refs[lr].tag == w.tag)
    978:       {
    979:         int s = ld_refs[lr].segStart;
    980:         int e = ld_refs[lr].segEnd;
    981:         if (s == 1 && w.segs > 1 && !end1_connected)
    982:         {
    983:           snprintf(msg, sizeof(msg), "The LD on line %d starts at segment 1 of tag %d, which is an open wire end.", ld_refs[lr].line, w.tag);
    984:           add_error(ctx, errors, msg, 0);
    985:         }
    986:         if (e == 0)
    987:           e = s; // single-segment load
    988:         if (e == w.segs && w.segs > 1 && !end2_connected)
    989:         {
    990:           snprintf(msg, sizeof(msg), "The LD on line %d ends at segment %d of tag %d, which is an open wire end.", ld_refs[lr].line, w.segs, w.tag);
    991:           add_error(ctx, errors, msg, 0);
    992:         }
    993:       }
    994:     }
    995:   }
    996: 
    997:   // parallel wire segmentation check (0.05 wavelengths threshold)
    998:   if (freq_mhz > 0.0 && wire_count > 1)
    999:   {
   1000:     check_parallel_wire_segmentation(ctx, errors, wires, wire_count, freq_mhz);
   1001:   }
   1002:   if (freq_mhz > 0.0 && wire_count > 0)
   1003:   {
   1004:     check_segment_length_and_radius(ctx, errors, wires, wire_count, freq_mhz, ek_enabled);
   1005:   }
   1006:   if (wire_count > 0)
   1007:   {
   1008:     check_ge_low_height_hazard(ctx, errors, wires, wire_count, GEType);
   1009:     check_junction_segmentation_consistency(ctx, errors, wires, wire_count);
   1010:   }
   1011: 
   1012:   // TL segment bounds validation: ensure referenced segments exist on the given tags
   1013:   if (tl_ref_count > 0 && wire_count > 0)
   1014:   {
   1015:     for (int r = 0; r < tl_ref_count; r++)
   1016:     {
   1017:       int segs1 = -1, segs2 = -1;
   1018:       for (int w = 0; w < wire_count; w++)
   1019:       {
   1020:         if (wires[w].tag == tl_refs[r].tag1)
   1021:           segs1 = wires[w].segs;
   1022:         if (wires[w].tag == tl_refs[r].tag2)
   1023:           segs2 = wires[w].segs;
   1024:       }
   1025:       if (segs1 > 0 && (tl_refs[r].seg1 <= 0 || tl_refs[r].seg1 > segs1))
   1026:       {
   1027:         snprintf(msg, sizeof(msg), "The TL on line %d references an invalid segment on the first endpoint; segment index out of range for the wire tag %d.", tl_refs[r].line, tl_refs[r].tag1);
>  1028:         add_error(ctx, errors, msg, 0);
   1029:       }
   1030:       if (segs2 > 0 && (tl_refs[r].seg2 <= 0 || tl_refs[r].seg2 > segs2))
   1031:       {
   1032:         snprintf(msg, sizeof(msg), "The TL on line %d references an invalid segment on the second endpoint; segment index out of range for the wire tag %d.", tl_refs[r].line, tl_refs[r].tag2);
   1033:         add_error(ctx, errors, msg, 0);

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L829:       snprintf(msg, sizeof(msg), "The card on line %d is a %s, which is not supported by OpenNEC.", i + 1, code);
L850:           snprintf(msg, sizeof(msg), "The card on line %d is an EX referencing tag %d, which was not found.", i + 1, tag);
L855:           snprintf(msg, sizeof(msg), "The card on line %d is an EX referencing tag %d, which is marked as ignored.", i + 1, tag);
L877:           snprintf(msg, sizeof(msg), "The TL on line %d references tag %d (first endpoint), which was not found.", i + 1, tag1);
L882:           snprintf(msg, sizeof(msg), "The TL on line %d references tag %d (first endpoint), which is marked as ignored.", i + 1, tag1);
L899:           snprintf(msg, sizeof(msg), "The TL on line %d references tag %d (second endpoint), which was not found.", i + 1, tag2);
L904:           snprintf(msg, sizeof(msg), "The TL on line %d references tag %d (second endpoint), which is marked as ignored.", i + 1, tag2);
L925:           snprintf(msg, sizeof(msg), "The card on line %d is an LD referencing tag %d, which was not found.", i + 1, tag);
L930:           snprintf(msg, sizeof(msg), "The card on line %d is an LD referencing tag %d, which is marked as ignored.", i + 1, tag);
L964:           snprintf(msg, sizeof(msg), "The EX on line %d is placed on segment 1 of tag %d, which is an open wire end.", ex_refs[er].line, w.tag);
L969:           snprintf(msg, sizeof(msg), "The EX on line %d is placed on segment %d of tag %d, which is an open wire end.", ex_refs[er].line, w.segs, w.tag);
L983:           snprintf(msg, sizeof(msg), "The LD on line %d starts at segment 1 of tag %d, which is an open wire end.", ld_refs[lr].line, w.tag);
L990:           snprintf(msg, sizeof(msg), "The LD on line %d ends at segment %d of tag %d, which is an open wire end.", ld_refs[lr].line, w.segs, w.tag);
L1027:         snprintf(msg, sizeof(msg), "The TL on line %d references an invalid segment on the first endpoint; segment index out of range for the wire tag %d.", tl_refs[r].line, tl_refs[r].tag1);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 1033
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 973-1038) ---
    973:     }
    974:     // check LD references to this wire (consider start/end segments)
    975:     for (int lr = 0; lr < ld_ref_count; lr++)
    976:     {
    977:       if (ld_refs[lr].tag == w.tag)
    978:       {
    979:         int s = ld_refs[lr].segStart;
    980:         int e = ld_refs[lr].segEnd;
    981:         if (s == 1 && w.segs > 1 && !end1_connected)
    982:         {
    983:           snprintf(msg, sizeof(msg), "The LD on line %d starts at segment 1 of tag %d, which is an open wire end.", ld_refs[lr].line, w.tag);
    984:           add_error(ctx, errors, msg, 0);
    985:         }
    986:         if (e == 0)
    987:           e = s; // single-segment load
    988:         if (e == w.segs && w.segs > 1 && !end2_connected)
    989:         {
    990:           snprintf(msg, sizeof(msg), "The LD on line %d ends at segment %d of tag %d, which is an open wire end.", ld_refs[lr].line, w.segs, w.tag);
    991:           add_error(ctx, errors, msg, 0);
    992:         }
    993:       }
    994:     }
    995:   }
    996: 
    997:   // parallel wire segmentation check (0.05 wavelengths threshold)
    998:   if (freq_mhz > 0.0 && wire_count > 1)
    999:   {
   1000:     check_parallel_wire_segmentation(ctx, errors, wires, wire_count, freq_mhz);
   1001:   }
   1002:   if (freq_mhz > 0.0 && wire_count > 0)
   1003:   {
   1004:     check_segment_length_and_radius(ctx, errors, wires, wire_count, freq_mhz, ek_enabled);
   1005:   }
   1006:   if (wire_count > 0)
   1007:   {
   1008:     check_ge_low_height_hazard(ctx, errors, wires, wire_count, GEType);
   1009:     check_junction_segmentation_consistency(ctx, errors, wires, wire_count);
   1010:   }
   1011: 
   1012:   // TL segment bounds validation: ensure referenced segments exist on the given tags
   1013:   if (tl_ref_count > 0 && wire_count > 0)
   1014:   {
   1015:     for (int r = 0; r < tl_ref_count; r++)
   1016:     {
   1017:       int segs1 = -1, segs2 = -1;
   1018:       for (int w = 0; w < wire_count; w++)
   1019:       {
   1020:         if (wires[w].tag == tl_refs[r].tag1)
   1021:           segs1 = wires[w].segs;
   1022:         if (wires[w].tag == tl_refs[r].tag2)
   1023:           segs2 = wires[w].segs;
   1024:       }
   1025:       if (segs1 > 0 && (tl_refs[r].seg1 <= 0 || tl_refs[r].seg1 > segs1))
   1026:       {
   1027:         snprintf(msg, sizeof(msg), "The TL on line %d references an invalid segment on the first endpoint; segment index out of range for the wire tag %d.", tl_refs[r].line, tl_refs[r].tag1);
   1028:         add_error(ctx, errors, msg, 0);
   1029:       }
   1030:       if (segs2 > 0 && (tl_refs[r].seg2 <= 0 || tl_refs[r].seg2 > segs2))
   1031:       {
   1032:         snprintf(msg, sizeof(msg), "The TL on line %d references an invalid segment on the second endpoint; segment index out of range for the wire tag %d.", tl_refs[r].line, tl_refs[r].tag2);
>  1033:         add_error(ctx, errors, msg, 0);
   1034:       }
   1035:       // TL self-loop: same tag+segment on both ends
   1036:       if (tl_refs[r].tag1 == tl_refs[r].tag2 && tl_refs[r].seg1 == tl_refs[r].seg2)
   1037:       {
   1038:         snprintf(msg, sizeof(msg), "The TL on line %d connects the same tag and segment on both ends; this is a no-op or invalid connection.", tl_refs[r].line);

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L850:           snprintf(msg, sizeof(msg), "The card on line %d is an EX referencing tag %d, which was not found.", i + 1, tag);
L855:           snprintf(msg, sizeof(msg), "The card on line %d is an EX referencing tag %d, which is marked as ignored.", i + 1, tag);
L877:           snprintf(msg, sizeof(msg), "The TL on line %d references tag %d (first endpoint), which was not found.", i + 1, tag1);
L882:           snprintf(msg, sizeof(msg), "The TL on line %d references tag %d (first endpoint), which is marked as ignored.", i + 1, tag1);
L899:           snprintf(msg, sizeof(msg), "The TL on line %d references tag %d (second endpoint), which was not found.", i + 1, tag2);
L904:           snprintf(msg, sizeof(msg), "The TL on line %d references tag %d (second endpoint), which is marked as ignored.", i + 1, tag2);
L925:           snprintf(msg, sizeof(msg), "The card on line %d is an LD referencing tag %d, which was not found.", i + 1, tag);
L930:           snprintf(msg, sizeof(msg), "The card on line %d is an LD referencing tag %d, which is marked as ignored.", i + 1, tag);
L964:           snprintf(msg, sizeof(msg), "The EX on line %d is placed on segment 1 of tag %d, which is an open wire end.", ex_refs[er].line, w.tag);
L969:           snprintf(msg, sizeof(msg), "The EX on line %d is placed on segment %d of tag %d, which is an open wire end.", ex_refs[er].line, w.segs, w.tag);
L983:           snprintf(msg, sizeof(msg), "The LD on line %d starts at segment 1 of tag %d, which is an open wire end.", ld_refs[lr].line, w.tag);
L990:           snprintf(msg, sizeof(msg), "The LD on line %d ends at segment %d of tag %d, which is an open wire end.", ld_refs[lr].line, w.segs, w.tag);
L1027:         snprintf(msg, sizeof(msg), "The TL on line %d references an invalid segment on the first endpoint; segment index out of range for the wire tag %d.", tl_refs[r].line, tl_refs[r].tag1);
L1032:         snprintf(msg, sizeof(msg), "The TL on line %d references an invalid segment on the second endpoint; segment index out of range for the wire tag %d.", tl_refs[r].line, tl_refs[r].tag2);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 1039
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 979-1044) ---
    979:         int s = ld_refs[lr].segStart;
    980:         int e = ld_refs[lr].segEnd;
    981:         if (s == 1 && w.segs > 1 && !end1_connected)
    982:         {
    983:           snprintf(msg, sizeof(msg), "The LD on line %d starts at segment 1 of tag %d, which is an open wire end.", ld_refs[lr].line, w.tag);
    984:           add_error(ctx, errors, msg, 0);
    985:         }
    986:         if (e == 0)
    987:           e = s; // single-segment load
    988:         if (e == w.segs && w.segs > 1 && !end2_connected)
    989:         {
    990:           snprintf(msg, sizeof(msg), "The LD on line %d ends at segment %d of tag %d, which is an open wire end.", ld_refs[lr].line, w.segs, w.tag);
    991:           add_error(ctx, errors, msg, 0);
    992:         }
    993:       }
    994:     }
    995:   }
    996: 
    997:   // parallel wire segmentation check (0.05 wavelengths threshold)
    998:   if (freq_mhz > 0.0 && wire_count > 1)
    999:   {
   1000:     check_parallel_wire_segmentation(ctx, errors, wires, wire_count, freq_mhz);
   1001:   }
   1002:   if (freq_mhz > 0.0 && wire_count > 0)
   1003:   {
   1004:     check_segment_length_and_radius(ctx, errors, wires, wire_count, freq_mhz, ek_enabled);
   1005:   }
   1006:   if (wire_count > 0)
   1007:   {
   1008:     check_ge_low_height_hazard(ctx, errors, wires, wire_count, GEType);
   1009:     check_junction_segmentation_consistency(ctx, errors, wires, wire_count);
   1010:   }
   1011: 
   1012:   // TL segment bounds validation: ensure referenced segments exist on the given tags
   1013:   if (tl_ref_count > 0 && wire_count > 0)
   1014:   {
   1015:     for (int r = 0; r < tl_ref_count; r++)
   1016:     {
   1017:       int segs1 = -1, segs2 = -1;
   1018:       for (int w = 0; w < wire_count; w++)
   1019:       {
   1020:         if (wires[w].tag == tl_refs[r].tag1)
   1021:           segs1 = wires[w].segs;
   1022:         if (wires[w].tag == tl_refs[r].tag2)
   1023:           segs2 = wires[w].segs;
   1024:       }
   1025:       if (segs1 > 0 && (tl_refs[r].seg1 <= 0 || tl_refs[r].seg1 > segs1))
   1026:       {
   1027:         snprintf(msg, sizeof(msg), "The TL on line %d references an invalid segment on the first endpoint; segment index out of range for the wire tag %d.", tl_refs[r].line, tl_refs[r].tag1);
   1028:         add_error(ctx, errors, msg, 0);
   1029:       }
   1030:       if (segs2 > 0 && (tl_refs[r].seg2 <= 0 || tl_refs[r].seg2 > segs2))
   1031:       {
   1032:         snprintf(msg, sizeof(msg), "The TL on line %d references an invalid segment on the second endpoint; segment index out of range for the wire tag %d.", tl_refs[r].line, tl_refs[r].tag2);
   1033:         add_error(ctx, errors, msg, 0);
   1034:       }
   1035:       // TL self-loop: same tag+segment on both ends
   1036:       if (tl_refs[r].tag1 == tl_refs[r].tag2 && tl_refs[r].seg1 == tl_refs[r].seg2)
   1037:       {
   1038:         snprintf(msg, sizeof(msg), "The TL on line %d connects the same tag and segment on both ends; this is a no-op or invalid connection.", tl_refs[r].line);
>  1039:         add_error(ctx, errors, msg, 0);
   1040:       }
   1041:     }
   1042:   }
   1043: 
   1044:   // LD segment bounds validation: ensure referenced segment range falls within the wire's segment count

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L850:           snprintf(msg, sizeof(msg), "The card on line %d is an EX referencing tag %d, which was not found.", i + 1, tag);
L855:           snprintf(msg, sizeof(msg), "The card on line %d is an EX referencing tag %d, which is marked as ignored.", i + 1, tag);
L877:           snprintf(msg, sizeof(msg), "The TL on line %d references tag %d (first endpoint), which was not found.", i + 1, tag1);
L882:           snprintf(msg, sizeof(msg), "The TL on line %d references tag %d (first endpoint), which is marked as ignored.", i + 1, tag1);
L899:           snprintf(msg, sizeof(msg), "The TL on line %d references tag %d (second endpoint), which was not found.", i + 1, tag2);
L904:           snprintf(msg, sizeof(msg), "The TL on line %d references tag %d (second endpoint), which is marked as ignored.", i + 1, tag2);
L925:           snprintf(msg, sizeof(msg), "The card on line %d is an LD referencing tag %d, which was not found.", i + 1, tag);
L930:           snprintf(msg, sizeof(msg), "The card on line %d is an LD referencing tag %d, which is marked as ignored.", i + 1, tag);
L964:           snprintf(msg, sizeof(msg), "The EX on line %d is placed on segment 1 of tag %d, which is an open wire end.", ex_refs[er].line, w.tag);
L969:           snprintf(msg, sizeof(msg), "The EX on line %d is placed on segment %d of tag %d, which is an open wire end.", ex_refs[er].line, w.segs, w.tag);
L983:           snprintf(msg, sizeof(msg), "The LD on line %d starts at segment 1 of tag %d, which is an open wire end.", ld_refs[lr].line, w.tag);
L990:           snprintf(msg, sizeof(msg), "The LD on line %d ends at segment %d of tag %d, which is an open wire end.", ld_refs[lr].line, w.segs, w.tag);
L1027:         snprintf(msg, sizeof(msg), "The TL on line %d references an invalid segment on the first endpoint; segment index out of range for the wire tag %d.", tl_refs[r].line, tl_refs[r].tag1);
L1032:         snprintf(msg, sizeof(msg), "The TL on line %d references an invalid segment on the second endpoint; segment index out of range for the wire tag %d.", tl_refs[r].line, tl_refs[r].tag2);
L1038:         snprintf(msg, sizeof(msg), "The TL on line %d connects the same tag and segment on both ends; this is a no-op or invalid connection.", tl_refs[r].line);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 1065
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 1005-1070) ---
   1005:   }
   1006:   if (wire_count > 0)
   1007:   {
   1008:     check_ge_low_height_hazard(ctx, errors, wires, wire_count, GEType);
   1009:     check_junction_segmentation_consistency(ctx, errors, wires, wire_count);
   1010:   }
   1011: 
   1012:   // TL segment bounds validation: ensure referenced segments exist on the given tags
   1013:   if (tl_ref_count > 0 && wire_count > 0)
   1014:   {
   1015:     for (int r = 0; r < tl_ref_count; r++)
   1016:     {
   1017:       int segs1 = -1, segs2 = -1;
   1018:       for (int w = 0; w < wire_count; w++)
   1019:       {
   1020:         if (wires[w].tag == tl_refs[r].tag1)
   1021:           segs1 = wires[w].segs;
   1022:         if (wires[w].tag == tl_refs[r].tag2)
   1023:           segs2 = wires[w].segs;
   1024:       }
   1025:       if (segs1 > 0 && (tl_refs[r].seg1 <= 0 || tl_refs[r].seg1 > segs1))
   1026:       {
   1027:         snprintf(msg, sizeof(msg), "The TL on line %d references an invalid segment on the first endpoint; segment index out of range for the wire tag %d.", tl_refs[r].line, tl_refs[r].tag1);
   1028:         add_error(ctx, errors, msg, 0);
   1029:       }
   1030:       if (segs2 > 0 && (tl_refs[r].seg2 <= 0 || tl_refs[r].seg2 > segs2))
   1031:       {
   1032:         snprintf(msg, sizeof(msg), "The TL on line %d references an invalid segment on the second endpoint; segment index out of range for the wire tag %d.", tl_refs[r].line, tl_refs[r].tag2);
   1033:         add_error(ctx, errors, msg, 0);
   1034:       }
   1035:       // TL self-loop: same tag+segment on both ends
   1036:       if (tl_refs[r].tag1 == tl_refs[r].tag2 && tl_refs[r].seg1 == tl_refs[r].seg2)
   1037:       {
   1038:         snprintf(msg, sizeof(msg), "The TL on line %d connects the same tag and segment on both ends; this is a no-op or invalid connection.", tl_refs[r].line);
   1039:         add_error(ctx, errors, msg, 0);
   1040:       }
   1041:     }
   1042:   }
   1043: 
   1044:   // LD segment bounds validation: ensure referenced segment range falls within the wire's segment count
   1045:   if (ld_ref_count > 0 && wire_count > 0)
   1046:   {
   1047:     for (int r = 0; r < ld_ref_count; r++)
   1048:     {
   1049:       int segs = -1;
   1050:       for (int w = 0; w < wire_count; w++)
   1051:       {
   1052:         if (wires[w].tag == ld_refs[r].tag)
   1053:         {
   1054:           segs = wires[w].segs;
   1055:           break;
   1056:         }
   1057:       }
   1058:       if (segs > 0)
   1059:       {
   1060:         int s = ld_refs[r].segStart;
   1061:         int e = ld_refs[r].segEnd == 0 ? ld_refs[r].segStart : ld_refs[r].segEnd;
   1062:         if (s <= 0 || s > segs)
   1063:         {
   1064:           snprintf(msg, sizeof(msg), "The LD on line %d references an invalid start segment; out of range for wire tag %d.", ld_refs[r].line, ld_refs[r].tag);
>  1065:           add_error(ctx, errors, msg, 0);
   1066:         }
   1067:         if (e <= 0 || e > segs)
   1068:         {
   1069:           snprintf(msg, sizeof(msg), "The LD on line %d references an invalid end segment; out of range for wire tag %d.", ld_refs[r].line, ld_refs[r].tag);
   1070:           add_error(ctx, errors, msg, 0);

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L877:           snprintf(msg, sizeof(msg), "The TL on line %d references tag %d (first endpoint), which was not found.", i + 1, tag1);
L882:           snprintf(msg, sizeof(msg), "The TL on line %d references tag %d (first endpoint), which is marked as ignored.", i + 1, tag1);
L899:           snprintf(msg, sizeof(msg), "The TL on line %d references tag %d (second endpoint), which was not found.", i + 1, tag2);
L904:           snprintf(msg, sizeof(msg), "The TL on line %d references tag %d (second endpoint), which is marked as ignored.", i + 1, tag2);
L925:           snprintf(msg, sizeof(msg), "The card on line %d is an LD referencing tag %d, which was not found.", i + 1, tag);
L930:           snprintf(msg, sizeof(msg), "The card on line %d is an LD referencing tag %d, which is marked as ignored.", i + 1, tag);
L964:           snprintf(msg, sizeof(msg), "The EX on line %d is placed on segment 1 of tag %d, which is an open wire end.", ex_refs[er].line, w.tag);
L969:           snprintf(msg, sizeof(msg), "The EX on line %d is placed on segment %d of tag %d, which is an open wire end.", ex_refs[er].line, w.segs, w.tag);
L983:           snprintf(msg, sizeof(msg), "The LD on line %d starts at segment 1 of tag %d, which is an open wire end.", ld_refs[lr].line, w.tag);
L990:           snprintf(msg, sizeof(msg), "The LD on line %d ends at segment %d of tag %d, which is an open wire end.", ld_refs[lr].line, w.segs, w.tag);
L1027:         snprintf(msg, sizeof(msg), "The TL on line %d references an invalid segment on the first endpoint; segment index out of range for the wire tag %d.", tl_refs[r].line, tl_refs[r].tag1);
L1032:         snprintf(msg, sizeof(msg), "The TL on line %d references an invalid segment on the second endpoint; segment index out of range for the wire tag %d.", tl_refs[r].line, tl_refs[r].tag2);
L1038:         snprintf(msg, sizeof(msg), "The TL on line %d connects the same tag and segment on both ends; this is a no-op or invalid connection.", tl_refs[r].line);
L1064:           snprintf(msg, sizeof(msg), "The LD on line %d references an invalid start segment; out of range for wire tag %d.", ld_refs[r].line, ld_refs[r].tag);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 1070
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 1010-1075) ---
   1010:   }
   1011: 
   1012:   // TL segment bounds validation: ensure referenced segments exist on the given tags
   1013:   if (tl_ref_count > 0 && wire_count > 0)
   1014:   {
   1015:     for (int r = 0; r < tl_ref_count; r++)
   1016:     {
   1017:       int segs1 = -1, segs2 = -1;
   1018:       for (int w = 0; w < wire_count; w++)
   1019:       {
   1020:         if (wires[w].tag == tl_refs[r].tag1)
   1021:           segs1 = wires[w].segs;
   1022:         if (wires[w].tag == tl_refs[r].tag2)
   1023:           segs2 = wires[w].segs;
   1024:       }
   1025:       if (segs1 > 0 && (tl_refs[r].seg1 <= 0 || tl_refs[r].seg1 > segs1))
   1026:       {
   1027:         snprintf(msg, sizeof(msg), "The TL on line %d references an invalid segment on the first endpoint; segment index out of range for the wire tag %d.", tl_refs[r].line, tl_refs[r].tag1);
   1028:         add_error(ctx, errors, msg, 0);
   1029:       }
   1030:       if (segs2 > 0 && (tl_refs[r].seg2 <= 0 || tl_refs[r].seg2 > segs2))
   1031:       {
   1032:         snprintf(msg, sizeof(msg), "The TL on line %d references an invalid segment on the second endpoint; segment index out of range for the wire tag %d.", tl_refs[r].line, tl_refs[r].tag2);
   1033:         add_error(ctx, errors, msg, 0);
   1034:       }
   1035:       // TL self-loop: same tag+segment on both ends
   1036:       if (tl_refs[r].tag1 == tl_refs[r].tag2 && tl_refs[r].seg1 == tl_refs[r].seg2)
   1037:       {
   1038:         snprintf(msg, sizeof(msg), "The TL on line %d connects the same tag and segment on both ends; this is a no-op or invalid connection.", tl_refs[r].line);
   1039:         add_error(ctx, errors, msg, 0);
   1040:       }
   1041:     }
   1042:   }
   1043: 
   1044:   // LD segment bounds validation: ensure referenced segment range falls within the wire's segment count
   1045:   if (ld_ref_count > 0 && wire_count > 0)
   1046:   {
   1047:     for (int r = 0; r < ld_ref_count; r++)
   1048:     {
   1049:       int segs = -1;
   1050:       for (int w = 0; w < wire_count; w++)
   1051:       {
   1052:         if (wires[w].tag == ld_refs[r].tag)
   1053:         {
   1054:           segs = wires[w].segs;
   1055:           break;
   1056:         }
   1057:       }
   1058:       if (segs > 0)
   1059:       {
   1060:         int s = ld_refs[r].segStart;
   1061:         int e = ld_refs[r].segEnd == 0 ? ld_refs[r].segStart : ld_refs[r].segEnd;
   1062:         if (s <= 0 || s > segs)
   1063:         {
   1064:           snprintf(msg, sizeof(msg), "The LD on line %d references an invalid start segment; out of range for wire tag %d.", ld_refs[r].line, ld_refs[r].tag);
   1065:           add_error(ctx, errors, msg, 0);
   1066:         }
   1067:         if (e <= 0 || e > segs)
   1068:         {
   1069:           snprintf(msg, sizeof(msg), "The LD on line %d references an invalid end segment; out of range for wire tag %d.", ld_refs[r].line, ld_refs[r].tag);
>  1070:           add_error(ctx, errors, msg, 0);
   1071:         }
   1072:       }
   1073:     }
   1074:   }
   1075: 

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L877:           snprintf(msg, sizeof(msg), "The TL on line %d references tag %d (first endpoint), which was not found.", i + 1, tag1);
L882:           snprintf(msg, sizeof(msg), "The TL on line %d references tag %d (first endpoint), which is marked as ignored.", i + 1, tag1);
L899:           snprintf(msg, sizeof(msg), "The TL on line %d references tag %d (second endpoint), which was not found.", i + 1, tag2);
L904:           snprintf(msg, sizeof(msg), "The TL on line %d references tag %d (second endpoint), which is marked as ignored.", i + 1, tag2);
L925:           snprintf(msg, sizeof(msg), "The card on line %d is an LD referencing tag %d, which was not found.", i + 1, tag);
L930:           snprintf(msg, sizeof(msg), "The card on line %d is an LD referencing tag %d, which is marked as ignored.", i + 1, tag);
L964:           snprintf(msg, sizeof(msg), "The EX on line %d is placed on segment 1 of tag %d, which is an open wire end.", ex_refs[er].line, w.tag);
L969:           snprintf(msg, sizeof(msg), "The EX on line %d is placed on segment %d of tag %d, which is an open wire end.", ex_refs[er].line, w.segs, w.tag);
L983:           snprintf(msg, sizeof(msg), "The LD on line %d starts at segment 1 of tag %d, which is an open wire end.", ld_refs[lr].line, w.tag);
L990:           snprintf(msg, sizeof(msg), "The LD on line %d ends at segment %d of tag %d, which is an open wire end.", ld_refs[lr].line, w.segs, w.tag);
L1027:         snprintf(msg, sizeof(msg), "The TL on line %d references an invalid segment on the first endpoint; segment index out of range for the wire tag %d.", tl_refs[r].line, tl_refs[r].tag1);
L1032:         snprintf(msg, sizeof(msg), "The TL on line %d references an invalid segment on the second endpoint; segment index out of range for the wire tag %d.", tl_refs[r].line, tl_refs[r].tag2);
L1038:         snprintf(msg, sizeof(msg), "The TL on line %d connects the same tag and segment on both ends; this is a no-op or invalid connection.", tl_refs[r].line);
L1064:           snprintf(msg, sizeof(msg), "The LD on line %d references an invalid start segment; out of range for wire tag %d.", ld_refs[r].line, ld_refs[r].tag);
L1069:           snprintf(msg, sizeof(msg), "The LD on line %d references an invalid end segment; out of range for wire tag %d.", ld_refs[r].line, ld_refs[r].tag);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 1088
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 1028-1093) ---
   1028:         add_error(ctx, errors, msg, 0);
   1029:       }
   1030:       if (segs2 > 0 && (tl_refs[r].seg2 <= 0 || tl_refs[r].seg2 > segs2))
   1031:       {
   1032:         snprintf(msg, sizeof(msg), "The TL on line %d references an invalid segment on the second endpoint; segment index out of range for the wire tag %d.", tl_refs[r].line, tl_refs[r].tag2);
   1033:         add_error(ctx, errors, msg, 0);
   1034:       }
   1035:       // TL self-loop: same tag+segment on both ends
   1036:       if (tl_refs[r].tag1 == tl_refs[r].tag2 && tl_refs[r].seg1 == tl_refs[r].seg2)
   1037:       {
   1038:         snprintf(msg, sizeof(msg), "The TL on line %d connects the same tag and segment on both ends; this is a no-op or invalid connection.", tl_refs[r].line);
   1039:         add_error(ctx, errors, msg, 0);
   1040:       }
   1041:     }
   1042:   }
   1043: 
   1044:   // LD segment bounds validation: ensure referenced segment range falls within the wire's segment count
   1045:   if (ld_ref_count > 0 && wire_count > 0)
   1046:   {
   1047:     for (int r = 0; r < ld_ref_count; r++)
   1048:     {
   1049:       int segs = -1;
   1050:       for (int w = 0; w < wire_count; w++)
   1051:       {
   1052:         if (wires[w].tag == ld_refs[r].tag)
   1053:         {
   1054:           segs = wires[w].segs;
   1055:           break;
   1056:         }
   1057:       }
   1058:       if (segs > 0)
   1059:       {
   1060:         int s = ld_refs[r].segStart;
   1061:         int e = ld_refs[r].segEnd == 0 ? ld_refs[r].segStart : ld_refs[r].segEnd;
   1062:         if (s <= 0 || s > segs)
   1063:         {
   1064:           snprintf(msg, sizeof(msg), "The LD on line %d references an invalid start segment; out of range for wire tag %d.", ld_refs[r].line, ld_refs[r].tag);
   1065:           add_error(ctx, errors, msg, 0);
   1066:         }
   1067:         if (e <= 0 || e > segs)
   1068:         {
   1069:           snprintf(msg, sizeof(msg), "The LD on line %d references an invalid end segment; out of range for wire tag %d.", ld_refs[r].line, ld_refs[r].tag);
   1070:           add_error(ctx, errors, msg, 0);
   1071:         }
   1072:       }
   1073:     }
   1074:   }
   1075: 
   1076:   // duplicate wire endpoints: overlapping wires
   1077:   for (int i = 0; i < wire_count; i++)
   1078:   {
   1079:     for (int j = i + 1; j < wire_count; j++)
   1080:     {
   1081:       wire_info_t a = wires[i];
   1082:       wire_info_t b = wires[j];
   1083:       int same_dir = (a.x1 == b.x1 && a.y1 == b.y1 && a.z1 == b.z1 && a.x2 == b.x2 && a.y2 == b.y2 && a.z2 == b.z2);
   1084:       int reversed = (a.x1 == b.x2 && a.y1 == b.y2 && a.z1 == b.z2 && a.x2 == b.x1 && a.y2 == b.y1 && a.z2 == b.z1);
   1085:       if (same_dir || reversed)
   1086:       {
   1087:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has the same endpoints as the wire on line %d (tag %d). Overlapping wires can cause issues.", a.line, a.tag, b.line, b.tag);
>  1088:         add_error(ctx, errors, msg, 0);
   1089:       }
   1090:     }
   1091:   }
   1092: 
   1093:   // Ground intersection: warn if wires cross or extend below z=0 when ground is enabled

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L899:           snprintf(msg, sizeof(msg), "The TL on line %d references tag %d (second endpoint), which was not found.", i + 1, tag2);
L904:           snprintf(msg, sizeof(msg), "The TL on line %d references tag %d (second endpoint), which is marked as ignored.", i + 1, tag2);
L925:           snprintf(msg, sizeof(msg), "The card on line %d is an LD referencing tag %d, which was not found.", i + 1, tag);
L930:           snprintf(msg, sizeof(msg), "The card on line %d is an LD referencing tag %d, which is marked as ignored.", i + 1, tag);
L964:           snprintf(msg, sizeof(msg), "The EX on line %d is placed on segment 1 of tag %d, which is an open wire end.", ex_refs[er].line, w.tag);
L969:           snprintf(msg, sizeof(msg), "The EX on line %d is placed on segment %d of tag %d, which is an open wire end.", ex_refs[er].line, w.segs, w.tag);
L983:           snprintf(msg, sizeof(msg), "The LD on line %d starts at segment 1 of tag %d, which is an open wire end.", ld_refs[lr].line, w.tag);
L990:           snprintf(msg, sizeof(msg), "The LD on line %d ends at segment %d of tag %d, which is an open wire end.", ld_refs[lr].line, w.segs, w.tag);
L1027:         snprintf(msg, sizeof(msg), "The TL on line %d references an invalid segment on the first endpoint; segment index out of range for the wire tag %d.", tl_refs[r].line, tl_refs[r].tag1);
L1032:         snprintf(msg, sizeof(msg), "The TL on line %d references an invalid segment on the second endpoint; segment index out of range for the wire tag %d.", tl_refs[r].line, tl_refs[r].tag2);
L1038:         snprintf(msg, sizeof(msg), "The TL on line %d connects the same tag and segment on both ends; this is a no-op or invalid connection.", tl_refs[r].line);
L1064:           snprintf(msg, sizeof(msg), "The LD on line %d references an invalid start segment; out of range for wire tag %d.", ld_refs[r].line, ld_refs[r].tag);
L1069:           snprintf(msg, sizeof(msg), "The LD on line %d references an invalid end segment; out of range for wire tag %d.", ld_refs[r].line, ld_refs[r].tag);
L1087:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has the same endpoints as the wire on line %d (tag %d). Overlapping wires can cause issues.", a.line, a.tag, b.line, b.tag);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 1104
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 1044-1109) ---
   1044:   // LD segment bounds validation: ensure referenced segment range falls within the wire's segment count
   1045:   if (ld_ref_count > 0 && wire_count > 0)
   1046:   {
   1047:     for (int r = 0; r < ld_ref_count; r++)
   1048:     {
   1049:       int segs = -1;
   1050:       for (int w = 0; w < wire_count; w++)
   1051:       {
   1052:         if (wires[w].tag == ld_refs[r].tag)
   1053:         {
   1054:           segs = wires[w].segs;
   1055:           break;
   1056:         }
   1057:       }
   1058:       if (segs > 0)
   1059:       {
   1060:         int s = ld_refs[r].segStart;
   1061:         int e = ld_refs[r].segEnd == 0 ? ld_refs[r].segStart : ld_refs[r].segEnd;
   1062:         if (s <= 0 || s > segs)
   1063:         {
   1064:           snprintf(msg, sizeof(msg), "The LD on line %d references an invalid start segment; out of range for wire tag %d.", ld_refs[r].line, ld_refs[r].tag);
   1065:           add_error(ctx, errors, msg, 0);
   1066:         }
   1067:         if (e <= 0 || e > segs)
   1068:         {
   1069:           snprintf(msg, sizeof(msg), "The LD on line %d references an invalid end segment; out of range for wire tag %d.", ld_refs[r].line, ld_refs[r].tag);
   1070:           add_error(ctx, errors, msg, 0);
   1071:         }
   1072:       }
   1073:     }
   1074:   }
   1075: 
   1076:   // duplicate wire endpoints: overlapping wires
   1077:   for (int i = 0; i < wire_count; i++)
   1078:   {
   1079:     for (int j = i + 1; j < wire_count; j++)
   1080:     {
   1081:       wire_info_t a = wires[i];
   1082:       wire_info_t b = wires[j];
   1083:       int same_dir = (a.x1 == b.x1 && a.y1 == b.y1 && a.z1 == b.z1 && a.x2 == b.x2 && a.y2 == b.y2 && a.z2 == b.z2);
   1084:       int reversed = (a.x1 == b.x2 && a.y1 == b.y2 && a.z1 == b.z2 && a.x2 == b.x1 && a.y2 == b.y1 && a.z2 == b.z1);
   1085:       if (same_dir || reversed)
   1086:       {
   1087:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has the same endpoints as the wire on line %d (tag %d). Overlapping wires can cause issues.", a.line, a.tag, b.line, b.tag);
   1088:         add_error(ctx, errors, msg, 0);
   1089:       }
   1090:     }
   1091:   }
   1092: 
   1093:   // Ground intersection: warn if wires cross or extend below z=0 when ground is enabled
   1094:   int ground_enabled = ((GEType == 1 || GEType == 2) || sawGN);
   1095:   if (ground_enabled)
   1096:   {
   1097:     for (int wi = 0; wi < wire_count; wi++)
   1098:     {
   1099:       wire_info_t w = wires[wi];
   1100:       double z1 = w.z1, z2 = w.z2;
   1101:       if (z1 < 0.0 && z2 < 0.0)
   1102:       {
   1103:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) lies below the ground plane (z<0).", w.line, w.tag);
>  1104:         add_error(ctx, errors, msg, 0);
   1105:       }
   1106:       else if (z1 * z2 < 0.0)
   1107:       {
   1108:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) crosses the ground plane (z=0), which is typically invalid.", w.line, w.tag);
   1109:         add_error(ctx, errors, msg, 0);

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L904:           snprintf(msg, sizeof(msg), "The TL on line %d references tag %d (second endpoint), which is marked as ignored.", i + 1, tag2);
L925:           snprintf(msg, sizeof(msg), "The card on line %d is an LD referencing tag %d, which was not found.", i + 1, tag);
L930:           snprintf(msg, sizeof(msg), "The card on line %d is an LD referencing tag %d, which is marked as ignored.", i + 1, tag);
L964:           snprintf(msg, sizeof(msg), "The EX on line %d is placed on segment 1 of tag %d, which is an open wire end.", ex_refs[er].line, w.tag);
L969:           snprintf(msg, sizeof(msg), "The EX on line %d is placed on segment %d of tag %d, which is an open wire end.", ex_refs[er].line, w.segs, w.tag);
L983:           snprintf(msg, sizeof(msg), "The LD on line %d starts at segment 1 of tag %d, which is an open wire end.", ld_refs[lr].line, w.tag);
L990:           snprintf(msg, sizeof(msg), "The LD on line %d ends at segment %d of tag %d, which is an open wire end.", ld_refs[lr].line, w.segs, w.tag);
L1027:         snprintf(msg, sizeof(msg), "The TL on line %d references an invalid segment on the first endpoint; segment index out of range for the wire tag %d.", tl_refs[r].line, tl_refs[r].tag1);
L1032:         snprintf(msg, sizeof(msg), "The TL on line %d references an invalid segment on the second endpoint; segment index out of range for the wire tag %d.", tl_refs[r].line, tl_refs[r].tag2);
L1038:         snprintf(msg, sizeof(msg), "The TL on line %d connects the same tag and segment on both ends; this is a no-op or invalid connection.", tl_refs[r].line);
L1064:           snprintf(msg, sizeof(msg), "The LD on line %d references an invalid start segment; out of range for wire tag %d.", ld_refs[r].line, ld_refs[r].tag);
L1069:           snprintf(msg, sizeof(msg), "The LD on line %d references an invalid end segment; out of range for wire tag %d.", ld_refs[r].line, ld_refs[r].tag);
L1087:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has the same endpoints as the wire on line %d (tag %d). Overlapping wires can cause issues.", a.line, a.tag, b.line, b.tag);
L1103:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) lies below the ground plane (z<0).", w.line, w.tag);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 1109
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 1049-1114) ---
   1049:       int segs = -1;
   1050:       for (int w = 0; w < wire_count; w++)
   1051:       {
   1052:         if (wires[w].tag == ld_refs[r].tag)
   1053:         {
   1054:           segs = wires[w].segs;
   1055:           break;
   1056:         }
   1057:       }
   1058:       if (segs > 0)
   1059:       {
   1060:         int s = ld_refs[r].segStart;
   1061:         int e = ld_refs[r].segEnd == 0 ? ld_refs[r].segStart : ld_refs[r].segEnd;
   1062:         if (s <= 0 || s > segs)
   1063:         {
   1064:           snprintf(msg, sizeof(msg), "The LD on line %d references an invalid start segment; out of range for wire tag %d.", ld_refs[r].line, ld_refs[r].tag);
   1065:           add_error(ctx, errors, msg, 0);
   1066:         }
   1067:         if (e <= 0 || e > segs)
   1068:         {
   1069:           snprintf(msg, sizeof(msg), "The LD on line %d references an invalid end segment; out of range for wire tag %d.", ld_refs[r].line, ld_refs[r].tag);
   1070:           add_error(ctx, errors, msg, 0);
   1071:         }
   1072:       }
   1073:     }
   1074:   }
   1075: 
   1076:   // duplicate wire endpoints: overlapping wires
   1077:   for (int i = 0; i < wire_count; i++)
   1078:   {
   1079:     for (int j = i + 1; j < wire_count; j++)
   1080:     {
   1081:       wire_info_t a = wires[i];
   1082:       wire_info_t b = wires[j];
   1083:       int same_dir = (a.x1 == b.x1 && a.y1 == b.y1 && a.z1 == b.z1 && a.x2 == b.x2 && a.y2 == b.y2 && a.z2 == b.z2);
   1084:       int reversed = (a.x1 == b.x2 && a.y1 == b.y2 && a.z1 == b.z2 && a.x2 == b.x1 && a.y2 == b.y1 && a.z2 == b.z1);
   1085:       if (same_dir || reversed)
   1086:       {
   1087:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has the same endpoints as the wire on line %d (tag %d). Overlapping wires can cause issues.", a.line, a.tag, b.line, b.tag);
   1088:         add_error(ctx, errors, msg, 0);
   1089:       }
   1090:     }
   1091:   }
   1092: 
   1093:   // Ground intersection: warn if wires cross or extend below z=0 when ground is enabled
   1094:   int ground_enabled = ((GEType == 1 || GEType == 2) || sawGN);
   1095:   if (ground_enabled)
   1096:   {
   1097:     for (int wi = 0; wi < wire_count; wi++)
   1098:     {
   1099:       wire_info_t w = wires[wi];
   1100:       double z1 = w.z1, z2 = w.z2;
   1101:       if (z1 < 0.0 && z2 < 0.0)
   1102:       {
   1103:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) lies below the ground plane (z<0).", w.line, w.tag);
   1104:         add_error(ctx, errors, msg, 0);
   1105:       }
   1106:       else if (z1 * z2 < 0.0)
   1107:       {
   1108:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) crosses the ground plane (z=0), which is typically invalid.", w.line, w.tag);
>  1109:         add_error(ctx, errors, msg, 0);
   1110:       }
   1111:     }
   1112:   }
   1113: 
   1114:   // wires that are connected must contact at segment ends (connection separation < len/1000)

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L925:           snprintf(msg, sizeof(msg), "The card on line %d is an LD referencing tag %d, which was not found.", i + 1, tag);
L930:           snprintf(msg, sizeof(msg), "The card on line %d is an LD referencing tag %d, which is marked as ignored.", i + 1, tag);
L964:           snprintf(msg, sizeof(msg), "The EX on line %d is placed on segment 1 of tag %d, which is an open wire end.", ex_refs[er].line, w.tag);
L969:           snprintf(msg, sizeof(msg), "The EX on line %d is placed on segment %d of tag %d, which is an open wire end.", ex_refs[er].line, w.segs, w.tag);
L983:           snprintf(msg, sizeof(msg), "The LD on line %d starts at segment 1 of tag %d, which is an open wire end.", ld_refs[lr].line, w.tag);
L990:           snprintf(msg, sizeof(msg), "The LD on line %d ends at segment %d of tag %d, which is an open wire end.", ld_refs[lr].line, w.segs, w.tag);
L1027:         snprintf(msg, sizeof(msg), "The TL on line %d references an invalid segment on the first endpoint; segment index out of range for the wire tag %d.", tl_refs[r].line, tl_refs[r].tag1);
L1032:         snprintf(msg, sizeof(msg), "The TL on line %d references an invalid segment on the second endpoint; segment index out of range for the wire tag %d.", tl_refs[r].line, tl_refs[r].tag2);
L1038:         snprintf(msg, sizeof(msg), "The TL on line %d connects the same tag and segment on both ends; this is a no-op or invalid connection.", tl_refs[r].line);
L1064:           snprintf(msg, sizeof(msg), "The LD on line %d references an invalid start segment; out of range for wire tag %d.", ld_refs[r].line, ld_refs[r].tag);
L1069:           snprintf(msg, sizeof(msg), "The LD on line %d references an invalid end segment; out of range for wire tag %d.", ld_refs[r].line, ld_refs[r].tag);
L1087:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has the same endpoints as the wire on line %d (tag %d). Overlapping wires can cause issues.", a.line, a.tag, b.line, b.tag);
L1103:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) lies below the ground plane (z<0).", w.line, w.tag);
L1108:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) crosses the ground plane (z=0), which is typically invalid.", w.line, w.tag);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 1163
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 1103-1168) ---
   1103:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) lies below the ground plane (z<0).", w.line, w.tag);
   1104:         add_error(ctx, errors, msg, 0);
   1105:       }
   1106:       else if (z1 * z2 < 0.0)
   1107:       {
   1108:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) crosses the ground plane (z=0), which is typically invalid.", w.line, w.tag);
   1109:         add_error(ctx, errors, msg, 0);
   1110:       }
   1111:     }
   1112:   }
   1113: 
   1114:   // wires that are connected must contact at segment ends (connection separation < len/1000)
   1115:   for (int ai = 0; ai < wire_count; ai++)
   1116:   {
   1117:     wire_info_t a = wires[ai];
   1118:     double ax[2] = {a.x1, a.x2};
   1119:     double ay[2] = {a.y1, a.y2};
   1120:     double az[2] = {a.z1, a.z2};
   1121:     for (int end = 0; end < 2; end++)
   1122:     {
   1123:       double px = ax[end], py = ay[end], pz = az[end];
   1124:       for (int bi = 0; bi < wire_count; bi++)
   1125:       {
   1126:         if (bi == ai)
   1127:           continue;
   1128:         wire_info_t b = wires[bi];
   1129:         double vx = b.x2 - b.x1, vy = b.y2 - b.y1, vz = b.z2 - b.z1;
   1130:         double L2 = vx * vx + vy * vy + vz * vz;
   1131:         if (L2 == 0.0 || b.segs <= 0)
   1132:           continue;
   1133:         double L = sqrt(L2);
   1134:         double segLen = L / (double)b.segs;
   1135:         double tol = segLen / 1000.0;
   1136:         // project point onto line b
   1137:         double wx = px - b.x1, wy = py - b.y1, wz = pz - b.z1;
   1138:         double t = (wx * vx + wy * vy + wz * vz) / L2;
   1139:         if (t < 0.0 || t > 1.0)
   1140:           continue; // closest point lies outside the wire extent
   1141:         double fx = b.x1 + t * vx, fy = b.y1 + t * vy, fz = b.z1 + t * vz;
   1142:         double dx = px - fx, dy = py - fy, dz = pz - fz;
   1143:         double dist = sqrt(dx * dx + dy * dy + dz * dz);
   1144:         if (dist <= tol)
   1145:         {
   1146:           // near-connected; ensure this footpoint is at a segment endpoint of b
   1147:           int isEndpoint = 0;
   1148:           for (int k = 0; k <= b.segs; k++)
   1149:           {
   1150:             double ek = (double)k / (double)b.segs;
   1151:             double ex = b.x1 + ek * vx, ey = b.y1 + ek * vy, ez = b.z1 + ek * vz;
   1152:             double edx = fx - ex, edy = fy - ey, edz = fz - ez;
   1153:             double ed = sqrt(edx * edx + edy * edy + edz * edz);
   1154:             if (ed <= tol)
   1155:             {
   1156:               isEndpoint = 1;
   1157:               break;
   1158:             }
   1159:           }
   1160:           if (!isEndpoint)
   1161:           {
   1162:             snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) endpoint is connected near the middle of the wire on line %d (tag %d). Connections must be at segment ends.", a.line, a.tag, b.line, b.tag);
>  1163:             add_error(ctx, errors, msg, 0);
   1164:           }
   1165:         }
   1166:       }
   1167:     }
   1168:   }

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L964:           snprintf(msg, sizeof(msg), "The EX on line %d is placed on segment 1 of tag %d, which is an open wire end.", ex_refs[er].line, w.tag);
L969:           snprintf(msg, sizeof(msg), "The EX on line %d is placed on segment %d of tag %d, which is an open wire end.", ex_refs[er].line, w.segs, w.tag);
L983:           snprintf(msg, sizeof(msg), "The LD on line %d starts at segment 1 of tag %d, which is an open wire end.", ld_refs[lr].line, w.tag);
L990:           snprintf(msg, sizeof(msg), "The LD on line %d ends at segment %d of tag %d, which is an open wire end.", ld_refs[lr].line, w.segs, w.tag);
L1027:         snprintf(msg, sizeof(msg), "The TL on line %d references an invalid segment on the first endpoint; segment index out of range for the wire tag %d.", tl_refs[r].line, tl_refs[r].tag1);
L1032:         snprintf(msg, sizeof(msg), "The TL on line %d references an invalid segment on the second endpoint; segment index out of range for the wire tag %d.", tl_refs[r].line, tl_refs[r].tag2);
L1038:         snprintf(msg, sizeof(msg), "The TL on line %d connects the same tag and segment on both ends; this is a no-op or invalid connection.", tl_refs[r].line);
L1064:           snprintf(msg, sizeof(msg), "The LD on line %d references an invalid start segment; out of range for wire tag %d.", ld_refs[r].line, ld_refs[r].tag);
L1069:           snprintf(msg, sizeof(msg), "The LD on line %d references an invalid end segment; out of range for wire tag %d.", ld_refs[r].line, ld_refs[r].tag);
L1087:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has the same endpoints as the wire on line %d (tag %d). Overlapping wires can cause issues.", a.line, a.tag, b.line, b.tag);
L1103:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) lies below the ground plane (z<0).", w.line, w.tag);
L1108:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) crosses the ground plane (z=0), which is typically invalid.", w.line, w.tag);
L1162:             snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) endpoint is connected near the middle of the wire on line %d (tag %d). Connections must be at segment ends.", a.line, a.tag, b.line, b.tag);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 1174
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 1114-1179) ---
   1114:   // wires that are connected must contact at segment ends (connection separation < len/1000)
   1115:   for (int ai = 0; ai < wire_count; ai++)
   1116:   {
   1117:     wire_info_t a = wires[ai];
   1118:     double ax[2] = {a.x1, a.x2};
   1119:     double ay[2] = {a.y1, a.y2};
   1120:     double az[2] = {a.z1, a.z2};
   1121:     for (int end = 0; end < 2; end++)
   1122:     {
   1123:       double px = ax[end], py = ay[end], pz = az[end];
   1124:       for (int bi = 0; bi < wire_count; bi++)
   1125:       {
   1126:         if (bi == ai)
   1127:           continue;
   1128:         wire_info_t b = wires[bi];
   1129:         double vx = b.x2 - b.x1, vy = b.y2 - b.y1, vz = b.z2 - b.z1;
   1130:         double L2 = vx * vx + vy * vy + vz * vz;
   1131:         if (L2 == 0.0 || b.segs <= 0)
   1132:           continue;
   1133:         double L = sqrt(L2);
   1134:         double segLen = L / (double)b.segs;
   1135:         double tol = segLen / 1000.0;
   1136:         // project point onto line b
   1137:         double wx = px - b.x1, wy = py - b.y1, wz = pz - b.z1;
   1138:         double t = (wx * vx + wy * vy + wz * vz) / L2;
   1139:         if (t < 0.0 || t > 1.0)
   1140:           continue; // closest point lies outside the wire extent
   1141:         double fx = b.x1 + t * vx, fy = b.y1 + t * vy, fz = b.z1 + t * vz;
   1142:         double dx = px - fx, dy = py - fy, dz = pz - fz;
   1143:         double dist = sqrt(dx * dx + dy * dy + dz * dz);
   1144:         if (dist <= tol)
   1145:         {
   1146:           // near-connected; ensure this footpoint is at a segment endpoint of b
   1147:           int isEndpoint = 0;
   1148:           for (int k = 0; k <= b.segs; k++)
   1149:           {
   1150:             double ek = (double)k / (double)b.segs;
   1151:             double ex = b.x1 + ek * vx, ey = b.y1 + ek * vy, ez = b.z1 + ek * vz;
   1152:             double edx = fx - ex, edy = fy - ey, edz = fz - ez;
   1153:             double ed = sqrt(edx * edx + edy * edy + edz * edz);
   1154:             if (ed <= tol)
   1155:             {
   1156:               isEndpoint = 1;
   1157:               break;
   1158:             }
   1159:           }
   1160:           if (!isEndpoint)
   1161:           {
   1162:             snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) endpoint is connected near the middle of the wire on line %d (tag %d). Connections must be at segment ends.", a.line, a.tag, b.line, b.tag);
   1163:             add_error(ctx, errors, msg, 0);
   1164:           }
   1165:         }
   1166:       }
   1167:     }
   1168:   }
   1169: 
   1170:   // and with the entire deck tested, make sure we got the key cards
   1171:   if (!sawCE)
   1172:   {
   1173:     snprintf(msg, sizeof(msg), "A deck should have a CE card.");
>  1174:     add_error(ctx, errors, msg, 0);
   1175:   }
   1176:   if (!sawGx)
   1177:   {
   1178:     snprintf(msg, sizeof(msg), "A deck has to have at least one geometry card.");
   1179:     add_error(ctx, errors, msg, 1);

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L983:           snprintf(msg, sizeof(msg), "The LD on line %d starts at segment 1 of tag %d, which is an open wire end.", ld_refs[lr].line, w.tag);
L990:           snprintf(msg, sizeof(msg), "The LD on line %d ends at segment %d of tag %d, which is an open wire end.", ld_refs[lr].line, w.segs, w.tag);
L1027:         snprintf(msg, sizeof(msg), "The TL on line %d references an invalid segment on the first endpoint; segment index out of range for the wire tag %d.", tl_refs[r].line, tl_refs[r].tag1);
L1032:         snprintf(msg, sizeof(msg), "The TL on line %d references an invalid segment on the second endpoint; segment index out of range for the wire tag %d.", tl_refs[r].line, tl_refs[r].tag2);
L1038:         snprintf(msg, sizeof(msg), "The TL on line %d connects the same tag and segment on both ends; this is a no-op or invalid connection.", tl_refs[r].line);
L1064:           snprintf(msg, sizeof(msg), "The LD on line %d references an invalid start segment; out of range for wire tag %d.", ld_refs[r].line, ld_refs[r].tag);
L1069:           snprintf(msg, sizeof(msg), "The LD on line %d references an invalid end segment; out of range for wire tag %d.", ld_refs[r].line, ld_refs[r].tag);
L1087:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has the same endpoints as the wire on line %d (tag %d). Overlapping wires can cause issues.", a.line, a.tag, b.line, b.tag);
L1103:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) lies below the ground plane (z<0).", w.line, w.tag);
L1108:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) crosses the ground plane (z=0), which is typically invalid.", w.line, w.tag);
L1162:             snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) endpoint is connected near the middle of the wire on line %d (tag %d). Connections must be at segment ends.", a.line, a.tag, b.line, b.tag);
L1173:     snprintf(msg, sizeof(msg), "A deck should have a CE card.");

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 1179
CALL: add_error(ctx, errors, msg, 1);
--- Context (lines 1119-1184) ---
   1119:     double ay[2] = {a.y1, a.y2};
   1120:     double az[2] = {a.z1, a.z2};
   1121:     for (int end = 0; end < 2; end++)
   1122:     {
   1123:       double px = ax[end], py = ay[end], pz = az[end];
   1124:       for (int bi = 0; bi < wire_count; bi++)
   1125:       {
   1126:         if (bi == ai)
   1127:           continue;
   1128:         wire_info_t b = wires[bi];
   1129:         double vx = b.x2 - b.x1, vy = b.y2 - b.y1, vz = b.z2 - b.z1;
   1130:         double L2 = vx * vx + vy * vy + vz * vz;
   1131:         if (L2 == 0.0 || b.segs <= 0)
   1132:           continue;
   1133:         double L = sqrt(L2);
   1134:         double segLen = L / (double)b.segs;
   1135:         double tol = segLen / 1000.0;
   1136:         // project point onto line b
   1137:         double wx = px - b.x1, wy = py - b.y1, wz = pz - b.z1;
   1138:         double t = (wx * vx + wy * vy + wz * vz) / L2;
   1139:         if (t < 0.0 || t > 1.0)
   1140:           continue; // closest point lies outside the wire extent
   1141:         double fx = b.x1 + t * vx, fy = b.y1 + t * vy, fz = b.z1 + t * vz;
   1142:         double dx = px - fx, dy = py - fy, dz = pz - fz;
   1143:         double dist = sqrt(dx * dx + dy * dy + dz * dz);
   1144:         if (dist <= tol)
   1145:         {
   1146:           // near-connected; ensure this footpoint is at a segment endpoint of b
   1147:           int isEndpoint = 0;
   1148:           for (int k = 0; k <= b.segs; k++)
   1149:           {
   1150:             double ek = (double)k / (double)b.segs;
   1151:             double ex = b.x1 + ek * vx, ey = b.y1 + ek * vy, ez = b.z1 + ek * vz;
   1152:             double edx = fx - ex, edy = fy - ey, edz = fz - ez;
   1153:             double ed = sqrt(edx * edx + edy * edy + edz * edz);
   1154:             if (ed <= tol)
   1155:             {
   1156:               isEndpoint = 1;
   1157:               break;
   1158:             }
   1159:           }
   1160:           if (!isEndpoint)
   1161:           {
   1162:             snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) endpoint is connected near the middle of the wire on line %d (tag %d). Connections must be at segment ends.", a.line, a.tag, b.line, b.tag);
   1163:             add_error(ctx, errors, msg, 0);
   1164:           }
   1165:         }
   1166:       }
   1167:     }
   1168:   }
   1169: 
   1170:   // and with the entire deck tested, make sure we got the key cards
   1171:   if (!sawCE)
   1172:   {
   1173:     snprintf(msg, sizeof(msg), "A deck should have a CE card.");
   1174:     add_error(ctx, errors, msg, 0);
   1175:   }
   1176:   if (!sawGx)
   1177:   {
   1178:     snprintf(msg, sizeof(msg), "A deck has to have at least one geometry card.");
>  1179:     add_error(ctx, errors, msg, 1);
   1180:   }
   1181:   if (!sawGE)
   1182:   {
   1183:     snprintf(msg, sizeof(msg), "A deck has to have a GE card.");
   1184:     add_error(ctx, errors, msg, 1);

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L983:           snprintf(msg, sizeof(msg), "The LD on line %d starts at segment 1 of tag %d, which is an open wire end.", ld_refs[lr].line, w.tag);
L990:           snprintf(msg, sizeof(msg), "The LD on line %d ends at segment %d of tag %d, which is an open wire end.", ld_refs[lr].line, w.segs, w.tag);
L1027:         snprintf(msg, sizeof(msg), "The TL on line %d references an invalid segment on the first endpoint; segment index out of range for the wire tag %d.", tl_refs[r].line, tl_refs[r].tag1);
L1032:         snprintf(msg, sizeof(msg), "The TL on line %d references an invalid segment on the second endpoint; segment index out of range for the wire tag %d.", tl_refs[r].line, tl_refs[r].tag2);
L1038:         snprintf(msg, sizeof(msg), "The TL on line %d connects the same tag and segment on both ends; this is a no-op or invalid connection.", tl_refs[r].line);
L1064:           snprintf(msg, sizeof(msg), "The LD on line %d references an invalid start segment; out of range for wire tag %d.", ld_refs[r].line, ld_refs[r].tag);
L1069:           snprintf(msg, sizeof(msg), "The LD on line %d references an invalid end segment; out of range for wire tag %d.", ld_refs[r].line, ld_refs[r].tag);
L1087:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has the same endpoints as the wire on line %d (tag %d). Overlapping wires can cause issues.", a.line, a.tag, b.line, b.tag);
L1103:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) lies below the ground plane (z<0).", w.line, w.tag);
L1108:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) crosses the ground plane (z=0), which is typically invalid.", w.line, w.tag);
L1162:             snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) endpoint is connected near the middle of the wire on line %d (tag %d). Connections must be at segment ends.", a.line, a.tag, b.line, b.tag);
L1173:     snprintf(msg, sizeof(msg), "A deck should have a CE card.");
L1178:     snprintf(msg, sizeof(msg), "A deck has to have at least one geometry card.");

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 1184
CALL: add_error(ctx, errors, msg, 1);
--- Context (lines 1124-1189) ---
   1124:       for (int bi = 0; bi < wire_count; bi++)
   1125:       {
   1126:         if (bi == ai)
   1127:           continue;
   1128:         wire_info_t b = wires[bi];
   1129:         double vx = b.x2 - b.x1, vy = b.y2 - b.y1, vz = b.z2 - b.z1;
   1130:         double L2 = vx * vx + vy * vy + vz * vz;
   1131:         if (L2 == 0.0 || b.segs <= 0)
   1132:           continue;
   1133:         double L = sqrt(L2);
   1134:         double segLen = L / (double)b.segs;
   1135:         double tol = segLen / 1000.0;
   1136:         // project point onto line b
   1137:         double wx = px - b.x1, wy = py - b.y1, wz = pz - b.z1;
   1138:         double t = (wx * vx + wy * vy + wz * vz) / L2;
   1139:         if (t < 0.0 || t > 1.0)
   1140:           continue; // closest point lies outside the wire extent
   1141:         double fx = b.x1 + t * vx, fy = b.y1 + t * vy, fz = b.z1 + t * vz;
   1142:         double dx = px - fx, dy = py - fy, dz = pz - fz;
   1143:         double dist = sqrt(dx * dx + dy * dy + dz * dz);
   1144:         if (dist <= tol)
   1145:         {
   1146:           // near-connected; ensure this footpoint is at a segment endpoint of b
   1147:           int isEndpoint = 0;
   1148:           for (int k = 0; k <= b.segs; k++)
   1149:           {
   1150:             double ek = (double)k / (double)b.segs;
   1151:             double ex = b.x1 + ek * vx, ey = b.y1 + ek * vy, ez = b.z1 + ek * vz;
   1152:             double edx = fx - ex, edy = fy - ey, edz = fz - ez;
   1153:             double ed = sqrt(edx * edx + edy * edy + edz * edz);
   1154:             if (ed <= tol)
   1155:             {
   1156:               isEndpoint = 1;
   1157:               break;
   1158:             }
   1159:           }
   1160:           if (!isEndpoint)
   1161:           {
   1162:             snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) endpoint is connected near the middle of the wire on line %d (tag %d). Connections must be at segment ends.", a.line, a.tag, b.line, b.tag);
   1163:             add_error(ctx, errors, msg, 0);
   1164:           }
   1165:         }
   1166:       }
   1167:     }
   1168:   }
   1169: 
   1170:   // and with the entire deck tested, make sure we got the key cards
   1171:   if (!sawCE)
   1172:   {
   1173:     snprintf(msg, sizeof(msg), "A deck should have a CE card.");
   1174:     add_error(ctx, errors, msg, 0);
   1175:   }
   1176:   if (!sawGx)
   1177:   {
   1178:     snprintf(msg, sizeof(msg), "A deck has to have at least one geometry card.");
   1179:     add_error(ctx, errors, msg, 1);
   1180:   }
   1181:   if (!sawGE)
   1182:   {
   1183:     snprintf(msg, sizeof(msg), "A deck has to have a GE card.");
>  1184:     add_error(ctx, errors, msg, 1);
   1185:   }
   1186:   if (!sawEN)
   1187:   {
   1188:     snprintf(msg, sizeof(msg), "A deck should end with a EN card.");
   1189:     add_error(ctx, errors, msg, 0);

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L990:           snprintf(msg, sizeof(msg), "The LD on line %d ends at segment %d of tag %d, which is an open wire end.", ld_refs[lr].line, w.segs, w.tag);
L1027:         snprintf(msg, sizeof(msg), "The TL on line %d references an invalid segment on the first endpoint; segment index out of range for the wire tag %d.", tl_refs[r].line, tl_refs[r].tag1);
L1032:         snprintf(msg, sizeof(msg), "The TL on line %d references an invalid segment on the second endpoint; segment index out of range for the wire tag %d.", tl_refs[r].line, tl_refs[r].tag2);
L1038:         snprintf(msg, sizeof(msg), "The TL on line %d connects the same tag and segment on both ends; this is a no-op or invalid connection.", tl_refs[r].line);
L1064:           snprintf(msg, sizeof(msg), "The LD on line %d references an invalid start segment; out of range for wire tag %d.", ld_refs[r].line, ld_refs[r].tag);
L1069:           snprintf(msg, sizeof(msg), "The LD on line %d references an invalid end segment; out of range for wire tag %d.", ld_refs[r].line, ld_refs[r].tag);
L1087:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has the same endpoints as the wire on line %d (tag %d). Overlapping wires can cause issues.", a.line, a.tag, b.line, b.tag);
L1103:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) lies below the ground plane (z<0).", w.line, w.tag);
L1108:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) crosses the ground plane (z=0), which is typically invalid.", w.line, w.tag);
L1162:             snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) endpoint is connected near the middle of the wire on line %d (tag %d). Connections must be at segment ends.", a.line, a.tag, b.line, b.tag);
L1173:     snprintf(msg, sizeof(msg), "A deck should have a CE card.");
L1178:     snprintf(msg, sizeof(msg), "A deck has to have at least one geometry card.");
L1183:     snprintf(msg, sizeof(msg), "A deck has to have a GE card.");

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 1189
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 1129-1194) ---
   1129:         double vx = b.x2 - b.x1, vy = b.y2 - b.y1, vz = b.z2 - b.z1;
   1130:         double L2 = vx * vx + vy * vy + vz * vz;
   1131:         if (L2 == 0.0 || b.segs <= 0)
   1132:           continue;
   1133:         double L = sqrt(L2);
   1134:         double segLen = L / (double)b.segs;
   1135:         double tol = segLen / 1000.0;
   1136:         // project point onto line b
   1137:         double wx = px - b.x1, wy = py - b.y1, wz = pz - b.z1;
   1138:         double t = (wx * vx + wy * vy + wz * vz) / L2;
   1139:         if (t < 0.0 || t > 1.0)
   1140:           continue; // closest point lies outside the wire extent
   1141:         double fx = b.x1 + t * vx, fy = b.y1 + t * vy, fz = b.z1 + t * vz;
   1142:         double dx = px - fx, dy = py - fy, dz = pz - fz;
   1143:         double dist = sqrt(dx * dx + dy * dy + dz * dz);
   1144:         if (dist <= tol)
   1145:         {
   1146:           // near-connected; ensure this footpoint is at a segment endpoint of b
   1147:           int isEndpoint = 0;
   1148:           for (int k = 0; k <= b.segs; k++)
   1149:           {
   1150:             double ek = (double)k / (double)b.segs;
   1151:             double ex = b.x1 + ek * vx, ey = b.y1 + ek * vy, ez = b.z1 + ek * vz;
   1152:             double edx = fx - ex, edy = fy - ey, edz = fz - ez;
   1153:             double ed = sqrt(edx * edx + edy * edy + edz * edz);
   1154:             if (ed <= tol)
   1155:             {
   1156:               isEndpoint = 1;
   1157:               break;
   1158:             }
   1159:           }
   1160:           if (!isEndpoint)
   1161:           {
   1162:             snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) endpoint is connected near the middle of the wire on line %d (tag %d). Connections must be at segment ends.", a.line, a.tag, b.line, b.tag);
   1163:             add_error(ctx, errors, msg, 0);
   1164:           }
   1165:         }
   1166:       }
   1167:     }
   1168:   }
   1169: 
   1170:   // and with the entire deck tested, make sure we got the key cards
   1171:   if (!sawCE)
   1172:   {
   1173:     snprintf(msg, sizeof(msg), "A deck should have a CE card.");
   1174:     add_error(ctx, errors, msg, 0);
   1175:   }
   1176:   if (!sawGx)
   1177:   {
   1178:     snprintf(msg, sizeof(msg), "A deck has to have at least one geometry card.");
   1179:     add_error(ctx, errors, msg, 1);
   1180:   }
   1181:   if (!sawGE)
   1182:   {
   1183:     snprintf(msg, sizeof(msg), "A deck has to have a GE card.");
   1184:     add_error(ctx, errors, msg, 1);
   1185:   }
   1186:   if (!sawEN)
   1187:   {
   1188:     snprintf(msg, sizeof(msg), "A deck should end with a EN card.");
>  1189:     add_error(ctx, errors, msg, 0);
   1190:   }
   1191:   // EN should be the last card when present
   1192:   if (sawEN && sawEN != deck->num_cards - 1)
   1193:   {
   1194:     snprintf(msg, sizeof(msg), "The EN card should be the final card in the deck (found earlier at card %d).", sawEN + 1);

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L990:           snprintf(msg, sizeof(msg), "The LD on line %d ends at segment %d of tag %d, which is an open wire end.", ld_refs[lr].line, w.segs, w.tag);
L1027:         snprintf(msg, sizeof(msg), "The TL on line %d references an invalid segment on the first endpoint; segment index out of range for the wire tag %d.", tl_refs[r].line, tl_refs[r].tag1);
L1032:         snprintf(msg, sizeof(msg), "The TL on line %d references an invalid segment on the second endpoint; segment index out of range for the wire tag %d.", tl_refs[r].line, tl_refs[r].tag2);
L1038:         snprintf(msg, sizeof(msg), "The TL on line %d connects the same tag and segment on both ends; this is a no-op or invalid connection.", tl_refs[r].line);
L1064:           snprintf(msg, sizeof(msg), "The LD on line %d references an invalid start segment; out of range for wire tag %d.", ld_refs[r].line, ld_refs[r].tag);
L1069:           snprintf(msg, sizeof(msg), "The LD on line %d references an invalid end segment; out of range for wire tag %d.", ld_refs[r].line, ld_refs[r].tag);
L1087:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has the same endpoints as the wire on line %d (tag %d). Overlapping wires can cause issues.", a.line, a.tag, b.line, b.tag);
L1103:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) lies below the ground plane (z<0).", w.line, w.tag);
L1108:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) crosses the ground plane (z=0), which is typically invalid.", w.line, w.tag);
L1162:             snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) endpoint is connected near the middle of the wire on line %d (tag %d). Connections must be at segment ends.", a.line, a.tag, b.line, b.tag);
L1173:     snprintf(msg, sizeof(msg), "A deck should have a CE card.");
L1178:     snprintf(msg, sizeof(msg), "A deck has to have at least one geometry card.");
L1183:     snprintf(msg, sizeof(msg), "A deck has to have a GE card.");
L1188:     snprintf(msg, sizeof(msg), "A deck should end with a EN card.");

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 1195
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 1135-1200) ---
   1135:         double tol = segLen / 1000.0;
   1136:         // project point onto line b
   1137:         double wx = px - b.x1, wy = py - b.y1, wz = pz - b.z1;
   1138:         double t = (wx * vx + wy * vy + wz * vz) / L2;
   1139:         if (t < 0.0 || t > 1.0)
   1140:           continue; // closest point lies outside the wire extent
   1141:         double fx = b.x1 + t * vx, fy = b.y1 + t * vy, fz = b.z1 + t * vz;
   1142:         double dx = px - fx, dy = py - fy, dz = pz - fz;
   1143:         double dist = sqrt(dx * dx + dy * dy + dz * dz);
   1144:         if (dist <= tol)
   1145:         {
   1146:           // near-connected; ensure this footpoint is at a segment endpoint of b
   1147:           int isEndpoint = 0;
   1148:           for (int k = 0; k <= b.segs; k++)
   1149:           {
   1150:             double ek = (double)k / (double)b.segs;
   1151:             double ex = b.x1 + ek * vx, ey = b.y1 + ek * vy, ez = b.z1 + ek * vz;
   1152:             double edx = fx - ex, edy = fy - ey, edz = fz - ez;
   1153:             double ed = sqrt(edx * edx + edy * edy + edz * edz);
   1154:             if (ed <= tol)
   1155:             {
   1156:               isEndpoint = 1;
   1157:               break;
   1158:             }
   1159:           }
   1160:           if (!isEndpoint)
   1161:           {
   1162:             snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) endpoint is connected near the middle of the wire on line %d (tag %d). Connections must be at segment ends.", a.line, a.tag, b.line, b.tag);
   1163:             add_error(ctx, errors, msg, 0);
   1164:           }
   1165:         }
   1166:       }
   1167:     }
   1168:   }
   1169: 
   1170:   // and with the entire deck tested, make sure we got the key cards
   1171:   if (!sawCE)
   1172:   {
   1173:     snprintf(msg, sizeof(msg), "A deck should have a CE card.");
   1174:     add_error(ctx, errors, msg, 0);
   1175:   }
   1176:   if (!sawGx)
   1177:   {
   1178:     snprintf(msg, sizeof(msg), "A deck has to have at least one geometry card.");
   1179:     add_error(ctx, errors, msg, 1);
   1180:   }
   1181:   if (!sawGE)
   1182:   {
   1183:     snprintf(msg, sizeof(msg), "A deck has to have a GE card.");
   1184:     add_error(ctx, errors, msg, 1);
   1185:   }
   1186:   if (!sawEN)
   1187:   {
   1188:     snprintf(msg, sizeof(msg), "A deck should end with a EN card.");
   1189:     add_error(ctx, errors, msg, 0);
   1190:   }
   1191:   // EN should be the last card when present
   1192:   if (sawEN && sawEN != deck->num_cards - 1)
   1193:   {
   1194:     snprintf(msg, sizeof(msg), "The EN card should be the final card in the deck (found earlier at card %d).", sawEN + 1);
>  1195:     add_error(ctx, errors, msg, 0);
   1196:   }
   1197:   if (!sawFR && !sawRP)
   1198:   {
   1199:     snprintf(msg, sizeof(msg), "A deck has to have an FR card.");
   1200:     add_error(ctx, errors, msg, 1);

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L1027:         snprintf(msg, sizeof(msg), "The TL on line %d references an invalid segment on the first endpoint; segment index out of range for the wire tag %d.", tl_refs[r].line, tl_refs[r].tag1);
L1032:         snprintf(msg, sizeof(msg), "The TL on line %d references an invalid segment on the second endpoint; segment index out of range for the wire tag %d.", tl_refs[r].line, tl_refs[r].tag2);
L1038:         snprintf(msg, sizeof(msg), "The TL on line %d connects the same tag and segment on both ends; this is a no-op or invalid connection.", tl_refs[r].line);
L1064:           snprintf(msg, sizeof(msg), "The LD on line %d references an invalid start segment; out of range for wire tag %d.", ld_refs[r].line, ld_refs[r].tag);
L1069:           snprintf(msg, sizeof(msg), "The LD on line %d references an invalid end segment; out of range for wire tag %d.", ld_refs[r].line, ld_refs[r].tag);
L1087:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has the same endpoints as the wire on line %d (tag %d). Overlapping wires can cause issues.", a.line, a.tag, b.line, b.tag);
L1103:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) lies below the ground plane (z<0).", w.line, w.tag);
L1108:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) crosses the ground plane (z=0), which is typically invalid.", w.line, w.tag);
L1162:             snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) endpoint is connected near the middle of the wire on line %d (tag %d). Connections must be at segment ends.", a.line, a.tag, b.line, b.tag);
L1173:     snprintf(msg, sizeof(msg), "A deck should have a CE card.");
L1178:     snprintf(msg, sizeof(msg), "A deck has to have at least one geometry card.");
L1183:     snprintf(msg, sizeof(msg), "A deck has to have a GE card.");
L1188:     snprintf(msg, sizeof(msg), "A deck should end with a EN card.");
L1194:     snprintf(msg, sizeof(msg), "The EN card should be the final card in the deck (found earlier at card %d).", sawEN + 1);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 1200
CALL: add_error(ctx, errors, msg, 1);
--- Context (lines 1140-1205) ---
   1140:           continue; // closest point lies outside the wire extent
   1141:         double fx = b.x1 + t * vx, fy = b.y1 + t * vy, fz = b.z1 + t * vz;
   1142:         double dx = px - fx, dy = py - fy, dz = pz - fz;
   1143:         double dist = sqrt(dx * dx + dy * dy + dz * dz);
   1144:         if (dist <= tol)
   1145:         {
   1146:           // near-connected; ensure this footpoint is at a segment endpoint of b
   1147:           int isEndpoint = 0;
   1148:           for (int k = 0; k <= b.segs; k++)
   1149:           {
   1150:             double ek = (double)k / (double)b.segs;
   1151:             double ex = b.x1 + ek * vx, ey = b.y1 + ek * vy, ez = b.z1 + ek * vz;
   1152:             double edx = fx - ex, edy = fy - ey, edz = fz - ez;
   1153:             double ed = sqrt(edx * edx + edy * edy + edz * edz);
   1154:             if (ed <= tol)
   1155:             {
   1156:               isEndpoint = 1;
   1157:               break;
   1158:             }
   1159:           }
   1160:           if (!isEndpoint)
   1161:           {
   1162:             snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) endpoint is connected near the middle of the wire on line %d (tag %d). Connections must be at segment ends.", a.line, a.tag, b.line, b.tag);
   1163:             add_error(ctx, errors, msg, 0);
   1164:           }
   1165:         }
   1166:       }
   1167:     }
   1168:   }
   1169: 
   1170:   // and with the entire deck tested, make sure we got the key cards
   1171:   if (!sawCE)
   1172:   {
   1173:     snprintf(msg, sizeof(msg), "A deck should have a CE card.");
   1174:     add_error(ctx, errors, msg, 0);
   1175:   }
   1176:   if (!sawGx)
   1177:   {
   1178:     snprintf(msg, sizeof(msg), "A deck has to have at least one geometry card.");
   1179:     add_error(ctx, errors, msg, 1);
   1180:   }
   1181:   if (!sawGE)
   1182:   {
   1183:     snprintf(msg, sizeof(msg), "A deck has to have a GE card.");
   1184:     add_error(ctx, errors, msg, 1);
   1185:   }
   1186:   if (!sawEN)
   1187:   {
   1188:     snprintf(msg, sizeof(msg), "A deck should end with a EN card.");
   1189:     add_error(ctx, errors, msg, 0);
   1190:   }
   1191:   // EN should be the last card when present
   1192:   if (sawEN && sawEN != deck->num_cards - 1)
   1193:   {
   1194:     snprintf(msg, sizeof(msg), "The EN card should be the final card in the deck (found earlier at card %d).", sawEN + 1);
   1195:     add_error(ctx, errors, msg, 0);
   1196:   }
   1197:   if (!sawFR && !sawRP)
   1198:   {
   1199:     snprintf(msg, sizeof(msg), "A deck has to have an FR card.");
>  1200:     add_error(ctx, errors, msg, 1);
   1201:   }
   1202:   if (!sawEX && !sawLD)
   1203:   {
   1204:     snprintf(msg, sizeof(msg), "A deck has to have at least one EX or LD card.");
   1205:     add_error(ctx, errors, msg, 1);

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L1027:         snprintf(msg, sizeof(msg), "The TL on line %d references an invalid segment on the first endpoint; segment index out of range for the wire tag %d.", tl_refs[r].line, tl_refs[r].tag1);
L1032:         snprintf(msg, sizeof(msg), "The TL on line %d references an invalid segment on the second endpoint; segment index out of range for the wire tag %d.", tl_refs[r].line, tl_refs[r].tag2);
L1038:         snprintf(msg, sizeof(msg), "The TL on line %d connects the same tag and segment on both ends; this is a no-op or invalid connection.", tl_refs[r].line);
L1064:           snprintf(msg, sizeof(msg), "The LD on line %d references an invalid start segment; out of range for wire tag %d.", ld_refs[r].line, ld_refs[r].tag);
L1069:           snprintf(msg, sizeof(msg), "The LD on line %d references an invalid end segment; out of range for wire tag %d.", ld_refs[r].line, ld_refs[r].tag);
L1087:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has the same endpoints as the wire on line %d (tag %d). Overlapping wires can cause issues.", a.line, a.tag, b.line, b.tag);
L1103:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) lies below the ground plane (z<0).", w.line, w.tag);
L1108:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) crosses the ground plane (z=0), which is typically invalid.", w.line, w.tag);
L1162:             snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) endpoint is connected near the middle of the wire on line %d (tag %d). Connections must be at segment ends.", a.line, a.tag, b.line, b.tag);
L1173:     snprintf(msg, sizeof(msg), "A deck should have a CE card.");
L1178:     snprintf(msg, sizeof(msg), "A deck has to have at least one geometry card.");
L1183:     snprintf(msg, sizeof(msg), "A deck has to have a GE card.");
L1188:     snprintf(msg, sizeof(msg), "A deck should end with a EN card.");
L1194:     snprintf(msg, sizeof(msg), "The EN card should be the final card in the deck (found earlier at card %d).", sawEN + 1);
L1199:     snprintf(msg, sizeof(msg), "A deck has to have an FR card.");

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 1205
CALL: add_error(ctx, errors, msg, 1);
--- Context (lines 1145-1210) ---
   1145:         {
   1146:           // near-connected; ensure this footpoint is at a segment endpoint of b
   1147:           int isEndpoint = 0;
   1148:           for (int k = 0; k <= b.segs; k++)
   1149:           {
   1150:             double ek = (double)k / (double)b.segs;
   1151:             double ex = b.x1 + ek * vx, ey = b.y1 + ek * vy, ez = b.z1 + ek * vz;
   1152:             double edx = fx - ex, edy = fy - ey, edz = fz - ez;
   1153:             double ed = sqrt(edx * edx + edy * edy + edz * edz);
   1154:             if (ed <= tol)
   1155:             {
   1156:               isEndpoint = 1;
   1157:               break;
   1158:             }
   1159:           }
   1160:           if (!isEndpoint)
   1161:           {
   1162:             snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) endpoint is connected near the middle of the wire on line %d (tag %d). Connections must be at segment ends.", a.line, a.tag, b.line, b.tag);
   1163:             add_error(ctx, errors, msg, 0);
   1164:           }
   1165:         }
   1166:       }
   1167:     }
   1168:   }
   1169: 
   1170:   // and with the entire deck tested, make sure we got the key cards
   1171:   if (!sawCE)
   1172:   {
   1173:     snprintf(msg, sizeof(msg), "A deck should have a CE card.");
   1174:     add_error(ctx, errors, msg, 0);
   1175:   }
   1176:   if (!sawGx)
   1177:   {
   1178:     snprintf(msg, sizeof(msg), "A deck has to have at least one geometry card.");
   1179:     add_error(ctx, errors, msg, 1);
   1180:   }
   1181:   if (!sawGE)
   1182:   {
   1183:     snprintf(msg, sizeof(msg), "A deck has to have a GE card.");
   1184:     add_error(ctx, errors, msg, 1);
   1185:   }
   1186:   if (!sawEN)
   1187:   {
   1188:     snprintf(msg, sizeof(msg), "A deck should end with a EN card.");
   1189:     add_error(ctx, errors, msg, 0);
   1190:   }
   1191:   // EN should be the last card when present
   1192:   if (sawEN && sawEN != deck->num_cards - 1)
   1193:   {
   1194:     snprintf(msg, sizeof(msg), "The EN card should be the final card in the deck (found earlier at card %d).", sawEN + 1);
   1195:     add_error(ctx, errors, msg, 0);
   1196:   }
   1197:   if (!sawFR && !sawRP)
   1198:   {
   1199:     snprintf(msg, sizeof(msg), "A deck has to have an FR card.");
   1200:     add_error(ctx, errors, msg, 1);
   1201:   }
   1202:   if (!sawEX && !sawLD)
   1203:   {
   1204:     snprintf(msg, sizeof(msg), "A deck has to have at least one EX or LD card.");
>  1205:     add_error(ctx, errors, msg, 1);
   1206:   }
   1207:   if (sawSY && !sawCE)
   1208:   {
   1209:     snprintf(msg, sizeof(msg), "We found SY cards in the deck, but there is no CE in the deck. SYs should follow the CE.");
   1210:     add_error(ctx, errors, msg, 0);

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L1027:         snprintf(msg, sizeof(msg), "The TL on line %d references an invalid segment on the first endpoint; segment index out of range for the wire tag %d.", tl_refs[r].line, tl_refs[r].tag1);
L1032:         snprintf(msg, sizeof(msg), "The TL on line %d references an invalid segment on the second endpoint; segment index out of range for the wire tag %d.", tl_refs[r].line, tl_refs[r].tag2);
L1038:         snprintf(msg, sizeof(msg), "The TL on line %d connects the same tag and segment on both ends; this is a no-op or invalid connection.", tl_refs[r].line);
L1064:           snprintf(msg, sizeof(msg), "The LD on line %d references an invalid start segment; out of range for wire tag %d.", ld_refs[r].line, ld_refs[r].tag);
L1069:           snprintf(msg, sizeof(msg), "The LD on line %d references an invalid end segment; out of range for wire tag %d.", ld_refs[r].line, ld_refs[r].tag);
L1087:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has the same endpoints as the wire on line %d (tag %d). Overlapping wires can cause issues.", a.line, a.tag, b.line, b.tag);
L1103:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) lies below the ground plane (z<0).", w.line, w.tag);
L1108:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) crosses the ground plane (z=0), which is typically invalid.", w.line, w.tag);
L1162:             snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) endpoint is connected near the middle of the wire on line %d (tag %d). Connections must be at segment ends.", a.line, a.tag, b.line, b.tag);
L1173:     snprintf(msg, sizeof(msg), "A deck should have a CE card.");
L1178:     snprintf(msg, sizeof(msg), "A deck has to have at least one geometry card.");
L1183:     snprintf(msg, sizeof(msg), "A deck has to have a GE card.");
L1188:     snprintf(msg, sizeof(msg), "A deck should end with a EN card.");
L1194:     snprintf(msg, sizeof(msg), "The EN card should be the final card in the deck (found earlier at card %d).", sawEN + 1);
L1199:     snprintf(msg, sizeof(msg), "A deck has to have an FR card.");
L1204:     snprintf(msg, sizeof(msg), "A deck has to have at least one EX or LD card.");

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 1210
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 1150-1215) ---
   1150:             double ek = (double)k / (double)b.segs;
   1151:             double ex = b.x1 + ek * vx, ey = b.y1 + ek * vy, ez = b.z1 + ek * vz;
   1152:             double edx = fx - ex, edy = fy - ey, edz = fz - ez;
   1153:             double ed = sqrt(edx * edx + edy * edy + edz * edz);
   1154:             if (ed <= tol)
   1155:             {
   1156:               isEndpoint = 1;
   1157:               break;
   1158:             }
   1159:           }
   1160:           if (!isEndpoint)
   1161:           {
   1162:             snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) endpoint is connected near the middle of the wire on line %d (tag %d). Connections must be at segment ends.", a.line, a.tag, b.line, b.tag);
   1163:             add_error(ctx, errors, msg, 0);
   1164:           }
   1165:         }
   1166:       }
   1167:     }
   1168:   }
   1169: 
   1170:   // and with the entire deck tested, make sure we got the key cards
   1171:   if (!sawCE)
   1172:   {
   1173:     snprintf(msg, sizeof(msg), "A deck should have a CE card.");
   1174:     add_error(ctx, errors, msg, 0);
   1175:   }
   1176:   if (!sawGx)
   1177:   {
   1178:     snprintf(msg, sizeof(msg), "A deck has to have at least one geometry card.");
   1179:     add_error(ctx, errors, msg, 1);
   1180:   }
   1181:   if (!sawGE)
   1182:   {
   1183:     snprintf(msg, sizeof(msg), "A deck has to have a GE card.");
   1184:     add_error(ctx, errors, msg, 1);
   1185:   }
   1186:   if (!sawEN)
   1187:   {
   1188:     snprintf(msg, sizeof(msg), "A deck should end with a EN card.");
   1189:     add_error(ctx, errors, msg, 0);
   1190:   }
   1191:   // EN should be the last card when present
   1192:   if (sawEN && sawEN != deck->num_cards - 1)
   1193:   {
   1194:     snprintf(msg, sizeof(msg), "The EN card should be the final card in the deck (found earlier at card %d).", sawEN + 1);
   1195:     add_error(ctx, errors, msg, 0);
   1196:   }
   1197:   if (!sawFR && !sawRP)
   1198:   {
   1199:     snprintf(msg, sizeof(msg), "A deck has to have an FR card.");
   1200:     add_error(ctx, errors, msg, 1);
   1201:   }
   1202:   if (!sawEX && !sawLD)
   1203:   {
   1204:     snprintf(msg, sizeof(msg), "A deck has to have at least one EX or LD card.");
   1205:     add_error(ctx, errors, msg, 1);
   1206:   }
   1207:   if (sawSY && !sawCE)
   1208:   {
   1209:     snprintf(msg, sizeof(msg), "We found SY cards in the deck, but there is no CE in the deck. SYs should follow the CE.");
>  1210:     add_error(ctx, errors, msg, 0);
   1211:   }
   1212:   // warn if GD appears without any preceding GN
   1213:   if (sawGD && !sawGN)
   1214:   {
   1215:     snprintf(msg, sizeof(msg), "A GD card appears in the deck, but there is no GN card.");

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L1027:         snprintf(msg, sizeof(msg), "The TL on line %d references an invalid segment on the first endpoint; segment index out of range for the wire tag %d.", tl_refs[r].line, tl_refs[r].tag1);
L1032:         snprintf(msg, sizeof(msg), "The TL on line %d references an invalid segment on the second endpoint; segment index out of range for the wire tag %d.", tl_refs[r].line, tl_refs[r].tag2);
L1038:         snprintf(msg, sizeof(msg), "The TL on line %d connects the same tag and segment on both ends; this is a no-op or invalid connection.", tl_refs[r].line);
L1064:           snprintf(msg, sizeof(msg), "The LD on line %d references an invalid start segment; out of range for wire tag %d.", ld_refs[r].line, ld_refs[r].tag);
L1069:           snprintf(msg, sizeof(msg), "The LD on line %d references an invalid end segment; out of range for wire tag %d.", ld_refs[r].line, ld_refs[r].tag);
L1087:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has the same endpoints as the wire on line %d (tag %d). Overlapping wires can cause issues.", a.line, a.tag, b.line, b.tag);
L1103:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) lies below the ground plane (z<0).", w.line, w.tag);
L1108:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) crosses the ground plane (z=0), which is typically invalid.", w.line, w.tag);
L1162:             snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) endpoint is connected near the middle of the wire on line %d (tag %d). Connections must be at segment ends.", a.line, a.tag, b.line, b.tag);
L1173:     snprintf(msg, sizeof(msg), "A deck should have a CE card.");
L1178:     snprintf(msg, sizeof(msg), "A deck has to have at least one geometry card.");
L1183:     snprintf(msg, sizeof(msg), "A deck has to have a GE card.");
L1188:     snprintf(msg, sizeof(msg), "A deck should end with a EN card.");
L1194:     snprintf(msg, sizeof(msg), "The EN card should be the final card in the deck (found earlier at card %d).", sawEN + 1);
L1199:     snprintf(msg, sizeof(msg), "A deck has to have an FR card.");
L1204:     snprintf(msg, sizeof(msg), "A deck has to have at least one EX or LD card.");
L1209:     snprintf(msg, sizeof(msg), "We found SY cards in the deck, but there is no CE in the deck. SYs should follow the CE.");

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 1216
CALL: add_error(ctx, errors, msg, 1);
--- Context (lines 1156-1221) ---
   1156:               isEndpoint = 1;
   1157:               break;
   1158:             }
   1159:           }
   1160:           if (!isEndpoint)
   1161:           {
   1162:             snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) endpoint is connected near the middle of the wire on line %d (tag %d). Connections must be at segment ends.", a.line, a.tag, b.line, b.tag);
   1163:             add_error(ctx, errors, msg, 0);
   1164:           }
   1165:         }
   1166:       }
   1167:     }
   1168:   }
   1169: 
   1170:   // and with the entire deck tested, make sure we got the key cards
   1171:   if (!sawCE)
   1172:   {
   1173:     snprintf(msg, sizeof(msg), "A deck should have a CE card.");
   1174:     add_error(ctx, errors, msg, 0);
   1175:   }
   1176:   if (!sawGx)
   1177:   {
   1178:     snprintf(msg, sizeof(msg), "A deck has to have at least one geometry card.");
   1179:     add_error(ctx, errors, msg, 1);
   1180:   }
   1181:   if (!sawGE)
   1182:   {
   1183:     snprintf(msg, sizeof(msg), "A deck has to have a GE card.");
   1184:     add_error(ctx, errors, msg, 1);
   1185:   }
   1186:   if (!sawEN)
   1187:   {
   1188:     snprintf(msg, sizeof(msg), "A deck should end with a EN card.");
   1189:     add_error(ctx, errors, msg, 0);
   1190:   }
   1191:   // EN should be the last card when present
   1192:   if (sawEN && sawEN != deck->num_cards - 1)
   1193:   {
   1194:     snprintf(msg, sizeof(msg), "The EN card should be the final card in the deck (found earlier at card %d).", sawEN + 1);
   1195:     add_error(ctx, errors, msg, 0);
   1196:   }
   1197:   if (!sawFR && !sawRP)
   1198:   {
   1199:     snprintf(msg, sizeof(msg), "A deck has to have an FR card.");
   1200:     add_error(ctx, errors, msg, 1);
   1201:   }
   1202:   if (!sawEX && !sawLD)
   1203:   {
   1204:     snprintf(msg, sizeof(msg), "A deck has to have at least one EX or LD card.");
   1205:     add_error(ctx, errors, msg, 1);
   1206:   }
   1207:   if (sawSY && !sawCE)
   1208:   {
   1209:     snprintf(msg, sizeof(msg), "We found SY cards in the deck, but there is no CE in the deck. SYs should follow the CE.");
   1210:     add_error(ctx, errors, msg, 0);
   1211:   }
   1212:   // warn if GD appears without any preceding GN
   1213:   if (sawGD && !sawGN)
   1214:   {
   1215:     snprintf(msg, sizeof(msg), "A GD card appears in the deck, but there is no GN card.");
>  1216:     add_error(ctx, errors, msg, 1);
   1217:   }
   1218: 
   1219:   // if the GE card was -1, there has to be a GN
   1220:   if (sawGE && GEType == -1 && !sawGN)
   1221:   {

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L1027:         snprintf(msg, sizeof(msg), "The TL on line %d references an invalid segment on the first endpoint; segment index out of range for the wire tag %d.", tl_refs[r].line, tl_refs[r].tag1);
L1032:         snprintf(msg, sizeof(msg), "The TL on line %d references an invalid segment on the second endpoint; segment index out of range for the wire tag %d.", tl_refs[r].line, tl_refs[r].tag2);
L1038:         snprintf(msg, sizeof(msg), "The TL on line %d connects the same tag and segment on both ends; this is a no-op or invalid connection.", tl_refs[r].line);
L1064:           snprintf(msg, sizeof(msg), "The LD on line %d references an invalid start segment; out of range for wire tag %d.", ld_refs[r].line, ld_refs[r].tag);
L1069:           snprintf(msg, sizeof(msg), "The LD on line %d references an invalid end segment; out of range for wire tag %d.", ld_refs[r].line, ld_refs[r].tag);
L1087:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has the same endpoints as the wire on line %d (tag %d). Overlapping wires can cause issues.", a.line, a.tag, b.line, b.tag);
L1103:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) lies below the ground plane (z<0).", w.line, w.tag);
L1108:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) crosses the ground plane (z=0), which is typically invalid.", w.line, w.tag);
L1162:             snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) endpoint is connected near the middle of the wire on line %d (tag %d). Connections must be at segment ends.", a.line, a.tag, b.line, b.tag);
L1173:     snprintf(msg, sizeof(msg), "A deck should have a CE card.");
L1178:     snprintf(msg, sizeof(msg), "A deck has to have at least one geometry card.");
L1183:     snprintf(msg, sizeof(msg), "A deck has to have a GE card.");
L1188:     snprintf(msg, sizeof(msg), "A deck should end with a EN card.");
L1194:     snprintf(msg, sizeof(msg), "The EN card should be the final card in the deck (found earlier at card %d).", sawEN + 1);
L1199:     snprintf(msg, sizeof(msg), "A deck has to have an FR card.");
L1204:     snprintf(msg, sizeof(msg), "A deck has to have at least one EX or LD card.");
L1209:     snprintf(msg, sizeof(msg), "We found SY cards in the deck, but there is no CE in the deck. SYs should follow the CE.");
L1215:     snprintf(msg, sizeof(msg), "A GD card appears in the deck, but there is no GN card.");

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 1223
CALL: add_error(ctx, errors, msg, 1);
--- Context (lines 1163-1228) ---
   1163:             add_error(ctx, errors, msg, 0);
   1164:           }
   1165:         }
   1166:       }
   1167:     }
   1168:   }
   1169: 
   1170:   // and with the entire deck tested, make sure we got the key cards
   1171:   if (!sawCE)
   1172:   {
   1173:     snprintf(msg, sizeof(msg), "A deck should have a CE card.");
   1174:     add_error(ctx, errors, msg, 0);
   1175:   }
   1176:   if (!sawGx)
   1177:   {
   1178:     snprintf(msg, sizeof(msg), "A deck has to have at least one geometry card.");
   1179:     add_error(ctx, errors, msg, 1);
   1180:   }
   1181:   if (!sawGE)
   1182:   {
   1183:     snprintf(msg, sizeof(msg), "A deck has to have a GE card.");
   1184:     add_error(ctx, errors, msg, 1);
   1185:   }
   1186:   if (!sawEN)
   1187:   {
   1188:     snprintf(msg, sizeof(msg), "A deck should end with a EN card.");
   1189:     add_error(ctx, errors, msg, 0);
   1190:   }
   1191:   // EN should be the last card when present
   1192:   if (sawEN && sawEN != deck->num_cards - 1)
   1193:   {
   1194:     snprintf(msg, sizeof(msg), "The EN card should be the final card in the deck (found earlier at card %d).", sawEN + 1);
   1195:     add_error(ctx, errors, msg, 0);
   1196:   }
   1197:   if (!sawFR && !sawRP)
   1198:   {
   1199:     snprintf(msg, sizeof(msg), "A deck has to have an FR card.");
   1200:     add_error(ctx, errors, msg, 1);
   1201:   }
   1202:   if (!sawEX && !sawLD)
   1203:   {
   1204:     snprintf(msg, sizeof(msg), "A deck has to have at least one EX or LD card.");
   1205:     add_error(ctx, errors, msg, 1);
   1206:   }
   1207:   if (sawSY && !sawCE)
   1208:   {
   1209:     snprintf(msg, sizeof(msg), "We found SY cards in the deck, but there is no CE in the deck. SYs should follow the CE.");
   1210:     add_error(ctx, errors, msg, 0);
   1211:   }
   1212:   // warn if GD appears without any preceding GN
   1213:   if (sawGD && !sawGN)
   1214:   {
   1215:     snprintf(msg, sizeof(msg), "A GD card appears in the deck, but there is no GN card.");
   1216:     add_error(ctx, errors, msg, 1);
   1217:   }
   1218: 
   1219:   // if the GE card was -1, there has to be a GN
   1220:   if (sawGE && GEType == -1 && !sawGN)
   1221:   {
   1222:     snprintf(msg, sizeof(msg), "The GE is set to -1, but there is no GN card in the deck.");
>  1223:     add_error(ctx, errors, msg, 1);
   1224:   }
   1225: }
   1226: 
   1227: // Helper: point-to-segment distance in 3D
   1228: static double point_to_segment_distance(double px, double py, double pz,

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L1027:         snprintf(msg, sizeof(msg), "The TL on line %d references an invalid segment on the first endpoint; segment index out of range for the wire tag %d.", tl_refs[r].line, tl_refs[r].tag1);
L1032:         snprintf(msg, sizeof(msg), "The TL on line %d references an invalid segment on the second endpoint; segment index out of range for the wire tag %d.", tl_refs[r].line, tl_refs[r].tag2);
L1038:         snprintf(msg, sizeof(msg), "The TL on line %d connects the same tag and segment on both ends; this is a no-op or invalid connection.", tl_refs[r].line);
L1064:           snprintf(msg, sizeof(msg), "The LD on line %d references an invalid start segment; out of range for wire tag %d.", ld_refs[r].line, ld_refs[r].tag);
L1069:           snprintf(msg, sizeof(msg), "The LD on line %d references an invalid end segment; out of range for wire tag %d.", ld_refs[r].line, ld_refs[r].tag);
L1087:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has the same endpoints as the wire on line %d (tag %d). Overlapping wires can cause issues.", a.line, a.tag, b.line, b.tag);
L1103:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) lies below the ground plane (z<0).", w.line, w.tag);
L1108:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) crosses the ground plane (z=0), which is typically invalid.", w.line, w.tag);
L1162:             snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) endpoint is connected near the middle of the wire on line %d (tag %d). Connections must be at segment ends.", a.line, a.tag, b.line, b.tag);
L1173:     snprintf(msg, sizeof(msg), "A deck should have a CE card.");
L1178:     snprintf(msg, sizeof(msg), "A deck has to have at least one geometry card.");
L1183:     snprintf(msg, sizeof(msg), "A deck has to have a GE card.");
L1188:     snprintf(msg, sizeof(msg), "A deck should end with a EN card.");
L1194:     snprintf(msg, sizeof(msg), "The EN card should be the final card in the deck (found earlier at card %d).", sawEN + 1);
L1199:     snprintf(msg, sizeof(msg), "A deck has to have an FR card.");
L1204:     snprintf(msg, sizeof(msg), "A deck has to have at least one EX or LD card.");
L1209:     snprintf(msg, sizeof(msg), "We found SY cards in the deck, but there is no CE in the deck. SYs should follow the CE.");
L1215:     snprintf(msg, sizeof(msg), "A GD card appears in the deck, but there is no GN card.");
L1222:     snprintf(msg, sizeof(msg), "The GE is set to -1, but there is no GN card in the deck.");

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 1307
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 1247-1312) ---
   1247: // Helper: warn if parallel wires closer than 0.05 wavelengths have different segmentation
   1248: static void check_parallel_wire_segmentation(const nec_context_t *ctx, errors_list_t *errors,
   1249:                                              const wire_info_t *wires, int wire_count,
   1250:                                              double freq_mhz)
   1251: {
   1252:   char msg[MAX_ERROR_LEN];
   1253:   double wlam_m = (freq_mhz > 0.0) ? (CVEL / freq_mhz) : 0.0; // CVEL in m/us vs MHz => meters
   1254:   if (wlam_m <= 0.0)
   1255:   {
   1256:     return;
   1257:   }
   1258:   double thr = 0.05 * wlam_m;
   1259: 
   1260:   for (int i = 0; i < wire_count; i++)
   1261:   {
   1262:     for (int j = i + 1; j < wire_count; j++)
   1263:     {
   1264:       const wire_info_t *a = &wires[i];
   1265:       const wire_info_t *b = &wires[j];
   1266:       // direction vectors
   1267:       double ax = a->x2 - a->x1, ay = a->y2 - a->y1, az = a->z2 - a->z1;
   1268:       double bx = b->x2 - b->x1, by = b->y2 - b->y1, bz = b->z2 - b->z1;
   1269:       double al = sqrt(ax * ax + ay * ay + az * az);
   1270:       double bl = sqrt(bx * bx + by * by + bz * bz);
   1271:       if (al == 0.0 || bl == 0.0)
   1272:         continue;
   1273:       // unit direction vectors
   1274:       ax /= al;
   1275:       ay /= al;
   1276:       az /= al;
   1277:       bx /= bl;
   1278:       by /= bl;
   1279:       bz /= bl;
   1280:       // parallel if |cross| small or |dot| close to 1
   1281:       double cx = ay * bz - az * by;
   1282:       double cy = az * bx - ax * bz;
   1283:       double cz = ax * by - ay * bx;
   1284:       double cross_mag = sqrt(cx * cx + cy * cy + cz * cz);
   1285:       double dot = ax * bx + ay * by + az * bz;
   1286:       if (cross_mag > 1e-3 && fabs(dot) < 0.999)
   1287:         continue; // not parallel enough
   1288: 
   1289:       // minimal distance between segments (approx): sample endpoints to other segment lines
   1290:       // point-to-line distance from a->x1 to b, and a->x2 to b, and vice versa; take min
   1291:       double min_dist = thr * 10.0; // init larger than thr
   1292:       double d1 = point_to_segment_distance(a->x1, a->y1, a->z1, b->x1, b->y1, b->z1, b->x2, b->y2, b->z2);
   1293:       double d2 = point_to_segment_distance(a->x2, a->y2, a->z2, b->x1, b->y1, b->z1, b->x2, b->y2, b->z2);
   1294:       double d3 = point_to_segment_distance(b->x1, b->y1, b->z1, a->x1, a->y1, a->z1, a->x2, a->y2, a->z2);
   1295:       double d4 = point_to_segment_distance(b->x2, b->y2, b->z2, a->x1, a->y1, a->z1, a->x2, a->y2, a->z2);
   1296:       min_dist = fmin(fmin(d1, d2), fmin(d3, d4));
   1297: 
   1298:       if (min_dist < thr)
   1299:       {
   1300:         double segA = al / (double)a->segs;
   1301:         double segB = bl / (double)b->segs;
   1302:         // different segmentation: either segment counts differ, or segment lengths differ >10%
   1303:         double rel = fabs(segA - segB) / fmax(segA, segB);
   1304:         if (a->segs != b->segs || rel > 0.10)
   1305:         {
   1306:           snprintf(msg, sizeof(msg), "Parallel wires (tags %d and %d; lines %d and %d) are closer than 0.05 wavelengths but have different segmentation.", a->tag, b->tag, a->line, b->line);
>  1307:           add_error(ctx, errors, msg, 0);
   1308:         }
   1309:       }
   1310:     }
   1311:   }
   1312: }

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L1108:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) crosses the ground plane (z=0), which is typically invalid.", w.line, w.tag);
L1162:             snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) endpoint is connected near the middle of the wire on line %d (tag %d). Connections must be at segment ends.", a.line, a.tag, b.line, b.tag);
L1173:     snprintf(msg, sizeof(msg), "A deck should have a CE card.");
L1178:     snprintf(msg, sizeof(msg), "A deck has to have at least one geometry card.");
L1183:     snprintf(msg, sizeof(msg), "A deck has to have a GE card.");
L1188:     snprintf(msg, sizeof(msg), "A deck should end with a EN card.");
L1194:     snprintf(msg, sizeof(msg), "The EN card should be the final card in the deck (found earlier at card %d).", sawEN + 1);
L1199:     snprintf(msg, sizeof(msg), "A deck has to have an FR card.");
L1204:     snprintf(msg, sizeof(msg), "A deck has to have at least one EX or LD card.");
L1209:     snprintf(msg, sizeof(msg), "We found SY cards in the deck, but there is no CE in the deck. SYs should follow the CE.");
L1215:     snprintf(msg, sizeof(msg), "A GD card appears in the deck, but there is no GN card.");
L1222:     snprintf(msg, sizeof(msg), "The GE is set to -1, but there is no GN card in the deck.");
L1306:           snprintf(msg, sizeof(msg), "Parallel wires (tags %d and %d; lines %d and %d) are closer than 0.05 wavelengths but have different segmentation.", a->tag, b->tag, a->line, b->line);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 1346
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 1286-1351) ---
   1286:       if (cross_mag > 1e-3 && fabs(dot) < 0.999)
   1287:         continue; // not parallel enough
   1288: 
   1289:       // minimal distance between segments (approx): sample endpoints to other segment lines
   1290:       // point-to-line distance from a->x1 to b, and a->x2 to b, and vice versa; take min
   1291:       double min_dist = thr * 10.0; // init larger than thr
   1292:       double d1 = point_to_segment_distance(a->x1, a->y1, a->z1, b->x1, b->y1, b->z1, b->x2, b->y2, b->z2);
   1293:       double d2 = point_to_segment_distance(a->x2, a->y2, a->z2, b->x1, b->y1, b->z1, b->x2, b->y2, b->z2);
   1294:       double d3 = point_to_segment_distance(b->x1, b->y1, b->z1, a->x1, a->y1, a->z1, a->x2, a->y2, a->z2);
   1295:       double d4 = point_to_segment_distance(b->x2, b->y2, b->z2, a->x1, a->y1, a->z1, a->x2, a->y2, a->z2);
   1296:       min_dist = fmin(fmin(d1, d2), fmin(d3, d4));
   1297: 
   1298:       if (min_dist < thr)
   1299:       {
   1300:         double segA = al / (double)a->segs;
   1301:         double segB = bl / (double)b->segs;
   1302:         // different segmentation: either segment counts differ, or segment lengths differ >10%
   1303:         double rel = fabs(segA - segB) / fmax(segA, segB);
   1304:         if (a->segs != b->segs || rel > 0.10)
   1305:         {
   1306:           snprintf(msg, sizeof(msg), "Parallel wires (tags %d and %d; lines %d and %d) are closer than 0.05 wavelengths but have different segmentation.", a->tag, b->tag, a->line, b->line);
   1307:           add_error(ctx, errors, msg, 0);
   1308:         }
   1309:       }
   1310:     }
   1311:   }
   1312: }
   1313: 
   1314: // Helper: junction segmentation consistency — connected wire endpoints should have similar segment lengths
   1315: static void check_junction_segmentation_consistency(const nec_context_t *ctx, errors_list_t *errors,
   1316:                                                     const wire_info_t *wires, int wire_count)
   1317: {
   1318:   char msg[MAX_ERROR_LEN];
   1319:   for (int i = 0; i < wire_count; i++)
   1320:   {
   1321:     const wire_info_t *a = &wires[i];
   1322:     if (a->segs <= 0)
   1323:       continue;
   1324:     double aL = sqrt(pow(a->x2 - a->x1, 2) + pow(a->y2 - a->y1, 2) + pow(a->z2 - a->z1, 2));
   1325:     double aSeg = aL / (double)a->segs;
   1326:     for (int j = i + 1; j < wire_count; j++)
   1327:     {
   1328:       const wire_info_t *b = &wires[j];
   1329:       if (b->segs <= 0)
   1330:         continue;
   1331:       double bL = sqrt(pow(b->x2 - b->x1, 2) + pow(b->y2 - b->y1, 2) + pow(b->z2 - b->z1, 2));
   1332:       double bSeg = bL / (double)b->segs;
   1333:       double tol = fmax(fmin(aSeg, bSeg) / 1000.0, 1e-9);
   1334:       // check direct endpoint-to-endpoint distances for proximity
   1335:       double d11 = sqrt(pow(a->x1 - b->x1, 2) + pow(a->y1 - b->y1, 2) + pow(a->z1 - b->z1, 2));
   1336:       double d12 = sqrt(pow(a->x1 - b->x2, 2) + pow(a->y1 - b->y2, 2) + pow(a->z1 - b->z2, 2));
   1337:       double d21 = sqrt(pow(a->x2 - b->x1, 2) + pow(a->y2 - b->y1, 2) + pow(a->z2 - b->z1, 2));
   1338:       double d22 = sqrt(pow(a->x2 - b->x2, 2) + pow(a->y2 - b->y2, 2) + pow(a->z2 - b->z2, 2));
   1339:       int connected = (d11 <= tol) || (d12 <= tol) || (d21 <= tol) || (d22 <= tol);
   1340:       if (connected)
   1341:       {
   1342:         double rel = fabs(aSeg - bSeg) / fmax(aSeg, bSeg);
   1343:         if (rel > 0.20)
   1344:         {
   1345:           snprintf(msg, sizeof(msg), "Connected wires (tags %d and %d; lines %d and %d) have very different segment lengths near the junction (%.4g m vs %.4g m); consider harmonizing segmentation.", a->tag, b->tag, a->line, b->line, aSeg, bSeg);
>  1346:           add_error(ctx, errors, msg, 0);
   1347:         }
   1348:       }
   1349:     }
   1350:   }
   1351: }

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L1162:             snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) endpoint is connected near the middle of the wire on line %d (tag %d). Connections must be at segment ends.", a.line, a.tag, b.line, b.tag);
L1173:     snprintf(msg, sizeof(msg), "A deck should have a CE card.");
L1178:     snprintf(msg, sizeof(msg), "A deck has to have at least one geometry card.");
L1183:     snprintf(msg, sizeof(msg), "A deck has to have a GE card.");
L1188:     snprintf(msg, sizeof(msg), "A deck should end with a EN card.");
L1194:     snprintf(msg, sizeof(msg), "The EN card should be the final card in the deck (found earlier at card %d).", sawEN + 1);
L1199:     snprintf(msg, sizeof(msg), "A deck has to have an FR card.");
L1204:     snprintf(msg, sizeof(msg), "A deck has to have at least one EX or LD card.");
L1209:     snprintf(msg, sizeof(msg), "We found SY cards in the deck, but there is no CE in the deck. SYs should follow the CE.");
L1215:     snprintf(msg, sizeof(msg), "A GD card appears in the deck, but there is no GN card.");
L1222:     snprintf(msg, sizeof(msg), "The GE is set to -1, but there is no GN card in the deck.");
L1306:           snprintf(msg, sizeof(msg), "Parallel wires (tags %d and %d; lines %d and %d) are closer than 0.05 wavelengths but have different segmentation.", a->tag, b->tag, a->line, b->line);
L1345:           snprintf(msg, sizeof(msg), "Connected wires (tags %d and %d; lines %d and %d) have very different segment lengths near the junction (%.4g m vs %.4g m); consider harmonizing segmentation.", a->tag, b->tag, a->line, b->line, aSeg, bSeg);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 1373
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 1313-1378) ---
   1313: 
   1314: // Helper: junction segmentation consistency — connected wire endpoints should have similar segment lengths
   1315: static void check_junction_segmentation_consistency(const nec_context_t *ctx, errors_list_t *errors,
   1316:                                                     const wire_info_t *wires, int wire_count)
   1317: {
   1318:   char msg[MAX_ERROR_LEN];
   1319:   for (int i = 0; i < wire_count; i++)
   1320:   {
   1321:     const wire_info_t *a = &wires[i];
   1322:     if (a->segs <= 0)
   1323:       continue;
   1324:     double aL = sqrt(pow(a->x2 - a->x1, 2) + pow(a->y2 - a->y1, 2) + pow(a->z2 - a->z1, 2));
   1325:     double aSeg = aL / (double)a->segs;
   1326:     for (int j = i + 1; j < wire_count; j++)
   1327:     {
   1328:       const wire_info_t *b = &wires[j];
   1329:       if (b->segs <= 0)
   1330:         continue;
   1331:       double bL = sqrt(pow(b->x2 - b->x1, 2) + pow(b->y2 - b->y1, 2) + pow(b->z2 - b->z1, 2));
   1332:       double bSeg = bL / (double)b->segs;
   1333:       double tol = fmax(fmin(aSeg, bSeg) / 1000.0, 1e-9);
   1334:       // check direct endpoint-to-endpoint distances for proximity
   1335:       double d11 = sqrt(pow(a->x1 - b->x1, 2) + pow(a->y1 - b->y1, 2) + pow(a->z1 - b->z1, 2));
   1336:       double d12 = sqrt(pow(a->x1 - b->x2, 2) + pow(a->y1 - b->y2, 2) + pow(a->z1 - b->z2, 2));
   1337:       double d21 = sqrt(pow(a->x2 - b->x1, 2) + pow(a->y2 - b->y1, 2) + pow(a->z2 - b->z1, 2));
   1338:       double d22 = sqrt(pow(a->x2 - b->x2, 2) + pow(a->y2 - b->y2, 2) + pow(a->z2 - b->z2, 2));
   1339:       int connected = (d11 <= tol) || (d12 <= tol) || (d21 <= tol) || (d22 <= tol);
   1340:       if (connected)
   1341:       {
   1342:         double rel = fabs(aSeg - bSeg) / fmax(aSeg, bSeg);
   1343:         if (rel > 0.20)
   1344:         {
   1345:           snprintf(msg, sizeof(msg), "Connected wires (tags %d and %d; lines %d and %d) have very different segment lengths near the junction (%.4g m vs %.4g m); consider harmonizing segmentation.", a->tag, b->tag, a->line, b->line, aSeg, bSeg);
   1346:           add_error(ctx, errors, msg, 0);
   1347:         }
   1348:       }
   1349:     }
   1350:   }
   1351: }
   1352: 
   1353: // Helper: GE I1=1 connects segment ends if wire height < 1e-3 * segment length; suggest GE -1
   1354: static void check_ge_low_height_hazard(const nec_context_t *ctx, errors_list_t *errors,
   1355:                                        const wire_info_t *wires, int wire_count,
   1356:                                        int GEType)
   1357: {
   1358:   if (GEType != 1)
   1359:     return;
   1360:   char msg[MAX_ERROR_LEN];
   1361:   for (int i = 0; i < wire_count; i++)
   1362:   {
   1363:     const wire_info_t *w = &wires[i];
   1364:     if (w->segs <= 0)
   1365:       continue;
   1366:     double L = sqrt(pow(w->x2 - w->x1, 2) + pow(w->y2 - w->y1, 2) + pow(w->z2 - w->z1, 2));
   1367:     double segLen = L / (double)w->segs;
   1368:     double dz = fabs(w->z2 - w->z1);
   1369:     double h = fmin(fabs(w->z1), fabs(w->z2));
   1370:     if (dz < 1e-9 && h < (1e-3 * segLen))
   1371:     {
   1372:       snprintf(msg, sizeof(msg), "With GE I1=1, the wire on line %d (tag %d) has height %.4g m < 1e-3 x segment length (%.4g m); segment ends may be connected to ground. Consider GE -1.", w->line, w->tag, h, segLen);
>  1373:       add_error(ctx, errors, msg, 0);
   1374:     }
   1375:   }
   1376: }
   1377: 
   1378: // Helper: segment length vs wavelength and radius sanity

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L1173:     snprintf(msg, sizeof(msg), "A deck should have a CE card.");
L1178:     snprintf(msg, sizeof(msg), "A deck has to have at least one geometry card.");
L1183:     snprintf(msg, sizeof(msg), "A deck has to have a GE card.");
L1188:     snprintf(msg, sizeof(msg), "A deck should end with a EN card.");
L1194:     snprintf(msg, sizeof(msg), "The EN card should be the final card in the deck (found earlier at card %d).", sawEN + 1);
L1199:     snprintf(msg, sizeof(msg), "A deck has to have an FR card.");
L1204:     snprintf(msg, sizeof(msg), "A deck has to have at least one EX or LD card.");
L1209:     snprintf(msg, sizeof(msg), "We found SY cards in the deck, but there is no CE in the deck. SYs should follow the CE.");
L1215:     snprintf(msg, sizeof(msg), "A GD card appears in the deck, but there is no GN card.");
L1222:     snprintf(msg, sizeof(msg), "The GE is set to -1, but there is no GN card in the deck.");
L1306:           snprintf(msg, sizeof(msg), "Parallel wires (tags %d and %d; lines %d and %d) are closer than 0.05 wavelengths but have different segmentation.", a->tag, b->tag, a->line, b->line);
L1345:           snprintf(msg, sizeof(msg), "Connected wires (tags %d and %d; lines %d and %d) have very different segment lengths near the junction (%.4g m vs %.4g m); consider harmonizing segmentation.", a->tag, b->tag, a->line, b->line, aSeg, bSeg);
L1372:       snprintf(msg, sizeof(msg), "With GE I1=1, the wire on line %d (tag %d) has height %.4g m < 1e-3 x segment length (%.4g m); segment ends may be connected to ground. Consider GE -1.", w->line, w->tag, h, segLen);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 1401
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 1341-1406) ---
   1341:       {
   1342:         double rel = fabs(aSeg - bSeg) / fmax(aSeg, bSeg);
   1343:         if (rel > 0.20)
   1344:         {
   1345:           snprintf(msg, sizeof(msg), "Connected wires (tags %d and %d; lines %d and %d) have very different segment lengths near the junction (%.4g m vs %.4g m); consider harmonizing segmentation.", a->tag, b->tag, a->line, b->line, aSeg, bSeg);
   1346:           add_error(ctx, errors, msg, 0);
   1347:         }
   1348:       }
   1349:     }
   1350:   }
   1351: }
   1352: 
   1353: // Helper: GE I1=1 connects segment ends if wire height < 1e-3 * segment length; suggest GE -1
   1354: static void check_ge_low_height_hazard(const nec_context_t *ctx, errors_list_t *errors,
   1355:                                        const wire_info_t *wires, int wire_count,
   1356:                                        int GEType)
   1357: {
   1358:   if (GEType != 1)
   1359:     return;
   1360:   char msg[MAX_ERROR_LEN];
   1361:   for (int i = 0; i < wire_count; i++)
   1362:   {
   1363:     const wire_info_t *w = &wires[i];
   1364:     if (w->segs <= 0)
   1365:       continue;
   1366:     double L = sqrt(pow(w->x2 - w->x1, 2) + pow(w->y2 - w->y1, 2) + pow(w->z2 - w->z1, 2));
   1367:     double segLen = L / (double)w->segs;
   1368:     double dz = fabs(w->z2 - w->z1);
   1369:     double h = fmin(fabs(w->z1), fabs(w->z2));
   1370:     if (dz < 1e-9 && h < (1e-3 * segLen))
   1371:     {
   1372:       snprintf(msg, sizeof(msg), "With GE I1=1, the wire on line %d (tag %d) has height %.4g m < 1e-3 x segment length (%.4g m); segment ends may be connected to ground. Consider GE -1.", w->line, w->tag, h, segLen);
   1373:       add_error(ctx, errors, msg, 0);
   1374:     }
   1375:   }
   1376: }
   1377: 
   1378: // Helper: segment length vs wavelength and radius sanity
   1379: static void check_segment_length_and_radius(const nec_context_t *ctx, errors_list_t *errors,
   1380:                                             const wire_info_t *wires, int wire_count,
   1381:                                             double freq_mhz, int ek_enabled)
   1382: {
   1383:   char msg[MAX_ERROR_LEN];
   1384:   double wlam_m = (freq_mhz > 0.0) ? (CVEL / freq_mhz) : 0.0;
   1385:   if (wlam_m <= 0.0)
   1386:   {
   1387:     return;
   1388:   }
   1389: 
   1390:   for (int i = 0; i < wire_count; i++)
   1391:   {
   1392:     const wire_info_t *w = &wires[i];
   1393:     double L = sqrt(pow(w->x2 - w->x1, 2) + pow(w->y2 - w->y1, 2) + pow(w->z2 - w->z1, 2));
   1394:     if (w->segs > 0)
   1395:     {
   1396:       double segLen = L / (double)w->segs;
   1397:       double segFrac = segLen / wlam_m; // in wavelengths
   1398:       if (segFrac >= 0.10)
   1399:       {
   1400:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has segment length %.4g m (%.4g lambda) which is >= 0.1 lambda; consider increasing segmentation.", w->line, w->tag, segLen, segFrac);
>  1401:         add_error(ctx, errors, msg, 0);
   1402:       }
   1403:       else if (segFrac >= 0.05)
   1404:       {
   1405:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has segment length %.4g m (%.4g lambda); in critical regions, aim for < 0.05 lambda.", w->line, w->tag, segLen, segFrac);
   1406:         add_error(ctx, errors, msg, 0);

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L1204:     snprintf(msg, sizeof(msg), "A deck has to have at least one EX or LD card.");
L1209:     snprintf(msg, sizeof(msg), "We found SY cards in the deck, but there is no CE in the deck. SYs should follow the CE.");
L1215:     snprintf(msg, sizeof(msg), "A GD card appears in the deck, but there is no GN card.");
L1222:     snprintf(msg, sizeof(msg), "The GE is set to -1, but there is no GN card in the deck.");
L1306:           snprintf(msg, sizeof(msg), "Parallel wires (tags %d and %d; lines %d and %d) are closer than 0.05 wavelengths but have different segmentation.", a->tag, b->tag, a->line, b->line);
L1345:           snprintf(msg, sizeof(msg), "Connected wires (tags %d and %d; lines %d and %d) have very different segment lengths near the junction (%.4g m vs %.4g m); consider harmonizing segmentation.", a->tag, b->tag, a->line, b->line, aSeg, bSeg);
L1372:       snprintf(msg, sizeof(msg), "With GE I1=1, the wire on line %d (tag %d) has height %.4g m < 1e-3 x segment length (%.4g m); segment ends may be connected to ground. Consider GE -1.", w->line, w->tag, h, segLen);
L1400:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has segment length %.4g m (%.4g lambda) which is >= 0.1 lambda; consider increasing segmentation.", w->line, w->tag, segLen, segFrac);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 1406
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 1346-1411) ---
   1346:           add_error(ctx, errors, msg, 0);
   1347:         }
   1348:       }
   1349:     }
   1350:   }
   1351: }
   1352: 
   1353: // Helper: GE I1=1 connects segment ends if wire height < 1e-3 * segment length; suggest GE -1
   1354: static void check_ge_low_height_hazard(const nec_context_t *ctx, errors_list_t *errors,
   1355:                                        const wire_info_t *wires, int wire_count,
   1356:                                        int GEType)
   1357: {
   1358:   if (GEType != 1)
   1359:     return;
   1360:   char msg[MAX_ERROR_LEN];
   1361:   for (int i = 0; i < wire_count; i++)
   1362:   {
   1363:     const wire_info_t *w = &wires[i];
   1364:     if (w->segs <= 0)
   1365:       continue;
   1366:     double L = sqrt(pow(w->x2 - w->x1, 2) + pow(w->y2 - w->y1, 2) + pow(w->z2 - w->z1, 2));
   1367:     double segLen = L / (double)w->segs;
   1368:     double dz = fabs(w->z2 - w->z1);
   1369:     double h = fmin(fabs(w->z1), fabs(w->z2));
   1370:     if (dz < 1e-9 && h < (1e-3 * segLen))
   1371:     {
   1372:       snprintf(msg, sizeof(msg), "With GE I1=1, the wire on line %d (tag %d) has height %.4g m < 1e-3 x segment length (%.4g m); segment ends may be connected to ground. Consider GE -1.", w->line, w->tag, h, segLen);
   1373:       add_error(ctx, errors, msg, 0);
   1374:     }
   1375:   }
   1376: }
   1377: 
   1378: // Helper: segment length vs wavelength and radius sanity
   1379: static void check_segment_length_and_radius(const nec_context_t *ctx, errors_list_t *errors,
   1380:                                             const wire_info_t *wires, int wire_count,
   1381:                                             double freq_mhz, int ek_enabled)
   1382: {
   1383:   char msg[MAX_ERROR_LEN];
   1384:   double wlam_m = (freq_mhz > 0.0) ? (CVEL / freq_mhz) : 0.0;
   1385:   if (wlam_m <= 0.0)
   1386:   {
   1387:     return;
   1388:   }
   1389: 
   1390:   for (int i = 0; i < wire_count; i++)
   1391:   {
   1392:     const wire_info_t *w = &wires[i];
   1393:     double L = sqrt(pow(w->x2 - w->x1, 2) + pow(w->y2 - w->y1, 2) + pow(w->z2 - w->z1, 2));
   1394:     if (w->segs > 0)
   1395:     {
   1396:       double segLen = L / (double)w->segs;
   1397:       double segFrac = segLen / wlam_m; // in wavelengths
   1398:       if (segFrac >= 0.10)
   1399:       {
   1400:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has segment length %.4g m (%.4g lambda) which is >= 0.1 lambda; consider increasing segmentation.", w->line, w->tag, segLen, segFrac);
   1401:         add_error(ctx, errors, msg, 0);
   1402:       }
   1403:       else if (segFrac >= 0.05)
   1404:       {
   1405:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has segment length %.4g m (%.4g lambda); in critical regions, aim for < 0.05 lambda.", w->line, w->tag, segLen, segFrac);
>  1406:         add_error(ctx, errors, msg, 0);
   1407:       }
   1408:       if (segFrac < 0.001)
   1409:       {
   1410:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has very small segment length %.4g m (%.4g lambda); this may cause excessive segmentation.", w->line, w->tag, segLen, segFrac);
   1411:         add_error(ctx, errors, msg, 0);

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L1209:     snprintf(msg, sizeof(msg), "We found SY cards in the deck, but there is no CE in the deck. SYs should follow the CE.");
L1215:     snprintf(msg, sizeof(msg), "A GD card appears in the deck, but there is no GN card.");
L1222:     snprintf(msg, sizeof(msg), "The GE is set to -1, but there is no GN card in the deck.");
L1306:           snprintf(msg, sizeof(msg), "Parallel wires (tags %d and %d; lines %d and %d) are closer than 0.05 wavelengths but have different segmentation.", a->tag, b->tag, a->line, b->line);
L1345:           snprintf(msg, sizeof(msg), "Connected wires (tags %d and %d; lines %d and %d) have very different segment lengths near the junction (%.4g m vs %.4g m); consider harmonizing segmentation.", a->tag, b->tag, a->line, b->line, aSeg, bSeg);
L1372:       snprintf(msg, sizeof(msg), "With GE I1=1, the wire on line %d (tag %d) has height %.4g m < 1e-3 x segment length (%.4g m); segment ends may be connected to ground. Consider GE -1.", w->line, w->tag, h, segLen);
L1400:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has segment length %.4g m (%.4g lambda) which is >= 0.1 lambda; consider increasing segmentation.", w->line, w->tag, segLen, segFrac);
L1405:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has segment length %.4g m (%.4g lambda); in critical regions, aim for < 0.05 lambda.", w->line, w->tag, segLen, segFrac);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 1411
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 1351-1416) ---
   1351: }
   1352: 
   1353: // Helper: GE I1=1 connects segment ends if wire height < 1e-3 * segment length; suggest GE -1
   1354: static void check_ge_low_height_hazard(const nec_context_t *ctx, errors_list_t *errors,
   1355:                                        const wire_info_t *wires, int wire_count,
   1356:                                        int GEType)
   1357: {
   1358:   if (GEType != 1)
   1359:     return;
   1360:   char msg[MAX_ERROR_LEN];
   1361:   for (int i = 0; i < wire_count; i++)
   1362:   {
   1363:     const wire_info_t *w = &wires[i];
   1364:     if (w->segs <= 0)
   1365:       continue;
   1366:     double L = sqrt(pow(w->x2 - w->x1, 2) + pow(w->y2 - w->y1, 2) + pow(w->z2 - w->z1, 2));
   1367:     double segLen = L / (double)w->segs;
   1368:     double dz = fabs(w->z2 - w->z1);
   1369:     double h = fmin(fabs(w->z1), fabs(w->z2));
   1370:     if (dz < 1e-9 && h < (1e-3 * segLen))
   1371:     {
   1372:       snprintf(msg, sizeof(msg), "With GE I1=1, the wire on line %d (tag %d) has height %.4g m < 1e-3 x segment length (%.4g m); segment ends may be connected to ground. Consider GE -1.", w->line, w->tag, h, segLen);
   1373:       add_error(ctx, errors, msg, 0);
   1374:     }
   1375:   }
   1376: }
   1377: 
   1378: // Helper: segment length vs wavelength and radius sanity
   1379: static void check_segment_length_and_radius(const nec_context_t *ctx, errors_list_t *errors,
   1380:                                             const wire_info_t *wires, int wire_count,
   1381:                                             double freq_mhz, int ek_enabled)
   1382: {
   1383:   char msg[MAX_ERROR_LEN];
   1384:   double wlam_m = (freq_mhz > 0.0) ? (CVEL / freq_mhz) : 0.0;
   1385:   if (wlam_m <= 0.0)
   1386:   {
   1387:     return;
   1388:   }
   1389: 
   1390:   for (int i = 0; i < wire_count; i++)
   1391:   {
   1392:     const wire_info_t *w = &wires[i];
   1393:     double L = sqrt(pow(w->x2 - w->x1, 2) + pow(w->y2 - w->y1, 2) + pow(w->z2 - w->z1, 2));
   1394:     if (w->segs > 0)
   1395:     {
   1396:       double segLen = L / (double)w->segs;
   1397:       double segFrac = segLen / wlam_m; // in wavelengths
   1398:       if (segFrac >= 0.10)
   1399:       {
   1400:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has segment length %.4g m (%.4g lambda) which is >= 0.1 lambda; consider increasing segmentation.", w->line, w->tag, segLen, segFrac);
   1401:         add_error(ctx, errors, msg, 0);
   1402:       }
   1403:       else if (segFrac >= 0.05)
   1404:       {
   1405:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has segment length %.4g m (%.4g lambda); in critical regions, aim for < 0.05 lambda.", w->line, w->tag, segLen, segFrac);
   1406:         add_error(ctx, errors, msg, 0);
   1407:       }
   1408:       if (segFrac < 0.001)
   1409:       {
   1410:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has very small segment length %.4g m (%.4g lambda); this may cause excessive segmentation.", w->line, w->tag, segLen, segFrac);
>  1411:         add_error(ctx, errors, msg, 0);
   1412:       }
   1413:       // radius sanity relative to segment length
   1414:       if (w->radius > 0.0)
   1415:       {
   1416:         if (ek_enabled)

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L1215:     snprintf(msg, sizeof(msg), "A GD card appears in the deck, but there is no GN card.");
L1222:     snprintf(msg, sizeof(msg), "The GE is set to -1, but there is no GN card in the deck.");
L1306:           snprintf(msg, sizeof(msg), "Parallel wires (tags %d and %d; lines %d and %d) are closer than 0.05 wavelengths but have different segmentation.", a->tag, b->tag, a->line, b->line);
L1345:           snprintf(msg, sizeof(msg), "Connected wires (tags %d and %d; lines %d and %d) have very different segment lengths near the junction (%.4g m vs %.4g m); consider harmonizing segmentation.", a->tag, b->tag, a->line, b->line, aSeg, bSeg);
L1372:       snprintf(msg, sizeof(msg), "With GE I1=1, the wire on line %d (tag %d) has height %.4g m < 1e-3 x segment length (%.4g m); segment ends may be connected to ground. Consider GE -1.", w->line, w->tag, h, segLen);
L1400:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has segment length %.4g m (%.4g lambda) which is >= 0.1 lambda; consider increasing segmentation.", w->line, w->tag, segLen, segFrac);
L1405:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has segment length %.4g m (%.4g lambda); in critical regions, aim for < 0.05 lambda.", w->line, w->tag, segLen, segFrac);
L1410:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has very small segment length %.4g m (%.4g lambda); this may cause excessive segmentation.", w->line, w->tag, segLen, segFrac);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 1421
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 1361-1426) ---
   1361:   for (int i = 0; i < wire_count; i++)
   1362:   {
   1363:     const wire_info_t *w = &wires[i];
   1364:     if (w->segs <= 0)
   1365:       continue;
   1366:     double L = sqrt(pow(w->x2 - w->x1, 2) + pow(w->y2 - w->y1, 2) + pow(w->z2 - w->z1, 2));
   1367:     double segLen = L / (double)w->segs;
   1368:     double dz = fabs(w->z2 - w->z1);
   1369:     double h = fmin(fabs(w->z1), fabs(w->z2));
   1370:     if (dz < 1e-9 && h < (1e-3 * segLen))
   1371:     {
   1372:       snprintf(msg, sizeof(msg), "With GE I1=1, the wire on line %d (tag %d) has height %.4g m < 1e-3 x segment length (%.4g m); segment ends may be connected to ground. Consider GE -1.", w->line, w->tag, h, segLen);
   1373:       add_error(ctx, errors, msg, 0);
   1374:     }
   1375:   }
   1376: }
   1377: 
   1378: // Helper: segment length vs wavelength and radius sanity
   1379: static void check_segment_length_and_radius(const nec_context_t *ctx, errors_list_t *errors,
   1380:                                             const wire_info_t *wires, int wire_count,
   1381:                                             double freq_mhz, int ek_enabled)
   1382: {
   1383:   char msg[MAX_ERROR_LEN];
   1384:   double wlam_m = (freq_mhz > 0.0) ? (CVEL / freq_mhz) : 0.0;
   1385:   if (wlam_m <= 0.0)
   1386:   {
   1387:     return;
   1388:   }
   1389: 
   1390:   for (int i = 0; i < wire_count; i++)
   1391:   {
   1392:     const wire_info_t *w = &wires[i];
   1393:     double L = sqrt(pow(w->x2 - w->x1, 2) + pow(w->y2 - w->y1, 2) + pow(w->z2 - w->z1, 2));
   1394:     if (w->segs > 0)
   1395:     {
   1396:       double segLen = L / (double)w->segs;
   1397:       double segFrac = segLen / wlam_m; // in wavelengths
   1398:       if (segFrac >= 0.10)
   1399:       {
   1400:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has segment length %.4g m (%.4g lambda) which is >= 0.1 lambda; consider increasing segmentation.", w->line, w->tag, segLen, segFrac);
   1401:         add_error(ctx, errors, msg, 0);
   1402:       }
   1403:       else if (segFrac >= 0.05)
   1404:       {
   1405:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has segment length %.4g m (%.4g lambda); in critical regions, aim for < 0.05 lambda.", w->line, w->tag, segLen, segFrac);
   1406:         add_error(ctx, errors, msg, 0);
   1407:       }
   1408:       if (segFrac < 0.001)
   1409:       {
   1410:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has very small segment length %.4g m (%.4g lambda); this may cause excessive segmentation.", w->line, w->tag, segLen, segFrac);
   1411:         add_error(ctx, errors, msg, 0);
   1412:       }
   1413:       // radius sanity relative to segment length
   1414:       if (w->radius > 0.0)
   1415:       {
   1416:         if (ek_enabled)
   1417:         {
   1418:           if (w->radius >= (2.0 * segLen))
   1419:           {
   1420:             snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has radius %.4g m >= 2*len (%.4g m) with extended kernel; reduce radius or increase segmentation.", w->line, w->tag, w->radius, 2.0 * segLen);
>  1421:             add_error(ctx, errors, msg, 0);
   1422:           }
   1423:         }
   1424:         else
   1425:         {
   1426:           if (w->radius >= (segLen / 2.0))

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L1222:     snprintf(msg, sizeof(msg), "The GE is set to -1, but there is no GN card in the deck.");
L1306:           snprintf(msg, sizeof(msg), "Parallel wires (tags %d and %d; lines %d and %d) are closer than 0.05 wavelengths but have different segmentation.", a->tag, b->tag, a->line, b->line);
L1345:           snprintf(msg, sizeof(msg), "Connected wires (tags %d and %d; lines %d and %d) have very different segment lengths near the junction (%.4g m vs %.4g m); consider harmonizing segmentation.", a->tag, b->tag, a->line, b->line, aSeg, bSeg);
L1372:       snprintf(msg, sizeof(msg), "With GE I1=1, the wire on line %d (tag %d) has height %.4g m < 1e-3 x segment length (%.4g m); segment ends may be connected to ground. Consider GE -1.", w->line, w->tag, h, segLen);
L1400:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has segment length %.4g m (%.4g lambda) which is >= 0.1 lambda; consider increasing segmentation.", w->line, w->tag, segLen, segFrac);
L1405:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has segment length %.4g m (%.4g lambda); in critical regions, aim for < 0.05 lambda.", w->line, w->tag, segLen, segFrac);
L1410:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has very small segment length %.4g m (%.4g lambda); this may cause excessive segmentation.", w->line, w->tag, segLen, segFrac);
L1420:             snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has radius %.4g m >= 2*len (%.4g m) with extended kernel; reduce radius or increase segmentation.", w->line, w->tag, w->radius, 2.0 * segLen);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 1429
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 1369-1434) ---
   1369:     double h = fmin(fabs(w->z1), fabs(w->z2));
   1370:     if (dz < 1e-9 && h < (1e-3 * segLen))
   1371:     {
   1372:       snprintf(msg, sizeof(msg), "With GE I1=1, the wire on line %d (tag %d) has height %.4g m < 1e-3 x segment length (%.4g m); segment ends may be connected to ground. Consider GE -1.", w->line, w->tag, h, segLen);
   1373:       add_error(ctx, errors, msg, 0);
   1374:     }
   1375:   }
   1376: }
   1377: 
   1378: // Helper: segment length vs wavelength and radius sanity
   1379: static void check_segment_length_and_radius(const nec_context_t *ctx, errors_list_t *errors,
   1380:                                             const wire_info_t *wires, int wire_count,
   1381:                                             double freq_mhz, int ek_enabled)
   1382: {
   1383:   char msg[MAX_ERROR_LEN];
   1384:   double wlam_m = (freq_mhz > 0.0) ? (CVEL / freq_mhz) : 0.0;
   1385:   if (wlam_m <= 0.0)
   1386:   {
   1387:     return;
   1388:   }
   1389: 
   1390:   for (int i = 0; i < wire_count; i++)
   1391:   {
   1392:     const wire_info_t *w = &wires[i];
   1393:     double L = sqrt(pow(w->x2 - w->x1, 2) + pow(w->y2 - w->y1, 2) + pow(w->z2 - w->z1, 2));
   1394:     if (w->segs > 0)
   1395:     {
   1396:       double segLen = L / (double)w->segs;
   1397:       double segFrac = segLen / wlam_m; // in wavelengths
   1398:       if (segFrac >= 0.10)
   1399:       {
   1400:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has segment length %.4g m (%.4g lambda) which is >= 0.1 lambda; consider increasing segmentation.", w->line, w->tag, segLen, segFrac);
   1401:         add_error(ctx, errors, msg, 0);
   1402:       }
   1403:       else if (segFrac >= 0.05)
   1404:       {
   1405:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has segment length %.4g m (%.4g lambda); in critical regions, aim for < 0.05 lambda.", w->line, w->tag, segLen, segFrac);
   1406:         add_error(ctx, errors, msg, 0);
   1407:       }
   1408:       if (segFrac < 0.001)
   1409:       {
   1410:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has very small segment length %.4g m (%.4g lambda); this may cause excessive segmentation.", w->line, w->tag, segLen, segFrac);
   1411:         add_error(ctx, errors, msg, 0);
   1412:       }
   1413:       // radius sanity relative to segment length
   1414:       if (w->radius > 0.0)
   1415:       {
   1416:         if (ek_enabled)
   1417:         {
   1418:           if (w->radius >= (2.0 * segLen))
   1419:           {
   1420:             snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has radius %.4g m >= 2*len (%.4g m) with extended kernel; reduce radius or increase segmentation.", w->line, w->tag, w->radius, 2.0 * segLen);
   1421:             add_error(ctx, errors, msg, 0);
   1422:           }
   1423:         }
   1424:         else
   1425:         {
   1426:           if (w->radius >= (segLen / 2.0))
   1427:           {
   1428:             snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has radius %.4g m >= len/2 (%.4g m); this violates thin-wire assumptions.", w->line, w->tag, w->radius, segLen / 2.0);
>  1429:             add_error(ctx, errors, msg, 0);
   1430:           }
   1431:           else if (w->radius >= (segLen / 10.0))
   1432:           {
   1433:             snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has radius %.4g m; typical thin-wire usage prefers radius < len/10 (%.4g m).", w->line, w->tag, w->radius, segLen / 10.0);
   1434:             add_error(ctx, errors, msg, 0);

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L1306:           snprintf(msg, sizeof(msg), "Parallel wires (tags %d and %d; lines %d and %d) are closer than 0.05 wavelengths but have different segmentation.", a->tag, b->tag, a->line, b->line);
L1345:           snprintf(msg, sizeof(msg), "Connected wires (tags %d and %d; lines %d and %d) have very different segment lengths near the junction (%.4g m vs %.4g m); consider harmonizing segmentation.", a->tag, b->tag, a->line, b->line, aSeg, bSeg);
L1372:       snprintf(msg, sizeof(msg), "With GE I1=1, the wire on line %d (tag %d) has height %.4g m < 1e-3 x segment length (%.4g m); segment ends may be connected to ground. Consider GE -1.", w->line, w->tag, h, segLen);
L1400:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has segment length %.4g m (%.4g lambda) which is >= 0.1 lambda; consider increasing segmentation.", w->line, w->tag, segLen, segFrac);
L1405:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has segment length %.4g m (%.4g lambda); in critical regions, aim for < 0.05 lambda.", w->line, w->tag, segLen, segFrac);
L1410:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has very small segment length %.4g m (%.4g lambda); this may cause excessive segmentation.", w->line, w->tag, segLen, segFrac);
L1420:             snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has radius %.4g m >= 2*len (%.4g m) with extended kernel; reduce radius or increase segmentation.", w->line, w->tag, w->radius, 2.0 * segLen);
L1428:             snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has radius %.4g m >= len/2 (%.4g m); this violates thin-wire assumptions.", w->line, w->tag, w->radius, segLen / 2.0);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 1434
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 1374-1439) ---
   1374:     }
   1375:   }
   1376: }
   1377: 
   1378: // Helper: segment length vs wavelength and radius sanity
   1379: static void check_segment_length_and_radius(const nec_context_t *ctx, errors_list_t *errors,
   1380:                                             const wire_info_t *wires, int wire_count,
   1381:                                             double freq_mhz, int ek_enabled)
   1382: {
   1383:   char msg[MAX_ERROR_LEN];
   1384:   double wlam_m = (freq_mhz > 0.0) ? (CVEL / freq_mhz) : 0.0;
   1385:   if (wlam_m <= 0.0)
   1386:   {
   1387:     return;
   1388:   }
   1389: 
   1390:   for (int i = 0; i < wire_count; i++)
   1391:   {
   1392:     const wire_info_t *w = &wires[i];
   1393:     double L = sqrt(pow(w->x2 - w->x1, 2) + pow(w->y2 - w->y1, 2) + pow(w->z2 - w->z1, 2));
   1394:     if (w->segs > 0)
   1395:     {
   1396:       double segLen = L / (double)w->segs;
   1397:       double segFrac = segLen / wlam_m; // in wavelengths
   1398:       if (segFrac >= 0.10)
   1399:       {
   1400:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has segment length %.4g m (%.4g lambda) which is >= 0.1 lambda; consider increasing segmentation.", w->line, w->tag, segLen, segFrac);
   1401:         add_error(ctx, errors, msg, 0);
   1402:       }
   1403:       else if (segFrac >= 0.05)
   1404:       {
   1405:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has segment length %.4g m (%.4g lambda); in critical regions, aim for < 0.05 lambda.", w->line, w->tag, segLen, segFrac);
   1406:         add_error(ctx, errors, msg, 0);
   1407:       }
   1408:       if (segFrac < 0.001)
   1409:       {
   1410:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has very small segment length %.4g m (%.4g lambda); this may cause excessive segmentation.", w->line, w->tag, segLen, segFrac);
   1411:         add_error(ctx, errors, msg, 0);
   1412:       }
   1413:       // radius sanity relative to segment length
   1414:       if (w->radius > 0.0)
   1415:       {
   1416:         if (ek_enabled)
   1417:         {
   1418:           if (w->radius >= (2.0 * segLen))
   1419:           {
   1420:             snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has radius %.4g m >= 2*len (%.4g m) with extended kernel; reduce radius or increase segmentation.", w->line, w->tag, w->radius, 2.0 * segLen);
   1421:             add_error(ctx, errors, msg, 0);
   1422:           }
   1423:         }
   1424:         else
   1425:         {
   1426:           if (w->radius >= (segLen / 2.0))
   1427:           {
   1428:             snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has radius %.4g m >= len/2 (%.4g m); this violates thin-wire assumptions.", w->line, w->tag, w->radius, segLen / 2.0);
   1429:             add_error(ctx, errors, msg, 0);
   1430:           }
   1431:           else if (w->radius >= (segLen / 10.0))
   1432:           {
   1433:             snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has radius %.4g m; typical thin-wire usage prefers radius < len/10 (%.4g m).", w->line, w->tag, w->radius, segLen / 10.0);
>  1434:             add_error(ctx, errors, msg, 0);
   1435:           }
   1436:         }
   1437:       }
   1438:     }
   1439:   }

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L1306:           snprintf(msg, sizeof(msg), "Parallel wires (tags %d and %d; lines %d and %d) are closer than 0.05 wavelengths but have different segmentation.", a->tag, b->tag, a->line, b->line);
L1345:           snprintf(msg, sizeof(msg), "Connected wires (tags %d and %d; lines %d and %d) have very different segment lengths near the junction (%.4g m vs %.4g m); consider harmonizing segmentation.", a->tag, b->tag, a->line, b->line, aSeg, bSeg);
L1372:       snprintf(msg, sizeof(msg), "With GE I1=1, the wire on line %d (tag %d) has height %.4g m < 1e-3 x segment length (%.4g m); segment ends may be connected to ground. Consider GE -1.", w->line, w->tag, h, segLen);
L1400:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has segment length %.4g m (%.4g lambda) which is >= 0.1 lambda; consider increasing segmentation.", w->line, w->tag, segLen, segFrac);
L1405:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has segment length %.4g m (%.4g lambda); in critical regions, aim for < 0.05 lambda.", w->line, w->tag, segLen, segFrac);
L1410:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has very small segment length %.4g m (%.4g lambda); this may cause excessive segmentation.", w->line, w->tag, segLen, segFrac);
L1420:             snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has radius %.4g m >= 2*len (%.4g m) with extended kernel; reduce radius or increase segmentation.", w->line, w->tag, w->radius, 2.0 * segLen);
L1428:             snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has radius %.4g m >= len/2 (%.4g m); this violates thin-wire assumptions.", w->line, w->tag, w->radius, segLen / 2.0);
L1433:             snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has radius %.4g m; typical thin-wire usage prefers radius < len/10 (%.4g m).", w->line, w->tag, w->radius, segLen / 10.0);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 1483
CALL: add_error(ctx, errors, msg, 1);
--- Context (lines 1423-1488) ---
   1423:         }
   1424:         else
   1425:         {
   1426:           if (w->radius >= (segLen / 2.0))
   1427:           {
   1428:             snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has radius %.4g m >= len/2 (%.4g m); this violates thin-wire assumptions.", w->line, w->tag, w->radius, segLen / 2.0);
   1429:             add_error(ctx, errors, msg, 0);
   1430:           }
   1431:           else if (w->radius >= (segLen / 10.0))
   1432:           {
   1433:             snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has radius %.4g m; typical thin-wire usage prefers radius < len/10 (%.4g m).", w->line, w->tag, w->radius, segLen / 10.0);
   1434:             add_error(ctx, errors, msg, 0);
   1435:           }
   1436:         }
   1437:       }
   1438:     }
   1439:   }
   1440: }
   1441: 
   1442: /*******************************************************************
   1443:  * test_duplicate_tags
   1444:  *
   1445:  * test_duplicate_tags checks to see if there is more than one card
   1446:  * with the same tag on it. this will not notice problems if there
   1447:  * is a GM or similar card that creates new tags, that only happens
   1448:  * when the geometry is segmented
   1449:  *
   1450:  * @param deck the deck_t to be tested
   1451:  * @param errors the errors_list_t to add new messages to
   1452:  *
   1453:  */
   1454: void test_duplicate_tags(const nec_context_t *ctx, const deck_t *deck, errors_list_t *errors)
   1455: {
   1456:   // we will also check to see if there are duplicate tags
   1457:   char msg[MAX_ERROR_LEN];
   1458: 
   1459:   // Only consider duplicates within the geometry section
   1460:   int gstart = deck->geometry_start;
   1461:   int gend = deck->geometry_end; // index of GE card
   1462:   if (gstart < 0 || gend < 0 || gend <= gstart)
   1463:   {
   1464:     // Fallback: search all cards but restrict to geometry types for both sides
   1465:     gstart = 0;
   1466:     gend = deck->num_cards;
   1467:   }
   1468: 
   1469:   // now check if there are any duplicate tags in the geometry
   1470:   // NOTE: this doesn't test for new tags generated by GM or similar
   1471:   for (int i = gstart; i < gend; i++)
   1472:   {
   1473:     if (is_geometry(&deck->cards[i]) && card_has_itag(&deck->cards[i]) && deck->cards[i].i[1] > 0)
   1474:     {
   1475:       int tag_i = deck->cards[i].i[1];
   1476:       for (int j = i + 1; j < gend; j++)
   1477:       {
   1478:         if (is_geometry(&deck->cards[j]) && card_has_itag(&deck->cards[j]) && deck->cards[j].i[1] > 0)
   1479:         {
   1480:           if (deck->cards[j].i[1] == tag_i)
   1481:           {
   1482:             snprintf(msg, sizeof(msg), "The tag number %d is found on card %d and card %d.", tag_i, i + 1, j + 1);
>  1483:             add_error(ctx, errors, msg, 1);
   1484:           }
   1485:         }
   1486:       }
   1487:     }
   1488:   }

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L1306:           snprintf(msg, sizeof(msg), "Parallel wires (tags %d and %d; lines %d and %d) are closer than 0.05 wavelengths but have different segmentation.", a->tag, b->tag, a->line, b->line);
L1345:           snprintf(msg, sizeof(msg), "Connected wires (tags %d and %d; lines %d and %d) have very different segment lengths near the junction (%.4g m vs %.4g m); consider harmonizing segmentation.", a->tag, b->tag, a->line, b->line, aSeg, bSeg);
L1372:       snprintf(msg, sizeof(msg), "With GE I1=1, the wire on line %d (tag %d) has height %.4g m < 1e-3 x segment length (%.4g m); segment ends may be connected to ground. Consider GE -1.", w->line, w->tag, h, segLen);
L1400:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has segment length %.4g m (%.4g lambda) which is >= 0.1 lambda; consider increasing segmentation.", w->line, w->tag, segLen, segFrac);
L1405:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has segment length %.4g m (%.4g lambda); in critical regions, aim for < 0.05 lambda.", w->line, w->tag, segLen, segFrac);
L1410:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has very small segment length %.4g m (%.4g lambda); this may cause excessive segmentation.", w->line, w->tag, segLen, segFrac);
L1420:             snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has radius %.4g m >= 2*len (%.4g m) with extended kernel; reduce radius or increase segmentation.", w->line, w->tag, w->radius, 2.0 * segLen);
L1428:             snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has radius %.4g m >= len/2 (%.4g m); this violates thin-wire assumptions.", w->line, w->tag, w->radius, segLen / 2.0);
L1433:             snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has radius %.4g m; typical thin-wire usage prefers radius < len/10 (%.4g m).", w->line, w->tag, w->radius, segLen / 10.0);
L1482:             snprintf(msg, sizeof(msg), "The tag number %d is found on card %d and card %d.", tag_i, i + 1, j + 1);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 1521
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 1461-1526) ---
   1461:   int gend = deck->geometry_end; // index of GE card
   1462:   if (gstart < 0 || gend < 0 || gend <= gstart)
   1463:   {
   1464:     // Fallback: search all cards but restrict to geometry types for both sides
   1465:     gstart = 0;
   1466:     gend = deck->num_cards;
   1467:   }
   1468: 
   1469:   // now check if there are any duplicate tags in the geometry
   1470:   // NOTE: this doesn't test for new tags generated by GM or similar
   1471:   for (int i = gstart; i < gend; i++)
   1472:   {
   1473:     if (is_geometry(&deck->cards[i]) && card_has_itag(&deck->cards[i]) && deck->cards[i].i[1] > 0)
   1474:     {
   1475:       int tag_i = deck->cards[i].i[1];
   1476:       for (int j = i + 1; j < gend; j++)
   1477:       {
   1478:         if (is_geometry(&deck->cards[j]) && card_has_itag(&deck->cards[j]) && deck->cards[j].i[1] > 0)
   1479:         {
   1480:           if (deck->cards[j].i[1] == tag_i)
   1481:           {
   1482:             snprintf(msg, sizeof(msg), "The tag number %d is found on card %d and card %d.", tag_i, i + 1, j + 1);
   1483:             add_error(ctx, errors, msg, 1);
   1484:           }
   1485:         }
   1486:       }
   1487:     }
   1488:   }
   1489: }
   1490: /* end test_duplicate_tags */
   1491: 
   1492: /******************************************************************************
   1493:  * test_card_inputs
   1494:  *
   1495:  * test_card_inputs looks at each card to ensure it has the right number
   1496:  * and type of inputs. For instance, an FR card has two forms; if I1 is 0
   1497:  * then it has to have no other values, if I1 is non-zero, it has to have
   1498:  * F1 and F2.
   1499:  *
   1500:  * @param deck the deck_t to be tested
   1501:  * @param errors the errors_list_t to add new messages to
   1502:  *
   1503:  * TODO: this needs to be greatly expanded!
   1504:  *
   1505:  */
   1506: void test_card_inputs(const nec_context_t *ctx, const deck_t *deck, errors_list_t *errors)
   1507: {
   1508:   const char *code;
   1509:   char msg[MAX_ERROR_LEN];
   1510: 
   1511:   for (int i = 0; i < deck->num_cards; i++)
   1512:   {
   1513:     code = deck->cards[i].card_code;
   1514: 
   1515:     // CE: comment end — should not have numeric inputs
   1516:     if (strcmp(code, "CE") == 0)
   1517:     {
   1518:       if (deck->cards[i].ints_used > 0 || deck->cards[i].flts_used > 0)
   1519:       {
   1520:         snprintf(msg, sizeof(msg), "The card on line %d is a CE but has numeric inputs.", i);
>  1521:         add_error(ctx, errors, msg, 0);
   1522:       }
   1523:     }
   1524: 
   1525:     // EN: end of deck — should not have numeric inputs
   1526:     if (strcmp(code, "EN") == 0)

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L1345:           snprintf(msg, sizeof(msg), "Connected wires (tags %d and %d; lines %d and %d) have very different segment lengths near the junction (%.4g m vs %.4g m); consider harmonizing segmentation.", a->tag, b->tag, a->line, b->line, aSeg, bSeg);
L1372:       snprintf(msg, sizeof(msg), "With GE I1=1, the wire on line %d (tag %d) has height %.4g m < 1e-3 x segment length (%.4g m); segment ends may be connected to ground. Consider GE -1.", w->line, w->tag, h, segLen);
L1400:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has segment length %.4g m (%.4g lambda) which is >= 0.1 lambda; consider increasing segmentation.", w->line, w->tag, segLen, segFrac);
L1405:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has segment length %.4g m (%.4g lambda); in critical regions, aim for < 0.05 lambda.", w->line, w->tag, segLen, segFrac);
L1410:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has very small segment length %.4g m (%.4g lambda); this may cause excessive segmentation.", w->line, w->tag, segLen, segFrac);
L1420:             snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has radius %.4g m >= 2*len (%.4g m) with extended kernel; reduce radius or increase segmentation.", w->line, w->tag, w->radius, 2.0 * segLen);
L1428:             snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has radius %.4g m >= len/2 (%.4g m); this violates thin-wire assumptions.", w->line, w->tag, w->radius, segLen / 2.0);
L1433:             snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has radius %.4g m; typical thin-wire usage prefers radius < len/10 (%.4g m).", w->line, w->tag, w->radius, segLen / 10.0);
L1482:             snprintf(msg, sizeof(msg), "The tag number %d is found on card %d and card %d.", tag_i, i + 1, j + 1);
L1520:         snprintf(msg, sizeof(msg), "The card on line %d is a CE but has numeric inputs.", i);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 1531
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 1471-1536) ---
   1471:   for (int i = gstart; i < gend; i++)
   1472:   {
   1473:     if (is_geometry(&deck->cards[i]) && card_has_itag(&deck->cards[i]) && deck->cards[i].i[1] > 0)
   1474:     {
   1475:       int tag_i = deck->cards[i].i[1];
   1476:       for (int j = i + 1; j < gend; j++)
   1477:       {
   1478:         if (is_geometry(&deck->cards[j]) && card_has_itag(&deck->cards[j]) && deck->cards[j].i[1] > 0)
   1479:         {
   1480:           if (deck->cards[j].i[1] == tag_i)
   1481:           {
   1482:             snprintf(msg, sizeof(msg), "The tag number %d is found on card %d and card %d.", tag_i, i + 1, j + 1);
   1483:             add_error(ctx, errors, msg, 1);
   1484:           }
   1485:         }
   1486:       }
   1487:     }
   1488:   }
   1489: }
   1490: /* end test_duplicate_tags */
   1491: 
   1492: /******************************************************************************
   1493:  * test_card_inputs
   1494:  *
   1495:  * test_card_inputs looks at each card to ensure it has the right number
   1496:  * and type of inputs. For instance, an FR card has two forms; if I1 is 0
   1497:  * then it has to have no other values, if I1 is non-zero, it has to have
   1498:  * F1 and F2.
   1499:  *
   1500:  * @param deck the deck_t to be tested
   1501:  * @param errors the errors_list_t to add new messages to
   1502:  *
   1503:  * TODO: this needs to be greatly expanded!
   1504:  *
   1505:  */
   1506: void test_card_inputs(const nec_context_t *ctx, const deck_t *deck, errors_list_t *errors)
   1507: {
   1508:   const char *code;
   1509:   char msg[MAX_ERROR_LEN];
   1510: 
   1511:   for (int i = 0; i < deck->num_cards; i++)
   1512:   {
   1513:     code = deck->cards[i].card_code;
   1514: 
   1515:     // CE: comment end — should not have numeric inputs
   1516:     if (strcmp(code, "CE") == 0)
   1517:     {
   1518:       if (deck->cards[i].ints_used > 0 || deck->cards[i].flts_used > 0)
   1519:       {
   1520:         snprintf(msg, sizeof(msg), "The card on line %d is a CE but has numeric inputs.", i);
   1521:         add_error(ctx, errors, msg, 0);
   1522:       }
   1523:     }
   1524: 
   1525:     // EN: end of deck — should not have numeric inputs
   1526:     if (strcmp(code, "EN") == 0)
   1527:     {
   1528:       if (deck->cards[i].ints_used > 0 || deck->cards[i].flts_used > 0)
   1529:       {
   1530:         snprintf(msg, sizeof(msg), "The card on line %d is an EN but has numeric inputs.", i);
>  1531:         add_error(ctx, errors, msg, 0);
   1532:       }
   1533:     }
   1534: 
   1535:     // FRs: allow single-frequency (I2=0) or stepped (I2>0)
   1536:     if (strcmp(code, "FR") == 0)

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L1345:           snprintf(msg, sizeof(msg), "Connected wires (tags %d and %d; lines %d and %d) have very different segment lengths near the junction (%.4g m vs %.4g m); consider harmonizing segmentation.", a->tag, b->tag, a->line, b->line, aSeg, bSeg);
L1372:       snprintf(msg, sizeof(msg), "With GE I1=1, the wire on line %d (tag %d) has height %.4g m < 1e-3 x segment length (%.4g m); segment ends may be connected to ground. Consider GE -1.", w->line, w->tag, h, segLen);
L1400:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has segment length %.4g m (%.4g lambda) which is >= 0.1 lambda; consider increasing segmentation.", w->line, w->tag, segLen, segFrac);
L1405:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has segment length %.4g m (%.4g lambda); in critical regions, aim for < 0.05 lambda.", w->line, w->tag, segLen, segFrac);
L1410:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has very small segment length %.4g m (%.4g lambda); this may cause excessive segmentation.", w->line, w->tag, segLen, segFrac);
L1420:             snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has radius %.4g m >= 2*len (%.4g m) with extended kernel; reduce radius or increase segmentation.", w->line, w->tag, w->radius, 2.0 * segLen);
L1428:             snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has radius %.4g m >= len/2 (%.4g m); this violates thin-wire assumptions.", w->line, w->tag, w->radius, segLen / 2.0);
L1433:             snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has radius %.4g m; typical thin-wire usage prefers radius < len/10 (%.4g m).", w->line, w->tag, w->radius, segLen / 10.0);
L1482:             snprintf(msg, sizeof(msg), "The tag number %d is found on card %d and card %d.", tag_i, i + 1, j + 1);
L1520:         snprintf(msg, sizeof(msg), "The card on line %d is a CE but has numeric inputs.", i);
L1530:         snprintf(msg, sizeof(msg), "The card on line %d is an EN but has numeric inputs.", i);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 1542
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 1482-1547) ---
   1482:             snprintf(msg, sizeof(msg), "The tag number %d is found on card %d and card %d.", tag_i, i + 1, j + 1);
   1483:             add_error(ctx, errors, msg, 1);
   1484:           }
   1485:         }
   1486:       }
   1487:     }
   1488:   }
   1489: }
   1490: /* end test_duplicate_tags */
   1491: 
   1492: /******************************************************************************
   1493:  * test_card_inputs
   1494:  *
   1495:  * test_card_inputs looks at each card to ensure it has the right number
   1496:  * and type of inputs. For instance, an FR card has two forms; if I1 is 0
   1497:  * then it has to have no other values, if I1 is non-zero, it has to have
   1498:  * F1 and F2.
   1499:  *
   1500:  * @param deck the deck_t to be tested
   1501:  * @param errors the errors_list_t to add new messages to
   1502:  *
   1503:  * TODO: this needs to be greatly expanded!
   1504:  *
   1505:  */
   1506: void test_card_inputs(const nec_context_t *ctx, const deck_t *deck, errors_list_t *errors)
   1507: {
   1508:   const char *code;
   1509:   char msg[MAX_ERROR_LEN];
   1510: 
   1511:   for (int i = 0; i < deck->num_cards; i++)
   1512:   {
   1513:     code = deck->cards[i].card_code;
   1514: 
   1515:     // CE: comment end — should not have numeric inputs
   1516:     if (strcmp(code, "CE") == 0)
   1517:     {
   1518:       if (deck->cards[i].ints_used > 0 || deck->cards[i].flts_used > 0)
   1519:       {
   1520:         snprintf(msg, sizeof(msg), "The card on line %d is a CE but has numeric inputs.", i);
   1521:         add_error(ctx, errors, msg, 0);
   1522:       }
   1523:     }
   1524: 
   1525:     // EN: end of deck — should not have numeric inputs
   1526:     if (strcmp(code, "EN") == 0)
   1527:     {
   1528:       if (deck->cards[i].ints_used > 0 || deck->cards[i].flts_used > 0)
   1529:       {
   1530:         snprintf(msg, sizeof(msg), "The card on line %d is an EN but has numeric inputs.", i);
   1531:         add_error(ctx, errors, msg, 0);
   1532:       }
   1533:     }
   1534: 
   1535:     // FRs: allow single-frequency (I2=0) or stepped (I2>0)
   1536:     if (strcmp(code, "FR") == 0)
   1537:     {
   1538:       // there must be a value in F1
   1539:       if (deck->cards[i].f[1] == 0)
   1540:       {
   1541:         snprintf(msg, sizeof(msg), "The card on line %d is a FR but has no base frequency in F1.", i);
>  1542:         add_error(ctx, errors, msg, 0);
   1543:       }
   1544:       // Single-frequency: I2==0 should have F2==0
   1545:       if (deck->cards[i].i[2] == 0 && deck->cards[i].f[2] != 0)
   1546:       {
   1547:         snprintf(msg, sizeof(msg), "The card on line %d is a FR with I2 = 0 (single frequency), but has a non-zero F2.", i);

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L1345:           snprintf(msg, sizeof(msg), "Connected wires (tags %d and %d; lines %d and %d) have very different segment lengths near the junction (%.4g m vs %.4g m); consider harmonizing segmentation.", a->tag, b->tag, a->line, b->line, aSeg, bSeg);
L1372:       snprintf(msg, sizeof(msg), "With GE I1=1, the wire on line %d (tag %d) has height %.4g m < 1e-3 x segment length (%.4g m); segment ends may be connected to ground. Consider GE -1.", w->line, w->tag, h, segLen);
L1400:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has segment length %.4g m (%.4g lambda) which is >= 0.1 lambda; consider increasing segmentation.", w->line, w->tag, segLen, segFrac);
L1405:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has segment length %.4g m (%.4g lambda); in critical regions, aim for < 0.05 lambda.", w->line, w->tag, segLen, segFrac);
L1410:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has very small segment length %.4g m (%.4g lambda); this may cause excessive segmentation.", w->line, w->tag, segLen, segFrac);
L1420:             snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has radius %.4g m >= 2*len (%.4g m) with extended kernel; reduce radius or increase segmentation.", w->line, w->tag, w->radius, 2.0 * segLen);
L1428:             snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has radius %.4g m >= len/2 (%.4g m); this violates thin-wire assumptions.", w->line, w->tag, w->radius, segLen / 2.0);
L1433:             snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has radius %.4g m; typical thin-wire usage prefers radius < len/10 (%.4g m).", w->line, w->tag, w->radius, segLen / 10.0);
L1482:             snprintf(msg, sizeof(msg), "The tag number %d is found on card %d and card %d.", tag_i, i + 1, j + 1);
L1520:         snprintf(msg, sizeof(msg), "The card on line %d is a CE but has numeric inputs.", i);
L1530:         snprintf(msg, sizeof(msg), "The card on line %d is an EN but has numeric inputs.", i);
L1541:         snprintf(msg, sizeof(msg), "The card on line %d is a FR but has no base frequency in F1.", i);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 1548
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 1488-1553) ---
   1488:   }
   1489: }
   1490: /* end test_duplicate_tags */
   1491: 
   1492: /******************************************************************************
   1493:  * test_card_inputs
   1494:  *
   1495:  * test_card_inputs looks at each card to ensure it has the right number
   1496:  * and type of inputs. For instance, an FR card has two forms; if I1 is 0
   1497:  * then it has to have no other values, if I1 is non-zero, it has to have
   1498:  * F1 and F2.
   1499:  *
   1500:  * @param deck the deck_t to be tested
   1501:  * @param errors the errors_list_t to add new messages to
   1502:  *
   1503:  * TODO: this needs to be greatly expanded!
   1504:  *
   1505:  */
   1506: void test_card_inputs(const nec_context_t *ctx, const deck_t *deck, errors_list_t *errors)
   1507: {
   1508:   const char *code;
   1509:   char msg[MAX_ERROR_LEN];
   1510: 
   1511:   for (int i = 0; i < deck->num_cards; i++)
   1512:   {
   1513:     code = deck->cards[i].card_code;
   1514: 
   1515:     // CE: comment end — should not have numeric inputs
   1516:     if (strcmp(code, "CE") == 0)
   1517:     {
   1518:       if (deck->cards[i].ints_used > 0 || deck->cards[i].flts_used > 0)
   1519:       {
   1520:         snprintf(msg, sizeof(msg), "The card on line %d is a CE but has numeric inputs.", i);
   1521:         add_error(ctx, errors, msg, 0);
   1522:       }
   1523:     }
   1524: 
   1525:     // EN: end of deck — should not have numeric inputs
   1526:     if (strcmp(code, "EN") == 0)
   1527:     {
   1528:       if (deck->cards[i].ints_used > 0 || deck->cards[i].flts_used > 0)
   1529:       {
   1530:         snprintf(msg, sizeof(msg), "The card on line %d is an EN but has numeric inputs.", i);
   1531:         add_error(ctx, errors, msg, 0);
   1532:       }
   1533:     }
   1534: 
   1535:     // FRs: allow single-frequency (I2=0) or stepped (I2>0)
   1536:     if (strcmp(code, "FR") == 0)
   1537:     {
   1538:       // there must be a value in F1
   1539:       if (deck->cards[i].f[1] == 0)
   1540:       {
   1541:         snprintf(msg, sizeof(msg), "The card on line %d is a FR but has no base frequency in F1.", i);
   1542:         add_error(ctx, errors, msg, 0);
   1543:       }
   1544:       // Single-frequency: I2==0 should have F2==0
   1545:       if (deck->cards[i].i[2] == 0 && deck->cards[i].f[2] != 0)
   1546:       {
   1547:         snprintf(msg, sizeof(msg), "The card on line %d is a FR with I2 = 0 (single frequency), but has a non-zero F2.", i);
>  1548:         add_error(ctx, errors, msg, 0);
   1549:       }
   1550:       // Stepped: I2>0 requires positive step in F2
   1551:       else if (deck->cards[i].i[2] > 0 && deck->cards[i].f[2] <= 0)
   1552:       {
   1553:         snprintf(msg, sizeof(msg), "The card on line %d is a FR with I2 > 0 (stepped), but F2 is not a positive step.", i);

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L1372:       snprintf(msg, sizeof(msg), "With GE I1=1, the wire on line %d (tag %d) has height %.4g m < 1e-3 x segment length (%.4g m); segment ends may be connected to ground. Consider GE -1.", w->line, w->tag, h, segLen);
L1400:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has segment length %.4g m (%.4g lambda) which is >= 0.1 lambda; consider increasing segmentation.", w->line, w->tag, segLen, segFrac);
L1405:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has segment length %.4g m (%.4g lambda); in critical regions, aim for < 0.05 lambda.", w->line, w->tag, segLen, segFrac);
L1410:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has very small segment length %.4g m (%.4g lambda); this may cause excessive segmentation.", w->line, w->tag, segLen, segFrac);
L1420:             snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has radius %.4g m >= 2*len (%.4g m) with extended kernel; reduce radius or increase segmentation.", w->line, w->tag, w->radius, 2.0 * segLen);
L1428:             snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has radius %.4g m >= len/2 (%.4g m); this violates thin-wire assumptions.", w->line, w->tag, w->radius, segLen / 2.0);
L1433:             snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has radius %.4g m; typical thin-wire usage prefers radius < len/10 (%.4g m).", w->line, w->tag, w->radius, segLen / 10.0);
L1482:             snprintf(msg, sizeof(msg), "The tag number %d is found on card %d and card %d.", tag_i, i + 1, j + 1);
L1520:         snprintf(msg, sizeof(msg), "The card on line %d is a CE but has numeric inputs.", i);
L1530:         snprintf(msg, sizeof(msg), "The card on line %d is an EN but has numeric inputs.", i);
L1541:         snprintf(msg, sizeof(msg), "The card on line %d is a FR but has no base frequency in F1.", i);
L1547:         snprintf(msg, sizeof(msg), "The card on line %d is a FR with I2 = 0 (single frequency), but has a non-zero F2.", i);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 1554
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 1494-1559) ---
   1494:  *
   1495:  * test_card_inputs looks at each card to ensure it has the right number
   1496:  * and type of inputs. For instance, an FR card has two forms; if I1 is 0
   1497:  * then it has to have no other values, if I1 is non-zero, it has to have
   1498:  * F1 and F2.
   1499:  *
   1500:  * @param deck the deck_t to be tested
   1501:  * @param errors the errors_list_t to add new messages to
   1502:  *
   1503:  * TODO: this needs to be greatly expanded!
   1504:  *
   1505:  */
   1506: void test_card_inputs(const nec_context_t *ctx, const deck_t *deck, errors_list_t *errors)
   1507: {
   1508:   const char *code;
   1509:   char msg[MAX_ERROR_LEN];
   1510: 
   1511:   for (int i = 0; i < deck->num_cards; i++)
   1512:   {
   1513:     code = deck->cards[i].card_code;
   1514: 
   1515:     // CE: comment end — should not have numeric inputs
   1516:     if (strcmp(code, "CE") == 0)
   1517:     {
   1518:       if (deck->cards[i].ints_used > 0 || deck->cards[i].flts_used > 0)
   1519:       {
   1520:         snprintf(msg, sizeof(msg), "The card on line %d is a CE but has numeric inputs.", i);
   1521:         add_error(ctx, errors, msg, 0);
   1522:       }
   1523:     }
   1524: 
   1525:     // EN: end of deck — should not have numeric inputs
   1526:     if (strcmp(code, "EN") == 0)
   1527:     {
   1528:       if (deck->cards[i].ints_used > 0 || deck->cards[i].flts_used > 0)
   1529:       {
   1530:         snprintf(msg, sizeof(msg), "The card on line %d is an EN but has numeric inputs.", i);
   1531:         add_error(ctx, errors, msg, 0);
   1532:       }
   1533:     }
   1534: 
   1535:     // FRs: allow single-frequency (I2=0) or stepped (I2>0)
   1536:     if (strcmp(code, "FR") == 0)
   1537:     {
   1538:       // there must be a value in F1
   1539:       if (deck->cards[i].f[1] == 0)
   1540:       {
   1541:         snprintf(msg, sizeof(msg), "The card on line %d is a FR but has no base frequency in F1.", i);
   1542:         add_error(ctx, errors, msg, 0);
   1543:       }
   1544:       // Single-frequency: I2==0 should have F2==0
   1545:       if (deck->cards[i].i[2] == 0 && deck->cards[i].f[2] != 0)
   1546:       {
   1547:         snprintf(msg, sizeof(msg), "The card on line %d is a FR with I2 = 0 (single frequency), but has a non-zero F2.", i);
   1548:         add_error(ctx, errors, msg, 0);
   1549:       }
   1550:       // Stepped: I2>0 requires positive step in F2
   1551:       else if (deck->cards[i].i[2] > 0 && deck->cards[i].f[2] <= 0)
   1552:       {
   1553:         snprintf(msg, sizeof(msg), "The card on line %d is a FR with I2 > 0 (stepped), but F2 is not a positive step.", i);
>  1554:         add_error(ctx, errors, msg, 0);
   1555:       }
   1556:     }
   1557: 
   1558:     // GW: wire geometry — require positive segment count and radius
   1559:     if (strcmp(code, "GW") == 0)

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L1372:       snprintf(msg, sizeof(msg), "With GE I1=1, the wire on line %d (tag %d) has height %.4g m < 1e-3 x segment length (%.4g m); segment ends may be connected to ground. Consider GE -1.", w->line, w->tag, h, segLen);
L1400:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has segment length %.4g m (%.4g lambda) which is >= 0.1 lambda; consider increasing segmentation.", w->line, w->tag, segLen, segFrac);
L1405:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has segment length %.4g m (%.4g lambda); in critical regions, aim for < 0.05 lambda.", w->line, w->tag, segLen, segFrac);
L1410:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has very small segment length %.4g m (%.4g lambda); this may cause excessive segmentation.", w->line, w->tag, segLen, segFrac);
L1420:             snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has radius %.4g m >= 2*len (%.4g m) with extended kernel; reduce radius or increase segmentation.", w->line, w->tag, w->radius, 2.0 * segLen);
L1428:             snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has radius %.4g m >= len/2 (%.4g m); this violates thin-wire assumptions.", w->line, w->tag, w->radius, segLen / 2.0);
L1433:             snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has radius %.4g m; typical thin-wire usage prefers radius < len/10 (%.4g m).", w->line, w->tag, w->radius, segLen / 10.0);
L1482:             snprintf(msg, sizeof(msg), "The tag number %d is found on card %d and card %d.", tag_i, i + 1, j + 1);
L1520:         snprintf(msg, sizeof(msg), "The card on line %d is a CE but has numeric inputs.", i);
L1530:         snprintf(msg, sizeof(msg), "The card on line %d is an EN but has numeric inputs.", i);
L1541:         snprintf(msg, sizeof(msg), "The card on line %d is a FR but has no base frequency in F1.", i);
L1547:         snprintf(msg, sizeof(msg), "The card on line %d is a FR with I2 = 0 (single frequency), but has a non-zero F2.", i);
L1553:         snprintf(msg, sizeof(msg), "The card on line %d is a FR with I2 > 0 (stepped), but F2 is not a positive step.", i);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 1565
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 1505-1570) ---
   1505:  */
   1506: void test_card_inputs(const nec_context_t *ctx, const deck_t *deck, errors_list_t *errors)
   1507: {
   1508:   const char *code;
   1509:   char msg[MAX_ERROR_LEN];
   1510: 
   1511:   for (int i = 0; i < deck->num_cards; i++)
   1512:   {
   1513:     code = deck->cards[i].card_code;
   1514: 
   1515:     // CE: comment end — should not have numeric inputs
   1516:     if (strcmp(code, "CE") == 0)
   1517:     {
   1518:       if (deck->cards[i].ints_used > 0 || deck->cards[i].flts_used > 0)
   1519:       {
   1520:         snprintf(msg, sizeof(msg), "The card on line %d is a CE but has numeric inputs.", i);
   1521:         add_error(ctx, errors, msg, 0);
   1522:       }
   1523:     }
   1524: 
   1525:     // EN: end of deck — should not have numeric inputs
   1526:     if (strcmp(code, "EN") == 0)
   1527:     {
   1528:       if (deck->cards[i].ints_used > 0 || deck->cards[i].flts_used > 0)
   1529:       {
   1530:         snprintf(msg, sizeof(msg), "The card on line %d is an EN but has numeric inputs.", i);
   1531:         add_error(ctx, errors, msg, 0);
   1532:       }
   1533:     }
   1534: 
   1535:     // FRs: allow single-frequency (I2=0) or stepped (I2>0)
   1536:     if (strcmp(code, "FR") == 0)
   1537:     {
   1538:       // there must be a value in F1
   1539:       if (deck->cards[i].f[1] == 0)
   1540:       {
   1541:         snprintf(msg, sizeof(msg), "The card on line %d is a FR but has no base frequency in F1.", i);
   1542:         add_error(ctx, errors, msg, 0);
   1543:       }
   1544:       // Single-frequency: I2==0 should have F2==0
   1545:       if (deck->cards[i].i[2] == 0 && deck->cards[i].f[2] != 0)
   1546:       {
   1547:         snprintf(msg, sizeof(msg), "The card on line %d is a FR with I2 = 0 (single frequency), but has a non-zero F2.", i);
   1548:         add_error(ctx, errors, msg, 0);
   1549:       }
   1550:       // Stepped: I2>0 requires positive step in F2
   1551:       else if (deck->cards[i].i[2] > 0 && deck->cards[i].f[2] <= 0)
   1552:       {
   1553:         snprintf(msg, sizeof(msg), "The card on line %d is a FR with I2 > 0 (stepped), but F2 is not a positive step.", i);
   1554:         add_error(ctx, errors, msg, 0);
   1555:       }
   1556:     }
   1557: 
   1558:     // GW: wire geometry — require positive segment count and radius
   1559:     if (strcmp(code, "GW") == 0)
   1560:     {
   1561:       // At least tag and segment count
   1562:       if (deck->cards[i].ints_used < 2)
   1563:       {
   1564:         snprintf(msg, sizeof(msg), "The card on line %d is a GW but has fewer than 2 integer inputs (tag, segments).", i);
>  1565:         add_error(ctx, errors, msg, 0);
   1566:       }
   1567:       else
   1568:       {
   1569:         int segs = deck->cards[i].i[2];
   1570:         if (segs <= 0)

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L1372:       snprintf(msg, sizeof(msg), "With GE I1=1, the wire on line %d (tag %d) has height %.4g m < 1e-3 x segment length (%.4g m); segment ends may be connected to ground. Consider GE -1.", w->line, w->tag, h, segLen);
L1400:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has segment length %.4g m (%.4g lambda) which is >= 0.1 lambda; consider increasing segmentation.", w->line, w->tag, segLen, segFrac);
L1405:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has segment length %.4g m (%.4g lambda); in critical regions, aim for < 0.05 lambda.", w->line, w->tag, segLen, segFrac);
L1410:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has very small segment length %.4g m (%.4g lambda); this may cause excessive segmentation.", w->line, w->tag, segLen, segFrac);
L1420:             snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has radius %.4g m >= 2*len (%.4g m) with extended kernel; reduce radius or increase segmentation.", w->line, w->tag, w->radius, 2.0 * segLen);
L1428:             snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has radius %.4g m >= len/2 (%.4g m); this violates thin-wire assumptions.", w->line, w->tag, w->radius, segLen / 2.0);
L1433:             snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has radius %.4g m; typical thin-wire usage prefers radius < len/10 (%.4g m).", w->line, w->tag, w->radius, segLen / 10.0);
L1482:             snprintf(msg, sizeof(msg), "The tag number %d is found on card %d and card %d.", tag_i, i + 1, j + 1);
L1520:         snprintf(msg, sizeof(msg), "The card on line %d is a CE but has numeric inputs.", i);
L1530:         snprintf(msg, sizeof(msg), "The card on line %d is an EN but has numeric inputs.", i);
L1541:         snprintf(msg, sizeof(msg), "The card on line %d is a FR but has no base frequency in F1.", i);
L1547:         snprintf(msg, sizeof(msg), "The card on line %d is a FR with I2 = 0 (single frequency), but has a non-zero F2.", i);
L1553:         snprintf(msg, sizeof(msg), "The card on line %d is a FR with I2 > 0 (stepped), but F2 is not a positive step.", i);
L1564:         snprintf(msg, sizeof(msg), "The card on line %d is a GW but has fewer than 2 integer inputs (tag, segments).", i);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 1573
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 1513-1578) ---
   1513:     code = deck->cards[i].card_code;
   1514: 
   1515:     // CE: comment end — should not have numeric inputs
   1516:     if (strcmp(code, "CE") == 0)
   1517:     {
   1518:       if (deck->cards[i].ints_used > 0 || deck->cards[i].flts_used > 0)
   1519:       {
   1520:         snprintf(msg, sizeof(msg), "The card on line %d is a CE but has numeric inputs.", i);
   1521:         add_error(ctx, errors, msg, 0);
   1522:       }
   1523:     }
   1524: 
   1525:     // EN: end of deck — should not have numeric inputs
   1526:     if (strcmp(code, "EN") == 0)
   1527:     {
   1528:       if (deck->cards[i].ints_used > 0 || deck->cards[i].flts_used > 0)
   1529:       {
   1530:         snprintf(msg, sizeof(msg), "The card on line %d is an EN but has numeric inputs.", i);
   1531:         add_error(ctx, errors, msg, 0);
   1532:       }
   1533:     }
   1534: 
   1535:     // FRs: allow single-frequency (I2=0) or stepped (I2>0)
   1536:     if (strcmp(code, "FR") == 0)
   1537:     {
   1538:       // there must be a value in F1
   1539:       if (deck->cards[i].f[1] == 0)
   1540:       {
   1541:         snprintf(msg, sizeof(msg), "The card on line %d is a FR but has no base frequency in F1.", i);
   1542:         add_error(ctx, errors, msg, 0);
   1543:       }
   1544:       // Single-frequency: I2==0 should have F2==0
   1545:       if (deck->cards[i].i[2] == 0 && deck->cards[i].f[2] != 0)
   1546:       {
   1547:         snprintf(msg, sizeof(msg), "The card on line %d is a FR with I2 = 0 (single frequency), but has a non-zero F2.", i);
   1548:         add_error(ctx, errors, msg, 0);
   1549:       }
   1550:       // Stepped: I2>0 requires positive step in F2
   1551:       else if (deck->cards[i].i[2] > 0 && deck->cards[i].f[2] <= 0)
   1552:       {
   1553:         snprintf(msg, sizeof(msg), "The card on line %d is a FR with I2 > 0 (stepped), but F2 is not a positive step.", i);
   1554:         add_error(ctx, errors, msg, 0);
   1555:       }
   1556:     }
   1557: 
   1558:     // GW: wire geometry — require positive segment count and radius
   1559:     if (strcmp(code, "GW") == 0)
   1560:     {
   1561:       // At least tag and segment count
   1562:       if (deck->cards[i].ints_used < 2)
   1563:       {
   1564:         snprintf(msg, sizeof(msg), "The card on line %d is a GW but has fewer than 2 integer inputs (tag, segments).", i);
   1565:         add_error(ctx, errors, msg, 0);
   1566:       }
   1567:       else
   1568:       {
   1569:         int segs = deck->cards[i].i[2];
   1570:         if (segs <= 0)
   1571:         {
   1572:           snprintf(msg, sizeof(msg), "The card on line %d is a GW with non-positive segment count.", i);
>  1573:           add_error(ctx, errors, msg, 0);
   1574:         }
   1575:       }
   1576:       // Endpoints should not be identical (zero-length wire)
   1577:       double x1 = deck->cards[i].f[1], y1 = deck->cards[i].f[2], z1 = deck->cards[i].f[3];
   1578:       double x2 = deck->cards[i].f[4], y2 = deck->cards[i].f[5], z2 = deck->cards[i].f[6];

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L1400:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has segment length %.4g m (%.4g lambda) which is >= 0.1 lambda; consider increasing segmentation.", w->line, w->tag, segLen, segFrac);
L1405:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has segment length %.4g m (%.4g lambda); in critical regions, aim for < 0.05 lambda.", w->line, w->tag, segLen, segFrac);
L1410:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has very small segment length %.4g m (%.4g lambda); this may cause excessive segmentation.", w->line, w->tag, segLen, segFrac);
L1420:             snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has radius %.4g m >= 2*len (%.4g m) with extended kernel; reduce radius or increase segmentation.", w->line, w->tag, w->radius, 2.0 * segLen);
L1428:             snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has radius %.4g m >= len/2 (%.4g m); this violates thin-wire assumptions.", w->line, w->tag, w->radius, segLen / 2.0);
L1433:             snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has radius %.4g m; typical thin-wire usage prefers radius < len/10 (%.4g m).", w->line, w->tag, w->radius, segLen / 10.0);
L1482:             snprintf(msg, sizeof(msg), "The tag number %d is found on card %d and card %d.", tag_i, i + 1, j + 1);
L1520:         snprintf(msg, sizeof(msg), "The card on line %d is a CE but has numeric inputs.", i);
L1530:         snprintf(msg, sizeof(msg), "The card on line %d is an EN but has numeric inputs.", i);
L1541:         snprintf(msg, sizeof(msg), "The card on line %d is a FR but has no base frequency in F1.", i);
L1547:         snprintf(msg, sizeof(msg), "The card on line %d is a FR with I2 = 0 (single frequency), but has a non-zero F2.", i);
L1553:         snprintf(msg, sizeof(msg), "The card on line %d is a FR with I2 > 0 (stepped), but F2 is not a positive step.", i);
L1564:         snprintf(msg, sizeof(msg), "The card on line %d is a GW but has fewer than 2 integer inputs (tag, segments).", i);
L1572:           snprintf(msg, sizeof(msg), "The card on line %d is a GW with non-positive segment count.", i);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 1582
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 1522-1587) ---
   1522:       }
   1523:     }
   1524: 
   1525:     // EN: end of deck — should not have numeric inputs
   1526:     if (strcmp(code, "EN") == 0)
   1527:     {
   1528:       if (deck->cards[i].ints_used > 0 || deck->cards[i].flts_used > 0)
   1529:       {
   1530:         snprintf(msg, sizeof(msg), "The card on line %d is an EN but has numeric inputs.", i);
   1531:         add_error(ctx, errors, msg, 0);
   1532:       }
   1533:     }
   1534: 
   1535:     // FRs: allow single-frequency (I2=0) or stepped (I2>0)
   1536:     if (strcmp(code, "FR") == 0)
   1537:     {
   1538:       // there must be a value in F1
   1539:       if (deck->cards[i].f[1] == 0)
   1540:       {
   1541:         snprintf(msg, sizeof(msg), "The card on line %d is a FR but has no base frequency in F1.", i);
   1542:         add_error(ctx, errors, msg, 0);
   1543:       }
   1544:       // Single-frequency: I2==0 should have F2==0
   1545:       if (deck->cards[i].i[2] == 0 && deck->cards[i].f[2] != 0)
   1546:       {
   1547:         snprintf(msg, sizeof(msg), "The card on line %d is a FR with I2 = 0 (single frequency), but has a non-zero F2.", i);
   1548:         add_error(ctx, errors, msg, 0);
   1549:       }
   1550:       // Stepped: I2>0 requires positive step in F2
   1551:       else if (deck->cards[i].i[2] > 0 && deck->cards[i].f[2] <= 0)
   1552:       {
   1553:         snprintf(msg, sizeof(msg), "The card on line %d is a FR with I2 > 0 (stepped), but F2 is not a positive step.", i);
   1554:         add_error(ctx, errors, msg, 0);
   1555:       }
   1556:     }
   1557: 
   1558:     // GW: wire geometry — require positive segment count and radius
   1559:     if (strcmp(code, "GW") == 0)
   1560:     {
   1561:       // At least tag and segment count
   1562:       if (deck->cards[i].ints_used < 2)
   1563:       {
   1564:         snprintf(msg, sizeof(msg), "The card on line %d is a GW but has fewer than 2 integer inputs (tag, segments).", i);
   1565:         add_error(ctx, errors, msg, 0);
   1566:       }
   1567:       else
   1568:       {
   1569:         int segs = deck->cards[i].i[2];
   1570:         if (segs <= 0)
   1571:         {
   1572:           snprintf(msg, sizeof(msg), "The card on line %d is a GW with non-positive segment count.", i);
   1573:           add_error(ctx, errors, msg, 0);
   1574:         }
   1575:       }
   1576:       // Endpoints should not be identical (zero-length wire)
   1577:       double x1 = deck->cards[i].f[1], y1 = deck->cards[i].f[2], z1 = deck->cards[i].f[3];
   1578:       double x2 = deck->cards[i].f[4], y2 = deck->cards[i].f[5], z2 = deck->cards[i].f[6];
   1579:       if (x1 == x2 && y1 == y2 && z1 == z2)
   1580:       {
   1581:         snprintf(msg, sizeof(msg), "The card on line %d is a GW with identical endpoints (zero-length wire).", i);
>  1582:         add_error(ctx, errors, msg, 0);
   1583:       }
   1584:       // Radius must be present and positive (F7).
   1585:       double gw_radius = deck->cards[i].f[7];
   1586:       if (deck->cards[i].flts_used < 7 && !deck->cards[i].flt_form_inline[7])
   1587:       {

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L1400:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has segment length %.4g m (%.4g lambda) which is >= 0.1 lambda; consider increasing segmentation.", w->line, w->tag, segLen, segFrac);
L1405:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has segment length %.4g m (%.4g lambda); in critical regions, aim for < 0.05 lambda.", w->line, w->tag, segLen, segFrac);
L1410:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has very small segment length %.4g m (%.4g lambda); this may cause excessive segmentation.", w->line, w->tag, segLen, segFrac);
L1420:             snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has radius %.4g m >= 2*len (%.4g m) with extended kernel; reduce radius or increase segmentation.", w->line, w->tag, w->radius, 2.0 * segLen);
L1428:             snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has radius %.4g m >= len/2 (%.4g m); this violates thin-wire assumptions.", w->line, w->tag, w->radius, segLen / 2.0);
L1433:             snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has radius %.4g m; typical thin-wire usage prefers radius < len/10 (%.4g m).", w->line, w->tag, w->radius, segLen / 10.0);
L1482:             snprintf(msg, sizeof(msg), "The tag number %d is found on card %d and card %d.", tag_i, i + 1, j + 1);
L1520:         snprintf(msg, sizeof(msg), "The card on line %d is a CE but has numeric inputs.", i);
L1530:         snprintf(msg, sizeof(msg), "The card on line %d is an EN but has numeric inputs.", i);
L1541:         snprintf(msg, sizeof(msg), "The card on line %d is a FR but has no base frequency in F1.", i);
L1547:         snprintf(msg, sizeof(msg), "The card on line %d is a FR with I2 = 0 (single frequency), but has a non-zero F2.", i);
L1553:         snprintf(msg, sizeof(msg), "The card on line %d is a FR with I2 > 0 (stepped), but F2 is not a positive step.", i);
L1564:         snprintf(msg, sizeof(msg), "The card on line %d is a GW but has fewer than 2 integer inputs (tag, segments).", i);
L1572:           snprintf(msg, sizeof(msg), "The card on line %d is a GW with non-positive segment count.", i);
L1581:         snprintf(msg, sizeof(msg), "The card on line %d is a GW with identical endpoints (zero-length wire).", i);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 1589
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 1529-1594) ---
   1529:       {
   1530:         snprintf(msg, sizeof(msg), "The card on line %d is an EN but has numeric inputs.", i);
   1531:         add_error(ctx, errors, msg, 0);
   1532:       }
   1533:     }
   1534: 
   1535:     // FRs: allow single-frequency (I2=0) or stepped (I2>0)
   1536:     if (strcmp(code, "FR") == 0)
   1537:     {
   1538:       // there must be a value in F1
   1539:       if (deck->cards[i].f[1] == 0)
   1540:       {
   1541:         snprintf(msg, sizeof(msg), "The card on line %d is a FR but has no base frequency in F1.", i);
   1542:         add_error(ctx, errors, msg, 0);
   1543:       }
   1544:       // Single-frequency: I2==0 should have F2==0
   1545:       if (deck->cards[i].i[2] == 0 && deck->cards[i].f[2] != 0)
   1546:       {
   1547:         snprintf(msg, sizeof(msg), "The card on line %d is a FR with I2 = 0 (single frequency), but has a non-zero F2.", i);
   1548:         add_error(ctx, errors, msg, 0);
   1549:       }
   1550:       // Stepped: I2>0 requires positive step in F2
   1551:       else if (deck->cards[i].i[2] > 0 && deck->cards[i].f[2] <= 0)
   1552:       {
   1553:         snprintf(msg, sizeof(msg), "The card on line %d is a FR with I2 > 0 (stepped), but F2 is not a positive step.", i);
   1554:         add_error(ctx, errors, msg, 0);
   1555:       }
   1556:     }
   1557: 
   1558:     // GW: wire geometry — require positive segment count and radius
   1559:     if (strcmp(code, "GW") == 0)
   1560:     {
   1561:       // At least tag and segment count
   1562:       if (deck->cards[i].ints_used < 2)
   1563:       {
   1564:         snprintf(msg, sizeof(msg), "The card on line %d is a GW but has fewer than 2 integer inputs (tag, segments).", i);
   1565:         add_error(ctx, errors, msg, 0);
   1566:       }
   1567:       else
   1568:       {
   1569:         int segs = deck->cards[i].i[2];
   1570:         if (segs <= 0)
   1571:         {
   1572:           snprintf(msg, sizeof(msg), "The card on line %d is a GW with non-positive segment count.", i);
   1573:           add_error(ctx, errors, msg, 0);
   1574:         }
   1575:       }
   1576:       // Endpoints should not be identical (zero-length wire)
   1577:       double x1 = deck->cards[i].f[1], y1 = deck->cards[i].f[2], z1 = deck->cards[i].f[3];
   1578:       double x2 = deck->cards[i].f[4], y2 = deck->cards[i].f[5], z2 = deck->cards[i].f[6];
   1579:       if (x1 == x2 && y1 == y2 && z1 == z2)
   1580:       {
   1581:         snprintf(msg, sizeof(msg), "The card on line %d is a GW with identical endpoints (zero-length wire).", i);
   1582:         add_error(ctx, errors, msg, 0);
   1583:       }
   1584:       // Radius must be present and positive (F7).
   1585:       double gw_radius = deck->cards[i].f[7];
   1586:       if (deck->cards[i].flts_used < 7 && !deck->cards[i].flt_form_inline[7])
   1587:       {
   1588:         snprintf(msg, sizeof(msg), "The card on line %d is a GW but has no radius specified in F7.", i);
>  1589:         add_error(ctx, errors, msg, 0);
   1590:       }
   1591:       else if (gw_radius <= 0.0)
   1592:       {
   1593:         snprintf(msg, sizeof(msg), "The card on line %d is a GW with non-positive radius in F7.", i);
   1594:         add_error(ctx, errors, msg, 0);

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L1400:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has segment length %.4g m (%.4g lambda) which is >= 0.1 lambda; consider increasing segmentation.", w->line, w->tag, segLen, segFrac);
L1405:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has segment length %.4g m (%.4g lambda); in critical regions, aim for < 0.05 lambda.", w->line, w->tag, segLen, segFrac);
L1410:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has very small segment length %.4g m (%.4g lambda); this may cause excessive segmentation.", w->line, w->tag, segLen, segFrac);
L1420:             snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has radius %.4g m >= 2*len (%.4g m) with extended kernel; reduce radius or increase segmentation.", w->line, w->tag, w->radius, 2.0 * segLen);
L1428:             snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has radius %.4g m >= len/2 (%.4g m); this violates thin-wire assumptions.", w->line, w->tag, w->radius, segLen / 2.0);
L1433:             snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has radius %.4g m; typical thin-wire usage prefers radius < len/10 (%.4g m).", w->line, w->tag, w->radius, segLen / 10.0);
L1482:             snprintf(msg, sizeof(msg), "The tag number %d is found on card %d and card %d.", tag_i, i + 1, j + 1);
L1520:         snprintf(msg, sizeof(msg), "The card on line %d is a CE but has numeric inputs.", i);
L1530:         snprintf(msg, sizeof(msg), "The card on line %d is an EN but has numeric inputs.", i);
L1541:         snprintf(msg, sizeof(msg), "The card on line %d is a FR but has no base frequency in F1.", i);
L1547:         snprintf(msg, sizeof(msg), "The card on line %d is a FR with I2 = 0 (single frequency), but has a non-zero F2.", i);
L1553:         snprintf(msg, sizeof(msg), "The card on line %d is a FR with I2 > 0 (stepped), but F2 is not a positive step.", i);
L1564:         snprintf(msg, sizeof(msg), "The card on line %d is a GW but has fewer than 2 integer inputs (tag, segments).", i);
L1572:           snprintf(msg, sizeof(msg), "The card on line %d is a GW with non-positive segment count.", i);
L1581:         snprintf(msg, sizeof(msg), "The card on line %d is a GW with identical endpoints (zero-length wire).", i);
L1588:         snprintf(msg, sizeof(msg), "The card on line %d is a GW but has no radius specified in F7.", i);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 1594
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 1534-1599) ---
   1534: 
   1535:     // FRs: allow single-frequency (I2=0) or stepped (I2>0)
   1536:     if (strcmp(code, "FR") == 0)
   1537:     {
   1538:       // there must be a value in F1
   1539:       if (deck->cards[i].f[1] == 0)
   1540:       {
   1541:         snprintf(msg, sizeof(msg), "The card on line %d is a FR but has no base frequency in F1.", i);
   1542:         add_error(ctx, errors, msg, 0);
   1543:       }
   1544:       // Single-frequency: I2==0 should have F2==0
   1545:       if (deck->cards[i].i[2] == 0 && deck->cards[i].f[2] != 0)
   1546:       {
   1547:         snprintf(msg, sizeof(msg), "The card on line %d is a FR with I2 = 0 (single frequency), but has a non-zero F2.", i);
   1548:         add_error(ctx, errors, msg, 0);
   1549:       }
   1550:       // Stepped: I2>0 requires positive step in F2
   1551:       else if (deck->cards[i].i[2] > 0 && deck->cards[i].f[2] <= 0)
   1552:       {
   1553:         snprintf(msg, sizeof(msg), "The card on line %d is a FR with I2 > 0 (stepped), but F2 is not a positive step.", i);
   1554:         add_error(ctx, errors, msg, 0);
   1555:       }
   1556:     }
   1557: 
   1558:     // GW: wire geometry — require positive segment count and radius
   1559:     if (strcmp(code, "GW") == 0)
   1560:     {
   1561:       // At least tag and segment count
   1562:       if (deck->cards[i].ints_used < 2)
   1563:       {
   1564:         snprintf(msg, sizeof(msg), "The card on line %d is a GW but has fewer than 2 integer inputs (tag, segments).", i);
   1565:         add_error(ctx, errors, msg, 0);
   1566:       }
   1567:       else
   1568:       {
   1569:         int segs = deck->cards[i].i[2];
   1570:         if (segs <= 0)
   1571:         {
   1572:           snprintf(msg, sizeof(msg), "The card on line %d is a GW with non-positive segment count.", i);
   1573:           add_error(ctx, errors, msg, 0);
   1574:         }
   1575:       }
   1576:       // Endpoints should not be identical (zero-length wire)
   1577:       double x1 = deck->cards[i].f[1], y1 = deck->cards[i].f[2], z1 = deck->cards[i].f[3];
   1578:       double x2 = deck->cards[i].f[4], y2 = deck->cards[i].f[5], z2 = deck->cards[i].f[6];
   1579:       if (x1 == x2 && y1 == y2 && z1 == z2)
   1580:       {
   1581:         snprintf(msg, sizeof(msg), "The card on line %d is a GW with identical endpoints (zero-length wire).", i);
   1582:         add_error(ctx, errors, msg, 0);
   1583:       }
   1584:       // Radius must be present and positive (F7).
   1585:       double gw_radius = deck->cards[i].f[7];
   1586:       if (deck->cards[i].flts_used < 7 && !deck->cards[i].flt_form_inline[7])
   1587:       {
   1588:         snprintf(msg, sizeof(msg), "The card on line %d is a GW but has no radius specified in F7.", i);
   1589:         add_error(ctx, errors, msg, 0);
   1590:       }
   1591:       else if (gw_radius <= 0.0)
   1592:       {
   1593:         snprintf(msg, sizeof(msg), "The card on line %d is a GW with non-positive radius in F7.", i);
>  1594:         add_error(ctx, errors, msg, 0);
   1595:       }
   1596:       // If radius is zero (and not a formula), next card should be a GC with tapering info
   1597:       if ((deck->cards[i].flts_used >= 7 || deck->cards[i].flt_form_inline[7]) && gw_radius == 0.0)
   1598:       {
   1599:         if (i + 1 < deck->num_cards)

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L1400:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has segment length %.4g m (%.4g lambda) which is >= 0.1 lambda; consider increasing segmentation.", w->line, w->tag, segLen, segFrac);
L1405:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has segment length %.4g m (%.4g lambda); in critical regions, aim for < 0.05 lambda.", w->line, w->tag, segLen, segFrac);
L1410:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has very small segment length %.4g m (%.4g lambda); this may cause excessive segmentation.", w->line, w->tag, segLen, segFrac);
L1420:             snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has radius %.4g m >= 2*len (%.4g m) with extended kernel; reduce radius or increase segmentation.", w->line, w->tag, w->radius, 2.0 * segLen);
L1428:             snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has radius %.4g m >= len/2 (%.4g m); this violates thin-wire assumptions.", w->line, w->tag, w->radius, segLen / 2.0);
L1433:             snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has radius %.4g m; typical thin-wire usage prefers radius < len/10 (%.4g m).", w->line, w->tag, w->radius, segLen / 10.0);
L1482:             snprintf(msg, sizeof(msg), "The tag number %d is found on card %d and card %d.", tag_i, i + 1, j + 1);
L1520:         snprintf(msg, sizeof(msg), "The card on line %d is a CE but has numeric inputs.", i);
L1530:         snprintf(msg, sizeof(msg), "The card on line %d is an EN but has numeric inputs.", i);
L1541:         snprintf(msg, sizeof(msg), "The card on line %d is a FR but has no base frequency in F1.", i);
L1547:         snprintf(msg, sizeof(msg), "The card on line %d is a FR with I2 = 0 (single frequency), but has a non-zero F2.", i);
L1553:         snprintf(msg, sizeof(msg), "The card on line %d is a FR with I2 > 0 (stepped), but F2 is not a positive step.", i);
L1564:         snprintf(msg, sizeof(msg), "The card on line %d is a GW but has fewer than 2 integer inputs (tag, segments).", i);
L1572:           snprintf(msg, sizeof(msg), "The card on line %d is a GW with non-positive segment count.", i);
L1581:         snprintf(msg, sizeof(msg), "The card on line %d is a GW with identical endpoints (zero-length wire).", i);
L1588:         snprintf(msg, sizeof(msg), "The card on line %d is a GW but has no radius specified in F7.", i);
L1593:         snprintf(msg, sizeof(msg), "The card on line %d is a GW with non-positive radius in F7.", i);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 1604
CALL: add_error(ctx, errors, msg, 1);
--- Context (lines 1544-1609) ---
   1544:       // Single-frequency: I2==0 should have F2==0
   1545:       if (deck->cards[i].i[2] == 0 && deck->cards[i].f[2] != 0)
   1546:       {
   1547:         snprintf(msg, sizeof(msg), "The card on line %d is a FR with I2 = 0 (single frequency), but has a non-zero F2.", i);
   1548:         add_error(ctx, errors, msg, 0);
   1549:       }
   1550:       // Stepped: I2>0 requires positive step in F2
   1551:       else if (deck->cards[i].i[2] > 0 && deck->cards[i].f[2] <= 0)
   1552:       {
   1553:         snprintf(msg, sizeof(msg), "The card on line %d is a FR with I2 > 0 (stepped), but F2 is not a positive step.", i);
   1554:         add_error(ctx, errors, msg, 0);
   1555:       }
   1556:     }
   1557: 
   1558:     // GW: wire geometry — require positive segment count and radius
   1559:     if (strcmp(code, "GW") == 0)
   1560:     {
   1561:       // At least tag and segment count
   1562:       if (deck->cards[i].ints_used < 2)
   1563:       {
   1564:         snprintf(msg, sizeof(msg), "The card on line %d is a GW but has fewer than 2 integer inputs (tag, segments).", i);
   1565:         add_error(ctx, errors, msg, 0);
   1566:       }
   1567:       else
   1568:       {
   1569:         int segs = deck->cards[i].i[2];
   1570:         if (segs <= 0)
   1571:         {
   1572:           snprintf(msg, sizeof(msg), "The card on line %d is a GW with non-positive segment count.", i);
   1573:           add_error(ctx, errors, msg, 0);
   1574:         }
   1575:       }
   1576:       // Endpoints should not be identical (zero-length wire)
   1577:       double x1 = deck->cards[i].f[1], y1 = deck->cards[i].f[2], z1 = deck->cards[i].f[3];
   1578:       double x2 = deck->cards[i].f[4], y2 = deck->cards[i].f[5], z2 = deck->cards[i].f[6];
   1579:       if (x1 == x2 && y1 == y2 && z1 == z2)
   1580:       {
   1581:         snprintf(msg, sizeof(msg), "The card on line %d is a GW with identical endpoints (zero-length wire).", i);
   1582:         add_error(ctx, errors, msg, 0);
   1583:       }
   1584:       // Radius must be present and positive (F7).
   1585:       double gw_radius = deck->cards[i].f[7];
   1586:       if (deck->cards[i].flts_used < 7 && !deck->cards[i].flt_form_inline[7])
   1587:       {
   1588:         snprintf(msg, sizeof(msg), "The card on line %d is a GW but has no radius specified in F7.", i);
   1589:         add_error(ctx, errors, msg, 0);
   1590:       }
   1591:       else if (gw_radius <= 0.0)
   1592:       {
   1593:         snprintf(msg, sizeof(msg), "The card on line %d is a GW with non-positive radius in F7.", i);
   1594:         add_error(ctx, errors, msg, 0);
   1595:       }
   1596:       // If radius is zero (and not a formula), next card should be a GC with tapering info
   1597:       if ((deck->cards[i].flts_used >= 7 || deck->cards[i].flt_form_inline[7]) && gw_radius == 0.0)
   1598:       {
   1599:         if (i + 1 < deck->num_cards)
   1600:         {
   1601:           if (strcmp(deck->cards[i + 1].card_code, "GC") != 0)
   1602:           {
   1603:             snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius, but the next card is not a GC.", i + 1);
>  1604:             add_error(ctx, errors, msg, 1);
   1605:           }
   1606:         }
   1607:         else
   1608:         {
   1609:           snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius and is the last card; a following GC is required.", i + 1);

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L1405:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has segment length %.4g m (%.4g lambda); in critical regions, aim for < 0.05 lambda.", w->line, w->tag, segLen, segFrac);
L1410:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has very small segment length %.4g m (%.4g lambda); this may cause excessive segmentation.", w->line, w->tag, segLen, segFrac);
L1420:             snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has radius %.4g m >= 2*len (%.4g m) with extended kernel; reduce radius or increase segmentation.", w->line, w->tag, w->radius, 2.0 * segLen);
L1428:             snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has radius %.4g m >= len/2 (%.4g m); this violates thin-wire assumptions.", w->line, w->tag, w->radius, segLen / 2.0);
L1433:             snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has radius %.4g m; typical thin-wire usage prefers radius < len/10 (%.4g m).", w->line, w->tag, w->radius, segLen / 10.0);
L1482:             snprintf(msg, sizeof(msg), "The tag number %d is found on card %d and card %d.", tag_i, i + 1, j + 1);
L1520:         snprintf(msg, sizeof(msg), "The card on line %d is a CE but has numeric inputs.", i);
L1530:         snprintf(msg, sizeof(msg), "The card on line %d is an EN but has numeric inputs.", i);
L1541:         snprintf(msg, sizeof(msg), "The card on line %d is a FR but has no base frequency in F1.", i);
L1547:         snprintf(msg, sizeof(msg), "The card on line %d is a FR with I2 = 0 (single frequency), but has a non-zero F2.", i);
L1553:         snprintf(msg, sizeof(msg), "The card on line %d is a FR with I2 > 0 (stepped), but F2 is not a positive step.", i);
L1564:         snprintf(msg, sizeof(msg), "The card on line %d is a GW but has fewer than 2 integer inputs (tag, segments).", i);
L1572:           snprintf(msg, sizeof(msg), "The card on line %d is a GW with non-positive segment count.", i);
L1581:         snprintf(msg, sizeof(msg), "The card on line %d is a GW with identical endpoints (zero-length wire).", i);
L1588:         snprintf(msg, sizeof(msg), "The card on line %d is a GW but has no radius specified in F7.", i);
L1593:         snprintf(msg, sizeof(msg), "The card on line %d is a GW with non-positive radius in F7.", i);
L1603:             snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius, but the next card is not a GC.", i + 1);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 1610
CALL: add_error(ctx, errors, msg, 1);
--- Context (lines 1550-1615) ---
   1550:       // Stepped: I2>0 requires positive step in F2
   1551:       else if (deck->cards[i].i[2] > 0 && deck->cards[i].f[2] <= 0)
   1552:       {
   1553:         snprintf(msg, sizeof(msg), "The card on line %d is a FR with I2 > 0 (stepped), but F2 is not a positive step.", i);
   1554:         add_error(ctx, errors, msg, 0);
   1555:       }
   1556:     }
   1557: 
   1558:     // GW: wire geometry — require positive segment count and radius
   1559:     if (strcmp(code, "GW") == 0)
   1560:     {
   1561:       // At least tag and segment count
   1562:       if (deck->cards[i].ints_used < 2)
   1563:       {
   1564:         snprintf(msg, sizeof(msg), "The card on line %d is a GW but has fewer than 2 integer inputs (tag, segments).", i);
   1565:         add_error(ctx, errors, msg, 0);
   1566:       }
   1567:       else
   1568:       {
   1569:         int segs = deck->cards[i].i[2];
   1570:         if (segs <= 0)
   1571:         {
   1572:           snprintf(msg, sizeof(msg), "The card on line %d is a GW with non-positive segment count.", i);
   1573:           add_error(ctx, errors, msg, 0);
   1574:         }
   1575:       }
   1576:       // Endpoints should not be identical (zero-length wire)
   1577:       double x1 = deck->cards[i].f[1], y1 = deck->cards[i].f[2], z1 = deck->cards[i].f[3];
   1578:       double x2 = deck->cards[i].f[4], y2 = deck->cards[i].f[5], z2 = deck->cards[i].f[6];
   1579:       if (x1 == x2 && y1 == y2 && z1 == z2)
   1580:       {
   1581:         snprintf(msg, sizeof(msg), "The card on line %d is a GW with identical endpoints (zero-length wire).", i);
   1582:         add_error(ctx, errors, msg, 0);
   1583:       }
   1584:       // Radius must be present and positive (F7).
   1585:       double gw_radius = deck->cards[i].f[7];
   1586:       if (deck->cards[i].flts_used < 7 && !deck->cards[i].flt_form_inline[7])
   1587:       {
   1588:         snprintf(msg, sizeof(msg), "The card on line %d is a GW but has no radius specified in F7.", i);
   1589:         add_error(ctx, errors, msg, 0);
   1590:       }
   1591:       else if (gw_radius <= 0.0)
   1592:       {
   1593:         snprintf(msg, sizeof(msg), "The card on line %d is a GW with non-positive radius in F7.", i);
   1594:         add_error(ctx, errors, msg, 0);
   1595:       }
   1596:       // If radius is zero (and not a formula), next card should be a GC with tapering info
   1597:       if ((deck->cards[i].flts_used >= 7 || deck->cards[i].flt_form_inline[7]) && gw_radius == 0.0)
   1598:       {
   1599:         if (i + 1 < deck->num_cards)
   1600:         {
   1601:           if (strcmp(deck->cards[i + 1].card_code, "GC") != 0)
   1602:           {
   1603:             snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius, but the next card is not a GC.", i + 1);
   1604:             add_error(ctx, errors, msg, 1);
   1605:           }
   1606:         }
   1607:         else
   1608:         {
   1609:           snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius and is the last card; a following GC is required.", i + 1);
>  1610:           add_error(ctx, errors, msg, 1);
   1611:         }
   1612:       }
   1613:     }
   1614: 
   1615:     // RP: radiation pattern — counts, steps, and basic range sanity

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L1410:         snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has very small segment length %.4g m (%.4g lambda); this may cause excessive segmentation.", w->line, w->tag, segLen, segFrac);
L1420:             snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has radius %.4g m >= 2*len (%.4g m) with extended kernel; reduce radius or increase segmentation.", w->line, w->tag, w->radius, 2.0 * segLen);
L1428:             snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has radius %.4g m >= len/2 (%.4g m); this violates thin-wire assumptions.", w->line, w->tag, w->radius, segLen / 2.0);
L1433:             snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has radius %.4g m; typical thin-wire usage prefers radius < len/10 (%.4g m).", w->line, w->tag, w->radius, segLen / 10.0);
L1482:             snprintf(msg, sizeof(msg), "The tag number %d is found on card %d and card %d.", tag_i, i + 1, j + 1);
L1520:         snprintf(msg, sizeof(msg), "The card on line %d is a CE but has numeric inputs.", i);
L1530:         snprintf(msg, sizeof(msg), "The card on line %d is an EN but has numeric inputs.", i);
L1541:         snprintf(msg, sizeof(msg), "The card on line %d is a FR but has no base frequency in F1.", i);
L1547:         snprintf(msg, sizeof(msg), "The card on line %d is a FR with I2 = 0 (single frequency), but has a non-zero F2.", i);
L1553:         snprintf(msg, sizeof(msg), "The card on line %d is a FR with I2 > 0 (stepped), but F2 is not a positive step.", i);
L1564:         snprintf(msg, sizeof(msg), "The card on line %d is a GW but has fewer than 2 integer inputs (tag, segments).", i);
L1572:           snprintf(msg, sizeof(msg), "The card on line %d is a GW with non-positive segment count.", i);
L1581:         snprintf(msg, sizeof(msg), "The card on line %d is a GW with identical endpoints (zero-length wire).", i);
L1588:         snprintf(msg, sizeof(msg), "The card on line %d is a GW but has no radius specified in F7.", i);
L1593:         snprintf(msg, sizeof(msg), "The card on line %d is a GW with non-positive radius in F7.", i);
L1603:             snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius, but the next card is not a GC.", i + 1);
L1609:           snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius and is the last card; a following GC is required.", i + 1);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 1622
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 1562-1627) ---
   1562:       if (deck->cards[i].ints_used < 2)
   1563:       {
   1564:         snprintf(msg, sizeof(msg), "The card on line %d is a GW but has fewer than 2 integer inputs (tag, segments).", i);
   1565:         add_error(ctx, errors, msg, 0);
   1566:       }
   1567:       else
   1568:       {
   1569:         int segs = deck->cards[i].i[2];
   1570:         if (segs <= 0)
   1571:         {
   1572:           snprintf(msg, sizeof(msg), "The card on line %d is a GW with non-positive segment count.", i);
   1573:           add_error(ctx, errors, msg, 0);
   1574:         }
   1575:       }
   1576:       // Endpoints should not be identical (zero-length wire)
   1577:       double x1 = deck->cards[i].f[1], y1 = deck->cards[i].f[2], z1 = deck->cards[i].f[3];
   1578:       double x2 = deck->cards[i].f[4], y2 = deck->cards[i].f[5], z2 = deck->cards[i].f[6];
   1579:       if (x1 == x2 && y1 == y2 && z1 == z2)
   1580:       {
   1581:         snprintf(msg, sizeof(msg), "The card on line %d is a GW with identical endpoints (zero-length wire).", i);
   1582:         add_error(ctx, errors, msg, 0);
   1583:       }
   1584:       // Radius must be present and positive (F7).
   1585:       double gw_radius = deck->cards[i].f[7];
   1586:       if (deck->cards[i].flts_used < 7 && !deck->cards[i].flt_form_inline[7])
   1587:       {
   1588:         snprintf(msg, sizeof(msg), "The card on line %d is a GW but has no radius specified in F7.", i);
   1589:         add_error(ctx, errors, msg, 0);
   1590:       }
   1591:       else if (gw_radius <= 0.0)
   1592:       {
   1593:         snprintf(msg, sizeof(msg), "The card on line %d is a GW with non-positive radius in F7.", i);
   1594:         add_error(ctx, errors, msg, 0);
   1595:       }
   1596:       // If radius is zero (and not a formula), next card should be a GC with tapering info
   1597:       if ((deck->cards[i].flts_used >= 7 || deck->cards[i].flt_form_inline[7]) && gw_radius == 0.0)
   1598:       {
   1599:         if (i + 1 < deck->num_cards)
   1600:         {
   1601:           if (strcmp(deck->cards[i + 1].card_code, "GC") != 0)
   1602:           {
   1603:             snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius, but the next card is not a GC.", i + 1);
   1604:             add_error(ctx, errors, msg, 1);
   1605:           }
   1606:         }
   1607:         else
   1608:         {
   1609:           snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius and is the last card; a following GC is required.", i + 1);
   1610:           add_error(ctx, errors, msg, 1);
   1611:         }
   1612:       }
   1613:     }
   1614: 
   1615:     // RP: radiation pattern — counts, steps, and basic range sanity
   1616:     if (strcmp(code, "RP") == 0)
   1617:     {
   1618:       // Typical RP uses at least 4 integers and 4 floats
   1619:       if (deck->cards[i].ints_used < 4)
   1620:       {
   1621:         snprintf(msg, sizeof(msg), "The card on line %d is an RP but has fewer than 4 integer inputs.", i);
>  1622:         add_error(ctx, errors, msg, 0);
   1623:       }
   1624:       if (deck->cards[i].flts_used < 4)
   1625:       {
   1626:         snprintf(msg, sizeof(msg), "The card on line %d is an RP but has fewer than 4 floating-point inputs.", i);
   1627:         add_error(ctx, errors, msg, 0);

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L1428:             snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has radius %.4g m >= len/2 (%.4g m); this violates thin-wire assumptions.", w->line, w->tag, w->radius, segLen / 2.0);
L1433:             snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has radius %.4g m; typical thin-wire usage prefers radius < len/10 (%.4g m).", w->line, w->tag, w->radius, segLen / 10.0);
L1482:             snprintf(msg, sizeof(msg), "The tag number %d is found on card %d and card %d.", tag_i, i + 1, j + 1);
L1520:         snprintf(msg, sizeof(msg), "The card on line %d is a CE but has numeric inputs.", i);
L1530:         snprintf(msg, sizeof(msg), "The card on line %d is an EN but has numeric inputs.", i);
L1541:         snprintf(msg, sizeof(msg), "The card on line %d is a FR but has no base frequency in F1.", i);
L1547:         snprintf(msg, sizeof(msg), "The card on line %d is a FR with I2 = 0 (single frequency), but has a non-zero F2.", i);
L1553:         snprintf(msg, sizeof(msg), "The card on line %d is a FR with I2 > 0 (stepped), but F2 is not a positive step.", i);
L1564:         snprintf(msg, sizeof(msg), "The card on line %d is a GW but has fewer than 2 integer inputs (tag, segments).", i);
L1572:           snprintf(msg, sizeof(msg), "The card on line %d is a GW with non-positive segment count.", i);
L1581:         snprintf(msg, sizeof(msg), "The card on line %d is a GW with identical endpoints (zero-length wire).", i);
L1588:         snprintf(msg, sizeof(msg), "The card on line %d is a GW but has no radius specified in F7.", i);
L1593:         snprintf(msg, sizeof(msg), "The card on line %d is a GW with non-positive radius in F7.", i);
L1603:             snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius, but the next card is not a GC.", i + 1);
L1609:           snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius and is the last card; a following GC is required.", i + 1);
L1621:         snprintf(msg, sizeof(msg), "The card on line %d is an RP but has fewer than 4 integer inputs.", i);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 1627
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 1567-1632) ---
   1567:       else
   1568:       {
   1569:         int segs = deck->cards[i].i[2];
   1570:         if (segs <= 0)
   1571:         {
   1572:           snprintf(msg, sizeof(msg), "The card on line %d is a GW with non-positive segment count.", i);
   1573:           add_error(ctx, errors, msg, 0);
   1574:         }
   1575:       }
   1576:       // Endpoints should not be identical (zero-length wire)
   1577:       double x1 = deck->cards[i].f[1], y1 = deck->cards[i].f[2], z1 = deck->cards[i].f[3];
   1578:       double x2 = deck->cards[i].f[4], y2 = deck->cards[i].f[5], z2 = deck->cards[i].f[6];
   1579:       if (x1 == x2 && y1 == y2 && z1 == z2)
   1580:       {
   1581:         snprintf(msg, sizeof(msg), "The card on line %d is a GW with identical endpoints (zero-length wire).", i);
   1582:         add_error(ctx, errors, msg, 0);
   1583:       }
   1584:       // Radius must be present and positive (F7).
   1585:       double gw_radius = deck->cards[i].f[7];
   1586:       if (deck->cards[i].flts_used < 7 && !deck->cards[i].flt_form_inline[7])
   1587:       {
   1588:         snprintf(msg, sizeof(msg), "The card on line %d is a GW but has no radius specified in F7.", i);
   1589:         add_error(ctx, errors, msg, 0);
   1590:       }
   1591:       else if (gw_radius <= 0.0)
   1592:       {
   1593:         snprintf(msg, sizeof(msg), "The card on line %d is a GW with non-positive radius in F7.", i);
   1594:         add_error(ctx, errors, msg, 0);
   1595:       }
   1596:       // If radius is zero (and not a formula), next card should be a GC with tapering info
   1597:       if ((deck->cards[i].flts_used >= 7 || deck->cards[i].flt_form_inline[7]) && gw_radius == 0.0)
   1598:       {
   1599:         if (i + 1 < deck->num_cards)
   1600:         {
   1601:           if (strcmp(deck->cards[i + 1].card_code, "GC") != 0)
   1602:           {
   1603:             snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius, but the next card is not a GC.", i + 1);
   1604:             add_error(ctx, errors, msg, 1);
   1605:           }
   1606:         }
   1607:         else
   1608:         {
   1609:           snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius and is the last card; a following GC is required.", i + 1);
   1610:           add_error(ctx, errors, msg, 1);
   1611:         }
   1612:       }
   1613:     }
   1614: 
   1615:     // RP: radiation pattern — counts, steps, and basic range sanity
   1616:     if (strcmp(code, "RP") == 0)
   1617:     {
   1618:       // Typical RP uses at least 4 integers and 4 floats
   1619:       if (deck->cards[i].ints_used < 4)
   1620:       {
   1621:         snprintf(msg, sizeof(msg), "The card on line %d is an RP but has fewer than 4 integer inputs.", i);
   1622:         add_error(ctx, errors, msg, 0);
   1623:       }
   1624:       if (deck->cards[i].flts_used < 4)
   1625:       {
   1626:         snprintf(msg, sizeof(msg), "The card on line %d is an RP but has fewer than 4 floating-point inputs.", i);
>  1627:         add_error(ctx, errors, msg, 0);
   1628:       }
   1629:       // Number of theta/phi points should be positive
   1630:       int ntheta = deck->cards[i].i[2];
   1631:       int nphi = deck->cards[i].i[3];
   1632:       if (ntheta <= 0)

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L1428:             snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has radius %.4g m >= len/2 (%.4g m); this violates thin-wire assumptions.", w->line, w->tag, w->radius, segLen / 2.0);
L1433:             snprintf(msg, sizeof(msg), "The wire on line %d (tag %d) has radius %.4g m; typical thin-wire usage prefers radius < len/10 (%.4g m).", w->line, w->tag, w->radius, segLen / 10.0);
L1482:             snprintf(msg, sizeof(msg), "The tag number %d is found on card %d and card %d.", tag_i, i + 1, j + 1);
L1520:         snprintf(msg, sizeof(msg), "The card on line %d is a CE but has numeric inputs.", i);
L1530:         snprintf(msg, sizeof(msg), "The card on line %d is an EN but has numeric inputs.", i);
L1541:         snprintf(msg, sizeof(msg), "The card on line %d is a FR but has no base frequency in F1.", i);
L1547:         snprintf(msg, sizeof(msg), "The card on line %d is a FR with I2 = 0 (single frequency), but has a non-zero F2.", i);
L1553:         snprintf(msg, sizeof(msg), "The card on line %d is a FR with I2 > 0 (stepped), but F2 is not a positive step.", i);
L1564:         snprintf(msg, sizeof(msg), "The card on line %d is a GW but has fewer than 2 integer inputs (tag, segments).", i);
L1572:           snprintf(msg, sizeof(msg), "The card on line %d is a GW with non-positive segment count.", i);
L1581:         snprintf(msg, sizeof(msg), "The card on line %d is a GW with identical endpoints (zero-length wire).", i);
L1588:         snprintf(msg, sizeof(msg), "The card on line %d is a GW but has no radius specified in F7.", i);
L1593:         snprintf(msg, sizeof(msg), "The card on line %d is a GW with non-positive radius in F7.", i);
L1603:             snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius, but the next card is not a GC.", i + 1);
L1609:           snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius and is the last card; a following GC is required.", i + 1);
L1621:         snprintf(msg, sizeof(msg), "The card on line %d is an RP but has fewer than 4 integer inputs.", i);
L1626:         snprintf(msg, sizeof(msg), "The card on line %d is an RP but has fewer than 4 floating-point inputs.", i);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 1635
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 1575-1640) ---
   1575:       }
   1576:       // Endpoints should not be identical (zero-length wire)
   1577:       double x1 = deck->cards[i].f[1], y1 = deck->cards[i].f[2], z1 = deck->cards[i].f[3];
   1578:       double x2 = deck->cards[i].f[4], y2 = deck->cards[i].f[5], z2 = deck->cards[i].f[6];
   1579:       if (x1 == x2 && y1 == y2 && z1 == z2)
   1580:       {
   1581:         snprintf(msg, sizeof(msg), "The card on line %d is a GW with identical endpoints (zero-length wire).", i);
   1582:         add_error(ctx, errors, msg, 0);
   1583:       }
   1584:       // Radius must be present and positive (F7).
   1585:       double gw_radius = deck->cards[i].f[7];
   1586:       if (deck->cards[i].flts_used < 7 && !deck->cards[i].flt_form_inline[7])
   1587:       {
   1588:         snprintf(msg, sizeof(msg), "The card on line %d is a GW but has no radius specified in F7.", i);
   1589:         add_error(ctx, errors, msg, 0);
   1590:       }
   1591:       else if (gw_radius <= 0.0)
   1592:       {
   1593:         snprintf(msg, sizeof(msg), "The card on line %d is a GW with non-positive radius in F7.", i);
   1594:         add_error(ctx, errors, msg, 0);
   1595:       }
   1596:       // If radius is zero (and not a formula), next card should be a GC with tapering info
   1597:       if ((deck->cards[i].flts_used >= 7 || deck->cards[i].flt_form_inline[7]) && gw_radius == 0.0)
   1598:       {
   1599:         if (i + 1 < deck->num_cards)
   1600:         {
   1601:           if (strcmp(deck->cards[i + 1].card_code, "GC") != 0)
   1602:           {
   1603:             snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius, but the next card is not a GC.", i + 1);
   1604:             add_error(ctx, errors, msg, 1);
   1605:           }
   1606:         }
   1607:         else
   1608:         {
   1609:           snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius and is the last card; a following GC is required.", i + 1);
   1610:           add_error(ctx, errors, msg, 1);
   1611:         }
   1612:       }
   1613:     }
   1614: 
   1615:     // RP: radiation pattern — counts, steps, and basic range sanity
   1616:     if (strcmp(code, "RP") == 0)
   1617:     {
   1618:       // Typical RP uses at least 4 integers and 4 floats
   1619:       if (deck->cards[i].ints_used < 4)
   1620:       {
   1621:         snprintf(msg, sizeof(msg), "The card on line %d is an RP but has fewer than 4 integer inputs.", i);
   1622:         add_error(ctx, errors, msg, 0);
   1623:       }
   1624:       if (deck->cards[i].flts_used < 4)
   1625:       {
   1626:         snprintf(msg, sizeof(msg), "The card on line %d is an RP but has fewer than 4 floating-point inputs.", i);
   1627:         add_error(ctx, errors, msg, 0);
   1628:       }
   1629:       // Number of theta/phi points should be positive
   1630:       int ntheta = deck->cards[i].i[2];
   1631:       int nphi = deck->cards[i].i[3];
   1632:       if (ntheta <= 0)
   1633:       {
   1634:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with non-positive NTHETA (I2).", i);
>  1635:         add_error(ctx, errors, msg, 0);
   1636:       }
   1637:       if (nphi <= 0)
   1638:       {
   1639:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with non-positive NPHI (I3).", i);
   1640:         add_error(ctx, errors, msg, 0);

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L1482:             snprintf(msg, sizeof(msg), "The tag number %d is found on card %d and card %d.", tag_i, i + 1, j + 1);
L1520:         snprintf(msg, sizeof(msg), "The card on line %d is a CE but has numeric inputs.", i);
L1530:         snprintf(msg, sizeof(msg), "The card on line %d is an EN but has numeric inputs.", i);
L1541:         snprintf(msg, sizeof(msg), "The card on line %d is a FR but has no base frequency in F1.", i);
L1547:         snprintf(msg, sizeof(msg), "The card on line %d is a FR with I2 = 0 (single frequency), but has a non-zero F2.", i);
L1553:         snprintf(msg, sizeof(msg), "The card on line %d is a FR with I2 > 0 (stepped), but F2 is not a positive step.", i);
L1564:         snprintf(msg, sizeof(msg), "The card on line %d is a GW but has fewer than 2 integer inputs (tag, segments).", i);
L1572:           snprintf(msg, sizeof(msg), "The card on line %d is a GW with non-positive segment count.", i);
L1581:         snprintf(msg, sizeof(msg), "The card on line %d is a GW with identical endpoints (zero-length wire).", i);
L1588:         snprintf(msg, sizeof(msg), "The card on line %d is a GW but has no radius specified in F7.", i);
L1593:         snprintf(msg, sizeof(msg), "The card on line %d is a GW with non-positive radius in F7.", i);
L1603:             snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius, but the next card is not a GC.", i + 1);
L1609:           snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius and is the last card; a following GC is required.", i + 1);
L1621:         snprintf(msg, sizeof(msg), "The card on line %d is an RP but has fewer than 4 integer inputs.", i);
L1626:         snprintf(msg, sizeof(msg), "The card on line %d is an RP but has fewer than 4 floating-point inputs.", i);
L1634:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with non-positive NTHETA (I2).", i);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 1640
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 1580-1645) ---
   1580:       {
   1581:         snprintf(msg, sizeof(msg), "The card on line %d is a GW with identical endpoints (zero-length wire).", i);
   1582:         add_error(ctx, errors, msg, 0);
   1583:       }
   1584:       // Radius must be present and positive (F7).
   1585:       double gw_radius = deck->cards[i].f[7];
   1586:       if (deck->cards[i].flts_used < 7 && !deck->cards[i].flt_form_inline[7])
   1587:       {
   1588:         snprintf(msg, sizeof(msg), "The card on line %d is a GW but has no radius specified in F7.", i);
   1589:         add_error(ctx, errors, msg, 0);
   1590:       }
   1591:       else if (gw_radius <= 0.0)
   1592:       {
   1593:         snprintf(msg, sizeof(msg), "The card on line %d is a GW with non-positive radius in F7.", i);
   1594:         add_error(ctx, errors, msg, 0);
   1595:       }
   1596:       // If radius is zero (and not a formula), next card should be a GC with tapering info
   1597:       if ((deck->cards[i].flts_used >= 7 || deck->cards[i].flt_form_inline[7]) && gw_radius == 0.0)
   1598:       {
   1599:         if (i + 1 < deck->num_cards)
   1600:         {
   1601:           if (strcmp(deck->cards[i + 1].card_code, "GC") != 0)
   1602:           {
   1603:             snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius, but the next card is not a GC.", i + 1);
   1604:             add_error(ctx, errors, msg, 1);
   1605:           }
   1606:         }
   1607:         else
   1608:         {
   1609:           snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius and is the last card; a following GC is required.", i + 1);
   1610:           add_error(ctx, errors, msg, 1);
   1611:         }
   1612:       }
   1613:     }
   1614: 
   1615:     // RP: radiation pattern — counts, steps, and basic range sanity
   1616:     if (strcmp(code, "RP") == 0)
   1617:     {
   1618:       // Typical RP uses at least 4 integers and 4 floats
   1619:       if (deck->cards[i].ints_used < 4)
   1620:       {
   1621:         snprintf(msg, sizeof(msg), "The card on line %d is an RP but has fewer than 4 integer inputs.", i);
   1622:         add_error(ctx, errors, msg, 0);
   1623:       }
   1624:       if (deck->cards[i].flts_used < 4)
   1625:       {
   1626:         snprintf(msg, sizeof(msg), "The card on line %d is an RP but has fewer than 4 floating-point inputs.", i);
   1627:         add_error(ctx, errors, msg, 0);
   1628:       }
   1629:       // Number of theta/phi points should be positive
   1630:       int ntheta = deck->cards[i].i[2];
   1631:       int nphi = deck->cards[i].i[3];
   1632:       if (ntheta <= 0)
   1633:       {
   1634:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with non-positive NTHETA (I2).", i);
   1635:         add_error(ctx, errors, msg, 0);
   1636:       }
   1637:       if (nphi <= 0)
   1638:       {
   1639:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with non-positive NPHI (I3).", i);
>  1640:         add_error(ctx, errors, msg, 0);
   1641:       }
   1642:       // Steps must be non-zero when requesting multiple points
   1643:       double th_start = deck->cards[i].f[1];
   1644:       double ph_start = deck->cards[i].f[2];
   1645:       double th_step = deck->cards[i].f[3];

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L1482:             snprintf(msg, sizeof(msg), "The tag number %d is found on card %d and card %d.", tag_i, i + 1, j + 1);
L1520:         snprintf(msg, sizeof(msg), "The card on line %d is a CE but has numeric inputs.", i);
L1530:         snprintf(msg, sizeof(msg), "The card on line %d is an EN but has numeric inputs.", i);
L1541:         snprintf(msg, sizeof(msg), "The card on line %d is a FR but has no base frequency in F1.", i);
L1547:         snprintf(msg, sizeof(msg), "The card on line %d is a FR with I2 = 0 (single frequency), but has a non-zero F2.", i);
L1553:         snprintf(msg, sizeof(msg), "The card on line %d is a FR with I2 > 0 (stepped), but F2 is not a positive step.", i);
L1564:         snprintf(msg, sizeof(msg), "The card on line %d is a GW but has fewer than 2 integer inputs (tag, segments).", i);
L1572:           snprintf(msg, sizeof(msg), "The card on line %d is a GW with non-positive segment count.", i);
L1581:         snprintf(msg, sizeof(msg), "The card on line %d is a GW with identical endpoints (zero-length wire).", i);
L1588:         snprintf(msg, sizeof(msg), "The card on line %d is a GW but has no radius specified in F7.", i);
L1593:         snprintf(msg, sizeof(msg), "The card on line %d is a GW with non-positive radius in F7.", i);
L1603:             snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius, but the next card is not a GC.", i + 1);
L1609:           snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius and is the last card; a following GC is required.", i + 1);
L1621:         snprintf(msg, sizeof(msg), "The card on line %d is an RP but has fewer than 4 integer inputs.", i);
L1626:         snprintf(msg, sizeof(msg), "The card on line %d is an RP but has fewer than 4 floating-point inputs.", i);
L1634:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with non-positive NTHETA (I2).", i);
L1639:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with non-positive NPHI (I3).", i);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 1650
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 1590-1655) ---
   1590:       }
   1591:       else if (gw_radius <= 0.0)
   1592:       {
   1593:         snprintf(msg, sizeof(msg), "The card on line %d is a GW with non-positive radius in F7.", i);
   1594:         add_error(ctx, errors, msg, 0);
   1595:       }
   1596:       // If radius is zero (and not a formula), next card should be a GC with tapering info
   1597:       if ((deck->cards[i].flts_used >= 7 || deck->cards[i].flt_form_inline[7]) && gw_radius == 0.0)
   1598:       {
   1599:         if (i + 1 < deck->num_cards)
   1600:         {
   1601:           if (strcmp(deck->cards[i + 1].card_code, "GC") != 0)
   1602:           {
   1603:             snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius, but the next card is not a GC.", i + 1);
   1604:             add_error(ctx, errors, msg, 1);
   1605:           }
   1606:         }
   1607:         else
   1608:         {
   1609:           snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius and is the last card; a following GC is required.", i + 1);
   1610:           add_error(ctx, errors, msg, 1);
   1611:         }
   1612:       }
   1613:     }
   1614: 
   1615:     // RP: radiation pattern — counts, steps, and basic range sanity
   1616:     if (strcmp(code, "RP") == 0)
   1617:     {
   1618:       // Typical RP uses at least 4 integers and 4 floats
   1619:       if (deck->cards[i].ints_used < 4)
   1620:       {
   1621:         snprintf(msg, sizeof(msg), "The card on line %d is an RP but has fewer than 4 integer inputs.", i);
   1622:         add_error(ctx, errors, msg, 0);
   1623:       }
   1624:       if (deck->cards[i].flts_used < 4)
   1625:       {
   1626:         snprintf(msg, sizeof(msg), "The card on line %d is an RP but has fewer than 4 floating-point inputs.", i);
   1627:         add_error(ctx, errors, msg, 0);
   1628:       }
   1629:       // Number of theta/phi points should be positive
   1630:       int ntheta = deck->cards[i].i[2];
   1631:       int nphi = deck->cards[i].i[3];
   1632:       if (ntheta <= 0)
   1633:       {
   1634:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with non-positive NTHETA (I2).", i);
   1635:         add_error(ctx, errors, msg, 0);
   1636:       }
   1637:       if (nphi <= 0)
   1638:       {
   1639:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with non-positive NPHI (I3).", i);
   1640:         add_error(ctx, errors, msg, 0);
   1641:       }
   1642:       // Steps must be non-zero when requesting multiple points
   1643:       double th_start = deck->cards[i].f[1];
   1644:       double ph_start = deck->cards[i].f[2];
   1645:       double th_step = deck->cards[i].f[3];
   1646:       double ph_step = deck->cards[i].f[4];
   1647:       if (ntheta > 1 && th_step == 0.0)
   1648:       {
   1649:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with NTHETA > 1 but zero theta step (F3).", i);
>  1650:         add_error(ctx, errors, msg, 0);
   1651:       }
   1652:       if (nphi > 1 && ph_step == 0.0)
   1653:       {
   1654:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with NPHI > 1 but zero phi step (F4).", i);
   1655:         add_error(ctx, errors, msg, 0);

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L1482:             snprintf(msg, sizeof(msg), "The tag number %d is found on card %d and card %d.", tag_i, i + 1, j + 1);
L1520:         snprintf(msg, sizeof(msg), "The card on line %d is a CE but has numeric inputs.", i);
L1530:         snprintf(msg, sizeof(msg), "The card on line %d is an EN but has numeric inputs.", i);
L1541:         snprintf(msg, sizeof(msg), "The card on line %d is a FR but has no base frequency in F1.", i);
L1547:         snprintf(msg, sizeof(msg), "The card on line %d is a FR with I2 = 0 (single frequency), but has a non-zero F2.", i);
L1553:         snprintf(msg, sizeof(msg), "The card on line %d is a FR with I2 > 0 (stepped), but F2 is not a positive step.", i);
L1564:         snprintf(msg, sizeof(msg), "The card on line %d is a GW but has fewer than 2 integer inputs (tag, segments).", i);
L1572:           snprintf(msg, sizeof(msg), "The card on line %d is a GW with non-positive segment count.", i);
L1581:         snprintf(msg, sizeof(msg), "The card on line %d is a GW with identical endpoints (zero-length wire).", i);
L1588:         snprintf(msg, sizeof(msg), "The card on line %d is a GW but has no radius specified in F7.", i);
L1593:         snprintf(msg, sizeof(msg), "The card on line %d is a GW with non-positive radius in F7.", i);
L1603:             snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius, but the next card is not a GC.", i + 1);
L1609:           snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius and is the last card; a following GC is required.", i + 1);
L1621:         snprintf(msg, sizeof(msg), "The card on line %d is an RP but has fewer than 4 integer inputs.", i);
L1626:         snprintf(msg, sizeof(msg), "The card on line %d is an RP but has fewer than 4 floating-point inputs.", i);
L1634:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with non-positive NTHETA (I2).", i);
L1639:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with non-positive NPHI (I3).", i);
L1649:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with NTHETA > 1 but zero theta step (F3).", i);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 1655
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 1595-1660) ---
   1595:       }
   1596:       // If radius is zero (and not a formula), next card should be a GC with tapering info
   1597:       if ((deck->cards[i].flts_used >= 7 || deck->cards[i].flt_form_inline[7]) && gw_radius == 0.0)
   1598:       {
   1599:         if (i + 1 < deck->num_cards)
   1600:         {
   1601:           if (strcmp(deck->cards[i + 1].card_code, "GC") != 0)
   1602:           {
   1603:             snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius, but the next card is not a GC.", i + 1);
   1604:             add_error(ctx, errors, msg, 1);
   1605:           }
   1606:         }
   1607:         else
   1608:         {
   1609:           snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius and is the last card; a following GC is required.", i + 1);
   1610:           add_error(ctx, errors, msg, 1);
   1611:         }
   1612:       }
   1613:     }
   1614: 
   1615:     // RP: radiation pattern — counts, steps, and basic range sanity
   1616:     if (strcmp(code, "RP") == 0)
   1617:     {
   1618:       // Typical RP uses at least 4 integers and 4 floats
   1619:       if (deck->cards[i].ints_used < 4)
   1620:       {
   1621:         snprintf(msg, sizeof(msg), "The card on line %d is an RP but has fewer than 4 integer inputs.", i);
   1622:         add_error(ctx, errors, msg, 0);
   1623:       }
   1624:       if (deck->cards[i].flts_used < 4)
   1625:       {
   1626:         snprintf(msg, sizeof(msg), "The card on line %d is an RP but has fewer than 4 floating-point inputs.", i);
   1627:         add_error(ctx, errors, msg, 0);
   1628:       }
   1629:       // Number of theta/phi points should be positive
   1630:       int ntheta = deck->cards[i].i[2];
   1631:       int nphi = deck->cards[i].i[3];
   1632:       if (ntheta <= 0)
   1633:       {
   1634:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with non-positive NTHETA (I2).", i);
   1635:         add_error(ctx, errors, msg, 0);
   1636:       }
   1637:       if (nphi <= 0)
   1638:       {
   1639:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with non-positive NPHI (I3).", i);
   1640:         add_error(ctx, errors, msg, 0);
   1641:       }
   1642:       // Steps must be non-zero when requesting multiple points
   1643:       double th_start = deck->cards[i].f[1];
   1644:       double ph_start = deck->cards[i].f[2];
   1645:       double th_step = deck->cards[i].f[3];
   1646:       double ph_step = deck->cards[i].f[4];
   1647:       if (ntheta > 1 && th_step == 0.0)
   1648:       {
   1649:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with NTHETA > 1 but zero theta step (F3).", i);
   1650:         add_error(ctx, errors, msg, 0);
   1651:       }
   1652:       if (nphi > 1 && ph_step == 0.0)
   1653:       {
   1654:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with NPHI > 1 but zero phi step (F4).", i);
>  1655:         add_error(ctx, errors, msg, 0);
   1656:       }
   1657:       // Basic angle sanity: starts within typical ranges
   1658:       if (!(th_start >= -180.0 && th_start <= 180.0))
   1659:       {
   1660:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with theta start (F1) outside [-180,180].", i);

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L1482:             snprintf(msg, sizeof(msg), "The tag number %d is found on card %d and card %d.", tag_i, i + 1, j + 1);
L1520:         snprintf(msg, sizeof(msg), "The card on line %d is a CE but has numeric inputs.", i);
L1530:         snprintf(msg, sizeof(msg), "The card on line %d is an EN but has numeric inputs.", i);
L1541:         snprintf(msg, sizeof(msg), "The card on line %d is a FR but has no base frequency in F1.", i);
L1547:         snprintf(msg, sizeof(msg), "The card on line %d is a FR with I2 = 0 (single frequency), but has a non-zero F2.", i);
L1553:         snprintf(msg, sizeof(msg), "The card on line %d is a FR with I2 > 0 (stepped), but F2 is not a positive step.", i);
L1564:         snprintf(msg, sizeof(msg), "The card on line %d is a GW but has fewer than 2 integer inputs (tag, segments).", i);
L1572:           snprintf(msg, sizeof(msg), "The card on line %d is a GW with non-positive segment count.", i);
L1581:         snprintf(msg, sizeof(msg), "The card on line %d is a GW with identical endpoints (zero-length wire).", i);
L1588:         snprintf(msg, sizeof(msg), "The card on line %d is a GW but has no radius specified in F7.", i);
L1593:         snprintf(msg, sizeof(msg), "The card on line %d is a GW with non-positive radius in F7.", i);
L1603:             snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius, but the next card is not a GC.", i + 1);
L1609:           snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius and is the last card; a following GC is required.", i + 1);
L1621:         snprintf(msg, sizeof(msg), "The card on line %d is an RP but has fewer than 4 integer inputs.", i);
L1626:         snprintf(msg, sizeof(msg), "The card on line %d is an RP but has fewer than 4 floating-point inputs.", i);
L1634:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with non-positive NTHETA (I2).", i);
L1639:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with non-positive NPHI (I3).", i);
L1649:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with NTHETA > 1 but zero theta step (F3).", i);
L1654:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with NPHI > 1 but zero phi step (F4).", i);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 1661
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 1601-1666) ---
   1601:           if (strcmp(deck->cards[i + 1].card_code, "GC") != 0)
   1602:           {
   1603:             snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius, but the next card is not a GC.", i + 1);
   1604:             add_error(ctx, errors, msg, 1);
   1605:           }
   1606:         }
   1607:         else
   1608:         {
   1609:           snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius and is the last card; a following GC is required.", i + 1);
   1610:           add_error(ctx, errors, msg, 1);
   1611:         }
   1612:       }
   1613:     }
   1614: 
   1615:     // RP: radiation pattern — counts, steps, and basic range sanity
   1616:     if (strcmp(code, "RP") == 0)
   1617:     {
   1618:       // Typical RP uses at least 4 integers and 4 floats
   1619:       if (deck->cards[i].ints_used < 4)
   1620:       {
   1621:         snprintf(msg, sizeof(msg), "The card on line %d is an RP but has fewer than 4 integer inputs.", i);
   1622:         add_error(ctx, errors, msg, 0);
   1623:       }
   1624:       if (deck->cards[i].flts_used < 4)
   1625:       {
   1626:         snprintf(msg, sizeof(msg), "The card on line %d is an RP but has fewer than 4 floating-point inputs.", i);
   1627:         add_error(ctx, errors, msg, 0);
   1628:       }
   1629:       // Number of theta/phi points should be positive
   1630:       int ntheta = deck->cards[i].i[2];
   1631:       int nphi = deck->cards[i].i[3];
   1632:       if (ntheta <= 0)
   1633:       {
   1634:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with non-positive NTHETA (I2).", i);
   1635:         add_error(ctx, errors, msg, 0);
   1636:       }
   1637:       if (nphi <= 0)
   1638:       {
   1639:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with non-positive NPHI (I3).", i);
   1640:         add_error(ctx, errors, msg, 0);
   1641:       }
   1642:       // Steps must be non-zero when requesting multiple points
   1643:       double th_start = deck->cards[i].f[1];
   1644:       double ph_start = deck->cards[i].f[2];
   1645:       double th_step = deck->cards[i].f[3];
   1646:       double ph_step = deck->cards[i].f[4];
   1647:       if (ntheta > 1 && th_step == 0.0)
   1648:       {
   1649:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with NTHETA > 1 but zero theta step (F3).", i);
   1650:         add_error(ctx, errors, msg, 0);
   1651:       }
   1652:       if (nphi > 1 && ph_step == 0.0)
   1653:       {
   1654:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with NPHI > 1 but zero phi step (F4).", i);
   1655:         add_error(ctx, errors, msg, 0);
   1656:       }
   1657:       // Basic angle sanity: starts within typical ranges
   1658:       if (!(th_start >= -180.0 && th_start <= 180.0))
   1659:       {
   1660:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with theta start (F1) outside [-180,180].", i);
>  1661:         add_error(ctx, errors, msg, 0);
   1662:       }
   1663:       if (!(ph_start >= -360.0 && ph_start <= 360.0))
   1664:       {
   1665:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with phi start (F2) outside [-360,360].", i);
   1666:         add_error(ctx, errors, msg, 0);

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L1482:             snprintf(msg, sizeof(msg), "The tag number %d is found on card %d and card %d.", tag_i, i + 1, j + 1);
L1520:         snprintf(msg, sizeof(msg), "The card on line %d is a CE but has numeric inputs.", i);
L1530:         snprintf(msg, sizeof(msg), "The card on line %d is an EN but has numeric inputs.", i);
L1541:         snprintf(msg, sizeof(msg), "The card on line %d is a FR but has no base frequency in F1.", i);
L1547:         snprintf(msg, sizeof(msg), "The card on line %d is a FR with I2 = 0 (single frequency), but has a non-zero F2.", i);
L1553:         snprintf(msg, sizeof(msg), "The card on line %d is a FR with I2 > 0 (stepped), but F2 is not a positive step.", i);
L1564:         snprintf(msg, sizeof(msg), "The card on line %d is a GW but has fewer than 2 integer inputs (tag, segments).", i);
L1572:           snprintf(msg, sizeof(msg), "The card on line %d is a GW with non-positive segment count.", i);
L1581:         snprintf(msg, sizeof(msg), "The card on line %d is a GW with identical endpoints (zero-length wire).", i);
L1588:         snprintf(msg, sizeof(msg), "The card on line %d is a GW but has no radius specified in F7.", i);
L1593:         snprintf(msg, sizeof(msg), "The card on line %d is a GW with non-positive radius in F7.", i);
L1603:             snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius, but the next card is not a GC.", i + 1);
L1609:           snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius and is the last card; a following GC is required.", i + 1);
L1621:         snprintf(msg, sizeof(msg), "The card on line %d is an RP but has fewer than 4 integer inputs.", i);
L1626:         snprintf(msg, sizeof(msg), "The card on line %d is an RP but has fewer than 4 floating-point inputs.", i);
L1634:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with non-positive NTHETA (I2).", i);
L1639:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with non-positive NPHI (I3).", i);
L1649:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with NTHETA > 1 but zero theta step (F3).", i);
L1654:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with NPHI > 1 but zero phi step (F4).", i);
L1660:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with theta start (F1) outside [-180,180].", i);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 1666
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 1606-1671) ---
   1606:         }
   1607:         else
   1608:         {
   1609:           snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius and is the last card; a following GC is required.", i + 1);
   1610:           add_error(ctx, errors, msg, 1);
   1611:         }
   1612:       }
   1613:     }
   1614: 
   1615:     // RP: radiation pattern — counts, steps, and basic range sanity
   1616:     if (strcmp(code, "RP") == 0)
   1617:     {
   1618:       // Typical RP uses at least 4 integers and 4 floats
   1619:       if (deck->cards[i].ints_used < 4)
   1620:       {
   1621:         snprintf(msg, sizeof(msg), "The card on line %d is an RP but has fewer than 4 integer inputs.", i);
   1622:         add_error(ctx, errors, msg, 0);
   1623:       }
   1624:       if (deck->cards[i].flts_used < 4)
   1625:       {
   1626:         snprintf(msg, sizeof(msg), "The card on line %d is an RP but has fewer than 4 floating-point inputs.", i);
   1627:         add_error(ctx, errors, msg, 0);
   1628:       }
   1629:       // Number of theta/phi points should be positive
   1630:       int ntheta = deck->cards[i].i[2];
   1631:       int nphi = deck->cards[i].i[3];
   1632:       if (ntheta <= 0)
   1633:       {
   1634:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with non-positive NTHETA (I2).", i);
   1635:         add_error(ctx, errors, msg, 0);
   1636:       }
   1637:       if (nphi <= 0)
   1638:       {
   1639:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with non-positive NPHI (I3).", i);
   1640:         add_error(ctx, errors, msg, 0);
   1641:       }
   1642:       // Steps must be non-zero when requesting multiple points
   1643:       double th_start = deck->cards[i].f[1];
   1644:       double ph_start = deck->cards[i].f[2];
   1645:       double th_step = deck->cards[i].f[3];
   1646:       double ph_step = deck->cards[i].f[4];
   1647:       if (ntheta > 1 && th_step == 0.0)
   1648:       {
   1649:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with NTHETA > 1 but zero theta step (F3).", i);
   1650:         add_error(ctx, errors, msg, 0);
   1651:       }
   1652:       if (nphi > 1 && ph_step == 0.0)
   1653:       {
   1654:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with NPHI > 1 but zero phi step (F4).", i);
   1655:         add_error(ctx, errors, msg, 0);
   1656:       }
   1657:       // Basic angle sanity: starts within typical ranges
   1658:       if (!(th_start >= -180.0 && th_start <= 180.0))
   1659:       {
   1660:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with theta start (F1) outside [-180,180].", i);
   1661:         add_error(ctx, errors, msg, 0);
   1662:       }
   1663:       if (!(ph_start >= -360.0 && ph_start <= 360.0))
   1664:       {
   1665:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with phi start (F2) outside [-360,360].", i);
>  1666:         add_error(ctx, errors, msg, 0);
   1667:       }
   1668:       // Step magnitudes should be reasonable
   1669:       if (fabs(th_step) > 180.0)
   1670:       {
   1671:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with an excessively large theta step (F3).", i);

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L1482:             snprintf(msg, sizeof(msg), "The tag number %d is found on card %d and card %d.", tag_i, i + 1, j + 1);
L1520:         snprintf(msg, sizeof(msg), "The card on line %d is a CE but has numeric inputs.", i);
L1530:         snprintf(msg, sizeof(msg), "The card on line %d is an EN but has numeric inputs.", i);
L1541:         snprintf(msg, sizeof(msg), "The card on line %d is a FR but has no base frequency in F1.", i);
L1547:         snprintf(msg, sizeof(msg), "The card on line %d is a FR with I2 = 0 (single frequency), but has a non-zero F2.", i);
L1553:         snprintf(msg, sizeof(msg), "The card on line %d is a FR with I2 > 0 (stepped), but F2 is not a positive step.", i);
L1564:         snprintf(msg, sizeof(msg), "The card on line %d is a GW but has fewer than 2 integer inputs (tag, segments).", i);
L1572:           snprintf(msg, sizeof(msg), "The card on line %d is a GW with non-positive segment count.", i);
L1581:         snprintf(msg, sizeof(msg), "The card on line %d is a GW with identical endpoints (zero-length wire).", i);
L1588:         snprintf(msg, sizeof(msg), "The card on line %d is a GW but has no radius specified in F7.", i);
L1593:         snprintf(msg, sizeof(msg), "The card on line %d is a GW with non-positive radius in F7.", i);
L1603:             snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius, but the next card is not a GC.", i + 1);
L1609:           snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius and is the last card; a following GC is required.", i + 1);
L1621:         snprintf(msg, sizeof(msg), "The card on line %d is an RP but has fewer than 4 integer inputs.", i);
L1626:         snprintf(msg, sizeof(msg), "The card on line %d is an RP but has fewer than 4 floating-point inputs.", i);
L1634:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with non-positive NTHETA (I2).", i);
L1639:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with non-positive NPHI (I3).", i);
L1649:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with NTHETA > 1 but zero theta step (F3).", i);
L1654:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with NPHI > 1 but zero phi step (F4).", i);
L1660:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with theta start (F1) outside [-180,180].", i);
L1665:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with phi start (F2) outside [-360,360].", i);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 1672
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 1612-1677) ---
   1612:       }
   1613:     }
   1614: 
   1615:     // RP: radiation pattern — counts, steps, and basic range sanity
   1616:     if (strcmp(code, "RP") == 0)
   1617:     {
   1618:       // Typical RP uses at least 4 integers and 4 floats
   1619:       if (deck->cards[i].ints_used < 4)
   1620:       {
   1621:         snprintf(msg, sizeof(msg), "The card on line %d is an RP but has fewer than 4 integer inputs.", i);
   1622:         add_error(ctx, errors, msg, 0);
   1623:       }
   1624:       if (deck->cards[i].flts_used < 4)
   1625:       {
   1626:         snprintf(msg, sizeof(msg), "The card on line %d is an RP but has fewer than 4 floating-point inputs.", i);
   1627:         add_error(ctx, errors, msg, 0);
   1628:       }
   1629:       // Number of theta/phi points should be positive
   1630:       int ntheta = deck->cards[i].i[2];
   1631:       int nphi = deck->cards[i].i[3];
   1632:       if (ntheta <= 0)
   1633:       {
   1634:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with non-positive NTHETA (I2).", i);
   1635:         add_error(ctx, errors, msg, 0);
   1636:       }
   1637:       if (nphi <= 0)
   1638:       {
   1639:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with non-positive NPHI (I3).", i);
   1640:         add_error(ctx, errors, msg, 0);
   1641:       }
   1642:       // Steps must be non-zero when requesting multiple points
   1643:       double th_start = deck->cards[i].f[1];
   1644:       double ph_start = deck->cards[i].f[2];
   1645:       double th_step = deck->cards[i].f[3];
   1646:       double ph_step = deck->cards[i].f[4];
   1647:       if (ntheta > 1 && th_step == 0.0)
   1648:       {
   1649:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with NTHETA > 1 but zero theta step (F3).", i);
   1650:         add_error(ctx, errors, msg, 0);
   1651:       }
   1652:       if (nphi > 1 && ph_step == 0.0)
   1653:       {
   1654:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with NPHI > 1 but zero phi step (F4).", i);
   1655:         add_error(ctx, errors, msg, 0);
   1656:       }
   1657:       // Basic angle sanity: starts within typical ranges
   1658:       if (!(th_start >= -180.0 && th_start <= 180.0))
   1659:       {
   1660:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with theta start (F1) outside [-180,180].", i);
   1661:         add_error(ctx, errors, msg, 0);
   1662:       }
   1663:       if (!(ph_start >= -360.0 && ph_start <= 360.0))
   1664:       {
   1665:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with phi start (F2) outside [-360,360].", i);
   1666:         add_error(ctx, errors, msg, 0);
   1667:       }
   1668:       // Step magnitudes should be reasonable
   1669:       if (fabs(th_step) > 180.0)
   1670:       {
   1671:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with an excessively large theta step (F3).", i);
>  1672:         add_error(ctx, errors, msg, 0);
   1673:       }
   1674:       if (fabs(ph_step) > 360.0)
   1675:       {
   1676:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with an excessively large phi step (F4).", i);
   1677:         add_error(ctx, errors, msg, 0);

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L1482:             snprintf(msg, sizeof(msg), "The tag number %d is found on card %d and card %d.", tag_i, i + 1, j + 1);
L1520:         snprintf(msg, sizeof(msg), "The card on line %d is a CE but has numeric inputs.", i);
L1530:         snprintf(msg, sizeof(msg), "The card on line %d is an EN but has numeric inputs.", i);
L1541:         snprintf(msg, sizeof(msg), "The card on line %d is a FR but has no base frequency in F1.", i);
L1547:         snprintf(msg, sizeof(msg), "The card on line %d is a FR with I2 = 0 (single frequency), but has a non-zero F2.", i);
L1553:         snprintf(msg, sizeof(msg), "The card on line %d is a FR with I2 > 0 (stepped), but F2 is not a positive step.", i);
L1564:         snprintf(msg, sizeof(msg), "The card on line %d is a GW but has fewer than 2 integer inputs (tag, segments).", i);
L1572:           snprintf(msg, sizeof(msg), "The card on line %d is a GW with non-positive segment count.", i);
L1581:         snprintf(msg, sizeof(msg), "The card on line %d is a GW with identical endpoints (zero-length wire).", i);
L1588:         snprintf(msg, sizeof(msg), "The card on line %d is a GW but has no radius specified in F7.", i);
L1593:         snprintf(msg, sizeof(msg), "The card on line %d is a GW with non-positive radius in F7.", i);
L1603:             snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius, but the next card is not a GC.", i + 1);
L1609:           snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius and is the last card; a following GC is required.", i + 1);
L1621:         snprintf(msg, sizeof(msg), "The card on line %d is an RP but has fewer than 4 integer inputs.", i);
L1626:         snprintf(msg, sizeof(msg), "The card on line %d is an RP but has fewer than 4 floating-point inputs.", i);
L1634:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with non-positive NTHETA (I2).", i);
L1639:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with non-positive NPHI (I3).", i);
L1649:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with NTHETA > 1 but zero theta step (F3).", i);
L1654:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with NPHI > 1 but zero phi step (F4).", i);
L1660:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with theta start (F1) outside [-180,180].", i);
L1665:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with phi start (F2) outside [-360,360].", i);
L1671:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with an excessively large theta step (F3).", i);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 1677
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 1617-1682) ---
   1617:     {
   1618:       // Typical RP uses at least 4 integers and 4 floats
   1619:       if (deck->cards[i].ints_used < 4)
   1620:       {
   1621:         snprintf(msg, sizeof(msg), "The card on line %d is an RP but has fewer than 4 integer inputs.", i);
   1622:         add_error(ctx, errors, msg, 0);
   1623:       }
   1624:       if (deck->cards[i].flts_used < 4)
   1625:       {
   1626:         snprintf(msg, sizeof(msg), "The card on line %d is an RP but has fewer than 4 floating-point inputs.", i);
   1627:         add_error(ctx, errors, msg, 0);
   1628:       }
   1629:       // Number of theta/phi points should be positive
   1630:       int ntheta = deck->cards[i].i[2];
   1631:       int nphi = deck->cards[i].i[3];
   1632:       if (ntheta <= 0)
   1633:       {
   1634:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with non-positive NTHETA (I2).", i);
   1635:         add_error(ctx, errors, msg, 0);
   1636:       }
   1637:       if (nphi <= 0)
   1638:       {
   1639:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with non-positive NPHI (I3).", i);
   1640:         add_error(ctx, errors, msg, 0);
   1641:       }
   1642:       // Steps must be non-zero when requesting multiple points
   1643:       double th_start = deck->cards[i].f[1];
   1644:       double ph_start = deck->cards[i].f[2];
   1645:       double th_step = deck->cards[i].f[3];
   1646:       double ph_step = deck->cards[i].f[4];
   1647:       if (ntheta > 1 && th_step == 0.0)
   1648:       {
   1649:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with NTHETA > 1 but zero theta step (F3).", i);
   1650:         add_error(ctx, errors, msg, 0);
   1651:       }
   1652:       if (nphi > 1 && ph_step == 0.0)
   1653:       {
   1654:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with NPHI > 1 but zero phi step (F4).", i);
   1655:         add_error(ctx, errors, msg, 0);
   1656:       }
   1657:       // Basic angle sanity: starts within typical ranges
   1658:       if (!(th_start >= -180.0 && th_start <= 180.0))
   1659:       {
   1660:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with theta start (F1) outside [-180,180].", i);
   1661:         add_error(ctx, errors, msg, 0);
   1662:       }
   1663:       if (!(ph_start >= -360.0 && ph_start <= 360.0))
   1664:       {
   1665:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with phi start (F2) outside [-360,360].", i);
   1666:         add_error(ctx, errors, msg, 0);
   1667:       }
   1668:       // Step magnitudes should be reasonable
   1669:       if (fabs(th_step) > 180.0)
   1670:       {
   1671:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with an excessively large theta step (F3).", i);
   1672:         add_error(ctx, errors, msg, 0);
   1673:       }
   1674:       if (fabs(ph_step) > 360.0)
   1675:       {
   1676:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with an excessively large phi step (F4).", i);
>  1677:         add_error(ctx, errors, msg, 0);
   1678:       }
   1679:       // Derived end angles should remain within sensible bounds and match step direction
   1680:       if (ntheta > 1)
   1681:       {
   1682:         double th_end = th_start + (ntheta - 1) * th_step;

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L1482:             snprintf(msg, sizeof(msg), "The tag number %d is found on card %d and card %d.", tag_i, i + 1, j + 1);
L1520:         snprintf(msg, sizeof(msg), "The card on line %d is a CE but has numeric inputs.", i);
L1530:         snprintf(msg, sizeof(msg), "The card on line %d is an EN but has numeric inputs.", i);
L1541:         snprintf(msg, sizeof(msg), "The card on line %d is a FR but has no base frequency in F1.", i);
L1547:         snprintf(msg, sizeof(msg), "The card on line %d is a FR with I2 = 0 (single frequency), but has a non-zero F2.", i);
L1553:         snprintf(msg, sizeof(msg), "The card on line %d is a FR with I2 > 0 (stepped), but F2 is not a positive step.", i);
L1564:         snprintf(msg, sizeof(msg), "The card on line %d is a GW but has fewer than 2 integer inputs (tag, segments).", i);
L1572:           snprintf(msg, sizeof(msg), "The card on line %d is a GW with non-positive segment count.", i);
L1581:         snprintf(msg, sizeof(msg), "The card on line %d is a GW with identical endpoints (zero-length wire).", i);
L1588:         snprintf(msg, sizeof(msg), "The card on line %d is a GW but has no radius specified in F7.", i);
L1593:         snprintf(msg, sizeof(msg), "The card on line %d is a GW with non-positive radius in F7.", i);
L1603:             snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius, but the next card is not a GC.", i + 1);
L1609:           snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius and is the last card; a following GC is required.", i + 1);
L1621:         snprintf(msg, sizeof(msg), "The card on line %d is an RP but has fewer than 4 integer inputs.", i);
L1626:         snprintf(msg, sizeof(msg), "The card on line %d is an RP but has fewer than 4 floating-point inputs.", i);
L1634:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with non-positive NTHETA (I2).", i);
L1639:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with non-positive NPHI (I3).", i);
L1649:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with NTHETA > 1 but zero theta step (F3).", i);
L1654:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with NPHI > 1 but zero phi step (F4).", i);
L1660:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with theta start (F1) outside [-180,180].", i);
L1665:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with phi start (F2) outside [-360,360].", i);
L1671:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with an excessively large theta step (F3).", i);
L1676:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with an excessively large phi step (F4).", i);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 1686
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 1626-1691) ---
   1626:         snprintf(msg, sizeof(msg), "The card on line %d is an RP but has fewer than 4 floating-point inputs.", i);
   1627:         add_error(ctx, errors, msg, 0);
   1628:       }
   1629:       // Number of theta/phi points should be positive
   1630:       int ntheta = deck->cards[i].i[2];
   1631:       int nphi = deck->cards[i].i[3];
   1632:       if (ntheta <= 0)
   1633:       {
   1634:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with non-positive NTHETA (I2).", i);
   1635:         add_error(ctx, errors, msg, 0);
   1636:       }
   1637:       if (nphi <= 0)
   1638:       {
   1639:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with non-positive NPHI (I3).", i);
   1640:         add_error(ctx, errors, msg, 0);
   1641:       }
   1642:       // Steps must be non-zero when requesting multiple points
   1643:       double th_start = deck->cards[i].f[1];
   1644:       double ph_start = deck->cards[i].f[2];
   1645:       double th_step = deck->cards[i].f[3];
   1646:       double ph_step = deck->cards[i].f[4];
   1647:       if (ntheta > 1 && th_step == 0.0)
   1648:       {
   1649:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with NTHETA > 1 but zero theta step (F3).", i);
   1650:         add_error(ctx, errors, msg, 0);
   1651:       }
   1652:       if (nphi > 1 && ph_step == 0.0)
   1653:       {
   1654:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with NPHI > 1 but zero phi step (F4).", i);
   1655:         add_error(ctx, errors, msg, 0);
   1656:       }
   1657:       // Basic angle sanity: starts within typical ranges
   1658:       if (!(th_start >= -180.0 && th_start <= 180.0))
   1659:       {
   1660:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with theta start (F1) outside [-180,180].", i);
   1661:         add_error(ctx, errors, msg, 0);
   1662:       }
   1663:       if (!(ph_start >= -360.0 && ph_start <= 360.0))
   1664:       {
   1665:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with phi start (F2) outside [-360,360].", i);
   1666:         add_error(ctx, errors, msg, 0);
   1667:       }
   1668:       // Step magnitudes should be reasonable
   1669:       if (fabs(th_step) > 180.0)
   1670:       {
   1671:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with an excessively large theta step (F3).", i);
   1672:         add_error(ctx, errors, msg, 0);
   1673:       }
   1674:       if (fabs(ph_step) > 360.0)
   1675:       {
   1676:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with an excessively large phi step (F4).", i);
   1677:         add_error(ctx, errors, msg, 0);
   1678:       }
   1679:       // Derived end angles should remain within sensible bounds and match step direction
   1680:       if (ntheta > 1)
   1681:       {
   1682:         double th_end = th_start + (ntheta - 1) * th_step;
   1683:         if (!(th_end >= -180.0 && th_end <= 180.0))
   1684:         {
   1685:           snprintf(msg, sizeof(msg), "The card on line %d is an RP whose theta sweep leaves the [-180,180] range.", i);
>  1686:           add_error(ctx, errors, msg, 0);
   1687:         }
   1688:         if (th_step > 0.0 && th_end < th_start)
   1689:         {
   1690:           snprintf(msg, sizeof(msg), "The card on line %d is an RP with NTHETA>1 and positive theta step but decreasing sweep.", i);
   1691:           add_error(ctx, errors, msg, 0);

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L1520:         snprintf(msg, sizeof(msg), "The card on line %d is a CE but has numeric inputs.", i);
L1530:         snprintf(msg, sizeof(msg), "The card on line %d is an EN but has numeric inputs.", i);
L1541:         snprintf(msg, sizeof(msg), "The card on line %d is a FR but has no base frequency in F1.", i);
L1547:         snprintf(msg, sizeof(msg), "The card on line %d is a FR with I2 = 0 (single frequency), but has a non-zero F2.", i);
L1553:         snprintf(msg, sizeof(msg), "The card on line %d is a FR with I2 > 0 (stepped), but F2 is not a positive step.", i);
L1564:         snprintf(msg, sizeof(msg), "The card on line %d is a GW but has fewer than 2 integer inputs (tag, segments).", i);
L1572:           snprintf(msg, sizeof(msg), "The card on line %d is a GW with non-positive segment count.", i);
L1581:         snprintf(msg, sizeof(msg), "The card on line %d is a GW with identical endpoints (zero-length wire).", i);
L1588:         snprintf(msg, sizeof(msg), "The card on line %d is a GW but has no radius specified in F7.", i);
L1593:         snprintf(msg, sizeof(msg), "The card on line %d is a GW with non-positive radius in F7.", i);
L1603:             snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius, but the next card is not a GC.", i + 1);
L1609:           snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius and is the last card; a following GC is required.", i + 1);
L1621:         snprintf(msg, sizeof(msg), "The card on line %d is an RP but has fewer than 4 integer inputs.", i);
L1626:         snprintf(msg, sizeof(msg), "The card on line %d is an RP but has fewer than 4 floating-point inputs.", i);
L1634:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with non-positive NTHETA (I2).", i);
L1639:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with non-positive NPHI (I3).", i);
L1649:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with NTHETA > 1 but zero theta step (F3).", i);
L1654:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with NPHI > 1 but zero phi step (F4).", i);
L1660:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with theta start (F1) outside [-180,180].", i);
L1665:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with phi start (F2) outside [-360,360].", i);
L1671:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with an excessively large theta step (F3).", i);
L1676:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with an excessively large phi step (F4).", i);
L1685:           snprintf(msg, sizeof(msg), "The card on line %d is an RP whose theta sweep leaves the [-180,180] range.", i);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 1691
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 1631-1696) ---
   1631:       int nphi = deck->cards[i].i[3];
   1632:       if (ntheta <= 0)
   1633:       {
   1634:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with non-positive NTHETA (I2).", i);
   1635:         add_error(ctx, errors, msg, 0);
   1636:       }
   1637:       if (nphi <= 0)
   1638:       {
   1639:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with non-positive NPHI (I3).", i);
   1640:         add_error(ctx, errors, msg, 0);
   1641:       }
   1642:       // Steps must be non-zero when requesting multiple points
   1643:       double th_start = deck->cards[i].f[1];
   1644:       double ph_start = deck->cards[i].f[2];
   1645:       double th_step = deck->cards[i].f[3];
   1646:       double ph_step = deck->cards[i].f[4];
   1647:       if (ntheta > 1 && th_step == 0.0)
   1648:       {
   1649:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with NTHETA > 1 but zero theta step (F3).", i);
   1650:         add_error(ctx, errors, msg, 0);
   1651:       }
   1652:       if (nphi > 1 && ph_step == 0.0)
   1653:       {
   1654:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with NPHI > 1 but zero phi step (F4).", i);
   1655:         add_error(ctx, errors, msg, 0);
   1656:       }
   1657:       // Basic angle sanity: starts within typical ranges
   1658:       if (!(th_start >= -180.0 && th_start <= 180.0))
   1659:       {
   1660:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with theta start (F1) outside [-180,180].", i);
   1661:         add_error(ctx, errors, msg, 0);
   1662:       }
   1663:       if (!(ph_start >= -360.0 && ph_start <= 360.0))
   1664:       {
   1665:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with phi start (F2) outside [-360,360].", i);
   1666:         add_error(ctx, errors, msg, 0);
   1667:       }
   1668:       // Step magnitudes should be reasonable
   1669:       if (fabs(th_step) > 180.0)
   1670:       {
   1671:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with an excessively large theta step (F3).", i);
   1672:         add_error(ctx, errors, msg, 0);
   1673:       }
   1674:       if (fabs(ph_step) > 360.0)
   1675:       {
   1676:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with an excessively large phi step (F4).", i);
   1677:         add_error(ctx, errors, msg, 0);
   1678:       }
   1679:       // Derived end angles should remain within sensible bounds and match step direction
   1680:       if (ntheta > 1)
   1681:       {
   1682:         double th_end = th_start + (ntheta - 1) * th_step;
   1683:         if (!(th_end >= -180.0 && th_end <= 180.0))
   1684:         {
   1685:           snprintf(msg, sizeof(msg), "The card on line %d is an RP whose theta sweep leaves the [-180,180] range.", i);
   1686:           add_error(ctx, errors, msg, 0);
   1687:         }
   1688:         if (th_step > 0.0 && th_end < th_start)
   1689:         {
   1690:           snprintf(msg, sizeof(msg), "The card on line %d is an RP with NTHETA>1 and positive theta step but decreasing sweep.", i);
>  1691:           add_error(ctx, errors, msg, 0);
   1692:         }
   1693:         if (th_step < 0.0 && th_end > th_start)
   1694:         {
   1695:           snprintf(msg, sizeof(msg), "The card on line %d is an RP with NTHETA>1 and negative theta step but increasing sweep.", i);
   1696:           add_error(ctx, errors, msg, 0);

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L1520:         snprintf(msg, sizeof(msg), "The card on line %d is a CE but has numeric inputs.", i);
L1530:         snprintf(msg, sizeof(msg), "The card on line %d is an EN but has numeric inputs.", i);
L1541:         snprintf(msg, sizeof(msg), "The card on line %d is a FR but has no base frequency in F1.", i);
L1547:         snprintf(msg, sizeof(msg), "The card on line %d is a FR with I2 = 0 (single frequency), but has a non-zero F2.", i);
L1553:         snprintf(msg, sizeof(msg), "The card on line %d is a FR with I2 > 0 (stepped), but F2 is not a positive step.", i);
L1564:         snprintf(msg, sizeof(msg), "The card on line %d is a GW but has fewer than 2 integer inputs (tag, segments).", i);
L1572:           snprintf(msg, sizeof(msg), "The card on line %d is a GW with non-positive segment count.", i);
L1581:         snprintf(msg, sizeof(msg), "The card on line %d is a GW with identical endpoints (zero-length wire).", i);
L1588:         snprintf(msg, sizeof(msg), "The card on line %d is a GW but has no radius specified in F7.", i);
L1593:         snprintf(msg, sizeof(msg), "The card on line %d is a GW with non-positive radius in F7.", i);
L1603:             snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius, but the next card is not a GC.", i + 1);
L1609:           snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius and is the last card; a following GC is required.", i + 1);
L1621:         snprintf(msg, sizeof(msg), "The card on line %d is an RP but has fewer than 4 integer inputs.", i);
L1626:         snprintf(msg, sizeof(msg), "The card on line %d is an RP but has fewer than 4 floating-point inputs.", i);
L1634:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with non-positive NTHETA (I2).", i);
L1639:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with non-positive NPHI (I3).", i);
L1649:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with NTHETA > 1 but zero theta step (F3).", i);
L1654:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with NPHI > 1 but zero phi step (F4).", i);
L1660:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with theta start (F1) outside [-180,180].", i);
L1665:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with phi start (F2) outside [-360,360].", i);
L1671:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with an excessively large theta step (F3).", i);
L1676:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with an excessively large phi step (F4).", i);
L1685:           snprintf(msg, sizeof(msg), "The card on line %d is an RP whose theta sweep leaves the [-180,180] range.", i);
L1690:           snprintf(msg, sizeof(msg), "The card on line %d is an RP with NTHETA>1 and positive theta step but decreasing sweep.", i);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 1696
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 1636-1701) ---
   1636:       }
   1637:       if (nphi <= 0)
   1638:       {
   1639:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with non-positive NPHI (I3).", i);
   1640:         add_error(ctx, errors, msg, 0);
   1641:       }
   1642:       // Steps must be non-zero when requesting multiple points
   1643:       double th_start = deck->cards[i].f[1];
   1644:       double ph_start = deck->cards[i].f[2];
   1645:       double th_step = deck->cards[i].f[3];
   1646:       double ph_step = deck->cards[i].f[4];
   1647:       if (ntheta > 1 && th_step == 0.0)
   1648:       {
   1649:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with NTHETA > 1 but zero theta step (F3).", i);
   1650:         add_error(ctx, errors, msg, 0);
   1651:       }
   1652:       if (nphi > 1 && ph_step == 0.0)
   1653:       {
   1654:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with NPHI > 1 but zero phi step (F4).", i);
   1655:         add_error(ctx, errors, msg, 0);
   1656:       }
   1657:       // Basic angle sanity: starts within typical ranges
   1658:       if (!(th_start >= -180.0 && th_start <= 180.0))
   1659:       {
   1660:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with theta start (F1) outside [-180,180].", i);
   1661:         add_error(ctx, errors, msg, 0);
   1662:       }
   1663:       if (!(ph_start >= -360.0 && ph_start <= 360.0))
   1664:       {
   1665:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with phi start (F2) outside [-360,360].", i);
   1666:         add_error(ctx, errors, msg, 0);
   1667:       }
   1668:       // Step magnitudes should be reasonable
   1669:       if (fabs(th_step) > 180.0)
   1670:       {
   1671:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with an excessively large theta step (F3).", i);
   1672:         add_error(ctx, errors, msg, 0);
   1673:       }
   1674:       if (fabs(ph_step) > 360.0)
   1675:       {
   1676:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with an excessively large phi step (F4).", i);
   1677:         add_error(ctx, errors, msg, 0);
   1678:       }
   1679:       // Derived end angles should remain within sensible bounds and match step direction
   1680:       if (ntheta > 1)
   1681:       {
   1682:         double th_end = th_start + (ntheta - 1) * th_step;
   1683:         if (!(th_end >= -180.0 && th_end <= 180.0))
   1684:         {
   1685:           snprintf(msg, sizeof(msg), "The card on line %d is an RP whose theta sweep leaves the [-180,180] range.", i);
   1686:           add_error(ctx, errors, msg, 0);
   1687:         }
   1688:         if (th_step > 0.0 && th_end < th_start)
   1689:         {
   1690:           snprintf(msg, sizeof(msg), "The card on line %d is an RP with NTHETA>1 and positive theta step but decreasing sweep.", i);
   1691:           add_error(ctx, errors, msg, 0);
   1692:         }
   1693:         if (th_step < 0.0 && th_end > th_start)
   1694:         {
   1695:           snprintf(msg, sizeof(msg), "The card on line %d is an RP with NTHETA>1 and negative theta step but increasing sweep.", i);
>  1696:           add_error(ctx, errors, msg, 0);
   1697:         }
   1698:       }
   1699:       if (nphi > 1)
   1700:       {
   1701:         double ph_end = ph_start + (nphi - 1) * ph_step;

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L1520:         snprintf(msg, sizeof(msg), "The card on line %d is a CE but has numeric inputs.", i);
L1530:         snprintf(msg, sizeof(msg), "The card on line %d is an EN but has numeric inputs.", i);
L1541:         snprintf(msg, sizeof(msg), "The card on line %d is a FR but has no base frequency in F1.", i);
L1547:         snprintf(msg, sizeof(msg), "The card on line %d is a FR with I2 = 0 (single frequency), but has a non-zero F2.", i);
L1553:         snprintf(msg, sizeof(msg), "The card on line %d is a FR with I2 > 0 (stepped), but F2 is not a positive step.", i);
L1564:         snprintf(msg, sizeof(msg), "The card on line %d is a GW but has fewer than 2 integer inputs (tag, segments).", i);
L1572:           snprintf(msg, sizeof(msg), "The card on line %d is a GW with non-positive segment count.", i);
L1581:         snprintf(msg, sizeof(msg), "The card on line %d is a GW with identical endpoints (zero-length wire).", i);
L1588:         snprintf(msg, sizeof(msg), "The card on line %d is a GW but has no radius specified in F7.", i);
L1593:         snprintf(msg, sizeof(msg), "The card on line %d is a GW with non-positive radius in F7.", i);
L1603:             snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius, but the next card is not a GC.", i + 1);
L1609:           snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius and is the last card; a following GC is required.", i + 1);
L1621:         snprintf(msg, sizeof(msg), "The card on line %d is an RP but has fewer than 4 integer inputs.", i);
L1626:         snprintf(msg, sizeof(msg), "The card on line %d is an RP but has fewer than 4 floating-point inputs.", i);
L1634:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with non-positive NTHETA (I2).", i);
L1639:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with non-positive NPHI (I3).", i);
L1649:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with NTHETA > 1 but zero theta step (F3).", i);
L1654:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with NPHI > 1 but zero phi step (F4).", i);
L1660:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with theta start (F1) outside [-180,180].", i);
L1665:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with phi start (F2) outside [-360,360].", i);
L1671:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with an excessively large theta step (F3).", i);
L1676:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with an excessively large phi step (F4).", i);
L1685:           snprintf(msg, sizeof(msg), "The card on line %d is an RP whose theta sweep leaves the [-180,180] range.", i);
L1690:           snprintf(msg, sizeof(msg), "The card on line %d is an RP with NTHETA>1 and positive theta step but decreasing sweep.", i);
L1695:           snprintf(msg, sizeof(msg), "The card on line %d is an RP with NTHETA>1 and negative theta step but increasing sweep.", i);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 1705
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 1645-1710) ---
   1645:       double th_step = deck->cards[i].f[3];
   1646:       double ph_step = deck->cards[i].f[4];
   1647:       if (ntheta > 1 && th_step == 0.0)
   1648:       {
   1649:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with NTHETA > 1 but zero theta step (F3).", i);
   1650:         add_error(ctx, errors, msg, 0);
   1651:       }
   1652:       if (nphi > 1 && ph_step == 0.0)
   1653:       {
   1654:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with NPHI > 1 but zero phi step (F4).", i);
   1655:         add_error(ctx, errors, msg, 0);
   1656:       }
   1657:       // Basic angle sanity: starts within typical ranges
   1658:       if (!(th_start >= -180.0 && th_start <= 180.0))
   1659:       {
   1660:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with theta start (F1) outside [-180,180].", i);
   1661:         add_error(ctx, errors, msg, 0);
   1662:       }
   1663:       if (!(ph_start >= -360.0 && ph_start <= 360.0))
   1664:       {
   1665:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with phi start (F2) outside [-360,360].", i);
   1666:         add_error(ctx, errors, msg, 0);
   1667:       }
   1668:       // Step magnitudes should be reasonable
   1669:       if (fabs(th_step) > 180.0)
   1670:       {
   1671:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with an excessively large theta step (F3).", i);
   1672:         add_error(ctx, errors, msg, 0);
   1673:       }
   1674:       if (fabs(ph_step) > 360.0)
   1675:       {
   1676:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with an excessively large phi step (F4).", i);
   1677:         add_error(ctx, errors, msg, 0);
   1678:       }
   1679:       // Derived end angles should remain within sensible bounds and match step direction
   1680:       if (ntheta > 1)
   1681:       {
   1682:         double th_end = th_start + (ntheta - 1) * th_step;
   1683:         if (!(th_end >= -180.0 && th_end <= 180.0))
   1684:         {
   1685:           snprintf(msg, sizeof(msg), "The card on line %d is an RP whose theta sweep leaves the [-180,180] range.", i);
   1686:           add_error(ctx, errors, msg, 0);
   1687:         }
   1688:         if (th_step > 0.0 && th_end < th_start)
   1689:         {
   1690:           snprintf(msg, sizeof(msg), "The card on line %d is an RP with NTHETA>1 and positive theta step but decreasing sweep.", i);
   1691:           add_error(ctx, errors, msg, 0);
   1692:         }
   1693:         if (th_step < 0.0 && th_end > th_start)
   1694:         {
   1695:           snprintf(msg, sizeof(msg), "The card on line %d is an RP with NTHETA>1 and negative theta step but increasing sweep.", i);
   1696:           add_error(ctx, errors, msg, 0);
   1697:         }
   1698:       }
   1699:       if (nphi > 1)
   1700:       {
   1701:         double ph_end = ph_start + (nphi - 1) * ph_step;
   1702:         if (!(ph_end >= -720.0 && ph_end <= 720.0))
   1703:         {
   1704:           snprintf(msg, sizeof(msg), "The card on line %d is an RP whose phi sweep leaves a reasonable range.", i);
>  1705:           add_error(ctx, errors, msg, 0);
   1706:         }
   1707:         if (ph_step > 0.0 && ph_end < ph_start)
   1708:         {
   1709:           snprintf(msg, sizeof(msg), "The card on line %d is an RP with NPHI>1 and positive phi step but decreasing sweep.", i);
   1710:           add_error(ctx, errors, msg, 0);

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L1520:         snprintf(msg, sizeof(msg), "The card on line %d is a CE but has numeric inputs.", i);
L1530:         snprintf(msg, sizeof(msg), "The card on line %d is an EN but has numeric inputs.", i);
L1541:         snprintf(msg, sizeof(msg), "The card on line %d is a FR but has no base frequency in F1.", i);
L1547:         snprintf(msg, sizeof(msg), "The card on line %d is a FR with I2 = 0 (single frequency), but has a non-zero F2.", i);
L1553:         snprintf(msg, sizeof(msg), "The card on line %d is a FR with I2 > 0 (stepped), but F2 is not a positive step.", i);
L1564:         snprintf(msg, sizeof(msg), "The card on line %d is a GW but has fewer than 2 integer inputs (tag, segments).", i);
L1572:           snprintf(msg, sizeof(msg), "The card on line %d is a GW with non-positive segment count.", i);
L1581:         snprintf(msg, sizeof(msg), "The card on line %d is a GW with identical endpoints (zero-length wire).", i);
L1588:         snprintf(msg, sizeof(msg), "The card on line %d is a GW but has no radius specified in F7.", i);
L1593:         snprintf(msg, sizeof(msg), "The card on line %d is a GW with non-positive radius in F7.", i);
L1603:             snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius, but the next card is not a GC.", i + 1);
L1609:           snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius and is the last card; a following GC is required.", i + 1);
L1621:         snprintf(msg, sizeof(msg), "The card on line %d is an RP but has fewer than 4 integer inputs.", i);
L1626:         snprintf(msg, sizeof(msg), "The card on line %d is an RP but has fewer than 4 floating-point inputs.", i);
L1634:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with non-positive NTHETA (I2).", i);
L1639:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with non-positive NPHI (I3).", i);
L1649:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with NTHETA > 1 but zero theta step (F3).", i);
L1654:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with NPHI > 1 but zero phi step (F4).", i);
L1660:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with theta start (F1) outside [-180,180].", i);
L1665:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with phi start (F2) outside [-360,360].", i);
L1671:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with an excessively large theta step (F3).", i);
L1676:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with an excessively large phi step (F4).", i);
L1685:           snprintf(msg, sizeof(msg), "The card on line %d is an RP whose theta sweep leaves the [-180,180] range.", i);
L1690:           snprintf(msg, sizeof(msg), "The card on line %d is an RP with NTHETA>1 and positive theta step but decreasing sweep.", i);
L1695:           snprintf(msg, sizeof(msg), "The card on line %d is an RP with NTHETA>1 and negative theta step but increasing sweep.", i);
L1704:           snprintf(msg, sizeof(msg), "The card on line %d is an RP whose phi sweep leaves a reasonable range.", i);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 1710
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 1650-1715) ---
   1650:         add_error(ctx, errors, msg, 0);
   1651:       }
   1652:       if (nphi > 1 && ph_step == 0.0)
   1653:       {
   1654:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with NPHI > 1 but zero phi step (F4).", i);
   1655:         add_error(ctx, errors, msg, 0);
   1656:       }
   1657:       // Basic angle sanity: starts within typical ranges
   1658:       if (!(th_start >= -180.0 && th_start <= 180.0))
   1659:       {
   1660:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with theta start (F1) outside [-180,180].", i);
   1661:         add_error(ctx, errors, msg, 0);
   1662:       }
   1663:       if (!(ph_start >= -360.0 && ph_start <= 360.0))
   1664:       {
   1665:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with phi start (F2) outside [-360,360].", i);
   1666:         add_error(ctx, errors, msg, 0);
   1667:       }
   1668:       // Step magnitudes should be reasonable
   1669:       if (fabs(th_step) > 180.0)
   1670:       {
   1671:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with an excessively large theta step (F3).", i);
   1672:         add_error(ctx, errors, msg, 0);
   1673:       }
   1674:       if (fabs(ph_step) > 360.0)
   1675:       {
   1676:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with an excessively large phi step (F4).", i);
   1677:         add_error(ctx, errors, msg, 0);
   1678:       }
   1679:       // Derived end angles should remain within sensible bounds and match step direction
   1680:       if (ntheta > 1)
   1681:       {
   1682:         double th_end = th_start + (ntheta - 1) * th_step;
   1683:         if (!(th_end >= -180.0 && th_end <= 180.0))
   1684:         {
   1685:           snprintf(msg, sizeof(msg), "The card on line %d is an RP whose theta sweep leaves the [-180,180] range.", i);
   1686:           add_error(ctx, errors, msg, 0);
   1687:         }
   1688:         if (th_step > 0.0 && th_end < th_start)
   1689:         {
   1690:           snprintf(msg, sizeof(msg), "The card on line %d is an RP with NTHETA>1 and positive theta step but decreasing sweep.", i);
   1691:           add_error(ctx, errors, msg, 0);
   1692:         }
   1693:         if (th_step < 0.0 && th_end > th_start)
   1694:         {
   1695:           snprintf(msg, sizeof(msg), "The card on line %d is an RP with NTHETA>1 and negative theta step but increasing sweep.", i);
   1696:           add_error(ctx, errors, msg, 0);
   1697:         }
   1698:       }
   1699:       if (nphi > 1)
   1700:       {
   1701:         double ph_end = ph_start + (nphi - 1) * ph_step;
   1702:         if (!(ph_end >= -720.0 && ph_end <= 720.0))
   1703:         {
   1704:           snprintf(msg, sizeof(msg), "The card on line %d is an RP whose phi sweep leaves a reasonable range.", i);
   1705:           add_error(ctx, errors, msg, 0);
   1706:         }
   1707:         if (ph_step > 0.0 && ph_end < ph_start)
   1708:         {
   1709:           snprintf(msg, sizeof(msg), "The card on line %d is an RP with NPHI>1 and positive phi step but decreasing sweep.", i);
>  1710:           add_error(ctx, errors, msg, 0);
   1711:         }
   1712:         if (ph_step < 0.0 && ph_end > ph_start)
   1713:         {
   1714:           snprintf(msg, sizeof(msg), "The card on line %d is an RP with NPHI>1 and negative phi step but increasing sweep.", i);
   1715:           add_error(ctx, errors, msg, 0);

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L1520:         snprintf(msg, sizeof(msg), "The card on line %d is a CE but has numeric inputs.", i);
L1530:         snprintf(msg, sizeof(msg), "The card on line %d is an EN but has numeric inputs.", i);
L1541:         snprintf(msg, sizeof(msg), "The card on line %d is a FR but has no base frequency in F1.", i);
L1547:         snprintf(msg, sizeof(msg), "The card on line %d is a FR with I2 = 0 (single frequency), but has a non-zero F2.", i);
L1553:         snprintf(msg, sizeof(msg), "The card on line %d is a FR with I2 > 0 (stepped), but F2 is not a positive step.", i);
L1564:         snprintf(msg, sizeof(msg), "The card on line %d is a GW but has fewer than 2 integer inputs (tag, segments).", i);
L1572:           snprintf(msg, sizeof(msg), "The card on line %d is a GW with non-positive segment count.", i);
L1581:         snprintf(msg, sizeof(msg), "The card on line %d is a GW with identical endpoints (zero-length wire).", i);
L1588:         snprintf(msg, sizeof(msg), "The card on line %d is a GW but has no radius specified in F7.", i);
L1593:         snprintf(msg, sizeof(msg), "The card on line %d is a GW with non-positive radius in F7.", i);
L1603:             snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius, but the next card is not a GC.", i + 1);
L1609:           snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius and is the last card; a following GC is required.", i + 1);
L1621:         snprintf(msg, sizeof(msg), "The card on line %d is an RP but has fewer than 4 integer inputs.", i);
L1626:         snprintf(msg, sizeof(msg), "The card on line %d is an RP but has fewer than 4 floating-point inputs.", i);
L1634:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with non-positive NTHETA (I2).", i);
L1639:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with non-positive NPHI (I3).", i);
L1649:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with NTHETA > 1 but zero theta step (F3).", i);
L1654:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with NPHI > 1 but zero phi step (F4).", i);
L1660:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with theta start (F1) outside [-180,180].", i);
L1665:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with phi start (F2) outside [-360,360].", i);
L1671:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with an excessively large theta step (F3).", i);
L1676:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with an excessively large phi step (F4).", i);
L1685:           snprintf(msg, sizeof(msg), "The card on line %d is an RP whose theta sweep leaves the [-180,180] range.", i);
L1690:           snprintf(msg, sizeof(msg), "The card on line %d is an RP with NTHETA>1 and positive theta step but decreasing sweep.", i);
L1695:           snprintf(msg, sizeof(msg), "The card on line %d is an RP with NTHETA>1 and negative theta step but increasing sweep.", i);
L1704:           snprintf(msg, sizeof(msg), "The card on line %d is an RP whose phi sweep leaves a reasonable range.", i);
L1709:           snprintf(msg, sizeof(msg), "The card on line %d is an RP with NPHI>1 and positive phi step but decreasing sweep.", i);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 1715
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 1655-1720) ---
   1655:         add_error(ctx, errors, msg, 0);
   1656:       }
   1657:       // Basic angle sanity: starts within typical ranges
   1658:       if (!(th_start >= -180.0 && th_start <= 180.0))
   1659:       {
   1660:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with theta start (F1) outside [-180,180].", i);
   1661:         add_error(ctx, errors, msg, 0);
   1662:       }
   1663:       if (!(ph_start >= -360.0 && ph_start <= 360.0))
   1664:       {
   1665:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with phi start (F2) outside [-360,360].", i);
   1666:         add_error(ctx, errors, msg, 0);
   1667:       }
   1668:       // Step magnitudes should be reasonable
   1669:       if (fabs(th_step) > 180.0)
   1670:       {
   1671:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with an excessively large theta step (F3).", i);
   1672:         add_error(ctx, errors, msg, 0);
   1673:       }
   1674:       if (fabs(ph_step) > 360.0)
   1675:       {
   1676:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with an excessively large phi step (F4).", i);
   1677:         add_error(ctx, errors, msg, 0);
   1678:       }
   1679:       // Derived end angles should remain within sensible bounds and match step direction
   1680:       if (ntheta > 1)
   1681:       {
   1682:         double th_end = th_start + (ntheta - 1) * th_step;
   1683:         if (!(th_end >= -180.0 && th_end <= 180.0))
   1684:         {
   1685:           snprintf(msg, sizeof(msg), "The card on line %d is an RP whose theta sweep leaves the [-180,180] range.", i);
   1686:           add_error(ctx, errors, msg, 0);
   1687:         }
   1688:         if (th_step > 0.0 && th_end < th_start)
   1689:         {
   1690:           snprintf(msg, sizeof(msg), "The card on line %d is an RP with NTHETA>1 and positive theta step but decreasing sweep.", i);
   1691:           add_error(ctx, errors, msg, 0);
   1692:         }
   1693:         if (th_step < 0.0 && th_end > th_start)
   1694:         {
   1695:           snprintf(msg, sizeof(msg), "The card on line %d is an RP with NTHETA>1 and negative theta step but increasing sweep.", i);
   1696:           add_error(ctx, errors, msg, 0);
   1697:         }
   1698:       }
   1699:       if (nphi > 1)
   1700:       {
   1701:         double ph_end = ph_start + (nphi - 1) * ph_step;
   1702:         if (!(ph_end >= -720.0 && ph_end <= 720.0))
   1703:         {
   1704:           snprintf(msg, sizeof(msg), "The card on line %d is an RP whose phi sweep leaves a reasonable range.", i);
   1705:           add_error(ctx, errors, msg, 0);
   1706:         }
   1707:         if (ph_step > 0.0 && ph_end < ph_start)
   1708:         {
   1709:           snprintf(msg, sizeof(msg), "The card on line %d is an RP with NPHI>1 and positive phi step but decreasing sweep.", i);
   1710:           add_error(ctx, errors, msg, 0);
   1711:         }
   1712:         if (ph_step < 0.0 && ph_end > ph_start)
   1713:         {
   1714:           snprintf(msg, sizeof(msg), "The card on line %d is an RP with NPHI>1 and negative phi step but increasing sweep.", i);
>  1715:           add_error(ctx, errors, msg, 0);
   1716:         }
   1717:       }
   1718:     }
   1719:   }
   1720: }

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L1520:         snprintf(msg, sizeof(msg), "The card on line %d is a CE but has numeric inputs.", i);
L1530:         snprintf(msg, sizeof(msg), "The card on line %d is an EN but has numeric inputs.", i);
L1541:         snprintf(msg, sizeof(msg), "The card on line %d is a FR but has no base frequency in F1.", i);
L1547:         snprintf(msg, sizeof(msg), "The card on line %d is a FR with I2 = 0 (single frequency), but has a non-zero F2.", i);
L1553:         snprintf(msg, sizeof(msg), "The card on line %d is a FR with I2 > 0 (stepped), but F2 is not a positive step.", i);
L1564:         snprintf(msg, sizeof(msg), "The card on line %d is a GW but has fewer than 2 integer inputs (tag, segments).", i);
L1572:           snprintf(msg, sizeof(msg), "The card on line %d is a GW with non-positive segment count.", i);
L1581:         snprintf(msg, sizeof(msg), "The card on line %d is a GW with identical endpoints (zero-length wire).", i);
L1588:         snprintf(msg, sizeof(msg), "The card on line %d is a GW but has no radius specified in F7.", i);
L1593:         snprintf(msg, sizeof(msg), "The card on line %d is a GW with non-positive radius in F7.", i);
L1603:             snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius, but the next card is not a GC.", i + 1);
L1609:           snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius and is the last card; a following GC is required.", i + 1);
L1621:         snprintf(msg, sizeof(msg), "The card on line %d is an RP but has fewer than 4 integer inputs.", i);
L1626:         snprintf(msg, sizeof(msg), "The card on line %d is an RP but has fewer than 4 floating-point inputs.", i);
L1634:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with non-positive NTHETA (I2).", i);
L1639:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with non-positive NPHI (I3).", i);
L1649:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with NTHETA > 1 but zero theta step (F3).", i);
L1654:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with NPHI > 1 but zero phi step (F4).", i);
L1660:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with theta start (F1) outside [-180,180].", i);
L1665:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with phi start (F2) outside [-360,360].", i);
L1671:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with an excessively large theta step (F3).", i);
L1676:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with an excessively large phi step (F4).", i);
L1685:           snprintf(msg, sizeof(msg), "The card on line %d is an RP whose theta sweep leaves the [-180,180] range.", i);
L1690:           snprintf(msg, sizeof(msg), "The card on line %d is an RP with NTHETA>1 and positive theta step but decreasing sweep.", i);
L1695:           snprintf(msg, sizeof(msg), "The card on line %d is an RP with NTHETA>1 and negative theta step but increasing sweep.", i);
L1704:           snprintf(msg, sizeof(msg), "The card on line %d is an RP whose phi sweep leaves a reasonable range.", i);
L1709:           snprintf(msg, sizeof(msg), "The card on line %d is an RP with NPHI>1 and positive phi step but decreasing sweep.", i);
L1714:           snprintf(msg, sizeof(msg), "The card on line %d is an RP with NPHI>1 and negative phi step but increasing sweep.", i);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 1762
CALL: add_error(ctx, errors, msg, WARNING);
--- Context (lines 1702-1767) ---
   1702:         if (!(ph_end >= -720.0 && ph_end <= 720.0))
   1703:         {
   1704:           snprintf(msg, sizeof(msg), "The card on line %d is an RP whose phi sweep leaves a reasonable range.", i);
   1705:           add_error(ctx, errors, msg, 0);
   1706:         }
   1707:         if (ph_step > 0.0 && ph_end < ph_start)
   1708:         {
   1709:           snprintf(msg, sizeof(msg), "The card on line %d is an RP with NPHI>1 and positive phi step but decreasing sweep.", i);
   1710:           add_error(ctx, errors, msg, 0);
   1711:         }
   1712:         if (ph_step < 0.0 && ph_end > ph_start)
   1713:         {
   1714:           snprintf(msg, sizeof(msg), "The card on line %d is an RP with NPHI>1 and negative phi step but increasing sweep.", i);
   1715:           add_error(ctx, errors, msg, 0);
   1716:         }
   1717:       }
   1718:     }
   1719:   }
   1720: }
   1721: /* end test_card_inputs */
   1722: 
   1723: /******************************************************************************
   1724:  * test_bad_symbols
   1725:  *
   1726:  * looks at all the SYmbol cards, if any, and warns if they override one of
   1727:  * the system-wide symbols like "mm" or "awg".
   1728:  *
   1729:  * also warns about duplicate definitions, as only the last value will be used
   1730:  * NOTE: is this correct? can you define HEIGHT=7 and then 14 lower in the deck?
   1731:  *
   1732:  * @param deck the deck_t to be tested
   1733:  * @param errors the errors_list_t to add new messages to
   1734:  *
   1735:  */
   1736: void test_bad_symbols(const nec_context_t *ctx, deck_t *deck, errors_list_t *errors)
   1737: {
   1738:   char msg[MAX_ERROR_LEN];
   1739: 
   1740:   // List of default symbol names (pi, c, and unit constants)
   1741:   // Note: AWG symbols (awg0-awg40) are also defaults but checked separately below
   1742:   const char *default_symbols[] = {
   1743:       "pi", "c", "m", "cm", "mm", "ft", "in", "mil",
   1744:       "pf", "nf", "uf", "nh", "uh", "mh"};
   1745:   int num_defaults = sizeof(default_symbols) / sizeof(default_symbols[0]);
   1746: 
   1747:   // Check if any user-defined symbols override defaults
   1748:   for (int i = 0; i < deck->num_cards; i++)
   1749:   {
   1750:     card_t *card = &deck->cards[i];
   1751:     if (strcmp(card->card_code, "SY") == 0 && card->formulas)
   1752:     {
   1753:       key_value_t *kv = card->formulas;
   1754:       while (kv)
   1755:       {
   1756:         // Check against standard defaults
   1757:         for (int d = 0; d < num_defaults; d++)
   1758:         {
   1759:           if (strcasecmp(kv->key, default_symbols[d]) == 0)
   1760:           {
   1761:             snprintf(msg, sizeof(msg), "The symbol '%s' overrides a default symbol. This may cause unexpected results.", kv->key);
>  1762:             add_error(ctx, errors, msg, WARNING);
   1763:           }
   1764:         }
   1765: 
   1766:         // Check against AWG symbols (awg0-awg40)
   1767:         if (strlen(kv->key) >= 4 && strlen(kv->key) <= 5)

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L1564:         snprintf(msg, sizeof(msg), "The card on line %d is a GW but has fewer than 2 integer inputs (tag, segments).", i);
L1572:           snprintf(msg, sizeof(msg), "The card on line %d is a GW with non-positive segment count.", i);
L1581:         snprintf(msg, sizeof(msg), "The card on line %d is a GW with identical endpoints (zero-length wire).", i);
L1588:         snprintf(msg, sizeof(msg), "The card on line %d is a GW but has no radius specified in F7.", i);
L1593:         snprintf(msg, sizeof(msg), "The card on line %d is a GW with non-positive radius in F7.", i);
L1603:             snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius, but the next card is not a GC.", i + 1);
L1609:           snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius and is the last card; a following GC is required.", i + 1);
L1621:         snprintf(msg, sizeof(msg), "The card on line %d is an RP but has fewer than 4 integer inputs.", i);
L1626:         snprintf(msg, sizeof(msg), "The card on line %d is an RP but has fewer than 4 floating-point inputs.", i);
L1634:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with non-positive NTHETA (I2).", i);
L1639:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with non-positive NPHI (I3).", i);
L1649:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with NTHETA > 1 but zero theta step (F3).", i);
L1654:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with NPHI > 1 but zero phi step (F4).", i);
L1660:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with theta start (F1) outside [-180,180].", i);
L1665:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with phi start (F2) outside [-360,360].", i);
L1671:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with an excessively large theta step (F3).", i);
L1676:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with an excessively large phi step (F4).", i);
L1685:           snprintf(msg, sizeof(msg), "The card on line %d is an RP whose theta sweep leaves the [-180,180] range.", i);
L1690:           snprintf(msg, sizeof(msg), "The card on line %d is an RP with NTHETA>1 and positive theta step but decreasing sweep.", i);
L1695:           snprintf(msg, sizeof(msg), "The card on line %d is an RP with NTHETA>1 and negative theta step but increasing sweep.", i);
L1704:           snprintf(msg, sizeof(msg), "The card on line %d is an RP whose phi sweep leaves a reasonable range.", i);
L1709:           snprintf(msg, sizeof(msg), "The card on line %d is an RP with NPHI>1 and positive phi step but decreasing sweep.", i);
L1714:           snprintf(msg, sizeof(msg), "The card on line %d is an RP with NPHI>1 and negative phi step but increasing sweep.", i);
L1761:             snprintf(msg, sizeof(msg), "The symbol '%s' overrides a default symbol. This may cause unexpected results.", kv->key);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 1777
CALL: add_error(ctx, errors, msg, WARNING);
--- Context (lines 1717-1782) ---
   1717:       }
   1718:     }
   1719:   }
   1720: }
   1721: /* end test_card_inputs */
   1722: 
   1723: /******************************************************************************
   1724:  * test_bad_symbols
   1725:  *
   1726:  * looks at all the SYmbol cards, if any, and warns if they override one of
   1727:  * the system-wide symbols like "mm" or "awg".
   1728:  *
   1729:  * also warns about duplicate definitions, as only the last value will be used
   1730:  * NOTE: is this correct? can you define HEIGHT=7 and then 14 lower in the deck?
   1731:  *
   1732:  * @param deck the deck_t to be tested
   1733:  * @param errors the errors_list_t to add new messages to
   1734:  *
   1735:  */
   1736: void test_bad_symbols(const nec_context_t *ctx, deck_t *deck, errors_list_t *errors)
   1737: {
   1738:   char msg[MAX_ERROR_LEN];
   1739: 
   1740:   // List of default symbol names (pi, c, and unit constants)
   1741:   // Note: AWG symbols (awg0-awg40) are also defaults but checked separately below
   1742:   const char *default_symbols[] = {
   1743:       "pi", "c", "m", "cm", "mm", "ft", "in", "mil",
   1744:       "pf", "nf", "uf", "nh", "uh", "mh"};
   1745:   int num_defaults = sizeof(default_symbols) / sizeof(default_symbols[0]);
   1746: 
   1747:   // Check if any user-defined symbols override defaults
   1748:   for (int i = 0; i < deck->num_cards; i++)
   1749:   {
   1750:     card_t *card = &deck->cards[i];
   1751:     if (strcmp(card->card_code, "SY") == 0 && card->formulas)
   1752:     {
   1753:       key_value_t *kv = card->formulas;
   1754:       while (kv)
   1755:       {
   1756:         // Check against standard defaults
   1757:         for (int d = 0; d < num_defaults; d++)
   1758:         {
   1759:           if (strcasecmp(kv->key, default_symbols[d]) == 0)
   1760:           {
   1761:             snprintf(msg, sizeof(msg), "The symbol '%s' overrides a default symbol. This may cause unexpected results.", kv->key);
   1762:             add_error(ctx, errors, msg, WARNING);
   1763:           }
   1764:         }
   1765: 
   1766:         // Check against AWG symbols (awg0-awg40)
   1767:         if (strlen(kv->key) >= 4 && strlen(kv->key) <= 5)
   1768:         {
   1769:           if ((kv->key[0] == 'a' || kv->key[0] == 'A') &&
   1770:               (kv->key[1] == 'w' || kv->key[1] == 'W') &&
   1771:               (kv->key[2] == 'g' || kv->key[2] == 'G'))
   1772:           {
   1773:             int awg_num = atoi(&kv->key[3]);
   1774:             if (awg_num >= 0 && awg_num <= 40)
   1775:             {
   1776:               snprintf(msg, sizeof(msg), "The symbol '%s' overrides a default AWG constant. This may cause unexpected results.", kv->key);
>  1777:               add_error(ctx, errors, msg, WARNING);
   1778:             }
   1779:           }
   1780:         }
   1781: 
   1782:         kv = kv->next;

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L1581:         snprintf(msg, sizeof(msg), "The card on line %d is a GW with identical endpoints (zero-length wire).", i);
L1588:         snprintf(msg, sizeof(msg), "The card on line %d is a GW but has no radius specified in F7.", i);
L1593:         snprintf(msg, sizeof(msg), "The card on line %d is a GW with non-positive radius in F7.", i);
L1603:             snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius, but the next card is not a GC.", i + 1);
L1609:           snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius and is the last card; a following GC is required.", i + 1);
L1621:         snprintf(msg, sizeof(msg), "The card on line %d is an RP but has fewer than 4 integer inputs.", i);
L1626:         snprintf(msg, sizeof(msg), "The card on line %d is an RP but has fewer than 4 floating-point inputs.", i);
L1634:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with non-positive NTHETA (I2).", i);
L1639:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with non-positive NPHI (I3).", i);
L1649:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with NTHETA > 1 but zero theta step (F3).", i);
L1654:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with NPHI > 1 but zero phi step (F4).", i);
L1660:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with theta start (F1) outside [-180,180].", i);
L1665:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with phi start (F2) outside [-360,360].", i);
L1671:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with an excessively large theta step (F3).", i);
L1676:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with an excessively large phi step (F4).", i);
L1685:           snprintf(msg, sizeof(msg), "The card on line %d is an RP whose theta sweep leaves the [-180,180] range.", i);
L1690:           snprintf(msg, sizeof(msg), "The card on line %d is an RP with NTHETA>1 and positive theta step but decreasing sweep.", i);
L1695:           snprintf(msg, sizeof(msg), "The card on line %d is an RP with NTHETA>1 and negative theta step but increasing sweep.", i);
L1704:           snprintf(msg, sizeof(msg), "The card on line %d is an RP whose phi sweep leaves a reasonable range.", i);
L1709:           snprintf(msg, sizeof(msg), "The card on line %d is an RP with NPHI>1 and positive phi step but decreasing sweep.", i);
L1714:           snprintf(msg, sizeof(msg), "The card on line %d is an RP with NPHI>1 and negative phi step but increasing sweep.", i);
L1761:             snprintf(msg, sizeof(msg), "The symbol '%s' overrides a default symbol. This may cause unexpected results.", kv->key);
L1776:               snprintf(msg, sizeof(msg), "The symbol '%s' overrides a default AWG constant. This may cause unexpected results.", kv->key);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 1804
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 1744-1809) ---
   1744:       "pf", "nf", "uf", "nh", "uh", "mh"};
   1745:   int num_defaults = sizeof(default_symbols) / sizeof(default_symbols[0]);
   1746: 
   1747:   // Check if any user-defined symbols override defaults
   1748:   for (int i = 0; i < deck->num_cards; i++)
   1749:   {
   1750:     card_t *card = &deck->cards[i];
   1751:     if (strcmp(card->card_code, "SY") == 0 && card->formulas)
   1752:     {
   1753:       key_value_t *kv = card->formulas;
   1754:       while (kv)
   1755:       {
   1756:         // Check against standard defaults
   1757:         for (int d = 0; d < num_defaults; d++)
   1758:         {
   1759:           if (strcasecmp(kv->key, default_symbols[d]) == 0)
   1760:           {
   1761:             snprintf(msg, sizeof(msg), "The symbol '%s' overrides a default symbol. This may cause unexpected results.", kv->key);
   1762:             add_error(ctx, errors, msg, WARNING);
   1763:           }
   1764:         }
   1765: 
   1766:         // Check against AWG symbols (awg0-awg40)
   1767:         if (strlen(kv->key) >= 4 && strlen(kv->key) <= 5)
   1768:         {
   1769:           if ((kv->key[0] == 'a' || kv->key[0] == 'A') &&
   1770:               (kv->key[1] == 'w' || kv->key[1] == 'W') &&
   1771:               (kv->key[2] == 'g' || kv->key[2] == 'G'))
   1772:           {
   1773:             int awg_num = atoi(&kv->key[3]);
   1774:             if (awg_num >= 0 && awg_num <= 40)
   1775:             {
   1776:               snprintf(msg, sizeof(msg), "The symbol '%s' overrides a default AWG constant. This may cause unexpected results.", kv->key);
   1777:               add_error(ctx, errors, msg, WARNING);
   1778:             }
   1779:           }
   1780:         }
   1781: 
   1782:         kv = kv->next;
   1783:       }
   1784:     }
   1785:   }
   1786: 
   1787:   // Check for duplicate symbol names
   1788:   for (int i = 0; i < deck->num_symbols; i++)
   1789:   {
   1790:     key_value_t *outer = deck->symbols[i];
   1791:     if (outer == NULL)
   1792:       continue;
   1793: 
   1794:     // Check if any other symbol has the same name
   1795:     for (int k = i + 1; k < deck->num_symbols; k++)
   1796:     {
   1797:       key_value_t *inner = deck->symbols[k];
   1798:       if (inner == NULL)
   1799:         continue;
   1800: 
   1801:       if (strcasecmp(outer->key, inner->key) == 0)
   1802:       {
   1803:         snprintf(msg, sizeof(msg), "The symbol '%s' has been defined more than once.", outer->key);
>  1804:         add_error(ctx, errors, msg, 0);
   1805:       }
   1806:     }
   1807:   }
   1808: } /* end of test_bad_symbols */
   1809: 

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L1609:           snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius and is the last card; a following GC is required.", i + 1);
L1621:         snprintf(msg, sizeof(msg), "The card on line %d is an RP but has fewer than 4 integer inputs.", i);
L1626:         snprintf(msg, sizeof(msg), "The card on line %d is an RP but has fewer than 4 floating-point inputs.", i);
L1634:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with non-positive NTHETA (I2).", i);
L1639:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with non-positive NPHI (I3).", i);
L1649:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with NTHETA > 1 but zero theta step (F3).", i);
L1654:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with NPHI > 1 but zero phi step (F4).", i);
L1660:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with theta start (F1) outside [-180,180].", i);
L1665:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with phi start (F2) outside [-360,360].", i);
L1671:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with an excessively large theta step (F3).", i);
L1676:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with an excessively large phi step (F4).", i);
L1685:           snprintf(msg, sizeof(msg), "The card on line %d is an RP whose theta sweep leaves the [-180,180] range.", i);
L1690:           snprintf(msg, sizeof(msg), "The card on line %d is an RP with NTHETA>1 and positive theta step but decreasing sweep.", i);
L1695:           snprintf(msg, sizeof(msg), "The card on line %d is an RP with NTHETA>1 and negative theta step but increasing sweep.", i);
L1704:           snprintf(msg, sizeof(msg), "The card on line %d is an RP whose phi sweep leaves a reasonable range.", i);
L1709:           snprintf(msg, sizeof(msg), "The card on line %d is an RP with NPHI>1 and positive phi step but decreasing sweep.", i);
L1714:           snprintf(msg, sizeof(msg), "The card on line %d is an RP with NPHI>1 and negative phi step but increasing sweep.", i);
L1761:             snprintf(msg, sizeof(msg), "The symbol '%s' overrides a default symbol. This may cause unexpected results.", kv->key);
L1776:               snprintf(msg, sizeof(msg), "The symbol '%s' overrides a default AWG constant. This may cause unexpected results.", kv->key);
L1803:         snprintf(msg, sizeof(msg), "The symbol '%s' has been defined more than once.", outer->key);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 1856
CALL: add_error(ctx, errors, msg, WARNING);
--- Context (lines 1796-1861) ---
   1796:     {
   1797:       key_value_t *inner = deck->symbols[k];
   1798:       if (inner == NULL)
   1799:         continue;
   1800: 
   1801:       if (strcasecmp(outer->key, inner->key) == 0)
   1802:       {
   1803:         snprintf(msg, sizeof(msg), "The symbol '%s' has been defined more than once.", outer->key);
   1804:         add_error(ctx, errors, msg, 0);
   1805:       }
   1806:     }
   1807:   }
   1808: } /* end of test_bad_symbols */
   1809: 
   1810: /******************************************************************************
   1811:  * test_field_separators
   1812:  *
   1813:  * Checks whether all cards in the geometry section use the same field
   1814:  * separator style, and likewise for the control section.  Mixed separators
   1815:  * within a section produce a warning — the deck will still calculate, but
   1816:  * it suggests the file was edited inconsistently and may cause problems
   1817:  * for any output code attempting to preserve the original formatting.
   1818:  *
   1819:  * @param ctx  the nec_context_t (used for error reporting)
   1820:  * @param deck the deck_t to test
   1821:  * @param errors the errors_list_t to append warnings to
   1822:  */
   1823: void test_field_separators(const nec_context_t *ctx, const deck_t *deck, errors_list_t *errors)
   1824: {
   1825:   char msg[MAX_ERROR_LEN];
   1826: 
   1827:   // helper: scan a range of cards for separator consistency
   1828:   // returns false (and adds a warning) if mixed separators are found
   1829:   int geo_end = (deck->geometry_end >= 0) ? deck->geometry_end : deck->num_cards - 1;
   1830:   int ctrl_end = (deck->deck_end >= 0) ? deck->deck_end : deck->num_cards - 1;
   1831:   int ctrl_start = geo_end + 1;
   1832: 
   1833:   // --- geometry section ---
   1834:   if (deck->geometry_start >= 0)
   1835:   {
   1836:     field_sep_t first_sep = FSEP_UNKNOWN;
   1837:     int first_sep_idx = -1;
   1838:     for (int i = deck->geometry_start; i <= geo_end; i++)
   1839:     {
   1840:       const card_t *c = &deck->cards[i];
   1841:       if (!is_geometry(c))
   1842:         continue;
   1843:       if (c->field_sep == FSEP_UNKNOWN)
   1844:         continue;
   1845:       if (first_sep == FSEP_UNKNOWN)
   1846:       {
   1847:         first_sep = c->field_sep;
   1848:         first_sep_idx = i + 1; // 1-based for message
   1849:       }
   1850:       else if (c->field_sep != first_sep)
   1851:       {
   1852:         snprintf(msg, sizeof(msg),
   1853:                  "Geometry section has mixed field separators: card %d uses a different style "
   1854:                  "from card %d. Output formatting may not preserve the original file style.",
   1855:                  i + 1, first_sep_idx);
>  1856:         add_error(ctx, errors, msg, WARNING);
   1857:         break; // one warning per section is enough
   1858:       }
   1859:     }
   1860:   }
   1861: 

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L1660:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with theta start (F1) outside [-180,180].", i);
L1665:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with phi start (F2) outside [-360,360].", i);
L1671:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with an excessively large theta step (F3).", i);
L1676:         snprintf(msg, sizeof(msg), "The card on line %d is an RP with an excessively large phi step (F4).", i);
L1685:           snprintf(msg, sizeof(msg), "The card on line %d is an RP whose theta sweep leaves the [-180,180] range.", i);
L1690:           snprintf(msg, sizeof(msg), "The card on line %d is an RP with NTHETA>1 and positive theta step but decreasing sweep.", i);
L1695:           snprintf(msg, sizeof(msg), "The card on line %d is an RP with NTHETA>1 and negative theta step but increasing sweep.", i);
L1704:           snprintf(msg, sizeof(msg), "The card on line %d is an RP whose phi sweep leaves a reasonable range.", i);
L1709:           snprintf(msg, sizeof(msg), "The card on line %d is an RP with NPHI>1 and positive phi step but decreasing sweep.", i);
L1714:           snprintf(msg, sizeof(msg), "The card on line %d is an RP with NPHI>1 and negative phi step but increasing sweep.", i);
L1761:             snprintf(msg, sizeof(msg), "The symbol '%s' overrides a default symbol. This may cause unexpected results.", kv->key);
L1776:               snprintf(msg, sizeof(msg), "The symbol '%s' overrides a default AWG constant. This may cause unexpected results.", kv->key);
L1803:         snprintf(msg, sizeof(msg), "The symbol '%s' has been defined more than once.", outer->key);
L1852:         snprintf(msg, sizeof(msg),

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck_validations.c
LINE: 1885
CALL: add_error(ctx, errors, msg, WARNING);
--- Context (lines 1825-1890) ---
   1825:   char msg[MAX_ERROR_LEN];
   1826: 
   1827:   // helper: scan a range of cards for separator consistency
   1828:   // returns false (and adds a warning) if mixed separators are found
   1829:   int geo_end = (deck->geometry_end >= 0) ? deck->geometry_end : deck->num_cards - 1;
   1830:   int ctrl_end = (deck->deck_end >= 0) ? deck->deck_end : deck->num_cards - 1;
   1831:   int ctrl_start = geo_end + 1;
   1832: 
   1833:   // --- geometry section ---
   1834:   if (deck->geometry_start >= 0)
   1835:   {
   1836:     field_sep_t first_sep = FSEP_UNKNOWN;
   1837:     int first_sep_idx = -1;
   1838:     for (int i = deck->geometry_start; i <= geo_end; i++)
   1839:     {
   1840:       const card_t *c = &deck->cards[i];
   1841:       if (!is_geometry(c))
   1842:         continue;
   1843:       if (c->field_sep == FSEP_UNKNOWN)
   1844:         continue;
   1845:       if (first_sep == FSEP_UNKNOWN)
   1846:       {
   1847:         first_sep = c->field_sep;
   1848:         first_sep_idx = i + 1; // 1-based for message
   1849:       }
   1850:       else if (c->field_sep != first_sep)
   1851:       {
   1852:         snprintf(msg, sizeof(msg),
   1853:                  "Geometry section has mixed field separators: card %d uses a different style "
   1854:                  "from card %d. Output formatting may not preserve the original file style.",
   1855:                  i + 1, first_sep_idx);
   1856:         add_error(ctx, errors, msg, WARNING);
   1857:         break; // one warning per section is enough
   1858:       }
   1859:     }
   1860:   }
   1861: 
   1862:   // --- control section ---
   1863:   if (ctrl_start < deck->num_cards)
   1864:   {
   1865:     field_sep_t first_sep = FSEP_UNKNOWN;
   1866:     int first_sep_idx = -1;
   1867:     for (int i = ctrl_start; i <= ctrl_end; i++)
   1868:     {
   1869:       const card_t *c = &deck->cards[i];
   1870:       if (!is_control(c))
   1871:         continue;
   1872:       if (c->field_sep == FSEP_UNKNOWN)
   1873:         continue;
   1874:       if (first_sep == FSEP_UNKNOWN)
   1875:       {
   1876:         first_sep = c->field_sep;
   1877:         first_sep_idx = i + 1;
   1878:       }
   1879:       else if (c->field_sep != first_sep)
   1880:       {
   1881:         snprintf(msg, sizeof(msg),
   1882:                  "Control section has mixed field separators: card %d uses a different style "
   1883:                  "from card %d. Output formatting may not preserve the original file style.",
   1884:                  i + 1, first_sep_idx);
>  1885:         add_error(ctx, errors, msg, WARNING);
   1886:         break;
   1887:       }
   1888:     }
   1889:   }
   1890: } /* end of test_field_separators */

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L1685:           snprintf(msg, sizeof(msg), "The card on line %d is an RP whose theta sweep leaves the [-180,180] range.", i);
L1690:           snprintf(msg, sizeof(msg), "The card on line %d is an RP with NTHETA>1 and positive theta step but decreasing sweep.", i);
L1695:           snprintf(msg, sizeof(msg), "The card on line %d is an RP with NTHETA>1 and negative theta step but increasing sweep.", i);
L1704:           snprintf(msg, sizeof(msg), "The card on line %d is an RP whose phi sweep leaves a reasonable range.", i);
L1709:           snprintf(msg, sizeof(msg), "The card on line %d is an RP with NPHI>1 and positive phi step but decreasing sweep.", i);
L1714:           snprintf(msg, sizeof(msg), "The card on line %d is an RP with NPHI>1 and negative phi step but increasing sweep.", i);
L1761:             snprintf(msg, sizeof(msg), "The symbol '%s' overrides a default symbol. This may cause unexpected results.", kv->key);
L1776:               snprintf(msg, sizeof(msg), "The symbol '%s' overrides a default AWG constant. This may cause unexpected results.", kv->key);
L1803:         snprintf(msg, sizeof(msg), "The symbol '%s' has been defined more than once.", outer->key);
L1852:         snprintf(msg, sizeof(msg),
L1881:         snprintf(msg, sizeof(msg),

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/misc.h
LINE: 19
CALL: void add_error(const nec_context_t *ctx, errors_list_t *errors, char *message, int severity);
--- Context (lines 1-24) ---
      1: /*
      2:  * misc.h - Core utility functions for OpenNEC
      3:  * 
      4:  * Memory management, error handling, and timing utilities
      5:  * used throughout the codebase.
      6:  */
      7: 
      8: #ifndef MISC_H
      9: #define MISC_H
     10: 
     11: #include "types.h"
     12: 
     13: /* Memory management */
     14: int mem_alloc(const nec_context_t *ctx, void **ptr, size_t req);
     15: int mem_realloc(const nec_context_t *ctx, void **ptr, size_t req);
     16: void mem_free(const nec_context_t *ctx, void **ptr);
     17: 
     18: /* Error and message handling */
>    19: void add_error(const nec_context_t *ctx, errors_list_t *errors, char *message, int severity);
     20: void add_message(const nec_context_t *ctx, outputs_list_t *outputs, char *message);
     21: void transfer_errors(errors_list_t *src, errors_list_t *dst);
     22: 
     23: /* Path utilities */
     24: /**

-- Message variable name candidates --
VAR_EXPR: char *message  VAR_NAME: *message
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
NONE FOUND - manual review needed

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/geometry.c
LINE: 169
CALL: add_error(ctx, errors, msg, WARNING);
--- Context (lines 109-174) ---
    109:     // a flag saying whether this card should be ignored. That makes it easy to
    110:     // have a GUI with a switch to turn off a card during testing (for example)
    111:     // without having to physically remove it from the deck. this is not the same
    112:     // as commenting it out, because the card is still read and parsed, and the
    113:     // segments are in the geometry and can still be used in a GUI
    114:     //
    115:     // Commented-out cards (leading marker like '!' or ''') are skipped entirely —
    116:     // they produce no geometry at all, not even in ignored_geometry.
    117:     if (card_is_commented_out(card)) continue;
    118: 
    119:     // Invisible cards (annotated ignore=true but no leading marker) still generate
    120:     // geometry, routed to ignored_geometry so the GUI can display them.
    121:     geometry_t *target_geom = card_is_invisible(card) ? &ctx->ignored_geometry : &ctx->geometry;
    122:     
    123:     // convert the code into its numeric value so we can switch on it
    124:     for(code_num = 0; code_num < NUM_GEOMETRY_CODES; code_num++) {
    125:       if(strncmp(card->card_code, geometry_codes[code_num], 2) == 0) break;
    126:     }
    127:     
    128:     // ignore SY and other extension cards in the geometry section
    129:     // they were already evaluated in update_deck_values()
    130:     if (is_extension(card)) continue;
    131: 
    132:     // now read in the values that are the same for all the cards
    133:     // NOTE: remember to read the VALUES, not the original inputs!
    134:     tag = card->i[1];
    135:     segs = card->i[2];
    136:     xw1 = card->f[1];
    137:     yw1 = card->f[2];
    138:     zw1 = card->f[3];
    139:     xw2 = card->f[4];
    140:     yw2 = card->f[5];
    141:     zw2 = card->f[6];
    142:     rad = card->f[7];
    143:     
    144:     // set the card's tag number and number of segments
    145:     // Only set card->tag for card types that actually assign an ITG (tag)
    146:     // to generated segments. Some geometry-like cards (GC, GN, GE, etc.)
    147:     // use I1 for other purposes and should not be treated as tags.
    148:     if (card_has_itag(card)) {
    149:       card->tag = tag;
    150:     } else {
    151:       card->tag = 0;
    152:     }
    153:     card->num_segments = segs;
    154:     
    155:     // and now the switch. basically all this does is call the appropriate
    156:     // function to insert the segments for that card type, or complete
    157:     // processing when it sees the GE
    158:     switch(code_num) {
    159:       case 0: // GW, make a wire
    160:         // the radius can be in the f7 field, or it can be on the next card if its tapered
    161:         if(rad != 0.0) {
    162:           xs1 = 1.0;
    163:           ys1 = 1.0;
    164:         } else {
    165:           // make sure the next card is a GC, although we should have already done that
    166:           int next_idx = peek_next_geometry(deck, i);
    167:           if(next_idx == -1 || strcmp(deck->cards[next_idx].card_code, "GC") != 0) {
    168:             snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius, but the next card is not a GC with the tapering info.", i + 1);
>   169:             add_error(ctx, errors, msg, WARNING);
    170:             continue;
    171:           }
    172:           // and also that the values in it are valid
    173:           // Use the GC card tapering info.
    174:           card_t *gc_card = &deck->cards[next_idx];

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L168:             snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius, but the next card is not a GC with the tapering info.", i + 1);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/geometry.c
LINE: 187
CALL: add_error(ctx, errors, msg, WARNING);
--- Context (lines 127-192) ---
    127:     
    128:     // ignore SY and other extension cards in the geometry section
    129:     // they were already evaluated in update_deck_values()
    130:     if (is_extension(card)) continue;
    131: 
    132:     // now read in the values that are the same for all the cards
    133:     // NOTE: remember to read the VALUES, not the original inputs!
    134:     tag = card->i[1];
    135:     segs = card->i[2];
    136:     xw1 = card->f[1];
    137:     yw1 = card->f[2];
    138:     zw1 = card->f[3];
    139:     xw2 = card->f[4];
    140:     yw2 = card->f[5];
    141:     zw2 = card->f[6];
    142:     rad = card->f[7];
    143:     
    144:     // set the card's tag number and number of segments
    145:     // Only set card->tag for card types that actually assign an ITG (tag)
    146:     // to generated segments. Some geometry-like cards (GC, GN, GE, etc.)
    147:     // use I1 for other purposes and should not be treated as tags.
    148:     if (card_has_itag(card)) {
    149:       card->tag = tag;
    150:     } else {
    151:       card->tag = 0;
    152:     }
    153:     card->num_segments = segs;
    154:     
    155:     // and now the switch. basically all this does is call the appropriate
    156:     // function to insert the segments for that card type, or complete
    157:     // processing when it sees the GE
    158:     switch(code_num) {
    159:       case 0: // GW, make a wire
    160:         // the radius can be in the f7 field, or it can be on the next card if its tapered
    161:         if(rad != 0.0) {
    162:           xs1 = 1.0;
    163:           ys1 = 1.0;
    164:         } else {
    165:           // make sure the next card is a GC, although we should have already done that
    166:           int next_idx = peek_next_geometry(deck, i);
    167:           if(next_idx == -1 || strcmp(deck->cards[next_idx].card_code, "GC") != 0) {
    168:             snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius, but the next card is not a GC with the tapering info.", i + 1);
    169:             add_error(ctx, errors, msg, WARNING);
    170:             continue;
    171:           }
    172:           // and also that the values in it are valid
    173:           // Use the GC card tapering info.
    174:           card_t *gc_card = &deck->cards[next_idx];
    175: 
    176:             double gc_x1 = gc_card->f[1];
    177:             // In many decks a GC F1 value of 0 means "no tapering of spacing";
    178:             // treat 0 the same as 1 (equal spacing) to avoid producing zero
    179:             // rd values that collapse segment lengths to zero.
    180:             if (gc_x1 == 0.0) gc_x1 = 1.0;
    181:             double gc_y1 = gc_card->f[2];
    182:             double gc_z1 = gc_card->f[3];
    183: 
    184: 
    185:           if((gc_y1 == 0.0) || (gc_z1 == 0.0)) {
    186:             snprintf(msg, sizeof(msg), "The card on line %d is a GC with tapering info for GW in card %d, but there is a zero in Y1 or Z1.", next_idx + 1, i + 1);
>   187:             add_error(ctx, errors, msg, WARNING);
    188:             i = next_idx; // skip the invalid GC card
    189:             continue;
    190:           }
    191: 
    192:             // override the original inputs with the ones from the GC

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L168:             snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius, but the next card is not a GC with the tapering info.", i + 1);
L186:             snprintf(msg, sizeof(msg), "The card on line %d is a GC with tapering info for GW in card %d, but there is a zero in Y1 or Z1.", next_idx + 1, i + 1);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/geometry.c
LINE: 297
CALL: add_error(ctx, errors, msg, FATAL);
--- Context (lines 237-302) ---
    237:         
    238:       case 2: // GR, rotate the structure
    239:         // I2 is the number of times to duplicate the structure as it rotates
    240:         // ix is set to -1 to indicate this is a rotation, not reflection
    241:         if (!card->ignore) {
    242:           rotate(ctx, i, tag, segs);
    243:         }
    244:         if (ctx->ignored_geometry.n > 0 || ctx->ignored_geometry.m > 0) {
    245:           geometry_t _live = ctx->geometry;
    246:           ctx->geometry = ctx->ignored_geometry;
    247:           rotate(ctx, i, tag, segs);
    248:           ctx->ignored_geometry = ctx->geometry;
    249:           ctx->geometry = _live;
    250:         }
    251:         continue;
    252:         
    253:       case 3: // GS, scale structure dimensions by factor xw1
    254:         if (xw1 == 0.0) {
    255:           /*
    256:            * Special-case handling: sometimes unit tokens (e.g. "in", "ft")
    257:            * are separated by spaces and the preprocessor merges them into
    258:            * the previous integer field (producing "0*in"). In that case the
    259:            * float field may be empty but the original card contains the unit
    260:            * as the third token. Try to recover the scale by parsing the raw
    261:            * `card->card_str` (which contains the un-preprocessed card contents
    262:            * after the mnemonic) and evaluating the third token as a standalone
    263:            * formula (e.g. "in" -> 0.0254).
    264:            */
    265:           char tmp[256];
    266:           char *s = trim_start(card->card_str);
    267:           strncpy(tmp, s, sizeof(tmp)-1);
    268:           tmp[sizeof(tmp)-1] = '\0';
    269:           /* Skip the mnemonic (first token) and then pick the third field
    270:            * after the mnemonic (i.e., the float field). This handles lines
    271:            * like: "GS 0 0 in" where tokens are [GS,0,0,in] and we want "in".
    272:            */
    273:           char *tok = strtok(tmp, " \t");
    274:           int count = 0;
    275:           char *third = NULL;
    276:           while ((tok = strtok(NULL, " \t")) != NULL) {
    277:             count++;
    278:             if (count == 3) { /* third field after mnemonic */
    279:               third = tok;
    280:               break;
    281:             }
    282:           }
    283:           if (third) {
    284:             key_value_t temp_kv = {0};
    285:             temp_kv.key = "GS_TMP";
    286:             temp_kv.value = strdup(third);
    287:             temp_kv.fv = 0.0;
    288:             evaluate_formula(ctx, &temp_kv, deck, errors);
    289:             if (temp_kv.fv != 0.0) {
    290:               xw1 = temp_kv.fv;
    291:             }
    292:             free(temp_kv.value);
    293:           }
    294: 
    295:           if (xw1 == 0.0) {
    296:             snprintf(msg, sizeof(msg), "The GS card on line %d has a scale factor of zero. This is a fatal error.", i + 1);
>   297:             add_error(ctx, errors, msg, FATAL);
    298:             return; // Stops further geometry processing
    299:           }
    300:         }
    301:         if (!card->ignore) {
    302:           scale(ctx, xw1);

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L168:             snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius, but the next card is not a GC with the tapering info.", i + 1);
L186:             snprintf(msg, sizeof(msg), "The card on line %d is a GC with tapering info for GW in card %d, but there is a zero in Y1 or Z1.", next_idx + 1, i + 1);
L296:             snprintf(msg, sizeof(msg), "The GS card on line %d has a scale factor of zero. This is a fatal error.", i + 1);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/geometry.c
LINE: 358
CALL: add_error(ctx,errors, msg, WARNING);
--- Context (lines 298-363) ---
    298:             return; // Stops further geometry processing
    299:           }
    300:         }
    301:         if (!card->ignore) {
    302:           scale(ctx, xw1);
    303:         }
    304:         if (ctx->ignored_geometry.n > 0 || ctx->ignored_geometry.m > 0) {
    305:           geometry_t _live = ctx->geometry;
    306:           ctx->geometry = ctx->ignored_geometry;
    307:           scale(ctx, xw1);
    308:           ctx->ignored_geometry = ctx->geometry;
    309:           ctx->geometry = _live;
    310:         }
    311:         continue;
    312:         
    313:       case 4: // GE, finish off the segments and patches, and calculate everything
    314:         // FIXME: it's not clear what this is testing, on a GE card there shouldn't be an ns input
    315:         //  perhaps it is  clearing out the ns from the previous line? but why bother when it's
    316:         //  about to return anyway?
    317:         if(segs != 0) {
    318:           ctx->plot.iplp1 = 1;
    319:           ctx->plot.iplp2 = 1;
    320:         }
    321:         
    322:         // if we're at the end of the geometry section, we have all the segments
    323:         // so now is an opportune time to connect them together
    324:         if (connect_segments(ctx, tag, outputs) != 0) {
    325:           return; // Stop if there's a fatal geometry error (e.g. below ground)
    326:         }
    327:         
    328:         // ... and calculate the midpoints and other bits
    329:         finish_geometry(ctx);
    330:         
    331:         // and in this case, we're done
    332:         return;
    333:         
    334:       case 5: // GM, move structure or reproduce/duplicate original structure in new positions
    335:         xw1 = xw1 * TA;
    336:         yw1 = yw1 * TA;
    337:         zw1 = zw1 * TA;
    338:         // convert the original float value in F7 to int
    339:         int tag_increment = (int)(card->f[7] + .5);
    340:         if (!card->ignore) {
    341:           reproduce(ctx, xw1, yw1, zw1, xw2, yw2, zw2, tag_increment, segs, tag);
    342:         }
    343:         if (ctx->ignored_geometry.n > 0 || ctx->ignored_geometry.m > 0) {
    344:           geometry_t _live = ctx->geometry;
    345:           ctx->geometry = ctx->ignored_geometry;
    346:           reproduce(ctx, xw1, yw1, zw1, xw2, yw2, zw2, tag_increment, segs, tag);
    347:           ctx->ignored_geometry = ctx->geometry;
    348:           ctx->geometry = _live;
    349:         }
    350:         continue;
    351:         
    352:       case 6: // SP, generate single new patch or a series of patches with SC
    353:         //ns++;
    354:         
    355:         // SP cards have to have a blank in I1, but is this really an error?
    356:         if (tag != 0) {
    357:           snprintf(msg, sizeof(msg), "card_t %d is a SP, but it has data in I1.", i);
>   358:           add_error(ctx,errors, msg, WARNING);
    359:         }
    360:         
    361:         // start with the simple case of a simple, single patch, no set shape
    362:         if(segs == 0) {
    363:           xw2 = xw2 * TA;

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L168:             snprintf(msg, sizeof(msg), "The card on line %d is a GW with a zero radius, but the next card is not a GC with the tapering info.", i + 1);
L186:             snprintf(msg, sizeof(msg), "The card on line %d is a GC with tapering info for GW in card %d, but there is a zero in Y1 or Z1.", next_idx + 1, i + 1);
L296:             snprintf(msg, sizeof(msg), "The GS card on line %d has a scale factor of zero. This is a fatal error.", i + 1);
L357:           snprintf(msg, sizeof(msg), "card_t %d is a SP, but it has data in I1.", i);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/geometry.c
LINE: 374
CALL: add_error(ctx, errors, msg, WARNING);
--- Context (lines 314-379) ---
    314:         // FIXME: it's not clear what this is testing, on a GE card there shouldn't be an ns input
    315:         //  perhaps it is  clearing out the ns from the previous line? but why bother when it's
    316:         //  about to return anyway?
    317:         if(segs != 0) {
    318:           ctx->plot.iplp1 = 1;
    319:           ctx->plot.iplp2 = 1;
    320:         }
    321:         
    322:         // if we're at the end of the geometry section, we have all the segments
    323:         // so now is an opportune time to connect them together
    324:         if (connect_segments(ctx, tag, outputs) != 0) {
    325:           return; // Stop if there's a fatal geometry error (e.g. below ground)
    326:         }
    327:         
    328:         // ... and calculate the midpoints and other bits
    329:         finish_geometry(ctx);
    330:         
    331:         // and in this case, we're done
    332:         return;
    333:         
    334:       case 5: // GM, move structure or reproduce/duplicate original structure in new positions
    335:         xw1 = xw1 * TA;
    336:         yw1 = yw1 * TA;
    337:         zw1 = zw1 * TA;
    338:         // convert the original float value in F7 to int
    339:         int tag_increment = (int)(card->f[7] + .5);
    340:         if (!card->ignore) {
    341:           reproduce(ctx, xw1, yw1, zw1, xw2, yw2, zw2, tag_increment, segs, tag);
    342:         }
    343:         if (ctx->ignored_geometry.n > 0 || ctx->ignored_geometry.m > 0) {
    344:           geometry_t _live = ctx->geometry;
    345:           ctx->geometry = ctx->ignored_geometry;
    346:           reproduce(ctx, xw1, yw1, zw1, xw2, yw2, zw2, tag_increment, segs, tag);
    347:           ctx->ignored_geometry = ctx->geometry;
    348:           ctx->geometry = _live;
    349:         }
    350:         continue;
    351:         
    352:       case 6: // SP, generate single new patch or a series of patches with SC
    353:         //ns++;
    354:         
    355:         // SP cards have to have a blank in I1, but is this really an error?
    356:         if (tag != 0) {
    357:           snprintf(msg, sizeof(msg), "card_t %d is a SP, but it has data in I1.", i);
    358:           add_error(ctx,errors, msg, WARNING);
    359:         }
    360:         
    361:         // start with the simple case of a simple, single patch, no set shape
    362:         if(segs == 0) {
    363:           xw2 = xw2 * TA;
    364:           yw2 = yw2 * TA;
    365:           patch(ctx, target_geom, i, tag, segs + 1, xw1, yw1, zw1, xw2, yw2, zw2, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
    366:         }
    367:         // other shapes, segs=1,2,3, require more inputs and there will be additional SC cards
    368:         else {
    369:           // make sure the next card is an SC
    370:           // TODO: we should test the sanity of the inputs based on the ns
    371:           int next_idx = peek_next_geometry(deck, i);
    372:           if(next_idx == -1 || strcmp(deck->cards[next_idx].card_code, "SC") != 0) {
    373:             snprintf(msg, sizeof(msg), "The card on line %d is a SP with type %d, but the next card is not an SC, which it needs.", i + 1, segs);
>   374:             add_error(ctx, errors, msg, WARNING);
    375:             continue;
    376:           }
    377:           // if it's a triangle we just read one more point from the new card and go...
    378:           if(segs == 2) {
    379:             x3 = deck->cards[next_idx].f[1];

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L186:             snprintf(msg, sizeof(msg), "The card on line %d is a GC with tapering info for GW in card %d, but there is a zero in Y1 or Z1.", next_idx + 1, i + 1);
L296:             snprintf(msg, sizeof(msg), "The GS card on line %d has a scale factor of zero. This is a fatal error.", i + 1);
L357:           snprintf(msg, sizeof(msg), "card_t %d is a SP, but it has data in I1.", i);
L373:             snprintf(msg, sizeof(msg), "The card on line %d is a SP with type %d, but the next card is not an SC, which it needs.", i + 1, segs);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/geometry.c
LINE: 425
CALL: add_error(ctx, errors, msg, 1);
--- Context (lines 365-430) ---
    365:           patch(ctx, target_geom, i, tag, segs + 1, xw1, yw1, zw1, xw2, yw2, zw2, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
    366:         }
    367:         // other shapes, segs=1,2,3, require more inputs and there will be additional SC cards
    368:         else {
    369:           // make sure the next card is an SC
    370:           // TODO: we should test the sanity of the inputs based on the ns
    371:           int next_idx = peek_next_geometry(deck, i);
    372:           if(next_idx == -1 || strcmp(deck->cards[next_idx].card_code, "SC") != 0) {
    373:             snprintf(msg, sizeof(msg), "The card on line %d is a SP with type %d, but the next card is not an SC, which it needs.", i + 1, segs);
    374:             add_error(ctx, errors, msg, WARNING);
    375:             continue;
    376:           }
    377:           // if it's a triangle we just read one more point from the new card and go...
    378:           if(segs == 2) {
    379:             x3 = deck->cards[next_idx].f[1];
    380:             y3 = deck->cards[next_idx].f[2];
    381:             z3 = deck->cards[next_idx].f[3];
    382:             i = next_idx; // skip the SC card next time through the main loop
    383:             patch(ctx, target_geom, i, tag, segs, xw1, yw1, zw1, xw2, yw2, zw2, x3, y3, z3, 0.0, 0.0, 0.0);
    384:           } /* ns == 2 */
    385:           // if it's not a triangle, we have to loop over the following cards
    386:           else {
    387:             // there has to be at least one following...
    388:             x3 = deck->cards[next_idx].f[1];
    389:             y3 = deck->cards[next_idx].f[2];
    390:             z3 = deck->cards[next_idx].f[3];
    391:             x4 = deck->cards[next_idx].f[4];
    392:             y4 = deck->cards[next_idx].f[5];
    393:             z4 = deck->cards[next_idx].f[6];
    394:             i = next_idx;
    395:             patch(ctx, target_geom, i, tag, segs, xw1, yw1, zw1, xw2, yw2, zw2, x3, y3, z3, x4, y4, z4);
    396:             
    397:             // if it was segs=1 we are done at this point, for segs=3 there's more,
    398:             // so loop until we run out of following SC's
    399:             while((next_idx = peek_next_geometry(deck, i)) != -1 && strcmp(deck->cards[next_idx].card_code, "SC") == 0) {
    400:               // copy the last set of end coords into this set's start coords
    401:               xw1 = x3;
    402:               yw1 = y3;
    403:               zw1 = z3;
    404:               xw2 = x4;
    405:               yw2 = y4;
    406:               zw2 = z4;
    407:               // and then get the next set of end coords
    408:               x3 = deck->cards[next_idx].f[1];
    409:               y3 = deck->cards[next_idx].f[2];
    410:               z3 = deck->cards[next_idx].f[3];
    411:               x4 = deck->cards[next_idx].f[4];
    412:               y4 = deck->cards[next_idx].f[5];
    413:               z4 = deck->cards[next_idx].f[6];
    414:               i = next_idx;
    415:               patch(ctx, target_geom, i, tag, segs, xw1, yw1, zw1, xw2, yw2, zw2, x3, y3, z3, x4, y4, z4);
    416:             } /* while cards are SC's */
    417:           }/* ns = 2 */
    418:         } /* ns > 0 */
    419:         
    420:         continue;
    421:         
    422:       case 7: // SM, generate multiple-patch rectangular surface
    423:         if(tag < 1 || segs < 1) {
    424:           snprintf(msg, sizeof(msg), "The card on line %d is a SM, but the number of patches in I1 or I2 is too small.", i + 1);
>   425:           add_error(ctx, errors, msg, 1);
    426:           continue;
    427:         }
    428:         int sm_next = peek_next_geometry(deck, i);
    429:         if(sm_next == -1 || strcmp(deck->cards[sm_next].card_code, "SC") != 0) {
    430:           snprintf(msg, sizeof(msg), "The card on line %d is a SM, but the next card is not an SC, which it needs.", i + 1);

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L296:             snprintf(msg, sizeof(msg), "The GS card on line %d has a scale factor of zero. This is a fatal error.", i + 1);
L357:           snprintf(msg, sizeof(msg), "card_t %d is a SP, but it has data in I1.", i);
L373:             snprintf(msg, sizeof(msg), "The card on line %d is a SP with type %d, but the next card is not an SC, which it needs.", i + 1, segs);
L424:           snprintf(msg, sizeof(msg), "The card on line %d is a SM, but the number of patches in I1 or I2 is too small.", i + 1);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/geometry.c
LINE: 431
CALL: add_error(ctx, errors, msg, 1);
--- Context (lines 371-436) ---
    371:           int next_idx = peek_next_geometry(deck, i);
    372:           if(next_idx == -1 || strcmp(deck->cards[next_idx].card_code, "SC") != 0) {
    373:             snprintf(msg, sizeof(msg), "The card on line %d is a SP with type %d, but the next card is not an SC, which it needs.", i + 1, segs);
    374:             add_error(ctx, errors, msg, WARNING);
    375:             continue;
    376:           }
    377:           // if it's a triangle we just read one more point from the new card and go...
    378:           if(segs == 2) {
    379:             x3 = deck->cards[next_idx].f[1];
    380:             y3 = deck->cards[next_idx].f[2];
    381:             z3 = deck->cards[next_idx].f[3];
    382:             i = next_idx; // skip the SC card next time through the main loop
    383:             patch(ctx, target_geom, i, tag, segs, xw1, yw1, zw1, xw2, yw2, zw2, x3, y3, z3, 0.0, 0.0, 0.0);
    384:           } /* ns == 2 */
    385:           // if it's not a triangle, we have to loop over the following cards
    386:           else {
    387:             // there has to be at least one following...
    388:             x3 = deck->cards[next_idx].f[1];
    389:             y3 = deck->cards[next_idx].f[2];
    390:             z3 = deck->cards[next_idx].f[3];
    391:             x4 = deck->cards[next_idx].f[4];
    392:             y4 = deck->cards[next_idx].f[5];
    393:             z4 = deck->cards[next_idx].f[6];
    394:             i = next_idx;
    395:             patch(ctx, target_geom, i, tag, segs, xw1, yw1, zw1, xw2, yw2, zw2, x3, y3, z3, x4, y4, z4);
    396:             
    397:             // if it was segs=1 we are done at this point, for segs=3 there's more,
    398:             // so loop until we run out of following SC's
    399:             while((next_idx = peek_next_geometry(deck, i)) != -1 && strcmp(deck->cards[next_idx].card_code, "SC") == 0) {
    400:               // copy the last set of end coords into this set's start coords
    401:               xw1 = x3;
    402:               yw1 = y3;
    403:               zw1 = z3;
    404:               xw2 = x4;
    405:               yw2 = y4;
    406:               zw2 = z4;
    407:               // and then get the next set of end coords
    408:               x3 = deck->cards[next_idx].f[1];
    409:               y3 = deck->cards[next_idx].f[2];
    410:               z3 = deck->cards[next_idx].f[3];
    411:               x4 = deck->cards[next_idx].f[4];
    412:               y4 = deck->cards[next_idx].f[5];
    413:               z4 = deck->cards[next_idx].f[6];
    414:               i = next_idx;
    415:               patch(ctx, target_geom, i, tag, segs, xw1, yw1, zw1, xw2, yw2, zw2, x3, y3, z3, x4, y4, z4);
    416:             } /* while cards are SC's */
    417:           }/* ns = 2 */
    418:         } /* ns > 0 */
    419:         
    420:         continue;
    421:         
    422:       case 7: // SM, generate multiple-patch rectangular surface
    423:         if(tag < 1 || segs < 1) {
    424:           snprintf(msg, sizeof(msg), "The card on line %d is a SM, but the number of patches in I1 or I2 is too small.", i + 1);
    425:           add_error(ctx, errors, msg, 1);
    426:           continue;
    427:         }
    428:         int sm_next = peek_next_geometry(deck, i);
    429:         if(sm_next == -1 || strcmp(deck->cards[sm_next].card_code, "SC") != 0) {
    430:           snprintf(msg, sizeof(msg), "The card on line %d is a SM, but the next card is not an SC, which it needs.", i + 1);
>   431:           add_error(ctx, errors, msg, 1);
    432:           continue;
    433:         }
    434:         
    435:         // read the sc and skip it
    436:         x3 = deck->cards[sm_next].f[1];

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L296:             snprintf(msg, sizeof(msg), "The GS card on line %d has a scale factor of zero. This is a fatal error.", i + 1);
L357:           snprintf(msg, sizeof(msg), "card_t %d is a SP, but it has data in I1.", i);
L373:             snprintf(msg, sizeof(msg), "The card on line %d is a SP with type %d, but the next card is not an SC, which it needs.", i + 1, segs);
L424:           snprintf(msg, sizeof(msg), "The card on line %d is a SM, but the number of patches in I1 or I2 is too small.", i + 1);
L430:           snprintf(msg, sizeof(msg), "The card on line %d is a SM, but the next card is not an SC, which it needs.", i + 1);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/geometry.c
LINE: 504
CALL: add_error(ctx, errors, msg, FATAL);
--- Context (lines 444-509) ---
    444:           y4 = yw1 + y3 - yw2;
    445:           z4 = zw1 + z3 - zw2;
    446:         }
    447:         
    448:         patch(ctx, target_geom, i, tag, segs, xw1, yw1, zw1, xw2, yw2, zw2, x3, y3, z3, x4, y4, z4);
    449:         continue;
    450:         
    451:       case 8: // GA, generate segment data for wire arc
    452:         arc(ctx, target_geom, i, tag, segs, xw1, yw1, zw1, xw2);
    453:         continue;
    454:         
    455:       case 9: // SC card, skip it - but it should never happen because SP/SM should have read it
    456:         continue;
    457:         
    458:       case 10: // GH, generate helix
    459:         // Detect 4NEC2's "NEC-4" GH format vs standard NEC-2 format.
    460:         // NEC-2: F1=spacing, F2=length(signed), F3=a1, F4=b1, F5=a2, F6=b2, F7=rad
    461:         // 4NEC2: F1=turns(signed), F2=length, F3=a1, F4=b1, F5=rad, F6=rad, F7=flag
    462:         //   F7=0: log spiral, F7=1: Archimedes spiral (flag only, geometry is the same)
    463:         //
    464:         // Detection: if F7 is 0 or 1 AND F5 is much smaller than F3 (wire radius vs
    465:         // helix radius), this is the 4NEC2 format. Convert turns to spacing and use
    466:         // F5 as the wire radius.
    467:         if((rad == 0.0 || rad == 1.0) && yw2 > 0.0 && zw1 > 0.0 && yw2 < zw1 * 0.5) {
    468:           // 4NEC2 format: F1=turns, F5=wire radius, F7=spiral type flag
    469:           double turns = xw1;      // F1 = number of turns (signed for handedness)
    470:           double wire_rad = yw2;   // F5 = wire radius
    471:           // Convert to NEC-2 parameters: spacing = length / turns
    472:           xw1 = yw1 / turns;       // spacing = total_length / turns (sign carries handedness)
    473:           // For 4NEC2 format, a2=a1 and b2=b1 (uniform helix assumed)
    474:           yw2 = zw1;               // a2 = a1
    475:           zw2 = xw2;               // b2 = b1
    476:           rad = wire_rad;
    477:           // Update card f[] so output display shows the converted NEC-2 values
    478:           card->f[1] = xw1;
    479:           card->f[5] = yw2;
    480:           card->f[6] = zw2;
    481:           card->f[7] = rad;
    482:           snprintf(msg, sizeof(msg), "GH card on line %d: detected 4NEC2 format (%.0f turns, wire radius %.4g).", i + 1, fabs(turns), rad);
    483:           add_message(ctx, outputs, msg);
    484:         }
    485:         helix(ctx, target_geom, i, tag, segs, xw1, yw1, zw1, xw2, yw2, zw2, rad, outputs);
    486:         continue;
    487:         
    488:       case 11: { // GF - load Numerical Green's Function file
    489:         const char *ngf_filename = card->comment;
    490:         char gf_default[MAX_PATH_LEN + 1];
    491:         char gf_resolved[MAX_PATH_LEN + 1];
    492:         if (!ngf_filename || *ngf_filename == '\0') {
    493:           if (ctx->source_filename) {
    494:             strncpy(gf_default, ctx->source_filename, MAX_PATH_LEN);
    495:             gf_default[MAX_PATH_LEN] = '\0';
    496:             char *dot   = strrchr(gf_default, '.');
    497:             char *slash = strrchr(gf_default, '/');
    498:             if (dot && (!slash || dot > slash))
    499:               *dot = '\0';
    500:             strncat(gf_default, ".ngf", MAX_PATH_LEN - strlen(gf_default));
    501:             ngf_filename = gf_default;
    502:           } else {
    503:             snprintf(msg, sizeof(msg), "GF card on line %d has no filename and no input file to derive one from.", i + 1);
>   504:             add_error(ctx, errors, msg, FATAL);
    505:             return;
    506:           }
    507:         } else {
    508:           /* Explicit filename: resolve relative to input file's directory */
    509:           resolve_path_relative_to_input(ngf_filename, ctx->source_filename,

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L357:           snprintf(msg, sizeof(msg), "card_t %d is a SP, but it has data in I1.", i);
L373:             snprintf(msg, sizeof(msg), "The card on line %d is a SP with type %d, but the next card is not an SC, which it needs.", i + 1, segs);
L424:           snprintf(msg, sizeof(msg), "The card on line %d is a SM, but the number of patches in I1 or I2 is too small.", i + 1);
L430:           snprintf(msg, sizeof(msg), "The card on line %d is a SM, but the next card is not an SC, which it needs.", i + 1);
L482:           snprintf(msg, sizeof(msg), "GH card on line %d: detected 4NEC2 format (%.0f turns, wire radius %.4g).", i + 1, fabs(turns), rad);
L503:             snprintf(msg, sizeof(msg), "GF card on line %d has no filename and no input file to derive one from.", i + 1);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/geometry.c
LINE: 516
CALL: add_error(ctx, errors, msg, FATAL);
--- Context (lines 456-521) ---
    456:         continue;
    457:         
    458:       case 10: // GH, generate helix
    459:         // Detect 4NEC2's "NEC-4" GH format vs standard NEC-2 format.
    460:         // NEC-2: F1=spacing, F2=length(signed), F3=a1, F4=b1, F5=a2, F6=b2, F7=rad
    461:         // 4NEC2: F1=turns(signed), F2=length, F3=a1, F4=b1, F5=rad, F6=rad, F7=flag
    462:         //   F7=0: log spiral, F7=1: Archimedes spiral (flag only, geometry is the same)
    463:         //
    464:         // Detection: if F7 is 0 or 1 AND F5 is much smaller than F3 (wire radius vs
    465:         // helix radius), this is the 4NEC2 format. Convert turns to spacing and use
    466:         // F5 as the wire radius.
    467:         if((rad == 0.0 || rad == 1.0) && yw2 > 0.0 && zw1 > 0.0 && yw2 < zw1 * 0.5) {
    468:           // 4NEC2 format: F1=turns, F5=wire radius, F7=spiral type flag
    469:           double turns = xw1;      // F1 = number of turns (signed for handedness)
    470:           double wire_rad = yw2;   // F5 = wire radius
    471:           // Convert to NEC-2 parameters: spacing = length / turns
    472:           xw1 = yw1 / turns;       // spacing = total_length / turns (sign carries handedness)
    473:           // For 4NEC2 format, a2=a1 and b2=b1 (uniform helix assumed)
    474:           yw2 = zw1;               // a2 = a1
    475:           zw2 = xw2;               // b2 = b1
    476:           rad = wire_rad;
    477:           // Update card f[] so output display shows the converted NEC-2 values
    478:           card->f[1] = xw1;
    479:           card->f[5] = yw2;
    480:           card->f[6] = zw2;
    481:           card->f[7] = rad;
    482:           snprintf(msg, sizeof(msg), "GH card on line %d: detected 4NEC2 format (%.0f turns, wire radius %.4g).", i + 1, fabs(turns), rad);
    483:           add_message(ctx, outputs, msg);
    484:         }
    485:         helix(ctx, target_geom, i, tag, segs, xw1, yw1, zw1, xw2, yw2, zw2, rad, outputs);
    486:         continue;
    487:         
    488:       case 11: { // GF - load Numerical Green's Function file
    489:         const char *ngf_filename = card->comment;
    490:         char gf_default[MAX_PATH_LEN + 1];
    491:         char gf_resolved[MAX_PATH_LEN + 1];
    492:         if (!ngf_filename || *ngf_filename == '\0') {
    493:           if (ctx->source_filename) {
    494:             strncpy(gf_default, ctx->source_filename, MAX_PATH_LEN);
    495:             gf_default[MAX_PATH_LEN] = '\0';
    496:             char *dot   = strrchr(gf_default, '.');
    497:             char *slash = strrchr(gf_default, '/');
    498:             if (dot && (!slash || dot > slash))
    499:               *dot = '\0';
    500:             strncat(gf_default, ".ngf", MAX_PATH_LEN - strlen(gf_default));
    501:             ngf_filename = gf_default;
    502:           } else {
    503:             snprintf(msg, sizeof(msg), "GF card on line %d has no filename and no input file to derive one from.", i + 1);
    504:             add_error(ctx, errors, msg, FATAL);
    505:             return;
    506:           }
    507:         } else {
    508:           /* Explicit filename: resolve relative to input file's directory */
    509:           resolve_path_relative_to_input(ngf_filename, ctx->source_filename,
    510:                                          gf_resolved, sizeof(gf_resolved));
    511:           ngf_filename = gf_resolved;
    512:         }
    513:         FILE *gfp = fopen(ngf_filename, "rb");
    514:         if (!gfp) {
    515:           snprintf(msg, sizeof(msg), "GF card on line %d cannot open the NGF file '%s'.", i + 1, ngf_filename);
>   516:           add_error(ctx, errors, msg, FATAL);
    517:           return;
    518:         }
    519:         bool ngf_ok = read_greens_binary(gfp, ctx);
    520:         fclose(gfp);
    521:         if (!ngf_ok) {

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L357:           snprintf(msg, sizeof(msg), "card_t %d is a SP, but it has data in I1.", i);
L373:             snprintf(msg, sizeof(msg), "The card on line %d is a SP with type %d, but the next card is not an SC, which it needs.", i + 1, segs);
L424:           snprintf(msg, sizeof(msg), "The card on line %d is a SM, but the number of patches in I1 or I2 is too small.", i + 1);
L430:           snprintf(msg, sizeof(msg), "The card on line %d is a SM, but the next card is not an SC, which it needs.", i + 1);
L482:           snprintf(msg, sizeof(msg), "GH card on line %d: detected 4NEC2 format (%.0f turns, wire radius %.4g).", i + 1, fabs(turns), rad);
L503:             snprintf(msg, sizeof(msg), "GF card on line %d has no filename and no input file to derive one from.", i + 1);
L515:           snprintf(msg, sizeof(msg), "GF card on line %d cannot open the NGF file '%s'.", i + 1, ngf_filename);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/geometry.c
LINE: 534
CALL: add_error(ctx, errors, msg, WARNING);
--- Context (lines 474-539) ---
    474:           yw2 = zw1;               // a2 = a1
    475:           zw2 = xw2;               // b2 = b1
    476:           rad = wire_rad;
    477:           // Update card f[] so output display shows the converted NEC-2 values
    478:           card->f[1] = xw1;
    479:           card->f[5] = yw2;
    480:           card->f[6] = zw2;
    481:           card->f[7] = rad;
    482:           snprintf(msg, sizeof(msg), "GH card on line %d: detected 4NEC2 format (%.0f turns, wire radius %.4g).", i + 1, fabs(turns), rad);
    483:           add_message(ctx, outputs, msg);
    484:         }
    485:         helix(ctx, target_geom, i, tag, segs, xw1, yw1, zw1, xw2, yw2, zw2, rad, outputs);
    486:         continue;
    487:         
    488:       case 11: { // GF - load Numerical Green's Function file
    489:         const char *ngf_filename = card->comment;
    490:         char gf_default[MAX_PATH_LEN + 1];
    491:         char gf_resolved[MAX_PATH_LEN + 1];
    492:         if (!ngf_filename || *ngf_filename == '\0') {
    493:           if (ctx->source_filename) {
    494:             strncpy(gf_default, ctx->source_filename, MAX_PATH_LEN);
    495:             gf_default[MAX_PATH_LEN] = '\0';
    496:             char *dot   = strrchr(gf_default, '.');
    497:             char *slash = strrchr(gf_default, '/');
    498:             if (dot && (!slash || dot > slash))
    499:               *dot = '\0';
    500:             strncat(gf_default, ".ngf", MAX_PATH_LEN - strlen(gf_default));
    501:             ngf_filename = gf_default;
    502:           } else {
    503:             snprintf(msg, sizeof(msg), "GF card on line %d has no filename and no input file to derive one from.", i + 1);
    504:             add_error(ctx, errors, msg, FATAL);
    505:             return;
    506:           }
    507:         } else {
    508:           /* Explicit filename: resolve relative to input file's directory */
    509:           resolve_path_relative_to_input(ngf_filename, ctx->source_filename,
    510:                                          gf_resolved, sizeof(gf_resolved));
    511:           ngf_filename = gf_resolved;
    512:         }
    513:         FILE *gfp = fopen(ngf_filename, "rb");
    514:         if (!gfp) {
    515:           snprintf(msg, sizeof(msg), "GF card on line %d cannot open the NGF file '%s'.", i + 1, ngf_filename);
    516:           add_error(ctx, errors, msg, FATAL);
    517:           return;
    518:         }
    519:         bool ngf_ok = read_greens_binary(gfp, ctx);
    520:         fclose(gfp);
    521:         if (!ngf_ok) {
    522:           /* error already recorded by read_greens_binary */
    523:           return;
    524:         }
    525:         snprintf(msg, sizeof(msg),
    526:                  "GF card on line %d: loaded %d segments from '%s'.",
    527:                  i + 1, ctx->ngf_n_segs, ngf_filename);
    528:         add_message(ctx, outputs, msg);
    529:         continue;
    530:       }
    531:         
    532:       case 12: // GC, geometry continuation - should only appear after GW
    533:         snprintf(msg, sizeof(msg), "GC card on line %d found outside of GW tapering context.", i + 1);
>   534:         add_error(ctx, errors, msg, WARNING);
    535:         continue;
    536:         
    537:       default: // error message if this isn't a comment
    538:         if(!is_comment(card)) {
    539:           snprintf(msg, sizeof(msg), "Geometry card on line %d has an unknown mnemonic, '%s'.", i + 1, card->card_code);

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L357:           snprintf(msg, sizeof(msg), "card_t %d is a SP, but it has data in I1.", i);
L373:             snprintf(msg, sizeof(msg), "The card on line %d is a SP with type %d, but the next card is not an SC, which it needs.", i + 1, segs);
L424:           snprintf(msg, sizeof(msg), "The card on line %d is a SM, but the number of patches in I1 or I2 is too small.", i + 1);
L430:           snprintf(msg, sizeof(msg), "The card on line %d is a SM, but the next card is not an SC, which it needs.", i + 1);
L482:           snprintf(msg, sizeof(msg), "GH card on line %d: detected 4NEC2 format (%.0f turns, wire radius %.4g).", i + 1, fabs(turns), rad);
L503:             snprintf(msg, sizeof(msg), "GF card on line %d has no filename and no input file to derive one from.", i + 1);
L515:           snprintf(msg, sizeof(msg), "GF card on line %d cannot open the NGF file '%s'.", i + 1, ngf_filename);
L525:         snprintf(msg, sizeof(msg),
L533:         snprintf(msg, sizeof(msg), "GC card on line %d found outside of GW tapering context.", i + 1);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/geometry.c
LINE: 540
CALL: add_error(ctx, errors, msg, 1);
--- Context (lines 480-545) ---
    480:           card->f[6] = zw2;
    481:           card->f[7] = rad;
    482:           snprintf(msg, sizeof(msg), "GH card on line %d: detected 4NEC2 format (%.0f turns, wire radius %.4g).", i + 1, fabs(turns), rad);
    483:           add_message(ctx, outputs, msg);
    484:         }
    485:         helix(ctx, target_geom, i, tag, segs, xw1, yw1, zw1, xw2, yw2, zw2, rad, outputs);
    486:         continue;
    487:         
    488:       case 11: { // GF - load Numerical Green's Function file
    489:         const char *ngf_filename = card->comment;
    490:         char gf_default[MAX_PATH_LEN + 1];
    491:         char gf_resolved[MAX_PATH_LEN + 1];
    492:         if (!ngf_filename || *ngf_filename == '\0') {
    493:           if (ctx->source_filename) {
    494:             strncpy(gf_default, ctx->source_filename, MAX_PATH_LEN);
    495:             gf_default[MAX_PATH_LEN] = '\0';
    496:             char *dot   = strrchr(gf_default, '.');
    497:             char *slash = strrchr(gf_default, '/');
    498:             if (dot && (!slash || dot > slash))
    499:               *dot = '\0';
    500:             strncat(gf_default, ".ngf", MAX_PATH_LEN - strlen(gf_default));
    501:             ngf_filename = gf_default;
    502:           } else {
    503:             snprintf(msg, sizeof(msg), "GF card on line %d has no filename and no input file to derive one from.", i + 1);
    504:             add_error(ctx, errors, msg, FATAL);
    505:             return;
    506:           }
    507:         } else {
    508:           /* Explicit filename: resolve relative to input file's directory */
    509:           resolve_path_relative_to_input(ngf_filename, ctx->source_filename,
    510:                                          gf_resolved, sizeof(gf_resolved));
    511:           ngf_filename = gf_resolved;
    512:         }
    513:         FILE *gfp = fopen(ngf_filename, "rb");
    514:         if (!gfp) {
    515:           snprintf(msg, sizeof(msg), "GF card on line %d cannot open the NGF file '%s'.", i + 1, ngf_filename);
    516:           add_error(ctx, errors, msg, FATAL);
    517:           return;
    518:         }
    519:         bool ngf_ok = read_greens_binary(gfp, ctx);
    520:         fclose(gfp);
    521:         if (!ngf_ok) {
    522:           /* error already recorded by read_greens_binary */
    523:           return;
    524:         }
    525:         snprintf(msg, sizeof(msg),
    526:                  "GF card on line %d: loaded %d segments from '%s'.",
    527:                  i + 1, ctx->ngf_n_segs, ngf_filename);
    528:         add_message(ctx, outputs, msg);
    529:         continue;
    530:       }
    531:         
    532:       case 12: // GC, geometry continuation - should only appear after GW
    533:         snprintf(msg, sizeof(msg), "GC card on line %d found outside of GW tapering context.", i + 1);
    534:         add_error(ctx, errors, msg, WARNING);
    535:         continue;
    536:         
    537:       default: // error message if this isn't a comment
    538:         if(!is_comment(card)) {
    539:           snprintf(msg, sizeof(msg), "Geometry card on line %d has an unknown mnemonic, '%s'.", i + 1, card->card_code);
>   540:           add_error(ctx, errors, msg, 1);
    541:         }
    542:     } /* switch on card type */
    543:   } /* for loop over cards */
    544:   
    545: } /* calculate_geometry */

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L357:           snprintf(msg, sizeof(msg), "card_t %d is a SP, but it has data in I1.", i);
L373:             snprintf(msg, sizeof(msg), "The card on line %d is a SP with type %d, but the next card is not an SC, which it needs.", i + 1, segs);
L424:           snprintf(msg, sizeof(msg), "The card on line %d is a SM, but the number of patches in I1 or I2 is too small.", i + 1);
L430:           snprintf(msg, sizeof(msg), "The card on line %d is a SM, but the next card is not an SC, which it needs.", i + 1);
L482:           snprintf(msg, sizeof(msg), "GH card on line %d: detected 4NEC2 format (%.0f turns, wire radius %.4g).", i + 1, fabs(turns), rad);
L503:             snprintf(msg, sizeof(msg), "GF card on line %d has no filename and no input file to derive one from.", i + 1);
L515:           snprintf(msg, sizeof(msg), "GF card on line %d cannot open the NGF file '%s'.", i + 1, ngf_filename);
L525:         snprintf(msg, sizeof(msg),
L533:         snprintf(msg, sizeof(msg), "GC card on line %d found outside of GW tapering context.", i + 1);
L539:           snprintf(msg, sizeof(msg), "Geometry card on line %d has an unknown mnemonic, '%s'.", i + 1, card->card_code);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/geometry.c
LINE: 565
CALL: add_error(ctx, &ctx->geometry.errors, msg, 1);
--- Context (lines 505-570) ---
    505:             return;
    506:           }
    507:         } else {
    508:           /* Explicit filename: resolve relative to input file's directory */
    509:           resolve_path_relative_to_input(ngf_filename, ctx->source_filename,
    510:                                          gf_resolved, sizeof(gf_resolved));
    511:           ngf_filename = gf_resolved;
    512:         }
    513:         FILE *gfp = fopen(ngf_filename, "rb");
    514:         if (!gfp) {
    515:           snprintf(msg, sizeof(msg), "GF card on line %d cannot open the NGF file '%s'.", i + 1, ngf_filename);
    516:           add_error(ctx, errors, msg, FATAL);
    517:           return;
    518:         }
    519:         bool ngf_ok = read_greens_binary(gfp, ctx);
    520:         fclose(gfp);
    521:         if (!ngf_ok) {
    522:           /* error already recorded by read_greens_binary */
    523:           return;
    524:         }
    525:         snprintf(msg, sizeof(msg),
    526:                  "GF card on line %d: loaded %d segments from '%s'.",
    527:                  i + 1, ctx->ngf_n_segs, ngf_filename);
    528:         add_message(ctx, outputs, msg);
    529:         continue;
    530:       }
    531:         
    532:       case 12: // GC, geometry continuation - should only appear after GW
    533:         snprintf(msg, sizeof(msg), "GC card on line %d found outside of GW tapering context.", i + 1);
    534:         add_error(ctx, errors, msg, WARNING);
    535:         continue;
    536:         
    537:       default: // error message if this isn't a comment
    538:         if(!is_comment(card)) {
    539:           snprintf(msg, sizeof(msg), "Geometry card on line %d has an unknown mnemonic, '%s'.", i + 1, card->card_code);
    540:           add_error(ctx, errors, msg, 1);
    541:         }
    542:     } /* switch on card type */
    543:   } /* for loop over cards */
    544:   
    545: } /* calculate_geometry */
    546: 
    547: /******************************************************************************
    548:  * segment_number
    549:  *
    550:  * segment_number (formerly isegno) returns the segment number for the @p m th
    551:  * segment within the structure generated by the card with tag number @p tag.
    552:  * For instance, the 5th segment within tag 7 might be segment_number 25.
    553:  *
    554:  * @param tag The tag number of the structure/card
    555:  * @param m The segment number within that structure
    556:  *
    557:  */
    558: int segment_number(nec_context_t *ctx, int tag, int seg)
    559: {
    560:   int icnt, iseg;
    561:   char msg[MAX_ERROR_LEN]; // used for seg <= 0 error below
    562:   
    563:   if (seg <= 0) {
    564:     snprintf(msg, sizeof(msg), "segment_number was called with a segment number less or equal to zero.");
>   565:     add_error(ctx, &ctx->geometry.errors, msg, 1);
    566:   }
    567:   
    568:   // if the tag number is zero, then simply return the mth segment as the answer
    569:   // FIXME: is there any point assigning iseg here?
    570:   if (tag == 0) {

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L373:             snprintf(msg, sizeof(msg), "The card on line %d is a SP with type %d, but the next card is not an SC, which it needs.", i + 1, segs);
L424:           snprintf(msg, sizeof(msg), "The card on line %d is a SM, but the number of patches in I1 or I2 is too small.", i + 1);
L430:           snprintf(msg, sizeof(msg), "The card on line %d is a SM, but the next card is not an SC, which it needs.", i + 1);
L482:           snprintf(msg, sizeof(msg), "GH card on line %d: detected 4NEC2 format (%.0f turns, wire radius %.4g).", i + 1, fabs(turns), rad);
L503:             snprintf(msg, sizeof(msg), "GF card on line %d has no filename and no input file to derive one from.", i + 1);
L515:           snprintf(msg, sizeof(msg), "GF card on line %d cannot open the NGF file '%s'.", i + 1, ngf_filename);
L525:         snprintf(msg, sizeof(msg),
L533:         snprintf(msg, sizeof(msg), "GC card on line %d found outside of GW tapering context.", i + 1);
L539:           snprintf(msg, sizeof(msg), "Geometry card on line %d has an unknown mnemonic, '%s'.", i + 1, card->card_code);
L561:   char msg[MAX_ERROR_LEN]; // used for seg <= 0 error below
L564:     snprintf(msg, sizeof(msg), "segment_number was called with a segment number less or equal to zero.");

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/geometry.c
LINE: 641
CALL: add_error(ctx, &ctx->errors, err_msg, FATAL);
--- Context (lines 581-646) ---
    581:       
    582:       icnt++;
    583:       if (icnt == seg) {
    584:         iseg = i + 1;
    585:         return(iseg);
    586:       }
    587:     } /* for( i = 0; i < ctx->geometry.n; i++ ) */
    588:   } /* if( ctx->geometry.n > 0) */
    589:   
    590:   // if we didn't find it, return 0 (caller is responsible for reporting)
    591:   return(0);
    592: } /* end of segment_number */
    593: 
    594: /******************************************************************************
    595:  * connect_segments
    596:  *
    597:  * connect_segments (formerly CONECT) sets up segment connection data in
    598:  * arrays icon1 and icon2 by searching for segment ends that are in contact.
    599:  *
    600:  * @param ignd If a ground plane is in use, checks if wires touch ground
    601:  *
    602:  */
    603: int connect_segments(nec_context_t *ctx, int ignd, outputs_list_t *outputs)
    604: {
    605:   int i, iz, ic, j, jx, ix, ixx, iseg, iend, jend, jump, ipf;
    606:   double sep=0., xi1, yi1, zi1, xi2, yi2, zi2;
    607:   double slen, xa, ya, za, xs, ys, zs;
    608:   size_t mreq;
    609:   char msg[MAX_ERROR_LEN * 64];
    610: 
    611:   // Default: np/mp span the full geometry (symmetry commands may reduce them).
    612:   // Matches nec2c conect() lines 39-41.
    613:   ctx->geometry.np = ctx->geometry.n;
    614:   ctx->geometry.mp = ctx->geometry.m;
    615:   ctx->geometry.ipsym = 0;
    616: 
    617:   ctx->segj.maxcon = 1;
    618:   
    619:   if(ignd != 0) {
    620:     add_message(ctx, outputs, "\n\n     GROUND PLANE SPECIFIED.");
    621: 
    622:     if( ignd > 0)
    623:       add_message(ctx, outputs,
    624:               "\n     WHERE WIRE ENDS TOUCH GROUND, CURRENT WILL"
    625:               " BE INTERPOLATED TO IMAGE IN GROUND PLANE.\n" );
    626: 
    627:     if(ctx->geometry.ipsym == 2) {
    628:       ctx->geometry.np = 2 * ctx->geometry.np;
    629:       ctx->geometry.mp = 2 * ctx->geometry.mp;
    630:     }
    631: 
    632:     if(abs(ctx->geometry.ipsym) > 2) {
    633:       ctx->geometry.np = ctx->geometry.n;
    634:       ctx->geometry.mp = ctx->geometry.m;
    635:     }
    636:     
    637:     /** possibly should be error condition?? **/
    638:     if(ctx->geometry.np > ctx->geometry.n) {
    639:       char err_msg[256];
    640:       snprintf(err_msg, sizeof(err_msg), "connect_segments was called np > n, %d > %d", ctx->geometry.np, ctx->geometry.n);
>   641:       add_error(ctx, &ctx->errors, err_msg, FATAL);
    642:       return -1;
    643:     }
    644:     
    645:     if((ctx->geometry.np == ctx->geometry.n) && (ctx->geometry.mp == ctx->geometry.m))
    646:       ctx->geometry.ipsym = 0;

-- Message variable name candidates --
VAR_EXPR: err_msg  VAR_NAME: err_msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L640:       snprintf(err_msg, sizeof(err_msg), "connect_segments was called np > n, %d > %d", ctx->geometry.np, ctx->geometry.n);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/geometry.c
LINE: 675
CALL: add_error(ctx, &ctx->geometry.errors, l_msg, 1);
--- Context (lines 615-680) ---
    615:   ctx->geometry.ipsym = 0;
    616: 
    617:   ctx->segj.maxcon = 1;
    618:   
    619:   if(ignd != 0) {
    620:     add_message(ctx, outputs, "\n\n     GROUND PLANE SPECIFIED.");
    621: 
    622:     if( ignd > 0)
    623:       add_message(ctx, outputs,
    624:               "\n     WHERE WIRE ENDS TOUCH GROUND, CURRENT WILL"
    625:               " BE INTERPOLATED TO IMAGE IN GROUND PLANE.\n" );
    626: 
    627:     if(ctx->geometry.ipsym == 2) {
    628:       ctx->geometry.np = 2 * ctx->geometry.np;
    629:       ctx->geometry.mp = 2 * ctx->geometry.mp;
    630:     }
    631: 
    632:     if(abs(ctx->geometry.ipsym) > 2) {
    633:       ctx->geometry.np = ctx->geometry.n;
    634:       ctx->geometry.mp = ctx->geometry.m;
    635:     }
    636:     
    637:     /** possibly should be error condition?? **/
    638:     if(ctx->geometry.np > ctx->geometry.n) {
    639:       char err_msg[256];
    640:       snprintf(err_msg, sizeof(err_msg), "connect_segments was called np > n, %d > %d", ctx->geometry.np, ctx->geometry.n);
    641:       add_error(ctx, &ctx->errors, err_msg, FATAL);
    642:       return -1;
    643:     }
    644:     
    645:     if((ctx->geometry.np == ctx->geometry.n) && (ctx->geometry.mp == ctx->geometry.m))
    646:       ctx->geometry.ipsym = 0;
    647:     
    648:   } /* if( ignd != 0) */
    649:   
    650:   if(ctx->geometry.n != 0) {
    651:     /* Allocate memory to connections */
    652:     mreq = (size_t)(ctx->geometry.n + ctx->geometry.m);
    653:     mreq *= sizeof(int);
    654:     mem_realloc(ctx, (void *)&ctx->geometry.icon1, mreq);
    655:     mem_realloc(ctx, (void *)&ctx->geometry.icon2, mreq);
    656:     
    657:     for(i = 0; i < ctx->geometry.n; i++) {
    658:       ctx->geometry.icon1[i] = ctx->geometry.icon2[i] = 0;
    659:       iz = i+1;
    660:       xi1 = ctx->geometry.x1[i];
    661:       yi1 = ctx->geometry.y1[i];
    662:       zi1 = ctx->geometry.z1[i];
    663:       xi2 = ctx->geometry.x2[i];
    664:       yi2 = ctx->geometry.y2[i];
    665:       zi2 = ctx->geometry.z2[i];
    666:       slen = sqrt( (xi2- xi1)*(xi2- xi1) + (yi2- yi1) *
    667:                   (yi2- yi1) + (zi2- zi1)*(zi2- zi1) ) * SMIN;
    668:       
    669:       // determine connection data for end 1 of segment
    670:       jump = false;
    671:       if(ignd > 0) {
    672:         if(zi1 <= -slen) {
    673:           char l_msg[MAX_ERROR_LEN];
    674:           snprintf(l_msg, sizeof(l_msg), "GEOMETRY DATA ERROR -- SEGMENT %d EXTENDS BELOW GROUND", iz);
>   675:           add_error(ctx, &ctx->geometry.errors, l_msg, 1);
    676:           return -1;
    677:         }
    678:         
    679:         if( zi1 <= slen) {
    680:           ctx->geometry.icon1[i]= iz;

-- Message variable name candidates --
VAR_EXPR: l_msg  VAR_NAME: l_msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L674:           snprintf(l_msg, sizeof(l_msg), "GEOMETRY DATA ERROR -- SEGMENT %d EXTENDS BELOW GROUND", iz);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/geometry.c
LINE: 712
CALL: add_error(ctx, &ctx->errors, err_msg, FATAL);
--- Context (lines 652-717) ---
    652:     mreq = (size_t)(ctx->geometry.n + ctx->geometry.m);
    653:     mreq *= sizeof(int);
    654:     mem_realloc(ctx, (void *)&ctx->geometry.icon1, mreq);
    655:     mem_realloc(ctx, (void *)&ctx->geometry.icon2, mreq);
    656:     
    657:     for(i = 0; i < ctx->geometry.n; i++) {
    658:       ctx->geometry.icon1[i] = ctx->geometry.icon2[i] = 0;
    659:       iz = i+1;
    660:       xi1 = ctx->geometry.x1[i];
    661:       yi1 = ctx->geometry.y1[i];
    662:       zi1 = ctx->geometry.z1[i];
    663:       xi2 = ctx->geometry.x2[i];
    664:       yi2 = ctx->geometry.y2[i];
    665:       zi2 = ctx->geometry.z2[i];
    666:       slen = sqrt( (xi2- xi1)*(xi2- xi1) + (yi2- yi1) *
    667:                   (yi2- yi1) + (zi2- zi1)*(zi2- zi1) ) * SMIN;
    668:       
    669:       // determine connection data for end 1 of segment
    670:       jump = false;
    671:       if(ignd > 0) {
    672:         if(zi1 <= -slen) {
    673:           char l_msg[MAX_ERROR_LEN];
    674:           snprintf(l_msg, sizeof(l_msg), "GEOMETRY DATA ERROR -- SEGMENT %d EXTENDS BELOW GROUND", iz);
    675:           add_error(ctx, &ctx->geometry.errors, l_msg, 1);
    676:           return -1;
    677:         }
    678:         
    679:         if( zi1 <= slen) {
    680:           ctx->geometry.icon1[i]= iz;
    681:           ctx->geometry.z1[i]=0.;
    682:           jump = true;
    683:         } /* if( zi1 <= slen) */
    684:       } /* if( ignd > 0) */
    685:       
    686:       if( !jump ) {
    687:         ic= i;
    688:         for( j = 1; j < ctx->geometry.n; j++) {
    689:           ic++;
    690:           if( ic >= ctx->geometry.n)
    691:             ic=0;
    692:           
    693:           sep= fabs( xi1- ctx->geometry.x1[ic])+ fabs(yi1- ctx->geometry.y1[ic])+ fabs(zi1- ctx->geometry.z1[ic]);
    694:           if( sep <= slen) {
    695:             ctx->geometry.icon1[i]= -(ic+1);
    696:             break;
    697:           }
    698:           
    699:           sep= fabs( xi1- ctx->geometry.x2[ic])+ fabs(yi1- ctx->geometry.y2[ic])+ fabs(zi1- ctx->geometry.z2[ic]);
    700:           if( sep <= slen) {
    701:             ctx->geometry.icon1[i]= (ic+1);
    702:             break;
    703:           }
    704:         } /* for( j = 1; j < data.n; j++) */
    705:       } /* if( ! jump ) */
    706:       
    707:       /* determine connection data for end 2 of segment. */
    708:       if( (ignd > 0) || jump ) {
    709:         if( zi2 <= -slen) {
    710:           char err_msg[256];
    711:           snprintf(err_msg, sizeof(err_msg), "GEOMETRY DATA ERROR -- SEGMENT %d EXTENDS BELOW GROUND", iz);
>   712:           add_error(ctx, &ctx->errors, err_msg, FATAL);
    713:           return -1;
    714:         }
    715:         
    716:         if( zi2 <= slen) {
    717:           if( ctx->geometry.icon1[i] == iz ) {

-- Message variable name candidates --
VAR_EXPR: err_msg  VAR_NAME: err_msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L640:       snprintf(err_msg, sizeof(err_msg), "connect_segments was called np > n, %d > %d", ctx->geometry.np, ctx->geometry.n);
L711:           snprintf(err_msg, sizeof(err_msg), "GEOMETRY DATA ERROR -- SEGMENT %d EXTENDS BELOW GROUND", iz);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/geometry.c
LINE: 720
CALL: add_error(ctx, &ctx->errors, err_msg, FATAL);
--- Context (lines 660-725) ---
    660:       xi1 = ctx->geometry.x1[i];
    661:       yi1 = ctx->geometry.y1[i];
    662:       zi1 = ctx->geometry.z1[i];
    663:       xi2 = ctx->geometry.x2[i];
    664:       yi2 = ctx->geometry.y2[i];
    665:       zi2 = ctx->geometry.z2[i];
    666:       slen = sqrt( (xi2- xi1)*(xi2- xi1) + (yi2- yi1) *
    667:                   (yi2- yi1) + (zi2- zi1)*(zi2- zi1) ) * SMIN;
    668:       
    669:       // determine connection data for end 1 of segment
    670:       jump = false;
    671:       if(ignd > 0) {
    672:         if(zi1 <= -slen) {
    673:           char l_msg[MAX_ERROR_LEN];
    674:           snprintf(l_msg, sizeof(l_msg), "GEOMETRY DATA ERROR -- SEGMENT %d EXTENDS BELOW GROUND", iz);
    675:           add_error(ctx, &ctx->geometry.errors, l_msg, 1);
    676:           return -1;
    677:         }
    678:         
    679:         if( zi1 <= slen) {
    680:           ctx->geometry.icon1[i]= iz;
    681:           ctx->geometry.z1[i]=0.;
    682:           jump = true;
    683:         } /* if( zi1 <= slen) */
    684:       } /* if( ignd > 0) */
    685:       
    686:       if( !jump ) {
    687:         ic= i;
    688:         for( j = 1; j < ctx->geometry.n; j++) {
    689:           ic++;
    690:           if( ic >= ctx->geometry.n)
    691:             ic=0;
    692:           
    693:           sep= fabs( xi1- ctx->geometry.x1[ic])+ fabs(yi1- ctx->geometry.y1[ic])+ fabs(zi1- ctx->geometry.z1[ic]);
    694:           if( sep <= slen) {
    695:             ctx->geometry.icon1[i]= -(ic+1);
    696:             break;
    697:           }
    698:           
    699:           sep= fabs( xi1- ctx->geometry.x2[ic])+ fabs(yi1- ctx->geometry.y2[ic])+ fabs(zi1- ctx->geometry.z2[ic]);
    700:           if( sep <= slen) {
    701:             ctx->geometry.icon1[i]= (ic+1);
    702:             break;
    703:           }
    704:         } /* for( j = 1; j < data.n; j++) */
    705:       } /* if( ! jump ) */
    706:       
    707:       /* determine connection data for end 2 of segment. */
    708:       if( (ignd > 0) || jump ) {
    709:         if( zi2 <= -slen) {
    710:           char err_msg[256];
    711:           snprintf(err_msg, sizeof(err_msg), "GEOMETRY DATA ERROR -- SEGMENT %d EXTENDS BELOW GROUND", iz);
    712:           add_error(ctx, &ctx->errors, err_msg, FATAL);
    713:           return -1;
    714:         }
    715:         
    716:         if( zi2 <= slen) {
    717:           if( ctx->geometry.icon1[i] == iz ) {
    718:             char err_msg[256];
    719:             snprintf(err_msg, sizeof(err_msg), "GEOMETRY DATA ERROR -- SEGMENT %d LIES IN GROUND PLANE", iz);
>   720:             add_error(ctx, &ctx->errors, err_msg, FATAL);
    721:             return -1;
    722:           }
    723:           
    724:           ctx->geometry.icon2[i] = iz;
    725:           ctx->geometry.z2[i] = 0.;

-- Message variable name candidates --
VAR_EXPR: err_msg  VAR_NAME: err_msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L640:       snprintf(err_msg, sizeof(err_msg), "connect_segments was called np > n, %d > %d", ctx->geometry.np, ctx->geometry.n);
L711:           snprintf(err_msg, sizeof(err_msg), "GEOMETRY DATA ERROR -- SEGMENT %d EXTENDS BELOW GROUND", iz);
L719:             snprintf(err_msg, sizeof(err_msg), "GEOMETRY DATA ERROR -- SEGMENT %d LIES IN GROUND PLANE", iz);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/geometry.c
LINE: 837
CALL: add_error(ctx, &ctx->geometry.errors, msg, FATAL);
--- Context (lines 777-842) ---
    777:             ic=0;
    778:             calculate_patch(ctx, i, ic);
    779:             break;
    780:           }
    781:           
    782:           sep = fabs(xi2- xs)+ fabs(yi2- ys)+ fabs(zi2- zs);
    783:           if(sep <= slen) {
    784:             ctx->geometry.icon2[iseg] = PCHCON + i;
    785:             ic = 0;
    786:             calculate_patch(ctx, i, ic);
    787:             break;
    788:           }
    789:           
    790:         } /* for( iseg = 0; iseg < data.n; iseg++ ) */
    791:       } /* while( ++i <= data.m ) */
    792:     } /* if( data.m != 0) */
    793:   } /* if( data.n != 0) */
    794:   
    795:   // if we have no geometry, we're done
    796:   if(ctx->geometry.n == 0) {
    797:     return 0;
    798:   }
    799:   
    800:   // allocate to connection buffers
    801:   mreq = (size_t)ctx->segj.maxcon;
    802:   mreq *= sizeof(int);
    803:   mem_realloc(ctx, (void *)&ctx->segj.jco, mreq);
    804:   
    805:   /* adjust connected segment ends to exactly coincide.  print junctions */
    806:   /* of 3 or more seg.  also find old seg. connecting to new seg. */
    807:   iseg = 0;
    808:   ipf = false;
    809:   for(j = 0; j < ctx->geometry.n; j++) {
    810:     jx = j + 1;
    811:     iend = -1;
    812:     jend = -1;
    813:     ix = ctx->geometry.icon1[j];
    814:     ic = 1;
    815:     ctx->segj.jco[0] = -jx;
    816:     xa = ctx->geometry.x1[j];
    817:     ya = ctx->geometry.y1[j];
    818:     za = ctx->geometry.z1[j];
    819:     
    820:     /* if( ix == 0 ) Not needed??
    821:      {
    822:      fprintf( output_fp,
    823:      "\n  CONNECT - SEGMENT CONNECTION ERROR FOR SEGMENT: %d", ix );
    824:      stop(ctx, -1);
    825:      } */
    826:     
    827:     while(true) {
    828:       if((ix != 0) && (ix != (j+1)) && (ix <= PCHCON)) {
    829:         /* chain_limit: a valid connection chain can visit each segment at most
    830:          * once before terminating (ix==0).  If we exceed ctx->geometry.n hops
    831:          * the graph has a cycle and we would loop forever. */
    832:         int chain_hops = 0;
    833:         do {
    834:           if(++chain_hops > ctx->geometry.n) {
    835:             snprintf(msg, sizeof(msg),
    836:               "Segment connection cycle detected at segment %d — geometry is degenerate", j + 1);
>   837:             add_error(ctx, &ctx->geometry.errors, msg, FATAL);
    838:             return -1;
    839:           }
    840: 
    841:           if(ix < 0)
    842:             ix = -ix;

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L640:       snprintf(err_msg, sizeof(err_msg), "connect_segments was called np > n, %d > %d", ctx->geometry.np, ctx->geometry.n);
L674:           snprintf(l_msg, sizeof(l_msg), "GEOMETRY DATA ERROR -- SEGMENT %d EXTENDS BELOW GROUND", iz);
L711:           snprintf(err_msg, sizeof(err_msg), "GEOMETRY DATA ERROR -- SEGMENT %d EXTENDS BELOW GROUND", iz);
L719:             snprintf(err_msg, sizeof(err_msg), "GEOMETRY DATA ERROR -- SEGMENT %d LIES IN GROUND PLANE", iz);
L835:             snprintf(msg, sizeof(msg),

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/geometry.c
LINE: 1035
CALL: add_error(ctx, &ctx->geometry.errors, msg, 1);
--- Context (lines 975-1040) ---
    975:  * finish_geometry
    976:  *
    977:  * finish_geometry (formerly part of calculate_geometry) calculates midpoints
    978:  * of wires and patches and similar values that run when the GE is seen.
    979:  *
    980:  * Some of the calculations it performed were used only for display in the
    981:  * output files, including the angles of segments and the midpoints of patches.
    982:  * These have been moved to output.c. As a result, this code no longer does
    983:  * anything with the patches and it's possible that more of the values being
    984:  * cached here may be removed entirely.
    985:  *
    986:  */
    987: void finish_geometry(nec_context_t *ctx)
    988: {
    989:   size_t mreq;
    990:   double xw1, yw1, zw1;
    991:   double xw2, yw2;
    992:   char msg[MAX_ERROR_LEN];
    993:   
    994:   // and now we calculate various geometry-related data for wires,
    995:   // like the centerpoints and orientation
    996:   if(ctx->geometry.n != 0) {
    997:     // reallocate the buffers
    998:     mreq = (size_t)ctx->geometry.n * sizeof(double);
    999:     mem_realloc(ctx, (void *)&ctx->geometry.si, mreq);
   1000:     mem_realloc(ctx, (void *)&ctx->geometry.sab, mreq);
   1001:     mem_realloc(ctx, (void *)&ctx->geometry.cab, mreq);
   1002:     mem_realloc(ctx, (void *)&ctx->geometry.salp, mreq);
   1003:     mem_realloc(ctx, (void *)&ctx->geometry.x, mreq);
   1004:     mem_realloc(ctx, (void *)&ctx->geometry.y, mreq);
   1005:     mem_realloc(ctx, (void *)&ctx->geometry.z, mreq);
   1006:     
   1007:     for(int i = 0; i < ctx->geometry.n; i++) {
   1008:       // calculate the segment midpoints
   1009:       xw1 = ctx->geometry.x2[i] - ctx->geometry.x1[i];
   1010:       yw1 = ctx->geometry.y2[i] - ctx->geometry.y1[i];
   1011:       zw1 = ctx->geometry.z2[i] - ctx->geometry.z1[i];
   1012:       ctx->geometry.x[i] = (ctx->geometry.x1[i] + ctx->geometry.x2[i]) / 2.0;
   1013:       ctx->geometry.y[i] = (ctx->geometry.y1[i] + ctx->geometry.y2[i]) / 2.0;
   1014:       ctx->geometry.z[i] = (ctx->geometry.z1[i] + ctx->geometry.z2[i]) / 2.0;
   1015:       
   1016:       // and lengths
   1017:       xw2 = xw1 * xw1 + yw1 * yw1 + zw1 * zw1;
   1018:       yw2 = sqrt(xw2);
   1019:       yw2 = (xw2 / yw2 + yw2) * 0.5;
   1020:       ctx->geometry.si[i] = yw2;
   1021:       
   1022:       // and angles
   1023:       ctx->geometry.cab[i] = xw1 / yw2;
   1024:       ctx->geometry.sab[i] = yw1 / yw2;
   1025:       xw2 = zw1 / yw2;
   1026:       
   1027:       if(xw2 > 1.0)
   1028:         xw2 = 1.0;
   1029:       if(xw2 < -1.0)
   1030:         xw2 = -1.0;
   1031:       ctx->geometry.salp[i] = xw2;
   1032:       
   1033:       if(ctx->geometry.si[i] <= 1.e-20) {
   1034:         snprintf(msg, sizeof(msg), "The length of segment %d is too small to process.", i + 1);
>  1035:         add_error(ctx, &ctx->geometry.errors, msg, 1);
   1036:       }
   1037:       if(ctx->geometry.bi[i] <= 0.0) {
   1038:         snprintf(msg, sizeof(msg), "The radius of segment %d is too small to process.", i + 1);
   1039:         add_error(ctx, &ctx->geometry.errors, msg, 1);
   1040:       }

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L835:             snprintf(msg, sizeof(msg),
L922:             snprintf(msg, sizeof(msg), "\n\n    ---------- MULTIPLE WIRE JUNCTIONS ----------\n    JUNCTION  SEGMENTS (- FOR END 1, + FOR END 2)");
L928:           snprintf(msg, sizeof(msg), "\n   %5d      ", iseg);
L931:             size_t len = strlen(msg);
L933:               snprintf(msg + len, sizeof(msg) - len, " ...");
L936:             snprintf(msg + len, sizeof(msg) - len, "%5d", ctx->segj.jco[i-1]);
L938:               len = strlen(msg);
L940:                 snprintf(msg + len, sizeof(msg) - len, " ...");
L943:               snprintf(msg + len, sizeof(msg) - len, "\n              ");
L1034:         snprintf(msg, sizeof(msg), "The length of segment %d is too small to process.", i + 1);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/geometry.c
LINE: 1039
CALL: add_error(ctx, &ctx->geometry.errors, msg, 1);
--- Context (lines 979-1044) ---
    979:  *
    980:  * Some of the calculations it performed were used only for display in the
    981:  * output files, including the angles of segments and the midpoints of patches.
    982:  * These have been moved to output.c. As a result, this code no longer does
    983:  * anything with the patches and it's possible that more of the values being
    984:  * cached here may be removed entirely.
    985:  *
    986:  */
    987: void finish_geometry(nec_context_t *ctx)
    988: {
    989:   size_t mreq;
    990:   double xw1, yw1, zw1;
    991:   double xw2, yw2;
    992:   char msg[MAX_ERROR_LEN];
    993:   
    994:   // and now we calculate various geometry-related data for wires,
    995:   // like the centerpoints and orientation
    996:   if(ctx->geometry.n != 0) {
    997:     // reallocate the buffers
    998:     mreq = (size_t)ctx->geometry.n * sizeof(double);
    999:     mem_realloc(ctx, (void *)&ctx->geometry.si, mreq);
   1000:     mem_realloc(ctx, (void *)&ctx->geometry.sab, mreq);
   1001:     mem_realloc(ctx, (void *)&ctx->geometry.cab, mreq);
   1002:     mem_realloc(ctx, (void *)&ctx->geometry.salp, mreq);
   1003:     mem_realloc(ctx, (void *)&ctx->geometry.x, mreq);
   1004:     mem_realloc(ctx, (void *)&ctx->geometry.y, mreq);
   1005:     mem_realloc(ctx, (void *)&ctx->geometry.z, mreq);
   1006:     
   1007:     for(int i = 0; i < ctx->geometry.n; i++) {
   1008:       // calculate the segment midpoints
   1009:       xw1 = ctx->geometry.x2[i] - ctx->geometry.x1[i];
   1010:       yw1 = ctx->geometry.y2[i] - ctx->geometry.y1[i];
   1011:       zw1 = ctx->geometry.z2[i] - ctx->geometry.z1[i];
   1012:       ctx->geometry.x[i] = (ctx->geometry.x1[i] + ctx->geometry.x2[i]) / 2.0;
   1013:       ctx->geometry.y[i] = (ctx->geometry.y1[i] + ctx->geometry.y2[i]) / 2.0;
   1014:       ctx->geometry.z[i] = (ctx->geometry.z1[i] + ctx->geometry.z2[i]) / 2.0;
   1015:       
   1016:       // and lengths
   1017:       xw2 = xw1 * xw1 + yw1 * yw1 + zw1 * zw1;
   1018:       yw2 = sqrt(xw2);
   1019:       yw2 = (xw2 / yw2 + yw2) * 0.5;
   1020:       ctx->geometry.si[i] = yw2;
   1021:       
   1022:       // and angles
   1023:       ctx->geometry.cab[i] = xw1 / yw2;
   1024:       ctx->geometry.sab[i] = yw1 / yw2;
   1025:       xw2 = zw1 / yw2;
   1026:       
   1027:       if(xw2 > 1.0)
   1028:         xw2 = 1.0;
   1029:       if(xw2 < -1.0)
   1030:         xw2 = -1.0;
   1031:       ctx->geometry.salp[i] = xw2;
   1032:       
   1033:       if(ctx->geometry.si[i] <= 1.e-20) {
   1034:         snprintf(msg, sizeof(msg), "The length of segment %d is too small to process.", i + 1);
   1035:         add_error(ctx, &ctx->geometry.errors, msg, 1);
   1036:       }
   1037:       if(ctx->geometry.bi[i] <= 0.0) {
   1038:         snprintf(msg, sizeof(msg), "The radius of segment %d is too small to process.", i + 1);
>  1039:         add_error(ctx, &ctx->geometry.errors, msg, 1);
   1040:       }
   1041:     } /* for( i = 0; i < ctx->geometry.n; i++ ) */
   1042:   } /* if( ctx->geometry.n != 0) */
   1043:   
   1044:   // update the counters that track the total number of segments and patches

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L922:             snprintf(msg, sizeof(msg), "\n\n    ---------- MULTIPLE WIRE JUNCTIONS ----------\n    JUNCTION  SEGMENTS (- FOR END 1, + FOR END 2)");
L928:           snprintf(msg, sizeof(msg), "\n   %5d      ", iseg);
L931:             size_t len = strlen(msg);
L933:               snprintf(msg + len, sizeof(msg) - len, " ...");
L936:             snprintf(msg + len, sizeof(msg) - len, "%5d", ctx->segj.jco[i-1]);
L938:               len = strlen(msg);
L940:                 snprintf(msg + len, sizeof(msg) - len, " ...");
L943:               snprintf(msg + len, sizeof(msg) - len, "\n              ");
L1034:         snprintf(msg, sizeof(msg), "The length of segment %d is too small to process.", i + 1);
L1038:         snprintf(msg, sizeof(msg), "The radius of segment %d is too small to process.", i + 1);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/geometry.c
LINE: 1205
CALL: add_error(ctx, &ctx->geometry.errors, msg, 1);
--- Context (lines 1145-1210) ---
   1145:     // save these out
   1146:     geom->card_nums[i] = card_num;
   1147:     geom->tag_nums[i] = tag_num;
   1148:     
   1149:     // calculate the new locations
   1150:     xs2 = xs1 + xd * delz;
   1151:     ys2 = ys1 + yd * delz;
   1152:     zs2 = zs1 + zd * delz;
   1153:     
   1154:     // set the geometry
   1155:     geom->x1[i] = xs1;
   1156:     geom->y1[i] = ys1;
   1157:     geom->z1[i] = zs1;
   1158:     geom->x2[i] = xs2;
   1159:     geom->y2[i] = ys2;
   1160:     geom->z2[i] = zs2;
   1161:     geom->bi[i] = radz;
   1162:     
   1163:     // move to the other end and and re-taper
   1164:     delz = delz * rd;
   1165:     radz = radz * rrad;
   1166:     xs1 = xs2;
   1167:     ys1 = ys2;
   1168:     zs1 = zs2;
   1169:   } /* loop over remaining segments */
   1170:   
   1171:   // fill in the end of the line with the last point
   1172:   geom->x2[geom->n-1] = xw2;
   1173:   geom->y2[geom->n-1] = yw2;
   1174:   geom->z2[geom->n-1] = zw2;
   1175: } /* end of wire() */
   1176: 
   1177: /******************************************************************************
   1178:  * arc
   1179:  *
   1180:  * arc generates segment geometry data for an arc of @p segs segments.
   1181:  *
   1182:  * @param card_num card_t number for this set of segments
   1183:  * @param tag_num Tag number for this set of segments, maybe 0
   1184:  * @param segs Number of segments in the arc
   1185:  * @param arc_radius Radius of the arc
   1186:  * @param ang1 Starting angle
   1187:  * @param ang2 Ending angle - ang2-ang1 <= 360
   1188:  * @param wire_radius Radius of the wire
   1189:  *
   1190:  */
   1191: void arc(nec_context_t *ctx, geometry_t *geom, int card_num, int tag_num, int segs, double rada, double ang1, double ang2, double rad)
   1192: {
   1193:   double ang, dang, xs1, xs2, zs1, zs2;
   1194:   int first_segment_num = geom->n;
   1195:   
   1196:   // no point continuing if there are no segments
   1197:   if(segs < 1) return;
   1198:   
   1199:   // this test was previously performed at the end, which meant that
   1200:   // symmetry was removed even if it didn't actually build the arc.
   1201:   // as is the case in wire and helix, we will do the test now
   1202:   if(fabs(ang2- ang1) > 360.0000) {
   1203:     char msg[MAX_ERROR_LEN];
   1204:     snprintf(msg, sizeof(msg), "The card on line %d is a GA with an angle >360 degrees.", card_num + 1);
>  1205:     add_error(ctx, &ctx->geometry.errors, msg, 1);
   1206:     return;
   1207:   }
   1208:   
   1209:   // update the segment count
   1210:   geom->n += segs;

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L1034:         snprintf(msg, sizeof(msg), "The length of segment %d is too small to process.", i + 1);
L1038:         snprintf(msg, sizeof(msg), "The radius of segment %d is too small to process.", i + 1);
L1204:     snprintf(msg, sizeof(msg), "The card on line %d is a GA with an angle >360 degrees.", card_num + 1);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/geometry.c
LINE: 1641
CALL: add_error(ctx, &ctx->geometry.errors, msg, 1);
--- Context (lines 1581-1646) ---
   1581:         ctx->geometry.t1y[k]= xi* yx+ yi* yy+ zi* yz;
   1582:         ctx->geometry.t1z[k]= xi* zx+ yi* zy+ zi* zz;
   1583:         xi= ctx->geometry.t2x[i];
   1584:         yi= ctx->geometry.t2y[i];
   1585:         zi= ctx->geometry.t2z[i];
   1586:         ctx->geometry.t2x[k]= xi* xx+ yi* xy+ zi* xz;
   1587:         ctx->geometry.t2y[k]= xi* yx+ yi* yy+ zi* yz;
   1588:         ctx->geometry.t2z[k]= xi* zx+ yi* zy+ zi* zz;
   1589:         ctx->geometry.psalp[k]= ctx->geometry.psalp[i];
   1590:         ctx->geometry.pbi[k]= ctx->geometry.pbi[i];
   1591:         k++;
   1592:       } /* for( i = i1; i < data.m; i++ ) */
   1593: 
   1594:       ctx->geometry.m = k;
   1595:     } /* for( ii = 0; ii < nrp; ii++ ) */
   1596: 
   1597:   } /* if( data.m >= m2) */
   1598:   
   1599:   // test whether we did a complete rotation/copy
   1600:   if((nrpt == 0) && (ix == 1))
   1601:     return;
   1602:   
   1603:   // otherwise, reset the symmetry flags to "none"
   1604:   ctx->geometry.np = ctx->geometry.n;
   1605:   ctx->geometry.mp = ctx->geometry.m;
   1606:   ctx->geometry.ipsym = 0;
   1607: } /* end of reproduce */
   1608: 
   1609: /******************************************************************************
   1610:  * reflect
   1611:  *
   1612:  * reflect (formerly reflc) creates new geometry entries for all existing
   1613:  * entries to create reflections across the selected axes. reflect can
   1614:  * duplicate across the X, Y and/or Z axes in a single operation. If the
   1615:  * original entries had a tag number, it will be updated by the tag_increment,
   1616:  * while those with a zero tag will remain zero.
   1617:  *
   1618:  * reflect formerly performed two separate functions, reflecting for GX cards
   1619:  * or rotating for GR cards. The code was entirely separate for these two
   1620:  * functions, controlled by a long if statement. It made no sense to leave
   1621:  * them combined, so the handler for the GR case has been split out into its
   1622:  * own function, rotate.
   1623:  *
   1624:  * @param card_num card_t number that contains this instruction
   1625:  * @param tag_increment the number to increment the tag by, see notes below
   1626:  * @param ix see iz
   1627:  * @param iy see iz
   1628:  * @param iz flags indicating whether to relect on this axis
   1629:  *
   1630:  */
   1631: void reflect(nec_context_t *ctx, int card_num, int tag_increment, int ix, int iy, int iz)
   1632: {
   1633:   int iti, i, nx, itagi;
   1634:   size_t mreq;
   1635:   double e1, e2;
   1636: 
   1637:   // sanity check, formerly used nop>0 but we no longer pass that in
   1638:   if(ix == 0 && iy == 0 && iz == 0) {
   1639:     char msg[MAX_ERROR_LEN];
   1640:     snprintf(msg, sizeof(msg), "GX on card %d has no reflection axes.", card_num + 1);
>  1641:     add_error(ctx, &ctx->geometry.errors, msg, 1);
   1642:     return;
   1643:   }
   1644: 
   1645:   /* GX card: NEC-2 spec says GX affects new structure only.
   1646:    * n0 = number of frozen NGF segments at the start of the array.

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L1640:     snprintf(msg, sizeof(msg), "GX on card %d has no reflection axes.", card_num + 1);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/geometry.c
LINE: 1710
CALL: add_error(ctx, &ctx->geometry.errors, l_msg, 1);
--- Context (lines 1650-1715) ---
   1650:   int n0 = ctx->has_ngf ? ctx->ngf_n_segs : 0;
   1651: 
   1652:   // we are going to create symmetry one way or the other,
   1653:   // so we copy down how much geometry is in the symmetry "cell"
   1654:   ctx->geometry.np = ctx->geometry.n - n0;
   1655:   ctx->geometry.mp = ctx->geometry.m;
   1656:   iti = tag_increment;
   1657: 
   1658:   // both GR and GX cards use only the I1 and I2 inputs in the card. I1 is
   1659:   // passed in the tag_increment, and I2 in num_copies. However, the I2 value
   1660:   // means different things in the two cards, in the GR card is is the number
   1661:   // of times to make a copy of the wires, for the GX is is a flag saying which
   1662:   // axes to reflect along. Since the flag value is a value number of copies
   1663:   // value, the code that calls reflect copies the I2 value into the ix, iy and iz
   1664:   // so to indicate if we are performing
   1665: 
   1666:   // we are now symmetric
   1667:   // FIXME: the original code for this is confusing, this should be reviewed
   1668:   ctx->geometry.ipsym = 1;
   1669: 
   1670:   // reflect along z axis
   1671:   if(iz != 0) {
   1672:     ctx->geometry.ipsym = 2;
   1673: 
   1674:     // copy existing wires if there are any
   1675:     if(ctx->geometry.n > n0) {
   1676:       int nn = ctx->geometry.n;
   1677:       int new_count = nn - n0;
   1678: 
   1679:       // reallocate cards and tags buffers
   1680:       mreq = (size_t)(nn + new_count + ctx->geometry.m);
   1681:       mreq *= sizeof(int);
   1682:       mem_realloc(ctx, (void *)&ctx->geometry.tag_nums, mreq);
   1683:       mem_realloc(ctx, (void *)&ctx->geometry.card_nums, mreq);
   1684: 
   1685:       // Reallocate wire buffers
   1686:       mreq = (size_t)(nn + new_count);
   1687:       mreq *= sizeof(double);
   1688:       mem_realloc(ctx, (void *)&ctx->geometry.x1, mreq);
   1689:       mem_realloc(ctx, (void *)&ctx->geometry.y1, mreq);
   1690:       mem_realloc(ctx, (void *)&ctx->geometry.z1, mreq);
   1691:       mem_realloc(ctx, (void *)&ctx->geometry.x2, mreq);
   1692:       mem_realloc(ctx, (void *)&ctx->geometry.y2, mreq);
   1693:       mem_realloc(ctx, (void *)&ctx->geometry.z2, mreq);
   1694:       mem_realloc(ctx, (void *)&ctx->geometry.bi, mreq);
   1695: 
   1696:       for(i = n0; i < nn; i++) {
   1697:         // pack copies right after existing segments
   1698:         nx = nn + (i - n0);
   1699: 
   1700:         // get the existing z end points and test them
   1701:         e1 = ctx->geometry.z1[i];
   1702:         e2 = ctx->geometry.z2[i];
   1703: 
   1704:         if((fabs(e1) + fabs(e2) <= 1.0e-12) || (e1 * e2 < -1.0e-12)) {
   1705:           char l_msg[MAX_ERROR_LEN];
   1706:           snprintf(l_msg, sizeof(l_msg),
   1707:                   "\n  GEOMETRY DATA ERROR--SEGMENT %d"
   1708:                   " LIES IN PLANE OF SYMMETRY",
   1709:                   i + 1);
>  1710:           add_error(ctx, &ctx->geometry.errors, l_msg, 1);
   1711:           return;
   1712:         }
   1713: 
   1714:         ctx->geometry.x1[nx] = ctx->geometry.x1[i];
   1715:         ctx->geometry.y1[nx] = ctx->geometry.y1[i];

-- Message variable name candidates --
VAR_EXPR: l_msg  VAR_NAME: l_msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L1706:           snprintf(l_msg, sizeof(l_msg),

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/geometry.c
LINE: 1766
CALL: add_error(ctx, &ctx->geometry.errors, l_msg, 1);
--- Context (lines 1706-1771) ---
   1706:           snprintf(l_msg, sizeof(l_msg),
   1707:                   "\n  GEOMETRY DATA ERROR--SEGMENT %d"
   1708:                   " LIES IN PLANE OF SYMMETRY",
   1709:                   i + 1);
   1710:           add_error(ctx, &ctx->geometry.errors, l_msg, 1);
   1711:           return;
   1712:         }
   1713: 
   1714:         ctx->geometry.x1[nx] = ctx->geometry.x1[i];
   1715:         ctx->geometry.y1[nx] = ctx->geometry.y1[i];
   1716:         ctx->geometry.z1[nx] = -e1;
   1717:         ctx->geometry.x2[nx] = ctx->geometry.x2[i];
   1718:         ctx->geometry.y2[nx] = ctx->geometry.y2[i];
   1719:         ctx->geometry.z2[nx] = -e2;
   1720: 
   1721:         // get the last used tag num
   1722:         itagi = ctx->geometry.tag_nums[i];
   1723: 
   1724:         // now set the tag of the new entries to zero or that offset
   1725:         if(itagi == 0)
   1726:           ctx->geometry.tag_nums[nx] = 0;
   1727:         if(itagi != 0)
   1728:           ctx->geometry.tag_nums[nx]= itagi + iti;
   1729: 
   1730:         ctx->geometry.bi[nx]= ctx->geometry.bi[i];
   1731:       } /* for( i = n0; i < nn; i++ ) */
   1732: 
   1733:       // new count doubles the new structure (not the frozen NGF segments)
   1734:       ctx->geometry.n = nn + new_count;
   1735: 
   1736:       // and that if we make more entries they need to be
   1737:       // offset by a greater number
   1738:       iti = iti * 2;
   1739:     } /* if( geometry.n > n0) */
   1740: 
   1741:     // and now the patches, if there are any (patches are never NGF)
   1742:     if(ctx->geometry.m > 0) {
   1743:       /* Reallocate patch buffers */
   1744:       mreq = (size_t)(2 * ctx->geometry.m);
   1745:       mreq *= sizeof(double);
   1746:       mem_realloc(ctx, (void *)&ctx->geometry.px, mreq);
   1747:       mem_realloc(ctx, (void *)&ctx->geometry.py, mreq);
   1748:       mem_realloc(ctx, (void *)&ctx->geometry.pz, mreq);
   1749:       mem_realloc(ctx, (void *)&ctx->geometry.t1x, mreq);
   1750:       mem_realloc(ctx, (void *)&ctx->geometry.t1y, mreq);
   1751:       mem_realloc(ctx, (void *)&ctx->geometry.t1z, mreq);
   1752:       mem_realloc(ctx, (void *)&ctx->geometry.t2x, mreq);
   1753:       mem_realloc(ctx, (void *)&ctx->geometry.t2y, mreq);
   1754:       mem_realloc(ctx, (void *)&ctx->geometry.t2z, mreq);
   1755:       mem_realloc(ctx, (void *)&ctx->geometry.pbi, mreq);
   1756:       mem_realloc(ctx, (void *)&ctx->geometry.psalp, mreq);
   1757: 
   1758:       for(i = 0; i < ctx->geometry.m; i++) {
   1759:         nx = i+ctx->geometry.m;
   1760:         if(fabs(ctx->geometry.pz[i]) <= 1.0e-10) {
   1761:           char l_msg[MAX_ERROR_LEN];
   1762:           snprintf(l_msg, sizeof(l_msg),
   1763:                   "\n  GEOMETRY DATA ERROR--PATCH %d"
   1764:                   " LIES IN PLANE OF SYMMETRY",
   1765:                   i + 1);
>  1766:           add_error(ctx, &ctx->geometry.errors, l_msg, 1);
   1767:           return;
   1768:         }
   1769: 
   1770:         ctx->geometry.px[nx]= ctx->geometry.px[i];
   1771:         ctx->geometry.py[nx]= ctx->geometry.py[i];

-- Message variable name candidates --
VAR_EXPR: l_msg  VAR_NAME: l_msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L1706:           snprintf(l_msg, sizeof(l_msg),
L1762:           snprintf(l_msg, sizeof(l_msg),

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/geometry.c
LINE: 1820
CALL: add_error(ctx, &ctx->geometry.errors, l_msg, 1);
--- Context (lines 1760-1825) ---
   1760:         if(fabs(ctx->geometry.pz[i]) <= 1.0e-10) {
   1761:           char l_msg[MAX_ERROR_LEN];
   1762:           snprintf(l_msg, sizeof(l_msg),
   1763:                   "\n  GEOMETRY DATA ERROR--PATCH %d"
   1764:                   " LIES IN PLANE OF SYMMETRY",
   1765:                   i + 1);
   1766:           add_error(ctx, &ctx->geometry.errors, l_msg, 1);
   1767:           return;
   1768:         }
   1769: 
   1770:         ctx->geometry.px[nx]= ctx->geometry.px[i];
   1771:         ctx->geometry.py[nx]= ctx->geometry.py[i];
   1772:         ctx->geometry.pz[nx]= -ctx->geometry.pz[i];
   1773:         ctx->geometry.t1x[nx]= ctx->geometry.t1x[i];
   1774:         ctx->geometry.t1y[nx]= ctx->geometry.t1y[i];
   1775:         ctx->geometry.t1z[nx]= -ctx->geometry.t1z[i];
   1776:         ctx->geometry.t2x[nx]= ctx->geometry.t2x[i];
   1777:         ctx->geometry.t2y[nx]= ctx->geometry.t2y[i];
   1778:         ctx->geometry.t2z[nx]= -ctx->geometry.t2z[i];
   1779:         ctx->geometry.psalp[nx]= -ctx->geometry.psalp[i];
   1780:         ctx->geometry.pbi[nx]= ctx->geometry.pbi[i];
   1781:       }
   1782: 
   1783:       ctx->geometry.m= ctx->geometry.m*2;
   1784:     } /* if( data.m >= m2) */
   1785:   } /* if( iz != 0) */
   1786: 
   1787:   // now repeat all of that for the y-axis
   1788:   if(iy != 0) {
   1789:     if(ctx->geometry.n > n0) {
   1790:       int nn = ctx->geometry.n;
   1791:       int new_count = nn - n0;
   1792: 
   1793:       /* Reallocate tags buffer */
   1794:       mreq = (size_t)(nn + new_count + ctx->geometry.m);
   1795:       mreq *= sizeof(int);
   1796:       mem_realloc(ctx, (void *)&ctx->geometry.tag_nums, mreq);
   1797: 
   1798:       /* Reallocate wire buffers */
   1799:       mreq = (size_t)(nn + new_count);
   1800:       mreq *= sizeof(double);
   1801:       mem_realloc(ctx, (void *)&ctx->geometry.x1, mreq);
   1802:       mem_realloc(ctx, (void *)&ctx->geometry.y1, mreq);
   1803:       mem_realloc(ctx, (void *)&ctx->geometry.z1, mreq);
   1804:       mem_realloc(ctx, (void *)&ctx->geometry.x2, mreq);
   1805:       mem_realloc(ctx, (void *)&ctx->geometry.y2, mreq);
   1806:       mem_realloc(ctx, (void *)&ctx->geometry.z2, mreq);
   1807:       mem_realloc(ctx, (void *)&ctx->geometry.bi, mreq);
   1808: 
   1809:       for(i = n0; i < nn; i++) {
   1810:         nx = nn + (i - n0);
   1811:         e1= ctx->geometry.y1[i];
   1812:         e2= ctx->geometry.y2[i];
   1813: 
   1814:         if((fabs(e1)+fabs(e2) <= 1.0e-12) || (e1*e2 < -1.0e-12)) {
   1815:           char l_msg[MAX_ERROR_LEN];
   1816:           snprintf(l_msg, sizeof(l_msg),
   1817:                   "\n  GEOMETRY DATA ERROR--SEGMENT %d"
   1818:                   " LIES IN PLANE OF SYMMETRY",
   1819:                   i + 1);
>  1820:           add_error(ctx, &ctx->geometry.errors, l_msg, 1);
   1821:           return;
   1822:         }
   1823: 
   1824:         ctx->geometry.x1[nx] = ctx->geometry.x1[i];
   1825:         ctx->geometry.y1[nx] = -e1;

-- Message variable name candidates --
VAR_EXPR: l_msg  VAR_NAME: l_msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L1706:           snprintf(l_msg, sizeof(l_msg),
L1762:           snprintf(l_msg, sizeof(l_msg),
L1816:           snprintf(l_msg, sizeof(l_msg),

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/geometry.c
LINE: 1872
CALL: add_error(ctx, &ctx->geometry.errors, l_msg, 1);
--- Context (lines 1812-1877) ---
   1812:         e2= ctx->geometry.y2[i];
   1813: 
   1814:         if((fabs(e1)+fabs(e2) <= 1.0e-12) || (e1*e2 < -1.0e-12)) {
   1815:           char l_msg[MAX_ERROR_LEN];
   1816:           snprintf(l_msg, sizeof(l_msg),
   1817:                   "\n  GEOMETRY DATA ERROR--SEGMENT %d"
   1818:                   " LIES IN PLANE OF SYMMETRY",
   1819:                   i + 1);
   1820:           add_error(ctx, &ctx->geometry.errors, l_msg, 1);
   1821:           return;
   1822:         }
   1823: 
   1824:         ctx->geometry.x1[nx] = ctx->geometry.x1[i];
   1825:         ctx->geometry.y1[nx] = -e1;
   1826:         ctx->geometry.z1[nx] = ctx->geometry.z1[i];
   1827:         ctx->geometry.x2[nx] = ctx->geometry.x2[i];
   1828:         ctx->geometry.y2[nx] = -e2;
   1829:         ctx->geometry.z2[nx] = ctx->geometry.z2[i];
   1830:         itagi = ctx->geometry.tag_nums[i];
   1831: 
   1832:         if( itagi == 0)
   1833:           ctx->geometry.tag_nums[nx]=0;
   1834:         if( itagi != 0)
   1835:           ctx->geometry.tag_nums[nx]= itagi+ iti;
   1836: 
   1837:         ctx->geometry.bi[nx]= ctx->geometry.bi[i];
   1838: 
   1839:       } /* for( i = n0; i < nn; i++ ) */
   1840: 
   1841:       ctx->geometry.n = nn + new_count;
   1842:       iti= iti*2;
   1843: 
   1844:     } /* if( geometry.n > n0) */
   1845: 
   1846:     // reflect any patches
   1847:     if(ctx->geometry.m > 0)  {
   1848:       // reflection doubles the number of patches, so we start
   1849:       // by reallocating the patch list to hold the new ones
   1850:       mreq = (size_t)(2 * ctx->geometry.m);
   1851:       mreq *= sizeof(double);
   1852:       mem_realloc(ctx, (void *)&ctx->geometry.px, mreq);
   1853:       mem_realloc(ctx, (void *)&ctx->geometry.py, mreq);
   1854:       mem_realloc(ctx, (void *)&ctx->geometry.pz, mreq);
   1855:       mem_realloc(ctx, (void *)&ctx->geometry.t1x, mreq);
   1856:       mem_realloc(ctx, (void *)&ctx->geometry.t1y, mreq);
   1857:       mem_realloc(ctx, (void *)&ctx->geometry.t1z, mreq);
   1858:       mem_realloc(ctx, (void *)&ctx->geometry.t2x, mreq);
   1859:       mem_realloc(ctx, (void *)&ctx->geometry.t2y, mreq);
   1860:       mem_realloc(ctx, (void *)&ctx->geometry.t2z, mreq);
   1861:       mem_realloc(ctx, (void *)&ctx->geometry.pbi, mreq);
   1862:       mem_realloc(ctx, (void *)&ctx->geometry.psalp, mreq);
   1863: 
   1864:       for( i = 0; i < ctx->geometry.m; i++ ) {
   1865:         nx= i+ctx->geometry.m;
   1866:         if( fabs( ctx->geometry.py[i]) <= 1.0e-10) {
   1867:           char l_msg[MAX_ERROR_LEN];
   1868:           snprintf(l_msg, sizeof(l_msg),
   1869:                   "\n  GEOMETRY DATA ERROR--PATCH %d"
   1870:                   " LIES IN PLANE OF SYMMETRY",
   1871:                   i + 1);
>  1872:           add_error(ctx, &ctx->geometry.errors, l_msg, 1);
   1873:           return;
   1874:         }
   1875: 
   1876:         ctx->geometry.px[nx]= ctx->geometry.px[i];
   1877:         ctx->geometry.py[nx]= -ctx->geometry.py[i];

-- Message variable name candidates --
VAR_EXPR: l_msg  VAR_NAME: l_msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L1706:           snprintf(l_msg, sizeof(l_msg),
L1762:           snprintf(l_msg, sizeof(l_msg),
L1816:           snprintf(l_msg, sizeof(l_msg),
L1868:           snprintf(l_msg, sizeof(l_msg),

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/geometry.c
LINE: 1933
CALL: add_error(ctx, &ctx->geometry.errors, l_msg, 1);
--- Context (lines 1873-1938) ---
   1873:           return;
   1874:         }
   1875: 
   1876:         ctx->geometry.px[nx]= ctx->geometry.px[i];
   1877:         ctx->geometry.py[nx]= -ctx->geometry.py[i];
   1878:         ctx->geometry.pz[nx]= ctx->geometry.pz[i];
   1879:         ctx->geometry.t1x[nx]= -ctx->geometry.t1x[i];
   1880:         ctx->geometry.t1y[nx]= ctx->geometry.t1y[i];
   1881:         ctx->geometry.t1z[nx]= ctx->geometry.t1z[i];
   1882:         ctx->geometry.t2x[nx]= -ctx->geometry.t2x[i];
   1883:         ctx->geometry.t2y[nx]= ctx->geometry.t2y[i];
   1884:         ctx->geometry.t2z[nx]= ctx->geometry.t2z[i];
   1885:         ctx->geometry.psalp[nx]= -ctx->geometry.psalp[i];
   1886:         ctx->geometry.pbi[nx]= ctx->geometry.pbi[i];
   1887: 
   1888:       } /* for( i = m2; i <= ctx->geometry.m; i++ ) */
   1889: 
   1890:       ctx->geometry.m= ctx->geometry.m * 2;
   1891:     } /* if( ctx->geometry.m >= m2) */
   1892:   } /* if( iy != 0) */
   1893: 
   1894:   // and finally the x axis
   1895:   if(ix == 0) {
   1896:     /* When NGF is active, clear symmetry flag — per NEC-2 spec GX does not
   1897:      * result in use of symmetry in the solution when NGF is in use. */
   1898:     if(ctx->has_ngf) ctx->geometry.ipsym = 0;
   1899:     return;
   1900:   }
   1901: 
   1902:   if( ctx->geometry.n > n0 ) {
   1903:     int nn = ctx->geometry.n;
   1904:     int new_count = nn - n0;
   1905: 
   1906:     /* Reallocate tags buffer */
   1907:     mreq = (size_t)(nn + new_count + ctx->geometry.m);
   1908:     mreq *= sizeof(int);
   1909:     mem_realloc(ctx, (void *)&ctx->geometry.tag_nums, mreq);
   1910: 
   1911:     /* Reallocate wire buffers */
   1912:     mreq = (size_t)(nn + new_count);
   1913:     mreq *= sizeof(double);
   1914:     mem_realloc(ctx, (void *)&ctx->geometry.x1, mreq);
   1915:     mem_realloc(ctx, (void *)&ctx->geometry.y1, mreq);
   1916:     mem_realloc(ctx, (void *)&ctx->geometry.z1, mreq);
   1917:     mem_realloc(ctx, (void *)&ctx->geometry.x2, mreq);
   1918:     mem_realloc(ctx, (void *)&ctx->geometry.y2, mreq);
   1919:     mem_realloc(ctx, (void *)&ctx->geometry.z2, mreq);
   1920:     mem_realloc(ctx, (void *)&ctx->geometry.bi, mreq);
   1921: 
   1922:     for(i = n0; i < nn; i++) {
   1923:       nx = nn + (i - n0);
   1924:       e1= ctx->geometry.x1[i];
   1925:       e2= ctx->geometry.x2[i];
   1926: 
   1927:       if( (fabs(e1)+fabs(e2) <= 1.0e-12) || (e1*e2 < -1.0e-12) ) {
   1928:         char l_msg[MAX_ERROR_LEN];
   1929:         snprintf(l_msg, sizeof(l_msg),
   1930:                 "\n  GEOMETRY DATA ERROR--SEGMENT %d"
   1931:                 " LIES IN PLANE OF SYMMETRY",
   1932:                 i + 1);
>  1933:         add_error(ctx, &ctx->geometry.errors, l_msg, 1);
   1934:         return;
   1935:       }
   1936: 
   1937:       ctx->geometry.x1[nx]= -e1;
   1938:       ctx->geometry.y1[nx]= ctx->geometry.y1[i];

-- Message variable name candidates --
VAR_EXPR: l_msg  VAR_NAME: l_msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L1762:           snprintf(l_msg, sizeof(l_msg),
L1816:           snprintf(l_msg, sizeof(l_msg),
L1868:           snprintf(l_msg, sizeof(l_msg),
L1929:         snprintf(l_msg, sizeof(l_msg),

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/geometry.c
LINE: 1986
CALL: add_error(ctx, &ctx->geometry.errors, l_msg, 1);
--- Context (lines 1926-1991) ---
   1926: 
   1927:       if( (fabs(e1)+fabs(e2) <= 1.0e-12) || (e1*e2 < -1.0e-12) ) {
   1928:         char l_msg[MAX_ERROR_LEN];
   1929:         snprintf(l_msg, sizeof(l_msg),
   1930:                 "\n  GEOMETRY DATA ERROR--SEGMENT %d"
   1931:                 " LIES IN PLANE OF SYMMETRY",
   1932:                 i + 1);
   1933:         add_error(ctx, &ctx->geometry.errors, l_msg, 1);
   1934:         return;
   1935:       }
   1936: 
   1937:       ctx->geometry.x1[nx]= -e1;
   1938:       ctx->geometry.y1[nx]= ctx->geometry.y1[i];
   1939:       ctx->geometry.z1[nx]= ctx->geometry.z1[i];
   1940:       ctx->geometry.x2[nx]= -e2;
   1941:       ctx->geometry.y2[nx]= ctx->geometry.y2[i];
   1942:       ctx->geometry.z2[nx]= ctx->geometry.z2[i];
   1943:       itagi= ctx->geometry.tag_nums[i];
   1944: 
   1945:       if(itagi == 0)
   1946:         ctx->geometry.tag_nums[nx]=0;
   1947:       if(itagi != 0)
   1948:         ctx->geometry.tag_nums[nx]= itagi + iti;
   1949: 
   1950:       ctx->geometry.bi[nx]= ctx->geometry.bi[i];
   1951:     }
   1952: 
   1953:     ctx->geometry.n = nn + new_count;
   1954: 
   1955:   } /* if( data.n > n0) */
   1956: 
   1957:   if(ctx->geometry.m == 0) {
   1958:     /* When NGF is active, clear symmetry flag. */
   1959:     if(ctx->has_ngf) ctx->geometry.ipsym = 0;
   1960:     return;
   1961:   }
   1962: 
   1963:   /* Reallocate patch buffers */
   1964:   mreq = (size_t)(2 * ctx->geometry.m);
   1965:   mreq *= sizeof(double);
   1966:   mem_realloc(ctx, (void *)&ctx->geometry.px, mreq);
   1967:   mem_realloc(ctx, (void *)&ctx->geometry.py, mreq);
   1968:   mem_realloc(ctx, (void *)&ctx->geometry.pz, mreq);
   1969:   mem_realloc(ctx, (void *)&ctx->geometry.t1x, mreq);
   1970:   mem_realloc(ctx, (void *)&ctx->geometry.t1y, mreq);
   1971:   mem_realloc(ctx, (void *)&ctx->geometry.t1z, mreq);
   1972:   mem_realloc(ctx, (void *)&ctx->geometry.t2x, mreq);
   1973:   mem_realloc(ctx, (void *)&ctx->geometry.t2y, mreq);
   1974:   mem_realloc(ctx, (void *)&ctx->geometry.t2z, mreq);
   1975:   mem_realloc(ctx, (void *)&ctx->geometry.pbi, mreq);
   1976:   mem_realloc(ctx, (void *)&ctx->geometry.psalp, mreq);
   1977: 
   1978:   for( i = 0; i < ctx->geometry.m; i++ ) {
   1979:     nx = i+ctx->geometry.m;
   1980:     if(fabs(ctx->geometry.px[i]) <= 1.0e-10) {
   1981:       char l_msg[MAX_ERROR_LEN];
   1982:       snprintf(l_msg, sizeof(l_msg),
   1983:               "\n  GEOMETRY DATA ERROR--PATCH %d"
   1984:               " LIES IN PLANE OF SYMMETRY",
   1985:               i + 1);
>  1986:       add_error(ctx, &ctx->geometry.errors, l_msg, 1);
   1987:       return;
   1988:     }
   1989: 
   1990:     ctx->geometry.px[nx]= -ctx->geometry.px[i];
   1991:     ctx->geometry.py[nx]= ctx->geometry.py[i];

-- Message variable name candidates --
VAR_EXPR: l_msg  VAR_NAME: l_msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L1816:           snprintf(l_msg, sizeof(l_msg),
L1868:           snprintf(l_msg, sizeof(l_msg),
L1929:         snprintf(l_msg, sizeof(l_msg),
L1982:       snprintf(l_msg, sizeof(l_msg),

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/geometry.c
LINE: 2289
CALL: add_error(ctx, &ctx->geometry.errors, msg, 1);
--- Context (lines 2229-2294) ---
   2229:     s2z = az3 - az2;
   2230:     
   2231:     if(nx != 0) {
   2232:       s1x = s1x / nx;
   2233:       s1y = s1y / nx;
   2234:       s1z = s1z / nx;
   2235:       s2x = s2x / ny;
   2236:       s2y = s2y / ny;
   2237:       s2z = s2z / ny;
   2238:     }
   2239:     
   2240:     xnv = s1y * s2z - s1z * s2y;
   2241:     ynv = s1z * s2x - s1x * s2z;
   2242:     znv = s1x * s2y - s1y * s2x;
   2243:     xa = sqrt(xnv * xnv + ynv * ynv + znv * znv);
   2244:     xnv = xnv/ xa;
   2245:     ynv = ynv/ xa;
   2246:     znv = znv/ xa;
   2247:     xst = sqrt( s1x* s1x+ s1y* s1y+ s1z* s1z);
   2248:     geom->t1x[mi] = s1x / xst;
   2249:     geom->t1y[mi] = s1y / xst;
   2250:     geom->t1z[mi] = s1z / xst;
   2251:     
   2252:     if(ntp <= 2) {
   2253:       geom->px[mi] = ax1 + 0.5 * (s1x + s2x);
   2254:       geom->py[mi] = ay1 + 0.5 * (s1y + s2y);
   2255:       geom->pz[mi] = az1 + 0.5 * (s1z + s2z);
   2256:       geom->pbi[mi] = xa;
   2257:     }
   2258:     else {
   2259:       if( ntp != 4) {
   2260:         geom->px[mi] = (ax1 + ax2 + ax3) / 3.0;
   2261:         geom->py[mi] = (ay1 + ay2 + ay3) / 3.0;
   2262:         geom->pz[mi] = (az1 + az2 + az3) / 3.0;
   2263:         geom->pbi[mi] = 0.5 * xa;
   2264:       }
   2265:       else  {
   2266:         double salpn;
   2267:         s1x= ax3- ax1;
   2268:         s1y= ay3- ay1;
   2269:         s1z= az3- az1;
   2270:         s2x= ax4- ax1;
   2271:         s2y= ay4- ay1;
   2272:         s2z= az4- az1;
   2273:         xn2= s1y* s2z- s1z* s2y;
   2274:         yn2= s1z* s2x- s1x* s2z;
   2275:         zn2= s1x* s2y- s1y* s2x;
   2276:         xst= sqrt( xn2* xn2+ yn2* yn2+ zn2* zn2);
   2277:         salpn=1./(3.*( xa+ xst));
   2278:         geom->px[mi]=( xa*( ax1+ ax2+ ax3)+ xst*( ax1+ ax3+ ax4))* salpn;
   2279:         geom->py[mi]=( xa*( ay1+ ay2+ ay3)+ xst*( ay1+ ay3+ ay4))* salpn;
   2280:         geom->pz[mi]=( xa*( az1+ az2+ az3)+ xst*( az1+ az3+ az4))* salpn;
   2281:         geom->pbi[mi]=.5*( xa+ xst);
   2282:         s1x=( xnv* xn2+ ynv* yn2+ znv* zn2)/ xst;
   2283:         
   2284:         if(s1x <= 0.9998) {
   2285:           char msg[MAX_ERROR_LEN];
   2286:           snprintf(msg, sizeof(msg),
   2287:                   "\n  ERROR -- CORNERS OF QUADRILATERAL"
   2288:                   " PATCH DO NOT LIE IN A PLANE" );
>  2289:           add_error(ctx, &ctx->geometry.errors, msg, 1);
   2290:           return;
   2291:         }
   2292:       } /* if( ntp != 4) */
   2293:     } /* if( ntp <= 2) */
   2294:   } /* if( ntp <= 1) */

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L2286:           snprintf(msg, sizeof(msg),

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/control.c
LINE: 304
CALL: add_error(ctx, &ctx->errors, nx_geom_errors.errors[i].message,
--- Context (lines 244-309) ---
    244:                 }
    245:             }
    246: 
    247:             if (new_geom_start == -1 || new_geom_end == -1 ||
    248:                 strcmp(deck->cards[new_geom_end].card_code, "GE") != 0) {
    249:                 /* No geometry follows NX — treat as terminal (like EN).
    250:                  * This covers:
    251:                  *   - NX as the last card in the deck
    252:                  *   - NX followed only by EN (no new geometry section) */
    253:                 if (ctx->output_fp != NULL && ctx->frequency_loop_ran) {
    254:                     deck->deck_end = nx_pos;
    255:                     write_nec_output(ctx, deck, ctx->output_fp);
    256:                     deck->deck_end = -1;
    257:                     ctx->frequency_loop_ran = false; /* prevent double write in main */
    258:                 }
    259:                 deck_complete = true;
    260:                 break;
    261:             }
    262: 
    263:             /* Step 2: Flush output for the completed section.
    264:              * Temporarily set deck_end to the NX card so write_input_cards
    265:              * prints section 1's control cards (FR/EX/RP/NX range). */
    266:             if (ctx->output_fp != NULL && ctx->frequency_loop_ran) {
    267:                 deck->deck_end = nx_pos;
    268:                 write_nec_output(ctx, deck, ctx->output_fp);
    269:                 deck->deck_end = -1;  /* restore: section 1 has no EN */
    270:             }
    271: 
    272:             /* Step 3: Reset all per-section simulation state. */
    273:             reset_loading_buffers(ctx);
    274:             reset_network_buffers(ctx);
    275:             reset_coupling_buffers(ctx);
    276:             reset_vsorc_buffers(ctx);
    277: 
    278:             if (ctx->rpat.points != NULL) { free(ctx->rpat.points); ctx->rpat.points = NULL; }
    279:             ctx->rpat.num_points = 0;
    280: 
    281:             if (ctx->yparm.coupling_rows != NULL) {
    282:                 free(ctx->yparm.coupling_rows);
    283:                 ctx->yparm.coupling_rows = NULL;
    284:                 ctx->yparm.num_coupling_rows = 0;
    285:                 ctx->yparm.coupling_rows_cap = 0;
    286:             }
    287: 
    288:             if (ctx->ngf_cm != NULL) { free(ctx->ngf_cm); ctx->ngf_cm = NULL; }
    289:             ctx->has_ngf = false; ctx->ngf_n_segs = 0; ctx->ngf_neq = 0; ctx->ngf_fmhz = 0.0;
    290: 
    291:             /* Step 4: Update deck section pointers to the new section and re-run geometry. */
    292:             deck->comment_start  = new_comment_start;
    293:             deck->comment_end    = new_comment_end;
    294:             deck->symbol_start   = new_sym_start;
    295:             deck->symbol_end     = new_sym_end;
    296:             deck->geometry_start = new_geom_start;
    297:             deck->geometry_end   = new_geom_end;
    298:             deck->deck_end       = new_deck_end;
    299: 
    300:             errors_list_t nx_geom_errors = {0};
    301:             calculate_geometry(ctx, deck, &nx_geom_errors, &ctx->outputs);
    302:             if (nx_geom_errors.num_errors > 0) {
    303:                 for (int i = 0; i < nx_geom_errors.num_errors; i++)
>   304:                     add_error(ctx, &ctx->errors, nx_geom_errors.errors[i].message,
    305:                              nx_geom_errors.errors[i].severity);
    306:                 return -1;
    307:             }
    308:             if (nec_calculation_defaults(ctx) != 0) {
    309:                 add_error(ctx, &ctx->errors,

-- Message variable name candidates --
VAR_EXPR: nx_geom_errors.errors[i].message  VAR_NAME: nx_geom_errors.errors.message
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
NONE FOUND - manual review needed

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/control.c
LINE: 309
CALL: add_error(ctx, &ctx->errors,
--- Context (lines 249-314) ---
    249:                 /* No geometry follows NX — treat as terminal (like EN).
    250:                  * This covers:
    251:                  *   - NX as the last card in the deck
    252:                  *   - NX followed only by EN (no new geometry section) */
    253:                 if (ctx->output_fp != NULL && ctx->frequency_loop_ran) {
    254:                     deck->deck_end = nx_pos;
    255:                     write_nec_output(ctx, deck, ctx->output_fp);
    256:                     deck->deck_end = -1;
    257:                     ctx->frequency_loop_ran = false; /* prevent double write in main */
    258:                 }
    259:                 deck_complete = true;
    260:                 break;
    261:             }
    262: 
    263:             /* Step 2: Flush output for the completed section.
    264:              * Temporarily set deck_end to the NX card so write_input_cards
    265:              * prints section 1's control cards (FR/EX/RP/NX range). */
    266:             if (ctx->output_fp != NULL && ctx->frequency_loop_ran) {
    267:                 deck->deck_end = nx_pos;
    268:                 write_nec_output(ctx, deck, ctx->output_fp);
    269:                 deck->deck_end = -1;  /* restore: section 1 has no EN */
    270:             }
    271: 
    272:             /* Step 3: Reset all per-section simulation state. */
    273:             reset_loading_buffers(ctx);
    274:             reset_network_buffers(ctx);
    275:             reset_coupling_buffers(ctx);
    276:             reset_vsorc_buffers(ctx);
    277: 
    278:             if (ctx->rpat.points != NULL) { free(ctx->rpat.points); ctx->rpat.points = NULL; }
    279:             ctx->rpat.num_points = 0;
    280: 
    281:             if (ctx->yparm.coupling_rows != NULL) {
    282:                 free(ctx->yparm.coupling_rows);
    283:                 ctx->yparm.coupling_rows = NULL;
    284:                 ctx->yparm.num_coupling_rows = 0;
    285:                 ctx->yparm.coupling_rows_cap = 0;
    286:             }
    287: 
    288:             if (ctx->ngf_cm != NULL) { free(ctx->ngf_cm); ctx->ngf_cm = NULL; }
    289:             ctx->has_ngf = false; ctx->ngf_n_segs = 0; ctx->ngf_neq = 0; ctx->ngf_fmhz = 0.0;
    290: 
    291:             /* Step 4: Update deck section pointers to the new section and re-run geometry. */
    292:             deck->comment_start  = new_comment_start;
    293:             deck->comment_end    = new_comment_end;
    294:             deck->symbol_start   = new_sym_start;
    295:             deck->symbol_end     = new_sym_end;
    296:             deck->geometry_start = new_geom_start;
    297:             deck->geometry_end   = new_geom_end;
    298:             deck->deck_end       = new_deck_end;
    299: 
    300:             errors_list_t nx_geom_errors = {0};
    301:             calculate_geometry(ctx, deck, &nx_geom_errors, &ctx->outputs);
    302:             if (nx_geom_errors.num_errors > 0) {
    303:                 for (int i = 0; i < nx_geom_errors.num_errors; i++)
    304:                     add_error(ctx, &ctx->errors, nx_geom_errors.errors[i].message,
    305:                              nx_geom_errors.errors[i].severity);
    306:                 return -1;
    307:             }
    308:             if (nec_calculation_defaults(ctx) != 0) {
>   309:                 add_error(ctx, &ctx->errors,
    310:                     "NX: failed to initialize calculation defaults for new section", FATAL);
    311:                 return -1;
    312:             }
    313: 
    314:             ctx->current_card_idx = new_geom_end + 1;

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L138:                 snprintf(cmsg, sizeof(cmsg),
L144:                 snprintf(cmsg, sizeof(cmsg),

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/control.c
LINE: 613
CALL: add_error(ctx, &ctx->errors, msg, FATAL);
--- Context (lines 553-618) ---
    553:             }
    554:         }
    555:     }
    556:     if (!found_batch_end) {
    557:         is_final_batch = true;  // Implicit EN at end of deck
    558:     }
    559:     
    560:     // Now process the control cards in this batch to configure ctx
    561:     for (int card_idx = *batch_start; card_idx <= *batch_end; card_idx++) {
    562:         card_t *card = &deck->cards[card_idx];
    563:         
    564:         // Skip ignored, comment, or empty cards
    565:         if (card->ignore || is_comment(card)) {
    566:             continue;
    567:         }
    568:         
    569:         char *code = card->card_code;
    570:         
    571:         // Check if this is a SY card - if so, evaluate its formulas and continue to next card
    572:         if (strcmp(code, "SY") == 0) {
    573:             if (card->formulas) {
    574:                 key_value_t *kv = card->formulas;
    575:                 while (kv) {
    576:                     evaluate_formula(ctx, kv, deck, &ctx->errors);
    577:                     kv = kv->next;
    578:                 }
    579:             }
    580:             continue; // Skip to next card, SY cards don't configure anything
    581:         }
    582:         
    583:         // Skip XQ, EN, XT, NX cards (they don't configure anything)
    584:         if (strcmp(code, "XQ") == 0 || strcmp(code, "EN") == 0 || strcmp(code, "XT") == 0 || strcmp(code, "NX") == 0) {
    585:             continue;
    586:         }
    587:         
    588:         // Get field values for convenience
    589:         int i1 = card->i[1], i2 = card->i[2], i3 = card->i[3], i4 = card->i[4];
    590:         double f1 = card->f[1], f2 = card->f[2], f3 = card->f[3];
    591:         double f4 = card->f[4], f5 = card->f[5], f6 = card->f[6];
    592:         
    593:         // Process based on card type
    594:         if (strcmp(code, "FR") == 0) {
    595:             // FR card - Frequency specification
    596:             if (ctx->iflow != 1) {
    597:                 ctx->iflow = 1;
    598:             }
    599:             ctx->save.ifrq = i1;
    600:             ctx->save.nfrq = (i2 == 0) ? 1 : i2;
    601:             ctx->save.fmhz = f1;
    602:             ctx->save.delfrq = f2;
    603:         }
    604:         else if (strcmp(code, "LD") == 0) {
    605:             // LD card - Loading
    606:             if (i1 == -1) {
    607:                 continue;
    608:             }
    609: 
    610:             if (i1 > 5) {
    611:                 char msg[MAX_ERROR_LEN];
    612:                 snprintf(msg, sizeof(msg), "Card %d is an LD card with type %d, which is not supported.", card_idx + 1, i1);
>   613:                 add_error(ctx, &ctx->errors, msg, FATAL);
    614:                 return -1;
    615:             }
    616:             
    617:             // First LD in batch resets loading (iflow transition to 3)
    618:             if (ctx->iflow != 3 && ctx->zload.nload == 0) {

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L612:                 snprintf(msg, sizeof(msg), "Card %d is an LD card with type %d, which is not supported.", card_idx + 1, i1);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/control.c
LINE: 648
CALL: add_error(ctx, &ctx->errors, msg, FATAL);
--- Context (lines 588-653) ---
    588:         // Get field values for convenience
    589:         int i1 = card->i[1], i2 = card->i[2], i3 = card->i[3], i4 = card->i[4];
    590:         double f1 = card->f[1], f2 = card->f[2], f3 = card->f[3];
    591:         double f4 = card->f[4], f5 = card->f[5], f6 = card->f[6];
    592:         
    593:         // Process based on card type
    594:         if (strcmp(code, "FR") == 0) {
    595:             // FR card - Frequency specification
    596:             if (ctx->iflow != 1) {
    597:                 ctx->iflow = 1;
    598:             }
    599:             ctx->save.ifrq = i1;
    600:             ctx->save.nfrq = (i2 == 0) ? 1 : i2;
    601:             ctx->save.fmhz = f1;
    602:             ctx->save.delfrq = f2;
    603:         }
    604:         else if (strcmp(code, "LD") == 0) {
    605:             // LD card - Loading
    606:             if (i1 == -1) {
    607:                 continue;
    608:             }
    609: 
    610:             if (i1 > 5) {
    611:                 char msg[MAX_ERROR_LEN];
    612:                 snprintf(msg, sizeof(msg), "Card %d is an LD card with type %d, which is not supported.", card_idx + 1, i1);
    613:                 add_error(ctx, &ctx->errors, msg, FATAL);
    614:                 return -1;
    615:             }
    616:             
    617:             // First LD in batch resets loading (iflow transition to 3)
    618:             if (ctx->iflow != 3 && ctx->zload.nload == 0) {
    619:                 reset_loading_buffers(ctx);
    620:                 ctx->iflow = 3;
    621:             }
    622:             
    623:             // Reallocate loading buffers
    624:             ctx->zload.nload++;
    625:             size_t mreq = (size_t)ctx->zload.nload * sizeof(int);
    626:             mem_realloc(ctx, (void **)&ctx->zload.ldtyp, mreq);
    627:             mem_realloc(ctx, (void **)&ctx->zload.ldtag, mreq);
    628:             mem_realloc(ctx, (void **)&ctx->zload.ldtagf, mreq);
    629:             mem_realloc(ctx, (void **)&ctx->zload.ldtagt, mreq);
    630:             
    631:             mreq = (size_t)ctx->zload.nload * sizeof(double);
    632:             mem_realloc(ctx, (void **)&ctx->zload.zlr, mreq);
    633:             mem_realloc(ctx, (void **)&ctx->zload.zli, mreq);
    634:             mem_realloc(ctx, (void **)&ctx->zload.zlc, mreq);
    635:             
    636:             int idx = ctx->zload.nload - 1;
    637:             ctx->zload.ldtyp[idx] = i1;
    638:             ctx->zload.ldtag[idx] = i2;
    639:             ctx->zload.ldtagf[idx] = (i4 == 0) ? i3 : i3;
    640:             ctx->zload.ldtagt[idx] = (i4 == 0) ? i3 : i4;
    641:             
    642:             if (ctx->zload.ldtagt[idx] < ctx->zload.ldtagf[idx]) {
    643:                 char msg[MAX_ERROR_LEN];
    644:                 snprintf(msg, sizeof(msg),
    645:                     "DATA FAULT ON LOADING CARD No: %d: ITAG "
    646:                     "STEP1: %d IS GREATER THAN ITAG STEP2: %d",
    647:                     ctx->zload.nload, i3, i4);
>   648:                 add_error(ctx, &ctx->errors, msg, FATAL);
    649:                 return -1;
    650:             }
    651:             
    652:             ctx->zload.zlr[idx] = f1;
    653:             ctx->zload.zli[idx] = f2;

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L612:                 snprintf(msg, sizeof(msg), "Card %d is an LD card with type %d, which is not supported.", card_idx + 1, i1);
L644:                 snprintf(msg, sizeof(msg),

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/control.c
LINE: 673
CALL: add_error(ctx, &ctx->errors,
--- Context (lines 613-678) ---
    613:                 add_error(ctx, &ctx->errors, msg, FATAL);
    614:                 return -1;
    615:             }
    616:             
    617:             // First LD in batch resets loading (iflow transition to 3)
    618:             if (ctx->iflow != 3 && ctx->zload.nload == 0) {
    619:                 reset_loading_buffers(ctx);
    620:                 ctx->iflow = 3;
    621:             }
    622:             
    623:             // Reallocate loading buffers
    624:             ctx->zload.nload++;
    625:             size_t mreq = (size_t)ctx->zload.nload * sizeof(int);
    626:             mem_realloc(ctx, (void **)&ctx->zload.ldtyp, mreq);
    627:             mem_realloc(ctx, (void **)&ctx->zload.ldtag, mreq);
    628:             mem_realloc(ctx, (void **)&ctx->zload.ldtagf, mreq);
    629:             mem_realloc(ctx, (void **)&ctx->zload.ldtagt, mreq);
    630:             
    631:             mreq = (size_t)ctx->zload.nload * sizeof(double);
    632:             mem_realloc(ctx, (void **)&ctx->zload.zlr, mreq);
    633:             mem_realloc(ctx, (void **)&ctx->zload.zli, mreq);
    634:             mem_realloc(ctx, (void **)&ctx->zload.zlc, mreq);
    635:             
    636:             int idx = ctx->zload.nload - 1;
    637:             ctx->zload.ldtyp[idx] = i1;
    638:             ctx->zload.ldtag[idx] = i2;
    639:             ctx->zload.ldtagf[idx] = (i4 == 0) ? i3 : i3;
    640:             ctx->zload.ldtagt[idx] = (i4 == 0) ? i3 : i4;
    641:             
    642:             if (ctx->zload.ldtagt[idx] < ctx->zload.ldtagf[idx]) {
    643:                 char msg[MAX_ERROR_LEN];
    644:                 snprintf(msg, sizeof(msg),
    645:                     "DATA FAULT ON LOADING CARD No: %d: ITAG "
    646:                     "STEP1: %d IS GREATER THAN ITAG STEP2: %d",
    647:                     ctx->zload.nload, i3, i4);
    648:                 add_error(ctx, &ctx->errors, msg, FATAL);
    649:                 return -1;
    650:             }
    651:             
    652:             ctx->zload.zlr[idx] = f1;
    653:             ctx->zload.zli[idx] = f2;
    654:             ctx->zload.zlc[idx] = f3;
    655:         }
    656:         else if (strcmp(code, "GN") == 0) {
    657:             // GN card - Ground parameters  
    658:             if (i1 == -1) {
    659:                 ctx->gnd.ksymp = 1;
    660:                 ctx->gnd.nradl = 0;
    661:                 ctx->gnd.iperf = 0;
    662:                 continue;
    663:             }
    664:             
    665:             ctx->gnd.iperf = i1;
    666:             ctx->gnd.nradl = i2;
    667:             ctx->gnd.ksymp = 2;
    668:             ctx->save.epsr = f1;
    669:             ctx->save.sig = f2;
    670:             
    671:             if (ctx->gnd.nradl != 0) {
    672:                 if (ctx->gnd.iperf == 2) {
>   673:                     add_error(ctx, &ctx->errors,
    674:                         "RADIAL WIRE G.S. APPROXIMATION MAY "
    675:                         "NOT BE USED WITH SOMMERFELD GROUND OPTION", FATAL);
    676:                     return -1;
    677:                 }
    678:                 if (f3 >= 1.0e-20 || f4 >= 1.0e-20) {

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L612:                 snprintf(msg, sizeof(msg), "Card %d is an LD card with type %d, which is not supported.", card_idx + 1, i1);
L644:                 snprintf(msg, sizeof(msg),

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/control.c
LINE: 694
CALL: add_error(ctx, &ctx->errors, msg, WARNING);
--- Context (lines 634-699) ---
    634:             mem_realloc(ctx, (void **)&ctx->zload.zlc, mreq);
    635:             
    636:             int idx = ctx->zload.nload - 1;
    637:             ctx->zload.ldtyp[idx] = i1;
    638:             ctx->zload.ldtag[idx] = i2;
    639:             ctx->zload.ldtagf[idx] = (i4 == 0) ? i3 : i3;
    640:             ctx->zload.ldtagt[idx] = (i4 == 0) ? i3 : i4;
    641:             
    642:             if (ctx->zload.ldtagt[idx] < ctx->zload.ldtagf[idx]) {
    643:                 char msg[MAX_ERROR_LEN];
    644:                 snprintf(msg, sizeof(msg),
    645:                     "DATA FAULT ON LOADING CARD No: %d: ITAG "
    646:                     "STEP1: %d IS GREATER THAN ITAG STEP2: %d",
    647:                     ctx->zload.nload, i3, i4);
    648:                 add_error(ctx, &ctx->errors, msg, FATAL);
    649:                 return -1;
    650:             }
    651:             
    652:             ctx->zload.zlr[idx] = f1;
    653:             ctx->zload.zli[idx] = f2;
    654:             ctx->zload.zlc[idx] = f3;
    655:         }
    656:         else if (strcmp(code, "GN") == 0) {
    657:             // GN card - Ground parameters  
    658:             if (i1 == -1) {
    659:                 ctx->gnd.ksymp = 1;
    660:                 ctx->gnd.nradl = 0;
    661:                 ctx->gnd.iperf = 0;
    662:                 continue;
    663:             }
    664:             
    665:             ctx->gnd.iperf = i1;
    666:             ctx->gnd.nradl = i2;
    667:             ctx->gnd.ksymp = 2;
    668:             ctx->save.epsr = f1;
    669:             ctx->save.sig = f2;
    670:             
    671:             if (ctx->gnd.nradl != 0) {
    672:                 if (ctx->gnd.iperf == 2) {
    673:                     add_error(ctx, &ctx->errors,
    674:                         "RADIAL WIRE G.S. APPROXIMATION MAY "
    675:                         "NOT BE USED WITH SOMMERFELD GROUND OPTION", FATAL);
    676:                     return -1;
    677:                 }
    678:                 if (f3 >= 1.0e-20 || f4 >= 1.0e-20) {
    679:                     ctx->save.scrwlt = f3;
    680:                     ctx->save.scrwrt = f4;
    681:                 }
    682:             }
    683:         }
    684:         // Continue processing other cards...
    685:         else if (strcmp(code, "EX") == 0) {
    686:             // EX card - Excitation
    687:             ctx->fpat.ixtyp = i1;
    688:             ctx->netcx.masym = i4 / 10;
    689:             
    690:             // warn about unsupported EX types
    691:             if (i1 == 6 || i1 == 7) {
    692:                 char msg[MAX_ERROR_LEN];
    693:                 snprintf(msg, sizeof(msg), "Card %d is an EX card with type %d, which is not supported.", card_idx + 1, i1);
>   694:                 add_error(ctx, &ctx->errors, msg, WARNING);
    695:             }
    696:             
    697:             // For voltage source types (0 and 5)
    698:             if (i1 == 0 || i1 == 5) {
    699:                 ctx->netcx.ntsol = 0;

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L612:                 snprintf(msg, sizeof(msg), "Card %d is an LD card with type %d, which is not supported.", card_idx + 1, i1);
L644:                 snprintf(msg, sizeof(msg),
L693:                 snprintf(msg, sizeof(msg), "Card %d is an EX card with type %d, which is not supported.", card_idx + 1, i1);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/control.c
LINE: 718
CALL: add_error(ctx, &ctx->errors, msg, FATAL);
--- Context (lines 658-723) ---
    658:             if (i1 == -1) {
    659:                 ctx->gnd.ksymp = 1;
    660:                 ctx->gnd.nradl = 0;
    661:                 ctx->gnd.iperf = 0;
    662:                 continue;
    663:             }
    664:             
    665:             ctx->gnd.iperf = i1;
    666:             ctx->gnd.nradl = i2;
    667:             ctx->gnd.ksymp = 2;
    668:             ctx->save.epsr = f1;
    669:             ctx->save.sig = f2;
    670:             
    671:             if (ctx->gnd.nradl != 0) {
    672:                 if (ctx->gnd.iperf == 2) {
    673:                     add_error(ctx, &ctx->errors,
    674:                         "RADIAL WIRE G.S. APPROXIMATION MAY "
    675:                         "NOT BE USED WITH SOMMERFELD GROUND OPTION", FATAL);
    676:                     return -1;
    677:                 }
    678:                 if (f3 >= 1.0e-20 || f4 >= 1.0e-20) {
    679:                     ctx->save.scrwlt = f3;
    680:                     ctx->save.scrwrt = f4;
    681:                 }
    682:             }
    683:         }
    684:         // Continue processing other cards...
    685:         else if (strcmp(code, "EX") == 0) {
    686:             // EX card - Excitation
    687:             ctx->fpat.ixtyp = i1;
    688:             ctx->netcx.masym = i4 / 10;
    689:             
    690:             // warn about unsupported EX types
    691:             if (i1 == 6 || i1 == 7) {
    692:                 char msg[MAX_ERROR_LEN];
    693:                 snprintf(msg, sizeof(msg), "Card %d is an EX card with type %d, which is not supported.", card_idx + 1, i1);
    694:                 add_error(ctx, &ctx->errors, msg, WARNING);
    695:             }
    696:             
    697:             // For voltage source types (0 and 5)
    698:             if (i1 == 0 || i1 == 5) {
    699:                 ctx->netcx.ntsol = 0;
    700:                 
    701:                 if (i1 == 5) {
    702:                     // Incident plane wave or elementary current source
    703:                     ctx->vsorc.nvqd++;
    704:                     size_t mreq = (size_t)ctx->vsorc.nvqd * sizeof(int);
    705:                     mem_realloc(ctx, (void **)&ctx->vsorc.ivqd, mreq);
    706:                     mem_realloc(ctx, (void **)&ctx->vsorc.iqds, mreq);
    707:                     
    708:                     mreq = (size_t)ctx->vsorc.nvqd * sizeof(complex double);
    709:                     mem_realloc(ctx, (void **)&ctx->vsorc.vqd, mreq);
    710:                     mem_realloc(ctx, (void **)&ctx->vsorc.vqds, mreq);
    711:                     
    712:                     int idx = ctx->vsorc.nvqd - 1;
    713:                     int i3_resolved = resolve_pct_segment(ctx, card, 3, i2);
    714:                     int seg_num = segment_number(ctx, i2, i3_resolved);
    715:                     if (seg_num == 0) {
    716:                         char msg[MAX_ERROR_LEN];
    717:                         snprintf(msg, sizeof(msg), "Card %d is an EX that references invalid tag %d, segment %d", card_idx + 1, i2, i3_resolved);
>   718:                         add_error(ctx, &ctx->errors, msg, FATAL);
    719:                         return -1;
    720:                     }
    721:                     ctx->vsorc.ivqd[idx] = seg_num;
    722:                     ctx->vsorc.vqd[idx] = f1 + I * f2;
    723:                     if (cabs(ctx->vsorc.vqd[idx]) < 1.e-20) {

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L612:                 snprintf(msg, sizeof(msg), "Card %d is an LD card with type %d, which is not supported.", card_idx + 1, i1);
L644:                 snprintf(msg, sizeof(msg),
L693:                 snprintf(msg, sizeof(msg), "Card %d is an EX card with type %d, which is not supported.", card_idx + 1, i1);
L717:                         snprintf(msg, sizeof(msg), "Card %d is an EX that references invalid tag %d, segment %d", card_idx + 1, i2, i3_resolved);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/control.c
LINE: 741
CALL: add_error(ctx, &ctx->errors, msg, FATAL);
--- Context (lines 681-746) ---
    681:                 }
    682:             }
    683:         }
    684:         // Continue processing other cards...
    685:         else if (strcmp(code, "EX") == 0) {
    686:             // EX card - Excitation
    687:             ctx->fpat.ixtyp = i1;
    688:             ctx->netcx.masym = i4 / 10;
    689:             
    690:             // warn about unsupported EX types
    691:             if (i1 == 6 || i1 == 7) {
    692:                 char msg[MAX_ERROR_LEN];
    693:                 snprintf(msg, sizeof(msg), "Card %d is an EX card with type %d, which is not supported.", card_idx + 1, i1);
    694:                 add_error(ctx, &ctx->errors, msg, WARNING);
    695:             }
    696:             
    697:             // For voltage source types (0 and 5)
    698:             if (i1 == 0 || i1 == 5) {
    699:                 ctx->netcx.ntsol = 0;
    700:                 
    701:                 if (i1 == 5) {
    702:                     // Incident plane wave or elementary current source
    703:                     ctx->vsorc.nvqd++;
    704:                     size_t mreq = (size_t)ctx->vsorc.nvqd * sizeof(int);
    705:                     mem_realloc(ctx, (void **)&ctx->vsorc.ivqd, mreq);
    706:                     mem_realloc(ctx, (void **)&ctx->vsorc.iqds, mreq);
    707:                     
    708:                     mreq = (size_t)ctx->vsorc.nvqd * sizeof(complex double);
    709:                     mem_realloc(ctx, (void **)&ctx->vsorc.vqd, mreq);
    710:                     mem_realloc(ctx, (void **)&ctx->vsorc.vqds, mreq);
    711:                     
    712:                     int idx = ctx->vsorc.nvqd - 1;
    713:                     int i3_resolved = resolve_pct_segment(ctx, card, 3, i2);
    714:                     int seg_num = segment_number(ctx, i2, i3_resolved);
    715:                     if (seg_num == 0) {
    716:                         char msg[MAX_ERROR_LEN];
    717:                         snprintf(msg, sizeof(msg), "Card %d is an EX that references invalid tag %d, segment %d", card_idx + 1, i2, i3_resolved);
    718:                         add_error(ctx, &ctx->errors, msg, FATAL);
    719:                         return -1;
    720:                     }
    721:                     ctx->vsorc.ivqd[idx] = seg_num;
    722:                     ctx->vsorc.vqd[idx] = f1 + I * f2;
    723:                     if (cabs(ctx->vsorc.vqd[idx]) < 1.e-20) {
    724:                         ctx->vsorc.vqd[idx] = CPLX_10;
    725:                     }
    726:                 } else {
    727:                     // Applied voltage source
    728:                     ctx->vsorc.nsant++;
    729:                     size_t mreq = (size_t)ctx->vsorc.nsant * sizeof(int);
    730:                     mem_realloc(ctx, (void **)&ctx->vsorc.isant, mreq);
    731:                     
    732:                     mreq = (size_t)ctx->vsorc.nsant * sizeof(complex double);
    733:                     mem_realloc(ctx, (void **)&ctx->vsorc.vsant, mreq);
    734:                     
    735:                     int idx = ctx->vsorc.nsant - 1;
    736:                     int i3_resolved = resolve_pct_segment(ctx, card, 3, i2);
    737:                     int seg_num = segment_number(ctx, i2, i3_resolved);
    738:                     if (seg_num == 0) {
    739:                         char msg[MAX_ERROR_LEN];
    740:                         snprintf(msg, sizeof(msg), "Card %d is an EX that references invalid tag %d, segment %d", card_idx + 1, i2, i3_resolved);
>   741:                         add_error(ctx, &ctx->errors, msg, FATAL);
    742:                         return -1;
    743:                     }
    744:                     ctx->vsorc.isant[idx] = seg_num;
    745:                     ctx->vsorc.vsant[idx] = f1 + I * f2;
    746:                     if (cabs(ctx->vsorc.vsant[idx]) < 1.e-20) {

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L612:                 snprintf(msg, sizeof(msg), "Card %d is an LD card with type %d, which is not supported.", card_idx + 1, i1);
L644:                 snprintf(msg, sizeof(msg),
L693:                 snprintf(msg, sizeof(msg), "Card %d is an EX card with type %d, which is not supported.", card_idx + 1, i1);
L717:                         snprintf(msg, sizeof(msg), "Card %d is an EX that references invalid tag %d, segment %d", card_idx + 1, i2, i3_resolved);
L740:                         snprintf(msg, sizeof(msg), "Card %d is an EX that references invalid tag %d, segment %d", card_idx + 1, i2, i3_resolved);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/control.c
LINE: 795
CALL: add_error(ctx, &ctx->errors, msg, FATAL);
--- Context (lines 735-800) ---
    735:                     int idx = ctx->vsorc.nsant - 1;
    736:                     int i3_resolved = resolve_pct_segment(ctx, card, 3, i2);
    737:                     int seg_num = segment_number(ctx, i2, i3_resolved);
    738:                     if (seg_num == 0) {
    739:                         char msg[MAX_ERROR_LEN];
    740:                         snprintf(msg, sizeof(msg), "Card %d is an EX that references invalid tag %d, segment %d", card_idx + 1, i2, i3_resolved);
    741:                         add_error(ctx, &ctx->errors, msg, FATAL);
    742:                         return -1;
    743:                     }
    744:                     ctx->vsorc.isant[idx] = seg_num;
    745:                     ctx->vsorc.vsant[idx] = f1 + I * f2;
    746:                     if (cabs(ctx->vsorc.vsant[idx]) < 1.e-20) {
    747:                         ctx->vsorc.vsant[idx] = CPLX_10;
    748:                     }
    749:                 }
    750:             } else {
    751:                 // Far field pattern for receiving antenna
    752:                 ctx->fpat.xpr6 = f6;
    753:                 ctx->vsorc.nsant = 0;
    754:                 ctx->vsorc.nvqd = 0;
    755:             }
    756:         }
    757:         else if (strcmp(code, "NT") == 0 || strcmp(code, "TL") == 0) {
    758:             // NT/TL cards - Network parameters
    759:             if (i2 == -1) {
    760:                 continue;
    761:             }
    762:             
    763:             // First NT/TL in batch resets network (iflow transition to 6)
    764:             if (ctx->iflow != 6 && ctx->netcx.nonet == 0) {
    765:                 reset_network_buffers(ctx);
    766:                 ctx->iflow = 6;
    767:             }
    768:             
    769:             // Reallocate network buffers
    770:             ctx->netcx.nonet++;
    771:             size_t mreq = (size_t)ctx->netcx.nonet * sizeof(int);
    772:             mem_realloc(ctx, (void **)&ctx->netcx.ntyp, mreq);
    773:             mem_realloc(ctx, (void **)&ctx->netcx.iseg1, mreq);
    774:             mem_realloc(ctx, (void **)&ctx->netcx.iseg2, mreq);
    775:             
    776:             mreq = (size_t)ctx->netcx.nonet * sizeof(double);
    777:             mem_realloc(ctx, (void **)&ctx->netcx.x11r, mreq);
    778:             mem_realloc(ctx, (void **)&ctx->netcx.x11i, mreq);
    779:             mem_realloc(ctx, (void **)&ctx->netcx.x12r, mreq);
    780:             mem_realloc(ctx, (void **)&ctx->netcx.x12i, mreq);
    781:             mem_realloc(ctx, (void **)&ctx->netcx.x22r, mreq);
    782:             mem_realloc(ctx, (void **)&ctx->netcx.x22i, mreq);
    783:             
    784:             int idx = ctx->netcx.nonet - 1;
    785:             if (strcmp(code, "NT") == 0) {
    786:                 ctx->netcx.ntyp[idx] = 1;
    787:             } else {
    788:                 ctx->netcx.ntyp[idx] = 2;
    789:             }
    790:             
    791:             ctx->netcx.iseg1[idx] = segment_number(ctx, i1, i2);
    792:             if (ctx->netcx.iseg1[idx] == 0) {
    793:                 char msg[MAX_ERROR_LEN];
    794:                 snprintf(msg, sizeof(msg), "Card %d is a %s that references invalid tag %d, segment %d", card_idx + 1, code, i1, i2);
>   795:                 add_error(ctx, &ctx->errors, msg, FATAL);
    796:                 return -1;
    797:             }
    798:             ctx->netcx.iseg2[idx] = segment_number(ctx, i3, i4);
    799:             if (ctx->netcx.iseg2[idx] == 0) {
    800:                 char msg[MAX_ERROR_LEN];

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L612:                 snprintf(msg, sizeof(msg), "Card %d is an LD card with type %d, which is not supported.", card_idx + 1, i1);
L644:                 snprintf(msg, sizeof(msg),
L693:                 snprintf(msg, sizeof(msg), "Card %d is an EX card with type %d, which is not supported.", card_idx + 1, i1);
L717:                         snprintf(msg, sizeof(msg), "Card %d is an EX that references invalid tag %d, segment %d", card_idx + 1, i2, i3_resolved);
L740:                         snprintf(msg, sizeof(msg), "Card %d is an EX that references invalid tag %d, segment %d", card_idx + 1, i2, i3_resolved);
L794:                 snprintf(msg, sizeof(msg), "Card %d is a %s that references invalid tag %d, segment %d", card_idx + 1, code, i1, i2);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/control.c
LINE: 802
CALL: add_error(ctx, &ctx->errors, msg, FATAL);
--- Context (lines 742-807) ---
    742:                         return -1;
    743:                     }
    744:                     ctx->vsorc.isant[idx] = seg_num;
    745:                     ctx->vsorc.vsant[idx] = f1 + I * f2;
    746:                     if (cabs(ctx->vsorc.vsant[idx]) < 1.e-20) {
    747:                         ctx->vsorc.vsant[idx] = CPLX_10;
    748:                     }
    749:                 }
    750:             } else {
    751:                 // Far field pattern for receiving antenna
    752:                 ctx->fpat.xpr6 = f6;
    753:                 ctx->vsorc.nsant = 0;
    754:                 ctx->vsorc.nvqd = 0;
    755:             }
    756:         }
    757:         else if (strcmp(code, "NT") == 0 || strcmp(code, "TL") == 0) {
    758:             // NT/TL cards - Network parameters
    759:             if (i2 == -1) {
    760:                 continue;
    761:             }
    762:             
    763:             // First NT/TL in batch resets network (iflow transition to 6)
    764:             if (ctx->iflow != 6 && ctx->netcx.nonet == 0) {
    765:                 reset_network_buffers(ctx);
    766:                 ctx->iflow = 6;
    767:             }
    768:             
    769:             // Reallocate network buffers
    770:             ctx->netcx.nonet++;
    771:             size_t mreq = (size_t)ctx->netcx.nonet * sizeof(int);
    772:             mem_realloc(ctx, (void **)&ctx->netcx.ntyp, mreq);
    773:             mem_realloc(ctx, (void **)&ctx->netcx.iseg1, mreq);
    774:             mem_realloc(ctx, (void **)&ctx->netcx.iseg2, mreq);
    775:             
    776:             mreq = (size_t)ctx->netcx.nonet * sizeof(double);
    777:             mem_realloc(ctx, (void **)&ctx->netcx.x11r, mreq);
    778:             mem_realloc(ctx, (void **)&ctx->netcx.x11i, mreq);
    779:             mem_realloc(ctx, (void **)&ctx->netcx.x12r, mreq);
    780:             mem_realloc(ctx, (void **)&ctx->netcx.x12i, mreq);
    781:             mem_realloc(ctx, (void **)&ctx->netcx.x22r, mreq);
    782:             mem_realloc(ctx, (void **)&ctx->netcx.x22i, mreq);
    783:             
    784:             int idx = ctx->netcx.nonet - 1;
    785:             if (strcmp(code, "NT") == 0) {
    786:                 ctx->netcx.ntyp[idx] = 1;
    787:             } else {
    788:                 ctx->netcx.ntyp[idx] = 2;
    789:             }
    790:             
    791:             ctx->netcx.iseg1[idx] = segment_number(ctx, i1, i2);
    792:             if (ctx->netcx.iseg1[idx] == 0) {
    793:                 char msg[MAX_ERROR_LEN];
    794:                 snprintf(msg, sizeof(msg), "Card %d is a %s that references invalid tag %d, segment %d", card_idx + 1, code, i1, i2);
    795:                 add_error(ctx, &ctx->errors, msg, FATAL);
    796:                 return -1;
    797:             }
    798:             ctx->netcx.iseg2[idx] = segment_number(ctx, i3, i4);
    799:             if (ctx->netcx.iseg2[idx] == 0) {
    800:                 char msg[MAX_ERROR_LEN];
    801:                 snprintf(msg, sizeof(msg), "Card %d is a %s that references invalid tag %d, segment %d", card_idx + 1, code, i3, i4);
>   802:                 add_error(ctx, &ctx->errors, msg, FATAL);
    803:                 return -1;
    804:             }
    805:             ctx->netcx.x11r[idx] = f1;
    806:             ctx->netcx.x11i[idx] = f2;
    807:             ctx->netcx.x12r[idx] = f3;

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L612:                 snprintf(msg, sizeof(msg), "Card %d is an LD card with type %d, which is not supported.", card_idx + 1, i1);
L644:                 snprintf(msg, sizeof(msg),
L693:                 snprintf(msg, sizeof(msg), "Card %d is an EX card with type %d, which is not supported.", card_idx + 1, i1);
L717:                         snprintf(msg, sizeof(msg), "Card %d is an EX that references invalid tag %d, segment %d", card_idx + 1, i2, i3_resolved);
L740:                         snprintf(msg, sizeof(msg), "Card %d is an EX that references invalid tag %d, segment %d", card_idx + 1, i2, i3_resolved);
L794:                 snprintf(msg, sizeof(msg), "Card %d is a %s that references invalid tag %d, segment %d", card_idx + 1, code, i1, i2);
L801:                 snprintf(msg, sizeof(msg), "Card %d is a %s that references invalid tag %d, segment %d", card_idx + 1, code, i3, i4);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/control.c
LINE: 932
CALL: add_error(ctx, &ctx->errors, msg, FATAL);
--- Context (lines 872-937) ---
    872:             ctx->fpat.inor = ctx->fpat.inor - ctx->fpat.iax * 10;
    873:             
    874:             if (ctx->fpat.iax != 0) ctx->fpat.iax = 1;
    875:             if (ctx->fpat.ipd != 0) ctx->fpat.ipd = 1;
    876:             if ((ctx->fpat.nth < 2) || (ctx->fpat.nph < 2) || (ctx->gnd.ifar == 1)) {
    877:                 ctx->fpat.iavp = 0;
    878:             }
    879:             
    880:             ctx->fpat.thets = f1;
    881:             ctx->fpat.phis = f2;
    882:             ctx->fpat.dth = f3;
    883:             ctx->fpat.dph = f4;
    884:             ctx->fpat.rfld = f5;
    885:             ctx->fpat.gnor = f6;
    886:         }
    887:         else if (strcmp(code, "NE") == 0 || strcmp(code, "NH") == 0) {
    888:             // NE/NH cards - Near field calculation
    889:             ctx->fpat.nfeh = (strcmp(code, "NH") == 0) ? 1 : 0;
    890:             ctx->fpat.near = i1;
    891:             ctx->fpat.nrx = i2;
    892:             ctx->fpat.nry = i3;
    893:             ctx->fpat.nrz = i4;
    894:             ctx->fpat.xnr = f1;
    895:             ctx->fpat.ynr = f2;
    896:             ctx->fpat.znr = f3;
    897:             ctx->fpat.dxnr = f4;
    898:             ctx->fpat.dynr = f5;
    899:             ctx->fpat.dznr = f6;
    900:         }
    901:         else if (strcmp(code, "EK") == 0) {
    902:             // Extended thin-wire kernel
    903:             ctx->dataj.iexk = i1;
    904:         }
    905:         else if (strcmp(code, "KH") == 0) {
    906:             // Matrix integration limit
    907:             ctx->dataj.rkh = f1;
    908:         }
    909:         else if (strcmp(code, "WG") == 0) {
    910:             /* WG FILENAME: write Numerical Green's Function file after cmset.
    911:              * Open the output file now; write_greens_binary() is called in
    912:              * execute_frequency_loop() after the matrix is filled, then the
    913:              * frequency loop exits without factorizing or solving.
    914:              * If no filename is given on the card, derive one from the input
    915:              * deck path by replacing the extension with .ngf (same directory). */
    916:             const char *wg_filename = card->comment;
    917:             char wg_default[MAX_PATH_LEN + 1];
    918:             char wg_resolved[MAX_PATH_LEN + 1];
    919:             if (!wg_filename || *wg_filename == '\0') {
    920:                 if (ctx->source_filename) {
    921:                     strncpy(wg_default, ctx->source_filename, MAX_PATH_LEN);
    922:                     wg_default[MAX_PATH_LEN] = '\0';
    923:                     char *dot   = strrchr(wg_default, '.');
    924:                     char *slash = strrchr(wg_default, '/');
    925:                     if (dot && (!slash || dot > slash))
    926:                         *dot = '\0';
    927:                     strncat(wg_default, ".ngf", MAX_PATH_LEN - strlen(wg_default));
    928:                     wg_filename = wg_default;
    929:                 } else {
    930:                     char msg[MAX_ERROR_LEN];
    931:                     snprintf(msg, sizeof(msg), "WG card %d has no filename and no input file to derive one from.", card_idx + 1);
>   932:                     add_error(ctx, &ctx->errors, msg, FATAL);
    933:                     return -1;
    934:                 }
    935:             } else {
    936:                 /* Explicit filename: resolve relative to input file's directory */
    937:                 resolve_path_relative_to_input(wg_filename, ctx->source_filename,

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L740:                         snprintf(msg, sizeof(msg), "Card %d is an EX that references invalid tag %d, segment %d", card_idx + 1, i2, i3_resolved);
L794:                 snprintf(msg, sizeof(msg), "Card %d is a %s that references invalid tag %d, segment %d", card_idx + 1, code, i1, i2);
L801:                 snprintf(msg, sizeof(msg), "Card %d is a %s that references invalid tag %d, segment %d", card_idx + 1, code, i3, i4);
L931:                     snprintf(msg, sizeof(msg), "WG card %d has no filename and no input file to derive one from.", card_idx + 1);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/control.c
LINE: 950
CALL: add_error(ctx, &ctx->errors, msg, FATAL);
--- Context (lines 890-955) ---
    890:             ctx->fpat.near = i1;
    891:             ctx->fpat.nrx = i2;
    892:             ctx->fpat.nry = i3;
    893:             ctx->fpat.nrz = i4;
    894:             ctx->fpat.xnr = f1;
    895:             ctx->fpat.ynr = f2;
    896:             ctx->fpat.znr = f3;
    897:             ctx->fpat.dxnr = f4;
    898:             ctx->fpat.dynr = f5;
    899:             ctx->fpat.dznr = f6;
    900:         }
    901:         else if (strcmp(code, "EK") == 0) {
    902:             // Extended thin-wire kernel
    903:             ctx->dataj.iexk = i1;
    904:         }
    905:         else if (strcmp(code, "KH") == 0) {
    906:             // Matrix integration limit
    907:             ctx->dataj.rkh = f1;
    908:         }
    909:         else if (strcmp(code, "WG") == 0) {
    910:             /* WG FILENAME: write Numerical Green's Function file after cmset.
    911:              * Open the output file now; write_greens_binary() is called in
    912:              * execute_frequency_loop() after the matrix is filled, then the
    913:              * frequency loop exits without factorizing or solving.
    914:              * If no filename is given on the card, derive one from the input
    915:              * deck path by replacing the extension with .ngf (same directory). */
    916:             const char *wg_filename = card->comment;
    917:             char wg_default[MAX_PATH_LEN + 1];
    918:             char wg_resolved[MAX_PATH_LEN + 1];
    919:             if (!wg_filename || *wg_filename == '\0') {
    920:                 if (ctx->source_filename) {
    921:                     strncpy(wg_default, ctx->source_filename, MAX_PATH_LEN);
    922:                     wg_default[MAX_PATH_LEN] = '\0';
    923:                     char *dot   = strrchr(wg_default, '.');
    924:                     char *slash = strrchr(wg_default, '/');
    925:                     if (dot && (!slash || dot > slash))
    926:                         *dot = '\0';
    927:                     strncat(wg_default, ".ngf", MAX_PATH_LEN - strlen(wg_default));
    928:                     wg_filename = wg_default;
    929:                 } else {
    930:                     char msg[MAX_ERROR_LEN];
    931:                     snprintf(msg, sizeof(msg), "WG card %d has no filename and no input file to derive one from.", card_idx + 1);
    932:                     add_error(ctx, &ctx->errors, msg, FATAL);
    933:                     return -1;
    934:                 }
    935:             } else {
    936:                 /* Explicit filename: resolve relative to input file's directory */
    937:                 resolve_path_relative_to_input(wg_filename, ctx->source_filename,
    938:                                                wg_resolved, sizeof(wg_resolved));
    939:                 wg_filename = wg_resolved;
    940:             }
    941:             if (ctx->green_fp != NULL) {
    942:                 fclose(ctx->green_fp);
    943:                 ctx->green_fp = NULL;
    944:             }
    945:             ctx->green_fp = fopen(wg_filename, "wb");
    946:             if (!ctx->green_fp) {
    947:                 char msg[MAX_ERROR_LEN];
    948:                 snprintf(msg, sizeof(msg),
    949:                          "WG card %d: cannot open '%s' for writing.", card_idx + 1, wg_filename);
>   950:                 add_error(ctx, &ctx->errors, msg, FATAL);
    951:                 return -1;
    952:             }
    953:             ctx->wg_after_cmset = true;
    954:         }
    955:     }

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L794:                 snprintf(msg, sizeof(msg), "Card %d is a %s that references invalid tag %d, segment %d", card_idx + 1, code, i1, i2);
L801:                 snprintf(msg, sizeof(msg), "Card %d is a %s that references invalid tag %d, segment %d", card_idx + 1, code, i3, i4);
L931:                     snprintf(msg, sizeof(msg), "WG card %d has no filename and no input file to derive one from.", card_idx + 1);
L948:                 snprintf(msg, sizeof(msg),

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck.c
LINE: 933
CALL: //   add_error(NULL, errors, msg, WARNING);
--- Context (lines 873-938) ---
    873:   // Step 3: Evaluate symbols in comment section sequentially
    874:   // evaluate_symbols_in_comments(deck, errors);
    875: }
    876: 
    877: /******************************************************************************
    878:  * update_deck_values
    879:  *
    880:  * update_deck_values loops through the entire deck and calls
    881:  * update_card_values on any card that has a formula or unit. Normally called
    882:  * after making a change to any of the SY cards, or just before any deck-wide
    883:  * actions like saving it out or running a calculation.
    884:  */
    885: void update_deck_values(nec_context_t *ctx, deck_t *deck)
    886: {
    887:   // Reinitialize with defaults first
    888:   if (deck->symbols)
    889:   {
    890:     free(deck->symbols);
    891:     deck->symbols = NULL;
    892:   }
    893:   deck->num_symbols = 0;
    894:   add_default_symbols(deck);
    895: 
    896:   // then add symbols from the
    897:   update_symbol_list(deck, &ctx->errors);
    898: 
    899:   // Evaluate and update
    900:   update_symbol_values(ctx, deck, &ctx->errors);
    901:   update_card_values(deck);
    902: }
    903: 
    904: /******************************************************************************
    905:  * update_symbol_list
    906:  *
    907:  * update_symbol_list looks for any SY cards in the deck and adds their
    908:  * key/value pairs to the deck's symbol list. Assumes default symbols have
    909:  * already been added, and warns if a deck symbol tries to override a default.
    910:  */
    911: static void update_symbol_list(deck_t *deck, errors_list_t *errors)
    912: {
    913:   // Count how many default symbols exist before adding user symbols
    914:   int num_defaults = deck->num_symbols;
    915: 
    916:   // INVARIANT: only add pointers to key_value_t nodes owned by cards (e.g., from card->formulas)
    917:   for (int i = 0; i < deck->num_cards; i++)
    918:   {
    919:     card_t *card = &deck->cards[i];
    920:     if (strcmp(card->card_code, "SY") == 0 && card->formulas)
    921:     {
    922:       key_value_t *kv = card->formulas;
    923:       while (kv)
    924:       {
    925:         // Check if this symbol name conflicts with any existing symbol (case-insensitive)
    926:         for (int j = 0; j < num_defaults; j++)
    927:         {
    928:           if (deck->symbols[j] && strcasecmp(deck->symbols[j]->key, kv->key) == 0)
    929:           {
    930:             // if (errors) {
    931:             //   char msg[256];
    932:             //   snprintf(msg, sizeof(msg), "The symbol '%s' conflicts with existing symbol '%s'. The user symbol will take precedence.", kv->key, deck->symbols[j]->key);
>   933:             //   add_error(NULL, errors, msg, WARNING);
    934:             // }
    935:             // remove the conflicting default symbol
    936:             remove_symbol(deck, deck->symbols[j]->key);
    937:             num_defaults--; // Adjust count since we removed one
    938:             break;

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L932:             //   snprintf(msg, sizeof(msg), "The symbol '%s' conflicts with existing symbol '%s'. The user symbol will take precedence.", kv->key, deck->symbols[j]->key);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck.c
LINE: 1183
CALL: add_error(ctx, errors, err_msg, FATAL);
--- Context (lines 1123-1188) ---
   1123:       return true;
   1124:     }
   1125:     p += len;
   1126:   }
   1127:   return false;
   1128: }
   1129: 
   1130: // Helper function for better formula error messages
   1131: static char *get_formula_error_description(const char *formula, int error_pos)
   1132: {
   1133:   if (!formula || error_pos < 0 || error_pos >= (int)strlen(formula))
   1134:   {
   1135:     return NULL;
   1136:   }
   1137: 
   1138:   // look for function-like patterns around the error position
   1139:   const char *pos = formula + error_pos;
   1140: 
   1141:   // look backwards from the error position to find the start of a potential function name
   1142:   const char *start = pos - 1; // Start from the character before the error
   1143:   while (start >= formula && (isalnum((unsigned char)*start) || *start == '_'))
   1144:   {
   1145:     start--;
   1146:   }
   1147:   start++; // Move past the non-alphanumeric character we stopped at
   1148: 
   1149:   // check if this looks like a function call (we have a function name followed by '(')
   1150:   if (pos > start && *pos == '(')
   1151:   {
   1152:     // extract the function name
   1153:     size_t name_len = pos - start;
   1154:     if (name_len > 0 && name_len < 50)
   1155:     {
   1156:       char func_name[64];
   1157:       strncpy(func_name, start, name_len);
   1158:       func_name[name_len] = '\0';
   1159: 
   1160:       // check if it looks like a valid identifier
   1161:       if (isalpha((unsigned char)func_name[0]) || func_name[0] == '_')
   1162:       {
   1163:         char error_msg[128];
   1164:         snprintf(error_msg, sizeof(error_msg), "unknown function '%s'", func_name);
   1165:         return strdup(error_msg);
   1166:       }
   1167:     }
   1168:   }
   1169: 
   1170:   return NULL; // no specific error description available
   1171: }
   1172: 
   1173: // Recursive evaluation for symbol dependencies
   1174: static int eval_symbol(int i, int sym_count, key_value_t **syms, bool *evaluated, deck_t *deck, nec_context_t *ctx, errors_list_t *errors)
   1175: {
   1176:   // Check recursion depth to prevent infinite loops using ctx->eval_depth
   1177:   if (ctx)
   1178:     ctx->eval_depth++;
   1179:   if (ctx && ctx->eval_depth > 100)
   1180:   {
   1181:     char err_msg[256];
   1182:     snprintf(err_msg, sizeof(err_msg), "Maximum recursion depth exceeded evaluating symbol '%s'", syms[i]->key);
>  1183:     add_error(ctx, errors, err_msg, FATAL);
   1184:     ctx->eval_depth--;
   1185:     return -1;
   1186:   }
   1187: 
   1188:   // Check if already evaluated to prevent infinite recursion

-- Message variable name candidates --
VAR_EXPR: err_msg  VAR_NAME: err_msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L1182:     snprintf(err_msg, sizeof(err_msg), "Maximum recursion depth exceeded evaluating symbol '%s'", syms[i]->key);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck.c
LINE: 1293
CALL: add_error(ctx, errors, err_msg, FATAL);
--- Context (lines 1233-1298) ---
   1233:     int err = 0;
   1234:     te_expr *expr = te_compile(processed_formula, vars, sym_count, &err);
   1235:     if (expr)
   1236:     {
   1237:       sym->fv = te_eval(expr);
   1238:       te_free(expr);
   1239: 
   1240:       // DEBUG: Show symbol evaluation
   1241:       // fprintf(stderr, "DEBUG: Evaluated symbol '%s' = '%s' -> %g\n", sym->key, processed_formula, sym->fv);
   1242:     }
   1243:     else
   1244:     {
   1245:       // Find which card this symbol belongs to
   1246:       int card_num = -1;
   1247:       for (int c = 0; c < deck->num_cards; c++)
   1248:       {
   1249:         card_t *card = &deck->cards[c];
   1250:         if (strcmp(card->card_code, "SY") == 0 && card->formulas)
   1251:         {
   1252:           key_value_t *kv = card->formulas;
   1253:           while (kv)
   1254:           {
   1255:             if (kv == sym)
   1256:             {
   1257:               card_num = c + 1;
   1258:               break;
   1259:             }
   1260:             kv = kv->next;
   1261:           }
   1262:           if (card_num > 0)
   1263:             break;
   1264:         }
   1265:       }
   1266:       char err_msg[256];
   1267:       // Try to provide a more descriptive error message
   1268:       char *error_desc = get_formula_error_description(processed_formula, err);
   1269:       if (card_num > 0)
   1270:       {
   1271:         if (error_desc)
   1272:         {
   1273:           snprintf(err_msg, sizeof(err_msg), "Error evaluating formula '%s' on card %d: %s", processed_formula, card_num, error_desc);
   1274:           free(error_desc);
   1275:         }
   1276:         else
   1277:         {
   1278:           snprintf(err_msg, sizeof(err_msg), "Error evaluating formula '%s' at position %d on card %d", processed_formula, err, card_num);
   1279:         }
   1280:       }
   1281:       else
   1282:       {
   1283:         if (error_desc)
   1284:         {
   1285:           snprintf(err_msg, sizeof(err_msg), "Error evaluating formula '%s': %s", processed_formula, error_desc);
   1286:           free(error_desc);
   1287:         }
   1288:         else
   1289:         {
   1290:           snprintf(err_msg, sizeof(err_msg), "Error evaluating formula '%s' at position %d", processed_formula, err);
   1291:         }
   1292:       }
>  1293:       add_error(ctx, errors, err_msg, FATAL);
   1294:       free(processed_formula);
   1295:       free(vars);
   1296:       if (ctx)
   1297:         ctx->eval_depth--;
   1298:       return -1;

-- Message variable name candidates --
VAR_EXPR: err_msg  VAR_NAME: err_msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L1182:     snprintf(err_msg, sizeof(err_msg), "Maximum recursion depth exceeded evaluating symbol '%s'", syms[i]->key);
L1273:           snprintf(err_msg, sizeof(err_msg), "Error evaluating formula '%s' on card %d: %s", processed_formula, card_num, error_desc);
L1278:           snprintf(err_msg, sizeof(err_msg), "Error evaluating formula '%s' at position %d on card %d", processed_formula, err, card_num);
L1285:           snprintf(err_msg, sizeof(err_msg), "Error evaluating formula '%s': %s", processed_formula, error_desc);
L1290:           snprintf(err_msg, sizeof(err_msg), "Error evaluating formula '%s' at position %d", processed_formula, err);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck.c
LINE: 1434
CALL: * @param ctx      The context (for add_error)
--- Context (lines 1374-1439) ---
   1374:         const char *key = kv->key;
   1375:         const char *expr_str = kv->value;
   1376:         if (key != NULL && expr_str != NULL && key[0] != '\0')
   1377:         {
   1378:           char kind = key[0];
   1379:           int idx = atoi(key + 1);
   1380:           int err = 0;
   1381: 
   1382:           // Preprocess AWG syntax in the expression
   1383:           char *temp_expr = preprocess_awg(expr_str);
   1384: 
   1385:           // Preprocess implicit multiplication
   1386:           char *processed_expr = preprocess_implicit_multiplication(temp_expr);
   1387:           free(temp_expr);
   1388: 
   1389:           // Normalize to lowercase for case-insensitive symbol matching
   1390:           for (char *p = processed_expr; *p; p++)
   1391:           {
   1392:             *p = tolower((unsigned char)*p);
   1393:           }
   1394: 
   1395:           te_expr *expr = te_compile(processed_expr, vars, v, &err);
   1396: 
   1397:           if (expr != NULL)
   1398:           {
   1399:             double val = te_eval(expr);
   1400:             te_free(expr);
   1401: 
   1402:             if (kind == 'F' && idx >= 1 && idx <= MAX_FLT_FIELDS)
   1403:             {
   1404:               card->f[idx] = val;
   1405:               fvals[idx] = val; // keep variables in sync for subsequent formulas
   1406:             }
   1407:             else if (kind == 'I' && idx >= 1 && idx <= MAX_INT_FIELDS)
   1408:             {
   1409:               int ival = (int)val; // truncate; can switch to rounding if desired
   1410:               card->i[idx] = ival;
   1411:               ivals[idx] = (double)ival;
   1412:             }
   1413:           }
   1414:           free(processed_expr);
   1415:         }
   1416:         kv = kv->next;
   1417:       }
   1418:       free(vars);
   1419:     }
   1420: 
   1421:     // Unit conversions are now handled through formula evaluation
   1422:     // with unit constants in the symbol table (mm=0.001, ft=0.3048, etc.)
   1423:     // No post-processing needed here.
   1424:   }
   1425: } /* update_card_values() */
   1426: 
   1427: /******************************************************************************
   1428:  * evaluate_formula
   1429:  *
   1430:  * Evaluates a single formula (key_value_t) using currently-defined symbols
   1431:  * in deck->symbols[]. Updates the key_value_t->fv field with the result.
   1432:  * Reports errors if symbols are undefined or if there are syntax errors.
   1433:  *
>  1434:  * @param ctx      The context (for add_error)
   1435:  * @param formula  The formula to evaluate (its value string will be compiled)
   1436:  * @param deck     The deck containing the symbol table
   1437:  * @param errors   Error list for reporting undefined symbols or syntax errors
   1438:  */
   1439: void evaluate_formula(nec_context_t *ctx, key_value_t *formula, deck_t *deck, errors_list_t *errors)

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L1273:           snprintf(err_msg, sizeof(err_msg), "Error evaluating formula '%s' on card %d: %s", processed_formula, card_num, error_desc);
L1278:           snprintf(err_msg, sizeof(err_msg), "Error evaluating formula '%s' at position %d on card %d", processed_formula, err, card_num);
L1285:           snprintf(err_msg, sizeof(err_msg), "Error evaluating formula '%s': %s", processed_formula, error_desc);
L1290:           snprintf(err_msg, sizeof(err_msg), "Error evaluating formula '%s' at position %d", processed_formula, err);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/deck.c
LINE: 1497
CALL: add_error(ctx, errors, msg, WARNING);
--- Context (lines 1437-1502) ---
   1437:  * @param errors   Error list for reporting undefined symbols or syntax errors
   1438:  */
   1439: void evaluate_formula(nec_context_t *ctx, key_value_t *formula, deck_t *deck, errors_list_t *errors)
   1440: {
   1441:   // Prepare variables for tinyexpr - bind all current symbols (original case)
   1442:   int num_syms = deck ? deck->num_symbols : 0;
   1443:   te_variable *vars = calloc(num_syms, sizeof(te_variable));
   1444: 
   1445:   for (int k = 0; k < num_syms; k++)
   1446:   {
   1447:     vars[k].name = deck->symbols[k]->key;
   1448:     vars[k].address = &deck->symbols[k]->fv;
   1449:     vars[k].type = TE_VARIABLE;
   1450:     vars[k].context = NULL;
   1451:   }
   1452: 
   1453:   // Preprocess AWG syntax (#14 -> awg value)
   1454:   char *temp_formula = preprocess_awg(formula->value);
   1455: 
   1456:   // Preprocess implicit multiplication (135 ft -> 135*ft)
   1457:   char *processed_formula = preprocess_implicit_multiplication(temp_formula);
   1458:   free(temp_formula);
   1459: 
   1460:   // Normalize to lowercase for case-insensitive symbol matching
   1461:   for (char *p = processed_formula; *p; p++)
   1462:   {
   1463:     *p = tolower((unsigned char)*p);
   1464:   }
   1465: 
   1466:   // (debug removed)
   1467: 
   1468:   // Compile and evaluate
   1469:   int err = 0;
   1470:   te_expr *expr = te_compile(processed_formula, vars, num_syms, &err);
   1471: 
   1472:   if (expr)
   1473:   {
   1474:     formula->fv = te_eval(expr);
   1475: 
   1476:     te_free(expr);
   1477:   }
   1478:   else
   1479:   {
   1480:     // Report error if compilation failed
   1481:     if (errors)
   1482:     {
   1483:       char msg[MAX_ERROR_LEN];
   1484:       // Try to provide a more descriptive error message
   1485:       char *error_desc = get_formula_error_description(processed_formula, err);
   1486:       if (error_desc)
   1487:       {
   1488:         snprintf(msg, sizeof(msg), "Error evaluating formula '%s = %s': %s",
   1489:                  formula->key, formula->value, error_desc);
   1490:         free(error_desc);
   1491:       }
   1492:       else
   1493:       {
   1494:         snprintf(msg, sizeof(msg), "Error evaluating formula '%s = %s' at position %d",
   1495:                  formula->key, formula->value, err);
   1496:       }
>  1497:       add_error(ctx, errors, msg, WARNING);
   1498:     }
   1499:   }
   1500: 
   1501:   free(processed_formula);
   1502:   free(vars);

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L1488:         snprintf(msg, sizeof(msg), "Error evaluating formula '%s = %s': %s",
L1494:         snprintf(msg, sizeof(msg), "Error evaluating formula '%s = %s' at position %d",

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/input.c
LINE: 92
CALL: add_error(ctx, &ctx->errors, msg, FATAL);
--- Context (lines 32-97) ---
     32:  *
     33:  *****************************************************************************/
     34: 
     35: #include "internals.h"
     36: #include "input.h"
     37: 
     38: /* Forward declarations for internal functions */
     39: static int read_line(nec_context_t *ctx, char *buff, FILE *pfile, int line_num);
     40: static void parse_comment_card(nec_context_t *ctx, card_t *card, errors_list_t *errors);
     41: static void parse_geometry_or_control_card(nec_context_t *ctx, card_t *card, errors_list_t *errors);
     42: static void parse_onec_card(nec_context_t *ctx, card_t *card, errors_list_t *errors);
     43: static void parse_key_values(nec_context_t *ctx, card_t *card, errors_list_t *errors);
     44: 
     45: /******************************************************************************
     46:  * read_deck()
     47:  *
     48:  * Reads the entire deck line by line and fills out the deck's cards[]
     49:  * array with the resulting data. After this completes, the caller
     50:  * should call parse_deck() to process the data.
     51:  *
     52:  * @param deck deck_t structure that will hold the Cards
     53:  * @param pfile file pointer to the file to be read, assumed
     54:  *  to have been opened previous to this call
     55:  *
     56:  */
     57: void read_deck(nec_context_t *ctx, deck_t *deck, FILE *pfile)
     58: {
     59:   char line_buf[1024];  // make it large enough to hold any line
     60:   size_t line_len;      // actual length of the current card being read
     61:     
     62:   // set the card count to 0, it might have !=0 default
     63:   deck->num_cards = 0;
     64: 
     65:   // and the symbols...
     66:   deck->num_symbols = 0;
     67:   
     68:   // and set the default comment markers to empty
     69:   deck->extn_code = 0;
     70:   deck->cmt_code = 0;
     71: 
     72:   // loop and read lines one-by-one until we hit the EOF
     73:   int line_num = 0;
     74:   int last_line_nonempty = 0;
     75:   do {
     76:     line_num++;
     77:     int read_result = read_line(ctx, line_buf, pfile, line_num);
     78:     if(read_result == EOF) {
     79:       if (strlen(line_buf) > 0) {
     80:         last_line_nonempty = 1;
     81:       } else {
     82:         break;
     83:       }
     84:     }
     85:     line_len = strlen(line_buf);
     86:     if(deck->num_cards == 0) {
     87:       deck->num_cards++;
     88:       deck->cards = calloc(1, sizeof(card_t));
     89:       if (!deck->cards) {
     90:         char msg[MAX_ERROR_LEN];
     91:         snprintf(msg, MAX_ERROR_LEN, "[read_deck] ERROR: calloc failed for deck->cards at line %d", line_num);
>    92:         add_error(ctx, &ctx->errors, msg, FATAL);
     93:         return;
     94:       }
     95:     } else {
     96:       deck->num_cards++;
     97:       card_t *new_cards = realloc(deck->cards, deck->num_cards * sizeof(card_t));

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L91:         snprintf(msg, MAX_ERROR_LEN, "[read_deck] ERROR: calloc failed for deck->cards at line %d", line_num);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/input.c
LINE: 101
CALL: add_error(ctx, &ctx->errors, msg, FATAL);
--- Context (lines 41-106) ---
     41: static void parse_geometry_or_control_card(nec_context_t *ctx, card_t *card, errors_list_t *errors);
     42: static void parse_onec_card(nec_context_t *ctx, card_t *card, errors_list_t *errors);
     43: static void parse_key_values(nec_context_t *ctx, card_t *card, errors_list_t *errors);
     44: 
     45: /******************************************************************************
     46:  * read_deck()
     47:  *
     48:  * Reads the entire deck line by line and fills out the deck's cards[]
     49:  * array with the resulting data. After this completes, the caller
     50:  * should call parse_deck() to process the data.
     51:  *
     52:  * @param deck deck_t structure that will hold the Cards
     53:  * @param pfile file pointer to the file to be read, assumed
     54:  *  to have been opened previous to this call
     55:  *
     56:  */
     57: void read_deck(nec_context_t *ctx, deck_t *deck, FILE *pfile)
     58: {
     59:   char line_buf[1024];  // make it large enough to hold any line
     60:   size_t line_len;      // actual length of the current card being read
     61:     
     62:   // set the card count to 0, it might have !=0 default
     63:   deck->num_cards = 0;
     64: 
     65:   // and the symbols...
     66:   deck->num_symbols = 0;
     67:   
     68:   // and set the default comment markers to empty
     69:   deck->extn_code = 0;
     70:   deck->cmt_code = 0;
     71: 
     72:   // loop and read lines one-by-one until we hit the EOF
     73:   int line_num = 0;
     74:   int last_line_nonempty = 0;
     75:   do {
     76:     line_num++;
     77:     int read_result = read_line(ctx, line_buf, pfile, line_num);
     78:     if(read_result == EOF) {
     79:       if (strlen(line_buf) > 0) {
     80:         last_line_nonempty = 1;
     81:       } else {
     82:         break;
     83:       }
     84:     }
     85:     line_len = strlen(line_buf);
     86:     if(deck->num_cards == 0) {
     87:       deck->num_cards++;
     88:       deck->cards = calloc(1, sizeof(card_t));
     89:       if (!deck->cards) {
     90:         char msg[MAX_ERROR_LEN];
     91:         snprintf(msg, MAX_ERROR_LEN, "[read_deck] ERROR: calloc failed for deck->cards at line %d", line_num);
     92:         add_error(ctx, &ctx->errors, msg, FATAL);
     93:         return;
     94:       }
     95:     } else {
     96:       deck->num_cards++;
     97:       card_t *new_cards = realloc(deck->cards, deck->num_cards * sizeof(card_t));
     98:       if (!new_cards) {
     99:         char msg[MAX_ERROR_LEN];
    100:         snprintf(msg, MAX_ERROR_LEN, "[read_deck] ERROR: realloc failed for deck->cards at line %d", line_num);
>   101:         add_error(ctx, &ctx->errors, msg, FATAL);
    102:         return;
    103:       }
    104:       deck->cards = new_cards;
    105:     }
    106:     card_t *dest = &deck->cards[deck->num_cards - 1];

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L91:         snprintf(msg, MAX_ERROR_LEN, "[read_deck] ERROR: calloc failed for deck->cards at line %d", line_num);
L100:         snprintf(msg, MAX_ERROR_LEN, "[read_deck] ERROR: realloc failed for deck->cards at line %d", line_num);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/input.c
LINE: 116
CALL: add_error(ctx, &ctx->errors, msg, FATAL);
--- Context (lines 56-121) ---
     56:  */
     57: void read_deck(nec_context_t *ctx, deck_t *deck, FILE *pfile)
     58: {
     59:   char line_buf[1024];  // make it large enough to hold any line
     60:   size_t line_len;      // actual length of the current card being read
     61:     
     62:   // set the card count to 0, it might have !=0 default
     63:   deck->num_cards = 0;
     64: 
     65:   // and the symbols...
     66:   deck->num_symbols = 0;
     67:   
     68:   // and set the default comment markers to empty
     69:   deck->extn_code = 0;
     70:   deck->cmt_code = 0;
     71: 
     72:   // loop and read lines one-by-one until we hit the EOF
     73:   int line_num = 0;
     74:   int last_line_nonempty = 0;
     75:   do {
     76:     line_num++;
     77:     int read_result = read_line(ctx, line_buf, pfile, line_num);
     78:     if(read_result == EOF) {
     79:       if (strlen(line_buf) > 0) {
     80:         last_line_nonempty = 1;
     81:       } else {
     82:         break;
     83:       }
     84:     }
     85:     line_len = strlen(line_buf);
     86:     if(deck->num_cards == 0) {
     87:       deck->num_cards++;
     88:       deck->cards = calloc(1, sizeof(card_t));
     89:       if (!deck->cards) {
     90:         char msg[MAX_ERROR_LEN];
     91:         snprintf(msg, MAX_ERROR_LEN, "[read_deck] ERROR: calloc failed for deck->cards at line %d", line_num);
     92:         add_error(ctx, &ctx->errors, msg, FATAL);
     93:         return;
     94:       }
     95:     } else {
     96:       deck->num_cards++;
     97:       card_t *new_cards = realloc(deck->cards, deck->num_cards * sizeof(card_t));
     98:       if (!new_cards) {
     99:         char msg[MAX_ERROR_LEN];
    100:         snprintf(msg, MAX_ERROR_LEN, "[read_deck] ERROR: realloc failed for deck->cards at line %d", line_num);
    101:         add_error(ctx, &ctx->errors, msg, FATAL);
    102:         return;
    103:       }
    104:       deck->cards = new_cards;
    105:     }
    106:     card_t *dest = &deck->cards[deck->num_cards - 1];
    107:     *dest = (card_t){
    108:       .edited = false,
    109:       .ignore = false
    110:     };
    111: 
    112:     dest->orig_str = calloc(line_len + 1, sizeof(char));
    113:     if (!dest->orig_str) {
    114:       char msg[MAX_ERROR_LEN];
    115:       snprintf(msg, MAX_ERROR_LEN, "[read_deck] ERROR: calloc failed for card->orig_str at line %d", line_num);
>   116:       add_error(ctx, &ctx->errors, msg, FATAL);
    117:       return;
    118:     }
    119:     strncpy(dest->orig_str, line_buf, line_len);
    120:     dest->orig_str[line_len] = '\0';
    121:     if (read_result == EOF && !last_line_nonempty) {

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L91:         snprintf(msg, MAX_ERROR_LEN, "[read_deck] ERROR: calloc failed for deck->cards at line %d", line_num);
L100:         snprintf(msg, MAX_ERROR_LEN, "[read_deck] ERROR: realloc failed for deck->cards at line %d", line_num);
L115:       snprintf(msg, MAX_ERROR_LEN, "[read_deck] ERROR: calloc failed for card->orig_str at line %d", line_num);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/input.c
LINE: 236
CALL: add_error(ctx, &ctx->errors, msg, WARNING);
--- Context (lines 176-241) ---
    176:   
    177:   // if we're at the end of the file, return that, we're done
    178:   if((chr = getc(pfile)) == EOF) {
    179:     return(EOF);
    180:   }
    181: 
    182:   // the line parser below stops and returns as soon as it sees a single
    183:   // cr or lf. That means that when we re-enter the routine, the file might
    184:   // have leading cr's or lf's left over. this code eats them. note that this
    185:   // also eats totally empty lines, and it's not clear that's what we want,
    186:   // we might want to save those in order to report a warning. if that's the
    187:   // case, it would seem we should do this eating at the end of the routine?
    188:   while((chr == CR) || (chr == LF)) {
    189:     // eat the next char, and return if that's the eof
    190:     if((chr = getc(pfile)) == EOF) {
    191:       return(EOF);
    192:     }
    193: 
    194:     // eat any remaining line-ends
    195:     while((chr == CR) || (chr == LF)) {
    196:       if((chr = getc(pfile)) == EOF) {
    197:         return(EOF);
    198:       }
    199:     }
    200:   } /* end of while( (chr == CR) || ... */
    201: 
    202:   // read the line until you pick up any trailing cr's or lfs.
    203:   while(num_chr < MAX_LINE_LEN - 1) {
    204:     // if lf/cr reached before filling buffer, exit
    205:     if((chr == CR) || (chr == LF))
    206:       break;
    207: 
    208:     // enter new char to buffer
    209:     buff[num_chr++] = (char)chr;
    210: 
    211:     // if we get the EOF, end the string at that point by replacing it
    212:     // with a null and then setting the flag that we're done
    213:     if((chr = getc(pfile)) == EOF) {
    214:       buff[num_chr] = '\0';
    215:       eof = EOF;
    216:       break;
    217:     }
    218:   } /* end of while( num_chr < MAX_LINE_LEN - 1 ) */
    219:   
    220:   // If we exited the loop because the buffer is full (not because of CR/LF or EOF),
    221:   // we need to consume any remaining characters on this line so they don't become
    222:   // part of the next line. Also check if any are non-whitespace.
    223:   if(num_chr >= MAX_LINE_LEN - 1 && chr != CR && chr != LF && eof != EOF) {
    224:     bool found_nonws = false;
    225:     // Continue reading to end of line, checking for non-whitespace
    226:     while((chr = getc(pfile)) != EOF && chr != CR && chr != LF) {
    227:       if(!isspace((unsigned char)chr)) {
    228:         found_nonws = true;
    229:       }
    230:     }
    231:     // Report if we found non-whitespace beyond the limit
    232:     if(found_nonws) {
    233:       char msg[MAX_ERROR_LEN];
    234:       snprintf(msg, MAX_ERROR_LEN, "The card on line %d has non-whitespace characters beyond %d characters, these have been removed.", 
    235:                line_num, MAX_LINE_LEN);
>   236:       add_error(ctx, &ctx->errors, msg, WARNING);
    237:     }
    238:   }
    239:   
    240:   // terminate buffer as a string
    241:   buff[num_chr] = '\0';

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L91:         snprintf(msg, MAX_ERROR_LEN, "[read_deck] ERROR: calloc failed for deck->cards at line %d", line_num);
L100:         snprintf(msg, MAX_ERROR_LEN, "[read_deck] ERROR: realloc failed for deck->cards at line %d", line_num);
L115:       snprintf(msg, MAX_ERROR_LEN, "[read_deck] ERROR: calloc failed for card->orig_str at line %d", line_num);
L234:       snprintf(msg, MAX_ERROR_LEN, "The card on line %d has non-whitespace characters beyond %d characters, these have been removed.", 

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/input.c
LINE: 336
CALL: add_error(ctx, errors, msg, 0);
--- Context (lines 276-341) ---
    276:     // Check if current card is entirely a comment
    277:     line_len = strlen(curr_card->orig_str);
    278:     size_t first = 0;
    279:     while (first < line_len && isspace((unsigned char)curr_card->orig_str[first])) first++;
    280:     
    281:     bool curr_is_comment = false;
    282:     if (first < line_len) {
    283:       if ((toupper(curr_card->orig_str[first]) == 'C' && toupper(curr_card->orig_str[first+1]) == 'M') ||
    284:           (toupper(curr_card->orig_str[first]) == 'C' && toupper(curr_card->orig_str[first+1]) == 'E') ||
    285:           curr_card->orig_str[first] == '!' ||
    286:           curr_card->orig_str[first] == '#' ||
    287:           curr_card->orig_str[first] == '\'') {
    288:         curr_is_comment = true;
    289:       }
    290:     }
    291:     
    292:     // Check if previous card is not a comment and doesn't have a trailing comment
    293:     line_len = strlen(prev_card->orig_str);
    294:     first = 0;
    295:     while (first < line_len && isspace((unsigned char)prev_card->orig_str[first])) first++;
    296:     
    297:     bool prev_is_comment = false;
    298:     if (first < line_len) {
    299:       if ((toupper(prev_card->orig_str[first]) == 'C' && toupper(prev_card->orig_str[first+1]) == 'M') ||
    300:           (toupper(prev_card->orig_str[first]) == 'C' && toupper(prev_card->orig_str[first+1]) == 'E') ||
    301:           prev_card->orig_str[first] == '!' ||
    302:           prev_card->orig_str[first] == '#' ||
    303:           prev_card->orig_str[first] == '\'') {
    304:         prev_is_comment = true;
    305:       }
    306:     }
    307:     
    308:     // Check if current comment line has onec key/values
    309:     bool has_onec_extensions = false;
    310:     if (curr_is_comment) {
    311:       // Parse the comment to see if it has extensions
    312:       char temp_str[MAX_LINE_LEN];
    313:       size_t orig_len = strlen(curr_card->orig_str);
    314:       if (orig_len >= MAX_LINE_LEN) orig_len = MAX_LINE_LEN - 1;
    315:       strncpy(temp_str, curr_card->orig_str, orig_len);
    316:       temp_str[orig_len] = '\0';
    317:       
    318:       // Find the comment part (after the marker)
    319:       char *comment_start = NULL;
    320:       if (strstr(temp_str, "CM") == temp_str || strstr(temp_str, "CE") == temp_str) {
    321:         comment_start = temp_str + 2;
    322:       } else if (temp_str[0] == '!' || temp_str[0] == '#' || temp_str[0] == '\'') {
    323:         comment_start = temp_str + 1;
    324:       }
    325:       
    326:       if (comment_start && strstr(comment_start, "onec:")) {
    327:         has_onec_extensions = true;
    328:       }
    329:     }
    330:     
    331:     // If conditions are met, merge the comment
    332:     if (curr_is_comment && has_onec_extensions && !prev_is_comment && prev_card->comment == NULL) {
    333:       // Add error message
    334:       char msg[MAX_ERROR_LEN];
    335:       snprintf(msg, MAX_ERROR_LEN, "Comment line on card %d contains onec extensions and is being merged into line %d", i+1, i);
>   336:       add_error(ctx, errors, msg, 0);
    337:       
    338:       // copy the comment from current card to previous card
    339:       if (curr_card->comment) {
    340:         prev_card->comment = strdup(curr_card->comment);
    341:       } else {

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L234:       snprintf(msg, MAX_ERROR_LEN, "The card on line %d has non-whitespace characters beyond %d characters, these have been removed.", 
L335:       snprintf(msg, MAX_ERROR_LEN, "Comment line on card %d contains onec extensions and is being merged into line %d", i+1, i);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/input.c
LINE: 537
CALL: add_error(ctx, errors, msg, PROBLEM);
--- Context (lines 477-542) ---
    477:       if(pos < line_len && isspace((unsigned char)card->orig_str[pos])) {
    478:         pos++;
    479:       }
    480:       
    481:       // extract and uppercase the two characters at pos as a potential card code.
    482:       // Also require that a valid separator (whitespace, comma, or end-of-string)
    483:       // follows the two-character code — this prevents a plain comment line like
    484:       // "' Except for..." from being mis-detected as a hidden EX card just because
    485:       // its first two significant letters happen to match a known card mnemonic.
    486:       if(pos + 1 < line_len) {
    487:         hidden_type_buff[0] = toupper((unsigned char)card->orig_str[pos]);
    488:         hidden_type_buff[1] = toupper((unsigned char)card->orig_str[pos + 1]);
    489:         hidden_type_buff[2] = '\0';
    490:         // verify the character after the 2-char code is a separator or end-of-string
    491:         size_t after = pos + 2;
    492:         if(after < line_len && !isspace((unsigned char)card->orig_str[after]) && card->orig_str[after] != ',') {
    493:           hidden_type_buff[0] = '\0'; // not a valid card code boundary — treat as plain comment
    494:         }
    495:       } else {
    496:         hidden_type_buff[0] = '\0';
    497:       }
    498:       
    499:       // check all three code arrays: OpenNEC extensions, geometry, and control
    500:       bool isHidden = false;
    501:       bool hiddenIsExt = false, hiddenIsGeo = false, hiddenIsCtl = false;
    502:       for(int j = 0; j < NUM_ONEC_CODES && !isHidden; j++) {
    503:         if(strcmp(hidden_type_buff, onec_codes[j]) == 0) { isHidden = true; hiddenIsExt = true; }
    504:       }
    505:       for(int j = 0; j < NUM_GEOMETRY_CODES && !isHidden; j++) {
    506:         if(strcmp(hidden_type_buff, geometry_codes[j]) == 0) { isHidden = true; hiddenIsGeo = true; }
    507:       }
    508:       for(int j = 0; j < NUM_CONTROL_CODES && !isHidden; j++) {
    509:         if(strcmp(hidden_type_buff, control_codes[j]) == 0) { isHidden = true; hiddenIsCtl = true; }
    510:       }
    511:       
    512:       if(isHidden) {
    513:         // set card_code to the real code, not the comment marker
    514:         strncpy(card->card_code, hidden_type_buff, 2);
    515:         card->card_code[2] = '\0';
    516:         // save the leading comment marker character
    517:         card->cmt_code[0] = card->orig_str[first];
    518:         // mark as ignored (commented out); processing will skip it
    519:         card->ignore = true;
    520:         // update flags
    521:         isCmt = false;
    522:         isExt = hiddenIsExt;
    523:         isGeo = hiddenIsGeo;
    524:         isCtl = hiddenIsCtl;
    525:         // card_str starts at the actual code position, not the comment marker
    526:         card_str_offset = pos;
    527:       }
    528:     } // checking for hidden info
    529:     
    530:     // if we're past the end of the deck, everything that appears is a comment,
    531:     // and we'll just copy it into the comment string. but if we are not past
    532:     // the end, and we didn't recognize the code then we want to report
    533:     // an error
    534:     if (!sawEN && !isCmt && !isCtl && !isGeo && !isExt) {
    535:       char msg[MAX_ERROR_LEN];
    536:       snprintf(msg, MAX_ERROR_LEN, "The card on line %d has unknown type '%s'. Card skipped.", i+1, type_buff);
>   537:       add_error(ctx, errors, msg, PROBLEM);
    538:     }
    539:     
    540:     // if we did figure out the card type, then we want to put something in the card_str,
    541:     // but first we want to see if there is a comment inside the line ( > 0, < len ) and
    542:     // clip that part out separately into extn_str.

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L536:       snprintf(msg, MAX_ERROR_LEN, "The card on line %d has unknown type '%s'. Card skipped.", i+1, type_buff);

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/main.c
LINE: 87
CALL: * TODO: Make this static once all calculation files use add_error() instead
--- Context (lines 27-92) ---
     27: #include <sys/stat.h>
     28: #include <dirent.h>
     29: #include <unistd.h>
     30: 
     31: /** signal handler */
     32: // static void sig_handler(int signal);
     33: 
     34: // various switches for the command line arguments
     35: static bool run_simulation = true;
     36: static bool run_tests = false;
     37: static bool run_greens = false;
     38: static bool recursive = false;
     39: static char *input_file = "";
     40: static char *output_file = "";
     41: static char *error_file = "";
     42: static char *greens_file = "";
     43: static int jobs = 1; // number of parallel jobs (-j)
     44: 
     45: /******************************************************************************
     46:  * print_version()
     47:  *
     48:  * as the name implies, this simply prints the VERSION_STRING to stdout
     49:  *
     50:  */
     51: static void print_version(void);
     52: static void print_version(void)
     53: {
     54:   puts("onec " VERSION_STRING);
     55:   exit(0);
     56: }
     57: 
     58: /******************************************************************************
     59:  * print_usage()
     60:  *
     61:  * prints the usage notes
     62:  *
     63:  */
     64: void print_usage(char *argv[])
     65: {
     66:   printf("Usage: %s [-hvntgr] [-i input_file] [-o output_file] [-e error_file] <input_file...>\n", argv[0]);
     67:   puts("Options:");
     68:   puts("  -h, --help: print this description");
     69:   puts("  -v, --version: print version info");
     70:   puts("  -n, --no-run: don't run the simulation after parsing");
     71:   puts("  -t, --test-deck: run various sanity tests");
     72:   puts("  -i file, --input-file=file: read input file. this is not required if input_file is provided. if neither is provided, input is read from stdin");
     73:   puts("  -o file, --output-file=file: write output to file. omitted -o writes to stdout (single file) or <file>.out (multiple files)");
     74:   puts("  -e, --error-file: output errors to (path/)file, instead of stderr");
     75:   puts("  -g, --greens[=file]: write a Green's function file; filename defaults to input path with .ngf extension");
     76:   puts("  -r, --recursive: recurse into subdirectories");
     77:   puts("  -j, --jobs N: process up to N files in parallel (default 1)");
     78:   puts("Multiple input files or folders can be specified; each file will generate a .out file.");
     79:   puts("If no input_file is provided, input is read from stdin and output goes to stdout.");
     80:   exit(0);
     81: }
     82: 
     83: /******************************************************************************
     84:  * stop()
     85:  *
     86:  * Cleanup and exit - the single exit point for the program
>    87:  * TODO: Make this static once all calculation files use add_error() instead
     88:  *
     89:  */
     90: int stop(const nec_context_t *ctx, int flag)
     91: {
     92:   if (ctx->input_fp != NULL)

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
NONE FOUND - manual review needed

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/calculations.c
LINE: 309
CALL: add_error(ctx, &ctx->errors, err_msg, FATAL);
--- Context (lines 249-314) ---
    249: /* load calculates the impedance of specified */
    250: /* segments for various types of loading */
    251: int load(nec_context_t *ctx, int *ldtyp, int *ldtag, int *ldtagf, int *ldtagt,
    252:           double *zlr, double *zli, double *zlc )
    253: {
    254:   int i, istep, istepx, l1, l2, ldtags, jump, ichk;
    255:   bool iwarn;
    256:   complex double zt=CPLX_00, tpcj;
    257:   size_t mreq;
    258:   
    259:   tpcj = (0.0+I*1.883698955e+9);
    260:   
    261:   /* initialize d array, used for temporary */
    262:   /* storage of loading information. */
    263:   mreq = (size_t)ctx->geometry.npm;
    264:   mreq *= sizeof(complex double);
    265:   mem_realloc(ctx,  (void *)&ctx->zload.zarray, mreq );
    266:   for( i = 0; i < ctx->geometry.n; i++ )
    267:     ctx->zload.zarray[i]=CPLX_00;
    268:   
    269:   iwarn=false;
    270:   istep=0;
    271:   
    272:   /* cycle over loading cards */
    273:   while( true )
    274:   {
    275:     istepx = istep;
    276:     istep++;
    277:     
    278:     if( istep > ctx->zload.nload)
    279:     {
    280:       if( iwarn == true )
    281:         nec_report(ctx, ONEC_SEV_WARNING,
    282:                    "Some segments have been loaded more than once; impedances added.");
    283: 
    284:       ctx->smat.nop = ctx->geometry.n/ctx->geometry.np;
    285:       if( ctx->smat.nop == 1)
    286:         return 0;
    287:       
    288:       for( i = 0; i < ctx->geometry.np; i++ )
    289:       {
    290:         zt= ctx->zload.zarray[i];
    291:         l1= i;
    292:         
    293:         for( l2 = 1; l2 < ctx->smat.nop; l2++ )
    294:         {
    295:           l1 += ctx->geometry.np;
    296:           ctx->zload.zarray[l1]= zt;
    297:         }
    298:       }
    299:       return 0;
    300:       
    301:     } /* if( istep > ctx->zload.nload) */
    302:     
    303:     if( ldtyp[istepx] > 5 )
    304:     {
    305:       char err_msg[256];
    306:       snprintf(err_msg, sizeof(err_msg), 
    307:               "INTERNAL ERROR: IMPROPER LOAD TYPE %d processed in load().", ldtyp[istepx]);
    308: 
>   309:       add_error(ctx, &ctx->errors, err_msg, FATAL);
    310:       return -1;
    311:     }
    312:     
    313:     /* search segments for proper itags */
    314:     ldtags= ldtag[istepx];

-- Message variable name candidates --
VAR_EXPR: err_msg  VAR_NAME: err_msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L306:       snprintf(err_msg, sizeof(err_msg), 

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/calculations.c
LINE: 402
CALL: add_error(ctx, &ctx->errors, err_msg, FATAL);
--- Context (lines 342-407) ---
    342:         else
    343:           ichk=1;
    344:         
    345:       } /* if( ldtags != 0) */
    346:       else
    347:         ichk=1;
    348:       
    349:       /* calculation of lamda*imped. per unit length, */
    350:       /* jump to appropriate section for loading type */
    351:       switch( jump ) {
    352:         case 1:
    353:           zt= zlr[istepx]/ ctx->geometry.si[i]+ tpcj* zli[istepx]/( ctx->geometry.si[i]* ctx->geometry.wlam);
    354:           if( fabs( zlc[istepx]) > 1.0e-20)
    355:             zt += ctx->geometry.wlam/( tpcj* ctx->geometry.si[i]* zlc[istepx]);
    356:           break;
    357:           
    358:         case 2:
    359:           zt= tpcj* ctx->geometry.si[i]* zlc[istepx]/ ctx->geometry.wlam;
    360:           if( fabs( zli[istepx]) > 1.0e-20)
    361:             zt += ctx->geometry.si[i]* ctx->geometry.wlam/( tpcj* zli[istepx]);
    362:           if( fabs( zlr[istepx]) > 1.0e-20)
    363:             zt += ctx->geometry.si[i]/ zlr[istepx];
    364:           zt=1./ zt;
    365:           break;
    366:           
    367:         case 3:
    368:           zt= zlr[istepx]* ctx->geometry.wlam+ tpcj* zli[istepx];
    369:           if( fabs( zlc[istepx]) > 1.0e-20)
    370:             zt += 1./( tpcj* ctx->geometry.si[i]* ctx->geometry.si[i]* zlc[istepx]);
    371:           break;
    372:           
    373:         case 4:
    374:           zt= tpcj* ctx->geometry.si[i]* ctx->geometry.si[i]* zlc[istepx];
    375:           if( fabs( zli[istepx]) > 1.0e-20)
    376:             zt += 1./( tpcj* zli[istepx]);
    377:           if( fabs( zlr[istepx]) > 1.0e-20)
    378:             zt += 1./( zlr[istepx]* ctx->geometry.wlam);
    379:           zt=1./ zt;
    380:           break;
    381:           
    382:         case 5:
    383:           zt= cmplx( zlr[istepx], zli[istepx])/ ctx->geometry.si[i];
    384:           break;
    385:           
    386:         case 6:
    387:           zint( ctx, zlr[istepx]* ctx->geometry.wlam, ctx->geometry.bi[i], &zt );
    388:           
    389:       } /* switch( jump ) */
    390:       
    391:       if(( fabs( creal( ctx->zload.zarray[i]))+ fabs( cimag( ctx->zload.zarray[i]))) > 1.0e-20)
    392:         iwarn=true;
    393:       ctx->zload.zarray[i] += zt;
    394:       
    395:     } /* for( i = l1-1; i < l2; i++ ) */
    396:     
    397:     if( ichk == 0 )
    398:     {
    399:       char err_msg[256];
    400:       snprintf(err_msg, sizeof(err_msg),
    401:               "LOADING DATA CARD ERROR, NO SEGMENT HAS AN ITAG = %d", ldtags);
>   402:       add_error(ctx, &ctx->errors, err_msg, FATAL);
    403:       return -1;
    404:     }
    405:     
    406:     /* Store the segment loading data for output */
    407:     switch( jump )

-- Message variable name candidates --
VAR_EXPR: err_msg  VAR_NAME: err_msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L306:       snprintf(err_msg, sizeof(err_msg), 
L400:       snprintf(err_msg, sizeof(err_msg),

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/calculations.c
LINE: 913
CALL: add_error(ctx, &ctx->errors, err_msg, FATAL);
--- Context (lines 853-918) ---
    853: 
    854: /* test for convergence in numerical integration */
    855: void test(nec_context_t *ctx, double f1r, double f2r, double *tr,
    856:           double f1i, double f2i, double *ti, double dmin )
    857: {
    858:   double den;
    859:   
    860:   den= fabs( f2r);
    861:   *tr= fabs( f2i);
    862:   
    863:   if( den < *tr)
    864:     den= *tr;
    865:   if( den < dmin)
    866:     den= dmin;
    867:   
    868:   if( den < 1.0e-37)
    869:   {
    870:     *tr=0.;
    871:     *ti=0.;
    872:     return;
    873:   }
    874:   
    875:   *tr= fabs(( f1r- f2r)/ den);
    876:   *ti= fabs(( f1i- f2i)/ den);
    877:   
    878:   return;
    879: }
    880: 
    881: /*-----------------------------------------------------------------------*/
    882: 
    883: /* compute component of basis function i on segment is. */
    884: int sbf(nec_context_t *ctx, int i, int is, double *aa, double *bb, double *cc )
    885: {
    886:   int ix, jsno, june, jcox, jcoxx, jend, iend, njun1=0, njun2;
    887:   double d, sig, pp, sdh, cdh, sd, omc, aj, pm=0, cd, ap, qp, qm, xxi;
    888:   
    889:   *aa=0.;
    890:   *bb=0.;
    891:   *cc=0.;
    892:   june=0;
    893:   jsno=0;
    894:   pp=0.;
    895:   ix=i-1;
    896:   
    897:   jcox= ctx->geometry.icon1[ix];
    898:   if( jcox > PCHCON) jcox= i;
    899:   
    900:   jend=-1;
    901:   iend=-1;
    902:   sig=-1.;
    903:   int sbf_hops = 0;
    904:   
    905:   do
    906:   {
    907:     if( jcox != 0 )
    908:     {
    909:       if(++sbf_hops > ctx->geometry.n) {
    910:         char err_msg[256];
    911:         snprintf(err_msg, sizeof(err_msg),
    912:             "Segment connection cycle detected at segment %d — geometry is degenerate", i);
>   913:         add_error(ctx, &ctx->errors, err_msg, FATAL);
    914:         return -1;
    915:       }
    916: 
    917:       if( jcox < 0 )
    918:         jcox= -jcox;

-- Message variable name candidates --
VAR_EXPR: err_msg  VAR_NAME: err_msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L911:         snprintf(err_msg, sizeof(err_msg),

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/calculations.c
LINE: 965
CALL: add_error(ctx, &ctx->errors, err_msg, FATAL);
--- Context (lines 905-970) ---
    905:   do
    906:   {
    907:     if( jcox != 0 )
    908:     {
    909:       if(++sbf_hops > ctx->geometry.n) {
    910:         char err_msg[256];
    911:         snprintf(err_msg, sizeof(err_msg),
    912:             "Segment connection cycle detected at segment %d — geometry is degenerate", i);
    913:         add_error(ctx, &ctx->errors, err_msg, FATAL);
    914:         return -1;
    915:       }
    916: 
    917:       if( jcox < 0 )
    918:         jcox= -jcox;
    919:       else
    920:       {
    921:         sig= -sig;
    922:         jend= -jend;
    923:       }
    924:       jcoxx = jcox-1;
    925:       
    926:       jsno++;
    927:       d= PI* ctx->geometry.si[jcoxx];
    928:       sdh= sin( d);
    929:       cdh= cos( d);
    930:       sd=2.* sdh* cdh;
    931:       
    932:       if( d <= 0.015)
    933:       {
    934:         omc=4.* d* d;
    935:         omc=((1.3888889e-3* omc -4.1666666667e-2)* omc +.5)* omc;
    936:       }
    937:       else
    938:         omc=1.- cdh* cdh+ sdh* sdh;
    939:       
    940:       aj=1./( log(1./( PI* ctx->geometry.bi[jcoxx]))-.577215664);
    941:       pp -= omc/ sd* aj;
    942:       
    943:       if( jcox == is)
    944:       {
    945:         *aa= aj/ sd* sig;
    946:         *bb= aj/(2.* cdh);
    947:         *cc= -aj/(2.* sdh)* sig;
    948:         june= iend;
    949:       }
    950:       
    951:       if( jcox != i )
    952:       {
    953:         if( jend != 1)
    954:           jcox= ctx->geometry.icon1[jcoxx];
    955:         else
    956:           jcox= ctx->geometry.icon2[jcoxx];
    957:         
    958:         if( abs(jcox) != i )
    959:         {
    960:           if( jcox == 0 )
    961:           {
    962:             char err_msg[256];
    963:             snprintf(err_msg, sizeof(err_msg),
    964:                     "SBF - SEGMENT CONNECTION ERROR FOR SEGMENT %d", i);
>   965:             add_error(ctx, &ctx->errors, err_msg, FATAL);
    966:             return -1;
    967:           }
    968:           else
    969:             continue;
    970:         }

-- Message variable name candidates --
VAR_EXPR: err_msg  VAR_NAME: err_msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L911:         snprintf(err_msg, sizeof(err_msg),
L963:             snprintf(err_msg, sizeof(err_msg),

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/calculations.c
LINE: 1173
CALL: add_error(ctx, &ctx->errors, err_msg, FATAL);
--- Context (lines 1113-1178) ---
   1113:   ctx->segj.jsno=0;
   1114:   pp=0.;
   1115:   ix = i-1;
   1116:   jcox= ctx->geometry.icon1[ix];
   1117:   
   1118:   if( jcox > PCHCON) jcox= i;
   1119:   
   1120:   jend=-1;
   1121:   iend=-1;
   1122:   sig=-1.;
   1123:   
   1124:   do {
   1125:     if( jcox != 0 ) {
   1126:       if( jcox < 0 )
   1127:         jcox= -jcox;
   1128:       else
   1129:       {
   1130:         sig= -sig;
   1131:         jend= -jend;
   1132:       }
   1133:       
   1134:       jcoxx = jcox-1;
   1135:       ctx->segj.jsno++;
   1136:       jsnox = ctx->segj.jsno-1;
   1137:       ctx->segj.jco[jsnox]= jcox;
   1138:       d= PI* ctx->geometry.si[jcoxx];
   1139:       sdh= sin( d);
   1140:       cdh= cos( d);
   1141:       sd=2.* sdh* cdh;
   1142:       
   1143:       if( d <= 0.015)
   1144:       {
   1145:         omc=4.* d* d;
   1146:         omc=((1.3888889e-3* omc-4.1666666667e-2)* omc+.5)* omc;
   1147:       }
   1148:       else
   1149:         omc=1.- cdh* cdh+ sdh* sdh;
   1150:       
   1151:       aj=1./( log(1./( PI* ctx->geometry.bi[jcoxx]))-.577215664);
   1152:       pp= pp- omc/ sd* aj;
   1153:       ctx->segj.ax[jsnox]= aj/ sd* sig;
   1154:       ctx->segj.bx[jsnox]= aj/(2.* cdh);
   1155:       ctx->segj.cx[jsnox]= -aj/(2.* sdh)* sig;
   1156:       
   1157:       if( jcox != i)
   1158:       {
   1159:         if( jend == 1)
   1160:           jcox= ctx->geometry.icon2[jcoxx];
   1161:         else
   1162:           jcox= ctx->geometry.icon1[jcoxx];
   1163:         
   1164:         if( abs(jcox) != i )
   1165:         {
   1166:           if( jcox != 0 )
   1167:             continue;
   1168:           else
   1169:           {
   1170:             char err_msg[256];
   1171:             snprintf(err_msg, sizeof(err_msg),
   1172:                     "TBF - SEGMENT CONNECTION ERROR FOR SEGMENT %5d", i);
>  1173:             add_error(ctx, &ctx->errors, err_msg, FATAL);
   1174:             return -1;
   1175:           }
   1176:         }
   1177:         
   1178:       } /* if( jcox != i) */

-- Message variable name candidates --
VAR_EXPR: err_msg  VAR_NAME: err_msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L1171:             snprintf(err_msg, sizeof(err_msg),

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/calculations.c
LINE: 1420
CALL: add_error(ctx, &ctx->errors, err_msg, FATAL);
--- Context (lines 1360-1425) ---
   1360:         ctx->segj.maxcon = ctx->segj.jsno +1;
   1361:         size_t mreq = (size_t)ctx->segj.maxcon;
   1362:         mreq *= sizeof(int);
   1363:         mem_realloc(ctx,  (void *)&ctx->segj.jco, mreq );
   1364:         mreq = (size_t)ctx->segj.maxcon;
   1365:         mreq *= sizeof(double);
   1366:         mem_realloc(ctx,  (void *) &ctx->segj.ax, mreq );
   1367:         mem_realloc(ctx,  (void *) &ctx->segj.bx, mreq );
   1368:         mem_realloc(ctx,  (void *) &ctx->segj.cx, mreq );
   1369:       }
   1370:       
   1371:       if (sbf(ctx,  j, j, &ctx->segj.ax[jsnox], &ctx->segj.bx[jsnox], &ctx->segj.cx[jsnox]) != 0)
   1372:         return -1;
   1373:       ctx->segj.jco[jsnox]= j;
   1374:       return 0;
   1375:     }
   1376:     
   1377:   } /* if( (jcox == 0) || (jcox > PCHCON) ) */
   1378:   
   1379:   do
   1380:   {
   1381:     if( jcox < 0 )
   1382:       jcox= -jcox;
   1383:     else
   1384:       jend= -jend;
   1385:     jcoxx = jcox-1;
   1386:     
   1387:     if( jcox != j)
   1388:     {
   1389:       jsnox = ctx->segj.jsno;
   1390:       ctx->segj.jsno++;
   1391:       
   1392:       /* Allocate to connections buffers */
   1393:       if( ctx->segj.jsno >= ctx->segj.maxcon )
   1394:       {
   1395:         ctx->segj.maxcon = ctx->segj.jsno +1;
   1396:         size_t mreq = (size_t)ctx->segj.maxcon;
   1397:         mreq *= sizeof(int);
   1398:         mem_realloc(ctx,  (void *)&ctx->segj.jco, mreq );
   1399:         mreq = (size_t)ctx->segj.maxcon;
   1400:         mreq *= sizeof(double);
   1401:         mem_realloc(ctx,  (void *) &ctx->segj.ax, mreq );
   1402:         mem_realloc(ctx,  (void *) &ctx->segj.bx, mreq );
   1403:         mem_realloc(ctx,  (void *) &ctx->segj.cx, mreq );
   1404:       }
   1405:       
   1406:       if (sbf(ctx,  jcox, j, &ctx->segj.ax[jsnox], &ctx->segj.bx[jsnox], &ctx->segj.cx[jsnox]) != 0)
   1407:         return -1;
   1408:       ctx->segj.jco[jsnox]= jcox;
   1409:       
   1410:       if( jend != 1)
   1411:         jcox= ctx->geometry.icon1[jcoxx];
   1412:       else
   1413:         jcox= ctx->geometry.icon2[jcoxx];
   1414:       
   1415:       if( jcox == 0 )
   1416:       {
   1417:         char err_msg[256];
   1418:         snprintf(err_msg, sizeof(err_msg),
   1419:                 "TRIO - SEGMENT CONNENTION ERROR FOR SEGMENT %5d", j);
>  1420:         add_error(ctx, &ctx->errors, err_msg, FATAL);
   1421:         return -1;
   1422:       }
   1423:       else
   1424:         continue;
   1425:       

-- Message variable name candidates --
VAR_EXPR: err_msg  VAR_NAME: err_msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L1418:         snprintf(err_msg, sizeof(err_msg),

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/misc.c
LINE: 19
CALL: void add_error(const nec_context_t *ctx, errors_list_t *errors, char *message, int severity)
--- Context (lines 1-24) ---
      1: /******************************************************************************
      2:  * misc.c
      3:  *
      4:  * Miscellaneous support functions for OpenNEC. This module provides utility
      5:  * and helper routines used throughout the codebase, including error handling
      6:  * and general-purpose helpers.
      7:  *
      8:  *****************************************************************************/
      9: 
     10: #include "internals.h"
     11: #include "geometry.h"
     12: #include <unistd.h>
     13: #include <time.h>
     14: #include <stdarg.h>
     15: #include <stdio.h>
     16: 
     17: /***  ONEC utils ***/
     18: 
>    19: void add_error(const nec_context_t *ctx, errors_list_t *errors, char *message, int severity)
     20: {
     21:   // Trigger callback and logging via unified helper
     22:   nec_report(ctx, severity, "%s", message);
     23: 
     24:   // make a new error object and fill it out

-- Message variable name candidates --
VAR_EXPR: char *message  VAR_NAME: *message
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
NONE FOUND - manual review needed

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/output.c
LINE: 1195
CALL: add_error(ctx, &ctx->errors,
--- Context (lines 1135-1200) ---
   1135: 
   1136:   /* Skip optional symmetry submatrix (NOP > 1) */
   1137:   if (NPEQ > 0 && NEQ / NPEQ > 1)
   1138:   {
   1139:     if (!fr_skip(file))
   1140:       goto err;
   1141:   }
   1142: 
   1143:   /* ---------- Record 8: IP[NEQ] + COM[100] — read and discard ---------- */
   1144:   if (!fr_skip(file))
   1145:     goto err;
   1146: 
   1147:   /* ---------- Record 9: CM matrix ----------
   1148:    * IOUT = NEQ * NPEQ for ICASE <= 2; use IMAT as fallback for out-of-core. */
   1149:   int IOUT = (ICASE <= 2) ? NEQ * NPEQ : (int)IMAT;
   1150:   if (IOUT <= 0 || IOUT > 100000000)
   1151:     goto err;
   1152: 
   1153:   complex double *mat = (complex double *)malloc((size_t)IOUT * sizeof(complex double));
   1154:   if (!mat)
   1155:     goto err;
   1156:   if (!fr1(file, mat, (int32_t)(IOUT * 16)))
   1157:   {
   1158:     free(mat);
   1159:     goto err;
   1160:   }
   1161: 
   1162:   /* Install NGF state */
   1163:   if (ctx->ngf_cm != NULL)
   1164:     free(ctx->ngf_cm);
   1165:   ctx->ngf_cm = mat;
   1166:   ctx->ngf_n_segs = (int)N;
   1167:   ctx->ngf_neq = NEQ;
   1168:   ctx->ngf_fmhz = FMHZ;
   1169:   ctx->has_ngf = true;
   1170: 
   1171:   /* Update geometry bookkeeping */
   1172:   ctx->geometry.n = (int)N;
   1173:   ctx->geometry.np = (NP > 0) ? (int)NP : (int)N;
   1174:   ctx->geometry.m = (int)M;
   1175:   ctx->geometry.mp = (int)MP;
   1176:   ctx->geometry.wlam = WLAM;
   1177:   ctx->geometry.ipsym = (int)IPSYM;
   1178:   ctx->geometry.npm = (int)(N + M);
   1179:   ctx->geometry.np2m = (int)(N + 2 * M);
   1180:   ctx->geometry.np3m = (int)(N + 3 * M);
   1181: 
   1182:   /* Restore ground and frequency parameters from the NGF */
   1183:   ctx->gnd.ksymp = (int)KSYMP;
   1184:   ctx->gnd.iperf = (int)IPERF;
   1185:   ctx->gnd.nradl = (int)NRADL;
   1186:   ctx->gnd.scrwl = SCRWLT;
   1187:   ctx->gnd.scrwr = SCRWRT;
   1188:   ctx->save.epsr = EPSR;
   1189:   ctx->save.sig = SIG;
   1190:   ctx->save.fmhz = FMHZ;
   1191: 
   1192:   return true;
   1193: 
   1194: err:
>  1195:   add_error(ctx, &ctx->errors,
   1196:             "Failed to read NGF/WGF file (not a valid Fortran NEC-2 unformatted "
   1197:             "file, or file is truncated or corrupted)",
   1198:             FATAL);
   1199:   return false;
   1200: }

-- Message variable name candidates --
VAR_EXPR: msg  VAR_NAME: msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
NONE FOUND - manual review needed

========================================================================

FILE: /Volumes/Bigger/Users/maury/Desktop/OpenNEC/src/matrix.c
LINE: 1284
CALL: add_error(ctx, &ctx->errors, err_msg, FATAL);
--- Context (lines 1224-1289) ---
   1224: 
   1225:   } /* for( r=0; r < n; r++ ) */
   1226: 
   1227: 	mem_free(ctx, (void *)&scm );
   1228: #endif /* HAVE_ACCELERATE || HAVE_OPENBLAS || HAVE_BLAS || HAVE_MKL */
   1229: 
   1230:   return;
   1231: }
   1232: 
   1233: /*-----------------------------------------------------------------------*/
   1234: 
   1235: /* factrs, for symmetric structure, transforms submatricies to form */
   1236: /* matricies of the symmetric modes and calls routine to factor */
   1237: /* matricies.  if no symmetry, the routine is called to factor the */
   1238: /* complete matrix. */
   1239: void factrs(nec_context_t *restrict ctx, int np, int nrow, complex double *restrict a, int *restrict ip )
   1240: {
   1241:   int kk, ka;
   1242: 
   1243:   ctx->smat.nop = nrow/np;
   1244:   for( kk = 0; kk < ctx->smat.nop; kk++ )
   1245:   {
   1246: 	ka= kk* np;
   1247: 	factr(ctx, np, &a[ka], &ip[ka], nrow );
   1248:   }
   1249:   return;
   1250: }
   1251: 
   1252: /*-----------------------------------------------------------------------*/
   1253: 
   1254: /* fblock sets parameters for out-of-core */
   1255: /* solution for the primary matrix (a) */
   1256: int fblock(nec_context_t *ctx, int nrow, int ncol, int imax, int ipsym )
   1257: {
   1258:   int i, j, k, ka, kk;
   1259:   double phaz, arg;
   1260:   complex double deter;
   1261: 
   1262:   if( nrow*ncol <= imax)
   1263:   {
   1264: 	ctx->matpar.npblk= nrow;
   1265: 	ctx->matpar.nlast= nrow;
   1266: 	ctx->matpar.imat= nrow* ncol;
   1267: 
   1268: 	if( nrow == ncol)
   1269: 	{
   1270: 	  ctx->matpar.icase=1;
   1271: 	  return 0;
   1272: 	}
   1273: 	else
   1274: 	  ctx->matpar.icase=2;
   1275: 
   1276:   } /* if( nrow*ncol <= imax) */
   1277: 
   1278:   ctx->smat.nop = ncol/nrow;
   1279:   if( ctx->smat.nop*nrow != ncol)
   1280:   {
   1281: 	char err_msg[256];
   1282: 	snprintf(err_msg, sizeof(err_msg),
   1283: 		"SYMMETRY ERROR - NROW: %d NCOL: %d", nrow, ncol);
>  1284: 	add_error(ctx, &ctx->errors, err_msg, FATAL);
   1285: 	return -1;
   1286:   }
   1287: 
   1288:   /* set up smat.ssx matrix for rotational symmetry. */
   1289:   if( ipsym <= 0)

-- Message variable name candidates --
VAR_EXPR: err_msg  VAR_NAME: err_msg
-- Searching prior 200 lines for assignments/sprintf/asprintf/etc --
L1282: 	snprintf(err_msg, sizeof(err_msg),

========================================================================

