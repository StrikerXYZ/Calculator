#pragma once

#include "../core/types.h"
#include "../core/platform.h"
#include "../core/renderer.h"
#include "simulation.h"

#define NUMPAD_ROWS 5
#define NUMPAD_COLUMNS 5
#define NUMPAD_COUNT (NUMPAD_ROWS * NUMPAD_COLUMNS)

typedef struct
{
	F4 button_press_timers[NUMPAD_COUNT];
	F4 display_flash_timer;
	U8 last_simulation_tick;
} PresentationState;

void presentation_initialize(PresentationState* presentation_state);

SimulationInput presentation_update(
	PresentationState* presentation_state, 
	SimulationState const * simulation_state,
	InputState const * input_state,
	RendererContext renderer_context,
	U4 surface_width, 
	U4 surface_height
);
