// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "LagCompensationTypes.generated.h"

/**
 *  Stores world-space transforms for the head and torso hitboxes of a character,
 *  together with the server timestamp at which the snapshot was recorded.
 *
 *  Recorded server-side at ~20 Hz on every replicated character.
 *  The buffer is kept to the most recent 1.0 second of history (≤ 20 entries).
 */
USTRUCT(BlueprintType)
struct FHitboxSnapshot
{
	GENERATED_BODY()

	/** World-space transform of the head hitbox bone/socket at this moment */
	UPROPERTY()
	FTransform HeadTransform;

	/** World-space transform of the torso (chest) hitbox bone/socket at this moment */
	UPROPERTY()
	FTransform TorsoTransform;

	/**
	 *  Server-synchronized time (seconds) at which this snapshot was captured.
	 *  Obtained via AGameStateBase::GetServerWorldTimeSeconds() — returns the same
	 *  clock value on both server and client, so no offset math is required.
	 */
	UPROPERTY()
	float Timestamp = 0.0f;

	FHitboxSnapshot() = default;

	FHitboxSnapshot(const FTransform& InHead, const FTransform& InTorso, float InTimestamp)
		: HeadTransform(InHead)
		, TorsoTransform(InTorso)
		, Timestamp(InTimestamp)
	{}
};

/**
 *  Result returned by ULagCompensationComponent::ConfirmHit.
 *  Tells the weapon whether the rewound trace struck a target, and which region.
 */
USTRUCT(BlueprintType)
struct FServerSideRewindResult
{
	GENERATED_BODY()

	/** True if the rewound line trace intersected a hitbox */
	UPROPERTY(BlueprintReadOnly)
	bool bHitConfirmed = false;

	/** True when the confirmed hit was the head hitbox (future head-shot multiplier hook) */
	UPROPERTY(BlueprintReadOnly)
	bool bHeadShot = false;
};
