// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NSSGameMode.h"
#include "NSSpawnPoint.generated.h"

UCLASS()
class NS_API ANSSpawnPoint : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ANSSpawnPoint();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION()
		void ActorBeginOverlaps(AActor* OverlappedActor,AActor* OtherActor);

	UFUNCTION()
		void ActorEndOverlaps(AActor* OverlappedActor,AActor* OtherActor);

	bool GetBlcoked() {
		return OverlappingActors.Num() != 0;
	}
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		ETeam Team;

private:
	class UCapsuleComponent* SpawnCapsule;
	TArray<class AActor*> OverlappingActors;
};
