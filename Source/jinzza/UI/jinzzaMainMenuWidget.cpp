// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaMainMenuWidget.h"
#include "jinzzaSettingsWidget.h"
#include "jinzzaCustomizationWidget.h"
#include "jinzzaVoiceTestWidget.h"
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
		Page_VoiceTest = 3,
	};

	constexpr float FadeInDuration = 0.35f;

	// TEMP placeholder panel-open sound (menu entry, opening Settings/Customization) - swap for
	// real SFX later. Shared here since three call sites in this file all want the same one-shot.
	void PlayPanelOpenSound(const UObject* WorldContext)
	{
		if (USoundBase* Sound = LoadObject<USoundBase>(nullptr, TEXT("/Game/JINZZA/Audio/Sounds/UISounds/LobbyPannelOpen__cut_1sec_.LobbyPannelOpen__cut_1sec_")))
		{
			UGameplayStatics::PlaySound2D(WorldContext, Sound);
		}
	}
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

	// RootOverlay holds the page Switcher plus the logo badge, so the badge stays on screen no
	// matter which switcher page (Buttons/Settings/Customization/VoiceTest) is active.
	UOverlay* RootOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("RootOverlay"));
	Background->SetContent(RootOverlay);

	Switcher = WidgetTree->ConstructWidget<UWidgetSwitcher>(UWidgetSwitcher::StaticClass(), TEXT("Switcher"));
	if (UOverlaySlot* SwitcherSlot = RootOverlay->AddChildToOverlay(Switcher))
	{
		SwitcherSlot->SetHorizontalAlignment(HAlign_Fill);
		SwitcherSlot->SetVerticalAlignment(VAlign_Fill);
	}

	// Top-left company logo badge: a gold-tinted mask icon (JinzzaUI::MakeMaskIcon, built from
	// primitives, no source art) instead of a plain "LOGO" text placeholder - the mask directly
	// evokes the "Imitator" disguise premise the whole game is built around, giving the badge real
	// identity until a real logo asset exists.
	USizeBox* LogoBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("LogoBox"));
	LogoBox->SetWidthOverride(84.f);
	LogoBox->SetHeightOverride(84.f);
	if (UOverlaySlot* LogoSlot = RootOverlay->AddChildToOverlay(LogoBox))
	{
		LogoSlot->SetHorizontalAlignment(HAlign_Left);
		LogoSlot->SetVerticalAlignment(VAlign_Top);
		LogoSlot->SetPadding(FMargin(24.f));
	}

	UBorder* LogoBadge = JinzzaUI::MakePanelBackground(WidgetTree, TEXT("LogoBadge"));
	LogoBadge->SetHorizontalAlignment(HAlign_Center);
	LogoBadge->SetVerticalAlignment(VAlign_Center);
	LogoBox->AddChild(LogoBadge);
	LogoBadge->SetContent(JinzzaUI::MakeMaskIcon(WidgetTree, TEXT("LogoMaskIcon"), 48.f, JinzzaUI::Color_Accent));

	// Page 0: the button-list page. ButtonsPageRoot is the whole page (fade target).
	UOverlay* ButtonsPage = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("ButtonsPageRoot"));
	ButtonsPageRoot = ButtonsPage;

	// Branding column (title/divider/status) sits top-right, per user request - distinct from the
	// small top-left LogoBox above (that's the company logo corner badge, not the game title).
	USizeBox* BrandingBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("BrandingBox"));
	BrandingBox->SetWidthOverride(360.f);
	if (UOverlaySlot* BrandingSlot = ButtonsPage->AddChildToOverlay(BrandingBox))
	{
		BrandingSlot->SetHorizontalAlignment(HAlign_Right);
		BrandingSlot->SetVerticalAlignment(VAlign_Top);
		BrandingSlot->SetPadding(FMargin(0.f, 32.f, 48.f, 0.f));
	}

	UVerticalBox* BrandingStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("BrandingStack"));
	BrandingBox->AddChild(BrandingStack);

	UTextBlock* TitleText = JinzzaUI::MakeTitleText(WidgetTree, TEXT("TitleText"), FText::FromString(TEXT("JINZZA")), 44);
	TitleText->SetJustification(ETextJustify::Right);
	JinzzaUI::AddSpaced(BrandingStack, TitleText, 0.f);

	// Korean tagline directly under the title - "Find the Real One", the game's own concept in one
	// line, so the logo lockup reads as more than just a wordmark. Uses the same SacheonUju font
	// every other Korean string in this project already relies on (kiosk signs, etc.), so there's
	// no tofu-glyph risk.
	UTextBlock* TaglineText = JinzzaUI::MakeBodyText(WidgetTree, TEXT("TaglineText"), FText::FromString(TEXT("진짜를 찾아라")), true);
	TaglineText->SetJustification(ETextJustify::Right);
	JinzzaUI::AddSpaced(BrandingStack, TaglineText, 2.f);

	if (UVerticalBoxSlot* DividerSlot = JinzzaUI::AddSpaced(BrandingStack, JinzzaUI::MakeDivider(WidgetTree, TEXT("TitleDivider")), 8.f))
	{
		DividerSlot->SetHorizontalAlignment(HAlign_Right);
	}

	StatusText = JinzzaUI::MakeBodyText(WidgetTree, TEXT("StatusText"), FText::GetEmpty(), true);
	StatusText->SetJustification(ETextJustify::Right);
	JinzzaUI::AddSpaced(BrandingStack, StatusText, 10.f);

	// Left button column - Host/Settings/Quit - and right button column - Customize/Voice Test -
	// both anchored to the bottom of the screen. Reworked this round to match JINZZA's own
	// established noir courtroom/interrogation theme (see JinzzaUI's header comment) rather than
	// NOOB-GAME's cute pastel look used in earlier rounds: Host/Settings/Quit are back to
	// JinzzaUI::MakePrimaryButton/MakeSecondaryButton/MakeWarningButton - the exact same noir pill
	// buttons every other jinzza screen (Settings/Customization/kiosks) already uses, instead of
	// the imported NOOB pill art. (T_ButtonHost/T_ButtonSettings/T_ButtonQuit/T_ButtonCustomize are
	// now unused content assets, left in place rather than deleted.)
	USizeBox* ButtonsLeftBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ButtonsLeftBox"));
	if (UOverlaySlot* ButtonsLeftSlot = ButtonsPage->AddChildToOverlay(ButtonsLeftBox))
	{
		ButtonsLeftSlot->SetHorizontalAlignment(HAlign_Left);
		ButtonsLeftSlot->SetVerticalAlignment(VAlign_Bottom);
		ButtonsLeftSlot->SetPadding(FMargin(64.f, 0.f, 0.f, 64.f));
	}

	// Dark noir panel (JinzzaUI::MakePanelBackground - the same "room wall" surface Settings/
	// Customization use) behind the button column, replacing last round's light NOOB-toned
	// parchment note-card now that the buttons themselves are noir again - a light card behind
	// dark noir buttons would clash the same way the pastel buttons clashed with the dark page
	// background before that.
	UBorder* ButtonsLeftPanel = JinzzaUI::MakePanelBackground(WidgetTree, TEXT("ButtonsLeftPanel"));
	ButtonsLeftPanel->SetPadding(FMargin(28.f, 24.f, 28.f, 24.f));
	ButtonsLeftBox->AddChild(ButtonsLeftPanel);

	UVerticalBox* ButtonsLeftStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ButtonsLeftStack"));
	ButtonsLeftPanel->SetContent(ButtonsLeftStack);

	HostButton = JinzzaUI::MakePrimaryButton(WidgetTree, TEXT("HostButton"), FText::FromString(TEXT("Host Game")));
	JinzzaUI::AddSpaced(ButtonsLeftStack, HostButton, 0.f);

	SettingsButton = JinzzaUI::MakeSecondaryButton(WidgetTree, TEXT("SettingsButton"), FText::FromString(TEXT("Settings")));
	JinzzaUI::AddSpaced(ButtonsLeftStack, SettingsButton);

	QuitButton = JinzzaUI::MakeWarningButton(WidgetTree, TEXT("QuitButton"), FText::FromString(TEXT("Quit")));
	JinzzaUI::AddSpaced(ButtonsLeftStack, QuitButton);

	USizeBox* ButtonsRightBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ButtonsRightBox"));
	if (UOverlaySlot* ButtonsRightSlot = ButtonsPage->AddChildToOverlay(ButtonsRightBox))
	{
		ButtonsRightSlot->SetHorizontalAlignment(HAlign_Right);
		ButtonsRightSlot->SetVerticalAlignment(VAlign_Bottom);
		ButtonsRightSlot->SetPadding(FMargin(0.f, 0.f, 64.f, 64.f));
	}

	// Same noir panel as ButtonsLeftPanel above.
	UBorder* ButtonsRightPanel = JinzzaUI::MakePanelBackground(WidgetTree, TEXT("ButtonsRightPanel"));
	ButtonsRightPanel->SetPadding(FMargin(28.f, 24.f, 28.f, 24.f));
	ButtonsRightBox->AddChild(ButtonsRightPanel);

	UVerticalBox* ButtonsRightStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ButtonsRightStack"));
	ButtonsRightPanel->SetContent(ButtonsRightStack);

	// Customize and Voice Test are circular icon buttons (JinzzaUI::MakeCircleIconButton) with
	// purpose-built icons instead of NOOB-GAME's cat/headphones art: a masquerade mask
	// (JinzzaUI::MakeMaskIcon, crimson - the "guilty"/Imitator accent) directly evokes the
	// Imitator disguise premise for Customize, and a microphone (JinzzaUI::MakeMicIcon, gold - the
	// "judge" accent) for Voice Test - both built from plain rounded-box primitives, no source art
	// needed, so there's no dependency on finding an on-theme asset that doesn't exist.
	UWidget* MaskIcon = JinzzaUI::MakeMaskIcon(WidgetTree, TEXT("CustomizationMaskIcon"), 40.f, JinzzaUI::Color_TextPrimary);
	CustomizationButton = JinzzaUI::MakeCircleIconButton(WidgetTree, TEXT("CustomizationButton"), FText::FromString(TEXT("Customize")), MaskIcon, JinzzaUI::Color_AccentAlt);
	if (UVerticalBoxSlot* ButtonSlot = JinzzaUI::AddSpaced(ButtonsRightStack, CustomizationButton, 0.f)) { ButtonSlot->SetHorizontalAlignment(HAlign_Center); }

	// Voice Test button - opens VoiceTestWidget as a central switcher page (Page_VoiceTest),
	// same pattern as Settings/Customization, rather than the old always-visible corner panel.
	UWidget* MicIcon = JinzzaUI::MakeMicIcon(WidgetTree, TEXT("VoiceTestMicIcon"), 40.f, JinzzaUI::Color_Background);
	VoiceTestButton = JinzzaUI::MakeCircleIconButton(WidgetTree, TEXT("VoiceTestButton"), FText::FromString(TEXT("Voice Test")), MicIcon, JinzzaUI::Color_Accent);
	if (UVerticalBoxSlot* ButtonSlot = JinzzaUI::AddSpaced(ButtonsRightStack, VoiceTestButton, 20.f)) { ButtonSlot->SetHorizontalAlignment(HAlign_Center); }

	Switcher->AddChild(ButtonsPage);

	// Page 1: Settings, built directly as a nested UjinzzaSettingsWidget (which builds its own
	// tree the same way) rather than loading a WBP_Settings Blueprint class.
	SettingsWidget = WidgetTree->ConstructWidget<UjinzzaSettingsWidget>(UjinzzaSettingsWidget::StaticClass(), TEXT("SettingsWidget"));
	Switcher->AddChild(SettingsWidget);

	// Page 2: Customization, same nested-widget pattern as Settings above.
	CustomizationWidget = WidgetTree->ConstructWidget<UjinzzaCustomizationWidget>(UjinzzaCustomizationWidget::StaticClass(), TEXT("CustomizationWidget"));
	Switcher->AddChild(CustomizationWidget);

	// Page 3: VoiceTest, same nested-widget pattern as Settings/Customization above.
	VoiceTestWidget = WidgetTree->ConstructWidget<UjinzzaVoiceTestWidget>(UjinzzaVoiceTestWidget::StaticClass(), TEXT("VoiceTestWidget"));
	Switcher->AddChild(VoiceTestWidget);
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

	if (VoiceTestButton)
	{
		VoiceTestButton->OnClicked.AddDynamic(this, &UjinzzaMainMenuWidget::OnVoiceTestClicked);
	}

	if (SettingsWidget)
	{
		SettingsWidget->OnBackRequested.AddUObject(this, &UjinzzaMainMenuWidget::ShowButtonsPage);
	}

	if (CustomizationWidget)
	{
		CustomizationWidget->OnBackRequested.AddUObject(this, &UjinzzaMainMenuWidget::ShowButtonsPage);
	}

	if (VoiceTestWidget)
	{
		VoiceTestWidget->OnBackRequested.AddUObject(this, &UjinzzaMainMenuWidget::ShowButtonsPage);
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

	PlayPanelOpenSound(this);
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
		PlayPanelOpenSound(this);
	}
}

void UjinzzaMainMenuWidget::OnCustomizationClicked()
{
	if (Switcher)
	{
		Switcher->SetActiveWidgetIndex(Page_Customization);
		PlayPanelOpenSound(this);
	}
}

void UjinzzaMainMenuWidget::OnVoiceTestClicked()
{
	if (Switcher)
	{
		Switcher->SetActiveWidgetIndex(Page_VoiceTest);
		PlayPanelOpenSound(this);
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
