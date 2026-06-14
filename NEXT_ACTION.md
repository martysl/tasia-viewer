# Next Action

## Current

Manual prerelease `v8.0.1.78484` is published:

https://github.com/martysl/tasia-viewer/releases/tag/v8.0.1.78484

Windows must keep `quictls` for older Windows QUIC support. Do not revert Windows to `schannel` unless Mom explicitly accepts losing older-Windows QUIC.

## Next

1. Runtime-test `v8.0.1.78484` Linux package.
2. Runtime-test `v8.0.1.78484` Windows package.
3. If runtime checks pass, use this as the current public prerelease.

## Blockers

- None for publishing. Runtime verification still needed.
