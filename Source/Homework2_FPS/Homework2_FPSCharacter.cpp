// Copyright Epic Games, Inc. All Rights Reserved.

#include "Homework2_FPSCharacter.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/KismetSystemLibrary.h"


//////////////////////////////////////////////////////////////////////////
// AHomework2_FPSCharacter

AHomework2_FPSCharacter::AHomework2_FPSCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;
	bCanController = true;
	// Configure character movement
	
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f); // ...at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 300.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm


	bIsJumping = false;
	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named MyCharacter (to avoid direct content references in C++)
}

//////////////////////////////////////////////////////////////////////////
// Input

void AHomework2_FPSCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &AHomework2_FPSCharacter::OnJump);
	//PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &AHomework2_FPSCharacter::StopOnJump);
	

	PlayerInputComponent->BindAxis("MoveForward", this, &AHomework2_FPSCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AHomework2_FPSCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &AHomework2_FPSCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AHomework2_FPSCharacter::LookUpAtRate);

	// handle touch devices
	//PlayerInputComponent->BindTouch(IE_Pressed, this, &AHomework2_FPSCharacter::TouchStarted);
	//PlayerInputComponent->BindTouch(IE_Released, this, &AHomework2_FPSCharacter::TouchStopped);

	// VR headset functionality
	//PlayerInputComponent->BindAction("ResetVR", IE_Pressed, this, &AHomework2_FPSCharacter::OnResetVR);

}


FRotator AHomework2_FPSCharacter::GetAimOffsets() const
{
	const FVector AimDirWS = GetBaseAimRotation().Vector();
	const FVector AimDirLS = ActorToWorld().InverseTransformVectorNoScale(AimDirWS);
	const FRotator AimRotLS = AimDirLS.Rotation();

	return AimRotLS;
}

bool AHomework2_FPSCharacter::bIsSprinting() const
{
	if (!GetCharacterMovement())
	{
		return false;
	}
	return !GetVelocity().IsZero() && (FVector::DotProduct(GetVelocity().GetSafeNormal2D(), GetActorRotation().Vector()) > 0.8);
}

bool AHomework2_FPSCharacter::IsInitiatedJump() const
{
	return bIsJumping;
}

void AHomework2_FPSCharacter::SetIsJumping(bool NewJumping)
{

	if (NewJumping != bIsJumping)
	{
		bIsJumping = NewJumping;

		if (bIsJumping)
		{
			/* Perform the built-in Jump on the character */
			Jump();
		}
	}



}

void AHomework2_FPSCharacter::OnJump()
{
	//UE_LOG(LogTemp, Warning, TEXT(bIsJumping));
	bIsJumping = true;
	//UE_LOG(LogTemp, Warning, TEXT(bIsJumping));
	Jump();
	//SetIsJumping(true);
}

void AHomework2_FPSCharacter::StopOnJump()
{
	bIsJumping = false;
	StopJumping();
	//SetIsJumping(false);
}

void AHomework2_FPSCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
//	
//	DOREPLIFETIME_CONDITION(AHomework2_FPSCharacter, bIsJumping, COND_SkipOwner);
//
//
}


void AHomework2_FPSCharacter::OnResetVR()
{
	UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition();
}

void AHomework2_FPSCharacter::TouchStarted(ETouchIndex::Type FingerIndex, FVector Location)
{
	if (bCanController)
		Jump();
}

void AHomework2_FPSCharacter::TouchStopped(ETouchIndex::Type FingerIndex, FVector Location)
{
	if (bCanController)
		StopJumping();
}



void AHomework2_FPSCharacter::TurnAtRate(float Rate)
{
	if (bCanController)
		// calculate delta for this frame from the rate information
	{
		AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
	}
}

void AHomework2_FPSCharacter::LookUpAtRate(float Rate)
{
	if (bCanController)
		// calculate delta for this frame from the rate information
	{
		AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
	}
}

void AHomework2_FPSCharacter::MoveForward(float Value)
{
	if (bCanController)
	{
		if ((Controller != NULL) && (Value != 0.0f))
		{
			// find out which way is forward
			const FRotator Rotation = Controller->GetControlRotation();
			const FRotator YawRotation(0, Rotation.Yaw, 0);

			// get forward vector
			const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
			AddMovementInput(Direction, Value);
		}
	}
}

void AHomework2_FPSCharacter::MoveRight(float Value)
{
	if (bCanController)
	{
		if ((Controller != NULL) && (Value != 0.0f))
		{
			// find out which way is right
			const FRotator Rotation = Controller->GetControlRotation();
			const FRotator YawRotation(0, Rotation.Yaw, 0);

			// get right vector 
			const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
			// add movement in that direction
			AddMovementInput(Direction, Value);
		}
	}
}


