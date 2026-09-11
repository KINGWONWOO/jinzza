// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class AActor;
class USkeletalMeshComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class UjinzzaGameUserSettings;

/**
 * Shared "apply the local player's saved appearance to a skeletal mesh" logic - the actual
 * material-swap/socket-mesh-swap work behind both UjinzzaCharacterCustomizationComponent (the
 * real playable character) and AjinzzaCharacterPreviewCapture (the live preview shown on the
 * Customization screen's left panel / main menu), so the two stay visually identical without
 * duplicating this ~60-line block per caller.
 *
 * Head style -> M_Face_Master's "FaceIndex" scalar param (slot 0 of Mesh, via a lazily-created
 * dynamic material instance - a round disguise assigned later just overwrites this same param,
 * see UjinzzaDisguiseComponent). Hair color -> a lazily-created placeholder mesh socketed to the
 * skeleton's "Head" socket, tinted via M_Hair_Master's "HairColor" param. Accessory style -> a
 * second placeholder mesh on the same socket, hidden entirely when None, tinted via the same
 * M_Hair_Master (reused rather than authoring a dedicated accessory material - there's no real
 * accessory content yet either, see jinzzaCustomizationTypes.h). Top/Eyebrows/Eyes still have no
 * matching content and stay no-ops.
 *
 * DEFERRED, not implemented here: a square region on the character's face showing the local
 * player's Steam profile picture. Two things need to exist first - a texture param on
 * M_Face_Master (a material-graph edit, needs hands-on editor time - this project's materials
 * aren't scriptable remotely, see docs/unreal-mcp-gotchas) and an actual Steam avatar fetch
 * (ISteamFriends::GetLargeFriendAvatar via OnlineSubsystemSteam, converted to a UTexture2D -
 * a real feature, not a stub). Once both exist, wire it in here the same way FaceIndex/HairColor
 * are wired: SetTextureParameterValue on FaceMaterial, same slot-0 dynamic-MID contract.
 */
namespace JinzzaCustomization
{
	void ApplyToMesh(
		USkeletalMeshComponent* Mesh,
		AActor* MeshOwner,
		TObjectPtr<UMaterialInstanceDynamic>& FaceMaterial,
		TObjectPtr<UStaticMeshComponent>& HairMesh,
		TObjectPtr<UMaterialInstanceDynamic>& HairMaterial,
		TObjectPtr<UStaticMeshComponent>& AccessoryMesh,
		TObjectPtr<UMaterialInstanceDynamic>& AccessoryMaterial,
		const UjinzzaGameUserSettings& Settings);
}
