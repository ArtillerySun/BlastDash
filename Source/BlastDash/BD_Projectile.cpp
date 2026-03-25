// Fill out your copyright notice in the Description page of Project Settings.


#include "BD_Projectile.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/OverlapResult.h"
#include "Engine/DamageEvents.h"
#include "Perception/AISense_Sight.h"

// Sets default values
ABD_Projectile::ABD_Projectile()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	TimeElapsed = 0.0f;
	bHasExploded = false;
	Velocity = FVector::ZeroVector;
	ExplosionDelayTime = 3.0f;
	ExplosionRadius = 400.0f;
	BaseDamage = 50.0f;

	StimuliSource = CreateDefaultSubobject<UAIPerceptionStimuliSourceComponent>(TEXT("StimuliSource"));
	StimuliSource->bAutoRegister = true;
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

		// Block other bombs
		RootPrim->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Block);
	}

	if (StimuliSource)
	{
		StimuliSource->RegisterForSense(UAISense_Sight::StaticClass());
		StimuliSource->RegisterWithPerceptionSystem();
	}
}

void ABD_Projectile::SetHeld(bool bHeld)
{
	bIsHeld = bHeld;

	if (bIsHeld)
	{
		// Stop hovering immediately
		bIsHovering = false;

		// Reset velocity
		Velocity = FVector::ZeroVector;

		// Broadcast pickup event
		OnBombPickedUp.Broadcast();
	}
	else
	{
		if (StimuliSource)
		{
			StimuliSource->RegisterForSense(UAISense_Sight::StaticClass());
			StimuliSource->RegisterWithPerceptionSystem();
		}
	}
}


void ABD_Projectile::SetHoverState(bool bHover, FVector AnchorLoc)
{
	bIsHovering = bHover;
	HoverAnchorLocation = AnchorLoc;
	HoverSineTime = 0.0f;     // Reset
	Velocity = FVector::ZeroVector;
}

float ABD_Projectile::GetExplosionDelayTime()
{
	return ExplosionDelayTime;
}

void ABD_Projectile::SetExplosionDelayTime(float NewExplosionDelayTime)
{
	ExplosionDelayTime = NewExplosionDelayTime;
}

// Called every frame
void ABD_Projectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsActivated) {
		TimeElapsed += DeltaTime;
		if (TimeElapsed >= ExplosionDelayTime) {
			ExecuteExplosion();
		}
	}

	UPrimitiveComponent* RootPrim = Cast<UPrimitiveComponent>(GetRootComponent());
	if (RootPrim && RootPrim->IsSimulatingPhysics())
	{
		if (bIsHovering)
		{
			bIsHovering = false; // End Hovering
			OnBombPickedUp.Broadcast();
		}

		Velocity = RootPrim->GetComponentVelocity();
		return;
	}

	if (bIsHovering)
	{
		HoverSineTime += DeltaTime;

		// Use sin() to simulate hovering
		float ZOffset = FMath::Sin(HoverSineTime * 3.0f) * 10.0f;

		FVector NewLocation = HoverAnchorLocation + FVector(0, 0, ZOffset);
		SetActorLocation(NewLocation);

		return;
	}

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

	if (bHit) {
		HandleCollision(HitResult);
	}
	else {
		SetActorLocation(NextLocation);
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

	// Search several channels
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectParams.AddObjectTypesToQuery(ECC_PhysicsBody);
	ObjectParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjectParams.AddObjectTypesToQuery(ECC_Destructible);

	bool bHasHurtSomething = GetWorld()->OverlapMultiByObjectType(
		Overlaps,
		GetActorLocation(),
		FQuat::Identity,
		ObjectParams,
		Sphere,
		QueryParams
	);

	if (bHasHurtSomething) {
		for (auto& Result : Overlaps)
		{
			AActor* HitActor = Result.GetActor();
			if (!HitActor || HitActor == this) continue;

			// Breakable tag check
			if (HitActor->ActorHasTag(FName("Breakable")))
			{
				UGameplayStatics::ApplyDamage(
					HitActor,
					BaseDamage,
					GetInstigatorController(),
					this,
					UDamageType::StaticClass()
				);
			}

			// 1. Physics Knockback
			FVector Direction = HitActor->GetActorLocation() - GetActorLocation();
			float Distance = Direction.Size();
			Direction.Normalize();

			// Calculate Strength Decay
			float Strength = (1.0f - FMath::Clamp(Distance / ExplosionRadius, 0.f, 1.f)) * ExplosionForce;

			//if (ProjectileOwner && HitActor == ProjectileOwner)
			//{
			//	Strength *= 0.3f; // now the owner will only get 30% damage and 30% impulse
			//}

			// Make the Charactor Fly
			FVector FinalImpulse = Direction * Strength + FVector(0, 0, ExplosionUpwardBias);


			if (HitActor->GetClass()->ImplementsInterface(UBD_PhysicsInteractable::StaticClass()))
			{
				// If it is a custom bomb
				IBD_PhysicsInteractable::Execute_ApplyCustomImpulse(HitActor, FinalImpulse, false);
			}
			else if (ACharacter* Character = Cast<ACharacter>(HitActor))
			{
				// Apply Force to Charactors
				Character->LaunchCharacter(FinalImpulse, true, true);
			}
			else
			{
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

void ABD_Projectile::ApplyCustomImpulse_Implementation(FVector Impulse, bool bVelocityChange)
{
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

	UE_LOG(LogTemp, Warning, TEXT("Velocity AFTER = %s"), *Velocity.ToString());
}

void ABD_Projectile::ActivateBomb()
{
	bIsActivated = true;
	// Can add more relative logics
}

void ABD_Projectile::TriggerEarlyDetonation()
{
	if (bHasExploded) return;

	if (!bIsActivated)
	{
		ActivateBomb();
		ExplosionDelayTime = TimeElapsed + MinDetonationAge;
		return;
	}
}