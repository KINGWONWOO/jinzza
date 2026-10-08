// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "jinzzaQuestionTypes.h"
#include "jinzzaSketchPad.generated.h"

class UBorder;

DECLARE_MULTICAST_DELEGATE(FOnJinzzaSketchChanged);

/**
 * A sketchbook page: paper plus FJinzzaDrawing strokes drawn on top. Editable = mouse drawing with the
 * current color/size (Question Time's answer screen); otherwise it just shows a drawing (the in-world
 * answer board). Strokes are kept in JinzzaSketch::CanvasSize coordinates and scaled to fit (4:3).
 */
UCLASS()
class JINZZA_API UjinzzaSketchPad : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeOnInitialized() override;

	void SetEditable(bool bInEditable);
	bool IsEditable() const { return bEditable; }

	void SetDrawing(const FJinzzaDrawing& InDrawing);
	const FJinzzaDrawing& GetDrawing() const { return Drawing; }

	void SetColorIndex(int32 Index) { ColorIndex = Index; }
	int32 GetColorIndex() const { return ColorIndex; }
	void SetSizeIndex(int32 Index) { SizeIndex = Index; }
	int32 GetSizeIndex() const { return SizeIndex; }

	void Undo();
	void Clear();

	/** 0..1 of JinzzaSketch::MaxPoints used. */
	float GetInkUsed() const;

	/** Fired after every finished stroke, undo and clear (editable pads only). */
	FOnJinzzaSketchChanged OnChanged;

protected:
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent) override;

private:
	void BuildWidgetTree();
	FVector2D LocalToCanvas(const FGeometry& Geometry, const FVector2D& Local) const;
	bool AddPoint(const FVector2D& CanvasPoint);
	void EndStroke();

	UPROPERTY()
	TObjectPtr<UBorder> Paper;

	FJinzzaDrawing Drawing;
	bool bEditable = false;
	bool bStroking = false;
	int32 ColorIndex = 1;
	int32 SizeIndex = 1;

	/** Brush preview under the mouse (local space), editable pads only. */
	bool bHovering = false;
	FVector2D HoverLocal = FVector2D::ZeroVector;
};
