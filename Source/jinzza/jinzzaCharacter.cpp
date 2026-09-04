// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaCharacter.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "jinzzaGameUserSettings.h"
#include "jinzzaDisguiseComponent.h"
#include "jinzzaInteractableProp.h"
#include "jinzza.h"

AjinzzaCharacter::AjinzzaCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);
	
	// Create the first person mesh that will be viewed only by this character's owner
	FirstPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("First Person Mesh"));

	FirstPersonMesh->SetupAttachment(GetMesh());
	FirstPersonMesh->SetOnlyOwnerSee(true);
	FirstPersonMesh->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::FirstPerson;
	FirstPersonMesh->SetCollisionProfileName(FName("NoCollision"));

	// Create the Camera Component	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("First Person Camera"));
	FirstPersonCameraComponent->SetupAttachment(FirstPersonMesh, FName("head"));
	FirstPersonCameraComponent->SetRelativeLocationAndRotation(FVector(-2.8f, 5.89f, 0.0f), FRotator(0.0f, 90.0f, -90.0f));
	FirstPersonCameraComponent->bUsePawnControlRotation = true;
	FirstPersonCameraComponent->bEnableFirstPersonFieldOfView = true;
	FirstPersonCameraComponent->bEnableFirstPersonScale = true;
	FirstPersonCameraComponent->FirstPersonFieldOfView = 70.0f;
	FirstPersonCameraComponent->FirstPersonScale = 0.6f;

	// configure the character comps
	GetMesh()->SetOwnerNoSee(true);
	GetMesh()->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::WorldSpaceRepresentation;

	GetCapsuleComponent()->SetCapsuleSize(34.0f, 96.0f);

	// Configure character movement
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;
	GetCharacterMovement()->AirControl = 0.5f;

	DisguiseComponent = CreateDefaultSubobject<UjinzzaDisguiseComponent>(TEXT("DisguiseComponent"));
}

void AjinzzaCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{	
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AjinzzaCharacter::DoJumpStart);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &AjinzzaCharacter::DoJumpEnd);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AjinzzaCharacter::MoveInput);

		// Looking/Aiming
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AjinzzaCharacter::LookInput);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AjinzzaCharacter::LookInput);

		// Free-time props: F to pick up / activate, left click to use what's held, Q to drop it
		EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &AjinzzaCharacter::DoInteract);
		EnhancedInputComponent->BindAction(UseHeldPropAction, ETriggerEvent::Started, this, &AjinzzaCharacter::DoUseHeldProp);
		EnhancedInputComponent->BindAction(DropHeldPropAction, ETriggerEvent::Started, this, &AjinzzaCharacter::DoDropHeldProp);
	}
	else
	{
		UE_LOG(Logjinzza, Error, TEXT("'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}


void AjinzzaCharacter::MoveInput(const FInputActionValue& Value)
{
	// get the Vector2D move axis
	FVector2D MovementVector = Value.Get<FVector2D>();

	// pass the axis values to the move input
	DoMove(MovementVector.X, MovementVector.Y);

}

void AjinzzaCharacter::LookInput(const FInputActionValue& Value)
{
	// get the Vector2D look axis
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// pass the axis values to the aim input
	DoAim(LookAxisVector.X, LookAxisVector.Y);

}

void AjinzzaCharacter::DoAim(float Yaw, float Pitch)
{
	if (GetController())
	{
		float SensitizedYaw = Yaw;
		float SensitizedPitch = Pitch;
		if (const UjinzzaGameUserSettings* Settings = UjinzzaGameUserSettings::Get())
		{
			SensitizedYaw *= Settings->GetMouseSensitivity();
			SensitizedPitch *= Settings->GetMouseSensitivity() * (Settings->GetInvertYAxis() ? -1.f : 1.f);
		}

		// pass the rotation inputs
		AddControllerYawInput(SensitizedYaw);
		AddControllerPitchInput(SensitizedPitch);
	}
}

void AjinzzaCharacter::DoMove(float Right, float Forward)
{
	if (GetController())
	{
		// pass the move inputs
		AddMovementInput(GetActorRightVector(), Right);
		AddMovementInput(GetActorForwardVector(), Forward);
	}
}

void AjinzzaCharacter::DoJumpStart()
{
	// pass Jump to the character
	Jump();
}

void AjinzzaCharacter::DoJumpEnd()
{
	// pass StopJumping to the character
	StopJumping();
}

void AjinzzaCharacter::DoInteract()
{
	if (!FirstPersonCameraComponent)
	{
		return;
	}

	const FVector TraceStart = FirstPersonCameraComponent->GetComponentLocation();
	const FVector TraceEnd = TraceStart + (FirstPersonCameraComponent->GetForwardVector() * InteractTraceDistance);

	FHitResult Hit;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	if (GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
	{
		if (AjinzzaInteractableProp* Prop = Cast<AjinzzaInteractableProp>(Hit.GetActor()))
		{
			Server_InteractWithProp(Prop);
		}
	}
}

void AjinzzaCharacter::DoUseHeldProp()
{
	Server_UseHeldProp();
}

void AjinzzaCharacter::DoDropHeldProp()
{
	Server_DropHeldProp();
}

void AjinzzaCharacter::Server_InteractWithProp_Implementation(AjinzzaInteractableProp* Prop)
{
	if (!Prop)
	{
		return;
	}

	if (Prop->GetInteractionType() == EJinzzaPropInteractionType::Handheld)
	{
		if (!Prop->IsHeld())
		{
			Prop->AttachToHolder(this);
			HeldProp = Prop;
		}
	}
	else
	{
		Prop->Activate();
	}
}

void AjinzzaCharacter::Server_UseHeldProp_Implementation()
{
	if (HeldProp)
	{
		HeldProp->Activate();
	}
}

void AjinzzaCharacter::Server_DropHeldProp_Implementation()
{
	if (HeldProp)
	{
		HeldProp->DropFromHolder();
		HeldProp = nullptr;
	}
}
