//
// Created by pbeaujea on 1/22/16.
//

#include "CLAP.h"

enum {
    _FLAG_OPT_LONG = 1 << 0,
    _FLAG_OPT_POSITIONAL = 1 << 1,
    _FLAG_REQUIRE_NO_VAL = 1 << 2,
    _FLAG_LONG_OPT_WITH_VALUE = 1 << 3
};

ClapOption * clap_check_options(ClapOption *options) {
    /* Check that the type of option is allowed (so that the programmer have done his job correctly).
     *
     * Return NULL if OK.
     * Return the wrong argument otherwise.
     */

    ClapOption * initial = options;
    while(options->type != CLAP_OPT_END) {
        if (options->type > CLAP_OPT_SIZE || options->type < CLAP_OPT_END)
            return options;

        options++;
    }

    return NULL;
}

void clap_error(ClapHandler *handler, ClapOption *option, char *reason, int flags) {
    /* Print error related to a given option in stderr.
     */

    fputs("error: option ", stderr);
    if(flags & _FLAG_OPT_LONG)
        fprintf(stderr, "`--%s`", option->long_option);
    else if (flags & _FLAG_OPT_POSITIONAL)
        fprintf(stderr, "`%s`", option->long_option);
    else
        fprintf(stderr, "`-%c`", option->short_option);

    fprintf(stderr, " %s\n", reason);
}

ClapHandler *clap_handler_new(ClapOption *options, const char *name, const char *description, const char *epilog) {
    /* Create a new handler.
     *
     * Return NULL if `options` is NULL, if there is a memory allocation error or if the type of an option is not allowed.
     * Return a pointer to a initialized `ClapHandler` otherwise.
     */

    if (options == NULL)
        return NULL;

    ClapHandler * handler = malloc(sizeof(ClapHandler));
    if (handler == NULL)
        return NULL;

    ClapOption* n = clap_check_options(options);

    if (n != NULL) {
        clap_error(
                handler, n, "is of an unknown type", (n->short_option != 0) ? true : false);

        clap_handler_delete(handler);
        return NULL;
    }

    handler->options = options;
    handler->name = name;
    handler->description = description;
    handler->epilog = epilog;

    return handler;
}

int clap_handler_delete(ClapHandler *handler) {
    /* Free the memory for the handler. Note that you are responsible to clean the strings and the options, if any.
     *
     * Return -1 if handler is NULL.
     * Return 0 otherwise.
     */

    if (handler == NULL)
        return -1;

    free(handler);
    return 0;
}

int clap_option_set_value(ClapHandler *handler, ClapOption *option, int flags) {
    /* Set the value of the option to the variable (make string conversion if needed).
     * This function also extract the value from long option (`--option=value`) if any.
     *
     * Return -3 if an error happen.
     * Return the value of the callback function, if there is a callback to call.
     * Return 0 otherwise.
     */

    if(!(flags & _FLAG_OPT_POSITIONAL) && option->flags & CLAP_FLAG_POSITIONAL) {
        clap_error(handler, option, "is positional, so should not be used with `--`", _FLAG_OPT_POSITIONAL);
        return -3;
    }

    if (option->type == CLAP_OPT_BOOL) {
        *(bool*)option->ptr_value = ! *(bool*)option->ptr_value;
    } else if (option->type != CLAP_OPT_NONE) {
        char* next_arg = *(handler->argv-1); // if positional, the value is the argument

        if (flags & _FLAG_LONG_OPT_WITH_VALUE) { // the value is right after the option
            next_arg = strchr(next_arg, '=') + 1;
        }

        else if (!(option->flags & CLAP_FLAG_POSITIONAL)) { // if not positional, the value is the next argument
            next_arg = *(handler->argv++);
            if (next_arg == NULL) {
                clap_error(handler, option, "requires a value", flags);
                return -3;
            }
        }

        char* end = NULL;
        char* tmp_ = NULL;

        switch (option->type) {
            case CLAP_OPT_STRING:
                *(char**)option->ptr_value = next_arg; // this is pointer manipulation, so don't free() !!
                break;
            case CLAP_OPT_INT:
                *(int*)option->ptr_value = (int) strtol(next_arg, &end, 10);
                if (end[0] != '\0') {
                    clap_error(handler, option, "expects a numerical value", flags);
                    return -3;
                }
                break;
            case CLAP_OPT_DOUBLE:
                *(double*)option->ptr_value = strtod(next_arg, &end);
                if (end[0] != '\0') {
                    clap_error(handler, option, "expects a numerical value", flags);
                    return -3;
                }
                break;
            case CLAP_OPT_FILE_R:
            case CLAP_OPT_FILE_W:
                *(FILE**)option->ptr_value = fopen(next_arg, (option->type == CLAP_OPT_FILE_W) ? "w" : "r");
                if (*(FILE**)option->ptr_value == NULL) {
                    char* __UTO1 = "can't open `";
                    char* __UTO2 = "`";
                    size_t i = strlen(__UTO1), j = strlen(next_arg), k = strlen(__UTO2);
                    tmp_ = malloc(i+j+k);
                    memcpy(tmp_, __UTO1, i);
                    memcpy(tmp_+i, next_arg, j);
                    memcpy(tmp_+i+j, __UTO2, k);
                    clap_error(handler, option, tmp_, flags);
                    free(tmp_);
                    return -3;
                }
                break;
            default:
                clap_error(handler, option, "'s type is not (yet?) supported", flags);
                return -3;
        }
    }

    option->flags |= CLAP_FLAG_SET; // to check mandatory field later

    // callbacking if any !
    if(option->flags & CLAP_FLAG_CALLBACK)
        return option->callback(handler, option);

    return 0;
}

int clap_option_short(ClapHandler *handler, char opt, int flags) {
    /* Browse all the options and match short option, if exists.
     *
     * Return -2 if the short option is unknown or not bundable (because it expects a value) but bundled.
     * Return the value of `clap_option_set_value()` otherwise.
     */

    ClapOption * options = handler->options;

    while(options->type != CLAP_OPT_END) {
        if (options->type != CLAP_OPT_GROUP) {
            if (options->short_option == opt) {
                if ((flags & _FLAG_REQUIRE_NO_VAL) && (options-> type != CLAP_OPT_BOOL || options->type == CLAP_OPT_NONE)) {
                    clap_error(handler, options, "is not bundlable", 0);
                    return -2;
                }

                return clap_option_set_value(handler, options, false);
            }
        }

        options++;
    }

    fprintf(stderr, "error: unknown option `-%c`\n", opt);
    return -2;
}

int clap_option_positional(ClapHandler *handler, char *opt, int flags) {
    /* Browse all the options and match positional option, if exists. Positional option can only be set once.
     *
     * Return -2 if positional option is unknown.
     * Return the value of `clap_option_set_value()` otherwise.
     */

    ClapOption * options = handler->options;

    while(options->type != CLAP_OPT_END) {

        if (options->type != CLAP_OPT_GROUP) {
            if (options->flags & CLAP_FLAG_POSITIONAL && !(options->flags & CLAP_FLAG_SET)) {
                return clap_option_set_value(handler, options, flags | _FLAG_OPT_POSITIONAL);
            }
        }

        options++;
    }

    fprintf(stderr, "error: no positional arguments to match with `%s`\n", opt);
    return -2;
}

int strcmp_long_option(const char *str, const char *str_with_equals) {
    /* Little tweak function to stop comparison if there is an `=` in the option. Useful for long options.
     *
     * Return 0 if the two string matches.
     * Return n>0 if the first character which is different is larger in `str`, n<0 otherwise.
     */

    while(*str != '\0' && *str_with_equals != '\0' && *str_with_equals != '=') {
        if(*str != *str_with_equals)
            return (*str < *str_with_equals) ? -*str : *str_with_equals;

        str++;
        str_with_equals++;
    }

    if(*str == '\0' && (*str_with_equals == '\0' || *str_with_equals == '='))
        return 0;
    else if (*str == 0)
        return *str_with_equals;
    else if (*str_with_equals == 0)
        return *str;
    else
        return (*str < *str_with_equals) ? -*str : *str_with_equals;
}

int clap_option_long(ClapHandler *handler, char *opt, int flags) {
    /* Browse all the options and match long option, if exists.
     * This function recognize if the option contains the value, as `--option=value`.
     *
     * Return -2 if positional option is unknown.
     * Return the value of `clap_option_set_value()` otherwise.
     */

    ClapOption * options = handler->options;

    // check if the option have `=` in it
    char* equals = strchr(opt, '=');
    if (equals != NULL) {
        flags |= _FLAG_LONG_OPT_WITH_VALUE;
    }

    while(options->type != CLAP_OPT_END) {
        if (options->type != CLAP_OPT_GROUP) {
            if (strcmp_long_option(options->long_option, opt) == 0) {
                return clap_option_set_value(handler, options, flags | _FLAG_OPT_LONG);
            }
        }

        options++;
    }

    fprintf(stderr, "error: unknown option `--%s`\n", opt);
    return -2;
}

int clap_parse(ClapHandler *handler, int argc, char **argv) {
    /* Browse through `argv` to find options.
     *
     * Return -1 if the option is too short (namely, only `-` of `--`) or an argument is mandatory, but not set.
     * Return the value of `clap_option_*()` otherwise.
     */

    handler->argc = argc;
    handler->argv = argv+1;
    int err = 0;

    if (argc > 1) {
        while(*(handler->argv) != NULL) {
            char* arg = *(handler->argv++);

            size_t length = strlen(arg);

            if(arg[0] != '-') { // this is positional
                err = clap_option_positional(handler, arg, 0);

                if (err)
                    return err;
            }

            else if (length < 2) {
                fprintf(stderr, "error: option `%s` is too short\n", arg);
                return -1;
            }

            else if(arg[0] == '-' && arg[1] != '-') { // this is short option

                if (length == 2) {
                    err = clap_option_short(handler, *(arg + 1), 0);

                    if(err)
                        return err;
                }

                else {
                    for (int i = 1; i < length; ++i) {
                        err = clap_option_short(handler, *(arg + i), _FLAG_REQUIRE_NO_VAL);

                        if(err)
                            return err;
                    }
                }
            }

            else if (length < 3) {
                fprintf(stderr, "error: option `%s` is too short\n", arg);
                return -1;
            }

            else { // this is long option
                err = clap_option_long(handler, arg + 2, 0);

                if (err)
                    return err;
            }
        }
    }

    // mandatory field check
    ClapOption * options = handler->options;

    while(options->type != CLAP_OPT_END) {
        if (options->flags & CLAP_FLAG_MANDATORY && (options->flags & CLAP_FLAG_SET) == 0) {
            clap_error(handler, options, "is mandatory but not set", 0);
            return -1;
        }

        options++;
    }

    return 0;
}


int clap_help_cb(ClapHandler *handler, ClapOption *option) {
    /* Callback to call the `clap_usage()` function when `CLAP_HELP()` (`-h`) is used.
     *
     * Return -4.
     */

    clap_usage(handler);
    return -4;
}

void clap_print_option_in_list(ClapOption *option) {
    /* List the mandatory and positional options as one line.
     */

    if (! (option->flags & CLAP_FLAG_MANDATORY))
        fputc('[', stdout);

    bool is_short_option = false;
    bool is_positional = false;

    if (option->flags & CLAP_FLAG_POSITIONAL)
        is_positional = true;
    else if (option->short_option != '\0')
        is_short_option = true;

    if (is_positional) {
        fprintf(stdout, "%s", option->long_option);
    }

    else {
        fputc('-', stdout);

        if(is_short_option) {
            fputc(option->short_option, stdout);
            fputc(' ', stdout);
        }
        else
            fprintf(stdout, "-%s=", option->long_option);
    }

    // type
    if (!is_positional){
        switch (option->type) {
            case CLAP_OPT_INT:
                fprintf(stdout, "<int>");
                break;
            case CLAP_OPT_DOUBLE:
                fprintf(stdout, "<float>");
                break;
            case CLAP_OPT_STRING:
                fprintf(stdout, "<str>");
                break;
            case CLAP_OPT_FILE_R:
            case CLAP_OPT_FILE_W:
                fprintf(stdout, "<path>");
            default:
                break;
        }
    }

    if (! (option->flags & CLAP_FLAG_MANDATORY))
        fputc(']', stdout);
}


void pad_out(size_t n) {
    for(size_t i = n; i > 0; --i) {
        fputc(' ', stdout);
    }
}

void clap_print_option(ClapOption *option) {
    /* Print an option, depending whether it is short, long, positional, expect value, ...
     */

    fputs("    ", stdout);
    size_t len = 4;

    if (option->flags & CLAP_FLAG_POSITIONAL) {
        len += strlen(option->long_option);
        fputs(option->long_option, stdout);
    }

    else {
        if (option->short_option != '\0') {
            fprintf(stdout, "-%c", option->short_option);

            if(option->long_option != NULL)
                fputs(", ", stdout);
            else
                fputc(' ', stdout);
        }
        else {
            fputs("    ", stdout);
        }

        len = 8;

        if (option->long_option != NULL) {
            len += strlen(option->long_option)+2;
            fprintf(stdout, "--%s", option->long_option);
        }

        if (option->type != CLAP_OPT_BOOL && option->type != CLAP_OPT_NONE) {
            // type
            if (option->long_option != NULL) {
                fputc('=', stdout);
                len ++;
            }

            switch (option->type) {
                case CLAP_OPT_INT:
                    fputs("INTEGER", stdout);
                    len += 7;
                    break;
                case CLAP_OPT_DOUBLE:
                    fputs("FLOAT", stdout);
                    len += 5;
                    break;
                case CLAP_OPT_STRING:
                    fputs("STRING", stdout);
                    len += 6;
                    break;
                case CLAP_OPT_FILE_R:
                case CLAP_OPT_FILE_W:
                    fputs("PATH", stdout);
                    len += 4;
                default:
                    break;
            }
        }
    }

    fputs("    ", stdout); // some additional padding
    len +=4;

    if (option->help != NULL) {
        if (len <= CLAP_MIN_OPTIONS_WIDTH)
            pad_out(CLAP_MIN_OPTIONS_WIDTH - len);
        else {
            fputc('\n', stdout);
            pad_out(CLAP_MIN_OPTIONS_WIDTH);
        }

        fputs(option->help, stdout);
    }


}

void clap_usage(ClapHandler *handler) {
    /* Show usage.
     */

    fprintf(stdout, "Usage: ");
    const char* name = "program";
    if(handler->name)
        name = handler->name;

    fprintf(stdout, "%s [OPTION]... ", name);

    // then print mandatory and positional options
    ClapOption * options = handler->options;
    size_t num_args = 0;
    size_t positionals = 0;

    while(options->type != CLAP_OPT_END) {
        if (options->flags & CLAP_FLAG_MANDATORY || options->flags & CLAP_FLAG_POSITIONAL) {
            clap_print_option_in_list(options);
            fputc(' ', stdout);

            if (options->flags & CLAP_FLAG_POSITIONAL)
                positionals++;
        }

        num_args++;
        options++;
    }

    fprintf(stdout, "\n");

    if(handler->description)
        fprintf(stdout, "\n%s\n\n", handler->description);

    // first positionnals
    if(positionals) {
        fputs("Positional arguments:\n", stdout);
        options = handler->options;
        while(options->type != CLAP_OPT_END) {
            if (options->type != CLAP_OPT_GROUP && options->flags & CLAP_FLAG_POSITIONAL) {
                clap_print_option(options);
                fputc('\n', stdout);
                positionals += 1;
            }
            options++;
        }
    }


    // then, others
    if (positionals != num_args) { // if there is still lefts

        if (positionals)
            fputs("Other arguments:\n", stdout);
        else
            fputs("Arguments:\n", stdout);

        options = handler->options;
        while(options->type != CLAP_OPT_END) {
            if (!(options->flags & CLAP_FLAG_POSITIONAL)) {
                if (options->type != CLAP_OPT_GROUP)
                    clap_print_option(options);
                else
                    fprintf(stdout, "%s:", options->help);

                fputc('\n', stdout);
            }

            options++;
        }
    }


    if(handler->epilog)
        fprintf(stdout, "\n%s\n", handler->epilog);
}
