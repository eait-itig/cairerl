/*
%%
%% cairo erlang binding
%%
%% Copyright (c) 2014, The University of Queensland
%% Author: Alex Wilson <alex@uq.edu.au>
%%
%% Redistribution and use in source and binary forms, with or without
%% modification, are permitted provided that the following conditions are met:
%%
%%  * Redistributions of source code must retain the above copyright notice,
%%    this list of conditions and the following disclaimer.
%%  * Redistributions in binary form must reproduce the above copyright notice,
%%    this list of conditions and the following disclaimer in the documentation
%%    and/or other materials provided with the distribution.
%%
%% THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
%% AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
%% IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
%% ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
%% LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
%% CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED  TO, PROCUREMENT OF
%% SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR  BUSINESS
%% INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
%% CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
%% ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
%% POSSIBILITY OF SUCH DAMAGE.
%%
*/

#if !defined(_COMMON_H)
#define _COMMON_H

#include <cairo.h>
#include <stdint.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "erl_nif.h"

#include "tree.h"

enum tag_type {
	TAG_DOUBLE,
	TAG_TEXT_EXTENTS,
	TAG_PATTERN,
	TAG_PATH,
	TAG_FONT_EXTENTS
};

struct tag_node;
struct tag_node {
	RB_ENTRY(tag_node) entry;
	ERL_NIF_TERM tag;
	enum tag_type type;
	union {
		void *v_ptr;
		double v_dbl;
		cairo_text_extents_t *v_text_exts;
		cairo_pattern_t *v_pattern;
		cairo_path_t *v_path;
		cairo_font_extents_t *v_font_exts;
	};
};

struct context {
	cairo_t *cairo;
	cairo_surface_t *sfc;
	int w, h;
	ErlNifBinary out;
	RB_HEAD(tag_tree, tag_node) tag_head;
};

enum op_return {
	OP_OK = 0,
	ERR_NOT_TUPLE = -1,
	ERR_NOT_ATOM = -2,
	ERR_UNKNOWN_OP = -3,
	ERR_NOT_INIT = -4,
	ERR_FAILURE = -5,
	ERR_TAG_ALREADY = -6,
	ERR_TAG_NOT_SET = -7,
	ERR_BAD_ARGS = -10
};

int tag_cmp(struct tag_node *, struct tag_node *);
RB_PROTOTYPE(tag_tree, tag_node, entry, tag_cmp);

struct op_handler {
	const char *name;
	enum op_return (*handler)(ErlNifEnv*, struct context *, const ERL_NIF_TERM *, int);
};

extern struct op_handler op_handlers[];
extern const int n_handlers;

int get_tag_double(ErlNifEnv *, struct context *, const ERL_NIF_TERM, double *);
void *get_tag_ptr(ErlNifEnv *, struct context *, enum tag_type, const ERL_NIF_TERM);
enum op_return set_tag_double(ErlNifEnv *, struct context *, const ERL_NIF_TERM, double);
enum op_return set_tag_ptr(ErlNifEnv *, struct context *, const ERL_NIF_TERM, enum tag_type, void *);
int create_surface_from_image(ErlNifEnv *, const ERL_NIF_TERM, cairo_surface_t **, ERL_NIF_TERM *);

#endif
