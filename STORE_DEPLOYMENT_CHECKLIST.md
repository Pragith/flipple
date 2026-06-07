# Flipple Store Deployment Checklist

## Status

Nearly ready for Apps Catalog submission. A qFlipper screenshot is still
required before opening the Apps Catalog pull request.

## Ready

- [x] Minimal source bundle is present.
- [x] Shared guess engine module is split out.
- [x] External wordlist asset is included.
- [x] Local deployment script exists.
- [x] Host-side logic test exists.
- [x] App icon is defined in `application.fam`.
- [x] Catalog README is present.
- [x] Catalog changelog is present.
- [x] Catalog submission draft is present.
- [x] Word list is packaged with `fap_file_assets`.
- [x] Official uFBT SDK build passes.
- [x] App uploads and launches on the connected Flipper Zero via `ufbt launch`.

## Blockers

- [ ] Capture and commit at least one qFlipper screenshot at `screenshots/flipple_main.png`.
- [ ] Open an Apps Catalog pull request with `applications/Games/flipple/manifest.yml`.

## Action Plan

1. Capture a qFlipper screenshot at `screenshots/flipple_main.png`.
2. Commit the screenshot.
3. Add the generated manifest to the Apps Catalog repo at `applications/Games/flipple/manifest.yml`.
4. Open the Apps Catalog pull request using the prepared PR text.

## Notes

- Apps Catalog requires source code in a public GitHub repository, a README,
  changelog, icon, screenshots, and a manifest pointing at a fixed commit SHA.
- Screenshot files must come from qFlipper and should not be hand-edited.
