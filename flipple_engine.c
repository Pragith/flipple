#include "flipple_engine.h"

#include <string.h>

void flipple_evaluate_guess(const char* target, const char* guess, FlippleGuessResult* result) {
    bool target_used[FLIPPLE_WORD_LENGTH] = {false};
    result->win = true;
    for(int i = 0; i < FLIPPLE_WORD_LENGTH; i++) {
        if(guess[i] == target[i]) {
            result->states[i] = FLIPPLE_TILE_GREEN;
            target_used[i] = true;
        } else {
            result->win = false;
            result->states[i] = FLIPPLE_TILE_EMPTY;
        }
    }

    for(int i = 0; i < FLIPPLE_WORD_LENGTH; i++) {
        if(result->states[i] == FLIPPLE_TILE_GREEN) continue;

        bool found = false;
        for(int j = 0; j < FLIPPLE_WORD_LENGTH; j++) {
            if(!target_used[j] && guess[i] == target[j]) {
                result->states[i] = FLIPPLE_TILE_YELLOW;
                target_used[j] = true;
                found = true;
                break;
            }
        }

        if(!found) {
            result->states[i] = FLIPPLE_TILE_GRAY;
        }
    }
}

void flipple_apply_round_result(FlippleStats* stats, bool win, uint32_t guess_index) {
    stats->games_played++;
    if(win) {
        stats->wins++;
        stats->current_streak++;
        if(stats->current_streak > stats->best_streak) {
            stats->best_streak = stats->current_streak;
        }
        uint32_t score = FLIPPLE_MAX_GUESSES - guess_index;
        if(score > stats->best_score) {
            stats->best_score = score;
        }
    } else {
        stats->current_streak = 0;
    }
}
