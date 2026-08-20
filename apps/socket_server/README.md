# Socket Server 9091

Persistent Python launcher example. It listens on TCP port `9091`, prints
incoming UTF-8 data to the launcher console, and replies with `OK`.

The listener uses a short accept timeout so the launcher Stop action can
interrupt it cleanly.

From another machine, send a line with:

```sh
printf 'hello from netcat\n' | nc PS5_IP 9091
```
