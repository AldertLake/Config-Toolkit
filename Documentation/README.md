# Config Toolkit Documentation

Config Toolkit is an Unreal Engine plugin for reading and writing Unreal `.ini` config files from Blueprint. It supports wildcard value pins, native config arrays, encrypted strings, soft asset/class references, and config utility operations.

## Install And Setup First

1. Copy or keep the plugin folder at:

```text
YourProject/Plugins/ConfigToolkit
```

2. Open the project in Unreal Engine.
3. Open `Edit > Plugins`, search for `Config Toolkit`, and enable it if it is not already enabled.
4. Restart the editor if Unreal asks.
5. Open:

```text
Project Settings > Plugins > Config Toolkit
```

6. Configure the core settings:

- `Default Config File Name`: the config file used when a node's `File Name` pin is empty. Default is `Game`.
- `Automatically Flush Config`: if enabled, write/remove/delete-style operations flush to disk immediately when appropriate.
- `Automatically Handle Soft Reference Paths`: if enabled, wildcard value and array nodes read/write Soft Object Reference and Soft Class Reference pins as path strings.
- `AES Encryption Key`: required only for encrypted string nodes. It must be exactly 32 UTF-8 bytes.

7. For Unreal Engine 5.4 and newer, allow generated config sections to be saved. Add this to the project config file that owns the generated config you want to write, commonly `Config/DefaultGame.ini` when using `Game.ini`:

```ini
[SectionsToSave]
bCanSaveAllSections=true
```

Without this, Unreal can reject saving sections in generated configs such as `Game.ini` or `Engine.ini`, even when the Blueprint node reports that the in-memory config value was written.

## Recommended First Blueprint Test

1. Keep `Default Config File Name` set to `Game`.
2. Keep `Automatically Flush Config` enabled.
3. Add a `Write Config Value` node:

```text
Section: Player
Key: Speed
Value: 600.0
File Name: empty
```

4. Add a `Read Config Value` node with the same section/key.
5. Connect the read output to a float variable or print it.
6. Branch on the `Success` bool.

Expected config output:

```ini
[Player]
Speed=600.0
```

## Documentation Files

- [Settings](Settings.md): all Project Settings and recommended defaults.
- [Blueprint Usage](BlueprintUsage.md): Blueprint node examples and screenshots.
- [C++ Usage](CppUsage.md): safe C++ calls and when to use `GConfig` directly.
- [Troubleshooting](Troubleshooting.md): common failures and what each return bool means.

## Main Concepts

Config Toolkit writes through Unreal's `GConfig` system. Bare file names such as `Game` or `MyConfig` resolve to generated platform config files. In editor on Windows, the disk path is usually:

```text
Saved/Config/WindowsEditor/Game.ini
```

In packaged Windows builds, it is usually:

```text
Saved/Config/Windows/Game.ini
```

The plugin does not synchronously load assets or classes when reading config values. Asset and class config data is stored as soft reference paths. If you need a loaded asset or class after reading, read into a Soft Object Reference or Soft Class Reference, then use Unreal's async loading nodes.
