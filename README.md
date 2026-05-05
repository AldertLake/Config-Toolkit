<img src="./Resources/Icon128.png" alt="Screenshot" width="600">

# Configuration Toolkit

Config Toolkit is an Unreal Engine Blueprint-focused config plugin for reading and writing `.ini` values, arrays, encrypted strings, and soft asset/class references.

Start here:

- [Documentation overview](https://aldertlake-docs.vercel.app/docs/config-toolkit)
- [Buy The Plugin](Documentation/BlueprintUsage.md)
- [Download The Plugin](https://github.com/AldertLake/Config-Toolkit/releases)

Key behavior:

- Soft Object Reference and Soft Class Reference pins can save/read path strings without synchronous loading.
- Hard object/class reads are rejected by design; read soft references and use Unreal's async load nodes.
- Normal `String` pins are preserved exactly and are never guessed as asset paths.
- Config file deletion, key existence checks, section removal, and manual flush workflows are exposed to Blueprint.
