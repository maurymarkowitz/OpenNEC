#include <libgen.h>
// nec_batch_tester.c
// Standalone tool to recursively parse all .nec files in a directory tree and output a markdown summary of issues.
// Usage: ./nec_batch_tester <root_directory> <output_report.md>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdbool.h>
#include "opennec.h"

// Structure to hold error info for a file
#define MAX_PATH 4096
#define MAX_FILES 4096
static char sy_report_path[MAX_PATH];
struct file_issue
{
    char path[MAX_PATH];
    char error[256];
    bool found_it;
    bool found_op;
    bool found_sy;
};
// Global counter for total .nec files found
static int g_total_files_found = 0;

// Recursively find all .nec files in a directory
typedef void (*file_callback)(const char *filepath, void *userdata);

struct stat st; // removed duplicate, keep only the one outside main

void find_nec_files(const char *dir, file_callback cb, void *userdata)
{
    DIR *dp = opendir(dir);
    if (!dp) {
        return;
    }
    struct dirent *entry;
    char path[MAX_PATH];
    while ((entry = readdir(dp)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        snprintf(path, sizeof(path), "%s/%s", dir, entry->d_name);

        if (stat(path, &st) == -1)
            continue;
        if (S_ISDIR(st.st_mode))
        {
            find_nec_files(path, cb, userdata);
        }
        else
        {
            size_t len = strlen(entry->d_name);
            if (len > 4)
            {
                const char *ext = entry->d_name + len - 4;
                if (strcasecmp(ext, ".nec") == 0)
                {
                    cb(path, userdata);
                }
            }
        }
    }
    closedir(dp);
}

// Try to parse a .nec file and record any issue
void test_nec_file(const char *filepath, void *userdata) {
    printf("[DEBUG] START test_nec_file: %s\n", filepath);
    g_total_files_found++;
    int has_error = 0;
    FILE *fp = fopen(filepath, "r");
    nec_context_t ctx = {0};
    deck_t deck = {0};
    errors_list_t errors = {0};
    struct file_issue *issues = (struct file_issue *)userdata;
    int idx = 0;
    while (issues[idx].path[0] && idx < MAX_FILES)
        idx++;
    int found_it = 0, found_op = 0, found_sy = 0;

    if (!fp)
    {
        strncpy(issues[idx].path, filepath, sizeof(issues[idx].path) - 1);
        strncpy(issues[idx].error, "Could not open file", sizeof(issues[idx].error) - 1);
        has_error = 1;
    }
    else
    {
        read_deck(&ctx, &deck, fp);
        fclose(fp);

        parse_deck(&ctx, &deck, &errors);
        // Removed incorrect linked-list symbol print loop. Use array-based loop below.
        update_deck_values(&deck);

        // --- SY card reporting ---
        extern char sy_report_path[MAX_PATH];
        FILE *sy_out = fopen(sy_report_path, "a");
        if (sy_out)
        {
            for (int sym_idx = 0; sym_idx < deck.num_symbols; ++sym_idx)
            {
                key_value_t *sym = deck.symbols[sym_idx];
                if (sym && sym->key && (strcasecmp(sym->key, "pi") == 0 || strcasecmp(sym->key, "c") == 0))
                {
                    continue; // skip default symbols
                }
                 fprintf(sy_out, "    Parsed: %s = %s = %f\n", sym && sym->key ? sym->key : "(null)", sym && sym->value ? sym->value : "(null)", sym ? sym->fv : 0.0);
                fflush(sy_out);
            }
            fflush(sy_out);
            fclose(sy_out);
        }

        // Scan for IT/OP cards
        for (int i = 0; i < deck.num_cards; ++i)
        {
            if (deck.cards && deck.cards[i].orig_str)
            {
                char code[3] = {0};
                strncpy(code, deck.cards[i].orig_str, 2);
                code[0] = toupper((unsigned char)code[0]);
                code[1] = toupper((unsigned char)code[1]);
                if (strcmp(code, "IT") == 0)
                    found_it = 1;
                if (strcmp(code, "OP") == 0)
                    found_op = 1;
                if (strcmp(code, "SY") == 0)
                    found_sy = 1;
            }
        }
        if (errors.num_errors > 0)
        {
            strncpy(issues[idx].path, filepath, sizeof(issues[idx].path) - 1);
            if (errors.errors && errors.errors[0].message)
            {
                strncpy(issues[idx].error, errors.errors[0].message, sizeof(issues[idx].error) - 1);
            }
            else
            {
                issues[idx].error[0] = '\0';
            }
            has_error = 1;
        }
        if (found_it)
        {
            issues[idx].found_it = 1;
            has_error = 1;
        }
        if (found_op)
        {
            issues[idx].found_op = 1;
            has_error = 1;
        }
        if (found_sy)
        {
            issues[idx].found_sy = 1;
            has_error = 1;
        }

        free_deck(&deck);
        if (errors.errors)
        {
            free(errors.errors);
        }
    }
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <directory_or_filename> [output_report.md]\n", argv[0]);
        return 1;
    }
    const char *input_path = argv[1];
    char report_path[MAX_PATH] = "";
    bool write_report = (argc >= 3);
    struct stat st;
    if (stat(input_path, &st) == -1)
    {
        fprintf(stderr, "Error: Cannot access %s\n", input_path);
        return 2;
    }
    if (S_ISDIR(st.st_mode))
    {
        if (write_report)
        {
            snprintf(report_path, sizeof(report_path), "%s", argv[2]);
        }
        else
        {
            snprintf(report_path, sizeof(report_path), "%s/batch.md", input_path);
        }
        snprintf(sy_report_path, sizeof(sy_report_path), "%s/sy_cards_report.md", input_path);
    }
    else
    {
        char input_path_copy[MAX_PATH];
        strncpy(input_path_copy, input_path, sizeof(input_path_copy) - 1);
        input_path_copy[sizeof(input_path_copy) - 1] = '\0';
        char *dir = dirname(input_path_copy);
        if (write_report)
        {
            snprintf(report_path, sizeof(report_path), "%s", argv[2]);
        }
        else
        {
            snprintf(report_path, sizeof(report_path), "%s/batch.md", dir);
        }
        snprintf(sy_report_path, sizeof(sy_report_path), "%s/sy_cards_report.md", dir);
    }
    // Clear sy_cards_report.md at the start of each run
    FILE *sy_clear = fopen(sy_report_path, "w");
    if (sy_clear)
        fclose(sy_clear);
    if (stat(input_path, &st) == -1)
    {
        fprintf(stderr, "Error: Cannot access %s\n", input_path);
        return 2;
    }
    struct file_issue *issues = calloc(MAX_FILES, sizeof(struct file_issue));
    if (!issues)
    {
        fprintf(stderr, "Failed to allocate memory for issues array\n");
        return 2;
    }
    if (S_ISDIR(st.st_mode))
    {
        find_nec_files(input_path, test_nec_file, issues);
    }
    else
    {
        // Single file mode: only process if .nec extension
        size_t len = strlen(input_path);
        if (len > 4 && strcasecmp(input_path + len - 4, ".nec") == 0)
        {
            test_nec_file(input_path, issues);
            g_total_files_found = 1;
        }
        else
        {
            fprintf(stderr, "Error: File is not a .nec file: %s\n", input_path);
            free(issues);
            return 3;
        }
    }
    int files_with_errors = 0;
    int only_errors = 1;
    FILE *out = NULL;
    if (write_report)
    {
        out = fopen(report_path, "w");
        if (!out)
        {
            perror("fopen");
            free(issues);
            return 2;
        }
        fprintf(out, "# OpenNEC 4nec2 Deck Parsing Summary (Batch Test)\n\n");
    }
    for (int i = 0; i < MAX_FILES && issues[i].path[0]; ++i)
    {
        int has_error = 0;
        if (issues[i].error[0])
            has_error = 1;
        if (issues[i].found_it || issues[i].found_op || issues[i].found_sy)
            has_error = 1;
        if (only_errors && !has_error)
            continue;
        if (has_error)
            ++files_with_errors;
        if (write_report && out)
            fprintf(out, "- **%s**\n", issues[i].path);
        printf("- %s\n", issues[i].path);
        if (issues[i].error[0])
        {
            if (write_report && out)
                fprintf(out, "    - Error: %s\n", issues[i].error);
            printf("    - Error: %s\n", issues[i].error);
        }
        if (issues[i].found_it)
        {
            if (write_report && out)
                fprintf(out, "    - Contains IT card (unsupported)\n");
            printf("    - Contains IT card (unsupported)\n");
        }
        if (issues[i].found_op)
        {
            if (write_report && out)
                fprintf(out, "    - Contains OP card (unsupported)\n");
            printf("    - Contains OP card (unsupported)\n");
        }
    }
    if (write_report && out)
    {
        fprintf(out, "\n---\n");
        fprintf(out, "**Total files found:** %d\n", g_total_files_found);
        fprintf(out, "**Files with errors:** %d\n", files_with_errors);
        fclose(out);
        printf("Summary written to %s\n", report_path);
    }
    printf("Total files found: %d\n", g_total_files_found);
    printf("Files with errors: %d\n", files_with_errors);
    printf("\n==== Batch Test Complete ====\n");
    free(issues);
    return 0;
}
