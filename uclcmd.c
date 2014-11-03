/*-
 * Copyright (c) 2014 Allan Jude <allanjude@freebsd.org>
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

#include <errno.h>
#include <getopt.h>
#include <stdio.h>

#include <ucl.h>

#ifndef __DECONST
#define __DECONST(type, var)    ((type)(uintptr_t)(const void *)(var))
#endif

/*
 * XXX: TODO:
 *
 */

static int show_keys = 0, show_raw = 0, mode = 0, debug = 0;
static int output_type = 254;
static ucl_object_t *root_obj = NULL;
static char *dot = ".";
static char *sepchar = ".";

void usage();
void get_mode(char *requested_node);
int process_get_command(const ucl_object_t *obj, char *nodepath,
    const char *command_str, char *remaining_commands, int recurse);
void set_mode(char *requested_node, char *data);
void output_key(const ucl_object_t *obj, char *nodepath, const char *key);
void replace_sep(char *key, char *oldsep, char *newsep);

/*
 * This application provides a shell scripting friendly interface for reading
 * and writing UCL config files, using libUCL.
 */
int
main(int argc, char *argv[])
{
    const char *filename = NULL;
    unsigned char inbuf[8192];
    struct ucl_parser *parser;
    struct ucl_parser *setparser = NULL;
    ucl_object_t *dst_obj = NULL;
    ucl_object_t *sub_obj = NULL;
    ucl_object_t *set_obj = NULL;
    int ret = 0, r = 0, k = 0, ch;
    bool success = false;
    FILE *in;

    if (argc == 0) {
	usage();
    }

    /* Initialize parser */
    parser = ucl_parser_new(UCL_PARSER_KEY_LOWERCASE);

    /*	options	descriptor */
    static struct option longopts[] = {
	{ "cjson",	no_argument,            &output_type,
	    UCL_EMIT_JSON_COMPACT },
	{ "debug",      optional_argument,      NULL,       	'd' },
	{ "file",       required_argument,      NULL,       	'f' },
	{ "get",        no_argument,            &mode,      	0 },
	{ "json",       no_argument,            &output_type,
	    UCL_EMIT_JSON },
	{ "keys",       no_argument,            &show_keys, 	1 },
	{ "merge",    	no_argument,            &mode,  	2 },
	{ "noquote",    no_argument,            &show_raw,  	1 },
	{ "remove",     no_argument,            &mode,      	3 },
	{ "set",        no_argument,            &mode,      	1 },
	{ "shellvars",  no_argument,            NULL,      	'v' },
	{ "ucl",        no_argument,            &output_type,
	    UCL_EMIT_CONFIG },
	{ "yaml",       no_argument,            &output_type,	UCL_EMIT_YAML },
	{ NULL,         0,                      NULL,       	0 }
    };
    
    while ((ch = getopt_long(argc, argv, "cdf:gjkmqrsuvy", longopts, NULL)) != -1) {
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
	case 'f':
	    ucl_parser_add_file(parser, optarg);
	    if (ucl_parser_get_error(parser)) {
		fprintf(stderr, "Error occured: %s\n",
		    ucl_parser_get_error(parser));
		ret = 1;
		goto end;
	    }
	    filename = optarg;
	    break;
	case 'j':
	    output_type = UCL_EMIT_JSON;
	    break;
	case 'g':
	    mode = 0;
	    break;
	case 'k':
	    show_keys = 1;
	    break;
	case 'm':
	    mode = 2;
	    break;
	case 'q':
	    show_raw = 1;
	    break;
	case 'r':
	    mode = 3;
	    break;
	case 's':
	    mode = 1;
	    break;
	case 'u':
	    output_type = UCL_EMIT_CONFIG;
	    break;
	case 'v':
	    sepchar = "_";
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
    
    switch (mode) {
    case 0: /* Get Mode */
	if (argc > 0) {
	    if (filename == NULL) {
		in = stdin;
		while (!feof(in) && r < (int)sizeof(inbuf)) {
		    r += fread(inbuf + r, 1, sizeof(inbuf) - r, in);
		}
		ucl_parser_add_chunk(parser, inbuf, r);
		fclose(in);
	    }

	    if (ucl_parser_get_error(parser)) {
		fprintf(stderr, "Error occured: %s\n", ucl_parser_get_error(parser));
		ret = 1;
		goto end;
	    }

	    root_obj = ucl_parser_get_object(parser);
	    if (ucl_parser_get_error (parser)) {
		fprintf(stderr, "Error: Parse Error occured: %s\n",
		    ucl_parser_get_error(parser));
		ret = 1;
		goto end;
	    }

	    for (k = 0; k < argc; k++) {
		get_mode(argv[k]);
	    }
	} else {
	    fprintf(stderr, "Error: No search performed.\n");
	}
	break;
    case 1: /* Set Mode */
	/* Parse the original UCL */
	if (ucl_parser_get_error(parser)) {
	    fprintf(stderr, "Error S1 occured: %s\n", ucl_parser_get_error(parser));
	    ret = 1;
	    goto end;
	}
	char *dst_key = argv[0];
	char *dst_root = strdup(dst_key);
	char *dst_frag = strrchr(dst_root, '.');

	root_obj = ucl_parser_get_object(parser);
	if (ucl_parser_get_error (parser)) {
	    fprintf(stderr, "Error: S2 Parse Error occured: %s\n",
		ucl_parser_get_error(parser));
	    ret = 1;
	    goto end;
	}

	setparser = ucl_parser_new(UCL_PARSER_KEY_LOWERCASE);
	/* Lookup the destination to write to */
	if (debug > 0) {
	    fprintf(stderr, "selecting key: %s\n", dst_key);
	    fprintf(stderr, "selected sub-key: %s\n", dst_frag);
	}
	if (dst_frag == NULL || strlen(dst_frag) == 0) {
	    dst_frag = dst_key;
	    dst_obj = root_obj;
	    sub_obj = __DECONST(ucl_object_t *,
		ucl_object_find_key(dst_obj, dst_frag));
	} else {
	    dst_frag[0] = '\0';
	    dst_frag++;
	    dst_obj = __DECONST(ucl_object_t *,
		ucl_lookup_path(root_obj, dst_root));
	    if (dst_obj == NULL) {
		ret = 1;
		goto end;
	    }
	    if (dst_obj->type == UCL_ARRAY) {
		sub_obj = __DECONST(ucl_object_t *,
		    ucl_lookup_path(root_obj, dst_key));
	    } else {
		sub_obj = __DECONST(ucl_object_t *,
		    ucl_object_find_key(dst_obj, dst_frag));
	    }
	}
	if (sub_obj == NULL) {
	    sub_obj = dst_obj;
	}

	if (argc > 1) {
	    if (sub_obj->type != UCL_OBJECT && sub_obj->type != UCL_ARRAY) {
		/* Destination is a string, number, etc */
		set_obj = ucl_object_fromstring_common(argv[1], 0,
		    UCL_STRING_PARSE);
	    } else {
		/* Destination is an Object or Array */
		success = ucl_parser_add_string(setparser, argv[1], 0);
		if (ucl_parser_get_error(setparser)) {
		    fprintf(stderr, "Error S3 occured: %s\n",
			ucl_parser_get_error(setparser));
		    ret = 1;
		    goto end;
		}
		set_obj = ucl_parser_get_object(setparser);
	    }
	} else {
	    /* get UCL to add from stdin */
	    in = stdin;
	    while (!feof(in) && r < (int)sizeof(inbuf)) {
		r += fread(inbuf + r, 1, sizeof(inbuf) - r, in);
	    }
	    ucl_parser_add_chunk(setparser, inbuf, r);
	    fclose(in);

	    if (ucl_parser_get_error(setparser)) {
		fprintf(stderr, "Error S4 occured: %s\n",
		    ucl_parser_get_error(setparser));
		ret = 1;
		goto end;
	    }
	    set_obj = ucl_parser_get_object(setparser);
	}

	if (ucl_parser_get_error(setparser)) {
	    fprintf(stderr, "Error: S5 Parse Error occured: %s\n",
		ucl_parser_get_error(setparser));
	    ret = 1;
	    goto end;
	}
	/* Add it to the object here */
	if (debug > 0) {
	    fprintf(stderr, "Inserting key %s to root: %s\n", dst_frag, dst_root);
	}

	if (dst_obj->type == UCL_ARRAY) {
	    ucl_object_t *trash = ucl_array_delete(dst_obj, sub_obj);
	    ucl_object_unref(trash);
	    success = ucl_array_append(dst_obj, set_obj);
	    success = true;
	} else {
	    success = ucl_object_replace_key(dst_obj, set_obj,
		dst_frag, 0, false);
	}
	if (success) {
	    get_mode(dot);
	} else {
	    fprintf(stderr, "Error: Failed to apply the set operation.\n");    
	    ret = 1;
	}
	break;
    case 2: /* merge */
	fprintf(stderr, "Error: Merge mode not implemented yet.\n");
	ret = 99;
	goto end;
	/* get UCL to add from stdin */
	in = stdin;
	while (!feof(in) && r < (int)sizeof(inbuf)) {
	    r += fread(inbuf + r, 1, sizeof(inbuf) - r, in);
	}
	ucl_parser_add_chunk(parser, inbuf, r);
	fclose(in);

	if (ucl_parser_get_error(parser)) {
	    fprintf(stderr, "Error occured: %s\n", ucl_parser_get_error(parser));
	    ret = 1;
	    goto end;
	}

	root_obj = ucl_parser_get_object(parser);
	if (ucl_parser_get_error (parser)) {
	    fprintf(stderr, "Error: Parse Error occured: %s\n",
		ucl_parser_get_error(parser));
	    ret = 1;
	    goto end;
	}
	break;
    case 3: /* remove */
	/* Parse the original UCL */
	if (ucl_parser_get_error(parser)) {
	    fprintf(stderr, "Error occured: %s\n", ucl_parser_get_error(parser));
	    ret = 1;
	    goto end;
	}

	root_obj = ucl_parser_get_object(parser);
	if (ucl_parser_get_error (parser)) {
	    fprintf(stderr, "Error: Parse Error occured: %s\n",
		ucl_parser_get_error(parser));
	    ret = 1;
	    goto end;
	}

	ret = ucl_object_delete_key(root_obj, argv[0]);
	get_mode(dot);
	break;
    }

end:
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

    return ret;
}

void
usage()
{
    fprintf(stderr, "%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n"
"%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n"
"%s\n%s\n%s\n%s\n%s\n%s\n%s\n",
"Usage: uclcmd [-cdjkmqruvy] [-f filename] --get variable",
"       uclcmd [-cdjkmqruvy] [-f filename] --set variable UCL",
"",
"OPTIONS:",
"       -c --cjson	output compacted JSON",
"       -d --debug     	enable verbose debugging output",
"       -f --file      	path to a file to read or write",
"       -j --json	output pretty JSON",
"       -k --keys      	show key=value rather than just the value",
"	-m --merge	merge the provided UCL into the indicated key",
"       -q --noquote   	do not enclose strings in quotes",
"	-r --remove	delete the indicated key",
"       -g --get       	read a variable",
"       -s --set       	write a block of UCL",
"       -u --ucl     	output universal config language",
"       -v --shellvars	keys are output with underscores instead of dots",
"       -y --yaml     	output YAML",
"       variable    	The key of the variable to read, in object notation",
"       UCL         	A block of UCL to be written to the specified variable",
"",
"EXAMPLES:",
"       uclcmd --file vmconfig --get .name",
"           \"value\"",
"",
"       uclcmd --file vmconfig --keys --raw --get .array.1.name",
"           name=value",
"");
    exit(1);
}

void
get_mode(char *requested_node)
{
    const ucl_object_t *found_object;
    char *cmd = requested_node;
    char *node_name = strsep(&cmd, "|");
    char *command_str = strsep(&cmd, "|");
    char *nodepath = "\0";
    int command_count = 0, i;
    
    found_object = root_obj;
    
    if (strlen(node_name) == 0) {
	/* Requested root node */
	if (debug > 0) {
	    fprintf(stderr, "DEBUG: Using root node\n");
	}
	found_object = root_obj;
    } else if (strcmp(node_name, ".") == 0) {
	if (debug > 0) {
	    fprintf(stderr, "DEBUG: Using root node\n");
	}
	found_object = root_obj;
    } else {
	if (strncmp(node_name, ".", 1) == 0) {
	    /* Removing leading dot */
	    node_name++;
	}
	/* Search for selected node */
	if (debug > 0) {
	    fprintf(stderr, "DEBUG: Searching node %s\n", node_name);
	}
	found_object = ucl_lookup_path(found_object, node_name);
	asprintf(&nodepath, "%s", node_name);
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
	unsigned char *result = NULL;
	switch (output_type) {
	case 254: /* Text */
	    output_key(found_object, nodepath, "");
	    break;
	case UCL_EMIT_CONFIG: /* UCL */
	    result = ucl_object_emit(found_object, output_type);
	    printf("%s\n", result);
	    free(result);
	    break;
	case UCL_EMIT_JSON: /* JSON */
	    result = ucl_object_emit(found_object, output_type);
	    printf("%s\n", result);
	    free(result);
	    break;
	case UCL_EMIT_JSON_COMPACT: /* Compact JSON */
	    result = ucl_object_emit(found_object, output_type);
	    printf("%s\n", result);
	    free(result);
	    break;
	case UCL_EMIT_YAML: /* YAML */
	    result = ucl_object_emit(found_object, output_type);
	    printf("%s\n", result);
	    free(result);
	    break;
	default:
	    fprintf(stderr, "Error: Invalid output mode: %i\n",
		output_type);
	    break;
	}
    }
}

int
process_get_command(const ucl_object_t *obj, char *nodepath,
    const char *command_str, char *remaining_commands, int recurse)
{
    ucl_object_iter_t it = NULL;
    const ucl_object_t *cur;
    int command_count = 0, loopcount = 0;
    int recurse_level = recurse;
    int arrindex = 0;
    
    if (debug >= 2) {
	fprintf(stderr, "DEBUG: Got command: %s - next command: %s\n",
	    command_str, remaining_commands);
    }
    if (strcmp(command_str, "length") == 0) {
	/* Return the number of items in an array */
	if (obj == NULL) {
	    if (show_keys == 1)
		printf("(null)=");
	    printf("0\n");
	} else {
	    if (show_keys == 1)
		printf("%s%s%s=", nodepath, sepchar, obj->key);
	    printf("%u\n", obj->len);
	}
    } else if (strcmp(command_str, "type") == 0) {
	/* Return the type of the current object */
	if (obj == NULL) {
	    if (show_keys == 1)
		printf("(null)=");
	    printf("null\n");
	} else {
	    if (show_keys == 1)
		printf("%s%s%s=", nodepath, sepchar, obj->key);
	    switch(obj->type) {
	    case UCL_OBJECT:
		printf("object\n");
		break;
	    case UCL_ARRAY:
		printf("array\n");
		break;
	    case UCL_INT:
		printf("int\n");
		break;
	    case UCL_FLOAT:
		printf("float\n");
		break;
	    case UCL_STRING:
		printf("string\n");
		break;
	    case UCL_BOOLEAN:
		printf("boolean\n");
		break;
	    case UCL_TIME:
		printf("time\n");
		break;
	    case UCL_USERDATA:
		printf("userdata\n");
		break;
	    case UCL_NULL:
		printf("null\n");
		break;
	    default:
		printf("unknown\n");
		break;
	    }
	}
    } else if (strcmp(command_str, "keys") == 0) {
	if (obj != NULL) {
	    /* Return the keys of the current object */
	    while ((cur = ucl_iterate_object(obj, &it, true))) {
		printf("%s\n", ucl_object_key(cur));
		loopcount++;
	    }
	}
	if (loopcount == 0 && debug > 0) {
	    fprintf(stderr, "DEBUG: Found 0 keys\n");
	}
    } else if (strcmp(command_str, "values") == 0) {
	if (obj != NULL) {
	    /* Return the values of the current object */
	    while ((cur = ucl_iterate_object(obj, &it, true))) {
		char *newkey = NULL;
		if (cur == NULL) {
		    continue;
		}
		if (obj->type == UCL_ARRAY) {
		    asprintf(&newkey, "%s%i", sepchar, arrindex);
		    arrindex++;
		} else {
		    asprintf(&newkey, "%s%s", sepchar, ucl_object_key(cur));
		}
		output_key(cur, nodepath, newkey);
		loopcount++;
		free(newkey);
	    }
	}
	if (loopcount == 0 && debug > 0) {
	    fprintf(stderr, "DEBUG: Found 0 values\n");
	}
    } else if (strcmp(command_str, "each") == 0) {
	if (remaining_commands == NULL) {
	    while ((cur = ucl_iterate_object(obj, &it, true))) {
		char *newkey = NULL;
		if (cur == NULL) {
		    continue;
		}
		if (obj->type == UCL_ARRAY) {
		    asprintf(&newkey, "%s%i", sepchar, arrindex);
		    arrindex++;
		} else {
		    asprintf(&newkey, "%s%s", sepchar, ucl_object_key(cur));
		}
		output_key(cur, nodepath, newkey);
		loopcount++;
		free(newkey);
	    }
	} else if (obj != NULL) {
	    /* Return the values of the current object */
	    char *rcmds = malloc(strlen(remaining_commands) + 1);
	    strcpy(rcmds, remaining_commands);
	    char *next_command = strsep(&rcmds, "|");
	    if (next_command != NULL) {
		while ((cur = ucl_iterate_object(obj, &it, true))) {
		    char *newnodepath = NULL;
		    if (cur == NULL) {
			continue;
		    }
		    if (obj->type == UCL_ARRAY) {
			asprintf(&newnodepath, "%s%s%i", nodepath, sepchar,
			    arrindex);
			arrindex++;
		    } else {
			asprintf(&newnodepath, "%s%s%s", nodepath, sepchar,
			    ucl_object_key(cur));
		    }
		    recurse_level = process_get_command(cur, newnodepath,
			next_command, rcmds, recurse + 1);
		    loopcount++;
		    free(newnodepath);
		}
	    }
	}
	if (loopcount == 0 && debug > 0) {
	    fprintf(stderr, "DEBUG: Found 0 objects to each over\n");
	}
    } else if (strncmp(command_str, ".", 1) == 0) {
	/* User has provided an identifier after the commands */
	/* Search for selected node */
	if (debug > 0) {
	    fprintf(stderr, "DEBUG: Searching for subnode %s\n", command_str);
	}
	cur = ucl_lookup_path(obj, command_str);
	/* If this is the last thing on the stack, output */
	if (remaining_commands == NULL) {
	    /* Would also check cur==null here, but that breaks |keys */
	    output_key(cur, nodepath, command_str);
	} else {
	    /* Return the values of the current object */
	    char *rcmds = malloc(strlen(remaining_commands) + 1);
	    strcpy(rcmds, remaining_commands);
	    char *next_command = strsep(&rcmds, "|");
	    if (next_command != NULL) {
		char *newnodepath = NULL;
		if (obj->type == UCL_ARRAY) {
		    asprintf(&newnodepath, "%s%s%i", nodepath, sepchar,
			arrindex);
		    arrindex++;
		} else {
		    asprintf(&newnodepath, "%s%s%s", nodepath, sepchar,
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
    } else {
	/* Not a valid command */
	fprintf(stderr, "Error: invalid command %s\n\n", command_str);
	exit(1);
    }
    command_count++;
    if (debug >= 3) {
	fprintf(stderr, "DEBUG: Returning p_g_c with c_count=%i rlevel=%i\n",
	    command_count, recurse_level);
    }
    return recurse_level;
}

void
set_mode(char *requested_node, char *data)
{
    /* ucl_object_insert_key_merged */
    
    /* commands: array_append array_prepend array_delete */
}

void
output_key(const ucl_object_t *obj, char *nodepath, const char *inkey)
{
    char *key = strdup(inkey);
    replace_sep(nodepath, ".", sepchar);
    replace_sep(key, ".", sepchar);
    if (obj == NULL) {
	if (show_keys == 1) {
	    printf("%s%s=", nodepath, key);
	}
	printf("null\n");
	return;
    }
    switch (obj->type) {
    case UCL_OBJECT:
	if (debug >= 3) {
	    fprintf(stderr, "DEBUG: key=%s\nlen=%u\ntype=UCL_OBJECT\n"
		"value={object}\n", obj->key, obj->len);
	}
	if (show_keys == 1)
	    printf("%s%s=", nodepath, key);
	printf("{object}\n");
	break;
    case UCL_ARRAY:
	if (debug >= 3) {
	    fprintf(stderr, "DEBUG: key=%s\nlen=%u\ntype=UCL_ARRAY\n"
		"value=[array]\n", obj->key, obj->len);
	}
	if (show_keys == 1)
	    printf("%s%s=", nodepath, key);
	printf("[array]\n");
	break;
    case UCL_INT:
	if (debug >= 3) {
	    fprintf(stderr, "DEBUG: key=%s\nlen=%u\ntype=UCL_INT\nvalue=%jd\n",
		obj->key, obj->len, (intmax_t)ucl_object_toint(obj));
	}
	if (show_keys == 1)
	    printf("%s%s=", nodepath, key);
	printf("%jd\n", (intmax_t)ucl_object_toint(obj));
	break;
    case UCL_FLOAT:
	if (debug >= 3) {
	    fprintf(stderr, "DEBUG: key=%s\nlen=%u\ntype=UCL_FLOAT\nvalue=%f\n",
		obj->key, obj->len, ucl_object_todouble(obj));
	}
	if (show_keys == 1)
	    printf("%s%s=", nodepath, key);
	printf("%f\n", ucl_object_todouble(obj));
	break;
    case UCL_STRING:
	if (debug >= 3) {
	    fprintf(stderr, "DEBUG: key=%s\nlen=%u\ntype=UCL_STRING\n"
		"value=\"%s\"\n", obj->key, obj->len, ucl_object_tostring(obj));
	}
	if (show_keys == 1)
	    printf("%s%s=", nodepath, key);
	if (show_raw == 1)
	    printf("%s\n", ucl_object_tostring(obj));
	else
	    printf("\"%s\"\n", ucl_object_tostring(obj));
	break;
    case UCL_BOOLEAN:
	if (debug >= 3) {
	    fprintf(stderr, "DEBUG: key=%s\nlen=%u\ntype=UCL_BOOLEAN\n"
		"value=%s\n", obj->key, obj->len,
		ucl_object_tostring_forced(obj));
	}
	if (show_keys == 1)
	    printf("%s%s=", nodepath, key);
	printf("%s\n", ucl_object_tostring_forced(obj));
	break;
    case UCL_TIME:
	if (debug >= 3) {
	    fprintf(stderr, "DEBUG: key=%s\nlen=%u\ntype=UCL_TIME\nvalue=%f\n",
		obj->key, obj->len, ucl_object_todouble(obj));
	}
	if (show_keys == 1)
	    printf("%s%s=", nodepath, key);
	printf("%f\n", ucl_object_todouble(obj));
	break;
    case UCL_USERDATA:
	if (debug >= 3) {
	    fprintf(stderr, "DEBUG: key=%s\nlen=%u\ntype=UCL_USERDATA\n"
		"value=%p\n", obj->key, obj->len, obj->value.ud);
	}
	if (show_keys == 1)
	    printf("%s%s=", nodepath, key);
	printf("{userdata}\n");
	break;
    default:
	if (debug >= 3) {
	    printf("error=Object of unknown type\n");
	    fprintf(stderr, "DEBUG: key=%s\nlen=%u\ntype=UCL_ERROR\n"
		"value=null\n", obj->key, obj->len);
	}
	break;
    }
}

void
replace_sep(char *key, char *oldsep, char *newsep) {
    int idx = 0;
    if (oldsep == newsep) return;
    while (key[idx] != '\0') {
	if (key[idx] == oldsep[0]) {
	    key[idx] = newsep[0];
	}
	idx++;
    }
}
