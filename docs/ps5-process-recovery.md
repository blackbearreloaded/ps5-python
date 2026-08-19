# PS5 payload process recovery

The payload SDK's `prospero-deploy` command can start an ELF, but it does not
provide a remote process-list or stop command. For test recovery, this project
builds a deliberately small `ps5-kill.elf` helper.

Build and run it with a known target PID:

```sh
bash tools/run_ps5_kill.sh PID
```

The default signal is `SIGTERM` (15). A signal can be supplied explicitly,
for example `SIGKILL` (9):

```sh
bash tools/run_ps5_kill.sh PID 9
```

This helper only sends a signal to the PID supplied by the operator. It does
not enumerate processes or broadcast signals. It is a test/recovery payload,
not part of the Python runtime or the web launcher deployment.

The helper requires a PID that is already known. The current PS5 payload
workflow does not expose a portable remote PID query; if the PID cannot be
identified, restart the console or use a separate authorized launcher port.
