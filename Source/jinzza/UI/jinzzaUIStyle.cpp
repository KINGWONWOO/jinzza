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
#include "Components/Slider.h"
#include "Components/CheckBox.h"
#include "Components/ComboBoxString.h"
#include "Components/SpinBox.h"
#include "Components/EditableTextBox.h"
#include "Styling/SlateTypes.h"

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

	UWidget* MakeKeyCap(UWidgetTree* Tree, FName Name, UTextBlock*& OutKeyText, float Height)
	{
		USizeBox* Box = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), Name);
		Box->SetHeightOverride(Height);
		Box->SetMinDesiredWidth(Height);

		// Parchment key face with a gold rim - the same light-on-dark contrast as the note panels, so the key
		// reads as "the thing to press" next to the prompt's muted body text.
		UBorder* Cap = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), *(Name.ToString() + TEXT("_Cap")));
		Cap->SetBrush(FSlateRoundedBoxBrush(FLinearColor(0.93f, 0.89f, 0.80f, 1.f), 6.f, Color_Accent, 2.f));
		Cap->SetPadding(FMargin(8.f, 0.f));
		Cap->SetHorizontalAlignment(HAlign_Center);
		Cap->SetVerticalAlignment(VAlign_Center);
		Box->AddChild(Cap);

		OutKeyText = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *(Name.ToString() + TEXT("_Key")));
		OutKeyText->SetFont(HeadingFont(FMath::RoundToInt(Height * 0.5f)));
		OutKeyText->SetColorAndOpacity(FSlateColor(Color_Background));
		OutKeyText->SetJustification(ETextJustify::Center);
		Cap->SetContent(OutKeyText);

		return Box;
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

	// --- Sticker style (see header). sRGB picks so they match the logo / a paint program. ---
	const FLinearColor Sticker_Ink = FLinearColor::FromSRGBColor(FColor(18, 18, 22));
	const FLinearColor Sticker_White = FLinearColor::White;
	const FLinearColor Sticker_Shadow(0.f, 0.f, 0.f, 0.55f);
	const FLinearColor Sticker_SubText = FLinearColor::FromSRGBColor(FColor(185, 185, 195));
	const FLinearColor Sticker_Yellow = FLinearColor::FromSRGBColor(FColor(255, 204, 40));
	const FLinearColor Sticker_Sky = FLinearColor::FromSRGBColor(FColor(115, 199, 255));
	const FLinearColor Sticker_Coral = FLinearColor::FromSRGBColor(FColor(255, 107, 115));
	const FLinearColor Sticker_Pink = FLinearColor::FromSRGBColor(FColor(255, 105, 170));
	const FLinearColor Sticker_Teal = FLinearColor::FromSRGBColor(FColor(40, 185, 165));

	namespace
	{
		/** Corner radius for small sticker pills/fields - about half their height, so they read as pills. */
		constexpr float StickerPillRadius = 20.f;
		constexpr float StickerFieldRadius = 12.f;

		FSlateBrush MakeStickerBrush(const FLinearColor& Fill, float Radius, float Outline = 3.f, const FVector2D& ImageSize = FVector2D::ZeroVector)
		{
			FSlateBrush Brush = FSlateRoundedBoxBrush(Fill, Radius, Sticker_White, Outline);
			if (!ImageSize.IsZero())
			{
				Brush.ImageSize = ImageSize;
			}
			return Brush;
		}
	}

	UOverlay* MakeSticker(UWidgetTree* Tree, FName Name, const FLinearColor& Fill, float Radius, UBorder*& OutFace, float Outline, float ShadowDepth)
	{
		const FString Base = Name.ToString();
		UOverlay* Root = Tree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), Name);

		UBorder* Shadow = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), *(Base + TEXT("_Shadow")));
		Shadow->SetBrush(FSlateRoundedBoxBrush(Sticker_Shadow, Radius));
		if (UOverlaySlot* ShadowSlot = Root->AddChildToOverlay(Shadow))
		{
			ShadowSlot->SetHorizontalAlignment(HAlign_Fill);
			ShadowSlot->SetVerticalAlignment(VAlign_Fill);
			ShadowSlot->SetPadding(FMargin(0.f, ShadowDepth, 0.f, 0.f));
		}

		OutFace = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), *(Base + TEXT("_Face")));
		OutFace->SetBrush(FSlateRoundedBoxBrush(Fill, Radius, Sticker_White, Outline));
		OutFace->SetPadding(FMargin(0.f));
		if (UOverlaySlot* FaceSlot = Root->AddChildToOverlay(OutFace))
		{
			FaceSlot->SetHorizontalAlignment(HAlign_Fill);
			FaceSlot->SetVerticalAlignment(VAlign_Fill);
			FaceSlot->SetPadding(FMargin(0.f, 0.f, 0.f, ShadowDepth));
		}
		return Root;
	}

	UOverlay* MakeStickerPanel(UWidgetTree* Tree, FName Name, UBorder*& OutFace)
	{
		UOverlay* Root = MakeSticker(Tree, Name, Sticker_Ink, 32.f, OutFace, 5.f, 10.f);
		OutFace->SetPadding(FMargin(32.f, 26.f, 32.f, 28.f));
		return Root;
	}

	FButtonStyle MakeStickerButtonStyle(const FLinearColor& Accent, bool bFilled)
	{
		const FLinearColor HoverFill = bFilled ? FMath::Lerp(Accent, Sticker_White, 0.3f) : Accent;
		const FLinearColor PressedFill = FMath::Lerp(Accent, Sticker_Ink, 0.15f);

		FButtonStyle Style;
		Style.SetNormal(MakeStickerBrush(bFilled ? Accent : Sticker_Ink, StickerPillRadius));
		Style.SetHovered(MakeStickerBrush(HoverFill, StickerPillRadius));
		Style.SetPressed(MakeStickerBrush(PressedFill, StickerPillRadius));
		Style.SetDisabled(MakeStickerBrush(FLinearColor::FromSRGBColor(FColor(60, 60, 66)), StickerPillRadius));
		Style.SetNormalForeground(FSlateColor(bFilled ? Sticker_Ink : Sticker_White));
		Style.SetHoveredForeground(FSlateColor(Sticker_Ink));
		Style.SetPressedForeground(FSlateColor(Sticker_Ink));
		Style.SetDisabledForeground(FSlateColor(Sticker_SubText));
		Style.SetNormalPadding(FMargin(20.f, 7.f, 20.f, 9.f));
		Style.SetPressedPadding(FMargin(20.f, 9.f, 20.f, 7.f));
		return Style;
	}

	UButton* MakeStickerButton(UWidgetTree* Tree, FName Name, const FText& Label, const FLinearColor& Accent, float FontSize, bool bFilled)
	{
		UButton* Button = Tree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
		Button->SetStyle(MakeStickerButtonStyle(Accent, bFilled));

		UTextBlock* ButtonText = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *(Name.ToString() + TEXT("_Label")));
		ButtonText->SetText(Label);
		ButtonText->SetFont(HeadingFont(FMath::RoundToInt(FontSize)));
		ButtonText->SetJustification(ETextJustify::Center);
		// Follows the button style's foreground: white normally, dark on a filled/hovered sticker.
		ButtonText->SetColorAndOpacity(FSlateColor::UseForeground());

		UJinzzaUIButtonSounds* SoundBinder = NewObject<UJinzzaUIButtonSounds>(Button);
		Button->OnClicked.AddDynamic(SoundBinder, &UJinzzaUIButtonSounds::HandleClicked);
		Button->OnHovered.AddDynamic(SoundBinder, &UJinzzaUIButtonSounds::HandleHovered);

		Button->AddChild(ButtonText);
		return Button;
	}

	void SetStickerButtonSelected(UButton* Button, const FLinearColor& Accent, bool bSelected)
	{
		if (Button)
		{
			Button->SetStyle(MakeStickerButtonStyle(Accent, bSelected));
		}
	}

	UTextBlock* MakeStickerHeading(UWidgetTree* Tree, FName Name, const FText& Text, int32 Size)
	{
		UTextBlock* Heading = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		Heading->SetText(Text);
		Heading->SetFont(HeadingFont(Size));
		Heading->SetColorAndOpacity(FSlateColor(Sticker_White));
		return Heading;
	}

	UWidget* MakeStickerDot(UWidgetTree* Tree, FName Name, const FLinearColor& Color, float Size)
	{
		USizeBox* Box = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), Name);
		Box->SetWidthOverride(Size);
		Box->SetHeightOverride(Size);
		UBorder* Dot = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), *(Name.ToString() + TEXT("_Shape")));
		Dot->SetBrush(FSlateRoundedBoxBrush(Color, Size * 0.5f, Sticker_White, 2.f));
		Box->AddChild(Dot);
		return Box;
	}

	void ApplyStickerStyle(USlider* Slider)
	{
		if (!Slider)
		{
			return;
		}

		// Chunky cartoon slider: a thick rounded bar and a big round yellow thumb with a white
		// outline (the logo's outline look). Colors are baked into the brushes, so the tints stay white.
		const FVector2D ThumbSize(30.f, 30.f);
		FSliderStyle Style = FCoreStyle::Get().GetWidgetStyle<FSliderStyle>("Slider");
		Style.SetNormalBarImage(FSlateRoundedBoxBrush(Sticker_White.CopyWithNewOpacity(0.35f), 6.f));
		Style.SetHoveredBarImage(FSlateRoundedBoxBrush(Sticker_White.CopyWithNewOpacity(0.5f), 6.f));
		Style.SetDisabledBarImage(FSlateRoundedBoxBrush(Sticker_White.CopyWithNewOpacity(0.15f), 6.f));
		Style.SetNormalThumbImage(MakeStickerBrush(Sticker_Yellow, ThumbSize.X * 0.5f, 3.f, ThumbSize));
		Style.SetHoveredThumbImage(MakeStickerBrush(FMath::Lerp(Sticker_Yellow, Sticker_White, 0.3f), ThumbSize.X * 0.5f, 3.f, ThumbSize));
		Style.SetDisabledThumbImage(MakeStickerBrush(Sticker_SubText, ThumbSize.X * 0.5f, 3.f, ThumbSize));
		Style.SetBarThickness(10.f);
		Slider->SetWidgetStyle(Style);
		Slider->SetSliderBarColor(FLinearColor::White);
		Slider->SetSliderHandleColor(FLinearColor::White);
	}

	void ApplyStickerStyle(UCheckBox* CheckBox)
	{
		if (!CheckBox)
		{
			return;
		}

		// A round-cornered box: black with a white outline when off, filled yellow when on.
		const FVector2D BoxSize(30.f, 30.f);
		const FLinearColor OffHover = FLinearColor::FromSRGBColor(FColor(55, 55, 62));
		FCheckBoxStyle Style = FCoreStyle::Get().GetWidgetStyle<FCheckBoxStyle>("Checkbox");
		Style.SetCheckBoxType(ESlateCheckBoxType::CheckBox);
		Style.SetUncheckedImage(MakeStickerBrush(Sticker_Ink, 9.f, 3.f, BoxSize));
		Style.SetUncheckedHoveredImage(MakeStickerBrush(OffHover, 9.f, 3.f, BoxSize));
		Style.SetUncheckedPressedImage(MakeStickerBrush(OffHover, 9.f, 3.f, BoxSize));
		Style.SetCheckedImage(MakeStickerBrush(Sticker_Yellow, 9.f, 3.f, BoxSize));
		Style.SetCheckedHoveredImage(MakeStickerBrush(FMath::Lerp(Sticker_Yellow, Sticker_White, 0.3f), 9.f, 3.f, BoxSize));
		Style.SetCheckedPressedImage(MakeStickerBrush(Sticker_Yellow, 9.f, 3.f, BoxSize));
		Style.SetUndeterminedImage(MakeStickerBrush(Sticker_Sky, 9.f, 3.f, BoxSize));
		CheckBox->SetWidgetStyle(Style);
	}

	void ApplyStickerStyle(UComboBoxString* ComboBox)
	{
		if (!ComboBox)
		{
			return;
		}

		FComboBoxStyle Style = FCoreStyle::Get().GetWidgetStyle<FComboBoxStyle>("ComboBox");
		FButtonStyle ButtonStyle = Style.ComboButtonStyle.ButtonStyle;
		ButtonStyle.SetNormal(MakeStickerBrush(Sticker_Ink, StickerFieldRadius));
		ButtonStyle.SetHovered(MakeStickerBrush(FLinearColor::FromSRGBColor(FColor(45, 45, 52)), StickerFieldRadius, 3.f));
		ButtonStyle.SetPressed(MakeStickerBrush(FLinearColor::FromSRGBColor(FColor(45, 45, 52)), StickerFieldRadius, 3.f));
		ButtonStyle.SetNormalForeground(FSlateColor(Sticker_White));
		ButtonStyle.SetHoveredForeground(FSlateColor(Sticker_White));
		ButtonStyle.SetPressedForeground(FSlateColor(Sticker_White));
		Style.ComboButtonStyle.SetButtonStyle(ButtonStyle);
		ComboBox->SetWidgetStyle(Style);
		ComboBox->SetContentPadding(FMargin(16.f, 8.f));
		// Font/foreground only take effect before the Slate widget is built (i.e. right after
		// ConstructWidget, which is when every caller styles its combos). The Init* setters are
		// protected, so this writes the (deprecated-for-direct-access) properties themselves.
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
		ComboBox->Font = BodyFont(20);
		ComboBox->ForegroundColor = FSlateColor(Sticker_White);
		PRAGMA_ENABLE_DEPRECATION_WARNINGS
	}

	void ApplyStickerStyle(USpinBox* SpinBox)
	{
		if (!SpinBox)
		{
			return;
		}

		FSpinBoxStyle Style = FCoreStyle::Get().GetWidgetStyle<FSpinBoxStyle>("SpinBox");
		Style.SetBackgroundBrush(MakeStickerBrush(Sticker_Ink, StickerFieldRadius));
		Style.SetHoveredBackgroundBrush(MakeStickerBrush(FLinearColor::FromSRGBColor(FColor(45, 45, 52)), StickerFieldRadius));
		// The value-proportional fill bar inside the field.
		Style.SetActiveFillBrush(FSlateRoundedBoxBrush(Sticker_Yellow.CopyWithNewOpacity(0.55f), StickerFieldRadius));
		Style.SetInactiveFillBrush(FSlateRoundedBoxBrush(Sticker_Yellow.CopyWithNewOpacity(0.35f), StickerFieldRadius));
		Style.SetForegroundColor(FSlateColor(Sticker_White));
		Style.SetTextPadding(FMargin(14.f, 5.f));
		SpinBox->SetWidgetStyle(Style);
		SpinBox->SetFont(BodyFont(20));
	}

	void ApplyStickerStyle(UEditableTextBox* TextBox)
	{
		if (!TextBox)
		{
			return;
		}

		FEditableTextBoxStyle Style = FCoreStyle::Get().GetWidgetStyle<FEditableTextBoxStyle>("NormalEditableTextBox");
		Style.SetBackgroundImageNormal(MakeStickerBrush(Sticker_Ink, StickerFieldRadius));
		Style.SetBackgroundImageHovered(MakeStickerBrush(FLinearColor::FromSRGBColor(FColor(45, 45, 52)), StickerFieldRadius));
		// Yellow outline while typing.
		Style.SetBackgroundImageFocused(FSlateRoundedBoxBrush(Sticker_Ink, StickerFieldRadius, Sticker_Yellow, 3.f));
		Style.SetBackgroundImageReadOnly(MakeStickerBrush(FLinearColor::FromSRGBColor(FColor(40, 40, 46)), StickerFieldRadius));
		Style.SetBackgroundColor(FSlateColor(FLinearColor::White));
		Style.SetForegroundColor(FSlateColor(Sticker_White));
		Style.SetFocusedForegroundColor(FSlateColor(Sticker_White));
		Style.SetReadOnlyForegroundColor(FSlateColor(Sticker_SubText));
		Style.SetFont(BodyFont(20));
		Style.SetPadding(FMargin(14.f, 8.f));
		TextBox->SetWidgetStyle(Style);
	}

	void AddStickerSection(UWidgetTree* Tree, UVerticalBox* Page, FName Name, const FText& Title, bool bFirst)
	{
		UOverlay* Bubble = Tree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), Name);

		// The bubble's tail: a small white square turned 45 degrees, poking out bottom-left.
		USizeBox* TailBox = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), *(Name.ToString() + TEXT("_Tail")));
		TailBox->SetWidthOverride(16.f);
		TailBox->SetHeightOverride(16.f);
		TailBox->SetRenderTransformAngle(45.f);
		TailBox->SetRenderTranslation(FVector2D(0.f, 7.f));
		UBorder* Tail = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), *(Name.ToString() + TEXT("_TailShape")));
		Tail->SetBrush(FSlateRoundedBoxBrush(Sticker_White, 3.f));
		TailBox->AddChild(Tail);
		if (UOverlaySlot* TailSlot = Bubble->AddChildToOverlay(TailBox))
		{
			TailSlot->SetHorizontalAlignment(HAlign_Left);
			TailSlot->SetVerticalAlignment(VAlign_Bottom);
			TailSlot->SetPadding(FMargin(22.f, 0.f, 0.f, 0.f));
		}

		UBorder* Body = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), *(Name.ToString() + TEXT("_Body")));
		Body->SetBrush(FSlateRoundedBoxBrush(Sticker_White, 22.f));
		Body->SetPadding(FMargin(20.f, 4.f, 20.f, 6.f));
		UTextBlock* TitleText = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *(Name.ToString() + TEXT("_Title")));
		TitleText->SetText(Title);
		TitleText->SetFont(HeadingFont(22));
		TitleText->SetColorAndOpacity(FSlateColor(Sticker_Ink));
		Body->SetContent(TitleText);
		Bubble->AddChildToOverlay(Body);

		if (UVerticalBoxSlot* BubbleSlot = Page->AddChildToVerticalBox(Bubble))
		{
			BubbleSlot->SetHorizontalAlignment(HAlign_Left);
			BubbleSlot->SetPadding(FMargin(4.f, bFirst ? 4.f : 30.f, 0.f, 8.f));
		}
	}

	UHorizontalBox* AddStickerRow(UWidgetTree* Tree, UVerticalBox* Page, FName Name, const FText& Label, UWidget* Control,
		float ControlWidth, int32 LabelFontSize)
	{
		UHorizontalBox* Row = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), Name);
		if (UVerticalBoxSlot* RowSlot = Page->AddChildToVerticalBox(Row))
		{
			RowSlot->SetPadding(FMargin(8.f, 14.f, 16.f, 0.f));
		}

		UTextBlock* LabelText = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *(Name.ToString() + TEXT("_Label")));
		LabelText->SetText(Label);
		LabelText->SetFont(HeadingFont(LabelFontSize));
		LabelText->SetColorAndOpacity(FSlateColor(Sticker_White));
		if (UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(LabelText))
		{
			LabelSlot->SetSize(ESlateSizeRule::Fill);
			LabelSlot->SetVerticalAlignment(VAlign_Center);
			LabelSlot->SetPadding(FMargin(0.f, 0.f, 20.f, 0.f));
		}

		USizeBox* ControlBox = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), *(Name.ToString() + TEXT("_ControlBox")));
		ControlBox->SetWidthOverride(ControlWidth);
		ControlBox->SetMinDesiredHeight(48.f);
		if (Control)
		{
			ControlBox->AddChild(Control);
		}
		if (UHorizontalBoxSlot* ControlSlot = Row->AddChildToHorizontalBox(ControlBox))
		{
			ControlSlot->SetVerticalAlignment(VAlign_Center);
		}
		return Row;
	}

	UTextBlock* MakeStickerText(UWidgetTree* Tree, FName Name, const FText& Text, int32 Size, bool bMuted)
	{
		UTextBlock* Body = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		Body->SetText(Text);
		Body->SetFont(BodyFont(Size));
		Body->SetColorAndOpacity(FSlateColor(bMuted ? Sticker_SubText : Sticker_White));
		return Body;
	}

	UWidget* MakeStickerKeyCap(UWidgetTree* Tree, FName Name, UTextBlock*& OutKeyText, float Height)
	{
		USizeBox* Box = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), Name);
		Box->SetHeightOverride(Height);
		Box->SetMinDesiredWidth(Height);

		UBorder* Cap = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), *(Name.ToString() + TEXT("_Cap")));
		Cap->SetBrush(FSlateRoundedBoxBrush(Sticker_White, 9.f, Sticker_Yellow, 3.f));
		Cap->SetPadding(FMargin(9.f, 0.f));
		Cap->SetHorizontalAlignment(HAlign_Center);
		Cap->SetVerticalAlignment(VAlign_Center);
		Box->AddChild(Cap);

		OutKeyText = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *(Name.ToString() + TEXT("_Key")));
		OutKeyText->SetFont(HeadingFont(FMath::RoundToInt(Height * 0.5f)));
		OutKeyText->SetColorAndOpacity(FSlateColor(Sticker_Ink));
		OutKeyText->SetJustification(ETextJustify::Center);
		Cap->SetContent(OutKeyText);
		return Box;
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
