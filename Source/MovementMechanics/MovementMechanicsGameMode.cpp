// Copyright Epic Games, Inc. All Rights Reserved.

#include "MovementMechanicsGameMode.h"
#include "MovementMechanicsCharacter.h"
#include "UObject/ConstructorHelpers.h"

AMovementMechanicsGameMode::AMovementMechanicsGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

}
