#include "ui.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

static const char *const options[] = {
    "Load Documents", "Build Index", "Search", "Statistics",
    "View Document", "Save Report", "Benchmark", "Exit"
};

enum { OPTION_COUNT = sizeof options / sizeof options[0] };

static void show_menu(void)
{
    puts("\nDocument Search and Information Retrieval");
    for (int i = 0; i < OPTION_COUNT; ++i) {
        printf("%d. %s\n", i + 1, options[i]);
    }
    printf("Choose an option (1-%d): ", OPTION_COUNT);
    fflush(stdout);
}

static int read_choice(void)
{
    char line[128];
    if (fgets(line, sizeof line, stdin) == NULL) {
        return EOF;
    }

    if (strchr(line, '\n') == NULL && !feof(stdin)) {
        int ch;
        while ((ch = getchar()) != '\n' && ch != EOF) {
        }
        return 0;
    }

    const char *p = line;
    while (isspace((unsigned char)*p)) {
        ++p;
    }
    if (*p < '1' || *p > '0' + OPTION_COUNT) {
        return 0;
    }
    int choice = *p++ - '0';
    while (isspace((unsigned char)*p)) {
        ++p;
    }
    return *p == '\0' ? choice : 0;
}

void run_cli_loop(void)
{
    for (;;) {
        show_menu();
        int choice = read_choice();
        if (choice == EOF) {
            if (ferror(stdin)) {
                fputs("Error reading menu input.\n", stderr);
            }
            return;
        }
        if (choice == OPTION_COUNT) {
            puts("Goodbye.");
            return;
        }
        if (choice == 0) {
            puts("Invalid input. Enter one menu number from 1 to 8.");
            continue;
        }
        printf("%s: integration pending.\n", options[choice - 1]);
    }
}
