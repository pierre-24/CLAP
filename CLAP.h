//
// Created by pbeaujea on 1/22/16.
//

#ifndef INCLUDE_CLAP_H
#define INCLUDE_CLAP_H

#ifdef __cplusplus
extern "C" {
#endif

#define CLAP_MIN_OPTIONS_WIDTH 30

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

/* Inspired by "argparse" of Cofyc: https://github.com/Cofyc/argparse/ */

typedef enum ClapOptType_ {
    // special
    CLAP_OPT_END,
    CLAP_OPT_GROUP,
    // basic types
    CLAP_OPT_NONE, // may be useful for callbacks
    CLAP_OPT_INT,
    CLAP_OPT_DOUBLE,
    CLAP_OPT_STRING,
    CLAP_OPT_BOOL,
    // files
    CLAP_OPT_FILE_R,
    CLAP_OPT_FILE_W,

    CLAP_OPT_SIZE
} ClapOptType;

typedef enum ClapFlags_ {
    CLAP_FLAG_SET = 1 << 0,
    CLAP_FLAG_MANDATORY = 1 << 1,
    CLAP_FLAG_CALLBACK = 1 << 2,
    CLAP_FLAG_POSITIONAL = 1 << 3
} ClapFlags;

struct ClapHandler_;
struct ClapOption_;

typedef int argparser_callback (struct ClapHandler_ * handler, struct ClapOption_ * option);

typedef struct ClapOption_ {
    ClapOptType type;
    char short_option;
    const char* long_option;
    void* ptr_value;
    const char* help;
    int flags;
    argparser_callback* callback;
} ClapOption;

typedef struct ClapHandler_ {
    ClapOption * options;
    const char* name;
    const char* description;
    const char* epilog; // description at the end, e.g. copyrighting, mail ...

    // internal stuffs
    int argc;
    char** argv;
} ClapHandler;

ClapHandler* clap_handler_new(ClapOption *options, const char *name, const char *description, const char *epilog);
int clap_handler_delete(ClapHandler *handler);

int clap_parse(ClapHandler *handler, int argc, char **argv);
void clap_usage(ClapHandler *handler);
int clap_help_cb(ClapHandler *handler, ClapOption *option);

#define CLAP_END()        { CLAP_OPT_END, 0, NULL, NULL, NULL, 0}
#define CLAP_BOOLEAN(...) { CLAP_OPT_BOOL, __VA_ARGS__ }
#define CLAP_INTEGER(...) { CLAP_OPT_INT, __VA_ARGS__ }
#define CLAP_DOUBLE(...) { CLAP_OPT_DOUBLE, __VA_ARGS__ }
#define CLAP_STRING(...)  { CLAP_OPT_STRING, __VA_ARGS__ }
#define CLAP_FILE_R(...)  { CLAP_OPT_FILE_R, __VA_ARGS__ }
#define CLAP_FILE_W(...)  { CLAP_OPT_FILE_W, __VA_ARGS__ }
#define CLAP_GROUP(h)     { CLAP_OPT_GROUP, 0, NULL, NULL, h, 0}
#define CLAP_HELP() { CLAP_OPT_NONE, 'h', "help", NULL, "show this help message and exit", CLAP_FLAG_CALLBACK, clap_help_cb}

#ifdef __cplusplus
}
#endif

#endif // INCLUDE_CLAP_H
