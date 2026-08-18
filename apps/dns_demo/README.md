# DNS Demo

The demo resolves `localhost` with IPv4 TCP `getaddrinfo()` and prints the
numeric addresses returned by the PS5 runtime. This local check is the required
success criterion and does not depend on internet access.

To probe an external hostname, set `CPYTHONPS5_DNS_HOST` before launching the
app. External DNS failure is reported and does not turn the local demo into a
failure.
