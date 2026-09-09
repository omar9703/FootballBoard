#include "PlayerPiece.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimationAsset.h"

APlayerPiece::APlayerPiece()
{
	PrimaryActorTick.bCanEverTick = false;

	RootScene = CreateDefaultSubobject<USceneComponent>(
		TEXT("Root"));
	RootComponent = RootScene;

	// ---------- Disco (Sender) ----------
	DiscComp = CreateDefaultSubobject<UStaticMeshComponent>(
		TEXT("DiscComp"));
	DiscComp->SetupAttachment(RootComponent);

	static ConstructorHelpers::FObjectFinder<UStaticMesh>
		CylF(TEXT(
			"/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylF.Succeeded())
		DiscComp->SetStaticMesh(CylF.Object);

	// Disco grande y aplanado tipo ficha de tablero
	DiscComp->SetRelativeScale3D(
		FVector(1.9f, 1.9f, 0.28f));
	DiscComp->SetMobility(EComponentMobility::Movable);
	DiscComp->SetCollisionEnabled(
		ECollisionEnabled::QueryOnly);
	DiscComp->SetCollisionObjectType(ECC_WorldDynamic);
	DiscComp->SetCollisionResponseToAllChannels(
		ECR_Ignore);
	DiscComp->SetCollisionResponseToChannel(
		ECC_Visibility, ECR_Block);
	DiscComp->SetCastShadow(false);

	static ConstructorHelpers::FObjectFinder<UMaterial>
		MatF(TEXT(
			"/Engine/BasicShapes/BasicShapeMaterial"
			".BasicShapeMaterial"));
	if (MatF.Succeeded())
		DiscComp->SetMaterial(0, MatF.Object);

	// ---------- Maniqui (Receiver) ----------
	SkelComp =
		CreateDefaultSubobject<USkeletalMeshComponent>(
			TEXT("SkelComp"));
	SkelComp->SetupAttachment(RootComponent);
	SkelComp->SetCollisionEnabled(
		ECollisionEnabled::NoCollision);
	SkelComp->SetVisibility(false);
	SkelComp->SetCastShadow(false);
	SkelComp->bEnableUpdateRateOptimizations = false;
	SkelComp->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
}

void APlayerPiece::BeginPlay()
{
	Super::BeginPlay();
}

void APlayerPiece::ApplyTeamColorToDisc()
{
	if (!DiscComp) return;
	UMaterialInterface* Base = DiscComp->GetMaterial(0);
	if (!Base) return;

	DiscMID = UMaterialInstanceDynamic::Create(Base, this);
	if (!DiscMID) return;
	DiscComp->SetMaterial(0, DiscMID);

	FLinearColor C = (TeamID == 0)
		? FLinearColor(1.0f, 0.15f, 0.15f)
		: FLinearColor(0.15f, 0.3f, 1.0f);
	DiscMID->SetVectorParameterValue(TEXT("Color"), C);
}

void APlayerPiece::SetupAsDisc()
{
	if (SkelComp) SkelComp->SetVisibility(false);
	if (DiscComp)
	{
		DiscComp->SetVisibility(true);
		DiscComp->SetCollisionEnabled(
			ECollisionEnabled::QueryOnly);
	}
	ApplyTeamColorToDisc();
}

void APlayerPiece::SetupAsRunner(
	USkeletalMesh* Mesh, UAnimationAsset* WalkAnim, UAnimationAsset* IdleAnim, float MeshScale)
{
	if (DiscComp)
	{
		DiscComp->SetVisibility(false);
		DiscComp->SetCollisionEnabled(
			ECollisionEnabled::NoCollision);
	}
	ApplyTeamColorToDisc();
	if (!SkelComp) return;

	SkelComp->SetVisibility(true);
	SkelComp->SetRelativeScale3D(FVector(MeshScale));
	// UE 5.8: SetSkeletalMesh() quedo relegada; el setter
	// publico del asset es SetSkeletalMeshAsset().
	if (Mesh) SkelComp->SetSkeletalMeshAsset(Mesh);
	WalkingAnimation = WalkAnim;
	IdleAnimation = IdleAnim;
	PreviousLocation = GetActorLocation();
	bWalking = false;
	if (IdleAnimation)
	{
		SkelComp->SetAnimationMode(
			EAnimationMode::AnimationSingleNode);
		SkelComp->PlayAnimation(IdleAnimation, true);
	}
	// El mesh del maniqui de UE mira hacia +Y de forma
	// nativa (igual que en el template Third Person, que
	// rota el Mesh -90 en yaw). Sin esto, correria de lado.
	SkelComp->SetRelativeRotation(
		FRotator(0.0f, -90.0f, 0.0f));
	// Su origen esta en los pies: va a ras de cesped.
}

void APlayerPiece::UpdateLocomotion(float DeltaTime)
{
	const FVector Movement = GetActorLocation() - PreviousLocation;
	PreviousLocation = GetActorLocation();
	const float Speed = DeltaTime > SMALL_NUMBER ? Movement.Size2D() / DeltaTime : 0.0f;
	// Keep the walk cycle running through brief gaps between network updates.
	const bool bMoving = Speed > 3.0f;
	StillTime = bMoving ? 0.0f : StillTime + DeltaTime;
	const bool bNowWalking = bMoving || (bWalking && StillTime < 0.2f);
	if (bNowWalking != bWalking)
	{
		bWalking = bNowWalking;
		UAnimationAsset* Animation = bWalking ? WalkingAnimation : IdleAnimation;
		if (Animation) SkelComp->PlayAnimation(Animation, true);
	}
	if (bMoving)
	{
		const FRotator Facing(0, Movement.Rotation().Yaw, 0);
		SetActorRotation(FMath::RInterpTo(GetActorRotation(), Facing, DeltaTime, 12.0f));
	}
}
