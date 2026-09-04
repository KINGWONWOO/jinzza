// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaMainMenuWidget.h"
#include "jinzzaSettingsWidget.h"
#include "jinzzaCustomizationWidget.h"
#include "jinzzaCharacterPreviewCapture.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Widget.h"
#include "Components/Image.h"
#include "Components/WidgetSwitcher.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/PlayerController.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Styling/SlateBrush.h"
#include "EngineUtils.h"

namespace
{
	enum EMenuPage : int32
	{
		Page_Buttons = 0,
		Page_Settings = 1,
		Page_Customization = 2,
	};

	constexpr float FadeInDuration = 0.35f;
}

void UjinzzaMainMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (HostButton)
	{
		HostButton->OnClicked.AddDynamic(this, &UjinzzaMainMenuWidget::OnHostClicked);
	}

	if (SettingsButton)
	{
		SettingsButton->OnClicked.AddDynamic(this, &UjinzzaMainMenuWidget::OnSettingsClicked);
	}

	if (QuitButton)
	{
		QuitButton->OnClicked.AddDynamic(this, &UjinzzaMainMenuWidget::OnQuitClicked);
	}

	if (CustomizationButton)
	{
		CustomizationButton->OnClicked.AddDynamic(this, &UjinzzaMainMenuWidget::OnCustomizationClicked);
	}

	if (SettingsWidget)
	{
		SettingsWidget->OnBackRequested.AddUObject(this, &UjinzzaMainMenuWidget::ShowButtonsPage);
	}

	if (CustomizationWidget)
	{
		CustomizationWidget->OnBackRequested.AddUObject(this, &UjinzzaMainMenuWidget::ShowButtonsPage);
	}

	if (Switcher)
	{
		Switcher->SetActiveWidgetIndex(Page_Buttons);
	}

	TryWireCharacterPreview();

	if (UjinzzaGameInstance* GI = GetJinzzaGameInstance())
	{
		SessionStatusHandle = GI->OnSessionStatusChanged.AddUObject(this, &UjinzzaMainMenuWidget::HandleSessionStatusChanged);
	}

	if (ButtonsPageRoot)
	{
		ButtonsPageRoot->SetRenderOpacity(0.f);
	}

	if (USoundBase* Bgm = LoadObject<USoundBase>(nullptr, TEXT("/Game/JINZZA/Audio/Sounds/MainMenu/MainMenuBgm_Cue.MainMenuBgm_Cue")))
	{
		MusicComponent = UGameplayStatics::SpawnSound2D(this, Bgm, 1.f, 1.f, 0.f, nullptr, true, false);
	}
}

void UjinzzaMainMenuWidget::NativeDestruct()
{
	if (MusicComponent)
	{
		MusicComponent->Stop();
		MusicComponent = nullptr;
	}

	if (UjinzzaGameInstance* GI = GetJinzzaGameInstance())
	{
		GI->OnSessionStatusChanged.Remove(SessionStatusHandle);
	}

	Super::NativeDestruct();
}

void UjinzzaMainMenuWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (ButtonsPageRoot && FadeInElapsed < FadeInDuration && Switcher && Switcher->GetActiveWidgetIndex() == Page_Buttons)
	{
		FadeInElapsed = FMath::Min(FadeInElapsed + InDeltaTime, FadeInDuration);
		ButtonsPageRoot->SetRenderOpacity(FMath::Clamp(FadeInElapsed / FadeInDuration, 0.f, 1.f));
	}

	if (!bCharacterPreviewWired)
	{
		TryWireCharacterPreview();
	}
}

void UjinzzaMainMenuWidget::TryWireCharacterPreview()
{
	if (!CharacterPreviewImage)
	{
		bCharacterPreviewWired = true; // nothing to wire until the Designer adds this widget
		return;
	}

	// AjinzzaCharacterPreviewCapture creates its render target in BeginPlay, whose ordering
	// relative to this widget's own creation (from AjinzzaMenuPlayerController::BeginPlay) isn't
	// guaranteed - retry each tick (cheap: one actor-iterator scan) until it's ready.
	for (TActorIterator<AjinzzaCharacterPreviewCapture> It(GetWorld()); It; ++It)
	{
		if (UTextureRenderTarget2D* RT = It->GetRenderTarget())
		{
			// SetBrushFromTexture only accepts UTexture2D specifically - UTextureRenderTarget2D
			// is a sibling (both derive from UTexture), so the brush needs setting up directly.
			FSlateBrush Brush;
			Brush.SetResourceObject(RT);
			Brush.ImageSize = FVector2D(RT->SizeX, RT->SizeY);
			CharacterPreviewImage->SetBrush(Brush);
			bCharacterPreviewWired = true;
		}
		break;
	}
}

UjinzzaGameInstance* UjinzzaMainMenuWidget::GetJinzzaGameInstance() const
{
	return Cast<UjinzzaGameInstance>(UGameplayStatics::GetGameInstance(this));
}

void UjinzzaMainMenuWidget::ShowButtonsPage()
{
	if (Switcher)
	{
		Switcher->SetActiveWidgetIndex(Page_Buttons);
	}
}

void UjinzzaMainMenuWidget::OnHostClicked()
{
	if (UjinzzaGameInstance* GI = GetJinzzaGameInstance())
	{
		GI->HostSession(FJinzzaMatchSettings());
	}
}

void UjinzzaMainMenuWidget::OnSettingsClicked()
{
	if (Switcher)
	{
		Switcher->SetActiveWidgetIndex(Page_Settings);
	}
}

void UjinzzaMainMenuWidget::OnCustomizationClicked()
{
	if (Switcher)
	{
		Switcher->SetActiveWidgetIndex(Page_Customization);
	}
}

void UjinzzaMainMenuWidget::OnQuitClicked()
{
	if (APlayerController* PC = GetOwningPlayer())
	{
		UKismetSystemLibrary::QuitGame(this, PC, EQuitPreference::Quit, false);
	}
}

void UjinzzaMainMenuWidget::HandleSessionStatusChanged(EJinzzaSessionStatus Status, const FString& Message)
{
	if (StatusText)
	{
		StatusText->SetText(FText::FromString(Message));
	}
}
