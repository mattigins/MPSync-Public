// ILikeBanas

#pragma once

#include "SteamFriendsFunctionLibrary.h"
#include "OnlineSubsystemUtils.h"
#include "CoreMinimal.h"
#include "OnlineFriendsInterface.h"
#include "Net/OnlineBlueprintCallProxyBase.h"
#include "GetSteamFriendsCallProxyBase.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGetSteamFriendsDelegate, const TArray<FSteamFriend>&, Results);
UCLASS()
class MPSYNC_API UGetSteamFriendsCallProxyBase : public UOnlineBlueprintCallProxyBase
{
	GENERATED_UCLASS_BODY()

private:
	TWeakObjectPtr<APlayerController> PlayerControllerWeakPtr;
	
	UObject* WorldContextObject;
	
	FOnReadFriendsListComplete OnReadFriendsCompleteDelegate;

public:
	UPROPERTY(BlueprintAssignable)
		FGetSteamFriendsDelegate OnSuccess;
	
	UPROPERTY(BlueprintAssignable)
		FGetSteamFriendsDelegate OnFailure;

	UFUNCTION(BlueprintCallable, meta=(BlueprintInternalUseOnly = "true", WorldContext = "InWorldContextObject"), Category = "Steam|Friends")
		static UGetSteamFriendsCallProxyBase* GetSteamFriends(UObject* InWorldContextObject, class APlayerController* InPlayerController);

	void OnGetSteamFriendsComplete(int32 LocalUserNum, bool bWasSuccessful, const FString& ListName, const FString& ErrorStr);
	
	virtual void Activate() override;
};
