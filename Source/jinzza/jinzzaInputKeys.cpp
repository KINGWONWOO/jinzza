// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaInputKeys.h"
#include "Blueprint/UserWidget.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"

FName JinzzaInput::GetPushToTalkActionName()
{
	static const FName Name(TEXT("IA_PushToTalk"));
	return Name;
}

FKey JinzzaInput::GetDefaultPushToTalkKey()
{
	return EKeys::V;
}

UInputAction* JinzzaInput::GetInteractAction()
{
	static TWeakObjectPtr<UInputAction> Cached;
	if (!Cached.IsValid())
	{
		Cached = LoadObject<UInputAction>(nullptr, TEXT("/Game/JINZZA/Input/Actions/IA_Interact.IA_Interact"));
	}
	return Cached.Get();
}

FKey JinzzaInput::GetBoundKey(const ULocalPlayer* LocalPlayer, const UInputAction* Action)
{
	if (!LocalPlayer || !Action)
	{
		return EKeys::Invalid;
	}

	const UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	if (!Subsystem)
	{
		return EKeys::Invalid;
	}

	FKey Fallback = EKeys::Invalid;
	for (const FKey& Key : Subsystem->QueryKeysMappedToAction(Action))
	{
		if (!Key.IsGamepadKey())
		{
			return Key;
		}
		if (!Fallback.IsValid())
		{
			Fallback = Key;
		}
	}
	return Fallback;
}

FText JinzzaInput::GetKeyCapText(const FKey& Key)
{
	if (!Key.IsValid())
	{
		return FText::FromString(TEXT("?"));
	}
	if (Key == EKeys::LeftMouseButton)
	{
		return FText::FromString(TEXT("LMB"));
	}
	if (Key == EKeys::RightMouseButton)
	{
		return FText::FromString(TEXT("RMB"));
	}
	if (Key == EKeys::MiddleMouseButton)
	{
		return FText::FromString(TEXT("MMB"));
	}
	return Key.GetDisplayName(false);
}

ULocalPlayer* JinzzaInput::ResolveLocalPlayer(const UUserWidget* Widget)
{
	if (!Widget)
	{
		return nullptr;
	}
	if (ULocalPlayer* Owning = Widget->GetOwningLocalPlayer())
	{
		return Owning;
	}
	const UWorld* World = Widget->GetWorld();
	return World ? World->GetFirstLocalPlayerFromController() : nullptr;
}
