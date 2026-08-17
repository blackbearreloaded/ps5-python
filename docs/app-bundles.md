# Python app bundles

The project supports multiple Python applications under one shared runtime.
Each application has its own entry script, pure-Python libraries, assets, and
metadata.

```text
/data/python/
├── runtime/
│   ├── python.elf
│   └── cpython-lib/
└── apps/
    └── hello/
        ├── app.json
        ├── main.py
        ├── lib/
        └── assets/
```

The local source layout mirrors the PS5 layout:

```text
apps/hello/
├── app.json
├── main.py
├── lib/
└── assets/
```

## Manifest

The first manifest format is intentionally small:

```json
{
  "id": "hello",
  "name": "Hello Python App",
  "version": "0.1.0",
  "entry": "main.py",
  "runtime": "shared"
}
```

The deployment tool currently requires `entry` and rejects absolute or parent
directory paths. The other fields are metadata for the future native app
launcher.

## Module and asset paths

The native launcher adds both of these paths to CPython's module search path:

```text
/data/python/apps/<id>/
/data/python/apps/<id>/lib/
```

This allows the entry script to import app-local modules:

```python
from greeting import build_message
```

The launcher also sets `__file__` to the real PS5 entry path. Applications
should derive asset paths from it rather than assuming a current working
directory:

```python
app_dir = __file__.replace("\\", "/").rsplit("/", 1)[0]
asset_path = app_dir + "/assets/message.txt"
```

## Build and deploy

Validate an app on the host:

```sh
make host-app APP=apps/hello
```

Deploy it to the PS5:

```sh
PS5_HOST=192.168.4.30 make ps5-app APP=apps/hello
```

The app mode stores the shared interpreter under `/data/python/runtime/` and
the selected bundle under `/data/python/apps/<id>/`. The normal `ps5-run`
target remains available for one-off scripts.
