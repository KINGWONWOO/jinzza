// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaCustomizationWidget.h"
#include "jinzzaGameUserSettings.h"
#include "jinzzaCharacterCustomizationComponent.h"
#include "jinzzaCharacterPreviewCapture.h"
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
#include "Components/WidgetSwitcher.h"
#include "Components/SizeBox.h"
#include "Components/Border.h"
#include "Blueprint/WidgetTree.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Styling/SlateBrush.h"

namespace
{
	constexpr int32 NumStyles = 3; // EJinzzaCustomizationStyle::StyleA/B/C
	constexpr int32 NumHairColors = 5; // EJinzzaHairColor::Black/Brown/Blonde/Red/Blue
	constexpr int32 NumAccessoryStyles = 4; // EJinzzaAccessoryStyle::None/StyleA/B/C

	// Named distinctly from jinzzaSettingsWidget.cpp's own anonymous-namespace ETabPage - two
	// same-named enums in anonymous namespaces would collide the moment unity build happens to
	// group both files into one translation unit (see [[unreal-mcp-gotchas]] #8, which bit this
	// exact pattern before with a duplicated AddSpaced helper).
	enum ECustomizationTabPage : int32
	{
		Tab_Head = 0,
		Tab_Clothes = 1,
		Tab_Accessories = 2,
		Tab_Colors = 3,
	};

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
	PanelBox->SetWidthOverride(820.f);
	Panel->SetContent(PanelBox);

	UVerticalBox* OuterStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("OuterStack"));
	PanelBox->AddChild(OuterStack);

	JinzzaUI::AddSpaced(OuterStack, JinzzaUI::MakeSectionHeading(WidgetTree, TEXT("Heading"), FText::FromString(TEXT("Customization"))), 0.f);
	JinzzaUI::AddSpaced(OuterStack, JinzzaUI::MakeDivider(WidgetTree, TEXT("HeaderDivider")));

	UHorizontalBox* SplitRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("SplitRow"));
	JinzzaUI::AddSpaced(OuterStack, SplitRow, 16.f);

	// --- Left: live character preview (AjinzzaCharacterPreviewCapture, spawned on demand - see RefreshCharacterPreview) ---
	USizeBox* PreviewBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PreviewBox"));
	PreviewBox->SetWidthOverride(260.f);
	PreviewBox->SetHeightOverride(390.f);
	if (UHorizontalBoxSlot* PreviewSlot = SplitRow->AddChildToHorizontalBox(PreviewBox))
	{
		PreviewSlot->SetVerticalAlignment(VAlign_Top);
		PreviewSlot->SetPadding(FMargin(0.f, 0.f, 20.f, 0.f));
	}

	UBorder* PreviewFrame = JinzzaUI::MakePanelBackground(WidgetTree, TEXT("PreviewFrame"));
	PreviewBox->AddChild(PreviewFrame);

	CharacterPreviewImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("CharacterPreviewImage"));
	PreviewFrame->SetContent(CharacterPreviewImage);

	// --- Right: tab bar + a page per category ---
	UVerticalBox* RightStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RightStack"));
	if (UHorizontalBoxSlot* RightSlot = SplitRow->AddChildToHorizontalBox(RightStack))
	{
		RightSlot->SetSize(ESlateSizeRule::Fill);
	}

	UHorizontalBox* TabRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("TabRow"));
	JinzzaUI::AddSpaced(RightStack, TabRow, 0.f);

	auto AddTabButton = [this, TabRow](const TCHAR* Name, const FText& Label) -> UButton*
	{
		UButton* TabButton = JinzzaUI::MakeSecondaryButton(WidgetTree, Name, Label, 16.f);
		if (UHorizontalBoxSlot* TabSlot = TabRow->AddChildToHorizontalBox(TabButton))
		{
			TabSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
		}
		return TabButton;
	};

	HeadTabButton = AddTabButton(TEXT("HeadTabButton"), FText::FromString(TEXT("Head")));
	ClothesTabButton = AddTabButton(TEXT("ClothesTabButton"), FText::FromString(TEXT("Clothes")));
	AccessoriesTabButton = AddTabButton(TEXT("AccessoriesTabButton"), FText::FromString(TEXT("Accessories")));
	ColorsTabButton = AddTabButton(TEXT("ColorsTabButton"), FText::FromString(TEXT("Colors")));

	TabSwitcher = WidgetTree->ConstructWidget<UWidgetSwitcher>(UWidgetSwitcher::StaticClass(), TEXT("TabSwitcher"));
	JinzzaUI::AddSpaced(RightStack, TabSwitcher, 12.f);

	// Builds one Prev/Value/Next row (optionally with a color swatch, for Hair Color) and adds it
	// to the given tab page. Writes straight into the member Prev/Value/Next pointers passed by
	// reference so this mirrors exactly what a hand-authored WBP would bind.
	auto MakeCycleRow = [this](UVerticalBox* Page, const TCHAR* NamePrefix, const FText& RowLabel,
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

		JinzzaUI::AddSpaced(Page, JinzzaUI::MakeLabeledRow(WidgetTree, *(FString(NamePrefix) + TEXT("_LabeledRow")), RowLabel, Row));
	};

	auto MakeTabPage = [this](const TCHAR* Name) -> UVerticalBox*
	{
		UVerticalBox* Page = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), Name);
		TabSwitcher->AddChild(Page);
		return Page;
	};

	UVerticalBox* HeadPage = MakeTabPage(TEXT("HeadPage"));
	MakeCycleRow(HeadPage, TEXT("Head"), FText::FromString(TEXT("Head")), HeadPrevButton, HeadValueText, HeadNextButton);
	MakeCycleRow(HeadPage, TEXT("Eyebrows"), FText::FromString(TEXT("Eyebrows")), EyebrowsPrevButton, EyebrowsValueText, EyebrowsNextButton);
	MakeCycleRow(HeadPage, TEXT("Eyes"), FText::FromString(TEXT("Eyes")), EyesPrevButton, EyesValueText, EyesNextButton);

	UVerticalBox* ClothesPage = MakeTabPage(TEXT("ClothesPage"));
	MakeCycleRow(ClothesPage, TEXT("Top"), FText::FromString(TEXT("Top")), TopPrevButton, TopValueText, TopNextButton);

	UVerticalBox* AccessoriesPage = MakeTabPage(TEXT("AccessoriesPage"));
	MakeCycleRow(AccessoriesPage, TEXT("Accessory"), FText::FromString(TEXT("Accessory")), AccessoryPrevButton, AccessoryValueText, AccessoryNextButton);

	UVerticalBox* ColorsPage = MakeTabPage(TEXT("ColorsPage"));
	UImage* HairSwatch = nullptr;
	MakeCycleRow(ColorsPage, TEXT("HairColor"), FText::FromString(TEXT("Hair Color")), HairColorPrevButton, HairColorValueText, HairColorNextButton, &HairSwatch);
	HairColorSwatch = HairSwatch;

	UHorizontalBox* ButtonRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ButtonRow"));
	if (UVerticalBoxSlot* ButtonRowSlot = JinzzaUI::AddSpaced(RightStack, ButtonRow, 20.f))
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
	if (AccessoryPrevButton) AccessoryPrevButton->OnClicked.AddDynamic(this, &UjinzzaCustomizationWidget::OnAccessoryPrevClicked);
	if (AccessoryNextButton) AccessoryNextButton->OnClicked.AddDynamic(this, &UjinzzaCustomizationWidget::OnAccessoryNextClicked);
	if (DoneButton) DoneButton->OnClicked.AddDynamic(this, &UjinzzaCustomizationWidget::OnDoneClicked);

	if (HeadTabButton) HeadTabButton->OnClicked.AddDynamic(this, &UjinzzaCustomizationWidget::OnHeadTabClicked);
	if (ClothesTabButton) ClothesTabButton->OnClicked.AddDynamic(this, &UjinzzaCustomizationWidget::OnClothesTabClicked);
	if (AccessoriesTabButton) AccessoriesTabButton->OnClicked.AddDynamic(this, &UjinzzaCustomizationWidget::OnAccessoriesTabClicked);
	if (ColorsTabButton) ColorsTabButton->OnClicked.AddDynamic(this, &UjinzzaCustomizationWidget::OnColorsTabClicked);

	ShowTab(Tab_Head);
	RefreshAllRows();
	RefreshCharacterPreview();
}

void UjinzzaCustomizationWidget::NativeDestruct()
{
	if (PreviewCapture)
	{
		PreviewCapture->Destroy();
		PreviewCapture = nullptr;
	}

	Super::NativeDestruct();
}

void UjinzzaCustomizationWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bCharacterPreviewWired)
	{
		RefreshCharacterPreview();
	}
}

void UjinzzaCustomizationWidget::ShowTab(int32 TabIndex)
{
	if (TabSwitcher)
	{
		TabSwitcher->SetActiveWidgetIndex(TabIndex);
	}

	// Cheap "you are here" indicator until a real visual design pass replaces these placeholder
	// tab buttons - the active tab's button is disabled, the rest re-enabled.
	if (HeadTabButton) HeadTabButton->SetIsEnabled(TabIndex != Tab_Head);
	if (ClothesTabButton) ClothesTabButton->SetIsEnabled(TabIndex != Tab_Clothes);
	if (AccessoriesTabButton) AccessoriesTabButton->SetIsEnabled(TabIndex != Tab_Accessories);
	if (ColorsTabButton) ColorsTabButton->SetIsEnabled(TabIndex != Tab_Colors);
}

void UjinzzaCustomizationWidget::RefreshCharacterPreview()
{
	if (!CharacterPreviewImage)
	{
		return;
	}

	if (!PreviewCapture)
	{
		if (UWorld* World = GetWorld())
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			// Spawned far below any level geometry - the capture renders independently of where
			// it sits in the world, so this just keeps it out of everyone else's way. Spawning our
			// own instance (rather than requiring one hand-placed per level) is what lets this
			// preview work identically whether the Customization screen is opened from the main
			// menu or popped up from AjinzzaWardrobeKiosk in the lobby.
			PreviewCapture = World->SpawnActor<AjinzzaCharacterPreviewCapture>(FVector(0.f, 0.f, -6000.f), FRotator::ZeroRotator, SpawnParams);
		}
	}

	if (!PreviewCapture)
	{
		return;
	}

	if (UTextureRenderTarget2D* RT = PreviewCapture->GetRenderTarget())
	{
		// SetBrushFromTexture only accepts UTexture2D specifically - UTextureRenderTarget2D is a
		// sibling (both derive from UTexture), so the brush needs setting up directly.
		FSlateBrush Brush;
		Brush.SetResourceObject(RT);
		Brush.ImageSize = FVector2D(RT->SizeX, RT->SizeY);
		CharacterPreviewImage->SetBrush(Brush);
		bCharacterPreviewWired = true;
	}

	PreviewCapture->RefreshAppearance();
}

void UjinzzaCustomizationWidget::OnHeadTabClicked() { ShowTab(Tab_Head); }
void UjinzzaCustomizationWidget::OnClothesTabClicked() { ShowTab(Tab_Clothes); }
void UjinzzaCustomizationWidget::OnAccessoriesTabClicked() { ShowTab(Tab_Accessories); }
void UjinzzaCustomizationWidget::OnColorsTabClicked() { ShowTab(Tab_Colors); }

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

void UjinzzaCustomizationWidget::OnAccessoryPrevClicked()
{
	if (UjinzzaGameUserSettings* Settings = UjinzzaGameUserSettings::Get())
	{
		Settings->SetAccessoryStyle(CycleEnum(Settings->GetAccessoryStyle(), -1, NumAccessoryStyles));
		CommitChange();
	}
}

void UjinzzaCustomizationWidget::OnAccessoryNextClicked()
{
	if (UjinzzaGameUserSettings* Settings = UjinzzaGameUserSettings::Get())
	{
		Settings->SetAccessoryStyle(CycleEnum(Settings->GetAccessoryStyle(), 1, NumAccessoryStyles));
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
	RefreshCharacterPreview();

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
	if (AccessoryValueText) AccessoryValueText->SetText(GetAccessoryStyleDisplayName(Settings->GetAccessoryStyle()));

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

FText UjinzzaCustomizationWidget::GetAccessoryStyleDisplayName(EJinzzaAccessoryStyle Style)
{
	switch (Style)
	{
	case EJinzzaAccessoryStyle::StyleA: return FText::FromString(TEXT("Style A"));
	case EJinzzaAccessoryStyle::StyleB: return FText::FromString(TEXT("Style B"));
	case EJinzzaAccessoryStyle::StyleC: return FText::FromString(TEXT("Style C"));
	default:                            return FText::FromString(TEXT("None"));
	}
}
