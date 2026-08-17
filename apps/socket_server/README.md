# Socket Server 9091

Persistent Python launcher example. It listens on TCP port `9091`, prints
incoming UTF-8 data to the launcher console, and replies with `OK`.

From another machine, send a line with:

```sh
printf 'hello from netcat\n' | nc PS5_IP 9091
```
