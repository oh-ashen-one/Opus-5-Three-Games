// Copyright Opus 5 Three Games. Original work.

#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "SFTestCommandlet.generated.h"

/**
 * Runs every automation test under the "Stormfall." prefix and exits non-zero if
 * any fail.
 *
 *   UnrealEditor-Cmd Stormfall.uproject -run=SFTest -unattended -nullrhi -nosplash
 *
 * This exists because on macOS the plain `-ExecCmds="Automation RunTests ..."`
 * entry point never boots the engine — it falls through to a build step and exits.
 * The commandlet path does boot, so tests run through here instead.
 */
UCLASS()
class USFTestCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	virtual int32 Main(const FString& Params) override;
};
