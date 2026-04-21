#include <stdio.h>
#include <string.h>

static void format_coord(char *buf, int len, double value, const char *fmt)
{
    snprintf(buf, len, fmt, value);
    // Find the position of the first non-space character
    int i = 0;
    while (i < len && buf[i] == ' ')
        i++;
    
    // If we found "-0." at position i, check if all digits after are zeros
    if (i < len - 2 && buf[i] == '-' && buf[i+1] == '0' && buf[i+2] == '.')
    {
        // Check if all remaining digits are zero
        int all_zero = 1;
        for (int j = i + 3; j < len && buf[j]; j++)
        {
            if (buf[j] != '0')
            {
                all_zero = 0;
                break;
            }
        }
        if (all_zero)
        {
            buf[i] = ' ';  // Replace minus with space for proper alignment
        }
    }
}

int main() {
    char buf[12];
    
    // Test negative zero
    double negzero = -0.0;
    format_coord(buf, sizeof(buf), negzero, "%10.5f");
    printf("Negative zero: '%s'\n", buf);
    
    // Test negative value with zero after decimal
    format_coord(buf, sizeof(buf), -0.357, "%10.5f");
    printf("Negative 0.357: '%s'\n", buf);
    
    // Test actual negative zero from arithmetic
    double result = -0.5 + 0.5;
    format_coord(buf, sizeof(buf), result, "%10.5f");
    printf("Result of -0.5 + 0.5: '%s'\n", buf);
    
    return 0;
}
