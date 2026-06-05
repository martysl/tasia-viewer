#!/usr/bin/env python3
"""
Safe Tasia Viewer branding replacement helper.

This script intentionally avoids a global Firestorm -> Tasia replacement.
It only changes controlled, user-facing strings and known support/crash URLs.

Usage:
  python3 scripts/tasia_branding_replace.py --dry-run
  python3 scripts/tasia_branding_replace.py --apply
"""

from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
from typing import Iterable


REPO_ROOT = Path(__file__).resolve().parents[1]
REPORT_PATH = REPO_ROOT / "branding_report.json"


SKIP_DIR_NAMES = {
    ".git",
    ".hg",
    ".svn",
    ".idea",
    ".vscode",
    "__pycache__",
    "build",
    "build-darwin-x86_64",
    "build-linux-x86_64",
    "build-vc170-64",
    "packages",
    "venv",
    "node_modules",
    "artifacts",
    "stage",
}

SKIP_EXTENSIONS = {
    ".a",
    ".app",
    ".bmp",
    ".bz2",
    ".cur",
    ".dll",
    ".dmg",
    ".dylib",
    ".exe",
    ".gif",
    ".icns",
    ".ico",
    ".jar",
    ".jpg",
    ".jpeg",
    ".lib",
    ".nib",
    ".o",
    ".pdb",
    ".png",
    ".so",
    ".tar",
    ".tga",
    ".xz",
    ".zip",
}

TEXT_EXTENSIONS = {
    ".bat",
    ".cmake",
    ".cpp",
    ".css",
    ".h",
    ".hpp",
    ".html",
    ".in",
    ".ini",
    ".json",
    ".llsd",
    ".md",
    ".nsi",
    ".plist",
    ".ps1",
    ".py",
    ".sh",
    ".txt",
    ".xml",
    ".yaml",
    ".yml",
}


# -- Protection placeholders -----------------------------------------------
# These patterns must be preserved (e.g. license headers). We protect them by
# replacing with a placeholder before any broad replacements run, then restore
# them afterwards.
# These license/copyright lines must be preserved as-is.
# Each gets its own unique placeholder to avoid cross-corruption during restore.
PROTECTED_PATTERNS = {
    "Phoenix Firestorm Viewer Source Code": "__PHOENIX_FIRESTORM_VIEWER_SRC__",
    "Phoenix Firestorm Viewer Common Library": "__PHOENIX_FIRESTORM_VIEWER_LIB__",
    "Copyright (C) 2025, The Phoenix Firestorm Project, Inc.": "__PHOENIX_FIRESTORM_CP2025__",
    "Copyright (C) 2014, Nicky Dasmijn": "__PHOENIX_NICKY_COPYRIGHT__",
    "The Phoenix Firestorm Project, Inc.": "__PHOENIX_FIRESTORM_PROJECT_INC__",
    "Developer ID Application: The Phoenix Firestorm Project, Inc.": "__PHOENIX_DEVELOPER_ID__",
    "http://www.firestormviewer.org": "__PHOENIX_FIRESTORM_URL__",
}


# Global safe replacements: URLs/support text and explicit product strings only.
GLOBAL_REPLACEMENTS = [
    # Download URLs → GitHub releases
    ("https://www.firestormviewer.org/downloads", "https://github.com/martysl/tasia-viewer/releases"),
    ("https://www.firestormviewer.org/download", "https://github.com/martysl/tasia-viewer/releases"),
    # Support URLs → easierit
    ("https://www.firestormviewer.org/support/", "https://support.easierit.org"),
    ("https://www.firestormviewer.org/support", "https://support.easierit.org"),
    ("http://www.firestormviewer.org/support", "https://support.easierit.org"),
    ("www.firestormviewer.org/support", "support.easierit.org"),
    # Wiki → let-us.cyou
    ("https://wiki.firestormviewer.org/", "https://i.let-us.cyou/"),
    ("http://wiki.firestormviewer.org/", "https://i.let-us.cyou/"),
    ("wiki.firestormviewer.org", "i.let-us.cyou"),
    # Bug tracker → email
    ("https://phoenixviewer.com/app/file-a-jira/?environment=[ENVIRONMENT]", "mailto:009daw+viewersupport@gmail.com?subject=Tasia%20Viewer%20Bug%20Report&body=Environment:%20[ENVIRONMENT]%0ALocation:%20[LOCATION]%0A%0ADescribe%20the%20issue:%0A"),
    ("https://phoenixviewer.com/app/file-a-jira/", "mailto:009daw+viewersupport@gmail.com"),
    # Email
    ("support@secondlife.com", "009daw+viewersupport@gmail.com"),
    # Product names
    ("Firestorm-Releasex64", "Tasia"),
    ("Firestorm Viewer", "Tasia Viewer"),
    ("Firestorm viewer", "Tasia Viewer"),
    ("Firestorm has crashed", "Tasia Viewer has crashed"),
    ("Send crash reports to firestormviewer.org", "Send crash reports to support.easierit.org"),
]


# XML/UI-only replacements. These are visible labels/tooltips, not C++ identifiers.
UI_REPLACEMENTS = [
    ("Firestorm viewer", "Tasia Viewer"),
    ("Firestorm Viewer", "Tasia Viewer"),
]


# Avoid changing these files unless targeted elsewhere.
RISKY_FILE_SUBSTRINGS = {
    "autobuild.xml",
    "LICENSE",
    "licenses",
    "LICENSES",
}

ALLOW_PATH_PREFIXES = (
    "ci/",
    "scripts/",
    "doc/",
    "indra/llcommon/",
    "indra/llcrashlogger/",
    "indra/llui/",
    "indra/linux_crash_logger/",
    "indra/newview/app_settings/",
    "indra/newview/installers/",
    "indra/newview/linux_tools/",
    "indra/newview/skins/",
    "indra/newview/viewer_manifest.py",
    "indra/newview/llappviewer.cpp",
    "indra/newview/llpanelobject.cpp",
    "indra/newview/llversioninfo.cpp",
    "indra/newview/fsversionvalues.h.in",
    "server/",
)


def iter_candidate_files(root: Path) -> Iterable[Path]:
    for dirpath, dirnames, filenames in os.walk(root):
        dirnames[:] = [d for d in dirnames if d not in SKIP_DIR_NAMES]
        current = Path(dirpath)
        for filename in filenames:
            path = current / filename
            if path == Path(__file__).resolve() or path == REPORT_PATH:
                continue
            if path.suffix.lower() in SKIP_EXTENSIONS:
                continue
            if path.suffix.lower() not in TEXT_EXTENSIONS:
                continue
            if any(part in str(path.relative_to(root)) for part in RISKY_FILE_SUBSTRINGS):
                continue
            yield path


def read_text(path: Path) -> str | None:
    try:
        data = path.read_bytes()
    except OSError:
        return None
    if b"\x00" in data:
        return None
    try:
        return data.decode("utf-8")
    except UnicodeDecodeError:
        try:
            return data.decode("utf-8-sig")
        except UnicodeDecodeError:
            return None


def replacements_for(path: Path) -> list[tuple[str, str]]:
    rel = str(path.relative_to(REPO_ROOT)).replace("\\", "/")
    if not rel.startswith(ALLOW_PATH_PREFIXES):
        return []
    replacements = list(GLOBAL_REPLACEMENTS)
    if "/skins/" in rel and path.suffix.lower() == ".xml":
        replacements.extend(UI_REPLACEMENTS)
    return replacements


def process_file(path: Path) -> dict | None:
    raw = read_text(path)
    if raw is None:
        return None

    # Protect license/copyright headers from partial replacement by broad patterns
    applied_placeholders: dict[str, str] = {}  # placeholder → original phrase
    working = raw
    for phrase, placeholder in PROTECTED_PATTERNS.items():
        if phrase in working:
            working = working.replace(phrase, placeholder)
            applied_placeholders[placeholder] = phrase

    updated = working
    hits: list[dict] = []
    for old, new in replacements_for(path):
        count = updated.count(old)
        if count:
            updated = updated.replace(old, new)
            hits.append({"from": old, "to": new, "count": count})

    # Restore protected patterns (reverse order to avoid placeholder collision)
    for placeholder, phrase in applied_placeholders.items():
        updated = updated.replace(placeholder, phrase)

    if updated == raw:
        return None

    return {
        "path": str(path.relative_to(REPO_ROOT)),
        "hits": hits,
        "updated": updated,
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--dry-run", action="store_true", help="report changes without editing files")
    mode.add_argument("--apply", action="store_true", help="apply changes")
    args = parser.parse_args()

    changes = []
    for path in iter_candidate_files(REPO_ROOT):
        result = process_file(path)
        if result is None:
            continue
        changes.append({"path": result["path"], "hits": result["hits"]})
        if args.apply:
            path.write_text(result["updated"], encoding="utf-8", newline="")

    REPORT_PATH.write_text(json.dumps({"applied": args.apply, "changes": changes}, indent=2) + "\n", encoding="utf-8")

    action = "Applied" if args.apply else "Would update"
    print(f"{action} {len(changes)} files")
    print(f"Report: {REPORT_PATH}")
    for item in changes[:50]:
        total = sum(hit["count"] for hit in item["hits"])
        print(f"  {item['path']} ({total} replacements)")
    if len(changes) > 50:
        print(f"  ... {len(changes) - 50} more files")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
