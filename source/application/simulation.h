#pragma once

#include "../core/types.h"

#define SIMULATION_DISPLAY_CAP 20

typedef enum
{
	SIMULATION_OP_NONE,
	SIMULATION_OP_ADD,
	SIMULATION_OP_SUB,
	SIMULATION_OP_MUL,
	SIMULATION_OP_DIV,
	SIMULATION_OP_MOD,
	SIMULATION_OP_POW,
} SimulationOperation;

// SimulationState
// Serializable struct
// no pointers, no handles
typedef struct
{
	Char display[SIMULATION_DISPLAY_CAP];
	U4 display_size;

	F8 operand_a;
	SimulationOperation pending_operation;

	U1 result_shown;
	U1 has_error;

	F8 last_answer;
	U1 has_last_answer;

	U8 simulation_tick;
	U1 initialized;
} SimulationState;

typedef struct
{
	Char chars[32];
	U4 char_count;
	U1 key_backspace;
	U1 key_enter;
	U1 key_escape;

	U4 mouse_button_indices[8];
	U4 mouse_button_click_count;
} SimulationInput;

void simulation_initialize(SimulationState* simulation_state);
void simulation_tick(SimulationState* simulation_state, SimulationInput const * input);
