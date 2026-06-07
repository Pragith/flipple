# Flipple

Flipple is a compact five-letter word guessing game for Flipper Zero.

## Gameplay

- Each row is a horizontal letter reel.
- The framed vertical lane is the current guess.
- Use `Up` and `Down` to choose a row.
- Use `Left` and `Right` to rotate that row.
- Press `OK` to spend a key and test the centered word.
- Hold `Back` or press `Back` to exit the app.
- Solve the word before the key counter reaches zero.

## Feedback

- Filled centered tile: revealed letter.
- Underlined filled tile: correct letter locked in the correct position.
- Unfilled centered tile: untested letter.

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
