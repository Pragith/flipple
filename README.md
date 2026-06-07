# Flipple

Flipple is a compact five-letter word guessing game for Flipper Zero.

## Gameplay

- Pick letters from the on-screen keyboard with the directional pad.
- Press `OK` to enter the selected key.
- Use `Back` to delete the previous letter while playing.
- Hold `Back` to exit the app.
- Submit a guess with the on-screen `OK` key.
- Solve the word in six guesses.

## Feedback

- Filled tile: correct letter in the correct position.
- Underlined tile: correct letter in a different position.
- Dotted tile: letter is not in the answer.

## Data

- Built-in fallback words are compiled into the app.
- `files/wordlist.txt` is bundled as app file assets for catalog builds.
- Scoreboard progress is stored in app data as `stats.txt`.

## Build

Build with uFBT from this directory:

```bash
ufbt
```

Run the host-side engine test:

```bash
cc -std=c11 -Wall -Wextra -Werror -I. tests/engine_test.c flipple_engine.c -o /tmp/flipple_engine_test
/tmp/flipple_engine_test
```

