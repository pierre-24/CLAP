# Name

Command line arguments parsing (CLAP) library, written in iso-C99 (and C++ compatible).

# Short description

This module is inspired by [`ArgP`](http://www.gnu.org/software/libc/manual/html_node/Argp.html) (which is GNU C), 
the [python `argparse` module](https://docs.python.org/3/library/argparse.html) and, 
for the code, by the [`argparse` libray of CofyC](https://github.com/Cofyc/argparse) with a few additions and modifications.

Indeed, `getopt` is quite difficult to use, and GNU C (so is ArgP), which is not always available. The goal is to provide high-level library
for command line argument parsing, in the spirit of the awesome python `argparse` module. CLAP provides help and usage messages. It takes care 
of recognising arguments and values from `argv`. It delivers errors if any.

# Features

+ Handles long (starting with two dashes, e.g. `--something`), short (one dash and one alphanumeric character, e.g. `-x`) and positional arguments () ; 
+ Issues errors messages when arguments are invalid ;
+ Options are case-sensitive.
+ Short options are bundlable (e.g. `-a -b` can be given as `-ab`) ;
+ Convert strings into other types (boolean, float, integers) ;
+ Basic file management available ;
+ Callbacking is possible ;
+ An option can be set mandatory.

# How to use CLAP ?

You first need `CLAP.c` and `CLAP.h` (maybe you should consider [adding a submodule](https://git-scm.com/docs/git-submodule)).
And, of course, you should include the header:

```c
#include "CLAP.h"
```

## Define the options


A `ClapOption`'s array need to be construct. Syntax may be, for example,

```c
double threshold = 10;
bool verbose = false;

ClapOption options[] = {
        CLAP_HELP(),
        CLAP_DOUBLE('e', "threshold", &threshold, "Threshold", CLAP_FLAG_MANDATORY),
        CLAP_BOOLEAN('v', "verbose", &verbose, "Talk more !", 0),
        CLAP_END()
};
```

You **have to** end the array with `CLAP_END()`. You can provide as many argument as you want, using some macros. Most of them have this form:

```c
CLAP_BOOLEAN(short, long, pointer, help, flags [, callback])
```

where,

+ `short` (`char`) is the alphanumeric character for short option (`-x`). Set to 0 if you don't want a short option.
+ `long` (`const char*`) is the string for long option (`--long`). Set to `NULL` if you don't want a long option.
+ `pointer` (`void*`) is a pointer to the variable which has to be modified is the option is used.
+ `help` (`const char*`) is a text printed when usage is requested.
+ `flags` (`unsigned int`) is a variable for flagging (see below). Set to 0 if there is no flags.
+ `callback` (`int fun(ClapHandler* h, ClapOption* c)`)  is a pointer to a callback function (see below). 
    If there is no callback, you may not set this option. If there is a callback, set the `CLAP_FLAG_CALLBACK` flag.
 
The following macros are defined:

+ `CLAP_INTEGER(...)`: integer option, converted to `int`.
+ `CLAP_DOUBLE(...)`: floatting option, converted to `double`.
+ `CLAP_STRING(...)`: string option, left as `char*`. Note that the string is not copied, so **the pointer is set to one of `argv` string**.
+ `CLAP_FILE_R(...)`: the path given in argument is opened with `fopen(.., 'r')`, converted to `FILE*`.
+ `CLAP_FILE_W(...)`:  the path given in argument is opened with `fopen(.., 'w')`, converted to `FILE*`.
+ `CLAP_BOOLEAN(...)`: **this option does not set the variable**, it reverses the value when used (e.g. set it to `false` if `true`).

Flags can be:

+ `CLAP_FLAG_MANDATORY`: the option is mandatory, it must be set, otherwise there is an error issued.
+ `CLAP_FLAG_CALLBACK`; the option calls a callback function at the end. 
+ `CLAP_FLAG_POSITIONAL`: this option is positional. In the macro, you need to set `short` to 0, while `long` is used as the name of the option in *usage*.

Additionnaly, there is 3 more macros you can use:

+ `CLAP_GROUP(h)`: option below are in a new group when printed in the *usage*.
+ `CLAP_HELP()`: set `-h` and `--help` to call the *usage*.
+ `CLAP_END()`: signal the end of the array. Options given after that macro are ignored.

## Parsing

Then, use:

```c
ClapHandler *handler = clap_handler_new(
    options,
    "program_name",
    "A description of the programm",
    "An epilog (e.g. copyrigthing, author ...)"
);

if (handler == NULL) {
    return EXIT_FAILURE;
}

if (clap_parse(handler, argc, argv) != 0) {
    return EXIT_FAILURE;
}
```

where `clap_handler_new()` is the function to initialize the parser, which returns `NULL` if there is a mistake in the array of options that you provide.

The `clap_parse()` function finally perform the job. Its return value depends where the error occured:

+ `-1`: error happens in the level of parsing. Most of the time, because a mandatory argument is not set.
+ `-2` error happens in the level of recognition. The option probably does not exists.
+ `-3`: error happens in the level of setting. The value is probably not what expected.
+ `-4`: this is not an error, but the help was requested, so parsing stops.
+ Your callbacks can also return errors (see below).

## Cleaning

When done, simply use

```c
clap_handler_delete(handler);
```

Note that the option array and string are not `free()`'d, you are responsible for that.

## Callbacks

Here is an example of callback use:

```c
#include "CLAP.h"
#include <stdio.h>
#include <stdlib.h>

int a_callback(ClapHandler* handler, ClapOption* opt) {
    char* string =  *((char**) opt->ptr_value);
    printf("The callback was called, and the value is `%s`.\n", string);
    return 0;
}

int main(int argc, char** argv) {
    char* a_string = NULL;

    ClapOption options[] = {
            CLAP_HELP(),
            CLAP_STRING('c', "callback", &a_string, "Call the callback", CLAP_FLAG_CALLBACK, a_callback),
            CLAP_END()
    };

    ClapHandler *handler = clap_handler_new(
            options,
            "program_name",
            "A description of the programm",
            "An epilog (e.g. copyrigthing, author ...)"
    );

    if (handler == NULL) {
        return EXIT_FAILURE;
    }

    if (clap_parse(handler, argc, argv) != 0) {
        return EXIT_FAILURE;
    }

    clap_handler_delete(handler);
}
```

Notice, in the `CLAP_STRING()` macro, the `CLAP_FLAG_CALLBACK` flag (if you don't set it, the callback is not called) and then the function pointer. Also, the signature of the callback function must be

```
int a_callback(ClapHandler* handler, ClapOption* opt)
```

The return value must be `0` if you want the program to continue, otherwise it wilkl stop (return nothing else but 0 if there is an error, for example).

# Example

See the code in [example.c](./example.c). Compilation is performed by using `cmake && make`.

Here is some outputs:

```
~ $ ./CLAP -h  ## usage
Usage: program_name [OPTION]... [file] -e <float> 

A description of the program

Positional arguments:
    file                      Read a file
Other arguments:
    -h, --help                show this help message and exit
    -e, --threshold=FLOAT     thresholding
    -c, --callback=STRING     call the callback with a value
Some test options:
    -v, --verbose             talk more !
    -t, --test                play with boolean !

An epilog (e.g. copyrighting, author ...)
```

```
~ $ ./CLAP 
error: option `-e` is mandatory but not set
```

```
~ $ ./CLAP -e 1
Threshold is set to 1.000
Variable test is true !
```

```
~ $ ./CLAP -e 1 -v -t
Threshold is set to 1.000
Something else !
Variable test is false !
```

```
~ $ ./CLAP -e 1 -vt  ## short option are bundlable
Threshold is set to 1.000
Something else !
Variable test is false !
```

```
~ $ ./CLAP example.c -e 1 -vt   ## positional arguments
File can be read !
Threshold is set to 1.000
Something else !
Variable test is false !
```

```
~ $ ./CLAP invalid_file -e 1 -vt  ## invalid file is invalid
error: option `file` can't open `invalid_file`
```