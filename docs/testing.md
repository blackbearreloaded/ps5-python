# Testing and PS5 storage paths

## Phase 4 commands

Run the lifetime checks on the host:

```sh
make host-lifetime
```

Run the bounded process-lifetime checks on the PS5:

```sh
PS5_HOST=192.168.4.30 make ps5-lifetime
```

The PS5 runner starts a fresh `python.elf` process for each test and repeats
the pair three times by default. Set `PS5_LIFETIME_RUNS` to change the count.

The native launcher reports an OS peak resident-size value in its final
success notification when `getrusage(RUSAGE_SELF)` is available. The unit and
meaning of `ru_maxrss` are platform-defined, so this is a comparative signal
between runs, not yet a complete CPython heap profiler.

Deliberate out-of-memory injection is intentionally deferred: forcing an
allocation failure inside the first PS5 runtime could destabilize the whole
payload process rather than produce a useful test result.

The intentional error scripts are:

```text
tests/lifetime/expected_uncaught.py
tests/lifetime/expected_syntax.py
```

They must fail. Deploy them one at a time with `make ps5-run
SCRIPT=tests/lifetime/expected_uncaught.py` (and then the syntax script) and
confirm that the output includes `CPYTHON SCRIPT FAIL`.

## Why the current bundle uses `/data/python`

The current project now places the standalone ELF, script, and runtime bundle
under `/data/python`. This gives the test runtime a stable, easy-to-inspect
location while keeping it separate from a future native app's content and
save-data mounts.

`/download0` remains useful when a packaged app needs persistent app-managed
data that survives an app restart, such as user configuration, logs, caches,
or a Python package bundle installed by the app. It is not required for
CPython startup. `/data/python` is the selected host/deployment path for this
standalone project, provided that the target environment exposes it to the
ELF.

When this project becomes a native app, choose the path by lifetime:

| Data | Suitable location |
| --- | --- |
| ELF, test script, and runtime bundle | `/data/python` |
| Generated temporary files | `/user/temp` |
| User configuration and installed scripts | `/download0` |
| Save-game-style user data | a properly mounted save-data path |

The exact writable paths remain app/sandbox-dependent on the target system;
the table is a design choice for this project, not a claim that every PS5
process can write every path.

## Web launcher checks

`PS5_WEB_CHECK=1 make ps5-web` now validates the HTTP script backend in
addition to the WebSocket and raw TCP REPL paths. It posts a bounded complete
script to `/api/script/run`, checks the captured output and byte count, then
confirms that a variable created by the HTTP request is visible through the
WebSocket REPL.

The web deployment also starts `python-app-supervisor.elf`. Application
validation should confirm that two `/api/launch` requests return distinct job
IDs and child PIDs, that `/api/status` remains responsive while both jobs are
active, and that `POST /api/app/stop?job_id=...` stops only the selected job.
The existing `time_demo` app is the bounded manual stop test; use it alongside
`socket_server` or another non-conflicting app for the concurrency check.
