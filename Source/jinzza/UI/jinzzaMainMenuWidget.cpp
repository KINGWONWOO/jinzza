// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaMainMenuWidget.h"
#include "jinzzaSettingsWidget.h"
#include "jinzzaCustomizationWidget.h"
#include "jinzzaCharacterPreviewCapture.h"
#include "jinzzaUIStyle.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Widget.h"
#include "Components/Image.h"
#include "Components/WidgetSwitcher.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Blueprint/WidgetTree.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/PlayerController.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Styling/SlateBrush.h"
#include "Brushes/SlateColorBrush.h"
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

void UjinzzaMainMenuWidget::BuildWidgetTree()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UBorder* Background = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Background"));
	Background->SetBrush(FSlateColorBrush(JinzzaUI::Color_Background));
	Background->SetHorizontalAlignment(HAlign_Fill);
	Background->SetVerticalAlignment(VAlign_Fill);
	WidgetTree->RootWidget = Background;

	Switcher = WidgetTree->ConstructWidget<UWidgetSwitcher>(UWidgetSwitcher::StaticClass(), TEXT("Switcher"));
	Background->SetContent(Switcher);

	// Page 0: the button-list page. ButtonsPageRoot is the whole page (fade target); its content
	// is centered in a fixed-width column via a Size Box.
	UOverlay* ButtonsPage = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("ButtonsPageRoot"));
	ButtonsPageRoot = ButtonsPage;

	USizeBox* CenterBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ButtonsCenterBox"));
	CenterBox->SetWidthOverride(420.f);
	if (UOverlaySlot* CenterSlot = ButtonsPage->AddChildToOverlay(CenterBox))
	{
		CenterSlot->SetHorizontalAlignment(HAlign_Center);
		CenterSlot->SetVerticalAlignment(VAlign_Center);
	}

	UVerticalBox* ButtonsStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ButtonsStack"));
	CenterBox->AddChild(ButtonsStack);

	auto AddSpaced = [ButtonsStack](UWidget* Child, float TopPadding = 12.f)
	{
		if (UVerticalBoxSlot* Slot = ButtonsStack->AddChildToVerticalBox(Child))
		{
			Slot->SetHorizontalAlignment(HAlign_Fill);
			Slot->SetPadding(FMargin(0.f, TopPadding, 0.f, 0.f));
		}
	};

	AddSpaced(JinzzaUI::MakeTitleText(WidgetTree, TEXT("TitleText"), FText::FromString(TEXT("JINZZA")), 48), 0.f);
	AddSpaced(JinzzaUI::MakeDivider(WidgetTree, TEXT("TitleDivider")));

	StatusText = JinzzaUI::MakeBodyText(WidgetTree, TEXT("StatusText"), FText::GetEmpty(), true);
	AddSpaced(StatusText, 10.f);

	HostButton = JinzzaUI::MakePrimaryButton(WidgetTree, TEXT("HostButton"), FText::FromString(TEXT("Host Game")));
	AddSpaced(HostButton, 24.f);

	SettingsButton = JinzzaUI::MakeSecondaryButton(WidgetTree, TEXT("SettingsButton"), FText::FromString(TEXT("Settings")));
	AddSpaced(SettingsButton);

	QuitButton = JinzzaUI::MakeWarningButton(WidgetTree, TEXT("QuitButton"), FText::FromString(TEXT("Quit")));
	AddSpaced(QuitButton);

	Switcher->AddChild(ButtonsPage);

	// Page 1: Settings, built directly as a nested UjinzzaSettingsWidget (which builds its own
	// tree the same way) rather than loading a WBP_Settings Blueprint class.
	SettingsWidget = WidgetTree->ConstructWidget<UjinzzaSettingsWidget>(UjinzzaSettingsWidget::StaticClass(), TEXT("SettingsWidget"));
	Switcher->AddChild(SettingsWidget);
}

void UjinzzaMainMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	BuildWidgetTree();

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
