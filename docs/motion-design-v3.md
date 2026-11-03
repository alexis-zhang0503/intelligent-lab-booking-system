# Motion design V3

V3 replaces only the initial brand-opening layer. Existing post-login transition and all product behaviour remain unchanged.

| Stage | Time | Visual |
|---|---:|---|
| Initialization | 0–550ms | white scientific grid |
| Pixel laboratory | 550–2050ms | original laboratory SVG, stepped construction |
| Intelligence connection | 1700–2750ms | equipment nodes and resource connection (Variant B) |
| Brand | 2800–3450ms | LABSPACE reveal |
| Website reveal | 4300–6000ms | paired curtains open to the real application |

`web/ui/introMotion.js` owns one GSAP timeline with a strict six-second duration. If GSAP cannot load, a CSS fallback removes the layer at the same duration. `sessionStorage` skips repeat visits; `?intro=1` forces replay and `?introVariant=b` enables the narrative variant. Reduced-motion skips immediately.
