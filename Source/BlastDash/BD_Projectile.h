// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BD_Projectile.generated.h"

UCLASS()
class BLASTDASH_API ABD_Projectile : public AActor
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

	UPROPERTY(EditAnywhere, Category = "BlastDash|Logic")
	float ExplosionDelayTime = 3.0f;

	UPROPERTY(EditAnywhere, Category = "BlastDash|Logic")
	float BaseDamage = 50.0f;

	UPROPERTY(EditAnywhere, Category = "BlastDash|Logic")
	float ExplosionUpwardBias = 500.0f;

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


	UFUNCTION(BlueprintImplementableEvent, Category = "BlastDash|Events")
	void OnExplosionEffects();

	float TimeElapsed = 0.0f;
	bool bHasExploded = false;

public:	

	ABD_Projectile();
	void OnSelfDestroy();

	// Called every frame
	virtual void Tick(float DeltaTime) override;
};
