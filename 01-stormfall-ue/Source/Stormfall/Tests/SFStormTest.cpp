// Copyright Opus 5 Three Games. Original work.

#include "Misc/AutomationTest.h"
#include "SFStorm.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSFStormTimelineTest,
	"Stormfall.Storm.Timeline",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FSFStormTimelineTest::RunTest(const FString& Parameters)
{
	const FVector2D Origin(0.f, 0.f);
	const float R0 = 10000.f;

	TArray<FSFStormPhase> Phases;
	Phases.Add({ 10.f, 10.f, 0.5f, FVector2D(1000.f, 0.f), 1.f });
	Phases.Add({ 10.f, 10.f, 0.5f, FVector2D(2000.f, 0.f), 4.f });

	TestEqual(TEXT("total duration"), SFStorm::TotalDuration(Phases), 40.f);

	// t=0: holding at the initial circle, nothing has moved yet.
	{
		const FSFStormState S = SFStorm::Evaluate(Phases, Origin, R0, 0.f);
		TestEqual(TEXT("phase 0 at t=0"), S.PhaseIndex, 0);
		TestEqual(TEXT("radius unchanged during hold"), S.Radius, R0);
		TestFalse(TEXT("not shrinking during hold"), S.bShrinking);
		TestEqual(TEXT("counts down to shrink"), S.TimeToNextEvent, 10.f);
		TestFalse(TEXT("not finished"), S.bFinished);
	}

	// Negative time clamps rather than reading off the front of the timeline.
	{
		const FSFStormState S = SFStorm::Evaluate(Phases, Origin, R0, -50.f);
		TestEqual(TEXT("negative time clamps to start"), S.Radius, R0);
	}

	// Mid-shrink of phase 0: half way to 0.5*R0, half way to the new center.
	{
		const FSFStormState S = SFStorm::Evaluate(Phases, Origin, R0, 15.f);
		TestTrue(TEXT("shrinking"), S.bShrinking);
		TestNearlyEqual(TEXT("radius halfway to 0.5x"), S.Radius, (R0 + R0 * 0.5f) * 0.5f, 0.01f);
		TestNearlyEqual(TEXT("center halfway"), static_cast<float>(S.Center.X), 500.f, 0.01f);
		TestEqual(TEXT("damage is phase 0's"), S.DamagePerSecond, 1.f);
	}

	// End of phase 0 == start of phase 1: radius must be exactly 0.5*R0 with no jump.
	{
		const FSFStormState S = SFStorm::Evaluate(Phases, Origin, R0, 20.f);
		TestNearlyEqual(TEXT("radius at phase boundary"), S.Radius, R0 * 0.5f, 0.01f);
		TestNearlyEqual(TEXT("center at phase boundary"), static_cast<float>(S.Center.X), 1000.f, 0.01f);
		TestEqual(TEXT("now in phase 1"), S.PhaseIndex, 1);
	}

	// After the last phase the final circle persists and still deals damage.
	{
		const FSFStormState S = SFStorm::Evaluate(Phases, Origin, R0, 9999.f);
		TestTrue(TEXT("finished"), S.bFinished);
		TestNearlyEqual(TEXT("final radius"), S.Radius, R0 * 0.25f, 0.01f);
		TestNearlyEqual(TEXT("final center"), static_cast<float>(S.Center.X), 2000.f, 0.01f);
		TestTrue(TEXT("still lethal after the last phase"), S.DamagePerSecond > 0.f);
	}

	// Radius must never increase — a storm that reopens would be a real bug.
	{
		float Previous = TNumericLimits<float>::Max();
		for (float T = 0.f; T <= 45.f; T += 0.25f)
		{
			const FSFStormState S = SFStorm::Evaluate(Phases, Origin, R0, T);
			TestTrue(TEXT("radius never grows"), S.Radius <= Previous + 0.01f);
			TestTrue(TEXT("radius never negative"), S.Radius >= 0.f);
			Previous = S.Radius;
		}
	}

	// Empty phase list is finished immediately rather than crashing.
	{
		const FSFStormState S = SFStorm::Evaluate({}, Origin, R0, 5.f);
		TestTrue(TEXT("empty timeline is finished"), S.bFinished);
	}

	// Inside/outside test uses XY only, so height must not matter.
	{
		FSFStormState S;
		S.Center = FVector2D(0.f, 0.f);
		S.Radius = 1000.f;
		TestFalse(TEXT("inside"), SFStorm::IsOutside(S, FVector(500.f, 0.f, 0.f)));
		TestFalse(TEXT("inside but high up"), SFStorm::IsOutside(S, FVector(500.f, 0.f, 50000.f)));
		TestTrue(TEXT("outside"), SFStorm::IsOutside(S, FVector(1500.f, 0.f, 0.f)));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSFStormDefaultsTest,
	"Stormfall.Storm.DefaultTimeline",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FSFStormDefaultsTest::RunTest(const FString& Parameters)
{
	const float MapRadius = 50000.f; // 500m, a ~1km island
	const TArray<FSFStormPhase> Phases = SFStorm::DefaultPhases(MapRadius);

	TestEqual(TEXT("five phases"), Phases.Num(), 5);

	// The whole point of the spec: a match must land in the 8-10 minute window.
	// Storm runs ~7.5 min, with the drop and early looting on top.
	const float Total = SFStorm::TotalDuration(Phases);
	TestTrue(TEXT("storm is at least 6 minutes"), Total >= 360.f);
	TestTrue(TEXT("storm is under 9 minutes"), Total <= 540.f);

	// Damage must escalate, or the late game has no pressure.
	for (int32 i = 1; i < Phases.Num(); ++i)
	{
		TestTrue(TEXT("damage escalates each phase"), Phases[i].DamagePerSecond > Phases[i - 1].DamagePerSecond);
	}

	// The final circle has to be small enough to force a fight.
	const FSFStormState Final = SFStorm::Evaluate(Phases, FVector2D::ZeroVector, MapRadius, 99999.f);
	TestTrue(TEXT("final circle is tiny"), Final.Radius < MapRadius * 0.02f);
	TestTrue(TEXT("final circle is not degenerate"), Final.Radius > 0.f);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
