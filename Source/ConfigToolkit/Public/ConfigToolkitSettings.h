#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "ConfigToolkitSettings.generated.h"

/**
 * Global project settings for Config Toolkit Blueprint nodes.
 */
UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="Config Toolkit"))
class CONFIGTOOLKIT_API UConfigToolkitSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UConfigToolkitSettings();

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="General", meta=(DisplayName="Default Config File Name", ToolTip="Config file name used when a Blueprint node receives an empty File Name pin."))
	FString DefaultConfigFilename;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="General", meta=(DisplayName="Automatically Flush Config", ToolTip="When enabled, write and clear nodes flush immediately after changing config. When disabled, call Flush Config manually."))
	bool bAutomaticallyFlushConfig;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Encryption", meta=(DisplayName="AES Encryption Key", PasswordField=true, ToolTip="AES-256 key used by encrypted value nodes. Must be exactly 32 characters and 32 UTF-8 bytes."))
	FString AESEncryptionKey;

	virtual FName GetCategoryName() const override;
	virtual FName GetSectionName() const override;

#if WITH_EDITOR
	virtual FText GetSectionText() const override;
	virtual FText GetSectionDescription() const override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
