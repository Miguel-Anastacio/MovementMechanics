// Copyright Epic Games, Inc. All Rights Reserved.

#include "MovementMechanicsCharacter.h"
#include "MovementMechanicsProjectile.h"
#include "GrapplingHookComponent.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/InputSettings.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"

//////////////////////////////////////////////////////////////////////////
// AMovementMechanicsCharacter

AMovementMechanicsCharacter::AMovementMechanicsCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);

	// set our turn rates for input
	TurnRateGamepad = 45.f;

	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->SetRelativeLocation(FVector(-39.56f, 1.75f, 64.f)); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(true);
	Mesh1P->SetupAttachment(FirstPersonCameraComponent);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	Mesh1P->SetRelativeRotation(FRotator(1.9f, -19.19f, 5.2f));
	Mesh1P->SetRelativeLocation(FVector(-0.5f, -4.4f, -155.7f));

	GrappleHookComponent = CreateDefaultSubobject<UGrapplingHookComponent>(TEXT("GrapplingHookComponent"));
}

void AMovementMechanicsCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();
	PlayerCharacterMovement = GetCharacterMovement();
	PlayerCharacterMovement->MaxWalkSpeed = 800;
	GetCapsuleComponent()->OnComponentHit.AddDynamic(this, &AMovementMechanicsCharacter::OnCompHit);

	if (!GrappleHookComponent)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("ERROR WITH GRAPLE HOOK COMPONENT"));
	}
}

//////////////////////////////////////////////////////////////////////////// Input

void AMovementMechanicsCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);


	// Bind jump events
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &AMovementMechanicsCharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);
	// Bind grapple event
	PlayerInputComponent->BindAction("FireGrapple", IE_Pressed, this, &AMovementMechanicsCharacter::UseGrapple);

	// Bind fire event
	PlayerInputComponent->BindAction("PrimaryAction", IE_Pressed, this, &AMovementMechanicsCharacter::OnPrimaryAction);

	// Enable touchscreen input
	EnableTouchscreenMovement(PlayerInputComponent);

	// Bind movement events
	PlayerInputComponent->BindAxis("Move Forward / Backward", this, &AMovementMechanicsCharacter::MoveForward);
	PlayerInputComponent->BindAxis("Move Right / Left", this, &AMovementMechanicsCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "Mouse" versions handle devices that provide an absolute delta, such as a mouse.
	// "Gamepad" versions are for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn Right / Left Mouse", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("Look Up / Down Mouse", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("Turn Right / Left Gamepad", this, &AMovementMechanicsCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("Look Up / Down Gamepad", this, &AMovementMechanicsCharacter::LookUpAtRate);
}

void AMovementMechanicsCharacter::OnPrimaryAction()
{
	// Trigger the OnItemUsed Event
	OnUseItem.Broadcast();
}

void AMovementMechanicsCharacter::BeginTouch(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	if (TouchItem.bIsPressed == true)
	{
		return;
	}
	if ((FingerIndex == TouchItem.FingerIndex) && (TouchItem.bMoved == false))
	{
		OnPrimaryAction();
	}
	TouchItem.bIsPressed = true;
	TouchItem.FingerIndex = FingerIndex;
	TouchItem.Location = Location;
	TouchItem.bMoved = false;
}

void AMovementMechanicsCharacter::EndTouch(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	if (TouchItem.bIsPressed == false)
	{
		return;
	}
	TouchItem.bIsPressed = false;
}

void AMovementMechanicsCharacter::Jump()
{
	bPressedJump = true;
	JumpKeyHoldTime = 0.0f;
	if (JumpCurrentCount < JumpMaxCount)
		LaunchCharacter(FindLaunchVelocity(), false, false);
	if (WallRunning)
		EndWallRun();
}
void AMovementMechanicsCharacter::ResetJumpState()
{
	bPressedJump = false;
	bWasJumping = false;
	JumpKeyHoldTime = 0.0f;
	JumpForceTimeRemaining = 0.0f;

	if (PlayerCharacterMovement && !PlayerCharacterMovement->IsFalling())
	{
		JumpCurrentCount = 0;
		JumpCurrentCountPreJump = 0;
	}
}

void AMovementMechanicsCharacter::Landed(const FHitResult& Hit)
{
	EndWallRun();
}

void AMovementMechanicsCharacter::OnCompHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (CanSurfaceBeWallRan(Hit.ImpactNormal))
	{
		if(PlayerCharacterMovement->IsFalling())
		{

			if(!WallRunning)
				FindRunDirectionAndSide(Hit.ImpactNormal);

			if (AreRequiredKeysDown() && GetActorLocation().Z > WallHeight)
				BeginWallRun();
			else
			{
				if (WallRunning)
					EndWallRun();
			}
			
		}
	}
}

FVector AMovementMechanicsCharacter::FindLaunchVelocity()
{
	FVector launchDirection;

	// if wall running
	// jump away from the wall
	if (WallRunning)
	{
		FVector up;
		switch (WallSide)
		{
		case LEFT:
			up = FVector(0, 0, 1.0);
			break;
		case RIGHT:
			up = FVector(0, 0, -1.0);
			break;
		default:
			break;
		}
		// get a vector that points away from the wall by doing the cross product of the vector up
		// with the direction that we are running along
		launchDirection = UKismetMathLibrary::Cross_VectorVector(WallRunDirection, up);
	}
	else
	{
		if (PlayerCharacterMovement->IsFalling())
		{
			FVector rightVector = GetActorRightVector() * RightAxis;
			FVector forwardVector = GetActorForwardVector() * ForwardAxis;

			launchDirection = rightVector + forwardVector;
		}
	}

	//UE_LOG(LogTemp, Warning, TEXT("%f"), launchDirection.Z);
	// make sure launch direction has a z component
	launchDirection += FVector(0, 0, 1.0f);
	//UE_LOG(LogTemp, Warning, TEXT("%f"), launchDirection.Z);
	return launchDirection * PlayerCharacterMovement->JumpZVelocity;
}

bool AMovementMechanicsCharacter::CanSurfaceBeWallRan(const FVector ImpactNormal)
{
	// if the z component of the surface is very smal then it can't be wall ran
	if(ImpactNormal.Z < -0.05)
		return false;
	else
	{
		FVector normalXYplane = FVector(ImpactNormal.X, ImpactNormal.Y, 0.0f);
		normalXYplane.Normalize();
		const FVector normal = normalXYplane;
		
		// get slope of wall
		float slope = FVector::DotProduct(ImpactNormal, normalXYplane);
		float angleOfWall = UKismetMathLibrary::DegAcos(slope);

		if (angleOfWall < PlayerCharacterMovement->GetWalkableFloorAngle())
			return true;
		else
			return false;
	}
}

void AMovementMechanicsCharacter::FindRunDirectionAndSide(FVector wallNormal)
{
	const FVector2D rightVector = FVector2D(GetActorRightVector().X, GetActorRightVector().Y);
	const FVector2D wallNormal2D = FVector2D(wallNormal.X, wallNormal.Y);

	// if the dot product of the right vector of the actor and the wall normal is positive then
	// then the actor is to the right of the wall
	
	if (UKismetMathLibrary::DotProduct2D(rightVector, wallNormal2D) > 0)
		WallSide = RIGHT;
	else
		WallSide = LEFT;

	FVector up;
	if (WallSide == LEFT)
	{
		up = FVector(0, 0, -1.0f);
		WallRunDirection = UKismetMathLibrary::Cross_VectorVector(wallNormal, up);
	}
	else if(WallSide == RIGHT)
	{
		up = FVector(0, 0, 1.0f);
		WallRunDirection = UKismetMathLibrary::Cross_VectorVector(wallNormal, up);
	}
}

bool AMovementMechanicsCharacter::AreRequiredKeysDown()
{
	//UE_LOG(LogTemp, Warning, TEXT("%f"), ForwardAxis);

	if (ForwardAxis > 0.1)
		return true;
	else
		return false;
}

void AMovementMechanicsCharacter::BeginWallRun()
{

	NormalGravity = PlayerCharacterMovement->GravityScale;
	PlayerCharacterMovement->GravityScale = GravityScale;
	PlayerCharacterMovement->AirControl = 1.0f;
	//PlayerCharacterMovement->SetPlaneConstraintNormal(FVector(0, 0, 1));
	JumpCurrentCount = 0;
	WallRunning = true;
	PlayerCharacterMovement->MaxWalkSpeed = WalkingSpeed;
	CameraTilted = true;
}

void AMovementMechanicsCharacter::EndWallRun()
{
	PlayerCharacterMovement->GravityScale = 1.0f;
	PlayerCharacterMovement->AirControl = 0.05f;
	PlayerCharacterMovement->SetPlaneConstraintNormal(FVector(0, 0, 0));
	PlayerCharacterMovement->MaxWalkSpeed = 800;
	CameraTilted = false;
	WallRunning = false;
	
}

void AMovementMechanicsCharacter::UpdateWallRun(FHitResult Hit)
{
	if (!AreRequiredKeysDown())
	{
		EndWallRun();
		return;
	}
	WallSideENUM previousSide = WallSide;
	FindRunDirectionAndSide(Hit.ImpactNormal);
	// make sure side is the same
	// if it is different then end wall run
	if (previousSide != WallSide)
	{
		EndWallRun();
		return;
	}

	float maxSpeed = PlayerCharacterMovement->GetMaxSpeed();
	FVector playerVelocity = FVector(WallRunDirection.X * maxSpeed, WallRunDirection.Y * maxSpeed, PlayerCharacterMovement->Velocity.Z * GravityScale);
	UE_LOG(LogTemp, Warning, TEXT("Velocity in X %f"), playerVelocity.X);
	UE_LOG(LogTemp, Warning, TEXT("Velocity in Y %f"), playerVelocity.Y);
	UE_LOG(LogTemp, Warning, TEXT("Velocity in Z %f"), playerVelocity.Z);

	PlayerCharacterMovement->Velocity = playerVelocity;

}

bool AMovementMechanicsCharacter::ShootRayToWall(FHitResult& Hit)
{
	// get actor position
	FVector startRay = GetActorLocation();
	// get a vector from the actor to the wall
	FVector up;
	switch (WallSide)
	{
	case LEFT:
		up = FVector(0, 0, -1.0);
		break;
	case RIGHT:
		up = FVector(0, 0, 1.0);
		break;
	default:
		break;
	}
	FVector actorToWall = UKismetMathLibrary::Cross_VectorVector(WallRunDirection, up);
	actorToWall *= 200;
	// end position of ray
	FVector endRay = startRay + actorToWall;

	// You can use FCollisionQueryParams to further configure the query
	// Here we add ourselves to the ignored list so we won't block the trace
	FCollisionQueryParams TraceParams = FCollisionQueryParams(FName(TEXT("Trace")), true, this);


	ECollisionChannel Channel = ECC_WorldStatic;

	// shoot a ray from the position of the actor to where the wall should be
	return GetWorld()->LineTraceSingleByChannel(Hit, startRay, endRay, Channel, TraceParams);
	
}

void AMovementMechanicsCharacter::HandleCameraRotation()
{
	// rotate camera towards angle wanted
	if (CameraTilted)
	{
		float currentTilt = GetControlRotation().Roll;

		float increase = 1.0f;
		switch (WallSide)
		{
		case LEFT:
			if (GetControlRotation().Roll > 330 || GetControlRotation().Roll <= 30)
				AddControllerRollInput(-increase);
			break;
		case RIGHT:
			if (GetControlRotation().Roll < 30 || GetControlRotation().Roll >= 330)
				AddControllerRollInput(increase);
			break;
		default:
			break;
		}
	}
	// rotate camera back to 0 degree
	else
	{
		// end should be zero
		float increase = 1.0f;
		float currentTilt = GetControlRotation().Roll;
		if (currentTilt >= 330)
			AddControllerRollInput(increase);
		else if (currentTilt <= 30 && currentTilt > 0)
			AddControllerRollInput(-increase);

	}
}

void AMovementMechanicsCharacter::Tick(float DeltaSeconds)
{
	if(GrappleHookComponent)
		TimeSinceLastGrappleDetach = GrappleHookComponent->GetTimeSinceLastGrappleDetach();
	ClampHorizontalVelocity();
	ForwardAxis = InputComponent->GetAxisValue(FName("Move Forward / Backward"));
	RightAxis = InputComponent->GetAxisValue(FName("Move Right / Left"));

	if (GrappleHookComponent->IsGrappleAttached() && WallRunning)
		EndWallRun();

	// if falling and wall run has started then stick to the wall while WallRunning is true
	if (PlayerCharacterMovement->IsFalling())
	{
		if (WallRunning)
		{
			FHitResult hit;
			if (ShootRayToWall(hit))
			{
				UpdateWallRun(hit);
			}
			else
			{
				EndWallRun();
			}
		}
	}
	HandleCameraRotation();

}

void AMovementMechanicsCharacter::UseGrapple()
{
	if (GrappleHookComponent)
	{
		if (GrappleHookComponent->IsInUse())
		{
			GrappleHookComponent->DetachGrapple();
		}
		else
		{
			if(GrappleHookComponent->GetTimeSinceLastGrappleDetach() > GrappleCooldown)
				ShootGrappleRay();
			else
			{
				GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("Grapple is on cooldown"));
			}
		}
	}
}

void AMovementMechanicsCharacter::ShootGrappleRay()
{
	// shoot a ray from the camera position along the camera forward vector 

	FHitResult hit;
	FVector  start = FirstPersonCameraComponent->GetComponentLocation();
	FVector end = start + FirstPersonCameraComponent->GetForwardVector() * 10000;

	// You can use FCollisionQueryParams to further configure the query
	// Here we add ourselves to the ignored list so we won't block the trace
	FCollisionQueryParams TraceParams = FCollisionQueryParams(FName(TEXT("Trace")), true, this);
	//const FName TraceTag("MyTraceTag");

	//GetWorld()->DebugDrawTraceTag = TraceTag;

	//TraceParams.TraceTag = TraceTag;
	ECollisionChannel Channel = ECC_WorldStatic;

	if (GetWorld()->LineTraceSingleByChannel(hit, start, end, Channel, TraceParams))
	{
		GrappleHookComponent->FireGrapple(hit.Location, SetGrappleLocalOffset());
	}
	else
		GrappleHookComponent->FireGrapple(hit.TraceEnd, SetGrappleLocalOffset());

}

FVector AMovementMechanicsCharacter::SetGrappleLocalOffset()
{
	FVector localOffset;
	if (WallRunning)
	{
		switch (WallSide)
		{
			// if player is to left of the wall, spawn grapple to the left of the player
		case LEFT: localOffset = FVector(-30, -30, 40);
			break;
			// if player is to right of the wall, spawn grapple to the right of the player
		case RIGHT: localOffset = FVector(-30, 30, 40);
			break;
		default:
			localOffset = FVector(50, 0, 40);
			break;
		}
	}
	else
	{
		// when is not wall running just spawn it to the left
		localOffset = FVector(50, 0, 40);
	}

	return localOffset;
}

void AMovementMechanicsCharacter::ClampHorizontalVelocity()
{
	if (PlayerCharacterMovement->IsFalling())
	{
		FVector2D horizontalVelocity = FVector2D(PlayerCharacterMovement->Velocity.X, PlayerCharacterMovement->Velocity.Y);

		float speedRatio = horizontalVelocity.Length() / PlayerCharacterMovement->GetMaxSpeed();
		if (speedRatio > 1.0f)
		{
			horizontalVelocity = horizontalVelocity / speedRatio;
			PlayerCharacterMovement->Velocity.X = horizontalVelocity.X;
			PlayerCharacterMovement->Velocity.Y = horizontalVelocity.Y;
		}
	}
}


void AMovementMechanicsCharacter::MoveForward(float Value)
{
	//player must be mpving forward to stuck to the wall
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorForwardVector(), Value);
	}
}

void AMovementMechanicsCharacter::MoveRight(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorRightVector(), Value);
	}
}

void AMovementMechanicsCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * TurnRateGamepad * GetWorld()->GetDeltaSeconds());
}

void AMovementMechanicsCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * TurnRateGamepad * GetWorld()->GetDeltaSeconds());
}

bool AMovementMechanicsCharacter::EnableTouchscreenMovement(class UInputComponent* PlayerInputComponent)
{
	if (FPlatformMisc::SupportsTouchInput() || GetDefault<UInputSettings>()->bUseMouseForTouch)
	{
		PlayerInputComponent->BindTouch(EInputEvent::IE_Pressed, this, &AMovementMechanicsCharacter::BeginTouch);
		PlayerInputComponent->BindTouch(EInputEvent::IE_Released, this, &AMovementMechanicsCharacter::EndTouch);

		return true;
	}
	
	return false;
}


