# Veridical palette

Every colour in the series. If a value is not here it does not go on a panel.

Contrast ratios are WCAG, computed against the panel ground each colour sits on.
4.5:1 is the threshold for small text, which is what panel labels are.

## Panels

| Role | Hex |
|---|---|
| Dark panel | `#101010` |
| Light panel | `#EDEAE3` |

Flat fills. No gradient, no bevel, no texture, no header bar. nanosvg only handles
simple two-colour linear gradients reliably, so flat is both the intended look and
the safe one.

## Labels

Two sets, because no single value works on both grounds. The best compromise is
`#6F6F6F` at 3.79:1 on black and 4.18:1 on offwhite — it fails on both. Do not try
to share one colour across the variants.

### On the dark panel

| Role | Hex | Contrast |
|---|---|---|
| Module name | `#B4B4B4` | 9.18:1 |
| Control label | `#9A9A9A` | 6.76:1 |
| Range, tick marks, secondary | `#6F6F6F` | 3.79:1 |

`#6F6F6F` is below the small-text threshold. Use it only for marks and range text,
never for anything a user has to read to work the module.

### On the light panel

| Role | Hex | Contrast |
|---|---|---|
| Module name | `#2E2E2E` | 11.30:1 |
| Control label | `#3A3A3A` | 9.47:1 |
| Range, tick marks, secondary | `#5F5F5F` | 5.31:1 |

## Knobs

| Role | Hex |
|---|---|
| Fill | `#8B9A8C` |
| Outline and indicator | `#4F5A50` |

One knob asset serves both panel variants. The outline is what makes that possible:
a flat gray-green disc disappears into the offwhite panel and floats on the black
one. It is structural, not decoration, and must not be dropped for a flatter look.

The attenuverters are the same two colours at a smaller size.

Labels stay neutral gray. Do not tint them green to match the knobs — the knobs
being the only colour on the panel is the whole design.

## Jacks, switch and rules

The jacks and the three-position switch are drawn for this plugin too, for the same
reason as the knobs: one asset has to sit on both grounds. They stay achromatic so
the knobs remain the only colour.

| Role | Hex |
|---|---|
| Jack and switch body | `#242424` |
| Jack and switch outer ring | `#7D7D7D` |
| Jack inner ring | `#3D3D3D` |
| Jack hole | `#0A0A0A` |

The switch handle uses the knob fill and outline, so the moving part reads as a
control rather than as part of the panel.

The one horizontal divider is the only panel-drawn line, and it is the only colour
that differs per variant without being type:

| Role | Dark | Light |
|---|---|---|
| Divider rule | `#3A3A3A` | `#C6C1B6` |

## The adjacency that breaks

`#8B9A8C` knob fill against `#9A9A9A` control label is close to identical in
luminance. A label touching or overlapping a knob on the dark panel will vanish
into it.

Two rules follow, and both are mandatory:

- Labels sit above their widget, never at widget-centre height. Label and knob
  bounding boxes must not overlap.
- The knob keeps its `#4F5A50` outline.

Check this on the rendered PNG, not in the SVG source. Find the label closest to a
knob on the dark variant and confirm it still reads.

## Not in this palette

`#5A7350` is deliberately excluded. It is a gray-green from an unrelated plugin's
palette and using it would create a measurable colour link between two series that
have nothing to do with each other.
