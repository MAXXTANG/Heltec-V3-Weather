# Third-party notices

The MIT license at the repository root covers original project code, procedural
seven-segment drawing, original weather-icon geometry, and project documentation.
It does not replace the licenses of the materials below.

## Bundled font

- `third_party/fonts/BarlowCondensed-Bold.ttf`: Barlow Condensed, Copyright 2017
  The Barlow Project Authors, [SIL Open Font License 1.1](third_party/fonts/OFL.txt).
- Source: [Google Fonts / Barlow Condensed](https://github.com/google/fonts/tree/main/ofl/barlowcondensed),
  upstream [jpt/barlow](https://github.com/jpt/barlow).
- `src/ui_fonts.h` is a format-converted derivative of that font; the generated
  glyph tables remain under OFL 1.1. The exporter and procedural seven-segment digit
  implementation are original MIT code. Keep the bundled OFL notice when sharing.
- No macOS DIN, Hiragino or other system font binaries/embedded glyph tables are
  included in this public source release.

## Dependencies installed during build

- Arduino-ESP32 2.0.14: LGPL-2.1-or-later; its SDK components carry their own
  notices. [Upstream](https://github.com/espressif/arduino-esp32/tree/2.0.14).
- ArduinoJson 7.4.3: MIT. [Upstream](https://github.com/bblanchon/ArduinoJson).
- PlatformIO and the Espressif toolchain install additional build-time packages
  under their own licenses. They are not vendored into this repository.
- Pillow is used to generate assets, under its own license.

This source-only release does not bundle compiled firmware or the above libraries.
If you distribute a compiled application, retain all required dependency notices
and comply with their source/relinking requirements where applicable.

## Weather, time and trust anchor

- Weather data: [Open-Meteo](https://open-meteo.com/),
  [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/).
  Icons map its WMO weather codes to original project drawings. Values in examples
  are either illustrative or historical screen readings, not a live forecast.
- NTP servers are external time services, not bundled software.
- `src/weather_ca.h` embeds the public ISRG Root X1 certificate for HTTPS
  verification. It is a public trust anchor, not a private key or credential.
  [Issuer certificate page](https://letsencrypt.org/certificates/).

## Images and design references

- `docs/images/weather-in-use.jpg` and `clock-in-use.jpg` are the project owner's
  original photographs, provided for public project documentation under this
  project's MIT license. EXIF/GPS metadata has been removed. They show an earlier
  display revision, before the battery icon was added.
- `docs/images/cover.png` is an AI-assisted cover illustration based on those
  photos, not a documentary photograph. Its prompt is in `docs/COVER-PROMPT.md`.
- Other screen and weather-icon figures are generated from this repository's
  renderer using its bundled open font. Their visible example locations and
  readings are not device credentials.
- The earlier e-paper layout drew inspiration from
  [G6EJD/ESP32-e-Paper-Weather-Display](https://github.com/G6EJD/ESP32-e-Paper-Weather-Display).
  No code or icon assets from that project are included here.
- Heltec and other product names identify the compatible hardware; this is an
  independent community project, not an official Heltec product.
