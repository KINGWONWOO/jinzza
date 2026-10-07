// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaGameGameMode.h"
#include "jinzzaGamePlayerController.h"
#include "jinzzaGameGameState.h"
#include "jinzzaPartyPlayerState.h"
#include "jinzzaRoundPhaseSubsystem.h"
#include "jinzzaAuditionCurtain.h"
#include "jinzzaGameInstance.h"
#include "jinzzaLoadingSettings.h"
#include "jinzza.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "UObject/ConstructorHelpers.h"
#include "TimerManager.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

AjinzzaGameGameMode::AjinzzaGameGameMode()
{
	PlayerControllerClass = AjinzzaGamePlayerController::StaticClass();
	GameStateClass = AjinzzaGameGameState::StaticClass();
	PlayerStateClass = AjinzzaPartyPlayerState::StaticClass();
	bUseSeamlessTravel = true;

	// Lvl_Game used to spawn a bare flying ADefaultPawn - never wired to the actual first-person
	// character, so nobody had a mesh for disguise (or anything else) to apply to.
	static ConstructorHelpers::FClassFinder<APawn> CharacterBPClass(TEXT("/Game/JINZZA/FirstPerson/Blueprints/BP_FirstPersonCharacter"));
	if (CharacterBPClass.Succeeded())
	{
		DefaultPawnClass = CharacterBPClass.Class;
	}
}

void AjinzzaGameGameMode::EliminateToGhost(AjinzzaPartyPlayerState* Target)
{
	if (!HasAuthority() || !Target || Target->IsGhost())
	{
		return;
	}

	Target->ServerSetGhost(true);
}

void AjinzzaGameGameMode::StartPlay()
{
	Super::StartPlay();

	UGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	if (UjinzzaRoundPhaseSubsystem* RoundPhase = GI ? GI->GetSubsystem<UjinzzaRoundPhaseSubsystem>() : nullptr)
	{
		RoundPhase->OnServerPhaseEntered.AddUObject(this, &AjinzzaGameGameMode::OnRoundPhaseEntered);
	}
	// StartRound() is NOT called here - players arrive one at a time as each finishes loading this
	// level; calling StartRound() straight from StartPlay() assigned roles to whichever single player
	// (usually just the host) had arrived first, silently starving every later-arriving player of a
	// role (3-client PIE test, 2026-09-01). The round starts once every player has reported loaded -
	// see NotifyPlayerLoaded / OpenMatchStartGate.
	if (UjinzzaGameInstance* JinzzaGI = GetGameInstance<UjinzzaGameInstance>())
	{
		ExpectedPlayers = JinzzaGI->ConsumeExpectedMatchPlayers();
	}
	if (AjinzzaGameGameState* MatchState = GetGameState<AjinzzaGameGameState>())
	{
		MatchState->ServerSetLoadProgress(0, ExpectedPlayers, false);
	}
	GetWorldTimerManager().SetTimer(MatchStartTimeoutHandle, this, &AjinzzaGameGameMode::OnMatchStartTimeout,
		UjinzzaLoadingSettings::Get()->MatchStartTimeoutSeconds, false);
}

void AjinzzaGameGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	// Joined mid-match: next free User number, like everyone else who isn't the Judge.
	if (bAliasesAssigned)
	{
		if (AjinzzaPartyPlayerState* PartyPS = NewPlayer ? NewPlayer->GetPlayerState<AjinzzaPartyPlayerState>() : nullptr)
		{
			PartyPS->ServerSetDisplayAlias(FString::Printf(TEXT("User%d"), NextUserAliasNumber++));
		}
	}

	if (bAllPlayersLoaded)
	{
		return;
	}

	// Only matters when nobody told us how many players to expect (level opened directly): wait until
	// no one new has joined for a beat. Re-checked when the grace period runs out.
	LastJoinTime = FPlatformTime::Seconds();
	GetWorldTimerManager().SetTimer(RoundStartGraceTimerHandle, this, &AjinzzaGameGameMode::CheckAllPlayersLoaded, 1.6f, false);
}

void AjinzzaGameGameMode::AssignDisplayAliases(const TArray<AjinzzaPartyPlayerState*>& Players, AjinzzaPartyPlayerState* Judge)
{
	TArray<AjinzzaPartyPlayerState*> Candidates;
	for (AjinzzaPartyPlayerState* PartyPS : Players)
	{
		if (PartyPS && PartyPS != Judge)
		{
			Candidates.Add(PartyPS);
		}
	}
	for (int32 i = Candidates.Num() - 1; i > 0; --i)
	{
		Candidates.Swap(i, FMath::RandRange(0, i));
	}

	NextUserAliasNumber = 1;
	for (AjinzzaPartyPlayerState* Candidate : Candidates)
	{
		Candidate->ServerSetDisplayAlias(FString::Printf(TEXT("User%d"), NextUserAliasNumber++));
	}
	if (Judge)
	{
		Judge->ServerSetDisplayAlias(TEXT("Judge"));
	}
	bAliasesAssigned = true;
}

void AjinzzaGameGameMode::NotifyPlayerLoaded(APlayerController* Player)
{
	if (!Player || bAllPlayersLoaded)
	{
		return;
	}
	LoadedPlayers.Add(Player);
	CheckAllPlayersLoaded();
}

void AjinzzaGameGameMode::CheckAllPlayersLoaded()
{
	if (bAllPlayersLoaded || !GetWorld())
	{
		return;
	}

	int32 Current = 0;
	int32 Loaded = 0;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (APlayerController* PC = It->Get())
		{
			++Current;
			Loaded += LoadedPlayers.Contains(PC) ? 1 : 0;
		}
	}

	const int32 Expected = FMath::Max(ExpectedPlayers, Current);
	if (AjinzzaGameGameState* MatchState = GetGameState<AjinzzaGameGameState>())
	{
		MatchState->ServerSetLoadProgress(Loaded, Expected, false);
	}

	const bool bGraceOver = ExpectedPlayers > 0 || FPlatformTime::Seconds() - LastJoinTime >= 1.5;
	if (Current > 0 && Loaded >= Expected && bGraceOver)
	{
		OpenMatchStartGate(false);
	}
}

void AjinzzaGameGameMode::OpenMatchStartGate(bool bForce)
{
	if (bAllPlayersLoaded)
	{
		return;
	}
	bAllPlayersLoaded = true;
	GetWorldTimerManager().ClearTimer(MatchStartTimeoutHandle);
	GetWorldTimerManager().ClearTimer(RoundStartGraceTimerHandle);

	if (bForce)
	{
		UE_LOG(Logjinzza, Warning, TEXT("Not every player loaded the match in time - starting with %d loaded."), LoadedPlayers.Num());
	}

	if (AjinzzaGameGameState* MatchState = GetGameState<AjinzzaGameGameState>())
	{
		MatchState->ServerSetLoadProgress(LoadedPlayers.Num(), FMath::Max(ExpectedPlayers, LoadedPlayers.Num()), true);
	}

	TryStartRound();
}

void AjinzzaGameGameMode::Logout(AController* Exiting)
{
	const AjinzzaPartyPlayerState* PartyState = Exiting ? Exiting->GetPlayerState<AjinzzaPartyPlayerState>() : nullptr;
	const bool bRealOneLeft = PartyState && PartyState->ServerRole == EJinzzaPartyRole::RealOne;
	// The host's own controller only "leaves" when the server itself is shutting down.
	const bool bHostLeft = Exiting && Exiting->IsLocalController();

	Super::Logout(Exiting);

	UWorld* World = GetWorld();

	// Someone dropped before the match started - stop waiting for them.
	if (!bAllPlayersLoaded && World && !World->bIsTearingDown)
	{
		LoadedPlayers.Remove(Cast<APlayerController>(Exiting));
		ExpectedPlayers = FMath::Max(0, ExpectedPlayers - 1);
		GetWorldTimerManager().SetTimerForNextTick(this, &AjinzzaGameGameMode::CheckAllPlayersLoaded);
	}

	if (!bRealOneLeft || bHostLeft || bReturningToLobby || !World || World->bIsTearingDown)
	{
		return;
	}

	bReturningToLobby = true;
	UE_LOG(Logjinzza, Log, TEXT("Real One left mid-match - returning everyone to the lobby."));

	// Next tick, not from inside Logout - the leaving connection is still being torn down.
	GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		if (UjinzzaGameInstance* JinzzaGI = GetGameInstance<UjinzzaGameInstance>())
		{
			JinzzaGI->EndGameReturnToLobby();
		}
	}));
}

void AjinzzaGameGameMode::TryStartRound()
{
	if (bRoundStarted)
	{
		return;
	}
	bRoundStarted = true;

	UGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	if (UjinzzaRoundPhaseSubsystem* RoundPhase = GI ? GI->GetSubsystem<UjinzzaRoundPhaseSubsystem>() : nullptr)
	{
		RoundPhase->StartRound();
	}
}

void AjinzzaGameGameMode::OnRoundPhaseEntered(EJinzzaRoundPhase NewPhase)
{
	if (NewPhase == EJinzzaRoundPhase::RoleAssignment)
	{
		AssignRoles();
	}

	UpdateZoneForPhase(NewPhase);
}

FName AjinzzaGameGameMode::GetZoneTagForPhase(EJinzzaRoundPhase Phase)
{
	switch (Phase)
	{
	case EJinzzaRoundPhase::SelfIntroduction:
		return TEXT("Zone.SelfIntro");
	case EJinzzaRoundPhase::QuestionTime:
		return TEXT("Zone.Question");
	case EJinzzaRoundPhase::FreeTime1:
	case EJinzzaRoundPhase::FreeTime2:
	case EJinzzaRoundPhase::Interview:
		// Interview is reached FROM FreeTime by teleport only, same level, no separate "home"
		// zone of its own (design doc 8-6) - FreeTime stays the zone the majority of players are
		// in for this phase too.
		return TEXT("Zone.FreeTime");
	case EJinzzaRoundPhase::MidEvaluation:
	case EJinzzaRoundPhase::FinalDecision:
	case EJinzzaRoundPhase::RoundComplete:
		return TEXT("Zone.Evaluation");
	case EJinzzaRoundPhase::None:
	case EJinzzaRoundPhase::RoleAssignment:
	default:
		return TEXT("Zone.Lobby");
	}
}

void AjinzzaGameGameMode::UpdateZoneForPhase(EJinzzaRoundPhase NewPhase)
{
	if (!HasAuthority())
	{
		return;
	}

	// Leaving Interview (to any other phase) releases whoever was seated there.
	if (NewPhase != EJinzzaRoundPhase::Interview)
	{
		ExitInterviewZone();
	}

	static const FName AllZoneTags[] = {
		TEXT("Zone.Lobby"), TEXT("Zone.SelfIntro"), TEXT("Zone.Question"),
		TEXT("Zone.FreeTime"), TEXT("Zone.Interview"), TEXT("Zone.Evaluation")
	};

	const FName HomeZoneTag = GetZoneTagForPhase(NewPhase);
	// Interview is layered ON TOP OF FreeTime (same level, teleport-only per design doc 8-6), not
	// a replacement for it - most players stay in FreeTime while the Judge+candidate pair step
	// into Interview.
	const bool bShowInterviewZone = (NewPhase == EJinzzaRoundPhase::Interview);

	for (const FName& Tag : AllZoneTags)
	{
		const bool bVisible = (Tag == HomeZoneTag) || (bShowInterviewZone && Tag == TEXT("Zone.Interview"));

		TArray<AActor*> ZoneActors;
		UGameplayStatics::GetAllActorsWithTag(GetWorld(), Tag, ZoneActors);
		for (AActor* ZoneActor : ZoneActors)
		{
			if (ZoneActor)
			{
				ZoneActor->SetActorHiddenInGame(!bVisible);
				ZoneActor->SetActorEnableCollision(bVisible);
			}
		}
	}

	// Self-intro curtain: rises only for the duration of that phase.
	TArray<AActor*> Curtains;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AjinzzaAuditionCurtain::StaticClass(), Curtains);
	for (AActor* CurtainActor : Curtains)
	{
		if (AjinzzaAuditionCurtain* Curtain = Cast<AjinzzaAuditionCurtain>(CurtainActor))
		{
			(NewPhase == EJinzzaRoundPhase::SelfIntroduction) ? Curtain->Open() : Curtain->Close();
		}
	}

	if (NewPhase == EJinzzaRoundPhase::Interview)
	{
		// Interview only moves the designated pair - everyone else stays put in FreeTime.
		EnterInterviewZone();
		return;
	}

	// Every other phase moves everyone to the phase's home zone.
	AjinzzaGameGameState* JinzzaGameState = GetGameState<AjinzzaGameGameState>();
	if (!JinzzaGameState)
	{
		return;
	}

	TArray<AActor*> ZonePlayerStarts;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), HomeZoneTag, ZonePlayerStarts);
	ZonePlayerStarts.RemoveAll([](AActor* A) { return !A || !A->IsA<APlayerStart>(); });
	if (ZonePlayerStarts.Num() == 0)
	{
		return;
	}

	int32 Index = 0;
	for (APlayerState* PS : JinzzaGameState->PlayerArray)
	{
		APlayerController* PC = PS ? PS->GetPlayerController() : nullptr;
		APawn* Pawn = PC ? PC->GetPawn() : nullptr;
		if (!Pawn)
		{
			continue;
		}

		AActor* Spot = ZonePlayerStarts[Index % ZonePlayerStarts.Num()];
		Pawn->TeleportTo(Spot->GetActorLocation(), Spot->GetActorRotation());
		++Index;
	}
}

void AjinzzaGameGameMode::EnterInterviewZone()
{
	AjinzzaGameGameState* JinzzaGameState = GetGameState<AjinzzaGameGameState>();
	if (!JinzzaGameState)
	{
		return;
	}

	AjinzzaPartyPlayerState* Judge = nullptr;
	AjinzzaPartyPlayerState* Candidate = nullptr; // TODO(Week 7): use the real judge-designated interview target once that system exists.
	for (APlayerState* PS : JinzzaGameState->PlayerArray)
	{
		AjinzzaPartyPlayerState* PartyPS = Cast<AjinzzaPartyPlayerState>(PS);
		if (!PartyPS)
		{
			continue;
		}
		if (PartyPS->ServerRole == EJinzzaPartyRole::Judge)
		{
			Judge = PartyPS;
		}
		else if (!Candidate && PartyPS->ServerRole != EJinzzaPartyRole::None)
		{
			Candidate = PartyPS;
		}
	}

	if (!Judge || !Candidate)
	{
		return;
	}

	TArray<AActor*> JudgeSeats, CandidateSeats;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), TEXT("Zone.Interview.Seat.Judge"), JudgeSeats);
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), TEXT("Zone.Interview.Seat.Candidate"), CandidateSeats);
	if (JudgeSeats.Num() == 0 || CandidateSeats.Num() == 0)
	{
		return;
	}

	auto SeatPawn = [](APlayerState* PS, AActor* Seat) -> APawn*
	{
		APlayerController* PC = PS ? PS->GetPlayerController() : nullptr;
		ACharacter* Character = PC ? Cast<ACharacter>(PC->GetPawn()) : nullptr;
		if (!Character || !Seat)
		{
			return nullptr;
		}
		Character->TeleportTo(Seat->GetActorLocation(), Seat->GetActorRotation());
		if (UCharacterMovementComponent* Move = Character->GetCharacterMovement())
		{
			Move->DisableMovement();
		}
		return Character;
	};

	SeatedJudgePawn = SeatPawn(Judge, JudgeSeats[0]);
	SeatedCandidatePawn = SeatPawn(Candidate, CandidateSeats[0]);
}

void AjinzzaGameGameMode::ExitInterviewZone()
{
	auto Unseat = [](TWeakObjectPtr<APawn>& SeatedPawn)
	{
		if (ACharacter* Character = Cast<ACharacter>(SeatedPawn.Get()))
		{
			if (UCharacterMovementComponent* Move = Character->GetCharacterMovement())
			{
				Move->SetMovementMode(MOVE_Walking);
			}
		}
		SeatedPawn = nullptr;
	};

	Unseat(SeatedJudgePawn);
	Unseat(SeatedCandidatePawn);
}

void AjinzzaGameGameMode::AssignRoles()
{
	AjinzzaGameGameState* JinzzaGameState = GetGameState<AjinzzaGameGameState>();
	if (!JinzzaGameState)
	{
		return;
	}

	TArray<AjinzzaPartyPlayerState*> Players;
	for (APlayerState* PS : JinzzaGameState->PlayerArray)
	{
		if (AjinzzaPartyPlayerState* PartyPS = Cast<AjinzzaPartyPlayerState>(PS))
		{
			Players.Add(PartyPS);
		}
	}

	// Need at least 1 Real One + 1 Imitator + 1 Judge for the roles to mean anything.
	if (Players.Num() < 3)
	{
		UE_LOG(Logjinzza, Warning, TEXT("AssignRoles: need at least 3 players, got %d - skipping role assignment."), Players.Num());
		AssignDisplayAliases(Players, nullptr);
		return;
	}

	// Fisher-Yates shuffle. RoleAssignMethod == "Host Picks" (design doc's "후보자 투표" stretch
	// option, simplified to a host-choice in this project's lobby UI) needs a picker UI that
	// doesn't exist yet, so it falls back to Random for now too - see todo.txt.
	for (int32 i = Players.Num() - 1; i > 0; --i)
	{
		const int32 j = FMath::RandRange(0, i);
		Players.Swap(i, j);
	}

	AjinzzaPartyPlayerState* RealOne = Players[0];
	AjinzzaPartyPlayerState* Judge = Players[1];
	// 2-judge mode is marked experimental/stretch in the design doc (sections 9 and 15) with its
	// vote/interview rules explicitly undecided - only ever assigning 1 judge until that's spec'd.

	const EJinzzaFaceType RoundFace = static_cast<EJinzzaFaceType>(FMath::RandRange(1, 3)); // A/B/C
	const EJinzzaVoiceFilter RoundVoiceFilter = static_cast<EJinzzaVoiceFilter>(FMath::RandRange(1, 3)); // High/Low/Robot

	RealOne->ServerRole = EJinzzaPartyRole::RealOne;
	RealOne->ServerSetFaceType(RoundFace);
	RealOne->ServerSetVoiceFilter(RoundVoiceFilter);

	Judge->ServerRole = EJinzzaPartyRole::Judge;
	// The Judge isn't a candidate and gets no disguise.

	for (int32 i = 2; i < Players.Num(); ++i)
	{
		AjinzzaPartyPlayerState* Imitator = Players[i];
		Imitator->ServerRole = EJinzzaPartyRole::Imitator;
		// Imitators clone the Real One's face/voice disguise per the design doc's replication
		// rule. Full lobby-customization cloning (colors/outfit/accessories) is future work -
		// UCharacterCustomizationComponent (section 11) doesn't exist yet.
		Imitator->ServerSetFaceType(RoundFace);
		Imitator->ServerSetVoiceFilter(RoundVoiceFilter);
	}

	AssignDisplayAliases(Players, Judge);

	// Deliver role knowledge - each player learns only what the design doc says they're allowed
	// to know (info asymmetry is the whole point; see AjinzzaPartyPlayerState's class comment).
	for (AjinzzaPartyPlayerState* PartyPS : Players)
	{
		AjinzzaGamePlayerController* PC = Cast<AjinzzaGamePlayerController>(PartyPS->GetPlayerController());
		if (!PC)
		{
			continue;
		}

		switch (PartyPS->ServerRole)
		{
		case EJinzzaPartyRole::RealOne:
			PC->Client_ReceiveRoleAssignment(EJinzzaPartyRole::RealOne, nullptr);
			break;
		case EJinzzaPartyRole::Imitator:
			PC->Client_ReceiveRoleAssignment(EJinzzaPartyRole::Imitator, RealOne);
			break;
		case EJinzzaPartyRole::Judge:
			PC->Client_ReceiveRoleAssignment(EJinzzaPartyRole::Judge, nullptr);
			break;
		default:
			break;
		}
	}
}
