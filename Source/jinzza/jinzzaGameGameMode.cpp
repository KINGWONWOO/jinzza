// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaGameGameMode.h"
#include "jinzzaGamePlayerController.h"
#include "jinzzaGameGameState.h"
#include "jinzzaPartyPlayerState.h"
#include "jinzzaRoundPhaseSubsystem.h"
#include "jinzza.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "UObject/ConstructorHelpers.h"
#include "TimerManager.h"
#include "GameFramework/PlayerController.h"

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

void AjinzzaGameGameMode::StartPlay()
{
	Super::StartPlay();

	UGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	if (UjinzzaRoundPhaseSubsystem* RoundPhase = GI ? GI->GetSubsystem<UjinzzaRoundPhaseSubsystem>() : nullptr)
	{
		RoundPhase->OnServerPhaseEntered.AddUObject(this, &AjinzzaGameGameMode::OnRoundPhaseEntered);
	}
	// StartRound() is NOT called here - see PostLogin()/TryStartRound(). Seamless travel from
	// Lvl_Lobby delivers players one PostLogin at a time as each reconnects to this level, not all
	// at once; calling StartRound() straight from StartPlay() raced against that and assigned roles
	// to whichever single player (usually just the host, who reconnects fastest) had arrived first,
	// silently starving every later-arriving player of a role. Confirmed via a 3-client PIE test
	// (listen server, 3 players) on 2026-09-01: AssignRoles logged "got 1" even though all 3
	// PlayerStates existed a few seconds later.
}

void AjinzzaGameGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (bRoundStarted)
	{
		return;
	}

	// Debounce instead of waiting for an exact expected head count: restart the timer on every
	// login, so the round starts once no new player has joined for a beat. Avoids needing the
	// lobby to hand off how many players it expected (which could be wrong anyway if someone
	// disconnects mid-travel and never reconnects) at the cost of a small, one-time delay.
	GetWorldTimerManager().SetTimer(RoundStartGraceTimerHandle, this, &AjinzzaGameGameMode::TryStartRound, 1.5f, false);
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

	// TEMP DEBUG (remove once the PostLogin/TryStartRound fix is trusted - see todo.txt round 4):
	// success-path log, since the only existing AssignRoles log line was on the skip path and gave
	// no positive signal that a real assignment (as opposed to none happening at all) occurred.
	UE_LOG(Logjinzza, Warning, TEXT("[TEMP DEBUG] AssignRoles succeeded with %d players - RealOne=%s Judge=%s Imitators=%d"),
		Players.Num(), *RealOne->GetPlayerName(), *Judge->GetPlayerName(), Players.Num() - 2);

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
