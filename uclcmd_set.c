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
set_main(int argc, char *argv[])
{
    const char *filename = NULL;
    int ret = 0, ch;
    bool success = false;

    /* Initialize parser */
    parser = ucl_parser_new(UCLCMD_PARSER_FLAGS | UCL_PARSER_DISABLE_MACRO);

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
	{ "input",	no_argument,		NULL,		'i' },
	{ "msgpack",	no_argument,		&output_type,
	    UCL_EMIT_MSGPACK },
	{ "nonewline",	no_argument,		&nonewline,	1 },
	{ "noquotes",	no_argument,		&show_raw,	1 },
	{ "shellvars",	no_argument,		NULL,		'l' },
	{ "ucl",	no_argument,		&output_type,
	    UCL_EMIT_CONFIG },
	{ "yaml",	no_argument,		&output_type,	UCL_EMIT_YAML },
	{ NULL,		0,			NULL,		0 }
    };

    while ((ch = getopt_long(argc, argv, "cdD:ef:i:jklmnquy", longopts, NULL)) != -1) {
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
	    if (strcmp(optarg, "-") == 0) {
		/* Input from STDIN */
		root_obj = parse_input(parser, stdin);
	    } else {
		root_obj = parse_file(parser, filename);
	    }
	    break;
	case 'i':
	    include_file = optarg;
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
	    nonewline = 1;
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

    if (filename == NULL) {
	root_obj = parse_input(parser, stdin);
    }

    if (argc > 1) { 
	success = set_mode(argv[0], argv[1]);
    } else {
	success = set_mode(argv[0], NULL);
    }

    if (success) {
	get_mode("");
    } else {
	fprintf(stderr, "Error: Failed to apply the set operation.\n");
	ret = 1;
    }

    cleanup();

    if (nonewline) {
	printf("\n");
    }
    return(ret);
}

int
set_mode(char *destination_node, char *data)
{
    ucl_object_t *dst_obj = NULL;
    ucl_object_t *sub_obj = NULL;
    ucl_object_t *old_obj = NULL;
    int success = 0;

    setparser = ucl_parser_new(UCLCMD_PARSER_FLAGS);

    /* Lookup the destination to write to */
    dst_obj = get_parent(destination_node);
    sub_obj = get_object(destination_node);

    if (include_file != NULL) {
	/* get UCL to add from file */
	set_obj = parse_file(setparser, include_file);
    } else if (data == NULL || strcmp(data, "-") == 0) {
	/* get UCL to add from stdin */
	set_obj = parse_input(setparser, stdin);
    } else {
	/* User provided data inline */
	set_obj = parse_string(setparser, data);
    }

    if (debug > 0) {
	char *rt = NULL, *dt = NULL, *st = NULL;
	rt = type_as_string(dst_obj);
	dt = type_as_string(sub_obj);
	st = type_as_string(set_obj);
	fprintf(stderr, "root type: %s, destination type: %s, new type: %s\n",
	    rt, dt, st);
	if (rt != NULL) free(rt);
	if (dt != NULL) free(dt);
	if (st != NULL) free(st);

	fprintf(stderr, "Inserting key %s to root: %s\n",
	    ucl_object_key(sub_obj), ucl_object_key(dst_obj));
    }

    char *dst_frag = strrchr(destination_node, input_sepchar);
    dst_frag++;
    /* Replace it in the object here */
    if (ucl_object_type(dst_obj) == UCL_ARRAY) {
	/* XXX TODO: What if the destination_node only points to an array */
	/* XXX TODO: What if we want to replace an entire array? */
	if (debug > 0) {
	    fprintf(stderr, "Replacing array index %s\n", dst_frag);
	}
	if (sub_obj == dst_obj) {
	    /* Sub-object does not exist, create a new one */
	    success = ucl_array_append(dst_obj, set_obj);
	} else {
	    old_obj = ucl_array_replace_index(dst_obj, set_obj, strtoul(dst_frag,
		NULL, 0));
	    success = false;
	    if (old_obj != NULL) {
		ucl_object_unref(old_obj);
		set_obj = NULL;
		success = true;
	    }
	}
    } else {
	if (debug > 0) {
	    fprintf(stderr, "Replacing key %s\n", ucl_object_key(sub_obj));
	}
	if (sub_obj == dst_obj) {
	    /* Sub-object does not exist, create a new one */
	    success = ucl_object_insert_key(dst_obj, set_obj, dst_frag, 0,
		true);
	} else {
	    success = ucl_object_replace_key(dst_obj, set_obj,
		ucl_object_key(sub_obj), 0, true);
	}
	set_obj = NULL;
    }

    return success;
}
