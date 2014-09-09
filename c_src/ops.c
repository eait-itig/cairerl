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

#include "common.h"

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
handle_op_scale(ErlNifEnv *env, struct context *ctx, const ERL_NIF_TERM *argv, int argc)
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

	cairo_scale(ctx->cairo, x, y);

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
handle_op_pattern_create_for_surface(ErlNifEnv *env, struct context *ctx, const ERL_NIF_TERM *argv, int argc)
{
	cairo_surface_t *sfc = NULL;
	cairo_pattern_t *ptn = NULL;

	if (ctx->cairo == NULL)
		return ERR_NOT_INIT;
	if (argc != 2)
		return ERR_BAD_ARGS;

	if (!create_surface_from_image(env, argv[1], &sfc, NULL))
		return ERR_FAILURE;

	ptn = cairo_pattern_create_for_surface(sfc);
	if (cairo_pattern_status(ptn) != CAIRO_STATUS_SUCCESS) {
		cairo_surface_destroy(sfc);
		return ERR_FAILURE;
	}

	cairo_surface_destroy(sfc);

	return set_tag_ptr(env, ctx, argv[0], TAG_PATTERN, ptn);
}

static enum op_return
handle_op_text_extents(ErlNifEnv *env, struct context *ctx, const ERL_NIF_TERM *argv, int argc)
{
	cairo_text_extents_t *exts = NULL;
	ErlNifBinary textbin;

	if (ctx->cairo == NULL)
		return ERR_NOT_INIT;
	if (argc != 2)
		return ERR_BAD_ARGS;

	memset(&textbin, 0, sizeof(textbin));
	if (!enif_inspect_binary(env, argv[1], &textbin)) {
		if (!enif_inspect_iolist_as_binary(env, argv[1], &textbin)) {
			return ERR_BAD_ARGS;
		}
	}
	if (textbin.data[textbin.size-1] != 0)
		return ERR_BAD_ARGS;

	exts = enif_alloc(sizeof(*exts));
	assert(exts != NULL);
	memset(exts, 0, sizeof(*exts));

	cairo_text_extents(ctx->cairo, (const char *)textbin.data, exts);

	return set_tag_ptr(env, ctx, argv[0], TAG_TEXT_EXTENTS, exts);
}

static enum op_return
handle_op_show_text(ErlNifEnv *env, struct context *ctx, const ERL_NIF_TERM *argv, int argc)
{
	ErlNifBinary textbin;

	if (ctx->cairo == NULL)
		return ERR_NOT_INIT;
	if (argc != 1)
		return ERR_BAD_ARGS;

	memset(&textbin, 0, sizeof(textbin));
	if (!enif_inspect_binary(env, argv[0], &textbin)) {
		if (!enif_inspect_iolist_as_binary(env, argv[0], &textbin)) {
			return ERR_BAD_ARGS;
		}
	}
	if (textbin.data[textbin.size-1] != 0)
		return ERR_BAD_ARGS;

	cairo_show_text(ctx->cairo, (const char *)textbin.data);

	return OP_OK;
}

static enum op_return
handle_op_set_source(ErlNifEnv *env, struct context *ctx, const ERL_NIF_TERM *argv, int argc)
{
	cairo_pattern_t *ptn = NULL;

	if (ctx->cairo == NULL)
		return ERR_NOT_INIT;
	if (argc != 1)
		return ERR_BAD_ARGS;

	ptn = (cairo_pattern_t *)get_tag_ptr(env, ctx, TAG_PATTERN, argv[0]);
	if (ptn == NULL)
		return ERR_BAD_ARGS;

	cairo_set_source(ctx->cairo, ptn);

	return OP_OK;
}

static enum op_return
handle_op_pattern_translate(ErlNifEnv *env, struct context *ctx, const ERL_NIF_TERM *argv, int argc)
{
	cairo_pattern_t *ptn = NULL;
	double x, y;
	cairo_matrix_t m;

	if (ctx->cairo == NULL)
		return ERR_NOT_INIT;
	if (argc != 3)
		return ERR_BAD_ARGS;

	if (!get_tag_double(env, ctx, argv[1], &x))
		return ERR_BAD_ARGS;
	if (!get_tag_double(env, ctx, argv[2], &y))
		return ERR_BAD_ARGS;

	ptn = (cairo_pattern_t *)get_tag_ptr(env, ctx, TAG_PATTERN, argv[0]);
	if (ptn == NULL)
		return ERR_BAD_ARGS;

	cairo_pattern_get_matrix(ptn, &m);
	cairo_matrix_translate(&m, x, y);
	cairo_pattern_set_matrix(ptn, &m);

	return OP_OK;
}

static enum op_return
handle_op_set_font_size(ErlNifEnv *env, struct context *ctx, const ERL_NIF_TERM *argv, int argc)
{
	double size;

	if (ctx->cairo == NULL)
		return ERR_NOT_INIT;
	if (argc != 1)
		return ERR_BAD_ARGS;

	if (!get_tag_double(env, ctx, argv[0], &size))
		return ERR_BAD_ARGS;

	cairo_set_font_size(ctx->cairo, size);
	return OP_OK;
}

static enum op_return
handle_op_select_font_face(ErlNifEnv *env, struct context *ctx, const ERL_NIF_TERM *argv, int argc)
{
	char facebuf[256];
	ErlNifBinary facebin;
	cairo_font_slant_t slant;
	cairo_font_weight_t weight;

	if (ctx->cairo == NULL)
		return ERR_NOT_INIT;
	if (argc != 3)
		return ERR_BAD_ARGS;

	memset(&facebin, 0, sizeof(facebin));
	memset(facebuf, 0, sizeof(facebuf));

	/* get the font family name */
	if (!enif_inspect_binary(env, argv[0], &facebin)) {
		if (!enif_inspect_iolist_as_binary(env, argv[0], &facebin)) {
			return ERR_BAD_ARGS;
		}
	}
	assert(facebin.size < sizeof(facebuf) - 1);
	memcpy(facebuf, facebin.data, facebin.size);
	facebuf[facebin.size] = 0;

	if (enif_is_identical(argv[1], enif_make_atom(env, "normal"))) {
		slant = CAIRO_FONT_SLANT_NORMAL;
	} else if (enif_is_identical(argv[1], enif_make_atom(env, "italic"))) {
		slant = CAIRO_FONT_SLANT_ITALIC;
	} else if (enif_is_identical(argv[1], enif_make_atom(env, "oblique"))) {
		slant = CAIRO_FONT_SLANT_OBLIQUE;
	} else {
		return ERR_BAD_ARGS;
	}

	if (enif_is_identical(argv[2], enif_make_atom(env, "normal"))) {
		weight = CAIRO_FONT_WEIGHT_NORMAL;
	} else if (enif_is_identical(argv[2], enif_make_atom(env, "bold"))) {
		weight = CAIRO_FONT_WEIGHT_BOLD;
	} else {
		return ERR_BAD_ARGS;
	}

	cairo_select_font_face(ctx->cairo, facebuf, slant, weight);

	return OP_OK;
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

struct op_handler op_handlers[] = {
	/* path operations */
	{"cairo_arc", handle_op_arc},
	{"cairo_rectangle", handle_op_rectangle},
	{"cairo_new_sub_path", handle_op_new_sub_path},
	{"cairo_line_to", handle_op_line_to},
	{"cairo_move_to", handle_op_move_to},
	{"cairo_close_path", handle_op_close_path},

	/* rendering operations */
	{"cairo_set_line_width", handle_op_set_line_width},
	{"cairo_set_source", handle_op_set_source},
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
	{"cairo_pattern_create_for_surface", handle_op_pattern_create_for_surface},
	{"cairo_pattern_translate", handle_op_pattern_translate},

	/* transform operations */
	{"cairo_identity_matrix", handle_op_identity_matrix},
	{"cairo_translate", handle_op_translate},
	{"cairo_scale", handle_op_scale},
	/*{"cairo_rotate", handle_op_rotate},*/

	/* text operations */
	{"cairo_text_extents", handle_op_text_extents},
	{"cairo_select_font_face", handle_op_select_font_face},
	{"cairo_set_font_size", handle_op_set_font_size},
	{"cairo_show_text", handle_op_show_text},

	/* tag ops */
	{"cairo_set_tag", handle_op_set_tag},
	{"cairo_tag_deref", handle_op_tag_deref}
};
const int n_handlers = sizeof(op_handlers) / sizeof(struct op_handler);

