SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
git clone https://github.com/FirestormViewer/fs-build-variables.git
export AUTOBUILD_VARIABLES_FILE="$(pwd)/fs-build-variables/variables"
cd viewer
autobuild installables edit fmodstudio platform=linux64 hash=c2cb6978b5060fd178be389ca751a7a4 url=file://$SCRIPT_DIR/artifacts/fmodstudio-2.02.24-linux64-242871432.tar.bz2
autobuild installables edit webrtc platform=linux64 hash=f94e4dbf13ad3e86e8036e4d24645ab4 url=file://$SCRIPT_DIR/artifacts/webrtc-m114_release.251141020-linux64-251141020.tar.bz2
XZ_DEFAULTS=-T0 autobuild build -A 64 -c ReleaseFS_open -- --fmodstudio --avx2 --crashreporting --package --ninja -DUSE_BUGSPLAT=ON -DBUGSPLAT_DB=$BUGSPLAT_DATABASE