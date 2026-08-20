# Third-party notices

Python-PS5 combines project-owned launcher code with upstream CPython,
PS5-payload tooling, native libraries, and example web packages. This file is
an attribution index; each upstream project remains the authority for its
license and notice text.

| Project | Used for | License / notices |
| --- | --- | --- |
| [CPython 3.14.7](https://github.com/python/cpython/tree/v3.14.7) | Interpreter and standard-library source | [PSF License](https://docs.python.org/3.14/license.html) |
| [PS5 Payload SDK](https://github.com/ps5-payload-dev/sdk) | PS5 compiler, linker, headers, and deployment tools | See upstream repository; SDK is build-time only and is not redistributed here |
| [Flask](https://github.com/pallets/flask) | Flask dashboard example | See upstream repository |
| [Gunicorn](https://github.com/benoitc/gunicorn) | WSGI server example | See upstream repository |
| [Werkzeug](https://github.com/pallets/werkzeug), [Jinja](https://github.com/pallets/jinja), [MarkupSafe](https://github.com/markupsafe/markupsafe), [ItsDangerous](https://github.com/pallets/itsdangerous), [Click](https://github.com/pallets/click), [Blinker](https://github.com/pallets-eco/blinker) | Flask dependency closure | See each upstream repository |
| [OpenSSL](https://github.com/openssl/openssl) | TLS and cryptography | See upstream `LICENSE.txt` |
| [SQLite](https://www.sqlite.org/) | Database runtime | See upstream copyright and public-domain notice |
| [zlib](https://github.com/madler/zlib), [bzip2](https://sourceware.org/bzip2/), [XZ Utils](https://github.com/tukaani-project/xz) | Compression | See each upstream license and notice |
| [libffi](https://github.com/libffi/libffi) | `_ctypes` support | See upstream `LICENSE` |
| [libmicrohttpd](https://git.gnunet.org/libmicrohttpd.git/) | Web launcher HTTP support | See upstream `COPYING` |

The repository does not include the PS5 SDK, the pinned CPython checkout, or
the downloaded native dependency source trees. They are fetched or supplied
at build time according to the project build scripts and their own terms.

Before the first public release, the maintainer should select and add a
top-level license for project-owned code. The project currently does not make
that legal choice implicitly.
