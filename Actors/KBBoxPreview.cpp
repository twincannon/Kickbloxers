// Fill out your copyright notice in the Description page of Project Settings.


#include "KBBoxPreview.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Actors/KBBox.h"

// Sets default values
AKBBoxPreview::AKBBoxPreview()
{
	BoxPreviewMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BoxPreviewMesh"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> boxMeshAsset(TEXT("/Game/Geometry/Meshes/1M_Cube_Chamfer.1M_Cube_Chamfer"));
	if (boxMeshAsset.Succeeded())
	{
		BoxPreviewMesh->SetStaticMesh(boxMeshAsset.Object);
	}
	BoxPreviewMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RootComponent = BoxPreviewMesh;

	static ConstructorHelpers::FObjectFinder<UMaterial> prevMat(TEXT("/Game/Materials/Boxes/M_BoxPreview.M_BoxPreview"));
	if (prevMat.Succeeded())
	{
		PreviewMaterial = prevMat.Object;
	}


 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = false;
	bOnlyRelevantToOwner = true;
	bAlwaysRelevant = false;
	bNetLoadOnClient = false;
}

void AKBBoxPreview::SetupPreview(AKBBox * box, bool holographic /* = false */)
{
	if (BoxPreviewMesh->GetStaticMesh() != box->BoxMesh->GetStaticMesh())
	{
		if (box->BoxMesh->GetStaticMesh())
		{
			BoxPreviewMesh->SetStaticMesh(box->BoxMesh->GetStaticMesh());

			if (holographic)
			{
				for (int32 i = 0; i < BoxPreviewMesh->GetNumMaterials(); ++i)
				{
					UMaterialInstanceDynamic * mid = BoxPreviewMesh->CreateDynamicMaterialInstance(i, PreviewMaterial);
				}
			}
		}
	}
}

// Called when the game starts or when spawned
void AKBBoxPreview::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AKBBoxPreview::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}
