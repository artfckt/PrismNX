# PrismNX architecture

PrismNX is a native C++20 Tesla/Ultrahand overlay built with devkitA64/libnx.
It uses Fizeau IPC for internal-display color processing. Telemetry and quick
controls use independently initialized optional console services.

## Modules

- `model.cpp` validates settings and implements bounded manual controls.
- `presets.cpp` provides 18 static presets and category metadata.
- `backend.cpp` snapshots all four profiles and verifies transactions. A
  failed hardware application is treated as potentially partial; recovery
  must both succeed and match readback. Reconnecting cannot clear uncertainty.
- `switch_backend.cpp` implements the real Fizeau IPC connection.
- `storage.cpp` preserves other profiles and configuration comments, honors
  upstream path precedence and uses checked staging, rollback and backups.
- `telemetry.cpp` reads optional services and handles explicit brightness and
  volume actions. Missing values are unavailable, never fabricated zeroes.
- `main.cpp` and UI modules implement the hub, sliders, presets and tools.
  Polling runs in updates, not draw callbacks; GUI pops are deferred to avoid
  invalid references in the pinned libtesla implementation.

## Persistence and compatibility

Manual edits unify day and night for the selected internal profile. Save is
explicit. Live restoration does not silently replace saved configuration.
Spatial sharpening is outside the Fizeau color-matrix/LUT backend.

The PrismNX rename retains `SwitchColor.ovl`, `config/SwitchColor/reports/`
and `.switchcolor.*` recovery suffixes for compatibility with existing
installations. Fizeau paths, protocol, title ID and settings are unchanged.

The bundled backend is Fizeau 2.8.3 plus the documented EOF-profile fix;
original dependency source and license notices remain intact. See
[THIRD_PARTY.md](../THIRD_PARTY.md) and [VALIDATION.md](VALIDATION.md).
