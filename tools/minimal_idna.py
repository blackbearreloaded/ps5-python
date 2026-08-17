"""Minimal IDNA codec for ASCII DNS hostnames on PS5.

This intentionally supports ordinary ASCII DNS names only. Full Unicode
IDNA2008/Punycode is outside the first PS5 runtime bundle.
"""

import codecs


def _encode(input, errors="strict"):
    if not isinstance(input, str):
        raise TypeError("encoding target must be str")
    try:
        value = input.encode("ascii", errors)
    except UnicodeEncodeError:
        raise UnicodeError("non-ASCII IDNA names are not supported on PS5")
    return value, len(input)


def _decode(input, errors="strict"):
    value = bytes(input).decode("ascii", errors)
    return value, len(input)


def _incremental_encoder(errors="strict"):
    return codecs.getincrementalencoder("ascii")(errors)


def _incremental_decoder(errors="strict"):
    return codecs.getincrementaldecoder("ascii")(errors)


def getregentry():
    return codecs.CodecInfo(
        name="idna",
        encode=_encode,
        decode=_decode,
        incrementalencoder=_incremental_encoder,
        incrementaldecoder=_incremental_decoder,
        streamwriter=codecs.getwriter("ascii"),
        streamreader=codecs.getreader("ascii"),
    )
