#!/usr/bin/env bash
set -euo pipefail
: "${ARTIFACTS_DIR:?ARTIFACTS_DIR must be set before sourcing mirror_installables.sh}"
case "${ARTIFACTS_DIR}" in
  file://*) _ART_URL="${ARTIFACTS_DIR}" ;;
  /*)       _ART_URL="file://${ARTIFACTS_DIR}" ;;
  *)        _ART_URL="file:///${ARTIFACTS_DIR}" ;;
esac

autobuild installables edit dictionaries platform=common hash=2d59bb6f4bd38a18d9250ff010a13b59 hash_algorithm=md5 url="${_ART_URL}/dictionaries-2.46203-common-46203.tar.bz2"
autobuild installables edit discord-rpc platform=windows64 hash=f63f07d4c87fbbb698b9039583f08b76 hash_algorithm=md5 url="${_ART_URL}/discord_rpc-3.4.0-windows64-192581529.tar.bz2"
autobuild installables edit glod platform=windows64 hash=e906cf08bfbfbd9d4fc78557e021e7d0 hash_algorithm=md5 url="${_ART_URL}/glod-1.0pre3.vs2017-1906061512-windows64-vs2017-1906061512.tar.bz2"
autobuild installables edit gntp-growl platform=windows64 hash=a333b335104b3d0df14b9be005ef1571 hash_algorithm=md5 url="${_ART_URL}/gntp_growl-1.0-windows64-vs2017-1906061512.tar.bz2"
autobuild installables edit ndPhysicsStub platform=windows64 hash=02f70159e14c7b7213b22a0225508c46 hash_algorithm=md5 url="${_ART_URL}/ndPhysicsStub-1.0-windows64-202121823.tar.bz2"
autobuild installables edit viewer-fonts platform=common hash=52bfa9057a5d34cd841e93624c03a0cdcdf7b12a hash_algorithm=sha1 url="${_ART_URL}/viewer_fonts-1.0.0.242151216-common-242151216.tar.bz2"
