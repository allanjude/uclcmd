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
set_mode(char *destination_node, char *data)
{
    ucl_object_t *dst_obj = NULL;
    ucl_object_t *sub_obj = NULL;
    ucl_object_t *old_obj = NULL;
    int success = 0;

    setparser = ucl_parser_new(UCL_PARSER_KEY_LOWERCASE |
	UCL_PARSER_NO_IMPLICIT_ARRAYS);

    /* Lookup the destination to write to */
    dst_obj = get_parent(destination_node);
    sub_obj = get_object(destination_node);

    if (sub_obj == NULL) {
	return false;
    }

    if (data == NULL || strcmp(data, "-") == 0) {
	/* get UCL to add from stdin */
	set_obj = parse_input(setparser, stdin);
    } else {
	/* User provided data inline */
	if (ucl_object_type(sub_obj) != UCL_OBJECT && ucl_object_type(sub_obj) != UCL_ARRAY) {
	    /* Destination is a scalar etc */
	    set_obj = ucl_object_fromstring_common(data, 0,
		UCL_STRING_PARSE);
	} else {
	    /* Destination is an Object or Array */
	    set_obj = parse_string(setparser, data);
	}
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

    /* Replace it in the object here */
    if (ucl_object_type(dst_obj) == UCL_ARRAY) {
	char *dst_frag = strrchr(destination_node, input_sepchar);

	/* XXX TODO: What if the destination_node only points to an array */
	/* XXX TODO: What if we want to replace an entire array? */
	dst_frag++;
	if (debug > 0) {
	    fprintf(stderr, "Replacing array index %s\n", dst_frag);
	}
	old_obj = ucl_array_replace_index(dst_obj, set_obj, strtoul(dst_frag,
	    NULL, 0));
	success = false;
	if (old_obj != NULL) {
	    ucl_object_unref(old_obj);
	    set_obj = NULL;
	    success = true;
	}
    } else {
	if (debug > 0) {
	    fprintf(stderr, "Replacing key %s\n", ucl_object_key(sub_obj));
	}
	success = ucl_object_replace_key(dst_obj, set_obj,
	    ucl_object_key(sub_obj), 0, true);
	set_obj = NULL;
    }

    return success;
}
