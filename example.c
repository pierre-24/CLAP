#include "CLAP.h"
#include <stdio.h>
#include <stdlib.h>

int a_callback(ClapHandler* handler, ClapOption* opt) {
    char* string =  *((char**) opt->ptr_value);
    printf("The callback was called, and the value is `%s`.\n", string);
    return 0;
}

int main(int argc, char** argv) {
    FILE* a_file = NULL;
    char* a_string = NULL;
    double threshold = 10;
    bool verbose = false;
    bool test = true;

    ClapOption options[] = {
            CLAP_HELP(),
            CLAP_FILE_R(0, "file", &a_file, "Read a file", CLAP_FLAG_POSITIONAL),
            CLAP_DOUBLE('e', "threshold", &threshold, "thresholding", CLAP_FLAG_MANDATORY),
            CLAP_STRING('c', "callback", &a_string, "call the callback with a value", CLAP_FLAG_CALLBACK, a_callback),
            CLAP_GROUP("Some test options"),
            CLAP_BOOLEAN('v', "verbose", &verbose, "talk more !", 0),
            CLAP_BOOLEAN('t', "test", &test, "play with boolean !", 0),
            CLAP_END()
    };

    ClapHandler *handler = clap_handler_new(
            options,
            "program_name",
            "A description of the program",
            "An epilog (e.g. copyrighting, author ...)"
    );

    if (handler == NULL) {
        return EXIT_FAILURE;
    }

    if (clap_parse(handler, argc, argv) != 0) {
        return EXIT_FAILURE;
    }

    clap_handler_delete(handler);

    /* dealing with options: */
    if (a_file != NULL)
        printf("File can be read !\n");

    printf("Threshold is set to %.3f\n", threshold);

    if (verbose)  {
        printf("Something else !\n");
    }

    printf("Variable test is %s !\n", (test) ? "true" : "false");
}