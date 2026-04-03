# Fcasher Architecture

## Design Goals

Fcasher is designed as a conservative system utility rather than a bulk deletion script. The architecture prioritizes:

- predictable safety boundaries
- clear inspection and cleanup phases
- low-level traversal routines that stay lightweight
- high-level orchestration that remains easy to extend

## Layer Split

### C++ Application Layer

The C++ layer owns the user-facing behavior:

- `cli.*`: parses commands, flags, and selection tokens
- `command_dispatcher.*`: builds scan plans, orchestrates execution, prompts for confirmation, and coordinates exports
- `category_registry.*`: maps CLI selectors to supported cleanup categories and their scan roots
- `safety_policy.*`: enforces additional high-level path restrictions and confirmation rules
- `report_formatter.*` and `report_service.*`: turn raw scan and cleanup data into readable console and text reports
- `windows_paths.*`: resolves Windows-specific roots from the environment

### C Low-Level Layer

The C layer owns the narrow operational core:

- `scan_engine.*`: traverses roots, records findings, and reports inaccessible paths
- `path_filters.*`: normalizes paths, applies wildcard or token checks, and rejects protected roots
- `scan_result.*`: manages dynamic result buffers for findings, scanned roots, and skipped items
- `cleanup_engine.*`: executes deletions or dry-run cleanup plans and records outcomes
- `file_record.*`: shared category metadata and record helpers

## Execution Flow

1. The CLI parser resolves the command and raw selection tokens.
2. The category registry expands tokens such as `temp` into concrete category definitions.
3. Windows-specific paths are resolved from environment variables and fixed-drive enumeration.
4. The dispatcher converts category definitions into `fc_scan_target` instances for the C core.
5. The scan engine traverses each target root, applies filters, and records candidates.
6. The reporting layer prints a preview-first report and optionally exports text or JSON.
7. For cleanup, the dispatcher requires confirmation, then the cleanup engine deletes eligible files or simulates deletion in dry-run mode.

## Safety Boundaries

Fcasher uses overlapping safeguards rather than a single trust boundary:

- categories are limited to known cache, temp, log, and dump locations
- core path filters reject protected system roots
- the C++ safety policy blocks additional user-content paths
- cleanup is opt-in and confirmation-gated
- inaccessible and locked files are skipped instead of forced

This layered model keeps the system defensible even when categories evolve over time.
