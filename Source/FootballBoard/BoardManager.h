#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Networking.h"
#include "BoardManager.generated.h"

class FSocket;
class FTcpListener;
class APlayerPiece;
class AFieldActor;
class ACameraActor;
class USkeletalMesh;
class UAnimationAsset;

UCLASS()
class FOOTBALLBOARD_API ABoardManager : public AActor
{
	GENERATED_BODY()

public:
	// Screen distances are measured at a reference viewport height of 1080 pixels.
	UPROPERTY(EditAnywhere, Category = "Touch", meta = (ClampMin = "0.0"))
	float TouchPickPadding = 24.0f;
	UPROPERTY(EditAnywhere, Category = "Touch", meta = (ClampMin = "0.0", ClampMax = "0.5"))
	float TouchReleaseGraceSeconds = 0.15f;
	UPROPERTY(EditAnywhere, Category = "Touch", meta = (ClampMin = "0.0"))
	float TouchResumeDistance = 80.0f;

	ABoardManager();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void EndPlay(
		const EEndPlayReason::Type Reason) override;

	// ----- Cancha -----
	UPROPERTY(EditAnywhere, Category = "Field")
	float FieldHalfLengthX = 2500.0f;
	UPROPERTY(EditAnywhere, Category = "Field")
	float FieldHalfWidthY = 1500.0f;

	// Altura de cada pieza segun el rol
	UPROPERTY(EditAnywhere, Category = "Field")
	float SenderPieceZ = 20.0f;   // disco sobre cesped
	UPROPERTY(EditAnywhere, Category = "Field")
	float ReceiverPieceZ = 0.0f;  // pies del maniqui

	// ----- Camara -----
	UPROPERTY(EditAnywhere, Category = "Camera")
	float CameraHeight = 4800.0f;
	UPROPERTY(EditAnywhere, Category = "Camera", meta = (ClampMin = "1.0", ClampMax = "4.0"))
	float ReceiverPlayerScale = 2.3f;

	// ----- Red -----
	UPROPERTY(EditAnywhere, Category = "Network")
	int32 ListenPort = 7777;
	UPROPERTY(EditAnywhere, Category = "Network")
	bool bIsSender = true;
	UPROPERTY(EditAnywhere, Category = "Network")
	FString ServerIP = TEXT("127.0.0.1");
	UPROPERTY(EditAnywhere, Category = "Network")
	float SendRateHz = 60.0f;

	// ----- Movimiento (Receiver) -----
	UPROPERTY(EditAnywhere, Category = "Movement")
	float InterpSpeed = 12.0f;

	// ----- Visual del Receiver -----
	// (asignar en el editor tras anadir Third Person)
	UPROPERTY(EditAnywhere, Category = "Receiver Visual")
	USkeletalMesh* RunnerMeshTeam0 = nullptr;  // rojo
	UPROPERTY(EditAnywhere, Category = "Receiver Visual")
	USkeletalMesh* RunnerMeshTeam1 = nullptr;  // azul
	UPROPERTY(EditAnywhere, Category = "Receiver Visual")
	UAnimationAsset* RunnerRunAnim = nullptr;
	UPROPERTY(EditAnywhere, Category = "Player Visual")
	UAnimationAsset* PlayerWalkAnim = nullptr;
	UPROPERTY(EditAnywhere, Category = "Player Visual")
	UAnimationAsset* PlayerIdleAnim = nullptr;

private:
	UPROPERTY() TArray<APlayerPiece*> Players;
	UPROPERTY() AFieldActor* Field = nullptr;

	void SpawnField();
	void SpawnPlayers();
	void SpawnTeam(int32 TeamID,
		const TArray<FVector2D>& Positions, int32 StartID);
	void SetupTopDownCamera();

	float PieceZForRole() const {
		return bIsSender ? SenderPieceZ : ReceiverPieceZ; }

	FVector ClampToField(const FVector& Pos) const;
	FVector ResolveCollisions(APlayerPiece* Mover,
		const FVector& DesiredPos) const;

	// ----- Drag (raton + touch) -----
	UPROPERTY() APlayerPiece* DraggedPlayer = nullptr;
	FVector DragOffset;
	bool bWasPointerDownLastFrame = false;
	bool bDragUsesTouch = false;
	double LastPointerTime = 0.0;
	FVector2D LastPointerScreen = FVector2D::ZeroVector;
	void HandleDragInput();

	// ----- Envio con throttling -----
	float SendAccumulator = 0.0f;
	bool bPendingSend = false;
	int32 PendingSendID = -1;
	FVector PendingSendPos;
	void TickSendThrottle(float DeltaTime);

	// ----- Sender TCP -----
	FTcpListener* TcpListener = nullptr;
	FSocket* ClientSocket = nullptr;
	TQueue<FSocket*, EQueueMode::Mpsc> PendingConnections;
	bool OnConnectionAccepted(FSocket* Socket,
		const FIPv4Endpoint& Endpoint);
	void ProcessPendingConnections();
	void SendPlayerPosition(int32 ID, const FVector& Pos);
	void SendAllPositions();

	// ----- Receiver TCP -----
	FSocket* ReceiverSocket = nullptr;
	FString ReceiveBuffer;
	float ReconnectTimer = 0.0f;
	bool bReceiverConnected = false;
	void ConnectToServer();
	void TickReconnect(float DeltaTime);
	void ReceiveData();
	void ParseMessage(const FString& Line);
	TMap<int32, FVector> TargetPositions;
	void InterpolateReceiverPlayers(float DeltaTime);
};
