#include "framework.h"

DECLARE_HOOK(AShooterPlayerController_ServerRequestLevelUp_Implementation, void, AShooterPlayerController*, UPrimalCharacterStatusComponent*, EPrimalCharacterStatusValue::Type);

std::vector<struct MyEngram> EngramList{};

static bool bUseChatCommand = true;
static bool bAutoLearnEngramsOnLevelUp = false;
static bool bChatCommand_IgnoreLevelRequirements = true;
static bool bAutoLearnTek = true;
static bool bNotifyPlayerHUD = true;
static bool bAdminOnly = false;

/// <summary>
/// My struct where we will host information about the engram and requirements
/// </summary>
struct MyEngram
{
	UPrimalEngramEntry* Engram;
	std::vector<MyEngram> Requirements;
	MyEngram(UPrimalEngramEntry* engram) : Engram(engram) { }
};

/// <summary>
/// Required struct that is not defined in the API that holds information about the requirements
/// </summary>
struct FEngramEntries
{
	TArray<struct UClass*> EngramEntries;
};

/// <summary>
/// Recursive function used to get the engram and it's requirements
/// </summary>
/// <param name="Engram">The engram to get information and requirements on</param>
/// <returns>The engram as a MyEngram struct complete with all requirements</returns>
MyEngram GetEngram(UPrimalEngramEntry* Engram)
{
	FString Out;
	MyEngram myEngram(Engram);
	for (auto&& Requirement : Engram->EngramRequirementSetsField()) {
		for (auto&& EngramEntry : Requirement.EngramEntries) {
			auto EngramClass = (UPrimalEngramEntry*)EngramEntry->GetDefaultObject(true);
			myEngram.Requirements.push_back(GetEngram(EngramClass));
		}
	}
	return myEngram;
}

/// <summary>
/// This should be called and populated once, if EngramList.size() is 0 or when the world is initially loaded
/// </summary>
void GetEngrams()
{
	auto GameSingleton = (UPrimalGlobals*)Globals::GEngine().Get()->GameSingletonField();
	auto GameData = GameSingleton->PrimalGameDataOverrideField()
		? GameSingleton->PrimalGameDataOverrideField()
		: GameSingleton->PrimalGameDataField();
	auto EngramClasses = GameData->EngramBlueprintEntriesField();
	for (auto&& EngramClass : EngramClasses)
	{
		EngramList.push_back(GetEngram(EngramClass));
	}
}

/// <summary>
/// For debug purposes, indents the required engram a certain amount to make it more readable
/// </summary>
/// <param name="Indent"></param>
/// <returns></returns>
std::string Tabs(int Indent) {
	std::string ReturnValue = "";
	for (int i = 0; i < Indent; i++) { ReturnValue += "  "; }
	return ReturnValue;
}

/// <summary>
/// For debug purposes
/// </summary>
/// <param name="Engram"></param>
/// <param name="Indent"></param>
void PrintEngram(const MyEngram& Engram, int Indent = 0)
{
	FString EngramName;
	Engram.Engram->NameField().ToString(&EngramName);
	Log::GetLog()->info("{}{}", Tabs(Indent), EngramName.ToString());
	for (auto&& Inner : Engram.Requirements) {
		PrintEngram(Inner, Indent + 1);
	}
}

/// <summary>
/// Recursive function to determine whether an engram entry can be unlocked based on itself and its requiremnets
/// </summary>
/// <param name="EngramEntry"></param>
/// <returns></returns>
bool CanUnlock(const MyEngram& EngramEntry)
{
	FString Out;

	if (!EngramEntry.Engram->bCanBeManuallyUnlocked().Get())
		return false;

	for (auto&& Entry : EngramEntry.Requirements)
	{
		if (!CanUnlock(Entry)) 
			return false;
	}
	return true;
}

/// <summary>
/// Check to make sure the engram list is populated
/// </summary>
void CheckForEngrams()
{
	if (EngramList.size() == 0) { 
		GetEngrams();
	}
}

/// <summary>
/// The command that gets called when the chat commands are run
/// </summary>
/// <param name="PlayerController">The PlayerController of the person who called the command</param>
/// <param name="Message"></param>
/// <param name="Mode"></param>
void UnlockEngrams(AShooterPlayerController* PlayerController, FString* Message, int Mode)
{
	if (!bUseChatCommand || (!PlayerController->bIsAdmin().Get() && bAdminOnly)) 
		return;

	CheckForEngrams();

	// Avoid a server crash
	if (!PlayerController || !PlayerController->GetPlayerCharacter())
		return;

	auto CharacterLevel = PlayerController->GetPlayerCharacter()->MyCharacterStatusComponentField()->GetCharacterLevel();
	for (auto&& EngramEntry : EngramList) 
	{
		if (CanUnlock(EngramEntry) || (!CanUnlock(EngramEntry) && bAutoLearnTek))
		{
			if (CharacterLevel >= EngramEntry.Engram->RequiredCharacterLevelField() || bChatCommand_IgnoreLevelRequirements)
			{
				auto PlayerState = (AShooterPlayerState*)PlayerController->PlayerStateField();
				//auto PreviousPoints = PlayerState->FreeEngramPointsField();
				PlayerState->ServerUnlockEngram(EngramEntry.Engram->BluePrintEntryField(), bNotifyPlayerHUD, true);
			}
		}
	}
}

/// <summary>
/// Hook for when a player levels up to autolearn engrams
/// </summary>
/// <param name="PlayerController"></param>
/// <param name="ForStatusComp"></param>
/// <param name="ValueType"></param>
void Hook_AShooterPlayerController_ServerRequestLevelUp_Implementation(AShooterPlayerController* PlayerController, UPrimalCharacterStatusComponent* ForStatusComp, EPrimalCharacterStatusValue::Type ValueType)
{
	// Call original function, then proceed with our code next
	AShooterPlayerController_ServerRequestLevelUp_Implementation_original(PlayerController, ForStatusComp, ValueType);

	if (!bAutoLearnEngramsOnLevelUp) 
		return;

	// Avoid a server crash
	if (!PlayerController || !PlayerController->PawnField())
		return;

	// It's important to make sure we're dealing with a shooter character that is leveling up, and not a tame
	if (PlayerController->PawnField()->IsA(AShooterCharacter::GetPrivateStaticClass()))
	{
		CheckForEngrams();

		auto CharacterLevel = ForStatusComp->GetCharacterLevel();
		for (auto&& EngramEntry : EngramList) 
		{
			if (CharacterLevel >= EngramEntry.Engram->RequiredCharacterLevelField())
			{
				if (CanUnlock(EngramEntry) || (!CanUnlock(EngramEntry) && bAutoLearnTek))
				{
					auto PlayerState = (AShooterPlayerState*)PlayerController->PlayerStateField();
					//auto PreviousPoints = PlayerState->FreeEngramPointsField();
					PlayerState->ServerUnlockEngram(EngramEntry.Engram->BluePrintEntryField(), bNotifyPlayerHUD, true);
				}
			}
		}
	}
}

#pragma region Admin Commands
/// <summary>
/// Save your settings to config.json!
/// </summary>
/// <param name="PlayerController"></param>
/// <param name="Message"></param>
/// <param name="Mode"></param>
void SaveConfig(AShooterPlayerController* PlayerController, FString* Message, int Mode)
{
	if (!PlayerController || !PlayerController->bIsAdmin().Get())
		return;

	namespace fs = std::filesystem;

	nlohmann::json jsonfile;

	jsonfile["settings"]["UseChatCommand"] = bUseChatCommand;
	jsonfile["settings"]["ChatCommand_IgnoreLevelRequirements"] = bChatCommand_IgnoreLevelRequirements;
	jsonfile["settings"]["NotifyPlayerHUD"] = bNotifyPlayerHUD;
	jsonfile["settings"]["AutoLearnEngramsOnLevelUp"] = bAutoLearnEngramsOnLevelUp;
	jsonfile["settings"]["AutoLearnTek"] = bAutoLearnTek;
	jsonfile["settings"]["AdminOnly"] = bAdminOnly;

	std::string Path = ConfigReader::GetCurrentDir() + "/ArkApi/Plugins/MyEngramUnlocker/config.json";
	std::ofstream jsonOut;
	jsonOut.open(Path);
	jsonOut << jsonfile;
	jsonOut.close();

	ArkApi::GetApiUtils().SendServerMessage(PlayerController, FLinearColor(0.f, 1.f, 0.f, 1.f), "Settings saved!");
	Log::GetLog()->info("Settings saved!");
}

/// <summary>
/// Enables admin only mode, making it so only admins can execute the /unlock commands
/// </summary>
/// <param name="PlayerController"></param>
/// <param name="Message"></param>
/// <param name="Mode"></param>
void Toggle(AShooterPlayerController* PlayerController, FString* Message, int Mode)
{
	if (!PlayerController || !PlayerController->bIsAdmin().Get())
		return;

	TArray<FString> Parsed;
	Message->ParseIntoArray(Parsed, L" ", true);

	if (Parsed.Num() > 0)
	{
		auto ToggleBool = [&p = PlayerController](bool* b, const std::wstring Caption) {
			*b = !*b;
			ArkApi::GetApiUtils().SendServerMessage(p, *b ? FLinearColor(0.f, 1.f, 0.f, 1.f) : FLinearColor(1.f, 0.f, 0.f, 1.f), L"{} mode is set to {}", Caption.c_str(), *b);
		};

		if (strcmp(Parsed[1].ToString().c_str(), "chatcmd") == 0) {
			ToggleBool(&bUseChatCommand, L"UseChatCommand");
		}
		else if (strcmp(Parsed[1].ToString().c_str(), "lvlup") == 0) {
			ToggleBool(&bAutoLearnEngramsOnLevelUp, L"AutoLearnEngramsOnLevelUp");
		}
		else if (strcmp(Parsed[1].ToString().c_str(), "tek") == 0) {
			ToggleBool(&bAutoLearnTek, L"AutoLearnTek");
		}
		else if (strcmp(Parsed[1].ToString().c_str(), "admin") == 0) {
			ToggleBool(&bAdminOnly, L"AdminOnly");
		}
		else if (strcmp(Parsed[1].ToString().c_str(), "ignorelvl") == 0) {
			ToggleBool(&bChatCommand_IgnoreLevelRequirements, L"IgnoreLevelRequirements");
		}
	}
}

void LoadPlugin()
{
	Log::Get().Init("MyEngramUnlocker - Plugin loaded");

	ConfigReader cfgReader;
	if (cfgReader.ReadConfig())
	{
		Log::GetLog()->debug("Reading config.json...");
		bUseChatCommand = cfgReader.config_["settings"].value("UseChatCommand", true);
		bChatCommand_IgnoreLevelRequirements = cfgReader.config_["settings"].value("ChatCommand_IgnoreLevelRequirements", false);
		bNotifyPlayerHUD = cfgReader.config_["settings"].value("NotifyPlayerHUD", true);
		bAutoLearnEngramsOnLevelUp = cfgReader.config_["settings"].value("AutoLearnEngramsOnLevelUp", true);
		bAutoLearnTek = cfgReader.config_["settings"].value("AutoLearnTek", true);
		bAdminOnly = cfgReader.config_["settings"].value("AdminOnly", false);
	}

	Log::GetLog()->info("Settings:");
	Log::GetLog()->info("  - UseChatCommand:             {}", bUseChatCommand);
	Log::GetLog()->info("  - IgnoreLevelRequirements:    {}", bChatCommand_IgnoreLevelRequirements);
	Log::GetLog()->info("  - AutoLearnEngramsOnLevelUp:  {}", bAutoLearnEngramsOnLevelUp);
	Log::GetLog()->info("  - AutoLearnTek:               {}", bAutoLearnTek);
	Log::GetLog()->info("  - NotifyPlayerHUD:            {}", bNotifyPlayerHUD);
	Log::GetLog()->info("Admin only commands:");
	Log::GetLog()->info("  - /toggle chatcmd             Toggle use of chat command to unlock engrams");
	Log::GetLog()->info("  - /toggle lvlup               Toggle whether or not engrams autoassign on level-up");
	Log::GetLog()->info("  - /toggle tek                 Toggle whether or not tek is automatically learned");
	Log::GetLog()->info("  - /toggle admin               Toggle admin only mode (only admins can execute /unlock commands)");
	Log::GetLog()->info("  - /toggle ignorelvl           Toggle whether or not level requirements are ignored when unlocking engrams");
	Log::GetLog()->info("  - /dump                       Dumps all engrams to engrams.txt");
	Log::GetLog()->info("  - /save                       Save the config file!");
	Log::GetLog()->info("Chat commands:");
	Log::GetLog()->info("  - /unlockengrams              Unlocks all engrams");
	Log::GetLog()->info("  - /engrams                    Unlocks all engrams");
	Log::GetLog()->info("  - /unlock                     Unlocks all engrams");

	ArkApi::GetCommands().AddChatCommand("/unlockengrams", &UnlockEngrams);
	ArkApi::GetCommands().AddChatCommand("/engrams", &UnlockEngrams);
	ArkApi::GetCommands().AddChatCommand("/unlock", &UnlockEngrams);

	ArkApi::GetCommands().AddChatCommand("/save", &SaveConfig);
	ArkApi::GetCommands().AddChatCommand("/toggle", &Toggle);
	ArkApi::GetCommands().AddChatCommand("/toggle", NULL);
	ArkApi::GetCommands().AddChatCommand("/dump", NULL);

	ArkApi::GetHooks().SetHook("AShooterPlayerController.ServerRequestLevelUp_Implementation", &Hook_AShooterPlayerController_ServerRequestLevelUp_Implementation, &AShooterPlayerController_ServerRequestLevelUp_Implementation_original);
}
#pragma endregion

void UnloadPlugin()
{
	Log::GetLog()->info("MyEngramUnlocker - Unloaded plugin");

	ArkApi::GetCommands().RemoveChatCommand("/unlockengrams");
	ArkApi::GetCommands().RemoveChatCommand("/engrams");
	ArkApi::GetCommands().RemoveChatCommand("/unlock");

	ArkApi::GetCommands().RemoveChatCommand("/save");
	ArkApi::GetCommands().RemoveChatCommand("/toggle");
	ArkApi::GetCommands().RemoveChatCommand("/dump");

	ArkApi::GetHooks().DisableHook("AShooterPlayerController.ServerRequestLevelUp_Implementation", &Hook_AShooterPlayerController_ServerRequestLevelUp_Implementation);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	if (ul_reason_for_call == DLL_PROCESS_ATTACH)
	{
		LoadPlugin();
	}
	else if (ul_reason_for_call == DLL_PROCESS_DETACH)
	{
		UnloadPlugin();
	}
	return TRUE;
}

