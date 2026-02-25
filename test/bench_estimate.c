/*
 * bench_estimate.c — Benchmark nec_estimate_time() against real run times.
 *
 * Recursively walks test/4nec2 example models/, computes the dimensionless
 * complexity T for each .nec/.NEC file, runs ./onec on it with a configurable
 * timeout, and writes a CSV for plotting.
 *
 * Build: see "make bench_estimate" in the root Makefile
 * Run:   ./bench_estimate [output.csv]
 *        Default output: test/estimate_benchmark.csv
 *
 * CSV columns:
 *   path        – relative path to the .nec file
 *   T           – nec_estimate_time() value (dimensionless, -1 if parse failed)
 *   elapsed_ms  – wall-clock run time of ./onec (ms)
 *   exit_code   – process exit code (0 = success, 127 = exec failure)
 *   timed_out   – 1 if the process was killed after TIMEOUT_MS
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>     /* strcasecmp */
#include <ftw.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include "internals.h"

/* ---- tunables ---- */
#define TIMEOUT_MS  5000   /* kill a run after this many ms */
#define POLL_MS       20   /* waitpid poll interval (ms)     */
#define MAX_FILES   4096

/* ---- file list (collected by nftw callback) ---- */
static char *file_list[MAX_FILES];
static int   num_files = 0;

static int collect_file(const char *fpath, const struct stat *sb,
                        int typeflag, struct FTW *ftwbuf)
{
    (void)sb; (void)ftwbuf;
    if (typeflag != FTW_F) return 0;
    const char *dot = strrchr(fpath, '.');
    if (!dot) return 0;
    if (strcasecmp(dot, ".nec") != 0) return 0;
    if (num_files < MAX_FILES)
        file_list[num_files++] = strdup(fpath);
    return 0;
}

/* ---- monotonic millisecond clock ---- */
static double ms_now(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1.0e6;
}

/* ---- run ./onec <path> with a hard timeout ---- */
typedef struct {
    double elapsed_ms;
    int    exit_code;
    int    timed_out;
} run_result_t;

static run_result_t run_with_timeout(const char *path)
{
    run_result_t r = { 0.0, -1, 0 };

    int devnull = open("/dev/null", O_WRONLY);
    double t0 = ms_now();

    pid_t pid = fork();
    if (pid == 0) {
        /* child: silence stdout/stderr, exec onec */
        if (devnull >= 0) {
            dup2(devnull, STDOUT_FILENO);
            dup2(devnull, STDERR_FILENO);
            close(devnull);
        }
        execlp("./onec", "onec", path, (char *)NULL);
        exit(127); /* exec failed */
    }
    if (devnull >= 0) close(devnull);

    if (pid < 0) {
        /* fork failed */
        r.exit_code = -1;
        return r;
    }

    /* parent: poll every POLL_MS until done or timeout */
    int status = 0;
    int loops  = (TIMEOUT_MS + POLL_MS - 1) / POLL_MS;
    int done   = 0;
    for (int i = 0; i < loops; i++) {
        if (waitpid(pid, &status, WNOHANG) > 0) { done = 1; break; }
        usleep((useconds_t)(POLL_MS * 1000));
    }
    if (!done) {
        kill(pid, SIGKILL);
        waitpid(pid, &status, 0);
        r.timed_out = 1;
    }

    r.elapsed_ms = ms_now() - t0;
    r.exit_code  = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    return r;
}

/* ---- null log callback: suppress console noise while parsing ---- */
static void null_log(void *ud, int level, const char *msg)
{
    (void)ud; (void)level; (void)msg;
}

/* ---- parse a deck and return T (-1 on failure) ---- */
static double compute_T(const char *path)
{
    nec_context_t *ctx = nec_create_context();
    nec_set_log_callback(ctx, null_log, NULL);

    deck_t deck;
    memset(&deck, 0, sizeof(deck));

    FILE *f = fopen(path, "r");
    if (!f) {
        nec_destroy_context(ctx);
        return -1.0;
    }
    read_deck(ctx, &deck, f);
    fclose(f);

    errors_list_t errs = {0};
    parse_deck(ctx, &deck, &errs);
    initialize_symbol_table(&deck, &errs);
    update_deck_values(ctx, &deck);

    double T = nec_estimate_time(&deck);

    free_deck(&deck);
    for (int i = 0; i < errs.num_errors; i++) free(errs.errors[i].message);
    free(errs.errors);
    nec_destroy_context(ctx);

    return T;
}

/* ---- main ---- */
int main(int argc, char *argv[])
{
    const char *csv_path = (argc > 1) ? argv[1] : "test/estimate_benchmark.csv";
    const char *scan_dir = "test/4nec2 example models";

    /* collect .nec/.NEC files */
    nftw(scan_dir, collect_file, 32, FTW_PHYS);
    if (num_files == 0) {
        fprintf(stderr, "No .nec files found under '%s'\n", scan_dir);
        return 1;
    }
    fprintf(stderr, "Found %d files under '%s'\n", num_files, scan_dir);
    fprintf(stderr, "CSV  → %s\n", csv_path);
    fprintf(stderr, "Timeout per file: %d ms\n\n", TIMEOUT_MS);

    FILE *csv = fopen(csv_path, "w");
    if (!csv) { perror(csv_path); return 1; }
    fprintf(csv, "path,T,elapsed_ms,exit_code,timed_out\n");

    int n_ok = 0, n_fail = 0, n_tmo = 0;

    for (int i = 0; i < num_files; i++) {
        const char *path = file_list[i];

        /* compute T (parses the deck) */
        double T = compute_T(path);

        /* time ./onec on this file */
        run_result_t run = run_with_timeout(path);

        /* write CSV row (quote the path to handle commas/spaces) */
        fprintf(csv, "\"%s\",%.6g,%.3f,%d,%d\n",
                path, T, run.elapsed_ms, run.exit_code, run.timed_out);
        fflush(csv);

        const char *status_str;
        if      (run.timed_out)          { status_str = "TIMEOUT"; n_tmo++;  }
        else if (run.exit_code == 0)     { status_str = "OK";      n_ok++;   }
        else                             { status_str = "FAIL";    n_fail++; }

        fprintf(stderr, "[%3d/%3d]  T=%10.3e  %7.1f ms  %-7s  %s\n",
                i + 1, num_files, T, run.elapsed_ms, status_str, path);
    }

    fclose(csv);
    for (int i = 0; i < num_files; i++) free(file_list[i]);

    fprintf(stderr, "\nDone.  OK=%d  FAIL=%d  TIMEOUT=%d\n", n_ok, n_fail, n_tmo);
    fprintf(stderr, "Wrote %s\n", csv_path);
    return 0;
}
