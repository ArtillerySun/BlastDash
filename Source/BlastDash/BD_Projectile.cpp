// Fill out your copyright notice in the Description page of Project Settings.


#include "BD_Projectile.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/OverlapResult.h"
#include "Engine/DamageEvents.h"

// Sets default values
ABD_Projectile::ABD_Projectile()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	TimeElapsed = 0.0f;
	bHasExploded = false;
	Velocity = FVector::ZeroVector;
	ExplosionDelayTime = 3.0f;
	ExplosionRadius = 200.0f;
	BaseDamage = 50.0f;
}

// Called when the game starts or when spawned
void ABD_Projectile::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ABD_Projectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	FVector Acceleration = Gravity;

	// Update Velocity
	Velocity += Acceleration * DeltaTime;

	// Apply Drag force
	Velocity *= (1.0f - DragForce * DeltaTime);

	// Collision Detection
	FVector CurrentLocation = GetActorLocation();
	FVector NextLocation = CurrentLocation + (Velocity * DeltaTime);

	FHitResult HitResult;
	FCollisionShape SphereShape = FCollisionShape::MakeSphere(15.f);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this); // Ignore itself

	bool bHit = GetWorld()->SweepSingleByChannel(
		HitResult,
		CurrentLocation,
		NextLocation,
		FQuat::Identity,
		ECC_Camera,
		SphereShape,
		QueryParams
	);

	if (bHit) {
		HandleCollision(HitResult);
	}
	else {
		SetActorLocation(NextLocation);
	}

	TimeElapsed += DeltaTime;
	if (TimeElapsed >= ExplosionDelayTime) {
		ExecuteExplosion();
	}
}

void ABD_Projectile::HandleCollision(const FHitResult& Hit) {
	// V_new = V_old - 2 * (V_old \dot Normal) * Normal
	FVector Normal = Hit.Normal;
	Velocity = Velocity - 2 * FVector::DotProduct(Velocity, Normal) * Normal;

	// Apply Damping Factor
	Velocity *= DampingFactor;

	// Prevent Stucking in the Wall
	SetActorLocation(Hit.Location + Normal * 2.0f);
}

void ABD_Projectile::ExecuteExplosion() {

	SetActorTickEnabled(false);

	if (bHasExploded) return;
	bHasExploded = true;

	UWorld* World = GetWorld();
	if (!World) return;

	OnExplosionEffects();

	TArray<AActor*> IgnoreActors;
	IgnoreActors.Add(this);

	TArray<FOverlapResult> Overlaps;
	FCollisionShape Sphere = FCollisionShape::MakeSphere(ExplosionRadius);
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActors(IgnoreActors);

	bool bHasHurtSomething = GetWorld()->OverlapMultiByChannel(
		Overlaps,
		GetActorLocation(),
		FQuat::Identity,
		ECC_WorldDynamic,
		Sphere,
		QueryParams
	);

	if (bHasHurtSomething) {
		for (auto& Result : Overlaps)
		{
			AActor* HitActor = Result.GetActor();
			if (!HitActor) continue;

			// 1. Physics Knockback
			FVector Direction = HitActor->GetActorLocation() - GetActorLocation();
			float Distance = Direction.Size();
			Direction.Normalize();

			// Calculate Strength Decay
			float Strength = (1.0f - FMath::Clamp(Distance / ExplosionRadius, 0.f, 1.f)) * ExplosionForce;

			// Make the Charactor Fly
			FVector FinalImpulse = Direction * Strength + FVector(0, 0, ExplosionUpwardBias);

			// Apply Force to Charactors
			ACharacter* Character = Cast<ACharacter>(HitActor);
			if (Character)
			{
				Character->LaunchCharacter(FinalImpulse, true, true);
			}
			else  {
				// Only for Objects with Simulate Physics Enabled
				USceneComponent* Root = HitActor->GetRootComponent();
				if (Root)
				{
					UPrimitiveComponent* PhysComp = Cast<UPrimitiveComponent>(Root);
					if (PhysComp && PhysComp->IsSimulatingPhysics())
					{
						PhysComp->AddImpulse(FinalImpulse, NAME_None, true);
					}
				}
			}
		}

		// 2. Deal with Damage
		FVector DamageOrigin = GetActorLocation() + FVector(0.f, 0.f, 20.f);
		UGameplayStatics::ApplyRadialDamage(
			this,
			BaseDamage,
			DamageOrigin,
			ExplosionRadius,
			UDamageType::StaticClass(),
			IgnoreActors,
			this,
			GetInstigatorController(),
			false,
			ECC_WorldDynamic
		);
	}
	
	// Debug Information for Now
	DrawDebugSphere(GetWorld(), GetActorLocation(), ExplosionRadius, 32, FColor::Red, false, 2.0f, 0, 1.5f);
	DrawDebugSphere(GetWorld(), GetActorLocation(), ExplosionRadius / 2.0, 32, FColor::Yellow, false, 2.0f, 0, 1.5f);

	OnSelfDestroy();
}

void ABD_Projectile::OnSelfDestroy() {
	this->Destroy();
}