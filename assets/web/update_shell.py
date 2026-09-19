#!/usr/bin/env python3
"""Update the generated Emscripten HTML shell without rebuilding WASM."""

from pathlib import Path
import re
import sys


def main() -> int:
    output_dir = Path(sys.argv[1]) if len(sys.argv) > 1 else Path("build-web")
    template_path = Path("assets/web/shell.html")
    generated_path = output_dir / "GSSquared.html"

    if not template_path.is_file():
        print(f"Missing shell template: {template_path}", file=sys.stderr)
        return 1
    if not generated_path.is_file():
        print(f"Missing generated HTML: {generated_path}; run buildweb.bat first", file=sys.stderr)
        return 1

    template = template_path.read_text(encoding="utf-8")
    generated = generated_path.read_text(encoding="utf-8")
    script = re.search(
        r'<script\s+async\s+type="text/javascript"\s+src="GSSquared\.js"></script>',
        generated,
    )
    if script is None:
        print(f"Could not find Emscripten script tag in {generated_path}", file=sys.stderr)
        return 1

    updated = template.replace("  {{{ SCRIPT }}}", f"  {script.group(0)}", 1)
    if updated == template:
        print("Shell template is missing the {{{ SCRIPT }}} placeholder", file=sys.stderr)
        return 1

    generated_path.write_text(updated, encoding="utf-8", newline="\n")
    print(f"Updated {generated_path} from {template_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
