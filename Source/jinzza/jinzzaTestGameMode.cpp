// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaTestGameMode.h"
#include "jinzzaTestDummyController.h"
#include "jinzzaTestPlayerController.h"
#include "jinzzaTestDrillKiosk.h"
#include "jinzzaCharacter.h"
#include "jinzzaGameGameState.h"
#include "jinzzaPartyPlayerState.h"
#include "jinzza.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	const TCHAR* const TestSelfIntroLines[] = {
		TEXT("Hi everyone! I'm {0}, a completely ordinary seal."),
		TEXT("I like fish. A normal amount of fish."),
		TEXT("My hobby is lying on warm rocks."),
		TEXT("I was born on an iceberg. Obviously."),
		TEXT("Ask me anything about the ocean!"),
	};

	const TCHAR* const TestFinalArgumentLines[] = {
		TEXT("Wait, wait! I can explain!"),
		TEXT("I'm the real one, I promise!"),
		TEXT("You'll regret this, Judge..."),
		TEXT("At least let me keep my fish."),
		TEXT("Fine. Goodbye, cruel ocean."),
	};

	const TCHAR* const TestDummyQuestions[] = {
		TEXT("Draw your favorite food!"),
		TEXT("Draw where you'd go on vacation."),
		TEXT("Draw what you had for breakfast."),
		TEXT("Draw your dream house."),
		TEXT("Draw the best animal in the world."),
	};

	/** Chance a dummy hands in a drawing at all (the rest time out and show "?"). */
	constexpr float TestDummyAnswerChance = 0.85f;

	/** Number from a "<Prefix><N>" tag, MAX_int32 if there's none. */
	int32 TestTagNumber(const AActor& Actor, const FString& Prefix)
	{
		for (const FName& Tag : Actor.Tags)
		{
			const FString TagString = Tag.ToString();
			if (TagString.StartsWith(Prefix) && TagString.Len() > Prefix.Len())
			{
				return FCString::Atoi(*TagString.Mid(Prefix.Len()));
			}
		}
		return MAX_int32;
	}

	// --- Dummy doodles (canvas space, JinzzaSketch::CanvasSize) ---

	void TestDoodleStroke(FJinzzaDrawing& Drawing, const TArray<FVector2D>& Points, uint8 Color, uint8 Size)
	{
		FJinzzaDrawStroke& Stroke = Drawing.Strokes.AddDefaulted_GetRef();
		Stroke.Color = Color;
		Stroke.Size = Size;
		for (const FVector2D& Point : Points)
		{
			Stroke.Points.Add(static_cast<uint16>(FMath::Clamp(FMath::RoundToInt(Point.X), 0, static_cast<int32>(JinzzaSketch::CanvasSize.X))));
			Stroke.Points.Add(static_cast<uint16>(FMath::Clamp(FMath::RoundToInt(Point.Y), 0, static_cast<int32>(JinzzaSketch::CanvasSize.Y))));
		}
	}

	TArray<FVector2D> TestDoodleEllipse(const FVector2D& Center, float RadiusX, float RadiusY, float FromDeg = 0.f, float ToDeg = 360.f, int32 Segments = 28)
	{
		TArray<FVector2D> Points;
		for (int32 Index = 0; Index <= Segments; ++Index)
		{
			const float Angle = FMath::DegreesToRadians(FMath::Lerp(FromDeg, ToDeg, static_cast<float>(Index) / Segments));
			Points.Add(Center + FVector2D(FMath::Cos(Angle) * RadiusX, FMath::Sin(Angle) * RadiusY));
		}
		return Points;
	}
}

AjinzzaTestGameMode::AjinzzaTestGameMode()
{
	PlayerControllerClass = AjinzzaTestPlayerController::StaticClass();
	PrimaryActorTick.bCanEverTick = true;
	// One cycle per drill.
	QuestionCyclesOverride = 1;
}

void AjinzzaTestGameMode::StartPlay()
{
	Super::StartPlay();
	SpawnDummies();
	SpawnDrillStations();
}

void AjinzzaTestGameMode::SpawnDrillStations()
{
	UWorld* World = GetWorld();
	const UEnum* DrillEnum = StaticEnum<EJinzzaTestDrill>();
	if (!World || !DrillEnum)
	{
		return;
	}

	int32 Count = 0;
	for (int32 Index = 0; Index < DrillEnum->NumEnums() - 1; ++Index)
	{
		const EJinzzaTestDrill Drill = static_cast<EJinzzaTestDrill>(DrillEnum->GetValueByIndex(Index));
		TArray<AActor*> Markers;
		UGameplayStatics::GetAllActorsWithTag(World, FName(*(TEXT("Test.Drill.") + DrillEnum->GetNameStringByIndex(Index))), Markers);
		for (const AActor* Marker : Markers)
		{
			if (!Marker)
			{
				continue;
			}
			// Deferred, so Drill is set before the station builds its look.
			const FTransform Transform(FRotator(0.f, Marker->GetActorRotation().Yaw, 0.f), Marker->GetActorLocation());
			if (AjinzzaTestDrillKiosk* Station = World->SpawnActorDeferred<AjinzzaTestDrillKiosk>(AjinzzaTestDrillKiosk::StaticClass(), Transform))
			{
				Station->Drill = Drill;
				Station->FinishSpawning(Transform);
				++Count;
			}
		}
	}
	UE_LOG(Logjinzza, Log, TEXT("Test map: %d drill stations."), Count);
}

void AjinzzaTestGameMode::SpawnDummies()
{
	UWorld* World = GetWorld();
	if (!World || !DefaultPawnClass)
	{
		return;
	}

	auto Collect = [World](const TCHAR* Tag)
	{
		TArray<AActor*> Markers;
		UGameplayStatics::GetAllActorsWithTag(World, FName(Tag), Markers);
		Markers.RemoveAll([](const AActor* A) { return A == nullptr; });
		const FString Prefix = FString(Tag) + TEXT(".");
		Markers.Sort([&Prefix](const AActor& A, const AActor& B)
		{
			const int32 OrderA = TestTagNumber(A, Prefix);
			const int32 OrderB = TestTagNumber(B, Prefix);
			return OrderA != OrderB ? OrderA < OrderB : A.GetName() < B.GetName();
		});
		return Markers;
	};

	auto Spawn = [this, World](const AActor* Marker, bool bDrill, const FString& Name)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		const FRotator Facing(0.f, Marker->GetActorRotation().Yaw, 0.f);
		APawn* Pawn = World->SpawnActor<APawn>(DefaultPawnClass, Marker->GetActorLocation(), Facing, Params);
		AjinzzaTestDummyController* Controller = Pawn ? World->SpawnActor<AjinzzaTestDummyController>(Params) : nullptr;
		if (!Controller)
		{
			return;
		}
		Controller->bDrillDummy = bDrill;
		Controller->Possess(Pawn);
		Controller->HomeTransform = Pawn->GetActorTransform();
		if (APlayerState* PS = Controller->PlayerState)
		{
			PS->SetPlayerName(Name);
		}
		Dummies.Add(Controller);
	};

	int32 Letter = 0;
	for (const AActor* Marker : Collect(TEXT("Test.Dummy")))
	{
		Spawn(Marker, true, FString::Printf(TEXT("Dummy %c"), TCHAR('A' + Letter++ % 26)));
	}
	for (const AActor* Marker : Collect(TEXT("Test.TargetDummy")))
	{
		Spawn(Marker, false, TEXT("Target Dummy"));
	}
	UE_LOG(Logjinzza, Log, TEXT("Test map: %d practice dummies (%d for drills)."), Dummies.Num(), GetDrillDummies(100).Num());
}

TArray<AjinzzaPartyPlayerState*> AjinzzaTestGameMode::GetDrillDummies(int32 Count) const
{
	TArray<AjinzzaPartyPlayerState*> Result;
	for (const AjinzzaTestDummyController* Dummy : Dummies)
	{
		AjinzzaPartyPlayerState* PS = Dummy && Dummy->bDrillDummy ? Dummy->GetPlayerState<AjinzzaPartyPlayerState>() : nullptr;
		if (PS && Dummy->GetPawn() && Result.Num() < Count)
		{
			Result.Add(PS);
		}
	}
	return Result;
}

bool AjinzzaTestGameMode::IsDummy(const APlayerState* PlayerState) const
{
	return PlayerState && Cast<AjinzzaTestDummyController>(PlayerState->GetOwningController()) != nullptr;
}

void AjinzzaTestGameMode::SendDummiesHome()
{
	for (AjinzzaTestDummyController* Dummy : Dummies)
	{
		ACharacter* Character = Dummy ? Cast<ACharacter>(Dummy->GetPawn()) : nullptr;
		if (!Character)
		{
			continue;
		}
		if (AjinzzaCharacter* JinzzaCharacter = Cast<AjinzzaCharacter>(Character); JinzzaCharacter && JinzzaCharacter->IsSeated())
		{
			JinzzaCharacter->ServerSetSeated(false, 0.f);
		}
		if (UCharacterMovementComponent* Move = Character->GetCharacterMovement(); Move && Move->MovementMode == MOVE_None)
		{
			Move->SetMovementMode(MOVE_Walking);
		}
		Character->TeleportTo(Dummy->HomeTransform.GetLocation(), Dummy->HomeTransform.Rotator());
		Dummy->SetControlRotation(Dummy->HomeTransform.Rotator());
	}
}

void AjinzzaTestGameMode::ResetDrill()
{
	CancelTurnsAndVote();
	SelfIntroIndex = INDEX_NONE;

	if (const AjinzzaGameGameState* MatchState = GetGameState<AjinzzaGameGameState>())
	{
		for (APlayerState* PS : MatchState->PlayerArray)
		{
			if (AjinzzaPartyPlayerState* PartyPS = Cast<AjinzzaPartyPlayerState>(PS))
			{
				PartyPS->ServerRole = EJinzzaPartyRole::None;
				PartyPS->ServerSetDisplayAlias(FString());
				if (PartyPS->IsGhost())
				{
					PartyPS->ServerSetGhost(false);
				}
			}
		}
	}

	SendDummiesHome();
	bDrillRunning = false;
	DummyAnswerTimes.Reset();
	DummyAskTime = 0.0;
}

void AjinzzaTestGameMode::StartDrill(EJinzzaTestDrill Drill, APlayerController* Player)
{
	ResetDrill();

	AjinzzaPartyPlayerState* Me = Player ? Player->GetPlayerState<AjinzzaPartyPlayerState>() : nullptr;
	if (Drill == EJinzzaTestDrill::Reset || !Me)
	{
		return;
	}

	auto Alias = [](AjinzzaPartyPlayerState* PS, const FString& Name) { PS->ServerSetDisplayAlias(Name); };
	auto MakeJudge = [&Alias](AjinzzaPartyPlayerState* PS)
	{
		PS->ServerRole = EJinzzaPartyRole::Judge;
		Alias(PS, TEXT("Judge"));
	};

	switch (Drill)
	{
	case EJinzzaTestDrill::SelfIntroduction:
	{
		// A dummy goes first (watch it from the audience), then it's your turn on the spotlight.
		const TArray<AjinzzaPartyPlayerState*> Others = GetDrillDummies(1);
		for (int32 Index = 0; Index < Others.Num(); ++Index)
		{
			Alias(Others[Index], FString::Printf(TEXT("User%d"), Index + 1));
		}
		Alias(Me, FString::Printf(TEXT("User%d"), Others.Num() + 1));
		StartSelfIntroductions();
		break;
	}
	case EJinzzaTestDrill::QuestionAsJudge:
	{
		MakeJudge(Me);
		const TArray<AjinzzaPartyPlayerState*> Others = GetDrillDummies(4);
		for (int32 Index = 0; Index < Others.Num(); ++Index)
		{
			Alias(Others[Index], FString::Printf(TEXT("User%d"), Index + 1));
		}
		StartQuestionTime();
		break;
	}
	case EJinzzaTestDrill::QuestionAsCandidate:
	{
		// Dummy A is the Judge; you sit second among the candidates.
		const TArray<AjinzzaPartyPlayerState*> Others = GetDrillDummies(4);
		int32 Number = 1;
		for (int32 Index = 0; Index < Others.Num(); ++Index)
		{
			if (Index == 0)
			{
				MakeJudge(Others[Index]);
				continue;
			}
			Alias(Others[Index], FString::Printf(TEXT("User%d"), Number++));
			if (Index == 1)
			{
				Alias(Me, FString::Printf(TEXT("User%d"), Number++));
			}
		}
		if (Others.Num() <= 1)
		{
			Alias(Me, TEXT("User1"));
		}
		StartQuestionTime();
		break;
	}
	case EJinzzaTestDrill::Vote:
	{
		MakeJudge(Me);
		// The ballot only shows for a player who knows they're the Judge.
		if (AjinzzaGamePlayerController* GamePC = Cast<AjinzzaGamePlayerController>(Player))
		{
			GamePC->Client_ReceiveRoleAssignment(EJinzzaPartyRole::Judge, nullptr);
		}
		const TArray<AjinzzaPartyPlayerState*> Others = GetDrillDummies(3);
		for (int32 Index = 0; Index < Others.Num(); ++Index)
		{
			Alias(Others[Index], FString::Printf(TEXT("User%d"), Index + 1));
		}
		OpenVote(EJinzzaRoundPhase::MidEvaluation);
		break;
	}
	default:
		return;
	}

	bDrillRunning = true;
	ActiveDrill = Drill;
	DrillStartTime = GetWorld()->GetTimeSeconds();
	DrillIdleSeconds = 0.f;
	TalkTurnSerial = -1;
	QuestionSerialHandled = -1;
	UE_LOG(Logjinzza, Log, TEXT("Test map: drill %s started."), *UEnum::GetValueAsString(Drill));
}

bool AjinzzaTestGameMode::IsDrillBusy() const
{
	const AjinzzaGameGameState* MatchState = GetGameState<AjinzzaGameGameState>();
	return MatchState && (MatchState->IsSpeakTurnActive() || MatchState->IsVoteOpen() || MatchState->IsQuestionTimeActive()
		|| CondemnedPlayer.IsValid() || SelfIntroOrder.Num() > 0);
}

void AjinzzaTestGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bDrillRunning)
	{
		return;
	}

	const double Now = GetWorld()->GetTimeSeconds();
	UpdateDummyTalk(Now);
	UpdateDummyQuestion(Now);

	// Over once nothing has been going on for a second: everyone back to where they started.
	if (Now - DrillStartTime > 1.0 && !IsDrillBusy())
	{
		DrillIdleSeconds += DeltaSeconds;
		if (DrillIdleSeconds > 1.f)
		{
			bDrillRunning = false;
			SendDummiesHome();
			UE_LOG(Logjinzza, Log, TEXT("Test map: drill %s finished."), *UEnum::GetValueAsString(ActiveDrill));
		}
	}
	else
	{
		DrillIdleSeconds = 0.f;
	}
}

void AjinzzaTestGameMode::UpdateDummyTalk(double Now)
{
	const AjinzzaGameGameState* MatchState = GetGameState<AjinzzaGameGameState>();
	if (!MatchState || !MatchState->IsSpeakTurnActive())
	{
		return;
	}
	const FJinzzaSpeakTurn& Turn = MatchState->GetSpeakTurn();
	if (!IsDummy(Turn.Speaker))
	{
		return;
	}

	if (Turn.Serial != TalkTurnSerial)
	{
		TalkTurnSerial = Turn.Serial;
		TalkLineIndex = 0;
		NextTalkTime = Now + 1.2;
	}
	if (Now < NextTalkTime)
	{
		return;
	}

	const bool bIntro = Turn.Kind == EJinzzaSpeakTurnKind::SelfIntroduction;
	const int32 NumLines = static_cast<int32>(bIntro ? UE_ARRAY_COUNT(TestSelfIntroLines) : UE_ARRAY_COUNT(TestFinalArgumentLines));
	if (TalkLineIndex >= NumLines)
	{
		return;
	}
	const FString Line = bIntro
		? (TalkLineIndex == 0
			? FString::Format(TestSelfIntroLines[0], { AjinzzaPartyPlayerState::GetDisplayNameFor(Turn.Speaker) })
			: FString(TestSelfIntroLines[TalkLineIndex]))
		: FString(TestFinalArgumentLines[TalkLineIndex]);
	++TalkLineIndex;
	NextTalkTime = Now + 3.0;
	BroadcastTurnMessage(Turn.Speaker, Line);
}

void AjinzzaTestGameMode::UpdateDummyQuestion(double Now)
{
	if (!QuestionState.IsActive())
	{
		return;
	}

	if (QuestionState.Serial != QuestionSerialHandled)
	{
		QuestionSerialHandled = QuestionState.Serial;
		if (QuestionState.Step == EJinzzaQuestionStep::Asking)
		{
			const bool bDummyAsks = QuestionState.Seats.ContainsByPredicate([this](const FJinzzaQuestionSeat& Seat) { return !Seat.bAnswerer && IsDummy(Seat.Player); });
			DummyAskTime = bDummyAsks ? Now + FMath::FRandRange(3.f, 5.f) : 0.0;
		}
		else if (QuestionState.Step == EJinzzaQuestionStep::Answering)
		{
			// Each dummy hands in at its own moment (so the "answered" count ticks up), and now and then one doesn't.
			DummyAnswerTimes.Reset();
			for (const FJinzzaQuestionSeat& Seat : QuestionState.Seats)
			{
				if (Seat.bAnswerer && IsDummy(Seat.Player) && FMath::FRand() < TestDummyAnswerChance)
				{
					DummyAnswerTimes.Add(Seat.Player.Get(), Now + FMath::FRandRange(2.5f, 11.5f));
				}
			}
		}
	}

	if (QuestionState.Step == EJinzzaQuestionStep::Asking && DummyAskTime > 0.0 && Now >= DummyAskTime)
	{
		DummyAskTime = 0.0;
		for (const FJinzzaQuestionSeat& Seat : QuestionState.Seats)
		{
			if (!Seat.bAnswerer && IsDummy(Seat.Player))
			{
				SubmitQuestionFrom(Seat.Player, TestDummyQuestions[FMath::RandRange(0, static_cast<int32>(UE_ARRAY_COUNT(TestDummyQuestions)) - 1)]);
				break;
			}
		}
		return;
	}

	if (QuestionState.Step == EJinzzaQuestionStep::Answering)
	{
		TArray<TWeakObjectPtr<APlayerState>> Due;
		for (const TPair<TWeakObjectPtr<APlayerState>, double>& Entry : DummyAnswerTimes)
		{
			if (Now >= Entry.Value)
			{
				Due.Add(Entry.Key);
			}
		}
		for (const TWeakObjectPtr<APlayerState>& Answerer : Due)
		{
			DummyAnswerTimes.Remove(Answerer);
			// Handing in can end the step (everyone's in) - stop then.
			if (QuestionState.Step != EJinzzaQuestionStep::Answering)
			{
				break;
			}
			if (APlayerState* PS = Answerer.Get(); PS && !QuestionState.HasSubmitted(PS))
			{
				SubmitAnswerDrawing(PS, MakeDoodle());
			}
		}
	}
}

FJinzzaDrawing AjinzzaTestGameMode::MakeDoodle()
{
	// Palette indices (JinzzaSketch::GetPalette): 1 black, 2 red, 3 blue, 4 green, 5 orange, 6 purple, 7 pink, 8 brown.
	FJinzzaDrawing Drawing;
	const FVector2D Center = JinzzaSketch::CanvasSize * 0.5 + FVector2D(FMath::FRandRange(-70.f, 70.f), FMath::FRandRange(-40.f, 40.f));
	const float Scale = FMath::FRandRange(0.8f, 1.15f);
	auto At = [&Center, Scale](float X, float Y) { return Center + FVector2D(X, Y) * Scale; };

	switch (FMath::RandRange(0, 5))
	{
	case 0: // Sun
		TestDoodleStroke(Drawing, TestDoodleEllipse(Center, 90.f * Scale, 90.f * Scale), 5, 2);
		for (int32 Ray = 0; Ray < 8; ++Ray)
		{
			const FVector2D Dir(FMath::Cos(Ray * PI / 4.f), FMath::Sin(Ray * PI / 4.f));
			TestDoodleStroke(Drawing, { Center + Dir * 120.f * Scale, Center + Dir * 175.f * Scale }, 5, 1);
		}
		break;
	case 1: // House
		TestDoodleStroke(Drawing, { At(-110, -10), At(110, -10), At(110, 150), At(-110, 150), At(-110, -10) }, 8, 1);
		TestDoodleStroke(Drawing, { At(-140, -10), At(0, -140), At(140, -10) }, 2, 1);
		TestDoodleStroke(Drawing, { At(-25, 150), At(-25, 70), At(25, 70), At(25, 150) }, 3, 1);
		break;
	case 2: // Smiley
		TestDoodleStroke(Drawing, TestDoodleEllipse(Center, 150.f * Scale, 150.f * Scale), 1, 1);
		TestDoodleStroke(Drawing, { At(-50, -40) }, 1, 2);
		TestDoodleStroke(Drawing, { At(50, -40) }, 1, 2);
		TestDoodleStroke(Drawing, TestDoodleEllipse(Center + FVector2D(0.f, 10.f) * Scale, 85.f * Scale, 70.f * Scale, 20.f, 160.f, 14), 2, 1);
		break;
	case 3: // Fish
		TestDoodleStroke(Drawing, TestDoodleEllipse(Center, 150.f * Scale, 80.f * Scale), 3, 1);
		TestDoodleStroke(Drawing, { At(140, 0), At(230, -70), At(230, 70), At(140, 0) }, 3, 1);
		TestDoodleStroke(Drawing, { At(-85, -20) }, 1, 2);
		break;
	case 4: // Star
	{
		TArray<FVector2D> Star;
		for (int32 Point = 0; Point <= 10; ++Point)
		{
			const float Radius = (Point % 2 == 0 ? 170.f : 70.f) * Scale;
			const float Angle = -PI / 2.f + Point * PI / 5.f;
			Star.Add(Center + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius);
		}
		TestDoodleStroke(Drawing, Star, 5, 1);
		break;
	}
	default: // Flower
		TestDoodleStroke(Drawing, { At(0, 30), At(-10, 120), At(0, 220) }, 4, 1);
		for (int32 Petal = 0; Petal < 5; ++Petal)
		{
			const float Angle = Petal * 2.f * PI / 5.f;
			TestDoodleStroke(Drawing, TestDoodleEllipse(At(FMath::Cos(Angle) * 70.f, -60.f + FMath::Sin(Angle) * 70.f), 42.f * Scale, 42.f * Scale, 0.f, 360.f, 16), 7, 1);
		}
		TestDoodleStroke(Drawing, { At(0, -60) }, 5, 2);
		break;
	}

	// Sometimes a scribbled underline in another color.
	if (FMath::FRand() < 0.35f)
	{
		TArray<FVector2D> Wave;
		for (int32 Index = 0; Index <= 12; ++Index)
		{
			Wave.Add(FVector2D(220.f + Index * 30.f, 540.f + FMath::Sin(Index * 1.3f) * 14.f));
		}
		TestDoodleStroke(Drawing, Wave, static_cast<uint8>(FMath::RandRange(1, 8)), 0);
	}

	JinzzaSketch::Sanitize(Drawing);
	return Drawing;
}
