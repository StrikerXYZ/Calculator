#pragma once

#include "types.h"

typedef struct
{
	F4 left;
	F4 top;
	F4 width;
	F4 height;
} UIRect; //pixels

typedef struct
{
	F4 x;
	F4 y;
} UIAnchor; //0-1

typedef struct
{
	UIAnchor anchor_min;
	UIAnchor anchor_max;

	//offset from the anchor position in pixels
	F4 offset_left;
	F4 offset_top;
	F4 offset_right;
	F4 offset_bottom;
} UIWidget;

typedef struct
{
	F4 x;
	F4 y;
	F4 font_size;
} UIText; //0-1

UIRect ui_resolve_widget_rect(UIWidget widget, UIRect parent_rect);
UIRect ui_resolve_grid_cell(U4 row, U4 column, U4 total_rows, U4 total_columns, F4 gap, UIRect parent_rect);
