#include "widget.h"

UIRect ui_resolve_widget_rect(UIWidget widget, UIRect parent_rect)
{
	// Compute the anchor position in pixels
	F4 anchor_left = parent_rect.left + widget.anchor_min.x * parent_rect.width;
	F4 anchor_top = parent_rect.top + widget.anchor_min.y * parent_rect.height;
	F4 anchor_right = parent_rect.left + widget.anchor_max.x * parent_rect.width;
	F4 anchor_bottom = parent_rect.top + widget.anchor_max.y * parent_rect.height;

	// Apply the offsets to the anchor position
	F4 pixel_left = anchor_left + widget.offset_left;
	F4 pixel_top = anchor_top + widget.offset_top;
	F4 pixel_right = anchor_right + widget.offset_right;
	F4 pixel_bottom = anchor_bottom + widget.offset_bottom;

	UIRect result = {
		.top = pixel_top,
		.left = pixel_left,
		.width = pixel_right - pixel_left,
		.height = pixel_bottom - pixel_top,
	};
	return result;
}
UIRect ui_resolve_grid_cell(U4 row, U4 column, U4 total_rows, U4 total_columns, F4 gap, UIRect parent_rect)
{
	// Total gap space reduces the available area for cells
	F4 cell_width = (parent_rect.width - gap * (F4)(total_columns - 1)) / (F4)total_columns;
	F4 cell_height = (parent_rect.height - gap * (F4)(total_rows - 1)) / (F4)total_rows;

	// Each cell is offset by its index times (cell size + one gap)
	F4 cell_left = parent_rect.left + (F4)column * (cell_width + gap);
	F4 cell_top = parent_rect.top + (F4)row * (cell_height + gap);

	UIRect result = {
		.left = cell_left,
		.top = cell_top,
		.width = cell_width,
		.height = cell_height
	};
	return result;
}

