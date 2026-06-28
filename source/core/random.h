#pragma once

#include "types.h"

typedef U8 SplitMix64State;

SplitMix64State RandomSplitMix64StateCreate(U8 seed);

U8 RandomSplitMix64Generate(SplitMix64State* random_state);


typedef struct {
	U8 state[4];
} XOShiro256State;

XOShiro256State RandomXOShiro256StateCreate(U8 seed);

U8 RandomXOShiro256Generate(XOShiro256State* random_state);

U8 GetRandomNumberInRange(XOShiro256State* random_state, U8 min, U8 max);
