#!/usr/bin/env python3
import io
import subprocess
import sys
from pathlib import Path

from PIL import Image

SIZE = 48


def main() -> int:
    root = Path(__file__).resolve().parent.parent / "assets" / "icons"
    out = root / "png"
    out.mkdir(exist_ok=True)
    names = []
    for svg in sorted((root / "svg").glob("*.svg")):
        png = subprocess.run(["rsvg-convert", "-w", str(SIZE), "-h", str(SIZE), str(svg)],
                             check=True, capture_output=True).stdout
        alpha = Image.open(io.BytesIO(png)).convert("RGBA").getchannel("A")
        white = Image.new("RGBA", alpha.size, (255, 255, 255, 0))
        white.putalpha(alpha)
        white.save(out / (svg.stem + ".png"))
        names.append(svg.stem)
    qrc = ['<RCC>', '  <qresource prefix="/icons">']
    qrc += ['    <file alias="%s.png">png/%s.png</file>' % (n, n) for n in names]
    qrc += ['  </qresource>', '</RCC>', '']
    (root / "icons.qrc").write_text("\n".join(qrc))
    return 0


if __name__ == "__main__":
    sys.exit(main())
