// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaGameEndWidget.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "jinzzaGameInstance.h"

void UjinzzaGameEndWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (EndGameButton)
	{
		EndGameButton->OnClicked.AddDynamic(this, &UjinzzaGameEndWidget::OnEndGameClicked);

		const APlayerController* PC = GetOwningPlayer();
		EndGameButton->SetVisibility(PC && PC->HasAuthority() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void UjinzzaGameEndWidget::OnEndGameClicked()
{
	if (UjinzzaGameInstance* GI = Cast<UjinzzaGameInstance>(UGameplayStatics::GetGameInstance(this)))
	{
		GI->EndGameReturnToLobby();
	}
}
