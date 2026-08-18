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


assert hashlib.sha256(b"CPythonPS5").hexdigest() == (
    "f1ed3d6270637bf6fc81bcc16cfd460929a9f270f5215fac17d4c8c127323087"
)
assert hashlib.md5(b"CPythonPS5").digest_size == 16
assert ssl.OPENSSL_VERSION.startswith("OpenSSL 3.5.")
assert _hashlib.openssl_sha256(b"CPythonPS5").digest() == hashlib.sha256(
    b"CPythonPS5"
).digest()
assert hashlib.sha3_256(b"CPythonPS5").digest_size == 32
assert _hashlib.new("blake2b512", b"CPythonPS5").digest_size == 64
assert hmac.compare_digest(hmac.new(b"key", b"payload", "sha256").hexdigest(),
                           hmac.new(b"key", b"payload", "sha256").hexdigest())
generator = random.Random(1234)
assert generator.randrange(100) == random.Random(1234).randrange(100)
context = ssl.create_default_context()
assert context.minimum_version >= ssl.TLSVersion.TLSv1_2
encoded = base64.b64encode(b"CPythonPS5")
assert encoded == b"Q1B5dGhvblBTNQ=="
assert base64.b64decode(encoded) == b"CPythonPS5"
with warnings.catch_warnings(record=True) as caught:
    warnings.simplefilter("always")
    warnings.warn("PS5 warning", UserWarning)
    assert len(caught) == 1
assert hasattr(_py_warnings, "warn")

print("test_ssl_hashlib: PASS")
