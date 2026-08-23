/*
 * test_sections.c - Simple test program to verify section parsing
 */

#include <stdio.h>
#include "internals.h"

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
  errors_list_t errors = {0};
  parse_deck(ctx, &deck, &errors);

  // Display section information
  printf("=== DECK SECTION ANALYSIS ===\n");
  printf("Total cards: %d\n", deck.num_cards);
  printf("Number of sections: %d\n\n", deck.num_sections);

  for (int s = 0; s < deck.num_sections; s++)
  {
    section_t *sec = deck.sections[s];
    printf("Section %d:\n", s + 1);
    printf("  Global range: cards %d to %d\n", sec->global_start + 1, sec->global_end + 1);
    printf("  Ends with: %s\n", sec->ends_with_nx ? "NX" : "EN");
    printf("  Comment section: ");
    if (sec->comment_start >= 0)
      printf("cards %d to %d\n", sec->comment_start + 1, sec->comment_end + 1);
    else
      printf("none\n");
    printf("  Symbol section: ");
    if (sec->symbol_start >= 0)
      printf("cards %d to %d\n", sec->symbol_start + 1, sec->symbol_end + 1);
    else
      printf("none\n");
    printf("  Geometry section: ");
    if (sec->geometry_start >= 0)
      printf("cards %d to %d (GE at %d)\n", sec->geometry_start + 1, sec->geometry_end + 1, sec->geometry_end + 1);
    else
      printf("none\n");
    printf("  Control section: ");
    if (sec->control_start >= 0)
      printf("cards %d to %d\n", sec->control_start + 1, sec->control_end + 1);
    else
      printf("none\n");
    printf("\n");
  }

  // Show any errors
  if (errors.num_errors > 0)
  {
    printf("=== PARSING ERRORS ===\n");
    for (int i = 0; i < errors.num_errors; i++)
    {
      printf("%s\n", errors.errors[i].message);
    }
  }

  // Cleanup
  destroy_deck(&deck);
  destroy_context(ctx);

  return 0;
}
