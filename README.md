# Fcasher

Fcasher is a Windows-first console utility for fast cache inspection, temporary file analysis, and controlled cleanup. It is built as a portfolio-grade systems project with a deliberate split between a high-level C++ application layer and low-level C scanning and cleanup modules.

## Why This Tool Exists

Windows cleanup tools are often either opaque or overly aggressive. Fcasher takes the opposite position:

- inspect before deleting
- show exactly what is eligible
- support dry-run execution
- clean by safe category boundaries
- export readable reports for review or automation

The result is a technical utility for users who want visibility, control, and a credible safety model.

## Features

- Preview-first scanning with per-file path visibility
- Category-based cleanup for temp, logs, browser cache, shader cache, thumbnails, crash dumps, and related artifacts
- Dry-run mode for cleanup rehearsal
- Explicit confirmation before destructive actions
- Safety guardrails around protected and system-critical locations
- Console reporting plus TXT and JSON export
- Modular CMake build with separate C and C++ components
- Basic unit tests covering CLI parsing, path filters, scanning, and safety policy behavior

## Safety Model

Fcasher is intentionally conservative.

- It does not touch documents, registry data, or persistence settings.
- It skips protected roots such as `System32`, `WinSxS`, `Program Files`, and user content folders.
- It reports skipped or inaccessible files instead of forcing deletion.
- Recycle Bin handling is excluded from `--all` and only considered when explicitly requested.
- Real cleanup requires confirmation unless `--yes` is supplied.

## Build

```powershell
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build --output-on-failure
```

The project targets Windows and requires a compiler with C11 and C++17 support.

## Usage

```powershell
fcasher scan --all
fcasher scan --temp --logs
fcasher preview --category browser-cache --json reports\browser.json
fcasher clean --category temp --dry-run
fcasher clean --all --yes --export reports\cleanup.txt
fcasher report --all --export reports\scan.txt --json reports\scan.json
```

Supported category selectors:

- `--all`
- `--temp`
- `--logs`
- `--browser`
- `--thumbnails`
- `--shader-cache`
- `--crash-dumps`
- `--recent`
- `--recycle-bin`
- `--category <name>`

## Architecture Summary

### C++ Application Layer

- CLI parsing and validation
- command dispatch and workflow orchestration
- category registry and path resolution
- safety policy
- report formatting and export

### C Low-Level Layer

- directory traversal
- path filtering and wildcard checks
- scan result accumulation
- deletion queue execution

This split keeps high-level behavior readable while preserving a lean, C-style core for traversal and cleanup primitives.

## Project Tree

```text
Fcasher/
├── CMakeLists.txt
├── LICENSE
├── README.md
├── .gitignore
├── docs/
│   ├── architecture.md
│   └── usage.md
├── include/
│   ├── app/
│   │   ├── category_registry.hpp
│   │   ├── cli.hpp
│   │   ├── command_dispatcher.hpp
│   │   ├── report_formatter.hpp
│   │   └── safety_policy.hpp
│   ├── core/
│   │   ├── cleanup_engine.h
│   │   ├── file_record.h
│   │   ├── path_filters.h
│   │   ├── scan_engine.h
│   │   └── scan_result.h
│   ├── platform/
│   │   └── windows_paths.hpp
│   └── services/
│       ├── export_service.hpp
│       └── report_service.hpp
├── src/
│   ├── app/
│   │   ├── category_registry.cpp
│   │   ├── cli.cpp
│   │   ├── command_dispatcher.cpp
│   │   ├── report_formatter.cpp
│   │   └── safety_policy.cpp
│   ├── core/
│   │   ├── cleanup_engine.c
│   │   ├── file_record.c
│   │   ├── path_filters.c
│   │   ├── scan_engine.c
│   │   └── scan_result.c
│   ├── platform/
│   │   └── windows_paths.cpp
│   ├── services/
│   │   ├── export_service.cpp
│   │   └── report_service.cpp
│   └── main.cpp
├── sample_output/
│   ├── cleanup_report.txt
│   ├── preview.txt
│   └── scan_result.json
└── tests/
    ├── test_cli.cpp
    ├── test_common.hpp
    ├── test_filters.cpp
    ├── test_main.cpp
    ├── test_safety.cpp
    └── test_scan.cpp
```

## Roadmap

- profile discovery for multi-profile browsers
- optional empty-directory cleanup for safe temp roots
- richer JSON schema and machine-readable cleanup outcomes
- PowerShell completion and packaging helpers
- benchmark mode for large cache trees

## License

Released under the MIT License. See [LICENSE](LICENSE).
