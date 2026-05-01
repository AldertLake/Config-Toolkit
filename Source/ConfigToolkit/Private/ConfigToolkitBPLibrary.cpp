#include "ConfigToolkitBPLibrary.h"

#include "ConfigToolkitSettings.h"
#include "Containers/StringConv.h"
#include "CoreGlobals.h"
#include "HAL/FileManager.h"
#include "Misc/AES.h"
#include "Misc/Base64.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"
#include "UObject/PropertyPortFlags.h"
#include "UObject/SoftObjectPath.h"
#include "UObject/Stack.h"
#include "UObject/UnrealType.h"

DEFINE_LOG_CATEGORY_STATIC(LogConfigToolkit, Log, All);

namespace ConfigToolkit::Private
{
	constexpr int32 AESKeyByteLength = FAES::FAESKey::KeySize;
	constexpr int32 PlaintextLengthHeaderBytes = sizeof(int32);

	FString GetDefaultConfigFilename()
	{
		const UConfigToolkitSettings* Settings = GetDefault<UConfigToolkitSettings>();
		const FString ConfigName = Settings ? Settings->DefaultConfigFilename.TrimStartAndEnd() : FString();
		return ConfigName.IsEmpty() ? FString(TEXT("Game")) : ConfigName;
	}

	bool ShouldAutomaticallyFlushConfig()
	{
		const UConfigToolkitSettings* Settings = GetDefault<UConfigToolkitSettings>();
		return !Settings || Settings->bAutomaticallyFlushConfig;
	}

	FString ResolveConfigFilename(const FString& Filename)
	{
		FString Resolved = Filename.TrimStartAndEnd();
		if (Resolved.IsEmpty())
		{
			Resolved = GetDefaultConfigFilename();
		}

		const bool bHasDirectory = !FPaths::GetPath(Resolved).IsEmpty();
		if (!bHasDirectory)
		{
			const FString BaseName = FPaths::GetBaseFilename(Resolved);
			if (GConfig)
			{
				FString ConfigFilename = GConfig->GetConfigFilename(*BaseName);
				return ConfigFilename.EndsWith(TEXT(".ini"))
					? FConfigCacheIni::NormalizeConfigIniPath(ConfigFilename)
					: ConfigFilename;
			}

			return FConfigCacheIni::NormalizeConfigIniPath(FConfigCacheIni::GetDestIniFilename(*BaseName, nullptr, *FPaths::GeneratedConfigDir()));
		}

		if (FPaths::GetExtension(Resolved, false).IsEmpty())
		{
			Resolved += TEXT(".ini");
		}

		if (FPaths::IsRelative(Resolved))
		{
			Resolved = FPaths::Combine(FPaths::GeneratedConfigDir(), Resolved);
		}

		return FConfigCacheIni::NormalizeConfigIniPath(Resolved);
	}

	bool IsConfigCachePath(const FString& ResolvedFilename)
	{
		return ResolvedFilename.EndsWith(TEXT(".ini"));
	}

	FString GetDiskConfigFilename(const FString& ResolvedFilename)
	{
		if (IsConfigCachePath(ResolvedFilename))
		{
			return FConfigCacheIni::NormalizeConfigIniPath(ResolvedFilename);
		}

		return FConfigCacheIni::NormalizeConfigIniPath(FConfigCacheIni::GetDestIniFilename(*ResolvedFilename, nullptr, *FPaths::GeneratedConfigDir()));
	}

	bool ValidateSectionAndKey(const TCHAR* Operation, const FString& Section, const FString& Key)
	{
		if (Section.TrimStartAndEnd().IsEmpty())
		{
			UE_LOG(LogConfigToolkit, Warning, TEXT("%s failed: Section is empty."), Operation);
			return false;
		}

		if (Key.TrimStartAndEnd().IsEmpty())
		{
			UE_LOG(LogConfigToolkit, Warning, TEXT("%s failed: Key is empty. Section='%s'."), Operation, *Section);
			return false;
		}

		return true;
	}

	bool ValidateSection(const TCHAR* Operation, const FString& Section)
	{
		if (Section.TrimStartAndEnd().IsEmpty())
		{
			UE_LOG(LogConfigToolkit, Warning, TEXT("%s failed: Section is empty."), Operation);
			return false;
		}

		return true;
	}

	bool HasConfig()
	{
		if (!GConfig)
		{
			UE_LOG(LogConfigToolkit, Error, TEXT("GConfig is not available."));
			return false;
		}

		return true;
	}

	bool DoesResolvedConfigFileExist(const FString& ResolvedFilename)
	{
		return IFileManager::Get().FileExists(*GetDiskConfigFilename(ResolvedFilename));
	}

	bool EnsureConfigFileReadyForWrite(const TCHAR* Operation, const FString& ResolvedFilename, const bool bCreateIfMissing)
	{
		if (!HasConfig())
		{
			return false;
		}

		const FString DiskFilename = GetDiskConfigFilename(ResolvedFilename);
		const FString Directory = FPaths::GetPath(DiskFilename);
		if (!Directory.IsEmpty() && !IFileManager::Get().DirectoryExists(*Directory))
		{
			if (!IFileManager::Get().MakeDirectory(*Directory, true))
			{
				UE_LOG(LogConfigToolkit, Error, TEXT("%s failed: Could not create config directory. File='%s', Directory='%s'."),
					Operation, *DiskFilename, *Directory);
				return false;
			}
		}

		FConfigFile* ConfigFile = GConfig->FindConfigFile(ResolvedFilename);
		if (!ConfigFile && IsConfigCachePath(ResolvedFilename))
		{
			if (IFileManager::Get().FileExists(*DiskFilename))
			{
				GConfig->LoadFile(ResolvedFilename);
			}
			else if (bCreateIfMissing)
			{
				FConfigFile NewConfigFile;
				NewConfigFile.Name = FName(*FPaths::GetBaseFilename(DiskFilename));
				GConfig->Add(ResolvedFilename, NewConfigFile);
			}

			ConfigFile = GConfig->FindConfigFile(ResolvedFilename);
		}

		if (!ConfigFile)
		{
			UE_LOG(LogConfigToolkit, Warning, TEXT("%s failed: Config file is not loaded in GConfig%s. ConfigName='%s', File='%s'."),
				Operation,
				bCreateIfMissing ? TEXT(" and could not be created") : TEXT(""),
				*ResolvedFilename,
				*DiskFilename);
			return false;
		}

		ConfigFile->NoSave = false;
		return true;
	}

	bool FinalizeConfigWrite(const TCHAR* Operation, const FString& ResolvedFilename, const bool bAllowMissingAfterFlush = false)
	{
		if (!ShouldAutomaticallyFlushConfig())
		{
			return true;
		}

		if (!HasConfig())
		{
			return false;
		}

		const FString DiskFilename = GetDiskConfigFilename(ResolvedFilename);
		GConfig->Flush(false, ResolvedFilename);

		if (!DoesResolvedConfigFileExist(ResolvedFilename))
		{
			if (bAllowMissingAfterFlush)
			{
				UE_LOG(LogConfigToolkit, Log, TEXT("%s completed: Config file is not present after flush because the operation left no values to save. ConfigName='%s', File='%s'."),
					Operation, *ResolvedFilename, *DiskFilename);
				return true;
			}

			UE_LOG(LogConfigToolkit, Warning, TEXT("%s failed: GConfig accepted the write but no config file exists on disk after flush. ConfigName='%s', File='%s'. This can happen when the written array is empty or config writes are globally disabled."),
				Operation, *ResolvedFilename, *DiskFilename);
			return false;
		}

		return true;
	}

	bool DoesConfigSectionExist(const FString& Section, const FString& ResolvedFilename)
	{
		return GConfig && GConfig->DoesSectionExist(*Section, ResolvedFilename);
	}

	bool DoesConfigKeyExist(const FString& Section, const FString& Key, const FString& ResolvedFilename)
	{
		if (!GConfig)
		{
			return false;
		}

		const FConfigSection* ConfigSection = GConfig->GetSection(*Section, false, ResolvedFilename);
		return ConfigSection && ConfigSection->Contains(FName(*Key));
	}

	void LogMissingConfigLocation(const TCHAR* Operation, const FString& Section, const FString& Key, const FString& ResolvedFilename)
	{
		const FString DiskFilename = GetDiskConfigFilename(ResolvedFilename);
		if (!DoesResolvedConfigFileExist(ResolvedFilename))
		{
			UE_LOG(LogConfigToolkit, Warning, TEXT("%s failed: Config file does not exist. ConfigName='%s', File='%s', Section='%s', Key='%s'."),
				Operation, *ResolvedFilename, *DiskFilename, *Section, *Key);
			return;
		}

		if (!DoesConfigSectionExist(Section, ResolvedFilename))
		{
			UE_LOG(LogConfigToolkit, Warning, TEXT("%s failed: Section was not found. ConfigName='%s', File='%s', Section='%s', Key='%s'."),
				Operation, *ResolvedFilename, *DiskFilename, *Section, *Key);
			return;
		}

		if (!DoesConfigKeyExist(Section, Key, ResolvedFilename))
		{
			UE_LOG(LogConfigToolkit, Warning, TEXT("%s failed: Key was not found. ConfigName='%s', File='%s', Section='%s', Key='%s'."),
				Operation, *ResolvedFilename, *DiskFilename, *Section, *Key);
			return;
		}

		UE_LOG(LogConfigToolkit, Warning, TEXT("%s failed: GConfig could not read the value even though the file, section, and key exist. ConfigName='%s', File='%s', Section='%s', Key='%s'."),
			Operation, *ResolvedFilename, *DiskFilename, *Section, *Key);
	}

	void LogMissingConfigSection(const TCHAR* Operation, const FString& Section, const FString& ResolvedFilename)
	{
		const FString DiskFilename = GetDiskConfigFilename(ResolvedFilename);
		if (!DoesResolvedConfigFileExist(ResolvedFilename))
		{
			UE_LOG(LogConfigToolkit, Warning, TEXT("%s failed: Config file does not exist. ConfigName='%s', File='%s', Section='%s'."),
				Operation, *ResolvedFilename, *DiskFilename, *Section);
			return;
		}

		if (!DoesConfigSectionExist(Section, ResolvedFilename))
		{
			UE_LOG(LogConfigToolkit, Warning, TEXT("%s failed: Section was not found. ConfigName='%s', File='%s', Section='%s'."),
				Operation, *ResolvedFilename, *DiskFilename, *Section);
			return;
		}

		UE_LOG(LogConfigToolkit, Warning, TEXT("%s failed: GConfig could not update the section even though it exists. ConfigName='%s', File='%s', Section='%s'."),
			Operation, *ResolvedFilename, *DiskFilename, *Section);
	}

	bool ExportPropertyValueToString(const FProperty* Property, const void* ValueAddress, FString& OutValue, UObject* OwnerObject)
	{
		if (!Property || !ValueAddress)
		{
			UE_LOG(LogConfigToolkit, Warning, TEXT("Config Toolkit wildcard export failed: property or value address is invalid."));
			return false;
		}

		OutValue.Reset();
		Property->ExportTextItem_Direct(OutValue, ValueAddress, nullptr, OwnerObject, PPF_None);
		return true;
	}

	bool ImportPropertyValueFromString(const FProperty* Property, void* ValueAddress, const FString& Value, UObject* OwnerObject)
	{
		if (!Property || !ValueAddress)
		{
			UE_LOG(LogConfigToolkit, Warning, TEXT("Config Toolkit wildcard import failed: property or value address is invalid."));
			return false;
		}

		const TCHAR* Result = Property->ImportText_Direct(*Value, ValueAddress, OwnerObject, PPF_None);
		if (!Result)
		{
			UE_LOG(LogConfigToolkit, Warning, TEXT("Config Toolkit wildcard import failed: value '%s' could not be imported into property '%s' of type '%s'."),
				*Value, *Property->GetName(), *Property->GetCPPType());
			return false;
		}

		return true;
	}

	bool ValidateImportedTextValues(const FProperty* Property, const TArray<FString>& Values, UObject* OwnerObject)
	{
		if (!Property)
		{
			UE_LOG(LogConfigToolkit, Warning, TEXT("Config Toolkit array import failed: array inner property is invalid."));
			return false;
		}

		for (const FString& Value : Values)
		{
			FDefaultConstructedPropertyElement TempValue(Property);
			if (!ImportPropertyValueFromString(Property, TempValue.GetObjAddress(), Value, OwnerObject))
			{
				return false;
			}
		}

		return true;
	}

	bool ExportArrayToStrings(const FArrayProperty* ArrayProperty, void* ArrayAddress, TArray<FString>& OutValues, UObject* OwnerObject)
	{
		if (!ArrayProperty || !ArrayAddress)
		{
			UE_LOG(LogConfigToolkit, Warning, TEXT("Config Toolkit array export failed: array property or address is invalid."));
			return false;
		}

		FScriptArrayHelper ArrayHelper(ArrayProperty, ArrayAddress);
		const FProperty* InnerProperty = ArrayProperty->Inner;
		OutValues.Reset(ArrayHelper.Num());

		for (int32 Index = 0; Index < ArrayHelper.Num(); ++Index)
		{
			FString ExportedValue;
			if (!ExportPropertyValueToString(InnerProperty, ArrayHelper.GetRawPtr(Index), ExportedValue, OwnerObject))
			{
				return false;
			}

			OutValues.Add(MoveTemp(ExportedValue));
		}

		return true;
	}

	bool ImportStringsToArray(const FArrayProperty* ArrayProperty, void* ArrayAddress, const TArray<FString>& Values, UObject* OwnerObject)
	{
		if (!ArrayProperty || !ArrayAddress)
		{
			UE_LOG(LogConfigToolkit, Warning, TEXT("Config Toolkit array import failed: array property or address is invalid."));
			return false;
		}

		const FProperty* InnerProperty = ArrayProperty->Inner;
		if (!ValidateImportedTextValues(InnerProperty, Values, OwnerObject))
		{
			UE_LOG(LogConfigToolkit, Warning, TEXT("Config Toolkit array import failed: one or more values could not be converted to array element type '%s'."),
				InnerProperty ? *InnerProperty->GetCPPType() : TEXT("<invalid>"));
			return false;
		}

		FScriptArrayHelper ArrayHelper(ArrayProperty, ArrayAddress);
		ArrayHelper.Resize(Values.Num());

		for (int32 Index = 0; Index < Values.Num(); ++Index)
		{
			if (!ImportPropertyValueFromString(InnerProperty, ArrayHelper.GetRawPtr(Index), Values[Index], OwnerObject))
			{
				ArrayHelper.EmptyValues();
				return false;
			}
		}

		return true;
	}

	bool IsSerializedValueEquivalent(const FProperty* Property, const FString& SerializedValue, const void* ValueAddress, UObject* OwnerObject)
	{
		if (!Property || !ValueAddress)
		{
			return false;
		}

		FDefaultConstructedPropertyElement TempValue(Property);
		return ImportPropertyValueFromString(Property, TempValue.GetObjAddress(), SerializedValue, OwnerObject)
			&& Property->Identical(TempValue.GetObjAddress(), ValueAddress, PPF_None);
	}

	bool GetAESKey(FAES::FAESKey& OutKey)
	{
		const UConfigToolkitSettings* Settings = GetDefault<UConfigToolkitSettings>();
		const FString KeyString = Settings ? Settings->AESEncryptionKey : FString();
		const FTCHARToUTF8 KeyBytes(*KeyString);

		if (KeyString.Len() != AESKeyByteLength || KeyBytes.Length() != AESKeyByteLength)
		{
			UE_LOG(LogConfigToolkit, Warning, TEXT("Config Toolkit AES Encryption Key must be exactly %d characters and %d UTF-8 bytes. Current length: %d characters, %d bytes."), AESKeyByteLength, AESKeyByteLength, KeyString.Len(), KeyBytes.Length());
			return false;
		}

		OutKey.Reset();
		FMemory::Memcpy(OutKey.Key, reinterpret_cast<const uint8*>(KeyBytes.Get()), AESKeyByteLength);
		return true;
	}

	TArray<uint8> StringToUTF8Bytes(const FString& Value)
	{
		const FTCHARToUTF8 Converted(*Value);

		TArray<uint8> Bytes;
		Bytes.SetNumUninitialized(Converted.Length());
		if (Converted.Length() > 0)
		{
			FMemory::Memcpy(Bytes.GetData(), reinterpret_cast<const uint8*>(Converted.Get()), Converted.Length());
		}

		return Bytes;
	}

	FString UTF8BytesToString(const uint8* Bytes, const int32 ByteCount)
	{
		if (ByteCount <= 0)
		{
			return FString();
		}

		const FUTF8ToTCHAR Converted(reinterpret_cast<const ANSICHAR*>(Bytes), ByteCount);
		return FString(Converted.Length(), Converted.Get());
	}

	bool EncryptStringToBase64(const FString& Plaintext, FString& OutBase64)
	{
		FAES::FAESKey Key;
		if (!GetAESKey(Key))
		{
			return false;
		}

		const TArray<uint8> PlaintextBytes = StringToUTF8Bytes(Plaintext);
		const int32 PlaintextByteCount = PlaintextBytes.Num();
		const int32 UnpaddedByteCount = PlaintextLengthHeaderBytes + PlaintextByteCount;
		const int32 PaddedByteCount = Align(UnpaddedByteCount, static_cast<int32>(FAES::AESBlockSize));

		TArray<uint8> EncryptedBytes;
		EncryptedBytes.SetNumZeroed(PaddedByteCount);
		FMemory::Memcpy(EncryptedBytes.GetData(), &PlaintextByteCount, PlaintextLengthHeaderBytes);
		if (PlaintextByteCount > 0)
		{
			FMemory::Memcpy(EncryptedBytes.GetData() + PlaintextLengthHeaderBytes, PlaintextBytes.GetData(), PlaintextByteCount);
		}

		FAES::EncryptData(EncryptedBytes.GetData(), EncryptedBytes.Num(), Key);
		OutBase64 = FBase64::Encode(EncryptedBytes);
		return true;
	}

	bool DecryptBase64ToString(const FString& Base64, FString& OutPlaintext)
	{
		FAES::FAESKey Key;
		if (!GetAESKey(Key))
		{
			return false;
		}

		TArray<uint8> EncryptedBytes;
		if (!FBase64::Decode(Base64, EncryptedBytes))
		{
			UE_LOG(LogConfigToolkit, Warning, TEXT("Config Toolkit failed to decode encrypted value from Base64."));
			return false;
		}

		if (EncryptedBytes.Num() < PlaintextLengthHeaderBytes || (EncryptedBytes.Num() % FAES::AESBlockSize) != 0)
		{
			UE_LOG(LogConfigToolkit, Warning, TEXT("Config Toolkit encrypted value has invalid byte length: %d."), EncryptedBytes.Num());
			return false;
		}

		FAES::DecryptData(EncryptedBytes.GetData(), EncryptedBytes.Num(), Key);

		int32 PlaintextByteCount = 0;
		FMemory::Memcpy(&PlaintextByteCount, EncryptedBytes.GetData(), PlaintextLengthHeaderBytes);

		if (PlaintextByteCount < 0 || PlaintextByteCount > EncryptedBytes.Num() - PlaintextLengthHeaderBytes)
		{
			UE_LOG(LogConfigToolkit, Warning, TEXT("Config Toolkit decrypted value has an invalid plaintext length."));
			return false;
		}

		OutPlaintext = UTF8BytesToString(EncryptedBytes.GetData() + PlaintextLengthHeaderBytes, PlaintextByteCount);
		return true;
	}

	bool GenericWriteAnyConfigValue(const FString& Section, const FString& Key, const FProperty* ValueProperty, const void* ValueAddress, const FString& Filename, UObject* OwnerObject)
	{
		static const TCHAR* Operation = TEXT("Write Config Value");

		if (!HasConfig() || !ValidateSectionAndKey(Operation, Section, Key))
		{
			return false;
		}

		FString SerializedValue;
		if (!ExportPropertyValueToString(ValueProperty, ValueAddress, SerializedValue, OwnerObject))
		{
			UE_LOG(LogConfigToolkit, Warning, TEXT("%s failed: Could not serialize wildcard Value. Section='%s', Key='%s'."),
				Operation, *Section, *Key);
			return false;
		}

		const FString ResolvedFilename = ResolveConfigFilename(Filename);
		if (!EnsureConfigFileReadyForWrite(Operation, ResolvedFilename, true))
		{
			return false;
		}

		GConfig->SetString(*Section, *Key, *SerializedValue, ResolvedFilename);
		return FinalizeConfigWrite(Operation, ResolvedFilename);
	}

	bool GenericReadAnyConfigValue(const FString& Section, const FString& Key, const FProperty* ValueProperty, void* ValueAddress, const FString& Filename, UObject* OwnerObject)
	{
		static const TCHAR* Operation = TEXT("Read Config Value");

		if (!HasConfig() || !ValidateSectionAndKey(Operation, Section, Key))
		{
			return false;
		}

		FString SerializedValue;
		const FString ResolvedFilename = ResolveConfigFilename(Filename);
		if (!GConfig->GetString(*Section, *Key, SerializedValue, ResolvedFilename))
		{
			LogMissingConfigLocation(Operation, Section, Key, ResolvedFilename);
			return false;
		}

		if (!ImportPropertyValueFromString(ValueProperty, ValueAddress, SerializedValue, OwnerObject))
		{
			UE_LOG(LogConfigToolkit, Warning, TEXT("%s failed: Config value could not be converted to the connected output pin type. ConfigName='%s', File='%s', Section='%s', Key='%s', RawValue='%s'."),
				Operation, *ResolvedFilename, *GetDiskConfigFilename(ResolvedFilename), *Section, *Key, *SerializedValue);
			return false;
		}

		return true;
	}

	bool GenericWriteConfigArray(const FString& Section, const FString& Key, const FArrayProperty* ArrayProperty, void* ArrayAddress, const FString& Filename, UObject* OwnerObject)
	{
		static const TCHAR* Operation = TEXT("Write Config Array");

		if (!HasConfig() || !ValidateSectionAndKey(Operation, Section, Key))
		{
			return false;
		}

		TArray<FString> SerializedValues;
		if (!ExportArrayToStrings(ArrayProperty, ArrayAddress, SerializedValues, OwnerObject))
		{
			UE_LOG(LogConfigToolkit, Warning, TEXT("%s failed: Could not serialize wildcard Values array. Section='%s', Key='%s'."),
				Operation, *Section, *Key);
			return false;
		}

		const FString ResolvedFilename = ResolveConfigFilename(Filename);
		if (!EnsureConfigFileReadyForWrite(Operation, ResolvedFilename, true))
		{
			return false;
		}

		if (SerializedValues.IsEmpty())
		{
			UE_LOG(LogConfigToolkit, Warning, TEXT("%s wrote an empty array. Native config arrays do not persist an explicit empty-array marker, so a later read returns false until at least one value is saved. ConfigName='%s', File='%s', Section='%s', Key='%s'."),
				Operation, *ResolvedFilename, *GetDiskConfigFilename(ResolvedFilename), *Section, *Key);
		}

		GConfig->SetArray(*Section, *Key, SerializedValues, ResolvedFilename);
		return FinalizeConfigWrite(Operation, ResolvedFilename, SerializedValues.IsEmpty());
	}

	bool GenericReadConfigArray(const FString& Section, const FString& Key, const FArrayProperty* ArrayProperty, void* ArrayAddress, const FString& Filename, UObject* OwnerObject)
	{
		static const TCHAR* Operation = TEXT("Read Config Array");

		if (!HasConfig() || !ValidateSectionAndKey(Operation, Section, Key))
		{
			return false;
		}

		TArray<FString> SerializedValues;
		const FString ResolvedFilename = ResolveConfigFilename(Filename);
		const int32 NumValues = GConfig->GetArray(*Section, *Key, SerializedValues, ResolvedFilename);

		if (NumValues <= 0)
		{
			LogMissingConfigLocation(Operation, Section, Key, ResolvedFilename);
			return false;
		}

		if (!ImportStringsToArray(ArrayProperty, ArrayAddress, SerializedValues, OwnerObject))
		{
			UE_LOG(LogConfigToolkit, Warning, TEXT("%s failed: Config array entries could not be converted to the connected output array type. ConfigName='%s', File='%s', Section='%s', Key='%s', EntryCount=%d."),
				Operation, *ResolvedFilename, *GetDiskConfigFilename(ResolvedFilename), *Section, *Key, SerializedValues.Num());
			return false;
		}

		return true;
	}

	bool GenericAddUniqueToConfigArray(const FString& Section, const FString& Key, const FProperty* ValueProperty, const void* ValueAddress, const FString& Filename, UObject* OwnerObject)
	{
		static const TCHAR* Operation = TEXT("Add Unique To Config Array");

		if (!HasConfig() || !ValidateSectionAndKey(Operation, Section, Key))
		{
			return false;
		}

		FString SerializedValue;
		if (!ExportPropertyValueToString(ValueProperty, ValueAddress, SerializedValue, OwnerObject))
		{
			UE_LOG(LogConfigToolkit, Warning, TEXT("%s failed: Could not serialize wildcard Value. Section='%s', Key='%s'."),
				Operation, *Section, *Key);
			return false;
		}

		const FString ResolvedFilename = ResolveConfigFilename(Filename);
		if (!EnsureConfigFileReadyForWrite(Operation, ResolvedFilename, true))
		{
			return false;
		}

		TArray<FString> SerializedValues;
		GConfig->GetArray(*Section, *Key, SerializedValues, ResolvedFilename);

		for (const FString& ExistingValue : SerializedValues)
		{
			if (ExistingValue == SerializedValue || IsSerializedValueEquivalent(ValueProperty, ExistingValue, ValueAddress, OwnerObject))
			{
				UE_LOG(LogConfigToolkit, Warning, TEXT("%s returned false: Value already exists in the config array. ConfigName='%s', File='%s', Section='%s', Key='%s', Value='%s'."),
					Operation, *ResolvedFilename, *GetDiskConfigFilename(ResolvedFilename), *Section, *Key, *SerializedValue);
				return false;
			}
		}

		SerializedValues.Add(MoveTemp(SerializedValue));
		GConfig->SetArray(*Section, *Key, SerializedValues, ResolvedFilename);
		return FinalizeConfigWrite(Operation, ResolvedFilename);
	}

	bool GenericRemoveFromConfigArray(const FString& Section, const FString& Key, const FProperty* ValueProperty, const void* ValueAddress, const FString& Filename, UObject* OwnerObject)
	{
		static const TCHAR* Operation = TEXT("Remove From Config Array");

		if (!HasConfig() || !ValidateSectionAndKey(Operation, Section, Key))
		{
			return false;
		}

		FString SerializedValue;
		if (!ExportPropertyValueToString(ValueProperty, ValueAddress, SerializedValue, OwnerObject))
		{
			UE_LOG(LogConfigToolkit, Warning, TEXT("%s failed: Could not serialize wildcard Value. Section='%s', Key='%s'."),
				Operation, *Section, *Key);
			return false;
		}

		const FString ResolvedFilename = ResolveConfigFilename(Filename);
		TArray<FString> SerializedValues;
		GConfig->GetArray(*Section, *Key, SerializedValues, ResolvedFilename);

		if (SerializedValues.IsEmpty())
		{
			LogMissingConfigLocation(Operation, Section, Key, ResolvedFilename);
			return false;
		}

		const int32 RemovedCount = SerializedValues.RemoveAll([ValueProperty, ValueAddress, OwnerObject, &SerializedValue](const FString& ExistingValue)
		{
			return ExistingValue == SerializedValue || IsSerializedValueEquivalent(ValueProperty, ExistingValue, ValueAddress, OwnerObject);
		});

		if (RemovedCount > 0)
		{
			if (!EnsureConfigFileReadyForWrite(Operation, ResolvedFilename, false))
			{
				return false;
			}

			GConfig->SetArray(*Section, *Key, SerializedValues, ResolvedFilename);
			return FinalizeConfigWrite(Operation, ResolvedFilename, SerializedValues.IsEmpty());
		}
		else
		{
			UE_LOG(LogConfigToolkit, Warning, TEXT("%s returned false: Value was not found in the config array. ConfigName='%s', File='%s', Section='%s', Key='%s', Value='%s'."),
				Operation, *ResolvedFilename, *GetDiskConfigFilename(ResolvedFilename), *Section, *Key, *SerializedValue);
		}

		return false;
	}
}

using namespace ConfigToolkit::Private;

bool UConfigToolkitBPLibrary::WriteAnyConfigValue(const FString& Section, const FString& Key, const int32& Value, const FString& Filename)
{
	UE_LOG(LogConfigToolkit, Error, TEXT("Write Config Value failed: This wildcard node must be executed through the Blueprint VM custom thunk path. Native C++ calls cannot use the placeholder int32 signature."));
	checkNoEntry();
	return false;
}

DEFINE_FUNCTION(UConfigToolkitBPLibrary::execWriteAnyConfigValue)
{
	P_GET_PROPERTY(FStrProperty, Section);
	P_GET_PROPERTY(FStrProperty, Key);

	Stack.MostRecentProperty = nullptr;
	Stack.MostRecentPropertyAddress = nullptr;
	Stack.StepCompiledIn<FProperty>(nullptr);
	const FProperty* ValueProperty = Stack.MostRecentProperty;
	const void* ValueAddress = Stack.MostRecentPropertyAddress;

	P_GET_PROPERTY(FStrProperty, Filename);
	P_FINISH;

	P_NATIVE_BEGIN;
	*(bool*)RESULT_PARAM = GenericWriteAnyConfigValue(Section, Key, ValueProperty, ValueAddress, Filename, Stack.Object);
	P_NATIVE_END;
}

bool UConfigToolkitBPLibrary::ReadAnyConfigValue(const FString& Section, const FString& Key, int32& Value, const FString& Filename)
{
	UE_LOG(LogConfigToolkit, Error, TEXT("Read Config Value failed: This wildcard node must be executed through the Blueprint VM custom thunk path. Native C++ calls cannot use the placeholder int32 signature."));
	checkNoEntry();
	return false;
}

DEFINE_FUNCTION(UConfigToolkitBPLibrary::execReadAnyConfigValue)
{
	P_GET_PROPERTY(FStrProperty, Section);
	P_GET_PROPERTY(FStrProperty, Key);

	Stack.MostRecentProperty = nullptr;
	Stack.MostRecentPropertyAddress = nullptr;
	Stack.StepCompiledIn<FProperty>(nullptr);
	const FProperty* ValueProperty = Stack.MostRecentProperty;
	void* ValueAddress = Stack.MostRecentPropertyAddress;

	P_GET_PROPERTY(FStrProperty, Filename);
	P_FINISH;

	P_NATIVE_BEGIN;
	*(bool*)RESULT_PARAM = GenericReadAnyConfigValue(Section, Key, ValueProperty, ValueAddress, Filename, Stack.Object);
	P_NATIVE_END;
}

bool UConfigToolkitBPLibrary::WriteConfigArray(const FString& Section, const FString& Key, const TArray<int32>& Values, const FString& Filename)
{
	UE_LOG(LogConfigToolkit, Error, TEXT("Write Config Array failed: This wildcard array node must be executed through the Blueprint VM custom thunk path. Native C++ calls cannot use the placeholder int32 array signature."));
	checkNoEntry();
	return false;
}

DEFINE_FUNCTION(UConfigToolkitBPLibrary::execWriteConfigArray)
{
	P_GET_PROPERTY(FStrProperty, Section);
	P_GET_PROPERTY(FStrProperty, Key);

	Stack.MostRecentProperty = nullptr;
	Stack.MostRecentPropertyAddress = nullptr;
	Stack.StepCompiledIn<FArrayProperty>(nullptr);
	void* ArrayAddress = Stack.MostRecentPropertyAddress;
	const FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Stack.MostRecentProperty);
	if (!ArrayProperty)
	{
		UE_LOG(LogConfigToolkit, Warning, TEXT("Write Config Array failed: Values pin is not a valid array property."));
		Stack.bArrayContextFailed = true;
		return;
	}

	P_GET_PROPERTY(FStrProperty, Filename);
	P_FINISH;

	P_NATIVE_BEGIN;
	*(bool*)RESULT_PARAM = GenericWriteConfigArray(Section, Key, ArrayProperty, ArrayAddress, Filename, Stack.Object);
	P_NATIVE_END;
}

bool UConfigToolkitBPLibrary::ReadConfigArray(const FString& Section, const FString& Key, TArray<int32>& Values, const FString& Filename)
{
	UE_LOG(LogConfigToolkit, Error, TEXT("Read Config Array failed: This wildcard array node must be executed through the Blueprint VM custom thunk path. Native C++ calls cannot use the placeholder int32 array signature."));
	checkNoEntry();
	return false;
}

DEFINE_FUNCTION(UConfigToolkitBPLibrary::execReadConfigArray)
{
	P_GET_PROPERTY(FStrProperty, Section);
	P_GET_PROPERTY(FStrProperty, Key);

	Stack.MostRecentProperty = nullptr;
	Stack.MostRecentPropertyAddress = nullptr;
	Stack.StepCompiledIn<FArrayProperty>(nullptr);
	void* ArrayAddress = Stack.MostRecentPropertyAddress;
	const FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Stack.MostRecentProperty);
	if (!ArrayProperty)
	{
		UE_LOG(LogConfigToolkit, Warning, TEXT("Read Config Array failed: Values pin is not a valid array property."));
		Stack.bArrayContextFailed = true;
		return;
	}

	P_GET_PROPERTY(FStrProperty, Filename);
	P_FINISH;

	P_NATIVE_BEGIN;
	*(bool*)RESULT_PARAM = GenericReadConfigArray(Section, Key, ArrayProperty, ArrayAddress, Filename, Stack.Object);
	P_NATIVE_END;
}

bool UConfigToolkitBPLibrary::AddUniqueToConfigArray(const FString& Section, const FString& Key, const int32& Value, const FString& Filename)
{
	UE_LOG(LogConfigToolkit, Error, TEXT("Add Unique To Config Array failed: This wildcard node must be executed through the Blueprint VM custom thunk path. Native C++ calls cannot use the placeholder int32 signature."));
	checkNoEntry();
	return false;
}

DEFINE_FUNCTION(UConfigToolkitBPLibrary::execAddUniqueToConfigArray)
{
	P_GET_PROPERTY(FStrProperty, Section);
	P_GET_PROPERTY(FStrProperty, Key);

	Stack.MostRecentProperty = nullptr;
	Stack.MostRecentPropertyAddress = nullptr;
	Stack.StepCompiledIn<FProperty>(nullptr);
	const FProperty* ValueProperty = Stack.MostRecentProperty;
	const void* ValueAddress = Stack.MostRecentPropertyAddress;

	P_GET_PROPERTY(FStrProperty, Filename);
	P_FINISH;

	P_NATIVE_BEGIN;
	*(bool*)RESULT_PARAM = GenericAddUniqueToConfigArray(Section, Key, ValueProperty, ValueAddress, Filename, Stack.Object);
	P_NATIVE_END;
}

bool UConfigToolkitBPLibrary::RemoveFromConfigArray(const FString& Section, const FString& Key, const int32& Value, const FString& Filename)
{
	UE_LOG(LogConfigToolkit, Error, TEXT("Remove From Config Array failed: This wildcard node must be executed through the Blueprint VM custom thunk path. Native C++ calls cannot use the placeholder int32 signature."));
	checkNoEntry();
	return false;
}

DEFINE_FUNCTION(UConfigToolkitBPLibrary::execRemoveFromConfigArray)
{
	P_GET_PROPERTY(FStrProperty, Section);
	P_GET_PROPERTY(FStrProperty, Key);

	Stack.MostRecentProperty = nullptr;
	Stack.MostRecentPropertyAddress = nullptr;
	Stack.StepCompiledIn<FProperty>(nullptr);
	const FProperty* ValueProperty = Stack.MostRecentProperty;
	const void* ValueAddress = Stack.MostRecentPropertyAddress;

	P_GET_PROPERTY(FStrProperty, Filename);
	P_FINISH;

	P_NATIVE_BEGIN;
	*(bool*)RESULT_PARAM = GenericRemoveFromConfigArray(Section, Key, ValueProperty, ValueAddress, Filename, Stack.Object);
	P_NATIVE_END;
}

bool UConfigToolkitBPLibrary::WriteEncryptedString(const FString& Section, const FString& Key, const FString& Value, const FString& Filename)
{
	static const TCHAR* Operation = TEXT("Write Encrypted String");

	if (!HasConfig() || !ValidateSectionAndKey(Operation, Section, Key))
	{
		return false;
	}

	FString EncryptedValue;
	if (!EncryptStringToBase64(Value, EncryptedValue))
	{
		UE_LOG(LogConfigToolkit, Warning, TEXT("%s failed: Plain text value could not be encrypted. Section='%s', Key='%s'."),
			Operation, *Section, *Key);
		return false;
	}

	const FString ResolvedFilename = ResolveConfigFilename(Filename);
	if (!EnsureConfigFileReadyForWrite(Operation, ResolvedFilename, true))
	{
		return false;
	}

	GConfig->SetString(*Section, *Key, *EncryptedValue, ResolvedFilename);
	return FinalizeConfigWrite(Operation, ResolvedFilename);
}

bool UConfigToolkitBPLibrary::ReadEncryptedString(const FString& Section, const FString& Key, FString& Value, const FString& Filename)
{
	static const TCHAR* Operation = TEXT("Read Encrypted String");

	if (!HasConfig() || !ValidateSectionAndKey(Operation, Section, Key))
	{
		return false;
	}

	FString EncryptedValue;
	const FString ResolvedFilename = ResolveConfigFilename(Filename);
	if (!GConfig->GetString(*Section, *Key, EncryptedValue, ResolvedFilename))
	{
		LogMissingConfigLocation(Operation, Section, Key, ResolvedFilename);
		return false;
	}

	if (!DecryptBase64ToString(EncryptedValue, Value))
	{
		UE_LOG(LogConfigToolkit, Warning, TEXT("%s failed: Config value exists but could not be decrypted. ConfigName='%s', File='%s', Section='%s', Key='%s'."),
			Operation, *ResolvedFilename, *GetDiskConfigFilename(ResolvedFilename), *Section, *Key);
		return false;
	}

	return true;
}

FString UConfigToolkitBPLibrary::ConvertAssetToPath(UObject* Asset)
{
	if (!Asset)
	{
		UE_LOG(LogConfigToolkit, Warning, TEXT("Convert Asset To Path returned an empty path: Asset is None."));
	}

	return Asset ? FSoftObjectPath(Asset).ToString() : FString();
}

FString UConfigToolkitBPLibrary::ConvertClassToPath(UClass* Class)
{
	if (!Class)
	{
		UE_LOG(LogConfigToolkit, Warning, TEXT("Convert Class To Path returned an empty path: Class is None."));
	}

	return Class ? FSoftClassPath(Class).ToString() : FString();
}

bool UConfigToolkitBPLibrary::ClearConfigKey(const FString& Section, const FString& Key, const FString& Filename)
{
	static const TCHAR* Operation = TEXT("Clear Config Key");

	if (!HasConfig() || !ValidateSectionAndKey(Operation, Section, Key))
	{
		return false;
	}

	const FString ResolvedFilename = ResolveConfigFilename(Filename);
	if (!EnsureConfigFileReadyForWrite(Operation, ResolvedFilename, false))
	{
		return false;
	}

	if (!GConfig->RemoveKey(*Section, *Key, ResolvedFilename))
	{
		LogMissingConfigLocation(Operation, Section, Key, ResolvedFilename);
		return false;
	}

	return FinalizeConfigWrite(Operation, ResolvedFilename, true);
}

bool UConfigToolkitBPLibrary::ClearConfigSection(const FString& Section, const FString& Filename)
{
	static const TCHAR* Operation = TEXT("Clear Config Section");

	if (!HasConfig() || !ValidateSection(Operation, Section))
	{
		return false;
	}

	const FString ResolvedFilename = ResolveConfigFilename(Filename);
	if (!EnsureConfigFileReadyForWrite(Operation, ResolvedFilename, false))
	{
		return false;
	}

	if (!GConfig->EmptySection(*Section, ResolvedFilename))
	{
		LogMissingConfigSection(Operation, Section, ResolvedFilename);
		return false;
	}

	return FinalizeConfigWrite(Operation, ResolvedFilename, true);
}

bool UConfigToolkitBPLibrary::DoesConfigFileExist(const FString& Filename)
{
	const FString ResolvedFilename = ResolveConfigFilename(Filename);
	const FString DiskFilename = GetDiskConfigFilename(ResolvedFilename);
	const bool bExists = DoesResolvedConfigFileExist(ResolvedFilename);
	if (!bExists)
	{
		const FString Directory = FPaths::GetPath(DiskFilename);
		const bool bDirectoryExists = IFileManager::Get().DirectoryExists(*Directory);
		UE_LOG(LogConfigToolkit, Warning, TEXT("Does Config File Exist returned false: Config file was not found on disk. ConfigName='%s', File='%s', Directory='%s', DirectoryExists=%s."),
			*ResolvedFilename,
			*DiskFilename,
			*Directory,
			bDirectoryExists ? TEXT("true") : TEXT("false"));
	}

	return bExists;
}

TArray<FString> UConfigToolkitBPLibrary::GetConfigSections(const FString& Filename)
{
	static const TCHAR* Operation = TEXT("Get Config Sections");

	TArray<FString> Sections;
	if (!HasConfig())
	{
		return Sections;
	}

	const FString ResolvedFilename = ResolveConfigFilename(Filename);
	const FString DiskFilename = GetDiskConfigFilename(ResolvedFilename);
	if (!DoesResolvedConfigFileExist(ResolvedFilename))
	{
		UE_LOG(LogConfigToolkit, Warning, TEXT("%s failed: Config file does not exist. ConfigName='%s', File='%s'."), Operation, *ResolvedFilename, *DiskFilename);
		return Sections;
	}

	GConfig->GetSectionNames(ResolvedFilename, Sections);
	if (Sections.IsEmpty())
	{
		UE_LOG(LogConfigToolkit, Warning, TEXT("%s returned zero sections: File exists but no sections were loaded. ConfigName='%s', File='%s'."),
			Operation, *ResolvedFilename, *DiskFilename);
	}

	return Sections;
}

bool UConfigToolkitBPLibrary::FlushConfig(const FString& Filename)
{
	static const TCHAR* Operation = TEXT("Flush Config");

	if (!HasConfig())
	{
		return false;
	}

	const FString ResolvedFilename = ResolveConfigFilename(Filename);
	if (!EnsureConfigFileReadyForWrite(Operation, ResolvedFilename, false))
	{
		return false;
	}

	GConfig->Flush(false, ResolvedFilename);
	UE_LOG(LogConfigToolkit, Log, TEXT("%s completed. ConfigName='%s', File='%s'."),
		Operation, *ResolvedFilename, *GetDiskConfigFilename(ResolvedFilename));
	return true;
}
