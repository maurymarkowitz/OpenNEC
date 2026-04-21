#include <stdio.h>
#include <math.h>

int main() {
    double negzero = -0.0;
    double poszero = 0.0;
    
    printf("Negative zero: %11.5f\n", negzero);
    printf("Positive zero: %11.5f\n", poszero);
    printf("Are they equal? %d\n", negzero == poszero);
    printf("Negative zero bits: %016llx\n", *(unsigned long long*)&negzero);
    printf("Positive zero bits: %016llx\n", *(unsigned long long*)&poszero);
    
    // Test with some arithmetic
    double val = -0.5;
    double result = val + 0.5;  // Should give positive zero
    printf("Result of -0.5 + 0.5: %11.5f\n", result);
    
    return 0;
}
