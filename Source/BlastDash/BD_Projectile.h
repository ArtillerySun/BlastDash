// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BD_Projectile.generated.h"

UCLASS()
class BLASTDASH_API ABD_Projectile : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ABD_Projectile();
	void OnSelfDestroy();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	void HandleCollision(const FHitResult& Hit);

	void ExecuteExplosion();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BlastDash|Physics")
	float ExplosionRadius = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BlastDash|Physics")
	float ExplosionForce = 50000.f;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
	FVector Velocity; // Velocity

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
	float Mass = 1.0f; // Mass

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
	float DragForce = 0.1f; // Drag Force

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
	float DampingFactor = 0.98f; // Damping Factor

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics")
	FVector Gravity = FVector(0.f, 0.f, -980.f); // Gravity

};
