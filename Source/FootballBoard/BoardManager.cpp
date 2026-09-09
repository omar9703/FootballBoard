#include "BoardManager.h"
#include "PlayerPiece.h"
#include "FieldActor.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimationAsset.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Common/TcpListener.h"
#include "Common/TcpSocketBuilder.h"
#include "Interfaces/IPv4/IPv4Address.h"
#include "Interfaces/IPv4/IPv4Endpoint.h"

ABoardManager::ABoardManager()
{
	PrimaryActorTick.bCanEverTick = true;
	static ConstructorHelpers::FObjectFinder<UAnimationAsset> Walk(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Walk/MF_Unarmed_Walk_Fwd.MF_Unarmed_Walk_Fwd"));
	static ConstructorHelpers::FObjectFinder<UAnimationAsset> Idle(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle.MM_Idle"));
	PlayerWalkAnim = Walk.Object;
	PlayerIdleAnim = Idle.Object;
}

void ABoardManager::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PC =
		UGameplayStatics::GetPlayerController(this, 0);
	if (PC)
	{
		PC->bShowMouseCursor = true;
		PC->bEnableClickEvents = true;
		PC->bEnableMouseOverEvents = true;
		PC->bEnableTouchEvents = true;      // touch
		PC->bEnableTouchOverEvents = true;  // touch
	}

	SpawnField();
	SpawnPlayers();
	SetupTopDownCamera();

	if (bIsSender)
	{
		FIPv4Endpoint Endpoint(
			FIPv4Address::Any, ListenPort);
		TcpListener = new FTcpListener(Endpoint);
		TcpListener->OnConnectionAccepted()
			.BindUObject(this,
				&ABoardManager::OnConnectionAccepted);

		if (TcpListener->Init())
		{
			UE_LOG(LogTemp, Log,
				TEXT("[SENDER] Escuchando en %d"),
				ListenPort);
		}
		else
		{
			UE_LOG(LogTemp, Error,
				TEXT("[SENDER] Error. Puerto %d ocupado?"),
				ListenPort);
		}
	}
	else
	{
		ConnectToServer();
	}
}

void ABoardManager::SpawnField()
{
	UWorld* World = GetWorld();
	if (!World) return;

	// Deferred: fijamos el tamano ANTES de BuildField
	Field = World->SpawnActorDeferred<AFieldActor>(
		AFieldActor::StaticClass(), FTransform::Identity);
	if (Field)
	{
		Field->HalfLengthX = FieldHalfLengthX;
		Field->HalfWidthY  = FieldHalfWidthY;
		Field->FinishSpawning(FTransform::Identity);
	}
}

void ABoardManager::SetupTopDownCamera()
{
	UWorld* World = GetWorld();
	if (!World) return;

	FVector CamLoc(0, 0, CameraHeight);
	FRotator CamRot(-90.0f, -90.0f, 0.0f); // largo horizontal
	float ViewWidth = FMath::Max(FieldHalfLengthX * 2.2f, FieldHalfWidthY * 2.2f * (16.0f / 9.0f));
	if (!bIsSender)
	{
		CamRot = FRotator(-35.264f, -45.0f, 0.0f);
		const float PlayerHeight = 200.0f * ReceiverPlayerScale;
		const FVector Focus(0, 0, PlayerHeight * 0.5f);
		CamLoc = Focus - CamRot.Vector() * CameraHeight * 2.0f;
		const FRotationMatrix Axes(CamRot);
		const FVector Right = Axes.GetUnitAxis(EAxis::Y);
		const FVector Up = Axes.GetUnitAxis(EAxis::Z);
		const FVector Extents(FieldHalfLengthX + 100, FieldHalfWidthY + 100, PlayerHeight * 0.5f);
		const float HalfWidth = FVector::DotProduct(Right.GetAbs(), Extents);
		const float HalfHeight = FVector::DotProduct(Up.GetAbs(), Extents);
		ViewWidth = 2.1f * FMath::Max(HalfWidth, HalfHeight * (16.0f / 9.0f));
	}

	FActorSpawnParameters Params;
	ACameraActor* Camera =
		World->SpawnActor<ACameraActor>(
			ACameraActor::StaticClass(),
			CamLoc, CamRot, Params);

	if (Camera)
	{
		Camera->GetCameraComponent()->ProjectionMode = ECameraProjectionMode::Orthographic;
		Camera->GetCameraComponent()->OrthoWidth = ViewWidth;
		Camera->GetCameraComponent()->SetAspectRatio(16.0f / 9.0f);
		APlayerController* PC =
			UGameplayStatics::GetPlayerController(this, 0);
		if (PC)
			PC->SetViewTargetWithBlend(Camera, 0.0f);
	}
}

void ABoardManager::SpawnPlayers()
{
	Players.Empty();
	Players.SetNum(22);

	TArray<FVector2D> Red = {
		FVector2D(-2200, 0),
		FVector2D(-1600, -1000), FVector2D(-1600, -350),
		FVector2D(-1600, 350),   FVector2D(-1600, 1000),
		FVector2D(-800, -1000),  FVector2D(-800, -350),
		FVector2D(-800, 350),    FVector2D(-800, 1000),
		FVector2D(-200, -400),   FVector2D(-200, 400)
	};
	TArray<FVector2D> Blue = {
		FVector2D(2200, 0),
		FVector2D(1600, -1000), FVector2D(1600, -350),
		FVector2D(1600, 350),   FVector2D(1600, 1000),
		FVector2D(800, -1000),  FVector2D(800, -350),
		FVector2D(800, 350),    FVector2D(800, 1000),
		FVector2D(200, -400),   FVector2D(200, 400)
	};

	SpawnTeam(0, Red, 0);
	SpawnTeam(1, Blue, 11);
}

void ABoardManager::SpawnTeam(int32 TeamID,
	const TArray<FVector2D>& Positions, int32 StartID)
{
	UWorld* World = GetWorld();
	if (!World) return;
	const float Z = PieceZForRole();

	for (int32 i = 0; i < Positions.Num(); i++)
	{
		FVector Loc(Positions[i].X, Positions[i].Y, Z);
		FTransform Trans(FRotator::ZeroRotator, Loc);

		APlayerPiece* Piece =
			World->SpawnActorDeferred<APlayerPiece>(
				APlayerPiece::StaticClass(), Trans, this,
				nullptr,
				ESpawnActorCollisionHandlingMethod
					::AlwaysSpawn);
		if (Piece)
		{
			Piece->PlayerID = StartID + i;
			Piece->TeamID = TeamID;
			Piece->FinishSpawning(Trans);

			if (bIsSender) Piece->SetupAsDisc();
			else
			{
				USkeletalMesh* M = (TeamID == 0) ? RunnerMeshTeam0 : RunnerMeshTeam1;
				Piece->SetupAsRunner(M, PlayerWalkAnim, PlayerIdleAnim, ReceiverPlayerScale);
			}

			Players[StartID + i] = Piece;
			TargetPositions.Add(StartID + i, Loc);
		}
	}
}

FVector ABoardManager::ClampToField(
	const FVector& Pos) const
{
	FVector C = Pos;
	C.X = FMath::Clamp(C.X,
		-FieldHalfLengthX, FieldHalfLengthX);
	C.Y = FMath::Clamp(C.Y,
		-FieldHalfWidthY, FieldHalfWidthY);
	return C;
}

FVector ABoardManager::ResolveCollisions(
	APlayerPiece* Mover, const FVector& DesiredPos) const
{
	if (!Mover) return DesiredPos;
	FVector Result = DesiredPos;

	const int32 MaxPasses = 3;
	for (int32 Pass = 0; Pass < MaxPasses; Pass++)
	{
		bool bOverlap = false;
		for (APlayerPiece* Other : Players)
		{
			if (!Other || Other == Mover) continue;

			FVector OP = Other->GetActorLocation();
			float Dist = FVector::Dist2D(Result, OP);
			float MinD = Mover->CollisionRadius
				+ Other->CollisionRadius;

			if (Dist < MinD && Dist > 0.01f)
			{
				FVector Dir =
					(Result - OP).GetSafeNormal2D();
				Result = OP + Dir * MinD;
				Result.Z = PieceZForRole();
				bOverlap = true;
			}
		}
		if (!bOverlap) break;
	}
	return Result;
}

void ABoardManager::HandleDragInput()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC) return;

	float X = 0.0f, Y = 0.0f;
	bool bTouchDown = false;
	PC->GetInputTouchState(ETouchIndex::Touch1, X, Y, bTouchDown);
	bool bDown = bTouchDown;
	// A touch drag must never switch to a synthetic mouse event during a dropout.
	if (!bTouchDown && !(DraggedPlayer && bDragUsesTouch))
	{
		bDown = PC->IsInputKeyDown(EKeys::LeftMouseButton)
			&& PC->GetMousePosition(X, Y);
	}

	const double Now = GetWorld()->GetTimeSeconds();
	const bool bJustPressed = bDown && !bWasPointerDownLastFrame;
	bWasPointerDownLastFrame = bDown;
	if (!bDown)
	{
		// Freeze in place through brief hardware contact losses.
		if (DraggedPlayer && bDragUsesTouch && Now - LastPointerTime <= TouchReleaseGraceSeconds)
			return;
		DraggedPlayer = nullptr;
		return;
	}

	int32 ViewWidth = 0, ViewHeight = 0;
	PC->GetViewportSize(ViewWidth, ViewHeight);
	const float ScreenScale = FMath::Max(1.0f, float(ViewHeight)) / 1080.0f;
	const FVector2D Screen(X, Y);
	if (DraggedPlayer && bDragUsesTouch && bJustPressed)
	{
		// A new distant touch is a new gesture, not a continuation of the old one.
		if (Now - LastPointerTime > TouchReleaseGraceSeconds
			|| FVector2D::Distance(Screen, LastPointerScreen) > TouchResumeDistance * ScreenScale)
			DraggedPlayer = nullptr;
	}

	FVector RayOrigin, RayDirection;
	if (!PC->DeprojectScreenPositionToWorld(X, Y, RayOrigin, RayDirection)
		|| FMath::Abs(RayDirection.Z) < KINDA_SMALL_NUMBER)
		return;
	const float RayDistance = (PieceZForRole() - RayOrigin.Z) / RayDirection.Z;
	if (RayDistance < 0.0f) return;
	// Intersect a fixed plane: other discs cannot disturb the drag position.
	const FVector PointerWorld = RayOrigin + RayDirection * RayDistance;

	if (bJustPressed && !DraggedPlayer)
	{
		APlayerPiece* Selected = nullptr;
		FHitResult Hit;
		if (PC->GetHitResultAtScreenPosition(Screen, ECC_Visibility, false, Hit))
			Selected = Cast<APlayerPiece>(Hit.GetActor());
		if (!Selected && bTouchDown)
		{
			float BestDistance = TNumericLimits<float>::Max();
			for (APlayerPiece* Piece : Players)
			{
				if (!Piece) continue;
				FVector2D Center, Edge;
				const FVector Location = Piece->GetActorLocation();
				if (!PC->ProjectWorldLocationToScreen(Location, Center)
					|| !PC->ProjectWorldLocationToScreen(Location + PC->PlayerCameraManager->GetCameraRotation().RotateVector(FVector(0, Piece->CollisionRadius, 0)), Edge))
					continue;
				const float Distance = FVector2D::Distance(Screen, Center);
				const float Radius = FVector2D::Distance(Center, Edge) + TouchPickPadding * ScreenScale;
				if (Distance <= Radius && Distance < BestDistance)
				{
					BestDistance = Distance;
					Selected = Piece;
				}
			}
		}
		if (Selected)
		{
			DraggedPlayer = Selected;
			bDragUsesTouch = bTouchDown;
			DragOffset = Selected->GetActorLocation() - PointerWorld;
			DragOffset.Z = 0.0f;
		}
	}

	LastPointerTime = Now;
	LastPointerScreen = Screen;
	if (DraggedPlayer)
	{
		FVector Desired = PointerWorld + DragOffset;
		Desired.Z = PieceZForRole();
		Desired = ClampToField(Desired);
		Desired = ResolveCollisions(DraggedPlayer, Desired);
		Desired = ClampToField(Desired);
		DraggedPlayer->SetActorLocation(Desired);
		bPendingSend = true;
		PendingSendID = DraggedPlayer->PlayerID;
		PendingSendPos = Desired;
	}
}
void ABoardManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsSender)
	{
		ProcessPendingConnections();
		HandleDragInput();
		TickSendThrottle(DeltaTime);
	}
	else
	{
		TickReconnect(DeltaTime);
		ReceiveData();
		InterpolateReceiverPlayers(DeltaTime);
	}
	if (!bIsSender)
		for (APlayerPiece* Player : Players)
			if (Player) Player->UpdateLocomotion(DeltaTime);
}

void ABoardManager::TickSendThrottle(float DeltaTime)
{
	if (SendRateHz <= 0.0f)
	{
		if (bPendingSend)
		{
			SendPlayerPosition(
				PendingSendID, PendingSendPos);
			bPendingSend = false;
		}
		return;
	}

	SendAccumulator += DeltaTime;
	float Interval = 1.0f / SendRateHz;
	if (SendAccumulator >= Interval)
	{
		SendAccumulator = 0.0f;
		if (bPendingSend)
		{
			SendPlayerPosition(
				PendingSendID, PendingSendPos);
			bPendingSend = false;
		}
	}
}

bool ABoardManager::OnConnectionAccepted(
	FSocket* Socket, const FIPv4Endpoint& Endpoint)
{
	// OJO: este callback corre en OTRO hilo.
	// Solo encolamos; se procesa en el Tick.
	PendingConnections.Enqueue(Socket);
	return true;
}

void ABoardManager::ProcessPendingConnections()
{
	FSocket* New = nullptr;
	while (PendingConnections.Dequeue(New))
	{
		if (ClientSocket)
		{
			ClientSocket->Close();
			ISocketSubsystem::Get(
				PLATFORM_SOCKETSUBSYSTEM)
				->DestroySocket(ClientSocket);
		}
		ClientSocket = New;
		ClientSocket->SetNoDelay(true);
		UE_LOG(LogTemp, Log,
			TEXT("[SENDER] Cliente activo"));
		SendAllPositions();
	}
}

void ABoardManager::SendPlayerPosition(
	int32 ID, const FVector& Pos)
{
	if (!ClientSocket) return;

	FString Msg = FString::Printf(
		TEXT("%d,%.2f,%.2f,%.2f\n"),
		ID, Pos.X, Pos.Y, Pos.Z);

	FTCHARToUTF8 Conv(*Msg);
	const uint8* Data = (const uint8*)Conv.Get();
	int32 Total = Conv.Length();
	int32 Done = 0;

	while (Done < Total)
	{
		int32 Sent = 0;
		bool bOk = ClientSocket->Send(
			Data + Done, Total - Done, Sent);
		if (!bOk || Sent <= 0)
		{
			ClientSocket->Close();
			ISocketSubsystem::Get(
				PLATFORM_SOCKETSUBSYSTEM)
				->DestroySocket(ClientSocket);
			ClientSocket = nullptr;
			return;
		}
		Done += Sent;
	}
}

void ABoardManager::SendAllPositions()
{
	for (APlayerPiece* P : Players)
		if (P && ClientSocket)
			SendPlayerPosition(
				P->PlayerID, P->GetActorLocation());
}

void ABoardManager::ConnectToServer()
{
	bReceiverConnected = false;
	ReceiveBuffer.Empty();
	if (ReceiverSocket)
	{
		ReceiverSocket->Close();
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)
			->DestroySocket(ReceiverSocket);
		ReceiverSocket = nullptr;
	}

	ISocketSubsystem* SS =
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	if (!SS) return;

	FIPv4Address Addr;
	if (!FIPv4Address::Parse(ServerIP, Addr)) return;
	TSharedRef<FInternetAddr> RemoteAddr =
		SS->CreateInternetAddr();
	RemoteAddr->SetIp(Addr.Value);
	RemoteAddr->SetPort(ListenPort);

	ReceiverSocket = FTcpSocketBuilder(
		TEXT("ReceiverSocket"))
		.AsNonBlocking().AsReusable();
	if (!ReceiverSocket) return;

	bReceiverConnected =
		ReceiverSocket->Connect(*RemoteAddr);
	if (bReceiverConnected)
		UE_LOG(LogTemp, Log,
			TEXT("[RECEIVER] Conectando a %s:%d"),
			*ServerIP, ListenPort);
}

void ABoardManager::TickReconnect(float DeltaTime)
{
	if (ReceiverSocket)
	{
		ESocketConnectionState St =
			ReceiverSocket->GetConnectionState();
		if (St == SCS_Connected)
		{
			bReceiverConnected = true;
			ReconnectTimer = 0.0f;
			return;
		}
	}

	ReconnectTimer += DeltaTime;
	if (ReconnectTimer >= 3.0f)   // reintento cada 3 s
	{
		ReconnectTimer = 0.0f;
		UE_LOG(LogTemp, Log,
			TEXT("[RECEIVER] Reintentando conexion..."));
		ConnectToServer();
	}
}

void ABoardManager::ReceiveData()
{
	if (!ReceiverSocket) return;

	uint32 Pending = 0;
	while (ReceiverSocket->HasPendingData(Pending))
	{
		TArray<uint8> Buf;
		Buf.SetNumUninitialized(
			(int32)FMath::Min(Pending, 65507u));
		int32 Read = 0;
		if (!ReceiverSocket->Recv(
				Buf.GetData(), Buf.Num(), Read))
			break;
		if (Read <= 0) break;

		// Conversion con longitud explicita: el buffer
		// NO trae terminador nulo.
		// UE 5.8: el ctor FString(Len, Ptr) esta obsoleto
		// desde 5.4; se usa ConstructFromPtrSize.
		FUTF8ToTCHAR Conv(
			(const ANSICHAR*)Buf.GetData(), Read);
		ReceiveBuffer += FString::ConstructFromPtrSize(
			Conv.Get(), Conv.Length());

		// Procesar lineas completas (delimitador \n)
		int32 Idx;
		while (ReceiveBuffer.FindChar('\n', Idx))
		{
			FString Line = ReceiveBuffer.Left(Idx);
			ReceiveBuffer =
				ReceiveBuffer.RightChop(Idx + 1);
			ParseMessage(Line);
		}
	}
}

void ABoardManager::ParseMessage(const FString& Line)
{
	TArray<FString> Parts;
	Line.ParseIntoArray(Parts, TEXT(","), true);
	if (Parts.Num() < 4) return;

	int32 ID = FCString::Atoi(*Parts[0]);
	float X  = FCString::Atof(*Parts[1]);
	float Y  = FCString::Atof(*Parts[2]);
	// Ignoramos la Z entrante: el maniqui va a ras
	// de cesped en el Receiver.
	FVector Target(X, Y, ReceiverPieceZ);
	TargetPositions.Add(ID, Target);
}

void ABoardManager::InterpolateReceiverPlayers(
	float DeltaTime)
{
	for (APlayerPiece* Player : Players)
	{
		if (!Player) continue;
		FVector* Target =
			TargetPositions.Find(Player->PlayerID);
		if (!Target) continue;

		FVector Cur = Player->GetActorLocation();
		// Exponential smoothing is independent of the rendering frame rate.
		const float Alpha = InterpSpeed <= 0.0f ? 1.0f : 1.0f - FMath::Exp(-InterpSpeed * DeltaTime);
		FVector New = FMath::Lerp(Cur, *Target, Alpha);
		if (FVector::DistSquared2D(New, *Target) < 0.25f) New = *Target;
		New.Z = ReceiverPieceZ;

		Player->SetActorLocation(New);
	}
}

void ABoardManager::EndPlay(
	const EEndPlayReason::Type Reason)
{
	if (TcpListener)
	{
		TcpListener->Stop();
		delete TcpListener;
		TcpListener = nullptr;
	}

	ISocketSubsystem* SS =
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);

	if (ClientSocket && SS)
	{
		ClientSocket->Close();
		SS->DestroySocket(ClientSocket);
		ClientSocket = nullptr;
	}
	if (ReceiverSocket && SS)
	{
		ReceiverSocket->Close();
		SS->DestroySocket(ReceiverSocket);
		ReceiverSocket = nullptr;
	}

	// Vaciar conexiones pendientes sin procesar
	FSocket* Left = nullptr;
	while (PendingConnections.Dequeue(Left))
	{
		if (Left && SS)
		{
			Left->Close();
			SS->DestroySocket(Left);
		}
	}

	Super::EndPlay(Reason);
}
