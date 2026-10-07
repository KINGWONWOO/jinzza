// Copyright Epic Games, Inc. All Rights Reserved.

#include "jinzzaLoadingSettings.h"

UjinzzaLoadingSettings::UjinzzaLoadingSettings()
{
	CategoryName = TEXT("Game");

	auto Paths = [](std::initializer_list<const TCHAR*> List)
	{
		TArray<FSoftObjectPath> Result;
		for (const TCHAR* Path : List)
		{
			Result.Emplace(Path);
		}
		return Result;
	};

	CommonPreloadAssets = Paths({
		TEXT("/Game/JINZZA/Fonts/SacheonUju-Regular_Font.SacheonUju-Regular_Font"),
		TEXT("/Game/JINZZA/UI/Textures/T_Logo.T_Logo"),
		TEXT("/Game/JINZZA/UI/Textures/T_ButtonPill.T_ButtonPill"),
		TEXT("/Game/JINZZA/UI/Textures/T_NotePanel.T_NotePanel"),
		TEXT("/Game/JINZZA/Audio/Sounds/UISounds/ButtonHover_Cue.ButtonHover_Cue"),
		TEXT("/Game/JINZZA/Audio/Sounds/UISounds/ButtonClickPopSound_Cue.ButtonClickPopSound_Cue"),
		TEXT("/Game/JINZZA/Audio/Sounds/UISounds/LobbyPannelOpen__cut_1sec_.LobbyPannelOpen__cut_1sec_"),
		TEXT("/Game/JINZZA/Input/IMC_Default.IMC_Default"),
		TEXT("/Game/JINZZA/Input/IMC_Sprint.IMC_Sprint"),
		TEXT("/Game/JINZZA/Input/IMC_MouseLook.IMC_MouseLook"),
		TEXT("/Game/JINZZA/Input/Actions/IA_Interact.IA_Interact"),
		TEXT("/Game/JINZZA/Input/Actions/IA_EmoteWheel.IA_EmoteWheel"),
	});

	// Character + props + their sounds are shared by the lobby and the match.
	const TArray<FSoftObjectPath> PlayLevelAssets = Paths({
		TEXT("/Game/JINZZA/Characters/seal/SM_Seal.SM_Seal"),
		TEXT("/Game/JINZZA/Audio/Sounds/FootStep/Footstep.Footstep"),
		TEXT("/Game/JINZZA/Audio/Sounds/FootStep/FootstepGrass.FootstepGrass"),
		TEXT("/Game/JINZZA/Audio/Sounds/Emotes/WrongAnswerSound.WrongAnswerSound"),
		TEXT("/Game/JINZZA/Audio/Sounds/Bat/Punch1.Punch1"),
		TEXT("/Game/JINZZA/Audio/Sounds/Basketball/InteractWoodenBox__cut_0sec_.InteractWoodenBox__cut_0sec_"),
		TEXT("/Game/JINZZA/Audio/Sounds/BasketballHoop/correctanswer.correctanswer"),
		TEXT("/Game/JINZZA/Audio/Sounds/Megaphone/PressButton.PressButton"),
		TEXT("/Game/JINZZA/Props/Meshes/Bat/SM_Bat.SM_Bat"),
		TEXT("/Game/JINZZA/Props/Meshes/Basketball/SM_Basketball.SM_Basketball"),
		TEXT("/Game/JINZZA/Props/Meshes/Megaphone/SM_Megaphone.SM_Megaphone"),
		TEXT("/Game/JINZZA/Props/Meshes/StunGun/SM_StunGun.SM_StunGun"),
		TEXT("/Game/JINZZA/Props/Meshes/Boombox/SM_Boombox.SM_Boombox"),
		TEXT("/Game/JINZZA/Props/Meshes/StandMic/SM_StandMic.SM_StandMic"),
		TEXT("/Game/JINZZA/Materials/PostProcess/M_PP_InteractOutline.M_PP_InteractOutline"),
	});

	FJinzzaMapPreloadList Menu;
	Menu.MapName = TEXT("Lvl_MainMenu");
	Menu.Assets = Paths({
		TEXT("/Game/JINZZA/Audio/Sounds/MainMenu/MainMenuBgm_Cue.MainMenuBgm_Cue"),
		TEXT("/Game/JINZZA/Characters/CustomizationTest/M_Hair_Master.M_Hair_Master"),
		TEXT("/Game/JINZZA/Characters/CustomizationTest/M_Face_Master.M_Face_Master"),
	});
	MapPreloadAssets.Add(Menu);

	FJinzzaMapPreloadList Lobby;
	Lobby.MapName = TEXT("Lvl_Lobby");
	Lobby.Assets = PlayLevelAssets;
	Lobby.Assets.Append(Paths({
		TEXT("/Game/JINZZA/Audio/Sounds/Lobby/LobbyBgm__cut_83sec__Cue.LobbyBgm__cut_83sec__Cue"),
		TEXT("/Game/JINZZA/Audio/Sounds/Boombox/FruitGameBgm.FruitGameBgm"),
		TEXT("/Game/JINZZA/Characters/CustomizationTest/M_Hair_Master.M_Hair_Master"),
		TEXT("/Game/JINZZA/Characters/CustomizationTest/M_Face_Master.M_Face_Master"),
	}));
	MapPreloadAssets.Add(Lobby);

	FJinzzaMapPreloadList Game;
	Game.MapName = TEXT("Lvl_Game");
	Game.Assets = PlayLevelAssets;
	Game.Assets.Append(Paths({
		TEXT("/Game/JINZZA/Audio/Sounds/Game/QuizGameBgm.QuizGameBgm"),
		TEXT("/Game/JINZZA/Props/Meshes/HoopRim/SM_HoopRim.SM_HoopRim"),
		TEXT("/Game/JINZZA/Props/Meshes/Backboard/SM_Backboard.SM_Backboard"),
	}));
	MapPreloadAssets.Add(Game);

	Tips = {
		FText::FromString(TEXT("TIP: Hold your push-to-talk key to speak - only players near you can hear it.")),
		FText::FromString(TEXT("TIP: Press Tab to open the emote wheel.")),
		FText::FromString(TEXT("TIP: Press ESC any time for Settings or to leave the game.")),
		FText::FromString(TEXT("TIP: The Real One has to blend in - the Imitators know who it is.")),
		FText::FromString(TEXT("TIP: Walk up to a glowing kiosk in the lobby and press E to use it.")),
		FText::FromString(TEXT("TIP: Ghosts can still walk around and emote, but can't talk.")),
	};
}

TArray<FSoftObjectPath> UjinzzaLoadingSettings::GetPreloadAssetsForMap(const FString& MapName) const
{
	TArray<FSoftObjectPath> Result = CommonPreloadAssets;
	for (const FJinzzaMapPreloadList& Entry : MapPreloadAssets)
	{
		if (Entry.MapName == MapName)
		{
			Result.Append(Entry.Assets);
		}
	}
	return Result;
}
