npx_app="Tasia-Releasex64"
if [ -z "${TASIA_BUGSPLAT_DATABASE:-${BUGSPLAT_DATABASE:-}}" ] || [ -n "${TASIA_CRASH_REPORT_URL:-}" ]; then
  echo "BugSplat database not configured; skipping symbol upload."
  exit 0
fi

version=$(tr -d '\n' < ./viewer/indra/newview/VIEWER_VERSION.txt)
revision=$(tr -d '\n' < ./viewer/revision.txt)
VIEWER_VERSION="${version}.${revision}"
npx -y @bugsplat/symbol-upload@10.1.11 -d "viewer/build-*" -f "**/*.sym" -a "$npx_app" -v "${VIEWER_VERSION}"
