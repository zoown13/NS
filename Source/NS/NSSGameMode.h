// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "NSSGameMode.generated.h"

UENUM(BlueprintType)
enum class ETeam : uint8 {
	BLUE_TEAM,
	RED_TEAM
};

/**
 * 
 */
UCLASS(minimalapi)
class ANSSGameMode : public AGameMode
{
	GENERATED_BODY()
	
public:
	ANSSGameMode();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void Respawn(class ANSCharacter* Character);
	void Spawn(class ANSCharacter* Character);

private:
	TArray<class ANSCharacter*> RedTeam;
	TArray<class ANSCharacter*> BlueTeam;

	TArray<class ANSSpawnPoint*> RedSpawn;
	TArray<class ANSSpawnPoint*> BlueSpawn;
	TArray<class ANSCharacter*> ToBeSpawned;

	bool bGameStarted;
	static bool bInGameMenu;
	
	
};
