version=$(tr -d '\n' < ./viewer/indra/newview/VIEWER_VERSION.txt)
revision=$(tr -d '\n' < ./viewer/revision.txt)
VIEWER_VERSION="${version}.${revision}"
npx -y @bugsplat/symbol-upload -d "viewer/build-*" -f "**/*.sym" -a "Firestorm-Releasex64" -v "${VIEWER_VERSION}"