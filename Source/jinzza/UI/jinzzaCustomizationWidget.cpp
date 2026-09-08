// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaCustomizationWidget.h"
#include "jinzzaGameUserSettings.h"
#include "jinzzaCharacterCustomizationComponent.h"
#include "jinzzaUIStyle.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/Border.h"
#include "Blueprint/WidgetTree.h"
#include "GameFramework/Pawn.h"

namespace
{
	constexpr int32 NumStyles = 3; // EJinzzaCustomizationStyle::StyleA/B/C
	constexpr int32 NumHairColors = 5; // EJinzzaHairColor::Black/Brown/Blonde/Red/Blue

	template <typename EnumType>
	EnumType CycleEnum(EnumType Current, int32 Delta, int32 Count)
	{
		const int32 Value = (static_cast<int32>(Current) + Delta + Count) % Count;
		return static_cast<EnumType>(Value);
	}
}

void UjinzzaCustomizationWidget::BuildWidgetTree()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
	WidgetTree->RootWidget = Root;

	UBorder* Panel = JinzzaUI::MakePanelBackground(WidgetTree, TEXT("Panel"));
	Panel->SetPadding(FMargin(24.f));
	if (UOverlaySlot* PanelSlot = Root->AddChildToOverlay(Panel))
	{
		PanelSlot->SetHorizontalAlignment(HAlign_Center);
		PanelSlot->SetVerticalAlignment(VAlign_Center);
	}

	USizeBox* PanelBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PanelBox"));
	PanelBox->SetWidthOverride(460.f);
	Panel->SetContent(PanelBox);

	UVerticalBox* Stack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Stack"));
	PanelBox->AddChild(Stack);

	JinzzaUI::AddSpaced(Stack, JinzzaUI::MakeSectionHeading(WidgetTree, TEXT("Heading"), FText::FromString(TEXT("Customization"))), 0.f);
	JinzzaUI::AddSpaced(Stack, JinzzaUI::MakeDivider(WidgetTree, TEXT("HeaderDivider")));

	// Builds one Prev/Value/Next row (optionally with a color swatch, for Hair Color) and adds it
	// to Stack as a labeled row. Writes straight into the member Prev/Value/Next pointers passed
	// by reference so this mirrors exactly what a hand-authored WBP would bind.
	auto MakeCycleRow = [this, Stack](const TCHAR* NamePrefix, const FText& RowLabel,
		TObjectPtr<UButton>& OutPrev, TObjectPtr<UTextBlock>& OutValue, TObjectPtr<UButton>& OutNext, UImage** OutSwatch = nullptr)
	{
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), *(FString(NamePrefix) + TEXT("_Row")));

		OutPrev = JinzzaUI::MakeSecondaryButton(WidgetTree, *(FString(NamePrefix) + TEXT("_Prev")), FText::FromString(TEXT("<")), 16.f);
		if (UHorizontalBoxSlot* PrevSlot = Row->AddChildToHorizontalBox(OutPrev))
		{
			PrevSlot->SetVerticalAlignment(VAlign_Center);
			PrevSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
		}

		OutValue = JinzzaUI::MakeBodyText(WidgetTree, *(FString(NamePrefix) + TEXT("_Value")), FText::GetEmpty());
		OutValue->SetJustification(ETextJustify::Center);
		if (UHorizontalBoxSlot* ValueSlot = Row->AddChildToHorizontalBox(OutValue))
		{
			ValueSlot->SetVerticalAlignment(VAlign_Center);
			ValueSlot->SetHorizontalAlignment(HAlign_Center);
			ValueSlot->SetSize(ESlateSizeRule::Fill);
		}

		if (OutSwatch)
		{
			UImage* Swatch = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), *(FString(NamePrefix) + TEXT("_Swatch")));
			USizeBox* SwatchBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), *(FString(NamePrefix) + TEXT("_SwatchBox")));
			SwatchBox->SetWidthOverride(20.f);
			SwatchBox->SetHeightOverride(20.f);
			SwatchBox->AddChild(Swatch);
			if (UHorizontalBoxSlot* SwatchSlot = Row->AddChildToHorizontalBox(SwatchBox))
			{
				SwatchSlot->SetVerticalAlignment(VAlign_Center);
				SwatchSlot->SetPadding(FMargin(8.f, 0.f, 8.f, 0.f));
			}
			*OutSwatch = Swatch;
		}

		OutNext = JinzzaUI::MakeSecondaryButton(WidgetTree, *(FString(NamePrefix) + TEXT("_Next")), FText::FromString(TEXT(">")), 16.f);
		if (UHorizontalBoxSlot* NextSlot = Row->AddChildToHorizontalBox(OutNext))
		{
			NextSlot->SetVerticalAlignment(VAlign_Center);
			NextSlot->SetPadding(FMargin(8.f, 0.f, 0.f, 0.f));
		}

		JinzzaUI::AddSpaced(Stack, JinzzaUI::MakeLabeledRow(WidgetTree, *(FString(NamePrefix) + TEXT("_LabeledRow")), RowLabel, Row));
	};

	MakeCycleRow(TEXT("Head"), FText::FromString(TEXT("Head")), HeadPrevButton, HeadValueText, HeadNextButton);

	UImage* HairSwatch = nullptr;
	MakeCycleRow(TEXT("HairColor"), FText::FromString(TEXT("Hair Color")), HairColorPrevButton, HairColorValueText, HairColorNextButton, &HairSwatch);
	HairColorSwatch = HairSwatch;

	MakeCycleRow(TEXT("Top"), FText::FromString(TEXT("Top")), TopPrevButton, TopValueText, TopNextButton);
	MakeCycleRow(TEXT("Eyebrows"), FText::FromString(TEXT("Eyebrows")), EyebrowsPrevButton, EyebrowsValueText, EyebrowsNextButton);
	MakeCycleRow(TEXT("Eyes"), FText::FromString(TEXT("Eyes")), EyesPrevButton, EyesValueText, EyesNextButton);

	UHorizontalBox* ButtonRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ButtonRow"));
	if (UVerticalBoxSlot* ButtonRowSlot = JinzzaUI::AddSpaced(Stack, ButtonRow, 20.f))
	{
		ButtonRowSlot->SetHorizontalAlignment(HAlign_Right);
	}

	DoneButton = JinzzaUI::MakePrimaryButton(WidgetTree, TEXT("DoneButton"), FText::FromString(TEXT("Done")));
	ButtonRow->AddChildToHorizontalBox(DoneButton);
}

void UjinzzaCustomizationWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	BuildWidgetTree();

	if (HeadPrevButton) HeadPrevButton->OnClicked.AddDynamic(this, &UjinzzaCustomizationWidget::OnHeadPrevClicked);
	if (HeadNextButton) HeadNextButton->OnClicked.AddDynamic(this, &UjinzzaCustomizationWidget::OnHeadNextClicked);
	if (HairColorPrevButton) HairColorPrevButton->OnClicked.AddDynamic(this, &UjinzzaCustomizationWidget::OnHairColorPrevClicked);
	if (HairColorNextButton) HairColorNextButton->OnClicked.AddDynamic(this, &UjinzzaCustomizationWidget::OnHairColorNextClicked);
	if (TopPrevButton) TopPrevButton->OnClicked.AddDynamic(this, &UjinzzaCustomizationWidget::OnTopPrevClicked);
	if (TopNextButton) TopNextButton->OnClicked.AddDynamic(this, &UjinzzaCustomizationWidget::OnTopNextClicked);
	if (EyebrowsPrevButton) EyebrowsPrevButton->OnClicked.AddDynamic(this, &UjinzzaCustomizationWidget::OnEyebrowsPrevClicked);
	if (EyebrowsNextButton) EyebrowsNextButton->OnClicked.AddDynamic(this, &UjinzzaCustomizationWidget::OnEyebrowsNextClicked);
	if (EyesPrevButton) EyesPrevButton->OnClicked.AddDynamic(this, &UjinzzaCustomizationWidget::OnEyesPrevClicked);
	if (EyesNextButton) EyesNextButton->OnClicked.AddDynamic(this, &UjinzzaCustomizationWidget::OnEyesNextClicked);
	if (DoneButton) DoneButton->OnClicked.AddDynamic(this, &UjinzzaCustomizationWidget::OnDoneClicked);

	RefreshAllRows();
}

void UjinzzaCustomizationWidget::OnHeadPrevClicked()
{
	if (UjinzzaGameUserSettings* Settings = UjinzzaGameUserSettings::Get())
	{
		Settings->SetHeadStyle(CycleEnum(Settings->GetHeadStyle(), -1, NumStyles));
		CommitChange();
	}
}

void UjinzzaCustomizationWidget::OnHeadNextClicked()
{
	if (UjinzzaGameUserSettings* Settings = UjinzzaGameUserSettings::Get())
	{
		Settings->SetHeadStyle(CycleEnum(Settings->GetHeadStyle(), 1, NumStyles));
		CommitChange();
	}
}

void UjinzzaCustomizationWidget::OnHairColorPrevClicked()
{
	if (UjinzzaGameUserSettings* Settings = UjinzzaGameUserSettings::Get())
	{
		Settings->SetHairColor(CycleEnum(Settings->GetHairColor(), -1, NumHairColors));
		CommitChange();
	}
}

void UjinzzaCustomizationWidget::OnHairColorNextClicked()
{
	if (UjinzzaGameUserSettings* Settings = UjinzzaGameUserSettings::Get())
	{
		Settings->SetHairColor(CycleEnum(Settings->GetHairColor(), 1, NumHairColors));
		CommitChange();
	}
}

void UjinzzaCustomizationWidget::OnTopPrevClicked()
{
	if (UjinzzaGameUserSettings* Settings = UjinzzaGameUserSettings::Get())
	{
		Settings->SetTopStyle(CycleEnum(Settings->GetTopStyle(), -1, NumStyles));
		CommitChange();
	}
}

void UjinzzaCustomizationWidget::OnTopNextClicked()
{
	if (UjinzzaGameUserSettings* Settings = UjinzzaGameUserSettings::Get())
	{
		Settings->SetTopStyle(CycleEnum(Settings->GetTopStyle(), 1, NumStyles));
		CommitChange();
	}
}

void UjinzzaCustomizationWidget::OnEyebrowsPrevClicked()
{
	if (UjinzzaGameUserSettings* Settings = UjinzzaGameUserSettings::Get())
	{
		Settings->SetEyebrowsStyle(CycleEnum(Settings->GetEyebrowsStyle(), -1, NumStyles));
		CommitChange();
	}
}

void UjinzzaCustomizationWidget::OnEyebrowsNextClicked()
{
	if (UjinzzaGameUserSettings* Settings = UjinzzaGameUserSettings::Get())
	{
		Settings->SetEyebrowsStyle(CycleEnum(Settings->GetEyebrowsStyle(), 1, NumStyles));
		CommitChange();
	}
}

void UjinzzaCustomizationWidget::OnEyesPrevClicked()
{
	if (UjinzzaGameUserSettings* Settings = UjinzzaGameUserSettings::Get())
	{
		Settings->SetEyesStyle(CycleEnum(Settings->GetEyesStyle(), -1, NumStyles));
		CommitChange();
	}
}

void UjinzzaCustomizationWidget::OnEyesNextClicked()
{
	if (UjinzzaGameUserSettings* Settings = UjinzzaGameUserSettings::Get())
	{
		Settings->SetEyesStyle(CycleEnum(Settings->GetEyesStyle(), 1, NumStyles));
		CommitChange();
	}
}

void UjinzzaCustomizationWidget::OnDoneClicked()
{
	if (UjinzzaGameUserSettings* Settings = UjinzzaGameUserSettings::Get())
	{
		Settings->SaveSettings();
	}
	OnBackRequested.Broadcast();
}

void UjinzzaCustomizationWidget::CommitChange()
{
	RefreshAllRows();

	// Live feedback if a pawn already exists (the Lobby case) - a no-op if not (the main menu
	// case, before any session/pawn exists). Saved to disk only on Done, like UjinzzaSettingsWidget.
	if (APawn* LocalPawn = GetOwningPlayerPawn())
	{
		if (UjinzzaCharacterCustomizationComponent* Customization = LocalPawn->FindComponentByClass<UjinzzaCharacterCustomizationComponent>())
		{
			Customization->RefreshCustomization();
		}
	}
}

void UjinzzaCustomizationWidget::RefreshAllRows()
{
	UjinzzaGameUserSettings* Settings = UjinzzaGameUserSettings::Get();
	if (!Settings)
	{
		return;
	}

	if (HeadValueText) HeadValueText->SetText(GetStyleDisplayName(Settings->GetHeadStyle()));
	if (TopValueText) TopValueText->SetText(GetStyleDisplayName(Settings->GetTopStyle()));
	if (EyebrowsValueText) EyebrowsValueText->SetText(GetStyleDisplayName(Settings->GetEyebrowsStyle()));
	if (EyesValueText) EyesValueText->SetText(GetStyleDisplayName(Settings->GetEyesStyle()));

	const EJinzzaHairColor HairColor = Settings->GetHairColor();
	if (HairColorValueText) HairColorValueText->SetText(GetHairColorDisplayName(HairColor));
	if (HairColorSwatch) HairColorSwatch->SetColorAndOpacity(GetHairColorSwatchColor(HairColor));
}

FText UjinzzaCustomizationWidget::GetStyleDisplayName(EJinzzaCustomizationStyle Style)
{
	switch (Style)
	{
	case EJinzzaCustomizationStyle::StyleB: return FText::FromString(TEXT("Style B"));
	case EJinzzaCustomizationStyle::StyleC: return FText::FromString(TEXT("Style C"));
	default:                                return FText::FromString(TEXT("Style A"));
	}
}

FText UjinzzaCustomizationWidget::GetHairColorDisplayName(EJinzzaHairColor Color)
{
	switch (Color)
	{
	case EJinzzaHairColor::Brown:  return FText::FromString(TEXT("Brown"));
	case EJinzzaHairColor::Blonde: return FText::FromString(TEXT("Blonde"));
	case EJinzzaHairColor::Red:    return FText::FromString(TEXT("Red"));
	case EJinzzaHairColor::Blue:   return FText::FromString(TEXT("Blue"));
	default:                       return FText::FromString(TEXT("Black"));
	}
}

FLinearColor UjinzzaCustomizationWidget::GetHairColorSwatchColor(EJinzzaHairColor Color)
{
	switch (Color)
	{
	case EJinzzaHairColor::Brown:  return FLinearColor(0.36f, 0.20f, 0.09f);
	case EJinzzaHairColor::Blonde: return FLinearColor(0.85f, 0.70f, 0.35f);
	case EJinzzaHairColor::Red:    return FLinearColor(0.55f, 0.11f, 0.06f);
	case EJinzzaHairColor::Blue:   return FLinearColor(0.10f, 0.30f, 0.75f);
	default:                       return FLinearColor(0.03f, 0.03f, 0.03f);
	}
}
