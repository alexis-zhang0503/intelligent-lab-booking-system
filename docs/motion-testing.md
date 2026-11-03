# Motion testing

2026-11-03 local validation: `node --check app.js`, `node --check web/ui/introMotion.js` and `git diff --check` passed. A local desktop browser preview verified the dark SaaS login surface and the forced V3 entry (`?intro=1`) with no console errors.

The existing CommonJS UI test files fail before exercising product behaviour because current `app.js` already has ES-module imports and their VM harness evaluates it as a classic script (`Cannot use import statement outside a module`). This is a pre-existing test-harness incompatibility, not caused by V3 presentation code.

Remaining manual checks before merge: same-tab skip, Variant B (`?intro=1&introVariant=b`), 768px/375px, reduced-motion, authenticated dashboard, and deliberate GSAP CDN unavailability. Business regression checks remain authentication, reservation, usage, administration and agent flows; V3 only changes presentation assets and the opening controller.
