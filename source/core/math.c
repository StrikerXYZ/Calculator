// Striker 2026

#include "math.h"
#include "debug.h"

#include <immintrin.h>

// IEEE 754 bit access - 1 Sign bit, 11 Exponent bits, 52 Mantissa bits
typedef union
{
	F8 value;
	U8 bits;
} F8Bits;

// IEEE 754 bit access - 1 Sign bit, 8 Exponent bits, 23 Mantissa bits
typedef union
{
	F4 value;
	U4 bits;
} F4Bits;


U8 math_u8_min(U8 a, U8 b)
{
	U8 result = (a <= b) ? a : b;
	return result;
}

U8 math_u8_max(U8 a, U8 b)
{
	U8 result = (a >= b) ? a : b;
	return result;
}

F8 math_f8_min(F8 a, F8 b)
{
	F8 result = (a <= b) ? a : b;
	return result;
}

F8 math_f8_max(F8 a, F8 b)
{
	F8 result = (a >= b) ? a : b;
	return result;
}

U1 math_is_finite(F8 x)
{
	F8Bits f8_bits = { .value = x };
	U8 exponent = (f8_bits.bits >> 52) & 0x7FF; // Extract the exponent bits
	U1 result = (exponent != 0x7FF); // Exponent of 0x7FF indicates Inf or NaN
	return result;
}

U1 math_is_equal(F8 x, F8 y, F8 epsilon)
{
	if (!math_is_finite(x) || !math_is_finite(y)) return 0;

	U1 result = math_abs(x - y) < epsilon;
	return result;
}

F8 math_abs(F8 x)
{
	F8 result;
	F8Bits f8_bits = { .value = x };
	f8_bits.bits &= 0x7FFFFFFFFFFFFFFF; // Clear the sign bit
	result = f8_bits.value;
	return result;
}

F4 math_F4_abs(F4 x)
{
	F4 result;
	F4Bits f4_bits = { .value = x };
	f4_bits.bits &= 0x7FFFFFFF; // Clear the sign bit
	result = f4_bits.value;
	return result;
}

F8 math_trunc(F8 x)
{
	if (math_abs(x) > MATH_F8_MANTISSA_LIMIT) return x;

	F8 result = (F8)(S8)x;
	return result;
}

F8 math_floor(F8 x)
{
	if (math_abs(x) > MATH_F8_MANTISSA_LIMIT) return x;

	S8 integer = (S8)x;
	F8 result = (F8)(integer - (x < (F8)integer));
	return result;
}

F8 math_ceil(F8 x)
{
	if (math_abs(x) > MATH_F8_MANTISSA_LIMIT) return x;

	S8 integer = (S8)x;
	F8 result = (F8)(integer + (x > (F8)integer));
	return result;
}

// MathRound
//
// Rounds to nearest integer, ties away from zero.
// This is NOT IEEE 754 round-half-to-even (banker's rounding).
//
// Tradeoff: round-half-away-from-zero accumulates bias over many
// operations (half values always round the same direction). IEEE 754
// round-half-to-even distributes the bias — half of the time it rounds
// up, half of the time down. For game dev the bias is irrelevant.
// For financial calculations it matters — another reason to know this.
F8 math_round(F8 x)
{
	F8 result = (x >= 0) ? math_floor(x + 0.5) : math_ceil(x - 0.5);
	return result;
}

// MathFmod
//
// Tradeoff: this implementation has reduced accuracy for very large x
// divided by small y (catastrophic cancellation). For game dev inputs
// (angles rarely exceed 1000 * 2π) this is fine. For a general library
// you would use the extended precision Payne-Hanek reduction algorithm,
// which is significantly more complex.
F8 math_fmod(F8 x, F8 y)
{
	if (y == 0.0) 
	{
		DEBUG_RUNTIME_ASSERT(y != 0.0, "Division by zero in math_fmod");
		return 0.0;
	}

	F8 result;
	F8 quotient = math_trunc(x / y);
	result = x - quotient * y;
	return result;
}

F8 math_sqrt(F8 x)
{
	DEBUG_RUNTIME_ASSERT(x >= 0.0, "Square root of negative number");

	__m128d value = _mm_set_sd(x);
	F8 result = _mm_cvtsd_f64(_mm_sqrt_sd(value, value));
	return result;
}

F4 math_F4_sqrt(F4 x)
{
	DEBUG_RUNTIME_ASSERT(x >= 0.0f, "Square root of negative number");

	__m128 value = _mm_set_ss(x);
	F4 result = _mm_cvtss_f32(_mm_sqrt_ss(value));
	return result;
}

// Step 1: _mm_rsqrt_ss uses a lookup table to produce ~12-bit estimate
// Step 2: Newton-Raphson refinement: y = y * (1.5f - 0.5f * x * y * y) - gives ~23-bit estimate (7 decimal places)
F4 math_F4_rsqrt(F4 x)
{
	DEBUG_RUNTIME_ASSERT(x > 0.0f, "Reciprocal square root of non-positive number");

	__m128 value = _mm_set_ss(x);
	F4 value_rsqrt_12bit = _mm_cvtss_f32(_mm_rsqrt_ss(value));
	F4 result = value_rsqrt_12bit * (1.5f - 0.5f * x * value_rsqrt_12bit * value_rsqrt_12bit);
	return result;
}

F8 math_rsqrt(F8 x)
{
	DEBUG_RUNTIME_ASSERT(x > 0.0, "Reciprocal square root of non-positive number");

	__m128d value = _mm_set_sd(x);
	F8 sqrt_value = _mm_cvtsd_f64(_mm_sqrt_sd(value, value));
	F8 result = 1.0 / sqrt_value;
	return result;
}

// Reduce range to [-π/4, π/4] and determine quadrant for sin/cos
// Why k & 3 works for negative k:
// -1 & 3 = 3, -2 & 3 = 2, -3 & 3 = 1 — correct by the symmetry
// identities sin(-x) = -sin(x), cos(-x) = cos(x)
internal S4 math_sin_cos_reduce_radian(F8 x, F8* out_r)
{
	F8 value_rounded = math_round(x * MATH_2_OVER_PI);
	*out_r = (x - value_rounded * MATH_PI_OVER_2_HI) - value_rounded * MATH_PI_OVER_2_LO;

	// Get quadrant from the last 2 bits of the rounded value
	U1 result = (((S4)(value_rounded)) & 3);
	return result;
}
internal U1 math_sin_cos_reduce(Turn turns, Turn* out_turns)
{
	DEBUG_RUNTIME_ASSERT(out_turns, "out_turns is null");

	// Radians = half_turn * Pi
	F8 value_rounded = { math_round(turns.half_turn * 2) };
	out_turns->half_turn = (turns.half_turn - value_rounded * 0.5);

	// Get quadrant from the last 2 bits of the rounded value
	U1 result = (((S4)(value_rounded)) & 3);
	return result;
}

internal F8 math_sin_poly_radian(F8 x)
{
	F8 x2 = x * x;
	F8 result = x * (1.0 + x2 * (
		-1.6666666666666666e-1 + x2 * (
			8.3333333333333333e-3 + x2 * (
				-1.9841269841269841e-4 + x2 * (
					2.7557319223985888e-6 + x2 * (
						-2.5052108385441720e-8 + x2 * (
							1.6059043836821613e-10)))))));
	return result;
}
internal F8 math_sin_poly(Turn turns)
{
	F8 x = turns.half_turn;
	F8 x2 = turns.half_turn * turns.half_turn;

	F8 result = x * (MATH_PI + x2 * (
		-1.644934066848226370675 + x2 * (
			8.22467033424113214946e-2 + x2 * (
				-1.958254841485983826311e-3 + x2 * (
					2.71979839095275508951e-5 + x2 * (
						-2.472543991775232258452e-7 + x2 * (
							1.58496409729181531493e-9)))))));
	return result;
}

internal F8 math_cos_poly_radian(F8 x)
{
	F8 x2 = x * x;
	F8 result = 1.0 + x2 * (
		-5.0000000000000000e-1 + x2 * (
			4.1666666666666664e-2 + x2 * (
				-1.3888888888888889e-3 + x2 * (
					2.4801587301587302e-5 + x2 * (
						-2.7557319223985888e-7 + x2 * (
							2.0876756987868100e-9))))));
	return result;
}
internal F8 math_cos_poly(Turn turns)
{
	F8 x2 = turns.half_turn * turns.half_turn;

	F8 result = 1.0 + x2 * (
		-4.93480220054467930941 + x2 * (
			4.11233516712056582799e-1 + x2 * (
				-1.370778389040188708026e-2 + x2 * (
					2.447818551857479856910e-4 + x2 * (
						-2.71979839095275508951e-6 + x2 * (
							2.06045332647936021537e-8))))));
	return result;
}

F8 math_sin_radian(F8 x)
{
	F8 value_reduced;
	S4 quadrant = math_sin_cos_reduce_radian(x, &value_reduced);
	F8 result;
	switch (quadrant)
	{
	default:
	case 0: result = math_sin_poly_radian(value_reduced); break;
	case 1: result =  math_cos_poly_radian(value_reduced); break;
	case 2: result = -math_sin_poly_radian(value_reduced); break;
	case 3: result = -math_cos_poly_radian(value_reduced); break;
	}
	return result;
}

F8 math_sin(Turn turns)
{
	Turn turns_reduced;
	U1 quadrant = math_sin_cos_reduce(turns, &turns_reduced);
	F8 result;
	switch (quadrant)
	{
	default:
	case 0: result = math_sin_poly(turns_reduced); break;
	case 1: result = math_cos_poly(turns_reduced); break;
	case 2: result = -math_sin_poly(turns_reduced); break;
	case 3: result = -math_cos_poly(turns_reduced); break;
	}
	return result;
}

F8 math_cos_radian(F8 x)
{
	F8 value_reduced;
	S4 quadrant = math_sin_cos_reduce_radian(x, &value_reduced);
	F8 result;
	switch (quadrant)
	{
	default:
	case 0: result =  math_cos_poly_radian(value_reduced); break;
	case 1: result = -math_sin_poly_radian(value_reduced); break;
	case 2: result = -math_cos_poly_radian(value_reduced); break;
	case 3: result =  math_sin_poly_radian(value_reduced); break;
	}
	return result;
}

F8 math_cos(Turn x)
{
	Turn value_reduced;
	U1 quadrant = math_sin_cos_reduce(x, &value_reduced);
	F8 result;
	switch (quadrant)
	{
	default:
	case 0: result = math_cos_poly(value_reduced); break;
	case 1: result = -math_sin_poly(value_reduced); break;
	case 2: result = -math_cos_poly(value_reduced); break;
	case 3: result = math_sin_poly(value_reduced); break;
	}
	return result;
}

F8 math_tan_radian(F8 x)
{
	F8 r;
	S4 q = math_sin_cos_reduce_radian(x, &r);

	F8 s = math_sin_poly_radian(r);
	F8 c = math_cos_poly_radian(r);

	F8 sin_val, cos_val;
	switch (q)
	{
	case 0: sin_val = s; cos_val = c; break;
	case 1: sin_val = c; cos_val = -s; break;
	case 2: sin_val = -s; cos_val = -c; break;
	case 3: sin_val = -c; cos_val = s; break;
	default: return 0.0;
	}

	// Guard against division by near-zero (tan undefined at π/2 + kπ)
	if (math_abs(cos_val) < 1e-15) return 0.0;
	return sin_val / cos_val;
}

F8 math_tan(Turn turns)
{
	Turn r;
	S4 q = math_sin_cos_reduce(turns, &r);

	F8 s = math_sin_poly(r);
	F8 c = math_cos_poly(r);

	F8 sin_val, cos_val;
	switch (q)
	{
	case 0: sin_val = s; cos_val = c; break;
	case 1: sin_val = c; cos_val = -s; break;
	case 2: sin_val = -s; cos_val = -c; break;
	case 3: sin_val = -c; cos_val = s; break;
	default: return 0.0;
	}

	// Guard against division by near-zero (tan undefined at π/2 + kπ)
	if (math_abs(cos_val) < 1e-15) return 0.0;
	return sin_val / cos_val;
}

// MathLn
// Algorithm:
//   1. Extract exponent e and mantissa m from IEEE 754 bits
//   2. If m > sqrt(2): m /= 2, e += 1  (so m = [1/sqrt(2), sqrt(2)])
//   3. Compute ln(m) via substitution s = (m-1)/(m+1):
//      ln(m) = 2*s*(1 + s²/3 + s⁴/5 + s⁶/7 + s⁸/9 + s¹⁰/11 + s¹²/13)
//   4. Result: ln(m) + e * ln(2)
//
// Why step 2 reduces to [1/sqrt(2), sqrt(2)]:
//   Without it, m ∈ [1, 2) and s = (m-1)/(m+1) = [0, 0.333].
//   With it, m ∈ [1/sqrt(2), sqrt(2)] and |s| ≤ 0.172 — the series converges ~4x faster.
//
// Subnormal numbers (exponent field = 0) are not handled — their values
// are < 2^-1022, far below any calculator input.
//
// Accuracy: 7 terms gives error < (0.172)^15/15 = 1.4e-13 — 12 decimal places.
// -----------------------------------------------------------------------
F8 math_ln(F8 x)
{
	DEBUG_RUNTIME_ASSERT(x > 0.0, "ln of non-positive number");

	F8Bits f8_bits;
	f8_bits.value = x;

	// Extract unbiased exponent
	S4 e = (S4)((f8_bits.bits >> 52) & 0x7FF) - 1023;

	// Set exponent to 0 (biased: 1023) to isolate mantissa m in [1, 2)
	f8_bits.bits = (f8_bits.bits & 0x000FFFFFFFFFFFFFULL) | (0x3FFULL << 52);
	F8 m = f8_bits.value;

	// Reduce m to [1/√2, √2] for faster convergence
	if (m > MATH_SQRT2)
	{
		m *= 0.5;
		e += 1;
	}

	// s = (m-1)/(m+1), |s| <= 0.172
	F8 s = (m - 1.0) / (m + 1.0);
	F8 s2 = s * s;

	// ln(m) = 2*s*(1 + s²/3 + s⁴/5 + ...) — Horner form
	F8 ln_m = 2.0 * s * (1.0 + s2 * (
		1.0 / 3.0 + s2 * (
			1.0 / 5.0 + s2 * (
				1.0 / 7.0 + s2 * (
					1.0 / 9.0 + s2 * (
						1.0 / 11.0 + s2 *
						1.0 / 13.0))))));

	return ln_m + (F8)e * MATH_LN2;
}

// -----------------------------------------------------------------------
// MathExp
//
// Algorithm:
//   1. Decompose x = k*ln(2) + r, k = round(x * log2(e)), r ∈ [-ln2/2, ln2/2]
//   2. Compute e^r via polynomial on the small range
//   3. Scale by 2^k: add k to the IEEE 754 exponent field directly
//
// Step 3 via bit manipulation:
//   A double's value = 2^(exponent_field - 1023) * (1.mantissa_bits)
//   Adding k to exponent_field multiplies by 2^k exactly.
//   We use S8 arithmetic so negative k correctly subtracts from the field.
//
// Overflow/underflow bounds:
//   F8 max ≈ 1.8e308 = 2^1023 ≈ e^709.78. Above 709: overflow.
//   F8 min positive normal ≈ 2.2e-308. Below -709: result rounds to 0.
//
// Accuracy: 9 terms gives error < (0.347)^10/10! ≈ 2.6e-9.
// 10 terms: < (0.347)^11/11! ≈ 8.2e-11 — 10 decimal places.
// -----------------------------------------------------------------------
F8 math_exp(F8 x)
{
	DEBUG_RUNTIME_ASSERT(x < 709.0, "exp overflow");
	DEBUG_RUNTIME_ASSERT(x > -709.0, "exp underflow");

	F8Bits f8_bits;
	f8_bits.value = math_round(x * MATH_LOG2E);

	S4 k = (S4)f8_bits.bits;
	F8 r = x - (F8)k * MATH_LN2;

	// e^r via Taylor polynomial in Horner form, terms 1 through r^10/10!
	F8 exp_r = 1.0 + r * (1.0 + r * (
		5.0000000000000000e-1 + r * (
			1.6666666666666666e-1 + r * (
				4.1666666666666664e-2 + r * (
					8.3333333333333333e-3 + r * (
						1.3888888888888889e-3 + r * (
							1.9841269841269841e-4 + r * (
								2.4801587301587302e-5 + r * (
									2.7557319223985888e-6 + r *
									2.7557319223985888e-7)))))))));

	// Multiply by 2^k via exponent field adjustment
	//F8Bits f8_bits;
	f8_bits.value = exp_r;
	f8_bits.bits = (U8)((S8)f8_bits.bits + ((S8)k << 52));
	return f8_bits.value;
}

// -----------------------------------------------------------------------
// MathLog
//
// log10(x) = ln(x) / ln(10) = ln(x) * (1/ln(10))
// Precomputed constant avoids repeated division.
// -----------------------------------------------------------------------
F8 math_log(F8 x)
{
	DEBUG_RUNTIME_ASSERT(x > 0.0, "log of non-positive number");
	F8 ln_x = math_ln(x);
	return ln_x * MATH_INV_LN10;
}

// -----------------------------------------------------------------------
// MathPow
//
// x^y = e^(y * ln(x)) for x > 0.
//
// Special cases:
//   y = 0: 1.0 by convention (including 0^0)
//   x = 0, y > 0: 0
//   x = 0, y <= 0: undefined
//   x < 0, y is integer: handle sign via (-1)^y, compute |x|^y
//   x < 0, y non-integer: complex result — return NaN
//
// Why (-x)^y = |x|^y * sign:
//   (-2)^3 = -(2^3) = -8. The sign of the result is negative iff y is odd.
//   y_int & 1 tests the lowest bit — true for odd integers.
// -----------------------------------------------------------------------
F8 math_pow(F8 x, F8 y)
{
	if (y == 0.0)  return 1.0;
	if (x == 1.0)  return 1.0;

	if (x == 0.0)
	{
		if (y > 0.0) return 0.0;

		DEBUG_RUNTIME_ASSERT(y >= 0.0, "pow of non-positive number");
	}

	if (x < 0.0)
	{
		F8 y_floor = math_floor(y);

		DEBUG_RUNTIME_ASSERT(y == y_floor, "pow of non-whole number");

		F8 result = math_exp(y * math_ln(-x));
		S8 y_int = (S8)y;
		return (y_int & 1) ? -result : result; // odd power = negative
	}

	F8 result = math_exp(y * math_ln(x));
	return result;
}

// -----------------------------------------------------------------------
// MathAsin
//
// asin(x) = atan(x / sqrt(1 - x²))
//
// Why this works: if sin(θ) = x, then cos(θ) = sqrt(1 - x²), so
//   tan(θ) = sin(θ)/cos(θ) = x/sqrt(1 - x²), thus θ = atan(x/sqrt(1 - x²))
//
// At x = ±1: sqrt(1 - x²) = 0, division is undefined.
//   Numerically: x/sqrt(1-x²) → ∞ as x → ±1, and atan(∞) → ±π/2 correctly.
//   We handle exactly ±1 as a special case to avoid 0-division.
//
// Domain: [-1, 1]
// -----------------------------------------------------------------------
F8 math_asin(F8 x)
{
	DEBUG_RUNTIME_ASSERT(x >= -1.0 && x <= 1.0, "asin input out of range");

	if (x == 1.0) return  MATH_PI_OVER_2;
	if (x == -1.0) return -MATH_PI_OVER_2;
	return math_atan(x / math_sqrt(1.0 - x * x));
}

// -----------------------------------------------------------------------
// MathAcos
//
// acos(x) = π/2 - asin(x)
//
// From complementary angle identity: sin(θ) = cos(π/2 - θ)
//   If cos(θ) = x, then sin(π/2 - θ) = x, so π/2 - θ = asin(x)
//   Therefore θ = acos(x) = π/2 - asin(x)
//
// Domain: [-1, 1]
// -----------------------------------------------------------------------
F8 math_acos(F8 x)
{
	DEBUG_RUNTIME_ASSERT(x >= -1.0 && x <= 1.0, "acos input out of range");

	return MATH_PI_OVER_2 - math_asin(x);
}

// -----------------------------------------------------------------------
// math_atan_core (internal)
//
// Polynomial for atan(x) on x ∈ [-tan(π/8), tan(π/8)] ≈ [-0.414, 0.414]
//
// Taylor series: atan(x) = x - x³/3 + x⁵/5 - x⁷/7 + ...
// = x * (1 - x²/3 + x⁴/5 - x⁶/7 + x⁸/9 - x¹⁰/11 + x¹²/13 - x¹⁴/15 + x¹⁶/17)
//
// 9 terms in Horner form.
// Error at x = tan(π/8) ≈ 0.414: (0.414)^19/19 ≈ 2e-11 — 10 decimal places.
//
// The x * (...) structure preserves odd symmetry: atan(-x) = -atan(x).
// Negative x inputs are handled correctly without a sign branch.
// -----------------------------------------------------------------------
internal F8 math_atan_core(F8 x)
{
	F8 x2 = x * x;
	return x * (1.0 + x2 * (
		-1.0 / 3.0 + x2 * (
			1.0 / 5.0 + x2 * (
				-1.0 / 7.0 + x2 * (
					1.0 / 9.0 + x2 * (
						-1.0 / 11.0 + x2 * (
							1.0 / 13.0 + x2 * (
								-1.0 / 15.0 + x2 *
								1.0 / 17.0))))))));
}

// -----------------------------------------------------------------------
// MathAtan
//
// Range reduction — applied in order, undone in reverse:
//
//   If x > 1:
//     x ← 1/x                           [maps to (0, 1)]
//     result = π/2 - result              [because atan(x) + atan(1/x) = π/2]
//
//   If x > tan(π/8):
//     x ← (x-1)/(x+1)                  [maps to (-tan(π/8), 0] for x in (tan(π/8), 1]]
//     result += π/4                      [from addition formula for tan]
//
//   Both reductions can apply together — verified by exhaustive identity:
//     For x = 2: 1/x = 0.5, (0.5-1)/(0.5+1) = -1/3, result = atan(-1/3) + π/4 = atan(0.5)
//     Then result = π/2 - atan(0.5) = atan(2) ✓
//
//   Identity used: atan(x) = π/4 + atan((x-1)/(x+1))
//   Derivation: let x = tan(α), then (x-1)/(x+1) = (tan(α)-1)/(tan(α)+1)
//               = -tan(π/4 - α) so atan(...) = -(π/4 - α) = α - π/4 = atan(x) - π/4
//               Rearranged: atan(x) = π/4 + atan((x-1)/(x+1)) ✓
// -----------------------------------------------------------------------
F8 math_atan(F8 x)
{
	U1 negative = x < 0.0;
	if (negative) x = -x;

	U1 large = 0;
	U1 eighth = 0;

	if (x > 1.0)
	{
		x = 1.0 / x;
		large = 1;
	}
	if (x > MATH_TAN_PI8)
	{
		x = (x - 1.0) / (x + 1.0);
		eighth = 1;
	}

	F8 result = math_atan_core(x);

	if (eighth) result += MATH_PI_OVER_4;
	if (large)  result = MATH_PI_OVER_2 - result;
	if (negative) result = -result;

	return result;
}
