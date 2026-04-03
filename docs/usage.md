# Fcasher Usage

## Overview

Fcasher exposes four command modes:

- `scan`: inspect supported locations and report removable candidates
- `preview`: scan and emphasize the files that would be selected for cleanup
- `clean`: scan, preview, confirm, then delete eligible files unless `--dry-run` is active
- `report`: scan and focus on exporting results

## Common Commands

```powershell
fcasher scan --all
fcasher scan --temp --logs --verbose
fcasher preview --category browser-cache
fcasher clean --category shader-cache --dry-run
fcasher clean --all --yes --export reports\cleanup.txt
fcasher report --all --export reports\scan.txt --json reports\scan.json
```

## Category Selectors

- `--temp`: user temp, local app temp, and Windows temp
- `--logs`: log and diagnostic artifacts
- `--browser`: browser cache roots
- `--thumbnails`: Windows thumbnail and icon cache
- `--shader-cache`: graphics shader cache
- `--crash-dumps`: crash dump locations
- `--recent`: recently modified temp artifacts
- `--recycle-bin`: recycle bin preview or cleanup, excluded from `--all`
- `--category <name>`: direct category selection using canonical names such as `user-temp` or `browser-cache`

## Output Controls

- `--dry-run`: simulate cleanup without deleting files
- `--verbose`: include extra workflow details
- `--yes`: skip interactive confirmation for real cleanup
- `--export <path>`: write the primary text report to a file
- `--json <path>`: export scan results as JSON

## Safety Expectations

- Fcasher always scans before cleanup.
- Real cleanup requires confirmation unless `--yes` is supplied.
- Protected paths are blocked even if manually specified through categories.
- Locked or inaccessible files are logged as skipped.

## Notes

- `clean` requires explicit category selection or `--all`.
- `report` can be used without `--export`; it still prints to the console.
- `--all` intentionally excludes recycle bin handling.
