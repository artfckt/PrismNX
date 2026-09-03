# SwitchColor Implementation Plan

> **For agentic workers:** Execute the authorized implementation in this task. Use a bounded plan review and an independent final review; do not pause for redundant approval.

**Goal:** Build a native Switch Lite display-control overlay and deliver a compiled `.ovl` with Romanian installation instructions.

**Architecture:** Small libtesla UI, a tested controller over Fizeau IPC, and a checked backup/save path. Fizeau remains the hardware backend and is distributed separately from the overlay.

**Tech Stack:** C++20, devkitA64, libnx, libtesla, Fizeau v2.8.3 protocol; native host tests.

---

## Chunk 1: Implementation and delivery

- [x] Pin dependency sources under `third_party/`; retain licenses and record revisions in `THIRD_PARTY.md`.
- [x] Create `include/model.hpp`, `source/model.cpp`: bounded settings, modest manual presets, generation of a static day/night profile.
- [x] Create `include/backend.hpp`, `source/backend.cpp`: service-presence probe, snapshot all profiles, checked IPC, rollback and restore.
- [x] Create `include/storage.hpp`, `source/storage.cpp`: preserve profiles in Fizeau format, backup and checked writes, honor upstream config precedence.
- [x] Create `source/main.cpp`: live controls, preset page, status and information pages; do not send IPC from draw callbacks or during UI initialization.
- [x] Create `Makefile` and `scripts/build.ps1`; compile against installed devkitPro from its MSYS2 shell.
- [x] Add `tests/` for dangerous numeric inputs, profile-preserving serialization, missing-service behavior and rollback using host mocks. Test production code, not a second implementation.
- [x] Create `scripts/package.py`, README and installation checklist; package overlay and dependency separately, without replacing Fizeau config or the overlay menu.
- [x] Run host tests, build ARM64 overlay, inspect NRO header and package contents, then complete final code review.
- [x] Report artifacts and explicitly distinguish successful build/test from console validation.
