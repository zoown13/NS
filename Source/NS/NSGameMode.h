// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "NSGameMode.generated.h"

UENUM(BlueprintType)
enum class ETeam : uint8 {
	BLUE_TEAM,
	RED_TEAM
};


UCLASS(minimalapi)
class ANSGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ANSGameMode();
};



