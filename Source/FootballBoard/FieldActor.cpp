#include "FieldActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"

AFieldActor::AFieldActor()
{
	PrimaryActorTick.bCanEverTick = false;

	// Raiz sin escala: asi las lineas hijas no se
	// deforman por la escala del cesped.
	USceneComponent* Root =
		CreateDefaultSubobject<USceneComponent>(
			TEXT("Root"));
	RootComponent = Root;

	Grass = CreateDefaultSubobject<UStaticMeshComponent>(
		TEXT("Grass"));
	Grass->SetupAttachment(RootComponent);

	static ConstructorHelpers::FObjectFinder<UStaticMesh>
		PlaneF(TEXT(
			"/Engine/BasicShapes/Plane.Plane"));
	if (PlaneF.Succeeded()) PlaneMesh = PlaneF.Object;

	static ConstructorHelpers::FObjectFinder<UStaticMesh>
		CubeF(TEXT(
			"/Engine/BasicShapes/Cube.Cube"));
	if (CubeF.Succeeded()) CubeMesh = CubeF.Object;

	static ConstructorHelpers::FObjectFinder<UMaterial>
		MatF(TEXT(
			"/Engine/BasicShapes/BasicShapeMaterial"
			".BasicShapeMaterial"));
	if (MatF.Succeeded()) BaseMat = MatF.Object;

	if (PlaneMesh) Grass->SetStaticMesh(PlaneMesh);
	Grass->SetMobility(EComponentMobility::Movable);

	// El cesped bloquea Visibility para que el raycast
	// del Sender tenga punto de impacto.
	Grass->SetCollisionEnabled(
		ECollisionEnabled::QueryOnly);
	Grass->SetCollisionObjectType(ECC_WorldStatic);
	Grass->SetCollisionResponseToAllChannels(ECR_Ignore);
	Grass->SetCollisionResponseToChannel(
		ECC_Visibility, ECR_Block);
	Grass->SetCastShadow(false);
}

void AFieldActor::BeginPlay()
{
	Super::BeginPlay();
	BuildField();
}

void AFieldActor::BuildField()
{
	// Evitar duplicar lineas si se reconstruye
	for (UStaticMeshComponent* L : Lines)
		if (L) L->DestroyComponent();
	Lines.Empty();

	// ----- Cesped -----
	if (PlaneMesh) Grass->SetStaticMesh(PlaneMesh);
	// El Plane mide 100 uu a escala 1
	Grass->SetRelativeScale3D(FVector(
		HalfLengthX * 2.0f / 100.0f,
		HalfWidthY  * 2.0f / 100.0f, 1.0f));
	Grass->SetRelativeLocation(FVector::ZeroVector);

	if (BaseMat)
	{
		GrassMID = UMaterialInstanceDynamic::Create(
			BaseMat, this);
		if (GrassMID)
		{
			GrassMID->SetVectorParameterValue(
				TEXT("Color"),
				FLinearColor(0.05f, 0.35f, 0.08f));
			Grass->SetMaterial(0, GrassMID);
		}
	}

	const float T = LineThickness;
	const float X = HalfLengthX;
	const float Y = HalfWidthY;

	// ----- Perimetro -----
	SpawnLine(FVector2D(0,  Y), FVector2D(X*2 + T, T), 0);
	SpawnLine(FVector2D(0, -Y), FVector2D(X*2 + T, T), 0);
	SpawnLine(FVector2D( X, 0), FVector2D(T, Y*2 + T), 0);
	SpawnLine(FVector2D(-X, 0), FVector2D(T, Y*2 + T), 0);

	// ----- Linea de medio campo -----
	SpawnLine(FVector2D(0, 0), FVector2D(T, Y*2), 0);

	// ----- Circulo central + punto -----
	SpawnCircle(FVector2D(0, 0), 550.0f, 48);
	SpawnLine(FVector2D(0, 0), FVector2D(34, 34), 0);

	// ----- Areas grandes (penal) -----
	const float PenDepth = 660.0f;
	const float PenWidth = 1300.0f;
	SpawnBox(FVector2D(-X + PenDepth * 0.5f, 0),
		PenDepth, PenWidth);
	SpawnBox(FVector2D( X - PenDepth * 0.5f, 0),
		PenDepth, PenWidth);

	// ----- Areas chicas (porteria) -----
	const float SmDepth = 240.0f;
	const float SmWidth = 640.0f;
	SpawnBox(FVector2D(-X + SmDepth * 0.5f, 0),
		SmDepth, SmWidth);
	SpawnBox(FVector2D( X - SmDepth * 0.5f, 0),
		SmDepth, SmWidth);
}

void AFieldActor::SpawnLine(
	const FVector2D& Center,
	const FVector2D& SizeXY, float YawDeg)
{
	if (!CubeMesh) return;

	UStaticMeshComponent* L =
		NewObject<UStaticMeshComponent>(this);
	L->RegisterComponent();
	L->AttachToComponent(RootComponent,
		FAttachmentTransformRules::KeepRelativeTransform);
	L->SetStaticMesh(CubeMesh);
	L->SetMobility(EComponentMobility::Movable);
	L->SetCollisionEnabled(
		ECollisionEnabled::NoCollision);
	L->SetCastShadow(false);

	if (BaseMat)
	{
		UMaterialInstanceDynamic* MID =
			UMaterialInstanceDynamic::Create(BaseMat, this);
		if (MID)
		{
			MID->SetVectorParameterValue(TEXT("Color"),
				FLinearColor(0.9f, 0.9f, 0.9f));
			L->SetMaterial(0, MID);
		}
	}

	// El Cube mide 100 uu a escala 1
	L->SetRelativeScale3D(FVector(
		SizeXY.X / 100.0f,
		SizeXY.Y / 100.0f,
		LineHeight / 100.0f));
	L->SetRelativeLocationAndRotation(
		FVector(Center.X, Center.Y, LineHeight * 0.5f + 1),
		FRotator(0.0f, YawDeg, 0.0f));

	Lines.Add(L);
}

void AFieldActor::SpawnCircle(
	const FVector2D& Center, float Radius, int32 Segments)
{
	if (Segments < 3) return;
	const float Circ = 2.0f * PI * Radius;
	// *1.2 para solapar segmentos y no dejar huecos
	const float SegLen = (Circ / Segments) * 1.2f;

	for (int32 i = 0; i < Segments; i++)
	{
		float A = (2.0f * PI * i) / Segments;
		float Px = Center.X + Radius * FMath::Cos(A);
		float Py = Center.Y + Radius * FMath::Sin(A);
		float Yaw = FMath::RadiansToDegrees(A) + 90.0f;
		SpawnLine(FVector2D(Px, Py),
			FVector2D(SegLen, LineThickness), Yaw);
	}
}

void AFieldActor::SpawnBox(
	const FVector2D& Center, float SizeX, float SizeY)
{
	const float T = LineThickness;
	const float hx = SizeX * 0.5f;
	const float hy = SizeY * 0.5f;
	// 3 lados (el lado pegado a la porteria se omite):
	// la linea de frente va del lado que mira al centro.
	const float FrontX = (Center.X < 0.0f)
		? Center.X + hx : Center.X - hx;
	SpawnLine(FVector2D(FrontX, Center.Y),
		FVector2D(T, SizeY), 0);                 // frente
	SpawnLine(FVector2D(Center.X, Center.Y + hy),
		FVector2D(SizeX, T), 0);                 // arriba
	SpawnLine(FVector2D(Center.X, Center.Y - hy),
		FVector2D(SizeX, T), 0);                 // abajo
}
