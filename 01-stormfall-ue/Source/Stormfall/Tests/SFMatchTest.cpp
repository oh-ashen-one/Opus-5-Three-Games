// Copyright Opus 5 Three Games. Original work.

#include "Misc/AutomationTest.h"
#include "SFMatch.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSFMatchResolutionTest,
	"Stormfall.Match.Resolution",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FSFMatchResolutionTest::RunTest(const FString& Parameters)
{
	using SFMatch::ResolvePhase;

	// Mid-match with a field still standing: nothing resolves.
	TestEqual(TEXT("16 alive is still a battle"),
		ResolvePhase(true, 16, ESFMatchPhase::Battle), ESFMatchPhase::Battle);
	TestEqual(TEXT("2 alive is still a battle"),
		ResolvePhase(true, 2, ESFMatchPhase::Battle), ESFMatchPhase::Battle);

	// Last one standing.
	TestEqual(TEXT("alone and alive is a victory"),
		ResolvePhase(true, 1, ESFMatchPhase::Battle), ESFMatchPhase::Victory);

	// Dead is dead, regardless of how many bots remain.
	TestEqual(TEXT("player dead mid-field is defeat"),
		ResolvePhase(false, 9, ESFMatchPhase::Battle), ESFMatchPhase::Defeat);
	TestEqual(TEXT("player dead with one bot left is defeat"),
		ResolvePhase(false, 1, ESFMatchPhase::Battle), ESFMatchPhase::Defeat);

	// The case that would actually ruin a recording: you win, then the storm
	// finishes off a body still being counted, and Victory flips to Defeat.
	TestEqual(TEXT("victory latches against a later storm kill"),
		ResolvePhase(false, 0, ESFMatchPhase::Victory), ESFMatchPhase::Victory);
	TestEqual(TEXT("victory latches against further updates"),
		ResolvePhase(true, 1, ESFMatchPhase::Victory), ESFMatchPhase::Victory);
	TestEqual(TEXT("defeat latches too"),
		ResolvePhase(true, 1, ESFMatchPhase::Defeat), ESFMatchPhase::Defeat);

	// Degenerate counts must not produce a phantom victory before the match
	// has actually started.
	TestEqual(TEXT("zero alive and player dead is defeat"),
		ResolvePhase(false, 0, ESFMatchPhase::Battle), ESFMatchPhase::Defeat);

	// Deploy phase behaves like battle until someone dies.
	TestEqual(TEXT("deploy with a full field stays deploy"),
		ResolvePhase(true, 16, ESFMatchPhase::Deploy), ESFMatchPhase::Deploy);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
