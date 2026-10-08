// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaCharacter.h"
#include "jinzzaPlayerController.h"
#include "jinzzaNameplateWidget.h"
#include "jinzzaChatBoardComponent.h"
#include "jinzzaChatBoardWidget.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/WidgetComponent.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "jinzzaGameUserSettings.h"
#include "jinzzaDisguiseComponent.h"
#include "jinzzaCharacterCustomizationComponent.h"
#include "jinzzaProximityVoiceComponent.h"
#include "jinzzaInteractableProp.h"
#include "jinzzaAudio.h"
#include "jinzzaBoomboxProp.h"
#include "Blueprint/UserWidget.h"
#include "jinzzaPartyPlayerState.h"
#include "jinzzaEmoteWheelWidget.h"
#include "jinzzaPropUsageWidget.h"
#include "jinzza.h"
#include "UObject/ConstructorHelpers.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

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

	// Create the Camera Component. Attached to the capsule (not a mesh bone/socket) so its
	// position/orientation never depends on a specific skeleton's bone-local axis convention -
	// the old FirstPersonMesh "head" socket attachment baked in an offset/rotation
	// (FVector(-2.8,5.89,0) / FRotator(0,90,-90)) that only made sense for the original Mannequin
	// head bone's particular local frame; swapping to any other skeleton (e.g. a from-scratch
	// rig with default bone roll) would silently point the camera in an arbitrary direction.
	// A per-character eye height still belongs on the character (capsule half-height varies per
	// Blueprint), so RelativeLocation.Z is a sane default here and expected to be overridden per-
	// Blueprint the same way CapsuleHalfHeight/CapsuleRadius already are.
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("First Person Camera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->SetRelativeLocationAndRotation(FVector(0.0f, 0.0f, 80.0f), FRotator::ZeroRotator);
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

	// Just below the camera (eye height 80) - roughly where a mouth would be.
	VoiceAnchor = CreateDefaultSubobject<USceneComponent>(TEXT("VoiceAnchor"));
	VoiceAnchor->SetupAttachment(GetCapsuleComponent());
	VoiceAnchor->SetRelativeLocation(FVector(0.f, 0.f, 65.f));

	ProximityVoiceComponent = CreateDefaultSubobject<UjinzzaProximityVoiceComponent>(TEXT("ProximityVoiceComponent"));

	// Screen space: always faces the camera at a constant readable size. Above the capsule top (half height 96).
	NameplateComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("NameplateComponent"));
	NameplateComponent->SetupAttachment(GetCapsuleComponent());
	NameplateComponent->SetRelativeLocation(FVector(0.f, 0.f, 125.f));
	NameplateComponent->SetWidgetSpace(EWidgetSpace::Screen);
	NameplateComponent->SetDrawAtDesiredSize(true);
	NameplateComponent->SetPivot(FVector2D(0.5f, 1.f));
	NameplateComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	NameplateComponent->SetWidgetClass(UjinzzaNameplateWidget::StaticClass());

	// Chat board, held at chest height ~45 cm in front. The pivot's +X is the written side: the board
	// component turns it to 180 (toward this character) while writing and 0 (toward everyone) to show.
	ChatBoardPivot = CreateDefaultSubobject<USceneComponent>(TEXT("ChatBoardPivot"));
	ChatBoardPivot->SetupAttachment(GetCapsuleComponent());
	ChatBoardPivot->SetRelativeLocation(FVector(45.f, 0.f, 25.f));

	// 2 x 62 x 42 cm slab (engine cube is 100 cm).
	ChatBoardMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ChatBoardMesh"));
	ChatBoardMesh->SetupAttachment(ChatBoardPivot);
	ChatBoardMesh->SetRelativeScale3D(FVector(0.02f, 0.62f, 0.42f));
	ChatBoardMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ChatBoardMesh->SetCastShadow(false);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> BoardCubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (BoardCubeFinder.Succeeded())
	{
		ChatBoardMesh->SetStaticMesh(BoardCubeFinder.Object);
	}

	// 560 x 360 px at 0.1 scale = 56 x 36 cm, just proud of the front face. One-sided, so from behind
	// (while its owner is writing) the board is blank.
	ChatBoardTextComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("ChatBoardTextComponent"));
	ChatBoardTextComponent->SetupAttachment(ChatBoardPivot);
	ChatBoardTextComponent->SetRelativeLocation(FVector(1.2f, 0.f, 0.f));
	ChatBoardTextComponent->SetRelativeScale3D(FVector(0.1f));
	ChatBoardTextComponent->SetWidgetSpace(EWidgetSpace::World);
	ChatBoardTextComponent->SetDrawSize(FVector2D(560.f, 360.f));
	ChatBoardTextComponent->SetTwoSided(false);
	ChatBoardTextComponent->SetBlendMode(EWidgetBlendMode::Transparent);
	ChatBoardTextComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ChatBoardTextComponent->SetWidgetClass(UjinzzaChatBoardWidget::StaticClass());

	ChatBoardComponent = CreateDefaultSubobject<UjinzzaChatBoardComponent>(TEXT("ChatBoardComponent"));
	ChatBoardComponent->Bind(ChatBoardPivot, ChatBoardTextComponent);

	static ConstructorHelpers::FClassFinder<UjinzzaEmoteWheelWidget> EmoteWheelWidgetBPClass(TEXT("/Game/JINZZA/UI/Widgets/WBP_EmoteWheel"));
	if (EmoteWheelWidgetBPClass.Succeeded())
	{
		EmoteWheelWidgetClass = EmoteWheelWidgetBPClass.Class;
	}

	// No automatic WBP_PropUsageHUD lookup here: that Blueprint's generated class is not a child
	// class of jinzzaPropUsageWidget (confirmed via the live editor log - a stale leftover from
	// before this class got its own BuildWidgetTree()), so a ConstructorHelpers::FClassFinder
	// lookup here always failed anyway - it just logged a CDO-construction error on every compile.
	// Unlike the kiosk classes, BeginPlay() below has no StaticClass() fallback when
	// PropUsageWidgetClass is unset, so it's defaulted directly here instead - can still be
	// overridden by hand in the Details panel if a real Designer-authored subclass is ever built.
	PropUsageWidgetClass = UjinzzaPropUsageWidget::StaticClass();

	// TEMP placeholder audio defaults - swap these for real per-surface/animation sounds later.
	static ConstructorHelpers::FObjectFinder<USoundBase> FootstepSoundFinder(TEXT("/Game/JINZZA/Audio/Sounds/FootStep/Footstep.Footstep"));
	if (FootstepSoundFinder.Succeeded())
	{
		FootstepSound = FootstepSoundFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<USoundBase> FootstepSoundAltFinder(TEXT("/Game/JINZZA/Audio/Sounds/FootStep/FootstepGrass.FootstepGrass"));
	if (FootstepSoundAltFinder.Succeeded())
	{
		FootstepSoundAlt = FootstepSoundAltFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<USoundBase> JumpSoundFinder(TEXT("/Game/JINZZA/Audio/Sounds/UISounds/ButtonClickPopSound.ButtonClickPopSound"));
	if (JumpSoundFinder.Succeeded())
	{
		JumpSound = JumpSoundFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<USoundBase> LandSoundFinder(TEXT("/Game/JINZZA/Audio/Sounds/Basketball/InteractWoodenBox__cut_0sec_.InteractWoodenBox__cut_0sec_"));
	if (LandSoundFinder.Succeeded())
	{
		LandSound = LandSoundFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<USoundBase> ThumbsUpSoundFinder(TEXT("/Game/JINZZA/Audio/Sounds/BasketballHoop/correctanswer.correctanswer"));
	if (ThumbsUpSoundFinder.Succeeded())
	{
		ThumbsUpSound = ThumbsUpSoundFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<USoundBase> ThumbsDownSoundFinder(TEXT("/Game/JINZZA/Audio/Sounds/Emotes/WrongAnswerSound.WrongAnswerSound"));
	if (ThumbsDownSoundFinder.Succeeded())
	{
		ThumbsDownSound = ThumbsDownSoundFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<USoundBase> MiddleFingerSoundFinder(TEXT("/Game/JINZZA/Audio/Sounds/Bat/Punch1.Punch1"));
	if (MiddleFingerSoundFinder.Succeeded())
	{
		MiddleFingerSound = MiddleFingerSoundFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<USoundBase> PointSoundFinder(TEXT("/Game/JINZZA/Audio/Sounds/Megaphone/PressButton.PressButton"));
	if (PointSoundFinder.Succeeded())
	{
		PointSound = PointSoundFinder.Object;
	}
}

void AjinzzaCharacter::BeginPlay()
{
	Super::BeginPlay();

	BaseWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;

	ProximityVoiceComponent->SetVoiceAnchor(VoiceAnchor);

	if (UjinzzaNameplateWidget* Nameplate = Cast<UjinzzaNameplateWidget>(NameplateComponent->GetUserWidgetObject()))
	{
		Nameplate->SetOwnerPawn(this);
	}

	// Chalkboard green (BasicShapeMaterial's Color parameter).
	if (UMaterialInstanceDynamic* BoardMaterial = ChatBoardMesh->CreateDynamicMaterialInstance(0))
	{
		BoardMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.03f, 0.12f, 0.07f));
	}

	// The visible seal body is a Blueprint component (BP_FirstPersonCharacter's SealMesh) - turned as the
	// "head" while seated.
	TInlineComponentArray<UStaticMeshComponent*> MeshComponents(this);
	for (UStaticMeshComponent* MeshComponent : MeshComponents)
	{
		if (MeshComponent && MeshComponent->GetFName() == TEXT("SealMesh"))
		{
			SealMesh = MeshComponent;
			SealMeshBaseRotation = MeshComponent->GetRelativeRotation().Quaternion();
			break;
		}
	}

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

	// Human players only - an AI pawn (Lvl_test's practice dummies) is "locally controlled" on the server too,
	// and would pop up prop prompts on the host's screen.
	if (IsLocallyControlled() && IsPlayerControlled())
	{
		UpdateInteractionFocus();
	}

	if (UjinzzaNameplateWidget* Nameplate = Cast<UjinzzaNameplateWidget>(NameplateComponent->GetUserWidgetObject()))
	{
		Nameplate->Refresh();
	}

	UpdateFootsteps(DeltaSeconds);
	UpdateSeatedLook(DeltaSeconds);
}

void AjinzzaCharacter::UpdateFootsteps(float DeltaSeconds)
{
	if (!GetCharacterMovement()->IsMovingOnGround())
	{
		DistanceSinceLastFootstep = 0.f;
		return;
	}

	const float Speed = GetVelocity().Size2D();
	if (Speed < 10.f)
	{
		return;
	}

	DistanceSinceLastFootstep += Speed * DeltaSeconds;
	if (DistanceSinceLastFootstep < FootstepDistanceInterval)
	{
		return;
	}
	DistanceSinceLastFootstep = 0.f;

	USoundBase* StepSound = (FootstepSoundAlt && FMath::RandBool()) ? FootstepSoundAlt : FootstepSound;
	if (StepSound)
	{
		JinzzaAudio::PlaySoundAt(this, StepSound, GetActorLocation(), FootstepAudibleRadius);
	}
}

void AjinzzaCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	// Velocity still carries the fall speed here - the movement component zeroes it right after this call.
	const float FallSpeed = -GetVelocity().Z;
	const float Volume = FMath::GetMappedRangeValueClamped(FVector2D(LandSoundSpeedRange.X, LandSoundSpeedRange.Y), FVector2D(0.35f, 1.f), FallSpeed);
	PlayMovementSound(LandSound, Volume, false);
}

void AjinzzaCharacter::OnJumped_Implementation()
{
	Super::OnJumped_Implementation();

	PlayMovementSound(JumpSound, 1.f, true);
}

void AjinzzaCharacter::PlayMovementSound(USoundBase* Sound, float Volume, bool bIsJump)
{
	if (!Sound)
	{
		return;
	}

	// The owning client re-runs its recent moves after a server correction - don't replay the sound for those.
	if (bClientUpdating)
	{
		return;
	}

	// The player making the noise hears it right away, without waiting for a network round trip.
	if (IsLocallyControlled())
	{
		JinzzaAudio::PlaySoundAt(this, Sound, GetActorLocation(), ActionSoundAudibleRadius, Volume);
	}

	// Everyone else hears it via the server. (Simulated proxies never get here: they don't run the jump/landing
	// physics themselves, which is exactly why these sounds used to be heard only by the jumper.)
	if (HasAuthority())
	{
		if (bIsJump)
		{
			Multicast_PlayJumpSound();
		}
		else
		{
			Multicast_PlayLandSound(Volume);
		}
	}
}

void AjinzzaCharacter::Multicast_PlayJumpSound_Implementation()
{
	// Already played locally by the owner (see PlayMovementSound). Also covers the listen-server host's own character.
	if (!IsLocallyControlled() && JumpSound)
	{
		JinzzaAudio::PlaySoundAt(this, JumpSound, GetActorLocation(), ActionSoundAudibleRadius);
	}
}

void AjinzzaCharacter::Multicast_PlayLandSound_Implementation(float Volume)
{
	if (!IsLocallyControlled() && LandSound)
	{
		JinzzaAudio::PlaySoundAt(this, LandSound, GetActorLocation(), ActionSoundAudibleRadius, Volume);
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

		// Sprinting (held)
		EnhancedInputComponent->BindAction(SprintInputAction, ETriggerEvent::Started, this, &AjinzzaCharacter::DoSprintStart);
		EnhancedInputComponent->BindAction(SprintInputAction, ETriggerEvent::Completed, this, &AjinzzaCharacter::DoSprintEnd);
		EnhancedInputComponent->BindAction(SprintInputAction, ETriggerEvent::Canceled, this, &AjinzzaCharacter::DoSprintEnd);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AjinzzaCharacter::MoveInput);

		// Looking/Aiming
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AjinzzaCharacter::LookInput);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AjinzzaCharacter::LookInput);

		// Free-time props: F to pick up / activate / steal, left click to use what's held, Q to drop it, right click to throw it
		EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &AjinzzaCharacter::DoInteract);
		EnhancedInputComponent->BindAction(UseHeldPropAction, ETriggerEvent::Started, this, &AjinzzaCharacter::DoUseHeldProp);
		// Releasing the use button ends a hold-to-repeat use (e.g. the stun gun's continuous shock)
		EnhancedInputComponent->BindAction(UseHeldPropAction, ETriggerEvent::Completed, this, &AjinzzaCharacter::DoStopUseHeldProp);
		EnhancedInputComponent->BindAction(UseHeldPropAction, ETriggerEvent::Canceled, this, &AjinzzaCharacter::DoStopUseHeldProp);
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
	if (bEmoteWheelOpen || bHeldPropUIOpen)
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
	if (bStunned || bSeated)
	{
		return;
	}

	if (GetController())
	{
		// pass the move inputs
		AddMovementInput(GetActorRightVector(), Right);
		AddMovementInput(GetActorForwardVector(), Forward);
	}
}

void AjinzzaCharacter::DoJumpStart()
{
	if (bStunned || bSeated)
	{
		return;
	}

	// pass Jump to the character (the jump sound plays from OnJumped, once the jump really happens)
	Jump();
}

void AjinzzaCharacter::DoJumpEnd()
{
	// pass StopJumping to the character
	StopJumping();
}

void AjinzzaCharacter::DoSprintStart()
{
	if (bStunned)
	{
		return;
	}

	SetSprinting(true);
}

void AjinzzaCharacter::DoSprintEnd()
{
	SetSprinting(false);
}

void AjinzzaCharacter::SetSprinting(bool bNewSprinting)
{
	if (bIsSprinting == bNewSprinting)
	{
		return;
	}

	// Apply locally right away - this is what makes sprint feel instant on the machine that
	// pressed the key, whether that's the server or a client predicting ahead of its RPC.
	bIsSprinting = bNewSprinting;
	GetCharacterMovement()->MaxWalkSpeed = bNewSprinting ? BaseWalkSpeed * SprintSpeedMultiplier : BaseWalkSpeed;

	if (!HasAuthority())
	{
		Server_SetSprinting(bNewSprinting);
	}
}

void AjinzzaCharacter::Server_SetSprinting_Implementation(bool bNewSprinting)
{
	SetSprinting(bNewSprinting);
}

void AjinzzaCharacter::OnRep_IsSprinting()
{
	GetCharacterMovement()->MaxWalkSpeed = bIsSprinting ? BaseWalkSpeed * SprintSpeedMultiplier : BaseWalkSpeed;
}

void AjinzzaCharacter::DoInteract()
{
	// Interact again while the held prop's panel is open = close it (the panel doesn't take the key itself).
	if (bHeldPropUIOpen)
	{
		CloseHeldPropUI();
		return;
	}

	if (IsGhost())
	{
		return;
	}

	AjinzzaInteractableProp* Prop = TraceForInteractableProp();

	// Looking at the prop we're already holding (it's in the trace's way) doesn't count as looking at something else.
	if (Prop && !Prop->IsHeldBy(this))
	{
		Server_InteractWithProp(Prop);
	}
	else if (LocalHeldProp.IsValid())
	{
		OpenHeldPropUI();
	}
	else if (Prop)
	{
		Server_InteractWithProp(Prop);
	}
}

void AjinzzaCharacter::OpenHeldPropUI()
{
	AjinzzaInteractableProp* Prop = LocalHeldProp.Get();
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (bHeldPropUIOpen || bEmoteWheelOpen || !Prop || !PC || !PC->IsLocalController())
	{
		return;
	}

	UUserWidget* Widget = Prop->CreateHeldInteractionWidget(PC);
	if (!Widget)
	{
		return;
	}

	HeldPropWidget = Widget;
	HeldPropWidget->AddToViewport(90);
	bHeldPropUIOpen = true;

	// Nothing to look at or point at while the panel is up.
	if (AjinzzaInteractableProp* OldFocus = FocusedInteractProp.Get())
	{
		OldFocus->HideInteractionPrompt();
		FocusedInteractProp = nullptr;
	}

	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(HeldPropWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	PC->SetInputMode(InputMode);
	PC->bShowMouseCursor = true;
}

void AjinzzaCharacter::CloseHeldPropUI()
{
	if (!bHeldPropUIOpen)
	{
		return;
	}
	bHeldPropUIOpen = false;

	if (HeldPropWidget)
	{
		HeldPropWidget->RemoveFromParent();
		HeldPropWidget = nullptr;
	}

	RestoreControllerInputMode();
}

void AjinzzaCharacter::RestoreControllerInputMode()
{
	// The level's own input mode, not always game-only: the match keeps a visible cursor (End Game button,
	// the Judge's ballot) - see AjinzzaGamePlayerController::RestoreGameplayInputMode.
	if (AjinzzaPlayerController* JinzzaPC = Cast<AjinzzaPlayerController>(GetController()))
	{
		JinzzaPC->RestoreGameplayInputMode();
	}
	else if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->SetInputMode(FInputModeGameOnly());
		PC->bShowMouseCursor = false;
	}
}

void AjinzzaCharacter::RequestBoomboxMusic(AjinzzaBoomboxProp* Boombox, int32 TrackIndex, const FString& Url, bool bPlaying)
{
	if (Boombox && !IsGhost())
	{
		Server_SetBoomboxMusic(Boombox, TrackIndex, Url, bPlaying);
	}
}

void AjinzzaCharacter::Server_SetBoomboxMusic_Implementation(AjinzzaBoomboxProp* Boombox, int32 TrackIndex, const FString& Url, bool bPlaying)
{
	if (Boombox && Boombox->IsHeldBy(this) && !IsGhost())
	{
		Boombox->ServerApplyMusic(TrackIndex, Url, bPlaying);
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
	AjinzzaInteractableProp* NewFocus = (bEmoteWheelOpen || bHeldPropUIOpen || IsGhost()) ? nullptr : TraceForInteractableProp();

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
	if (Prop)
	{
		LocalHeldProp = Prop;
	}

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
	// Lost the prop (dropped / thrown / snatched): its own panel can't stay open, and it's no longer "held" for a second interact press.
	if (LocalHeldProp.Get() == Prop)
	{
		CloseHeldPropUI();
		LocalHeldProp = nullptr;
	}

	if (!PropUsageWidget || HUDDisplayedProp.Get() != Prop)
	{
		return;
	}

	PropUsageWidget->SetVisibility(ESlateVisibility::Collapsed);
	HUDDisplayedProp = nullptr;
}

void AjinzzaCharacter::DoUseHeldProp()
{
	if (IsGhost())
	{
		return;
	}
	Server_UseHeldProp();
}

void AjinzzaCharacter::DoStopUseHeldProp()
{
	// Deliberately not gated on IsGhost(): if someone turned into a ghost mid-hold, releasing must still be able to stop the repeat.
	Server_StopUseHeldProp();
}

void AjinzzaCharacter::DoDropHeldProp()
{
	if (IsGhost())
	{
		return;
	}
	Server_DropHeldProp();
}

void AjinzzaCharacter::DoThrowHeldProp()
{
	if (IsGhost())
	{
		return;
	}
	Server_ThrowHeldProp();
}

bool AjinzzaCharacter::IsGhost() const
{
	const AjinzzaPartyPlayerState* PartyPS = GetPlayerState<AjinzzaPartyPlayerState>();
	return PartyPS && PartyPS->IsGhost();
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

void AjinzzaCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AjinzzaCharacter, bStunned);
	DOREPLIFETIME(AjinzzaCharacter, bIsSprinting);
	DOREPLIFETIME(AjinzzaCharacter, bSeated);
	DOREPLIFETIME(AjinzzaCharacter, SeatYaw);
	DOREPLIFETIME_CONDITION(AjinzzaCharacter, SeatedLookYaw, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(AjinzzaCharacter, SeatedLookPitch, COND_SkipOwner);
}

// --- Seated ---------------------------------------------------------------------------------------

namespace
{
	/** How far a seated player can turn their view either side of the seat's facing. */
	constexpr float SeatedViewYawRange = 100.f;
	/** How much of the view turn the seal body shows (it has no neck), and the most it ever turns. */
	constexpr float SeatedBodyYawShare = 0.5f;
	constexpr float SeatedBodyYawMax = 45.f;
	constexpr float SeatedBodyPitchShare = 0.25f;
	constexpr float SeatedBodyPitchMax = 12.f;
	constexpr float SeatedLookSendInterval = 0.1f;
}

bool AjinzzaCharacter::CanJumpInternal_Implementation() const
{
	return !bSeated && Super::CanJumpInternal_Implementation();
}

void AjinzzaCharacter::ServerSetSeated(bool bInSeated, float InSeatYaw)
{
	if (!HasAuthority())
	{
		return;
	}
	bSeated = bInSeated;
	SeatYaw = FRotator::NormalizeAxis(InSeatYaw);
	SeatedLookYaw = 0;
	SeatedLookPitch = 0;
	if (bSeated)
	{
		StopJumping();
	}
	// The listen server's own copy never gets a RepNotify.
	ApplySeated();
}

void AjinzzaCharacter::OnRep_Seated()
{
	ApplySeated();
}

void AjinzzaCharacter::ApplySeated()
{
	// Seated: the body stays put facing the seat, the view (camera = control rotation) turns on its own.
	bUseControllerRotationYaw = !bSeated;
	if (bSeated)
	{
		SetActorRotation(FRotator(0.f, SeatYaw, 0.f));
	}

	if (IsLocallyControlled())
	{
		if (const APlayerController* PC = Cast<APlayerController>(GetController()); PC && PC->PlayerCameraManager)
		{
			// Back to the engine defaults (no limit) once standing again.
			PC->PlayerCameraManager->ViewYawMin = bSeated ? SeatYaw - SeatedViewYawRange : 0.f;
			PC->PlayerCameraManager->ViewYawMax = bSeated ? SeatYaw + SeatedViewYawRange : 359.999f;
		}
	}
}

void AjinzzaCharacter::UpdateSeatedLook(float DeltaSeconds)
{
	if (!bSeated && SealLook.IsNearlyZero(0.05f))
	{
		return;
	}

	FRotator Target = FRotator::ZeroRotator;
	if (bSeated)
	{
		if (IsLocallyControlled() && GetController())
		{
			// Owner: where am I looking, relative to the seat? Report it (throttled) for everyone else.
			const FRotator View = GetController()->GetControlRotation();
			const int8 Yaw = static_cast<int8>(FMath::Clamp(FMath::RoundToInt(FRotator::NormalizeAxis(View.Yaw - SeatYaw)), -127, 127));
			const int8 Pitch = static_cast<int8>(FMath::Clamp(FMath::RoundToInt(FRotator::NormalizeAxis(View.Pitch)), -90, 90));
			SeatedLookSendCooldown -= DeltaSeconds;
			if ((Yaw != SeatedLookYaw || Pitch != SeatedLookPitch) && SeatedLookSendCooldown <= 0.f)
			{
				SeatedLookYaw = Yaw;
				SeatedLookPitch = Pitch;
				SeatedLookSendCooldown = SeatedLookSendInterval;
				if (!HasAuthority())
				{
					Server_SetSeatedLook(Yaw, Pitch);
				}
			}
			Target = FRotator(Pitch, Yaw, 0.f);
		}
		else
		{
			Target = FRotator(SeatedLookPitch, SeatedLookYaw, 0.f);
		}
		Target.Yaw = FMath::Clamp(Target.Yaw * SeatedBodyYawShare, -SeatedBodyYawMax, SeatedBodyYawMax);
		Target.Pitch = FMath::Clamp(Target.Pitch * SeatedBodyPitchShare, -SeatedBodyPitchMax, SeatedBodyPitchMax);
	}

	SealLook = FMath::RInterpTo(SealLook, Target, DeltaSeconds, 8.f);
	if (USceneComponent* Body = SealMesh.Get())
	{
		// Turn in capsule space on top of the mesh's own rest rotation (its pivot sits at the capsule's feet).
		Body->SetRelativeRotation(SealLook.Quaternion() * SealMeshBaseRotation);
	}
}

void AjinzzaCharacter::Server_SetSeatedLook_Implementation(int8 Yaw, int8 Pitch)
{
	if (bSeated)
	{
		SeatedLookYaw = static_cast<int8>(FMath::Clamp<int32>(Yaw, -127, 127));
		SeatedLookPitch = static_cast<int8>(FMath::Clamp<int32>(Pitch, -90, 90));
	}
}

void AjinzzaCharacter::Stun(float Duration)
{
	if (!HasAuthority() || Duration <= 0.f)
	{
		return;
	}

	bStunned = true;
	OnRep_Stunned(); // server doesn't get its own RepNotify - apply locally too, matching AjinzzaInteractableProp::OnRep_HoldingPawn's pattern

	GetWorldTimerManager().SetTimer(StunTimerHandle, this, &AjinzzaCharacter::ClearStun, Duration, false);
}

void AjinzzaCharacter::ClearStun()
{
	if (!HasAuthority())
	{
		return;
	}

	bStunned = false;
	OnRep_Stunned();
}

void AjinzzaCharacter::OnRep_Stunned()
{
	if (bStunned)
	{
		// Kill existing momentum so "immobilized" reads immediately rather than sliding to a stop.
		GetCharacterMovement()->StopMovementImmediately();

		// Cancel sprint speed too - being stunned mid-sprint shouldn't leave MaxWalkSpeed raised
		// (harmless once actually stopped, but would let the character immediately move at sprint
		// speed the instant the stun timer clears, before DoSprintEnd ever fires again).
		if (bIsSprinting)
		{
			bIsSprinting = false;
			GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
		}
	}
}

void AjinzzaCharacter::Server_UseHeldProp_Implementation()
{
	if (HeldProp)
	{
		HeldProp->Activate();
		// For props that repeat while the button stays down (HoldRepeatInterval > 0); no-op for everything else.
		HeldProp->BeginHoldUse();
	}
}

void AjinzzaCharacter::Server_StopUseHeldProp_Implementation()
{
	if (HeldProp)
	{
		HeldProp->EndHoldUse();
	}
}

void AjinzzaCharacter::Server_DropHeldProp_Implementation()
{
	ServerForceDropHeldProp();
}

void AjinzzaCharacter::ServerForceDropHeldProp()
{
	if (!HasAuthority())
	{
		return;
	}

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

	RestoreControllerInputMode();

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
	// TEMP placeholder sound - plays independently of whether a montage exists yet, so emotes are
	// audible today even with no animation content.
	if (USoundBase* Sound = GetSoundForEmote(EmoteType))
	{
		JinzzaAudio::PlaySoundAt(this, Sound, GetActorLocation(), ActionSoundAudibleRadius);
	}

	// TEMP on-screen confirmation - there's no emote montage content yet (GetMontageForEmote below
	// returns null for every case until anim assets are assigned), so this is currently the only
	// visible signal on ANY client that an emote actually fired. Remove once real montages land and
	// the Montage_Play below is itself the visible feedback.
	if (GEngine)
	{
		FString EmoteName;
		switch (EmoteType)
		{
		case EJinzzaEmoteType::ThumbsUp:     EmoteName = TEXT("ThumbsUp"); break;
		case EJinzzaEmoteType::ThumbsDown:   EmoteName = TEXT("ThumbsDown"); break;
		case EJinzzaEmoteType::MiddleFinger: EmoteName = TEXT("MiddleFinger"); break;
		case EJinzzaEmoteType::Point:        EmoteName = TEXT("Point"); break;
		default:                              EmoteName = TEXT("None"); break;
		}
		const APlayerState* PS = GetPlayerState<APlayerState>();
		const FString PlayerLabel = PS ? PS->GetPlayerName() : GetName();
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 4.f, FColor::Yellow, FString::Printf(TEXT("%s: %s"), *PlayerLabel, *EmoteName));
	}

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

USoundBase* AjinzzaCharacter::GetSoundForEmote(EJinzzaEmoteType EmoteType) const
{
	switch (EmoteType)
	{
	case EJinzzaEmoteType::ThumbsUp:     return ThumbsUpSound;
	case EJinzzaEmoteType::ThumbsDown:   return ThumbsDownSound;
	case EJinzzaEmoteType::MiddleFinger: return MiddleFingerSound;
	case EJinzzaEmoteType::Point:        return PointSound;
	default:                             return nullptr;
	}
}
