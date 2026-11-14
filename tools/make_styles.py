"""Generates the lit sphere (matcap) styles in VolumeStyleRendering/assets/images/styles.

The ray marcher looks styles up by the reflected view vector, so each image is the
shading of one material as a painter would show it on a sphere: a warm lit side, a
cool or tinted shadow, a rim, highlights and, for the shiny ones, a reflected horizon.
Written in sRGB, the renderer linearizes them.

usage: python tools/make_styles.py
"""
import os

import numpy as np
from PIL import Image

SIZE = 512
OUT = os.path.join(os.path.dirname(__file__), "..", "VolumeStyleRendering", "assets", "images", "styles")

# lit, shadow, rim and highlight colors are linear; gloss scales the highlight, mirror the reflected horizon
MATTE = dict(mirror=0.0, sky=(0, 0, 0), ground=(0, 0, 0))
STYLES = {
    # warm ivory with cool blue-gray shadows, a crisp highlight
    "bone": dict(lit=(1.00, 0.86, 0.62), shadow=(0.05, 0.07, 0.14), rim=(0.55, 0.70, 1.00),
                 gloss=1.2, shininess=60, **MATTE),
    # translucent flesh: orange light bleeding into a deep red terminator
    "skin": dict(lit=(1.00, 0.55, 0.35), shadow=(0.20, 0.01, 0.01), rim=(1.00, 0.25, 0.10),
                 gloss=0.35, shininess=14, **MATTE),
    # deep saturated green leaves with golden highlights
    "foliage": dict(lit=(0.02, 0.32, 0.04), shadow=(0.0, 0.025, 0.005), rim=(0.15, 0.55, 0.08),
                    gloss=0.6, shininess=24, highlight=(1.00, 0.90, 0.40), **MATTE),
    # dark brown bark
    "wood": dict(lit=(0.07, 0.035, 0.018), shadow=(0.006, 0.003, 0.002), rim=(0.30, 0.17, 0.09),
                 gloss=0.35, shininess=16, highlight=(1.00, 0.85, 0.65), **MATTE),
    # warm brown earth and pebbles, matte
    "soil": dict(lit=(0.45, 0.28, 0.14), shadow=(0.03, 0.015, 0.008), rim=(0.65, 0.45, 0.25),
                 gloss=0.15, shininess=10, **MATTE),
    # blue-gray glazed ceramic reflecting a pale sky, a sharp highlight
    "ceramic": dict(lit=(0.08, 0.17, 0.36), shadow=(0.01, 0.015, 0.04), rim=(0.40, 0.60, 1.00),
                    gloss=2.0, shininess=150, mirror=0.2, sky=(0.35, 0.50, 0.85), ground=(0.02, 0.03, 0.05)),
    # blood vessels: deep red with blue-white highlights and a cool rim
    "vessel": dict(lit=(0.85, 0.06, 0.04), shadow=(0.10, 0.00, 0.02), rim=(0.40, 0.55, 1.00),
                   gloss=1.4, shininess=50, highlight=(0.75, 0.85, 1.00), **MATTE),
    # brushed steel: dark gray with a blue sky reflection
    "steel": dict(lit=(0.10, 0.12, 0.15), shadow=(0.01, 0.012, 0.015), rim=(0.40, 0.50, 0.70),
                  gloss=1.0, shininess=60, mirror=0.25, sky=(0.30, 0.40, 0.55), ground=(0.02, 0.02, 0.025)),
    # polished brass
    "brass": dict(lit=(0.60, 0.40, 0.08), shadow=(0.05, 0.03, 0.0), rim=(1.00, 0.85, 0.40),
                  gloss=1.6, shininess=90, highlight=(1.00, 0.95, 0.70), mirror=0.7,
                  sky=(1.00, 0.80, 0.35), ground=(0.05, 0.03, 0.005)),
}


def normalize(v):
    return v / np.linalg.norm(v, axis=-1, keepdims=True)


def smoothstep(a, b, x):
    t = np.clip((x - a) / (b - a), 0.0, 1.0)
    return t * t * (3.0 - 2.0 * t)


def render(lit, shadow, rim, gloss, shininess, mirror, sky, ground, highlight=(1.0, 1.0, 1.0)):
    coords = (np.arange(SIZE) + 0.5) / SIZE * 2.0 - 1.0
    x, y = np.meshgrid(coords, -coords)
    r2 = np.minimum(x * x + y * y, 1.0)
    # points outside the disc reuse the silhouette normal so filtering never reads black
    n = normalize(np.stack([x, y, np.sqrt(1.0 - r2)], axis=-1))
    view = np.array([0.0, 0.0, 1.0])
    key = normalize(np.array([-0.55, 0.65, 0.55]))
    lit, shadow, rim, sky, ground, highlight = (np.array(c, dtype=float)
                                                for c in (lit, shadow, rim, sky, ground, highlight))

    # a firm terminator from the lit color into the shadow tint
    t = smoothstep(-0.15, 0.75, n @ key)[..., None]
    color = shadow + (lit - shadow) * t

    # reflected environment: sky above a sharp horizon, ground below
    reflected = 2.0 * n[..., 2:3] * n - view
    horizon = smoothstep(-0.04, 0.04, reflected[..., 1:2] + 0.1)
    color += mirror * (ground + (sky - ground) * horizon)

    # rim light hugging the silhouette, strongest on the shadow side
    fresnel = (1.0 - np.clip(n @ view, 0.0, 1.0)) ** 2.5
    color += 0.7 * rim * (fresnel[..., None] * (1.0 - 0.7 * t))

    # key highlight and a small window highlight from the opposite side
    halfway = normalize(key + view)
    color += gloss * highlight * np.clip(n @ halfway, 0.0, 1.0)[..., None] ** shininess
    second = normalize(normalize(np.array([0.6, 0.35, 0.7])) + view)
    color += 0.35 * gloss * highlight * np.clip(n @ second, 0.0, 1.0)[..., None] ** (shininess * 2)

    rgb = np.clip(color, 0.0, 1.0) ** (1.0 / 2.2)
    rgba = np.concatenate([rgb, np.ones(rgb.shape[:2] + (1,))], axis=-1)
    return Image.fromarray((rgba * 255 + 0.5).astype(np.uint8), "RGBA")


def main():
    os.makedirs(OUT, exist_ok=True)
    for name, params in STYLES.items():
        render(**params).save(os.path.join(OUT, name + ".png"))
        print("wrote", name)


if __name__ == "__main__":
    main()
