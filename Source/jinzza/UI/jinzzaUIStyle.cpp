// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateBrush.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	UFont* LoadJinzzaUIFont()
	{
		static TWeakObjectPtr<UFont> Cached;
		if (!Cached.IsValid())
		{
			Cached = LoadObject<UFont>(nullptr, TEXT("/Game/JINZZA/Fonts/SacheonUju-Regular_Font.SacheonUju-Regular_Font"));
		}
		return Cached.Get();
	}

	/** Pill-shaped button base (source: noob-game's OptionButtonImage.png, re-tinted per style). */
	UTexture2D* LoadButtonPillTexture()
	{
		static TWeakObjectPtr<UTexture2D> Cached;
		if (!Cached.IsValid())
		{
			Cached = LoadObject<UTexture2D>(nullptr, TEXT("/Game/JINZZA/UI/Textures/T_ButtonPill.T_ButtonPill"));
		}
		return Cached.Get();
	}

	/** Pinned-note-card base (source: noob-game's MemoYellow.png, re-tinted to parchment). */
	UTexture2D* LoadNotePanelTexture()
	{
		static TWeakObjectPtr<UTexture2D> Cached;
		if (!Cached.IsValid())
		{
			Cached = LoadObject<UTexture2D>(nullptr, TEXT("/Game/JINZZA/UI/Textures/T_NotePanel.T_NotePanel"));
		}
		return Cached.Get();
	}

	/** 9-slice brush from a texture, tinted. Margins are fractions of the source image sized to
	 * protect each texture's own baked border/detail from stretch distortion. */
	FSlateBrush MakeNineSliceBrush(UTexture2D* Texture, const FMargin& Margin, const FLinearColor& Tint)
	{
		FSlateBrush Brush;
		if (Texture)
		{
			Brush.SetResourceObject(Texture);
			Brush.ImageSize = FVector2D(Texture->GetSizeX(), Texture->GetSizeY());
			Brush.DrawAs = ESlateBrushDrawType::Box;
			Brush.Margin = Margin;
			Brush.TintColor = FSlateColor(Tint);
		}
		return Brush;
	}

	USoundBase* LoadButtonClickSound()
	{
		static TWeakObjectPtr<USoundBase> Cached;
		if (!Cached.IsValid())
		{
			Cached = LoadObject<USoundBase>(nullptr, TEXT("/Game/JINZZA/Audio/Sounds/UISounds/ButtonClickPopSound_Cue.ButtonClickPopSound_Cue"));
		}
		return Cached.Get();
	}

	USoundBase* LoadButtonHoverSound()
	{
		static TWeakObjectPtr<USoundBase> Cached;
		if (!Cached.IsValid())
		{
			Cached = LoadObject<USoundBase>(nullptr, TEXT("/Game/JINZZA/Audio/Sounds/UISounds/ButtonHover_Cue.ButtonHover_Cue"));
		}
		return Cached.Get();
	}
}

void UJinzzaUIButtonSounds::HandleClicked()
{
	if (USoundBase* Sound = LoadButtonClickSound())
	{
		UGameplayStatics::PlaySound2D(this, Sound);
	}
}

void UJinzzaUIButtonSounds::HandleHovered()
{
	if (USoundBase* Sound = LoadButtonHoverSound())
	{
		UGameplayStatics::PlaySound2D(this, Sound);
	}
}

namespace JinzzaUI
{
	const FLinearColor Color_Background(0.035f, 0.030f, 0.045f, 1.0f);
	const FLinearColor Color_Panel(0.075f, 0.065f, 0.090f, 0.94f);
	const FLinearColor Color_PanelBorder(0.30f, 0.24f, 0.15f, 0.6f);
	const FLinearColor Color_Accent(0.85f, 0.68f, 0.25f, 1.0f);
	const FLinearColor Color_AccentAlt(0.75f, 0.09f, 0.12f, 1.0f);
	const FLinearColor Color_TextPrimary(0.95f, 0.95f, 0.96f, 1.0f);
	const FLinearColor Color_TextMuted(0.62f, 0.60f, 0.65f, 1.0f);

	namespace
	{
		const FLinearColor Color_ButtonNormal(0.11f, 0.095f, 0.14f, 0.95f);
		const FLinearColor Color_ButtonHovered(0.17f, 0.145f, 0.10f, 0.97f);
		const FLinearColor Color_ButtonPressed(0.22f, 0.17f, 0.08f, 1.0f);
		const FLinearColor Color_ButtonDisabled(0.08f, 0.08f, 0.09f, 0.6f);

		const FLinearColor Color_WarnButtonHovered(0.35f, 0.07f, 0.09f, 0.97f);
		const FLinearColor Color_WarnButtonPressed(0.45f, 0.05f, 0.08f, 1.0f);

		FButtonStyle MakeRoundedButtonStyle(const FLinearColor& NormalColor, const FLinearColor& HoveredColor,
			const FLinearColor& PressedColor, const FLinearColor& BorderColor)
		{
			constexpr float CornerRadius = 6.f;
			constexpr float OutlineWidth = 1.5f;

			FButtonStyle Style;
			if (UTexture2D* Pill = LoadButtonPillTexture())
			{
				// T_ButtonPill is 835x312 with its own baked outline/shine; the rounded caps eat
				// ~19% of the width, the outline stroke ~8% of the height - BorderColor isn't used
				// here since the outline is baked into the art, not drawn separately.
				const FMargin PillMargin(0.19f, 0.08f, 0.19f, 0.08f);
				Style.Normal = MakeNineSliceBrush(Pill, PillMargin, NormalColor);
				Style.Hovered = MakeNineSliceBrush(Pill, PillMargin, HoveredColor);
				Style.Pressed = MakeNineSliceBrush(Pill, PillMargin, PressedColor);
				Style.Disabled = MakeNineSliceBrush(Pill, PillMargin, Color_ButtonDisabled);
			}
			else
			{
				Style.Normal = FSlateRoundedBoxBrush(NormalColor, CornerRadius, BorderColor, OutlineWidth);
				Style.Hovered = FSlateRoundedBoxBrush(HoveredColor, CornerRadius, BorderColor, OutlineWidth);
				Style.Pressed = FSlateRoundedBoxBrush(PressedColor, CornerRadius, BorderColor, OutlineWidth);
				Style.Disabled = FSlateRoundedBoxBrush(Color_ButtonDisabled, CornerRadius, FLinearColor(0.f, 0.f, 0.f, 0.f), 0.f);
			}
			Style.NormalPadding = FMargin(16.f, 10.f);
			Style.PressedPadding = FMargin(16.f, 11.f, 16.f, 9.f);
			return Style;
		}

		UButton* MakeStyledButton(UWidgetTree* Tree, FName Name, const FText& Label, float FontSize,
			const FButtonStyle& Style, const FLinearColor& TextColor)
		{
			UButton* Button = Tree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
			Button->SetStyle(Style);

			UTextBlock* ButtonText = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *(Name.ToString() + TEXT("_Label")));
			ButtonText->SetText(Label);
			ButtonText->SetFont(HeadingFont(FMath::RoundToInt(FontSize)));
			ButtonText->SetJustification(ETextJustify::Center);
			ButtonText->SetColorAndOpacity(FSlateColor(TextColor));

			UJinzzaUIButtonSounds* SoundBinder = NewObject<UJinzzaUIButtonSounds>(Button);
			Button->OnClicked.AddDynamic(SoundBinder, &UJinzzaUIButtonSounds::HandleClicked);
			Button->OnHovered.AddDynamic(SoundBinder, &UJinzzaUIButtonSounds::HandleHovered);

			Button->AddChild(ButtonText);
			return Button;
		}
	}

	FSlateFontInfo TitleFont(int32 Size)
	{
		if (UFont* Font = LoadJinzzaUIFont())
		{
			return FSlateFontInfo(Font, Size);
		}
		return FCoreStyle::GetDefaultFontStyle("Bold", Size);
	}

	FSlateFontInfo HeadingFont(int32 Size)
	{
		if (UFont* Font = LoadJinzzaUIFont())
		{
			return FSlateFontInfo(Font, Size);
		}
		return FCoreStyle::GetDefaultFontStyle("Bold", Size);
	}

	FSlateFontInfo BodyFont(int32 Size)
	{
		if (UFont* Font = LoadJinzzaUIFont())
		{
			return FSlateFontInfo(Font, Size);
		}
		return FCoreStyle::GetDefaultFontStyle("Regular", Size);
	}

	UButton* MakePrimaryButton(UWidgetTree* Tree, FName Name, const FText& Label, float FontSize)
	{
		static const FButtonStyle Style = MakeRoundedButtonStyle(Color_ButtonNormal, Color_ButtonHovered, Color_ButtonPressed, Color_Accent);
		return MakeStyledButton(Tree, Name, Label, FontSize, Style, Color_TextPrimary);
	}

	UButton* MakeSecondaryButton(UWidgetTree* Tree, FName Name, const FText& Label, float FontSize)
	{
		static const FButtonStyle Style = MakeRoundedButtonStyle(
			FLinearColor(0.09f, 0.09f, 0.10f, 0.7f),
			FLinearColor(0.14f, 0.14f, 0.16f, 0.85f),
			FLinearColor(0.18f, 0.18f, 0.20f, 0.95f),
			Color_TextMuted);
		return MakeStyledButton(Tree, Name, Label, FontSize, Style, Color_TextMuted);
	}

	UButton* MakeWarningButton(UWidgetTree* Tree, FName Name, const FText& Label, float FontSize)
	{
		static const FButtonStyle Style = MakeRoundedButtonStyle(Color_ButtonNormal, Color_WarnButtonHovered, Color_WarnButtonPressed, Color_AccentAlt);
		return MakeStyledButton(Tree, Name, Label, FontSize, Style, Color_TextPrimary);
	}

	UButton* MakeMenuActionButton(UWidgetTree* Tree, FName Name, const FText& Label, const FLinearColor& TintColor, float FontSize)
	{
		// Hover/press feedback derived from the caller's own tint (lighten/darken in HSV) rather
		// than hand-picked per color, since this helper takes an arbitrary palette of tints instead
		// of the noir palette's fixed set.
		const FLinearColor Hovered = FLinearColor::LerpUsingHSV(TintColor, FLinearColor::White, 0.15f);
		const FLinearColor Pressed = FLinearColor::LerpUsingHSV(TintColor, FLinearColor::Black, 0.15f);
		const FButtonStyle Style = MakeRoundedButtonStyle(TintColor, Hovered, Pressed, TintColor);

		UButton* Button = Tree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
		Button->SetStyle(Style);

		UHorizontalBox* Row = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), *(Name.ToString() + TEXT("_Row")));

		// White circular icon-slot badge (see header comment) - no icon glyph inside, just the
		// NOOB-style badge shape/placement until real icon art exists.
		UBorder* IconBadge = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), *(Name.ToString() + TEXT("_IconBadge")));
		IconBadge->SetBrush(FSlateRoundedBoxBrush(FLinearColor::White, 14.f));
		IconBadge->SetPadding(FMargin(0.f));

		USizeBox* IconBox = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), *(Name.ToString() + TEXT("_IconBox")));
		IconBox->SetWidthOverride(28.f);
		IconBox->SetHeightOverride(28.f);
		IconBox->AddChild(IconBadge);

		if (UHorizontalBoxSlot* IconSlot = Row->AddChildToHorizontalBox(IconBox))
		{
			IconSlot->SetVerticalAlignment(VAlign_Center);
			IconSlot->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));
		}

		UTextBlock* ButtonText = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *(Name.ToString() + TEXT("_Label")));
		ButtonText->SetText(Label);
		ButtonText->SetFont(HeadingFont(FMath::RoundToInt(FontSize)));
		ButtonText->SetColorAndOpacity(FSlateColor(Color_TextPrimary));
		if (UHorizontalBoxSlot* TextSlot = Row->AddChildToHorizontalBox(ButtonText))
		{
			TextSlot->SetVerticalAlignment(VAlign_Center);
		}

		UJinzzaUIButtonSounds* SoundBinder = NewObject<UJinzzaUIButtonSounds>(Button);
		Button->OnClicked.AddDynamic(SoundBinder, &UJinzzaUIButtonSounds::HandleClicked);
		Button->OnHovered.AddDynamic(SoundBinder, &UJinzzaUIButtonSounds::HandleHovered);

		Button->AddChild(Row);
		return Button;
	}

	UButton* MakeNoobIconButton(UWidgetTree* Tree, FName Name, const FText& Label, const TCHAR* TexturePath, const FLinearColor& FallbackTintColor, float Height, float FontSize)
	{
		UTexture2D* ButtonTexture = LoadObject<UTexture2D>(nullptr, TexturePath);
		if (!ButtonTexture)
		{
			return MakeMenuActionButton(Tree, Name, Label, FallbackTintColor, FontSize);
		}

		const float AspectRatio = (float)ButtonTexture->GetSizeX() / (float)ButtonTexture->GetSizeY();
		const float Width = Height * AspectRatio;

		FSlateBrush Normal;
		Normal.SetResourceObject(ButtonTexture);
		Normal.ImageSize = FVector2D(Width, Height);
		Normal.DrawAs = ESlateBrushDrawType::Image;

		FSlateBrush Hovered = Normal;
		Hovered.TintColor = FSlateColor(FLinearColor(1.12f, 1.12f, 1.12f, 1.f));
		FSlateBrush Pressed = Normal;
		Pressed.TintColor = FSlateColor(FLinearColor(0.85f, 0.85f, 0.85f, 1.f));
		FSlateBrush Disabled = Normal;
		Disabled.TintColor = FSlateColor(FLinearColor(0.6f, 0.6f, 0.6f, 0.5f));

		FButtonStyle Style;
		Style.Normal = Normal;
		Style.Hovered = Hovered;
		Style.Pressed = Pressed;
		Style.Disabled = Disabled;
		Style.NormalPadding = FMargin(0.f);
		Style.PressedPadding = FMargin(0.f);

		UButton* Button = Tree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
		Button->SetStyle(Style);

		USizeBox* ButtonBox = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), *(Name.ToString() + TEXT("_Box")));
		ButtonBox->SetWidthOverride(Width);
		ButtonBox->SetHeightOverride(Height);

		UHorizontalBox* Row = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), *(Name.ToString() + TEXT("_Row")));

		// Invisible spacer reserving the baked icon's footprint (~22% of the pill's width in the
		// source art) so the label starts to its right instead of overlapping it.
		USizeBox* IconSpacer = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), *(Name.ToString() + TEXT("_IconSpacer")));
		IconSpacer->SetWidthOverride(Width * 0.22f);
		if (UHorizontalBoxSlot* SpacerSlot = Row->AddChildToHorizontalBox(IconSpacer))
		{
			SpacerSlot->SetVerticalAlignment(VAlign_Fill);
		}

		UTextBlock* ButtonText = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *(Name.ToString() + TEXT("_Label")));
		ButtonText->SetText(Label);
		ButtonText->SetFont(HeadingFont(FMath::RoundToInt(FontSize)));
		// White/cream text on the pastel pill, matching NOOB-GAME's own button label color.
		ButtonText->SetColorAndOpacity(FSlateColor(FLinearColor(0.98f, 0.98f, 0.96f, 1.f)));
		if (UHorizontalBoxSlot* TextSlot = Row->AddChildToHorizontalBox(ButtonText))
		{
			TextSlot->SetVerticalAlignment(VAlign_Center);
			TextSlot->SetSize(ESlateSizeRule::Fill);
		}

		ButtonBox->AddChild(Row);

		UJinzzaUIButtonSounds* SoundBinder = NewObject<UJinzzaUIButtonSounds>(Button);
		Button->OnClicked.AddDynamic(SoundBinder, &UJinzzaUIButtonSounds::HandleClicked);
		Button->OnHovered.AddDynamic(SoundBinder, &UJinzzaUIButtonSounds::HandleHovered);

		Button->AddChild(ButtonBox);
		return Button;
	}

	UButton* MakeCircleIconButton(UWidgetTree* Tree, FName Name, const FText& Label, UWidget* IconContent, const FLinearColor& TintColor, float Diameter, float FontSize)
	{
		// Fully transparent button style - the visible circle is a child UBorder (below), not the
		// button's own background, so the caption below the circle doesn't sit on a colored
		// rectangle too.
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

		UVerticalBox* Column = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), *(Name.ToString() + TEXT("_Column")));

		USizeBox* CircleBox = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), *(Name.ToString() + TEXT("_CircleBox")));
		CircleBox->SetWidthOverride(Diameter);
		CircleBox->SetHeightOverride(Diameter);

		UBorder* Circle = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), *(Name.ToString() + TEXT("_Circle")));
		Circle->SetBrush(FSlateRoundedBoxBrush(TintColor, Diameter * 0.5f));
		// UBorder's own HorizontalAlignment/VerticalAlignment control how its CONTENT (IconContent)
		// is aligned inside it - Center here, rather than Fill, so a smaller icon doesn't get
		// stretched to the full circle. (UImage/generic UWidget content has no such alignment API
		// of its own - only the container controls it.)
		Circle->SetHorizontalAlignment(HAlign_Center);
		Circle->SetVerticalAlignment(VAlign_Center);
		if (IconContent)
		{
			Circle->SetContent(IconContent);
		}
		CircleBox->AddChild(Circle);

		if (UVerticalBoxSlot* CircleSlot = Column->AddChildToVerticalBox(CircleBox))
		{
			CircleSlot->SetHorizontalAlignment(HAlign_Center);
		}

		UTextBlock* Caption = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *(Name.ToString() + TEXT("_Caption")));
		Caption->SetText(Label);
		Caption->SetFont(BodyFont(FMath::RoundToInt(FontSize)));
		Caption->SetJustification(ETextJustify::Center);
		Caption->SetColorAndOpacity(FSlateColor(Color_TextPrimary));
		if (UVerticalBoxSlot* CaptionSlot = Column->AddChildToVerticalBox(Caption))
		{
			CaptionSlot->SetHorizontalAlignment(HAlign_Center);
			CaptionSlot->SetPadding(FMargin(0.f, 6.f, 0.f, 0.f));
		}

		UJinzzaUIButtonSounds* SoundBinder = NewObject<UJinzzaUIButtonSounds>(Button);
		Button->OnClicked.AddDynamic(SoundBinder, &UJinzzaUIButtonSounds::HandleClicked);
		Button->OnHovered.AddDynamic(SoundBinder, &UJinzzaUIButtonSounds::HandleHovered);

		Button->AddChild(Column);
		return Button;
	}

	UButton* MakeNoobCircleIconButton(UWidgetTree* Tree, FName Name, const FText& Label, const TCHAR* TexturePath, const FLinearColor& TintColor, float Diameter, float FontSize)
	{
		UTexture2D* IconTexture = LoadObject<UTexture2D>(nullptr, TexturePath);

		UImage* Icon = nullptr;
		if (IconTexture)
		{
			// Aspect-fit within ~62% of the circle's diameter rather than stretching to a square,
			// so a non-square source icon (e.g. T_IconVoiceTest's headphones, wider than tall)
			// isn't squashed.
			const float FitSize = Diameter * 0.62f;
			const float AspectRatio = (float)IconTexture->GetSizeX() / (float)IconTexture->GetSizeY();
			const FVector2D IconSize = AspectRatio >= 1.f
				? FVector2D(FitSize, FitSize / AspectRatio)
				: FVector2D(FitSize * AspectRatio, FitSize);

			Icon = Tree->ConstructWidget<UImage>(UImage::StaticClass(), *(Name.ToString() + TEXT("_Icon")));
			FSlateBrush IconBrush;
			IconBrush.SetResourceObject(IconTexture);
			IconBrush.ImageSize = IconSize;
			Icon->SetBrush(IconBrush);
		}

		return MakeCircleIconButton(Tree, Name, Label, Icon, TintColor, Diameter, FontSize);
	}

	UWidget* MakeMaskIcon(UWidgetTree* Tree, FName Name, float Size, const FLinearColor& MaskColor)
	{
		USizeBox* Box = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), Name);
		Box->SetWidthOverride(Size);
		Box->SetHeightOverride(Size * 0.62f);

		UOverlay* MaskOverlay = Tree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), *(Name.ToString() + TEXT("_Overlay")));
		Box->AddChild(MaskOverlay);

		// Mask "face": a wide, low rounded bar.
		UBorder* Face = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), *(Name.ToString() + TEXT("_Face")));
		Face->SetBrush(FSlateRoundedBoxBrush(MaskColor, Size * 0.31f));
		if (UOverlaySlot* FaceSlot = MaskOverlay->AddChildToOverlay(Face))
		{
			FaceSlot->SetHorizontalAlignment(HAlign_Fill);
			FaceSlot->SetVerticalAlignment(VAlign_Fill);
		}

		// Two dark eye-hole cutouts, side by side.
		UHorizontalBox* Eyes = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), *(Name.ToString() + TEXT("_Eyes")));
		if (UOverlaySlot* EyesSlot = MaskOverlay->AddChildToOverlay(Eyes))
		{
			EyesSlot->SetHorizontalAlignment(HAlign_Center);
			EyesSlot->SetVerticalAlignment(VAlign_Center);
		}

		const float EyeSize = Size * 0.16f;
		const FLinearColor EyeColor(0.05f, 0.04f, 0.05f, 1.f);
		for (int32 EyeIndex = 0; EyeIndex < 2; ++EyeIndex)
		{
			const FString EyeName = Name.ToString() + FString::Printf(TEXT("_Eye%d"), EyeIndex);
			USizeBox* EyeBox = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), *EyeName);
			EyeBox->SetWidthOverride(EyeSize);
			EyeBox->SetHeightOverride(EyeSize);

			UBorder* Eye = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), *(EyeName + TEXT("_Shape")));
			Eye->SetBrush(FSlateRoundedBoxBrush(EyeColor, EyeSize * 0.5f));
			EyeBox->AddChild(Eye);

			if (UHorizontalBoxSlot* EyeSlot = Eyes->AddChildToHorizontalBox(EyeBox))
			{
				EyeSlot->SetPadding(FMargin(Size * 0.08f, 0.f, Size * 0.08f, 0.f));
			}
		}

		return Box;
	}

	UWidget* MakeMicIcon(UWidgetTree* Tree, FName Name, float Size, const FLinearColor& MicColor)
	{
		USizeBox* Box = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), Name);
		Box->SetWidthOverride(Size * 0.58f);
		Box->SetHeightOverride(Size);

		UVerticalBox* Stack = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), *(Name.ToString() + TEXT("_Stack")));
		Box->AddChild(Stack);

		// Mic head: a tall capsule (a rounded box whose corner radius is half its own width).
		const float HeadWidth = Size * 0.44f;
		const float HeadHeight = Size * 0.5f;
		USizeBox* HeadBox = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), *(Name.ToString() + TEXT("_HeadBox")));
		HeadBox->SetWidthOverride(HeadWidth);
		HeadBox->SetHeightOverride(HeadHeight);
		UBorder* Head = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), *(Name.ToString() + TEXT("_Head")));
		Head->SetBrush(FSlateRoundedBoxBrush(MicColor, HeadWidth * 0.5f));
		HeadBox->AddChild(Head);
		if (UVerticalBoxSlot* HeadSlot = Stack->AddChildToVerticalBox(HeadBox))
		{
			HeadSlot->SetHorizontalAlignment(HAlign_Center);
		}

		// Stand: a thin vertical bar.
		USizeBox* StandBox = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), *(Name.ToString() + TEXT("_StandBox")));
		StandBox->SetWidthOverride(Size * 0.08f);
		StandBox->SetHeightOverride(Size * 0.26f);
		UBorder* Stand = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), *(Name.ToString() + TEXT("_Stand")));
		Stand->SetBrush(FSlateRoundedBoxBrush(MicColor, Size * 0.02f));
		StandBox->AddChild(Stand);
		if (UVerticalBoxSlot* StandSlot = Stack->AddChildToVerticalBox(StandBox))
		{
			StandSlot->SetHorizontalAlignment(HAlign_Center);
			StandSlot->SetPadding(FMargin(0.f, Size * 0.02f, 0.f, 0.f));
		}

		// Base: a thin horizontal foot bar.
		USizeBox* BaseBox = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), *(Name.ToString() + TEXT("_BaseBox")));
		BaseBox->SetWidthOverride(Size * 0.30f);
		BaseBox->SetHeightOverride(Size * 0.06f);
		UBorder* Base = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), *(Name.ToString() + TEXT("_Base")));
		Base->SetBrush(FSlateRoundedBoxBrush(MicColor, Size * 0.03f));
		BaseBox->AddChild(Base);
		if (UVerticalBoxSlot* BaseSlot = Stack->AddChildToVerticalBox(BaseBox))
		{
			BaseSlot->SetHorizontalAlignment(HAlign_Center);
		}

		return Box;
	}

	UBorder* MakePanelBackground(UWidgetTree* Tree, FName Name)
	{
		UBorder* Panel = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), Name);
		FSlateBrush Brush = FSlateRoundedBoxBrush(Color_Panel, 10.f, Color_PanelBorder, 1.5f);
		Panel->SetBrush(Brush);
		return Panel;
	}

	UBorder* MakeNoteBackground(UWidgetTree* Tree, FName Name)
	{
		UBorder* Panel = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), Name);
		if (UTexture2D* Note = LoadNotePanelTexture())
		{
			// T_NotePanel is 1088x960: a folded corner (bottom-right) and washi tape overlapping the
			// top edge, so the margins are asymmetric to keep both un-stretched.
			const FMargin NoteMargin(0.22f, 0.30f, 0.22f, 0.22f);
			// Warm parchment tint, close to the source art's own cream rather than a heavy recolor -
			// the aged-note look already reads as "evidence/case file" against the dark noir panels.
			const FLinearColor NoteTint(0.90f, 0.82f, 0.60f, 0.97f);
			Panel->SetBrush(MakeNineSliceBrush(Note, NoteMargin, NoteTint));
		}
		else
		{
			Panel->SetBrush(FSlateRoundedBoxBrush(Color_Panel, 10.f, Color_PanelBorder, 1.5f));
		}
		return Panel;
	}

	UWidget* MakeDivider(UWidgetTree* Tree, FName Name, float Width, float Height)
	{
		UBorder* Bar = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), *(Name.ToString() + TEXT("_Bar")));
		Bar->SetBrush(FSlateRoundedBoxBrush(Color_Accent, Height * 0.5f));
		Bar->SetPadding(FMargin(0.f));

		USizeBox* SizeBox = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), Name);
		SizeBox->SetWidthOverride(Width);
		SizeBox->SetHeightOverride(Height);
		SizeBox->AddChild(Bar);
		return SizeBox;
	}

	UTextBlock* MakeTitleText(UWidgetTree* Tree, FName Name, const FText& Text, int32 Size)
	{
		UTextBlock* Title = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		Title->SetText(Text);
		Title->SetFont(TitleFont(Size));
		Title->SetJustification(ETextJustify::Center);
		Title->SetColorAndOpacity(FSlateColor(Color_Accent));
		return Title;
	}

	UTextBlock* MakeSectionHeading(UWidgetTree* Tree, FName Name, const FText& Text)
	{
		UTextBlock* Heading = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		Heading->SetText(Text);
		Heading->SetFont(HeadingFont(20));
		Heading->SetColorAndOpacity(FSlateColor(Color_TextPrimary));
		return Heading;
	}

	UTextBlock* MakeBodyText(UWidgetTree* Tree, FName Name, const FText& Text, bool bMuted)
	{
		UTextBlock* Body = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		Body->SetText(Text);
		Body->SetFont(BodyFont(15));
		Body->SetColorAndOpacity(FSlateColor(bMuted ? Color_TextMuted : Color_TextPrimary));
		return Body;
	}

	UVerticalBoxSlot* AddSpaced(UVerticalBox* Box, UWidget* Child, float TopPadding)
	{
		UVerticalBoxSlot* Slot = Box->AddChildToVerticalBox(Child);
		if (Slot)
		{
			Slot->SetPadding(FMargin(0.f, TopPadding, 0.f, 0.f));
		}
		return Slot;
	}

	UWidget* MakeLabeledRow(UWidgetTree* Tree, FName Name, const FText& LabelText, UWidget* Control, float LabelWidth)
	{
		UHorizontalBox* Row = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), Name);

		USizeBox* LabelBox = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), *(Name.ToString() + TEXT("_LabelBox")));
		LabelBox->SetWidthOverride(LabelWidth);
		LabelBox->AddChild(MakeBodyText(Tree, *(Name.ToString() + TEXT("_Label")), LabelText, true));

		if (UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(LabelBox))
		{
			LabelSlot->SetVerticalAlignment(VAlign_Center);
			LabelSlot->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));
		}

		if (Control)
		{
			if (UHorizontalBoxSlot* ControlSlot = Row->AddChildToHorizontalBox(Control))
			{
				ControlSlot->SetVerticalAlignment(VAlign_Center);
				ControlSlot->SetSize(ESlateSizeRule::Fill);
			}
		}

		return Row;
	}
}
