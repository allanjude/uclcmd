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

#ifndef UCLCMD_H_
#define UCLCMD_H_

#include <errno.h>
#include <getopt.h>
#include <stdio.h>

#include <ucl.h>

#ifndef __DECONST
#define __DECONST(type, var)    ((type)(uintptr_t)(const void *)(var))
#endif

extern int debug, expand, mode, nonewline, show_keys, show_raw;
extern bool firstline;
extern int output_type;
extern ucl_object_t *root_obj;
extern ucl_object_t *set_obj;
extern struct ucl_parser *parser;
extern struct ucl_parser *setparser;
extern char input_sepchar;
extern char output_sepchar;

void usage();
void cleanup();

char* expand_subkeys(const ucl_object_t *obj, char *nodepath);
ucl_object_t* get_object(char *selected_node);
ucl_object_t* get_parent(char *selected_node);
void replace_sep(char *key, char oldsep, char newsep);
char * type_as_string (const ucl_object_t *obj);
int set_mode(char *destination_node, char *data);
ucl_object_t* parse_file(struct ucl_parser *parser, const char *filename);
ucl_object_t* parse_input(struct ucl_parser *parser, FILE *source);
ucl_object_t* parse_string(struct ucl_parser *parser, char *data);
int merge_mode(char *destination_node, char *data);
void get_mode(char *requested_node);
int process_get_command(const ucl_object_t *obj, char *nodepath,
    const char *command_str, char *remaining_commands, int recurse);
void output_chunk(const ucl_object_t *obj, char *nodepath, const char *inkey);
void output_key(const ucl_object_t *obj, char *nodepath, const char *inkey);
void ucl_obj_dump (const ucl_object_t *obj, unsigned int shift);

#endif /* UCLCMD_H_ */
