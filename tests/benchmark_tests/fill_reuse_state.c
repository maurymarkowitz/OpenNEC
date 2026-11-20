/* Test-only driver: inspect the retained cache after a complete simulation.
 * Link against the same OpenMP libonec.a and backend libraries as onec.
 * Usage: fill_reuse_state deck.nec reuse|reject|ctx-on|ctx-off
 * ctx-on and ctx-off set the context's own fill_reuse (1 or -1), which must
 * win over OPENNEC_FILL_REUSE: run them with the environment saying the
 * opposite.
 */
#include "internals.h"
#include "reporting.h"

int main(int argc, char **argv)
{
  if (argc != 3 || (strcmp(argv[2], "reuse") && strcmp(argv[2], "reject") &&
                    strcmp(argv[2], "ctx-on") && strcmp(argv[2], "ctx-off")))
    return 2;
  context_t *ctx = create_context();
  int ctx_on = strcmp(argv[2], "ctx-on") == 0;
  int ctx_off = strcmp(argv[2], "ctx-off") == 0;
  if (ctx && (ctx_on || ctx_off))
    ctx->fill_reuse = ctx_on ? 1 : -1;
  FILE *input = fopen(argv[1], "r");
  FILE *output = tmpfile();
  if (!ctx || !input || !output) {
    if (input) fclose(input);
    if (output) fclose(output);
    if (ctx) destroy_context(ctx);
    return 2;
  }
  ctx->output_fp = output;
  ctx->error_fp = stderr;
  ctx->source_filename = argv[1];
  deck_t deck;
  init_deck(&deck);
  read_deck(ctx, &deck, input);
  fclose(input);
  errors_list_t errors = {0};
  parse_deck(ctx, &deck, &errors);
  initialize_symbol_table(&deck, &errors);
  update_deck_values(ctx, &deck);
  int rc = errors.num_errors ? -1 : process_deck_sequential(ctx, &deck);
  int reject = strcmp(argv[2], "reject") == 0;
  int ok = rc == 0 && ctx->fill_reuse_snapshot != NULL &&
           ctx->fill_reuse_skip_cached == reject;
  if (ctx_off)
    ok = rc == 0 && ctx->fill_reuse_snapshot == NULL && ctx->fill_reuse_class_ids == NULL;
  else if (ctx_on)
    ok = rc == 0 && ctx->fill_reuse_snapshot != NULL;
  else if (reject)
    ok = ok && ctx->fill_reuse_class_ids == NULL && ctx->fill_reuse_class_count == 0;
  else
    ok = ok && ctx->fill_reuse_class_ids != NULL &&
         ctx->fill_reuse_representatives != NULL && ctx->fill_reuse_class_count > 0;
  if (!ok)
    fprintf(stderr, "Unexpected reuse state: result=%d, skip=%d, classes=%zu\n",
            rc, ctx->fill_reuse_skip_cached, ctx->fill_reuse_class_count);
  for (int i = 0; i < errors.num_errors; ++i) free(errors.errors[i].message);
  free(errors.errors);
  destroy_deck(&deck);
  destroy_context(ctx);
  fclose(output);
  return ok ? 0 : 1;
}
