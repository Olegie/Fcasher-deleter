# Fcasher Architecture

## Design Goals

Fcasher is a conservative system utility rather than a bulk deletion script. The architecture optimizes for:

- predictable safety boundaries
- explicit scan-before-clean flow
- low-overhead traversal code
- modular growth without turning into a monolith
- compatibility with older Win32 surfaces where practical

## Layer Split

### C++ application layer

The C++ side owns policy, user experience, and orchestration:

- `gui/main_window.*`
  Hosts the classic Win32 desktop shell built from stable USER32 controls such as buttons, combo boxes, edit boxes, and group boxes.
- `gui/session_controller.*`
  Bridges GUI state into CLI-compatible options and captures backend output without duplicating workflow logic.
- `cli.*`
  Parses commands, flags, and category selectors.
- `command_dispatcher.*`
  Builds scan plans, runs the scan/preview/clean/analyze workflows, prompts for confirmation, applies high-level filters, and coordinates exports.
- `category_registry.*`
  Maps user-facing selectors and presets into stable category definitions and scan roots.
- `safety_policy.*`
  Adds higher-level path exclusions on top of the C core.
- `report_formatter.*` and `report_service.*`
  Turn raw scan and cleanup data into readable console and text output.
- `windows_paths.*`
  Resolves Windows-specific directories and legacy-compatible cache roots.
- `export_service.*`
  Writes text and JSON reports.

### C low-level layer

The C side owns the operational core:

- `scan_engine.*`
  Traverses directories with Win32 APIs and records scan candidates.
- `path_filters.*`
  Normalizes paths, checks protected roots, and evaluates category match rules.
- `scan_result.*`
  Manages dynamic buffers for findings, skips, and scanned roots.
- `cleanup_engine.*`
  Executes deletion or dry-run simulation and records per-item outcomes.
- `file_record.*`
  Provides the shared category and record metadata used by both layers.

## Execution Flow

1. The GUI shell or CLI parser resolves the requested command, selectors, presets, and filters.
2. The category registry expands grouped selectors such as `temp` and named presets such as `diagnostics` into concrete scan categories.
3. The Windows path resolver discovers environment-derived roots and safe default locations.
4. The dispatcher converts C++ category definitions into `fc_scan_target` structures for the C core.
5. The scan engine walks each root with `FindFirstFileA` and `FindNextFileA`, applies path filters, and records findings.
6. The C++ dispatcher applies high-level result filters such as size range, age window, sorting, and limit trimming.
7. The report layer renders scan, preview, analyze, cleanup, or catalog reports and optional TXT/JSON exports.
8. For `clean`, the dispatcher requires confirmation before the cleanup engine deletes files or simulates deletion in dry-run mode.

## Safety Boundaries

Fcasher intentionally layers safety checks instead of trusting one mechanism:

- category selection is limited to known cache, temp, log, and dump locations
- low-level path filters reject protected system roots
- the C++ safety policy rejects additional user-content paths
- cleanup is always preceded by a scan report
- locked, system-marked, inaccessible, or protected files are logged as skipped

## Compatibility Notes

The project avoids `std::filesystem` and other newer Windows APIs on purpose. The core relies on broadly available Win32 functions such as:

- `GetEnvironmentVariableA`
- `GetWindowsDirectoryA`
- `FindFirstFileA`
- `DeleteFileA`
- `CreateDirectoryA`

The GUI layer follows the same compatibility intent by using classic Win32 window classes instead of newer frameworks. That keeps the visual shell native, lightweight, and realistic for legacy-friendly system utility code.

That keeps the source realistic for Windows XP and Windows 2000 era API baselines. Actual runtime support still depends on the compiler runtime chosen at build time.
