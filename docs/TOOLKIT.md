# PrismNX 0.3.0 user guide

PrismNX provides a central hub for display settings, live console information,
quick controls and installed tools. B returns one level. Closing the overlay
releases its service connections and stops periodic readings.

## Display and presets

Open **Display and presets > Presets**, choose a category and style, then
select **Apply live**. Details show the values before application. Color
correction must be enabled to see the result. Use **Save for reboot** in the
Display menu to persist settings on SD.

| Category | Presets |
| --- | --- |
| Natural / OLED styles | Standard, Vibrant, OLED Soft, OLED Vivid, Deep Colors |
| Contrast / visibility | Contrast+, Contrast Soft, Shadow Lift |
| Warm / cinema / evening | Cinema, Night, Warm 5500K, Warm+ 4500K, Amber 3200K, Reading |
| Creative styles | Cool 8000K, Retro Warm, Monochrome, Pastel |

OLED styles cannot reproduce OLED black levels on the Lite's LCD. These are
visual starting points, not calibrations. Adjust the six sliders afterwards.
Color settings are global to the internal display, not automatic per-game
profiles. The first manual slider edit uses daytime values and unifies day
and night; it preserves schedule, dimming, channels and existing filters.
Presets also reset the filter/RGB channels and disable Fizeau dimming.
Physical backlight brightness is separate, under Quick controls.

## Live information

| Page | Readings |
| --- | --- |
| Overview | Model, firmware, active Title ID, battery, SoC temperature, local IP, free SD space |
| Battery | Percentage, raw charge, estimated remaining capacity, charger, charging state, voltage, temperature, charge current limit |
| Temperatures | SoC, PCB, skin and battery |
| Clock rates | CPU, GPU and EMC memory in MHz |
| Network | Local IPv4, Wi-Fi radio, connection type, internet status and signal |
| System | Console model, detected firmware and active Title ID |
| SD storage | Free, total and used space in GiB |

The visible page refreshes about once per second. Press A for the full value
and error code. Inaccessible readings display **Unavailable**. Clock rates
are not utilization or FPS. Battery capacity is a controller estimate; the
charge current limit is not instantaneous power consumption. These pages
and quick controls work independently of the Fizeau connection.

## Quick controls

- Physical brightness: steps of five percentage points, from 5% to 100%.
- Volume: one step for the active output, within the system-reported range.

Only selecting an action changes a value. Settings are read back after each
write; a failed confirmation displays an error. Automatic brightness and
dimming settings are not modified. These controls do not save to Fizeau.

## Reports and installed tools

**Export diagnostics to SD** creates a new report in
`config/SwitchColor/reports/`. It includes available readings and errors,
including local IP and Title ID. Reports remain on the SD card.

Shortcuts open installed Status Monitor, FPSLocker, sys-clk and Sysmodules
overlays. These are separate applications with their own dependencies.
Launching one closes PrismNX while leaving live color correction active.

## Future directions

These features are proposals, not implemented in 0.3.0:

1. Named custom presets, favorites, import/export and quick A/B comparison.
2. Automatic Title ID-to-preset switching, with a background service so it
   continues after the overlay closes.
3. FPS/frame-time integration from a compatible measurement source and
   performance/temperature graphs.
4. Session history, CSV recording and configurable battery/temperature
   display thresholds after sensor validation on hardware.
5. A full-screen companion application for profile editing and reports.

Spatial sharpening requires a different graphics integration than Fizeau/CMU.
PrismNX does not directly modify clocks, fan settings or charging limits.

See [INSTALL.md](INSTALL.md) for upgrades and [VALIDATION.md](VALIDATION.md)
for the distinction between build checks and physical console testing.
