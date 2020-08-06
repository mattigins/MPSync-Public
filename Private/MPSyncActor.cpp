#include "MPSyncActor.h"
#include "FGLocalPlayer.h"
#include "FGNetworkLibrary.h"
#include "util/Logging.h"
#include "Runtime/Json/Public/Json.h"
#include "HAL/FileManager.h"
#include "Containers/UnrealString.h"
#include "FGGameInstance.h"
#include "player/PlayerControllerHelper.h"
#include "player/component/SMLPlayerComponent.h"

void AMPSyncActor::GameLoaded(AFGGameMode* gm, APlayerController* pc)
{
    TArray<FString> options;
    gm->GetCurrentOptions().ParseIntoArray(options, TEXT("?"), true);
    options[0] = options[0].RightChop(options[0].Find("=") + 1);
    options[1] = options[1].RightChop(options[1].Find("=") + 1);
    options[2] = options[2].RightChop(options[2].Find("=") + 1);
    gameOptions = options;
    myNickname = pc->GetLocalPlayer()->GetNickname();
}

void AMPSyncActor::sendMessageToAll(FString message, FLinearColor color)
{
    TIndirectArray<FWorldContext> worldContexts = GEngine->GetWorldContexts();
    for (TActorIterator<AActor> playerControllers(worldContexts[0].World()); playerControllers; ++playerControllers)
    {
        APlayerController* pc = Cast<APlayerController>(*playerControllers);
        if (pc)
        {
            USMLPlayerComponent* Component = USMLPlayerComponent::Get(pc);
            Component->SendChatMessage(message, color);
        }
    }
}

void AMPSyncActor::savedGame(int32 saveVersion, int32 gameVersion, FString path, AFGGameMode* gm)
{
    //Only Act If Public
    if (gameOptions[2] == "SV_FriendsOnly")
    {
        FMPSyncConfigStruct config = readConfig();
        Logger("Game Synced");

        //Broadcast Sync Message
        if (config.broadcast)
        {
            sendMessageToAll("Game Synced with MPSync", FLinearColor::Red);
        }
        
        //Get Original Host ID
        FString origID;
        if (originalHostID.IsEmpty())
        {
            origID = IOnlineSubsystem::Get()->GetIdentityInterface()->GetUniquePlayerId(0)->ToString();
        }else{
            origID = originalHostID;
        }

        //Get Last Save
        IFileManager& IFM = IFileManager::Get();
        const TCHAR* RootPath = path.GetCharArray().GetData();
        TArray<FString> saves;
        const TCHAR* extension = _T("*.sav");
        IFM.FindFilesRecursive(saves, RootPath, extension, true, false, false);
        FDateTime lastDate = 0;
        FString lastSave = "";
        for (int i = 0; i < saves.Num(); i++) {
            FDateTime fileDate = IFileManager::Get().GetTimeStamp(saves[i].GetCharArray().GetData());
            if (fileDate > lastDate){
                lastDate = fileDate;
                lastSave = saves[i];
            }
        }
        //Copy Last Save and Rename
        FString x;
        FString fileName;
        FPaths::Split(lastSave, x, fileName, x);
        FString fileDestination = "../../MPSync/Cache/";
        fileDestination += fileName;
        fileDestination += ".mpsync";
        IFM.Copy(*fileDestination, *lastSave);

        //Create Json File
        TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
        FString JsonOutput;
        JsonObject->SetStringField("originalOwner", origID);
        JsonObject->SetStringField("mapName", gameOptions[1]);
        JsonObject->SetStringField("mapType", gameOptions[0]);
        JsonObject->SetStringField("saveFile", fileName);
        JsonObject->SetStringField("lastSave", "{{SAVE_TIME}}");
        JsonObject->SetStringField("hostedBy", myNickname);
        JsonObject->SetBoolField("isPublic", false);
        FJsonSerializer::Serialize(JsonObject.ToSharedRef(), TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&JsonOutput));
        FString jsonFileLocation = "../../MPSync/Cache/data.dat";
        FFileHelper::SaveStringToFile(JsonOutput, *jsonFileLocation);

        //Remove Special Characters
        FString encodedMapName = FGenericPlatformHttp::UrlEncode(gameOptions[1]);
        FString encodedfileName = FGenericPlatformHttp::UrlEncode(fileName);
        
        //Upload
        uploadFile(jsonFileLocation, FString::Printf(TEXT("?json&file=%s/%s"), *origID, *encodedMapName));
        uploadFile(fileDestination, FString::Printf(TEXT("?file=%s/%s"), *origID, *encodedfileName));
    }
}

void AMPSyncActor::uploadFile(FString file, FString queryString){
    FHttpModule* Http = &FHttpModule::Get();
    TSharedRef<IHttpRequest> Request = Http->CreateRequest();
    Request->OnProcessRequestComplete().BindUObject(this, &AMPSyncActor::OnUploadComplete);
    Request->SetURL(FString::Printf(TEXT("%hsupload.php%s"), API, *queryString));
    Request->SetVerb(TEXT("POST"));
    Request->SetContentAsStreamedFile(file);
    Request->SetHeader(TEXT("User-Agent"), TEXT("X-UnrealEngine-Agent"));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    Request->SetHeader(TEXT("Accepts"), TEXT("application/json"));
    Request->SetHeader(TEXT("Authorization"), API_AUTH);
    Request->ProcessRequest();
}

void AMPSyncActor::OnUploadComplete(FHttpRequestPtr Request, FHttpResponsePtr const Response, bool bWasSuccessful){}

void AMPSyncActor::Logger(FString const Log)
{
    FMPSyncConfigStruct config = GetInstance()->readConfig();
    if (config.debugLog)
    {
        SML::Logging::info(TEXT("MPSync: "), *Log);
    }
}


AMPSyncActor* AMPSyncActor::GetInstance()
{
    static AMPSyncActor* i = nullptr; if (!i)
    {
        i = NewObject<AMPSyncActor>();
        i->AddToRoot();
    }
    return i;
}

void AMPSyncActor::RequestCall(APlayerController* Player) {
    FString UID = IOnlineSubsystem::Get()->GetIdentityInterface()->GetUniquePlayerId(0)->ToString();
    
    UFGLocalPlayer* UFGPlayer = Cast<UFGLocalPlayer>(Player->GetLocalPlayer());
    UFGPlayer->GetFriendList(friendList);
    TArray<FString> friends;
    if (friendList.Num() > 0)
    {
        TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
        FString JsonOutput;
        for (int i = 0; i < friendList.Num(); i++)
        {
            TSharedPtr<FOnlineFriend> f = friendList[i].Friend;
            FString cleanFriendName = TCHAR_TO_ANSI(*f->GetDisplayName());
            FString friendID = f->GetUserId()->ToString();
            JsonObject->SetStringField(friendID, cleanFriendName);
        }
        JsonObject->SetStringField(UID, "Your Games");
        FJsonSerializer::Serialize(JsonObject.ToSharedRef(), TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&JsonOutput));
        FHttpModule* Http = &FHttpModule::Get();
        TSharedRef<IHttpRequest> Request = Http->CreateRequest();
        Request->OnProcessRequestComplete().BindUObject(this, &AMPSyncActor::OnResponseReceived);
        Request->SetURL(FString::Printf(TEXT("%hsgetData.php"), API));
        Request->SetVerb(TEXT("POST"));
        Request->SetContentAsString(JsonOutput);
        Request->SetHeader(TEXT("User-Agent"), TEXT("X-UnrealEngine-Agent"));
        Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
        Request->SetHeader(TEXT("Accepts"), TEXT("application/json"));
        Request->SetHeader(TEXT("Authorization"), API_AUTH);
        Request->ProcessRequest();
    }
    
}


void AMPSyncActor::OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr const Response, bool bWasSuccessful)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> const Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
    if (FJsonSerializer::Deserialize(Reader, JsonObject))
    {
        TArray<TSharedPtr<FJsonValue>> datas;
        datas = JsonObject->GetArrayField("data");
        for (int i = 0; i < datas.Num(); i++)
        {
            auto data = datas[i]->AsObject();
            FString uid = data->GetStringField("uid");
            FString name = data->GetStringField("name");
            

            TArray<TSharedPtr<FJsonValue>> games;
            games = data->GetArrayField("games");
            TArray<FgameStruct> outputGames;
            for (int x = 0; x < games.Num(); x++)
            {
                auto game = games[x]->AsObject();
                FString originalOwner = game->GetStringField("originalOwner");
                FString mapName = game->GetStringField("mapName");
                FString mapType = game->GetStringField("mapType");
                FString saveFile = game->GetStringField("saveFile");
                FString lastSave = game->GetStringField("lastSave");
                FString hostedBy = game->GetStringField("hostedBy");
                FgameStruct newGame;
                newGame.originalOwner = originalOwner;
                newGame.mapName = mapName;
                newGame.mapType = mapType;
                newGame.saveFile = saveFile;
                newGame.lastSave = lastSave;
                newGame.hostedBy = hostedBy;
                outputGames.Add(newGame);
            }
            MpSyncUserAdd.Broadcast(name, outputGames);
        }
    }
    MpSyncResponseReceived.Broadcast(Response->GetContentAsString());
}

void AMPSyncActor::RemoveHostedSave()
{
    FMPSyncConfigStruct config = readConfig();
    FString jsonFileLocation = "../../MPSync/sessionData.dat";
    if (FPaths::FileExists(jsonFileLocation))
    {
        FString jsonOut;
        FFileHelper::LoadFileToString(jsonOut, *jsonFileLocation);
        TSharedPtr<FJsonObject> JsonObject;
        TSharedRef<TJsonReader<>> const Reader = TJsonReaderFactory<>::Create(jsonOut);
        if (FJsonSerializer::Deserialize(Reader, JsonObject))
        {
            FString saveFile = JsonObject->GetStringField("saveFile");
            saveFile = saveFile.Replace(*FString(TEXT("%20")), *FString(TEXT("_"))).Replace(*FString(TEXT("%")), *FString(TEXT("")));
            FString uid = JsonObject->GetStringField("uid");
            FString saveFileLocation = FString::Printf(TEXT("%sSaveGames/%s/%s.sav"), *FPaths::ProjectSavedDir(), *uid, *saveFile);
            if (FPaths::FileExists(saveFileLocation))
            {
                if (config.removeLocal)
                {
                    IFileManager::Get().Delete(*saveFileLocation);
                }
                IFileManager::Get().Delete(*jsonFileLocation);
            }
        }
    }
}

void AMPSyncActor::ClearCurrentlyPlaying()
{
    RemoveHostedSave();
    if (gameOptions.Num() > 0)
    {
        FString origID;
        FString myName = myNickname;
        if (originalHostID.IsEmpty())
        {
            origID = IOnlineSubsystem::Get()->GetIdentityInterface()->GetUniquePlayerId(0)->ToString();
        }else{
            origID = originalHostID;
        }
        FString fileName = gameOptions[1];
        FString sendData = FString::Printf(TEXT("%s:%s:%s"), *origID, *fileName, *myName);
        FHttpModule* Http = &FHttpModule::Get();
        TSharedRef<IHttpRequest> Request = Http->CreateRequest();
        Request->SetURL(FString::Printf(TEXT("%hsremovePlaying.php"), API));
        Request->SetVerb(TEXT("POST"));
        Request->SetContentAsString(sendData);
        Request->SetHeader(TEXT("User-Agent"), TEXT("X-UnrealEngine-Agent"));
        Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
        Request->SetHeader(TEXT("Accepts"), TEXT("application/json"));
        Request->SetHeader(TEXT("Authorization"), API_AUTH);
        Request->ProcessRequest();
    }
}

void AMPSyncActor::HostGame(FgameStruct data)
{
    downloadedGame = data;
    FString uid = IOnlineSubsystem::Get()->GetIdentityInterface()->GetUniquePlayerId(0)->ToString();
    
    //Create Json File
    if (uid != data.originalOwner)
    {
        TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
        FString JsonOutput;
        JsonObject->SetStringField("originalOwner", data.originalOwner);
        JsonObject->SetStringField("mapName", data.mapName);
        JsonObject->SetStringField("mapType", data.mapType);
        JsonObject->SetStringField("saveFile", data.saveFile);
        JsonObject->SetStringField("uid", uid);
        FJsonSerializer::Serialize(JsonObject.ToSharedRef(), TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&JsonOutput));
        FString jsonFileLocation = "../../MPSync/sessionData.dat";
        FFileHelper::SaveStringToFile(JsonOutput, *jsonFileLocation);
    }

    //Download The Save
    originalHostID = data.originalOwner;
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
    FString JsonOutput;
    JsonObject->SetStringField("originalOwner", data.originalOwner);
    JsonObject->SetStringField("mapName", FGenericPlatformHttp::UrlEncode(data.saveFile));
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&JsonOutput));
    
    FString URL = FString::Printf(TEXT("%hsdownload.php"), API);
    FHttpModule* Http = &FHttpModule::Get();
    TSharedRef<IHttpRequest> Request = Http->CreateRequest();
    Request->OnProcessRequestComplete().BindUObject(this, &AMPSyncActor::OnDownloadResponseReceived);
    Request->SetURL(URL);
    Request->SetVerb(TEXT("POST"));
    Request->SetContentAsString(JsonOutput);
    Request->SetHeader(TEXT("Content-Type"), "application/octet-stream");
    Request->SetHeader(TEXT("User-Agent"), TEXT("X-UnrealEngine-Agent"));
    Request->SetHeader(TEXT("Authorization"), API_AUTH);
    Request->ProcessRequest();
}

void AMPSyncActor::OnDownloadResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr const Response, bool bWasSuccessful)
{
    //bWasSuccessful = false;
    FString responseString = Response->GetContentAsString();
    if (responseString.Contains("{{MPSYNC ERROR}}"))
    {
        bWasSuccessful = false;
    }
    FString uid = IOnlineSubsystem::Get()->GetIdentityInterface()->GetUniquePlayerId(0)->ToString();
    FString x;
    FString fileName = downloadedGame.saveFile;
    //URLDecode hangs the game, so this gets ugly
    fileName = fileName.Replace(*FString(TEXT("%20")), *FString(TEXT("_"))).Replace(*FString(TEXT("%")), *FString(TEXT("")));
    FString downloadLocation = FString::Printf(TEXT("%sSaveGames/%s/%s.sav"), *FPaths::ProjectSavedDir(), *uid, *fileName);
    if (bWasSuccessful){
        FFileHelper::SaveArrayToFile(Response->GetContent(), *downloadLocation);
    }
    FString errorString = FString::Printf(TEXT("%d\n%s\n%s\n%s"), Response->GetResponseCode(), *uid, *downloadLocation, *responseString);
    MpSyncDownloadResponseReceived.Broadcast(bWasSuccessful, errorString);
}

void AMPSyncActor::startGame(APlayerController* playerController, FgameStruct data)
{
    FString fileName = data.saveFile.Replace(*FString(TEXT("%20")), *FString(TEXT("_"))).Replace(*FString(TEXT("%")), *FString(TEXT("")));
    FString mapOptions = FString::Printf(TEXT("??startloc=%s?sessionName=%s?Visibility=SV_FriendsOnly?loadgame=%s?listen"), *data.mapType, *data.mapName, *fileName);
    UFGBlueprintFunctionLibrary::CreateSessionAndTravelToMap(playerController,"Persistent_Level", mapOptions, data.mapName,SV_FriendsOnly);
}

FMPSyncConfigStruct AMPSyncActor::readConfig()
{
    //If No Config, Create One First
    FString jsonFileLocation = "../../MPSync/Config/config.ini";
    if (!FPaths::FileExists(jsonFileLocation))
    {
        TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
        FString JsonOutput;
        JsonObject->SetBoolField("debugLog", false);
        JsonObject->SetBoolField("removeLocal", true);
        JsonObject->SetBoolField("broadcast", true);
        FJsonSerializer::Serialize(JsonObject.ToSharedRef(), TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&JsonOutput));
        FFileHelper::SaveStringToFile(JsonOutput, *jsonFileLocation);
    }

    //Read Config in to Struct and Return
    FString jsonOut;
    FMPSyncConfigStruct output;
    TSharedPtr<FJsonObject> JsonObject;
    FFileHelper::LoadFileToString(jsonOut, *jsonFileLocation);
    TSharedRef<TJsonReader<>> const Reader = TJsonReaderFactory<>::Create(jsonOut);
    if (FJsonSerializer::Deserialize(Reader, JsonObject))
    {
        output.broadcast = JsonObject->GetBoolField("broadcast");
        output.debugLog = JsonObject->GetBoolField("debugLog");
        output.removeLocal = JsonObject->GetBoolField("removeLocal");
    }else
    {
        //For Safety.. I guess?
        output.broadcast = true;
        output.debugLog = false;
        output.removeLocal = true;
    }
    return output;
}

void AMPSyncActor::setConfig(FMPSyncConfigStruct input)
{
    FString jsonFileLocation = "../../MPSync/Config/config.ini";
    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
    FString JsonOutput;
    JsonObject->SetBoolField("debugLog", input.debugLog);
    JsonObject->SetBoolField("removeLocal", input.removeLocal);
    JsonObject->SetBoolField("broadcast", input.broadcast);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&JsonOutput));
    FFileHelper::SaveStringToFile(JsonOutput, *jsonFileLocation);
}


AMPSyncActor::AMPSyncActor()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AMPSyncActor::BeginPlay()
{
	Super::BeginPlay();
}

void AMPSyncActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}