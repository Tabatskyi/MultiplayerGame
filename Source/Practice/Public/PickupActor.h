// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PickupActor.generated.h"

UCLASS()
class PRACTICE_API APickupActor : public AActor
{
    GENERATED_BODY()
    
public:	
    APickupActor();

protected:
    virtual void BeginPlay() override;

public:	
    virtual void Tick(float DeltaTime) override;

    virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;

private:
    UPROPERTY(VisibleAnywhere, Category = "Components")
    class UStaticMeshComponent* PickupMesh;

    FString GetRoleString(ENetRole Role);
};