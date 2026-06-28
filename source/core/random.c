#include "random.h"

SplitMix64State RandomSplitMix64StateCreate(U8 seed)
{
	return (SplitMix64State)(seed);
}

U8 RandomSplitMix64Generate(SplitMix64State* random_state)
{
	U8 z = (*random_state += 0x9E3779B97F4A7C15ULL);
	z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
	z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
	return z ^ (z >> 31);
}

XOShiro256State RandomXOShiro256StateCreate(U8 seed)
{
	XOShiro256State xoshire256_state;
	SplitMix64State state = RandomSplitMix64StateCreate(seed);

	xoshire256_state.state[0] = RandomSplitMix64Generate(&state);
	xoshire256_state.state[1] = RandomSplitMix64Generate(&state);
	xoshire256_state.state[2] = RandomSplitMix64Generate(&state);
	xoshire256_state.state[3] = RandomSplitMix64Generate(&state);

	return xoshire256_state;
}

U8 RandomXOShiro256Generate(XOShiro256State* random_state)
{
#define RandomXOShiro256Generate_ROTL(x, k) ((x << k) | (x >> (64 - k)))

	const U8 result = RandomXOShiro256Generate_ROTL(random_state->state[1] * 5, 7) * 9;

	random_state->state[2] ^= random_state->state[0];
	random_state->state[3] ^= random_state->state[1];
	random_state->state[1] ^= random_state->state[2];
	random_state->state[0] ^= random_state->state[3];

	random_state->state[2] ^= random_state->state[1] << 17;
	random_state->state[3] = RandomXOShiro256Generate_ROTL(random_state->state[3], 45);

	return result;
#undef RandomXOShiro256Generate_ROTL
}

U8 GetRandomNumberInRange(XOShiro256State* random_state, U8 min, U8 max)
{
	return (RandomXOShiro256Generate(random_state) % (max - min + 1)) + min;
}
