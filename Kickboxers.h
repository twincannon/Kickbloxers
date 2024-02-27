// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

DECLARE_LOG_CATEGORY_EXTERN(LogKickboxers, Log, All);

#define KLOG(a) UE_LOG(LogKickboxers, Log, TEXT(a))
#define WARNING(a) UE_LOG(LogKickboxers, Warning, TEXT(a))
#define ERROR(a) UE_LOG(LogKickboxers, Error, TEXT(a))
#define KMSG(a) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::White, a)
#define KMSG_F(a, b) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::White, FString::Printf(TEXT("%s:  %f"), a, b))
#define KMSG_I(a, b) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::White, FString::Printf(TEXT("%s:  %d"), a, b))



#define NETMODE_WORLD (((GEngine == nullptr) || (GetWorld() == nullptr)) ? TEXT("") \
        : (GEngine->GetNetMode(GetWorld()) == NM_Client) ? TEXT("[Client] ") \
        : (GEngine->GetNetMode(GetWorld()) == NM_ListenServer) ? TEXT("[ListenServer] ") \
        : (GEngine->GetNetMode(GetWorld()) == NM_DedicatedServer) ? TEXT("[DedicatedServer] ") \
        : TEXT("[Standalone] "))

#if _MSC_VER
#define FUNC_NAME    TEXT(__FUNCTION__)
#else
#define FUNC_NAME    TEXT(__func__)
#endif

#define TRACE() \
{ \
    SET_WARN_COLOR(COLOR_CYAN); \
    UE_LOG(LogYourCategory, Log, TEXT("%s%s() : %s"), NETMODE_WORLD, FUNC_NAME, *GetNameSafe(this)); \
    CLEAR_WARN_COLOR(); \
}

#define TRACELOG(Format, ...) \
{ \
    SET_WARN_COLOR(COLOR_CYAN);\
    const FString Msg = FString::Printf(TEXT(Format), ##__VA_ARGS__); \
    if (Msg == "") \
    { \
        UE_LOG(LogYourCategory, Log, TEXT("%s%s() : %s"), NETMODE_WORLD, FUNC_NAME, *GetNameSafe(this)); \
    } \
    else \
    { \
        UE_LOG(LogYourCategory, Log, TEXT("%s%s() : %s"), NETMODE_WORLD, FUNC_NAME, *Msg); \
    } \
    CLEAR_WARN_COLOR(); \
}

#define TRACEMSG(Format, ...) \
{ \
    const FString Msg = FString::Printf(TEXT(Format), ##__VA_ARGS__); \
    if (Msg == "") \
    { \
        TCHAR StdMsg[MAX_SPRINTF] = TEXT(""); \
        FCString::Sprintf(StdMsg, TEXT("%s%s() : %s"), NETMODE_WORLD, FUNC_NAME, *GetNameSafe(this)); \
        GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::White, StdMsg); \
    } \
    else \
    { \
        GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::White, Msg); \
    } \
}

// FCString::Sprintf(StdMsg, TEXT("%s%s(), %s : %s"), NETMODE_WORLD, FUNC_NAME, *GetNameSafe(this), Msg); \