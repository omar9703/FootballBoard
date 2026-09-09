#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FieldActor.generated.h"

class UStaticMeshComponent;
class UStaticMesh;
class UMaterialInterface;
class UMaterialInstanceDynamic;

UCLASS()
class FOOTBALLBOARD_API AFieldActor : public AActor
{
	GENERATED_BODY()

public:
	AFieldActor();

	UPROPERTY(EditAnywhere, Category = "Field")
	float HalfLengthX = 2500.0f;   // mitad del largo (X)

	UPROPERTY(EditAnywhere, Category = "Field")
	float HalfWidthY = 1500.0f;    // mitad del ancho (Y)

	UPROPERTY(EditAnywhere, Category = "Field")
	float LineThickness = 14.0f;   // grosor de linea

	UPROPERTY(EditAnywhere, Category = "Field")
	float LineHeight = 4.0f;       // alto de la linea

	// Construye cesped + lineas. Se llama en BeginPlay.
	void BuildField();

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY() UStaticMeshComponent* Grass;
	UPROPERTY() UMaterialInterface* BaseMat;
	UPROPERTY() UMaterialInstanceDynamic* GrassMID;
	UPROPERTY() TArray<UStaticMeshComponent*> Lines;
	UPROPERTY() UStaticMesh* CubeMesh;
	UPROPERTY() UStaticMesh* PlaneMesh;

	// Una linea blanca (caja fina) sobre el cesped.
	// SizeXY = (largo, ancho) antes de rotar; YawDeg gira en Z.
	void SpawnLine(const FVector2D& Center,
		const FVector2D& SizeXY, float YawDeg);

	// Un circulo hecho de segmentos.
	void SpawnCircle(const FVector2D& Center,
		float Radius, int32 Segments);

	// Un rectangulo de lineas (area, porteria).
	void SpawnBox(const FVector2D& Center,
		float SizeX, float SizeY);
};
