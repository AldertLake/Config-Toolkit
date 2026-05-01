#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ConfigToolkitBPLibrary.generated.h"

/**
 * Blueprint access to Unreal config files with wildcard value serialization,
 * native config arrays, per-value AES encryption, and asset path helpers.
 */
UCLASS()
class CONFIGTOOLKIT_API UConfigToolkitBPLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Writes any single Blueprint value to a config key.
	 * @param Section Config section name, for example "Player Settings".
	 * @param Key Config key name inside the section, for example "Mouse Sensitivity".
	 * @param Value Connect the value you want to save. The pin changes type to match your variable.
	 * @param FileName Optional config file name. Leave empty to use the Default Config File Name from Project Settings.
	 * @return True if the value was serialized and written to GConfig.
	 */
	UFUNCTION(BlueprintCallable, CustomThunk, Category="Config Toolkit|Values", meta=(DisplayName="Write Any Config Value", CustomStructureParam="Value", AutoCreateRefTerm="Value", ReturnDisplayName="Success"))
	static bool WriteAnyConfigValue(const FString& Section, const FString& Key, const int32& Value, UPARAM(DisplayName="File Name") const FString& FileName = FString(TEXT("")));
	DECLARE_FUNCTION(execWriteAnyConfigValue);

	/**
	 * Reads any single Blueprint value from a config key.
	 * @param Section Config section name, for example "Player Settings".
	 * @param Key Config key name inside the section, for example "Mouse Sensitivity".
	 * @param Value Output value read from config. Drag from this output pin and promote it to a variable, or connect it directly to later nodes.
	 * @param FileName Optional config file name. Leave empty to use the Default Config File Name from Project Settings.
	 * @return True if the key existed and the text value was imported into the output pin.
	 */
	UFUNCTION(BlueprintCallable, CustomThunk, Category="Config Toolkit|Values", meta=(DisplayName="Read Any Config Value", CustomStructureParam="Value", ReturnDisplayName="Success"))
	static bool ReadAnyConfigValue(const FString& Section, const FString& Key, int32& Value, UPARAM(DisplayName="File Name") const FString& FileName = FString(TEXT("")));
	DECLARE_FUNCTION(execReadAnyConfigValue);

	/**
	 * Writes a Blueprint array using Unreal's native repeated +Key=Value config array format.
	 * @param Section Config section name that owns the array.
	 * @param Key Config array key. Existing entries for this key are replaced.
	 * @param Values Array values to save. The pin changes type to match your array.
	 * @param FileName Optional config file name. Leave empty to use the Default Config File Name from Project Settings.
	 * @return True if the array was serialized and written to GConfig.
	 */
	UFUNCTION(BlueprintCallable, CustomThunk, Category="Config Toolkit|Arrays", meta=(DisplayName="Write Config Array", ArrayParm="Values", ReturnDisplayName="Success"))
	static bool WriteConfigArray(const FString& Section, const FString& Key, const TArray<int32>& Values, UPARAM(DisplayName="File Name") const FString& FileName = FString(TEXT("")));
	DECLARE_FUNCTION(execWriteConfigArray);

	/**
	 * Reads a Blueprint array from Unreal's native repeated +Key=Value config array format.
	 * @param Section Config section name that owns the array.
	 * @param Key Config array key to read.
	 * @param Values Output array filled from config. Drag from this output pin and promote it to a variable, or connect it directly to later nodes.
	 * @param FileName Optional config file name. Leave empty to use the Default Config File Name from Project Settings.
	 * @return True if one or more config array entries were found and imported.
	 */
	UFUNCTION(BlueprintCallable, CustomThunk, Category="Config Toolkit|Arrays", meta=(DisplayName="Read Config Array", ArrayParm="Values", ReturnDisplayName="Success"))
	static bool ReadConfigArray(const FString& Section, const FString& Key, TArray<int32>& Values, UPARAM(DisplayName="File Name") const FString& FileName = FString(TEXT("")));
	DECLARE_FUNCTION(execReadConfigArray);

	/**
	 * Adds one value to a native config array only if an equivalent value is not already present.
	 * @param Section Config section name that owns the array.
	 * @param Key Config array key to update.
	 * @param Value Value to add. The pin changes type to match your variable.
	 * @param FileName Optional config file name. Leave empty to use the Default Config File Name from Project Settings.
	 * @return True if the value was added. False means the value already existed or the write failed.
	 */
	UFUNCTION(BlueprintCallable, CustomThunk, Category="Config Toolkit|Arrays", meta=(DisplayName="Add Unique To Config Array", CustomStructureParam="Value", AutoCreateRefTerm="Value", ReturnDisplayName="Added"))
	static bool AddUniqueToConfigArray(const FString& Section, const FString& Key, const int32& Value, UPARAM(DisplayName="File Name") const FString& FileName = FString(TEXT("")));
	DECLARE_FUNCTION(execAddUniqueToConfigArray);

	/**
	 * Removes all matching values from a native config array.
	 * @param Section Config section name that owns the array.
	 * @param Key Config array key to update.
	 * @param Value Value to remove. The pin changes type to match your variable.
	 * @param FileName Optional config file name. Leave empty to use the Default Config File Name from Project Settings.
	 * @return True if one or more values were removed.
	 */
	UFUNCTION(BlueprintCallable, CustomThunk, Category="Config Toolkit|Arrays", meta=(DisplayName="Remove From Config Array", CustomStructureParam="Value", AutoCreateRefTerm="Value", ReturnDisplayName="Removed"))
	static bool RemoveFromConfigArray(const FString& Section, const FString& Key, const int32& Value, UPARAM(DisplayName="File Name") const FString& FileName = FString(TEXT("")));
	DECLARE_FUNCTION(execRemoveFromConfigArray);

	/**
	 * Encrypts one string value with the project AES key and writes the Base64 ciphertext to config.
	 * @param Section Config section name.
	 * @param Key Config key name inside the section.
	 * @param Value Plain text string to encrypt and save.
	 * @param FileName Optional config file name. Leave empty to use the Default Config File Name from Project Settings.
	 * @return True if encryption and config write succeeded.
	 */
	UFUNCTION(BlueprintCallable, Category="Config Toolkit|Encryption", meta=(DisplayName="Write Encrypted String", ReturnDisplayName="Success"))
	static bool WriteEncryptedString(const FString& Section, const FString& Key, const FString& Value, UPARAM(DisplayName="File Name") const FString& FileName = FString(TEXT("")));

	/**
	 * Reads a Base64 ciphertext config value and decrypts it with the project AES key.
	 * @param Section Config section name.
	 * @param Key Config key name inside the section.
	 * @param Value Output plain text string after decrypting the config value.
	 * @param FileName Optional config file name. Leave empty to use the Default Config File Name from Project Settings.
	 * @return True if the key existed and decryption succeeded.
	 */
	UFUNCTION(BlueprintCallable, Category="Config Toolkit|Encryption", meta=(DisplayName="Read Encrypted String", ReturnDisplayName="Success"))
	static bool ReadEncryptedString(const FString& Section, const FString& Key, FString& Value, UPARAM(DisplayName="File Name") const FString& FileName = FString(TEXT("")));

	/**
	 * Converts an asset object reference to a soft object path string.
	 * @param Asset Asset object to convert.
	 * @return Soft object path string suitable for saving to config.
	 */
	UFUNCTION(BlueprintPure, Category="Config Toolkit|Asset Paths", meta=(DisplayName="Convert Asset To Path", ReturnDisplayName="Path"))
	static FString ConvertAssetToPath(UObject* Asset);

	/**
	 * Converts a class reference to a soft class path string.
	 * @param Class Class to convert.
	 * @return Soft class path string suitable for saving to config.
	 */
	UFUNCTION(BlueprintPure, Category="Config Toolkit|Asset Paths", meta=(DisplayName="Convert Class To Path", ReturnDisplayName="Path"))
	static FString ConvertClassToPath(UClass* Class);

	/**
	 * Removes a scalar value or every repeated array entry for one key.
	 * @param Section Config section name.
	 * @param Key Config key to remove.
	 * @param FileName Optional config file name. Leave empty to use the Default Config File Name from Project Settings.
	 * @return True if the key existed and was removed.
	 */
	UFUNCTION(BlueprintCallable, Category="Config Toolkit|Utilities", meta=(DisplayName="Clear Config Key", ReturnDisplayName="Success"))
	static bool ClearConfigKey(const FString& Section, const FString& Key, UPARAM(DisplayName="File Name") const FString& FileName = FString(TEXT("")));

	/**
	 * Removes every key from one config section.
	 * @param Section Config section name to clear.
	 * @param FileName Optional config file name. Leave empty to use the Default Config File Name from Project Settings.
	 * @return True if the section existed and was cleared.
	 */
	UFUNCTION(BlueprintCallable, Category="Config Toolkit|Utilities", meta=(DisplayName="Clear Config Section", ReturnDisplayName="Success"))
	static bool ClearConfigSection(const FString& Section, UPARAM(DisplayName="File Name") const FString& FileName = FString(TEXT("")));

	/**
	 * Checks whether a config file exists on disk.
	 * @param FileName Optional config file name. Leave empty to use the Default Config File Name from Project Settings.
	 * @return True if the resolved file exists under the generated config directory, or at the supplied absolute path.
	 */
	UFUNCTION(BlueprintPure, Category="Config Toolkit|Utilities", meta=(DisplayName="Does Config File Exist", ReturnDisplayName="Exists"))
	static bool DoesConfigFileExist(UPARAM(DisplayName="File Name") const FString& FileName = FString(TEXT("")));

	/**
	 * Gets all section names currently known for a config file.
	 * @param FileName Optional config file name. Leave empty to use the Default Config File Name from Project Settings.
	 * @return Section names found in the resolved config file.
	 */
	UFUNCTION(BlueprintPure, Category="Config Toolkit|Utilities", meta=(DisplayName="Get Config Sections", ReturnDisplayName="Sections"))
	static TArray<FString> GetConfigSections(UPARAM(DisplayName="File Name") const FString& FileName = FString(TEXT("")));

	/**
	 * Forces the resolved config file to be written to disk.
	 * @param FileName Optional config file name. Leave empty to use the Default Config File Name from Project Settings.
	 * @return True if GConfig was available and the flush request was issued.
	 */
	UFUNCTION(BlueprintCallable, Category="Config Toolkit|Utilities", meta=(DisplayName="Flush Config", ReturnDisplayName="Success"))
	static bool FlushConfig(UPARAM(DisplayName="File Name") const FString& FileName = FString(TEXT("")));
};
