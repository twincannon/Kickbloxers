// Copyright Puny Human LLC 2020

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "KBPlayerInteractComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerBumpedInto, AKickboxersCharacter * const, character);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerInteract, AKickboxersCharacter * const, character);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class KICKBOXERS_API UKBPlayerInteractComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UKBPlayerInteractComponent();

	// Called when player tries and fails to move due to it being blocked by this actor (only called on components with pawn blocking actors)
	UPROPERTY(Category = "Kickboxers", BlueprintAssignable)
	FOnPlayerBumpedInto OnPlayerBumpedInto;

	UPROPERTY(Category = "Kickboxers", BlueprintAssignable)
	FOnPlayerInteract OnPlayerInteract;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

		
};
