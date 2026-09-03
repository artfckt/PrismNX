# Validation record — 2026-09-03

Target supplied by user: Switch Lite, Atmosphere 1.11.2|S, firmware 20.5.0.
No physical console was connected for this work.

## Completed

- ARM64 cross-compilation of SwitchColor 0.1.0 using devkitA64 r30-1 /
  GCC 16.1.0 and libnx 4.12.0-1. ELF machine = AArch64; output has NRO0 and
  ASET metadata headers. Final binaries are hashed in `dist/manifest.json`.
- ARM64 build of Fizeau v2.8.3 with the documented EOF profile patch.
  Output is a PFS0 exefs NSP. No changes to firmware-specific display logic.
- Host production-code tests with GCC 15.3.0, warnings treated as errors:
  control bounds, NaN/Inf, exact neutral values, presets, protocol struct sizes,
  missing service, state changes, profile preservation, partial mutation,
  checked rollback, uncertainty latch and explicit recovery.
- Storage tests with injected short writes and rename failures:
  first-backup retention, complete profiles, config precedence, invalid source,
  pending recovery files, replacement rollback and failure reporting.
- Real Fizeau parser regression: generated host harness uses the actual
  production reader, profile-switch callback, final parse block, Config parser
  and inih. Stock code loses profile4 at EOF. Patched code round-trips all four
  profiles/globals, CRLF, preserved comments, legacy aliases, reordered sections
  and small floating-point values.
- Packaging verifies the pinned official Fizeau release SHA256 before copying
  CMU IPS files. The SD package does not contain `ovlmenu.ovl` or overwrite-ready
  Fizeau config. Initial neutral config is under `optional/`.

Commands are recorded in README. Source and package checksums are generated
with the distribution rather than hard-coded in this record.

## Still requires Switch testing

Opening and navigating the overlay on the console, actual CMU visual effects,
exact firmware compatibility, overlay-menu compatibility, sleep/resume, and
persisting settings through a real reboot. No claims about those checks are
made from host mocks or a successful compiler result.

## Known scope limits

No spatial sharpening, automatic per-game profiles, or panel calibration.
Day/night color settings are deliberately unified by manual edits. Runtime
restore is separate from disk save. Power-loss atomicity on an SD filesystem
cannot be guaranteed by host fault injection; recovery/backup files are kept
and unresolved staged files block further saves.
