// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"

#include "MovementMechanicsCharacter.generated.h"
class UInputComponent;
class USkeletalMeshComponent;
class USceneComponent;
class UCameraComponent;
class UAnimMontage;
class USoundBase;
class UGrapplingHookComponent;

UENUM()
enum WallSideENUM
{
	LEFT    UMETA(DisplayName = "LEFT"),
	RIGHT     UMETA(DisplayName = "RIGHT"),
};


UENUM()
enum MovementStatus
{
	WALL_RUNNING   UMETA(DisplayName = "WALL_RUNNING"),
	GRAPPLING     UMETA(DisplayName = "GRAPPLING"),
};

// Declaration of the delegate that will be called when the Primary Action is triggered
// It is declared as dynamic so it can be accessed also in Blueprints
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnUseItem);

UCLASS(config=Game)
class AMovementMechanicsCharacter : public ACharacter
{

	GENERATED_BODY()


	/** Pawn mesh: 1st person view (arms; seen only by self) */
	UPROPERTY(VisibleDefaultsOnly, Category=Mesh)
	USkeletalMeshComponent* Mesh1P;

	/** First person camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FirstPersonCameraComponent;

	/** Movement component used for movement logic in various movement modes (walking, falling, etc), containing relevant settings and functions to control movement. */
	UPROPERTY(Category = Character, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		TObjectPtr<UCharacterMovementComponent> PlayerCharacterMovement;

public:
	AMovementMechanicsCharacter();

protected:
	virtual void BeginPlay();

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
		TObjectPtr<UGrapplingHookComponent> GrappleHookComponent;

	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float TurnRateGamepad;

	/** Delegate to whom anyone can subscribe to receive this event */
	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FOnUseItem OnUseItem;

	// make this variable acessible by blueprints
	// used in UI
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Attributes)
		float TimeSinceLastGrappleDetach = 1000.0f;
protected:
	
	/** Fires a projectile. */
	void OnPrimaryAction();

	/** Handles moving forward/backward */
	void MoveForward(float Val);

	/** Handles strafing movement, left and right */
	void MoveRight(float Val);

	/**
	 * Called via input to turn at a given rate.
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void TurnAtRate(float Rate);

	/**
	 * Called via input to turn look up/down at a given rate.
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void LookUpAtRate(float Rate);

	struct TouchData
	{
		TouchData() { bIsPressed = false;Location=FVector::ZeroVector;}
		bool bIsPressed;
		ETouchIndex::Type FingerIndex;
		FVector Location;
		bool bMoved;
	};
	void BeginTouch(const ETouchIndex::Type FingerIndex, const FVector Location);
	void EndTouch(const ETouchIndex::Type FingerIndex, const FVector Location);
	void TouchUpdate(const ETouchIndex::Type FingerIndex, const FVector Location);
	TouchData	TouchItem;
	
	void Jump() override;
	void ResetJumpState() override;
	void Landed(const FHitResult& Hit) override;

	UFUNCTION()
		void OnCompHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
	
	// wall running
	FVector FindLaunchVelocity();
	bool CanSurfaceBeWallRan(const FVector ImpactNormal);
	void FindRunDirectionAndSide(FVector wallNormal);
	bool AreRequiredKeysDown();
	void BeginWallRun();
	void EndWallRun();
	void UpdateWallRun(FHitResult hit);
	bool ShootRayToWall(FHitResult& hit);
	void HandleCameraRotation();

	void ClampHorizontalVelocity();

	void Tick(float deltaTime) override;
	// grapple
	void UseGrapple();;
	void ShootGrappleRay();
	// Used to set the start position of the grapple based on the side
	// of the wall that the player is wall running on
	FVector SetGrappleLocalOffset();
protected:
	// APawn interface
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;
	// End of APawn interface

	/* 
	 * Configures input for touchscreen devices if there is a valid touch interface for doing so 
	 *
	 * @param	InputComponent	The input component pointer to bind controls to
	 * @returns true if touch controls were enabled.
	 */
	bool EnableTouchscreenMovement(UInputComponent* InputComponent);

public:
	/** Returns Mesh1P subobject **/
	USkeletalMeshComponent* GetMesh1P() const { return Mesh1P; }
	/** Returns FirstPersonCameraComponent subobject **/
	UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }

	float GetGrappleCooldown() { return GrappleCooldown; };
	float GetTimeSinceLastGrappleDetach() { return TimeSinceLastGrappleDetach; };



	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Attributes)
		float WalkingSpeed = 1100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Attributes)
		float GravityScale = 0.6f;

	// make sure player is high enough to start wall run
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Attributes)
		float WallHeight = 200.0f;

	float NormalGravity = 0.0f;
	float ForwardAxis;
	float RightAxis;
	bool WallRunning = false;

	WallSideENUM WallSide;
	FVector WallRunDirection;
	
	// status of the camera is tilted or not
	bool CameraTilted = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Attributes)
		float GrappleCooldown = 5.0f;

};

