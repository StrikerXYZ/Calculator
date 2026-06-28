#include "../core/platform.h"
#include "../core/memory.h"
#include "../core/debug.h"
#include "simulation.h"
#include "presentation.h"
#include "glue.h"

global SimulationState   simulation_state;
global PresentationState presentation_state;

void run(
	Arena* arena, 
	InputState* input_state, 
	RendererContext renderer_context, 
	U4 surface_width, 
	U4 surface_height
)
{
	if (!simulation_state.initialized)
	{
		simulation_initialize(&simulation_state);
		presentation_initialize(&presentation_state);
	}

	SimulationInput simulation_input = presentation_update(
		&presentation_state,
		&simulation_state,
		input_state,
		renderer_context,
		surface_width,
		surface_height
	);

	simulation_tick(&simulation_state, &simulation_input);

	(void)arena; // Not used
}
