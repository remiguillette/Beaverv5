Brand & Graphic Guidelines

This document outlines the brand identity and graphic standards for the application, as derived from its core CSS stylesheet.

The brand's aesthetic is modern, tech-focused, and built on a "dark mode" first paradigm. The UI is clean, legible, and uses a strong primary accent color for key interactions, balanced by a set of secondary colors for categorical differentiation.

1. Color Palette

The color system is foundational to the brand. It consists of a core dark theme, a single powerful primary accent, and a palette of secondary accents.

Core Palette (Dark Theme)

This is the default theme for the entire application.

Variable

Hex/RGBA

Usage

--bg

#10111a

The main, darkest background for the application body.

--card-bg

#181820

The primary background color for all cards, sections, and modules.

--text-main

#f1f2f8

The default text color for all body copy, titles, and content.

--text-muted

#a8a8bf

For secondary text, hints, labels, and disabled-state text.

--surface-outline

rgba(255, 255, 255, 0.08)

The default border color for cards, buttons, and dividers.

--surface-highlight

rgba(255, 255, 255, 0.06)

Subtle backgrounds for elements like displays, and for hover states.

Accent Palette

This palette brings life and interactivity to the UI.

Variable

Hex

Usage

--accent-amber

#f89422

Primary Brand Color. Used for all key interactive elements, CTAs, links, page titles, and section headers.

--accent-violet

#9d6cff

Secondary accent. Used for categorical hovers (e.g., App Tiles).

--accent-cyan

#22d3ee

Secondary accent. Used for categorical hovers (e.g., App Tiles).

--accent-red

#ff6b6b

Secondary accent. Used for categorical hovers or potential error/danger states.

--accent-green

#47d17b

Secondary accent. Used for categorical hovers or success states.

Status Indicators

The brand uses a specific, accessible system for status indicators.

Status

Background Color

Text Color

OK / Success

rgba(71, 209, 123, 0.16)

#4fe18f

Warning

rgba(248, 148, 34, 0.18)

#fbb36b

Idle / Neutral

rgba(168, 168, 191, 0.16)

var(--text-muted)

2. Typography

Typography is clean, modern, and prioritizes legibility by using a system-native font stack.

Font Family: 'Segoe UI', 'Helvetica Neue', Arial, system-ui, sans-serif

Key Styles & Weights

Semi-Bold (600): This is the default weight for most UI elements, including titles, button text, card names, and dialpad keys. This creates a strong, confident feel.

Normal (400-500): Used for body copy, descriptions, and labels.

Bold (700): Used sparingly, such as for avatar initials.

Common Text Components

Page Titles (.menu-header__title, .system-header__title):

Large, responsive font size (using clamp()).

Weight: 600.

Color: Often var(--accent-amber).

Section Titles (.system-section__title, .dialpad-title):

Style: UPPERCASE

Weight: 600 (implied or explicit)

Tracking: letter-spacing is applied (e.g., 0.12em)

Color: var(--accent-amber) or a slightly muted version.

Muted Text (.menu-status__hint, .dialpad-key__letters):

Smaller font size.

Color: var(--text-muted).

3. Layout & Graphic Guide

The application's graphical language is based on cards, rounded corners, and clear separation.

Cards & Containers

Background: All cards use var(--card-bg).

Border: A standard 1px solid var(--surface-outline) is the default.

Border Radius: Corners are consistently and generously rounded. The standard is 16px or 20px.

Shadows: Subtle shadows (box-shadow: 0 6px 18px rgba(0, 0, 0, 0.22);) are applied to key modules like .dialpad-card to create depth.

Highlight Border: A key visual motif is the use of a 2px top border using the primary accent color (border-top: 2px solid rgba(248, 148, 34, 0.6);). This is seen on .dialpad-card, .system-section, and .system-card.

Buttons & Interactive Elements

Primary CTA (.dialpad-call-button):

Shape: Fully rounded pill (border-radius: 999px).

Fill: Solid var(--accent-amber).

Text: High-contrast dark text (#2f1900).

Effect: A strong, colored shadow (0 8px 20px rgba(255, 122, 24, 0.3)).

Secondary Actions (.dialpad-action, .phone-back-link):

Shape: Rounded (14px) or fully-rounded pill (999px).

Fill: var(--surface-highlight).

Border: 1px solid var(--surface-outline).

Text: var(--text-main).

Hover: Background and border become slightly lighter.

Toggles (.lang-toggle):

Shape: Pill container (border-radius: 16px).

Fill: A transparent, tinted background of the primary accent (rgba(248, 148, 34, 0.14)).

Active State: The active button receives a stronger tinted background (rgba(248, 148, 34, 0.24)) and var(--text-main) text color.

App Tiles (.app-tile):

Shape: Rounded card (border-radius: 16px).

State: A transparent 2px border.

Hover State: The border becomes visible using one of the secondary accent colors (e.g., border-color: var(--accent-violet);).
