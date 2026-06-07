#pragma once

#include <stdbool.h>
#include <stdint.h>

#define FLIPPLE_WORD_LENGTH 5
#define FLIPPLE_MAX_GUESSES 6

enum {
    FLIPPLE_TILE_EMPTY = 0,
    FLIPPLE_TILE_GRAY = 1,
    FLIPPLE_TILE_YELLOW = 2,
    FLIPPLE_TILE_GREEN = 3,
};

typedef struct {
    uint32_t games_played;
    uint32_t wins;
    uint32_t current_streak;
    uint32_t best_streak;
    uint32_t best_score;
} FlippleStats;

typedef struct {
    uint8_t states[FLIPPLE_WORD_LENGTH];
    bool win;
} FlippleGuessResult;

void flipple_evaluate_guess(const char* target, const char* guess, FlippleGuessResult* result);
void flipple_apply_round_result(FlippleStats* stats, bool win, uint32_t guess_index);
