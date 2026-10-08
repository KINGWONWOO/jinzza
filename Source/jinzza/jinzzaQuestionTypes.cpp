// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaQuestionTypes.h"
#include "GameFramework/PlayerState.h"
#include "Layout/Geometry.h"
#include "Rendering/DrawElements.h"
#include "Styling/SlateBrush.h"

int32 FJinzzaDrawing::CountPoints() const
{
	int32 Count = 0;
	for (const FJinzzaDrawStroke& Stroke : Strokes)
	{
		Count += Stroke.NumPoints();
	}
	return Count;
}

const FJinzzaQuestionSeat* FJinzzaQuestionState::FindSeat(const APlayerState* PlayerState) const
{
	return PlayerState ? Seats.FindByPredicate([PlayerState](const FJinzzaQuestionSeat& Seat) { return Seat.Player == PlayerState; }) : nullptr;
}

bool FJinzzaQuestionState::IsAnswerer(const APlayerState* PlayerState) const
{
	const FJinzzaQuestionSeat* Seat = FindSeat(PlayerState);
	return Seat && Seat->bAnswerer;
}

int32 FJinzzaQuestionState::CountAnswerers() const
{
	int32 Count = 0;
	for (const FJinzzaQuestionSeat& Seat : Seats)
	{
		Count += Seat.bAnswerer ? 1 : 0;
	}
	return Count;
}

namespace JinzzaSketch
{
	const FLinearColor& GetPaperColor()
	{
		// Sketchbook paper, a touch warm so it isn't glaring next to the cartoon lighting.
		static const FLinearColor Paper(0.97f, 0.96f, 0.92f, 1.f);
		return Paper;
	}

	const TArray<FLinearColor>& GetPalette()
	{
		static const TArray<FLinearColor> Palette = {
			GetPaperColor(),                        // 0: eraser
			FLinearColor(0.04f, 0.04f, 0.05f),      // black marker
			FLinearColor(0.86f, 0.10f, 0.12f),      // red
			FLinearColor(0.10f, 0.35f, 0.90f),      // blue
			FLinearColor(0.10f, 0.62f, 0.25f),      // green
			FLinearColor(1.00f, 0.62f, 0.05f),      // orange
			FLinearColor(0.55f, 0.20f, 0.80f),      // purple
			FLinearColor(1.00f, 0.42f, 0.70f),      // pink
			FLinearColor(0.45f, 0.27f, 0.12f),      // brown
		};
		return Palette;
	}

	void Sanitize(FJinzzaDrawing& Drawing)
	{
		if (Drawing.Strokes.Num() > MaxStrokes)
		{
			Drawing.Strokes.SetNum(MaxStrokes);
		}

		const int32 PaletteNum = GetPalette().Num();
		const uint16 MaxX = static_cast<uint16>(CanvasSize.X);
		const uint16 MaxY = static_cast<uint16>(CanvasSize.Y);
		int32 Budget = MaxPoints;
		for (int32 Index = 0; Index < Drawing.Strokes.Num(); ++Index)
		{
			FJinzzaDrawStroke& Stroke = Drawing.Strokes[Index];
			Stroke.Color = static_cast<uint8>(FMath::Min<int32>(Stroke.Color, PaletteNum - 1));
			Stroke.Size = static_cast<uint8>(FMath::Min<int32>(Stroke.Size, NumBrushSizes - 1));
			if (Stroke.Points.Num() % 2 != 0)
			{
				Stroke.Points.Pop();
			}
			if (Stroke.NumPoints() > MaxPointsPerStroke)
			{
				Stroke.Points.SetNum(MaxPointsPerStroke * 2);
			}
			if (Stroke.NumPoints() > Budget)
			{
				Stroke.Points.SetNum(Budget * 2);
			}
			Budget -= Stroke.NumPoints();
			for (int32 P = 0; P < Stroke.Points.Num(); P += 2)
			{
				Stroke.Points[P] = FMath::Min(Stroke.Points[P], MaxX);
				Stroke.Points[P + 1] = FMath::Min(Stroke.Points[P + 1], MaxY);
			}
		}
		Drawing.Strokes.RemoveAll([](const FJinzzaDrawStroke& Stroke) { return Stroke.Points.Num() == 0; });
	}

	int32 Paint(const FJinzzaDrawing& Drawing, const FGeometry& Geometry, FSlateWindowElementList& OutDrawElements, int32 LayerId, float Opacity)
	{
		static FSlateBrush DotBrush = []()
		{
			FSlateBrush Brush;
			Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
			Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::HalfHeightRadius;
			Brush.TintColor = FSlateColor(FLinearColor::White);
			return Brush;
		}();

		const FVector2D LocalSize = Geometry.GetLocalSize();
		const double Scale = FMath::Min(LocalSize.X / CanvasSize.X, LocalSize.Y / CanvasSize.Y);
		const FVector2D Offset = (LocalSize - CanvasSize * Scale) * 0.5;
		const TArray<FLinearColor>& Palette = GetPalette();

		auto ToLocal = [&](const FVector2D& CanvasPoint) { return Offset + CanvasPoint * Scale; };
		auto Dot = [&](const FVector2D& Center, float Diameter, const FLinearColor& Color)
		{
			const FVector2f Size(Diameter, Diameter);
			FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
				Geometry.ToPaintGeometry(Size, FSlateLayoutTransform(FVector2f(Center) - Size * 0.5f)),
				&DotBrush, ESlateDrawEffect::None, Color);
		};

		TArray<FVector2D> Line;
		for (const FJinzzaDrawStroke& Stroke : Drawing.Strokes)
		{
			const int32 Count = Stroke.NumPoints();
			if (Count == 0)
			{
				continue;
			}
			// One layer per stroke: Slate may reorder elements within a layer, and the eraser (and any
			// overlapping color) must land on top of what was drawn before it.
			++LayerId;
			FLinearColor Color = Palette.IsValidIndex(Stroke.Color) ? Palette[Stroke.Color] : Palette[1];
			Color.A *= Opacity;
			const float Width = BrushSizes[FMath::Clamp<int32>(Stroke.Size, 0, NumBrushSizes - 1)] * Scale;

			// Round caps: a dot at each end (and a lone dot for a single tap).
			Dot(ToLocal(Stroke.GetPoint(0)), Width, Color);
			if (Count == 1)
			{
				continue;
			}
			Dot(ToLocal(Stroke.GetPoint(Count - 1)), Width, Color);

			Line.Reset(Count);
			for (int32 Index = 0; Index < Count; ++Index)
			{
				Line.Add(ToLocal(Stroke.GetPoint(Index)));
			}
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId, Geometry.ToPaintGeometry(), Line, ESlateDrawEffect::None, Color, true, Width);
		}
		return LayerId;
	}
}
