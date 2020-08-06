#pragma once

#include "CoreMinimal.h"
#include "FGChatManager.h"
#include "FGGameMode.h"
#include "FGLocalPlayer.h"
#include "SteamFriendsFunctionLibrary.h"
#include "GameFramework/Actor.h"
#include "Runtime/Online/HTTP/Public/Http.h"
#include "MPSyncActor.generated.h"

USTRUCT(BlueprintType)
struct FgameStruct
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadWrite)
    FString originalOwner;
	UPROPERTY(BlueprintReadWrite)
	FString mapName;
	UPROPERTY(BlueprintReadWrite)
	FString mapType;
	UPROPERTY(BlueprintReadWrite)
	FString saveFile;
	UPROPERTY(BlueprintReadWrite)
	FString lastSave;
	UPROPERTY(BlueprintReadWrite)
	FString hostedBy;
};

USTRUCT(BlueprintType)
struct FMPSyncConfigStruct
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadWrite)
	bool debugLog;
	UPROPERTY(BlueprintReadWrite)
	bool removeLocal;
	UPROPERTY(BlueprintReadWrite)
	bool broadcast;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FHTTPOnResponseReceivedDelegate, FString, HttpData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FHTTPOnDownloadResponseReceivedDelegate, bool, wasSuccess, FString, errorString);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FonUserAdd, FString, username, const TArray<FgameStruct>&, games);

UCLASS()
class MPSYNC_API AMPSyncActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AMPSyncActor();

	UPROPERTY(BlueprintAssignable, Category = "MPSync|Events", VisibleAnywhere)
		FHTTPOnResponseReceivedDelegate MpSyncResponseReceived;

	UPROPERTY(BlueprintAssignable, Category = "MPSync|Events", VisibleAnywhere)
		FHTTPOnDownloadResponseReceivedDelegate MpSyncDownloadResponseReceived;

	UPROPERTY(BlueprintAssignable, Category = "MPSync|Events", VisibleAnywhere)
		FonUserAdd MpSyncUserAdd;

	UFUNCTION(BlueprintPure)
		static AMPSyncActor* GetInstance();

	UFUNCTION(BlueprintCallable, Category = "MPSync")
		static void Logger(FString const Log);

	UFUNCTION(BlueprintCallable, Category = "MPSync|HTTP")
		void RequestCall(APlayerController* Player);

	UFUNCTION(BlueprintCallable, Category = "MPSync")
		void HostGame(FgameStruct data);

	UFUNCTION(BlueprintCallable, Category = "MPSync")
		FMPSyncConfigStruct readConfig();

	UFUNCTION(BlueprintCallable, Category = "MPSync")
	void setConfig(FMPSyncConfigStruct input);

	UFUNCTION(BlueprintCallable, Category = "MPSync")
		void startGame(APlayerController* playerController, FgameStruct data);

	void OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr const Response, bool bWasSuccessful);
	void RemoveHostedSave();
	void ClearCurrentlyPlaying();
	void OnDownloadResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr const Response, bool bWasSuccessful);
	void uploadFile(FString file, FString queryString);
	void OnUploadComplete(FHttpRequestPtr Request, FHttpResponsePtr const Response, bool bWasSuccessful);
	void GameLoaded(AFGGameMode* gm, APlayerController* pc);
	void savedGame(int32 saveVersion, int32 gameVersion, FString path, AFGGameMode* gm);
	void sendMessageToAll(FString message, FLinearColor color);
	
	TArray<FString> gameOptions;
	TArray<FFGOnlineFriend> friendList;
	FString originalHostID;
	FString myNickname;
	char API[31];
	char API_AUTH[32];
	FgameStruct downloadedGame;
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
