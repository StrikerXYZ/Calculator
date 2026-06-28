#include "presentation.h"
#include "../core/widget.h"
#include "../core/string.h"
#include "../core/console.h"
#include "../core/debug.h"

// Row-major order: read left-to-right, top-to-bottom
global const Char* calculator_labels[NUMPAD_COUNT] = {
	"+",		"-",		"*",		"/",		"mod",
	"sin",		"cos",		"tan",		"ln",		"x^y",
	"asin",		"acos",		"atan",		"e^x",		"Pi",
	"x^2",		"sqrt",		"fac(x)",	"log",		"e",
	"C",		"+/-",		"1/x",		"ans",		"="
};

typedef enum
{
	CATEGORY_BINARY,
	CATEGORY_FUNCTION,
	CATEGORY_CONSTANT,
	CATEGORY_CONTROL,
} ButtonCategory;

global ButtonCategory calculator_categories[NUMPAD_COUNT] = {
	CATEGORY_BINARY,	CATEGORY_BINARY,	CATEGORY_BINARY,	CATEGORY_BINARY,	CATEGORY_BINARY,
	CATEGORY_FUNCTION,	CATEGORY_FUNCTION,	CATEGORY_FUNCTION,	CATEGORY_FUNCTION,	CATEGORY_BINARY,
	CATEGORY_FUNCTION,	CATEGORY_FUNCTION,	CATEGORY_FUNCTION,	CATEGORY_FUNCTION,	CATEGORY_CONSTANT,
	CATEGORY_FUNCTION,	CATEGORY_FUNCTION,	CATEGORY_FUNCTION,	CATEGORY_FUNCTION,	CATEGORY_CONSTANT,
	CATEGORY_CONTROL,	CATEGORY_FUNCTION,	CATEGORY_FUNCTION,	CATEGORY_CONTROL,	CATEGORY_CONTROL,
};

void presentation_initialize(PresentationState* presentation_state)
{
	memory_zero(presentation_state, sizeof(PresentationState));
}

SimulationInput presentation_update(
	PresentationState* presentation_state,
	SimulationState const * simulation_state,
	InputState const * input_state,
	RendererContext renderer_context,
	U4 surface_width, 
	U4 surface_height
)
{
	SimulationInput simulation_input = { 0 };

	for (U4 i = 0; i < input_state->char_count && i < 32; ++i)
	{
		simulation_input.chars[i] = input_state->chars[i];
	}

	simulation_input.char_count = input_state->char_count;
	simulation_input.key_backspace = input_state->backspace;
	simulation_input.key_enter = input_state->enter;
	simulation_input.key_escape = input_state->escape;

	if (simulation_state->simulation_tick != presentation_state->last_simulation_tick)
	{
		presentation_state->display_flash_timer = 1.0f;
		presentation_state->last_simulation_tick = simulation_state->simulation_tick;
	}

	UIRect screen = {
		.left = 0.f,
		.top = 0.f,
		.width = (F4)surface_width,
		.height = (F4)surface_height
	};

	//background
	UIWidget background = {
		.anchor_min = { 0.f, 0.f },
		.anchor_max = { 1.f, 1.f },
		.offset_left = 0.f,
		.offset_top = 0.f,
		.offset_right = -0.f,
		.offset_bottom = -0.f
	};
	UIRect background_rect = ui_resolve_widget_rect(background, screen);
	renderer_push_quad(renderer_context, background_rect.left, background_rect.top, background_rect.width, background_rect.height, 0.12f, 0.12f, 0.12f);

	// Display panel
	UIWidget display_panel = {
		.anchor_min = { 0.f, 0.f },
		.anchor_max = { 1.f, 0.f },
		.offset_left = 20.f,
		.offset_top = 20.f,
		.offset_right = -20.f,
		.offset_bottom = 100.f
	};
	UIRect display_panel_rect = ui_resolve_widget_rect(display_panel, background_rect);
	renderer_push_quad(renderer_context, display_panel_rect.left, display_panel_rect.top, display_panel_rect.width, display_panel_rect.height, 0.5f, 0.5f, 0.5f);

	{
		// fill widget with text
		StringView display_text = (StringView){
			.data = simulation_state->display,
			.size = simulation_state->display_size
		};

		F4 font_size = renderer_get_font_size(renderer_context);
		F4 text_width = renderer_measure_text(renderer_context, display_text);
		F4 text_padding = 12.f;
		F4 text_x = display_panel_rect.left + display_panel_rect.width - text_width - text_padding;
		F4 text_y = display_panel_rect.top + (display_panel_rect.height - font_size) * 0.5f;
		renderer_push_text(
			renderer_context,
			display_text,
			text_x,
			text_y,
			font_size,
			0.f, 0.f, 0.f
		);
	}

	UIWidget button_panel = {
		.anchor_min = { 0.f, 0.f },
		.anchor_max = { 1.f, 1.f },
		.offset_left = 20.f,
		.offset_top = 120.f,
		.offset_right = -20.f,
		.offset_bottom = -20.f
	};
	UIRect button_panel_rect = ui_resolve_widget_rect(button_panel, background_rect);
	renderer_push_quad(renderer_context, button_panel_rect.left, button_panel_rect.top, button_panel_rect.width, button_panel_rect.height, 0.12f, 0.12f, 0.12f);

	F4 color_blue_gray[] = { 0.25f, 0.30f, 0.40f };	//operators
	F4 color_dark_gray[] = { 0.20f, 0.20f, 0.20f };	//functions
	F4 color_muted_teal[] = { 0.15f, 0.30f, 0.30f };	//constants
	F4 color_light_gray[] = { 0.33f, 0.33f, 0.33f };	//controls
	F4 color_orange[] = { 1.00f, 0.22f, 0.04f };		//primary
	for (U8 button_index = 0; button_index < NUMPAD_COUNT; ++button_index)
	{
		U4 row = (U4)button_index / NUMPAD_COLUMNS;
		U4 column = (U4)button_index % NUMPAD_COLUMNS;

		UIRect cell_rect = ui_resolve_grid_cell(
			row, column,
			NUMPAD_ROWS, NUMPAD_COLUMNS,
			10.f,
			button_panel_rect
		);

		F4* color;
		switch (calculator_categories[button_index])
		{
		case CATEGORY_BINARY: color = color_light_gray; break;
		case CATEGORY_FUNCTION: color = color_dark_gray; break;
		case CATEGORY_CONSTANT: color = color_blue_gray; break;
		case CATEGORY_CONTROL: color = color_muted_teal; break;
		default:
		{
			DEBUG_RUNTIME_ASSERT(0, "Invalid button category");
			color = color_light_gray; break;
		}
		}

		if (button_index == NUMPAD_COUNT - 1)
		{
			color = color_orange;
		}

		// Hit test
		U1 is_hovered =
			input_state->mouse_x >= cell_rect.left &&
			input_state->mouse_x <= cell_rect.left + cell_rect.width &&
			input_state->mouse_y >= cell_rect.top &&
			input_state->mouse_y <= cell_rect.top + cell_rect.height;

		U1 is_pressed = is_hovered && input_state->mouse_left_released;

		F4 color_tinted[3];
		if (is_hovered && input_state->mouse_left_held)
		{
			for (int c = 0; c < 3; ++c)
			{
				color_tinted[c] = color[c] * 0.7f;
			}
		}
		else if (is_hovered)
		{
			for (int c = 0; c < 3; ++c)
			{
				color_tinted[c] = color[c] * 1.25f + 0.04f;
				if (color_tinted[c] > 1.f) color_tinted[c] = 1.f;
			}
		}
		else
		{
			for (int c = 0; c < 3; ++c)
			{
				color_tinted[c] = color[c];
			}
		}

		//Render button background
		renderer_push_quad(renderer_context, cell_rect.left, cell_rect.top, cell_rect.width, cell_rect.height, color_tinted[0], color_tinted[1], color_tinted[2]);

		// label text
		{
			StringView label = string_create_stack(calculator_labels[button_index]);
			F4 font_size = renderer_get_font_size(renderer_context);
			F4 text_width = renderer_measure_text(renderer_context, label);
			F4 text_x = cell_rect.left + (cell_rect.width - text_width) * 0.5f;
			F4 text_y = cell_rect.top + (cell_rect.height - font_size) * 0.5f;
			renderer_push_text(
				renderer_context,
				label,
				text_x,
				text_y,
				font_size,
				1.f, 1.f, 1.f
			);
		}

		if (is_pressed && simulation_input.mouse_button_click_count < 8)
		{
			simulation_input.mouse_button_indices[simulation_input.mouse_button_click_count++] = (U4)button_index;
			console_log_string(string_create_stack(calculator_labels[button_index]));
		}
	}

	return simulation_input;
}

