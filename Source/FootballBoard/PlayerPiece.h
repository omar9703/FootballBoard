#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PlayerPiece.generated.h"

class UStaticMeshComponent;
class USkeletalMeshComponent;
class UMaterialInstanceDynamic;
class USkeletalMesh;
class UAnimationAsset;

UCLASS()
class FOOTBALLBOARD_API APlayerPiece : public AActor
{
	GENERATED_BODY()

public:
	APlayerPiece();

	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		Category = "Player")
	int32 PlayerID = 0;

	// 0 = Rojo, 1 = Azul
	UPROPERTY(EditAnywhere, BlueprintReadWrite,
		Category = "Player")
	int32 TeamID = 0;

	// Radio grande: pensado para tocar con el dedo
	UPROPERTY(EditAnywhere, Category = "Player")
	float CollisionRadius = 95.0f;

	// Sender: disco grande.  Receiver: maniqui corriendo.
	void SetupAsDisc();
	void SetupAsRunner(USkeletalMesh* Mesh,
		UAnimationAsset* WalkAnim, UAnimationAsset* IdleAnim, float MeshScale);
	void UpdateLocomotion(float DeltaTime);

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY() USceneComponent* RootScene;
	UPROPERTY() UStaticMeshComponent* DiscComp;
	UPROPERTY() USkeletalMeshComponent* SkelComp;
	UPROPERTY() UMaterialInstanceDynamic* DiscMID;
	UPROPERTY() UAnimationAsset* WalkingAnimation;
	UPROPERTY() UAnimationAsset* IdleAnimation;
	FVector PreviousLocation = FVector::ZeroVector;
	bool bWalking = false;
	float StillTime = 0.0f;

	void ApplyTeamColorToDisc();
};
