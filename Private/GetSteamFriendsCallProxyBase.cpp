// ILikeBanas


#include "GetSteamFriendsCallProxyBase.h"
#include "Engine/LocalPlayer.h"
#include "Online.h"
#include "OnlineFriendsInterface.h"

UGetSteamFriendsCallProxyBase::UGetSteamFriendsCallProxyBase(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer),
    OnReadFriendsCompleteDelegate(FOnReadFriendsListComplete::CreateUObject(this, &UGetSteamFriendsCallProxyBase::OnGetSteamFriendsComplete))
{
    
}

UGetSteamFriendsCallProxyBase* UGetSteamFriendsCallProxyBase::GetSteamFriends(UObject* InWorldContextObject, APlayerController* InPlayerController)
{
    UGetSteamFriendsCallProxyBase* Proxy = NewObject<UGetSteamFriendsCallProxyBase>();
    Proxy->WorldContextObject = InWorldContextObject;
    Proxy->PlayerControllerWeakPtr = InPlayerController;

    return Proxy;
}

void UGetSteamFriendsCallProxyBase::OnGetSteamFriendsComplete(int32 LocalUserNum, bool bWasSuccessful, const FString& ListName, const FString& ErrorStr)
{
    TArray<FSteamFriend> SteamFriends;

    if (bWasSuccessful)
    {
        IOnlineFriendsPtr FriendInterface = Online::GetFriendsInterface();

        if (FriendInterface.IsValid())
        {
            TArray<TSharedRef<FOnlineFriend>> FriendList;
            FriendInterface->GetFriendsList(LocalUserNum, ListName, FriendList);

            for (TSharedRef<FOnlineFriend> Friend : FriendList)
            {
                FOnlineUserPresence Presence = Friend->GetPresence();

                FSteamFriend SteamFriend;

                SteamFriend.DisplayName = Friend->GetDisplayName();
                SteamFriend.RealName = Friend->GetRealName();
                SteamFriend.StatusString = Presence.Status.StatusStr;
                SteamFriend.bIsOnline = Presence.bIsOnline;
                SteamFriend.bIsPlaying = Presence.bIsPlaying;
                SteamFriend.bIsPlayingSameGame = Presence.bIsPlayingThisGame;
                SteamFriend.bIsJoinable = Presence.bIsJoinable;
                SteamFriend.bHasVoiceSupport = Presence.bHasVoiceSupport;
                SteamFriend.PresenceState = (EOnlinePresenceState::Type)(int32)Presence.Status.State;
                SteamFriend.UniqueNetId = Friend->GetUserId();

                SteamFriends.Add(SteamFriend);
            }

            OnSuccess.Broadcast(SteamFriends);
        }
    }
    OnFailure.Broadcast(SteamFriends);
}

void UGetSteamFriendsCallProxyBase::Activate()
{
    if (PlayerControllerWeakPtr.IsValid())
    {
        IOnlineFriendsPtr FriendInterface = Online::GetFriendsInterface();
        if (FriendInterface.IsValid())
        {
            const ULocalPlayer* LocalPlayer = PlayerControllerWeakPtr->GetLocalPlayer();

            FriendInterface->ReadFriendsList(LocalPlayer->GetControllerId(), EFriendsLists::ToString(EFriendsLists::Default), OnReadFriendsCompleteDelegate);

            return;
        }
    }

    TArray<FSteamFriend> SteamFriends;
    OnFailure.Broadcast(SteamFriends);
}
