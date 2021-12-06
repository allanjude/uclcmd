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

int
remove_main(int argc, char *argv[])
{
    int ret = 0, k = 0, ch;
    ucl_object_t *obj_parent = NULL, *obj_child = NULL, *obj_temp = NULL;
    bool success = false;

    /* When removing, don't expand macros */
    pflags |= UCL_PARSER_DISABLE_MACRO;

    /* Set the default output type */
    output_type = UCL_EMIT_CONFIG;

    /*	options	descriptor */
    static struct option longopts[] = {
	{ "cjson",	no_argument,		&output_type,
	    UCL_EMIT_JSON_COMPACT },
	{ "debug",	optional_argument,	NULL,		'd' },
	{ "delimiter",	required_argument,	NULL,		'D' },
	{ "expand",	no_argument,		&expand,	1 },
	{ "file",	required_argument,	NULL,		'f' },
	{ "json",	no_argument,		&output_type,
	    UCL_EMIT_JSON },
	{ "keys",	no_argument,		&show_keys,	1 },
	{ "foldcase",	no_argument,		NULL,		'I' },
	{ "msgpack",	no_argument,		&output_type,
	    UCL_EMIT_MSGPACK },
	{ "noop",	no_argument,		&noop,		1 },
	{ "nonewline",	no_argument,		&nonewline,	1 },
	{ "noquotes",	no_argument,		&show_raw,	1 },
	{ "output",	required_argument,	NULL,		'o' },
	{ "shellvars",	no_argument,		NULL,		'l' },
	{ "ucl",	no_argument,		&output_type,
	    UCL_EMIT_CONFIG },
	{ "yaml",	no_argument,		&output_type,	UCL_EMIT_YAML },
	{ NULL,		0,			NULL,		0 }
    };

    while ((ch = getopt_long(argc, argv, "cdD:ef:IjklmnNo:quy", longopts, NULL)) != -1) {
	switch (ch) {
	case 'c':
	    output_type = UCL_EMIT_JSON_COMPACT;
	    break;
	case 'd':
	    if (optarg != NULL) {
		debug = strtol(optarg, NULL, 0);
	    } else {
		debug = 1;
	    }
	    break;
	case 'D':
	    input_sepchar = optarg[0];
	    output_sepchar = optarg[0];
	    break;
	case 'e':
	    expand = 1;
	    break;
	case 'f':
	    filename = optarg;
	    break;
	case 'I':
	    pflags |= UCL_PARSER_KEY_LOWERCASE;
	    break;
	case 'j':
	    output_type = UCL_EMIT_JSON;
	    break;
	case 'k':
	    show_keys = 1;
	    break;
	case 'l':
	    output_sepchar = '_';
	    break;
	case 'm':
	    output_type = UCL_EMIT_MSGPACK;
	    break;
	case 'n':
	    noop = 1;
	    break;
	case 'N':
	    nonewline = 1;
	    break;
	case 'o':
	    outfile = optarg;
	    output = output_open(outfile);
	    break;
	case 'q':
	    show_raw = 1;
	    break;
	case 'u':
	    output_type = UCL_EMIT_CONFIG;
	    break;
	case 'y':
	    output_type = UCL_EMIT_YAML;
	    break;
	case 0:
	    break;
	default:
	    fprintf(stderr, "Error: Unexpected option: %i\n", ch);
	    usage();
	    break;
	}
    }
    argc -= optind;
    argv += optind;

    if (argc == 0) {
	usage();
    }

    /* Initialize parser */
    parser = ucl_parser_new(UCLCMD_PARSER_FLAGS | pflags);

    /* Parse the original UCL */
    if (filename == NULL || strcmp(filename, "-") == 0) {
	/* Input from STDIN */
	root_obj = parse_input(parser, stdin);
    } else {
	root_obj = parse_file(parser, filename);
    }

    for (k = 0; k < argc; k++) {
	success = false;

	obj_parent = get_parent(argv[k]);
	if (obj_parent == NULL) {
	    fprintf(stderr, "Failed to find parent of key %s, skipping...\n", argv[k]);
	    continue;
	}
	obj_child = get_object(argv[k]);
	if (obj_child == NULL || obj_child == obj_parent) {
	    fprintf(stderr, "Failed to find key %s, skipping...\n", argv[k]);
	    continue;
	}

	/* if parent is an array, special case */
	if (ucl_object_type(obj_parent) == UCL_ARRAY) {
	    if (debug > 0) {
		fprintf(stderr, "DEBUG: Attempting to removed index '%u' from '%s'\n",
		    ucl_array_index_of(obj_parent, obj_child), ucl_object_key(obj_parent));
	    }
	    obj_temp = ucl_array_delete(obj_parent, obj_child);
	    if (obj_temp != NULL) {
		success = true;
		ucl_object_unref(obj_temp);
	    }
	} else if (ucl_object_type(obj_parent) == UCL_OBJECT) {
	    if (ucl_object_key(obj_child) != NULL) {
		if (debug > 0) {
		    fprintf(stderr, "DEBUG: Attempting to removed node '%s' from '%s'\n",
			ucl_object_key(obj_child), ucl_object_key(obj_parent));
		}
		success = ucl_object_delete_key(obj_parent, ucl_object_key(obj_child));
	    } else {
		fprintf(stderr, "Failed to get key for '%s', skipping...\n", argv[k]);
		continue;
	    }
	} else {
	    fprintf(stderr, "Invalid parent object type for '%s', skipping...\n", argv[k]);
	    continue;
	}

	if (!success) {
	    fprintf(stderr, "Failed to remove key %s\n", argv[k]);
	} else if (debug > 0) {
	    fprintf(stderr, "DEBUG: Removed node %s\n", argv[k]);
	}
    }
    get_mode("");

    cleanup();

    return(ret);
}
