#!/usr/bin/env bash
set -euo pipefail
: "${ARTIFACTS_DIR:?ARTIFACTS_DIR must be set before sourcing mirror_installables.sh}"
case "${ARTIFACTS_DIR}" in
  file://*) _ART_URL="${ARTIFACTS_DIR}" ;;
  /*)       _ART_URL="file://${ARTIFACTS_DIR}" ;;
  *)        _ART_URL="file:///${ARTIFACTS_DIR}" ;;
esac

autobuild installables edit dictionaries platform=common hash=2d59bb6f4bd38a18d9250ff010a13b59 hash_algorithm=md5 url="${_ART_URL}/dictionaries-2.46203-common-46203.tar.bz2"
autobuild installables edit discord-rpc platform=darwin64 hash=3bc297a0fa47094bb52d361f80186387 hash_algorithm=md5 url="${_ART_URL}/discord_rpc-3.4.0-darwin64-192522358.tar.bz2"
autobuild installables edit glod platform=darwin64 hash=94fc457c46e1fb94b31251bd4747d10f hash_algorithm=md5 url="${_ART_URL}/glod-1.0pre3.171101143-darwin64-171101143.tar.bz2"
autobuild installables edit gntp-growl platform=darwin hash=33300134846d0f00ac4f31c1a190e3e6 hash_algorithm=md5 url="${_ART_URL}/gntp_growl-1.0-darwin-201505101047-r12.tar.bz2"
autobuild installables edit ndPhysicsStub platform=darwin hash=7d375112b162e32e37262da4a6c0a702 hash_algorithm=md5 url="${_ART_URL}/ndPhysicsStub-1.0-darwin-202330107.tar.bz2"
autobuild installables edit viewer-fonts platform=common hash=52bfa9057a5d34cd841e93624c03a0cdcdf7b12a hash_algorithm=sha1 url="${_ART_URL}/viewer_fonts-1.0.0.242151216-common-242151216.tar.bz2"
