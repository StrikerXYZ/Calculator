#pragma once

#include "types.h"
#include "memory.h"
#include "string.h"

typedef void* RendererContext;

U1 renderer_initialize(RendererContext* renderer_context, Arena* arena, Handle window_handle, Handle application_handle);

void renderer_draw_frame(RendererContext renderer_context, Arena* arena, U1 window_resized);

void renderer_cleanup(RendererContext renderer_context);

void renderer_begin_frame(RendererContext renderer_context);
void renderer_push_quad(RendererContext renderer_context, F4 x, F4 y, F4 w, F4 h, F4 r, F4 g, F4 b);
void renderer_push_text(RendererContext renderer_context, StringView string_view, F4 x, F4 y, F4 font_size, F4 r, F4 g, F4 b);
F4 renderer_measure_text(RendererContext renderer_context, StringView string_view);
F4 renderer_get_font_size(RendererContext renderer_context);
