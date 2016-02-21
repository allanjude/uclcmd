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

/*
 * I'll just leave this here for you
 * http://abstrusegoose.com/432
 */

#ifndef UCLCMD_H_
#define UCLCMD_H_

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <ucl.h>

#ifndef __DECONST
#define __DECONST(type, var)    ((type)(uintptr_t)(const void *)(var))
#endif

#define UCLCMD_PARSER_FLAGS	UCL_PARSER_KEY_LOWERCASE | \
		UCL_PARSER_NO_IMPLICIT_ARRAYS | UCL_PARSER_SAVE_COMMENTS

extern int debug, expand, noop, nonewline, show_keys, show_raw;
extern bool firstline, shvars;
extern int output_type;
extern ucl_object_t *root_obj;
extern ucl_object_t *set_obj;
extern struct ucl_parser *parser;
extern struct ucl_parser *setparser;
extern char input_sepchar;
extern char output_sepchar;
extern char *include_file;

typedef int (*verb_func_t)(int argc, char *argv[]);

typedef struct verbmap {
	const char *verb;
	verb_func_t callback;
} verbmap_t;

void cleanup();
char* expand_subkeys(const ucl_object_t *obj, char *nodepath);
int get_main(int argc, char *argv[]);
void get_mode(char *requested_node);
ucl_object_t* get_object(char *selected_node);
ucl_object_t* get_parent(char *selected_node);
int merge_main(int argc, char *argv[]);
int merge_mode(char *destination_node, char *data);
bool merge_recursive(ucl_object_t *top, ucl_object_t *elt, bool copy);
void output_chunk(const ucl_object_t *obj, char *nodepath, const char *inkey,
    FILE *target);
int output_file(const ucl_object_t *obj, const char *output_filename);
int replace_file(const ucl_object_t *obj, const char *output_filename);
int output_main(int argc, char *argv[]);
void output_key(const ucl_object_t *obj, char *nodepath, const char *inkey);
ucl_object_t* parse_file(struct ucl_parser *parser, const char *filename);
ucl_object_t* parse_input(struct ucl_parser *parser, FILE *source);
ucl_object_t* parse_string(struct ucl_parser *parser, char *data);
int process_get_command(const ucl_object_t *obj, char *nodepath,
    const char *command_str, char *remaining_commands, int recurse);
int remove_main(int argc, char *argv[]);
void replace_sep(char *key, char oldsep, char newsep);
int set_main(int argc, char *argv[]);
int set_mode(char *destination_node, char *data, ucl_type_t obj_type);
ucl_type_t string_to_type (const char *strtype);
char * type_as_string (ucl_type_t type);
char * objtype_as_string (const ucl_object_t *obj);
void ucl_obj_dump(const ucl_object_t *obj, unsigned int shift);
void ucl_obj_dump_safe(const ucl_object_t *obj, unsigned int shift);
void usage();

int get_cmd_each(const ucl_object_t *obj, char *nodepath,
    const char *command_str, char *remaining_commands, int recurse);
int get_cmd_iterate(const ucl_object_t *obj, char *nodepath,
    const char *command_str, char *remaining_commands, int recurse);
int get_cmd_keys(const ucl_object_t *obj, char *nodepath,
    const char *command_str, char *remaining_commands, int recurse);
int get_cmd_length(const ucl_object_t *obj, char *nodepath,
    const char *command_str, char *remaining_commands, int recurse);
int get_cmd_none(const ucl_object_t *obj, char *nodepath,
    const char *command_str, char *remaining_commands, int recurse);
int get_cmd_recurse(const ucl_object_t *obj, char *nodepath,
    const char *command_str, char *remaining_commands, int recurse);
int get_cmd_type(const ucl_object_t *obj, char *nodepath,
    const char *command_str, char *remaining_commands, int recurse);
int get_cmd_values(const ucl_object_t *obj, char *nodepath,
    const char *command_str, char *remaining_commands, int recurse);

void uclcmd_asprintf(char ** __restrict s, char const * __restrict fmt, ...)
    __printflike(2, 3);

#endif /* UCLCMD_H_ */
