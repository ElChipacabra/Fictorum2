// Copyright 2020-2021 Fly Dream Dev. All Rights Reserved.


#include "NpcDataComponent.h"

#include "NiagaraComponent.h"

#include "DirectorNPC.h"
#include "GameFramework/PawnMovementComponent.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"

#include "Engine.h"
#include "GameFramework/Pawn.h"

// Sets default values for this component's properties
UNpcDataComponent::UNpcDataComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}


// Called when the game starts
void UNpcDataComponent::BeginPlay()
{
	Super::BeginPlay();

	TArray<AActor*> allActorsArr_;
	UGameplayStatics::GetAllActorsOfClass(this, ADirectorNPC::StaticClass(), allActorsArr_);
	if (allActorsArr_.IsValidIndex(0))
	{
		if (allActorsArr_[0])
		{
			directorNPCRef = Cast<ADirectorNPC>(allActorsArr_[0]);
		}
	}

	hiddenTickComponentsInterval = FMath::RandRange(0.1f, 0.25f);

	ownerPawn = Cast<APawn>(GetOwner());

	TArray<UActorComponent*> allTickComponents_;
	GetOwner()->GetComponents(allTickComponents_);
	for (int compID_ = 0; compID_ < allTickComponents_.Num(); ++compID_)
	{
		defaultTickComponentsInterval.Add(allTickComponents_[compID_]->GetComponentTickInterval());
	}
	defaultTickActorInterval = ownerPawn->GetActorTickInterval();
	hiddenTickActorInterval = hiddenTickComponentsInterval;

	float rateOptimization_ = FMath::RandRange(0.1f, 0.2f);
	// Optimization timer.
	GetWorld()->GetTimerManager().SetTimer(npcOptimization_Timer, this, &UNpcDataComponent::OptimizationTimer, rateOptimization_, true, rateOptimization_);
	

	float randRate_ = FMath::RandRange(0.1f, 1.f);
	// Register NPC timer.
	GetWorld()->GetTimerManager().SetTimer(registerNpc_Timer, this, &UNpcDataComponent::RegisterNpcTimer, randRate_, true, randRate_);
}

void UNpcDataComponent::RegisterNpcTimer()
{
	if (!bIsRegistered && bIsActivate)
	{
		if (GetOwner())
		{
			if (ownerPawn != nullptr)
			{
				if (ownerPawn->GetMovementComponent())
				{
					npcData.bIsHasMoveComponent = true;

					if (!ownerPawn->GetMovementComponent()->IsFalling())
					{
						if (directorNPCRef->RegisterNPC(Cast<APawn>(GetOwner())))
						{
							bIsRegistered = true;
							GetWorld()->GetTimerManager().ClearTimer(registerNpc_Timer);
						}
					}
				}
				else
				{
					if (directorNPCRef->RegisterNPC(Cast<APawn>(GetOwner())))
					{
						bIsRegistered = true;
						GetWorld()->GetTimerManager().ClearTimer(registerNpc_Timer);
					}
				}
				
			}
				// Owner not pawn.
			else
			{
				GetWorld()->GetTimerManager().ClearTimer(registerNpc_Timer);
			}
		}
	}
}

void UNpcDataComponent::OptimizationTimer()
{
	if (!bIsActivate)
	{
		return;
	}
	
	TArray<UActorComponent*> allComponents = GetOwner()->GetComponentsByTag(UPrimitiveComponent::StaticClass(), componentsTag);

	bool bIsCameraSeeNPC_ = IsCameraSeeNPC();

	for (int i = 0; i < allComponents.Num(); ++i)
	{
		UPrimitiveComponent* primitiveComponent_ = Cast<UPrimitiveComponent>(allComponents[i]);
		if (primitiveComponent_)
		{
			primitiveComponent_->SetVisibility(bIsCameraSeeNPC_);
		}

		USkeletalMeshComponent* skeletalMesh_ = Cast<USkeletalMeshComponent>(allComponents[i]);
		if (skeletalMesh_)
		{
			if (bIsCameraSeeNPC_)
			{
				skeletalMesh_->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPose;
			}
			else
			{
				skeletalMesh_->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
			}
		}
	}

	// Set Tick Interval for all components in actor.
	TArray<UActorComponent*> allTickComponents_;
	GetOwner()->GetComponents(allTickComponents_);
	if (allTickComponents_.Num() > 0 && bIsOptimizeAllActorComponentsTickInterval)
	{
		if (bIsCameraSeeNPC_)
		{
			for (int compID_ = 0; compID_ < allTickComponents_.Num(); ++compID_)
			{
				if (bIsDisableTickIfBehindCameraFOV)
				{
					if (!Cast<UNiagaraComponent>(allTickComponents_[compID_]))
					{
						allTickComponents_[compID_]->SetComponentTickEnabled(true);
					}
					ownerPawn->SetActorTickEnabled(true);
				}
				else if (defaultTickComponentsInterval.IsValidIndex(compID_))
				{
					allTickComponents_[compID_]->SetComponentTickInterval(defaultTickComponentsInterval[compID_]);
					ownerPawn->SetActorTickInterval(defaultTickActorInterval);
				}
			}
		}
		else
		{
			for (int compID_ = 0; compID_ < allTickComponents_.Num(); ++compID_)
			{
				if (bIsDisableTickIfBehindCameraFOV)
				{
					if (!Cast<UNiagaraComponent>(allTickComponents_[compID_]))
					{
						allTickComponents_[compID_]->SetComponentTickEnabled(false);
					}
					ownerPawn->SetActorTickEnabled(false);
				}
				else
				{
					allTickComponents_[compID_]->SetComponentTickInterval(hiddenTickComponentsInterval);
					ownerPawn->SetActorTickInterval(hiddenTickActorInterval);
				}
			}
		}
	}

	if (bIsCameraSeeNPC_)
	{
		BroadcastInCameraFOV();
	}
	else
	{
		BroadcastBehindCameraFOV();
	}
}

bool UNpcDataComponent::IsCameraSeeNPC() const
{
	bool bIsSee_ = true;

	APlayerCameraManager* playerCam_ = UGameplayStatics::GetPlayerCameraManager(this, 0);
	if (playerCam_)
	{
		AActor* playerActor_ = Cast<AActor>(playerCam_->GetOwningPlayerController());
		if (playerActor_)
		{
			if (playerCam_->GetDotProductTo(GetOwner()) < 1.f - playerCam_->GetFOVAngle() / 100.f &&
				(playerCam_->GetCameraLocation() - GetOwner()->GetActorLocation()).Size() > distanceCamara)
			{
				bIsSee_ = false;
			}
			else
			{
				bIsSee_ = true;
			}
		}
	}

	return bIsSee_;
}


// Called every frame
void UNpcDataComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UNpcDataComponent::BroadcastOnPrepareForOptimization() const
{
	OnPrepareForOptimization.Broadcast();
}

void UNpcDataComponent::BroadcastOnRecoveryFromOptimization() const
{
	OnRecoveryFromOptimization.Broadcast();
}

void UNpcDataComponent::BroadcastBehindCameraFOV() const
{
	EventBehindCameraFOV.Broadcast();
}

void UNpcDataComponent::BroadcastInCameraFOV() const
{
	EventBehindCameraFOV.Broadcast();
}

void UNpcDataComponent::SetNpcUniqName(FString setName)
{
	npcUniqName = setName;
}

FString UNpcDataComponent::GetNpcUniqName()
{
	return npcUniqName;
}
