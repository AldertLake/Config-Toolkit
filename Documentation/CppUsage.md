# C++ Usage

Config Toolkit is primarily a Blueprint plugin. The wildcard value and array nodes use custom thunk reflection so Blueprint pins can decide the serialized type at runtime.

## Include

```cpp
#include "ConfigToolkitBPLibrary.h"
```

## Safe Direct C++ Calls

These functions are normal C++ APIs and can be called directly:

```cpp
const FString FileName = TEXT("Game");

const bool bFileExists = UConfigToolkitBPLibrary::DoesConfigFileExist(FileName);
const TArray<FString> Sections = UConfigToolkitBPLibrary::GetConfigSections(FileName);
const bool bKeyExists = UConfigToolkitBPLibrary::DoesConfigKeyExist(
	TEXT("Player"),
	TEXT("Speed"),
	FileName
);

const bool bRemovedSection = UConfigToolkitBPLibrary::RemoveConfigSection(
	TEXT("Player"),
	FileName
);

const bool bDeletedFile = UConfigToolkitBPLibrary::DeleteConfigFile(FileName);
const bool bFlushed = UConfigToolkitBPLibrary::FlushConfig(FileName);
```

Passing an empty section to `DoesConfigKeyExist` searches all sections in the resolved config file:

```cpp
const bool bKeyExistsAnywhere = UConfigToolkitBPLibrary::DoesConfigKeyExist(
	TEXT(""),
	TEXT("UnlockedSkin"),
	TEXT("Game")
);
```

## Soft Reference Helpers

These helpers convert between loaded references, soft path strings, and soft references. They do not synchronously load assets or classes.

```cpp
FString AssetPath = UConfigToolkitBPLibrary::ConvertAssetToPath(SomeObject);
FString ClassPath = UConfigToolkitBPLibrary::ConvertClassToPath(SomeClass);
```

```cpp
TSoftObjectPtr<UObject> SoftAsset;
const bool bValidAssetPath = UConfigToolkitBPLibrary::ConvertPathToSoftAssetReference(
	TEXT("/Game/Items/BP_Item.BP_Item"),
	SoftAsset
);

TSoftClassPtr<UObject> SoftClass;
const bool bValidClassPath = UConfigToolkitBPLibrary::ConvertPathToSoftClassReference(
	TEXT("/Game/UI/WBP_Menu.WBP_Menu_C"),
	SoftClass
);
```

The bool only means the path string could be represented as a soft reference path. It does not mean the asset was loaded.

## Encrypted Strings

Encrypted string helpers are direct C++ calls:

```cpp
const bool bWroteEncrypted = UConfigToolkitBPLibrary::WriteEncryptedString(
	TEXT("Account"),
	TEXT("Token"),
	TEXT("SecretValue"),
	TEXT("Game")
);

FString PlainText;
const bool bReadEncrypted = UConfigToolkitBPLibrary::ReadEncryptedString(
	TEXT("Account"),
	TEXT("Token"),
	PlainText,
	TEXT("Game")
);
```

The project setting `AES Encryption Key` must be exactly 32 UTF-8 bytes.

## Do Not Call Wildcard Stub Functions Directly

These Blueprint nodes use custom thunk behavior:

- `Write Config Value`
- `Read Config Value`
- `Write Config Array`
- `Read Config Array`
- `Add Unique To Config Array`
- `Remove From Config Array`

Their C++ declarations use stub parameter types so Unreal can expose wildcard Blueprint pins. Calling those functions directly from C++ will hit the fallback path and return false.

For native C++ serialization, use Unreal's config APIs directly:

```cpp
const FString ConfigName = GConfig->GetConfigFilename(TEXT("Game"));

GConfig->SetString(
	TEXT("Player"),
	TEXT("Name"),
	TEXT("Aldert"),
	ConfigName
);

FString PlayerName;
const bool bReadName = GConfig->GetString(
	TEXT("Player"),
	TEXT("Name"),
	PlayerName,
	ConfigName
);

GConfig->Flush(false, ConfigName);
```

Native config arrays:

```cpp
const FString ConfigName = GConfig->GetConfigFilename(TEXT("Game"));

TArray<FString> Items;
Items.Add(TEXT("Sword"));
Items.Add(TEXT("Shield"));

GConfig->SetArray(TEXT("Inventory"), TEXT("Items"), Items, ConfigName);

TArray<FString> LoadedItems;
const int32 Count = GConfig->GetArray(
	TEXT("Inventory"),
	TEXT("Items"),
	LoadedItems,
	ConfigName
);
```

## File Names In C++

For bare config names, prefer Unreal's own resolver:

```cpp
const FString GameConfig = GConfig->GetConfigFilename(TEXT("Game"));
const FString CustomConfig = GConfig->GetConfigFilename(TEXT("MyConfig"));
```

This lets Unreal place generated config files in the correct editor or packaged platform directory.

## Flush Behavior

The `Automatically Flush Config` project setting controls Config Toolkit nodes. It does not change how your own direct `GConfig` calls behave.

If you write with `GConfig` directly, call:

```cpp
GConfig->Flush(false, ConfigName);
```

when you need the value written to disk immediately.
