// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaSettingsWidget.h"
#include "jinzzaUIStyle.h"
#include "jinzzaGameUserSettings.h"
#include "Blueprint/WidgetTree.h"
#include "Components/PanelWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/ScrollBox.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/SpinBox.h"
#include "Components/Slider.h"
#include "Components/CheckBox.h"
#include "Components/WidgetSwitcher.h"
#include "Kismet/KismetSystemLibrary.h"
#include "InputCoreTypes.h"
#include "AudioCaptureBlueprintLibrary.h"

namespace
{
	enum ETabPage : int32
	{
		Tab_Graphics = 0,
		Tab_Audio = 1,
		Tab_Controls = 2,
		Tab_Gameplay = 3,
	};

	FString ResolutionToString(const FIntPoint& Res)
	{
		return FString::Printf(TEXT("%d x %d"), Res.X, Res.Y);
	}

	void AddHeading(UWidgetTree* Tree, UPanelWidget* Parent, const FText& Text)
	{
		Parent->AddChild(JinzzaUI::MakeSectionHeading(Tree, NAME_None, Text));
	}

	void AddLabel(UWidgetTree* Tree, UPanelWidget* Parent, const FText& Text)
	{
		UTextBlock* Label = JinzzaUI::MakeBodyText(Tree, NAME_None, Text, true);
		if (UVerticalBoxSlot* Slot = Cast<UVerticalBoxSlot>(Parent->AddChild(Label)))
		{
			Slot->SetPadding(FMargin(0.f, 10.f, 0.f, 2.f));
		}
	}

	USpinBox* AddSpinBoxRow(UWidgetTree* Tree, UPanelWidget* Parent, FName Name, const FText& LabelText, float Min, float Max, float Delta, float InitialValue)
	{
		AddLabel(Tree, Parent, LabelText);
		USpinBox* SpinBox = Tree->ConstructWidget<USpinBox>(USpinBox::StaticClass(), Name);
		SpinBox->SetMinValue(Min);
		SpinBox->SetMaxValue(Max);
		SpinBox->SetMinSliderValue(Min);
		SpinBox->SetMaxSliderValue(Max);
		SpinBox->SetDelta(Delta);
		SpinBox->SetValue(InitialValue);
		Parent->AddChild(SpinBox);
		return SpinBox;
	}

	UCheckBox* AddCheckBoxRow(UWidgetTree* Tree, UPanelWidget* Parent, FName Name, const FText& LabelText, bool bInitialValue)
	{
		UHorizontalBox* Row = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), *(Name.ToString() + TEXT("_Row")));
		if (UVerticalBoxSlot* RowSlot = Cast<UVerticalBoxSlot>(Parent->AddChild(Row)))
		{
			RowSlot->SetPadding(FMargin(0.f, 10.f, 0.f, 2.f));
		}

		UCheckBox* CheckBox = Tree->ConstructWidget<UCheckBox>(UCheckBox::StaticClass(), Name);
		CheckBox->SetIsChecked(bInitialValue);
		UHorizontalBoxSlot* CheckSlot = Row->AddChildToHorizontalBox(CheckBox);
		CheckSlot->SetVerticalAlignment(VAlign_Center);
		CheckSlot->SetPadding(FMargin(0.f, 0.f, 10.f, 0.f));

		Row->AddChildToHorizontalBox(JinzzaUI::MakeBodyText(Tree, NAME_None, LabelText, true));
		return CheckBox;
	}

	USlider* AddSliderRow(UWidgetTree* Tree, UPanelWidget* Parent, FName Name, const FText& LabelText, float Min, float Max, float InitialValue)
	{
		AddLabel(Tree, Parent, LabelText);
		USlider* Slider = Tree->ConstructWidget<USlider>(USlider::StaticClass(), Name);
		Slider->SetMinValue(Min);
		Slider->SetMaxValue(Max);
		Slider->SetValue(InitialValue);
		Parent->AddChild(Slider);
		return Slider;
	}

	UComboBoxString* AddComboRow(UWidgetTree* Tree, UPanelWidget* Parent, FName Name, const FText& LabelText, const TArray<FString>& Options, const FString& InitialSelection)
	{
		AddLabel(Tree, Parent, LabelText);
		UComboBoxString* Combo = Tree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), Name);
		for (const FString& Option : Options)
		{
			Combo->AddOption(Option);
		}
		Combo->SetSelectedOption(InitialSelection);
		Parent->AddChild(Combo);
		return Combo;
	}
}

void UjinzzaSettingsWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("SettingsRootCanvas"));
	WidgetTree->RootWidget = RootCanvas;

	UBorder* Background = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SettingsBackground"));
	Background->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.6f));
	UCanvasPanelSlot* BgSlot = RootCanvas->AddChildToCanvas(Background);
	BgSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
	BgSlot->SetOffsets(FMargin(0.f));

	UBorder* Panel = JinzzaUI::MakePanelBackground(WidgetTree, TEXT("SettingsPanel"));
	Panel->SetHorizontalAlignment(HAlign_Center);
	Panel->SetVerticalAlignment(VAlign_Center);
	Panel->SetPadding(FMargin(40.f, 30.f));
	Background->SetContent(Panel);

	UVerticalBox* Root = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SettingsRoot"));
	Panel->SetContent(Root);

	UTextBlock* Title = JinzzaUI::MakeTitleText(WidgetTree, TEXT("SettingsTitle"), FText::FromString(TEXT("SETTINGS")), 34);
	UVerticalBoxSlot* TitleSlot = Root->AddChildToVerticalBox(Title);
	TitleSlot->SetHorizontalAlignment(HAlign_Center);
	TitleSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 16.f));

	// Body: a left tab sidebar next to a right content column, rather than a top tab bar -
	// see the class header comment for why.
	UHorizontalBox* Body = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("SettingsBody"));
	Root->AddChildToVerticalBox(Body);

	USizeBox* SidebarSizer = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SidebarSizer"));
	SidebarSizer->SetWidthOverride(160.f);
	UHorizontalBoxSlot* SidebarSlot = Body->AddChildToHorizontalBox(SidebarSizer);
	SidebarSlot->SetPadding(FMargin(0.f, 0.f, 20.f, 0.f));

	UVerticalBox* Sidebar = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Sidebar"));
	SidebarSizer->AddChild(Sidebar);

	UVerticalBox* ContentColumn = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SettingsContentColumn"));
	UHorizontalBoxSlot* ContentColumnSlot = Body->AddChildToHorizontalBox(ContentColumn);
	ContentColumnSlot->SetSize(ESlateSizeRule::Fill);

	auto AddTabButton = [&](FName Name, const FText& Label, int32 TabIndex) -> UButton*
	{
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), *(Name.ToString() + TEXT("_Row")));
		UVerticalBoxSlot* RowSlot = Sidebar->AddChildToVerticalBox(Row);
		RowSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));

		USizeBox* AccentSizer = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), *(Name.ToString() + TEXT("_AccentSizer")));
		AccentSizer->SetWidthOverride(4.f);
		AccentSizer->SetHeightOverride(28.f);
		UBorder* Accent = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), *(Name.ToString() + TEXT("_Accent")));
		Accent->SetBrushColor(JinzzaUI::Color_Accent);
		Accent->SetVisibility(TabIndex == Tab_Graphics ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
		AccentSizer->AddChild(Accent);
		TabAccentBars.Add(Accent);
		UHorizontalBoxSlot* AccentSlot = Row->AddChildToHorizontalBox(AccentSizer);
		AccentSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));

		UButton* TabButton = JinzzaUI::MakeSecondaryButton(WidgetTree, Name, Label, 15.f);
		UHorizontalBoxSlot* ButtonSlot = Row->AddChildToHorizontalBox(TabButton);
		ButtonSlot->SetSize(ESlateSizeRule::Fill);
		return TabButton;
	};
	// AddDynamic stringifies its handler argument at the call site, so each binding has to be written out here
	// rather than passed through AddTabButton as a member-function-pointer parameter.
	AddTabButton(TEXT("TabGraphics"), FText::FromString(TEXT("Graphics")), Tab_Graphics)->OnClicked.AddDynamic(this, &UjinzzaSettingsWidget::OnTabGraphicsClicked);
	AddTabButton(TEXT("TabAudio"), FText::FromString(TEXT("Audio")), Tab_Audio)->OnClicked.AddDynamic(this, &UjinzzaSettingsWidget::OnTabAudioClicked);
	AddTabButton(TEXT("TabControls"), FText::FromString(TEXT("Controls")), Tab_Controls)->OnClicked.AddDynamic(this, &UjinzzaSettingsWidget::OnTabControlsClicked);
	AddTabButton(TEXT("TabGameplay"), FText::FromString(TEXT("Gameplay")), Tab_Gameplay)->OnClicked.AddDynamic(this, &UjinzzaSettingsWidget::OnTabGameplayClicked);

	// Content switcher, each page wrapped in a ScrollBox in case a page overflows the panel height.
	TabSwitcher = WidgetTree->ConstructWidget<UWidgetSwitcher>(UWidgetSwitcher::StaticClass(), TEXT("TabSwitcher"));
	UVerticalBoxSlot* SwitcherSlot = ContentColumn->AddChildToVerticalBox(TabSwitcher);
	SwitcherSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 16.f));
	SwitcherSlot->SetSize(ESlateSizeRule::Fill);

	auto MakePage = [&](FName Name) -> UVerticalBox*
	{
		UScrollBox* ScrollBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), *(Name.ToString() + TEXT("_Scroll")));
		TabSwitcher->AddChild(ScrollBox);

		// Rows are built against a UVerticalBox (for per-row padding via UVerticalBoxSlot) nested inside the
		// ScrollBox, rather than added to the ScrollBox directly (whose slots are UScrollBoxSlot, not UVerticalBoxSlot).
		UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), Name);
		ScrollBox->AddChild(Content);
		return Content;
	};

	UVerticalBox* GraphicsPage = MakePage(TEXT("GraphicsPage"));
	UVerticalBox* AudioPage = MakePage(TEXT("AudioPage"));
	UVerticalBox* ControlsPage = MakePage(TEXT("ControlsPage"));
	UVerticalBox* GameplayPage = MakePage(TEXT("GameplayPage"));

	BuildGraphicsPage(WidgetTree, GraphicsPage);
	BuildAudioPage(WidgetTree, AudioPage);
	BuildControlsPage(WidgetTree, ControlsPage);
	BuildGameplayPage(WidgetTree, GameplayPage);

	TabSwitcher->SetActiveWidgetIndex(Tab_Graphics);

	// Bottom-right button cluster (Apply / Back), matching the reference layout's bottom-right
	// Apply/Cancel corner rather than the previous centered row.
	UHorizontalBox* ButtonRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("SettingsButtonRow"));
	UVerticalBoxSlot* ButtonRowSlot = ContentColumn->AddChildToVerticalBox(ButtonRow);
	ButtonRowSlot->SetHorizontalAlignment(HAlign_Right);

	UButton* ApplyButton = JinzzaUI::MakePrimaryButton(WidgetTree, TEXT("SettingsApplyButton"), FText::FromString(TEXT("Apply")));
	ApplyButton->OnClicked.AddDynamic(this, &UjinzzaSettingsWidget::OnApplyClicked);
	UHorizontalBoxSlot* ApplySlot = ButtonRow->AddChildToHorizontalBox(ApplyButton);
	ApplySlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));

	UButton* BackButton = JinzzaUI::MakeSecondaryButton(WidgetTree, TEXT("SettingsBackButton"), FText::FromString(TEXT("Back")));
	BackButton->OnClicked.AddDynamic(this, &UjinzzaSettingsWidget::OnBackClicked);
	ButtonRow->AddChildToHorizontalBox(BackButton);
}

void UjinzzaSettingsWidget::BuildGraphicsPage(UWidgetTree* Tree, UPanelWidget* Parent)
{
	UjinzzaGameUserSettings* Settings = UjinzzaGameUserSettings::Get();
	if (!Settings)
	{
		return;
	}

	AddHeading(Tree, Parent, FText::FromString(TEXT("Display")));

	const TArray<FString> WindowModes = { TEXT("Fullscreen"), TEXT("Windowed Fullscreen"), TEXT("Windowed") };
	FString CurrentWindowMode = TEXT("Fullscreen");
	switch (Settings->GetFullscreenMode())
	{
	case EWindowMode::Fullscreen: CurrentWindowMode = TEXT("Fullscreen"); break;
	case EWindowMode::WindowedFullscreen: CurrentWindowMode = TEXT("Windowed Fullscreen"); break;
	case EWindowMode::Windowed: CurrentWindowMode = TEXT("Windowed"); break;
	default: break;
	}
	WindowModeCombo = AddComboRow(Tree, Parent, TEXT("WindowModeCombo"), FText::FromString(TEXT("Window Mode")), WindowModes, CurrentWindowMode);

	UKismetSystemLibrary::GetConvenientWindowedResolutions(AvailableResolutions);
	const FIntPoint CurrentResolution = Settings->GetScreenResolution();
	if (!AvailableResolutions.Contains(CurrentResolution))
	{
		AvailableResolutions.Insert(CurrentResolution, 0);
	}
	TArray<FString> ResolutionOptions;
	for (const FIntPoint& Res : AvailableResolutions)
	{
		ResolutionOptions.Add(ResolutionToString(Res));
	}
	ResolutionCombo = AddComboRow(Tree, Parent, TEXT("ResolutionCombo"), FText::FromString(TEXT("Resolution")), ResolutionOptions, ResolutionToString(CurrentResolution));

	VSyncCheckBox = AddCheckBoxRow(Tree, Parent, TEXT("VSyncCheckBox"), FText::FromString(TEXT("V-Sync")), Settings->IsVSyncEnabled());
	FrameRateLimitSpinBox = AddSpinBoxRow(Tree, Parent, TEXT("FrameRateLimitSpinBox"), FText::FromString(TEXT("Frame Rate Limit (0 = Unlimited)")), 0.f, 300.f, 1.f, Settings->GetFrameRateLimit());

	AddHeading(Tree, Parent, FText::FromString(TEXT("Quality")));

	const TArray<FString> QualityPresets = { TEXT("Low"), TEXT("Medium"), TEXT("High"), TEXT("Epic"), TEXT("Cinematic") };
	const int32 OverallLevel = FMath::Clamp(Settings->GetOverallScalabilityLevel(), 0, QualityPresets.Num() - 1);
	OverallQualityCombo = AddComboRow(Tree, Parent, TEXT("OverallQualityCombo"), FText::FromString(TEXT("Overall Quality")), QualityPresets, QualityPresets[OverallLevel]);

	ViewDistanceSpinBox = AddSpinBoxRow(Tree, Parent, TEXT("ViewDistanceSpinBox"), FText::FromString(TEXT("View Distance (0-4)")), 0.f, 4.f, 1.f, Settings->GetViewDistanceQuality());
	ShadowSpinBox = AddSpinBoxRow(Tree, Parent, TEXT("ShadowSpinBox"), FText::FromString(TEXT("Shadows (0-4)")), 0.f, 4.f, 1.f, Settings->GetShadowQuality());
	GlobalIlluminationSpinBox = AddSpinBoxRow(Tree, Parent, TEXT("GlobalIlluminationSpinBox"), FText::FromString(TEXT("Global Illumination (0-4)")), 0.f, 4.f, 1.f, Settings->GetGlobalIlluminationQuality());
	ReflectionSpinBox = AddSpinBoxRow(Tree, Parent, TEXT("ReflectionSpinBox"), FText::FromString(TEXT("Reflections (0-4)")), 0.f, 4.f, 1.f, Settings->GetReflectionQuality());
	AntiAliasingSpinBox = AddSpinBoxRow(Tree, Parent, TEXT("AntiAliasingSpinBox"), FText::FromString(TEXT("Anti-Aliasing (0-4)")), 0.f, 4.f, 1.f, Settings->GetAntiAliasingQuality());
	TextureSpinBox = AddSpinBoxRow(Tree, Parent, TEXT("TextureSpinBox"), FText::FromString(TEXT("Textures (0-4)")), 0.f, 4.f, 1.f, Settings->GetTextureQuality());
	EffectsSpinBox = AddSpinBoxRow(Tree, Parent, TEXT("EffectsSpinBox"), FText::FromString(TEXT("Effects (0-4)")), 0.f, 4.f, 1.f, Settings->GetVisualEffectQuality());
	FoliageSpinBox = AddSpinBoxRow(Tree, Parent, TEXT("FoliageSpinBox"), FText::FromString(TEXT("Foliage (0-4)")), 0.f, 4.f, 1.f, Settings->GetFoliageQuality());
	ShadingSpinBox = AddSpinBoxRow(Tree, Parent, TEXT("ShadingSpinBox"), FText::FromString(TEXT("Shading (0-4)")), 0.f, 4.f, 1.f, Settings->GetShadingQuality());
}

void UjinzzaSettingsWidget::BuildAudioPage(UWidgetTree* Tree, UPanelWidget* Parent)
{
	UjinzzaGameUserSettings* Settings = UjinzzaGameUserSettings::Get();
	if (!Settings)
	{
		return;
	}

	AddHeading(Tree, Parent, FText::FromString(TEXT("Volume")));
	MasterVolumeSlider = AddSliderRow(Tree, Parent, TEXT("MasterVolumeSlider"), FText::FromString(TEXT("Master")), 0.f, 1.f, Settings->GetMasterVolume());
	MusicVolumeSlider = AddSliderRow(Tree, Parent, TEXT("MusicVolumeSlider"), FText::FromString(TEXT("Music")), 0.f, 1.f, Settings->GetMusicVolume());
	SFXVolumeSlider = AddSliderRow(Tree, Parent, TEXT("SFXVolumeSlider"), FText::FromString(TEXT("SFX")), 0.f, 1.f, Settings->GetSFXVolume());
	VoiceVolumeSlider = AddSliderRow(Tree, Parent, TEXT("VoiceVolumeSlider"), FText::FromString(TEXT("Voice / Proximity Chat")), 0.f, 1.f, Settings->GetVoiceVolume());

	AddHeading(Tree, Parent, FText::FromString(TEXT("Microphone")));

	const TArray<FString> MicModes = { TEXT("Push to Talk"), TEXT("Open Mic") };
	const FString CurrentMicMode = MicModes[static_cast<int32>(Settings->GetMicInputMode())];
	MicInputModeCombo = AddComboRow(Tree, Parent, TEXT("MicInputModeCombo"), FText::FromString(TEXT("Mic Input")), MicModes, CurrentMicMode);

	AvailableMicDeviceIds = { FString() };
	const TArray<FString> DefaultDeviceOption = { TEXT("System Default") };
	MicDeviceCombo = AddComboRow(Tree, Parent, TEXT("MicDeviceCombo"), FText::FromString(TEXT("Microphone Device")), DefaultDeviceOption, TEXT("System Default"));

	// Device list is fetched async; OnAudioInputDevicesObtained repopulates the combo (and restores
	// the saved selection) once the platform responds, usually within a frame or two.
	FOnAudioInputDevicesObtained DevicesObtained;
	DevicesObtained.BindDynamic(this, &UjinzzaSettingsWidget::OnAudioInputDevicesObtained);
	UAudioCaptureBlueprintLibrary::GetAvailableAudioInputDevices(this, DevicesObtained);
}

void UjinzzaSettingsWidget::OnAudioInputDevicesObtained(const TArray<FAudioInputDeviceInfo>& AvailableDevices)
{
	if (!MicDeviceCombo)
	{
		return;
	}

	const UjinzzaGameUserSettings* Settings = UjinzzaGameUserSettings::Get();
	const FString SavedDeviceId = Settings ? Settings->GetMicDeviceId() : FString();

	MicDeviceCombo->ClearOptions();
	MicDeviceCombo->AddOption(TEXT("System Default"));
	AvailableMicDeviceIds = { FString() };

	for (const FAudioInputDeviceInfo& Device : AvailableDevices)
	{
		AvailableMicDeviceIds.Add(Device.DeviceId);
		MicDeviceCombo->AddOption(Device.DeviceName);
	}

	const int32 SavedIndex = AvailableMicDeviceIds.IndexOfByKey(SavedDeviceId);
	MicDeviceCombo->SetSelectedIndex(SavedIndex != INDEX_NONE ? SavedIndex : 0);
}

void UjinzzaSettingsWidget::BuildControlsPage(UWidgetTree* Tree, UPanelWidget* Parent)
{
	UjinzzaGameUserSettings* Settings = UjinzzaGameUserSettings::Get();
	if (!Settings)
	{
		return;
	}

	AddHeading(Tree, Parent, FText::FromString(TEXT("Mouse")));
	MouseSensitivitySlider = AddSliderRow(Tree, Parent, TEXT("MouseSensitivitySlider"), FText::FromString(TEXT("Sensitivity")), 0.1f, 3.f, Settings->GetMouseSensitivity());
	InvertYCheckBox = AddCheckBoxRow(Tree, Parent, TEXT("InvertYCheckBox"), FText::FromString(TEXT("Invert Y Axis")), Settings->GetInvertYAxis());

	AddHeading(Tree, Parent, FText::FromString(TEXT("Key Bindings")));

	UButton* JumpButton = AddRebindRow(Tree, Parent, TEXT("IA_Jump"), FText::FromString(TEXT("Jump")));
	JumpButton->OnClicked.AddDynamic(this, &UjinzzaSettingsWidget::OnRebindJumpClicked);

	UButton* ShootButton = AddRebindRow(Tree, Parent, TEXT("IA_Shoot"), FText::FromString(TEXT("Shoot")));
	ShootButton->OnClicked.AddDynamic(this, &UjinzzaSettingsWidget::OnRebindShootClicked);

	UButton* SwapWeaponButton = AddRebindRow(Tree, Parent, TEXT("IA_SwapWeapon"), FText::FromString(TEXT("Swap Weapon")));
	SwapWeaponButton->OnClicked.AddDynamic(this, &UjinzzaSettingsWidget::OnRebindSwapWeaponClicked);

	UButton* SprintButton = AddRebindRow(Tree, Parent, TEXT("IA_Sprint"), FText::FromString(TEXT("Sprint")));
	SprintButton->OnClicked.AddDynamic(this, &UjinzzaSettingsWidget::OnRebindSprintClicked);
}

void UjinzzaSettingsWidget::BuildGameplayPage(UWidgetTree* Tree, UPanelWidget* Parent)
{
	UjinzzaGameUserSettings* Settings = UjinzzaGameUserSettings::Get();
	if (!Settings)
	{
		return;
	}

	AddHeading(Tree, Parent, FText::FromString(TEXT("Accessibility")));
	SubtitlesCheckBox = AddCheckBoxRow(Tree, Parent, TEXT("SubtitlesCheckBox"), FText::FromString(TEXT("Subtitles")), Settings->GetSubtitlesEnabled());

	const TArray<FString> ColorblindOptions = { TEXT("Off"), TEXT("Deuteranope"), TEXT("Protanope"), TEXT("Tritanope") };
	ColorblindModeCombo = AddComboRow(Tree, Parent, TEXT("ColorblindModeCombo"), FText::FromString(TEXT("Colorblind Mode")),
		ColorblindOptions, ColorblindOptions[static_cast<int32>(Settings->GetColorblindMode())]);
	ColorblindStrengthSlider = AddSliderRow(Tree, Parent, TEXT("ColorblindStrengthSlider"), FText::FromString(TEXT("Colorblind Correction Strength")), 0.f, 1.f, Settings->GetColorblindStrength());
}

UButton* UjinzzaSettingsWidget::AddRebindRow(UWidgetTree* Tree, UPanelWidget* Parent, FName ActionName, const FText& RowLabel)
{
	UjinzzaGameUserSettings* Settings = UjinzzaGameUserSettings::Get();
	const FKey CurrentKey = Settings ? Settings->GetKeyRebind(ActionName) : EKeys::Invalid;

	UHorizontalBox* Row = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), *(ActionName.ToString() + TEXT("_RebindRow")));
	if (UVerticalBoxSlot* RowSlot = Cast<UVerticalBoxSlot>(Parent->AddChild(Row)))
	{
		RowSlot->SetPadding(FMargin(0.f, 6.f));
	}

	UTextBlock* NameText = JinzzaUI::MakeBodyText(Tree, NAME_None, RowLabel, false);
	UHorizontalBoxSlot* NameSlot = Row->AddChildToHorizontalBox(NameText);
	NameSlot->SetVerticalAlignment(VAlign_Center);
	NameSlot->SetSize(ESlateSizeRule::Fill);

	UButton* RebindButton = JinzzaUI::MakeSecondaryButton(Tree, *(ActionName.ToString() + TEXT("_RebindButton")),
		FText::FromString(CurrentKey.IsValid() ? CurrentKey.GetDisplayName().ToString() : TEXT("Default")), 15.f);
	Row->AddChildToHorizontalBox(RebindButton);

	UTextBlock* KeyLabel = Cast<UTextBlock>(RebindButton->GetChildAt(0));
	RebindLabels.Add(ActionName, KeyLabel);

	return RebindButton;
}

void UjinzzaSettingsWidget::StartRebind(FName ActionName)
{
	bWaitingForRebind = true;
	PendingRebindAction = ActionName;

	if (TObjectPtr<UTextBlock>* Label = RebindLabels.Find(ActionName))
	{
		if (*Label)
		{
			(*Label)->SetText(FText::FromString(TEXT("Press a key...")));
		}
	}

	SetKeyboardFocus();
}

void UjinzzaSettingsWidget::RefreshRebindButtonLabel(FName ActionName)
{
	UjinzzaGameUserSettings* Settings = UjinzzaGameUserSettings::Get();
	if (!Settings)
	{
		return;
	}

	if (TObjectPtr<UTextBlock>* Label = RebindLabels.Find(ActionName))
	{
		if (*Label)
		{
			const FKey Key = Settings->GetKeyRebind(ActionName);
			(*Label)->SetText(FText::FromString(Key.IsValid() ? Key.GetDisplayName().ToString() : TEXT("Default")));
		}
	}
}

FReply UjinzzaSettingsWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (bWaitingForRebind)
	{
		if (UjinzzaGameUserSettings* Settings = UjinzzaGameUserSettings::Get())
		{
			Settings->SetKeyRebind(PendingRebindAction, InKeyEvent.GetKey());
		}

		RefreshRebindButtonLabel(PendingRebindAction);
		bWaitingForRebind = false;
		PendingRebindAction = NAME_None;
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UjinzzaSettingsWidget::OnRebindJumpClicked() { StartRebind(TEXT("IA_Jump")); }
void UjinzzaSettingsWidget::OnRebindShootClicked() { StartRebind(TEXT("IA_Shoot")); }
void UjinzzaSettingsWidget::OnRebindSwapWeaponClicked() { StartRebind(TEXT("IA_SwapWeapon")); }
void UjinzzaSettingsWidget::OnRebindSprintClicked() { StartRebind(TEXT("IA_Sprint")); }

void UjinzzaSettingsWidget::SetActiveTab(int32 TabIndex)
{
	if (TabSwitcher)
	{
		TabSwitcher->SetActiveWidgetIndex(TabIndex);
	}

	for (int32 Index = 0; Index < TabAccentBars.Num(); ++Index)
	{
		if (UWidget* Accent = TabAccentBars[Index])
		{
			Accent->SetVisibility(Index == TabIndex ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
		}
	}
}

void UjinzzaSettingsWidget::OnTabGraphicsClicked() { SetActiveTab(Tab_Graphics); }
void UjinzzaSettingsWidget::OnTabAudioClicked() { SetActiveTab(Tab_Audio); }
void UjinzzaSettingsWidget::OnTabControlsClicked() { SetActiveTab(Tab_Controls); }
void UjinzzaSettingsWidget::OnTabGameplayClicked() { SetActiveTab(Tab_Gameplay); }

void UjinzzaSettingsWidget::OnBackClicked()
{
	OnBackRequested.Broadcast();
}

void UjinzzaSettingsWidget::OnApplyClicked()
{
	UjinzzaGameUserSettings* Settings = UjinzzaGameUserSettings::Get();
	if (!Settings)
	{
		return;
	}

	// Graphics
	if (WindowModeCombo)
	{
		const FString Selection = WindowModeCombo->GetSelectedOption();
		EWindowMode::Type Mode = EWindowMode::Fullscreen;
		if (Selection == TEXT("Windowed Fullscreen")) Mode = EWindowMode::WindowedFullscreen;
		else if (Selection == TEXT("Windowed")) Mode = EWindowMode::Windowed;
		Settings->SetFullscreenMode(Mode);
	}
	if (ResolutionCombo)
	{
		const int32 Index = ResolutionCombo->GetSelectedIndex();
		if (AvailableResolutions.IsValidIndex(Index))
		{
			Settings->SetScreenResolution(AvailableResolutions[Index]);
		}
	}
	if (VSyncCheckBox) Settings->SetVSyncEnabled(VSyncCheckBox->IsChecked());
	if (FrameRateLimitSpinBox) Settings->SetFrameRateLimit(FrameRateLimitSpinBox->GetValue());
	if (OverallQualityCombo) Settings->SetOverallScalabilityLevel(OverallQualityCombo->GetSelectedIndex());
	if (ViewDistanceSpinBox) Settings->SetViewDistanceQuality(FMath::RoundToInt(ViewDistanceSpinBox->GetValue()));
	if (ShadowSpinBox) Settings->SetShadowQuality(FMath::RoundToInt(ShadowSpinBox->GetValue()));
	if (GlobalIlluminationSpinBox) Settings->SetGlobalIlluminationQuality(FMath::RoundToInt(GlobalIlluminationSpinBox->GetValue()));
	if (ReflectionSpinBox) Settings->SetReflectionQuality(FMath::RoundToInt(ReflectionSpinBox->GetValue()));
	if (AntiAliasingSpinBox) Settings->SetAntiAliasingQuality(FMath::RoundToInt(AntiAliasingSpinBox->GetValue()));
	if (TextureSpinBox) Settings->SetTextureQuality(FMath::RoundToInt(TextureSpinBox->GetValue()));
	if (EffectsSpinBox) Settings->SetVisualEffectQuality(FMath::RoundToInt(EffectsSpinBox->GetValue()));
	if (FoliageSpinBox) Settings->SetFoliageQuality(FMath::RoundToInt(FoliageSpinBox->GetValue()));
	if (ShadingSpinBox) Settings->SetShadingQuality(FMath::RoundToInt(ShadingSpinBox->GetValue()));

	// Audio
	if (MasterVolumeSlider) Settings->SetMasterVolume(MasterVolumeSlider->GetValue());
	if (MusicVolumeSlider) Settings->SetMusicVolume(MusicVolumeSlider->GetValue());
	if (SFXVolumeSlider) Settings->SetSFXVolume(SFXVolumeSlider->GetValue());
	if (VoiceVolumeSlider) Settings->SetVoiceVolume(VoiceVolumeSlider->GetValue());
	if (MicInputModeCombo) Settings->SetMicInputMode(MicInputModeCombo->GetSelectedIndex() == 1 ? EJinzzaMicInputMode::OpenMic : EJinzzaMicInputMode::PushToTalk);
	if (MicDeviceCombo && AvailableMicDeviceIds.IsValidIndex(MicDeviceCombo->GetSelectedIndex()))
	{
		Settings->SetMicDeviceId(AvailableMicDeviceIds[MicDeviceCombo->GetSelectedIndex()]);
	}

	// Controls
	if (MouseSensitivitySlider) Settings->SetMouseSensitivity(MouseSensitivitySlider->GetValue());
	if (InvertYCheckBox) Settings->SetInvertYAxis(InvertYCheckBox->IsChecked());

	// Gameplay
	if (SubtitlesCheckBox) Settings->SetSubtitlesEnabled(SubtitlesCheckBox->IsChecked());
	if (ColorblindModeCombo) Settings->SetColorblindMode(static_cast<EJinzzaColorblindMode>(ColorblindModeCombo->GetSelectedIndex()));
	if (ColorblindStrengthSlider) Settings->SetColorblindStrength(ColorblindStrengthSlider->GetValue());

	Settings->ApplySettings(false);
	Settings->SaveSettings();
}
