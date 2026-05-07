#!/bin/bash
set -euo pipefail

LUALS="${LUALS_BIN:-}"
if [ -z "$LUALS" ]; then
  LUALS="$(command -v lua-language-server || true)"
fi

if [ -z "$LUALS" ]; then
  echo "LuaLS not found; skipping LuaLS fixture validation"
  exit 0
fi

if [ ! -x "$LUALS" ]; then
  echo "LuaLS binary is not executable: $LUALS" >&2
  exit 1
fi

TMP_DIR="$(mktemp -d)"
cleanup() {
  rm -rf "$TMP_DIR"
}
trap cleanup EXIT

run_luals_file() {
  local root="$1"
  local file="$2"
  local library="$3"
  local log="$4"
  local workspace="$TMP_DIR/workspace-$(basename "$root")-${file%.lua}"
  local abs_luals="$LUALS"

  case "$abs_luals" in
    /*) ;;
    *) abs_luals="$(command -v "$abs_luals")" ;;
  esac

  mkdir -p "$workspace"
  cp "$root/$file" "$workspace/$file"

  cat >"$workspace/.luarc.json" <<EOF
{
  "workspace.library": ["$library"],
  "runtime.version": "Lua 5.4",
  "diagnostics.globals": [],
  "diagnostics.disable": []
}
EOF

  "$abs_luals" \
    --check="$workspace" \
    --checklevel=Warning \
    --logpath="$TMP_DIR/log" \
    >"$log" 2>&1
}

expect_clean() {
  local root="$1"
  local file="$2"
  local library="$3"
  local log="$TMP_DIR/$(basename "$root").$file.valid.log"

  if ! run_luals_file "$root" "$file" "$library" "$log"; then
    echo "LuaLS failed for $root/$file" >&2
    cat "$log" >&2
    exit 1
  fi

  if grep -E "undefined-global|Undefined global" "$log" >/dev/null; then
    echo "LuaLS reported undefined globals for valid fixture: $root/$file" >&2
    cat "$log" >&2
    exit 1
  fi
}

expect_undefined_global() {
  local root="$1"
  local file="$2"
  local library="$3"
  local expected="$4"
  local log="$TMP_DIR/$(basename "$root").$file.invalid.$expected.log"

  run_luals_file "$root" "$file" "$library" "$log" || true

  if ! grep -E "undefined-global|Undefined global" "$log" >/dev/null; then
    echo "LuaLS did not report an undefined global for invalid fixture: $root/$file" >&2
    cat "$log" >&2
    exit 1
  fi

  if ! grep -F "$expected" "$log" >/dev/null; then
    echo "LuaLS undefined-global output did not mention expected symbol: $expected" >&2
    cat "$log" >&2
    exit 1
  fi
}

AUTHORED_LIBRARY="$PWD/generated/luals/authored_document"
RUNTIME_LIBRARY="$PWD/generated/luals/runtime_lua"

expect_clean \
  "tests/luals/authored_document" \
  "valid_synth_doc.lua" \
  "$AUTHORED_LIBRARY"
expect_clean \
  "tests/luals/authored_document" \
  "valid_sequencer_doc.lua" \
  "$AUTHORED_LIBRARY"

expect_clean \
  "tests/luals/runtime_lua" \
  "valid_runtime_lua.lua" \
  "$RUNTIME_LIBRARY"

expect_undefined_global \
  "tests/luals/authored_document" \
  "invalid_runtime_symbol.lua" \
  "$AUTHORED_LIBRARY" \
  "applyFile"
expect_undefined_global \
  "tests/luals/runtime_lua" \
  "invalid_document_symbol.lua" \
  "$RUNTIME_LIBRARY" \
  "TrackSettings"

echo "LuaLS fixture validation passed"

