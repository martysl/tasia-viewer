set -euo pipefail

BUGSPLAT_DATABASE="${TASIA_CRASH_REPORT_URL:-${TASIA_BUGSPLAT_DATABASE:-${BUGSPLAT_DATABASE:-}}}"
BUGSPLAT_ARGS=(--crashreporting -DUSE_BUGSPLAT=OFF -DBUGSPLAT_DB=)
if [ -n "$BUGSPLAT_DATABASE" ]; then
  BUGSPLAT_ARGS=(--crashreporting -DUSE_BUGSPLAT=ON -DBUGSPLAT_DB="$BUGSPLAT_DATABASE")
fi
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WIN_SCRIPT_DIR=$(cygpath -w "$SCRIPT_DIR")

git clone https://github.com/FirestormViewer/fs-build-variables.git
export AUTOBUILD_VARIABLES_FILE="$(pwd)/fs-build-variables/variables"

cd viewer
git reset --hard "$1"

python -m venv "$(pwd)/venv"
source "$(pwd)/venv/Scripts/activate"
pip install -r requirements.txt

git submodule update --init --recursive

FMOD_ARCHIVE_PATH_POSIX="${SCRIPT_DIR}/artifacts/fmodstudio-2.02.23-windows64-242661809.tar.bz2"
FMOD_ARCHIVE_PATH="${WIN_SCRIPT_DIR}\\artifacts\\fmodstudio-2.02.23-windows64-242661809.tar.bz2"

if [ ! -f "$FMOD_ARCHIVE_PATH_POSIX" ]; then
  echo "Error: FMOD archive not found at $FMOD_ARCHIVE_PATH"
  exit 1
fi

echo "Setting fmod to file:///${FMOD_ARCHIVE_PATH}"
autobuild installables edit fmodstudio platform=windows64 hash=f5844bc284eb47cd3e0642175eba80f1 url="file:///${FMOD_ARCHIVE_PATH}"

MIRROR_MANIFEST="${SCRIPT_DIR}/mirror_manifest.json"
if [ -f "$MIRROR_MANIFEST" ]; then
  ARTIFACTS_DIR_WIN=$(cygpath -m "${SCRIPT_DIR}/artifacts")
  CONFIG_FILE_WIN=$(cygpath -m "$(pwd)/autobuild.xml")
  MANIFEST_WIN=$(cygpath -m "$MIRROR_MANIFEST")
  SCRIPT_WIN=$(cygpath -m "$(pwd)/scripts/apply_mirror.py")
  echo "Rewriting autobuild.xml urls from $MIRROR_MANIFEST (artifacts=$ARTIFACTS_DIR_WIN)"
  python "$SCRIPT_WIN" \
    --config-file "$CONFIG_FILE_WIN" \
    --manifest "$MANIFEST_WIN" \
    --artifacts-dir "$ARTIFACTS_DIR_WIN" \
    --strict
fi

autobuild configure -A 64 -c ReleaseFS_open -- --fmodstudio --avx2 --package "${BUGSPLAT_ARGS[@]}"
XZ_DEFAULTS=-T0 autobuild build -A 64 -c ReleaseFS_open -- --fmodstudio --avx2 --package "${BUGSPLAT_ARGS[@]}"

version=$(tr -d '\n' < ./indra/newview/VIEWER_VERSION.txt)
revision=$(tr -d '\n' < ./revision.txt)
VIEWER_VERSION="${version}.${revision}"
if [ -n "$BUGSPLAT_DATABASE" ]; then
  npx -y @bugsplat/symbol-upload@10.1.11 -d "build-*" -f "**/*.pdb" -a "Tasia-Releasex64" -v "${VIEWER_VERSION}"
fi
