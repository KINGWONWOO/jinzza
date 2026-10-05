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
#include "Engine/Texture2D.h"
#include "Components/WidgetSwitcher.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Brushes/SlateRoundedBoxBrush.h"
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

	// Backdrop tint per page: fully clear on the button page so the 3D menu scene (the seal - see
	// AjinzzaMenuBackgroundCharacter) shows through, dimmed behind the popup pages so their
	// floating panels stay readable over it.
	const FLinearColor BackdropClear(0.f, 0.f, 0.f, 0.f);
	const FLinearColor BackdropDimmed(0.035f, 0.030f, 0.045f, 0.82f);

	// Sticker palette/constants and MakeSticker live in JinzzaUI (jinzzaUIStyle.h) - shared with the popup pages.

	enum class EStickerIcon : uint8
	{
		Plus,
		Lines,
		Cross,
	};

	/** A rounded bar (for the badge icons), optionally rotated. */
	UWidget* MakeStickerBar(UWidgetTree* Tree, const FString& Name, float Width, float Height, const FLinearColor& Color, float Angle = 0.f)
	{
		USizeBox* Box = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), *Name);
		Box->SetWidthOverride(Width);
		Box->SetHeightOverride(Height);
		Box->SetRenderTransformAngle(Angle);
		UBorder* Bar = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), *(Name + TEXT("_Shape")));
		Bar->SetBrush(FSlateRoundedBoxBrush(Color, Height * 0.5f));
		Box->AddChild(Bar);
		return Box;
	}

	/** White round badge with a simple black icon drawn from bars - the "face" of each menu entry. */
	UWidget* MakeStickerBadge(UWidgetTree* Tree, FName Name, EStickerIcon Icon, float Size)
	{
		const FString Base = Name.ToString();
		USizeBox* Box = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), Name);
		Box->SetWidthOverride(Size);
		Box->SetHeightOverride(Size);

		UBorder* Circle = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), *(Base + TEXT("_Circle")));
		Circle->SetBrush(FSlateRoundedBoxBrush(JinzzaUI::Sticker_White, Size * 0.5f));
		Circle->SetHorizontalAlignment(HAlign_Center);
		Circle->SetVerticalAlignment(VAlign_Center);
		Box->AddChild(Circle);

		const float Long = Size * 0.46f;
		const float Thick = FMath::Max(4.f, Size * 0.12f);
		if (Icon == EStickerIcon::Lines)
		{
			UVerticalBox* Lines = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), *(Base + TEXT("_Lines")));
			for (int32 Index = 0; Index < 3; ++Index)
			{
				UWidget* Bar = MakeStickerBar(Tree, Base + FString::Printf(TEXT("_Line%d"), Index), Long, Thick * 0.85f, JinzzaUI::Sticker_Ink);
				if (UVerticalBoxSlot* BarSlot = Lines->AddChildToVerticalBox(Bar))
				{
					BarSlot->SetPadding(FMargin(0.f, Index == 0 ? 0.f : Thick * 0.55f, 0.f, 0.f));
				}
			}
			Circle->SetContent(Lines);
		}
		else
		{
			const float Angle = (Icon == EStickerIcon::Cross) ? 45.f : 0.f;
			UOverlay* Glyph = Tree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), *(Base + TEXT("_Glyph")));
			for (int32 Index = 0; Index < 2; ++Index)
			{
				const bool bVertical = Index == 1;
				UWidget* Bar = MakeStickerBar(Tree, Base + FString::Printf(TEXT("_Bar%d"), Index),
					bVertical ? Thick : Long, bVertical ? Long : Thick, JinzzaUI::Sticker_Ink, Angle);
				if (UOverlaySlot* BarSlot = Glyph->AddChildToOverlay(Bar))
				{
					BarSlot->SetHorizontalAlignment(HAlign_Center);
					BarSlot->SetVerticalAlignment(VAlign_Center);
				}
			}
			Circle->SetContent(Glyph);
		}
		return Box;
	}

	UButton* MakeMainMenuInvisibleButton(UWidgetTree* Tree, FName Name)
	{
		FSlateBrush Transparent;
		Transparent.DrawAs = ESlateBrushDrawType::NoDrawType;
		FButtonStyle Style;
		Style.Normal = Transparent;
		Style.Hovered = Transparent;
		Style.Pressed = Transparent;
		Style.Disabled = Transparent;
		Style.NormalPadding = FMargin(0.f);
		Style.PressedPadding = FMargin(0.f);

		UButton* Button = Tree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
		Button->SetStyle(Style);

		UJinzzaUIButtonSounds* SoundBinder = NewObject<UJinzzaUIButtonSounds>(Button);
		Button->OnClicked.AddDynamic(SoundBinder, &UJinzzaUIButtonSounds::HandleClicked);
		Button->OnHovered.AddDynamic(SoundBinder, &UJinzzaUIButtonSounds::HandleHovered);
		return Button;
	}

	UJinzzaMenuStickerFx* BindStickerFx(UObject* Owner, UButton* Button, UBorder* Face, const FLinearColor& NormalFill, const FLinearColor& HoverFill, float Radius)
	{
		UJinzzaMenuStickerFx* Fx = NewObject<UJinzzaMenuStickerFx>(Owner);
		Fx->Bind(Button, Face,
			FSlateRoundedBoxBrush(NormalFill, Radius, JinzzaUI::Sticker_White, JinzzaUI::StickerOutline),
			FSlateRoundedBoxBrush(HoverFill, Radius, JinzzaUI::Sticker_White, JinzzaUI::StickerOutline));
		return Fx;
	}

	/**
	 * Bottom-left main-menu entry, in the logo's sticker style: a wide black pill with a thick white
	 * outline and drop shadow, a white round badge icon on the left, a big label over a small Korean
	 * sub-label. On hover the pill fills with Accent and the text turns dark (UJinzzaMenuStickerFx).
	 * Uniquely named since anonymous-namespace helpers can collide across .cpp files in unity
	 * builds - see JinzzaUI::AddSpaced's comment.
	 */
	UButton* MakeMainMenuStickerButton(UObject* Owner, TArray<TObjectPtr<UJinzzaMenuStickerFx>>& FxStore, UWidgetTree* Tree, FName Name,
		const FText& Label, const FText& SubLabel, const FLinearColor& Accent, EStickerIcon Icon, bool bBig)
	{
		const FString Base = Name.ToString();
		const float Height = bBig ? 92.f : 76.f;
		const float Radius = (Height - JinzzaUI::StickerShadowDepth) * 0.5f;

		UButton* Button = MakeMainMenuInvisibleButton(Tree, Name);

		USizeBox* Box = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), *(Base + TEXT("_Box")));
		Box->SetWidthOverride(bBig ? 430.f : 400.f);
		Box->SetHeightOverride(Height);
		Button->AddChild(Box);

		UBorder* Face = nullptr;
		Box->AddChild(JinzzaUI::MakeSticker(Tree, *(Base + TEXT("_Sticker")), JinzzaUI::Sticker_Ink, Radius, Face));

		UHorizontalBox* Row = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), *(Base + TEXT("_Row")));
		Face->SetContent(Row);

		const float BadgeSize = Height - JinzzaUI::StickerShadowDepth - 24.f;
		if (UHorizontalBoxSlot* BadgeSlot = Row->AddChildToHorizontalBox(MakeStickerBadge(Tree, *(Base + TEXT("_Badge")), Icon, BadgeSize)))
		{
			BadgeSlot->SetVerticalAlignment(VAlign_Center);
			BadgeSlot->SetPadding(FMargin(14.f, 0.f, 0.f, 0.f));
		}

		UVerticalBox* TextStack = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), *(Base + TEXT("_Text")));
		if (UHorizontalBoxSlot* TextSlot = Row->AddChildToHorizontalBox(TextStack))
		{
			TextSlot->SetSize(ESlateSizeRule::Fill);
			TextSlot->SetVerticalAlignment(VAlign_Center);
			TextSlot->SetPadding(FMargin(16.f, 0.f, 20.f, 0.f));
		}

		UTextBlock* LabelText = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *(Base + TEXT("_Label")));
		LabelText->SetText(Label);
		LabelText->SetFont(JinzzaUI::HeadingFont(bBig ? 36 : 29));
		LabelText->SetColorAndOpacity(FSlateColor(JinzzaUI::Sticker_White));
		TextStack->AddChildToVerticalBox(LabelText);

		UTextBlock* SubLabelText = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *(Base + TEXT("_SubLabel")));
		SubLabelText->SetText(SubLabel);
		SubLabelText->SetFont(JinzzaUI::BodyFont(bBig ? 16 : 14));
		SubLabelText->SetColorAndOpacity(FSlateColor(JinzzaUI::Sticker_SubText));
		if (UVerticalBoxSlot* SubSlot = TextStack->AddChildToVerticalBox(SubLabelText))
		{
			SubSlot->SetPadding(FMargin(2.f, -4.f, 0.f, 0.f));
		}

		UJinzzaMenuStickerFx* Fx = BindStickerFx(Owner, Button, Face, JinzzaUI::Sticker_Ink, Accent, Radius);
		Fx->AddText(LabelText, JinzzaUI::Sticker_White, JinzzaUI::Sticker_Ink);
		Fx->AddText(SubLabelText, JinzzaUI::Sticker_SubText, FLinearColor::FromSRGBColor(FColor(60, 50, 40)));
		FxStore.Add(Fx);
		return Button;
	}

	/**
	 * Bottom-right round sticker button (Customize / Voice Test): a colored circle with a thick white
	 * outline and drop shadow holding IconContent, over a small black caption pill. Hover brightens
	 * the circle toward white.
	 */
	UButton* MakeMainMenuRoundSticker(UObject* Owner, TArray<TObjectPtr<UJinzzaMenuStickerFx>>& FxStore, UWidgetTree* Tree, FName Name,
		const FText& Caption, UWidget* IconContent, const FLinearColor& Fill)
	{
		const FString Base = Name.ToString();
		constexpr float Diameter = 100.f;
		const float Radius = Diameter * 0.5f;

		UButton* Button = MakeMainMenuInvisibleButton(Tree, Name);
		UVerticalBox* Column = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), *(Base + TEXT("_Column")));
		Button->AddChild(Column);

		USizeBox* CircleBox = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), *(Base + TEXT("_CircleBox")));
		CircleBox->SetWidthOverride(Diameter);
		CircleBox->SetHeightOverride(Diameter + JinzzaUI::StickerShadowDepth);
		UBorder* Face = nullptr;
		CircleBox->AddChild(JinzzaUI::MakeSticker(Tree, *(Base + TEXT("_Circle")), Fill, Radius, Face));
		Face->SetHorizontalAlignment(HAlign_Center);
		Face->SetVerticalAlignment(VAlign_Center);
		if (IconContent)
		{
			Face->SetContent(IconContent);
		}
		if (UVerticalBoxSlot* CircleSlot = Column->AddChildToVerticalBox(CircleBox))
		{
			CircleSlot->SetHorizontalAlignment(HAlign_Center);
		}

		UBorder* CaptionFace = nullptr;
		UOverlay* CaptionSticker = JinzzaUI::MakeSticker(Tree, *(Base + TEXT("_Caption")), JinzzaUI::Sticker_Ink, 16.f, CaptionFace);
		CaptionFace->SetPadding(FMargin(14.f, 3.f, 14.f, 5.f));
		UTextBlock* CaptionText = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *(Base + TEXT("_CaptionText")));
		CaptionText->SetText(Caption);
		CaptionText->SetFont(JinzzaUI::HeadingFont(17));
		CaptionText->SetColorAndOpacity(FSlateColor(JinzzaUI::Sticker_White));
		CaptionText->SetJustification(ETextJustify::Center);
		CaptionFace->SetContent(CaptionText);
		if (UVerticalBoxSlot* CaptionSlot = Column->AddChildToVerticalBox(CaptionSticker))
		{
			CaptionSlot->SetHorizontalAlignment(HAlign_Center);
			CaptionSlot->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));
		}

		FxStore.Add(BindStickerFx(Owner, Button, Face, Fill, FMath::Lerp(Fill, JinzzaUI::Sticker_White, 0.35f), Radius));
		return Button;
	}

	/** Soft dark drop shadow so text stays readable over the 3D scene rather than a flat backdrop. */
	void AddMainMenuTextShadow(UTextBlock* Text, float Offset)
	{
		Text->SetShadowOffset(FVector2D(0.f, Offset));
		Text->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.75f));
	}

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

	// Clear on the button page so the 3D menu scene shows through - see ShowPage.
	UBorder* Background = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Background"));
	Background->SetBrush(FSlateColorBrush(FLinearColor::White));
	Background->SetBrushColor(BackdropClear);
	Backdrop = Background;
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

	// Top-left company logo badge (placeholder until a real company logo exists): a rounded-square
	// sticker in the same style as everything else on this page, holding the white mask icon - the
	// mask evokes the "Imitator" disguise premise the game is built around.
	USizeBox* LogoBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("LogoBox"));
	LogoBox->SetWidthOverride(84.f);
	LogoBox->SetHeightOverride(84.f + JinzzaUI::StickerShadowDepth);
	if (UOverlaySlot* LogoSlot = RootOverlay->AddChildToOverlay(LogoBox))
	{
		LogoSlot->SetHorizontalAlignment(HAlign_Left);
		LogoSlot->SetVerticalAlignment(VAlign_Top);
		LogoSlot->SetPadding(FMargin(24.f));
	}

	UBorder* LogoBadgeFace = nullptr;
	LogoBox->AddChild(JinzzaUI::MakeSticker(WidgetTree, TEXT("LogoBadge"), JinzzaUI::Sticker_Ink, 22.f, LogoBadgeFace));
	LogoBadgeFace->SetHorizontalAlignment(HAlign_Center);
	LogoBadgeFace->SetVerticalAlignment(VAlign_Center);
	LogoBadgeFace->SetContent(JinzzaUI::MakeMaskIcon(WidgetTree, TEXT("LogoMaskIcon"), 50.f, JinzzaUI::Sticker_White));

	// Page 0: the button-list page. ButtonsPageRoot is the whole page (fade target).
	UOverlay* ButtonsPage = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("ButtonsPageRoot"));
	ButtonsPageRoot = ButtonsPage;

	// Branding column (game logo + status) sits top-right - distinct from the small top-left
	// LogoBox above (that's the company logo corner badge, not the game title).
	USizeBox* BrandingBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("BrandingBox"));
	BrandingBox->SetWidthOverride(640.f);
	if (UOverlaySlot* BrandingSlot = ButtonsPage->AddChildToOverlay(BrandingBox))
	{
		BrandingSlot->SetHorizontalAlignment(HAlign_Right);
		BrandingSlot->SetVerticalAlignment(VAlign_Top);
		BrandingSlot->SetPadding(FMargin(0.f, 32.f, 48.f, 0.f));
	}

	UVerticalBox* BrandingStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("BrandingStack"));
	BrandingBox->AddChild(BrandingStack);

	// Game logo image (T_Logo: the seal + "who is? JINZZA" sticker wordmark, with alpha), sized by
	// width and right-aligned. Falls back to the plain text title if the texture is missing.
	if (UTexture2D* LogoTexture = LoadObject<UTexture2D>(nullptr, TEXT("/Game/JINZZA/UI/Textures/T_Logo.T_Logo")))
	{
		constexpr float LogoWidth = 600.f;
		const float LogoHeight = LogoWidth * LogoTexture->GetSizeY() / FMath::Max(LogoTexture->GetSizeX(), 1);

		USizeBox* LogoImageBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("TitleLogoBox"));
		LogoImageBox->SetWidthOverride(LogoWidth);
		LogoImageBox->SetHeightOverride(LogoHeight);

		UImage* LogoImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("TitleLogo"));
		LogoImage->SetBrushFromTexture(LogoTexture, false);
		LogoImageBox->AddChild(LogoImage);

		if (UVerticalBoxSlot* LogoSlot = JinzzaUI::AddSpaced(BrandingStack, LogoImageBox, 0.f))
		{
			LogoSlot->SetHorizontalAlignment(HAlign_Right);
		}
	}
	else
	{
		UTextBlock* TitleText = JinzzaUI::MakeTitleText(WidgetTree, TEXT("TitleText"), FText::FromString(TEXT("JINZZA")), 76);
		TitleText->SetJustification(ETextJustify::Right);
		TitleText->SetColorAndOpacity(FSlateColor(JinzzaUI::Sticker_White));
		AddMainMenuTextShadow(TitleText, 4.f);
		JinzzaUI::AddSpaced(BrandingStack, TitleText, 0.f);
	}

	StatusText = JinzzaUI::MakeBodyText(WidgetTree, TEXT("StatusText"), FText::GetEmpty(), false);
	StatusText->SetJustification(ETextJustify::Right);
	StatusText->SetFont(JinzzaUI::BodyFont(18));
	AddMainMenuTextShadow(StatusText, 2.f);
	JinzzaUI::AddSpaced(BrandingStack, StatusText, 8.f);

	// Left button column - Host/Settings/Quit - anchored bottom-left, as wide sticker pills (see
	// MakeMainMenuStickerButton). Host is the big primary one; hover colors: Host yellow, Settings
	// sky blue, Quit coral.
	USizeBox* ButtonsLeftBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ButtonsLeftBox"));
	if (UOverlaySlot* ButtonsLeftSlot = ButtonsPage->AddChildToOverlay(ButtonsLeftBox))
	{
		ButtonsLeftSlot->SetHorizontalAlignment(HAlign_Left);
		ButtonsLeftSlot->SetVerticalAlignment(VAlign_Bottom);
		ButtonsLeftSlot->SetPadding(FMargin(56.f, 0.f, 0.f, 56.f));
	}

	UVerticalBox* ButtonsLeftStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ButtonsLeftStack"));
	ButtonsLeftBox->AddChild(ButtonsLeftStack);

	HostButton = MakeMainMenuStickerButton(this, StickerFx, WidgetTree, TEXT("HostButton"), FText::FromString(TEXT("Host Game")),
		FText::FromString(TEXT("방 만들기")), JinzzaUI::Sticker_Yellow, EStickerIcon::Plus, true);
	JinzzaUI::AddSpaced(ButtonsLeftStack, HostButton, 0.f);

	SettingsButton = MakeMainMenuStickerButton(this, StickerFx, WidgetTree, TEXT("SettingsButton"), FText::FromString(TEXT("Settings")),
		FText::FromString(TEXT("설정")), JinzzaUI::Sticker_Sky, EStickerIcon::Lines, false);
	JinzzaUI::AddSpaced(ButtonsLeftStack, SettingsButton, 10.f);

	QuitButton = MakeMainMenuStickerButton(this, StickerFx, WidgetTree, TEXT("QuitButton"), FText::FromString(TEXT("Quit")),
		FText::FromString(TEXT("게임 종료")), JinzzaUI::Sticker_Coral, EStickerIcon::Cross, false);
	JinzzaUI::AddSpaced(ButtonsLeftStack, QuitButton, 10.f);

	// Right column - Customize / Voice Test - as round candy-colored stickers with their own caption
	// pills, so they need no panel behind them over the 3D scene.
	USizeBox* ButtonsRightBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ButtonsRightBox"));
	if (UOverlaySlot* ButtonsRightSlot = ButtonsPage->AddChildToOverlay(ButtonsRightBox))
	{
		ButtonsRightSlot->SetHorizontalAlignment(HAlign_Right);
		ButtonsRightSlot->SetVerticalAlignment(VAlign_Bottom);
		ButtonsRightSlot->SetPadding(FMargin(0.f, 0.f, 64.f, 56.f));
	}

	UVerticalBox* ButtonsRightStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ButtonsRightStack"));
	ButtonsRightBox->AddChild(ButtonsRightStack);

	// Mask (the "Imitator" disguise premise) for Customize, microphone for Voice Test - both drawn
	// from primitives (JinzzaUI::MakeMaskIcon/MakeMicIcon), white on the colored circle.
	UWidget* MaskIcon = JinzzaUI::MakeMaskIcon(WidgetTree, TEXT("CustomizationMaskIcon"), 56.f, JinzzaUI::Sticker_White);
	CustomizationButton = MakeMainMenuRoundSticker(this, StickerFx, WidgetTree, TEXT("CustomizationButton"), FText::FromString(TEXT("Customize")), MaskIcon, JinzzaUI::Sticker_Pink);
	if (UVerticalBoxSlot* ButtonSlot = JinzzaUI::AddSpaced(ButtonsRightStack, CustomizationButton, 0.f)) { ButtonSlot->SetHorizontalAlignment(HAlign_Center); }

	// Voice Test button - opens VoiceTestWidget as a central switcher page (Page_VoiceTest),
	// same pattern as Settings/Customization.
	UWidget* MicIcon = JinzzaUI::MakeMicIcon(WidgetTree, TEXT("VoiceTestMicIcon"), 50.f, JinzzaUI::Sticker_White);
	VoiceTestButton = MakeMainMenuRoundSticker(this, StickerFx, WidgetTree, TEXT("VoiceTestButton"), FText::FromString(TEXT("Voice Test")), MicIcon, JinzzaUI::Sticker_Teal);
	if (UVerticalBoxSlot* ButtonSlot = JinzzaUI::AddSpaced(ButtonsRightStack, VoiceTestButton, 18.f)) { ButtonSlot->SetHorizontalAlignment(HAlign_Center); }

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

	ShowPage(Page_Buttons);

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
	ShowPage(Page_Buttons);
}

void UjinzzaMainMenuWidget::ShowPage(int32 PageIndex)
{
	if (Switcher)
	{
		Switcher->SetActiveWidgetIndex(PageIndex);
	}

	if (Backdrop)
	{
		Backdrop->SetBrushColor(PageIndex == Page_Buttons ? BackdropClear : BackdropDimmed);
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
	ShowPage(Page_Settings);
	PlayPanelOpenSound(this);
}

void UjinzzaMainMenuWidget::OnCustomizationClicked()
{
	ShowPage(Page_Customization);
	PlayPanelOpenSound(this);
}

void UjinzzaMainMenuWidget::OnVoiceTestClicked()
{
	ShowPage(Page_VoiceTest);
	PlayPanelOpenSound(this);
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

void UJinzzaMenuStickerFx::Bind(UButton* InButton, UBorder* InFace, const FSlateBrush& InNormalBrush, const FSlateBrush& InHoverBrush)
{
	Face = InFace;
	NormalBrush = InNormalBrush;
	HoverBrush = InHoverBrush;

	if (InButton)
	{
		InButton->OnHovered.AddDynamic(this, &UJinzzaMenuStickerFx::HandleHovered);
		InButton->OnUnhovered.AddDynamic(this, &UJinzzaMenuStickerFx::HandleUnhovered);
		InButton->OnPressed.AddDynamic(this, &UJinzzaMenuStickerFx::HandlePressed);
		InButton->OnReleased.AddDynamic(this, &UJinzzaMenuStickerFx::HandleReleased);
	}
}

void UJinzzaMenuStickerFx::AddText(UTextBlock* Text, const FLinearColor& NormalColor, const FLinearColor& HoverColor)
{
	Texts.Add(Text);
	TextNormalColors.Add(NormalColor);
	TextHoverColors.Add(HoverColor);
}

void UJinzzaMenuStickerFx::Apply(bool bHot, float LiftY)
{
	if (Face)
	{
		Face->SetBrush(bHot ? HoverBrush : NormalBrush);
		Face->SetRenderTranslation(FVector2D(0.f, LiftY));
	}
	for (int32 Index = 0; Index < Texts.Num(); ++Index)
	{
		if (Texts[Index])
		{
			Texts[Index]->SetColorAndOpacity(FSlateColor(bHot ? TextHoverColors[Index] : TextNormalColors[Index]));
		}
	}
}

void UJinzzaMenuStickerFx::HandleHovered()
{
	bHovered = true;
	Apply(true, -3.f);
}

void UJinzzaMenuStickerFx::HandleUnhovered()
{
	bHovered = false;
	Apply(false, 0.f);
}

void UJinzzaMenuStickerFx::HandlePressed()
{
	// Sink most of the way onto the drop shadow.
	Apply(true, JinzzaUI::StickerShadowDepth - 2.f);
}

void UJinzzaMenuStickerFx::HandleReleased()
{
	Apply(bHovered, bHovered ? -3.f : 0.f);
}
