#!/usr/bin/env bash
set -euo pipefail

git clone https://github.com/FirestormViewer/fs-build-variables.git
export AUTOBUILD_VARIABLES_FILE="$(pwd)/fs-build-variables/variables"
export XZ_DEFAULTS="-T0"

python3 -m venv "$(pwd)/venv"
source "$(pwd)/venv/bin/activate"

VIEWER_PATH="$(pwd)/viewer"
cd "$VIEWER_PATH"
pip install -r requirements.txt

FMOD_ARCHIVE_PATH="$VIEWER_PATH/ci/linux/artifacts/fmodstudio-2.02.24-linux64-242871432.tar.bz2"
WEBRTC_ARCHIVE_PATH="$VIEWER_PATH/ci/linux/artifacts/webrtc-m114_release.251141020-linux64-251141020.tar.bz2"

if [ ! -f "$FMOD_ARCHIVE_PATH" ]; then
  echo "Error: FMOD archive not found at $FMOD_ARCHIVE_PATH"
  exit 1
fi

if [ ! -f "$WEBRTC_ARCHIVE_PATH" ]; then
  echo "Error: WebRTC archive not found at $WEBRTC_ARCHIVE_PATH"
  exit 1
fi

echo "Setting fmodstudio to file://$FMOD_ARCHIVE_PATH"
autobuild installables edit fmodstudio platform=linux64 hash=c2cb6978b5060fd178be389ca751a7a4 url="file://$FMOD_ARCHIVE_PATH"
echo "Setting webrtc to file://$WEBRTC_ARCHIVE_PATH"
autobuild installables edit webrtc platform=linux64 hash=f94e4dbf13ad3e86e8036e4d24645ab4 url="file://$WEBRTC_ARCHIVE_PATH"
autobuild build -A 64 -c ReleaseFS_open -- --fmodstudio --avx2 --crashreporting --package --ninja -DUSE_BUGSPLAT=ON -DBUGSPLAT_DB="$BUGSPLAT_DATABASE"