// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "Grapple.h"
#include "GrappleCable.h"
#include "Components/ActorComponent.h"
#include "GrapplingHookComponent.generated.h"

UENUM()
enum UGrappleState
{
	READY    UMETA(DisplayName = "READY"),
	FIRING     UMETA(DisplayName = "FIRING"),
	ATTACHED   UMETA(DisplayName = "ATTACHED"),
};



UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MOVEMENTMECHANICS_API UGrapplingHookComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UGrapplingHookComponent();

	UPROPERTY(EditAnywhere)
		float GrappleSpeed = 7500.0f;
	UPROPERTY(EditAnywhere)
		float PullInitialSpeed = 1500.0f;
	UPROPERTY(EditAnywhere)
		float PerTickPulForce = 100000.0f;
	UPROPERTY(EditAnywhere)
		float DisconnectDistance = 250.0f;


	
	// Use TSubclassOf to determine the base type of the Actor you want to spawn.
		// You can expose the variable to the editor with UPROPERTY()
		// You can also get the class type of any object with its StaticClass() function

	UPROPERTY(EditAnywhere)
		TSubclassOf<AGrapple> HookClass = AGrapple::StaticClass();
	AGrapple* GrappleHook;
	
	UPROPERTY(VisibleAnywhere)
		TSubclassOf<AGrappleCable> CableClass = AGrappleCable::StaticClass();
	AGrappleCable* GrappleCable;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	UGrappleState GrappleState = READY;
	FVector InitialHookDirection2D;
	float TimeSinceLastGrappleDetach = 1000.0f;
public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// see if grapple is being used
	// called by the player character when the grapple fire key is pressed
	bool IsInUse();
	// see if grapple is attached
	bool IsGrappleAttached();

	void FireGrapple(FVector targetLocation, FVector localOffset);
	void DetachGrapple();
	// returns the location of the start of the grapple cable
	FVector CableStartLocation(FVector localOffset);
	// returns a direction vector from the player location to the grapple hook location
	FVector ToGrappleHook();
	// returns a direction vector from the player location to the grapple hook location but in 2D (no Z)
	// used to detach grapple if we sing past it
	FVector ToGrappleHook2D();

	float GetTimeSinceLastGrappleDetach() { return TimeSinceLastGrappleDetach; };

	
private:
	UFUNCTION()
		void OnGrappleHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
	UFUNCTION()
		void OnGrappleDestroyed(AActor* Act);
};
