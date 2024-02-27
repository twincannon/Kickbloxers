// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KBBoxPreview.generated.h"

class AKBBox;

UCLASS()
class KICKBOXERS_API AKBBoxPreview : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AKBBoxPreview();

	
	UPROPERTY(Category = "Kickboxers", EditDefaultsOnly, BlueprintReadOnly)
	class USceneComponent * BoxRoot;

	UPROPERTY(Category = "Kickboxers", EditDefaultsOnly, BlueprintReadOnly)
	class UStaticMeshComponent * BoxPreviewMesh;

	UPROPERTY(Category = "Kickboxers", EditDefaultsOnly, BlueprintReadOnly)
	class UMaterial * PreviewMaterial;

	// Setup preview based on target box
	void SetupPreview(AKBBox * box, bool holographic = false);

	// Stored class for purposes of spawning an actual box later
	void SetBoxClass(TSubclassOf<AKBBox> newClass) { BoxClass = newClass; }
	TSubclassOf<AKBBox> GetBoxClass() const { return BoxClass; }

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	TSubclassOf<AKBBox> BoxClass;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
