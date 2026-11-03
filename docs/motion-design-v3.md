# Motion design V3

V3 replaces only the initial brand-opening layer. Existing post-login transition and all product behaviour remain unchanged.

| Stage | Time | Visual |
|---|---:|---|
| Initialization | 0–240ms | dark scientific grid |
| Pixel laboratory | 240–720ms | original laboratory SVG, stepped construction |
| Intelligence connection | 550–980ms | equipment nodes and resource connection (Variant B) |
| Brand | 880–1200ms | LABSPACE reveal |
| Website reveal | 1250–1850ms | paired curtains open to the real application |

`web/ui/introMotion.js` owns one GSAP timeline. If GSAP cannot load, a CSS fallback removes the layer at the same approximate duration. `sessionStorage` skips repeat visits; `?intro=1` forces replay and `?introVariant=b` enables the narrative variant. Reduced-motion skips immediately.
