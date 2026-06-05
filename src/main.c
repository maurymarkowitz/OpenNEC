/**
 * @file main.c
 * @brief Entry point for the command-line version of OpenNEC.
 *
 * This program works with input.c and output.c: parsing command lines,
 * reading input decks, executing the commands, and emitting output files.
 * It also exposes the import/export helpers in *-support for converting
 * to/from other formats.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "internals.h"
#include "input.h"
#include "output.h"
#include "control.h"
#include "deck_validations.h"
#include "import-export/nec2-support.h"
#include "import-export/nec4-support.h"
#include "import-export/maa-support.h"
#include "import-export/yo-support.h"
#include "import-export/nc-support.h"

#ifndef _GETOPT_H
#include <getopt.h>
#endif

#if defined(ENABLE_THREADS)
#include <pthread.h>
#endif
#include <signal.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <limits.h>

// various switches for the command line arguments
static bool do_run_simulation = true;
static bool run_tests = false;
static bool run_greens = false;
static bool recursive = false;
static bool skip_large = false;
static int num_input_files = 1; /* for skip-large group behavior */
static char *input_file = "";
static char *output_file = "";
static char *error_file = "";
static char *greens_file = "";
static char *write_file = "";  /* -w / --write-file: convert deck to this format */
static int jobs = 1; // number of parallel jobs (-j)
static int output_format_choice = DEFAULT_OUTPUT_FORMAT;  /* Output format selection */
static int line_ending_choice = DEFAULT_LINE_ENDING;  /* 0=LF (Unix), 1=CRLF (Windows) */

/**
 * @brief Print the version string and terminate.
 *
 * This helper writes the constant VERSION_STRING followed by a newline and
 * then exits the process with status zero.
 */
static void print_version(void);
static void print_version(void)
{
  puts("onec " VERSION_STRING);
  exit(0);
}

/**
 * @brief Display command-line usage instructions.
 *
 * @param argv[] Program arguments; used solely to display the program name in
 *                the usage text.
 */
void print_usage(char *argv[])
{
  printf("Usage: %s [-hvntgr] [-i input_file] [-o output_file] [-e error_file] [-w file] <input_file...>\n", argv[0]);
  puts("Options:");
  puts("  -h, --help: print this description");
  puts("  -v, --version: print version info");
  puts("  -r, --recursive: recurse into subdirectories");
  puts("    With a bare directory (e.g. /models/) only .nec/.deck files are collected.");
  puts("    With a quoted glob pattern (e.g. '/models/*.yo') files matching that");
  puts("    extension are collected; the filter is inherited by all subdirectories.");
  puts("  --skip-large: with multiple files (or -r), skip decks with complexity T >= 1e11");
  puts("  -n, --no-run: don't run the simulation after parsing");
  puts("  -t, --test-deck: run various sanity tests");
  puts("  -i file, --input-file=file: read input file. this is not required if <input_file> is provided. if neither is provided, input is read from stdin");
  puts("  -o file, --output-file=file: write output to file. omitted -o writes to stdout (single file) or <file>.out (multiple files)");
  puts("  -e, --error-file: output errors to (path/)file, instead of stderr");
  puts("  -g, --greens[=file]: write a Green's function file; filename defaults to input path with .ngf extension");
  puts("  -j, --jobs N: process up to N files in parallel (default 1)");
  puts("  -w file, --write-file=file: write the deck to file in the format inferred from its extension");
  puts("    Supported: .nec/.deck (OpenNEC), .nec2 (NEC-2), .nec4 (NEC-4), .maa/.mma (MMANA-GAL), .yo/.ant/.yag (Yagi Optimizer), .nc (cocoaNEC)");
  puts("    Pass a bare extension (e.g. -w .maa) to convert multiple input files in place.");
  puts("  -f, --format: output format for .out file: 'nec2c' (modern, default on Unix) or 'original' (Fortran, default on Windows)");
#if defined(_WIN32) || defined(__MINGW32__)
  puts("  --line-ending: line ending style: 'crlf' (default on Windows) or 'lf' (Unix/macOS)");
#else
  puts("  --line-ending: line ending style: 'lf' (default on Unix/macOS) or 'crlf' (Windows)");
#endif
  puts("Multiple input files or folders can be specified; each file will generate a .out file.");
  puts("If no input_file is provided, input is read from stdin and output goes to stdout.");
  exit(0);
}

static struct option program_options[] =
    {
        {"help", no_argument, NULL, 'h'},
        {"version", no_argument, NULL, 'v'},
        {"no-run", no_argument, NULL, 'n'},
        {"test-deck", no_argument, NULL, 't'},
        {"recursive", no_argument, NULL, 'r'},
        {"input-file", required_argument, NULL, 'i'},
        {"output-file", required_argument, NULL, 'o'},
        {"error-file", required_argument, NULL, 'e'},
        {"greens", optional_argument, NULL, 'g'},
        {"jobs", required_argument, NULL, 'j'},
        {"write-file", required_argument, NULL, 'w'},
        {"skip-large", no_argument, NULL, 'S'},
        {"format", required_argument, NULL, 'f'},
        {"line-ending", required_argument, NULL, 'L'},
        {0, 0, 0, 0}};

/**
 * @brief Parse command-line options.
 *
 * Populates the global flags such as `recursive`, `run_tests`, and file
 * paths based on argv/argc.  This wraps getopt_long() logic used throughout
 * the program.
 *
 * @param argc Argument count from main().
 * @param argv Argument vector from main().
 */
void parse_options(int argc, char *argv[])
{
  int option_index = 0;

#if defined(_WIN32)
  /* Support the traditional Windows help switch `/?'` as a special case. */
  for (int i = 1; i < argc; ++i) {
    if (strcmp(argv[i], "/?") == 0) {
      print_usage(argv);
    }
  }
#endif
  /* int printed_help = false; */

  while (1)
  {
    // eat an option and exit if we're done
    /* portable short options: 'g' has an optional argument */
    int c = getopt_long(argc, argv, "hvntri:o:e:g::j:w:Sf:", program_options, &option_index); // should match the items above
    if (c == -1)
      break;

    switch (c)
    {
    case 0:
      // flag-setting options return 0 - these are t and n
      if (program_options[option_index].flag != 0)
        break;

    case 'h':
      print_usage(argv);
      /* printed_help = true; */
      break;
    /* Note: `/?` (Windows) is handled above; do not treat `?` as a unix short option. */

    case 'v':
      print_version();
      /* printed_help =  true; */
      break;

    case 'r':
      recursive = true;
      break;

    case 'i':
      input_file = optarg;
      break;

    case 'o':
      output_file = optarg;
      break;

    case 'e':
      error_file = optarg;
      break;

    case 'n':
      do_run_simulation = false;
      break;

    case 't':
      run_tests = true;
      break;

    case 'g':
      run_greens = true;
      greens_file = optarg ? optarg : "";
      break;
    case 'j':
      jobs = atoi(optarg);
      if (jobs < 1)
        jobs = 1;
      break;
    case 'S':
      skip_large = true;
      break;

    case 'w':
      write_file = optarg;
      break;

    case 'f':
      if (strcasecmp(optarg, "nec2c") == 0) {
        output_format_choice = OUTPUT_FORMAT_NEC2C;
      } else if (strcasecmp(optarg, "original") == 0) {
        output_format_choice = OUTPUT_FORMAT_ORIGINAL;
      } else {
        fprintf(stderr, "onec: invalid format '%s'. Use 'nec2c' or 'original'.\n", optarg);
        exit(1);
      }
      break;

    case 'L':
      if (strcasecmp(optarg, "lf") == 0) {
        line_ending_choice = 0;  /* Unix LF */
      } else if (strcasecmp(optarg, "crlf") == 0) {
        line_ending_choice = 1;  /* Windows CRLF */
      } else {
        fprintf(stderr, "onec: invalid line-ending '%s'. Use 'lf' or 'crlf'.\n", optarg);
        exit(1);
      }
      break;

    default:
      abort();
    }
  } // while

  // now see if there's a filename at the end without an option
  // flag, if so it overrides -i if it was supplied
  if (optind < argc)
    input_file = argv[optind];

  /* Treat a bare question-mark as a request for usage (e.g. `onec ?`) */
  if (input_file && strcmp(input_file, "?") == 0) {
    print_usage(argv);
  }

  // if no input file, we'll use stdin
}

/* ---------- file-type detection ----------------------------------- */
typedef enum {
  FILETYPE_NEC,          /* .nec  .deck  — native NEC/OpenNEC deck    */
  FILETYPE_NEC2,         /* .nec2        — NEC-2 stripped output      */
  FILETYPE_NEC4,         /* .nec4        — NEC-4 output (stub)        */
  FILETYPE_YO,           /* .yo   .ant   .yag — Yagi Optimizer        */
  FILETYPE_MAA,          /* .maa  .mma   — MMANA-GAL                  */
  FILETYPE_NC,           /* .nc  — cocoaNEC script                    */
  FILETYPE_UNSUPPORTED,  /* known format, importer not yet available  */
  FILETYPE_UNKNOWN,      /* unrecognised extension — try NEC parsing  */
  FILETYPE_STDIN,        /* empty / "-" — stdin, no extension info    */
} filetype_t;

/* Maps extension to format.  Case-insensitive. */
static filetype_t classify_by_extension(const char *filename)
{
  if (!filename || filename[0] == '\0' || strcmp(filename, "-") == 0)
    return FILETYPE_STDIN;

  const char *ext = strrchr(filename, '.');
  const char *slash = strrchr(filename, '/');
  if (!ext || (slash && ext < slash))
    return FILETYPE_UNKNOWN;

  if (strcasecmp(ext, ".nec")  == 0) return FILETYPE_NEC;
  if (strcasecmp(ext, ".deck") == 0) return FILETYPE_NEC;

  if (strcasecmp(ext, ".nec2") == 0) return FILETYPE_NEC2;
  if (strcasecmp(ext, ".nec4") == 0) return FILETYPE_NEC4;

  if (strcasecmp(ext, ".yo")  == 0) return FILETYPE_YO;
  if (strcasecmp(ext, ".ant") == 0) return FILETYPE_YO;
  if (strcasecmp(ext, ".yag") == 0) return FILETYPE_YO;

  if (strcasecmp(ext, ".maa") == 0) return FILETYPE_MAA;
  if (strcasecmp(ext, ".mma") == 0) return FILETYPE_MAA;

  if (strcasecmp(ext, ".4nec") == 0) return FILETYPE_NEC;         /* 4nec2 project — identical to NEC deck */
  if (strcasecmp(ext, ".nc")   == 0) return FILETYPE_NC;

  if (strcasecmp(ext, ".ez")   == 0) return FILETYPE_UNSUPPORTED; /* EZNEC         */
  if (strcasecmp(ext, ".ezn")  == 0) return FILETYPE_UNSUPPORTED; /* EZNEC newer   */
  if (strcasecmp(ext, ".nwp")  == 0) return FILETYPE_UNSUPPORTED; /* NEC-Win Plus  */
  if (strcasecmp(ext, ".nwz")  == 0) return FILETYPE_UNSUPPORTED; /* NEC-Win Zip   */
  if (strcasecmp(ext, ".aci")  == 0) return FILETYPE_UNSUPPORTED; /* AntSolver     */
  if (strcasecmp(ext, ".mmae") == 0) return FILETYPE_UNSUPPORTED; /* MMANA-EZ      */

  return FILETYPE_UNKNOWN;
}

/* Gets the relative path to a file, so we don't print the full path during logging. */
static char **g_input_roots = NULL;
static int g_input_root_count = 0;
static void get_relative_input_path(const char *input, char *out, size_t outsz)
{
  if (!input || !*input) {
    if (outsz > 0) out[0] = '\0';
    return;
  }

  const char *best = input;
  size_t best_len = strlen(input);

  for (int i = 0; i < g_input_root_count; ++i) {
    const char *root = g_input_roots[i];
    size_t root_len = strlen(root);
    if (root_len == 0 || root_len > strlen(input))
      continue;

    if (strncmp(input, root, root_len) == 0) {
      const char *rel = input + root_len;
      if (*rel == '/' || *rel == '\\')
        rel++;
      size_t rel_len = strlen(rel);
      if (rel_len < best_len) {
        best = rel;
        best_len = rel_len;
      }
    }
  }

  if (best_len >= outsz) {
    best_len = outsz > 0 ? outsz - 1 : 0;
  }
  if (outsz > 0) {
    memcpy(out, best, best_len);
    out[best_len] = '\0';
  }
}

/* Human-readable format name for error messages. */
static const char *filetype_name(filetype_t ft, const char *filename)
{
  switch (ft) {
    case FILETYPE_NEC:   return "NEC deck";
    case FILETYPE_NEC2:  return "NEC-2";
    case FILETYPE_NEC4:  return "NEC-4";
    case FILETYPE_YO:    return "Yagi Optimizer";
    case FILETYPE_MAA:   return "MMANA-GAL";
    case FILETYPE_STDIN: return "stdin";
    default: break;
  }
  if (filename) {
    const char *ext = strrchr(filename, '.');
    if (ext) {
      if (strcasecmp(ext, ".ez")  == 0 || strcasecmp(ext, ".ezn") == 0) return "EZNEC";
      if (strcasecmp(ext, ".nwp") == 0 || strcasecmp(ext, ".nwz") == 0) return "NEC-Win";
      if (strcasecmp(ext, ".aci") == 0)   return "AntSolver";
      if (strcasecmp(ext, ".4nec") == 0)  return "4nec2 project";
      if (strcasecmp(ext, ".mmae") == 0)  return "MMANA-EZ";
    }
  }
  return "unknown format";
}

/* True for extensions that should be picked up during directory scanning.
 * Only native NEC formats are collected automatically; converter formats
 * (YO, MAA, etc.) must be passed as explicit filenames on the command line. */
static int has_nec_extension(const char *filename)
{
  filetype_t ft = classify_by_extension(filename);
  return ft == FILETYPE_NEC;
}

/* Parse a quoted glob argument such as "/path/to/STAR.yo".
 * The basename must start with '*' followed by an extension.
 * Returns 1 and fills dir_buf / ext_buf on success, 0 otherwise. */
static int parse_glob_arg(const char *arg,
                          char *dir_buf, size_t dir_sz,
                          char *ext_buf, size_t ext_sz)
{
  const char *last_slash = strrchr(arg, '/');
  if (!last_slash) return 0;
  const char *basename = last_slash + 1;
  if (basename[0] != '*') return 0;  /* not a glob */
  const char *dot = strchr(basename, '.');
  if (!dot || dot[1] == '\0') return 0; /* no extension */

  /* directory part */
  size_t dir_len = (size_t)(last_slash - arg);
  if (dir_len == 0) {
    strncpy(dir_buf, "/", dir_sz - 1);
  } else {
    size_t copy = dir_len < dir_sz - 1 ? dir_len : dir_sz - 1;
    strncpy(dir_buf, arg, copy);
    dir_buf[copy] = '\0';
  }

  /* extension part (e.g. ".yo") */
  strncpy(ext_buf, dot, ext_sz - 1);
  ext_buf[ext_sz - 1] = '\0';
  return 1;
}

/**
 * @brief Determine whether a directory entry should be processed.
 *
 * The rule depends on the current extension filter:
 *   - `NULL` or empty string: only NEC-style extensions are accepted.
 *   - otherwise, the entry must have the given extension (case-insensitive).
 *
 * @param entry_name filename from readdir()
 * @param ext_filter extension to match, or NULL/"" for NEC-only
 * @return 1 if the file should be collected, 0 otherwise
 */
static int file_matches_ext_filter(const char *entry_name, const char *ext_filter)
{
  if (!ext_filter || ext_filter[0] == '\0')
    return has_nec_extension(entry_name);
  const char *ext = strrchr(entry_name, '.');
  if (!ext) return 0;
  return strcasecmp(ext, ext_filter) == 0;
}

/**
 * @brief Copy a stream while converting Unix LF line endings to CRLF.
 *
 * When `src` contains LF line endings, this helper writes CRLF pairs to
 * `dst`. If a CR character is already present before a newline, it is left
 * intact so that existing CRLF sequences are preserved.
 *
 * @param src Source stream opened for reading.
 * @param dst Destination stream opened for writing.
 * @return 0 on success, -1 on write failure.
 */
static int copy_stream_crlf(FILE *src, FILE *dst)
{
  int ch;
  int prev = 0;

  while ((ch = fgetc(src)) != EOF)
  {
    if (ch == '\n')
    {
      if (prev != '\r')
      {
        if (fputc('\r', dst) == EOF)
          return -1;
      }
      if (fputc('\n', dst) == EOF)
        return -1;
    }
    else
    {
      if (fputc(ch, dst) == EOF)
        return -1;
    }
    prev = ch;
  }

  return 0;
}

/**
 * @brief Compute destination filename for a converted deck.
 *
 * If `wf` is a bare extension (".nec", ".maa", etc.) the output file
 * will be placed alongside `input` with that extension.  Otherwise `wf` is
 * treated as an explicit filename pattern.
 *
 * @param input  Input deck path
 * @param wf     Write-file argument from command line
 * @param buf    Output buffer to receive resolved filename
 * @param bufsz  Size of `buf`
 */
static void resolve_write_filename(const char *input, const char *wf,
                                   char *buf, size_t bufsz);

/**
 * @brief Run the full OpenNEC pipeline on one input deck.
 *
 * This function creates a context, reads the deck, optionally runs the
 * simulation, and writes any output.  Errors are reported to `error_fp`.
 *
 * @param input_filename  Path of the input deck
 * @param output_filename Path for the generated output (may be NULL)
 * @param error_fp        FILE* where warnings/errors are printed
 * @return 0 on success, -1 on fatal error
 */
static int process_single_file(const char *input_filename, const char *output_filename, FILE *error_fp)
{
  context_t *ctx = create_context();
  if (ctx == NULL)
  {
    fprintf(error_fp, "onec: Failed to allocate NEC context.\n");
    return -1;
  }

  // main variables
  deck_t deck; init_deck(&deck);
  errors_list_t import_errors = {0};
  errors_list_t test_errors = {0};

  FILE *input_fp = NULL;
  FILE *output_fp = NULL;
  FILE *temp_output_fp = NULL;
  bool using_temp_output = false;

  ctx->error_fp = error_fp;
  ctx->source_filename = (strlen(input_filename) > 0) ? (char *)input_filename : NULL;
  ctx->output_format = output_format_choice;
  ctx->line_ending = line_ending_choice;  /* Use command-line choice, default is 1 (Windows CRLF) */

  // open input file or use stdin
  // empty string or "-" both mean stdin
  if (strlen(input_filename) > 0 && strcmp(input_filename, "-") != 0)
  {
    if ((input_fp = fopen(input_filename, "r")) == NULL)
    {
      char mesg[88] = "onec: ";
      strcat(mesg, input_filename);
      perror(mesg);
      destroy_context(ctx);
      return -1;
    }
    ctx->input_fp = input_fp;
  }
  else
  {
    input_fp = stdin;
    ctx->input_fp = stdin;
  }

  // open output file or use stdout
  // empty string or "-" both mean stdout
  if (strlen(output_filename) > 0 && strcmp(output_filename, "-") != 0)
  {
    if (ctx->line_ending == LINE_ENDING_CRLF)
    {
      temp_output_fp = tmpfile();
      if (temp_output_fp != NULL)
      {
        output_fp = temp_output_fp;
        using_temp_output = true;
      }
    }

    if (output_fp == NULL)
    {
      if ((output_fp = fopen(output_filename, "w")) == NULL)
      {
        char mesg[88] = "onec: ";
        strcat(mesg, output_filename);
        perror(mesg);
        if (input_fp != stdin)
          fclose(input_fp);
        destroy_context(ctx);
        return -1;
      }
    }

    ctx->output_fp = output_fp;
  }
  else
  {
    if (ctx->line_ending == LINE_ENDING_CRLF)
    {
      temp_output_fp = tmpfile();
      if (temp_output_fp != NULL)
      {
        output_fp = temp_output_fp;
        using_temp_output = true;
      }
      else
      {
        output_fp = stdout;
      }
    }
    else
    {
      output_fp = stdout;
    }
    ctx->output_fp = output_fp;
  }

  // open greens output file if requested
  if (run_greens)
  {
    char ngfpath[512];
    const char *path = NULL;
    if (strlen(greens_file) > 0)
    {
      path = greens_file;
    }
    else if (strlen(input_filename) > 0)
    {
      // derive from input filename by replacing extension with .ngf
      strncpy(ngfpath, input_filename, sizeof(ngfpath) - 1);
      ngfpath[sizeof(ngfpath) - 1] = '\0';
      char *dot = strrchr(ngfpath, '.');
      char *slash = strrchr(ngfpath, '/');
      if (dot != NULL && (slash == NULL || dot > slash))
      {
        *dot = '\0';
      }
      strncat(ngfpath, ".ngf", sizeof(ngfpath) - strlen(ngfpath) - 1);
      path = ngfpath;
    }
    else
    {
      path = "greens.ngf";
    }
    ctx->green_fp = fopen(path, "wb");
    if (!ctx->green_fp)
    {
      report(ctx, ONEC_SEV_WARNING, "Could not open greens file '%s' for writing; skipping.", path);
    }
  }

  // detect format from extension and read/convert into the deck
  filetype_t ftype = classify_by_extension(input_filename);

  if (ftype == FILETYPE_UNSUPPORTED) {
    const char *fmt = filetype_name(ftype, input_filename);
    fprintf(error_fp, "onec: '%s' uses the %s format which is not yet supported for import.\n",
            input_filename, fmt);
    if (input_fp != stdin) fclose(input_fp);
    if (output_fp != stdout) fclose(output_fp);
    destroy_context(ctx);
    return -1;
  }

  if (ftype == FILETYPE_YO) {
    if (read_deck_yo(&deck, input_fp) != 0) {
      fprintf(error_fp, "onec: failed to parse Yagi Optimizer file '%s'\n", input_filename);
      if (input_fp != stdin) fclose(input_fp);
      if (output_fp != stdout) fclose(output_fp);
      destroy_context(ctx);
      return -1;
    }
  } else if (ftype == FILETYPE_MAA) {
    if (read_deck_maa(&deck, input_fp) != 0) {
      fprintf(error_fp, "onec: failed to parse MMANA-GAL file '%s'\n", input_filename);
      if (input_fp != stdin) fclose(input_fp);
      if (output_fp != stdout) fclose(output_fp);
      destroy_context(ctx);
      return -1;
    }
  } else if (ftype == FILETYPE_NC) {
    if (read_deck_nc(ctx, &deck, input_fp, &import_errors) != 0) {
      add_error(ctx, &import_errors, strdup("onec: failed to parse cocoaNEC .nc file"), FATAL);
      if (input_fp != stdin) fclose(input_fp);
      if (output_fp != stdout) fclose(output_fp);
      destroy_context(ctx);
      return -1;
    }
  } else {
    /* NEC, STDIN, or UNKNOWN extension — attempt native NEC parsing */
    read_deck(ctx, &deck, input_fp);
  }

  // and then parse what we read into the card
  parse_deck(ctx, &deck, &import_errors);

  // Initialize symbol table: collect all SY symbols, add defaults (pi, c),
  // and evaluate symbols in comment section for initial values
  initialize_symbol_table(&deck, &import_errors);

  // Evaluate all formulas in the deck
  update_deck_values(ctx, &deck);

  /* --skip-large: when group mode is active (multiple files or -r), skip files whose
     estimated complexity is too high (T >= 1e11). */
  if (skip_large && (recursive || num_input_files > 1)) {
    double T = estimate_time(ctx, &deck);
    if (T >= 1.0e11) {
      report(ctx, ONEC_SEV_WARNING,
                 "Skipping %s: complexity T=%.2e >= 1e11 (skip-large enabled)",
                 input_filename && input_filename[0] ? input_filename : "stdin", T);
      if (input_fp != stdin) fclose(input_fp);
      if (output_fp != stdout) fclose(output_fp);
      destroy_deck(&deck);
      destroy_context(ctx);
      return 0;
    }
  }

  /* -w / --write-file: export the deck in the requested format.
     This is independent of simulation; combine with -n for a pure conversion.
     Format is inferred from the output file's extension. */
  if (strlen(write_file) > 0) {
    char wpath[512];
    resolve_write_filename(input_filename, write_file, wpath, sizeof(wpath));
    /* determine format from the resolved path; fall back to write_file extension
       so that "-w .maa" with stdin still gives us a format hint */
    const char *ext_src = wpath[0] ? wpath : write_file;
    filetype_t wftype = classify_by_extension(ext_src);
    if (wftype == FILETYPE_UNSUPPORTED || wftype == FILETYPE_UNKNOWN || wftype == FILETYPE_STDIN) {
      fprintf(error_fp, "onec: -w: unrecognised output format for '%s' "
              "(supported: .nec .deck .nec2 .nec4 .maa .mma .yo .ant .yag)\n", write_file);
    } else {
      FILE *wfp = (wpath[0] && strcmp(wpath, "-") != 0) ? fopen(wpath, "w") : stdout;
      if (!wfp) {
        fprintf(error_fp, "onec: cannot open '%s' for writing: %s\n",
                wpath, strerror(errno));
      } else {
        if (wftype == FILETYPE_NEC) {
          write_deck_onec(ctx, &deck, wfp);
        } else if (wftype == FILETYPE_NEC2) {
          /* use the simplified NEC-2 writer introduced in nec2-support */
          write_deck_nec2(&deck, wfp);
        } else if (wftype == FILETYPE_NEC4) {
          /* use simplified NEC-4 writer */
          write_deck_nec4(&deck, wfp);
        } else if (wftype == FILETYPE_MAA) {
          write_deck_maa(&deck, wfp);
        } else if (wftype == FILETYPE_YO) {
          write_deck_yo(&deck, wfp);
        }
        if (wfp != stdout) fclose(wfp);
      }
    }
  }

  // TESTING: print any file errors
  if (import_errors.num_errors > 0)
  {
    const char *display_name = strlen(input_filename) > 0 ? input_filename : "stdin";
    report(ctx, ONEC_SEV_INFO, "=== Found %d Import Errors for %s ===", import_errors.num_errors, display_name);
  }

  // run basic sanity checks on the structure
  if (run_tests)
  {
    test_deck_structure(ctx, &deck, &test_errors);
    test_duplicate_tags(ctx, &deck, &test_errors);
    test_card_inputs(ctx, &deck, &test_errors);
    test_field_separators(ctx, &deck, &test_errors);
  }
  // TESTING: print any structure errors
  if (test_errors.num_errors > 0)
  {
    const char *display_name = strlen(input_filename) > 0 ? input_filename : "stdin";
    report(ctx, ONEC_SEV_INFO, "=== Found %d Structural Errors for %s ===", test_errors.num_errors, display_name);
  }

  // run it if we've been asked to
  int sim_result = -1;
  if (do_run_simulation)
  {
    // Run complete simulation with batch processing
    sim_result = run_simulation(ctx, &deck);
    if (ctx->errors.num_errors > 0)
    {
      report(ctx, ONEC_SEV_INFO, "=== Found %d Simulation Errors ===", ctx->errors.num_errors);
    }
  }

  // write out the results
  // According to NEC-2 standard, the output file should contain:
  // - Always: geometry preamble (header, structure, segments, patches)
  // - Optional: frequency-specific data (only if output request card present)
  // Exception: if -n (--no-run) was used, don't write any output at all
  if (do_run_simulation) {
    if (ctx->xt_terminated) {
      // XT card halted execution before any output request — expected, not an error
      report(ctx, ONEC_SEV_WARNING, "Simulation halted by XT card; no output generated.");
    } else {
      // Write geometry preamble always (unless already written inside frequency loop)
      if (!ctx->freq_step_output_written) {
        write_nec_preamble(ctx, &deck, output_fp);
      }
      
      // Check for simulation errors before writing frequency-step data
      if (ctx->errors.num_errors > 0 || sim_result != 0) {
        // Write error section instead of frequency-step output
        fprintf(output_fp, "\n\n");
        fprintf(output_fp, "                    *** SIMULATION ERRORS ***\n\n");
        if (sim_result != 0) {
          fprintf(output_fp, "Simulation failed with error code %d\n", sim_result);
        }
        if (ctx->errors.num_errors > 0) {
          fprintf(output_fp, "Found %d error(s) during simulation\n\n", ctx->errors.num_errors);
          
          // Print error details
          int max_display = (ctx->errors.num_errors < 10) ? ctx->errors.num_errors : 10;
          for (int i = 0; i < max_display; i++) {
            fprintf(output_fp, "  Error %d: %s\n", i + 1, ctx->errors.errors[i].message);
          }
          
          if (ctx->errors.num_errors > 10) {
            int remaining = ctx->errors.num_errors - 10;
            fprintf(output_fp, "  ... there are another %d error%s.\n", 
                    remaining, (remaining == 1) ? "" : "s");
          }
          fprintf(output_fp, "\n");
        }
      } else if (!ctx->has_output_request_cards) {
        fprintf(output_fp, "\n\n");
        fprintf(output_fp, "                    *** NO REPORT CARDS FOUND ***\n\n");
        fprintf(output_fp, "No RP/NE/NH/XQ/WG cards were found in the deck; no frequency-step output was generated.\n\n");
      } else {
        // Write frequency-specific data only if output request card was present
        // and hasn't been written already inside the frequency loop
        if (ctx->frequency_loop_ran && !ctx->freq_step_output_written) {
          write_frequency_step_output(output_fp, ctx);
        }
      }
      
      // Write footer (which emits EN/NX cards and runtime summary)
      write_footer(output_fp, ctx, &deck);
    }
  }

  // close greens file if open
  if (ctx->green_fp)
  {
    fclose(ctx->green_fp);
    ctx->green_fp = NULL;
  }

  if (using_temp_output)
  {
    int conv_error = 0;
    fflush(output_fp);
    rewind(output_fp);

    if (strlen(output_filename) > 0 && strcmp(output_filename, "-") != 0)
    {
      FILE *final_fp = fopen(output_filename, "wb");
      if (final_fp == NULL)
      {
        char mesg[88] = "onec: ";
        strcat(mesg, output_filename);
        perror(mesg);
        conv_error = 1;
      }
      else
      {
        if (copy_stream_crlf(output_fp, final_fp) != 0)
          conv_error = 1;
        fclose(final_fp);
      }
    }
    else
    {
      if (copy_stream_crlf(output_fp, stdout) != 0)
        conv_error = 1;
      fflush(stdout);
    }

    fclose(output_fp);
    output_fp = stdout;

    if (conv_error)
    {
      if (input_fp != stdin)
        fclose(input_fp);
      destroy_deck(&deck);
      destroy_context(ctx);
      return -1;
    }
  }

  destroy_deck(&deck);
  if (input_fp != stdin)
    fclose(input_fp);
  if (output_fp != stdout)
    fclose(output_fp);
  destroy_context(ctx);

  return 0;
}

/******************************************************************************
 * generate_output_filename()
 *
 * Generate output filename by replacing extension with .out
 *
 */
static void generate_output_filename(const char *input_filename, char *output_filename, size_t size)
{
  strncpy(output_filename, input_filename, size - 1);
  output_filename[size - 1] = '\0';

  // Find the last dot in the filename
  char *dot = strrchr(output_filename, '.');
  char *slash = strrchr(output_filename, '/');

  // Only use the dot if it's after the last slash (part of filename, not directory)
  if (dot != NULL && (slash == NULL || dot > slash))
  {
    *dot = '\0';
  }

  // Add .out extension
  strncat(output_filename, ".out", size - strlen(output_filename) - 1);
}

typedef struct
{
  const char *input;
  char output[512];
  int index;
  int status;    // 0 ok, -1 failure
  char *log_buf; // captured stderr
  size_t log_size;
} task_t;

typedef struct
{
  task_t *tasks;
  int task_count;
  int next_index;
#if defined(ENABLE_THREADS)
  pthread_mutex_t lock;
#endif
} work_queue_t;

#if defined(ENABLE_THREADS)
static void *worker_thread(void *arg)
{
  work_queue_t *q = (work_queue_t *)arg;
  while (1)
  {
    int idx = -1;
    pthread_mutex_lock(&q->lock);
    if (q->next_index < q->task_count)
    {
      idx = q->next_index++;
    }
    pthread_mutex_unlock(&q->lock);

    if (idx == -1)
      break;

    task_t *t = &q->tasks[idx];

    // Print progress for parallel processing
    {
      char rel_input[PATH_MAX];
      get_relative_input_path(t->input, rel_input, sizeof(rel_input));
      fprintf(stderr, "Processing %d of %d: %s...\n", idx + 1, q->task_count, rel_input);
      fflush(stderr);
    }

    // capture logs using open_memstream
    char *buf = NULL;
    size_t sz = 0;
    FILE *memfp = open_memstream(&buf, &sz);
    if (!memfp)
    {
      // fallback: use stderr (may interleave)
      {
        char rel_input[PATH_MAX];
        get_relative_input_path(t->input, rel_input, sizeof(rel_input));
        fprintf(stderr, "Processing %d of %d: %s...\n", idx + 1, q->task_count, rel_input);
      }
      fflush(stderr);
      t->status = process_single_file(t->input, t->output, stderr);
      t->log_buf = NULL;
      t->log_size = 0;
      continue;
    }

    // Emit the processing header into the captured stream for consistency
    t->status = process_single_file(t->input, t->output, memfp);
    fflush(memfp);
    fclose(memfp); // sets buf/sz
    t->log_buf = buf;
    t->log_size = sz;
  }
  return NULL;
}
#endif

static void add_to_string_list(char ***list, int *count, int *cap, const char *str)
{
  if (*list == NULL || *count >= *cap)
  {
    if (*cap == 0) {
      *cap = 16;
    } else {
      *cap *= 2;
    }
    char **new_list = realloc(*list, *cap * sizeof(char *));
    if (!new_list)
      abort();
    *list = new_list;
  }
  (*list)[(*count)++] = strdup(str);
}

/* Compute the output path for a given input file and -o option value.
 * Rules:
 *   out_opt == "-"    → stdout (pass "-" through)
 *   out_opt non-empty  → use as-is
 *   out_opt empty + real input file → generate <input>.out
 *   out_opt empty + stdin input ("")  → stdout ("")
 */
static void resolve_output(const char *input, const char *out_opt,
                           char *buf, size_t bufsz)
{
  if (strlen(out_opt) > 0)
  {
    strncpy(buf, out_opt, bufsz - 1);
    buf[bufsz - 1] = '\0';
  }
  else if (strlen(input) > 0 && strcmp(input, "-") != 0)
  {
    generate_output_filename(input, buf, bufsz);
  }
  else
  {
    buf[0] = '\0'; // stdin input → stdout
  }
}

/* Resolve the -w output path.
 * A "bare extension" is an argument that starts with '.' and contains no '/';
 * e.g. "-w .maa".  In that case the output path is the input filename with its
 * extension replaced by the given one (suitable for multi-file batch use).
 * Any other argument is used literally as a full output path.
 * Empty write_file causes an empty result (no write).
 */
static void resolve_write_filename(const char *input, const char *wf,
                                   char *buf, size_t bufsz)
{
  buf[0] = '\0';
  if (!wf || wf[0] == '\0') return;
  /* bare extension: starts with '.' and no '/' anywhere */
  if (wf[0] == '.' && strchr(wf, '/') == NULL) {
    /* derive from input by replacing extension */
    if (strlen(input) > 0 && strcmp(input, "-") != 0) {
      strncpy(buf, input, bufsz - 1);
      buf[bufsz - 1] = '\0';
      char *dot  = strrchr(buf, '.');
      char *slash = strrchr(buf, '/');
      if (dot && (!slash || dot > slash))
        *dot = '\0';
      strncat(buf, wf, bufsz - strlen(buf) - 1);
    }
    /* stdin + bare extension → leave empty (write to stdout) */
  } else {
    strncpy(buf, wf, bufsz - 1);
    buf[bufsz - 1] = '\0';
  }
}

/*-------------------------------------------------------------------*/
/**
 * @brief Command-line entry point for the onec executable.
 *
 * Parses options, collects input files, and dispatches them to
 * process_single_file().  Handles error/log file opening and
 * potentially runs regression or other special modes.
 */
int main(int argc, char **argv)
{
  FILE *error_fp = NULL;

  // process the command line options
  parse_options(argc, argv);

  /* On Windows, if invoked with no args, read two lines from stdin
   * (input and output filenames) to support 4nec2-style callers.
   * Print prompts to match the behavior of the original nec2d.exe. */
#if defined(_WIN32)
  if (argc == 1) {
    char inbuf[4096];
    char outbuf[4096];
    printf(" ENTER NAME OF INPUT FILE >\n");
    fflush(stdout);
    if (fgets(inbuf, sizeof inbuf, stdin) != NULL) {
      size_t l = strlen(inbuf);
      while (l > 0 && (inbuf[l-1] == '\n' || inbuf[l-1] == '\r')) { inbuf[--l] = '\0'; }
      if (l > 0) {
        input_file = strdup(inbuf);
      }
      printf(" ENTER NAME OF OUTPUT FILE >\n");
      fflush(stdout);
      if (fgets(outbuf, sizeof outbuf, stdin) != NULL) {
        size_t m = strlen(outbuf);
        while (m > 0 && (outbuf[m-1] == '\n' || outbuf[m-1] == '\r')) { outbuf[--m] = '\0'; }
        if (m > 0) {
          output_file = strdup(outbuf);
        }
      }
    }
  }
#endif

  // open the error file if it was provided, otherwise stderr
  if (strlen(error_file) > 0)
  {
    if ((error_fp = fopen(error_file, "w")) == NULL)
    {
      char mesg[128] = "onec: ";
      strcat(mesg, error_file);
      perror(mesg);
      exit(EXIT_FAILURE);
    }
  }
  else
  {
    error_fp = stderr;
  }

  // Collect input files and folders
  char **file_list = NULL;
  int num_files = 0;
  int file_cap = 4096;
  int failed_count = 0;

  // Track root paths supplied by the user for relative output normalization.
  char **input_roots = NULL;
  int input_root_count = 0;
  int input_root_cap = 64;

  if (optind >= argc)
  {
    // If -i was given, treat it like a single positional file argument
    if (strlen(input_file) > 0)
    {
      char out[512];
      resolve_output(input_file, output_file, out, sizeof(out));
      if (process_single_file(input_file, out, error_fp) != 0)
      {
        if (error_fp != stderr)
          fclose(error_fp);
        return EXIT_FAILURE;
      }
    }
    else if (!isatty(STDIN_FILENO))
    {
      // Stdin is redirected, process it
      char out[512];
      resolve_output("", output_file, out, sizeof(out));
      if (process_single_file("", out, error_fp) != 0)
      {
        fprintf(error_fp, "Error processing stdin\n");
        if (error_fp != stderr)
          fclose(error_fp);
        return EXIT_FAILURE;
      }
    }
    else
    {
      // No input files specified and stdin not redirected
      fprintf(error_fp, "onec: missing file operand\nTry 'onec --help' for more information.\n");
      if (error_fp != stderr)
        fclose(error_fp);
      return EXIT_FAILURE;
    }
  }
  else
  {
    // Collect all files, including from directories
    char **dir_queue = NULL;
    int dir_count = 0;
    int dir_cap = 4096;
    int dir_head = 0;
    /* Parallel to dir_queue: per-directory extension filter.
       "" = NEC-only (default); ".yo" etc = that specific extension. */
    char **dir_ext_queue = NULL;
    int dir_ext_count = 0;
    int dir_ext_cap = 4096;

    /* helper to add a directory+filter pair to both queues */
    #define ADD_DIR(path, ext) do { \
      add_to_string_list(&dir_queue,     &dir_count,     &dir_cap,     (path)); \
      add_to_string_list(&dir_ext_queue, &dir_ext_count, &dir_ext_cap, (ext));  \
    } while(0)

    // Initial files/dirs from command line
    for (int i = optind; i < argc; i++)
    {
      struct stat st;
      if (stat(argv[i], &st) == 0)
      {
        if (S_ISDIR(st.st_mode))
        {
          ADD_DIR(argv[i], ""); /* bare directory → NEC-only filter */
          add_to_string_list(&input_roots, &input_root_count, &input_root_cap, argv[i]);
        }
        else
        {
          add_to_string_list(&file_list, &num_files, &file_cap, argv[i]);
          /* Also support file-based partial paths relative to file parent */
          char parent_dir[PATH_MAX];
          strncpy(parent_dir, argv[i], sizeof(parent_dir)-1);
          parent_dir[sizeof(parent_dir)-1] = '\0';
          char *slash = strrchr(parent_dir, '/');
          if (slash) {
            *slash = '\0';
            add_to_string_list(&input_roots, &input_root_count, &input_root_cap, parent_dir);
          }
        }
      }
      else
      {
        /* stat failed — check whether it's a quoted glob like "/path/STAR.yo" */
        char glob_dir[1024], glob_ext[32];
        if (parse_glob_arg(argv[i], glob_dir, sizeof(glob_dir),
                                    glob_ext, sizeof(glob_ext)))
        {
          struct stat dst;
          if (stat(glob_dir, &dst) == 0 && S_ISDIR(dst.st_mode))
          {
            ADD_DIR(glob_dir, glob_ext); /* use explicit extension filter */
          }
          else
          {
            fprintf(error_fp, "Warning: cannot access directory '%s' from pattern '%s': %s\n",
                    glob_dir, argv[i], strerror(errno));
          }
        }
        else
        {
          fprintf(error_fp, "Warning: cannot access '%s': %s\n", argv[i], strerror(errno));
        }
      }
    }

    // Validate option/argument combinations now that we know what was given
    if (strlen(output_file) > 0)
    {
      // Explicit -o <path> is only meaningful for a single input file
      if (dir_count > 0)
      {
        fprintf(error_fp, "onec: '-o' cannot be used with a directory argument (multiple output files would be produced)\n");
        fprintf(error_fp, "Try 'onec --help' for more information.\n");
        if (error_fp != stderr)
          fclose(error_fp);
        return EXIT_FAILURE;
      }
      if (num_files > 1)
      {
        fprintf(error_fp, "onec: '-o' cannot be used with multiple input files\n");
        fprintf(error_fp, "Try 'onec --help' for more information.\n");
        if (error_fp != stderr)
          fclose(error_fp);
        return EXIT_FAILURE;
      }
    }

    /* -w with a full path (not a bare extension) is only valid for a single file */
    if (strlen(write_file) > 0 &&
        !(write_file[0] == '.' && strchr(write_file, '/') == NULL))
    {
      if (dir_count > 0 || num_files > 1)
      {
        fprintf(error_fp, "onec: '-w' with a full path cannot be used with multiple input files\n");
        fprintf(error_fp, "      Use a bare extension (e.g. -w .maa) to convert many files.\n");
        if (error_fp != stderr)
          fclose(error_fp);
        return EXIT_FAILURE;
      }
    }

    // BFS for directories
    while (dir_head < dir_count)
    {
      char *current_dir = dir_queue[dir_head];
      const char *current_ext = dir_ext_queue[dir_head]; /* "" = NEC-only */
      dir_head++;
      DIR *d = opendir(current_dir);
      if (!d)
      {
        fprintf(error_fp, "Warning: cannot open directory '%s': %s\n", current_dir, strerror(errno));
        continue;
      }

      // To satisfy "files first", we'll collect subdirs separately and add them to the queue after processing files in this dir
      char **subdirs_in_dir = NULL;
      int subdir_count = 0;
      int subdir_cap = 128;

      struct dirent *entry;
      while ((entry = readdir(d)) != NULL)
      {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
          continue;

        char entry_name[256];
        strncpy(entry_name, entry->d_name, sizeof(entry_name) - 1);
        entry_name[sizeof(entry_name) - 1] = '\0';

        char path[1024];
        snprintf(path, sizeof(path), "%s/%s", current_dir, entry_name);

        struct stat st;
        // For directories, check with stat
        if (stat(path, &st) == 0 && S_ISDIR(st.st_mode))
        {
          if (recursive)
          {
            add_to_string_list(&subdirs_in_dir, &subdir_count, &subdir_cap, path);
          }
        }
        else
        {
          if (file_matches_ext_filter(entry_name, current_ext))
          {
            add_to_string_list(&file_list, &num_files, &file_cap, path);
          }
        }
      }
      // closedir(d);

      // Add subdirs to our BFS queue — inherit the same extension filter
      for (int i = 0; i < subdir_count; i++)
      {
        ADD_DIR(subdirs_in_dir[i], current_ext);
        free(subdirs_in_dir[i]);
      }
      free(subdirs_in_dir);
    }
    // Free all directory paths
    for (int i = 0; i < dir_count; i++)
    {
      free(dir_queue[i]);
      free(dir_ext_queue[i]);
    }
    free(dir_queue);
    free(dir_ext_queue);
    #undef ADD_DIR

    g_input_roots = input_roots;
    g_input_root_count = input_root_count;

    if (num_files == 0)
    {
      fprintf(error_fp, "No compatible files found to process.\n");
      if (error_fp != stderr)
        fclose(error_fp);
      return EXIT_FAILURE;
    }

    num_input_files = num_files;

    // Process files (possibly in parallel)
    struct timespec _t0, _t1;
    clock_gettime(CLOCK_MONOTONIC, &_t0);
    if (jobs <= 1 || num_files == 1)
    {
      // Serial path
      if (num_files > 1)
      {
        fprintf(error_fp, "Found %d files to process\n", num_files);
      }
      for (int i = 0; i < num_files; i++)
      {
        const char *input = file_list[i];
        char output[512];
        resolve_output(input, (num_files == 1) ? output_file : "", output, sizeof(output));
        if (num_files > 1)
        {
          char rel_input[PATH_MAX];
          get_relative_input_path(input, rel_input, sizeof(rel_input));
          fprintf(error_fp, "Processing %d of %d: %s...\n", i + 1, num_files, rel_input);
          fflush(error_fp);
        }
        if (process_single_file(input, output, error_fp) != 0)
        {
          failed_count++;
        }
        free(file_list[i]);
        file_list[i] = NULL;
      }
      if (num_files > 1)
      {
        clock_gettime(CLOCK_MONOTONIC, &_t1);
        double elapsed_s = (_t1.tv_sec - _t0.tv_sec) + (_t1.tv_nsec - _t0.tv_nsec) / 1.0e9;
        if (failed_count > 0)
          fprintf(error_fp, "\nCompleted %d file(s) in %.1fs with %d error(s)\n",
                  num_files, elapsed_s, failed_count);
        else
          fprintf(error_fp, "\nCompleted %d file(s) in %.1fs\n", num_files, elapsed_s);
      }
    }
    else
    {
      // Parallel execution with deterministic log ordering
      int count = num_files;
      task_t *tasks = (task_t *)calloc((size_t)count, sizeof(task_t));
      if (!tasks)
      {
        fprintf(error_fp, "Error: Out of memory creating task list\n");
        if (error_fp != stderr)
          fclose(error_fp);
        return EXIT_FAILURE;
      }
      // Prepare tasks
      for (int k = 0; k < count; k++)
      {
        tasks[k].input = file_list[k];
        tasks[k].index = k;
        if (strlen(output_file) > 0 && count == 1)
        {
          strncpy(tasks[k].output, output_file, sizeof(tasks[k].output) - 1);
          tasks[k].output[sizeof(tasks[k].output) - 1] = '\0';
        }
        else
        {
          generate_output_filename(tasks[k].input, tasks[k].output, sizeof(tasks[k].output));
        }
      }

  #if defined(ENABLE_THREADS)
        // Start worker pool
        int nthreads = jobs;
        if (nthreads > count)
          nthreads = count;
        pthread_t *threads = (pthread_t *)calloc((size_t)nthreads, sizeof(pthread_t));
        work_queue_t queue;
        queue.tasks = tasks;
        queue.task_count = count;
        queue.next_index = 0;
        pthread_mutex_init(&queue.lock, NULL);
        for (int t = 0; t < nthreads; t++)
        {
          pthread_create(&threads[t], NULL, worker_thread, &queue);
        }
        for (int t = 0; t < nthreads; t++)
        {
          pthread_join(threads[t], NULL);
        }
        pthread_mutex_destroy(&queue.lock);
        free(threads);
  #else
        // Threading disabled at compile-time; fall back to serial processing
        fprintf(error_fp, "Threading disabled at compile time; running serially.\n");
        for (int k = 0; k < count; k++) {
          task_t *t = &tasks[k];
          fprintf(error_fp, "Processing %d of %d: %s...\n", k + 1, count, t->input);
          fflush(error_fp);
          t->status = process_single_file(t->input, t->output, error_fp);
          t->log_buf = NULL;
          t->log_size = 0;
        }
  #endif

      // Emit logs and summarize
      for (int k = 0; k < count; k++)
      {
        if (tasks[k].log_buf && tasks[k].log_size > 0)
        {
          fwrite(tasks[k].log_buf, 1, tasks[k].log_size, error_fp);
          free(tasks[k].log_buf);
          tasks[k].log_buf = NULL;
        }
        if (tasks[k].status != 0)
          failed_count++;
      }
      free(tasks);
      clock_gettime(CLOCK_MONOTONIC, &_t1);
      double elapsed_s = (_t1.tv_sec - _t0.tv_sec) + (_t1.tv_nsec - _t0.tv_nsec) / 1.0e9;
      if (failed_count > 0)
        fprintf(error_fp, "\nCompleted %d file(s) in %.1fs with %d error(s)\n",
                num_files, elapsed_s, failed_count);
      else
        fprintf(error_fp, "\nCompleted %d file(s) in %.1fs\n", num_files, elapsed_s);
    }

    // Clean up file list
    for (int i = 0; i < num_files; i++)
    {
      if (file_list[i])
        free(file_list[i]);
    }
    free(file_list);
  }

  if (error_fp != stderr)
    fclose(error_fp);
  return (failed_count > 0) ? EXIT_FAILURE : EXIT_SUCCESS;
} /* main */

/*-----------------------------------------------------------------------*/
#if defined(_WIN32)
__attribute__((unused))
static void sig_handler(int signal)
{
  fprintf(stderr, "\n");
  switch (signal)
  {
  case SIGINT:
    fprintf(stderr, "%s\n", "onec: exiting via user interrupt");
    exit(signal);

  case SIGSEGV:
    fprintf(stderr, "%s\n", "onec: segmentation fault");
    exit(signal);

  case SIGFPE:
    fprintf(stderr, "%s\n", "onec: floating point exception");
    exit(signal);

  case SIGABRT:
    fprintf(stderr, "%s\n", "onec: abort signal received");
    exit(signal);

  case SIGTERM:
    fprintf(stderr, "%s\n", "onec: termination request received");
    exit(signal);
  }

} /* end of sig_handler() */
#endif
/*------------------------------------------------------------------------*/
