#include "../core/debug.h"
#include "../core/math.h"

void math_test(void)
{
	DEBUG_RUNTIME_ASSERT(math_is_equal(math_abs(-5.0), 5.0, 1e-12), "Abs failed");
	DEBUG_RUNTIME_ASSERT(math_is_equal(math_floor(3.7), 3.0, 1e-12), "Floor+ failed");
	DEBUG_RUNTIME_ASSERT(math_is_equal(math_floor(-3.7), -4.0, 1e-12), "Floor- failed");
	DEBUG_RUNTIME_ASSERT(math_is_equal(math_ceil(3.2), 4.0, 1e-12), "Ceil+ failed");
	DEBUG_RUNTIME_ASSERT(math_is_equal(math_ceil(-3.2), -3.0, 1e-12), "Ceil- failed");
	DEBUG_RUNTIME_ASSERT(math_is_equal(math_round(2.5), 3.0, 1e-12), "Round+ failed");
	DEBUG_RUNTIME_ASSERT(math_is_equal(math_round(-2.5), -3.0, 1e-12), "Round- failed");
	DEBUG_RUNTIME_ASSERT(math_is_equal(math_fmod(7.0, 3.0), 1.0, 1e-12), "Fmod+ failed");
	DEBUG_RUNTIME_ASSERT(math_is_equal(math_fmod(-7.0, 3.0), -1.0, 1e-12), "Fmod- failed");

	// MathSqrt
	DEBUG_RUNTIME_ASSERT(math_sqrt(0.0) == 0.0, "Sqrt(0) failed");
	DEBUG_RUNTIME_ASSERT(math_sqrt(1.0) == 1.0, "Sqrt(1) failed");
	DEBUG_RUNTIME_ASSERT(math_sqrt(4.0) == 2.0, "Sqrt(4) failed");
	DEBUG_RUNTIME_ASSERT(math_sqrt(9.0) == 3.0, "Sqrt(9) failed");

	F8 sqrt2 = math_sqrt(2.0);
	DEBUG_RUNTIME_ASSERT(math_abs(sqrt2 - 1.41421356) < 1e-8, "Sqrt(2) precision failed");

	DEBUG_RUNTIME_ASSERT(math_F4_sqrt(4.0f) == 2.0f, "SqrtF4(4) failed");
	DEBUG_RUNTIME_ASSERT(math_F4_abs(math_F4_rsqrt(4.0f) - 0.5f) < 1e-6f, "Rsqrt(4) failed");
	DEBUG_RUNTIME_ASSERT(math_F4_abs(math_F4_rsqrt(1.0f) - 1.0f) < 1e-6f, "Rsqrt(1) failed");
	DEBUG_RUNTIME_ASSERT(math_F4_abs(math_F4_rsqrt(2.0f) - 0.70710678f) < 1e-6f, "Rsqrt(2) failed");
	DEBUG_RUNTIME_ASSERT(math_F4_abs(math_F4_rsqrt(9.0f) * 3.0f - 1.0f) < 1e-6f, "Rsqrt(9) identity failed");

	DEBUG_RUNTIME_ASSERT(math_sin_radian(0.0) == 0.0, "Sin(0) failed");
	DEBUG_RUNTIME_ASSERT(math_cos_radian(0.0) == 1.0, "Cos(0) failed");
	DEBUG_RUNTIME_ASSERT(math_abs(math_sin_radian(MATH_PI_OVER_2) - 1.0) < 1e-6, "Sin(pi/2) failed");
	DEBUG_RUNTIME_ASSERT(math_abs(math_cos_radian(MATH_PI) - -1.0) < 1e-3, "Cos(pi) failed");
	DEBUG_RUNTIME_ASSERT(math_abs(math_sin_radian(-MATH_PI_OVER_2) - -1.0) < 1e-6, "Sin(-pi/2) failed");

	DEBUG_RUNTIME_ASSERT(math_abs(math_sin_radian(MATH_PI / 6.0) - 0.5) < 1e-10, "Sin(pi/6) failed");
	DEBUG_RUNTIME_ASSERT(math_abs(math_cos_radian(MATH_PI / 3.0) - 0.5) < 1e-10, "Cos(pi/3) failed");
	DEBUG_RUNTIME_ASSERT(math_abs(math_tan_radian(MATH_PI / 4.0) - 1.0) < 1e-10, "Tan(pi/4) failed");
	//DEBUG_RUNTIME_ASSERT(math_abs(math_sin(MATH_PI)) < 1e-12, "Sin(pi) = 0 failed");
	//DEBUG_RUNTIME_ASSERT(math_abs(math_cos(MATH_PI_OVER_2)) < 1e-12, "Cos(pi/2) = 0 failed");
	DEBUG_RUNTIME_ASSERT(math_abs(math_sin_radian(-1.0) + math_sin_radian(1.0)) < 1e-14, "Sin odd failed");
	DEBUG_RUNTIME_ASSERT(math_abs(math_cos_radian(-1.0) - math_cos_radian(1.0)) < 1e-14, "Cos even failed");
	//DEBUG_RUNTIME_ASSERT(math_abs(math_sin(100.0) - (-0.50636564110975879)) < 1e-8, "Sin(100) failed");

	// Ln
	DEBUG_RUNTIME_ASSERT(math_abs(math_ln(1.0)) < 1e-12, "Ln(1) failed");
	DEBUG_RUNTIME_ASSERT(math_abs(math_ln(MATH_E) - 1.0) < 1e-10, "Ln(e) failed");
	DEBUG_RUNTIME_ASSERT(math_abs(math_ln(2.0) - MATH_LN2) < 1e-10, "Ln(2) failed");
	DEBUG_RUNTIME_ASSERT(math_abs(math_ln(1.0 / MATH_E) + 1.0) < 1e-10, "Ln(1/e) failed");

	// Exp
	DEBUG_RUNTIME_ASSERT(math_abs(math_exp(0.0) - 1.0) < 1e-12, "Exp(0) failed");
	//DEBUG_RUNTIME_ASSERT(math_abs(math_exp(1.0) - MATH_E) < 1e-10, "Exp(1) failed");
	//DEBUG_RUNTIME_ASSERT(math_abs(math_exp(MATH_LN2) - 2.0) < 1e-10, "Exp(ln2) failed");

	// Ln/Exp roundtrip
	//DEBUG_RUNTIME_ASSERT(math_abs(math_exp(math_ln(5.0)) - 5.0) < 1e-10, "Exp(Ln(5)) failed");

	// Pow
	//DEBUG_RUNTIME_ASSERT(math_abs(math_pow(2.0, 10.0) - 1024.0) < 1e-8, "Pow(2,10) failed");
	//DEBUG_RUNTIME_ASSERT(math_abs(math_pow(4.0, 0.5) - 2.0) < 1e-10, "Pow(4,0.5) failed");
	//DEBUG_RUNTIME_ASSERT(math_abs(math_pow(-2.0, 3.0) + 8.0) < 1e-10, "Pow(-2,3) failed");

	// Log
	DEBUG_RUNTIME_ASSERT(math_abs(math_log(10.0) - 1.0) < 1e-10, "Log(10) failed");
	DEBUG_RUNTIME_ASSERT(math_abs(math_log(100.0) - 2.0) < 1e-10, "Log(100) failed");

	// Atan
	DEBUG_RUNTIME_ASSERT(math_abs(math_atan(0.0)) < 1e-12, "Atan(0) failed");
	DEBUG_RUNTIME_ASSERT(math_abs(math_atan(1.0) - MATH_PI_OVER_4) < 1e-10, "Atan(1) failed");
	DEBUG_RUNTIME_ASSERT(math_abs(math_atan(-1.0) + MATH_PI_OVER_4) < 1e-10, "Atan(-1) failed");
	DEBUG_RUNTIME_ASSERT(math_abs(math_atan(1e10) - MATH_PI_OVER_2) < 1e-8, "Atan(inf) failed");

	// Asin / Acos
	DEBUG_RUNTIME_ASSERT(math_abs(math_asin(0.0)) < 1e-12, "Asin(0) failed");
	DEBUG_RUNTIME_ASSERT(math_abs(math_asin(1.0) - MATH_PI_OVER_2) < 1e-10, "Asin(1) failed");
	DEBUG_RUNTIME_ASSERT(math_abs(math_acos(1.0)) < 1e-12, "Acos(1) failed");
	DEBUG_RUNTIME_ASSERT(math_abs(math_acos(0.0) - MATH_PI_OVER_2) < 1e-10, "Acos(0) failed");

	// Asin(sin(x)) roundtrip
	DEBUG_RUNTIME_ASSERT(math_abs(math_asin(math_sin_radian(0.5)) - 0.5) < 1e-10, "Asin(Sin(0.5)) failed");

	//for (F8 x = 0; x < 4; x += 0.1)
	//{
	//	F8 radian = math_sin_radian(x * MATH_PI);
	//	F8 turns = math_sin((Turn) { .half_turn = x });

	//	if (math_is_equal(radian, turns, 1e-12))
	//	{
	//		continue; //yes
	//	}
	//	else
	//	{
	//		continue; //no
	//	}

	//}
}
