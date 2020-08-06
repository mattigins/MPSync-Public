#include "MPSyncModule.h"
#include "FGGameInstance.h"
#include "mod/blueprint_hooking.h"
#include "mod/hooking.h"
#include "util/Logging.h"
#include "util/ReflectionHelper.h"
#include "FGGameMode.h"
#include "FGNetworkLibrary.h"
#include "FGSaveSystem.h"
#include "MPSyncActor.h"
#include "FGPlayerState.h"
#include "FGGameSession.h"

TArray<FString> CreateMenuInfoText() {
	TArray<FString> resultText;
	resultText.Add(TEXT("MPSync Loaded"));
	return resultText;
}

UWidget* CreateModSubMenuWidget(UUserWidget* OwningWidget) {
	UClass* MenuBaseClass = LoadObject<UClass>(NULL, TEXT("/Game/MPSync/Widget_MPSyncMenu.Widget_MPSyncMenu_C"));
	if (MenuBaseClass == NULL) {
		return NULL;
	}
	UUserWidget* NewWidget = UUserWidget::CreateWidgetInstance(*OwningWidget, MenuBaseClass, TEXT("MPSyncSubMenu"));
	FReflectionHelper::SetPropertyValue<UObjectProperty>(NewWidget, TEXT("mOwningMenu"), OwningWidget);
	return NewWidget;
	
}

void FMPSyncModule::StartupModule() {
	getHost(AMPSyncActor::GetInstance()->API);
	getAuth(AMPSyncActor::GetInstance()->API_AUTH);
	SUBSCRIBE_METHOD(AFGGameSession::SetSessionVisibility, [](auto& scope, AFGGameSession* self, ESessionVisibility visibility)
	{
		if (visibility == SV_FriendsOnly){
			AMPSyncActor::GetInstance()->gameOptions[2] = "SV_FriendsOnly";
		}else{
			AMPSyncActor::GetInstance()->gameOptions[2] = "SV_Private";
		}
	});
	SUBSCRIBE_METHOD(AFGGameMode::PostLogin, [](auto& scope, AFGGameMode* gm, APlayerController* pc)
	{
	try
	{
		if (pc->IsLocalController())
		{
            AMPSyncActor::GetInstance()->Logger("Host Logging In");
            if (!gm->IsMainMenuGameMode()) {
                AMPSyncActor::GetInstance()->GameLoaded(gm, pc);
            }else{
                AMPSyncActor::GetInstance()->RemoveHostedSave();
                UFGLocalPlayer* UFGPlayer = Cast<UFGLocalPlayer>(pc->GetLocalPlayer());
                UFGPlayer->GetFriendList(AMPSyncActor::GetInstance()->friendList);
            }
        }else{
            AMPSyncActor::GetInstance()->Logger("Client Logging In");
        }
	}
	catch (int e)
	{
		AMPSyncActor::GetInstance()->Logger("Something Went Very Wrong");
		AMPSyncActor::GetInstance()->Logger(FString::FromInt(e));
	}
	});
	SUBSCRIBE_VIRTUAL_FUNCTION(AFGGameMode, AFGGameMode::PostSaveGame_Implementation, [](auto& scope, AFGGameMode* self, int32 saveVersion, int32 gameVersion)
	{
		FString path = UFGSaveSystem::GetSaveDirectoryPath();
		AMPSyncActor::GetInstance()->savedGame(saveVersion, gameVersion, path, self);
	});

	SUBSCRIBE_METHOD(UFGGameInstance::Shutdown, [](auto& scope, UFGGameInstance* self)
	{
	        AMPSyncActor::GetInstance()->ClearCurrentlyPlaying();
	});
	SUBSCRIBE_METHOD(UFGBlueprintFunctionLibrary::TravelToMainMenu, [](auto& scope, APlayerController* playerController)
    {
            AMPSyncActor::GetInstance()->ClearCurrentlyPlaying();
    });
	

	SML::Logging::info(TEXT("MPSync Initialized"));

	UClass* MenuWidgetClass = LoadObject<UClass>(NULL, TEXT("/Game/FactoryGame/Interface/UI/Menu/MainMenu/BP_MainMenuWidget.BP_MainMenuWidget_C"));
	checkf(MenuWidgetClass, TEXT("MainMenuWidget blueprint not found"));
	UFunction* ConstructFunction = MenuWidgetClass->FindFunctionByName(TEXT("Construct"));


	HookBlueprintFunction(ConstructFunction, [](FBlueprintHookHelper& HookHelper) {
		UUserWidget* MenuWidget = Cast<UUserWidget>(HookHelper.GetContext());
		UTextBlock* UsernameLabel = FReflectionHelper::GetObjectPropertyValue<UTextBlock>(MenuWidget, TEXT("UsernameLabel"));
		checkf(UsernameLabel, TEXT("UsernameLabel not found"));
		UVerticalBox* ParentVerticalBox = Cast<UVerticalBox>(UsernameLabel->GetParent());
		checkf(ParentVerticalBox, TEXT("UsernameLabel parent is not a vertical box"));

		UUserWidget* OptionsButton = FReflectionHelper::GetObjectPropertyValue<UUserWidget>(MenuWidget, TEXT("mButtonOptions"));
		UPanelWidget* SwitcherWidget = FReflectionHelper::GetObjectPropertyValue<UPanelWidget>(MenuWidget, TEXT("mSwitcher"));
		checkf(OptionsButton, TEXT("OptionsButton not found"));
		checkf(SwitcherWidget, TEXT("Switcher Widget not found"));

		UWidget* ModSubMenuWidget = CreateModSubMenuWidget(MenuWidget);
		if (ModSubMenuWidget != NULL) {
			SwitcherWidget->AddChild(ModSubMenuWidget);
		}

		UPanelWidget* ParentPanel = OptionsButton->GetParent();
		UClass* FrontEndButtonClass = LoadObject<UClass>(NULL, TEXT("/Game/FactoryGame/Interface/UI/Menu/Widget_FrontEnd_Button.Widget_FrontEnd_Button_C"));
		checkf(FrontEndButtonClass, TEXT("FrontEndButton class not found"));

		UUserWidget* ButtonWidget = UUserWidget::CreateWidgetInstance(*MenuWidget, FrontEndButtonClass, TEXT("mMPSyncButton"));
		FText ButtonText = FText::FromString(TEXT("MPSync"));
		FReflectionHelper::CallScriptFunction(ButtonWidget, TEXT("SetTitle"), ButtonText);
		if (ModSubMenuWidget != NULL) {
			FReflectionHelper::SetPropertyValue<UObjectProperty>(ButtonWidget, TEXT("mSwitcherWidget"), SwitcherWidget);
			FReflectionHelper::SetPropertyValue<UObjectProperty>(ButtonWidget, TEXT("mTargetWidget"), ModSubMenuWidget);
		}

		ParentPanel->AddChild(ButtonWidget);
		

		UTextBlock* SMLTextBlock = NewObject<UTextBlock>(ParentVerticalBox);
		SMLTextBlock->Text = FText::FromString(FString::Join(CreateMenuInfoText(), TEXT("\n")));
		SMLTextBlock->SetFont(UsernameLabel->Font);
		SMLTextBlock->SetColorAndOpacity(UsernameLabel->ColorAndOpacity);
		ParentVerticalBox->AddChild(SMLTextBlock);

		}, EPredefinedHookOffset::Return);
}

void FMPSyncModule::getHost(char* outStr)
{
	//Removed From Public Repo
}

void FMPSyncModule::getAuth(char* outStr)
{
	//Removed From Public Repo
}

IMPLEMENT_GAME_MODULE(FMPSyncModule, MPSync);
