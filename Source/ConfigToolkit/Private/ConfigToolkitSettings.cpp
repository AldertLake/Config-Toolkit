#include "ConfigToolkitSettings.h"

#include "Containers/StringConv.h"

#define LOCTEXT_NAMESPACE "ConfigToolkitSettings"

UConfigToolkitSettings::UConfigToolkitSettings()
	: DefaultConfigFilename(TEXT("Game"))
	, AESEncryptionKey(TEXT(""))
{
}

FName UConfigToolkitSettings::GetCategoryName() const
{
	return TEXT("Plugins");
}

FName UConfigToolkitSettings::GetSectionName() const
{
	return TEXT("ConfigToolkit");
}

#if WITH_EDITOR
FText UConfigToolkitSettings::GetSectionText() const
{
	return LOCTEXT("SectionText", "Config Toolkit");
}

FText UConfigToolkitSettings::GetSectionDescription() const
{
	return LOCTEXT("SectionDescription", "Default filename and AES key used by Config Toolkit Blueprint nodes.");
}

void UConfigToolkitSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (DefaultConfigFilename.TrimStartAndEnd().IsEmpty())
	{
		DefaultConfigFilename = TEXT("Game");
	}

	const FTCHARToUTF8 KeyBytes(*AESEncryptionKey);
	if (!AESEncryptionKey.IsEmpty() && (AESEncryptionKey.Len() != 32 || KeyBytes.Length() != 32))
	{
		UE_LOG(LogTemp, Warning, TEXT("Config Toolkit AES Encryption Key must be exactly 32 characters and 32 UTF-8 bytes for AES-256. Current length: %d characters, %d bytes."), AESEncryptionKey.Len(), KeyBytes.Length());
	}
}
#endif

#undef LOCTEXT_NAMESPACE
