"""Import-free checks for Unicode literals and string behavior."""


def main():
    greeting = "Olá, mundo 🌍"
    assert greeting == "Olá, mundo \U0001F30D"
    assert len(greeting) == 12
    assert greeting[0] == "O"
    assert greeting[2] == "á"
    assert greeting[-1] == "🌍"

    assert "café" == "cafe\u0301".replace("e\u0301", "é")
    assert "é".encode("utf-8") == b"\xc3\xa9"
    assert b"\xf0\x9f\x8c\x8d".decode("utf-8") == "🌍"
    assert "\u03bb" == "λ"
    assert "\U0001F680" == "🚀"
    assert ord("A") == 65
    assert chr(9731) == "☃"

    assert "Straße".upper() == "STRASSE"
    assert "CAFÉ".lower() == "café"
    assert "Σίσυφος".casefold() == "σίσυφοσ"
    assert "mañana"[1:4] == "aña"
    assert "日本語" in "こんにちは日本語"
    assert "🙂" * 3 == "🙂🙂🙂"

    print("unicode: PASS")


if __name__ == "__main__":
    main()
