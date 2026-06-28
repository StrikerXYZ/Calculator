#include "simulation.h"
#include "../core/debug.h"
#include "../core/math.h"

internal void simulation_set_display(SimulationState* simulation_state, StringView display_value)
{
	simulation_state->display_size = (U4)math_u8_min(display_value.size, SIMULATION_DISPLAY_CAP);
	memory_copy(simulation_state->display, display_value.data, simulation_state->display_size);
}

internal void simulation_set_error(SimulationState* simulation_state)
{
	simulation_state->has_error = 1;
	simulation_state->result_shown = 1;
	simulation_set_display(simulation_state, string_create_stack("Error"));
}

internal void simulation_reset(SimulationState* simulation_state)
{
	simulation_state->pending_operation = SIMULATION_OP_NONE;
	simulation_state->operand_a = 0.0;
	simulation_state->result_shown = 0;
	simulation_state->has_error = 0;
	simulation_state->last_answer = 0.0;
	simulation_state->has_last_answer = 0;
	simulation_set_display(simulation_state, string_create_stack("0"));
}

internal U4 simulation_format_f8(F8 value, Char* buffer_data, U8 buffer_capacity)
{
	DEBUG_RUNTIME_ASSERT(buffer_capacity > 2, "Buffer size must be greater than 2");

	if(value >= 1e15)
	{
		const Char message[] = "Overflow";
		U8 buffer_size = math_u8_min(sizeof(message), buffer_capacity);
		memory_copy(buffer_data, message, buffer_size);
		return (U4)buffer_size;
	}

	StringView string = buffer_convert_from_f8(buffer_data, buffer_capacity, value);
	U4 buffer_size = (U4)string.size;
	return buffer_size;
}

internal void simulation_set_result(SimulationState* simulation_state, F8 value)
{
	simulation_state->display_size = simulation_format_f8(value, simulation_state->display, SIMULATION_DISPLAY_CAP);
	simulation_state->result_shown = 1;
	simulation_state->has_error = 0;
	simulation_state->last_answer = value;
	simulation_state->has_last_answer = 1;
}

internal F8 simulation_apply_operation(F8 a, SimulationOperation operation, F8 b)
{
	switch (operation)
	{
	case SIMULATION_OP_ADD: return a + b;
	case SIMULATION_OP_SUB: return a - b;
	case SIMULATION_OP_MUL: return a * b;
	case SIMULATION_OP_DIV: return (b == 0.0) ? 0 : a / b;
	case SIMULATION_OP_MOD: return (b == 0.0) ? 0 : math_fmod(a, b);
	case SIMULATION_OP_POW: return math_pow(a, b);
	default:
	case SIMULATION_OP_NONE: return b;
	}
}

internal F8 simulation_factorial(F8 x)
{
	DEBUG_RUNTIME_ASSERT(x >= 0.0 && math_is_equal(x, math_floor(x), 1e-12), "Factorial input must be a non-negative integer");
	DEBUG_RUNTIME_ASSERT(x < 20.0, "Factorial input must be less than 20");
	U8 result = 1;
	for (U8 i = 2; i <= (U8)x; ++i) result *= i;
	return (F8)result;
}

internal void simulation_handle_digit(SimulationState* simulation_state, Char c)
{
	if (simulation_state->has_error)
	{
		simulation_reset(simulation_state);
	}

	// After a result: start fresh unless this digit continues a decimal
	if (simulation_state->result_shown)
	{
		simulation_set_display(simulation_state, string_create_stack("0"));
		simulation_state->result_shown = 0;
	}

	if (c == '.')
	{
		for (U4 i = 0; i < simulation_state->display_size; ++i)
			if (simulation_state->display[i] == '.') return; // already has dot
		if (simulation_state->display_size >= SIMULATION_DISPLAY_CAP) return;
		simulation_state->display[simulation_state->display_size++] = '.';
		return;
	}

	if (simulation_state->display_size == 1 && simulation_state->display[0] == '0' && c != '.')
	{
		simulation_state->display[0] = c;
		simulation_state->display_size = 1;
		return;
	}

	if (simulation_state->display_size >= SIMULATION_DISPLAY_CAP) return;
	simulation_state->display[simulation_state->display_size++] = c;
}

internal void simulation_handle_backspace(SimulationState* simulation_state)
{
	if (
		simulation_state->has_error 
		|| simulation_state->result_shown
		)
	{
		simulation_set_display(simulation_state, string_create_stack("0"));
		simulation_state->result_shown = 0;
		return;
	}
	if (simulation_state->display_size > 1)
	{
		simulation_state->display_size--;
	}
	else
	{
		simulation_set_display(simulation_state, string_create_stack("0"));
	}
}

internal void simulation_handle_binary_op(SimulationState* simulation_state, SimulationOperation op)
{
	if (simulation_state->has_error) return;

	F8 current = buffer_convert_to_f8(simulation_state->display, simulation_state->display_size);
	
	// Chain: compute pending before storing new op
	if (
		simulation_state->pending_operation != SIMULATION_OP_NONE 
		&& !simulation_state->result_shown
		)
	{
		F8 chain = simulation_apply_operation(simulation_state->operand_a, simulation_state->pending_operation, current);
		simulation_set_result(simulation_state, chain);
		simulation_state->operand_a = chain;
	}
	else
	{
		simulation_state->operand_a = current;
	}
	simulation_state->pending_operation = op;
	simulation_state->result_shown = 1;
}

internal void simulation_handle_equals(SimulationState* simulation_state)
{
	if (simulation_state->has_error || simulation_state->pending_operation == SIMULATION_OP_NONE) return;
	F8 b = buffer_convert_to_f8(simulation_state->display, simulation_state->display_size);
	F8 result = simulation_apply_operation(simulation_state->operand_a, simulation_state->pending_operation, b);
	simulation_state->pending_operation = SIMULATION_OP_NONE;
	simulation_set_result(simulation_state, result);
}

internal void simulation_handle_button(SimulationState* simulation_state, U8 index)
{
	if (simulation_state->has_error && index != 20) return; // only C works during error

	F8 value = buffer_convert_to_f8(simulation_state->display, simulation_state->display_size);

	switch (index)
	{
		default: break;

		// Row 0: binary operators
		case 0: simulation_handle_binary_op(simulation_state, SIMULATION_OP_ADD); break;
		case 1: simulation_handle_binary_op(simulation_state, SIMULATION_OP_SUB); break;
		case 2: simulation_handle_binary_op(simulation_state, SIMULATION_OP_MUL); break;
		case 3: simulation_handle_binary_op(simulation_state, SIMULATION_OP_DIV); break;
		case 4: simulation_handle_binary_op(simulation_state, SIMULATION_OP_MOD); break;

		// Row 1: sin, cos, tan, ln
		case 5: simulation_set_result(simulation_state, math_sin_radian(value)); break;
		case 6: simulation_set_result(simulation_state, math_cos_radian(value)); break;
		case 7: simulation_set_result(simulation_state, math_tan_radian(value)); break;
		case 8:
		{
			if (value <= 0.0) { simulation_set_error(simulation_state); break; }
			simulation_set_result(simulation_state, math_ln(value));
			break;
		}
		case 9: simulation_handle_binary_op(simulation_state, SIMULATION_OP_POW); break;

		// Row 2: asin, acos, atan, e^x, Pi
		case 10:
		{
			if (value < -1.0 || value > 1.0) { simulation_set_error(simulation_state); break; }
			simulation_set_result(simulation_state, math_asin(value));
			break;
		}

		case 11:
		{
			if (value < -1.0 || value > 1.0) { simulation_set_error(simulation_state); break; }
			simulation_set_result(simulation_state, math_acos(value));
			break;
		}

		case 12:
		{
			simulation_set_result(simulation_state, math_atan(value));
			break;
		}

		case 13:
		{
			if (value > 709.0) { simulation_set_error(simulation_state); break; }
			simulation_set_result(simulation_state, math_exp(value));
			break;
		}

		case 14:
		{
			simulation_set_result(simulation_state, MATH_PI);
			break;
		}

		case 15:
		{
			simulation_set_result(simulation_state, value * value);
			break;
		}

		case 16:
		{
			if (value < 0.0) { simulation_set_error(simulation_state); break; }
			simulation_set_result(simulation_state, math_sqrt(value));
			break;
		}

		case 17:
		{
			if (
				value < 0.0 
				|| !math_is_equal(value, math_floor(value), 1e-12) 
				|| value >= 20.0)
			{
				simulation_set_error(simulation_state);
				break;
			}
			simulation_set_result(simulation_state, simulation_factorial(value));
			break;
		}

		case 18:
		{
			if (value <= 0.0) { simulation_set_error(simulation_state); break; }
			simulation_set_result(simulation_state, math_log(value));
			break;
		}

		case 19:
		{
			simulation_set_result(simulation_state, MATH_E);
			break;
		}

		case 20:
		{
			simulation_reset(simulation_state); break; // Clear
		}

		case 21: // +/-
		{
			if (simulation_state->display[0] == '-')
			{
				for (U4 i = 0; i < simulation_state->display_size; ++i)
					simulation_state->display[i] = simulation_state->display[i + 1];
				simulation_state->display_size--;
			}
			else if (!(simulation_state->display_size == 1 && simulation_state->display[0] == '0'))
			{
				if (simulation_state->display_size < SIMULATION_DISPLAY_CAP)
				{
					for (U4 i = simulation_state->display_size + 1; i > 0; --i)
						simulation_state->display[i] = simulation_state->display[i - 1];
					simulation_state->display[0] = '-';
					simulation_state->display_size++;
				}
			}
			break;
		}

		case 22: // 1/x
		{
			if (value == 0.0) { simulation_set_error(simulation_state); break; }
			simulation_set_result(simulation_state, 1.0 / value);
			break;
		}

		case 23: // ans
		{
			if (simulation_state->has_last_answer)
			{
				simulation_state->display_size = simulation_format_f8(simulation_state->last_answer, simulation_state->display, SIMULATION_DISPLAY_CAP);
				simulation_state->result_shown = 1;
				simulation_state->has_error = 0;
			}
			break;
		}

		case 24:
		{
			simulation_handle_equals(simulation_state); break; // =
		}
	}
}

void simulation_initialize(SimulationState* simulation_state)
{
	DEBUG_RUNTIME_ASSERT(!simulation_state->initialized, "Simulation state already initialized");
	simulation_reset(simulation_state);
	simulation_state->initialized = 1;
}

void simulation_tick(SimulationState* simulation_state, SimulationInput const * input)
{
	//Process keyboard characters
	for (U4 i = 0; i < input->char_count; ++i)
	{
		Char c = input->chars[i];
		if (c >= '0' && c <= '9') simulation_handle_digit(simulation_state, c);
		else if (c == '.') simulation_handle_digit(simulation_state, c);
		else if (c == '+') simulation_handle_binary_op(simulation_state, SIMULATION_OP_ADD);
		else if (c == '-') simulation_handle_binary_op(simulation_state, SIMULATION_OP_SUB);
		else if (c == '*') simulation_handle_binary_op(simulation_state, SIMULATION_OP_MUL);
		else if (c == '/') simulation_handle_binary_op(simulation_state, SIMULATION_OP_DIV);
		else if (c == '%') simulation_handle_binary_op(simulation_state, SIMULATION_OP_MOD);
		else if (c == '^') simulation_handle_binary_op(simulation_state, SIMULATION_OP_POW);
		else if (c == '=') simulation_handle_equals(simulation_state);
	}

	if (input->key_backspace) simulation_handle_backspace(simulation_state);
	if (input->key_enter) simulation_handle_equals(simulation_state);
	if (input->key_escape) simulation_reset(simulation_state);

	// Process mouse button clicks
	for (U4 i = 0; i < input->mouse_button_click_count; ++i)
	{
		U8 button_index = input->mouse_button_indices[i];
		simulation_handle_button(simulation_state, button_index);
	}

	simulation_state->simulation_tick++;
}
