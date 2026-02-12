// Fill out your copyright notice in the Description page of Project Settings.


#include "BD_Projectile.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/OverlapResult.h"

// Sets default values
ABD_Projectile::ABD_Projectile()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

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

	FVector Acceleration = Gravity; // For simpliarity now

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

}

void ABD_Projectile::HandleCollision(const FHitResult& Hit) {
	// V_new = V_old - 2 * (V_old ¡¤ Normal) * Normal
	FVector Normal = Hit.Normal;
	Velocity = Velocity - 2 * FVector::DotProduct(Velocity, Normal) * Normal;

	// Apply Damping Factor
	Velocity *= DampingFactor;

	/*GEngine->AddOnScreenDebugMessage(-1, 0.01f, FColor::Yellow,
		FString::Printf(TEXT("Current Speed: %f"), Velocity.Size()));*/
	if (Velocity.Size() > 300.0f) { // Currently for testing
		ExecuteExplosion();
	}

	SetActorLocation(Hit.Location + Normal * 2.0f);
}

void ABD_Projectile::ExecuteExplosion() {
	TArray<FOverlapResult> Overlaps;
	FCollisionShape Sphere = FCollisionShape::MakeSphere(ExplosionRadius);
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	bool bHasHurtSomething = GetWorld()->OverlapMultiByChannel(
		Overlaps,
		GetActorLocation(),
		FQuat::Identity,
		ECC_WorldDynamic,
		Sphere,
		QueryParams
	);

	DrawDebugSphere(GetWorld(), GetActorLocation(), ExplosionRadius, 32, FColor::Red, false, 2.0f, 0, 1.5f);

	if (bHasHurtSomething) {
		for (auto& Result : Overlaps)
		{
			AActor* HitActor = Result.GetActor();
			if (!HitActor) continue;

			FVector Direction = HitActor->GetActorLocation() - GetActorLocation();
			float Distance = Direction.Size();
			Direction.Normalize();

			// Calculate Strength Decay
			float Strength = (1.0f - FMath::Clamp(Distance / ExplosionRadius, 0.f, 1.f)) * ExplosionForce;

			// Make the Charactor Fly
			FVector FinalImpulse = Direction * Strength + FVector(0, 0, 500.f);

			// Apply Force to Charactors
			ACharacter* Character = Cast<ACharacter>(HitActor);
			if (Character)
			{
				Character->LaunchCharacter(FinalImpulse, true, true);
			}
			else if (UPrimitiveComponent* PhysComp = Cast<UPrimitiveComponent>(HitActor->GetRootComponent())) {
				// Only for Objects with Simulate Physics Enabled
				if (PhysComp->IsSimulatingPhysics())
				{
					// Instant Impulse
					PhysComp->AddImpulse(FinalImpulse, NAME_None, true);
				}
			}
		}
		OnSelfDestroy();
	}
}

void ABD_Projectile::OnSelfDestroy() {
	this->Destroy();
}