"""Generates the lit sphere (matcap) styles in VolumeStyleRendering/assets/images/styles.

Each style is a sphere shaded with a key light, a fill light, a rim and a highlight.
The ray marcher looks styles up by the reflected view vector, so the images play the
role of hand painted lit spheres from the style transfer function paper.

usage: python tools/make_styles.py
"""
import os

import numpy as np
from PIL import Image

SIZE = 512
OUT = os.path.join(os.path.dirname(__file__), "..", "VolumeStyleRendering", "assets", "images", "styles")

STYLES = {
    # name: base, shadow tint, rim, highlight, shininess
    "bone": ((0.93, 0.88, 0.76), (0.45, 0.32, 0.22), (1.00, 0.95, 0.85), 0.35, 24),
    "skin": ((0.95, 0.66, 0.52), (0.55, 0.12, 0.10), (1.00, 0.70, 0.55), 0.20, 12),
    "foliage": ((0.52, 0.72, 0.25), (0.10, 0.25, 0.08), (0.85, 0.95, 0.45), 0.15, 8),
    "bark": ((0.55, 0.36, 0.20), (0.18, 0.09, 0.05), (0.80, 0.60, 0.40), 0.10, 10),
    "porcelain": ((0.90, 0.93, 0.97), (0.30, 0.38, 0.55), (1.00, 1.00, 1.00), 0.70, 64),
}


def normalize(v):
    return v / np.linalg.norm(v, axis=-1, keepdims=True)


def render(base, shadow, rim, highlight, shininess):
    coords = (np.arange(SIZE) + 0.5) / SIZE * 2.0 - 1.0
    x, y = np.meshgrid(coords, -coords)
    r2 = np.minimum(x * x + y * y, 1.0)
    # points outside the disc reuse the silhouette normal so filtering never reads black
    n = normalize(np.stack([x, y, np.sqrt(1.0 - r2)], axis=-1))

    key = normalize(np.array([-0.5, 0.6, 0.62]))
    fill = normalize(np.array([0.6, -0.2, 0.75]))
    view = np.array([0.0, 0.0, 1.0])

    base, shadow, rim = (np.array(c) for c in (base, shadow, rim))
    wrap = np.clip((n @ key + 0.25) / 1.25, 0.0, 1.0)[..., None]
    color = shadow + (base - shadow) * wrap
    color += 0.15 * base * np.clip(n @ fill, 0.0, 1.0)[..., None]
    fresnel = (1.0 - np.clip(n @ view, 0.0, 1.0)) ** 3
    color += 0.35 * rim * fresnel[..., None]
    halfway = normalize(key + view)
    color += highlight * np.clip(n @ halfway, 0.0, 1.0)[..., None] ** shininess

    rgb = np.clip(color, 0.0, 1.0) ** (1.0 / 2.2)
    rgba = np.concatenate([rgb, np.ones(rgb.shape[:2] + (1,))], axis=-1)
    return Image.fromarray((rgba * 255 + 0.5).astype(np.uint8), "RGBA")


def main():
    os.makedirs(OUT, exist_ok=True)
    for name, params in STYLES.items():
        render(*params).save(os.path.join(OUT, name + ".png"))
        print("wrote", name)


if __name__ == "__main__":
    main()
