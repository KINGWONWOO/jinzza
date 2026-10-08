// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaSketchPad.h"
#include "jinzzaUIStyle.h"
#include "Components/Border.h"
#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Rendering/DrawElements.h"

namespace
{
	/** A new point is only recorded this far (canvas units) from the last one - keeps drawings small. */
	constexpr double SketchMinPointSpacing = 3.0;
}

void UjinzzaSketchPad::BuildWidgetTree()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	// The paper itself; strokes are painted on top of it in NativePaint.
	Paper = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Paper"));
	Paper->SetBrush(FSlateRoundedBoxBrush(JinzzaSketch::GetPaperColor(), 14.f));
	Paper->SetVisibility(ESlateVisibility::Visible);
	WidgetTree->RootWidget = Paper;
}

void UjinzzaSketchPad::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	BuildWidgetTree();
	SetClipping(EWidgetClipping::ClipToBounds);
}

void UjinzzaSketchPad::SetEditable(bool bInEditable)
{
	bEditable = bInEditable;
	if (!bEditable)
	{
		EndStroke();
		bHovering = false;
	}
}

void UjinzzaSketchPad::SetDrawing(const FJinzzaDrawing& InDrawing)
{
	EndStroke();
	Drawing = InDrawing;
}

void UjinzzaSketchPad::Undo()
{
	EndStroke();
	if (Drawing.Strokes.Num() > 0)
	{
		Drawing.Strokes.Pop();
		OnChanged.Broadcast();
	}
}

void UjinzzaSketchPad::Clear()
{
	EndStroke();
	if (Drawing.Strokes.Num() > 0)
	{
		Drawing.Strokes.Reset();
		OnChanged.Broadcast();
	}
}

float UjinzzaSketchPad::GetInkUsed() const
{
	return FMath::Clamp(static_cast<float>(Drawing.CountPoints()) / JinzzaSketch::MaxPoints, 0.f, 1.f);
}

FVector2D UjinzzaSketchPad::LocalToCanvas(const FGeometry& Geometry, const FVector2D& Local) const
{
	// Same fit as JinzzaSketch::Paint.
	const FVector2D LocalSize = Geometry.GetLocalSize();
	const double Scale = FMath::Max(KINDA_SMALL_NUMBER, FMath::Min(LocalSize.X / JinzzaSketch::CanvasSize.X, LocalSize.Y / JinzzaSketch::CanvasSize.Y));
	const FVector2D Offset = (LocalSize - JinzzaSketch::CanvasSize * Scale) * 0.5;
	const FVector2D Canvas = (Local - Offset) / Scale;
	return FVector2D(FMath::Clamp(Canvas.X, 0.0, JinzzaSketch::CanvasSize.X), FMath::Clamp(Canvas.Y, 0.0, JinzzaSketch::CanvasSize.Y));
}

bool UjinzzaSketchPad::AddPoint(const FVector2D& CanvasPoint)
{
	if (Drawing.Strokes.Num() == 0 || Drawing.CountPoints() >= JinzzaSketch::MaxPoints)
	{
		return false;
	}

	FJinzzaDrawStroke* Stroke = &Drawing.Strokes.Last();
	if (Stroke->NumPoints() > 0 && FVector2D::Distance(Stroke->GetPoint(Stroke->NumPoints() - 1), CanvasPoint) < SketchMinPointSpacing)
	{
		return false;
	}
	// A very long stroke carries on as a new one from the same spot (see JinzzaSketch::MaxPointsPerStroke).
	if (Stroke->NumPoints() >= JinzzaSketch::MaxPointsPerStroke)
	{
		if (Drawing.Strokes.Num() >= JinzzaSketch::MaxStrokes)
		{
			return false;
		}
		const FJinzzaDrawStroke Previous = *Stroke;
		Stroke = &Drawing.Strokes.AddDefaulted_GetRef();
		Stroke->Color = Previous.Color;
		Stroke->Size = Previous.Size;
		Stroke->Points.Add(Previous.Points[Previous.Points.Num() - 2]);
		Stroke->Points.Add(Previous.Points[Previous.Points.Num() - 1]);
	}
	Stroke->Points.Add(static_cast<uint16>(FMath::RoundToInt(CanvasPoint.X)));
	Stroke->Points.Add(static_cast<uint16>(FMath::RoundToInt(CanvasPoint.Y)));
	return true;
}

void UjinzzaSketchPad::EndStroke()
{
	if (!bStroking)
	{
		return;
	}
	bStroking = false;
	OnChanged.Broadcast();
}

FReply UjinzzaSketchPad::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (!bEditable || InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	}
	if (Drawing.Strokes.Num() >= JinzzaSketch::MaxStrokes || Drawing.CountPoints() >= JinzzaSketch::MaxPoints)
	{
		// Out of ink - the answer screen shows the full ink meter.
		return FReply::Handled();
	}

	FJinzzaDrawStroke& Stroke = Drawing.Strokes.AddDefaulted_GetRef();
	Stroke.Color = static_cast<uint8>(ColorIndex);
	Stroke.Size = static_cast<uint8>(SizeIndex);
	AddPoint(LocalToCanvas(InGeometry, InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition())));
	bStroking = true;

	TSharedPtr<SWidget> Cached = GetCachedWidget();
	return Cached.IsValid() ? FReply::Handled().CaptureMouse(Cached.ToSharedRef()) : FReply::Handled();
}

FReply UjinzzaSketchPad::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	HoverLocal = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
	if (bEditable && bStroking)
	{
		AddPoint(LocalToCanvas(InGeometry, HoverLocal));
		return FReply::Handled();
	}
	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

FReply UjinzzaSketchPad::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bStroking && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		EndStroke();
		return FReply::Handled().ReleaseMouseCapture();
	}
	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

void UjinzzaSketchPad::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
	bHovering = true;
	HoverLocal = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
}

void UjinzzaSketchPad::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);
	bHovering = false;
}

void UjinzzaSketchPad::NativeOnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent)
{
	Super::NativeOnMouseCaptureLost(CaptureLostEvent);
	EndStroke();
}

int32 UjinzzaSketchPad::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	int32 MaxLayer = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
	MaxLayer = JinzzaSketch::Paint(Drawing, AllottedGeometry, OutDrawElements, MaxLayer, InWidgetStyle.GetColorAndOpacityTint().A);

	// Brush preview: a ring the size of the pen in its color, so you see what you're about to draw.
	if (bEditable && bHovering)
	{
		static FSlateBrush RingBrush = []()
		{
			FSlateBrush Brush;
			Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
			Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::HalfHeightRadius;
			Brush.OutlineSettings.Width = 2.f;
			Brush.OutlineSettings.Color = FSlateColor(JinzzaUI::Sticker_Ink);
			Brush.TintColor = FSlateColor(FLinearColor::White);
			return Brush;
		}();

		const FVector2D LocalSize = AllottedGeometry.GetLocalSize();
		const double Scale = FMath::Min(LocalSize.X / JinzzaSketch::CanvasSize.X, LocalSize.Y / JinzzaSketch::CanvasSize.Y);
		const float Diameter = FMath::Max(8.f, JinzzaSketch::BrushSizes[FMath::Clamp(SizeIndex, 0, JinzzaSketch::NumBrushSizes - 1)] * static_cast<float>(Scale));
		const FVector2f Size(Diameter, Diameter);
		const TArray<FLinearColor>& Palette = JinzzaSketch::GetPalette();
		const FLinearColor Fill = Palette.IsValidIndex(ColorIndex) ? Palette[ColorIndex] : Palette[1];
		FSlateDrawElement::MakeBox(OutDrawElements, ++MaxLayer,
			AllottedGeometry.ToPaintGeometry(Size, FSlateLayoutTransform(FVector2f(HoverLocal) - Size * 0.5f)),
			&RingBrush, ESlateDrawEffect::None, Fill.CopyWithNewOpacity(0.55f));
	}
	return MaxLayer;
}
