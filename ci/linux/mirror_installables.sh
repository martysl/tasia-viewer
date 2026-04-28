#!/usr/bin/env bash
set -euo pipefail
: "${ARTIFACTS_DIR:?ARTIFACTS_DIR must be set before sourcing mirror_installables.sh}"
case "${ARTIFACTS_DIR}" in
  file://*) _ART_URL="${ARTIFACTS_DIR}" ;;
  /*)       _ART_URL="file://${ARTIFACTS_DIR}" ;;
  *)        _ART_URL="file:///${ARTIFACTS_DIR}" ;;
esac

autobuild installables edit breakpad platform=linux64 hash=482c2b25bbfd25edc058a02f82da39b2 hash_algorithm=md5 url="${_ART_URL}/breakpad-4708e6fb-linux64_bionic-220392253.tar.bz2"
autobuild installables edit dictionaries platform=common hash=2d59bb6f4bd38a18d9250ff010a13b59 hash_algorithm=md5 url="${_ART_URL}/dictionaries-2.46203-common-46203.tar.bz2"
autobuild installables edit discord-rpc platform=linux64 hash=cefa1cdb50a85e36114ca89df66c0b5a hash_algorithm=md5 url="${_ART_URL}/discord_rpc-3.4.0-linux64-192540843.tar.bz2"
autobuild installables edit dullahan platform=linux64 hash=2845d791c0f00392ba1573bc645a0fc8a7fd37ae hash_algorithm=sha1 url="${_ART_URL}/dullahan-1.14.0.202403161609_118.6.8_ge44bee1_chromium-118.0.5993.117-linux64-240760509.tar.zst"
autobuild installables edit glib platform=linux64 hash=1e74b8cc3694150a7751aeb12f3456d414412c0e hash_algorithm=sha1 url="${_ART_URL}/glib-2.64.6.240620740-linux64-240620740.tar.zst"
autobuild installables edit glod platform=linux64 hash=acc1181cd31ef32c3724eda84ae4b580 hash_algorithm=md5 url="${_ART_URL}/glod-1.0pre3.180990827-linux64-180990827.tar.bz2"
autobuild installables edit gntp-growl platform=linux64 hash=af06208ec80b1f170cc560141602e2dc hash_algorithm=md5 url="${_ART_URL}/libnotify-0.4.4-linux-20101003.tar.bz2"
autobuild installables edit gstreamer10 platform=linux64 hash=01f39ecf80dae64e30402ac384035b3e hash_algorithm=md5 url="${_ART_URL}/gstreamer10-1.6.3.201605191852-linux-201605191852.tar.bz2"
autobuild installables edit jemalloc platform=linux64 hash=0008a7b291fa1863ba41424b4e3ac302 hash_algorithm=md5 url="${_ART_URL}/jemalloc-5.3.0-linux64-222631217.tar.bz2"
autobuild installables edit libuuid platform=linux64 hash=f3cc32c84b99f1277370ce88a0faf40e hash_algorithm=md5 url="${_ART_URL}/libuuid-1.6.2-linux64-180841554.tar.bz2"
autobuild installables edit ndPhysicsStub platform=linux64 hash=c266a8d6124fc11e41a82c288f2bf8e4 hash_algorithm=md5 url="${_ART_URL}/ndPhysicsStub-1.202321033-linux64-202321033.tar.bz2"
autobuild installables edit slvoice platform=linux64 hash=db89d5ce9695457cc1e94cf657678f1a hash_algorithm=md5 url="${_ART_URL}/slvoice-3.2-linux64-222532329.tar.bz2"
autobuild installables edit viewer-fonts platform=common hash=52bfa9057a5d34cd841e93624c03a0cdcdf7b12a hash_algorithm=sha1 url="${_ART_URL}/viewer_fonts-1.0.0.242151216-common-242151216.tar.bz2"
