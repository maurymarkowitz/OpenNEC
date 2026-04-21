#include <stdio.h>
#include <string.h>

int main() {
    char buf[12];
    double negzero = -0.0;
    
    snprintf(buf, sizeof(buf), "%10.5f", negzero);
    printf("Formatted: '%s'\n", buf);
    printf("Length: %zu\n", strlen(buf));
    printf("Char by char:\n");
    for (int i = 0; i < strlen(buf); i++) {
        printf("  [%d] = '%c' (0x%02x)\n", i, buf[i], (unsigned char)buf[i]);
    }
    
    return 0;
}
