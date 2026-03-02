---
description: describe when these instructions should be loaded
# applyTo: '**/*.{ino,cpp,hpp,h,md}'
---

## Purpose

These instructions define the **official project context and coding rules** for all AI-generated outputs in this repository.
Apply them when generating code, editing code, reviewing changes, writing technical explanations, or updating documentation.

All project-facing content must be in **English**.

---

## Project Objective (M2609 Uninterrupted Clock)

Implement a robust Arduino-based clock that continuously maintains date/time using multiple sources with strict priority and coherence filtering.

Primary required behavior:

1. Get date/time from **DAB+**.
2. Get date/time from **NTP** over Wi-Fi.
3. Get date/time from **GPS**.
4. Source priority is strictly: **DAB > NTP > GPS**.
5. If a source differs by more than **±10 seconds** compared to others, mark it incoherent and ignore it.
6. Display date/time and sync errors on the **8x2 LCD**.
7. `SET` acknowledges errors and can trigger manual resynchronization.
8. Errors auto-acknowledge after **60 seconds** if at least one valid source exists.
9. `CFG` enters/exits source-selection mode.
10. In source-selection mode, encoder scrolls sources and `SET` enables/disables selected source.
11. Long `CFG` press (3s) enters/exits manual date/time configuration mode.
12. In manual mode, encoder edits values, `SET` moves to next field, `CFG` saves+exits, long `SET` cancels+exits.

---

## Current Architecture (must be preserved)

- Entry point and composition root: `DabGps.ino`
- Time abstraction: `src/time/core/TimeSource.hpp`
- Time arbitration/coherence/software clock: `src/time/core/TimeCoordinator.*`
- Source implementations:
  - `src/time/sources/DABTimeSource.*`
  - `src/time/sources/NTPTimeSource.*`
  - `src/time/sources/GPSTimeSource.*`
- Error model: `src/core/errors/SyncErrors.hpp`
- UI state machine: `src/ui/UiController.*`
- Network/API: `src/network/RestApiServer.*`, `src/network/WifiManager.*`
- Hardware pins/constants: `src/platform/PinDefinitions.hpp`

Do not introduce alternate architecture patterns unless explicitly requested.

---

## Hard Consistency Rules

1. **English only** for comments, docs, API descriptions, commit-style summaries, and generated explanations.
2. Keep source priority and coherence threshold unchanged unless explicitly requested.
3. Preserve existing module boundaries and responsibilities.
4. Prefer small, focused, local changes over broad refactors.
5. Keep naming consistent with existing code and hardware documentation.
6. Do not rename public classes/functions/files without explicit need.
7. Do not duplicate business logic already owned by another module.

---

## C++ / Arduino Coding Rules

- Follow existing style in the touched file (indentation, braces, naming).
- Use fixed-width integer types for protocol/time-sensitive data (`uint8_t`, `uint32_t`, etc.).
- Keep runtime deterministic and lightweight (avoid heavy dynamic allocation).
- Avoid `String` for frequently updated paths when simpler C/C++ alternatives are practical.
- Use `constexpr` for constants when possible.
- Keep functions short and purpose-specific.
- Prefer explicit validation for external data (GPS/NTP/DAB fields).
- Guard every hardware/network dependent path with failure handling.
- NEVER add blocking segments (busy waits, long polling loops, or blocking delays) in runtime paths; all logic must stay non-blocking so concurrent timing-sensitive protocols (I2C, UART, timers) keep running reliably.
- Add no dead code, no speculative abstractions, no unrelated cleanup.

---

## Time and Synchronization Rules

- Any new time logic must integrate with `TimeCoordinator` and not bypass it.
- Coherence decisions must remain transparent and explainable.
- Manual time set must update coordinator reference time cleanly.
- Error handling must map to `SyncErrorCode` and `SyncErrorState` conventions.
- Auto-ack timing behavior must remain consistent with `AUTO_ACK_TIMEOUT_MS`.

---

## UI and Interaction Rules

- Respect existing UI modes and transitions in `UiController`.
- Keep LCD content concise and stable for 8x2 constraints.
- Do not add new UI modes, screens, or interaction patterns unless requested.
- Maintain current button semantics (`SET`, `CFG`, long press behavior).

---

## Network/API Rules

- Preserve existing REST endpoints and semantics unless explicitly asked.
- New endpoints must be minimal, documented in README, and non-breaking.
- Any Wi-Fi or NTP change must preserve offline robustness and fail-safe behavior.

---

## Documentation Rules

- Keep README and Doxygen content aligned with actual behavior.
- If behavior changes, update relevant docs in the same change set.
- Use concise, technical English.
- Do not claim features that are not implemented.
- Every function and method must have a Doxygen docstring.
- Required tags for every function/method docstring: `@brief`, `@details`, `@date`, `@author`.
- If parameters exist, include one `@param` per parameter.
- If a return value exists, include an `@return` tag.
- Private/internal helper methods must be wrapped with `@internal` and `@endinternal`.

---

## Out-of-Scope Guardrails

Unless explicitly requested, do **not**:

- Add new major features from “possible improvements” (captive portal, alarm radio, Bluetooth mesh sync).
- Change target hardware platform assumptions.
- Reorganize project folders or rename key modules.
- Modify build/upload tasks unrelated to the requested change.

---

## Change Acceptance Checklist (for AI outputs)

Before finalizing any generated change, verify:

1. Requirements still match: DAB > NTP > GPS and ±10s coherence.
2. No regression in UI mode behavior.
3. Error acknowledgement rules remain valid.
4. Code compiles conceptually for Arduino UNO R4 targets.
5. Documentation/comments are in English and reflect real behavior.
6. Changes are targeted and consistent with existing architecture.
