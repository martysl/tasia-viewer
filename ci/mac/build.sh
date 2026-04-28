#!/usr/bin/env zsh
set -euo pipefail

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

MIRROR_INSTALLABLES="$CI_PROJECT_DIR/viewer/ci/mac/mirror_installables.sh"
if [ -f "$MIRROR_INSTALLABLES" ]; then
  echo "Applying mirrored 3p installables from $MIRROR_INSTALLABLES"
  ARTIFACTS_DIR="$CI_PROJECT_DIR/viewer/ci/mac/artifacts" . "$MIRROR_INSTALLABLES"
fi

XZ_DEFAULTS=-T0 autobuild build -A 64 -c ReleaseFS_open -- --fmodstudio --crashreporting --package -DUSE_BUGSPLAT=ON -DBUGSPLAT_DB="$BUGSPLAT_DATABASE"