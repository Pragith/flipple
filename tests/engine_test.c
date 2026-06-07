#include "flipple_engine.h"

#include <stdio.h>
#include <string.h>

static int assert_states(const char* label, const uint8_t* actual, const uint8_t* expected) {
    if(memcmp(actual, expected, FLIPPLE_WORD_LENGTH) == 0) return 0;

    printf("%s: expected", label);
    for(size_t i = 0; i < FLIPPLE_WORD_LENGTH; i++) {
        printf(" %u", expected[i]);
    }
    printf(", got");
    for(size_t i = 0; i < FLIPPLE_WORD_LENGTH; i++) {
        printf(" %u", actual[i]);
    }
    printf("\n");
    return 1;
}

static int assert_u32(const char* label, uint32_t actual, uint32_t expected) {
    if(actual == expected) return 0;
    printf("%s: expected %lu, got %lu\n", label, (unsigned long)expected, (unsigned long)actual);
    return 1;
}

int main(void) {
    int failures = 0;
    FlippleGuessResult result = {0};

    const uint8_t exact[] = {3, 3, 3, 3, 3};
    flipple_evaluate_guess("APPLE", "APPLE", &result);
    failures += assert_states("exact match", result.states, exact);
    failures += assert_u32("exact win", result.win, true);

    const uint8_t duplicate[] = {3, 2, 1, 2, 1};
    flipple_evaluate_guess("APPLE", "ALLEY", &result);
    failures += assert_states("duplicate handling", result.states, duplicate);
    failures += assert_u32("duplicate win", result.win, false);

    const uint8_t mixed[] = {1, 2, 1, 2, 2};
    flipple_evaluate_guess("CRANE", "ZEBRA", &result);
    failures += assert_states("mixed feedback", result.states, mixed);
    failures += assert_u32("mixed win", result.win, false);

    FlippleStats stats = {0};
    flipple_apply_round_result(&stats, true, 2);
    failures += assert_u32("games after win", stats.games_played, 1);
    failures += assert_u32("wins after win", stats.wins, 1);
    failures += assert_u32("current streak after win", stats.current_streak, 1);
    failures += assert_u32("best streak after win", stats.best_streak, 1);
    failures += assert_u32("best score after win", stats.best_score, 4);

    flipple_apply_round_result(&stats, false, 5);
    failures += assert_u32("games after loss", stats.games_played, 2);
    failures += assert_u32("current streak after loss", stats.current_streak, 0);
    failures += assert_u32("best score after loss", stats.best_score, 4);

    if(failures) return 1;
    printf("engine_test: ok\n");
    return 0;
}
