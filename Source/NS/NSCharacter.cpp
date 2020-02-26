// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "NSCharacter.h"
#include "NSProjectile.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/InputSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"

#include "Net/UnrealNetwork.h"
#include "NSPlayerState.h"
#include "Materials/MaterialInstanceDynamic.h"

#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogFPChar, Warning, All);

//////////////////////////////////////////////////////////////////////////
// ANSCharacter

ANSCharacter::ANSCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->RelativeLocation = FVector(-39.56f, 1.75f, 64.f); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	FP_Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterFP_Mesh"));
	FP_Mesh->SetOnlyOwnerSee(true);
	FP_Mesh->SetupAttachment(FirstPersonCameraComponent);
	FP_Mesh->bCastDynamicShadow = false;
	FP_Mesh->CastShadow = false;
	FP_Mesh->RelativeRotation = FRotator(1.9f, -19.19f, 5.2f);
	FP_Mesh->RelativeLocation = FVector(-0.5f, -4.4f, -155.7f);

	// Create a gun mesh component
	FP_Gun = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FP_Gun"));
	FP_Gun->SetOnlyOwnerSee(true);			// only the owning player will see this mesh
	FP_Gun->bCastDynamicShadow = false;
	FP_Gun->CastShadow = false;
	FP_Gun->SetupAttachment(FP_Mesh, TEXT("GripPoint"));
	
	// 3��Ī �� �޽� ������Ʈ ����
	TP_Gun = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("TP_Gun"));
	TP_Gun->SetOwnerNoSee(true);
	TP_Gun->SetupAttachment(GetMesh(), TEXT("hand_rSocket"));

	GetMesh()->SetOwnerNoSee(true);

	//��ƼŬ ����(3��Ī)
	TP_GunShotParticle = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("ParticleSysTp"));
	TP_GunShotParticle->bAutoActivate = false;
	TP_GunShotParticle->SetupAttachment(TP_Gun);
	TP_GunShotParticle->SetOwnerNoSee(true);

	//��ƼŬ ����(1��Ī)
	FP_GunShotParticle= CreateDefaultSubobject<UParticleSystemComponent>(TEXT("ParticleSysFp"));
	FP_GunShotParticle->bAutoActivate = false;
	FP_GunShotParticle->SetupAttachment(FP_Gun);
	FP_GunShotParticle->SetOnlyOwnerSee(true);

	//�Ѿ� ��ƼŬ
	BulletParticle = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("BulletSysTP"));
	BulletParticle->bAutoActivate = false;
	BulletParticle->SetupAttachment(FirstPersonCameraComponent);

	FP_MuzzleLocation = CreateDefaultSubobject<USceneComponent>(TEXT("MuzzleLocation"));
	FP_MuzzleLocation->SetupAttachment(FP_Gun);
	FP_MuzzleLocation->SetRelativeLocation(FVector(0.2f, 48.4f, -10.6f));


	// Note: The ProjectileClass and the skeletal mesh/anim blueprints for FP_Mesh, FP_Gun, and VR_Gun 
	// are set in the derived blueprint asset named MyCharacter to avoid direct content references in C++.


	
}

void ANSCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	if (Role != ROLE_Authority) {
		SetTeam(CurrentTeam);
	}

}

void ANSCharacter::SetTeam_Implementation(ETeam NewTeam) {
	FLinearColor outColour;
	if (NewTeam == ETeam::BLUE_TEAM) {
		outColour = FLinearColor(0.0f, 0.0f, 0.5f);
	}

	else {
		outColour = FLinearColor(0.5f, 0.0f, 0.0f);
	}
	if (DynamicMat == nullptr) {
		DynamicMat = UMaterialInstanceDynamic::Create(GetMesh()->GetMaterial(0), this);
		DynamicMat->SetVectorParameterValue(TEXT("BodyColor"), outColour);
		GetMesh()->SetMaterial(0, DynamicMat);
		FP_Mesh->SetMaterial(0, DynamicMat);
	}
}

//////////////////////////////////////////////////////////////////////////
// Input

void ANSCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// set up gameplay key bindings
	check(PlayerInputComponent);

	// Bind jump events
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	// Bind fire event
	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &ANSCharacter::OnFire);


	// Bind movement events
	PlayerInputComponent->BindAxis("MoveForward", this, &ANSCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ANSCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &ANSCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &ANSCharacter::LookUpAtRate);
}

void ANSCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ANSCharacter, CurrentTeam);
}

float ANSCharacter::TakeDamage(float Damage, FDamageEvent const & DamageEvent, AController * EventInstigator, AActor * DamageCauser)
{
	Super::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);

	if (Role == ROLE_Authority && DamageCauser != this && NSPlayerState->Health > 0) {
		NSPlayerState->Health -= Damage;
		PlayPain();

		if (NSPlayerState->Health <= 0) {
			NSPlayerState->Deaths++;
			//�÷��̾ �������� �ð� ���� �״´�
			MultiCastRagdoll();
			ANSCharacter* OtherChar = Cast<ANSCharacter>(DamageCauser);

			if (OtherChar) {
				OtherChar->NSPlayerState->Score += 1.0f;
			}

			//3�ʵ� ������
			FTimerHandle thisTimer;

			GetWorldTimerManager().SetTimer<ANSCharacter>(thisTimer, this, &ANSCharacter::Respawn, 3.0f, false);
		}
	}
	return Damage;
}

void ANSCharacter::PossessedBy(AController * NewController)
{

	Super::PossessedBy(NewController);

	NSPlayerState = Cast<ANSPlayerState>(PlayerState);
	if (Role == ROLE_Authority && NSPlayerState != nullptr) {
		NSPlayerState->Health = 100.0f;
	}
}

bool ANSCharacter::ServerFire_Validate(const FVector pos, const FVector dir)
{
	if (pos != FVector(ForceInit) && dir != FVector(ForceInit)) {
		return true;
	}
	else {
		return false;
	}
}

void ANSCharacter::ServerFire_Implementation(const FVector pos, const FVector dir) {
	Fire(pos, dir);
	MultiCastShootEffects();
}

ANSPlayerState * ANSCharacter::GetNSPlayerState()
{
	if (NSPlayerState) {
		return NSPlayerState;
	}
	else {
		NSPlayerState = Cast<ANSPlayerState>(PlayerState);
		return NSPlayerState;
	}

}

void ANSCharacter::SetNSPlayerState(ANSPlayerState * newPS)
{
	//PS�� ��ȿ�ϰ� �������� �����ƴ��� Ȯ���Ѵ�
	if (newPS && Role == ROLE_Authority) {
		NSPlayerState = newPS;
		PlayerState = newPS;
	}
}

void ANSCharacter::Respawn()
{
	if (Role == ROLE_Authority) {
		//���� ���κ��� ��ġ ���
		NSPlayerState->Health = 100.0f;
		Cast<ANSGameMode>(GetWorld()->GetAuthGameMode())->Respawn(this);

		Destroy(true, true);
	}
}

void ANSCharacter::OnFire()
{

	// try and play a firing animation if specified
	if (FP_FireAnimation != NULL)
	{
		// Get the animation object for the arms mesh
		UAnimInstance* AnimInstance = FP_Mesh->GetAnimInstance();
		if (AnimInstance != NULL)
		{
			AnimInstance->Montage_Play(FP_FireAnimation, 1.f);
		}
	}

	//�����ƴٸ� FP ��ƼŬ ����Ʈ�� ����Ѵ�
	if (FP_GunShotParticle != nullptr) {
		FP_GunShotParticle->Activate(true);
	}

	FVector mousePos;
	FVector mouseDir;

	APlayerController* pController = Cast<APlayerController>(GetController());
	FVector2D ScreenPos = GEngine->GameViewport->Viewport->GetSizeXY();

	pController->DeprojectScreenPositionToWorld(ScreenPos.X / 2.0f, ScreenPos.Y / 2.0f, mousePos, mouseDir);
	mouseDir *= 10000000.0f;

	ServerFire(mousePos, mouseDir);
}//Onfire �ݱ�


//Commenting this section out to be consistent with FPS BP template.
//This allows the user to turn without using the right virtual joystick

//void ANSCharacter::TouchUpdate(const ETouchIndex::Type FingerIndex, const FVector Location)
//{
//	if ((TouchItem.bIsPressed == true) && (TouchItem.FingerIndex == FingerIndex))
//	{
//		if (TouchItem.bIsPressed)
//		{
//			if (GetWorld() != nullptr)
//			{
//				UGameViewportClient* ViewportClient = GetWorld()->GetGameViewport();
//				if (ViewportClient != nullptr)
//				{
//					FVector MoveDelta = Location - TouchItem.Location;
//					FVector2D ScreenSize;
//					ViewportClient->GetViewportSize(ScreenSize);
//					FVector2D ScaledDelta = FVector2D(MoveDelta.X, MoveDelta.Y) / ScreenSize;
//					if (FMath::Abs(ScaledDelta.X) >= 4.0 / ScreenSize.X)
//					{
//						TouchItem.bMoved = true;
//						float Value = ScaledDelta.X * BaseTurnRate;
//						AddControllerYawInput(Value);
//					}
//					if (FMath::Abs(ScaledDelta.Y) >= 4.0 / ScreenSize.Y)
//					{
//						TouchItem.bMoved = true;
//						float Value = ScaledDelta.Y * BaseTurnRate;
//						AddControllerPitchInput(Value);
//					}
//					TouchItem.Location = Location;
//				}
//				TouchItem.Location = Location;
//			}
//		}
//	}
//}

void ANSCharacter::MoveForward(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorForwardVector(), Value);
	}
}

void ANSCharacter::MoveRight(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorRightVector(), Value);
	}
}

void ANSCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void ANSCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void ANSCharacter::Fire(const FVector pos, const FVector dir)
{
	//����ĳ��Ʈ ����
	FCollisionObjectQueryParams ObjQuery;
	ObjQuery.AddObjectTypesToQuery(ECC_GameTraceChannel1);

	FCollisionQueryParams ColQuery;
	ColQuery.AddIgnoredActor(this);

	FHitResult HitRes;
	GetWorld()->LineTraceSingleByObjectType(HitRes, pos, dir, ObjQuery, ColQuery);

	DrawDebugLine(GetWorld(), pos, dir, FColor::Red, true, 100, 0, 5.0f);

	if (HitRes.bBlockingHit) {
		ANSCharacter* OtherChar = Cast<ANSCharacter>(HitRes.GetActor());
		if (OtherChar != nullptr&&OtherChar->GetNSPlayerState()->Team != this->GetNSPlayerState()->Team) {
			FDamageEvent thisEvent(UDamageType::StaticClass());
			OtherChar->TakeDamage(10.0f, thisEvent, this->GetController(), this);
			APlayerController* thisPC = Cast<APlayerController>(GetController());
			thisPC->ClientPlayForceFeedback(HitSuccessFeedback, false,true ,NAME_None);
		}
	}
}

void ANSCharacter::MultiCastShootEffects_Implementation() {

	//�����ƴٸ� �߻� �ִϸ��̼��� ����Ѵ�
	if (TP_FireAnimation != NULL) {

		//�� �޽��� �ִϸ��̼� ������Ʈ�� ��´�
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (AnimInstance != NULL)
		{
			AnimInstance->Montage_Play(TP_FireAnimation, 1.f);
		}
	}

	//������ ��� ���� ����� �õ��Ѵ�
	if (FireSound != NULL) {
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation());
	}
	if (TP_GunShotParticle != nullptr)
	{
		TP_GunShotParticle->Activate(true);
	}

	if (BulletParticle != nullptr)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), BulletParticle->Template, BulletParticle->GetComponentLocation(), BulletParticle->GetComponentRotation());
	}

}

void ANSCharacter::PlayPain_Implementation() {
	if (Role == ROLE_AutonomousProxy) {
		UGameplayStatics::PlaySoundAtLocation(this, PainSound, GetActorLocation());
	}
}

void ANSCharacter::MultiCastRagdoll_Implementation() {
	GetMesh()->SetPhysicsBlendWeight(1.0f);
	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->SetCollisionProfileName("Ragdoll");
}