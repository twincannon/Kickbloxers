// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "KBDefinitions.h"
#include "KBUtility.generated.h"

#define ENUM_AS_STRING(EnumName, EnumValue) UKBUtility::GetEnumValueAsString(#EnumName, (EnumValue))

static FORCEINLINE bool IsGarbageCollectingOnGameThread()
{
	return IsInGameThread() && IsGarbageCollecting();
}

/**
 * 
 */
UCLASS()
class KICKBOXERS_API UKBUtility : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	/** Looks up a UEnum by name, this uses an internal cache */
	static const UEnum* FindGlobalEnum(const FString& enumName);

	template<typename TEnum>
	UFUNCTION(Category = "Enum", BlueprintPure)
	static FORCEINLINE FString GetEnumValueAsString(const FString& Name, TEnum Value)
	{
		// This uses FindObjectSafe which fails if GC is running - this was never a problem on bards, though? (bards has a check here for IsGarbageCollectingOnGameThread)
		const UEnum* enumPtr = FindGlobalEnum(Name);
		if (!enumPtr)
		{
			return FString("null");
		}
		return enumPtr->GetNameStringByIndex((int32)Value);
	}

	template <typename EnumType>
	UFUNCTION(Category = "Enum", BlueprintPure)
	static FORCEINLINE EnumType GetEnumValueFromString(const FString& enumName, const FString& string)
	{
		const UEnum* enumPtr = FindGlobalEnum(enumName);
		if (!enumPtr)
		{
			return EnumType(0);
		}
		return static_cast<EnumType>(enumPtr->GetIndexByName(FName(*string)));
	}

	UFUNCTION(Category = "Kickboxers", BlueprintPure)
	static class UKBUserSettings * GetKBUserSettings();

	UFUNCTION(Category = "Kickboxers", BlueprintPure)
	static FIntPoint CardinalToPoint(ECardinalDirection dir)
	{
		switch (dir)
		{
		case ECardinalDirection::NORTH:
			return FIntPoint(0, -1);
		case ECardinalDirection::EAST:
			return FIntPoint(1, 0);
		case ECardinalDirection::SOUTH:
			return FIntPoint(0, 1);
		case ECardinalDirection::WEST:
			return FIntPoint(-1, 0);
		default:
			return FIntPoint::ZeroValue;
		}
	}

	UFUNCTION(Category = "Kickboxers", BlueprintPure)
	static ECardinalDirection CardinalFromPoint(FIntPoint dir)
	{
		if (dir == FIntPoint(0, -1))
		{
			return ECardinalDirection::NORTH;
		}
		else if (dir == FIntPoint(1, 0))
		{
			return ECardinalDirection::EAST;
		}
		else if (dir == FIntPoint(0, 1))
		{
			return ECardinalDirection::SOUTH;
		}
		else if (dir == FIntPoint(-1, 0))
		{
			return ECardinalDirection::WEST;
		}

		ensureMsgf(false, TEXT("DegreesToCardinalDir - shouldn't be here! point was: %d,%d"), dir.X, dir.Y);
		return ECardinalDirection::NORTH;
	}

	UFUNCTION(Category = "Kickboxers", BlueprintPure)
	static UStaticMesh * GetBoxMeshFromClass(TSubclassOf<AKBBox> boxClass);

	UFUNCTION(Category = "Kickboxers", BlueprintPure, meta = (WorldContext = "worldContextObject"))
	static AKBGameState * GetKBGameState(const UObject* worldContextObject);

	UFUNCTION(Category = "Kickboxers", BlueprintPure, meta = (WorldContext = "worldContextObject"))
	static bool HasGameStarted(const UObject* worldContextObject);

	UFUNCTION(Category = "Kickboxers", BlueprintCallable, meta = (WorldContext = "worldContextObject"))
	static bool DoServerTravel(const UObject* worldContextObject, const FString& levelName);
};
