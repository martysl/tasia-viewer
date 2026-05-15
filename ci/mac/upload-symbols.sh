if [ -z "${TASIA_BUGSPLAT_DATABASE:-${BUGSPLAT_DATABASE:-}}" ] || [ -n "${TASIA_CRASH_REPORT_URL:-}" ]; then
  echo "BugSplat database not configured; skipping symbol upload."
  exit 0
fi

version=$(tr -d '\n' < ./viewer/indra/newview/VIEWER_VERSION.txt)
revision=$(tr -d '\n' < ./viewer/revision.txt)
VIEWER_VERSION="${version}.${revision}"

mkdir symbols
cd symbols
cp ../viewer/build-darwin-x86_64/newview/Release/Firestorm.xcarchive.zip .
unzip Firestorm.xcarchive.zip
rm Firestorm.xcarchive.zip
npx -y @bugsplat/symbol-upload@10.1.11  -f "**/*.dSYM" -a "Tasia-Releasex64" -v "${VIEWER_VERSION}"
