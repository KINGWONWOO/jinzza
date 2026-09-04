// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaCharacter.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "jinzzaGameUserSettings.h"
#include "jinzzaDisguiseComponent.h"
#include "jinzzaCharacterCustomizationComponent.h"
#include "jinzzaInteractableProp.h"
#include "jinzzaEmoteWheelWidget.h"
#include "jinzzaPropUsageWidget.h"
#include "jinzza.h"
#include "UObject/ConstructorHelpers.h"

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
	CustomizationComponent = CreateDefaultSubobject<UjinzzaCharacterCustomizationComponent>(TEXT("CustomizationComponent"));

	static ConstructorHelpers::FClassFinder<UjinzzaEmoteWheelWidget> EmoteWheelWidgetBPClass(TEXT("/Game/JINZZA/UI/Widgets/WBP_EmoteWheel"));
	if (EmoteWheelWidgetBPClass.Succeeded())
	{
		EmoteWheelWidgetClass = EmoteWheelWidgetBPClass.Class;
	}

	static ConstructorHelpers::FClassFinder<UjinzzaPropUsageWidget> PropUsageWidgetBPClass(TEXT("/Game/JINZZA/UI/Widgets/WBP_PropUsageHUD"));
	if (PropUsageWidgetBPClass.Succeeded())
	{
		PropUsageWidgetClass = PropUsageWidgetBPClass.Class;
	}
}

void AjinzzaCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocallyControlled() && PropUsageWidgetClass)
	{
		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			PropUsageWidget = CreateWidget<UjinzzaPropUsageWidget>(PC, PropUsageWidgetClass);
			if (PropUsageWidget)
			{
				PropUsageWidget->AddToViewport(50);
				PropUsageWidget->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
	}
}

void AjinzzaCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (IsLocallyControlled())
	{
		UpdateInteractionFocus();
	}
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

		// Free-time props: F to pick up / activate / steal, left click to use what's held, Q to drop it, right click to throw it
		EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &AjinzzaCharacter::DoInteract);
		EnhancedInputComponent->BindAction(UseHeldPropAction, ETriggerEvent::Started, this, &AjinzzaCharacter::DoUseHeldProp);
		EnhancedInputComponent->BindAction(DropHeldPropAction, ETriggerEvent::Started, this, &AjinzzaCharacter::DoDropHeldProp);
		EnhancedInputComponent->BindAction(ThrowHeldPropAction, ETriggerEvent::Started, this, &AjinzzaCharacter::DoThrowHeldProp);

		// Emote wheel: hold E to open and steer with the mouse, release to play whatever's hovered
		EnhancedInputComponent->BindAction(EmoteWheelAction, ETriggerEvent::Started, this, &AjinzzaCharacter::DoOpenEmoteWheel);
		EnhancedInputComponent->BindAction(EmoteWheelAction, ETriggerEvent::Completed, this, &AjinzzaCharacter::DoCloseEmoteWheel);
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
	// While the emote wheel is open, mouse movement steers it instead of the camera.
	if (bEmoteWheelOpen)
	{
		return;
	}

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
	if (AjinzzaInteractableProp* Prop = TraceForInteractableProp())
	{
		Server_InteractWithProp(Prop);
	}
}

AjinzzaInteractableProp* AjinzzaCharacter::TraceForInteractableProp() const
{
	if (!FirstPersonCameraComponent)
	{
		return nullptr;
	}

	const FVector TraceStart = FirstPersonCameraComponent->GetComponentLocation();
	const FVector TraceEnd = TraceStart + (FirstPersonCameraComponent->GetForwardVector() * InteractTraceDistance);

	FHitResult Hit;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	if (GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
	{
		return Cast<AjinzzaInteractableProp>(Hit.GetActor());
	}
	return nullptr;
}

void AjinzzaCharacter::UpdateInteractionFocus()
{
	AjinzzaInteractableProp* NewFocus = bEmoteWheelOpen ? nullptr : TraceForInteractableProp();

	// Don't prompt to interact with whatever you're already holding (it's still in the trace's way).
	if (NewFocus && NewFocus->IsHeldBy(this))
	{
		NewFocus = nullptr;
	}

	if (NewFocus == FocusedInteractProp.Get())
	{
		return;
	}

	if (AjinzzaInteractableProp* OldFocus = FocusedInteractProp.Get())
	{
		OldFocus->HideInteractionPrompt();
	}

	FocusedInteractProp = NewFocus;

	if (NewFocus)
	{
		NewFocus->ShowInteractionPrompt();
	}
}

void AjinzzaCharacter::ShowPropUsageHUD(AjinzzaInteractableProp* Prop)
{
	if (!Prop || !PropUsageWidget)
	{
		return;
	}

	PropUsageWidget->SetPropInfo(Prop->GetUsageIcon(), Prop->GetUsageDescription());
	PropUsageWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
	HUDDisplayedProp = Prop;
}

void AjinzzaCharacter::HidePropUsageHUD(AjinzzaInteractableProp* Prop)
{
	if (!PropUsageWidget || HUDDisplayedProp.Get() != Prop)
	{
		return;
	}

	PropUsageWidget->SetVisibility(ESlateVisibility::Collapsed);
	HUDDisplayedProp = nullptr;
}

void AjinzzaCharacter::DoUseHeldProp()
{
	Server_UseHeldProp();
}

void AjinzzaCharacter::DoDropHeldProp()
{
	Server_DropHeldProp();
}

void AjinzzaCharacter::DoThrowHeldProp()
{
	Server_ThrowHeldProp();
}

void AjinzzaCharacter::Server_InteractWithProp_Implementation(AjinzzaInteractableProp* Prop)
{
	if (!Prop)
	{
		return;
	}

	if (Prop->GetInteractionType() == EJinzzaPropInteractionType::Handheld)
	{
		// Also true when Prop is held by someone else - pressing F snatches it, same as any pickup.
		if (!Prop->IsHeldBy(this))
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

void AjinzzaCharacter::ClearHeldPropIfMatches(const AjinzzaInteractableProp* Prop)
{
	if (HeldProp == Prop)
	{
		HeldProp = nullptr;
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

void AjinzzaCharacter::Server_ThrowHeldProp_Implementation()
{
	if (HeldProp)
	{
		const FVector LaunchVelocity = GetControlRotation().Vector() * ThrowSpeed;
		HeldProp->ThrowFromHolder(LaunchVelocity);
		HeldProp = nullptr;
	}
}

void AjinzzaCharacter::DoOpenEmoteWheel()
{
	if (bEmoteWheelOpen || !EmoteWheelWidgetClass)
	{
		return;
	}

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC || !PC->IsLocalController())
	{
		return;
	}

	EmoteWheelWidget = CreateWidget<UjinzzaEmoteWheelWidget>(PC, EmoteWheelWidgetClass);
	if (!EmoteWheelWidget)
	{
		return;
	}

	EmoteWheelWidget->AddToViewport(100);
	bEmoteWheelOpen = true;

	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(EmoteWheelWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	PC->SetInputMode(InputMode);
	PC->bShowMouseCursor = true;
}

void AjinzzaCharacter::DoCloseEmoteWheel()
{
	if (!bEmoteWheelOpen)
	{
		return;
	}
	bEmoteWheelOpen = false;

	const EJinzzaEmoteType SelectedEmote = EmoteWheelWidget ? EmoteWheelWidget->GetHoveredEmote() : EJinzzaEmoteType::None;

	if (EmoteWheelWidget)
	{
		EmoteWheelWidget->RemoveFromParent();
		EmoteWheelWidget = nullptr;
	}

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		FInputModeGameOnly InputMode;
		PC->SetInputMode(InputMode);
		PC->bShowMouseCursor = false;
	}

	if (SelectedEmote != EJinzzaEmoteType::None)
	{
		Server_PlayEmote(SelectedEmote);
	}
}

void AjinzzaCharacter::Server_PlayEmote_Implementation(EJinzzaEmoteType EmoteType)
{
	Multicast_PlayEmote(EmoteType);
}

void AjinzzaCharacter::Multicast_PlayEmote_Implementation(EJinzzaEmoteType EmoteType)
{
	UAnimMontage* Montage = GetMontageForEmote(EmoteType);
	if (!Montage)
	{
		return;
	}

	// FirstPersonMesh is attached to and shares GetMesh()'s skeleton/pose, so playing the montage
	// here animates both the owner's first-person arms and everyone else's third-person view of it.
	if (UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
	{
		AnimInstance->Montage_Play(Montage);
	}
}

UAnimMontage* AjinzzaCharacter::GetMontageForEmote(EJinzzaEmoteType EmoteType) const
{
	switch (EmoteType)
	{
	case EJinzzaEmoteType::ThumbsUp:     return ThumbsUpMontage;
	case EJinzzaEmoteType::ThumbsDown:   return ThumbsDownMontage;
	case EJinzzaEmoteType::MiddleFinger: return MiddleFingerMontage;
	case EJinzzaEmoteType::Point:        return PointMontage;
	default:                             return nullptr;
	}
}
