// Fill out your copyright notice in the Description page of Project Settings.


#include "Grapple.h"
#include "Kismet/KismetMathLibrary.h"
// Sets default values
AGrapple::AGrapple()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	if (!RootComponent)
	{
		RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("ProjectileSceneComponent"));
	}

	if (!CollisionComponent)
	{
		// Use a sphere as a simple collision representation.
		CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComponent"));
		// Set the sphere's collision radius.
		CollisionComponent->InitSphereRadius(0.25f);
		RootComponent = CollisionComponent;
		CollisionComponent->SetCollisionProfileName(TEXT("Projectile"));
	}

	// Add static mesh component to actor
	HookMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMeshComponent"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh>Mesh(TEXT("'/Game/GrappleHook/SM_GrappleHook.SM_GrappleHook'"));
	if (Mesh.Succeeded())
	{
		HookMeshComponent->SetStaticMesh(Mesh.Object);
		HookMeshComponent->SetupAttachment(RootComponent);

	}
	// Use a ProjectileMovementComponent to govern this projectile's movement
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileComp"));
	ProjectileMovement->SetUpdatedComponent(RootComponent);
	ProjectileMovement->InitialSpeed = 1.f;
	ProjectileMovement->MaxSpeed = 3000.f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->ProjectileGravityScale = 0.0f;
}

// Called when the game starts or when spawned
void AGrapple::BeginPlay()
{
	Super::BeginPlay();
	ProjectileMovement->Velocity = Velocity;
	StartLocation = GetActorLocation();;
	//CollisionComponent->OnComponentHit.AddDynamic(this, &AGrapple::OnCompHit);
}

// Called every frame
void AGrapple::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (UKismetMathLibrary::Vector_Distance(StartLocation, GetActorLocation()) >= MaxDistance)
	{
		Destroy();
	}
}

void AGrapple::SetVelocity(FVector vel)
{
	Velocity = vel;
	ProjectileMovement->Velocity = Velocity;
}

void AGrapple::SetMaxDistance(float dist)
{
	MaxDistance = dist;
}

USphereComponent* AGrapple::GetCollisionComponent()
{
	return CollisionComponent;
}


