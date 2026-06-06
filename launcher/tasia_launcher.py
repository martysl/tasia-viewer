#!/usr/bin/env python3
"""Tasia Viewer portable launcher/updater.

First-version goals:
- Cross-platform Python launcher (Windows/Linux)
- Reads public per-file manifests from https://upload.is-on.click/grid/
- Downloads only missing/changed files by sha256
- Backs up replaced files
- Launches the viewer after update

Windows packaging target: PyInstaller one-file exe.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import platform
import queue
import shutil
import stat
import subprocess
import sys
import tempfile
import threading
import time
import urllib.request
from dataclasses import dataclass
from pathlib import Path
from tkinter import BOTH, DISABLED, END, NORMAL, Button, Frame, Label, Tk, filedialog, messagebox, scrolledtext


DEFAULT_BASE_URL = "https://upload.is-on.click/grid"
MANIFEST_NAMES = {
    "windows": "windows/manifest.json",
    "linux": "linux/manifest.json",
}
VIEWER_CANDIDATES = {
    "windows": ["TasiaOS-Releasex64.exe", "TasiaViewer.exe", "Firestorm.exe"],
    "linux": ["firestorm", "tasia", "Tasia", "bin/do-not-directly-run-firestorm-bin"],
}


@dataclass
class RemoteFile:
    path: str
    size: int
    sha256: str
    url: str


def detect_platform() -> str:
    system = platform.system().lower()
    if system.startswith("win"):
        return "windows"
    if system.startswith("linux"):
        return "linux"
    raise RuntimeError(f"Unsupported platform for updater: {platform.system()}")


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def fetch_json(url: str) -> dict:
    req = urllib.request.Request(url, headers={"User-Agent": "TasiaLauncher/1.0"})
    with urllib.request.urlopen(req, timeout=60) as response:
        return json.loads(response.read().decode("utf-8"))


def download_file(url: str, destination: Path, progress=None) -> None:
    destination.parent.mkdir(parents=True, exist_ok=True)
    tmp = destination.with_suffix(destination.suffix + ".download")
    req = urllib.request.Request(url, headers={"User-Agent": "TasiaLauncher/1.0"})
    with urllib.request.urlopen(req, timeout=120) as response, tmp.open("wb") as out:
        total = int(response.headers.get("Content-Length") or 0)
        done = 0
        while True:
            chunk = response.read(1024 * 1024)
            if not chunk:
                break
            out.write(chunk)
            done += len(chunk)
            if progress:
                progress(done, total)
    tmp.replace(destination)


def load_manifest(base_url: str, target_platform: str) -> tuple[str, list[RemoteFile]]:
    manifest_url = f"{base_url.rstrip('/')}/{MANIFEST_NAMES[target_platform]}"
    manifest = fetch_json(manifest_url)
    files = [RemoteFile(path=f["path"], size=int(f["size"]), sha256=f["sha256"], url=f["url"]) for f in manifest.get("files", [])]
    return manifest.get("version", "unknown"), files


def find_changed_files(install_dir: Path, files: list[RemoteFile], log) -> list[RemoteFile]:
    changed: list[RemoteFile] = []
    for index, remote in enumerate(files, 1):
        local = install_dir / remote.path
        if not local.exists():
            changed.append(remote)
        else:
            try:
                if local.stat().st_size != remote.size or sha256_file(local).lower() != remote.sha256.lower():
                    changed.append(remote)
            except OSError:
                changed.append(remote)
        if index % 1000 == 0:
            log(f"Checked {index}/{len(files)} files...")
    return changed


def backup_file(path: Path, backup_path: Path) -> None:
    if not path.exists():
        return
    backup_path.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(path, backup_path)


def apply_updates(install_dir: Path, files: list[RemoteFile], log) -> None:
    backup_root = install_dir / "tasia_update_backup" / time.strftime("%Y%m%d-%H%M%S")
    with tempfile.TemporaryDirectory(prefix="tasia-update-") as td:
        temp_root = Path(td)
        for index, remote in enumerate(files, 1):
            log(f"Downloading {index}/{len(files)}: {remote.path}")
            temp_file = temp_root / remote.path
            download_file(remote.url, temp_file)
            got_hash = sha256_file(temp_file).lower()
            if got_hash != remote.sha256.lower():
                raise RuntimeError(f"SHA256 mismatch for {remote.path}")

        for index, remote in enumerate(files, 1):
            local = install_dir / remote.path
            staged = temp_root / remote.path
            log(f"Installing {index}/{len(files)}: {remote.path}")
            if local.exists():
                backup_file(local, backup_root / remote.path)
            local.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(staged, local)
            if detect_platform() == "linux" and ("/" not in remote.path or remote.path.startswith("bin/")):
                try:
                    local.chmod(local.stat().st_mode | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH)
                except OSError:
                    pass


def find_viewer_executable(install_dir: Path, target_platform: str) -> Path | None:
    for candidate in VIEWER_CANDIDATES[target_platform]:
        path = install_dir / candidate
        if path.exists():
            return path
    return None


def launch_viewer(install_dir: Path, target_platform: str) -> None:
    exe = find_viewer_executable(install_dir, target_platform)
    if not exe:
        raise RuntimeError(f"Viewer executable not found in {install_dir}")
    if target_platform == "linux":
        subprocess.Popen([str(exe)], cwd=str(install_dir), start_new_session=True)
    else:
        subprocess.Popen([str(exe)], cwd=str(install_dir))


class TasiaLauncherApp:
    def __init__(self, root: Tk, install_dir: Path, base_url: str, target_platform: str):
        self.root = root
        self.install_dir = install_dir
        self.base_url = base_url
        self.target_platform = target_platform
        self.queue: queue.Queue[str] = queue.Queue()
        self.busy = False

        root.title("Tasia Launcher")
        root.geometry("760x520")

        Label(root, text="💚 Tasia Viewer Launcher", font=("Arial", 18, "bold")).pack(pady=10)
        self.info = Label(root, text=f"Platform: {target_platform} | Folder: {install_dir}")
        self.info.pack(pady=4)

        buttons = Frame(root)
        buttons.pack(pady=8)
        self.update_button = Button(buttons, text="Check & Update", command=self.check_update)
        self.update_button.pack(side="left", padx=6)
        self.launch_button = Button(buttons, text="Launch Viewer", command=self.launch)
        self.launch_button.pack(side="left", padx=6)
        self.folder_button = Button(buttons, text="Choose Folder", command=self.choose_folder)
        self.folder_button.pack(side="left", padx=6)

        self.log_box = scrolledtext.ScrolledText(root, height=24)
        self.log_box.pack(fill=BOTH, expand=True, padx=12, pady=8)

        self.root.after(100, self.drain_queue)

    def log(self, text: str) -> None:
        self.queue.put(text)

    def drain_queue(self) -> None:
        while True:
            try:
                text = self.queue.get_nowait()
            except queue.Empty:
                break
            self.log_box.insert(END, text + "\n")
            self.log_box.see(END)
        self.root.after(100, self.drain_queue)

    def set_busy(self, busy: bool) -> None:
        self.busy = busy
        state = DISABLED if busy else NORMAL
        self.update_button.config(state=state)
        self.launch_button.config(state=state)
        self.folder_button.config(state=state)

    def choose_folder(self) -> None:
        folder = filedialog.askdirectory(initialdir=str(self.install_dir))
        if folder:
            self.install_dir = Path(folder)
            self.info.config(text=f"Platform: {self.target_platform} | Folder: {self.install_dir}")

    def check_update(self) -> None:
        if self.busy:
            return
        self.set_busy(True)
        threading.Thread(target=self._check_update_worker, daemon=True).start()

    def _check_update_worker(self) -> None:
        try:
            self.log("Fetching manifest...")
            version, files = load_manifest(self.base_url, self.target_platform)
            self.log(f"Remote version: {version}; files: {len(files)}")
            changed = find_changed_files(self.install_dir, files, self.log)
            self.log(f"Changed/missing files: {len(changed)}")
            if not changed:
                self.log("Viewer is up to date.")
                return
            apply_updates(self.install_dir, changed, self.log)
            self.log("Update complete ✅")
        except Exception as exc:
            self.log(f"ERROR: {exc}")
            messagebox.showerror("Tasia Launcher", str(exc))
        finally:
            self.root.after(0, lambda: self.set_busy(False))

    def launch(self) -> None:
        try:
            launch_viewer(self.install_dir, self.target_platform)
            self.root.after(500, self.root.destroy)
        except Exception as exc:
            messagebox.showerror("Tasia Launcher", str(exc))


def main() -> int:
    parser = argparse.ArgumentParser(description="Tasia Viewer launcher/updater")
    parser.add_argument("--base-url", default=os.environ.get("TASIA_UPDATE_BASE_URL", DEFAULT_BASE_URL))
    parser.add_argument("--install-dir", default=os.environ.get("TASIA_INSTALL_DIR", str(Path.cwd())))
    parser.add_argument("--platform", choices=["windows", "linux"], default=None)
    parser.add_argument("--cli", action="store_true", help="Run update check without GUI")
    parser.add_argument("--check-only", action="store_true", help="Only check changed files; do not download/apply updates")
    parser.add_argument("--launch", action="store_true", help="Launch viewer after successful update")
    args = parser.parse_args()

    target_platform = args.platform or detect_platform()
    install_dir = Path(args.install_dir).resolve()

    if args.cli:
        print(f"Tasia Launcher | platform={target_platform} | install_dir={install_dir}")
        version, files = load_manifest(args.base_url, target_platform)
        print(f"Remote version: {version}; files: {len(files)}")
        changed = find_changed_files(install_dir, files, print)
        print(f"Changed/missing files: {len(changed)}")
        if args.check_only:
            for remote in changed[:50]:
                print(f"changed: {remote.path}")
            if len(changed) > 50:
                print(f"... and {len(changed) - 50} more")
        elif changed:
            apply_updates(install_dir, changed, print)
            print("Update complete")
        if args.launch:
            launch_viewer(install_dir, target_platform)
        return 0

    root = Tk()
    TasiaLauncherApp(root, install_dir, args.base_url, target_platform)
    root.mainloop()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
