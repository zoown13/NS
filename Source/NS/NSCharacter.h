// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameFramework/ForceFeedbackEffect.h"
#include "NSSGameMode.h"
#include "NSCharacter.generated.h"

class UInputComponent;

UCLASS(config=Game)
class ANSCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Pawn mesh: 1st person view (arms; seen only by self) */
	UPROPERTY(VisibleDefaultsOnly, Category=Mesh)
	class USkeletalMeshComponent* FP_Mesh;

	/** Gun mesh: 1st person view (seen only by self) */
	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
	class USkeletalMeshComponent* FP_Gun;

	/** 건 메시: 3인칭 뷰(다른 사람에게만 보임) */
	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
	class USkeletalMeshComponent* TP_Gun;

	/** Location on gun mesh where projectiles should spawn. */
	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
	class USceneComponent* FP_MuzzleLocation;

	/** First person camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FirstPersonCameraComponent;


public:
	ANSCharacter();

protected:
	virtual void BeginPlay();

public:
	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseTurnRate;

	/** Base look up/down rate, in deg/sec. Other scaling may affect final rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseLookUpRate;

	/** Sound to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Gameplay)
	class USoundBase* FireSound;

	/** 맞을 때마다 플레이 할 사운드 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	class USoundBase* PainSound;

	/** 총 발사를 위한 3인칭 애니메이션 몽타주 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	class UAnimMontage* TP_FireAnimation;

	/** AnimMontage to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	class UAnimMontage* FP_FireAnimation;

	/** 총 발사효과를 위한 3인칭 파티클 시스템 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	class UParticleSystemComponent* TP_GunShotParticle;

	/** 총 발사효과를 위한 1인칭 파티클 시스템 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	class UParticleSystemComponent* FP_GunShotParticle;

	/** 총알을 표현할 파티클 시스템 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	class UParticleSystemComponent* BulletParticle;

	/** 포스 피드백 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	class UForceFeedbackEffect* HitSuccessFeedback;

	UPROPERTY(Replicated, BlueprintReadWrite, Category = Team)
		ETeam CurrentTeam;

	class ANSPlayerState* GetNSPlayerState();
	void SetNSPlayerState(class ANSPlayerState* newPS);
	void Respawn();

protected:
	
	/** Fires a projectile. */
	void OnFire();

	/** Handles moving forward/backward */
	void MoveForward(float Val);

	/** Handles stafing movement, left and right */
	void MoveRight(float Val);

	/**
	 * Called via input to turn at a given rate.
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void TurnAtRate(float Rate);

	/**
	 * Called via input to turn look up/down at a given rate.
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void LookUpAtRate(float Rate);

	//레이트레이스를 서버에서 수행하기 위해 호출
	void Fire(const FVector pos, const FVector dir);

protected:
	// APawn interface
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;
	virtual float TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
	virtual void PossessedBy(AController* NewController) override;
	// End of APawn interface

	class UMaterialInstanceDynamic* DynamicMat;
	class ANSPlayerState* NSPlayerState;



public:
	/** Returns FP_Mesh subobject **/
	FORCEINLINE class USkeletalMeshComponent* GetFP_Mesh() const { return FP_Mesh; }
	/** Returns FirstPersonCameraComponent subobject **/
	FORCEINLINE class UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }




/** 원격 프로시저 호출 */
private:
	//서버에서 fire 액션 수행
	UFUNCTION(Server, Reliable, WithValidation)
		void ServerFire(const FVector pos, const FVector dir);

	//모든 클라이언트가 발사 효과를 실행하는 멀티캐스트
	UFUNCTION(NetMultiCast, unreliable)
		void MultiCastShootEffects();

	//사망 시 모든 클라이언트에게 사망을 전달하기 위해 호출된다
	UFUNCTION(NetMultiCast, unreliable)
		void MultiCastRagdoll();

	//히트 시 소유 클라이언트에게 고통을 준다
	UFUNCTION(Client, Reliable)
		void PlayPain();

public:

	//팀 색상 결정
	UFUNCTION(NetMultiCast, Reliable)
		void SetTeam(ETeam NewTeam);




};

