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
merge_main(int argc, char *argv[])
{
    const char *filename = NULL;
    int ret = 0, ch;
    bool success = false;

    /* Initialize parser */
    parser = ucl_parser_new(UCLCMD_PARSER_FLAGS);

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
	{ "noop",	no_argument,		&noop,		1 },
	{ "nonewline",	no_argument,		&nonewline,	1 },
	{ "noquotes",	no_argument,		&show_raw,	1 },
	{ "shellvars",	no_argument,		NULL,		'l' },
	{ "ucl",	no_argument,		&output_type,
	    UCL_EMIT_CONFIG },
	{ "yaml",	no_argument,		&output_type,	UCL_EMIT_YAML },
	{ NULL,		0,			NULL,		0 }
    };

    while ((ch = getopt_long(argc, argv, "cdD:ef:i:jklmnNquy", longopts, NULL)) != -1) {
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
	    noop = 1;
	    break;
	case 'N':
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

    if (argc > 1) { /* XXX: need test for if > 2 inputs */
	success = merge_mode(argv[0], argv[1]);
    } else {
	success = merge_mode(argv[0], NULL);
    }

    if (success) {
	get_mode("");
    } else {
	fprintf(stderr, "Error: Failed to apply the merge operation.\n");
	ret = 1;
    }

    cleanup();

    if (nonewline) {
	printf("\n");
    }
    return(ret);
}

int
merge_mode(char *destination_node, char *data)
{
    ucl_object_t *dst_obj = NULL;
    ucl_object_t *sub_obj = NULL;
    ucl_object_t *old_obj = NULL;
    ucl_object_t *tmp_obj = NULL;
    int success = 0;

    setparser = ucl_parser_new(UCL_PARSER_KEY_LOWERCASE |
	UCL_PARSER_NO_IMPLICIT_ARRAYS);

    /* Lookup the destination to write to */
    dst_obj = get_parent(destination_node);
    sub_obj = get_object(destination_node);

    if (sub_obj == NULL) {
	return false;
    }

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
	rt = objtype_as_string(dst_obj);
	dt = objtype_as_string(sub_obj);
	st = objtype_as_string(set_obj);
	fprintf(stderr, "root type: %s, destination type: %s, new type: %s\n",
	    rt, dt, st);
	if (rt != NULL) free(rt);
	if (dt != NULL) free(dt);
	if (st != NULL) free(st);

	fprintf(stderr, "Merging key %s to root: %s\n",
	    ucl_object_key(sub_obj), ucl_object_key(dst_obj));
    }

    /* Add it to the object here */
    if (ucl_object_type(sub_obj) == UCL_ARRAY && ucl_object_type(set_obj) == UCL_ARRAY) {
	if (debug > 0) {
	    fprintf(stderr, "Merging array of size %u with array of size %u\n",
		sub_obj->len, set_obj->len);
	}
	success = ucl_array_merge(sub_obj, set_obj, true);
    } else if (ucl_object_type(sub_obj) == UCL_ARRAY) {
	if (debug > 0) {
	    fprintf(stderr, "Appending object to array of size %u\n",
		sub_obj->len);
	}
	success = ucl_array_append(sub_obj, ucl_object_ref(set_obj));
    } else if (ucl_object_type(sub_obj) == UCL_OBJECT && ucl_object_type(set_obj) == UCL_OBJECT) {
	if (debug > 0) {
	    fprintf(stderr, "Merging object %s with object %s\n",
		ucl_object_key(sub_obj), ucl_object_key(set_obj));
	}
	/* XXX not supported:
	 * success = ucl_object_merge(sub_obj, set_obj, false, true);
	 */
	
	/* Old non-recursive way */
	/*
	success = ucl_object_merge(sub_obj, set_obj, false);
	*/
	success = merge_recursive(sub_obj, set_obj, false);
    } else if (ucl_object_type(sub_obj) != UCL_OBJECT && ucl_object_type(sub_obj) != UCL_ARRAY) {
	/* Create an explicit array */
	if (debug > 0) {
	    fprintf(stderr, "Creating an array and appended the new item\n");
	}
	tmp_obj = ucl_object_typed_new(UCL_ARRAY);
	/*
	 * Reference and Append the original scalar
	 * The additional reference is required because the old object will be
	 * unreferenced as part of the ucl_object_replace_key operation
	 */
	ucl_array_append(tmp_obj, ucl_object_ref(sub_obj));
	/* Reference and Append the new scalar (unref in cleanup()) */
	ucl_array_append(tmp_obj, ucl_object_ref(set_obj));
	/* Replace the old object with the newly created one */
	if (ucl_object_type(dst_obj) == UCL_ARRAY) {
	    old_obj = ucl_array_replace_index(dst_obj, tmp_obj,
		ucl_array_index_of(dst_obj, sub_obj));
	    success = false;
	    if (old_obj != NULL) {
		ucl_object_unref(old_obj);
		success = true;
	    }
	} else {
	    success = ucl_object_replace_key(dst_obj, tmp_obj,
		ucl_object_key(sub_obj), 0, true);
	}
    } else {
	if (debug > 0) {
	    fprintf(stderr, "Merging object into key %s\n",
		ucl_object_key(sub_obj));
	}
	success = ucl_object_insert_key_merged(dst_obj, ucl_object_ref(set_obj),
	    ucl_object_key(sub_obj), 0, true);
    }

    return success;
}

bool
merge_recursive(ucl_object_t *top, ucl_object_t *elt, bool copy)
{
    const ucl_object_t *cur;
    ucl_object_iter_t it = NULL;
    ucl_object_t *found = NULL, *cp_obj = NULL;
    bool success = false;

    it = ucl_object_iterate_new(elt);

    while ((cur = ucl_object_iterate_safe(it, false))) {
	cp_obj = ucl_object_ref(cur);
	if (debug > 0) {
	    fprintf(stderr, "DEBUG: Looping over (elt)%s, found key: %s\n",
		ucl_object_key(top), ucl_object_key(cur));
	}
	if (ucl_object_type(cur) == UCL_OBJECT) {
	    found = __DECONST(ucl_object_t *, ucl_object_find_key(top, ucl_object_key(cur)));
	    if (found == NULL) {
		/* new key not found in old object, insert it */
		if (debug > 0) {
		    fprintf(stderr, "DEBUG: unmatched key, inserting: %s into top\n",
			ucl_object_key(cur));
		}
		success = ucl_object_insert_key_merged(top, cp_obj,
		    ucl_object_key(cp_obj), 0, true);
		if (success == false) { return false; }
		continue;
	    }
	    if (debug > 0) {
		fprintf(stderr, "DEBUG: (obj) Found key %s in (top)%s too, merging...\n",
		    ucl_object_key(found), ucl_object_key(top));
	    }
	    success = merge_recursive(found, cp_obj, copy);
	    if (success == false) { return false; }
	} else if (ucl_object_type(cur) == UCL_ARRAY) {
	    found = __DECONST(ucl_object_t *, ucl_object_find_key(top, ucl_object_key(cur)));
	    if (found == NULL) {
		/* new key not found in old object, insert it */
		if (debug > 0) {
		    fprintf(stderr, "DEBUG: unmatched key, inserting: %s into top\n",
			ucl_object_key(cur));
		}
		success = ucl_object_insert_key_merged(top, cp_obj,
		    ucl_object_key(cp_obj), 0, true);
		if (success == false) { return false; }
		continue;
	    }
	    if (debug > 0) {
		fprintf(stderr, "DEBUG: (arr) Found key %s in (top)%s too, merging...\n",
		    ucl_object_key(found), ucl_object_key(top));
	    }
	    success = ucl_array_merge(found, cp_obj, true);
	    if (success == false) { return false; }
	} else {
	    found = __DECONST(ucl_object_t *, ucl_object_find_key(top, ucl_object_key(cur)));
	    if (found == NULL) {
		/* new key not found in old object, insert it */
		if (debug > 0) {
		    fprintf(stderr, "DEBUG: inserting %s into %s\n",
			ucl_object_key(cur), ucl_object_key(top));
		}
		success = ucl_object_insert_key_merged(top, ucl_object_ref(cur),
		    ucl_object_key(cur), 0, true);
		if (success == false) { return false; }
		continue;
	    }
	    if (debug > 0) {
		fprintf(stderr, "DEBUG: replacing %s in %s\n",
		    ucl_object_key(found), ucl_object_key(top));
	    }
	    success = ucl_object_replace_key(top, cp_obj,
		ucl_object_key(cp_obj), 0, true);
	    if (success == false) { return false; }
	}
    }

    return success;
}