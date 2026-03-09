// Fill out your copyright notice in the Description page of Project Settings.

#include "BD_Projectile.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/OverlapResult.h"
#include "Engine/DamageEvents.h"

// Sets default values
ABD_Projectile::ABD_Projectile()
{
	// Set this actor to call Tick() every frame. You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	TimeElapsed = 0.0f;
	bHasExploded = false;
	Velocity = FVector::ZeroVector;
	ExplosionDelayTime = 3.0f;
	ExplosionRadius = 400.0f;
	BaseDamage = 50.0f;
}

// Called when the game starts or when spawned
void ABD_Projectile::BeginPlay()
{
	Super::BeginPlay();

	if (UPrimitiveComponent* RootPrim = Cast<UPrimitiveComponent>(GetRootComponent()))
	{
		RootPrim->SetCollisionObjectType(ECC_PhysicsBody);
		RootPrim->SetSimulatePhysics(false);
		RootPrim->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}
}

// Called every frame
void ABD_Projectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	TimeElapsed += DeltaTime;
	if (TimeElapsed >= ExplosionDelayTime)
	{
		ExecuteExplosion();
		return;
	}

	UPrimitiveComponent* RootPrim = Cast<UPrimitiveComponent>(GetRootComponent());
	if (RootPrim && RootPrim->IsSimulatingPhysics())
	{
		Velocity = RootPrim->GetComponentVelocity();
		return;
	}

	FVector Acceleration = Gravity;

	// Update velocity
	Velocity += Acceleration * DeltaTime;

	// Apply drag force
	Velocity *= (1.0f - DragForce * DeltaTime);

	// Collision detection
	FVector CurrentLocation = GetActorLocation();
	FVector NextLocation = CurrentLocation + (Velocity * DeltaTime);

	FHitResult HitResult;
	FCollisionShape SphereShape = FCollisionShape::MakeSphere(15.f);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this); // Ignore itself

	if (AActor* PlayerChar = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0))
	{
		QueryParams.AddIgnoredActor(PlayerChar);
	}

	bool bHit = GetWorld()->SweepSingleByChannel(
		HitResult,
		CurrentLocation,
		NextLocation,
		FQuat::Identity,
		ECC_Camera,
		SphereShape,
		QueryParams
	);

	if (bHit)
	{
		HandleCollision(HitResult);
	}
	else
	{
		SetActorLocation(NextLocation);
	}
}

void ABD_Projectile::HandleCollision(const FHitResult& Hit)
{
	// V_new = V_old - 2 * (V_old dot Normal) * Normal
	FVector Normal = Hit.Normal;
	Velocity = Velocity - 2 * FVector::DotProduct(Velocity, Normal) * Normal;

	// Apply damping factor
	Velocity *= DampingFactor;

	// Prevent sticking in the wall
	SetActorLocation(Hit.Location + Normal * 2.0f);
}

void ABD_Projectile::ExecuteExplosion()
{
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

	if (bHasHurtSomething)
	{
		for (auto& Result : Overlaps)
		{
			AActor* HitActor = Result.GetActor();
			if (!HitActor || HitActor == this) continue;

			// Reliable direct damage for breakable props / large actors
			if (HitActor->ActorHasTag(TEXT("Breakable")))
			{
				UGameplayStatics::ApplyDamage(
					HitActor,
					BaseDamage,
					GetInstigatorController(),
					this,
					UDamageType::StaticClass()
				);
			}

			// Physics knockback
			FVector Direction = HitActor->GetActorLocation() - GetActorLocation();
			float Distance = Direction.Size();

			if (Distance > KINDA_SMALL_NUMBER)
			{
				Direction.Normalize();
			}
			else
			{
				Direction = FVector::UpVector;
				Distance = 0.f;
			}

			// Calculate strength decay
			float Strength = (1.0f - FMath::Clamp(Distance / ExplosionRadius, 0.f, 1.f)) * ExplosionForce;

			// Make the character fly
			FVector FinalImpulse = Direction * Strength + FVector(0, 0, ExplosionUpwardBias);

			// Apply force to characters
			ACharacter* Character = Cast<ACharacter>(HitActor);
			if (Character)
			{
				Character->LaunchCharacter(FinalImpulse, true, true);
			}
			else
			{
				// Only for objects with Simulate Physics enabled
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
	}

	// Radial damage for player / normal actors with falloff-style blast behavior
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
		ECC_Visibility
	);

	// Debug information for now
	DrawDebugSphere(GetWorld(), GetActorLocation(), ExplosionRadius, 32, FColor::Red, false, 2.0f, 0, 1.5f);
	DrawDebugSphere(GetWorld(), GetActorLocation(), ExplosionRadius / 2.0f, 32, FColor::Yellow, false, 2.0f, 0, 1.5f);

	OnSelfDestroy();
}

void ABD_Projectile::OnSelfDestroy()
{
	this->Destroy();
}

void ABD_Projectile::ApplyCustomImpulse_Implementation(FVector Impulse, bool bVelocityChange)
{
	// Release physics engine
	if (UPrimitiveComponent* RootPrim = Cast<UPrimitiveComponent>(GetRootComponent()))
	{
		RootPrim->SetSimulatePhysics(false);
	}

	if (bVelocityChange)
	{
		Velocity = Impulse;
	}
	else
	{
		Velocity = Impulse / FMath::Max(Mass, 0.1f);
	}
}