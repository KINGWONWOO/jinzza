// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaGameGameMode.h"
#include "jinzzaGamePlayerController.h"
#include "jinzzaGameGameState.h"
#include "jinzzaPartyPlayerState.h"
#include "jinzzaRoundPhaseSubsystem.h"
#include "jinzzaAuditionCurtain.h"
#include "jinzzaGameInstance.h"
#include "jinzzaLoadingSettings.h"
#include "jinzzaChatBoardComponent.h"
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
	const AjinzzaGameGameState* MatchStateBefore = GetGameState<AjinzzaGameGameState>();
	const bool bSpeakerLeft = MatchStateBefore && PartyState && MatchStateBefore->IsTurnSpeaker(PartyState);
	const bool bWasSelfIntro = bSpeakerLeft && MatchStateBefore->GetSpeakTurn().Kind == EJinzzaSpeakTurnKind::SelfIntroduction;
	if (PartyState && PartyState == CondemnedPlayer.Get())
	{
		// They left - nothing to eliminate, but still end the phase after the (now empty) argument.
		CondemnedPlayer.Reset();
	}

	Super::Logout(Exiting);

	UWorld* World = GetWorld();

	if (World && !World->bIsTearingDown)
	{
		if (bSpeakerLeft)
		{
			// Their spotlight turn is over - move on next tick (they're still mid-removal here).
			SpotlightPawn.Reset();
			if (bWasSelfIntro)
			{
				GetWorldTimerManager().SetTimerForNextTick(this, &AjinzzaGameGameMode::AdvanceSelfIntroduction);
			}
			else
			{
				GetWorldTimerManager().SetTimerForNextTick(this, &AjinzzaGameGameMode::FinishFinalArgument);
			}
		}

		// A Judge left mid-vote: their vote is gone; if the rest have voted, close now.
		if (const AjinzzaGameGameState* MatchState = GetGameState<AjinzzaGameGameState>(); MatchState && MatchState->IsVoteOpen())
		{
			Votes.Remove(Cast<APlayerController>(Exiting));
			GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]()
			{
				const int32 Judges = CountConnectedJudges();
				if (Judges > 0 && Votes.Num() >= Judges)
				{
					CloseVote();
				}
			}));
		}
	}

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
	// Before the zone teleport, so a spotlight speaker is put back with everyone else first.
	CancelTurnsAndVote();

	if (NewPhase == EJinzzaRoundPhase::RoleAssignment)
	{
		AssignRoles();
	}

	UpdateZoneForPhase(NewPhase);

	if (NewPhase == EJinzzaRoundPhase::SelfIntroduction)
	{
		StartSelfIntroductions();
	}
	else if (NewPhase == EJinzzaRoundPhase::MidEvaluation || NewPhase == EJinzzaRoundPhase::FinalDecision)
	{
		OpenVote(NewPhase);
	}
}

// --- Speaking turns -------------------------------------------------------------------------------

TArray<AjinzzaPartyPlayerState*> AjinzzaGameGameMode::GetLivingCandidates() const
{
	TArray<AjinzzaPartyPlayerState*> Candidates;
	if (const AjinzzaGameGameState* MatchState = GetGameState<AjinzzaGameGameState>())
	{
		for (APlayerState* PS : MatchState->PlayerArray)
		{
			AjinzzaPartyPlayerState* PartyPS = Cast<AjinzzaPartyPlayerState>(PS);
			if (PartyPS && PartyPS->IsLivingCandidate() && PartyPS->ServerRole != EJinzzaPartyRole::Judge)
			{
				Candidates.Add(PartyPS);
			}
		}
	}
	Candidates.Sort([](const AjinzzaPartyPlayerState& A, const AjinzzaPartyPlayerState& B)
	{
		return A.GetAliasUserNumber() < B.GetAliasUserNumber();
	});
	return Candidates;
}

int32 AjinzzaGameGameMode::CountConnectedJudges() const
{
	int32 Count = 0;
	if (const AjinzzaGameGameState* MatchState = GetGameState<AjinzzaGameGameState>())
	{
		for (APlayerState* PS : MatchState->PlayerArray)
		{
			const AjinzzaPartyPlayerState* PartyPS = Cast<AjinzzaPartyPlayerState>(PS);
			Count += (PartyPS && PartyPS->ServerRole == EJinzzaPartyRole::Judge && PartyPS->GetPlayerController()) ? 1 : 0;
		}
	}
	return Count;
}

void AjinzzaGameGameMode::StartSelfIntroductions()
{
	SelfIntroOrder.Reset();
	for (AjinzzaPartyPlayerState* Candidate : GetLivingCandidates())
	{
		SelfIntroOrder.Add(Candidate);
	}
	SelfIntroIndex = INDEX_NONE;

	UE_LOG(Logjinzza, Log, TEXT("Self-introduction: %d candidates, User1 first."), SelfIntroOrder.Num());
	AdvanceSelfIntroduction();
}

void AjinzzaGameGameMode::AdvanceSelfIntroduction()
{
	EndSpeakTurn();

	// Next candidate still in the game (someone may have left mid-phase).
	while (++SelfIntroIndex < SelfIntroOrder.Num())
	{
		AjinzzaPartyPlayerState* Speaker = SelfIntroOrder[SelfIntroIndex].Get();
		if (Speaker && Speaker->GetPlayerController())
		{
			BeginSpeakTurn(EJinzzaSpeakTurnKind::SelfIntroduction, Speaker, SelfIntroSecondsPerCandidate,
				SelfIntroIndex + 1, SelfIntroOrder.Num(), TEXT("Zone.SelfIntro"));
			GetWorldTimerManager().SetTimer(SpeakTurnTimerHandle, this, &AjinzzaGameGameMode::AdvanceSelfIntroduction, SelfIntroSecondsPerCandidate, false);
			return;
		}
	}

	// Everyone has spoken - don't wait out the rest of the phase timer.
	SelfIntroOrder.Reset();
	if (UjinzzaRoundPhaseSubsystem* RoundPhase = GetGameInstance() ? GetGameInstance()->GetSubsystem<UjinzzaRoundPhaseSubsystem>() : nullptr)
	{
		RoundPhase->NotifyPhaseConditionMet(EJinzzaRoundPhase::SelfIntroduction);
	}
}

void AjinzzaGameGameMode::BeginSpeakTurn(EJinzzaSpeakTurnKind Kind, AjinzzaPartyPlayerState* Speaker, float Seconds, int32 Number, int32 Total, const FString& ZoneTag)
{
	AjinzzaGameGameState* MatchState = GetGameState<AjinzzaGameGameState>();
	if (!MatchState || !Speaker)
	{
		return;
	}

	auto FindTagged = [this](const FString& Tag) -> AActor*
	{
		TArray<AActor*> Found;
		UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName(*Tag), Found);
		return Found.Num() > 0 ? Found[0] : nullptr;
	};

	// Nobody holds a board up during a turn - the speaker's lines become speech bubbles.
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (APawn* Pawn = It->Get() ? It->Get()->GetPawn() : nullptr)
		{
			if (UjinzzaChatBoardComponent* Board = Pawn->FindComponentByClass<UjinzzaChatBoardComponent>())
			{
				Board->ServerHideNow();
			}
		}
	}

	APlayerController* SpeakerPC = Speaker->GetPlayerController();
	APawn* Pawn = SpeakerPC ? SpeakerPC->GetPawn() : nullptr;
	if (AActor* Spotlight = FindTagged(ZoneTag + TEXT(".Spotlight")); Spotlight && Pawn)
	{
		SpotlightPawn = Pawn;
		SpotlightReturnLocation = Pawn->GetActorLocation();
		SpotlightReturnRotation = Pawn->GetActorRotation();
		Pawn->TeleportTo(Spotlight->GetActorLocation(), Spotlight->GetActorRotation());
		// Control rotation is the owning client's - turn them to face the audience there.
		SpeakerPC->ClientSetRotation(Spotlight->GetActorRotation());
	}
	else if (!Spotlight)
	{
		UE_LOG(Logjinzza, Warning, TEXT("No '%s.Spotlight' actor in the level - the speaker stays where they are."), *ZoneTag);
	}

	FJinzzaSpeakTurn Turn;
	Turn.Kind = Kind;
	Turn.Speaker = Speaker;
	Turn.EndServerTime = MatchState->GetServerWorldTimeSeconds() + Seconds;
	Turn.Number = Number;
	Turn.Total = Total;
	Turn.Serial = ++SpeakTurnSerial;

	if (AActor* CameraMarker = FindTagged(ZoneTag + TEXT(".Camera")))
	{
		Turn.CameraLocation = CameraMarker->GetActorLocation();
		Turn.CameraRotation = CameraMarker->GetActorRotation();
	}
	else if (Pawn)
	{
		// 2.6 m in front of the speaker, a little above head height, looking at their face.
		const FVector Face = Pawn->GetActorLocation() + FVector(0.f, 0.f, 60.f);
		// The first-person character's yaw follows its controller, so this is where they're facing.
		const FVector Forward = Pawn->GetActorForwardVector().GetSafeNormal2D();
		Turn.CameraLocation = Face + Forward * 260.f + FVector(0.f, 0.f, 25.f);
		Turn.CameraRotation = (Face - Turn.CameraLocation).Rotation();
	}

	MatchState->ServerSetSpeakTurn(Turn);
	UE_LOG(Logjinzza, Log, TEXT("Speak turn %d/%d: %s (%s)."), Number, Total, *Speaker->GetDisplayName(),
		Kind == EJinzzaSpeakTurnKind::FinalArgument ? TEXT("final argument") : TEXT("self-introduction"));
}

void AjinzzaGameGameMode::EndSpeakTurn()
{
	GetWorldTimerManager().ClearTimer(SpeakTurnTimerHandle);

	if (APawn* Pawn = SpotlightPawn.Get())
	{
		Pawn->TeleportTo(SpotlightReturnLocation, SpotlightReturnRotation);
	}
	SpotlightPawn.Reset();

	if (AjinzzaGameGameState* MatchState = GetGameState<AjinzzaGameGameState>())
	{
		if (MatchState->IsSpeakTurnActive())
		{
			FJinzzaSpeakTurn Cleared;
			Cleared.Serial = ++SpeakTurnSerial;
			MatchState->ServerSetSpeakTurn(Cleared);
		}
	}
}

void AjinzzaGameGameMode::BroadcastTurnMessage(APlayerState* Speaker, const FString& Text)
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (AjinzzaGamePlayerController* PC = Cast<AjinzzaGamePlayerController>(It->Get()))
		{
			PC->Client_ReceiveTurnMessage(Speaker, Text);
		}
	}
}

// --- Judge vote -----------------------------------------------------------------------------------

void AjinzzaGameGameMode::OpenVote(EJinzzaRoundPhase Phase)
{
	AjinzzaGameGameState* MatchState = GetGameState<AjinzzaGameGameState>();
	if (!MatchState)
	{
		return;
	}

	VotePhase = Phase;
	Votes.Reset();
	CondemnedPlayer.Reset();

	if (GetLivingCandidates().Num() == 0)
	{
		UE_LOG(Logjinzza, Warning, TEXT("Vote: no living candidates - skipping."));
		return;
	}

	MatchState->ServerSetVote(true, Phase, MatchState->GetServerWorldTimeSeconds() + VoteSeconds);
	GetWorldTimerManager().SetTimer(VoteTimerHandle, this, &AjinzzaGameGameMode::CloseVote, VoteSeconds, false);

	if (CountConnectedJudges() == 0)
	{
		UE_LOG(Logjinzza, Warning, TEXT("Vote: no Judge connected - the vote will be settled at random."));
	}
}

void AjinzzaGameGameMode::HandleVote(APlayerController* Voter, APlayerState* Target)
{
	const AjinzzaGameGameState* MatchState = GetGameState<AjinzzaGameGameState>();
	const AjinzzaPartyPlayerState* VoterState = Voter ? Voter->GetPlayerState<AjinzzaPartyPlayerState>() : nullptr;
	AjinzzaPartyPlayerState* TargetState = Cast<AjinzzaPartyPlayerState>(Target);
	if (!MatchState || !MatchState->IsVoteOpen() || !VoterState || VoterState->ServerRole != EJinzzaPartyRole::Judge
		|| !TargetState || !TargetState->IsLivingCandidate() || TargetState->ServerRole == EJinzzaPartyRole::Judge)
	{
		return;
	}

	Votes.Add(Voter, TargetState);
	UE_LOG(Logjinzza, Log, TEXT("Vote: a Judge voted for %s."), *TargetState->GetDisplayName());

	// Every Judge has voted - no need to wait out the timer.
	if (Votes.Num() >= CountConnectedJudges())
	{
		CloseVote();
	}
}

void AjinzzaGameGameMode::CloseVote()
{
	AjinzzaGameGameState* MatchState = GetGameState<AjinzzaGameGameState>();
	if (!MatchState || !MatchState->IsVoteOpen())
	{
		return;
	}
	GetWorldTimerManager().ClearTimer(VoteTimerHandle);
	MatchState->ServerSetVote(false, VotePhase, 0.0);

	TMap<AjinzzaPartyPlayerState*, int32> Tally;
	for (const TPair<TWeakObjectPtr<APlayerController>, TWeakObjectPtr<AjinzzaPartyPlayerState>>& Vote : Votes)
	{
		AjinzzaPartyPlayerState* Target = Vote.Value.Get();
		if (Target && Target->IsLivingCandidate())
		{
			++Tally.FindOrAdd(Target);
		}
	}
	Votes.Reset();

	TArray<AjinzzaPartyPlayerState*> Top;
	int32 TopCount = 0;
	for (const TPair<AjinzzaPartyPlayerState*, int32>& Entry : Tally)
	{
		if (Entry.Value > TopCount)
		{
			TopCount = Entry.Value;
			Top.Reset();
		}
		if (Entry.Value == TopCount)
		{
			Top.Add(Entry.Key);
		}
	}

	if (Top.Num() == 0)
	{
		// No vote cast in time: random, same fallback the design doc uses for an undesignated interview.
		Top = GetLivingCandidates();
		UE_LOG(Logjinzza, Log, TEXT("Vote: no votes cast - picking at random."));
	}
	if (Top.Num() == 0)
	{
		return;
	}

	StartFinalArgument(Top[FMath::RandRange(0, Top.Num() - 1)]);
}

void AjinzzaGameGameMode::StartFinalArgument(AjinzzaPartyPlayerState* Condemned)
{
	CondemnedPlayer = Condemned;
	BeginSpeakTurn(EJinzzaSpeakTurnKind::FinalArgument, Condemned, FinalArgumentSeconds, 1, 1, TEXT("Zone.Evaluation"));
	GetWorldTimerManager().SetTimer(SpeakTurnTimerHandle, this, &AjinzzaGameGameMode::FinishFinalArgument, FinalArgumentSeconds, false);
}

void AjinzzaGameGameMode::FinishFinalArgument()
{
	EndSpeakTurn();

	if (AjinzzaPartyPlayerState* Condemned = CondemnedPlayer.Get())
	{
		UE_LOG(Logjinzza, Log, TEXT("Vote: %s eliminated."), *Condemned->GetDisplayName());
		EliminateToGhost(Condemned);
	}
	CondemnedPlayer.Reset();

	const EJinzzaRoundPhase Phase = VotePhase;
	VotePhase = EJinzzaRoundPhase::None;
	if (UjinzzaRoundPhaseSubsystem* RoundPhase = GetGameInstance() ? GetGameInstance()->GetSubsystem<UjinzzaRoundPhaseSubsystem>() : nullptr)
	{
		RoundPhase->NotifyPhaseConditionMet(Phase);
	}
}

void AjinzzaGameGameMode::CancelTurnsAndVote()
{
	GetWorldTimerManager().ClearTimer(VoteTimerHandle);
	EndSpeakTurn();
	SelfIntroOrder.Reset();
	Votes.Reset();
	CondemnedPlayer.Reset();
	VotePhase = EJinzzaRoundPhase::None;
	if (AjinzzaGameGameState* MatchState = GetGameState<AjinzzaGameGameState>(); MatchState && MatchState->IsVoteOpen())
	{
		MatchState->ServerSetVote(false, EJinzzaRoundPhase::None, 0.0);
	}
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
		else if (!Candidate && PartyPS->ServerRole != EJinzzaPartyRole::None && !PartyPS->IsGhost())
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
