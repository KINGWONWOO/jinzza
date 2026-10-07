// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaLobbyGameMode.h"
#include "jinzzaLobbyPlayerController.h"
#include "jinzzaLobbyGameState.h"
#include "jinzzaGameInstance.h"
#include "jinzzaPartyPlayerState.h"
#include "GameFramework/DefaultPawn.h"
#include "UObject/ConstructorHelpers.h"
#include "jinzzaLoadingSettings.h"
#include "jinzza.h"
#include "Engine/World.h"
#include "TimerManager.h"

const TCHAR* AjinzzaLobbyGameMode::MatchMapPath = TEXT("/Game/JINZZA/Level/Lvl_Game");

AjinzzaLobbyGameMode::AjinzzaLobbyGameMode()
{
	PlayerControllerClass = AjinzzaLobbyPlayerController::StaticClass();
	GameStateClass = AjinzzaLobbyGameState::StaticClass();

	// Lvl_Lobby used to spawn a bare flying ADefaultPawn - invisible, so players in the pre-match
	// lobby couldn't see each other's characters standing around (same bug fixed for Lvl_Game in
	// AjinzzaGameGameMode; see its constructor comment). Reuses the same first-person character BP
	// so the pawn players get in the lobby is the same one they'll carry into the match.
	static ConstructorHelpers::FClassFinder<APawn> CharacterBPClass(TEXT("/Game/JINZZA/FirstPerson/Blueprints/BP_FirstPersonCharacter"));
	if (CharacterBPClass.Succeeded())
	{
		DefaultPawnClass = CharacterBPClass.Class;
	}
	else
	{
		DefaultPawnClass = ADefaultPawn::StaticClass();
	}
	// Same PlayerState class Lvl_Game uses (AjinzzaGameGameMode), set here too so seamless travel
	// to Lvl_Game never has to worry about whether the carried-over PlayerState is still the base
	// APlayerState - AjinzzaGameGameMode::AssignRoles() casts every PlayerState to this type.
	PlayerStateClass = AjinzzaPartyPlayerState::StaticClass();
	bUseSeamlessTravel = true;
}

void AjinzzaLobbyGameMode::InitGameState()
{
	Super::InitGameState();

	if (AjinzzaLobbyGameState* LobbyGameState = GetGameState<AjinzzaLobbyGameState>())
	{
		if (const UjinzzaGameInstance* GI = GetGameInstance<UjinzzaGameInstance>())
		{
			LobbyGameState->MatchSettings = GI->GetPendingMatchSettings();
		}
	}
}

void AjinzzaLobbyGameMode::BeginMatchPreparation()
{
	if (bPreparingMatch)
	{
		return;
	}
	bPreparingMatch = true;
	PreloadedPlayers.Reset();

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (AjinzzaLobbyPlayerController* PC = Cast<AjinzzaLobbyPlayerController>(It->Get()))
		{
			PC->Client_PrepareForMatch(MatchMapPath);
		}
	}

	GetWorldTimerManager().SetTimer(MatchPreloadTimeoutHandle, this, &AjinzzaLobbyGameMode::TravelToMatch,
		UjinzzaLoadingSettings::Get()->MatchPreloadTimeoutSeconds, false);
	CheckMatchPreparation();
}

void AjinzzaLobbyGameMode::NotifyMatchPreloaded(APlayerController* Player)
{
	if (bPreparingMatch && Player)
	{
		PreloadedPlayers.Add(Player);
		CheckMatchPreparation();
	}
}

void AjinzzaLobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	// Joined while the others are already preloading - preload too, and count them in.
	if (bPreparingMatch && !bTravellingToMatch)
	{
		if (AjinzzaLobbyPlayerController* PC = Cast<AjinzzaLobbyPlayerController>(NewPlayer))
		{
			PC->Client_PrepareForMatch(MatchMapPath);
		}
		CheckMatchPreparation();
	}
}

void AjinzzaLobbyGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);

	if (bPreparingMatch && !bTravellingToMatch && GetWorld() && !GetWorld()->bIsTearingDown)
	{
		PreloadedPlayers.Remove(Cast<APlayerController>(Exiting));
		// Still in the controller list during Logout - re-count next tick.
		GetWorldTimerManager().SetTimerForNextTick(this, &AjinzzaLobbyGameMode::CheckMatchPreparation);
	}
}

void AjinzzaLobbyGameMode::CheckMatchPreparation()
{
	if (!bPreparingMatch || bTravellingToMatch)
	{
		return;
	}

	int32 Total = 0;
	int32 Ready = 0;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (APlayerController* PC = It->Get())
		{
			++Total;
			Ready += PreloadedPlayers.Contains(PC) ? 1 : 0;
		}
	}

	if (AjinzzaLobbyGameState* LobbyGameState = GetGameState<AjinzzaLobbyGameState>())
	{
		LobbyGameState->MatchPrepReadyCount = Ready;
		LobbyGameState->MatchPrepTotalCount = Total;
	}

	if (Total > 0 && Ready >= Total)
	{
		TravelToMatch();
	}
}

void AjinzzaLobbyGameMode::TravelToMatch()
{
	if (bTravellingToMatch)
	{
		return;
	}
	bTravellingToMatch = true;
	GetWorldTimerManager().ClearTimer(MatchPreloadTimeoutHandle);

	// The match waits for this many players before it starts (AjinzzaGameGameMode).
	if (UjinzzaGameInstance* GI = GetGameInstance<UjinzzaGameInstance>())
	{
		GI->SetExpectedMatchPlayers(GetWorld()->GetNumPlayerControllers());
	}

	UE_LOG(Logjinzza, Log, TEXT("All players preloaded the match (or timed out) - travelling."));
	GetWorld()->ServerTravel(MatchMapPath);
}
