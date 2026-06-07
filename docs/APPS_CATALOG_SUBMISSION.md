# Apps Catalog Submission Draft

## Catalog Path

`applications/Games/flipple/manifest.yml`

## Pull Request Title

`Add Flipple word guessing game`

## Pull Request Body

```markdown
Adds Flipple, a compact five-letter word guessing game for Flipper Zero.

Source repository: https://github.com/Pragith/flipple

Validation:
- Built with uFBT
- Ran host-side engine test
- Verified app metadata and file assets
```

## Manifest Template

Update `commit_sha` to the final source repository commit before opening the
Apps Catalog pull request.

```yaml
sourcecode:
  type: git
  location:
    origin: https://github.com/Pragith/flipple.git
    commit_sha: COMMIT_SHA
short_description: A compact word guessing game for Flipper Zero.
description: "@README.md"
changelog: "@docs/changelog.md"
screenshots:
  - screenshots/flipple_main.png
```

## Remaining External Requirement

The screenshot must be captured with qFlipper and committed at
`screenshots/flipple_main.png` before the Apps Catalog pull request is opened.

