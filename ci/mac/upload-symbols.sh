version=$(tr -d '\n' < ./viewer/indra/newview/VIEWER_VERSION.txt)
revision=$(tr -d '\n' < ./viewer/revision.txt)
VIEWER_VERSION="${version}.${revision}"

mkdir symbols
cd symbols
cp ../viewer/build-darwin-x86_64/newview/Release/Firestorm.xcarchive.zip .
unzip Firestorm.xcarchive.zip
rm Firestorm.xcarchive.zip
npx -y @bugsplat/symbol-upload@10.1.11  -f "**/*.dSYM" -a "Firestorm-Releasex64" -v "${VIEWER_VERSION}"
