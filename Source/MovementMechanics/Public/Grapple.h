// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Grapple.generated.h"

UCLASS()
class MOVEMENTMECHANICS_API AGrapple : public AActor
{
	GENERATED_BODY()

public:	

	// Sphere collision component.
	UPROPERTY(VisibleDefaultsOnly, Category = Projectile)
		USphereComponent* CollisionComponent;

	/** Projectile movement component */
	UPROPERTY(VisibleAnywhere, Category = Movement)
		UProjectileMovementComponent* ProjectileMovement;

	// Projectile mesh
	UPROPERTY(VisibleAnywhere)
		UStaticMeshComponent* HookMeshComponent;

	// max length for the grappke to work
	UPROPERTY(EditAnywhere)
		float MaxDistance = 3000.0f;

	AGrapple();


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	FVector Velocity;
	FVector StartLocation;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	void SetVelocity(FVector);
	void SetMaxDistance(float);

	USphereComponent* GetCollisionComponent();
	UStaticMeshComponent* GetMeshComponent() {	return HookMeshComponent;};


};
