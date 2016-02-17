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
output_main(int argc, char *argv[])
{
    const char *filename = NULL;
    int ret = 0, ch;

    /* Initialize parser */
    parser = ucl_parser_new(UCLCMD_PARSER_FLAGS);

    /*	options	descriptor */
    static struct option longopts[] = {
	{ "file",       required_argument,      NULL,       	'f' },
	{ "input",    	no_argument,            NULL,  		'i' },
	{ NULL,         0,                      NULL,       	0 }
    };

    while ((ch = getopt_long(argc, argv, "f:i:", longopts, NULL)) != -1) {
	switch (ch) {
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
	default:
	    fprintf(stderr, "Error: Unexpected option: %i\n", ch);
	    usage();
	    break;
	}
    }
    argc -= optind;
    argv += optind;

    if (filename == NULL) {
	root_obj = parse_input(parser, stdin);
    }

    ucl_obj_dump(root_obj, 0);

    cleanup();

    if (nonewline) {
	printf("\n");
    }

    return(ret);
}

void
output_chunk(const ucl_object_t *obj, char *nodepath, const char *inkey)
{
    unsigned char *result = NULL;
    char *key = strdup(inkey);

    if (shvars == true) {
	replace_sep(nodepath, '.', '_');
    }
    replace_sep(nodepath, input_sepchar, output_sepchar);
    replace_sep(key, input_sepchar, output_sepchar);

    switch (output_type) {
    case 254: /* Text */
	output_key(obj, nodepath, key);
	break;
    case UCL_EMIT_CONFIG: /* UCL */
	result = ucl_object_emit(obj, output_type);
	if (nonewline) {
	    fprintf(stderr, "WARN: UCL output cannot be 'nonewline'd\n");
	}
	if (show_keys == 1 && strlen(key) > 0)
	    printf("%s%s=", nodepath, key);
	printf("%s", result);
	free(result);
	if (nonewline) {
	    firstline = false;
	} else {
	    printf("\n");
	}
	break;
    case UCL_EMIT_JSON: /* JSON */
	result = ucl_object_emit(obj, output_type);
	if (nonewline) {
	    fprintf(stderr,
		"WARN: non-compact JSON output cannot be 'nonewline'd\n");
	}
	if (show_keys == 1 && strlen(key) > 0)
	    printf("%s%s=", nodepath, key);
	printf("%s", result);
	free(result);
	if (nonewline) {
	    firstline = false;
	} else {
	    printf("\n");
	}
	break;
    case UCL_EMIT_JSON_COMPACT: /* Compact JSON */
	result = ucl_object_emit(obj, output_type);
	if (show_keys == 1 && strlen(key) > 0)
	    printf("%s%s=", nodepath, key);
	printf("%s", result);
	free(result);
	if (nonewline) {
	    firstline = false;
	} else {
	    printf("\n");
	}
	break;
    case UCL_EMIT_YAML: /* YAML */
	result = ucl_object_emit(obj, output_type);
	if (nonewline) {
	    fprintf(stderr, "WARN: YAML output cannot be 'nonewline'd\n");
	}
	if (show_keys == 1 && strlen(key) > 0)
	    printf("%s%s=", nodepath, key);
	printf("%s", result);
	free(result);
	if (nonewline) {
	    firstline = false;
	} else {
	    printf("\n");
	}
	break;
    case UCL_EMIT_MSGPACK: /* Msgpack */
	result = ucl_object_emit(obj, output_type);
	if (nonewline) {
	    fprintf(stderr, "WARN: Msgpack output cannot be 'nonewline'd\n");
	}
	if (show_keys == 1 && strlen(key) > 0)
	    printf("%s%s=", nodepath, key);
	printf("%s", result);
	free(result);
	if (nonewline) {
	    firstline = false;
	} else {
	    printf("\n");
	}
	break;
    default:
	fprintf(stderr, "Error: Invalid output mode: %i\n",
	    output_type);
	break;
    }

    free(key);
}

void
output_key(const ucl_object_t *obj, char *nodepath, const char *inkey)
{
    char *key;

    if (inkey == NULL) {
	uclcmd_asprintf(&key, "");
    } else {
	key = strdup(inkey);
    }

    replace_sep(nodepath, input_sepchar, output_sepchar);
    replace_sep(key, input_sepchar, output_sepchar);
    if (firstline == false) {
	printf(" ");
    }
    if (obj == NULL) {
	if (show_keys == 1) {
	    printf("%s%s=", nodepath, key);
	}
	printf("null");
	if (nonewline) {
	    firstline = false;
	} else {
	    printf("\n");
	}
	return;
    }
    switch (ucl_object_type(obj)) {
    case UCL_OBJECT:
	if (debug >= 3) {
	    fprintf(stderr, "DEBUG: key=%s\nlen=%u\ntype=UCL_OBJECT\n"
		"value={object}\n", obj->key, obj->len);
	}
	if (show_keys == 1)
	    printf("%s%s=", nodepath, key);
	printf("{object}");
	break;
    case UCL_ARRAY:
	if (debug >= 3) {
	    fprintf(stderr, "DEBUG: key=%s\nlen=%u\ntype=UCL_ARRAY\n"
		"value=[array]\n", obj->key, obj->len);
	}
	if (show_keys == 1)
	    printf("%s%s=", nodepath, key);
	printf("[array]");
	break;
    case UCL_INT:
	if (debug >= 3) {
	    fprintf(stderr, "DEBUG: key=%s\nlen=%u\ntype=UCL_INT\nvalue=%jd\n",
		obj->key, obj->len, (intmax_t)ucl_object_toint(obj));
	}
	if (show_keys == 1)
	    printf("%s%s=", nodepath, key);
	printf("%jd", (intmax_t)ucl_object_toint(obj));
	break;
    case UCL_FLOAT:
	if (debug >= 3) {
	    fprintf(stderr, "DEBUG: key=%s\nlen=%u\ntype=UCL_FLOAT\nvalue=%f\n",
		obj->key, obj->len, ucl_object_todouble(obj));
	}
	if (show_keys == 1)
	    printf("%s%s=", nodepath, key);
	printf("%f", ucl_object_todouble(obj));
	break;
    case UCL_STRING:
	if (debug >= 3) {
	    fprintf(stderr, "DEBUG: key=%s\nlen=%u\ntype=UCL_STRING\n"
		"value=\"%s\"\n", obj->key, obj->len, ucl_object_tostring(obj));
	}
	if (show_keys == 1)
	    printf("%s%s=", nodepath, key);
	if (show_raw == 1)
	    printf("%s", ucl_object_tostring(obj));
	else
	    printf("\"%s\"", ucl_object_tostring(obj));
	break;
    case UCL_BOOLEAN:
	if (debug >= 3) {
	    fprintf(stderr, "DEBUG: key=%s\nlen=%u\ntype=UCL_BOOLEAN\n"
		"value=%s\n", obj->key, obj->len,
		ucl_object_tostring_forced(obj));
	}
	if (show_keys == 1)
	    printf("%s%s=", nodepath, key);
	printf("%s", ucl_object_tostring_forced(obj));
	break;
    case UCL_TIME:
	if (debug >= 3) {
	    fprintf(stderr, "DEBUG: key=%s\nlen=%u\ntype=UCL_TIME\nvalue=%f\n",
		obj->key, obj->len, ucl_object_todouble(obj));
	}
	if (show_keys == 1)
	    printf("%s%s=", nodepath, key);
	printf("%f", ucl_object_todouble(obj));
	break;
    case UCL_USERDATA:
	if (debug >= 3) {
	    fprintf(stderr, "DEBUG: key=%s\nlen=%u\ntype=UCL_USERDATA\n"
		"value=%p\n", obj->key, obj->len, obj->value.ud);
	}
	if (show_keys == 1)
	    printf("%s%s=", nodepath, key);
	printf("{userdata}");
	break;
    default:
	if (debug >= 3) {
	    printf("error=Object of unknown type\n");
	    fprintf(stderr, "DEBUG: key=%s\nlen=%u\ntype=UCL_ERROR\n"
		"value=null\n", obj->key, obj->len);
	}
	break;
    }
    if (nonewline) {
	firstline = false;
    } else {
	printf("\n");
    }

    free(key);
}

void
ucl_obj_dump(const ucl_object_t *obj, unsigned int shift)
{
    int num = shift * 2 + 5;
    char *pre = (char *) malloc (num * sizeof(char));
    const ucl_object_t *cur, *cur2;
    ucl_object_iter_t it = NULL, it2 = NULL;

    it = ucl_object_iterate_new(obj);
    it2 = ucl_object_iterate_new(obj);

    pre[--num] = 0x00;
    while (num--)
	    pre[num] = 0x20;

    while ((cur = ucl_object_iterate_safe(it, false))) {
	printf ("%sucl object address: %p\n", pre + 4, obj);
	if (cur->key != NULL) {
	    printf ("%skey: \"%s\"\n", pre, ucl_object_key (cur));
	}
	printf ("%sref: %u\n", pre, cur->ref);
	printf ("%slen: %u\n", pre, cur->len);
	printf ("%sprev: %p\n", pre, cur->prev);
	printf ("%snext: %p\n", pre, cur->next);
	printf ("%spriority: %d\n", pre, (cur->flags >> ((sizeof (cur->flags) * 8) - 4)));
	printf ("%sflags: %x\n", pre, (cur->flags & 0xfff));
	if (ucl_object_type(cur) == UCL_OBJECT) {
	    printf ("%stype: UCL_OBJECT\n", pre);
	    printf ("%svalue: %p\n", pre, cur->value.ov);
	    it2 = ucl_object_iterate_reset (it2, cur);
	    while ((cur2 = ucl_object_iterate_safe(it2, true))) {
		ucl_obj_dump (cur2, shift + 2);
	    }
	}
	else if (ucl_object_type(cur) == UCL_ARRAY) {
	    printf ("%stype: UCL_ARRAY\n", pre);
	    printf ("%svalue: %p\n", pre, cur->value.av);
	    it2 = ucl_object_iterate_reset (it2, cur);
	    while ((cur2 = ucl_object_iterate_safe(it2, true))) {
		ucl_obj_dump (cur2, shift + 2);
	    }
	}
	else if (ucl_object_type(cur) == UCL_INT) {
	    printf ("%stype: UCL_INT\n", pre);
	    printf ("%svalue: %jd\n", pre, (intmax_t)ucl_object_toint (cur));
	}
	else if (ucl_object_type(cur) == UCL_FLOAT) {
	    printf ("%stype: UCL_FLOAT\n", pre);
	    printf ("%svalue: %f\n", pre, ucl_object_todouble (cur));
	}
	else if (ucl_object_type(cur) == UCL_STRING) {
	    printf ("%stype: UCL_STRING\n", pre);
	    printf ("%svalue: \"%s\"\n", pre, ucl_object_tostring (cur));
	}
	else if (ucl_object_type(cur) == UCL_BOOLEAN) {
	    printf ("%stype: UCL_BOOLEAN\n", pre);
	    printf ("%svalue: %s\n", pre, ucl_object_tostring_forced (cur));
	}
	else if (ucl_object_type(cur) == UCL_TIME) {
	    printf ("%stype: UCL_TIME\n", pre);
	    printf ("%svalue: %f\n", pre, ucl_object_todouble (cur));
	}
	else if (ucl_object_type(cur) == UCL_USERDATA) {
	    printf ("%stype: UCL_USERDATA\n", pre);
	    printf ("%svalue: %p\n", pre, cur->value.ud);
	}
    }

    ucl_object_iterate_free (it);
    free (pre);
}
