# Troubleshooting

Config Toolkit logs detailed messages through `LogConfigToolkit`.

Open the Unreal Output Log and search for:

```text
LogConfigToolkit
```

The log usually includes the operation name, resolved config name, disk file path, section, key, and raw value when useful.

## Write Config Value Returns False

Common causes:

- `Section` is empty.
- `Key` is empty.
- The wildcard value pin could not be serialized.
- The target config directory could not be created.
- The config file could not be loaded or created in `GConfig`.
- A soft reference pin was used while `Automatically Handle Soft Reference Paths` is disabled.
- On Unreal Engine 5.4 or newer, the target generated config does not allow saving all sections.

If the value is a normal `String`, the plugin writes it exactly as provided. It does not inspect it as an asset path.

For generated configs such as `Game.ini`, add this to `Config/DefaultGame.ini`:

```ini
[SectionsToSave]
bCanSaveAllSections=true
```

## Read Config Value Returns False

Common causes:

- The file does not exist.
- The section does not exist.
- The key does not exist.
- The saved text cannot be imported into the connected output pin type.
- A hard object or hard class output pin was used.
- A soft reference output pin was used while `Automatically Handle Soft Reference Paths` is disabled.

Example: reading `Speed=Fast` into a float output pin fails because `Fast` is not a valid float.

## Hard Object Or Class Read Fails

This is expected behavior.

Config Toolkit does not synchronously load assets or classes inside a getter node. Hard object and hard class output pins would require loading, so the read returns false and logs a message explaining the soft-reference workflow.

Use this pattern instead:

1. Read into a `Soft Object Reference` or `Soft Class Reference` pin.
2. Branch on `Success`.
3. Use Unreal's native async load node when you need the loaded asset or class.

## Soft Object Or Class Reads Return Empty

Check these points:

- `Automatically Handle Soft Reference Paths` is enabled.
- The output pin is a Soft Object Reference or Soft Class Reference, not a hard object/class pin.
- The stored value is a valid soft path string.
- You are reading the same section, key, and file name that you wrote.

Manual conversion nodes can help debug path strings:

- `Convert Path To Asset`
- `Convert Path To Class`

Both return `Success` without loading the target.

## Normal Strings Look Like Paths

Normal `String` pins are never auto-converted.

This is intentional. A string like `/Game/Weapons/Rifle.Rifle` remains a string when the pin type is `String`. Soft-reference behavior only happens when the Blueprint pin type is a soft object/class reference and the project setting is enabled.

## Read Config Array Returns False

Common causes:

- The file does not exist.
- The section does not exist.
- The key does not exist.
- There are no native array entries.
- At least one entry cannot be imported into the connected array element type.
- The array element is a hard object/class type on read.

Native config arrays look like this:

```ini
[Inventory]
Items=Sword
Items=Shield
```

Unreal default config merge files can use `+Items=Value`, but generated saved config files commonly appear as repeated `Items=Value` lines. Both are normal Unreal config patterns depending on where the data is stored.

Empty arrays are an Unreal config limitation in this format. There is no explicit native empty-array marker, so reading an empty saved array returns false.

## Does Config Key Exist Returns False

Check the section behavior:

- If `Section` is non-empty, only that section is checked.
- If `Section` is empty, every section in the file is searched.

Also verify the resolved file path. If `Automatically Flush Config` is disabled and you have not called `Flush Config`, the file on disk might not contain the latest cached changes yet.

## Does Config File Exist Returns False

This does not always mean something is broken.

It can return false when:

- No write has happened yet.
- `Automatically Flush Config` is disabled and `Flush Config` has not been called.
- The file name points to a different config file than expected.
- The project is looking in a platform-specific folder such as `WindowsEditor`.

Check the log. It prints both the config name and resolved disk file path.

## Delete Config File Returns False

Common causes:

- The resolved `.ini` file does not exist.
- The file is read-only.
- Another process has locked the file.
- The file name could not be resolved to a valid disk path.

The node removes cached config data when deletion succeeds so stale in-memory values do not recreate the file on a later flush.

## Remove Config Section Did Not Change The Disk File

If `Automatically Flush Config` is disabled, section removal updates the config cache first. Call `Flush Config` for the same file to write the removal to disk.

This also applies to `Clear Config Section` and `Clear Config Key`.

## Encrypted String Fails

Common causes:

- `AES Encryption Key` is empty.
- The key is not exactly 32 characters and 32 UTF-8 bytes.
- The stored value is not valid Base64.
- The stored value was encrypted with a different key.

Only values are encrypted. Section names and key names are still plain text.

## Values Work In Editor But Not Packaged Game

Check the resolved disk path in the log.

In editor, Unreal commonly uses:

```text
Saved/Config/WindowsEditor/
```

In packaged Windows builds, Unreal commonly uses:

```text
Saved/Config/Windows/
```

Also verify that the packaged game can write to the target directory. Avoid absolute paths inside protected install folders.

## Best Debug Flow

1. Enable `Automatically Flush Config`.
2. Enable `Automatically Handle Soft Reference Paths` if testing soft object/class pins.
3. Call the write node.
4. Check the returned `Success` bool.
5. If false, read `LogConfigToolkit`.
6. Call `Does Config File Exist`.
7. Call `Does Config Key Exist`.
8. Inspect the resolved `.ini` file path printed in the log.
