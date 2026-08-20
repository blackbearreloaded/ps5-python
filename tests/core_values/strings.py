"""Import-free checks for core string operations."""


def main():
    word = "python"
    assert len(word) == 6
    assert word[0] == "p"
    assert word[-1] == "n"
    assert word[1:4] == "yth"
    assert word[::-1] == "nohtyp"

    assert "PS5" + " Python" == "PS5 Python"
    assert "ha" * 3 == "hahaha"
    assert "Python" in "Python-PS5"
    assert "java" not in "Python-PS5"
    assert "  padded  ".strip() == "padded"
    assert "mixed".upper() == "MIXED"
    assert "MIXED".lower() == "mixed"
    assert "hello world".title() == "Hello World"

    assert "a,b,c".split(",") == ["a", "b", "c"]
    assert " ".join(["PS5", "Python"]) == "PS5 Python"
    assert "one two one".replace("one", "1") == "1 two 1"
    assert "abcabc".find("ca") == 2
    assert "abcabc".count("ab") == 2
    assert "filename.py".endswith(".py")
    assert "filename.py".startswith("file")

    assert "value={}".format(42) == "value=42"
    assert "{name}:{answer}".format(name="PS5", answer=42) == "PS5:42"

    print("strings: PASS")


if __name__ == "__main__":
    main()
