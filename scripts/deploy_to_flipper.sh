#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
APP_NAME="flipple"
DIST_FAP="$ROOT_DIR/dist/${APP_NAME}.fap"
WORDLIST_SRC="$ROOT_DIR/assets/wordlist.txt"

log() {
  printf '[%s] %s\n' "$APP_NAME" "$*"
}

build_if_possible() {
  if command -v ufbt >/dev/null 2>&1; then
    log "ufbt found, building app"
    (cd "$ROOT_DIR" && ufbt)
    return 0
  fi

  if command -v fbt >/dev/null 2>&1; then
    log "fbt found, building app"
    (cd "$ROOT_DIR" && fbt)
    return 0
  fi

  return 1
}

find_flipper_volume() {
  for mount in /Volumes/*; do
    [ -d "$mount" ] || continue
    if [ -d "$mount/ext/apps" ] || [ -d "$mount/apps" ]; then
      printf '%s\n' "$mount"
      return 0
    fi
  done
  return 1
}

open_flipper_app() {
  if [ -d "/Applications/Flipper.app" ]; then
    open -a "/Applications/Flipper.app" >/dev/null 2>&1 || true
    log "opened Flipper.app"
  fi
}

install_fap() {
  local volume="$1"
  local target_dir=""

  if [ -d "$volume/ext/apps/Games" ]; then
    target_dir="$volume/ext/apps/Games"
  elif [ -d "$volume/apps/Games" ]; then
    target_dir="$volume/apps/Games"
  elif [ -d "$volume/apps" ]; then
    target_dir="$volume/apps/Games"
    mkdir -p "$target_dir"
  else
    target_dir="$volume/ext/apps/Games"
    mkdir -p "$target_dir"
  fi

  cp "$DIST_FAP" "$target_dir/${APP_NAME}.fap"
  log "installed to $target_dir/${APP_NAME}.fap"
}

install_wordlist() {
  local volume="$1"
  local assets_dir="$volume/ext/apps_assets/${APP_NAME}"
  mkdir -p "$assets_dir"
  cp "$WORDLIST_SRC" "$assets_dir/wordlist.txt"
  log "installed to $assets_dir/wordlist.txt"
}

main() {
  if [ ! -f "$DIST_FAP" ]; then
    log "missing $DIST_FAP"
    if ! build_if_possible; then
      cat <<EOF
[$APP_NAME] No build toolchain found.
[$APP_NAME] Provide an existing dist/flipple.fap or install ufbt/fbt, then rerun.
EOF
      exit 1
    fi
  fi

  if [ ! -f "$DIST_FAP" ]; then
    log "build finished but $DIST_FAP is still missing"
    exit 1
  fi

  if volume="$(find_flipper_volume)"; then
    log "found Flipper volume at $volume"
    install_fap "$volume"
    if [ -f "$WORDLIST_SRC" ]; then
      install_wordlist "$volume"
    fi
    log "open Flipper Zero -> Apps -> Games -> Flipple to run it"
    exit 0
  fi

  open_flipper_app
  cat <<EOF
[$APP_NAME] No mounted Flipper volume found.
[$APP_NAME] If qFlipper is managing the device, mount the storage on macOS and rerun.
[$APP_NAME] The Flipper desktop app is installed; leave it open and connected to Whid, then mount storage or use its app transfer flow.
[$APP_NAME] Otherwise copy $DIST_FAP manually to the device's Games app folder.
EOF
  exit 1
}

main "$@"
