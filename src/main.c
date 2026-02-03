/******************************************************************************
 *
 * main.c is the entry point for the command-line version of OpenNEC
 * It works along with input.c and output.c. Together they parse the
 * command line, read an input file if provided, run the commands in
 * the deck, and then print the output to more files.
 *
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "opennec.h"
#include "input.h"
#include "control.h"
#include "output.h"
#include "tests.h"

#ifndef _GETOPT_H
#include <getopt.h>
#endif

#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <sys/times.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>


/** signal handler */
//static void sig_handler(int signal);

// various switches for the command line arguments
static bool run_simulation = true;
static bool run_tests = false;
static bool run_greens = false;
static bool recursive = false;
static char *input_file = "";
static char *output_file = "";
static char *error_file = "";
static char *greens_file = "";
static int jobs = 1; // number of parallel jobs (-j)

/******************************************************************************
 * print_version()
 *
 * as the name implies, this simply prints the VERSION_STRING to stdout
 *
 */
static void print_version(void);
static void print_version(void)
{
  puts("Onec " VERSION_STRING);
}

/******************************************************************************
 * print_usage()
 *
 * prints the usage notes
 *
 */
void print_usage(char *argv[])
{
  printf("Usage: %s [-hvntgr] [-o output_file] [-e error_file] [source_file...]\n", argv[0]);
  puts("Options:");
  puts("  -h, --help: print this description");
  puts("  -v, --version: print version info");
  puts("  -n, --no-run: don't run the simulation after parsing");
  puts("  -t, --test-deck: run various sanity tests");
  puts("  -o, --output-file: (path/)name of the output file (single file only)");
  puts("  -e, --error-file: output errors to (path/)file, instead of stderr");
  puts("  -g, --greens: write a greens function file to *.ngf or provided filename");
  puts("  -j, --jobs N: process up to N files in parallel (default 1)");
  puts("  -r, --recursive: process directories recursively");
  puts("Multiple input files can be specified; each will generate a .out file.");
  puts("If no source_file is provided, input is read from stdin and output goes to stdout.");
}

/******************************************************************************
 * stop()
 *
 * Cleanup and exit - the single exit point for the program
 * TODO: Make this static once all calculation files use add_error() instead
 *
 */
int stop(nec_context_t *ctx, int flag)
{
  if (ctx->input_fp != NULL)
    fclose(ctx->input_fp);
  if (ctx->output_fp != NULL)
    fclose(ctx->output_fp);
  if (ctx->plot_fp != NULL)
    fclose(ctx->plot_fp);

  exit(flag);
}

static struct option program_options[] =
{
  {"help", no_argument, NULL, 'h'},
  {"version", no_argument, NULL, 'v'},
  {"no-run", no_argument, NULL, 'n'},
  {"test-deck", no_argument, NULL, 't'},
  {"recursive", no_argument, NULL, 'r'},
  {"input-file", required_argument, NULL, 'i'},
  {"output-file", required_argument,  NULL, 'o'},
  {"error-file", required_argument,  NULL, 'e'},
  {"greens", required_argument, NULL, 'g'},
  {"jobs", required_argument, NULL, 'j'},
  {0, 0, 0, 0}
};

/******************************************************************************
 * parse_options()
 *
 * parses the command line options
 *
 */
void parse_options(int argc, char *argv[])
{
  int option_index = 0;
  /* int printed_help = false; */
  
  while(1) {
    // eat an option and exit if we're done
    /* portable short options: 'g' requires an argument */
    int c = getopt_long(argc, argv, "hvntri:o:e:g:j:", program_options, &option_index); // should match the items above
    if(c == -1) break;
    
    switch(c) {
      case 0:
        // flag-setting options return 0 - these are t and n
        if (program_options[option_index].flag != 0)
          break;
        
      case 'h':
        print_usage(argv);
        /* printed_help = true; */
        break;
        
      case 'v':
        print_version();
        /* printed_help =  true; */
        break;
        
      case 'r':
        recursive = true;
        break;
        
      case 'o':
        output_file = optarg;
        break;
        
      case 'e':
        error_file = optarg;
        break;
        
      case 'n':
        run_simulation = false;
        break;
        
      case 't':
        run_tests = true;
        break;
        
      case 'g':
        run_greens = true;
        greens_file = optarg;
        break;
      case 'j':
        jobs = atoi(optarg);
        if (jobs < 1) jobs = 1;
        break;
        
      default:
        abort();
    }
  } // while
  
  // now see if there's a filename at the end without an option
  // flag, if so it overrides -i if it was supplied
  if (optind < argc)
    input_file = argv[optind];
  
  // if no input file, we'll use stdin
}

/******************************************************************************
 * process_single_file()
 *
 * Process a single input file through the complete simulation pipeline.
 * Returns 0 on success, -1 on error.
 *
 */
static int process_single_file(const char *input_filename, const char *output_filename, FILE *error_fp)
{
  nec_context_t ctx;
  nec_context_init(&ctx);

  // main variables
  deck_t deck;              // the deck we're processing, we'll make it local as it disappears on exit
  memset(&deck, 0, sizeof(deck_t));
  errors_list_t import_errors;   // a list of errors that occured during import
  errors_list_t test_errors;     // a list of errors and warnings about the deck's format
  errors_list_t geometry_errors; // a list of errors and warnings during the conversion to segments
  outputs_list_t geometry_outputs; // informational messages from geometry processing

  FILE *input_fp = NULL;
  FILE *output_fp = NULL;

  // empty these out so we can test them easier
  import_errors.num_errors = 0;
  import_errors.errors = NULL;
  test_errors.num_errors = 0;
  test_errors.errors = NULL;
  geometry_errors.num_errors = 0;
  geometry_errors.errors = NULL;
  geometry_outputs.num_messages = 0;
  geometry_outputs.messages = NULL;
  
  ctx.error_fp = error_fp;

  // open input file or use stdin
  if (strlen(input_filename) > 0) {
    if ((input_fp = fopen(input_filename, "r")) == NULL) {
      char mesg[88] = "onec: ";
      strcat(mesg, input_filename);
      perror(mesg);
      return -1;
    }
    ctx.input_fp = input_fp;
  } else {
    input_fp = stdin;
    ctx.input_fp = stdin;
  }

  // open output file or use stdout
  if (strlen(output_filename) > 0) {
    if((output_fp = fopen(output_filename, "w")) == NULL) {
      char mesg[88] = "onec: ";
      strcat(mesg, output_filename);
      perror(mesg);
      if (input_fp != stdin) fclose(input_fp);
      return -1;
    }
    ctx.output_fp = output_fp;
  } else {
    output_fp = stdout;
    ctx.output_fp = stdout;
  }

  // open greens output file if requested
  if (run_greens) {
    char ngfpath[512];
    const char *path = NULL;
    if (strlen(greens_file) > 0) {
      path = greens_file;
    } else if (strlen(input_filename) > 0) {
      // derive from input filename by replacing extension with .ngf
      strncpy(ngfpath, input_filename, sizeof(ngfpath) - 1);
      ngfpath[sizeof(ngfpath) - 1] = '\0';
      char *dot = strrchr(ngfpath, '.');
      char *slash = strrchr(ngfpath, '/');
      if (dot != NULL && (slash == NULL || dot > slash)) {
        *dot = '\0';
      }
      strncat(ngfpath, ".ngf", sizeof(ngfpath) - strlen(ngfpath) - 1);
      path = ngfpath;
    } else {
      path = "greens.ngf";
    }
    ctx.green_fp = fopen(path, "w");
    if (!ctx.green_fp) {
      fprintf(ctx.error_fp, "Warning: could not open greens file '%s' for writing; skipping.\n", path);
    }
  }

  // read input file into a deck
  read_deck(&ctx, &deck, input_fp);

  // and then parse what we read into the card
  parse_deck(&ctx, &deck, &import_errors);
  
  // Initialize symbol table: collect all SY symbols, add defaults (pi, c),
  // and evaluate symbols in comment section for initial values
  initialize_symbol_table(&deck, &import_errors);
  
  // Evaluate all formulas in the deck
  update_deck_values(&deck);
  
  // TESTING: print any file errors
  if (import_errors.num_errors > 0) {
    const char *display_name = strlen(input_filename) > 0 ? input_filename : "stdin";
    fprintf(ctx.error_fp, "\n=== Import Errors for %s ===\n", display_name);
    for(int i = 0; i < import_errors.num_errors; i++) {
      fprintf(ctx.error_fp, "%s\n", import_errors.errors[i].message);
    }
  }

  // run basic sanity checks on the structure
  if(run_tests) {
    test_deck_structure(&ctx, &deck, &test_errors);
    test_duplicate_tags(&ctx, &deck, &test_errors);
    test_card_inputs(&ctx, &deck, &test_errors);
  }
  // TESTING: print any structure errors
  if (test_errors.num_errors > 0) {
    const char *display_name = strlen(input_filename) > 0 ? input_filename : "stdin";
    fprintf(ctx.error_fp, "\n=== Test Errors for %s ===\n", display_name);
    for(int i = 0; i < test_errors.num_errors; i++) {
      fprintf(ctx.error_fp, "%d, '%s'\n", test_errors.errors[i].severity, test_errors.errors[i].message);
    }
  }

  // run it if we've been asked to
  if(run_simulation) {
    // Run complete simulation with batch processing
    int sim_result = nec_run_simulation(&ctx, &deck);
    
    // Check for any errors that occurred during calculation (whether simulation failed or succeeded)
    if (ctx.errors.num_errors > 0 || sim_result != 0) {
      if (sim_result != 0) {
        fprintf(ctx.error_fp, "Error: Failed to run simulation for %s.\n", 
                strlen(input_filename) > 0 ? input_filename : "stdin");
      }
      
      if (ctx.errors.num_errors > 0) {
        const char *display_name = strlen(input_filename) > 0 ? input_filename : "stdin";
        fprintf(ctx.error_fp, "\n=== Calculation Errors for %s ===\n", display_name);
        for (int i = 0; i < ctx.errors.num_errors; i++) {
          fprintf(ctx.error_fp, "%s\n", ctx.errors.errors[i].message);
        }
      }
      
      nec_context_cleanup(&ctx);
      if (input_fp != stdin) fclose(input_fp);
      if (output_fp != stdout) fclose(output_fp);
      return -1;
    }
  }
  if (geometry_errors.num_errors > 0) {
    const char *display_name = strlen(input_filename) > 0 ? input_filename : "stdin";
    fprintf(ctx.error_fp, "\n=== Geometry Errors for %s ===\n", display_name);
    for(int i = 0; i < geometry_errors.num_errors; i++) {
      fprintf(ctx.error_fp, "%d, '%s'\n", geometry_errors.errors[i].severity, geometry_errors.errors[i].message);
    }
  }

  // write out the results (only if simulation was configured and ran)
  if (run_simulation && ctx.save.nfrq > 0) {
    write_nec_output(&ctx, &deck, output_fp);
  } else if (run_simulation && ctx.save.nfrq == 0) {
    fprintf(ctx.error_fp, "Warning: No FR card found, skipping output generation\n");
  }

  // close greens file if open
  if (ctx.green_fp) {
    fclose(ctx.green_fp);
    ctx.green_fp = NULL;
  }

  // free_deck(&deck);
  nec_context_cleanup(&ctx);
  if (input_fp != stdin) fclose(input_fp);
  if (output_fp != stdout) fclose(output_fp);

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
  if (dot != NULL && (slash == NULL || dot > slash)) {
    *dot = '\0';
  }
  
  // Add .out extension
  strncat(output_filename, ".out", size - strlen(output_filename) - 1);
}

typedef struct {
  const char *input;
  char output[512];
  int index;
  int status; // 0 ok, -1 failure
  char *log_buf; // captured stderr
  size_t log_size;
} task_t;

typedef struct {
  task_t *tasks;
  int task_count;
  int next_index;
  pthread_mutex_t lock;
} work_queue_t;

static void *worker_thread(void *arg)
{
  work_queue_t *q = (work_queue_t *)arg;
  while (1) {
    int idx = -1;
    pthread_mutex_lock(&q->lock);
    if (q->next_index < q->task_count) {
      idx = q->next_index++;
    }
    pthread_mutex_unlock(&q->lock);

    if (idx == -1) break;

    task_t *t = &q->tasks[idx];

    // Print progress for parallel processing
    fprintf(stderr, "Processing %s...\n", t->input);
    fflush(stderr);

    // capture logs using open_memstream
    char *buf = NULL;
    size_t sz = 0;
    FILE *memfp = open_memstream(&buf, &sz);
    if (!memfp) {
      // fallback: use stderr (may interleave)
      fprintf(stderr, "Processing %s...\n", t->input);
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

static void add_to_string_list(char ***list, int *count, int *cap, const char *str) {
  if (*count >= *cap) {
    *cap = (*cap == 0) ? 16 : (*cap * 2);
    char **new_list = realloc(*list, *cap * sizeof(char *));
    if (!new_list) abort();
    *list = new_list;
  }
  (*list)[(*count)++] = strdup(str);
}

static int has_nec_extension(const char *filename) {
  const char *ext = strrchr(filename, '.');
  if (!ext) return false;
  return (strcasecmp(ext, ".nec") == 0 ||
          strcasecmp(ext, ".deck") == 0 ||
          strcasecmp(ext, ".onec") == 0);
}

/*-------------------------------------------------------------------*/
int main(int argc, char **argv)
{
  FILE *error_fp = NULL;
  
  // process the command line options
  parse_options(argc, argv);

  // open the error file if it was provided, otherwise stderr
  if(strlen(error_file) > 0) {
    if((error_fp = fopen(error_file, "w")) == NULL) {
      char mesg[128] = "onec: ";
      strcat(mesg, error_file);
      perror(mesg);
      exit(EXIT_FAILURE);
    }
  }
  else {
    error_fp = stderr;
  }

  // Collect input files and folders
  char **file_list = NULL;
  int num_files = 0;
  int file_cap = 0;

  if (optind >= argc) {
    // No input files specified - use stdin/stdout
    const char *out = (strlen(output_file) > 0) ? output_file : "";
    if (process_single_file("", out, error_fp) != 0) {
      fprintf(error_fp, "Error processing stdin\n");
      if (error_fp != stderr) fclose(error_fp);
      return EXIT_FAILURE;
    }
  } else {
    // Collect all files, including from directories
    char **dir_queue = NULL;
    int dir_count = 0;
    int dir_cap = 0;
    int dir_head = 0;

    // Initial files/dirs from command line
    for (int i = optind; i < argc; i++) {
      struct stat st;
      if (stat(argv[i], &st) == 0) {
        if (S_ISDIR(st.st_mode)) {
          add_to_string_list(&dir_queue, &dir_count, &dir_cap, argv[i]);
        } else {
          add_to_string_list(&file_list, &num_files, &file_cap, argv[i]);
        }
      } else {
        fprintf(error_fp, "Warning: cannot access '%s': %s\n", argv[i], strerror(errno));
      }
    }

    // BFS for directories
    while (dir_head < dir_count) {
      char *current_dir = dir_queue[dir_head++];
      DIR *d = opendir(current_dir);
      if (!d) {
        fprintf(error_fp, "Warning: cannot open directory '%s': %s\n", current_dir, strerror(errno));
        free(current_dir);
        continue;
      }

      // To satisfy "files first", we'll collect subdirs separately and add them to the queue after processing files in this dir
      char **subdirs_in_dir = NULL;
      int subdir_count = 0;
      int subdir_cap = 0;

      struct dirent *entry;
      while ((entry = readdir(d)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
          continue;

        char path[1024];
        snprintf(path, sizeof(path), "%s/%s", current_dir, entry->d_name);

        struct stat st;
        if (lstat(path, &st) == 0) { // Use lstat to avoid following symloops if we want, or stat
          if (S_ISDIR(st.st_mode)) {
            if (recursive) {
              add_to_string_list(&subdirs_in_dir, &subdir_count, &subdir_cap, path);
            }
          } else if (S_ISREG(st.st_mode)) {
            if (has_nec_extension(entry->d_name)) {
              add_to_string_list(&file_list, &num_files, &file_cap, path);
            }
          }
        }
      }
      closedir(d);

      // Add subdirs to our BFS queue
      for (int i = 0; i < subdir_count; i++) {
        add_to_string_list(&dir_queue, &dir_count, &dir_cap, subdirs_in_dir[i]);
        free(subdirs_in_dir[i]);
      }
      free(subdirs_in_dir);
      free(current_dir);
    }
    free(dir_queue);

    if (num_files == 0) {
      fprintf(error_fp, "No compatible files found to process.\n");
      if (error_fp != stderr) fclose(error_fp);
      return EXIT_FAILURE;
    }

    // Process files (possibly in parallel)
    int failed_count = 0;
    if (jobs <= 1 || num_files == 1) {
      // Serial path
      for (int i = 0; i < num_files; i++) {
        const char *input = file_list[i];
        char output[512];
        if (strlen(output_file) > 0 && num_files == 1) {
          strncpy(output, output_file, sizeof(output) - 1);
          output[sizeof(output) - 1] = '\0';
        } else {
          generate_output_filename(input, output, sizeof(output));
        }
        if (num_files > 1) {
          fprintf(error_fp, "Processing %s...\n", input);
          fflush(error_fp);
        }
        if (process_single_file(input, output, error_fp) != 0) {
          fprintf(error_fp, "Error processing %s, continuing to next file...\n", input);
          failed_count++;
        }
      }
      if (failed_count > 0) {
        fprintf(error_fp, "\nCompleted with %d error(s) out of %d file(s)\n", failed_count, num_files);
      }
    } else {
      // Parallel execution with deterministic log ordering
      int count = num_files;
      task_t *tasks = (task_t *)calloc((size_t)count, sizeof(task_t));
      if (!tasks) {
        fprintf(error_fp, "Error: Out of memory creating task list\n");
        if (error_fp != stderr) fclose(error_fp);
        return EXIT_FAILURE;
      }
      // Prepare tasks 
      for (int k = 0; k < count; k++) {
        tasks[k].input = file_list[k];
        tasks[k].index = k;
        if (strlen(output_file) > 0 && count == 1) {
          strncpy(tasks[k].output, output_file, sizeof(tasks[k].output) - 1);
          tasks[k].output[sizeof(tasks[k].output) - 1] = '\0';
        } else {
          generate_output_filename(tasks[k].input, tasks[k].output, sizeof(tasks[k].output));
        }
      }
      // Start worker pool
      int nthreads = jobs;
      if (nthreads > count) nthreads = count;
      pthread_t *threads = (pthread_t *)calloc((size_t)nthreads, sizeof(pthread_t));
      work_queue_t queue;
      queue.tasks = tasks;
      queue.task_count = count;
      queue.next_index = 0;
      pthread_mutex_init(&queue.lock, NULL);
      for (int t = 0; t < nthreads; t++) {
        pthread_create(&threads[t], NULL, worker_thread, &queue);
      }
      for (int t = 0; t < nthreads; t++) {
        pthread_join(threads[t], NULL);
      }
      pthread_mutex_destroy(&queue.lock);
      free(threads);

      // Emit logs and summarize
      for (int k = 0; k < count; k++) {
        if (tasks[k].log_buf && tasks[k].log_size > 0) {
          fwrite(tasks[k].log_buf, 1, tasks[k].log_size, error_fp);
          free(tasks[k].log_buf);
          tasks[k].log_buf = NULL;
        }
        if (tasks[k].status != 0) failed_count++;
      }
      free(tasks);
      if (failed_count > 0) {
        fprintf(error_fp, "\nCompleted with %d error(s) out of %d file(s)\n", failed_count, num_files);
      }
    }
    
    // Clean up file list
    for (int i = 0; i < num_files; i++) free(file_list[i]);
    free(file_list);
  }

  if (error_fp != stderr) fclose(error_fp);
  return EXIT_SUCCESS;
} /* main */

/*-----------------------------------------------------------------------*/

/* prnt sets up the print formats for impedance loading */
void prnt(nec_context_t *ctx, int in1, int in2, int in3, double fl1, double fl2,
          double fl3, double fl4, double fl5,
          double fl6, char *ia, int ichar )
{
  /* record to be output and buffer used to make it */
  char record[101+ichar*4], buff[16];
  int in[3], i1, i;
  double fl[6];

  in[0]= in1;
  in[1]= in2;
  in[2]= in3;
  fl[0]= fl1;
  fl[1]= fl2;
  fl[2]= fl3;
  fl[3]= fl4;
  fl[4]= fl5;
  fl[5]= fl6;

  /* integer format */
  i1=0;
  strcpy( record, "\n " );

  if( (in1 == 0) && (in2 == 0) && (in3 == 0) )
  {
    strcat( record, " ALL" );
    i1=1;
  }

  for( i = i1; i < 3; i++ )
  {
    if( in[i] == 0)
      strcat( record, "     " );
    else
    {
      snprintf( buff, 6, "%5d", in[i] );
      strcat( record, buff );
    }
  }

  /* floating point format */
  for( i = 0; i < 6; i++ )
  {
    if( fabs( fl[i]) >= 1.0e-20 )
    {
      snprintf( buff, 15, " %11.4E", fl[i] );
      strcat( record, buff );
    }
    else
      strcat( record, "            " );
  }

  strcat( record, "   " );
  strcat( record, ia );
  fprintf( ctx->output_fp, "%s", record );

  return;
}

/*-----------------------------------------------------------------------*/
#if __WIN32__
static void sig_handler( int signal )
{
  fprintf( error_fp, "\n" );
  switch( signal )
  {
    case SIGINT :
      fprintf( error_fp, "%s\n", "onec: exiting via user interrupt" );
      exit( signal );
      
    case SIGSEGV :
      fprintf( error_fp, "%s\n", "onec: segmentation fault" );
      exit( signal );
      
    case SIGFPE :
      fprintf( error_fp, "%s\n", "onec: floating point exception" );
      exit( signal );
      
    case SIGABRT :
      fprintf( error_fp, "%s\n", "onec: abort signal received" );
      exit( signal );
      
    case SIGTERM :
      fprintf( error_fp, "%s\n", "onec: termination request received" );
      exit( signal );
  }
  
} /* end of sig_handler() */
#endif
/*------------------------------------------------------------------------*/

