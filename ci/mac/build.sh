#!/usr/bin/env zsh
set -euo pipefail
source /Users/jordan/Projects/myvenv/bin/activate

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
git clone https://github.com/FirestormViewer/fs-build-variables.git
export AUTOBUILD_VARIABLES_FILE="$(pwd)/fs-build-variables/variables"

cd "$CI_PROJECT_DIR"
cd viewer
autobuild installables edit fmodstudio platform=darwin64 hash=2fa4a9dbb2365ef8c4b6acd735ef08e7 url=file://$CI_PROJECT_DIR/viewer/ci/mac/artifacts/fmodstudio-2.02.24-darwin64-242921144.tar.bz2
XZ_DEFAULTS=-T0 autobuild build -A 64 -c ReleaseFS_open -- --fmodstudio --crashreporting --package -DUSE_BUGSPLAT=ON -DBUGSPLAT_DB=$BUGSPLAT_DATABASE