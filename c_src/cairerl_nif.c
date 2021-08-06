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
handle_op(ErlNifEnv *env, struct context *ctx, ERL_NIF_TERM op)
{
	int arity = 16;
	int namesz = 64;
	const ERL_NIF_TERM *args;
	char namebuf[64];
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
				if ((namebuf[idx] != 0 && candidates[i]->name[idx] == 0) ||
						candidates[i]->name[idx] != namebuf[idx]) {
					candidates[i] = NULL;
					--ncand;
				} else if ((namebuf[idx] == 0 && candidates[i]->name[idx] == 0) || ncand == 1) {
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
	cairo_format_t fmt;
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
	if (enif_is_identical(img_tuple[3], enif_make_atom(env, "rgb24"))) {
		fmt = CAIRO_FORMAT_RGB24;
	} else if (enif_is_identical(img_tuple[3], enif_make_atom(env, "rgb16_565"))) {
		fmt = CAIRO_FORMAT_RGB16_565;
	} else if (enif_is_identical(img_tuple[3], enif_make_atom(env, "argb32"))) {
		fmt = CAIRO_FORMAT_ARGB32;
	} else {
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
	if (ctx->w < 0 || ctx->h < 0) {
		err = enif_make_atom(env, "negative_dimensions");
		goto fail;
	}
	if (ctx->w > 32768 || ctx->h > 32768) {
		err = enif_make_atom(env, "dimensions_too_big");
		goto fail;
	}

	/* allocate and fill the bitmap and cairo context */
	stride = cairo_format_stride_for_width(fmt, ctx->w);
	assert(enif_alloc_binary(ctx->h * stride, &ctx->out));
	assert(ctx->out.data != NULL);
	if (pixels.size > 0)
		memcpy(ctx->out.data, pixels.data, pixels.size);

	ctx->sfc = cairo_image_surface_create_for_data(
			ctx->out.data, fmt, ctx->w, ctx->h, stride);

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
				if ((status = cairo_status(ctx->cairo)) != CAIRO_STATUS_SUCCESS) {
					err = enif_make_tuple3(env,
						enif_make_atom(env, "cairo_error"),
						enif_make_string(env, cairo_status_to_string(status), ERL_NIF_LATIN1),
						head);
					goto fail;
				}
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
				status = cairo_status(ctx->cairo);
				err = enif_make_tuple3(env,
					enif_make_atom(env, "cairo_error"),
					enif_make_string(env, cairo_status_to_string(status), ERL_NIF_LATIN1),
					head);
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
	out_tuple[3] = img_tuple[3];
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
			case TAG_FONT_EXTENTS:
				val = enif_make_tuple6(env,
					enif_make_atom(env, "cairo_tag_font_extents"),
					enif_make_double(env, tn->v_font_exts->ascent),
					enif_make_double(env, tn->v_font_exts->descent),
					enif_make_double(env, tn->v_font_exts->height),
					enif_make_double(env, tn->v_font_exts->max_x_advance),
					enif_make_double(env, tn->v_font_exts->max_y_advance));
				enif_free(tn->v_font_exts);
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
				case TAG_FONT_EXTENTS:
					enif_free(tn->v_font_exts);
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
	ErlNifBinary fname;
	char fnamebuf[256];
	cairo_status_t status;
	cairo_surface_t *sfc = NULL;
	ERL_NIF_TERM err;

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

	if (!create_surface_from_image(env, argv[0], &sfc, &err))
		goto fail;

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
