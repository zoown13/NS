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
		//���� ����
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
	//�� ���� �� ����

	if (Role == ROLE_Authority && Teamless != nullptr) {
		if (BlueTeam.Num() > RedTeam.Num()) {
			RedTeam.Add(Teamless);
			NPlayerState->Team = ETeam::RED_TEAM;
		}
		else if (BlueTeam.Num() < RedTeam.Num()) {
			BlueTeam.Add(Teamless);
			NPlayerState->Team = ETeam::BLUE_TEAM;
		}
		else//������ ���� ����
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
				//���� ť ��ġ���� ����
				if (ToBeSpawned.Find(Character) != INDEX_NONE) {
					ToBeSpawned.Remove(Character);
				}
				//�׷��� ������ ���� ��ġ ����
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
