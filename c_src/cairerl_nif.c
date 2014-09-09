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
	TAG_PATH
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

static int
tag_cmp(struct tag_node *n1, struct tag_node *n2)
{
	return enif_compare(n1->tag, n2->tag);
}

RB_GENERATE(tag_tree, tag_node, entry, tag_cmp);

struct op_handler {
	const char *name;
	enum op_return (*handler)(ErlNifEnv*, struct context *, const ERL_NIF_TERM *, int);
};

static int
get_tag_double(ErlNifEnv *env, struct context *ctx, const ERL_NIF_TERM tagOrValue, double *out)
{
	struct tag_node node;
	struct tag_node *found;

	if (enif_get_double(env, tagOrValue, out)) {
		return 1;
	} else {
		memset(&node, 0, sizeof(node));
		node.tag = tagOrValue;

		found = RB_FIND(tag_tree, &ctx->tag_head, &node);
		if (found == NULL)
			return 0;

		if (found->type != TAG_DOUBLE)
			return 0;

		*out = found->v_dbl;
		return 1;
	}
}

static enum op_return
set_tag_double(ErlNifEnv *env, struct context *ctx, const ERL_NIF_TERM tag, double value)
{
	struct tag_node *tn, *rn;

	tn = enif_alloc(sizeof(*tn));
	memset(tn, 0, sizeof(*tn));

	tn->tag = tag;
	tn->type = TAG_DOUBLE;
	tn->v_dbl = value;

	rn = RB_INSERT(tag_tree, &ctx->tag_head, tn);
	if (rn != NULL) {
		enif_free(tn);
		return ERR_TAG_ALREADY;
	}

	return OP_OK;
}

static enum op_return
set_tag_ptr(ErlNifEnv *env, struct context *ctx, const ERL_NIF_TERM tag, enum tag_type type, void *value)
{
	struct tag_node *tn, *rn;

	tn = enif_alloc(sizeof(*tn));
	memset(tn, 0, sizeof(*tn));

	tn->tag = tag;
	tn->type = type;
	tn->v_ptr = value;

	rn = RB_INSERT(tag_tree, &ctx->tag_head, tn);
	if (rn != NULL) {
		enif_free(tn);
		return ERR_TAG_ALREADY;
	}

	return OP_OK;
}

static enum op_return
handle_op_arc(ErlNifEnv *env, struct context *ctx, const ERL_NIF_TERM *argv, int argc)
{
	if (ctx->cairo == NULL)
		return ERR_NOT_INIT;
	if (argc != 5)
		return ERR_BAD_ARGS;
	return OP_OK;
}

static enum op_return
handle_op_new_sub_path(ErlNifEnv *env, struct context *ctx, const ERL_NIF_TERM *argv, int argc)
{
	if (ctx->cairo == NULL)
		return ERR_NOT_INIT;
	if (argc != 0)
		return ERR_BAD_ARGS;
	cairo_new_sub_path(ctx->cairo);
	return OP_OK;
}

static enum op_return
handle_op_close_path(ErlNifEnv *env, struct context *ctx, const ERL_NIF_TERM *argv, int argc)
{
	if (ctx->cairo == NULL)
		return ERR_NOT_INIT;
	if (argc != 0)
		return ERR_BAD_ARGS;
	cairo_close_path(ctx->cairo);
	return OP_OK;
}

static enum op_return
handle_op_identity_matrix(ErlNifEnv *env, struct context *ctx, const ERL_NIF_TERM *argv, int argc)
{
	if (ctx->cairo == NULL)
		return ERR_NOT_INIT;
	if (argc != 0)
		return ERR_BAD_ARGS;
	cairo_identity_matrix(ctx->cairo);
	return OP_OK;
}

static enum op_return
handle_op_translate(ErlNifEnv *env, struct context *ctx, const ERL_NIF_TERM *argv, int argc)
{
	double x, y;
	if (ctx->cairo == NULL)
		return ERR_NOT_INIT;
	if (argc != 2)
		return ERR_BAD_ARGS;

	if (!get_tag_double(env, ctx, argv[0], &x))
		return ERR_BAD_ARGS;
	if (!get_tag_double(env, ctx, argv[1], &y))
		return ERR_BAD_ARGS;

	cairo_translate(ctx->cairo, x, y);

	return OP_OK;
}

static enum op_return
handle_op_rectangle(ErlNifEnv *env, struct context *ctx, const ERL_NIF_TERM *argv, int argc)
{
	double x, y, w, h;

	if (ctx->cairo == NULL)
		return ERR_NOT_INIT;
	if (argc != 4)
		return ERR_BAD_ARGS;

	if (!get_tag_double(env, ctx, argv[0], &x))
		return ERR_BAD_ARGS;
	if (!get_tag_double(env, ctx, argv[1], &y))
		return ERR_BAD_ARGS;
	if (!get_tag_double(env, ctx, argv[2], &w))
		return ERR_BAD_ARGS;
	if (!get_tag_double(env, ctx, argv[3], &h))
		return ERR_BAD_ARGS;

	cairo_rectangle(ctx->cairo, x, y, w, h);
	return OP_OK;
}

static enum op_return
handle_op_move_to(ErlNifEnv *env, struct context *ctx, const ERL_NIF_TERM *argv, int argc)
{
	double x, y;
	int relative = 0;
	ERL_NIF_TERM head, tail, relatom;

	if (ctx->cairo == NULL)
		return ERR_NOT_INIT;
	if (argc != 3)
		return ERR_BAD_ARGS;

	if (!get_tag_double(env, ctx, argv[0], &x))
		return ERR_BAD_ARGS;
	if (!get_tag_double(env, ctx, argv[1], &y))
		return ERR_BAD_ARGS;

	relatom = enif_make_atom(env, "relative");
	tail = argv[2];
	while (enif_get_list_cell(env, tail, &head, &tail)) {
		if (enif_is_identical(head, relatom)) {
			relative = 1;
		}
	}
	if (relative) {
		cairo_rel_move_to(ctx->cairo, x, y);
	} else {
		cairo_move_to(ctx->cairo, x, y);
	}
	return OP_OK;
}

static enum op_return
handle_op_line_to(ErlNifEnv *env, struct context *ctx, const ERL_NIF_TERM *argv, int argc)
{
	double x, y;
	int relative = 0;
	ERL_NIF_TERM head, tail, relatom;

	if (ctx->cairo == NULL)
		return ERR_NOT_INIT;
	if (argc != 3)
		return ERR_BAD_ARGS;

	if (!get_tag_double(env, ctx, argv[0], &x))
		return ERR_BAD_ARGS;
	if (!get_tag_double(env, ctx, argv[1], &y))
		return ERR_BAD_ARGS;

	relatom = enif_make_atom(env, "relative");
	tail = argv[2];
	while (enif_get_list_cell(env, tail, &head, &tail)) {
		if (enif_is_identical(head, relatom)) {
			relative = 1;
		}
	}
	if (relative) {
		cairo_rel_line_to(ctx->cairo, x, y);
	} else {
		cairo_line_to(ctx->cairo, x, y);
	}
	return OP_OK;
}

static enum op_return
handle_op_set_source_rgba(ErlNifEnv *env, struct context *ctx, const ERL_NIF_TERM *argv, int argc)
{
	double r, g, b, a;
	if (ctx->cairo == NULL)
		return ERR_NOT_INIT;
	if (argc != 4)
		return ERR_BAD_ARGS;

	if (!enif_get_double(env, argv[0], &r))
		return ERR_BAD_ARGS;
	if (!enif_get_double(env, argv[1], &g))
		return ERR_BAD_ARGS;
	if (!enif_get_double(env, argv[2], &b))
		return ERR_BAD_ARGS;
	if (!enif_get_double(env, argv[3], &a))
		return ERR_BAD_ARGS;

	cairo_set_source_rgba(ctx->cairo, r, g, b, a);

	return OP_OK;
}

static enum op_return
handle_op_clip(ErlNifEnv *env, struct context *ctx, const ERL_NIF_TERM *argv, int argc)
{
	ERL_NIF_TERM head, tail, psatom;
	int preserve = 0;
	if (ctx->cairo == NULL)
		return ERR_NOT_INIT;
	if (argc != 1)
		return ERR_BAD_ARGS;
	psatom = enif_make_atom(env, "preserve");
	tail = argv[0];
	while (enif_get_list_cell(env, tail, &head, &tail)) {
		if (enif_is_identical(head, psatom)) {
			preserve = 1;
		}
	}
	if (preserve) {
		cairo_clip_preserve(ctx->cairo);
	} else {
		cairo_clip(ctx->cairo);
	}
	return OP_OK;
}

static enum op_return
handle_op_paint(ErlNifEnv *env, struct context *ctx, const ERL_NIF_TERM *argv, int argc)
{
	ERL_NIF_TERM udatom;
	double alpha;
	if (ctx->cairo == NULL)
		return ERR_NOT_INIT;
	if (argc != 1)
		return ERR_BAD_ARGS;
	udatom = enif_make_atom(env, "undefined");
	if (enif_is_identical(argv[0], udatom)) {
		cairo_paint(ctx->cairo);
		return OP_OK;
	} else {
		if (!enif_get_double(env, argv[0], &alpha))
			return ERR_BAD_ARGS;
		cairo_paint_with_alpha(ctx->cairo, alpha);
		return OP_OK;
	}
}

static enum op_return
handle_op_stroke(ErlNifEnv *env, struct context *ctx, const ERL_NIF_TERM *argv, int argc)
{
	ERL_NIF_TERM head, tail, psatom;
	int preserve = 0;
	if (ctx->cairo == NULL)
		return ERR_NOT_INIT;
	if (argc != 1)
		return ERR_BAD_ARGS;
	psatom = enif_make_atom(env, "preserve");
	tail = argv[0];
	while (enif_get_list_cell(env, tail, &head, &tail)) {
		if (enif_is_identical(head, psatom)) {
			preserve = 1;
		}
	}
	if (preserve) {
		cairo_stroke_preserve(ctx->cairo);
	} else {
		cairo_stroke(ctx->cairo);
	}
	return OP_OK;
}

static enum op_return
handle_op_fill(ErlNifEnv *env, struct context *ctx, const ERL_NIF_TERM *argv, int argc)
{
	ERL_NIF_TERM head, tail, psatom;
	int preserve = 0;
	if (ctx->cairo == NULL)
		return ERR_NOT_INIT;
	if (argc != 1)
		return ERR_BAD_ARGS;
	psatom = enif_make_atom(env, "preserve");
	tail = argv[0];
	while (enif_get_list_cell(env, tail, &head, &tail)) {
		if (enif_is_identical(head, psatom)) {
			preserve = 1;
		}
	}
	if (preserve) {
		cairo_fill_preserve(ctx->cairo);
	} else {
		cairo_fill(ctx->cairo);
	}
	return OP_OK;
}

static enum op_return
handle_op_set_line_width(ErlNifEnv *env, struct context *ctx, const ERL_NIF_TERM *argv, int argc)
{
	double lw;
	if (ctx->cairo == NULL)
		return ERR_NOT_INIT;
	if (argc != 1)
		return ERR_BAD_ARGS;

	if (!get_tag_double(env, ctx, argv[0], &lw))
		return ERR_BAD_ARGS;

	cairo_set_line_width(ctx->cairo, lw);
	return OP_OK;
}

static enum op_return
handle_op_set_tag(ErlNifEnv *env, struct context *ctx, const ERL_NIF_TERM *argv, int argc)
{
	double val;
	if (ctx->cairo == NULL)
		return ERR_NOT_INIT;
	if (argc != 2)
		return ERR_BAD_ARGS;

	if (!get_tag_double(env, ctx, argv[1], &val))
		return ERR_BAD_ARGS;

	return set_tag_double(env, ctx, argv[0], val);
}

static enum op_return
handle_op_tag_deref(ErlNifEnv *env, struct context *ctx, const ERL_NIF_TERM *argv, int argc)
{
	struct tag_node node;
	struct tag_node *found;
	double val;

	if (ctx->cairo == NULL)
		return ERR_NOT_INIT;
	if (argc != 3)
		return ERR_BAD_ARGS;

	memset(&node, 0, sizeof(node));
	node.tag = argv[0];

	found = RB_FIND(tag_tree, &ctx->tag_head, &node);
	if (found == NULL)
		return ERR_TAG_NOT_SET;

	switch (found->type) {
		case TAG_TEXT_EXTENTS:
			if (enif_is_identical(argv[1], enif_make_atom(env, "x_bearing"))) {
				val = found->v_text_exts->x_bearing;
			} else if (enif_is_identical(argv[1], enif_make_atom(env, "y_bearing"))) {
				val = found->v_text_exts->y_bearing;
			} else if (enif_is_identical(argv[1], enif_make_atom(env, "width"))) {
				val = found->v_text_exts->width;
			} else if (enif_is_identical(argv[1], enif_make_atom(env, "height"))) {
				val = found->v_text_exts->height;
			} else if (enif_is_identical(argv[1], enif_make_atom(env, "x_advance"))) {
				val = found->v_text_exts->x_advance;
			} else if (enif_is_identical(argv[1], enif_make_atom(env, "y_advance"))) {
				val = found->v_text_exts->y_advance;
			} else {
				return ERR_BAD_ARGS;
			}
			return set_tag_double(env, ctx, argv[2], val);

		default:
			return ERR_BAD_ARGS;
	}
}

static enum op_return
handle_op_set_aa(ErlNifEnv *env, struct context *ctx, const ERL_NIF_TERM *argv, int argc)
{
	struct aa_mode { ERL_NIF_TERM atom; cairo_antialias_t mode; };
	int i;
	cairo_antialias_t *mode = NULL;
	struct aa_mode modes[] = {
		{enif_make_atom(env, "default"), CAIRO_ANTIALIAS_DEFAULT},
		{enif_make_atom(env, "gray"), CAIRO_ANTIALIAS_GRAY},
		{enif_make_atom(env, "fast"), CAIRO_ANTIALIAS_FAST},
		{enif_make_atom(env, "good"), CAIRO_ANTIALIAS_GOOD},
		{enif_make_atom(env, "best"), CAIRO_ANTIALIAS_BEST}
	};
	if (ctx->cairo == NULL)
		return ERR_NOT_INIT;
	if (argc != 1)
		return ERR_BAD_ARGS;

	for (i = 0; i < (sizeof(modes) / sizeof(*modes)); ++i) {
		if (enif_is_identical(argv[0], modes[i].atom)) {
			mode = &modes[i].mode;
		}
	}
	if (mode == NULL)
		return ERR_BAD_ARGS;

	cairo_set_antialias(ctx->cairo, *mode);

	return OP_OK;
}

static struct op_handler op_handlers[] = {
	/* path operations */
	{"cairo_arc", handle_op_arc},
	{"cairo_rectangle", handle_op_rectangle},
	{"cairo_new_sub_path", handle_op_new_sub_path},
	{"cairo_line_to", handle_op_line_to},
	{"cairo_move_to", handle_op_move_to},
	{"cairo_close_path", handle_op_close_path},

	/* rendering operations */
	{"cairo_set_line_width", handle_op_set_line_width},
	/*{"cairo_set_source", handle_op_set_source},*/
	{"cairo_set_source_rgba", handle_op_set_source_rgba},
	{"cairo_set_antialias", handle_op_set_aa},
	/*{"cairo_set_fill_rule", handle_op_set_fill_rule},*/
	{"cairo_clip", handle_op_clip},
	{"cairo_stroke", handle_op_stroke},
	{"cairo_fill", handle_op_fill},
	{"cairo_paint", handle_op_paint},

	/* pattern operations */
	/*{"cairo_pattern_create_linear", handle_op_pattern_create_linear},*/
	/*{"cairo_pattern_add_color_stop_rgba", handle_op_pattern_add_color_stop_rgba},*/
	/*{"cairo_pattern_create_for_surfce", handle_op_pattern_create_surface},*/
	/*{"cairo_pattern_translate", handle_op_pattern_translate},*/

	/* transform operations */
	{"cairo_identity_matrix", handle_op_identity_matrix},
	{"cairo_translate", handle_op_translate},
	/*{"cairo_scale", handle_op_scale},*/
	/*{"cairo_rotate", handle_op_rotate},*/

	/* text operations */
	/*{"cairo_text_extents", handle_op_text_extents},*/
	/*{"cairo_select_font_face", handle_op_select_font_face},*/
	/*{"cairo_set_font_size", handle_op_set_font_size},*/
	/*{"cairo_show_text", handle_op_show_text},*/

	/* tag ops */
	{"cairo_set_tag", handle_op_set_tag},
	{"cairo_tag_deref", handle_op_tag_deref}
};
const int n_handlers = sizeof(op_handlers) / sizeof(struct op_handler);

static enum op_return
handle_op(ErlNifEnv *env, struct context *ctx, ERL_NIF_TERM op)
{
	int arity = 16;
	int namesz = 32;
	const ERL_NIF_TERM *args;
	char namebuf[32];
	int i, idx = 0;
	struct op_handler *candidates[n_handlers];
	int ncand = n_handlers;

	if (!enif_get_tuple(env, op, &arity, &args))
		return ERR_NOT_TUPLE;
	if (!(namesz = enif_get_atom(env, args[0], namebuf, namesz, ERL_NIF_LATIN1)))
		return ERR_NOT_ATOM;

	for (i = 0; i < n_handlers; ++i)
		candidates[i] = &op_handlers[i];

	for (; idx < namesz; ++idx) {
		for (i = 0; i < n_handlers; ++i) {
			if (candidates[i] != NULL) {
				if (candidates[i]->name[idx] == 0 ||
						candidates[i]->name[idx] != namebuf[idx]) {
					candidates[i] = NULL;
					--ncand;
				} else if (ncand == 1) {
					return candidates[i]->handler(env, ctx, &args[1], arity - 1);
				} else if (ncand == 0) {
					break;
				}
			}
		}
	}
	return ERR_UNKNOWN_OP;
}

/* draw(Pixels :: binary(), InitTags :: tags(), Ops :: [cairerl:op()]) -> {ok, tags(), binary()} | {error, atom()} */
static ERL_NIF_TERM
draw(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[])
{
	ErlNifBinary pixels;
	struct context *ctx = NULL;
	struct tag_node *tn = NULL, *rn, *tn_next;
	ERL_NIF_TERM head, tail, out_tags, err = 0, ret;
	int arity, status, stride;
	const ERL_NIF_TERM *tuple;
	const ERL_NIF_TERM *img_tuple;
	ERL_NIF_TERM out_tuple[5];

	arity = 5;
	if (!enif_get_tuple(env, argv[0], &arity, &img_tuple)) {
		err = enif_make_atom(env, "bad_pixels");
		goto fail;
	}
	if (arity != 5 || !enif_is_identical(img_tuple[0], enif_make_atom(env, "cairo_image"))) {
		err = enif_make_atom(env, "bad_record");
		goto fail;
	}
	if (!enif_is_identical(img_tuple[3], enif_make_atom(env, "rgb24"))) {
		err = enif_make_atom(env, "bad_pixel_format");
		goto fail;
	}
	if (!enif_inspect_binary(env, img_tuple[4], &pixels)) {
		err = enif_make_atom(env, "bad_pixel_data");
		goto fail;
	}

	ctx = enif_alloc(sizeof(*ctx));
	assert(ctx != NULL);
	memset(ctx, 0, sizeof(*ctx));
	RB_INIT(&ctx->tag_head);

	/* get dimensions from the record */
	if (!enif_get_int(env, img_tuple[1], &ctx->w)) {
		err = enif_make_atom(env, "bad_width");
		goto fail;
	}
	if (!enif_get_int(env, img_tuple[2], &ctx->h)) {
		err = enif_make_atom(env, "bad_height");
		goto fail;
	}

	/* allocate and fill the bitmap and cairo context */
	assert(enif_alloc_binary(ctx->h * ctx->w * 4, &ctx->out));
	assert(ctx->out.data != NULL);
	if (pixels.size > 0)
		memcpy(ctx->out.data, pixels.data, pixels.size);

	stride = cairo_format_stride_for_width(CAIRO_FORMAT_RGB24, ctx->w);
	assert(stride == ctx->w * 4);
	ctx->sfc = cairo_image_surface_create_for_data(
			ctx->out.data, CAIRO_FORMAT_RGB24, ctx->w, ctx->h, ctx->w * 4);

	if ((status = cairo_surface_status(ctx->sfc)) != CAIRO_STATUS_SUCCESS) {
		err = enif_make_tuple2(env, enif_make_atom(env, "bad_surface_status"), enif_make_int(env, status));
		goto fail;
	}
	ctx->cairo = cairo_create(ctx->sfc);
	if ((status = cairo_status(ctx->cairo)) != CAIRO_STATUS_SUCCESS) {
		err = enif_make_tuple2(env, enif_make_atom(env, "bad_cairo_status"), enif_make_int(env, status));
		goto fail;
	}

	/* populate the initial tag tree */
	tail = argv[1];
	while (enif_get_list_cell(env, tail, &head, &tail)) {
		arity = 2;
		if (!enif_get_tuple(env, head, &arity, &tuple)) {
			err = enif_make_atom(env, "bad_init_args");
			goto fail;
		}
		if (arity != 2) {
			err = enif_make_atom(env, "bad_init_args");
			goto fail;
		}
		tn = enif_alloc(sizeof(*tn));
		memset(tn, 0, sizeof(*tn));
		tn->type = TAG_DOUBLE;
		tn->tag = tuple[0];
		if (!enif_get_double(env, tuple[1], &tn->v_dbl)) {
			enif_free(tn);
			err = enif_make_atom(env, "bad_init_tag_type");
			goto fail;
		}
		rn = RB_INSERT(tag_tree, &ctx->tag_head, tn);
		if (rn != NULL) {
			err = enif_make_atom(env, "duplicate_tag");
			enif_free(tn);
			goto fail;
		}
	}

	/* now run the ops */
	tail = argv[2];
	while (enif_get_list_cell(env, tail, &head, &tail)) {
		enum op_return ret;

		ret = handle_op(env, ctx, head);

		switch (ret) {
			case OP_OK:
				break;
			case ERR_NOT_TUPLE:
			case ERR_NOT_ATOM:
			case ERR_BAD_ARGS:
				err = enif_make_tuple2(env, enif_make_atom(env, "badarg"), head);
				goto fail;
			case ERR_UNKNOWN_OP:
				err = enif_make_tuple2(env, enif_make_atom(env, "unknown"), head);
				goto fail;
			case ERR_FAILURE:
				err = enif_make_tuple2(env, enif_make_atom(env, "cairo_error"), head);
				goto fail;
			case ERR_TAG_ALREADY:
				err = enif_make_tuple2(env, enif_make_atom(env, "tag_already_set"), head);
				goto fail;
			case ERR_TAG_NOT_SET:
				err = enif_make_tuple2(env, enif_make_atom(env, "tag_not_set"), head);
				goto fail;
			default:
				err = enif_make_tuple2(env, enif_make_atom(env, "error"), head);
				goto fail;
		}
	}

	/* we got through ok, construct our return values */
	out_tuple[0] = enif_make_atom(env, "cairo_image");
	out_tuple[1] = enif_make_int(env, ctx->w);
	out_tuple[2] = enif_make_int(env, ctx->h);
	out_tuple[3] = enif_make_atom(env, "rgb24");
	cairo_surface_finish(ctx->sfc);
	out_tuple[4] = enif_make_binary(env, &ctx->out);

	out_tags = enif_make_list(env, 0);
	for (tn = RB_MIN(tag_tree, &ctx->tag_head); tn != NULL; tn = tn_next) {
		ERL_NIF_TERM val;

		tn_next = RB_NEXT(tag_tree, &ctx->tag_head, tn);
		RB_REMOVE(tag_tree, &ctx->tag_head, tn);

		switch (tn->type) {
			case TAG_DOUBLE:
				val = enif_make_double(env, tn->v_dbl);
				break;
			case TAG_TEXT_EXTENTS:
				val = enif_make_tuple7(env,
					enif_make_atom(env, "cairo_tag_text_extents"),
					enif_make_double(env, tn->v_text_exts->x_bearing),
					enif_make_double(env, tn->v_text_exts->y_bearing),
					enif_make_double(env, tn->v_text_exts->width),
					enif_make_double(env, tn->v_text_exts->height),
					enif_make_double(env, tn->v_text_exts->x_advance),
					enif_make_double(env, tn->v_text_exts->y_advance));
				enif_free(tn->v_text_exts);
				break;
			case TAG_PATTERN:
				switch (cairo_pattern_get_type(tn->v_pattern)) {
					case CAIRO_PATTERN_TYPE_SOLID:
						val = enif_make_tuple2(env,
							enif_make_atom(env, "cairo_tag_pattern"),
							enif_make_atom(env, "solid"));
						break;
					case CAIRO_PATTERN_TYPE_SURFACE:
						val = enif_make_tuple2(env,
							enif_make_atom(env, "cairo_tag_pattern"),
							enif_make_atom(env, "surface"));
						break;
					case CAIRO_PATTERN_TYPE_LINEAR:
						val = enif_make_tuple2(env,
							enif_make_atom(env, "cairo_tag_pattern"),
							enif_make_atom(env, "linear"));
						break;
					default:
						err = enif_make_atom(env, "unhandled_tag_pattern_type");
						goto fail;
				}
				cairo_pattern_destroy(tn->v_pattern);
				break;
			case TAG_PATH:
				val = enif_make_tuple2(env,
					enif_make_atom(env, "cairo_tag_path"),
					enif_make_int(env, tn->v_path->num_data));
				cairo_path_destroy(tn->v_path);
				break;
			default:
				err = enif_make_tuple2(env,
					enif_make_atom(env, "unknown_tag_type"),
					enif_make_int(env, tn->type));
				goto fail;
		}

		out_tags = enif_make_list_cell(env,
			enif_make_tuple2(env, tn->tag, val), out_tags);

		enif_free(tn);
	}

	ret = enif_make_tuple3(env,
		enif_make_atom(env, "ok"),
		out_tags,
		enif_make_tuple_from_array(env, out_tuple, 5));
	goto free_and_exit;

fail:
	ret = enif_make_tuple2(env, enif_make_atom(env, "error"), err);

free_and_exit:
	if (ctx != NULL) {
		for (tn = RB_MIN(tag_tree, &ctx->tag_head); tn != NULL; tn = tn_next) {
			tn_next = RB_NEXT(tag_tree, &ctx->tag_head, tn);
			RB_REMOVE(tag_tree, &ctx->tag_head, tn);

			switch (tn->type) {
				case TAG_TEXT_EXTENTS:
					enif_free(tn->v_text_exts);
					break;
				case TAG_PATTERN:
					cairo_pattern_destroy(tn->v_pattern);
					break;
				case TAG_PATH:
					cairo_path_destroy(tn->v_path);
					break;
				default:
					/* nothing to free */
					break;
			}

			enif_free(tn);
		}

		if (ctx->cairo != NULL)
			cairo_destroy(ctx->cairo);
		if (ctx->sfc != NULL)
			cairo_surface_destroy(ctx->sfc);
		if (err != 0 && ctx->out.data != NULL)
			enif_release_binary(&ctx->out);
		enif_free(ctx);
	}
	return ret;
}

static ERL_NIF_TERM
png_read(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[])
{
	char fnamebuf[256];
	cairo_surface_t *sfc = NULL;
	cairo_format_t fmt;
	ErlNifBinary fname, img;
	cairo_status_t status;
	ERL_NIF_TERM err;
	int w, h, stride;
	ERL_NIF_TERM out_tuple[5];

	memset(&fname, 0, sizeof(fname));
	memset(&img, 0, sizeof(img));

	/* get the filename to read from */
	if (!enif_inspect_binary(env, argv[0], &fname)) {
		if (!enif_inspect_iolist_as_binary(env, argv[0], &fname)) {
			err = enif_make_atom(env, "bad_filename");
			goto fail;
		}
	}
	assert(fname.size < 255);
	memcpy(fnamebuf, fname.data, fname.size);
	fnamebuf[fname.size] = 0;

	sfc = cairo_image_surface_create_from_png(fnamebuf);
	if ((status = cairo_surface_status(sfc)) != CAIRO_STATUS_SUCCESS) {
		err = enif_make_tuple2(env, enif_make_atom(env, "bad_surface_status"), enif_make_int(env, status));
		goto fail;
	}

	fmt = cairo_image_surface_get_format(sfc);
	w = cairo_image_surface_get_width(sfc);
	h = cairo_image_surface_get_height(sfc);
	stride = cairo_image_surface_get_stride(sfc);
	assert(stride == cairo_format_stride_for_width(fmt, w));

	assert(enif_alloc_binary(h*stride, &img));
	memcpy(img.data, cairo_image_surface_get_data(sfc), h*stride);

	out_tuple[0] = enif_make_atom(env, "cairo_image");
	out_tuple[1] = enif_make_int(env, w);
	out_tuple[2] = enif_make_int(env, h);
	switch (fmt) {
		case CAIRO_FORMAT_RGB24:
			out_tuple[3] = enif_make_atom(env, "rgb24");
			break;
		case CAIRO_FORMAT_ARGB32:
			out_tuple[3] = enif_make_atom(env, "argb32");
			break;
		case CAIRO_FORMAT_RGB30:
			out_tuple[3] = enif_make_atom(env, "rgb30");
			break;
		case CAIRO_FORMAT_RGB16_565:
			out_tuple[3] = enif_make_atom(env, "rgb16_565");
			break;
		default:
			err = enif_make_atom(env, "invalid_format");
			goto fail;
	}
	out_tuple[4] = enif_make_binary(env, &img);
	img.data = NULL;

	cairo_surface_destroy(sfc);

	return enif_make_tuple2(env,
		enif_make_atom(env, "ok"),
		enif_make_tuple_from_array(env, out_tuple, 5));

fail:
	if (sfc != NULL)
		cairo_surface_destroy(sfc);
	if (img.data != NULL)
		enif_release_binary(&img);
	return enif_make_tuple2(env, enif_make_atom(env, "error"), err);
}

static ERL_NIF_TERM
png_write(ErlNifEnv *env, int argc, const ERL_NIF_TERM argv[])
{
	const ERL_NIF_TERM *img_tuple;
	cairo_surface_t *sfc = NULL;
	cairo_format_t fmt;
	ErlNifBinary pixels, fname;
	char fnamebuf[256];
	cairo_status_t status;
	int arity;
	ERL_NIF_TERM err;
	int w, h, stride;

	arity = 5;
	if (!enif_get_tuple(env, argv[0], &arity, &img_tuple)) {
		err = enif_make_atom(env, "bad_pixels");
		goto fail;
	}

	if (arity != 5 || !enif_is_identical(img_tuple[0], enif_make_atom(env, "cairo_image"))) {
		err = enif_make_atom(env, "bad_record");
		goto fail;
	}
	if (!enif_inspect_binary(env, img_tuple[4], &pixels)) {
		err = enif_make_atom(env, "bad_pixel_data");
		goto fail;
	}

	/* get dimensions from the record */
	if (!enif_get_int(env, img_tuple[1], &w)) {
		err = enif_make_atom(env, "bad_width");
		goto fail;
	}
	if (!enif_get_int(env, img_tuple[2], &h)) {
		err = enif_make_atom(env, "bad_height");
		goto fail;
	}

	if (enif_is_identical(img_tuple[3], enif_make_atom(env, "rgb24"))) {
		fmt = CAIRO_FORMAT_RGB24;
	} else if (enif_is_identical(img_tuple[3], enif_make_atom(env, "argb32"))) {
		fmt = CAIRO_FORMAT_ARGB32;
	} else if (enif_is_identical(img_tuple[3], enif_make_atom(env, "rgb30"))) {
		fmt = CAIRO_FORMAT_RGB30;
	} else if (enif_is_identical(img_tuple[3], enif_make_atom(env, "rgb16_565"))) {
		fmt = CAIRO_FORMAT_RGB16_565;
	} else {
		err = enif_make_atom(env, "bad_pixel_format");
		goto fail;
	}

	/* get the filename to write to */
	if (!enif_inspect_binary(env, argv[1], &fname)) {
		if (!enif_inspect_iolist_as_binary(env, argv[1], &fname)) {
			err = enif_make_atom(env, "bad_filename");
			goto fail;
		}
	}
	assert(fname.size < 255);
	memcpy(fnamebuf, fname.data, fname.size);
	fnamebuf[fname.size] = 0;

	/* allocate and fill the bitmap and cairo context */
	stride = cairo_format_stride_for_width(fmt, w);
	sfc = cairo_image_surface_create_for_data(
			pixels.data, fmt, w, h, stride);

	if ((status = cairo_surface_status(sfc)) != CAIRO_STATUS_SUCCESS) {
		err = enif_make_tuple2(env, enif_make_atom(env, "bad_surface_status"), enif_make_int(env, status));
		goto fail;
	}

	if ((status = cairo_surface_write_to_png(sfc, fnamebuf)) != CAIRO_STATUS_SUCCESS) {
		err = enif_make_tuple2(env, enif_make_atom(env, "bad_write_status"), enif_make_int(env, status));
		goto fail;
	}

	cairo_surface_destroy(sfc);

	return enif_make_atom(env, "ok");

fail:
	if (sfc != NULL)
		cairo_surface_destroy(sfc);
	return enif_make_tuple2(env, enif_make_atom(env, "error"), err);
}

static int
load_cb(ErlNifEnv *env, void **priv_data, ERL_NIF_TERM load_info)
{
	return 0;
}

static void
unload_cb(ErlNifEnv *env, void *priv_data)
{
}

static ErlNifFunc nif_funcs[] =
{
	{"draw", 3, draw},
	{"png_read", 1, png_read},
	{"png_write", 2, png_write}
};

ERL_NIF_INIT(cairerl_nif, nif_funcs, load_cb, NULL, NULL, unload_cb)
