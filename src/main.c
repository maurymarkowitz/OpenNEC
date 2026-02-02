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

#ifndef _GETOPT_H
#include <getopt.h>
#endif

#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <sys/times.h>


/** signal handler */
//static void sig_handler(int signal);

// various switches for the command line arguments
static int run_simulation = TRUE;
static int run_tests = FALSE;
static int run_greens = FALSE;
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
  printf("Usage: %s [-hvntg] [-o output_file] [-e error_file] [source_file...]\n", argv[0]);
  puts("Options:");
  puts("  -h, --help: print this description");
  puts("  -v, --version: print version info");
  puts("  -n, --no-run: don't run the simulation after parsing");
  puts("  -t, --test-deck: run various sanity tests");
  puts("  -o, --output-file: (path/)name of the output file (single file only)");
  puts("  -e, --error-file: output errors to (path/)file, instead of stderr");
  puts("  -g, --greens: write a greens function file to *.ngf or provided filename");
  puts("  -j, --jobs N: process up to N files in parallel (default 1)");
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
  /* int printed_help = FALSE; */
  
  while(1) {
    // eat an option and exit if we're done
    /* portable short options: 'g' requires an argument */
    int c = getopt_long(argc, argv, "hvnti:o:e:g:j:", program_options, &option_index); // should match the items above
    if(c == -1) break;
    
    switch(c) {
      case 0:
        // flag-setting options return 0 - these are t and n
        if (program_options[option_index].flag != 0)
          break;
        
      case 'h':
        print_usage(argv);
        /* printed_help = TRUE; */
        break;
        
      case 'v':
        print_version();
        /* printed_help =  TRUE; */
        break;
        
      case 'o':
        output_file = optarg;
        break;
        
      case 'e':
        error_file = optarg;
        break;
        
      case 'n':
        run_simulation = FALSE;
        break;
        
      case 't':
        run_tests = TRUE;
        break;
        
      case 'g':
        run_greens = TRUE;
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
  // TESTING: print any file errors
  for(int i = 0; i < import_errors.num_errors; i++) {
    fprintf(ctx.error_fp, "%s\n", import_errors.errors[i].message);
  }

  // run basic sanity checks on the structure
  if(run_tests) {
    test_deck_structure(&ctx, &deck, &test_errors);
    test_duplicate_tags(&ctx, &deck, &test_errors);
    test_card_inputs(&ctx, &deck, &test_errors);
  }
  // TESTING: print any structure errors
  for(int i = 0; i < test_errors.num_errors; i++) {
    fprintf(ctx.error_fp, "%d, '%s'\n", test_errors.errors[i].severity, test_errors.errors[i].message);
  }

  // run it if we've been asked to
  if(run_simulation) {
    // Run complete simulation with batch processing
    if (nec_run_simulation(&ctx, &deck) != 0) {
      fprintf(ctx.error_fp, "Error: Failed to run simulation for %s.\n", 
              strlen(input_filename) > 0 ? input_filename : "stdin");
      
      // Display any accumulated errors
      if (ctx.errors.num_errors > 0) {
        fprintf(ctx.error_fp, "\n=== Calculation Errors ===\n");
        for (int i = 0; i < ctx.errors.num_errors; i++) {
          fprintf(ctx.error_fp, "%s\n", ctx.errors.errors[i].message);
        }
      }
      
      nec_context_cleanup(&ctx);
      if (input_fp != stdin) fclose(input_fp);
      if (output_fp != stdout) fclose(output_fp);
      return -1;
    }
    
    // Check for any errors that occurred during calculation
    if (ctx.errors.num_errors > 0) {
      fprintf(ctx.error_fp, "\n=== Calculation Errors ===\n");
      for (int i = 0; i < ctx.errors.num_errors; i++) {
        fprintf(ctx.error_fp, "%s\n", ctx.errors.errors[i].message);
      }
      nec_context_cleanup(&ctx);
      if (input_fp != stdin) fclose(input_fp);
      if (output_fp != stdout) fclose(output_fp);
      return -1;
    }
  }
  for(int i = 0; i < geometry_errors.num_errors; i++) {
    fprintf(ctx.error_fp, "%d, '%s'\n", geometry_errors.errors[i].severity, geometry_errors.errors[i].message);
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

    // capture logs using open_memstream
    char *buf = NULL;
    size_t sz = 0;
    FILE *memfp = open_memstream(&buf, &sz);
    if (!memfp) {
      // fallback: use stderr (may interleave)
      t->status = process_single_file(t->input, t->output, stderr);
      t->log_buf = NULL;
      t->log_size = 0;
      continue;
    }

    // Emit the processing header into the captured stream for consistency
    fprintf(memfp, "Processing %s -> %s\n", t->input, t->output);
    t->status = process_single_file(t->input, t->output, memfp);
    fflush(memfp);
    fclose(memfp); // sets buf/sz
    t->log_buf = buf;
    t->log_size = sz;
  }
  return NULL;
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

  // Collect input files from command line arguments
  int num_files = argc - optind;
  
  if (num_files == 0) {
    // No input files specified - use stdin/stdout
    const char *out = (strlen(output_file) > 0) ? output_file : "";
    if (process_single_file("", out, error_fp) != 0) {
      fprintf(error_fp, "Error processing stdin\n");
      if (error_fp != stderr) fclose(error_fp);
      return EXIT_FAILURE;
    }
  } else {
    // Process files (possibly in parallel)
    int failed_count = 0;
    if (jobs <= 1 || num_files == 1) {
      // Serial path remains unchanged
      for (int i = optind; i < argc; i++) {
        const char *input = argv[i];
        char output[512];
        if (strlen(output_file) > 0 && num_files == 1) {
          strncpy(output, output_file, sizeof(output) - 1);
          output[sizeof(output) - 1] = '\0';
        } else {
          generate_output_filename(input, output, sizeof(output));
        }
        fprintf(error_fp, "Processing %s -> %s\n", input, output);
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
      // Prepare tasks in argv order
      for (int k = 0; k < count; k++) {
        int i = optind + k;
        tasks[k].input = argv[i];
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

      // Emit logs and summarize in argv order
      for (int k = 0; k < count; k++) {
        if (tasks[k].log_buf && tasks[k].log_size > 0) {
          fwrite(tasks[k].log_buf, 1, tasks[k].log_size, error_fp);
          free(tasks[k].log_buf);
          tasks[k].log_buf = NULL;
        } else {
          // if no captured log, at least show a line
          fprintf(error_fp, "Processing %s -> %s\n", tasks[k].input, tasks[k].output);
        }
        if (tasks[k].status != 0) failed_count++;
      }
      free(tasks);
      if (failed_count > 0) {
        fprintf(error_fp, "\nCompleted with %d error(s) out of %d file(s)\n", failed_count, num_files);
      }
    }
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

