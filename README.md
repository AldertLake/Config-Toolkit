# Config Toolkit

Config Toolkit is an Unreal Engine Blueprint-focused config plugin for reading and writing `.ini` values, arrays, encrypted strings, and soft asset/class references.

Start here:

- [Documentation overview](Documentation/README.md)
- [Blueprint usage](Documentation/BlueprintUsage.md)
- [Project settings](Documentation/Settings.md)
- [C++ usage](Documentation/CppUsage.md)
- [Troubleshooting](Documentation/Troubleshooting.md)

Key behavior:

- Soft Object Reference and Soft Class Reference pins can save/read path strings without synchronous loading.
- Hard object/class reads are rejected by design; read soft references and use Unreal's async load nodes.
- Normal `String` pins are preserved exactly and are never guessed as asset paths.
- Config file deletion, key existence checks, section removal, and manual flush workflows are exposed to Blueprint.
