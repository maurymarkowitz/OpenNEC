#!/usr/bin/env python3
with open("src/deck.c", "r") as f:
    content = f.read()

# Fix 1: Add early check for exact unit match
old1 = """    key_value_t *sym = syms[i];
    
    // Recursively evaluate all referenced symbols first"""

new1 = """    key_value_t *sym = syms[i];
    
    // Early check: if the value exactly matches a unit name, set multiplier directly
    for(int u = 1; u < NUM_ONEC_UNIT_CODES; u++) {
      if (strcmp(sym->value, unit_codes[u]) == 0) {
        sym->fv = unit_mult[u];
        return;
      }
    }
    
    // Recursively evaluate all referenced symbols first"""

content = content.replace(old1, new1)

# Fix 2: Handle empty formula after unit stripping
old2 = """          // trim trailing space
          while(strlen(formula_to_eval) > 0 && formula_to_eval[strlen(formula_to_eval)-1] == ' ') {
            formula_to_eval[strlen(formula_to_eval)-1] = '\\0';
          }
          break;"""

new2 = """          // trim trailing space
          while(strlen(formula_to_eval) > 0 && formula_to_eval[strlen(formula_to_eval)-1] == ' ') {
            formula_to_eval[strlen(formula_to_eval)-1] = '\\0';
          }
          // If formula is now empty, this was a pure unit value (e.g., "mm")
          if (strlen(formula_to_eval) == 0) {
            free(formula_to_eval);
            formula_to_eval = strdup("1");
          }
          break;"""

content = content.replace(old2, new2)

with open("src/deck.c", "w") as f:
    f.write(content)
    
print("Fixes applied successfully")
