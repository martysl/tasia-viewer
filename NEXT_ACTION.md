# Next Action

## Current

Windows must keep `quictls` for older Windows QUIC support. Do not revert Windows to `schannel` unless Mom explicitly accepts losing older-Windows QUIC.

## Next

1. Push current NASM/Strawberry Perl CI fix on `feature/tasia-giphy-welcome-loading-windows`.
2. Start Windows GitHub Actions build.
3. If CI fails, inspect the exact MsQuic/OpenSSL failure:
   - confirm Strawberry Perl is first in `PATH`
   - confirm `NASM_DIR` points to the directory containing `nasm.exe`
   - confirm OpenSSL Configure sees `nasm`
4. After Windows passes, mirror the workflow fix to any release branch/tag flow as needed.

## Blockers

- Waiting for CI verification after pushing this patch.
