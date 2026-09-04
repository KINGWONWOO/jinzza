// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaLobbyWidget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "jinzzaLobbyGameState.h"
#include "jinzzaGameInstance.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"

void UjinzzaLobbyWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (InviteButton)
	{
		InviteButton->OnClicked.AddDynamic(this, &UjinzzaLobbyWidget::OnInviteFriendsClicked);
	}

	if (StartButton)
	{
		StartButton->OnClicked.AddDynamic(this, &UjinzzaLobbyWidget::OnStartMatchClicked);

		const APlayerController* PC = GetOwningPlayer();
		StartButton->SetVisibility(PC && PC->HasAuthority() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	if (InteractPromptText)
	{
		InteractPromptText->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (USoundBase* Bgm = LoadObject<USoundBase>(nullptr, TEXT("/Game/JINZZA/Audio/Sounds/Lobby/LobbyBgm__cut_83sec__Cue.LobbyBgm__cut_83sec__Cue")))
	{
		MusicComponent = UGameplayStatics::SpawnSound2D(this, Bgm, 1.f, 1.f, 0.f, nullptr, true, false);
	}
}

void UjinzzaLobbyWidget::NativeDestruct()
{
	if (MusicComponent)
	{
		MusicComponent->Stop();
		MusicComponent = nullptr;
	}

	Super::NativeDestruct();
}

void UjinzzaLobbyWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (PlayerCountText)
	{
		int32 Count = 0;
		if (AGameStateBase* GameState = UGameplayStatics::GetGameState(this))
		{
			Count = GameState->PlayerArray.Num();
		}
		PlayerCountText->SetText(FText::FromString(FString::Printf(TEXT("Players connected: %d"), Count)));
	}

	if (SettingsText)
	{
		if (const AjinzzaLobbyGameState* LobbyGameState = Cast<AjinzzaLobbyGameState>(UGameplayStatics::GetGameState(this)))
		{
			const FJinzzaMatchSettings& Settings = LobbyGameState->MatchSettings;
			SettingsText->SetText(FText::FromString(FString::Printf(
				TEXT("%s\nMax Players: %d | Judges: %d | Votes: %d\nPhase Speed: %s | Roles: %s"),
				*Settings.RoomName, Settings.MaxPlayers, Settings.JudgeCount, Settings.VoteCount,
				*Settings.PhaseSpeed, *Settings.RoleAssignMethod)));
		}
	}
}

void UjinzzaLobbyWidget::SetInteractionPrompt(const FText& PromptText)
{
	if (!InteractPromptText)
	{
		return;
	}

	if (PromptText.IsEmpty())
	{
		InteractPromptText->SetVisibility(ESlateVisibility::Collapsed);
	}
	else
	{
		InteractPromptText->SetText(PromptText);
		InteractPromptText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void UjinzzaLobbyWidget::OnStartMatchClicked()
{
	APlayerController* PC = GetOwningPlayer();
	if (PC && PC->HasAuthority())
	{
		if (UWorld* World = GetWorld())
		{
			World->ServerTravel(TEXT("/Game/JINZZA/Level/Lvl_Game"));
		}
	}
}

void UjinzzaLobbyWidget::OnInviteFriendsClicked()
{
	if (UjinzzaGameInstance* GI = Cast<UjinzzaGameInstance>(UGameplayStatics::GetGameInstance(this)))
	{
		GI->InviteFriends();
	}
}
