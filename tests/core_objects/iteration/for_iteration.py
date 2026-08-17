"""Import-free checks for for-loop iteration semantics."""


def run():
    total = 0
    for number in (2, 4, 6, 8):
        total += number
    assert total == 20

    visited = []
    for number in range(7):
        if number % 2 == 0:
            continue
        visited.append(number)
    assert visited == [1, 3, 5]

    stopped = []
    for number in range(10):
        if number == 4:
            break
        stopped.append(number)
    assert stopped == [0, 1, 2, 3]

    characters = "PS5"
    rebuilt = ""
    for character in characters:
        rebuilt += character.lower()
    assert rebuilt == "ps5"

    keys = []
    values = []
    for key, value in (("language", "python"), ("target", "ps5")):
        keys.append(key)
        values.append(value)
    assert keys == ["language", "target"]
    assert values == ["python", "ps5"]

    searched = False
    for candidate in (3, 5, 7):
        if candidate == 11:
            searched = True
            break
    else:
        searched = False
    assert searched is False

    found = False
    for candidate in (3, 5, 7):
        if candidate == 5:
            found = True
            break
    else:
        found = False
    assert found is True

    print("PASS: for-loop iteration")


if __name__ == "__main__":
    run()
