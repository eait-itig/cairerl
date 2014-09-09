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

int
tag_cmp(struct tag_node *n1, struct tag_node *n2)
{
	return enif_compare(n1->tag, n2->tag);
}

RB_GENERATE(tag_tree, tag_node, entry, tag_cmp);

int
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

void *
get_tag_ptr(ErlNifEnv *env, struct context *ctx, enum tag_type type, const ERL_NIF_TERM tag)
{
	struct tag_node node;
	struct tag_node *found;

	memset(&node, 0, sizeof(node));
	node.tag = tag;

	found = RB_FIND(tag_tree, &ctx->tag_head, &node);
	if (found == NULL)
		return NULL;

	if (found->type != type)
		return NULL;

	return found->v_ptr;
}

enum op_return
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

enum op_return
set_tag_ptr(ErlNifEnv *env, struct context *ctx, const ERL_NIF_TERM tag, enum tag_type type, void *value)
{
	struct tag_node *tn, *rn;

	tn = enif_alloc(sizeof(*tn));
	assert(tn != NULL);
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

int
create_surface_from_image(ErlNifEnv *env, const ERL_NIF_TERM image, cairo_surface_t **sfc, ERL_NIF_TERM *err)
{
	ErlNifBinary pixels;
	cairo_status_t status;
	cairo_format_t fmt;
	int arity;
	const ERL_NIF_TERM *img_tuple;
	int w, h, stride;

	arity = 5;
	if (!enif_get_tuple(env, image, &arity, &img_tuple)) {
		if (err != NULL)
			*err = enif_make_atom(env, "bad_pixels");
		goto fail;
	}

	if (arity != 5 || !enif_is_identical(img_tuple[0], enif_make_atom(env, "cairo_image"))) {
		if (err != NULL)
			*err = enif_make_atom(env, "bad_record");
		goto fail;
	}
	if (!enif_inspect_binary(env, img_tuple[4], &pixels)) {
		if (err != NULL)
			*err = enif_make_atom(env, "bad_pixel_data");
		goto fail;
	}

	/* get dimensions from the record */
	if (!enif_get_int(env, img_tuple[1], &w)) {
		if (err != NULL)
			*err = enif_make_atom(env, "bad_width");
		goto fail;
	}
	if (!enif_get_int(env, img_tuple[2], &h)) {
		if (err != NULL)
			*err = enif_make_atom(env, "bad_height");
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
		if (err != NULL)
			*err = enif_make_atom(env, "bad_pixel_format");
		goto fail;
	}

	stride = cairo_format_stride_for_width(fmt, w);
	*sfc = cairo_image_surface_create_for_data(
			pixels.data, fmt, w, h, stride);

	if ((status = cairo_surface_status(*sfc)) != CAIRO_STATUS_SUCCESS) {
		if (err != NULL)
			*err = enif_make_tuple2(env,
				enif_make_atom(env, "bad_surface_status"), enif_make_int(env, status));
		goto fail;
	}

	return 1;
fail:
	if (*sfc != NULL) {
		cairo_surface_destroy(*sfc);
		*sfc = NULL;
	}
	return 0;
}
