# Tasia Launcher / Updater

Portable Python launcher for Tasia Viewer.

## What it does

- Reads public updater manifests from `https://upload.is-on.click/grid/`
- Compares local files by size + SHA256
- Downloads only changed/missing files
- Backs up replaced files under `tasia_update_backup/`
- Launches the viewer after update

## Manual run

From an installed/unpacked viewer folder:

```bash
python3 /path/to/launcher/tasia_launcher.py --cli
```

With GUI:

```bash
python3 /path/to/launcher/tasia_launcher.py
```

Override install folder:

```bash
python3 tasia_launcher.py --install-dir /path/to/TasiaViewer --platform linux
```

## Windows portable EXE

Build with PyInstaller:

```powershell
python -m pip install pyinstaller
pyinstaller --onefile --windowed --name TasiaLauncher launcher\tasia_launcher.py
```

Output:

```text
dist/TasiaLauncher.exe
```

Put `TasiaLauncher.exe` next to `TasiaOS-Releasex64.exe`.

## Linux portable launcher

You can run directly with Python, or package with PyInstaller:

```bash
python3 -m pip install pyinstaller
pyinstaller --onefile --name tasia-launcher launcher/tasia_launcher.py
```

Put `tasia-launcher` in the unpacked viewer folder.

## Updater endpoints

```text
https://upload.is-on.click/grid/windows/manifest.json
https://upload.is-on.click/grid/linux/manifest.json
```

The manifests point to files under:

```text
https://upload.is-on.click/grid/windows/files/
https://upload.is-on.click/grid/linux/files/
```

## Notes

- The viewer must not be running while updating.
- On Windows, use the launcher instead of launching `TasiaOS-Releasex64.exe` directly.
- First update from an old install may still download the main executable and plugin DLLs.
