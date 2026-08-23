/*
 * test_validation.c - Test section validation
 */

#include <stdio.h>
#include "internals.h"
#include "deck_validations.h"

int main(int argc, char *argv[])
{
  if (argc != 2)
  {
    fprintf(stderr, "Usage: %s <nec-file>\n", argv[0]);
    return 1;
  }

  // Create context and deck
  context_t *ctx = create_context();
  deck_t deck = {0};
  init_deck(&deck);

  // Open and read the file
  FILE *fp = fopen(argv[1], "r");
  if (!fp)
  {
    fprintf(stderr, "Error: Cannot open file '%s'\n", argv[1]);
    destroy_context(ctx);
    return 1;
  }

  read_deck(ctx, &deck, fp);
  fclose(fp);

  // Parse the deck (this calls deck_create_sections)
  errors_list_t parse_errors = {0};
  parse_deck(ctx, &deck, &parse_errors);

  // Run validation
  errors_list_t validation_errors = {0};
  test_deck_structure(ctx, &deck, &validation_errors);

  // Display results
  printf("=== VALIDATION RESULTS ===\n");
  printf("Total cards: %d\n", deck.num_cards);
  printf("Number of sections: %d\n", deck.num_sections);
  printf("Parse errors: %d\n", parse_errors.num_errors);
  printf("Validation errors: %d\n\n", validation_errors.num_errors);

  if (parse_errors.num_errors > 0)
  {
    printf("=== PARSE ERRORS ===\n");
    for (int i = 0; i < parse_errors.num_errors; i++)
    {
      printf("[%d] %s\n", parse_errors.errors[i].severity, parse_errors.errors[i].message);
    }
    printf("\n");
  }

  if (validation_errors.num_errors > 0)
  {
    printf("=== VALIDATION ERRORS ===\n");
    for (int i = 0; i < validation_errors.num_errors; i++)
    {
      printf("[%d] %s\n", validation_errors.errors[i].severity, validation_errors.errors[i].message);
    }
    printf("\n");
  }
  else
  {
    printf("No validation errors found.\n");
  }

  // Cleanup
  destroy_deck(&deck);
  destroy_context(ctx);

  return 0;
}
