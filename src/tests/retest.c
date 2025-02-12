#include <assert.h>
#include <ctype.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "dfa.h"
#include "nfa.h"

static const int MAX_CAPTURES = 20;

static void usage() {
    fprintf(stderr, "usage: echo 'data' | retest [-adli] [-t std|min] <patterns>\n");
    exit(1);
}

static void print_error(int rc, regex_t* re, const char* prefix) {
    char buffer[128];
    regerror(rc, re, buffer, sizeof(buffer));
    fprintf(stderr, "%s, %s\n\n", prefix, buffer);
}

static void print_groups(regmatch_t* pmatch) {
    fputc('\n', stderr);

    for ( int i = 0; i < MAX_CAPTURES; i++ ) {
        if ( pmatch[i].rm_so != -1 )
            fprintf(stderr, "  capture group #%d: (%d,%d)\n", i, pmatch[i].rm_so, pmatch[i].rm_eo);
    }

    fputc('\n', stderr);
}

typedef struct {
    int length;
    int cflags;
    jrx_accept_id id;
} Flags;

static Flags parse_flags(const char* expr) {
    Flags flags = {-1, 0, 0};

    const char* p = expr;
    while ( (p = strstr(p, "{#")) ) {
        char* q = strchr(p, '}');
        if ( ! q ) {
            fprintf(stderr, "missing '}'\n");
            exit(1);
        }

        if ( flags.length < 0 )
            flags.length = p - expr;

        p += 2;

        if ( isdigit(*p) )
            flags.id = atoi(p);
        else {
            fprintf(stderr, "invalid flags\n");
            exit(1);
        }

        p = q + 1;
    };

    if ( flags.length < 0 )
        flags.length = strlen(expr);

    return flags;
}

// Uses standard POSIX API to match against a single regular expression.
// Removes, but otherwise ignores, any inline flags. Returns true if match is
// found, false if not.
static int match_single(const char* regexp, int options, const char* data) {
    fprintf(stderr, "--- Using single pattern API\n\n");

    Flags flags = parse_flags(regexp);

    char buffer[flags.length + 1];
    strncpy(buffer, regexp, flags.length);
    buffer[flags.length] = '\0';

    regex_t re;
    int rc = regcomp(&re, buffer, REG_EXTENDED | options | flags.cflags);
    if ( rc != 0 ) {
        print_error(rc, &re, "compile error");
        return 0;
    }

    regmatch_t pmatch[MAX_CAPTURES];
    rc = regexec(&re, data, MAX_CAPTURES, pmatch, 0);
    if ( rc != 0 ) {
        print_error(rc, &re, "pattern not found");
        regfree(&re);
        return 0;
    }

    fprintf(stderr, "match found!\n");
    print_groups(pmatch);

    regfree(&re);
    return 1;
}

// Uses group matching API. Applies any inline flags. Returns true if match is
// found, false if not. Sets id iff match is found.
static int match_group(char** regexps, int num_regexps, int options, const char* data, jrx_accept_id* id) {
    fprintf(stderr, "--- Using group matching API\n");

    regex_t re;
    jrx_regset_init(&re, -1, REG_EXTENDED | options);

    for ( int i = 0; i < num_regexps; i++ ) {
        Flags flags = parse_flags(regexps[i]);
        int rc = jrx_regset_add2(&re, regexps[i], flags.length, flags.cflags, flags.id);
        if ( rc != 0 ) {
            print_error(rc, &re, "parse error");
            regfree(&re);
            return 0;
        }
    }

    int rc = jrx_regset_finalize(&re);
    if ( rc != 0 ) {
        print_error(rc, &re, "compile error");
        return 0;
    }

    regmatch_t pmatch[MAX_CAPTURES];
    rc = jrx_regexec2(&re, data, MAX_CAPTURES, pmatch, 0, id);
    if ( rc != 0 ) {
        print_error(rc, &re, "pattern not found");
        regfree(&re);
        return 0;
    }

    if ( id )
        fprintf(stderr, "match found! (id %d)\n", *id);
    else
        fprintf(stderr, "match found!\n");

    print_groups(pmatch);

    regfree(&re);
    return 1;
}


// Branches into either single or group matching, depending on the number of
// patterns we have. Returns true if match is found, false if not. Sets `id`
// iff match is found. In group mode, sets it to the id of the matching group.
// In single mode, sets it to 1.
int match(char** regexps, int num_regexps, int opt, int options, const char* data, jrx_accept_id* id) {
    assert(num_regexps > 0);

    if ( num_regexps == 1 ) {
        int rc = match_single(regexps[0], options, data);
        if ( rc && id )
            *id = 1;

        return rc;
    }
    else
        return match_group(regexps, num_regexps, options, data, id);
}

char* read_input() {
    const int chunk = 5;
    char* buffer = 0;
    int i = 0;

    while ( 1 ) {
        buffer = realloc(buffer, (chunk * ++i) + 1);
        if ( ! buffer ) {
            fprintf(stderr, "cannot alloc\n");
            exit(1);
        }

        char* p = buffer + (chunk * (i - 1));
        size_t n = fread(p, 1, chunk, stdin);
        *(p + chunk) = '\0';

        if ( feof(stdin) )
            break;

        if ( ferror(stdin) ) {
            fprintf(stderr, "error while reading from stdin\n");
            exit(1);
        }
    }

    return buffer;
}

typedef enum { TEST_DEFAULT, TEST_STD, TEST_MIN } TestMode;

int main(int argc, char** argv) {
    int opt = 1;
    int cflags = 0;
    TestMode test_mode = TEST_DEFAULT;

    int i;
    const char* d;

    // Parse command line options with getopt().
    char c;
    while ( (c = getopt(argc, argv, "adlt:")) != -1 ) {
        switch ( c ) {
            case 'a': cflags |= REG_ANCHOR; break;
            case 'd': cflags |= REG_DEBUG; break;
            case 'l': cflags |= REG_LAZY; break;

            case 't': {
                if ( strcmp(optarg, "std") == 0 )
                    test_mode = TEST_STD;
                else if ( strcmp(optarg, "min") == 0 )
                    test_mode = TEST_MIN;
                else {
                    fprintf(stderr, "invalid test mode: %s\n", optarg);
                    return 1;
                }
                break;
            }

            case '?':
            default: usage();
        }
    }

    argc -= optind;
    argv += optind;

    if ( argc < 1 )
        usage();

    char** regexps = argv;
    int num_regexps = argc;
    const char* data = (const char*)read_input();

    fprintf(stderr, "=== Pattern: %s\n", regexps[0]);

    for ( int i = 1; i < num_regexps; i++ )
        fprintf(stderr, "             %s\n", regexps[i]);

    fputs("=== Data   : ", stderr);

    for ( d = data; *d; d++ ) {
        if ( isprint(*d) )
            fputc(*d, stderr);
        else
            fprintf(stderr, "\\x%02x", *d);
    }

    fputs("\n", stderr);

    switch ( test_mode ) {
        case TEST_DEFAULT: {
            jrx_accept_id id = 0;
            fprintf(stderr, "\n=== Standard matcher with subgroups\n");
            match(regexps, num_regexps, opt, cflags, data, &id);

            id = 0;
            fprintf(stderr, "=== Minimal matcher\n");
            match(regexps, num_regexps, opt, cflags | REG_NOSUB, data, &id);
            return 0;
        }

        case TEST_STD: {
            fprintf(stderr, "\n=== Standard matcher with subgroups\n");
            jrx_accept_id id = 0;
            if ( match(regexps, num_regexps, opt, cflags, data, &id) )
                exit(id);
            else
                exit(0);
        }

        case TEST_MIN: {
            fprintf(stderr, "\n=== Minimal matcher\n");
            jrx_accept_id id = 0;
            if ( match(regexps, num_regexps, opt, cflags | REG_NOSUB, data, &id) )
                exit(id);
            else
                exit(0);
        }
    }
}
