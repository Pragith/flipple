#!/usr/bin/env python3

import os

WORD_LENGTH = 5
TILE_EMPTY = 0
TILE_GRAY = 1
TILE_YELLOW = 2
TILE_GREEN = 3


def evaluate_guess(target: str, guess: str):
    states = [TILE_EMPTY] * WORD_LENGTH
    target_used = [False] * WORD_LENGTH
    win = True

    for i in range(WORD_LENGTH):
        if guess[i] == target[i]:
            states[i] = TILE_GREEN
            target_used[i] = True
        else:
            win = False

    for i in range(WORD_LENGTH):
        if states[i] == TILE_GREEN:
            continue

        found = False
        for j in range(WORD_LENGTH):
            if not target_used[j] and guess[i] == target[j]:
                states[i] = TILE_YELLOW
                target_used[j] = True
                found = True
                break
        if not found:
            states[i] = TILE_GRAY

    return states, win


def apply_round_result(stats, win, guess_index):
    stats["games"] += 1
    if win:
        stats["wins"] += 1
        stats["current"] += 1
        stats["best"] = max(stats["best"], stats["current"])
        score = 6 - guess_index
        stats["score"] = max(stats["score"], score)
    else:
        stats["current"] = 0


def assert_equal(actual, expected, label):
    if actual != expected:
        raise AssertionError(f"{label}: expected {expected}, got {actual}")


def main():
    states, win = evaluate_guess("APPLE", "APPLE")
    assert_equal(states, [3, 3, 3, 3, 3], "exact match states")
    assert_equal(win, True, "exact match win")

    states, win = evaluate_guess("APPLE", "ALLEY")
    assert_equal(states, [3, 2, 1, 2, 1], "duplicate-handling states")
    assert_equal(win, False, "duplicate-handling win")

    states, win = evaluate_guess("CRANE", "ZEBRA")
    assert_equal(states, [1, 2, 1, 2, 2], "mixed feedback states")
    assert_equal(win, False, "mixed feedback win")

    stats = {"games": 0, "wins": 0, "current": 0, "best": 0, "score": 0}
    apply_round_result(stats, True, 2)
    assert_equal(stats, {"games": 1, "wins": 1, "current": 1, "best": 1, "score": 4}, "win stats")
    apply_round_result(stats, False, 5)
    assert_equal(stats, {"games": 2, "wins": 1, "current": 0, "best": 1, "score": 4}, "loss stats")

    wordlist_path = os.path.join(os.path.dirname(__file__), "..", "assets", "wordlist.txt")
    with open(wordlist_path, "r", encoding="utf-8") as handle:
        words = [line.strip() for line in handle if line.strip()]
    assert_equal(len(words), 45, "wordlist length")
    assert_equal(all(len(word) == 5 and word.isalpha() and word.isupper() for word in words), True, "wordlist shape")

    print("guess_engine_test: ok")


if __name__ == "__main__":
    main()
