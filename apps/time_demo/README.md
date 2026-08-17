# Time Demo

Small launcher example using the built-in `time` module. It prints three
timestamped messages, waits five seconds between messages, then exits.

The wait uses `time.monotonic()` because the current PS5 libc reports
`ENOSYS` for CPython's `time.sleep()` syscall path. This keeps the example
working while the native sleep hook is investigated.
