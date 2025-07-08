#!/usr/bin/env bash
set -euo pipefail

git clone https://github.com/FirestormViewer/fs-build-variables.git
export AUTOBUILD_VARIABLES_FILE="$(pwd)/fs-build-variables/variables"
export XZ_DEFAULTS="-T0"

python3 -m venv "$(pwd)/venv"
source "$(pwd)/venv/bin/activate"

VIEWER_PATH="$(pwd)/viewer"
cd "$VIEWER_PATH"
echo "Setting fmod to file://$VIEWER_PATH/viewer/ci/linux/artifacts/fmodstudio-2.02.24-linux64-242871432.tar.bz2"
autobuild installables edit fmodstudio platform=linux64 hash=c2cb6978b5060fd178be389ca751a7a4 url="file://$VIEWER_PATH/ci/linux/artifacts/fmodstudio-2.02.24-linux64-242871432.tar.bz2"
autobuild installables edit webrtc platform=linux64 hash=f94e4dbf13ad3e86e8036e4d24645ab4 url="file://$VIEWER_PATH/ci/linux/artifacts/webrtc-m114_release.251141020-linux64-251141020.tar.bz2"
XZ_DEFAULTS=-T0 autobuild build -A 64 -c ReleaseFS_open -- --fmodstudio --avx2 --crashreporting --package --ninja -DUSE_BUGSPLAT=ON -DBUGSPLAT_DB="$BUGSPLAT_DATABASE"