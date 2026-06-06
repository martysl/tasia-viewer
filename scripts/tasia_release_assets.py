#!/usr/bin/env python3
"""Generate Tasia release metadata, file-level hotfix packages, and optional FTP uploads.

This script intentionally avoids storing credentials. FTP credentials must be passed
through environment variables by CI.
"""

from __future__ import annotations

import argparse
import fnmatch
import ftplib
import hashlib
import json
import os
import shutil
import tarfile
import tempfile
import zipfile
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable


PLATFORM_PATTERNS = {
    "windows": ["*Windows*.zip", "*windows*.zip"],
    "linux": ["*Linux*.zip", "*linux*.zip", "*.tar.xz", "*.tar.bz2", "*.bz2"],
    "macos-intel": ["*macOS*Intel*.dmg", "*macOS*Intel*.zip"],
    "macos-arm": ["*macOS*ARM*.dmg", "*macOS*ARM*.zip", "*Apple*Silicon*.dmg"],
}


@dataclass
class Asset:
    path: Path
    platform: str
    size: int
    sha256: str


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def discover_assets(root: Path) -> list[Asset]:
    files = [p for p in root.rglob("*") if p.is_file()]
    assets: list[Asset] = []
    for path in files:
        if path.name.endswith((".sha256", ".json")) or "patches" in path.parts:
            continue
        platform = "unknown"
        for candidate, patterns in PLATFORM_PATTERNS.items():
            if any(fnmatch.fnmatch(path.name, pattern) for pattern in patterns):
                platform = candidate
                break
        assets.append(Asset(path=path, platform=platform, size=path.stat().st_size, sha256=sha256_file(path)))
    return sorted(assets, key=lambda a: (a.platform, a.path.name))


def write_checksums(assets: Iterable[Asset], output: Path) -> None:
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("w", encoding="utf-8") as f:
        for asset in assets:
            f.write(f"{asset.sha256}  {asset.path.name}\n")


def write_manifest(assets: list[Asset], output: Path, *, version: str, repo: str, tag: str, base_url: str | None) -> None:
    output.parent.mkdir(parents=True, exist_ok=True)
    release_url = f"https://github.com/{repo}/releases/tag/{tag}" if repo else ""
    download_base = base_url.rstrip("/") if base_url else (f"https://github.com/{repo}/releases/download/{tag}" if repo else "")
    manifest = {
        "format": 1,
        "product": "Tasia Viewer",
        "channel": "Tasia",
        "version": version,
        "tag": tag,
        "release_url": release_url,
        "generated_at": os.environ.get("GITHUB_RUN_ID", "local"),
        "assets": [
            {
                "platform": asset.platform,
                "name": asset.path.name,
                "size": asset.size,
                "sha256": asset.sha256,
                "url": f"{download_base}/{asset.path.name}" if download_base else asset.path.name,
            }
            for asset in assets
        ],
        "patches": [],
    }
    output.write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def find_platform_asset(root: Path, platform: str) -> Asset | None:
    assets = [asset for asset in discover_assets(root) if asset.platform == platform]
    if not assets:
        return None
    # Prefer archives that can be unpacked for file-level updater manifests.
    unpackable = [a for a in assets if a.path.name.lower().endswith((".zip", ".tar.xz", ".tar.bz2", ".tgz", ".tar.gz"))]
    return unpackable[0] if unpackable else assets[0]


def write_file_manifest(root: Path, output: Path, *, platform: str, version: str, base_url: str) -> None:
    output.parent.mkdir(parents=True, exist_ok=True)
    base = base_url.rstrip("/")
    files = []
    for path in sorted(p for p in root.rglob("*") if p.is_file()):
        rel = path.relative_to(root).as_posix()
        files.append(
            {
                "path": rel,
                "size": path.stat().st_size,
                "sha256": sha256_file(path),
                "url": f"{base}/{platform}/files/{rel}",
            }
        )
    manifest = {
        "format": 1,
        "type": "file-manifest",
        "product": "Tasia Viewer",
        "channel": "Tasia",
        "platform": platform,
        "version": version,
        "files_base_url": f"{base}/{platform}/files",
        "files": files,
    }
    output.write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def extract_archive(archive: Path, dest: Path) -> None:
    dest.mkdir(parents=True, exist_ok=True)
    name = archive.name.lower()
    if name.endswith(".zip"):
        with zipfile.ZipFile(archive) as zf:
            zf.extractall(dest)
    elif name.endswith((".tar.xz", ".tar.bz2", ".tgz", ".tar.gz")):
        with tarfile.open(archive) as tf:
            tf.extractall(dest)
    else:
        raise ValueError(f"Unsupported archive for patch generation: {archive}")

    normalize_extracted_tree(dest)


def is_archive(path: Path) -> bool:
    return path.name.lower().endswith((".zip", ".tar.xz", ".tar.bz2", ".tgz", ".tar.gz"))


def normalize_extracted_tree(dest: Path) -> None:
    """Normalize CI artifacts before file comparison.

    GitHub artifact downloads may wrap the real viewer package in an extra zip,
    and Linux packages usually have a versioned top-level directory. Both would
    otherwise make every file look changed between releases.
    """
    # If the archive extracts to exactly one nested archive, unwrap it.
    entries = list(dest.iterdir())
    if len(entries) == 1 and entries[0].is_file() and is_archive(entries[0]):
        nested = entries[0]
        with tempfile.TemporaryDirectory() as td:
            tmp = Path(td)
            extract_archive(nested, tmp)
            shutil.rmtree(dest)
            dest.mkdir(parents=True, exist_ok=True)
            for item in tmp.iterdir():
                shutil.move(str(item), dest / item.name)

    # If the archive extracts to exactly one top-level directory, strip it.
    entries = list(dest.iterdir())
    if len(entries) == 1 and entries[0].is_dir():
        top = entries[0]
        with tempfile.TemporaryDirectory() as td:
            tmp = Path(td)
            for item in top.iterdir():
                shutil.move(str(item), tmp / item.name)
            shutil.rmtree(dest)
            dest.mkdir(parents=True, exist_ok=True)
            for item in tmp.iterdir():
                shutil.move(str(item), dest / item.name)


def file_hashes(root: Path) -> dict[str, str]:
    out: dict[str, str] = {}
    for path in root.rglob("*"):
        if path.is_file():
            out[str(path.relative_to(root)).replace("\\", "/")] = sha256_file(path)
    return out


def create_file_patch(old_archive: Path, new_archive: Path, output: Path) -> dict[str, object]:
    output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory() as td:
        tmp = Path(td)
        old_dir = tmp / "old"
        new_dir = tmp / "new"
        changed_dir = tmp / "changed"
        extract_archive(old_archive, old_dir)
        extract_archive(new_archive, new_dir)
        old_hash = file_hashes(old_dir)
        new_hash = file_hashes(new_dir)
        changed = [rel for rel, h in new_hash.items() if old_hash.get(rel) != h]
        removed = sorted(set(old_hash) - set(new_hash))
        for rel in changed:
            src = new_dir / rel
            dst = changed_dir / rel
            dst.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(src, dst)
        metadata = {
            "format": 1,
            "type": "file-level-hotfix",
            "base": old_archive.name,
            "target": new_archive.name,
            "changed": changed,
            "removed": removed,
        }
        (changed_dir / "tasia_patch_manifest.json").write_text(json.dumps(metadata, indent=2) + "\n", encoding="utf-8")
        with zipfile.ZipFile(output, "w", zipfile.ZIP_DEFLATED) as zf:
            for path in changed_dir.rglob("*"):
                if path.is_file():
                    zf.write(path, path.relative_to(changed_dir).as_posix())
        metadata["patch_name"] = output.name
        metadata["patch_size"] = output.stat().st_size
        metadata["patch_sha256"] = sha256_file(output)
        return metadata


def ensure_ftp_dir(ftp: ftplib.FTP, path: str) -> None:
    for part in path.strip("/").split("/"):
        if not part:
            continue
        try:
            ftp.mkd(part)
        except ftplib.error_perm:
            pass
        ftp.cwd(part)


def upload_ftp(root: Path, *, host: str, user: str, password: str, remote_dir: str, tls: bool) -> list[str]:
    files = [p for p in root.rglob("*") if p.is_file()]
    uploaded: list[str] = []
    ftp_cls = ftplib.FTP_TLS if tls else ftplib.FTP
    with ftp_cls(host, timeout=60) as ftp:
        ftp.login(user=user, passwd=password)
        if tls and isinstance(ftp, ftplib.FTP_TLS):
            ftp.prot_p()
        for path in files:
            rel = path.relative_to(root).as_posix()
            ftp.cwd("/")
            ensure_ftp_dir(ftp, remote_dir)
            parent_path = Path(rel).parent
            parent = "" if str(parent_path) == "." else parent_path.as_posix().strip("/")
            if parent:
                ensure_ftp_dir(ftp, parent)
            with path.open("rb") as f:
                ftp.storbinary(f"STOR {path.name}", f)
            uploaded.append(rel)
    return uploaded


def cmd_manifest(args: argparse.Namespace) -> None:
    assets = discover_assets(Path(args.artifacts))
    write_checksums(assets, Path(args.checksums))
    write_manifest(assets, Path(args.manifest), version=args.version, repo=args.repo, tag=args.tag, base_url=args.base_url)
    print(json.dumps({"assets": [a.path.name for a in assets], "manifest": args.manifest, "checksums": args.checksums}, indent=2))


def cmd_patch(args: argparse.Namespace) -> None:
    old_dir = Path(args.old)
    new_dir = Path(args.new)
    out_dir = Path(args.output)
    out_dir.mkdir(parents=True, exist_ok=True)
    patches = []
    for new_asset in discover_assets(new_dir):
        candidates = [a for a in discover_assets(old_dir) if a.platform == new_asset.platform]
        if not candidates:
            continue
        old_asset = candidates[0]
        if new_asset.path.suffix.lower() not in {".zip", ".xz", ".bz2", ".tgz", ".gz"}:
            continue
        patch_name = f"Tasia-Hotfix-{new_asset.platform}-{args.version}.zip"
        patches.append(create_file_patch(old_asset.path, new_asset.path, out_dir / patch_name))
    print(json.dumps({"patches": patches}, indent=2))


def cmd_unpacked(args: argparse.Namespace) -> None:
    artifacts = Path(args.artifacts)
    output = Path(args.output)
    base_url = args.base_url.rstrip("/")
    platforms = [p.strip() for p in args.platforms.split(",") if p.strip()]
    result = []
    for platform in platforms:
        asset = find_platform_asset(artifacts, platform)
        if not asset:
            continue
        files_dir = output / platform / "files"
        manifest = output / platform / "manifest.json"
        if files_dir.exists():
            shutil.rmtree(files_dir)
        files_dir.mkdir(parents=True, exist_ok=True)
        extract_archive(asset.path, files_dir)
        write_file_manifest(files_dir, manifest, platform=platform, version=args.version, base_url=base_url)
        result.append({"platform": platform, "asset": asset.path.name, "manifest": str(manifest)})
    print(json.dumps({"unpacked": result}, indent=2))


def cmd_ftp(args: argparse.Namespace) -> None:
    required = ["TASIA_FTP_HOST", "TASIA_FTP_USER", "TASIA_FTP_PASSWORD", "TASIA_FTP_REMOTE_DIR"]
    missing = [name for name in required if not os.environ.get(name)]
    if missing:
        raise SystemExit(f"Missing FTP env vars: {', '.join(missing)}")
    uploaded = upload_ftp(
        Path(args.path),
        host=os.environ["TASIA_FTP_HOST"],
        user=os.environ["TASIA_FTP_USER"],
        password=os.environ["TASIA_FTP_PASSWORD"],
        remote_dir=os.environ["TASIA_FTP_REMOTE_DIR"],
        tls=os.environ.get("TASIA_FTP_TLS", "true").lower() != "false",
    )
    print(json.dumps({"uploaded": uploaded}, indent=2))


def main() -> None:
    parser = argparse.ArgumentParser()
    sub = parser.add_subparsers(dest="cmd", required=True)
    m = sub.add_parser("manifest")
    m.add_argument("--artifacts", required=True)
    m.add_argument("--version", required=True)
    m.add_argument("--tag", required=True)
    m.add_argument("--repo", required=True)
    m.add_argument("--manifest", required=True)
    m.add_argument("--checksums", required=True)
    m.add_argument("--base-url")
    m.set_defaults(func=cmd_manifest)

    p = sub.add_parser("patch")
    p.add_argument("--old", required=True)
    p.add_argument("--new", required=True)
    p.add_argument("--output", required=True)
    p.add_argument("--version", required=True)
    p.set_defaults(func=cmd_patch)

    u = sub.add_parser("unpacked")
    u.add_argument("--artifacts", required=True)
    u.add_argument("--output", required=True)
    u.add_argument("--version", required=True)
    u.add_argument("--base-url", required=True)
    u.add_argument("--platforms", default="linux,windows")
    u.set_defaults(func=cmd_unpacked)

    f = sub.add_parser("ftp")
    f.add_argument("--path", required=True)
    f.set_defaults(func=cmd_ftp)

    args = parser.parse_args()
    args.func(args)


if __name__ == "__main__":
    main()
