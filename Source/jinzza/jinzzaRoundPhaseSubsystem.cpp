// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaRoundPhaseSubsystem.h"
#include "jinzzaGameGameState.h"
#include "jinzzaGameInstance.h"
#include "jinzzaMatchSettings.h"
#include "Engine/World.h"
#include "TimerManager.h"

void UjinzzaRoundPhaseSubsystem::StartRound()
{
	const UWorld* World = GetWorld();
	if (!World || !World->GetAuthGameMode())
	{
		// Not the server - clients only ever observe phase state via replicated AjinzzaGameGameState.
		return;
	}

	TotalMidEvaluations = FMath::Clamp(GetMatchSettings().VoteCount, 1, 3);
	MidEvaluationsCompleted = 0;

	EnterPhase(EJinzzaRoundPhase::RoleAssignment);
}

void UjinzzaRoundPhaseSubsystem::EnterPhase(EJinzzaRoundPhase NewPhase)
{
	CurrentPhase = NewPhase;

	const float Duration = GetPhaseDurationSeconds(NewPhase);
	const int32 MidEvalsRemaining = FMath::Max(0, TotalMidEvaluations - MidEvaluationsCompleted);

	if (AjinzzaGameGameState* GameState = GetJinzzaGameState())
	{
		GameState->ServerSetPhase(NewPhase, Duration, MidEvalsRemaining);
	}

	OnServerPhaseEntered.Broadcast(NewPhase);

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (NewPhase == EJinzzaRoundPhase::RoundComplete)
	{
		World->GetTimerManager().ClearTimer(PhaseTimerHandle);
		return;
	}

	World->GetTimerManager().SetTimer(PhaseTimerHandle, this, &UjinzzaRoundPhaseSubsystem::AdvancePhase, FMath::Max(0.1f, Duration), false);
}

void UjinzzaRoundPhaseSubsystem::AdvancePhase()
{
	EJinzzaRoundPhase NextPhase = EJinzzaRoundPhase::RoundComplete;

	switch (CurrentPhase)
	{
	case EJinzzaRoundPhase::RoleAssignment:
		NextPhase = EJinzzaRoundPhase::SelfIntroduction;
		break;
	case EJinzzaRoundPhase::SelfIntroduction:
		NextPhase = EJinzzaRoundPhase::FreeTime1;
		break;
	case EJinzzaRoundPhase::FreeTime1:
		NextPhase = EJinzzaRoundPhase::QuestionTime;
		break;
	case EJinzzaRoundPhase::QuestionTime:
		NextPhase = EJinzzaRoundPhase::MidEvaluation;
		break;
	case EJinzzaRoundPhase::MidEvaluation:
		++MidEvaluationsCompleted;
		NextPhase = EJinzzaRoundPhase::FreeTime2;
		break;
	case EJinzzaRoundPhase::FreeTime2:
		NextPhase = (MidEvaluationsCompleted < TotalMidEvaluations) ? EJinzzaRoundPhase::MidEvaluation : EJinzzaRoundPhase::Interview;
		break;
	case EJinzzaRoundPhase::Interview:
		NextPhase = EJinzzaRoundPhase::FinalDecision;
		break;
	case EJinzzaRoundPhase::FinalDecision:
	default:
		NextPhase = EJinzzaRoundPhase::RoundComplete;
		break;
	}

	EnterPhase(NextPhase);
}

float UjinzzaRoundPhaseSubsystem::GetPhaseDurationSeconds(EJinzzaRoundPhase Phase) const
{
	// "빠르게/기본/느긋하게" (design doc section 9) scales only "자유시간·면담 시간" per its own
	// description - free time and interview, not the mechanical phases (role assignment, timed
	// text input, evaluation reveal, final reveal). Exact multipliers aren't specified in the doc;
	// these are a reasonable first pass, tune from playtesting per section 17.
	float FreeTimeMultiplier = 1.f;
	const FString PhaseSpeed = GetMatchSettings().PhaseSpeed;
	if (PhaseSpeed == TEXT("Fast"))
	{
		FreeTimeMultiplier = 0.75f;
	}
	else if (PhaseSpeed == TEXT("Slow"))
	{
		FreeTimeMultiplier = 1.3f;
	}

	switch (Phase)
	{
	case EJinzzaRoundPhase::RoleAssignment:
		return 30.f;
	case EJinzzaRoundPhase::SelfIntroduction:
		// "자기소개 타임 시간 = 20초 x 후보 수(진짜+모방자)" - excludes the Judge.
		return 20.f * FMath::Max(1, GetCandidateCount());
	case EJinzzaRoundPhase::FreeTime1:
		return 120.f * FreeTimeMultiplier;
	case EJinzzaRoundPhase::QuestionTime:
		// 2 cycles of (작성 20s + 답변 15s + 토론 30s) = 130s. The cycle count (1-3, doc section 9)
		// isn't a lobby setting yet, so this is fixed at the doc's default of 2 for now.
		return 130.f;
	case EJinzzaRoundPhase::MidEvaluation:
		return 60.f;
	case EJinzzaRoundPhase::FreeTime2:
		return 120.f * FreeTimeMultiplier;
	case EJinzzaRoundPhase::Interview:
		return 30.f * FreeTimeMultiplier;
	case EJinzzaRoundPhase::FinalDecision:
		return 60.f;
	default:
		return 0.f;
	}
}

int32 UjinzzaRoundPhaseSubsystem::GetTotalPlayerCount() const
{
	const AjinzzaGameGameState* GameState = GetJinzzaGameState();
	return GameState ? GameState->PlayerArray.Num() : 0;
}

int32 UjinzzaRoundPhaseSubsystem::GetCandidateCount() const
{
	// 2-judge mode is marked experimental/stretch in the design doc (sections 9 and 15) with its
	// vote/interview rules explicitly undecided - always exactly 1 judge until that's spec'd
	// (matches AjinzzaGameGameMode::AssignRoles, which also never assigns a second judge).
	const int32 JudgeCount = 1;
	return FMath::Max(0, GetTotalPlayerCount() - JudgeCount);
}

AjinzzaGameGameState* UjinzzaRoundPhaseSubsystem::GetJinzzaGameState() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetGameState<AjinzzaGameGameState>() : nullptr;
}

FJinzzaMatchSettings UjinzzaRoundPhaseSubsystem::GetMatchSettings() const
{
	if (const UjinzzaGameInstance* GI = Cast<UjinzzaGameInstance>(GetGameInstance()))
	{
		return GI->GetPendingMatchSettings();
	}
	return FJinzzaMatchSettings();
}
