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

static int show_keys = 0, show_raw = 0, mode = 0, debug = 0;
static ucl_object_t *root_obj = NULL;

void usage();
void get_mode(char *requested_node);
int process_get_command(const ucl_object_t *obj, const char *command_str,
    char *remaining_commands, int recurse);
void set_mode(char *requested_node, char *data);
void output_key(const ucl_object_t *obj, const char *key);

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
    int ret = 0, r = 0, k = 0, ch;
    FILE *in;
    
    /*	options	descriptor */
    static struct option longopts[] = {
	{ "debug",      no_argument,            &debug,     1 },
	{ "file",       required_argument,      NULL,       'f' },
	{ "get",        no_argument,            &mode,      0 },
	{ "keys",       no_argument,            &show_keys, 1 },
	{ "raw",        no_argument,            &show_raw,  1 },
	{ "set",        no_argument,            &mode,      1 },
	{ NULL,         0,                      NULL,       0 }
    };
    
    while ((ch = getopt_long(argc, argv, "f:gks", longopts, NULL)) != -1) {
	switch (ch) {
	case 'd':
	    debug = 1;
	    break;
	case 'f':
	    filename = optarg;
	    break;
	case 'g':
	    mode = 0;
	    break;
	case 'k':
	    show_keys = 1;
	    break;
	case 'r':
	    show_raw = 1;
	    break;
	case 's':
	    mode = 1;
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
    
    parser = ucl_parser_new(UCL_PARSER_KEY_LOWERCASE);
    
    if (filename != NULL) {
	ucl_parser_add_file(parser, filename);
    } else {
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
    
    if (mode == 0) { /* Get Mode */
	if (argc > 0) {
	    for (k = 0; k < argc; k++) {
		get_mode(argv[k]);
	    }
	} else {
	    fprintf(stderr, "Error: No search performed.\n");
	}
    } else if (mode == 1) { /* Set Mode */
	fprintf(stderr, "Error: Set mode not implemented yet.\n");
	ret = 99;
	goto end;
    }

end:
    if (parser != NULL) {
	ucl_parser_free(parser);
    }
    if (root_obj != NULL) {
	ucl_object_unref(root_obj);
    }

    return ret;
}

void
usage()
{
    fprintf(stderr, "%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n"
"%s\n%s\n%s\n%s\n%s\n%s\n",
"Usage: ucl [--debug] [--file filename] [--keys] [--raw] --get variable",
"       ucl [--debug] [--file filename] [--keys] [--raw] --set variable UCL",
"",
"OPTIONS:",
"       --debug     enable verbose debugging output",
"       --file      path to a file to read or write",
"       --keys      show key=value rather than just the value",
"       --raw       do not enclose strings in quotes",
"       --get       read a variable",
"       --set       write a block of UCL ",
"       variable    The key of the variable to read, in object notation",
"       UCL         A block of UCL to be written to the specified variable",
"",
"EXAMPLES:",
"       ucl --file vmconfig --get name",
"           \"value\"",
"",
"       ucl --file vmconfig --keys --raw --get name",
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
    int command_count = 0, i;
    
    found_object = root_obj;
    
    if (strlen(node_name) == 0) {
	/* Requested root node */
	if (debug > 0) {
	    fprintf(stderr, "DEBUG: Using root node\n");
	}
	found_object = root_obj;
    } else {
	/* Search for selected node */
	if (debug > 0) {
	    fprintf(stderr, "DEBUG: Searching node %s\n", node_name);
	}
	found_object = ucl_lookup_path(found_object, node_name);
    }
    
    while (command_str != NULL) {
	if (debug > 0) {
	    fprintf(stderr, "DEBUG: Performing \"%s\" command on \"%s\"...\n",
		command_str, node_name);
	}
	int done = process_get_command(found_object, command_str, cmd, 1);
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
	output_key(found_object, node_name);
    }
}

int
process_get_command(const ucl_object_t *obj, const char *command_str,
    char *remaining_commands, int recurse)
{
    ucl_object_iter_t it = NULL;
    const ucl_object_t *cur;
    int command_count = 0, loopcount = 0;
    int recurse_level = recurse;
    
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
		printf("%s=", obj->key);
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
		printf("%s=", obj->key);
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
		output_key(cur, ucl_object_key(cur));
		loopcount++;
	    }
	}
	if (loopcount == 0 && debug > 0) {
	    fprintf(stderr, "DEBUG: Found 0 values\n");
	}
    } else if (strcmp(command_str, "each") == 0) {
	if (remaining_commands == NULL) {
	    /* Not a valid command */
	    fprintf(stderr, "Error: the \"each\" command must be followed by"
		"another command\n\n");
	    exit(1);
	}
	if (obj != NULL) {
	    /* Return the values of the current object */
	    char *rcmds = malloc(strlen(remaining_commands) + 1);
	    strcpy(rcmds, remaining_commands);
	    char *next_command = strsep(&rcmds, "|");
	    if (next_command != NULL) {
		while ((cur = ucl_iterate_object(obj, &it, true))) {
		    recurse_level = process_get_command(cur, next_command, rcmds,
			recurse + 1);
		    loopcount++;
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
	obj = ucl_lookup_path(obj, command_str);
	recurse_level = recurse + 1;
	loopcount++;
    } else {
	/* Not a valid command */
	fprintf(stderr, "Error: invalid command %s\n\n", command_str);
	exit(1);
    }
    command_count++;
    if (debug >= 2) {
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
output_key(const ucl_object_t *obj, const char *key)
{
    if (obj == NULL) {
	if (show_keys == 1) {
	    printf("%s=", key);
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
	    printf("%s=", obj->key);
	printf("{object}\n");
	break;
    case UCL_ARRAY:
	if (debug >= 3) {
	    fprintf(stderr, "DEBUG: key=%s\nlen=%u\ntype=UCL_ARRAY\n"
		"value=[array]\n", obj->key, obj->len);
	}
	if (show_keys == 1)
	    printf("%s=", obj->key);
	printf("[array]\n");
	break;
    case UCL_INT:
	if (debug >= 3) {
	    fprintf(stderr, "DEBUG: key=%s\nlen=%u\ntype=UCL_INT\nvalue=%jd\n",
		obj->key, obj->len, (intmax_t)ucl_object_toint(obj));
	}
	if (show_keys == 1)
	    printf("%s=", obj->key);
	printf("%jd\n", (intmax_t)ucl_object_toint(obj));
	break;
    case UCL_FLOAT:
	if (debug >= 3) {
	    fprintf(stderr, "DEBUG: key=%s\nlen=%u\ntype=UCL_FLOAT\nvalue=%f\n",
		obj->key, obj->len, ucl_object_todouble(obj));
	}
	if (show_keys == 1)
	    printf("%s=", obj->key);
	printf("%f\n", ucl_object_todouble(obj));
	break;
    case UCL_STRING:
	if (debug >= 3) {
	    fprintf(stderr, "DEBUG: key=%s\nlen=%u\ntype=UCL_STRING\n"
		"value=\"%s\"\n", obj->key, obj->len, ucl_object_tostring(obj));
	}
	if (show_keys == 1)
	    printf("%s=", obj->key);
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
	    printf("%s=", obj->key);
	printf("%s\n", ucl_object_tostring_forced(obj));
	break;
    case UCL_TIME:
	if (debug >= 3) {
	    fprintf(stderr, "DEBUG: key=%s\nlen=%u\ntype=UCL_TIME\nvalue=%f\n",
		obj->key, obj->len, ucl_object_todouble(obj));
	}
	if (show_keys == 1)
	    printf("%s=", obj->key);
	printf("%f\n", ucl_object_todouble(obj));
	break;
    case UCL_USERDATA:
	if (debug >= 3) {
	    fprintf(stderr, "DEBUG: key=%s\nlen=%u\ntype=UCL_USERDATA\n"
		"value=%p\n", obj->key, obj->len, obj->value.ud);
	}
	if (show_keys == 1)
	    printf("%s=", obj->key);
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
