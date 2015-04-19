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

ucl_object_t*
parse_file(struct ucl_parser *parser, const char *filename)
{
    ucl_object_t *obj = NULL;

    ucl_parser_add_file(parser, filename);

    if (ucl_parser_get_error(parser)) {
	fprintf(stderr, "Error occured: %s\n",
	    ucl_parser_get_error(parser));
	cleanup();
	exit(2);
    }

    obj = ucl_parser_get_object(parser);
    if (ucl_parser_get_error(parser)) {
	fprintf(stderr, "Error: Parse Error occured: %s\n",
	    ucl_parser_get_error(parser));
	cleanup();
	exit(3);
    }

    return obj;
}

ucl_object_t*
parse_input(struct ucl_parser *parser, FILE *source)
{
    unsigned char inbuf[8192];
    int r = 0;
    ucl_object_t *obj = NULL;
    bool success = false;

    while (!feof(source) && r < (int)sizeof(inbuf)) {
	r += fread(inbuf + r, 1, sizeof(inbuf) - r, source);
    }
    success = ucl_parser_add_chunk(parser, inbuf, r);
    fclose(source);

    if (success == false) {
	/* There must be a better way to detect a string */
	ucl_parser_clear_error(parser);
	success = true;
	obj = ucl_object_fromstring_common(data, 0, UCL_STRING_PARSE);
    } else {
	obj = ucl_parser_get_object(parser);
    }

    if (ucl_parser_get_error(parser)) {
	fprintf(stderr, "Error: Parse Error occured: %s\n",
	    ucl_parser_get_error(parser));
	cleanup();
	exit(3);
    }

    return obj;
}

ucl_object_t*
parse_string(struct ucl_parser *parser, char *data)
{
    ucl_object_t *obj = NULL;
    bool success = false;

    success = ucl_parser_add_string(parser, data, 0);
    if (success == false) {
	/* There must be a better way to detect a string */
	ucl_parser_clear_error(parser);
	success = true;
	obj = ucl_object_fromstring_common(data, 0, UCL_STRING_PARSE);
    } else {
	obj = ucl_parser_get_object(parser);
    }

    if (ucl_parser_get_error(parser)) {
	fprintf(stderr, "Error: Parse Error occured: %s\n",
	    ucl_parser_get_error(parser));
	cleanup();
	exit(3);
    }

    return obj;
}
