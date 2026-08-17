"""Import-free checks for Python comprehension semantics."""


def run():
    squares = [number * number for number in range(6)]
    assert squares == [0, 1, 4, 9, 16, 25]

    even_squares = [number * number for number in range(10) if number % 2 == 0]
    assert even_squares == [0, 4, 16, 36, 64]

    coordinates = [(x, y) for x in range(2) for y in range(3)]
    assert coordinates == [(0, 0), (0, 1), (0, 2), (1, 0), (1, 1), (1, 2)]

    lengths = {word: len(word) for word in ("ps5", "python", "elf")}
    assert lengths == {"ps5": 3, "python": 6, "elf": 3}

    unique_remainders = {number % 3 for number in range(8)}
    assert unique_remainders == {0, 1, 2}

    rows = [[value + offset for value in range(3)] for offset in (10, 20)]
    assert rows == [[10, 11, 12], [20, 21, 22]]

    print("PASS: comprehensions")


if __name__ == "__main__":
    run()
