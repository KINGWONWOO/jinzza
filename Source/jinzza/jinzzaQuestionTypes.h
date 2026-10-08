// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "jinzzaQuestionTypes.generated.h"

class APlayerState;
class FSlateWindowElementList;
struct FGeometry;

/**
 * Question Time (AjinzzaGameGameMode's question cycle, replicated through AjinzzaGameGameState). Each cycle:
 * Asking (the Judge types a question, everyone can talk) -> Revealing (the question sign drops down) ->
 * Answering (candidates draw their answer on a sketch board) -> Showing (boards flip round one by one,
 * everyone discusses). Everyone is seated and can't move for the whole phase.
 */
UENUM(BlueprintType)
enum class EJinzzaQuestionStep : uint8
{
	None,
	Asking,
	Revealing,
	Answering,
	Showing
};

/** One pen stroke on a sketch board. Points are canvas coordinates (JinzzaSketch::CanvasSize), X/Y interleaved. */
USTRUCT(BlueprintType)
struct FJinzzaDrawStroke
{
	GENERATED_BODY()

	/** Index into JinzzaSketch::GetPalette() (0 = eraser, i.e. the board's own color). */
	UPROPERTY()
	uint8 Color = 1;

	/** Index into JinzzaSketch::BrushSizes. */
	UPROPERTY()
	uint8 Size = 1;

	UPROPERTY()
	TArray<uint16> Points;

	int32 NumPoints() const { return Points.Num() / 2; }
	FVector2D GetPoint(int32 Index) const { return FVector2D(Points[Index * 2], Points[Index * 2 + 1]); }
};

/** A whole answer drawing. Empty = no answer. */
USTRUCT(BlueprintType)
struct FJinzzaDrawing
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FJinzzaDrawStroke> Strokes;

	bool IsEmpty() const { return Strokes.Num() == 0; }
	int32 CountPoints() const;
};

/** Where one player sits for Question Time, and the camera that films them for the split screen. */
USTRUCT(BlueprintType)
struct FJinzzaQuestionSeat
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<APlayerState> Player;

	/** False for the Judge (asks, doesn't answer). */
	UPROPERTY()
	bool bAnswerer = true;

	UPROPERTY()
	FVector CameraLocation = FVector::ZeroVector;

	UPROPERTY()
	FRotator CameraRotation = FRotator::ZeroRotator;
};

/** Replicated as one unit. */
USTRUCT(BlueprintType)
struct FJinzzaQuestionState
{
	GENERATED_BODY()

	UPROPERTY()
	EJinzzaQuestionStep Step = EJinzzaQuestionStep::None;

	/** 1-based. */
	UPROPERTY()
	int32 Cycle = 0;

	UPROPERTY()
	int32 TotalCycles = 0;

	/** Empty until the question is revealed - nobody sees it while the Judge is still typing. */
	UPROPERTY()
	FString Question;

	/** Server world time the current step's visible countdown ends at. */
	UPROPERTY()
	double StepEndServerTime = 0.0;

	/** Server world time Revealing started at (drives the sign's drop animation on every machine alike). */
	UPROPERTY()
	double RevealServerTime = 0.0;

	/** Seat order = split-screen panel order: the Judge first, then User1..UserN. */
	UPROPERTY()
	TArray<FJinzzaQuestionSeat> Seats;

	/** Answerers who have handed in their drawing this cycle (content stays on the server until Showing). */
	UPROPERTY()
	TArray<TObjectPtr<APlayerState>> Submitted;

	/** Bumped for every step change, so back-to-back steps are told apart even if replication merges them. */
	UPROPERTY()
	int32 Serial = 0;

	bool IsActive() const { return Step != EJinzzaQuestionStep::None; }
	const FJinzzaQuestionSeat* FindSeat(const APlayerState* PlayerState) const;
	bool IsAnswerer(const APlayerState* PlayerState) const;
	bool HasSubmitted(const APlayerState* PlayerState) const { return PlayerState && Submitted.Contains(PlayerState); }
	int32 CountAnswerers() const;
};

/** Sketch board constants shared by the drawing UI, the in-world board and the server's validation. */
namespace JinzzaSketch
{
	/** Logical canvas size every drawing is stored in (4:3, scaled to whatever it's shown on). */
	inline const FVector2D CanvasSize(800.0, 600.0);

	/** Pen widths in canvas units: small / medium / large. */
	inline constexpr float BrushSizes[] = { 5.f, 11.f, 24.f };
	inline constexpr int32 NumBrushSizes = UE_ARRAY_COUNT(BrushSizes);

	/** Server limits - keep a drawing comfortably inside one reliable RPC. */
	inline constexpr int32 MaxStrokes = 400;
	inline constexpr int32 MaxPoints = 3000;
	/** Per stroke (the pad starts a new stroke past this): the engine refuses to send an array of more than
	 * net.MaxRepArraySize (2048) elements, and each point is two. */
	inline constexpr int32 MaxPointsPerStroke = 1000;

	/** The board's paper color (also what the eraser paints with). */
	JINZZA_API const FLinearColor& GetPaperColor();

	/** Index 0 is the eraser (paper color); 1.. are the marker colors. */
	JINZZA_API const TArray<FLinearColor>& GetPalette();

	/** Clamps colors/sizes/coordinates and cuts anything over MaxStrokes/MaxPoints. */
	JINZZA_API void Sanitize(FJinzzaDrawing& Drawing);

	/**
	 * Draws Drawing (canvas space) into Geometry, scaled to fit, one layer per stroke above LayerId.
	 * Single-point strokes are drawn as dots. Returns the last layer used.
	 */
	JINZZA_API int32 Paint(const FJinzzaDrawing& Drawing, const FGeometry& Geometry, FSlateWindowElementList& OutDrawElements, int32 LayerId, float Opacity = 1.f);
}
