// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HAL/IConsoleManager.h"

/**
 *  Console variables for the lag-compensation debug system.
 *
 *  Usage (in UE console):
 *    lc.Debug 1   — draw rewound hitbox spheres for 3 s each time a shot is fired (server)
 *    lc.Debug 0   — disable debug drawing (default)
 */

/** Global debug draw toggle — declared here, defined in LagCompensationComponent.cpp */
extern TAutoConsoleVariable<int32> CVarLagCompDebug;
