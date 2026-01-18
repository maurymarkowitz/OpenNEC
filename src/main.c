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
#include "shared.h"

#ifndef _GETOPT_H
#include <getopt.h>
#endif

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

/******************************************************************************
 * print_version()
 *
 * as the name implies, this simply prints the VERSION_STRING to stdout
 *
 */
static void print_version();
static void print_version()
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
  printf("Usage: %s [-hvntg] [-o output_file] [-e error_file] [source_file]\n", argv[0]);
  puts("Options:");
  puts("  -h, --help: print this description");
  puts("  -v, --version: print version info");
  puts("  -n, --no-run: don't run the simulation after parsing");
  puts("  -t, --test-deck: run various sanity tests");
  puts("  -o, --output-file: (path/)name of the output file");
  puts("  -e, --error-file: output errors to (path/)file, instead of stderr");
  puts("  -g, --greens: write a greens function file to *.ngf or provided filename");
  puts("If source_file is omitted, input is read from stdin and output goes to stdout.");
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
    int c = getopt_long(argc, argv, "hvnti:o:e:g:", program_options, &option_index); // should match the items above
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

/*-------------------------------------------------------------------*/
int main(int argc, char **argv)
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
  FILE *error_fp = NULL;

  // empty these out so we can test them easier
  import_errors.num_errors = 0;
  import_errors.errors = NULL;
  test_errors.num_errors = 0;
  test_errors.errors = NULL;
  geometry_errors.num_errors = 0;
  geometry_errors.errors = NULL;
  geometry_outputs.num_messages = 0;
  geometry_outputs.messages = NULL;
  
  // process the command line options
  parse_options(argc, argv);

  // open input file or use stdin
  if (strlen(input_file) > 0) {
    if ((input_fp = fopen(input_file, "r")) == NULL) {
      char mesg[88] = "onec: ";
      strcat(mesg, input_file);
      perror(mesg);
      exit(EXIT_FAILURE);
    }
    ctx.input_fp = input_fp;
  } else {
    input_fp = stdin;
  }
  
  // open the error file if it was provided, otherwise stderr
  if(strlen(error_file) > 0) {
    if((error_fp = fopen(error_file, "w")) == NULL) {
      char mesg[128] = "onec: ";
      strcat(mesg, error_file);
      perror(mesg);
      exit(EXIT_FAILURE);
    }
    ctx.error_fp = error_fp;
  }
  else {
    error_fp = stderr;
    ctx.error_fp = stderr;
  }
  
  // make an output file name if not specified by user
  if(strlen(output_file) == 0) {
    if (strlen(input_file) > 0) {
      // give it some room, with a little at the end for a potential extension
      output_file = malloc(strlen(input_file) + 10);
      // start with the input file name
      strcpy(output_file, input_file);
      // strip file name extension if there is one (search from end)
      int len = strlen(output_file);
      int idx = len - 1;
      while (idx > 0 && output_file[idx] != '.') idx--;
      if (idx > 0 && output_file[idx] == '.')
        output_file[idx] = '\0';
      // add the extension
      strcat(output_file, ".out");
    } else {
      // no input file, use stdout
      output_fp = stdout;
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
  }
  // TESTING: print any structure errors
  for(int i = 0; i < test_errors.num_errors; i++) {
    fprintf(ctx.error_fp, "%d, '%s'\n", test_errors.errors[i].severity, test_errors.errors[i].message);
  }

  // run it if we've been asked to
  if(run_simulation) {
    calculate_geometry(&ctx, &deck, &geometry_errors, &geometry_outputs);
  }
  for(int i = 0; i < geometry_errors.num_errors; i++) {
    fprintf(ctx.error_fp, "%d, '%s'\n", geometry_errors.errors[i].severity, geometry_errors.errors[i].message);
  }

  // open output file if not already set to stdout
  if (output_fp == NULL) {
    if((output_fp = fopen(output_file, "w")) == NULL) {
      char mesg[88] = "onec: ";
      strcat(mesg, output_file);
      perror(mesg);
      exit(-1);
    }
    ctx.output_fp = output_fp;
  }
  // and write out the results
  write_nec_output(&ctx, &deck, output_fp);
  
  // TESTING: write it back out
  // TURNED OFF, SEEMS TO BE WORKING WELL
  //write_deck_onec(&deck, output_fp);

  nec_context_cleanup(&ctx);

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

      stop( signal );
  }
  
} /* end of sig_handler() */
#endif
/*------------------------------------------------------------------------*/

