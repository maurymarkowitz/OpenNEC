#include <stdio.h>
#include "src/types.h"

int main() {
    printf("NUM_ONEC_UNIT_CODES = %d\n", NUM_ONEC_UNIT_CODES);
    for (int i = 0; i < NUM_ONEC_UNIT_CODES; i++) {
        printf("unit_codes[%d] = \"%s\" (mult=%.6f)\n", i, unit_codes[i], unit_mult[i]);
    }
    return 0;
}
