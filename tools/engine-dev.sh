#!/bin/bash
# engine-dev.sh — switch engine submodule between local dev and remote
#
# Usage:
#   ./tools/engine-dev.sh local     # point engine at local repo via git submodule set-url
#   ./tools/engine-dev.sh remote    # restore engine to remote repo
#   ./tools/engine-dev.sh symlink   # replace engine dir with a symlink to local repo
#   ./tools/engine-dev.sh unsymlink # restore engine dir back to a real submodule
#   ./tools/engine-dev.sh status    # show current engine mode
#
# --- local vs symlink: what's the difference? ---
#
# LOCAL (git submodule set-url):
#   Points the submodule at your local repo path instead of GitHub.
#   git submodule update checks out the current *commit* of the local repo.
#   This means:
#     - Uncommitted changes in the engine are NOT visible here.
#     - After new engine commits, you must re-run `git submodule update --remote engine`
#       to pull them in. Without --remote it stays on the previously checked-out commit.
#     - The submodule is left in detached HEAD state (not tracking a branch).
#   Use this when you want a stable snapshot of a local commit for integration testing.
#
# SYMLINK:
#   Replaces the engine/ directory with a symlink to ~/dev/meh-synth-engine.
#   The groovebox sees the engine exactly as it is on disk right now — any file
#   save, any branch, any uncommitted change is instantly visible. No git
#   operations needed between engine edits and groovebox builds.
#   Use this during active development when you're iterating quickly on both
#   projects at the same time.
#
#   Caveat: git will show the submodule as modified while the symlink is active.
#   That's harmless as long as you run `unsymlink` before committing or pushing.

set -e

REMOTE_URL="git@github.com:nico-nunez/meh-synth-engine.git"
LOCAL_PATH="$HOME/dev/meh-synth-engine"
SUBMODULE="engine"

case "$1" in
  local)
    if [ ! -d "$LOCAL_PATH" ]; then
      echo "error: local engine not found at $LOCAL_PATH"
      exit 1
    fi
    echo "switching engine -> local submodule ($LOCAL_PATH)"
    git submodule set-url "$SUBMODULE" "$LOCAL_PATH"
    git submodule update --init "$SUBMODULE"
    echo "done. note: only committed engine changes are visible."
    echo "      run 'git submodule update --remote engine' after new engine commits."
    echo "      remember to restore to remote before pushing."
    ;;

  remote)
    echo "switching engine -> remote ($REMOTE_URL)"
    git submodule set-url "$SUBMODULE" "$REMOTE_URL"
    git submodule update --init "$SUBMODULE"
    echo "done."
    ;;

  symlink)
    if [ ! -d "$LOCAL_PATH" ]; then
      echo "error: local engine not found at $LOCAL_PATH"
      exit 1
    fi
    if [ -L "$SUBMODULE" ]; then
      echo "engine is already a symlink to $(readlink $SUBMODULE)"
      exit 0
    fi
    if [ -d "$SUBMODULE" ]; then
      echo "backing up engine/ -> engine.bak/"
      mv "$SUBMODULE" "${SUBMODULE}.bak"
    fi
    ln -s "$LOCAL_PATH" "$SUBMODULE"
    echo "done. engine/ is now a symlink to $LOCAL_PATH"
    echo "      all engine changes (including uncommitted) are instantly visible."
    echo "      run 'unsymlink' before committing or pushing."
    ;;

  unsymlink)
    if [ ! -L "$SUBMODULE" ]; then
      echo "engine/ is not a symlink, nothing to do."
      exit 0
    fi
    echo "removing symlink..."
    rm "$SUBMODULE"
    if [ -d "${SUBMODULE}.bak" ]; then
      echo "restoring engine/ from engine.bak/"
      mv "${SUBMODULE}.bak" "$SUBMODULE"
    else
      echo "no backup found, reinitializing submodule from remote..."
      git submodule update --init "$SUBMODULE"
    fi
    echo "done. engine/ restored to submodule."
    ;;

  status)
    if [ -L "$SUBMODULE" ]; then
      echo "engine: SYMLINK -> $(readlink $SUBMODULE)"
    else
      current=$(git config --file .git/config submodule.$SUBMODULE.url 2>/dev/null || echo "$REMOTE_URL")
      if [ "$current" = "$LOCAL_PATH" ]; then
        echo "engine: LOCAL submodule ($LOCAL_PATH)"
        echo "        note: only committed changes visible. use 'symlink' for live edits."
      else
        echo "engine: REMOTE ($current)"
      fi
    fi
    ;;

  help|*)
    echo ""
    echo "usage: $0 [command]"
    echo ""
    echo "commands:"
    echo "  symlink    Replace engine/ with a symlink to the local repo."
    echo "             All engine changes — including uncommitted — are instantly"
    echo "             visible to this project. Best for active development."
    echo ""
    echo "  unsymlink  Restore engine/ back to a real submodule directory."
    echo "             Reverts from symlink. Run this before committing or pushing."
    echo ""
    echo "  local      Point the submodule at the local repo via git set-url."
    echo "             Only committed engine changes are visible. After new engine"
    echo "             commits you must run: git submodule update --remote engine"
    echo "             The submodule will be in detached HEAD (not on a branch)."
    echo ""
    echo "  remote     Restore the submodule to the real GitHub remote."
    echo ""
    echo "  status     Show whether engine is symlinked, local, or remote."
    echo ""
    echo "gotchas:"
    echo "  - symlink: git shows the submodule as modified while active. harmless,"
    echo "             but run 'unsymlink' before any commit or push."
    echo "  - local:   detached HEAD means new engine commits won't auto-appear."
    echo "             you must run 'git submodule update --remote engine' manually."
    echo "  - local:   uncommitted engine changes are NOT visible. use 'symlink'"
    echo "             if you need to see dirty working state."
    echo "  - both:    restore to 'remote' before tagging a release — the local"
    echo "             path won't resolve for anyone else pulling the repo."
    echo ""
    if [ "$1" != "help" ]; then
      echo "unknown command: '$1'"
      exit 1
    fi
    ;;
esac
