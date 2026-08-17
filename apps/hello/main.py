from greeting import build_message


app_path = __file__.replace("\\", "/")
asset_path = app_path.rsplit("/", 1)[0] + "/assets/message.txt"
with open(asset_path, "r") as asset_file:
    asset_message = asset_file.read().strip()

message = build_message(asset_message)
print(message)
assert message == "Hello from a packaged Python app on PS5"
