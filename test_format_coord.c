#include <stdio.h>
#include <string.h>

static void format_coord(char *buf, int len, double value, const char *fmt)
{
    snprintf(buf, len, fmt, value);
    // If the result is "-0.xxxx" (negative zero), convert to " 0.xxxx"
    if (buf[0] == '-' && buf[1] == '0' && buf[2] == '.')
    {
        // Just replace the minus with a space for alignment
        buf[0] = ' ';
    }
}

int main() {
    char buf[12];
    double negzero = -0.0;
    double poszero = 0.0;
    double neggoodval = -1.5;
    
    format_coord(buf, sizeof(buf), negzero, "%9.4f");
    printf("Negative zero with %%9.4f: %s (first char: %d)\n", buf, (unsigned char)buf[0]);
    
    format_coord(buf, sizeof(buf), poszero, "%9.4f");
    printf("Positive zero with %%9.4f: %s (first char: %d)\n", buf, (unsigned char)buf[0]);
    
    format_coord(buf, sizeof(buf), neggoodval, "%9.4f");
    printf("Negative value with %%9.4f: %s (first char: %d)\n", buf, (unsigned char)buf[0]);
    
    return 0;
}
