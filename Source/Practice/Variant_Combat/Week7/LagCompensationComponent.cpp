// Copyright Epic Games, Inc. All Rights Reserved.

#include "LagCompensationComponent.h"
#include "LagCompCVars.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/GameStateBase.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY(LogLagCompensation);

// ---------------------------------------------------------------------------
//  Console variable definition
//  Toggle with:  lc.Debug 1   (draw rewound hitboxes)
//                lc.Debug 0   (default — no drawing)
// ---------------------------------------------------------------------------
TAutoConsoleVariable<int32> CVarLagCompDebug(
	TEXT("lc.Debug"),
	0,
	TEXT("Draw rewound hitbox spheres (server) when a shot is processed.\n")
	TEXT("  0 = off (default)\n")
	TEXT("  1 = on"),
	ECVF_Cheat);

// ---------------------------------------------------------------------------
//  Construction
// ---------------------------------------------------------------------------

ULagCompensationComponent::ULagCompensationComponent()
{
	// We only need to tick on the server — the client never records history.
	PrimaryComponentTick.bCanEverTick = true;

	// Pre-size the history array for ~1 s at 20 Hz so we avoid repeated heap
	// allocations during play.
	FrameHistory.Reserve(25);
}

// ---------------------------------------------------------------------------
//  Lifecycle
// ---------------------------------------------------------------------------

void ULagCompensationComponent::BeginPlay()
{
	Super::BeginPlay();

	// Disable ticking on clients — they don't need the history buffer.
	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		SetComponentTickEnabled(false);
	}
}

void ULagCompensationComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                               FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Only the authoritative server records snapshots.
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	// Gate recording to SnapshotRate Hz.
	TimeSinceLastSnapshot += DeltaTime;
	const float SnapshotInterval = (SnapshotRate > 0.0f) ? (1.0f / SnapshotRate) : 0.05f;

	if (TimeSinceLastSnapshot >= SnapshotInterval)
	{
		TimeSinceLastSnapshot = 0.0f;
		RecordSnapshot();
	}
}

// ---------------------------------------------------------------------------
//  Snapshot recording
// ---------------------------------------------------------------------------

void ULagCompensationComponent::RecordSnapshot()
{
	const ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter)
	{
		return;
	}

	const USkeletalMeshComponent* Mesh = OwnerCharacter->GetMesh();
	if (!Mesh)
	{
		return;
	}

	// Retrieve the server-synchronized clock — identical value on server and client.
	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const AGameStateBase* GS = World->GetGameState<AGameStateBase>();
	if (!GS)
	{
		return;
	}

	const float ServerTime = GS->GetServerWorldTimeSeconds();

	// Sample per-bone world transforms for head and torso hitboxes.
	const FTransform HeadTransform  = Mesh->GetBoneTransform(Mesh->GetBoneIndex(HeadBoneName));
	const FTransform TorsoTransform = Mesh->GetBoneTransform(Mesh->GetBoneIndex(TorsoBoneName));

	FrameHistory.Add(FHitboxSnapshot(HeadTransform, TorsoTransform, ServerTime));

	// Remove stale entries so the buffer stays bounded.
	PruneHistory(ServerTime);

	UE_LOG(LogLagCompensation, VeryVerbose,
	       TEXT("[%s] Snapshot recorded @ %.4f  (buffer size: %d)"),
	       *GetOwner()->GetName(), ServerTime, FrameHistory.Num());
}

void ULagCompensationComponent::PruneHistory(float CurrentServerTime)
{
	// Walk from the oldest entry (index 0) and remove anything that has aged out.
	// We keep at least two frames so ConfirmHit always has a bracket to interpolate.
	while (FrameHistory.Num() > 2)
	{
		const float Age = CurrentServerTime - FrameHistory[0].Timestamp;
		if (Age > HistoryDuration)
		{
			FrameHistory.RemoveAt(0, 1, EAllowShrinking::No);
		}
		else
		{
			// History is ordered oldest→newest; once we find a non-stale frame
			// all later ones are also non-stale.
			break;
		}
	}
}

// ---------------------------------------------------------------------------
//  Hit confirmation (server only)
// ---------------------------------------------------------------------------

FServerSideRewindResult ULagCompensationComponent::ConfirmHit(
	const FVector& TraceStart,
	const FVector& TraceDir,
	float ClientTimestamp) const
{
	FServerSideRewindResult Result;

	// Guard: must be called on the server.
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		UE_LOG(LogLagCompensation, Warning,
		       TEXT("ConfirmHit called on non-authority instance for %s — skipped."),
		       *GetOwner()->GetName());
		return Result;
	}

	// Guard: need at least two frames to interpolate.
	if (FrameHistory.Num() < 2)
	{
		UE_LOG(LogLagCompensation, Warning,
		       TEXT("[%s] ConfirmHit: not enough history frames (%d)."),
		       *GetOwner()->GetName(), FrameHistory.Num());
		return Result;
	}

	// Validate the rewind window.  Reject timestamps that are either too old or in
	// the future.  The 250 ms ceiling prevents around-corner-kill exploits.
	const AGameStateBase* GS = GetWorld()->GetGameState<AGameStateBase>();
	if (!GS)
	{
		return Result;
	}

	const float ServerNow       = GS->GetServerWorldTimeSeconds();
	const float MaxRewindSec    = MaxAllowedRewindMs / 1000.0f;
	const float EarliestAllowed = ServerNow - MaxRewindSec;

	if (ClientTimestamp < EarliestAllowed || ClientTimestamp > ServerNow)
	{
		UE_LOG(LogLagCompensation, Warning,
		       TEXT("[%s] ConfirmHit: timestamp %.4f rejected (allowed %.4f – %.4f)."),
		       *GetOwner()->GetName(), ClientTimestamp, EarliestAllowed, ServerNow);
		return Result;
	}

	// -----------------------------------------------------------------------
	//  Find the two frames that straddle ClientTimestamp.
	//  FrameHistory is ordered oldest (index 0) → newest (last index).
	// -----------------------------------------------------------------------
	int32 NewerIdx  = INDEX_NONE;
	int32 OlderIdx  = INDEX_NONE;

	// If the request is older than our earliest frame, clamp to it.
	if (ClientTimestamp <= FrameHistory[0].Timestamp)
	{
		OlderIdx = 0;
		NewerIdx = 0;
	}
	// If the request is newer than our latest frame, clamp to it.
	else if (ClientTimestamp >= FrameHistory.Last().Timestamp)
	{
		OlderIdx = FrameHistory.Num() - 1;
		NewerIdx = FrameHistory.Num() - 1;
	}
	else
	{
		// Walk from newest to oldest to find the straddling pair quickly.
		for (int32 i = FrameHistory.Num() - 1; i > 0; --i)
		{
			if (FrameHistory[i - 1].Timestamp <= ClientTimestamp &&
			    FrameHistory[i].Timestamp     >= ClientTimestamp)
			{
				OlderIdx = i - 1;
				NewerIdx = i;
				break;
			}
		}
	}

	if (OlderIdx == INDEX_NONE || NewerIdx == INDEX_NONE)
	{
		UE_LOG(LogLagCompensation, Warning,
		       TEXT("[%s] ConfirmHit: could not bracket timestamp %.4f."),
		       *GetOwner()->GetName(), ClientTimestamp);
		return Result;
	}

	// -----------------------------------------------------------------------
	//  Interpolate transforms between the two frames.
	// -----------------------------------------------------------------------
	const FHitboxSnapshot Rewound = (OlderIdx == NewerIdx)
		? FrameHistory[OlderIdx]
		: InterpolateSnapshots(FrameHistory[OlderIdx], FrameHistory[NewerIdx], ClientTimestamp);

	const FVector HeadCenter  = Rewound.HeadTransform.GetLocation();
	const FVector TorsoCenter = Rewound.TorsoTransform.GetLocation();
	const FVector DirNorm     = TraceDir.GetSafeNormal();

	// -----------------------------------------------------------------------
	//  Debug drawing — enabled via "lc.Debug 1" in the console.
	// -----------------------------------------------------------------------
	if (CVarLagCompDebug.GetValueOnGameThread() != 0)
	{
		ShowDebugHitboxes(Rewound.HeadTransform, Rewound.TorsoTransform, DebugDrawDuration);

		// Also draw the trace ray for context.
		DrawDebugLine(GetWorld(),
		              TraceStart,
		              TraceStart + DirNorm * 5000.0f,
		              FColor::Yellow, false, DebugDrawDuration, 0, 1.5f);
	}

	// -----------------------------------------------------------------------
	//  Geometric line-sphere intersection — no physics query needed.
	// -----------------------------------------------------------------------

	// Check head first (higher priority for future headshot multipliers).
	if (LineIntersectsSphere(TraceStart, DirNorm, HeadCenter, HeadHitboxRadius))
	{
		Result.bHitConfirmed = true;
		Result.bHeadShot     = true;

		UE_LOG(LogLagCompensation, Log,
		       TEXT("[%s] ConfirmHit: HEAD hit confirmed (t=%.4f)."),
		       *GetOwner()->GetName(), ClientTimestamp);

		return Result;
	}

	// Check torso.
	if (LineIntersectsSphere(TraceStart, DirNorm, TorsoCenter, TorsoHitboxRadius))
	{
		Result.bHitConfirmed = true;
		Result.bHeadShot     = false;

		UE_LOG(LogLagCompensation, Log,
		       TEXT("[%s] ConfirmHit: TORSO hit confirmed (t=%.4f)."),
		       *GetOwner()->GetName(), ClientTimestamp);

		return Result;
	}

	UE_LOG(LogLagCompensation, VeryVerbose,
	       TEXT("[%s] ConfirmHit: miss (t=%.4f)."),
	       *GetOwner()->GetName(), ClientTimestamp);

	return Result;
}

// ---------------------------------------------------------------------------
//  Debug visualisation
// ---------------------------------------------------------------------------

void ULagCompensationComponent::ShowDebugHitboxes(const FTransform& HeadT,
                                                   const FTransform& TorsoT,
                                                   float Duration) const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Head sphere — red
	DrawDebugSphere(World, HeadT.GetLocation(),  HeadHitboxRadius,  12,
	                FColor::Red,   false, Duration, 0, 1.0f);

	// Torso sphere — orange
	DrawDebugSphere(World, TorsoT.GetLocation(), TorsoHitboxRadius, 12,
	                FColor::Orange, false, Duration, 0, 1.0f);

	// Axis indicators for orientation
	const float AxisLen = 20.0f;

	DrawDebugLine(World, HeadT.GetLocation(),
	              HeadT.GetLocation() + HeadT.GetRotation().GetForwardVector() * AxisLen,
	              FColor::Blue, false, Duration, 0, 0.5f);

	DrawDebugLine(World, TorsoT.GetLocation(),
	              TorsoT.GetLocation() + TorsoT.GetRotation().GetForwardVector() * AxisLen,
	              FColor::Blue, false, Duration, 0, 0.5f);
}

// ---------------------------------------------------------------------------
//  Internal helpers
// ---------------------------------------------------------------------------

FHitboxSnapshot ULagCompensationComponent::InterpolateSnapshots(
	const FHitboxSnapshot& OlderFrame,
	const FHitboxSnapshot& NewerFrame,
	float TargetTime) const
{
	// Compute normalised alpha in [0,1].
	const float FrameRange = NewerFrame.Timestamp - OlderFrame.Timestamp;
	const float Alpha = (FrameRange > SMALL_NUMBER)
		? FMath::Clamp((TargetTime - OlderFrame.Timestamp) / FrameRange, 0.0f, 1.0f)
		: 0.0f;

	FHitboxSnapshot Out;
	Out.Timestamp = TargetTime;

	// Interpolate location and rotation independently.
	// FTransform::Blend() handles scale as well, which is harmless here.
	Out.HeadTransform.Blend(OlderFrame.HeadTransform,   NewerFrame.HeadTransform,  Alpha);
	Out.TorsoTransform.Blend(OlderFrame.TorsoTransform, NewerFrame.TorsoTransform, Alpha);

	return Out;
}

bool ULagCompensationComponent::LineIntersectsSphere(
	const FVector& Origin,
	const FVector& Dir,
	const FVector& Center,
	float Radius) const
{
	// Vector from ray origin to sphere centre.
	const FVector OC = Center - Origin;

	// Project OC onto the ray direction.
	const float T = FVector::DotProduct(OC, Dir);

	// Closest point on the ray to the sphere centre.
	const FVector Closest = Origin + Dir * FMath::Max(T, 0.0f);

	// Squared distance from that closest point to the sphere centre.
	const float DistSq = FVector::DistSquared(Closest, Center);

	return DistSq <= (Radius * Radius);
}
