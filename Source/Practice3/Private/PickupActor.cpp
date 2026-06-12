// Fill out your copyright notice in the Description page of Project Settings.

#include "PickupActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Logging/LogMacros.h"

// Sets default values
APickupActor::APickupActor()
{
    // Set this actor to call Tick() every frame. You can turn this off to improve performance if you don't need it.
    PrimaryActorTick.bCanEverTick = true;

    // Critical: Tell the engine this actor should replicate across the network
    // Must be set in the constructor, not BeginPlay
    bReplicates = true;

    // Create and attach a static mesh component so clients can see it
    PickupMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PickupMesh"));
    RootComponent = PickupMesh;

    // Enable overlaps so NotifyActorBeginOverlap fires on the server
    PickupMesh->SetGenerateOverlapEvents(true);
    PickupMesh->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
}

// Called when the game starts or when spawned
void APickupActor::BeginPlay()
{
    Super::BeginPlay();
    
    // Log on screen when spawned, displaying whether this instance is Authority or a Proxy
    {
        const FString RoleText = HasAuthority() ? TEXT("Authority") : TEXT("Proxy");
        if (GEngine)
        {
            const int32 SpawnMessageId = GetUniqueID() + 1;
            GEngine->AddOnScreenDebugMessage(SpawnMessageId, 2.0f, FColor::Yellow,
                                             FString::Printf(TEXT("Spawned with role: %s"), *RoleText));
        }
        UE_LOG(LogTemp, Log, TEXT("Spawned with role: %s"), *RoleText);
    }
}

// Called every frame
void APickupActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    {
        // Get local and remote roles
        ENetRole LocalRole = GetLocalRole();
        ENetRole RemoteRole = GetRemoteRole();

        // Convert them to strings
        FString LocalRoleStr = GetRoleString(LocalRole);
        FString RemoteRoleStr = GetRoleString(RemoteRole);

        // Format the debug string
        FString DebugMessage = FString::Printf(TEXT("Actor: %s | Local Role: %s | Remote Role: %s"), 
                                               *GetName(), *LocalRoleStr, *RemoteRoleStr);

        if (GEngine)
        {
            // Unique ID keeps one stable line per pickup on screen
            const int32 DebugMessageId = GetUniqueID();
            GEngine->AddOnScreenDebugMessage(DebugMessageId, 0.0f, FColor::Green, DebugMessage);
        }
    }
}

// Overridden overlap logic for handling the pickup
void APickupActor::NotifyActorBeginOverlap(AActor* OtherActor)
{
    Super::NotifyActorBeginOverlap(OtherActor);

    const FString OtherName = OtherActor ? OtherActor->GetName() : TEXT("None");
    UE_LOG(LogTemp, Log, TEXT("Overlap: %s with %s | HasAuthority=%d"),
           *GetName(), *OtherName, HasAuthority() ? 1 : 0);

    // CRITICAL: Only the server has authority to destroy replicated actors.
    // If a client calls Destroy(), nothing happens across the network.
    if (HasAuthority())
    {
        Destroy();
    }
}

// Helper function to turn network role enums into printable strings
FString APickupActor::GetRoleString(ENetRole Role)
{
    switch (Role)
    {
        case ROLE_Authority:
            return TEXT("ROLE_Authority");
        case ROLE_AutonomousProxy:
            return TEXT("ROLE_AutonomousProxy");
        case ROLE_SimulatedProxy:
            return TEXT("ROLE_SimulatedProxy");
        case ROLE_None:
            return TEXT("ROLE_None");
        default:
            return TEXT("Unknown");
    }
}