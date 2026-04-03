# Fcasher Usage

## Overview

Fcasher now ships with two front ends:

- `fcasher.exe`
  Classic Win32 GUI for old-school desktop interaction on Windows 2000 through Windows 11 style environments.
- `fcasher_cli.exe`
  CLI frontend for automation, scripting, and terminal-first usage.

The CLI exposes six command modes:

- `scan`
  Inspect supported locations and print categorized findings.
- `preview`
  Run a scan and emphasize the exact files that would be selected for cleanup.
- `clean`
  Run a scan, print the preview, require confirmation, and then delete or simulate deletion.
- `report`
  Scan and focus on exporting TXT and JSON results.
- `analyze`
  Scan and generate ranked views such as largest files, oldest files, and directory hot spots.
- `categories`
  Print the built-in category and preset catalog.

## Common Commands

GUI workflow:

1. Launch `build\fcasher.exe`.
2. Select mode, preset, categories, and optional filters.
3. Click `Run`.
4. Save the current report or export JSON from the GUI.

CLI examples:

```powershell
fcasher_cli scan --all
fcasher_cli preview --preset quick-sweep --sort size --limit 25
fcasher_cli analyze --all --min-size 8MB --sort size
fcasher_cli clean --category shader-cache --dry-run
fcasher_cli categories
fcasher_cli report --preset diagnostics --older-than-days 14 --json reports\scan.json
```

## Category Selectors

- `--temp`
  Expands to `user-temp`, `local-app-temp`, and `windows-temp`.
- `--logs`
  Selects supported log and WER report locations.
- `--browser`
  Selects supported browser cache roots.
- `--thumbnails`
  Selects thumbnail and icon cache files.
- `--shader-cache`
  Selects graphics shader cache directories.
- `--crash-dumps`
  Selects crash dump locations.
- `--recent`
  Selects recently modified temp artifacts. This heuristic category is not part of `--all`.
- `--recycle-bin`
  Selects recycle bin roots. This is always explicit and never part of `--all`.
- `--category <name>`
  Selects a canonical category such as `browser-cache`, `user-temp`, or `logs`.
- `--preset <name>`
  Expands a named preset such as `quick-sweep`, `browser-focus`, `graphics-stack`, `diagnostics`, or `safe-all`.

## Advanced Filters

- `--min-size <size>`
  Keep only files at or above the given size.
- `--max-size <size>`
  Keep only files at or below the given size.
- `--modified-within-days <n>`
  Keep only files modified within the last `n` days.
- `--older-than-days <n>`
  Keep only files older than `n` days.
- `--sort <mode>`
  Order the effective result set by `native`, `size`, `size-asc`, `newest`, `oldest`, `path`, or `category`.
- `--limit <n>`
  Trim the effective result set after filtering and sorting.

## Output Controls

- `--dry-run`
  Simulate cleanup without deleting files.
- `--verbose`
  Print export confirmations and extra workflow details.
- `--yes`
  Skip the interactive confirmation for real cleanup.
- `--export <path>`
  Write the primary text report to disk.
- `--json <path>`
  Write scan results to disk as JSON.

## Safety Expectations

- Fcasher always scans before cleanup.
- Real cleanup requires confirmation unless `--yes` is supplied.
- Protected paths are blocked even when a supported category points near them.
- Locked, inaccessible, or system-marked files are recorded as skipped.
- Recycle Bin cleanup is opt-in and separated from the normal `--all` flow.

## Practical Notes

- `clean` requires either explicit category selection or `--all`.
- `report` and `analyze` default to the safe built-in categories if no explicit selector is supplied.
- `--all` intentionally excludes recycle bin handling and the heuristic `recent-artifacts` category.
- The GUI keeps the same backend safety model; it does not bypass confirmation or protected path checks.
