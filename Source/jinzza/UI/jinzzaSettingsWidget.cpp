// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaSettingsWidget.h"
#include "jinzzaGameUserSettings.h"
#include "jinzzaInputKeys.h"
#include "jinzzaPlayerController.h"
#include "jinzzaUIStyle.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/SpinBox.h"
#include "Components/Slider.h"
#include "Components/CheckBox.h"
#include "Components/WidgetSwitcher.h"
#include "Components/Widget.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/ScrollBox.h"
#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateRoundedBoxBrush.h"
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

	/** Accent color for selected tabs/pills. */
	const FLinearColor& SettingsTabAccent()
	{
		return JinzzaUI::Sticker_Yellow;
	}

	// Big, readable sizes - this screen is meant to be read from the couch, not squinted at.
	constexpr float SettingsControlWidth = 580.f;
	constexpr float SettingsPillFont = 20.f;
	
	/** Header tab: a big sticker pill with a yellow dot under it (the dot is the "accent", shown by SetActiveTab on the active tab only). */
	UWidget* MakeSettingsTab(UWidgetTree* Tree, FName Name, const FText& Label, UButton*& OutButton, UWidget*& OutDot)
	{
		UVerticalBox* Column = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), *(Name.ToString() + TEXT("Tab")));
		OutButton = JinzzaUI::MakeStickerButton(Tree, *(Name.ToString() + TEXT("Button")), Label, SettingsTabAccent(), 24.f);
		Column->AddChildToVerticalBox(OutButton);
		OutDot = JinzzaUI::MakeStickerDot(Tree, *(Name.ToString() + TEXT("Accent")), SettingsTabAccent(), 12.f);
		if (UVerticalBoxSlot* DotSlot = Column->AddChildToVerticalBox(OutDot))
		{
			DotSlot->SetHorizontalAlignment(HAlign_Center);
			DotSlot->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));
		}
		return Column;
	}

}

void UJinzzaSettingsSegmentHandler::HandleClicked()
{
	if (UjinzzaSettingsWidget* Owner = OwnerWidget.Get())
	{
		Owner->SelectSegment(GroupIndex, OptionIndex);
	}
}

void UjinzzaSettingsWidget::BuildWidgetTree()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	// One big black sticker panel (thick white outline, like T_Logo) centered over the main
	// menu's dimmed 3D backdrop - see UjinzzaMainMenuWidget::ShowPage. The root is transparent.
	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	UBorder* PanelFace = nullptr;
	UOverlay* Panel = JinzzaUI::MakeStickerPanel(WidgetTree, TEXT("Panel"), PanelFace);
	PanelFace->SetPadding(FMargin(40.f, 30.f, 40.f, 32.f));
	if (UOverlaySlot* PanelSlot = Root->AddChildToOverlay(Panel))
	{
		PanelSlot->SetHorizontalAlignment(HAlign_Center);
		PanelSlot->SetVerticalAlignment(VAlign_Center);
	}

	USizeBox* PanelBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PanelBox"));
	PanelBox->SetWidthOverride(1240.f);
	PanelBox->SetHeightOverride(780.f);
	PanelFace->SetContent(PanelBox);

	UVerticalBox* PanelStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("PanelStack"));
	PanelBox->AddChild(PanelStack);

	// --- Header: big title left, big tab pills right ---
	UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Header"));
	PanelStack->AddChildToVerticalBox(Header);

	if (UHorizontalBoxSlot* TitleSlot = Header->AddChildToHorizontalBox(JinzzaUI::MakeStickerHeading(WidgetTree, TEXT("Heading"), FText::FromString(TEXT("Settings")), 48)))
	{
		TitleSlot->SetSize(ESlateSizeRule::Fill);
		TitleSlot->SetVerticalAlignment(VAlign_Top);
	}

	auto AddTab = [&](FName Name, const TCHAR* Label, TObjectPtr<UButton>& OutButton, TObjectPtr<UWidget>& OutAccent)
	{
		UButton* Button = nullptr;
		UWidget* Dot = nullptr;
		UWidget* Tab = MakeSettingsTab(WidgetTree, Name, FText::FromString(Label), Button, Dot);
		OutButton = Button;
		OutAccent = Dot;
		if (UHorizontalBoxSlot* TabSlot = Header->AddChildToHorizontalBox(Tab))
		{
			TabSlot->SetVerticalAlignment(VAlign_Top);
			TabSlot->SetPadding(FMargin(12.f, 6.f, 0.f, 0.f));
		}
	};
	AddTab(TEXT("Graphics"), TEXT("Graphics"), GraphicsTabButton, GraphicsTabAccent);
	AddTab(TEXT("Audio"), TEXT("Sound"), AudioTabButton, AudioTabAccent);
	AddTab(TEXT("Controls"), TEXT("Controls"), ControlsTabButton, ControlsTabAccent);
	AddTab(TEXT("Gameplay"), TEXT("Gameplay"), GameplayTabButton, GameplayTabAccent);

	// --- Pages ---
	TabSwitcher = WidgetTree->ConstructWidget<UWidgetSwitcher>(UWidgetSwitcher::StaticClass(), TEXT("TabSwitcher"));
	if (UVerticalBoxSlot* SwitcherSlot = JinzzaUI::AddSpaced(PanelStack, TabSwitcher, 18.f))
	{
		SwitcherSlot->SetSize(ESlateSizeRule::Fill);
	}

	auto MakePage = [&](FName Name) -> UVerticalBox*
	{
		UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), Name);
		UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), *(Name.ToString() + TEXT("_Content")));
		Scroll->AddChild(Content);
		TabSwitcher->AddChild(Scroll);
		return Content;
	};

	// Data widgets that no longer get their own row (the per-feature quality spin boxes) still
	// need to live in the tree for Populate/Apply - they're parked here, collapsed.
	UHorizontalBox* HiddenHolders = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HiddenHolders"));
	HiddenHolders->SetVisibility(ESlateVisibility::Collapsed);
	PanelStack->AddChildToVerticalBox(HiddenHolders);

	// Big pill selector bound to a data widget (Spin / Combo / Check) - see FSegmentGroup. The data
	// widget itself is parked collapsed in HiddenHolders.
	auto AddPillRow = [&](UVerticalBox* Page, FName Name, const FText& Label, const TArray<FString>& Options,
		USpinBox* Spin, UComboBoxString* Combo, UCheckBox* Check, const TArray<float>& Values = TArray<float>()) -> int32
	{
		const int32 GroupIndex = SegmentGroups.AddDefaulted();
		FSegmentGroup& Group = SegmentGroups[GroupIndex];
		Group.Spin = Spin;
		Group.Combo = Combo;
		Group.Check = Check;
		Group.Values = Values;

		UHorizontalBox* Pills = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), *(Name.ToString() + TEXT("_Pills")));
		for (int32 OptionIndex = 0; OptionIndex < Options.Num(); ++OptionIndex)
		{
			UButton* Pill = JinzzaUI::MakeStickerButton(WidgetTree, *FString::Printf(TEXT("%s_Pill%d"), *Name.ToString(), OptionIndex),
				FText::FromString(Options[OptionIndex]), SettingsTabAccent(), SettingsPillFont);
			if (UHorizontalBoxSlot* PillSlot = Pills->AddChildToHorizontalBox(Pill))
			{
				PillSlot->SetSize(ESlateSizeRule::Fill);
				PillSlot->SetPadding(FMargin(OptionIndex == 0 ? 0.f : 8.f, 0.f, 0.f, 0.f));
			}

			UJinzzaSettingsSegmentHandler* Handler = NewObject<UJinzzaSettingsSegmentHandler>(this);
			Handler->OwnerWidget = this;
			Handler->GroupIndex = GroupIndex;
			Handler->OptionIndex = OptionIndex;
			Pill->OnClicked.AddDynamic(Handler, &UJinzzaSettingsSegmentHandler::HandleClicked);
			SegmentHandlers.Add(Handler);
			SegmentGroups[GroupIndex].Buttons.Add(Pill);
		}

		JinzzaUI::AddStickerRow(WidgetTree, Page, Name, Label, Pills, SettingsControlWidth);
		if (UWidget* Holder = Spin ? static_cast<UWidget*>(Spin) : Combo ? static_cast<UWidget*>(Combo) : static_cast<UWidget*>(Check))
		{
			HiddenHolders->AddChildToHorizontalBox(Holder);
		}
		return GroupIndex;
	};

	auto AddOnOffRow = [&](UVerticalBox* Page, FName Name, const FText& Label, UCheckBox* Check)
	{
		AddPillRow(Page, Name, Label, { TEXT("Off"), TEXT("On") }, nullptr, nullptr, Check);
	};

	auto AddSliderRow = [&](UVerticalBox* Page, FName Name, const FText& Label, USlider* Slider, bool bPercent)
	{
		JinzzaUI::ApplyStickerStyle(Slider);
		UHorizontalBox* Control = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), *(Name.ToString() + TEXT("_Control")));
		if (UHorizontalBoxSlot* SliderSlot = Control->AddChildToHorizontalBox(Slider))
		{
			SliderSlot->SetSize(ESlateSizeRule::Fill);
			SliderSlot->SetVerticalAlignment(VAlign_Center);
		}
		UTextBlock* Readout = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *(Name.ToString() + TEXT("_Readout")));
		Readout->SetFont(JinzzaUI::HeadingFont(24));
		Readout->SetColorAndOpacity(FSlateColor(JinzzaUI::Sticker_Yellow));
		Readout->SetJustification(ETextJustify::Right);
		USizeBox* ReadoutBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), *(Name.ToString() + TEXT("_ReadoutBox")));
		ReadoutBox->SetWidthOverride(96.f);
		ReadoutBox->AddChild(Readout);
		if (UHorizontalBoxSlot* ReadoutSlot = Control->AddChildToHorizontalBox(ReadoutBox))
		{
			ReadoutSlot->SetVerticalAlignment(VAlign_Center);
		}
		JinzzaUI::AddStickerRow(WidgetTree, Page, Name, Label, Control, SettingsControlWidth);

		FSliderReadout Entry;
		Entry.Slider = Slider;
		Entry.Label = Readout;
		Entry.bPercent = bPercent;
		SliderReadouts.Add(Entry);
	};

	auto AddComboRow = [&](UVerticalBox* Page, FName Name, const FText& Label, UComboBoxString* Combo)
	{
		JinzzaUI::ApplyStickerStyle(Combo);
		JinzzaUI::AddStickerRow(WidgetTree, Page, Name, Label, Combo, SettingsControlWidth);
	};

	// --- Graphics page (TabSwitcher index 0) ---
	UVerticalBox* GraphicsPage = MakePage(TEXT("GraphicsPage"));
	{
		JinzzaUI::AddStickerSection(WidgetTree, GraphicsPage, TEXT("ScreenSection"), FText::FromString(TEXT("Screen")), true);

		// Option order matches PopulateGraphicsPage's AddOption order.
		WindowModeCombo = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("WindowModeCombo"));
		AddPillRow(GraphicsPage, TEXT("WindowMode"), FText::FromString(TEXT("Window")), { TEXT("Full"), TEXT("Borderless"), TEXT("Window") }, nullptr, WindowModeCombo, nullptr);

		ResolutionCombo = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("ResolutionCombo"));
		AddComboRow(GraphicsPage, TEXT("Resolution"), FText::FromString(TEXT("Resolution")), ResolutionCombo);

		VSyncCheckBox = WidgetTree->ConstructWidget<UCheckBox>(UCheckBox::StaticClass(), TEXT("VSyncCheckBox"));
		AddOnOffRow(GraphicsPage, TEXT("VSync"), FText::FromString(TEXT("VSync")), VSyncCheckBox);

		JinzzaUI::AddStickerSection(WidgetTree, GraphicsPage, TEXT("LookSection"), FText::FromString(TEXT("Look")), false);

		// One Quality row instead of ten: the preset also writes all nine per-feature quality spin
		// boxes (see SelectSegment), which stay parked in HiddenHolders for Populate/Apply.
		OverallQualityCombo = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("OverallQualityCombo"));
		OverallQualityGroup = AddPillRow(GraphicsPage, TEXT("OverallQuality"), FText::FromString(TEXT("Quality")),
			{ TEXT("Low"), TEXT("Med"), TEXT("High"), TEXT("Epic"), TEXT("Max") }, nullptr, OverallQualityCombo, nullptr);

		auto ParkQualitySpin = [&](const TCHAR* Name, TObjectPtr<USpinBox>& OutSpin)
		{
			OutSpin = WidgetTree->ConstructWidget<USpinBox>(USpinBox::StaticClass(), Name);
			HiddenHolders->AddChildToHorizontalBox(OutSpin);
			// A pill-less group, so SelectSegment's Overall preset can write it like the others.
			const int32 GroupIndex = SegmentGroups.AddDefaulted();
			SegmentGroups[GroupIndex].Spin = OutSpin;
			DetailQualityGroups.Add(GroupIndex);
		};
		ParkQualitySpin(TEXT("ViewDistanceSpinBox"), ViewDistanceSpinBox);
		ParkQualitySpin(TEXT("ShadowSpinBox"), ShadowSpinBox);
		ParkQualitySpin(TEXT("GlobalIlluminationSpinBox"), GlobalIlluminationSpinBox);
		ParkQualitySpin(TEXT("ReflectionSpinBox"), ReflectionSpinBox);
		ParkQualitySpin(TEXT("AntiAliasingSpinBox"), AntiAliasingSpinBox);
		ParkQualitySpin(TEXT("TextureSpinBox"), TextureSpinBox);
		ParkQualitySpin(TEXT("EffectsSpinBox"), EffectsSpinBox);
		ParkQualitySpin(TEXT("FoliageSpinBox"), FoliageSpinBox);
		ParkQualitySpin(TEXT("ShadingSpinBox"), ShadingSpinBox);

		// Frame rate cap as pills - the spin box stores the cap itself (0 = no cap).
		FrameRateLimitSpinBox = WidgetTree->ConstructWidget<USpinBox>(USpinBox::StaticClass(), TEXT("FrameRateLimitSpinBox"));
		AddPillRow(GraphicsPage, TEXT("FrameRateLimit"), FText::FromString(TEXT("FPS Limit")),
			{ TEXT("30"), TEXT("60"), TEXT("120"), TEXT("144"), TEXT("None") }, FrameRateLimitSpinBox, nullptr, nullptr,
			{ 30.f, 60.f, 120.f, 144.f, 0.f });
	}

	// --- Audio page (index 1) ---
	UVerticalBox* AudioPage = MakePage(TEXT("AudioPage"));
	{
		JinzzaUI::AddStickerSection(WidgetTree, AudioPage, TEXT("VolumeSection"), FText::FromString(TEXT("Volume")), true);
		MasterVolumeSlider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass(), TEXT("MasterVolumeSlider"));
		AddSliderRow(AudioPage, TEXT("MasterVolume"), FText::FromString(TEXT("Master")), MasterVolumeSlider, true);
		MusicVolumeSlider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass(), TEXT("MusicVolumeSlider"));
		AddSliderRow(AudioPage, TEXT("MusicVolume"), FText::FromString(TEXT("Music")), MusicVolumeSlider, true);
		SFXVolumeSlider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass(), TEXT("SFXVolumeSlider"));
		AddSliderRow(AudioPage, TEXT("SFXVolume"), FText::FromString(TEXT("Effects")), SFXVolumeSlider, true);
		VoiceVolumeSlider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass(), TEXT("VoiceVolumeSlider"));
		AddSliderRow(AudioPage, TEXT("VoiceVolume"), FText::FromString(TEXT("Voice Chat")), VoiceVolumeSlider, true);

		JinzzaUI::AddStickerSection(WidgetTree, AudioPage, TEXT("MicSection"), FText::FromString(TEXT("Mic")), false);
		// Option order matches PopulateAudioPage's AddOption order.
		MicInputModeCombo = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("MicInputModeCombo"));
		AddPillRow(AudioPage, TEXT("MicInputMode"), FText::FromString(TEXT("Talk")), { TEXT("Push to Talk"), TEXT("Always On") }, nullptr, MicInputModeCombo, nullptr);
		MicDeviceCombo = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("MicDeviceCombo"));
		AddComboRow(AudioPage, TEXT("MicDevice"), FText::FromString(TEXT("Device")), MicDeviceCombo);
	}

	// --- Controls page (index 2) ---
	UVerticalBox* ControlsPage = MakePage(TEXT("ControlsPage"));
	{
		JinzzaUI::AddStickerSection(WidgetTree, ControlsPage, TEXT("MouseSection"), FText::FromString(TEXT("Mouse")), true);
		MouseSensitivitySlider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass(), TEXT("MouseSensitivitySlider"));
		AddSliderRow(ControlsPage, TEXT("MouseSensitivity"), FText::FromString(TEXT("Sensitivity")), MouseSensitivitySlider, false);
		InvertYCheckBox = WidgetTree->ConstructWidget<UCheckBox>(UCheckBox::StaticClass(), TEXT("InvertYCheckBox"));
		AddOnOffRow(ControlsPage, TEXT("InvertY"), FText::FromString(TEXT("Invert Y")), InvertYCheckBox);

		JinzzaUI::AddStickerSection(WidgetTree, ControlsPage, TEXT("KeysSection"), FText::FromString(TEXT("Keys")), false);
		// Each binding: the bound key as a big sticker key cap (its text block is the XxxRebindLabel
		// that RefreshRebindButtonLabel/StartRebind write into) + a "Change" button.
		auto AddRebindRow = [&](FName Name, const TCHAR* Label, TObjectPtr<UButton>& OutButton, TObjectPtr<UTextBlock>& OutLabel)
		{
			UHorizontalBox* Control = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), *(Name.ToString() + TEXT("_Control")));
			UTextBlock* KeyText = nullptr;
			UWidget* KeyCap = JinzzaUI::MakeStickerKeyCap(WidgetTree, *(Name.ToString() + TEXT("_KeyCap")), KeyText, 46.f);
			OutLabel = KeyText;
			if (UHorizontalBoxSlot* KeySlot = Control->AddChildToHorizontalBox(KeyCap))
			{
				KeySlot->SetSize(ESlateSizeRule::Fill);
				KeySlot->SetHorizontalAlignment(HAlign_Left);
				KeySlot->SetVerticalAlignment(VAlign_Center);
			}
			OutButton = JinzzaUI::MakeStickerButton(WidgetTree, *(Name.ToString() + TEXT("RebindButton")), FText::FromString(TEXT("Change")), JinzzaUI::Sticker_Sky, SettingsPillFont);
			if (UHorizontalBoxSlot* ButtonSlot = Control->AddChildToHorizontalBox(OutButton))
			{
				ButtonSlot->SetVerticalAlignment(VAlign_Center);
			}
			JinzzaUI::AddStickerRow(WidgetTree, ControlsPage, Name, FText::FromString(Label), Control, SettingsControlWidth);
		};
		AddRebindRow(TEXT("Jump"), TEXT("Jump"), JumpRebindButton, JumpRebindLabel);
		AddRebindRow(TEXT("Sprint"), TEXT("Run"), SprintRebindButton, SprintRebindLabel);
		AddRebindRow(TEXT("Interact"), TEXT("Use"), InteractRebindButton, InteractRebindLabel);
		AddRebindRow(TEXT("PushToTalk"), TEXT("Talk"), PushToTalkRebindButton, PushToTalkRebindLabel);
	}

	// --- Gameplay page (index 3) ---
	UVerticalBox* GameplayPage = MakePage(TEXT("GameplayPage"));
	{
		JinzzaUI::AddStickerSection(WidgetTree, GameplayPage, TEXT("AccessibilitySection"), FText::FromString(TEXT("Easy to See")), true);
		SubtitlesCheckBox = WidgetTree->ConstructWidget<UCheckBox>(UCheckBox::StaticClass(), TEXT("SubtitlesCheckBox"));
		AddOnOffRow(GameplayPage, TEXT("Subtitles"), FText::FromString(TEXT("Subtitles")), SubtitlesCheckBox);
		// Option order matches PopulateGameplayPage's AddOption order.
		ColorblindModeCombo = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("ColorblindModeCombo"));
		AddPillRow(GameplayPage, TEXT("ColorblindMode"), FText::FromString(TEXT("Colorblind")), { TEXT("Off"), TEXT("Deutan"), TEXT("Protan"), TEXT("Tritan") }, nullptr, ColorblindModeCombo, nullptr);
		ColorblindStrengthSlider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass(), TEXT("ColorblindStrengthSlider"));
		AddSliderRow(GameplayPage, TEXT("ColorblindStrength"), FText::FromString(TEXT("Strength")), ColorblindStrengthSlider, true);
	}

	// --- Footer: big Back / Apply, bottom-right ---
	UHorizontalBox* Footer = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Footer"));
	if (UVerticalBoxSlot* FooterSlot = JinzzaUI::AddSpaced(PanelStack, Footer, 18.f))
	{
		FooterSlot->SetHorizontalAlignment(HAlign_Right);
	}

	BackButton = JinzzaUI::MakeStickerButton(WidgetTree, TEXT("BackButton"), FText::FromString(TEXT("Back")), JinzzaUI::Sticker_Sky, 26.f);
	if (UHorizontalBoxSlot* BackSlot = Footer->AddChildToHorizontalBox(BackButton))
	{
		BackSlot->SetVerticalAlignment(VAlign_Center);
		BackSlot->SetPadding(FMargin(0.f, 0.f, 14.f, 0.f));
	}

	ApplyButton = JinzzaUI::MakeStickerButton(WidgetTree, TEXT("ApplyButton"), FText::FromString(TEXT("Apply")), JinzzaUI::Sticker_Yellow, 28.f, true);
	if (UHorizontalBoxSlot* ApplySlot = Footer->AddChildToHorizontalBox(ApplyButton))
	{
		ApplySlot->SetVerticalAlignment(VAlign_Center);
	}
}

void UjinzzaSettingsWidget::SelectSegment(int32 GroupIndex, int32 OptionIndex)
{
	if (!SegmentGroups.IsValidIndex(GroupIndex))
	{
		return;
	}

	auto WriteGroup = [this](int32 Group, int32 Option)
	{
		const FSegmentGroup& Segment = SegmentGroups[Group];
		if (USpinBox* Spin = Segment.Spin.Get())
		{
			Spin->SetValue(Segment.Values.IsValidIndex(Option) ? Segment.Values[Option] : static_cast<float>(Option));
		}
		if (UComboBoxString* Combo = Segment.Combo.Get())
		{
			Combo->SetSelectedIndex(Option);
		}
		if (UCheckBox* Check = Segment.Check.Get())
		{
			Check->SetIsChecked(Option == 1);
		}
	};

	WriteGroup(GroupIndex, OptionIndex);

	// The Quality preset sets every per-feature quality to the same level, like the engine's own
	// scalability presets do.
	if (GroupIndex == OverallQualityGroup)
	{
		for (const int32 DetailGroup : DetailQualityGroups)
		{
			WriteGroup(DetailGroup, OptionIndex);
		}
	}

	RefreshSegments();
}

void UjinzzaSettingsWidget::RefreshSegments()
{
	for (const FSegmentGroup& Segment : SegmentGroups)
	{
		int32 Current = INDEX_NONE;
		if (const USpinBox* Spin = Segment.Spin.Get())
		{
			if (Segment.Values.Num() > 0)
			{
				// Values-backed group (FPS cap): highlight the pill whose value matches, if any.
				for (int32 Index = 0; Index < Segment.Values.Num(); ++Index)
				{
					if (FMath::IsNearlyEqual(Spin->GetValue(), Segment.Values[Index], 0.5f))
					{
						Current = Index;
						break;
					}
				}
			}
			else
			{
				Current = FMath::RoundToInt(Spin->GetValue());
			}
		}
		else if (const UComboBoxString* Combo = Segment.Combo.Get())
		{
			Current = Combo->GetSelectedIndex();
		}
		else if (const UCheckBox* Check = Segment.Check.Get())
		{
			Current = Check->IsChecked() ? 1 : 0;
		}

		for (int32 Index = 0; Index < Segment.Buttons.Num(); ++Index)
		{
			JinzzaUI::SetStickerButtonSelected(Segment.Buttons[Index].Get(), SettingsTabAccent(), Index == Current);
		}
	}
}

void UjinzzaSettingsWidget::HandleSliderValueChanged(float Value)
{
	RefreshSliderReadouts();
}

void UjinzzaSettingsWidget::RefreshSliderReadouts()
{
	for (const FSliderReadout& Entry : SliderReadouts)
	{
		const USlider* Slider = Entry.Slider.Get();
		UTextBlock* Label = Entry.Label.Get();
		if (!Slider || !Label)
		{
			continue;
		}
		const float Value = Slider->GetValue();
		Label->SetText(FText::FromString(Entry.bPercent
			? FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Value * 100.f))
			: FString::Printf(TEXT("%.1fx"), Value)));
	}
}

void UjinzzaSettingsWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	BuildWidgetTree();

	if (GraphicsTabButton) GraphicsTabButton->OnClicked.AddDynamic(this, &UjinzzaSettingsWidget::OnTabGraphicsClicked);
	if (AudioTabButton) AudioTabButton->OnClicked.AddDynamic(this, &UjinzzaSettingsWidget::OnTabAudioClicked);
	if (ControlsTabButton) ControlsTabButton->OnClicked.AddDynamic(this, &UjinzzaSettingsWidget::OnTabControlsClicked);
	if (GameplayTabButton) GameplayTabButton->OnClicked.AddDynamic(this, &UjinzzaSettingsWidget::OnTabGameplayClicked);

	PopulateGraphicsPage();
	PopulateAudioPage();
	PopulateControlsPage();
	PopulateGameplayPage();

	// Sync the segmented pills and slider readouts with the values Populate* just loaded.
	RefreshSegments();
	RefreshSliderReadouts();
	for (const FSliderReadout& Entry : SliderReadouts)
	{
		if (USlider* Slider = Entry.Slider.Get())
		{
			Slider->OnValueChanged.AddDynamic(this, &UjinzzaSettingsWidget::HandleSliderValueChanged);
		}
	}

	SetActiveTab(Tab_Graphics);

	if (ApplyButton) ApplyButton->OnClicked.AddDynamic(this, &UjinzzaSettingsWidget::OnApplyClicked);
	if (BackButton) BackButton->OnClicked.AddDynamic(this, &UjinzzaSettingsWidget::OnBackClicked);

	if (JumpRebindButton) JumpRebindButton->OnClicked.AddDynamic(this, &UjinzzaSettingsWidget::OnRebindJumpClicked);
	if (SprintRebindButton) SprintRebindButton->OnClicked.AddDynamic(this, &UjinzzaSettingsWidget::OnRebindSprintClicked);
	if (InteractRebindButton) InteractRebindButton->OnClicked.AddDynamic(this, &UjinzzaSettingsWidget::OnRebindInteractClicked);
	if (PushToTalkRebindButton) PushToTalkRebindButton->OnClicked.AddDynamic(this, &UjinzzaSettingsWidget::OnRebindPushToTalkClicked);
}

void UjinzzaSettingsWidget::PopulateGraphicsPage()
{
	UjinzzaGameUserSettings* Settings = UjinzzaGameUserSettings::Get();
	if (!Settings)
	{
		return;
	}

	if (WindowModeCombo)
	{
		WindowModeCombo->AddOption(TEXT("Fullscreen"));
		WindowModeCombo->AddOption(TEXT("Windowed Fullscreen"));
		WindowModeCombo->AddOption(TEXT("Windowed"));

		FString CurrentWindowMode = TEXT("Fullscreen");
		switch (Settings->GetFullscreenMode())
		{
		case EWindowMode::Fullscreen: CurrentWindowMode = TEXT("Fullscreen"); break;
		case EWindowMode::WindowedFullscreen: CurrentWindowMode = TEXT("Windowed Fullscreen"); break;
		case EWindowMode::Windowed: CurrentWindowMode = TEXT("Windowed"); break;
		default: break;
		}
		WindowModeCombo->SetSelectedOption(CurrentWindowMode);
	}

	if (ResolutionCombo)
	{
		UKismetSystemLibrary::GetConvenientWindowedResolutions(AvailableResolutions);
		const FIntPoint CurrentResolution = Settings->GetScreenResolution();
		if (!AvailableResolutions.Contains(CurrentResolution))
		{
			AvailableResolutions.Insert(CurrentResolution, 0);
		}
		for (const FIntPoint& Res : AvailableResolutions)
		{
			ResolutionCombo->AddOption(ResolutionToString(Res));
		}
		ResolutionCombo->SetSelectedOption(ResolutionToString(CurrentResolution));
	}

	if (VSyncCheckBox)
	{
		VSyncCheckBox->SetIsChecked(Settings->IsVSyncEnabled());
	}

	if (FrameRateLimitSpinBox)
	{
		FrameRateLimitSpinBox->SetMinValue(0.f);
		FrameRateLimitSpinBox->SetMaxValue(300.f);
		FrameRateLimitSpinBox->SetMinSliderValue(0.f);
		FrameRateLimitSpinBox->SetMaxSliderValue(300.f);
		FrameRateLimitSpinBox->SetDelta(1.f);
		FrameRateLimitSpinBox->SetValue(Settings->GetFrameRateLimit());
	}

	if (OverallQualityCombo)
	{
		const TArray<FString> QualityPresets = { TEXT("Low"), TEXT("Medium"), TEXT("High"), TEXT("Epic"), TEXT("Cinematic") };
		for (const FString& Preset : QualityPresets)
		{
			OverallQualityCombo->AddOption(Preset);
		}
		const int32 OverallLevel = FMath::Clamp(Settings->GetOverallScalabilityLevel(), 0, QualityPresets.Num() - 1);
		OverallQualityCombo->SetSelectedOption(QualityPresets[OverallLevel]);
	}

	auto InitQualitySpinBox = [](USpinBox* SpinBox, int32 InitialValue)
	{
		if (!SpinBox)
		{
			return;
		}
		SpinBox->SetMinValue(0.f);
		SpinBox->SetMaxValue(4.f);
		SpinBox->SetMinSliderValue(0.f);
		SpinBox->SetMaxSliderValue(4.f);
		SpinBox->SetDelta(1.f);
		SpinBox->SetValue(static_cast<float>(InitialValue));
	};

	InitQualitySpinBox(ViewDistanceSpinBox, Settings->GetViewDistanceQuality());
	InitQualitySpinBox(ShadowSpinBox, Settings->GetShadowQuality());
	InitQualitySpinBox(GlobalIlluminationSpinBox, Settings->GetGlobalIlluminationQuality());
	InitQualitySpinBox(ReflectionSpinBox, Settings->GetReflectionQuality());
	InitQualitySpinBox(AntiAliasingSpinBox, Settings->GetAntiAliasingQuality());
	InitQualitySpinBox(TextureSpinBox, Settings->GetTextureQuality());
	InitQualitySpinBox(EffectsSpinBox, Settings->GetVisualEffectQuality());
	InitQualitySpinBox(FoliageSpinBox, Settings->GetFoliageQuality());
	InitQualitySpinBox(ShadingSpinBox, Settings->GetShadingQuality());
}

void UjinzzaSettingsWidget::PopulateAudioPage()
{
	UjinzzaGameUserSettings* Settings = UjinzzaGameUserSettings::Get();
	if (!Settings)
	{
		return;
	}

	if (MasterVolumeSlider)
	{
		MasterVolumeSlider->SetMinValue(0.f);
		MasterVolumeSlider->SetMaxValue(1.f);
		MasterVolumeSlider->SetValue(Settings->GetMasterVolume());
	}
	if (MusicVolumeSlider)
	{
		MusicVolumeSlider->SetMinValue(0.f);
		MusicVolumeSlider->SetMaxValue(1.f);
		MusicVolumeSlider->SetValue(Settings->GetMusicVolume());
	}
	if (SFXVolumeSlider)
	{
		SFXVolumeSlider->SetMinValue(0.f);
		SFXVolumeSlider->SetMaxValue(1.f);
		SFXVolumeSlider->SetValue(Settings->GetSFXVolume());
	}
	if (VoiceVolumeSlider)
	{
		VoiceVolumeSlider->SetMinValue(0.f);
		VoiceVolumeSlider->SetMaxValue(1.f);
		VoiceVolumeSlider->SetValue(Settings->GetVoiceVolume());
	}

	if (MicInputModeCombo)
	{
		MicInputModeCombo->AddOption(TEXT("Push to Talk"));
		MicInputModeCombo->AddOption(TEXT("Open Mic"));
		const TArray<FString> MicModes = { TEXT("Push to Talk"), TEXT("Open Mic") };
		MicInputModeCombo->SetSelectedOption(MicModes[static_cast<int32>(Settings->GetMicInputMode())]);
	}

	if (MicDeviceCombo)
	{
		AvailableMicDeviceIds = { FString() };
		MicDeviceCombo->AddOption(TEXT("System Default"));
		MicDeviceCombo->SetSelectedOption(TEXT("System Default"));

		// Device list is fetched async; OnAudioInputDevicesObtained repopulates the combo (and
		// restores the saved selection) once the platform responds, usually within a frame or two.
		FOnAudioInputDevicesObtained DevicesObtained;
		DevicesObtained.BindDynamic(this, &UjinzzaSettingsWidget::OnAudioInputDevicesObtained);
		UAudioCaptureBlueprintLibrary::GetAvailableAudioInputDevices(this, DevicesObtained);
	}
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

void UjinzzaSettingsWidget::PopulateControlsPage()
{
	UjinzzaGameUserSettings* Settings = UjinzzaGameUserSettings::Get();
	if (!Settings)
	{
		return;
	}

	if (MouseSensitivitySlider)
	{
		MouseSensitivitySlider->SetMinValue(0.1f);
		MouseSensitivitySlider->SetMaxValue(3.f);
		MouseSensitivitySlider->SetValue(Settings->GetMouseSensitivity());
	}
	if (InvertYCheckBox)
	{
		InvertYCheckBox->SetIsChecked(Settings->GetInvertYAxis());
	}

	RebindLabels.Reset();
	RebindLabels.Add(TEXT("IA_Jump"), JumpRebindLabel);
	RebindLabels.Add(TEXT("IA_Sprint"), SprintRebindLabel);
	RebindLabels.Add(TEXT("IA_Interact"), InteractRebindLabel);
	RebindLabels.Add(JinzzaInput::GetPushToTalkActionName(), PushToTalkRebindLabel);

	for (const TPair<FName, TObjectPtr<UTextBlock>>& Pair : RebindLabels)
	{
		RefreshRebindButtonLabel(Pair.Key);
	}
}

void UjinzzaSettingsWidget::PopulateGameplayPage()
{
	UjinzzaGameUserSettings* Settings = UjinzzaGameUserSettings::Get();
	if (!Settings)
	{
		return;
	}

	if (SubtitlesCheckBox)
	{
		SubtitlesCheckBox->SetIsChecked(Settings->GetSubtitlesEnabled());
	}

	if (ColorblindModeCombo)
	{
		const TArray<FString> ColorblindOptions = { TEXT("Off"), TEXT("Deuteranope"), TEXT("Protanope"), TEXT("Tritanope") };
		for (const FString& Option : ColorblindOptions)
		{
			ColorblindModeCombo->AddOption(Option);
		}
		ColorblindModeCombo->SetSelectedOption(ColorblindOptions[static_cast<int32>(Settings->GetColorblindMode())]);
	}

	if (ColorblindStrengthSlider)
	{
		ColorblindStrengthSlider->SetMinValue(0.f);
		ColorblindStrengthSlider->SetMaxValue(1.f);
		ColorblindStrengthSlider->SetValue(Settings->GetColorblindStrength());
	}
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
			if (Key.IsValid())
			{
				(*Label)->SetText(Key.GetDisplayName());
			}
			else if (ActionName == JinzzaInput::GetPushToTalkActionName())
			{
				// Push-to-talk's default lives in code, not in an IMC asset - worth spelling out.
				(*Label)->SetText(FText::Format(FText::FromString(TEXT("{0} (Default)")), JinzzaInput::GetKeyCapText(JinzzaInput::GetDefaultPushToTalkKey())));
			}
			else
			{
				(*Label)->SetText(FText::FromString(TEXT("Default")));
			}
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

		// Live right away - key prompts ([E] Pick Up, kiosks) read the live mappings.
		if (AjinzzaPlayerController* PC = Cast<AjinzzaPlayerController>(GetOwningPlayer()))
		{
			PC->RefreshKeyBindings();
		}

		RefreshRebindButtonLabel(PendingRebindAction);
		bWaitingForRebind = false;
		PendingRebindAction = NAME_None;
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UjinzzaSettingsWidget::OnRebindJumpClicked() { StartRebind(TEXT("IA_Jump")); }
void UjinzzaSettingsWidget::OnRebindSprintClicked() { StartRebind(TEXT("IA_Sprint")); }
void UjinzzaSettingsWidget::OnRebindInteractClicked() { StartRebind(TEXT("IA_Interact")); }
void UjinzzaSettingsWidget::OnRebindPushToTalkClicked() { StartRebind(JinzzaInput::GetPushToTalkActionName()); }

void UjinzzaSettingsWidget::SetActiveTab(int32 TabIndex)
{
	if (TabSwitcher)
	{
		TabSwitcher->SetActiveWidgetIndex(TabIndex);
	}

	UWidget* Accents[] = { GraphicsTabAccent, AudioTabAccent, ControlsTabAccent, GameplayTabAccent };
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Accents); ++Index)
	{
		if (Accents[Index])
		{
			Accents[Index]->SetVisibility(Index == TabIndex ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
		}
	}

	// The active tab's sticker pill is filled yellow, the rest plain black.
	UButton* TabButtons[] = { GraphicsTabButton, AudioTabButton, ControlsTabButton, GameplayTabButton };
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(TabButtons); ++Index)
	{
		JinzzaUI::SetStickerButtonSelected(TabButtons[Index], SettingsTabAccent(), Index == TabIndex);
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

	if (AjinzzaPlayerController* PC = Cast<AjinzzaPlayerController>(GetOwningPlayer()))
	{
		PC->ApplyMicInputMode();
	}
}
