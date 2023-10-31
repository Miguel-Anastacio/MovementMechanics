// Fill out your copyright notice in the Description page of Project Settings.
#include "GrapplingHookComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "CableComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Character.h"

// Sets default values for this component's properties
UGrapplingHookComponent::UGrapplingHookComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	// ...
}


// Called when the game starts
void UGrapplingHookComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UGrapplingHookComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	TimeSinceLastGrappleDetach += DeltaTime;

	// if grapple is attached then apply force to the player
	if (GrappleState == ATTACHED)
	{
		ACharacter* playerCharacter = Cast<ACharacter>(GetOwner());
		UCharacterMovementComponent* playerMovement = playerCharacter->GetCharacterMovement();

		FVector grapplePull = ToGrappleHook() * PerTickPulForce;
		playerMovement->AddForce(grapplePull);

		// test if player is close enought to grapple then detach
		if (UKismetMathLibrary::Vector_Distance(GrappleHook->GetActorLocation(), GetOwner()->GetActorLocation()) < DisconnectDistance)
			DetachGrapple();
		else
		{
			// test if player swung past the grapple position
			// using the dot product between the initial direction to the grapple and  the current direction ( ignoring the z component)
			// below 0 then the player went past the grapple
			if (UKismetMathLibrary::Dot_VectorVector(InitialHookDirection2D, ToGrappleHook2D()) < 0)
				DetachGrapple();		
		}
	}
}

bool UGrapplingHookComponent::IsInUse()
{
	switch (GrappleState)
	{
	case READY: return false;
		break;
	case FIRING: return true;
		break;
	case ATTACHED: return true;
		break;
	default:
		//UE_LOG(LogTemp, Warning, TEXT("ERROR GRAPPLE STATE");
		return false;
		break;
	}


}

bool UGrapplingHookComponent::IsGrappleAttached()
{
	switch (GrappleState)
	{
	case READY: return false;
		break;
	case FIRING: return false;
		break;
	case ATTACHED: return true;
		break;
	default:
		return false;
		break;
	}
}

void UGrapplingHookComponent::FireGrapple(FVector targetLocation, FVector localOffset)
{
	if (IsInUse())
		return;

	GrappleState = FIRING;

	FVector fireDirection = targetLocation - CableStartLocation(localOffset);
	fireDirection.Normalize();
	FVector grappleVelocity = fireDirection * GrappleSpeed;
	FTransform SpawnTransform = GetOwner()->GetActorTransform();

	UWorld* MyLevel = GetWorld();
	if (MyLevel)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = GetOwner();
		SpawnTransform.SetRotation(FQuat4d(0, 0, 0, 1.0f));
		SpawnTransform.SetScale3D(FVector(1.0f, 1.0f, 1.0f));
		SpawnTransform.SetLocation(CableStartLocation(localOffset));
		// spawn grappple hook
		GrappleHook = MyLevel->SpawnActor<AGrapple>(HookClass, SpawnTransform, SpawnParams);

		if (!GrappleHook)
			GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("Error Spawning Grapple Hook"));
		else
		{
			// set initial velocity 
			GrappleHook->SetVelocity(grappleVelocity);
			// bind hit event
			GrappleHook->GetCollisionComponent()->OnComponentHit.AddDynamic(this, &UGrapplingHookComponent::OnGrappleHit);
			// bind destroy event
			GrappleHook->OnDestroyed.AddDynamic(this, &UGrapplingHookComponent::OnGrappleDestroyed);
		}
			
		if (CableClass)
		{
			SpawnTransform.SetScale3D(FVector(1.0f, 1.0f, 1.0f));
			SpawnTransform.Rotator() = UKismetMathLibrary::MakeRotFromX(fireDirection);
			SpawnTransform.SetLocation(CableStartLocation(localOffset));
			GrappleCable = MyLevel->SpawnActor<AGrappleCable>(CableClass, SpawnTransform);
		}

	}

	if(!GrappleHook)
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("Error Spawning Grapple Hook"));
	
	// attach cable to the player
	if (GrappleCable)
		GrappleCable->AttachToActor(GetOwner(), FAttachmentTransformRules::KeepWorldTransform);
	else
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("Error attaching cable to player"));
	// attach cable to hook
	if (GrappleHook && GrappleCable)
		GrappleCable->CableComponent->SetAttachEndTo(GrappleHook, FName());
	else
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("Error attaching cable to hook"));

	// by default end location of cable component is (100, 0, 0), set it to 0
	if (GrappleHook && GrappleCable)
		GrappleCable->CableComponent->EndLocation = FVector(0, 0, 0);
}

void UGrapplingHookComponent::DetachGrapple()
{
	if (GrappleHook)
	{
		GrappleHook->Destroy();
	}

	TimeSinceLastGrappleDetach = 0.0f;
}

FVector UGrapplingHookComponent::CableStartLocation(FVector localOffSet)
{
	FVector playerLocation = GetOwner()->GetActorLocation();
	// transform the local offset to local space
	FVector offsetInLocalSpace = UKismetMathLibrary::TransformDirection(GetOwner()->GetActorTransform(), localOffSet);

	return playerLocation + offsetInLocalSpace;
}

FVector UGrapplingHookComponent::ToGrappleHook()
{
	FVector direction = FVector(0, 0, 0);
	if (GrappleHook)
	{
		//UE_LOG(LogTemp, Warning, TEXT("Hook Location %f"), direction.X);
		direction = GrappleHook->GetActorLocation() - GetOwner()->GetActorLocation();
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("Error Grapple hook actor not created"));
	}
	direction.Normalize();
	return direction;
}

FVector UGrapplingHookComponent::ToGrappleHook2D()
{
	FVector directionXYplane = ToGrappleHook();
	directionXYplane.Z = 0;
	directionXYplane.Normalize();
	return directionXYplane;
}

void UGrapplingHookComponent::OnGrappleHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	GrappleState = ATTACHED;
	ACharacter* playerCharacter = Cast<ACharacter>(GetOwner());

	// set grappling movement characteristics
	UCharacterMovementComponent* playerMovement = playerCharacter->GetCharacterMovement();

	playerMovement->GroundFriction = 0.0f;
	playerMovement->GravityScale = 0.0f;
	playerMovement->AirControl = 0.2f;

	FVector dir = ToGrappleHook();


	FVector initialPlayerVelocity = dir * PullInitialSpeed;

	playerMovement->Velocity = initialPlayerVelocity;
	InitialHookDirection2D = ToGrappleHook2D();
}

void UGrapplingHookComponent::OnGrappleDestroyed(AActor* Act)
{
	GrappleState = READY;

	// add destroy the cable
	if (GrappleCable)
		GrappleCable->Destroy();

	ACharacter* playerCharacter = Cast<ACharacter>(GetOwner());

	// set grappling movement characteristics
	UCharacterMovementComponent* playerMovement = playerCharacter->GetCharacterMovement();

	playerMovement->GroundFriction = 1.0f;
	playerMovement->GravityScale = 1.0f;
	playerMovement->AirControl = 0.05f;
}



