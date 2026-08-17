"""Import-free checks for Python dictionary values and operations."""


config = {"host": "ps5", "port": 9021}
assert len(config) == 2
assert config["host"] == "ps5"
assert config.get("missing") is None
assert config.get("missing", "default") == "default"
assert "port" in config
assert "path" not in config

config["path"] = "/data/python"
assert config["path"] == "/data/python"
config.update({"port": 3232, "debug": True})
assert config == {"host": "ps5", "port": 3232, "path": "/data/python", "debug": True}
assert set(config.keys()) == {"host", "port", "path", "debug"}
assert config.pop("debug") is True
assert "debug" not in config

counts = {letter: letter * 2 for letter in "abc"}
assert counts == {"a": "aa", "b": "bb", "c": "cc"}
assert {"a": 1, "b": 2} == {"b": 2, "a": 1}

print("PASS: dictionary core values")
