// Copyright 2020-2021 Fly Dream Dev. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/UserDefinedStruct.h"

#include "NpcDataComponent.generated.h"

USTRUCT(BlueprintType)
struct FNpcData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Director NPC Parameters")
	bool bIsWander = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Director NPC Parameters")
	bool bIsRestoreOriginSpawnLocation = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Director NPC Parameters")
	float correctSpawnAxisZ = 80.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Director NPC Parameters")
	float wanderRadius = 3000.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Director NPC Parameters")
	float npcSpeed = 600.f;
	UPROPERTY(BlueprintReadOnly, Category = "Director NPC Struct", meta=(ClampMin="0.1"))
	float delayTimeFindLocation = 3.f;
	UPROPERTY(BlueprintReadWrite, Category = "Director NPC Parameters")
	FVector targetLocation = FVector::ZeroVector;

	bool bIsHasMoveComponent = false;

	// Override global layers in Director.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Director PRO Parameters")
	bool bIsOverrideLayers = false;
	// Override global layers in Director.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Director PRO Parameters")
	float cameraRadius = 3000.f;
	// Override global layers in Director.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Director PRO Parameters")
	float nearFastMovableRadiusNPC = 15000.f;
	// Override global layers in Director.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Director PRO Parameters")
	float farSlowMovableRadiusNPC = 30000.f;
	// Override global layers in Director.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Director PRO Parameters")
	float layerOffset = 500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Director PRO Parameters")
	bool bRandomYawRotationOnSpawn = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Director PRO Parameters")
	bool bRandomRollRotationOnSpawn = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Director PRO Parameters")
	bool bRandomPitchRotationOnSpawn = false;

	FNpcData()
	{
	};
};

UCLASS(Blueprintable)
class WORLDDIRECTORNPC_API UNpcDataComponent : public UActorComponent
{
	GENERATED_BODY()

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPrepareForOptimization);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FRecoveryFromOptimization);

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBehindCameraFOV);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInCameraFOV);

public:	
	// Sets default values for this component's properties
	UNpcDataComponent();

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	// Broadcasts notification
	void BroadcastOnPrepareForOptimization() const;
	// Broadcasts notification
	void BroadcastOnRecoveryFromOptimization() const;

	// Broadcasts notification
	void BroadcastBehindCameraFOV() const;
	// Broadcasts notification
	void BroadcastInCameraFOV() const;

	UFUNCTION(BlueprintImplementableEvent, Category = "Director NPC Parameters")
	void PrepareForOptimizationBP();
	UFUNCTION(BlueprintImplementableEvent, Category = "Director NPC Parameters")
	void RecoveryFromOptimization();

	void SetNpcUniqName(FString setName);
	FString GetNpcUniqName();

	// Delegate
	UPROPERTY(BlueprintAssignable)
	FPrepareForOptimization OnPrepareForOptimization;
	UPROPERTY(BlueprintAssignable)
	FRecoveryFromOptimization OnRecoveryFromOptimization;

	// Delegate
	UPROPERTY(BlueprintAssignable)
	FPrepareForOptimization EventBehindCameraFOV;
	UPROPERTY(BlueprintAssignable)
	FRecoveryFromOptimization EventInCameraFOV;

	// Set Enable Director Actor Component.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Director Actor Parameters")
	bool bIsActivate = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Director Actor Parameters")
	bool bIsOptimizeAllActorComponentsTickInterval = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Director Actor Parameters")
	bool bIsDisableTickIfBehindCameraFOV = false;
	
	// Enable this if used Population Control System.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Director NPC Parameters")
    bool bPopulationControlSupport = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Director NPC Parameters")
	FNpcData npcData;

	UPROPERTY(BlueprintReadWrite, Category = "Director NPC Parameters")
	FVector npcSpawnLocation;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	void RegisterNpcTimer();

	void OptimizationTimer();

	bool IsCameraSeeNPC() const;
	

private:

	TArray<float> defaultTickComponentsInterval;
	float hiddenTickComponentsInterval;
	float defaultTickActorInterval;
	float hiddenTickActorInterval;

	float distanceCamara = 1000.f;

	FName componentsTag = "DNPC";

	UPROPERTY()
	class ADirectorNPC *directorNPCRef;

	FTimerHandle registerNpc_Timer;

	bool bIsRegistered = false;

	FTimerHandle npcOptimization_Timer;

	UPROPERTY()
	class UPawnMovementComponent* movementComponent = nullptr;

	UPROPERTY()
	APawn* ownerPawn = nullptr;

	FString npcUniqName = "";
		
};
