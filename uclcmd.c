/*-
 * Copyright (c) 2014-2015 Allan Jude <allanjude@freebsd.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD$
 */

#include "uclcmd.h"

/*
 * XXX: TODO:
 * Error checking on strtoul etc
 * Does ucl_object_insert_key_common need to respect NO_IMPLICIT_ARRAY
 */

int debug = 0, expand = 0, mode = 0, noop = 0, nonewline = 0;
int show_keys = 0, show_raw = 0;
bool firstline = true, shvars = false;
int output_type = 254;
ucl_object_t *root_obj = NULL;
ucl_object_t *set_obj = NULL;
struct ucl_parser *parser = NULL;
struct ucl_parser *setparser = NULL;
char input_sepchar = '.';
char output_sepchar = '.';
const char *filename = NULL;
const char *outfile = NULL;
FILE *output = NULL;
char *include_file = NULL;

/*
 * This application provides a shell scripting friendly interface for reading
 * and writing UCL config files, using libUCL.
 *
 * http://xkcd.com/1513/
 *
 */
int
main(int argc, char *argv[])
{
    int ret = 0, i = 0;
    bool verbfound = false;
    verbmap_t cmdmap[] =
    {
	    { "get", get_main },
	    { "set", set_main },
	    { "merge", merge_main },
	    { "remove", remove_main },
	    { "del", remove_main },
	    { "dump", output_main },
	    { "help", (verb_func_t) usage },
	    { NULL, NULL }
    };

    if (argc < 2) {
	usage();
    }

    output = stdout;

    for (i = 0; cmdmap[i].verb; i++) {
	if (strcasecmp(cmdmap[i].verb, argv[1]) != 0)
	    continue;
	verbfound = true;
	/* Remove the verb */
	argv[1] = argv[0];
	argc--;
	argv++;
	/*
	for (ret = 0; ret < argc; ret++) {
	    printf("argv[%d] = %s\n", ret, argv[ret]);
	}
	*/
	if (cmdmap[i].callback != NULL) {
	    ret = cmdmap[i].callback(argc, argv);
	}
	break;
    }
    if (!verbfound) {
	/* unknown verb */
	usage();
    }
    
    return(ret);
}

void
usage()
{
    fprintf(stderr, "%s\n",
"Usage: uclcmd get [-cdejklmNquy] [-D char] [-f file] [-o file] variable\n"
"       uclcmd set [-cdjmnuy] [-t type] [-D char] [-f file] [-i file] [-o file] variable [UCL]\n"
"       uclcmd merge [-cdjmnuy] [-D char] [-f file] [-i file] [-o file] variable\n"
"       uclcmd remove [-cdjmnuy] [-D char] [-f file] [-o file] variable\n"
"\n"
"COMMON OPTIONS:\n"
"       -c --cjson      output compacted JSON\n"
"       -d --debug      enable verbose debugging output\n"
"       -D --delimiter  character to use as element delimiter (default is .)\n"
"       -e --expand     Output the list of keys when encountering an object\n"
"       -f --file       path to a file to read or write\n"
"       -j --json       output pretty JSON\n"
"       -k --keys       show key=value rather than just the value\n"
"       -l --shellvars  keys are output with underscores as delimiter\n"
"       -m --msgpack    output MSGPACK\n"
"       -n --noop       do not save changes to file, only output to STDOUT\n"
"       -N --nonewline  separate output with spaces rather than newlines\n"
"       -o --output     file to write output to, defaults to STDOUT\n"
"       -q --noquotes   do not enclose strings in quotes\n"
"       -t --type       make the new element this type\n"
"       -u --ucl        output universal config language\n"
"       -y --yaml       output YAML\n"
"       variable        The key of the variable to read, in object notation\n"
"       UCL             A block of UCL to be written to the specified variable\n"
"\n"
"GET OPTIONS:\n"
"\n"
"SET OPTIONS:\n"
"       -i --input      use indicated file as additional input (for combining)\n"
"\n"
"MERGE OPTIONS:\n"
"       -i --input      use indicated file as additional input (for merging)\n"
"\n"
"REMOVE OPTIONS:\n"
"\n"
"EXAMPLES:\n"
"       uclcmd get --file vmconfig .name\n"
"           \"value\"\n"
"\n"
"       uclcmd get --file vmconfig --keys --noquotes array.1.name\n"
"           array.1.name=value\n"
"\n"
"       uclcmd get --file vmconfig --keys --shellvars array.1.name\n"
"           array_1_name=\"value\"\n"
"\n");
    exit(1);
}

void
cleanup()
{
    if (parser != NULL) {
	ucl_parser_free(parser);
    }
    if (setparser != NULL) {
	ucl_parser_free(setparser);
    }
    if (root_obj != NULL) {
	ucl_object_unref(root_obj);
    }
    if (set_obj != NULL) {
	ucl_object_unref(set_obj);
    }
    if (nonewline) {
	fprintf(output, "\n");
    }
    if (output != stdout) {
	output_close(output);
    }
}

