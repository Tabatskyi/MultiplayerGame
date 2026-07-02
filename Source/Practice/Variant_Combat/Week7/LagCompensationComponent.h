// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LagCompensationTypes.h"
#include "LagCompensationComponent.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogLagCompensation, Log, All);

/**
 *  ULagCompensationComponent
 *
 *  Attach one of these to every replicated character.
 *  On the SERVER it records a ring-buffer of FHitboxSnapshot entries (~20 Hz)
 *  covering the last 1.0 second of movement.
 *
 *  When a client fires a weapon it sends its local server-synchronized timestamp
 *  via the ServerFireWeapon RPC.  The weapon calls ConfirmHit() here, which:
 *    1. Finds the two snapshots that straddle the client timestamp.
 *    2. Interpolates head + torso transforms between them.
 *    3. Performs a line trace against the interpolated capsule/sphere volumes.
 *    4. Returns the hit result so the weapon can apply damage.
 *
 *  The component also supports visual debugging: when lc.Debug is 1 it draws
 *  the rewound hitbox spheres for a configurable duration.
 */
UCLASS(ClassGroup=(LagCompensation), meta=(BlueprintSpawnableComponent))
class ULagCompensationComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	ULagCompensationComponent();

	// ------------------------------------------------------------------
	// Configuration — editable per-character in Blueprint defaults
	// ------------------------------------------------------------------

	/** Radius of the head hitbox sphere used for line-trace confirmation (cm) */
	UPROPERTY(EditAnywhere, Category="Lag Compensation|Hitboxes", meta=(ClampMin=5, ClampMax=50, Units="cm"))
	float HeadHitboxRadius = 15.0f;

	/** Radius of the torso hitbox sphere used for line-trace confirmation (cm) */
	UPROPERTY(EditAnywhere, Category="Lag Compensation|Hitboxes", meta=(ClampMin=5, ClampMax=100, Units="cm"))
	float TorsoHitboxRadius = 22.0f;

	/** Name of the head bone/socket on the owner's SkeletalMesh */
	UPROPERTY(EditAnywhere, Category="Lag Compensation|Hitboxes")
	FName HeadBoneName = FName("head");

	/** Name of the torso (chest) bone/socket on the owner's SkeletalMesh */
	UPROPERTY(EditAnywhere, Category="Lag Compensation|Hitboxes")
	FName TorsoBoneName = FName("spine_03");

	/** How long (seconds) to retain snapshot history.  Shots older than this are rejected. */
	UPROPERTY(EditAnywhere, Category="Lag Compensation|Buffer", meta=(ClampMin=0.1, ClampMax=2.0, Units="s"))
	float HistoryDuration = 1.0f;

	/** Snapshot capture rate in Hz — keep ≤ 20 to bound buffer size */
	UPROPERTY(EditAnywhere, Category="Lag Compensation|Buffer", meta=(ClampMin=5, ClampMax=60))
	float SnapshotRate = 20.0f;

	/**
	 *  Maximum age a client timestamp may claim, relative to the current server time.
	 *  Shots claiming to have fired more than this many milliseconds ago are rejected.
	 *  250 ms prevents the "around-corner kill" exploit while still compensating for
	 *  typical competitive latencies (≤ 150 ms RTT one-way lag).
	 */
	UPROPERTY(EditAnywhere, Category="Lag Compensation|Validation", meta=(ClampMin=50, ClampMax=1000, Units="ms"))
	float MaxAllowedRewindMs = 250.0f;

	/** How long to display rewound hitbox debug spheres (seconds). Requires lc.Debug 1. */
	UPROPERTY(EditAnywhere, Category="Lag Compensation|Debug", meta=(ClampMin=0.1, ClampMax=10.0, Units="s"))
	float DebugDrawDuration = 3.0f;

	// ------------------------------------------------------------------
	// Public API used by the weapon
	// ------------------------------------------------------------------

	/**
	 *  Confirm whether a line trace starting at TraceStart in direction TraceDir would
	 *  have hit this character at the moment the client fired (ClientTimestamp).
	 *
	 *  Must be called on the SERVER; returns a default (no-hit) result otherwise.
	 *
	 *  @param TraceStart       World-space muzzle position from the client RPC
	 *  @param TraceDir         Normalised aim direction from the client RPC
	 *  @param ClientTimestamp  GetServerWorldTimeSeconds() value recorded on the client
	 *  @return                 Filled FServerSideRewindResult (bHitConfirmed, bHeadShot)
	 */
	FServerSideRewindResult ConfirmHit(const FVector& TraceStart, const FVector& TraceDir, float ClientTimestamp) const;

	/**
	 *  Draw rewound hitbox spheres at the interpolated transforms for a shot timestamp.
	 *  Called from ConfirmHit when lc.Debug is non-zero.
	 */
	void ShowDebugHitboxes(const FTransform& HeadT, const FTransform& TorsoT, float Duration) const;

protected:

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:

	// ------------------------------------------------------------------
	// Internal helpers
	// ------------------------------------------------------------------

	/** Record a new snapshot of the owner's head and torso bone transforms */
	void RecordSnapshot();

	/**
	 *  Remove all snapshots older than HistoryDuration seconds from the front of the
	 *  buffer so the deque stays bounded.
	 */
	void PruneHistory(float CurrentServerTime);

	/**
	 *  Given two snapshots, return an interpolated one at the requested time.
	 *  Alpha = 0 → OlderFrame, Alpha = 1 → NewerFrame.
	 */
	FHitboxSnapshot InterpolateSnapshots(const FHitboxSnapshot& OlderFrame,
	                                      const FHitboxSnapshot& NewerFrame,
	                                      float TargetTime) const;

	/**
	 *  Check whether a line (Origin + t*Dir) intersects a sphere at Center with Radius.
	 *  Avoids triggering any physics query — pure geometric test.
	 */
	bool LineIntersectsSphere(const FVector& Origin, const FVector& Dir,
	                          const FVector& Center, float Radius) const;

	// ------------------------------------------------------------------
	// State
	// ------------------------------------------------------------------

	/**
	 *  Circular frame history: index 0 is the oldest, last index is the most recent.
	 *  Only populated on the authoritative server.
	 */
	TArray<FHitboxSnapshot> FrameHistory;

	/** Accumulated time since the last snapshot, used to gate captures at SnapshotRate */
	float TimeSinceLastSnapshot = 0.0f;
};
