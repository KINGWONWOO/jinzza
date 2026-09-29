// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"

class UInputAction;
class ULocalPlayer;
class UUserWidget;

/**
 * Which key is actually bound to an input action right now, for on-screen key icons ("[E] Pick Up",
 * the lobby kiosk prompt, ...).
 *
 * Reads the live Enhanced Input mappings of the local player - i.e. the runtime mapping contexts
 * AjinzzaPlayerController installs, which already have the player's rebinds from the Settings screen
 * applied - so a prompt always shows the key that will really fire, never a hardcoded letter.
 */
namespace JinzzaInput
{
	/** Name of the push-to-talk action. It has no .uasset: AjinzzaPlayerController creates it at runtime and
	 * maps it into its runtime mapping context; the Settings screen rebinds it under this name like any other action. */
	JINZZA_API FName GetPushToTalkActionName();

	/** Key push-to-talk uses until the player rebinds it. */
	JINZZA_API FKey GetDefaultPushToTalkKey();

	/** IA_Interact (props and lobby kiosks). Loaded once and cached. */
	JINZZA_API UInputAction* GetInteractAction();

	/** First keyboard/mouse key currently mapped to Action for LocalPlayer (gamepad keys only if nothing else is
	 * mapped), or EKeys::Invalid if the action isn't mapped / the player has no Enhanced Input subsystem yet. */
	JINZZA_API FKey GetBoundKey(const ULocalPlayer* LocalPlayer, const UInputAction* Action);

	/** Short label for a key cap: "E", "Space Bar", "LMB"/"RMB"/"MMB" for mouse buttons, "?" for an invalid key. */
	JINZZA_API FText GetKeyCapText(const FKey& Key);

	/** The local player a widget belongs to: its owning player, or - for a widget hosted on a WidgetComponent,
	 * which often has none - the world's first local player. */
	JINZZA_API ULocalPlayer* ResolveLocalPlayer(const UUserWidget* Widget);
}
