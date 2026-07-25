// Copyright Opus 5 Three Games. Original work.

#include "SFTestCommandlet.h"
#include "Misc/AutomationTest.h"

DEFINE_LOG_CATEGORY_STATIC(LogSFTest, Log, All);

int32 USFTestCommandlet::Main(const FString& Params)
{
#if WITH_DEV_AUTOMATION_TESTS
	FAutomationTestFramework& Framework = FAutomationTestFramework::Get();
	Framework.SetRequestedTestFilter(
		EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter);

	TArray<FAutomationTestInfo> AllTests;
	Framework.GetValidTestNames(AllTests);

	UE_LOG(LogSFTest, Display, TEXT("SFTEST discovered %d test(s) total"), AllTests.Num());

	int32 Ran = 0;
	int32 Failed = 0;

	for (const FAutomationTestInfo& Info : AllTests)
	{
		// GetDisplayName is the beautified path ("Stormfall.Damage.FallDamage");
		// GetTestName is the registration name the framework starts tests by.
		const FString Name = Info.GetDisplayName();
		if (!Name.StartsWith(TEXT("Stormfall.")))
		{
			continue;
		}

		++Ran;
		Framework.StartTestByName(Info.GetTestName(), 0);

		FAutomationTestExecutionInfo ExecutionInfo;
		const bool bPassed = Framework.StopTest(ExecutionInfo);

		if (bPassed)
		{
			UE_LOG(LogSFTest, Display, TEXT("SFTEST PASS  %s"), *Name);
		}
		else
		{
			++Failed;
			UE_LOG(LogSFTest, Error, TEXT("SFTEST FAIL  %s"), *Name);
			for (const FAutomationExecutionEntry& Entry : ExecutionInfo.GetEntries())
			{
				if (Entry.Event.Type == EAutomationEventType::Error)
				{
					UE_LOG(LogSFTest, Error, TEXT("SFTEST   %s"), *Entry.Event.Message);
				}
			}
		}
	}

	UE_LOG(LogSFTest, Display, TEXT("SFTEST SUMMARY ran=%d failed=%d"), Ran, Failed);

	// A run that matched nothing is a failure too — it usually means the tests
	// were compiled out, which would otherwise look like a clean pass.
	if (Ran == 0)
	{
		UE_LOG(LogSFTest, Error, TEXT("SFTEST no tests matched the 'Stormfall.' prefix"));
		return 1;
	}

	return Failed > 0 ? 1 : 0;
#else
	UE_LOG(LogSFTest, Error, TEXT("SFTEST built without WITH_DEV_AUTOMATION_TESTS"));
	return 1;
#endif
}
