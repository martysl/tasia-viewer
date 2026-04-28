#!/usr/bin/env python3
import argparse
import json
import sys
from pathlib import Path


def to_file_url(artifacts_dir: Path) -> str:
    raw = str(artifacts_dir).replace("\\", "/")
    if raw.startswith("file://"):
        return raw.rstrip("/")
    if raw.startswith("/"):
        return "file://" + raw.rstrip("/")
    return "file:///" + raw.rstrip("/")


def main() -> int:
    p = argparse.ArgumentParser(
        description="Rewrite autobuild.xml URLs in-place to point at locally mirrored archives.")
    p.add_argument("--config-file", required=True, type=Path,
                   help="Path to autobuild.xml.")
    p.add_argument("--manifest", required=True, type=Path,
                   help="Path to mirror_manifest.json produced by mirror_3p_packages.py.")
    p.add_argument("--artifacts-dir", required=True, type=Path,
                   help="Directory containing the mirrored archives. Will be embedded as a file:// URL.")
    p.add_argument("--strict", action="store_true",
                   help="Fail if any manifest entry's old_url is not present or any archive file is missing.")
    args = p.parse_args()

    if not args.config_file.is_file():
        print(f"error: autobuild.xml not found at {args.config_file}", file=sys.stderr)
        return 1
    if not args.manifest.is_file():
        print(f"error: manifest not found at {args.manifest}", file=sys.stderr)
        return 1

    artifacts_dir = args.artifacts_dir.resolve()
    if not artifacts_dir.is_dir():
        print(f"error: artifacts dir not found at {artifacts_dir}", file=sys.stderr)
        return 1

    manifest = json.loads(args.manifest.read_text())
    if not isinstance(manifest, list):
        print(f"error: manifest must be a JSON array (got {type(manifest).__name__})", file=sys.stderr)
        return 1

    base_url = to_file_url(artifacts_dir)
    text = args.config_file.read_text()
    original_len = len(text)

    rewritten = 0
    missing_url = []
    missing_file = []

    for entry in manifest:
        old_url = entry.get("old_url")
        filename = entry.get("filename")
        package = entry.get("package", "?")
        platform = entry.get("platform", "?")
        if not old_url or not filename:
            print(f"!! skipping malformed entry: {entry}", file=sys.stderr)
            continue

        archive = artifacts_dir / filename
        if not archive.is_file():
            missing_file.append((package, platform, str(archive)))
            print(f"!! missing archive: {archive}", file=sys.stderr)
            continue

        new_url = f"{base_url}/{filename}"
        if old_url not in text:
            missing_url.append((package, platform, old_url))
            print(f"!! url not found in autobuild.xml for {package}/{platform}: {old_url}", file=sys.stderr)
            continue

        if old_url == new_url:
            continue

        text = text.replace(old_url, new_url, 1)
        rewritten += 1
        print(f"-- {package} [{platform}]: -> {new_url}")

    if rewritten:
        args.config_file.write_text(text)
        print(f"\nrewrote {rewritten} url(s) in {args.config_file} "
              f"({original_len} -> {len(text)} bytes)")
    else:
        print("\nno changes written")

    if args.strict and (missing_url or missing_file):
        print(f"\nstrict mode: {len(missing_url)} missing url(s), "
              f"{len(missing_file)} missing file(s)", file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
