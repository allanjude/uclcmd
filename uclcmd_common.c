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

char*
expand_subkeys(const ucl_object_t *obj, char *nodepath)
{
	char *result = NULL;
	int count = 0;
	ucl_object_iter_t it = NULL;
	const ucl_object_t *cur;

	result = malloc(1024); /* XXX: use sbuf instead */
	result[0] = '\0';
	if (obj != NULL) {
	    /* Compile a list of the keys in the current object */
	    while ((cur = ucl_iterate_object(obj, &it, true))) {
		if (!ucl_object_key(cur))
		    continue;
		if (strlen(result) > (1024 - 5 - strlen(ucl_object_key(cur)))) {
			result = strcat(result, " ...");
			break;
		}
		if (count)
		    result = strcat(result, " ");
		result = strcat(result, ucl_object_key(cur));
		count++;
	    }
	}
	return result;
}

ucl_object_t*
get_object(char *selected_node)
{
    char *dst_key = selected_node;
    char *dst_prefix = NULL;
    char *dst_frag = NULL;
    ucl_object_t *parent_obj = NULL;
    ucl_object_t *selected_obj = NULL;

    if (strlen(dst_key) == 1 && dst_key[0] == input_sepchar) {
	dst_key++;
    }
    dst_prefix = strdup(dst_key);
    dst_frag = strrchr(dst_prefix, input_sepchar);

    if (dst_frag == NULL || strlen(dst_frag) == 0) {
	dst_frag = dst_key;
	parent_obj = root_obj;
    } else {
	/*
	 * dst_frag is a pointer to the last period in dst_prefix, write a
	 * null byte to split the two strings, and then advance the pointer
	 * to the first byte after the period
	 */
	dst_frag[0] = '\0';
	dst_frag++;
	parent_obj = __DECONST(ucl_object_t *,
	    ucl_lookup_path_char(root_obj, dst_prefix, input_sepchar));
	if (parent_obj == NULL) {
	    free(dst_prefix);
	    return NULL;
	}
	if (ucl_object_type(parent_obj) == UCL_ARRAY) {
	    selected_obj = __DECONST(ucl_object_t *,
		ucl_array_find_index(parent_obj, strtoul(dst_frag, NULL, 10)));
	} else {
	    selected_obj = __DECONST(ucl_object_t *,
		ucl_object_find_key(parent_obj, dst_frag));
	}
    }
    if (selected_obj == NULL) {
	selected_obj = parent_obj;
    }

    if (debug > 0) {
	fprintf(stderr, "selecting key: %s\n", dst_key);
	fprintf(stderr, "intended sub-key: %s\n", dst_frag);
	fprintf(stderr, "selected sub-key: %s\n", ucl_object_key(selected_obj));
    }

    free(dst_prefix);

    return selected_obj;
}

ucl_object_t*
get_parent(char *selected_node)
{
    char *dst_key = selected_node;
    char *dst_prefix = NULL;
    char *dst_frag = NULL;
    ucl_object_t *parent_obj = NULL;

    if (strlen(dst_key) == 1 && dst_key[0] == input_sepchar) {
	dst_key++;
    }
    dst_prefix = strdup(dst_key);
    dst_frag = strrchr(dst_prefix, input_sepchar);

    if (dst_frag == NULL || strlen(dst_frag) == 0) {
	dst_frag = dst_key;
	parent_obj = root_obj;
    } else {
	/*
	 * dst_frag is a pointer to the last period in dst_prefix, write a
	 * null byte to split the two strings, and then advance the pointer
	 * to the first byte after the period
	 */
	dst_frag[0] = '\0';
	dst_frag++;
	parent_obj = __DECONST(ucl_object_t *,
	    ucl_lookup_path_char(root_obj, dst_prefix, input_sepchar));
    }

    free(dst_prefix);

    if (parent_obj == NULL) {
	return NULL;
    }

    return parent_obj;
}

void
replace_sep(char *key, char oldsep, char newsep)
{
    int idx = 0;
    if (oldsep == newsep) return;
    while (key[idx] != '\0') {
	if (key[idx] == oldsep) {
	    key[idx] = newsep;
	}
	idx++;
    }
}

char *
type_as_string (const ucl_object_t *obj)
{
    char *ret = NULL;

    if (obj == NULL) {
	return NULL;
    } else if (ucl_object_type(obj) == UCL_OBJECT) {
	asprintf(&ret, "UCL_OBJECT");
    }
    else if (ucl_object_type(obj) == UCL_ARRAY) {
	asprintf(&ret, "UCL_ARRAY");
    }
    else if (ucl_object_type(obj) == UCL_INT) {
	asprintf(&ret, "UCL_INT");
    }
    else if (ucl_object_type(obj) == UCL_FLOAT) {
	asprintf(&ret, "UCL_FLOAT");
    }
    else if (ucl_object_type(obj) == UCL_STRING) {
	asprintf(&ret, "UCL_STRING");
    }
    else if (ucl_object_type(obj) == UCL_BOOLEAN) {
	asprintf(&ret, "UCL_BOOLEAN");
    }
    else if (ucl_object_type(obj) == UCL_TIME) {
	asprintf(&ret, "UCL_TIME");
    }
    else if (ucl_object_type(obj) == UCL_USERDATA) {
	asprintf(&ret, "UCL_USERDATA");
    }
    else if (ucl_object_type(obj) == UCL_NULL) {
	asprintf(&ret, "UCL_NULL");
    }

    return ret;
}
