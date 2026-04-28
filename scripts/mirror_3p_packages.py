#!/usr/bin/env python3
import argparse
import hashlib
import shutil
import sys
import urllib.error
import urllib.request
import xml.etree.ElementTree as ET
from pathlib import Path
from urllib.parse import urlparse

DEFAULT_HOST = "3p.firestormviewer.org"

PLATFORM_TO_CI = {
    "linux":     ["linux"],
    "linux64":   ["linux"],
    "windows":   ["windows"],
    "windows64": ["windows"],
    "darwin":    ["mac"],
    "darwin64":  ["mac"],
    "common":    ["linux", "windows", "mac"],
}

DOWNLOAD_UA = "Mozilla/5.0 (compatible; FirestormMirror/1.0)"
CHUNK = 1 << 20


def parse_llsd_value(elem):
    if elem.tag == "map":
        return parse_llsd_map(elem)
    if elem.tag == "array":
        return [parse_llsd_value(c) for c in elem]
    if elem.tag in ("string", "uri"):
        return elem.text or ""
    if elem.tag == "integer":
        return int(elem.text or "0")
    if elem.tag == "real":
        return float(elem.text or "0")
    if elem.tag == "boolean":
        return (elem.text or "").strip() in ("true", "1")
    return elem.text


def parse_llsd_map(elem):
    out = {}
    children = list(elem)
    i = 0
    while i < len(children):
        c = children[i]
        if c.tag == "key" and i + 1 < len(children):
            out[c.text] = parse_llsd_value(children[i + 1])
            i += 2
        else:
            i += 1
    return out


def load_installables(autobuild_xml):
    tree = ET.parse(autobuild_xml)
    root = tree.getroot()
    if root.tag != "llsd" or len(root) == 0:
        raise SystemExit(f"Unexpected XML structure in {autobuild_xml}")
    top = parse_llsd_value(root[0])
    if not isinstance(top, dict):
        raise SystemExit(f"Expected top-level map in {autobuild_xml}")
    return top.get("installables", {})


def file_hash(path, algo):
    h = hashlib.new(algo)
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(CHUNK), b""):
            h.update(chunk)
    return h.hexdigest()


def download(url, dest, attempts=3):
    req = urllib.request.Request(url, headers={"User-Agent": DOWNLOAD_UA})
    last = None
    for n in range(1, attempts + 1):
        try:
            with urllib.request.urlopen(req, timeout=120) as resp, open(dest, "wb") as out:
                shutil.copyfileobj(resp, out, CHUNK)
            return
        except (urllib.error.URLError, urllib.error.HTTPError, TimeoutError, ConnectionError) as e:
            last = e
            print(f"  attempt {n}/{attempts} failed: {e}", file=sys.stderr)
    raise RuntimeError(f"download failed: {url}: {last}")


def collect_jobs(installables, host_filter):
    jobs = []
    for pkg_name, pkg in installables.items():
        if not isinstance(pkg, dict):
            continue
        platforms = pkg.get("platforms") or {}
        for plat_key, plat in platforms.items():
            if not isinstance(plat, dict):
                continue
            archive = plat.get("archive") or {}
            url = archive.get("url")
            if not url:
                continue
            host = urlparse(url).netloc.lower()
            if host_filter and host_filter.lower() not in host:
                continue
            algo = (archive.get("hash_algorithm") or "md5").lower()
            jobs.append({
                "package": pkg_name,
                "platform": plat_key,
                "url": url,
                "hash": archive.get("hash", "").lower(),
                "hash_algorithm": algo,
                "filename": Path(urlparse(url).path).name,
            })
    return jobs


def ci_dirs_for(platform_key, repo_root):
    return [repo_root / "ci" / s / "artifacts" for s in PLATFORM_TO_CI.get(platform_key, [])]


def write_installables_sh(repo_root, jobs_by_ci):
    for ci_name, jobs in jobs_by_ci.items():
        out = repo_root / "ci" / ci_name / "mirror_installables.sh"
        lines = [
            "#!/usr/bin/env bash",
            "set -euo pipefail",
            ': "${ARTIFACTS_DIR:?ARTIFACTS_DIR must be set before sourcing mirror_installables.sh}"',
            'case "${ARTIFACTS_DIR}" in',
            '  file://*) _ART_URL="${ARTIFACTS_DIR}" ;;',
            '  /*)       _ART_URL="file://${ARTIFACTS_DIR}" ;;',
            '  *)        _ART_URL="file:///${ARTIFACTS_DIR}" ;;',
            "esac",
            "",
        ]
        for job in sorted(jobs, key=lambda j: (j["package"], j["platform"])):
            lines.append(
                f'autobuild installables edit {job["package"]} '
                f'platform={job["platform"]} '
                f'hash={job["hash"]} '
                f'hash_algorithm={job["hash_algorithm"]} '
                f'url="${{_ART_URL}}/{job["filename"]}"'
            )
        lines.append("")
        out.parent.mkdir(parents=True, exist_ok=True)
        out.write_text("\n".join(lines))
        out.chmod(0o755)
        print(f"wrote {out} ({len(jobs)} entries)")


def main():
    p = argparse.ArgumentParser(
        description="Mirror autobuild.xml archives (default: 3p.firestormviewer.org) "
                    "into ci/<linux|windows|mac>/artifacts and regenerate "
                    "ci/<plat>/mirror_installables.sh.")
    p.add_argument("--root", type=Path,
                   default=Path(__file__).resolve().parent.parent,
                   help="Repository root (defaults to parent of scripts/).")
    p.add_argument("--host", default=DEFAULT_HOST,
                   help=f"URL host substring filter (default: {DEFAULT_HOST}). "
                        "Use empty string to mirror every package.")
    p.add_argument("--package", action="append", default=None,
                   help="Restrict to specific package name(s); repeatable.")
    p.add_argument("--platform", action="append", default=None,
                   help="Restrict to specific platform key(s); repeatable.")
    p.add_argument("--force", action="store_true",
                   help="Re-download even if cached file matches the expected hash.")
    p.add_argument("--no-emit-sh", action="store_true",
                   help="Skip generating ci/<plat>/mirror_installables.sh.")
    p.add_argument("--dry-run", action="store_true",
                   help="Print actions without downloading or writing.")
    args = p.parse_args()

    repo_root = args.root.resolve()
    autobuild_xml = repo_root / "autobuild.xml"
    if not autobuild_xml.is_file():
        raise SystemExit(f"autobuild.xml not found at {autobuild_xml}")

    installables = load_installables(autobuild_xml)
    jobs = collect_jobs(installables, args.host)
    if args.package:
        wanted = set(args.package)
        jobs = [j for j in jobs if j["package"] in wanted]
    if args.platform:
        wanted = set(args.platform)
        jobs = [j for j in jobs if j["platform"] in wanted]

    if not jobs:
        print(f"No matching packages (host filter: '{args.host}').")
        return 0

    print(f"{'DRY RUN: ' if args.dry_run else ''}"
          f"Mirroring {len(jobs)} archive(s) (host filter: '{args.host}')")

    jobs_by_ci = {"linux": [], "windows": [], "mac": []}
    failures = []

    for job in jobs:
        targets = ci_dirs_for(job["platform"], repo_root)
        if not targets:
            print(f"!! unknown platform '{job['platform']}' for {job['package']}, skipping")
            continue

        for ci_name in PLATFORM_TO_CI.get(job["platform"], []):
            jobs_by_ci.setdefault(ci_name, []).append(job)

        if args.dry_run:
            for d in targets:
                print(f"   would place {job['filename']} in {d}")
            continue

        for d in targets:
            d.mkdir(parents=True, exist_ok=True)

        primary = targets[0] / job["filename"]
        need_dl = args.force or not primary.exists()
        if not need_dl:
            try:
                actual = file_hash(primary, job["hash_algorithm"])
            except ValueError as e:
                print(f"!! {job['package']}: unsupported hash algorithm '{job['hash_algorithm']}': {e}")
                failures.append((job["package"], str(e)))
                continue
            if actual.lower() != job["hash"]:
                print(f"   cached {primary.name} hash mismatch "
                      f"({actual} != {job['hash']}), redownloading")
                need_dl = True

        if need_dl:
            print(f"-- {job['package']} [{job['platform']}]: {job['url']}")
            tmp = primary.with_suffix(primary.suffix + ".part")
            try:
                download(job["url"], tmp)
            except Exception as e:
                print(f"!! download failed: {e}")
                failures.append((job["package"], str(e)))
                if tmp.exists():
                    tmp.unlink()
                continue
            try:
                actual = file_hash(tmp, job["hash_algorithm"])
            except ValueError as e:
                tmp.unlink()
                print(f"!! {job['package']}: unsupported hash algorithm '{job['hash_algorithm']}': {e}")
                failures.append((job["package"], str(e)))
                continue
            if actual.lower() != job["hash"]:
                tmp.unlink()
                msg = f"hash mismatch: got {actual}, expected {job['hash']}"
                print(f"!! {job['package']}: {msg}")
                failures.append((job["package"], msg))
                continue
            tmp.replace(primary)
            print(f"   ok ({job['hash_algorithm']}={actual})")
        else:
            print(f"== {job['package']} [{job['platform']}]: cached {primary}")

        for extra in targets[1:]:
            mirror = extra / job["filename"]
            if args.force or not mirror.exists():
                shutil.copy2(primary, mirror)
                print(f"   copied -> {mirror}")

    if not args.no_emit_sh and not args.dry_run:
        write_installables_sh(repo_root, jobs_by_ci)

    if failures:
        print(f"\n{len(failures)} failure(s):")
        for name, err in failures:
            print(f"  {name}: {err}")
        return 1

    print("\nDone.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
