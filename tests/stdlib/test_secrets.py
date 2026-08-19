"""CPython 3.14.7-derived tests for Tier 6 security and i18n modules."""

import io
import os

import getpass
import gettext
import hmac
import locale
import secrets
import unicodedata


# Adapted from Lib/test/test_secrets.py and test_hmac.py.
token = secrets.token_bytes(24)
assert isinstance(token, bytes) and len(token) == 24
assert len(secrets.token_hex(12)) == 24
assert len(secrets.token_urlsafe(12)) >= 12
assert 0 <= secrets.randbelow(7) < 7
assert secrets.compare_digest("same", "same")
assert not secrets.compare_digest("same", "different")

mac = hmac.new(b"key", b"message", "sha256")
assert mac.hexdigest() == (
    "6e9ef29b75fffc5b7abae527d58fdadb2fe42e7219011976917343065f58ed4a"
)
assert hmac.compare_digest(mac.digest(), hmac.new(b"key", b"message", "sha256").digest())
assert hmac.new(b"key", digestmod="sha256").update(b"message") is None

# Adapted from Lib/test/test_getpass.py without interactive terminal input.
source = io.StringIO("hidden\n")
display = io.StringIO()
assert getpass._raw_input("Password: ", display, input=source) == "hidden"
assert display.getvalue() == "Password: "
getpass._check_echo_char("*")
for invalid in ("", "**", "\\n", 1):
    try:
        getpass._check_echo_char(invalid)
    except (TypeError, ValueError):
        pass
    else:
        raise AssertionError("invalid echo character accepted")

# Adapted from Lib/test/test_gettext.py.
translations = gettext.NullTranslations()
assert translations.gettext("plain") == "plain"
assert translations.ngettext("one", "many", 1) == "one"
assert translations.ngettext("one", "many", 2) == "many"
assert translations.pgettext("context", "message") == "message"

# Adapted from Lib/test/test_locale.py.  The PS5 runtime exposes the C locale.
assert locale.setlocale(locale.LC_ALL, "C") == "C"
assert locale.localeconv()["decimal_point"] == "."
assert locale.format_string("%.2f", 12.5) == "12.50"
assert isinstance(locale.getpreferredencoding(False), str)

# Adapted from Lib/test/test_unicodedata.py.
assert unicodedata.name("A") == "LATIN CAPITAL LETTER A"
assert unicodedata.category("A") == "Lu"
assert unicodedata.lookup("SNOWMAN") == "\u2603"
assert unicodedata.normalize("NFC", "e" + chr(0x0301)) == chr(0x00e9)

print("test_secrets: PASS")
