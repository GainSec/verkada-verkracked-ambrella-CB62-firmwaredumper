#!/usr/bin/env python3
"""Fail-closed checks for the source-only Verkracked Part 10 release."""

import re
import subprocess
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
README = ROOT / "README.md"

RELATED_URLS = (
    "https://gainsec.com/2026/09/06/verkracked-security-research-on-verkada-anti-crime-devices-part-0/",
    "https://github.com/GainSec/verkada-verkracked-alarm-hub-local-framework",
    "https://gainsec.com/2026/09/06/verkracked-parts-4-5-local-cloud-and-sub-ghz-frameworks-for-verkada-alarm-hubs/",
    "https://github.com/GainSec/verkada-verkracked-subghz-framework",
    "https://github.com/GainSec/verkada-verkracked-bullet-cam-cloud-emulator",
    "https://github.com/GainSec/VerkEye",
    "https://gainsec.com/2026/10/04/verkracked-part-8-verkeye/",
)


def publishable_files():
    result = subprocess.run(
        ["git", "ls-files", "--cached", "--others", "--exclude-standard"],
        cwd=ROOT,
        text=True,
        capture_output=True,
        check=True,
    )
    return tuple(
        ROOT / line for line in result.stdout.splitlines()
        if line and not line.startswith(".git/")
    )


class PublicReleaseTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.readme = README.read_text(encoding="utf-8")
        cls.files = publishable_files()

    def test_part10_identity_and_scope(self):
        self.assertTrue(
            self.readme.startswith(
                "# Verkracked Part 10 — Verkada CB62 Firmware Dumper\n"
            )
        )
        for phrase in (
            "Part 10 of the Verkracked Security Research Project",
            "read-only",
            "lawfully owned",
            "not affiliated with or endorsed by Verkada",
        ):
            self.assertIn(phrase, self.readme)

    def test_related_release_links(self):
        for url in RELATED_URLS:
            self.assertIn(url, self.readme)
        for part in ("Part 0", "Part 4", "Part 5", "Part 7B", "Part 8"):
            self.assertIn(part, self.readme)

    def test_unlicensed_status_is_explicit(self):
        self.assertFalse((ROOT / "LICENSE").exists())
        self.assertFalse((ROOT / "LICENSE.md").exists())
        self.assertIn("No license is currently provided", self.readme)

    def test_profile_and_capability_boundaries(self):
        for profile in (
            "cb62-original-resident",
            "cb62-specimen2-2gb-bld0",
        ):
            self.assertIn(profile, self.readme)
        for phrase in (
            "not proof that every CV22 board is compatible",
            "does not include",
            "repair/write payload",
        ):
            self.assertIn(phrase, self.readme)

    def test_no_standalone_generated_artifacts(self):
        forbidden_suffixes = (".bin", ".elf", ".o", ".dylib", ".so", ".a")
        offenders = []
        for path in self.files:
            rel = path.relative_to(ROOT).as_posix()
            if rel.startswith(("build/", "out/")):
                offenders.append(rel)
            if rel.endswith(forbidden_suffixes) or ".dSYM/" in rel:
                offenders.append(rel)
        self.assertEqual([], sorted(set(offenders)))

    def test_no_private_paths_or_internal_hosts(self):
        patterns = (
            re.compile(b"/" + b"Users/"),
            re.compile(b"/" + rb"home/[A-Za-z0-9_.-]+/"),
            re.compile(b"/" + b"mnt/" + b"SECOND/"),
            re.compile(b"bench" + rb"\.gainpalfam\.com"),
            re.compile(b"git" + rb"\.gainpalfam\.com"),
            re.compile(rb"\bnigel\b", re.IGNORECASE),
        )
        offenders = []
        for path in self.files:
            if not path.is_file():
                continue
            data = path.read_bytes()
            for pattern in patterns:
                if pattern.search(data):
                    offenders.append(
                        f"{path.relative_to(ROOT)}:{pattern.pattern!r}"
                    )
        self.assertEqual([], offenders)

    def test_no_high_risk_payload_sources(self):
        forbidden_names = ("terminal", "shell", "repair", "writer", "write")
        offenders = []
        for path in self.files:
            rel = path.relative_to(ROOT).as_posix().lower()
            if rel.startswith(("src/", "payload/", "tools/")):
                name = path.name.lower()
                if any(word in name for word in forbidden_names):
                    offenders.append(rel)
        self.assertEqual([], offenders)

        source = b"\n".join(
            path.read_bytes()
            for path in self.files
            if path.is_file()
            and path.suffix in {".c", ".h"}
            and path.relative_to(ROOT).parts[0] in {"src", "payload"}
        ).lower()
        for symbol in (
            b"nand_write",
            b"nand_erase",
            b"erase_blocks",
            b"program_pages",
        ):
            self.assertNotIn(symbol, source)

    def test_relative_markdown_links_resolve(self):
        missing = []
        for markdown in (
            path for path in self.files
            if path.is_file() and path.suffix.lower() == ".md"
        ):
            text = markdown.read_text(encoding="utf-8")
            for target in re.findall(r"\[[^\]]+\]\(([^)]+)\)", text):
                target = target.strip()
                if target.startswith(("http://", "https://", "#", "mailto:")):
                    continue
                relative = target.split("#", 1)[0]
                if relative and not (markdown.parent / relative).exists():
                    missing.append(f"{markdown.relative_to(ROOT)} -> {target}")
        self.assertEqual([], missing)

    def test_release_audit_is_in_make_test(self):
        makefile = (ROOT / "Makefile").read_text(encoding="utf-8")
        self.assertIn("python3 tools/test_public_release.py -v", makefile)

    def test_public_release_notes_exist(self):
        self.assertTrue((ROOT / "PUBLIC_RELEASE_NOTES.md").is_file())
        self.assertIn(
            "[Public release notes](PUBLIC_RELEASE_NOTES.md)",
            self.readme,
        )


if __name__ == "__main__":
    unittest.main()
