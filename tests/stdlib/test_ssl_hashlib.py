"""PS5 adaptations of CPython hashlib and ssl import coverage."""

import hashlib
import ssl
import _hashlib
import _ssl


assert hashlib.sha256(b"CPythonPS5").hexdigest() == (
    "f1ed3d6270637bf6fc81bcc16cfd460929a9f270f5215fac17d4c8c127323087"
)
assert hashlib.md5(b"CPythonPS5").digest_size == 16
assert ssl.OPENSSL_VERSION.startswith("OpenSSL 3.5.")
assert _hashlib.openssl_sha256(b"CPythonPS5").digest() == hashlib.sha256(
    b"CPythonPS5"
).digest()
context = ssl.create_default_context()
assert context.minimum_version >= ssl.TLSVersion.TLSv1_2

print("test_ssl_hashlib: PASS")
