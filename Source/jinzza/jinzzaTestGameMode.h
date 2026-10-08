// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "jinzzaGameGameMode.h"
#include "jinzzaTestGameMode.generated.h"

class AjinzzaTestDummyController;
class AjinzzaPartyPlayerState;

/** A piece of the match you can try on your own in Lvl_test (AjinzzaTestDrillKiosk). */
UENUM(BlueprintType)
enum class EJinzzaTestDrill : uint8
{
	/** A dummy introduces itself, then you: spotlight, turn camera, speech bubbles (type with Enter). */
	SelfIntroduction,
	/** Question Time as the Judge: type a question, watch 4 dummies draw and flip their boards. */
	QuestionAsJudge,
	/** Question Time as a candidate: a dummy Judge asks, you draw alongside 3 dummies. */
	QuestionAsCandidate,
	/** You're the Judge: vote out a dummy, hear its final argument, watch it turn into a ghost. */
	Vote,
	/** Stops whatever is running and puts every dummy back. */
	Reset
};

/**
 * Lvl_test's game mode: the match's game mode (so the match HUD, speaking turns, Question Time and the
 * vote all work as they do in Lvl_Game) without the round - nothing starts by itself. Instead:
 *  - Practice dummies: a seal (BP_FirstPersonCharacter run by AjinzzaTestDummyController) is spawned on every
 *    actor tagged "Test.Dummy" (drill dummies, used in tag-number order "Test.Dummy.<N>") or
 *    "Test.TargetDummy" (stand-ins to hit with the bat / stun gun, never used in drills).
 *  - Drill stations: an AjinzzaTestDrillKiosk is spawned on every actor tagged "Test.Drill.<DrillName>" (e.g.
 *    "Test.Drill.QuestionAsJudge"), facing the way the marker faces - so the level only needs markers.
 *  - Drills (StartDrill, from AjinzzaTestDrillKiosk): one self-introduction round, one Question Time cycle
 *    (as Judge or as candidate) or one vote, with the dummies playing everyone else - they talk in speech
 *    bubbles, draw doodles, ask a question, look around. When the drill is over they walk back home
 *    (teleport), and a ghost stays a ghost until the next drill or Reset.
 * Uses the same level markers as Lvl_Game: "Zone.SelfIntro.Spotlight"/".Camera",
 * "Zone.Evaluation.Spotlight"/".Camera", "Zone.Question.Seat"+"Zone.Question.Seat.<N>".
 */
UCLASS()
class JINZZA_API AjinzzaTestGameMode : public AjinzzaGameGameMode
{
	GENERATED_BODY()

public:
	AjinzzaTestGameMode();

	/** Server: stops any running drill and starts Drill with Player in it. */
	void StartDrill(EJinzzaTestDrill Drill, APlayerController* Player);

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void StartPlay() override;
	/** Lvl_test never starts a round. */
	virtual void TryStartRound() override {}

private:
	void SpawnDummies();
	void SpawnDrillStations();
	/** Ends any turn/vote/Question Time, clears roles and aliases, revives ghosts, sends the dummies home. */
	void ResetDrill();
	void SendDummiesHome();
	/** The first Count drill dummies' player states (fewer if there aren't enough). */
	TArray<AjinzzaPartyPlayerState*> GetDrillDummies(int32 Count) const;
	bool IsDummy(const APlayerState* PlayerState) const;
	bool IsDrillBusy() const;

	/** Dummy behaviour while a drill runs: talking on their turn, asking as Judge, drawing an answer. */
	void UpdateDummyTalk(double Now);
	void UpdateDummyQuestion(double Now);

	static FJinzzaDrawing MakeDoodle();

	UPROPERTY()
	TArray<TObjectPtr<AjinzzaTestDummyController>> Dummies;

	bool bDrillRunning = false;
	EJinzzaTestDrill ActiveDrill = EJinzzaTestDrill::Reset;
	double DrillStartTime = 0.0;
	/** How long nothing has been happening (the drill is over once this passes a second). */
	float DrillIdleSeconds = 0.f;

	// Dummy talk: the speaking turn it's for, the next line's time and which line.
	int32 TalkTurnSerial = -1;
	double NextTalkTime = 0.0;
	int32 TalkLineIndex = 0;

	// Dummy Question Time: the step it's for, when the dummy Judge asks, when each dummy hands in.
	int32 QuestionSerialHandled = -1;
	double DummyAskTime = 0.0;
	TMap<TWeakObjectPtr<APlayerState>, double> DummyAnswerTimes;
};
