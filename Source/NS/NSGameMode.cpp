// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "NSGameMode.h"
#include "NSHUD.h"
#include "NSCharacter.h"
#include "UObject/ConstructorHelpers.h"
#include "NS.h"
#include "NSPlayerState.h"
#include "NSSpawnPoint.h"
#include "EngineUtils.h"


bool ANSGameMode::bInGameMenu = true;

ANSGameMode::ANSGameMode()
	: Super()
{
	PrimaryActorTick.bCanEverTick = true;

	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPersonCPP/Blueprints/FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;
	PlayerStateClass = ANSPlayerState::StaticClass();

	// use our custom HUD class
	HUDClass = ANSHUD::StaticClass();

	bReplicates = true;
}

void ANSGameMode::BeginPlay()
{
	Super::BeginPlay();
	if (Role == ROLE_Authority) {
		for (TActorIterator<ANSSpawnPoint>Iter(GetWorld()); Iter; ++Iter) {
			if ((*Iter)->Team == ETeam::RED_TEAM) {
				RedSpawn.Add(*Iter);
			}
			else {
				BlueSpawn.Add(*Iter);
			}
		}
		//서버 스폰
		APlayerController* thisCont = GetWorld()->GetFirstPlayerController();
		if (thisCont) {
			ANSCharacter* thisChar = Cast<ANSCharacter>(thisCont->GetPawn());
			thisChar->SetTeam(ETeam::BLUE_TEAM);
			BlueTeam.Add(thisChar);
			Spawn(thisChar);
		}
	}
}

void ANSGameMode::Tick(float DeltaSeconds)
{
	if (Role == ROLE_Authority) {
		APlayerController* thisCont = GetWorld()->GetFirstPlayerController();
		if (ToBeSpawned.Num() != 0) {
			for (auto charToSpawn : ToBeSpawned) {
				Spawn(charToSpawn);
			}
		}

		if (thisCont != nullptr&&thisCont->IsInputKeyDown(EKeys::R)) {
			bInGameMenu = false;
			GetWorld()->ServerTravel(L"/Game/FirstPersonCPP/Maps/FirstPersonExampleMap?Listen");
		}
	}
}

void ANSGameMode::PostLogin(APlayerController * NewPlayer)
{
	Super::PostLogin(NewPlayer);

	ANSCharacter* Teamless = Cast<ANSCharacter>(NewPlayer->GetPawn());
	ANSPlayerState* NPlayerState = Cast<ANSPlayerState>(NewPlayer->PlayerState);

	if (Teamless != nullptr&&NPlayerState != nullptr) {
		Teamless->SetNSPlayerState(NPlayerState);
	}
	//팀 지정 및 스폰

	if (Role == ROLE_Authority && Teamless != nullptr) {
		if (BlueTeam.Num() > RedTeam.Num()) {
			RedTeam.Add(Teamless);
			NPlayerState->Team = ETeam::RED_TEAM;
		}
		else if (BlueTeam.Num() < RedTeam.Num()) {
			BlueTeam.Add(Teamless);
			NPlayerState->Team = ETeam::BLUE_TEAM;
		}
		else//팀원의 수가 같다
		{
			BlueTeam.Add(Teamless);
			NPlayerState->Team = ETeam::BLUE_TEAM;
		}

		Teamless->CurrentTeam = NPlayerState->Team;
		Teamless->SetTeam(NPlayerState->Team);
		Spawn(Teamless);
	}
}

void ANSGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (EndPlayReason == EEndPlayReason::Quit || EndPlayReason == EEndPlayReason::EndPlayInEditor) {
		bInGameMenu = true;
	}
}

void ANSGameMode::Respawn(ANSCharacter * Character)
{
	if (Role == ROLE_Authority) {
		AController* thisPC = Character->GetController();
		Character->DetachFromControllerPendingDestroy();

		ANSCharacter* newChar = Cast<ANSCharacter>(GetWorld()->SpawnActor(DefaultPawnClass));

		if (newChar) {
			thisPC->Possess(newChar);
			ANSPlayerState* thisPS = Cast<ANSPlayerState>(newChar->GetController()->PlayerState);

			newChar->CurrentTeam = thisPS->Team;
			newChar->SetNSPlayerState(thisPS);

			Spawn(newChar);

			newChar->SetTeam(newChar->GetNSPlayerState()->Team);

		}
	}
}

void ANSGameMode::Spawn(ANSCharacter * Character)
{
	if (Role == ROLE_Authority) {
		ANSSpawnPoint* thisSpawn = nullptr;
		TArray<ANSSpawnPoint*>* targetTeam = nullptr;
		if (Character->CurrentTeam == ETeam::BLUE_TEAM) {
			targetTeam = &BlueSpawn;
		}
		else {
			targetTeam = &RedSpawn;
		}

		for (auto Spawn : (*targetTeam)) {
			if (!Spawn->GetBlcoked()) {
				//스폰 큐 위치에서 제거
				if (ToBeSpawned.Find(Character) != INDEX_NONE) {
					ToBeSpawned.Remove(Character);
				}
				//그렇지 않으면 액터 위치 설정
				Character->SetActorLocation(Spawn->GetActorLocation());
				Spawn->UpdateOverlaps();
				return;
			}

			if (ToBeSpawned.Find(Character) == INDEX_NONE) {
				ToBeSpawned.Add(Character);
			}
		}
	}
}
