/*
 * bench_estimate.c — Benchmark estimate_time() against real run times.
 *
 * Recursively walks test/4nec2 example models/, computes the dimensionless
 * complexity T for each .nec/.NEC file, then:
 *
 *   1. Calls run_simulation() directly (output → /dev/null) and records
 *      the simulation-only wall time in sim_ms.  This isolates the matrix
 *      fill + factorisation + solve + far-field work that T models.
 *
 *   2. Forks ./onec with a hard timeout and records the total wall time in
 *      total_ms (includes process start, deck I/O, parse, geometry, output).
 *      This is kept for reference to show the constant-overhead floor.
 *
 * Build: see "make bench_estimate" in the root Makefile
 * Run:   ./bench_estimate [output.csv]
 *        Default output: test/estimate_benchmark.csv
 *
 * CSV columns:
 *   path        – relative path to the .nec file
 *   T           – estimate_time() (-1 if parse failed)
 *   sim_ms      – run_simulation() wall time (ms); -1 on error
 *   total_ms    – ./onec subprocess wall time (ms)
 *   exit_code   – subprocess exit code (0 = success)
 *   timed_out   – 1 if the subprocess was killed after TIMEOUT_MS
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
#define TIMEOUT_MS  5000   /* kill a subprocess run after this many ms */
#define POLL_MS       20   /* waitpid poll interval (ms)               */
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

/* ---- run ./onec <path> as subprocess with a hard timeout ---- */
typedef struct {
    double total_ms;
    int    exit_code;
    int    timed_out;
} fork_result_t;

static fork_result_t run_subprocess(const char *path)
{
    fork_result_t r = { 0.0, -1, 0 };
    int devnull = open("/dev/null", O_WRONLY);
    double t0 = ms_now();
    pid_t pid = fork();
    if (pid == 0) {
        if (devnull >= 0) {
            dup2(devnull, STDOUT_FILENO);
            dup2(devnull, STDERR_FILENO);
            close(devnull);
        }
        execlp("./onec", "onec", path, (char *)NULL);
        exit(127);
    }
    if (devnull >= 0) close(devnull);
    if (pid < 0) { r.exit_code = -1; return r; }

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
    r.total_ms  = ms_now() - t0;
    r.exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    return r;
}

/* ---- null log callback: suppress console noise ---- */
static void null_log(void *ud, int level, const char *msg)
{
    (void)ud; (void)level; (void)msg;
}

/* ---- load and parse a deck into *deck (caller must call destroy_deck()) ---- */
static int load_deck_for_bench(context_t *ctx, const char *path, deck_t *deck)
{
    init_deck(deck);
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    read_deck(ctx, deck, f);
    fclose(f);
    errors_list_t errs = {0};
    parse_deck(ctx, deck, &errs);
    initialize_symbol_table(deck, &errs);
    update_deck_values(ctx, deck);
    for (int i = 0; i < errs.num_errors; i++) free(errs.errors[i].message);
    free(errs.errors);
    return 0;
}

/* ---- compute T (-1.0 on failure) ---- */
static double compute_T(const char *path)
{
    context_t *ctx = create_context();
    set_log_callback(ctx, null_log, NULL);
    deck_t deck;
    int rc = load_deck_for_bench(ctx, path, &deck);
    /* estimate_time() will call calculate_geometry() internally, so the
     * geometry is built and T reflects the true post-expansion segment count. */
    double T = (rc == 0) ? estimate_time(ctx, &deck) : -1.0;
    destroy_deck(&deck);
    destroy_context(ctx);
    return T;
}

/* ---- run run_simulation() directly, timing only that call ------------
 *
 * Deck load + parse are done before the timer starts so they are excluded.
 * Output is directed to /dev/null so file I/O does not inflate the time.
 * write_nec_output() is NOT called — output formatting is also excluded.
 *
 * Returns sim_ms, or -1.0 on load/parse error.
 */
static double run_sim_direct(const char *path, int *ok_out)
{
    *ok_out = 0;

    context_t *ctx = create_context();
    set_log_callback(ctx, null_log, NULL);

    /* redirect all output to /dev/null so ctx internals that write don't block */
    FILE *devnull = fopen("/dev/null", "w");
    ctx->output_fp = devnull ? devnull : stdout;
    ctx->error_fp  = devnull ? devnull : stderr;

    deck_t deck;
    int load_rc = load_deck_for_bench(ctx, path, &deck);
    if (load_rc != 0) {
        destroy_deck(&deck);
        if (devnull) fclose(devnull);
        destroy_context(ctx);
        return -1.0;
    }

    /* ---- start timer AFTER all deck setup ---- */
    double t0 = ms_now();
    int rc = run_simulation(ctx, &deck);
    double sim_ms = ms_now() - t0;

    *ok_out = (rc == 0 && ctx->errors.num_errors == 0) ? 1 : 0;

    destroy_deck(&deck);
    if (devnull) fclose(devnull);
    destroy_context(ctx);
    return sim_ms;
}

/* ---- main ---- */
int main(int argc, char *argv[])
{
    const char *csv_path = (argc > 1) ? argv[1] : "test/estimate_benchmark.csv";
    const char *scan_dir = "test/4nec2 example models";

    nftw(scan_dir, collect_file, 32, FTW_PHYS);
    if (num_files == 0) {
        fprintf(stderr, "No .nec files found under '%s'\n", scan_dir);
        return 1;
    }
    fprintf(stderr, "Found %d files under '%s'\n", num_files, scan_dir);
    fprintf(stderr, "CSV  → %s\n\n", csv_path);

    FILE *csv = fopen(csv_path, "w");
    if (!csv) { perror(csv_path); return 1; }
    fprintf(csv, "path,T,sim_ms,total_ms,exit_code,timed_out\n");

    int n_ok = 0, n_fail = 0, n_tmo = 0;

    for (int i = 0; i < num_files; i++) {
        const char *path = file_list[i];

        /* 1. T from deck scan (fast, no geometry) */
        double T = compute_T(path);

        /* 2. simulation-only time via direct library call */
        int sim_ok = 0;
        double sim_ms = run_sim_direct(path, &sim_ok);

        /* 3. total subprocess time (keeps overhead-floor reference) */
        fork_result_t frun = run_subprocess(path);

        fprintf(csv, "\"%s\",%.6g,%.3f,%.3f,%d,%d\n",
                path, T, sim_ms, frun.total_ms,
                frun.exit_code, frun.timed_out);
        fflush(csv);

        const char *status_str;
        if      (frun.timed_out)        { status_str = "TIMEOUT"; n_tmo++;  }
        else if (frun.exit_code == 0)   { status_str = "OK";      n_ok++;   }
        else                            { status_str = "FAIL";    n_fail++; }

        fprintf(stderr, "[%3d/%3d]  T=%10.3e  sim=%7.2f ms  total=%7.1f ms  %-7s  %s\n",
                i + 1, num_files, T, sim_ms, frun.total_ms, status_str, path);
    }

    fclose(csv);
    for (int i = 0; i < num_files; i++) free(file_list[i]);

    fprintf(stderr, "\nDone.  OK=%d  FAIL=%d  TIMEOUT=%d\n", n_ok, n_fail, n_tmo);
    fprintf(stderr, "Wrote %s\n", csv_path);
    return 0;
}
