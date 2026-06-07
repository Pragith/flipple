#!/usr/bin/env python3

import os
import subprocess
import tempfile


def assert_equal(actual, expected, label):
    if actual != expected:
        raise AssertionError(f"{label}: expected {expected}, got {actual}")


def main():
    repo_root = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
    wordlist_path = os.path.join(repo_root, "assets", "wordlist.txt")
    with open(wordlist_path, "r", encoding="utf-8") as handle:
        words = [line.strip() for line in handle if line.strip()]
    assert_equal(len(words), 45, "wordlist length")
    assert_equal(
        all(len(word) == 5 and word.isalpha() and word.isupper() for word in words),
        True,
        "wordlist shape",
    )

    with tempfile.TemporaryDirectory() as tmpdir:
        binary = os.path.join(tmpdir, "flipple_engine_test")
        subprocess.run(
            [
                "cc",
                "-std=c11",
                "-Wall",
                "-Wextra",
                "-Werror",
                "-I.",
                "tests/engine_test.c",
                "flipple_engine.c",
                "-o",
                binary,
            ],
            cwd=repo_root,
            check=True,
        )
        subprocess.run([binary], check=True)

    print("guess_engine_test: ok")


if __name__ == "__main__":
    main()
