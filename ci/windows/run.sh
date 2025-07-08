set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

git clone https://github.com/FirestormViewer/fs-build-variables.git
export AUTOBUILD_VARIABLES_FILE="$(pwd)/fs-build-variables/variables"

cd viewer
pip install -r requirements.txt
git reset --hard $1
git submodule update --init --recursive
autobuild installables edit fmodstudio platform=windows64 hash=f5844bc284eb47cd3e0642175eba80f1 url=file://$SCRIPT_DIR/artifacts/fmodstudio-2.02.23-windows64-242661809.tar.bz2
autobuild configure -A 64 -c ReleaseFS_open -- --fmodstudio --avx2 --crashreporting --package -DUSE_BUGSPLAT=ON -DBUGSPLAT_DB=$BUGSPLAT_DATABASE
XZ_DEFAULTS=-T0 autobuild build -A 64 -c ReleaseFS_open -- --fmodstudio --avx2 --crashreporting --package -DUSE_BUGSPLAT=ON -DBUGSPLAT_DB=$BUGSPLAT_DATABASE

version=$(tr -d '\n' < ./indra/newview/VIEWER_VERSION.txt)
revision=$(tr -d '\n' < ./revision.txt)
VIEWER_VERSION="${version}.${revision}"
npx -y @bugsplat/symbol-upload -d "build-*" -f "**/*.pdb" -a "Firestorm-Releasex64" -v "${VIEWER_VERSION}"
