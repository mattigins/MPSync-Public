// ILikeBanas

#pragma once

#include "OnlinePresenceInterface.h"
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SteamFriendsFunctionLibrary.generated.h"

USTRUCT(BlueprintType)
struct FSteamFriend
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "Steam|Friends")
		FString DisplayName;
	UPROPERTY(BlueprintReadOnly, Category = "Steam|Friends")
		FString RealName;
	UPROPERTY(BlueprintReadOnly, Category = "Steam|Friends")
		FString StatusString;
	UPROPERTY(BlueprintReadOnly, Category = "Steam|Friends")
		bool bIsOnline;
	UPROPERTY(BlueprintReadOnly, Category = "Steam|Friends")
		bool bIsPlaying;
	UPROPERTY(BlueprintReadOnly, Category = "Steam|Friends")
		bool bIsPlayingSameGame;
	UPROPERTY(BlueprintReadOnly, Category = "Steam|Friends")
		bool bIsJoinable;
	UPROPERTY(BlueprintReadOnly, Category = "Steam|Friends")
		bool bHasVoiceSupport;

	EOnlinePresenceState::Type PresenceState;
	TSharedPtr<const FUniqueNetId> UniqueNetId;
};

UCLASS()
class MPSYNC_API USteamFriendsFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
};
