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

-record(cairo_image, {width :: integer(), height :: integer(), data :: binary()}).

% tags (state data)
-record(cairo_set_tag, {tag :: atom(), value :: cairerl:value()}).
-record(cairo_tag_deref, {tag :: atom(), field :: atom(), out_tag :: atom()}).

% path operations
-record(cairo_arc, {xc :: cairerl:value(), yc :: cairerl:value(), radius :: cairerl:value(), angle1 :: cairerl:value(), angle2 :: cairerl:value()}).
-record(cairo_rectangle, {x :: cairerl:value(), y :: cairerl:value(), width :: cairerl:value(), height :: cairerl:value()}).
-record(cairo_new_sub_path, {}).
-record(cairo_curve_to, {x :: cairerl:value(), y :: cairerl:value(),
				   x2 :: cairerl:value(), y2 :: cairerl:value(),
				   x3 :: cairerl:value(), y3 :: cairerl:value(), flags = [] :: [relative]}).
-record(cairo_line_to, {x :: cairerl:value(), y :: cairerl:value(), flags = [] :: [relative]}).
-record(cairo_move_to, {x :: cairerl:value(), y :: cairerl:value(), flags = [] :: [relative]}).
-record(cairo_close_path, {}).

% rendering operations
-record(cairo_set_line_width, {width = 1.0 :: float()}).
-record(cairo_set_source, {tag :: atom()}).
-record(cairo_set_source_rgba, {r :: float(), g :: float(), b :: float(), a = 1.0 :: float()}).
-record(cairo_set_antialias, {mode = default :: cairerl:antialias_mode()}).
-record(cairo_set_fill_rule, {fill_rule = winding :: winding | even_odd}).
-record(cairo_clip, {flags = [] :: [preserve]}).
-record(cairo_stroke, {flags = [] :: [preserve]}).
-record(cairo_fill, {flags = [] :: [preserve]}).
-record(cairo_paint, {alpha :: undefined | float()}).

% pattern operations
-record(cairo_pattern_create_linear, {tag :: atom(), x :: cairerl:value(), y :: cairerl:value(), x2 :: cairerl:value(), y2 :: cairerl:value()}).
-record(cairo_pattern_add_color_stop_rgba, {tag :: atom(), offset :: float(), r :: float(), g :: float(), b :: float()}).

% transform operations
-record(cairo_identity_matrix, {}).
-record(cairo_translate, {x :: cairerl:value(), y :: cairerl:value()}).
-record(cairo_scale, {x :: cairerl:value(), y :: cairerl:value()}).
-record(cairo_rotate, {angle :: cairerl:value()}).

% text operations
-record(cairo_text_extents, {text :: binary(), tag :: atom()}).
-record(cairo_select_font_face, {family :: binary(), slant = normal :: normal | italic | oblique, weight = normal :: normal | bold}).
-record(cairo_set_font_size, {size :: float()}).
-record(cairo_show_text, {text :: binary()}).
