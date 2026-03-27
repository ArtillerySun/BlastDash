// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BD_PhysicsInteractable.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "BD_Projectile.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBombPickedUpSignature);

UCLASS()
class BLASTDASH_API ABD_Projectile : public AActor, public IBD_PhysicsInteractable
{
	GENERATED_BODY()
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	void HandleCollision(const FHitResult& Hit);

	void ExecuteExplosion();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BlastDash|Physics")
	float ExplosionRadius = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BlastDash|Physics")
	float ExplosionForce = 50000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BlastDash|Logic")
	float ExplosionDelayTime = 3.0f;

	UPROPERTY(EditAnywhere, Category = "BlastDash|Logic")
	float BaseDamage = 50.0f;

	UPROPERTY(EditAnywhere, Category = "BlastDash|Logic")
	float ExplosionUpwardBias = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BlastDash|Logic")
	bool bIsActivated = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BlastDash|Physics")
	FVector Velocity; // Velocity

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BlastDash|Physics")
	float Mass = 1.0f; // Mass

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BlastDash|Physics")
	float DragForce = 0.1f; // Drag Force

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BlastDash|Physics")
	float DampingFactor = 0.98f; // Damping Factor

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BlastDash|Physics")
	FVector Gravity = FVector(0.f, 0.f, -980.f); // Gravity

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BlastDash|State")
	bool bIsHeld = false;

	UFUNCTION(BlueprintImplementableEvent, Category = "BlastDash|Events")
	void OnExplosion();

	UFUNCTION(BlueprintImplementableEvent, Category = "BlastDash|Events")
	void OnExplosionEffects();

	UPROPERTY(BlueprintAssignable, Category = "BlastDash|Events")
	FOnBombPickedUpSignature OnBombPickedUp;

	// Hovering
	bool bIsHovering = false;
	FVector HoverAnchorLocation;
	float HoverSineTime = 0.0f;

	float TimeElapsed = 0.0f;
	bool bHasExploded = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	UAIPerceptionStimuliSourceComponent* StimuliSource;

public:	

	ABD_Projectile();
	void OnSelfDestroy();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual void ApplyCustomImpulse_Implementation(FVector Impulse, bool bVelocityChange) override;

	UFUNCTION(BlueprintCallable, Category = "BlastDash|Logic")
	void ActivateBomb();

	// Hovering
	UFUNCTION(BlueprintCallable, Category = "BlastDash|Logic")
	void SetHoverState(bool bHover, FVector AnchorLoc = FVector::ZeroVector);

	UFUNCTION(BlueprintCallable, Category = "BlastDash|Logic")
	float GetExplosionDelayTime();

	UFUNCTION(BlueprintCallable, Category = "BlastDash|Logic")
	void SetExplosionDelayTime(float NewExplosionDelayTime = 3.0f);

	UFUNCTION(BlueprintCallable, Category = "BlastDash|State")
	void SetHeld(bool bHeld);

	UFUNCTION(BlueprintPure, Category = "BlastDash|State")
	bool IsHeld() const { return bIsHeld; }
	// Premature detonation
	UFUNCTION(BlueprintCallable, Category = "BlastDash|Logic")
	void TriggerEarlyDetonation();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BlastDash|Logic")
	float MinDetonationAge = 0.5f; // In second

	// Track the last player who picked up the bomb
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "BlastDash|Logic")
	AActor* ProjectileOwner;

	UFUNCTION(BlueprintCallable, Category = "BlastDash|Logic")
	void SetProjectileOwner(AActor* NewOwner) { ProjectileOwner = NewOwner; }
};
