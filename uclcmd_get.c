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
get_main(int argc, char *argv[])
{
    const char *filename = NULL;
    int ret = 0, k = 0, ch;

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
	    printf("Not implemented yet\n");
	    exit(1);
	    break;
	case 'j':
	    output_type = UCL_EMIT_JSON;
	    break;
	case 'k':
	    show_keys = 1;
	    break;
	case 'l':
 	    shvars = true;
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

    for (k = 0; k < argc; k++) {
	get_mode(argv[k]);
    }

    cleanup();

    if (nonewline) {
	printf("\n");
    }

    return(ret);
}

void
get_mode(char *requested_node)
{
    const ucl_object_t *found_object;
    char *cmd = requested_node;
    char *node_name = strsep(&cmd, "|");
    char *command_str = strsep(&cmd, "|");
    char *nodepath = NULL;
    int command_count = 0, i = 0;

    uclcmd_asprintf(&nodepath, "");
    found_object = root_obj;

    if (strlen(node_name) == 0) {
	/* Requested root node */
	if (debug > 0) {
	    fprintf(stderr, "DEBUG: Using root node\n");
	}
	found_object = root_obj;
    } else if (strlen(node_name) == 1 && node_name[0] == input_sepchar) {
	if (debug > 0) {
	    fprintf(stderr, "DEBUG: Using root node\n");
	}
	found_object = root_obj;
    } else {
	if (node_name[0] == input_sepchar) {
	    /* Removing leading dot */
	    node_name++;
	}
	/* Search for selected node */
	if (debug > 0) {
	    fprintf(stderr, "DEBUG: Searching node %s\n", node_name);
	}
	found_object = ucl_lookup_path_char(found_object, node_name, input_sepchar);
	free(nodepath);
	uclcmd_asprintf(&nodepath, "%s", node_name);
    }

    while (command_str != NULL) {
	if (debug > 0) {
	    fprintf(stderr, "DEBUG: Performing \"%s\" command on \"%s\"...\n",
		command_str, node_name);
	}
	int done = process_get_command(found_object, nodepath, command_str,
	    cmd, 1);
	if (debug >= 2) {
	    fprintf(stderr, "DEBUG: Finished process, did: %i commands\n",
		done);
	}

	for (i = 0; i < done; i++) {
	    if (debug >= 2) {
		fprintf(stderr, "DEBUG: Removing command: %s\n", command_str);
	    }
	    command_str = strsep(&cmd, "|");
	}
	if (debug >= 2) {
	    fprintf(stderr, "DEBUG: Remaining command: %s\n", command_str);
	}
	command_count += done;
    }

    if (debug >= 2) {
	fprintf(stderr, "DEBUG: Ending get_mode with command_count=%i\n",
	    command_count);
    }
    if (command_count == 0) {
	output_chunk(found_object, nodepath, "");
    }
    free(nodepath);
}

int
process_get_command(const ucl_object_t *obj, char *nodepath,
    const char *command_str, char *remaining_commands, int recurse)
{
    int command_count = 0, recurse_level = recurse;

    if (debug >= 2) {
	fprintf(stderr, "DEBUG: Got command: %s - next command: %s\n",
	    command_str, remaining_commands);
    }
    if (strcmp(command_str, "length") == 0) {
	recurse_level = get_cmd_length(obj, nodepath, command_str,
		remaining_commands, recurse_level);
    } else if (strcmp(command_str, "dump") == 0) {
	ucl_obj_dump(obj,2);
    } else if (strcmp(command_str, "type") == 0) {
	recurse_level = get_cmd_type(obj, nodepath, command_str,
		remaining_commands, recurse_level);
    } else if (strcmp(command_str, "keys") == 0) {
	recurse_level = get_cmd_keys(obj, nodepath, command_str,
		remaining_commands, recurse_level);
    } else if (strcmp(command_str, "values") == 0) {
	recurse_level = get_cmd_values(obj, nodepath, command_str,
		remaining_commands, recurse_level);
    } else if (strcmp(command_str, "iterate") == 0) {
	recurse_level = get_cmd_iterate(obj, nodepath, command_str,
		remaining_commands, recurse_level);
    } else if (strcmp(command_str, "recurse") == 0) {
	recurse_level = get_cmd_recurse(obj, nodepath, command_str,
		remaining_commands, recurse_level);
    } else if (strcmp(command_str, "each") == 0) {
	recurse_level = get_cmd_each(obj, nodepath, command_str,
		remaining_commands, recurse_level);
    } else if (command_str[0] == input_sepchar) {
	recurse_level = get_cmd_none(obj, nodepath, command_str,
		remaining_commands, recurse_level);
    } else {
	/* Not a valid command */
	fprintf(stderr, "Error: invalid command %s\n", command_str);
	exit(1);
    }
    command_count++;
    if (debug >= 3) {
	fprintf(stderr, "DEBUG: Returning p_g_c with c_count=%i rlevel=%i\n",
	    command_count, recurse_level);
    }
    return recurse_level;
}

/*
 * Return the number of keys in an object or items in an array
 */
int
get_cmd_length(const ucl_object_t *obj, char *nodepath,
    const char *command_str, char *remaining_commands, int recurse)
{
    if (firstline == false) {
	printf(" ");
    }
    if (obj == NULL) {
	if (show_keys == 1)
	    printf("(null)=");
	printf("0");
    } else {
	if (show_keys == 1)
	    printf("%s", nodepath);
	printf("%u", obj->len);
    }
    if (nonewline) {
	firstline = false;
    } else {
	printf("\n");
    }

    return recurse;
}

/*
 * Return the type of the current object
 */
int
get_cmd_type(const ucl_object_t *obj, char *nodepath,
    const char *command_str, char *remaining_commands, int recurse)
{
    if (firstline == false) {
	printf(" ");
    }
    if (obj == NULL) {
	if (show_keys == 1)
	    printf("(null)=");
	printf("null");
    } else {
	if (show_keys == 1)
	    printf("%s=", nodepath);
	switch(ucl_object_type(obj)) {
	case UCL_OBJECT:
	    printf("object");
	    break;
	case UCL_ARRAY:
	    printf("array");
	    break;
	case UCL_INT:
	    printf("int");
	    break;
	case UCL_FLOAT:
	    printf("float");
	    break;
	case UCL_STRING:
	    printf("string");
	    break;
	case UCL_BOOLEAN:
	    printf("boolean");
	    break;
	case UCL_TIME:
	    printf("time");
	    break;
	case UCL_USERDATA:
	    printf("userdata");
	    break;
	case UCL_NULL:
	    printf("null");
	    break;
	default:
	    printf("unknown");
	    break;
	}
    }
    if (nonewline) {
	firstline = false;
    } else {
	printf("\n");
    }

    return(recurse);
}

/*
 * Return the keys of the current object
 */
int
get_cmd_keys(const ucl_object_t *obj, char *nodepath,
    const char *command_str, char *remaining_commands, int recurse)
{
    ucl_object_iter_t it = NULL;
    const ucl_object_t *cur;
    int loopcount = 0;

    if (obj != NULL) {
	while ((cur = ucl_iterate_object(obj, &it, true))) {
	    if (firstline == false) {
		printf(" ");
	    }
	    printf("%s", ucl_object_key(cur));
	    if (nonewline) {
		firstline = false;
	    } else {
		printf("\n");
	    }
	    loopcount++;
	}
    }
    if (loopcount == 0 && debug > 0) {
	fprintf(stderr, "DEBUG: Found 0 keys\n");
    }

    return(recurse);
}

/*
 * Return the values of each key in the current object
 */
int
get_cmd_values(const ucl_object_t *obj, char *nodepath,
    const char *command_str, char *remaining_commands, int recurse)
{
    ucl_object_iter_t it = NULL;
    const ucl_object_t *cur;
    int loopcount = 0, arrindex = 0;
    char *newkey = NULL;

    if (obj != NULL) {
	while ((cur = ucl_iterate_object(obj, &it, true))) {
	    if (cur == NULL) {
		continue;
	    }
	    if (ucl_object_type(obj) == UCL_ARRAY) {
		uclcmd_asprintf(&newkey, "%c%i", output_sepchar, arrindex);
		arrindex++;
	    } else {
		newkey = __DECONST(char *, ucl_object_key(cur));
		if (newkey != NULL) {
		    uclcmd_asprintf(&newkey, "%c%s", output_sepchar, ucl_object_key(cur));
		}
	    }
	    output_key(cur, nodepath, newkey);
	    loopcount++;
	    free(newkey);
	    newkey = NULL;
	}
    }
    if (loopcount == 0 && debug > 0) {
	fprintf(stderr, "DEBUG: Found 0 values\n");
    }

    return(recurse);
}

/*
 * Iterate over each key in the object
 */
int
get_cmd_iterate(const ucl_object_t *obj, char *nodepath,
    const char *command_str, char *remaining_commands, int recurse)
{
    ucl_object_iter_t it = NULL;
    const ucl_object_t *cur;
    int recurse_level = recurse;
    int loopcount = 0;

    if (remaining_commands == NULL) {
	it = NULL;
	char blankkey = '\0';
	while ((cur = ucl_iterate_object(obj, &it, false))) {
	    output_chunk(cur, nodepath, &blankkey);
	    loopcount++;
	}
    } else if (obj != NULL) {
	/* Return the values of the current object */
	char *rcmds = strdup(remaining_commands);
	char *next_command = strsep(&rcmds, "|");
	if (next_command != NULL) {
	    it = NULL;
	    while ((cur = ucl_iterate_object(obj, &it, false))) {
		recurse_level = process_get_command(cur, nodepath,
		    next_command, rcmds, recurse + 1);
	    }
	    loopcount++;
	}
    }
    if (loopcount == 0 && debug > 0) {
	fprintf(stderr, "DEBUG: Found 0 objects to each over\n");
    }

    return(recurse_level);
}

/*
 * Recurse through and output every key
 */
int
get_cmd_recurse(const ucl_object_t *obj, char *nodepath,
    const char *command_str, char *remaining_commands, int recurse)
{
    ucl_object_iter_t it = NULL, it2 = NULL;
    const ucl_object_t *cur, *cur2;
    char *tmpkeyname = NULL;
    int recurse_level = recurse;
    int loopcount = 0, arrindex = 0;

    if (strlen(nodepath) > 0) {
	output_chunk(obj, nodepath, "");
	if (expand && ucl_object_type(obj) == UCL_ARRAY) {
	    ucl_object_t *arrlen = NULL;

	    arrlen = ucl_object_fromint(obj->len);
	    uclcmd_asprintf(&tmpkeyname, "%c%s", output_sepchar, "_length");
	    output_chunk(arrlen, nodepath, tmpkeyname);
	    free(tmpkeyname);
	}
    }
    if (expand && ucl_object_type(obj) == UCL_OBJECT) {
	char *keylist = NULL;
	ucl_object_t *keystr = NULL;

	keylist = expand_subkeys(obj, nodepath);
	if (keylist != NULL) {
	    keystr = ucl_object_fromstring(keylist);
	    uclcmd_asprintf(&tmpkeyname, "%c%s", output_sepchar, "_keys");
	    output_chunk(keystr, nodepath, tmpkeyname);
	    free(tmpkeyname);
	    free(keylist);
	}
    }
    it = NULL;
    while ((cur = ucl_iterate_object(obj, &it, true))) {
	char *newkey = NULL;
	char *newnodepath = NULL;
	if (ucl_object_type(obj) == UCL_ARRAY) {
	    uclcmd_asprintf(&newkey, "%c%i", output_sepchar, arrindex);
	    arrindex++;
	} else if (strlen(nodepath) == 0) {
	    uclcmd_asprintf(&newkey, "%s", ucl_object_key(cur));
	} else {
	    uclcmd_asprintf(&newkey, "%c%s", output_sepchar, ucl_object_key(cur));
	}
	if (ucl_object_type(cur) == UCL_OBJECT ||
		ucl_object_type(cur) == UCL_ARRAY) {
	    it2 = NULL;
	    while ((cur2 = ucl_iterate_object(cur, &it2, false))) {
		if (nodepath != NULL && strlen(nodepath) > 0) {
		    uclcmd_asprintf(&newnodepath, "%s%s", nodepath, newkey);
		} else {
		    if (ucl_object_type(obj) == UCL_ARRAY) {
			uclcmd_asprintf(&newnodepath, "%i", arrindex);
		    } else {
			uclcmd_asprintf(&newnodepath, "%s", ucl_object_key(cur2));
		    }
		}
		recurse_level = process_get_command(cur2,
		    newnodepath, command_str, remaining_commands,
		    recurse + 1);
	    }
	} else {
	    output_chunk(cur, nodepath, newkey);
	}
	loopcount++;
	free(newkey);
	free(newnodepath);
    }
    if (loopcount == 0 && debug > 0) {
	fprintf(stderr, "DEBUG: Found 0 objects to each over\n");
    }

    return(recurse_level);
}

/*
 * Loop over each object and perform the next command on it
 */
int
get_cmd_each(const ucl_object_t *obj, char *nodepath,
    const char *command_str, char *remaining_commands, int recurse)
{
    ucl_object_iter_t it = NULL, it2 = NULL;
    const ucl_object_t *cur, *cur2;
    int recurse_level = recurse;
    int loopcount = 0, arrindex = 0;

    if (remaining_commands == NULL) {
	it = NULL;
	while ((cur = ucl_iterate_object(obj, &it, true))) {
	    char *newkey = NULL;
	    if (ucl_object_type(obj) == UCL_ARRAY) {
		uclcmd_asprintf(&newkey, "%c%i", output_sepchar, arrindex);
		arrindex++;
	    } else {
		uclcmd_asprintf(&newkey, "%c%s", output_sepchar, ucl_object_key(cur));
	    }
	    if (cur->next != 0 && cur->type != UCL_ARRAY) {
		/* Implicit array */
		it2 = NULL;
		while ((cur2 = ucl_iterate_object(cur, &it2, false))) {
		    output_chunk(cur2, nodepath, newkey);
		}
	    } else {
		output_chunk(cur, nodepath, newkey);
	    }
	    loopcount++;
	    free(newkey);
	}
    } else if (obj != NULL) {
	/* Return the values of the current object */
	char *rcmds = strdup(remaining_commands);
	char *next_command = strsep(&rcmds, "|");
	if (next_command != NULL) {
	    it = NULL;
	    while ((cur = ucl_iterate_object(obj, &it, true))) {
		char *newnodepath = NULL;
		if (ucl_object_type(obj) == UCL_ARRAY) {
		    uclcmd_asprintf(&newnodepath, "%s%c%i", nodepath, output_sepchar,
			arrindex);
		    arrindex++;
		} else {
		    uclcmd_asprintf(&newnodepath, "%s%c%s", nodepath, output_sepchar,
			ucl_object_key(cur));
		}
		if (cur->next != 0 && cur->type != UCL_ARRAY) {
		    /* Implicit array */
		    it2 = NULL;
		    while ((cur2 = ucl_iterate_object(cur, &it2, false))) {
			recurse_level = process_get_command(cur2,
			    newnodepath, next_command, rcmds, recurse + 1);
		    }
		} else {
		    recurse_level = process_get_command(cur, newnodepath,
			next_command, rcmds, recurse + 1);
		}
		loopcount++;
		free(newnodepath);
	    }
	}
    }
    if (loopcount == 0 && debug > 0) {
	fprintf(stderr, "DEBUG: Found 0 objects to each over\n");
    }

    return(recurse_level);
}

/*
 * Get a regular key
 */
int
get_cmd_none(const ucl_object_t *obj, char *nodepath,
    const char *command_str, char *remaining_commands, int recurse)
{
    const ucl_object_t *cur;
    char *reqnodelist = strdup(command_str);
    char *reqnode = NULL;
    int recurse_level = recurse;
    int arrindex = 0;

    /* Separate and loop here */
    while ((reqnode = strsep(&reqnodelist, " ")) != NULL) {
	/* User has provided an identifier after the commands */
	/* Search for selected node */
	if (debug > 0) {
	    fprintf(stderr, "DEBUG: Searching for subnode %s\n", reqnode);
	}
	cur = ucl_lookup_path_char(obj, reqnode, input_sepchar);
	/* If this is the last thing on the stack, output */
	if (remaining_commands == NULL) {
	    /* Would also check cur==null here, but that breaks |keys */
	    output_key(cur, nodepath, reqnode);
	} else {
	    /* Return the values of the current object */
	    char *rcmds = strdup(remaining_commands);
	    char *next_command = strsep(&rcmds, "|");
	    if (next_command != NULL) {
		char *newnodepath = NULL;
		if (ucl_object_type(obj) == UCL_ARRAY) {
		    uclcmd_asprintf(&newnodepath, "%s%c%i", nodepath, output_sepchar,
			arrindex);
		    arrindex++;
		} else {
		    uclcmd_asprintf(&newnodepath, "%s%c%s", nodepath, output_sepchar,
			ucl_object_key(cur));
		}
		if (debug > 2) {
		    fprintf(stderr, "DEBUG: Calling recurse with %s.%s on %s\n",
			newnodepath, next_command,
			ucl_object_emit(cur, UCL_EMIT_CONFIG));
		}
		recurse_level = process_get_command(cur, newnodepath,
		    next_command, rcmds, recurse + 1);
		free(newnodepath);
	    }
	}
    }

    return(recurse_level);
}

int
get_cmd_tab(const ucl_object_t *obj, char *nodepath,
    const char *command_str, char *remaining_commands, int recurse)
{
    return(recurse);
}
