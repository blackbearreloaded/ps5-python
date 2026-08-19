"""Temporary PS5 bootstrap codec package for the core proof of concept."""

import codecs


def search_function(encoding):
    normalized = encoding.replace('-', '_').replace(' ', '_').lower()
    if normalized not in (
        'utf_8',
        'ascii',
        'ansi_x3_4_1968',
        'iso_646_irv_1991',
        'iso_ir_6',
        'csascii',
        'us_ascii',
        '646',
        'idna',
        'cp437',
    ):
        return None
    if normalized == 'utf_8':
        from . import utf_8
        return utf_8.getregentry()
    if normalized == 'idna':
        from . import idna
        return idna.getregentry()
    if normalized == 'cp437':
        from . import cp437
        return cp437.getregentry()
    from . import ascii
    return ascii.getregentry()


codecs.register(search_function)
