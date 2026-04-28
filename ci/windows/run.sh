set -euo pipefail
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

NDPHYSICSSTUB_ARCHIVE_PATH_POSIX="${SCRIPT_DIR}/artifacts/ndPhysicsStub-1.0-windows64-202121823.tar.bz2"
NDPHYSICSSTUB_ARCHIVE_PATH="${WIN_SCRIPT_DIR}\\artifacts\\ndPhysicsStub-1.0-windows64-202121823.tar.bz2"

if [ ! -f "$NDPHYSICSSTUB_ARCHIVE_PATH_POSIX" ]; then
  echo "Error: ndPhysicsStub archive not found at $NDPHYSICSSTUB_ARCHIVE_PATH"
  exit 1
fi

echo "Setting ndPhysicsStub to file:///${NDPHYSICSSTUB_ARCHIVE_PATH}"
autobuild installables edit ndPhysicsStub platform=windows64 hash=02f70159e14c7b7213b22a0225508c46 url="file:///${NDPHYSICSSTUB_ARCHIVE_PATH}"

MIRROR_INSTALLABLES="${SCRIPT_DIR}/mirror_installables.sh"
if [ -f "$MIRROR_INSTALLABLES" ]; then
  ARTIFACTS_DIR_WIN=$(cygpath -m "${SCRIPT_DIR}/artifacts")
  echo "Applying mirrored 3p installables from $MIRROR_INSTALLABLES (ARTIFACTS_DIR=$ARTIFACTS_DIR_WIN)"
  ARTIFACTS_DIR="$ARTIFACTS_DIR_WIN" . "$MIRROR_INSTALLABLES"
fi

autobuild configure -A 64 -c ReleaseFS_open -- --fmodstudio --avx2 --crashreporting --package -DUSE_BUGSPLAT=ON -DBUGSPLAT_DB="$BUGSPLAT_DATABASE"
XZ_DEFAULTS=-T0 autobuild build -A 64 -c ReleaseFS_open -- --fmodstudio --avx2 --crashreporting --package -DUSE_BUGSPLAT=ON -DBUGSPLAT_DB="$BUGSPLAT_DATABASE"

version=$(tr -d '\n' < ./indra/newview/VIEWER_VERSION.txt)
revision=$(tr -d '\n' < ./revision.txt)
VIEWER_VERSION="${version}.${revision}"
npx -y @bugsplat/symbol-upload@10.1.11 -d "build-*" -f "**/*.pdb" -a "Firestorm-Releasex64" -v "${VIEWER_VERSION}"
