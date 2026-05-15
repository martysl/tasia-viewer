#!/usr/bin/env zsh
set -euo pipefail

BUGSPLAT_DATABASE="${TASIA_CRASH_REPORT_URL:-${TASIA_BUGSPLAT_DATABASE:-${BUGSPLAT_DATABASE:-}}}"
BUGSPLAT_ARGS=(--crashreporting -DUSE_BUGSPLAT=OFF -DBUGSPLAT_DB=)
if [ -n "$BUGSPLAT_DATABASE" ]; then
  BUGSPLAT_ARGS=(--crashreporting -DUSE_BUGSPLAT=ON -DBUGSPLAT_DB="$BUGSPLAT_DATABASE")
fi

python3 -m venv "$(pwd)/venv"
source "$(pwd)/venv/bin/activate"

git clone https://github.com/FirestormViewer/fs-build-variables.git
export AUTOBUILD_VARIABLES_FILE="$(pwd)/fs-build-variables/variables"

cd "$CI_PROJECT_DIR"
cd viewer
pip install -r requirements.txt

FMOD_ARCHIVE_PATH="$CI_PROJECT_DIR/viewer/ci/mac/artifacts/fmodstudio-2.02.24-darwin64-242921144.tar.bz2"

if [ ! -f "$FMOD_ARCHIVE_PATH" ]; then
  echo "Error: FMOD archive not found at $FMOD_ARCHIVE_PATH"
  exit 1
fi

echo "Installing fmodstudio: file://${FMOD_ARCHIVE_PATH}"
autobuild installables edit fmodstudio platform=darwin64 hash=2fa4a9dbb2365ef8c4b6acd735ef08e7 url="file://${FMOD_ARCHIVE_PATH}"

MIRROR_MANIFEST="$CI_PROJECT_DIR/viewer/ci/mac/mirror_manifest.json"
if [ -f "$MIRROR_MANIFEST" ]; then
  echo "Rewriting autobuild.xml urls from $MIRROR_MANIFEST"
  python3 "$CI_PROJECT_DIR/viewer/scripts/apply_mirror.py" \
    --config-file "$CI_PROJECT_DIR/viewer/autobuild.xml" \
    --manifest "$MIRROR_MANIFEST" \
    --artifacts-dir "$CI_PROJECT_DIR/viewer/ci/mac/artifacts" \
    --strict
fi

XZ_DEFAULTS=-T0 autobuild build -A 64 -c ReleaseFS_open -- --fmodstudio --package "${BUGSPLAT_ARGS[@]}"
