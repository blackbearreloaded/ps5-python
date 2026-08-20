"""PS5 adaptations of CPython hashlib and ssl import coverage."""

import hashlib
import ssl
import _hashlib
import _ssl
import base64
import warnings
import _py_warnings
import hmac
import random
import sys


assert hashlib.sha256(b"Python-PS5").hexdigest() == (
    "f389df734a67cd7233ce8fc6d8b29c2c7771ea01004958b82c1b45c42b8d5f7d"
)
assert hashlib.md5(b"Python-PS5").digest_size == 16
assert ssl.OPENSSL_VERSION.startswith("OpenSSL")
if sys.platform.startswith("freebsd"):
    assert ssl.OPENSSL_VERSION.startswith("OpenSSL 3.5.")
assert _hashlib.openssl_sha256(b"Python-PS5").digest() == hashlib.sha256(
    b"Python-PS5"
).digest()
assert hashlib.sha3_256(b"Python-PS5").digest_size == 32
assert _hashlib.new("blake2b512", b"Python-PS5").digest_size == 64
assert hmac.compare_digest(hmac.new(b"key", b"payload", "sha256").hexdigest(),
                           hmac.new(b"key", b"payload", "sha256").hexdigest())
generator = random.Random(1234)
assert generator.randrange(100) == random.Random(1234).randrange(100)
context = ssl.create_default_context()
assert context.minimum_version >= ssl.TLSVersion.TLSv1_2
encoded = base64.b64encode(b"Python-PS5")
assert encoded == b"UHl0aG9uLVBTNQ=="
assert base64.b64decode(encoded) == b"Python-PS5"
with warnings.catch_warnings(record=True) as caught:
    warnings.simplefilter("always")
    warnings.warn("PS5 warning", UserWarning)
    assert len(caught) == 1
assert hasattr(_py_warnings, "warn")

print("test_ssl_hashlib: PASS")
