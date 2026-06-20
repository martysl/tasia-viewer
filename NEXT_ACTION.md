# Next Action

## Current

Manual prerelease `v8.0.1.78497` is published:

https://github.com/martysl/tasia-viewer/releases/tag/v8.0.1.78497

Windows must keep `quictls` for older Windows QUIC support. Do not revert Windows to `schannel` unless Mom explicitly accepts losing older-Windows QUIC.

## Next

1. Runtime-test `v8.0.1.78497` Windows clipboard image upload.
2. If Windows runtime test passes, use `v8.0.1.78497` as the current public prerelease.

## Blockers

- None for publishing. Windows runtime verification still needed.
