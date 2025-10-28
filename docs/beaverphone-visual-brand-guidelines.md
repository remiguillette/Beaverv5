# BeaverPhone Visual & Brand Guidelines

## Overview
BeaverPhone delivers a premium kiosk-style dialing experience within the BeaverKiosk suite. These guidelines summarize the existing product design so designers, engineers, and marketers can produce assets that feel native to BeaverPhone.

## Core Brand Attributes
- **Confident and dependable:** Interface cues emphasise reliability with sturdy blocks, soft glows, and consistent accent usage.
- **Technologically warm:** The amber accent pairs with deep-space neutrals to keep the app inviting yet professional.
- **Operational clarity:** Copy favors short, direct labels ("Call", "Clear") and concise helper text that supports fast task completion.

## Logo & Wordmark
- **Product name:** Always set as “BeaverPhone” (camel-case, capital B and P). Avoid abbreviations like "BPhone" in external materials.
- **Typography treatment:** Use the primary type stack at medium/semibold weights, matching the in-product header styling.
- **Coloring:** Prefer the accent amber (`#f89422`). When placing over dark backgrounds use pure accent; over light backgrounds add a subtle drop shadow or 1px outline in `rgba(0,0,0,0.35)` to preserve legibility.

## Color Palette
| Usage | Token | Hex | Notes |
| --- | --- | --- | --- |
| Primary accent | `--accent-amber` | `#f89422` | Buttons, dividers, emphasis lines. 【F:public/css/styles.css†L1-L74】【F:public/css/styles.css†L240-L336】
| Secondary accent | `--accent-violet` | `#9d6cff` | Secondary tiles, optional highlights. 【F:public/css/styles.css†L1-L74】
| Support accent | `--accent-cyan` | `#22d3ee` | Network or status callouts. 【F:public/css/styles.css†L1-L74】
| Support accent | `--accent-red` | `#ff6b6b` | Error states, destructive actions. 【F:public/css/styles.css†L1-L74】
| Support accent | `--accent-green` | `#47d17b` | Positive confirmations, connection OK. 【F:public/css/styles.css†L1-L74】
| Base background | `--bg` | `#10111a` | Full-page background. 【F:public/css/styles.css†L1-L49】
| Surface / cards | `--card-bg` | `#181820` | Dialpad, card backgrounds. 【F:public/css/styles.css†L1-L49】【F:public/css/styles.css†L240-L336】
| Text primary | `--text-main` | `#f1f2f8` | Body text, dial digits. 【F:public/css/styles.css†L1-L49】【F:public/css/styles.css†L240-L336】
| Text muted | `--text-muted` | `#a8a8bf` | Secondary labels, dialpad letters. 【F:public/css/styles.css†L1-L74】【F:public/css/styles.css†L240-L336】

### Gradients & Glows
- Accent glow for the call button: apply drop shadow `0 8px 20px rgba(255, 122, 24, 0.3)` (hover: `0 12px 28px rgba(255, 122, 24, 0.35)`). 【F:public/css/styles.css†L312-L408】
- Connection indicator dot colors progress from `#ffba6b` (idle) to `#61d836` (connected). 【F:public/css/styles.css†L312-L432】【F:public/css/styles.css†L480-L520】

## Typography
- **Primary font stack:** `'Segoe UI', 'Helvetica Neue', Arial, system-ui, sans-serif` with letter spacing adjustments for headings. 【F:public/css/styles.css†L1-L49】【F:public/css/styles.css†L240-L336】
- **Headings:** Uppercase micro-headings use 0.08–0.18em tracking and amber coloring (e.g., dialpad title at 1.1rem). 【F:public/css/styles.css†L240-L384】
- **Body copy:** 0.9–1rem regular text with neutral color for readability.
- **Numbers:** Dialpad digits at 1.6rem, display at 1.75rem semibold. Maintain fixed-width alignment for clarity. 【F:public/css/styles.css†L312-L384】

## Iconography & Imagery
- **Dial Icon:** Inline SVG sized 26×26 inside the call button. Keep stroke simplified and filled with `currentColor`. 【F:public/css/styles.css†L312-L408】【F:src/ui/html_renderer.cpp†L320-L388】
- **Extension avatars:** Use 42×42 circles. Prefer imagery with transparent backgrounds; otherwise show single-letter initials centered in amber-backed circle. 【F:public/css/styles.css†L408-L480】
- **Status Indicators:** Use pill-shaped badges with subtle borders and the surface highlight background. 【F:public/css/styles.css†L312-L432】

## Layout Principles
- **Structure:** Two-column responsive grid—dialpad card and extension list—collapses via `auto-fit` minmax at 260px. Maintain 2rem gutter. 【F:public/css/styles.css†L240-L384】
- **Card styling:** Rounded 20px corners, 1px borders in `var(--surface-outline)` with top border accent strip `rgba(248, 148, 34, 0.6)` for hero cards. 【F:public/css/styles.css†L240-L384】
- **Spacing:** Use 1–2rem interior padding on primary surfaces; 0.75–1rem gaps within clusters.
- **Language toggle:** Displayed as pill switch anchored to header right, using translucent amber background and 13px font. 【F:public/css/styles.css†L48-L144】【F:src/ui/html_renderer.cpp†L300-L332】

## Interaction Guidance
- **Hover states:** Surfaces shift border color or glow; keep transitions ~0.2s and avoid heavy motion out of respect for reduced-motion preferences. 【F:public/css/styles.css†L48-L384】【F:public/css/styles.css†L480-L520】
- **Focus states:** Outline inputs and primary actions with `rgba(255, 186, 107, 0.8)` and `outline-offset: 2px`. 【F:public/css/styles.css†L312-L408】
- **Disabled states:** Reduce opacity to ~0.55 without altering layout. 【F:public/css/styles.css†L312-L408】

## Voice & Messaging
- **Tone:** Friendly but concise. System messages like connection status should use present-tense statements ("Not connected", "Connection in progress"). 【F:src/ui/html_renderer.cpp†L300-L352】
- **Language support:** Ensure all copy is localized for English (`?lang=en`) and French (`?lang=fr`). Route toggles must update label attributes for accessibility. 【F:src/ui/html_renderer.cpp†L300-L352】

## Accessibility
- Maintain contrast ratios above WCAG AA: amber accent on dark surfaces already exceeds 4.5:1, but verify for any new colors.
- Aria attributes: keep `aria-live="polite"` on the dialpad display and connection indicator for assistive tech parity. 【F:src/ui/html_renderer.cpp†L320-L388】
- Buttons should remain at least 44px square; current dialpad buttons meet this size. 【F:public/css/styles.css†L312-L384】

## Asset Production Checklist
1. Apply the primary accent for call-to-action elements.
2. Use rounded rectangles (14–20px radius) for containers.
3. Keep icons monochrome when embedded in accent pills to leverage `currentColor`.
4. Provide both English and French text layers for exported marketing visuals.
5. Test dark-mode legibility at 50% screen brightness to mirror kiosk lighting.

Following these guidelines will ensure new graphics and communications extend BeaverPhone’s established design language.
