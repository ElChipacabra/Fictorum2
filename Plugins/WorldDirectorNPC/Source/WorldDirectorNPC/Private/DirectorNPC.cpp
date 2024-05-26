// Copyright 2020-2021 Fly Dream Dev. All Rights Reserved.


#include "DirectorNPC.h"

#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "Kismet/GameplayStatics.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "TimerManager.h"
#include "Runtime/Core/Public/HAL/RunnableThread.h"
#include "Engine.h"
#include "GameFramework/Pawn.h"


// Sets default values
ADirectorNPC::ADirectorNPC()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	StaticMeshInstanceComponent = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("StaticMeshInstanceComponent"));
	StaticMeshInstanceComponent->SetupAttachment(RootComponent);
	StaticMeshInstanceComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	StaticMeshInstanceComponent->SetGenerateOverlapEvents(false);
	StaticMeshInstanceComponent->SetCastShadow(false);
}

// Called when the game starts or when spawned
void ADirectorNPC::BeginPlay()
{
	Super::BeginPlay();

	// Run thread.
	directorThreadRef = nullptr;
	directorThreadRef = new DirectorThread(this, directorParameters);

	// Worker timers.
	GetWorldTimerManager().SetTimer(exchangeInformation_Timer, this, &ADirectorNPC::ExchangeInformationTimer, exchangeInformationRate, true, exchangeInformationRate);
}

void ADirectorNPC::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (directorThreadRef)
	{
		directorThreadRef->EnsureCompletion();
		delete directorThreadRef;
		directorThreadRef = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}

void ADirectorNPC::BeginDestroy()
{
	if (directorThreadRef)
	{
		directorThreadRef->EnsureCompletion();
		delete directorThreadRef;
		directorThreadRef = nullptr;
	}
	Super::BeginDestroy();
}

// Called every frame
void ADirectorNPC::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void DirectorThread::WanderingNPCinBackground(FNpcStruct& npcStruct) const
{
	FVector npcLoc_ = npcStruct.npcLocation;
	FVector targetLoc_ = npcStruct.npcTargetLocation;

	if (npcStruct.pathPoints.Num() == 0 && npcStruct.bIsNeedFindWaypoint)
	{
		float errorTolerance_ = 10.f;
		if (UKismetMathLibrary::EqualEqual_VectorVector(FVector(npcLoc_.X, npcLoc_.Y, 0.f), FVector(targetLoc_.X, targetLoc_.Y, 0.f), errorTolerance_))
		{
			FindRandomLocation(npcStruct);
		}
	}
}

void DirectorThread::MoveNPCinBackground(FNpcStruct& npcStruct, const float& inDeltaTime)
{
	FVector npcLoc_ = npcStruct.npcLocation;
	FVector targetLoc_ = npcStruct.npcTargetLocation;
	float errorTolerance_ = 10.f;

	if (npcStruct.pathPoints.Num() > 0)
	{
		targetLoc_ = npcStruct.pathPoints[0];
		targetLoc_.Z += 80.f;
		npcStruct.npcTargetLocation = targetLoc_;

		if (UKismetMathLibrary::EqualEqual_VectorVector(FVector(npcLoc_.X, npcLoc_.Y, 0.f), FVector(targetLoc_.X, targetLoc_.Y, 0.f), errorTolerance_))
		{
			npcStruct.pathPoints.RemoveAt(0);
		}
	}

	// Move Ghost NPC.
	npcStruct.npcLocation = FMath::VInterpConstantTo(npcStruct.npcLocation, targetLoc_, inDeltaTime, npcStruct.npcData.npcSpeed);

	if (UKismetMathLibrary::EqualEqual_VectorVector(FVector(npcLoc_.X, npcLoc_.Y, 0.f),
	                                                FVector(targetLoc_.X, targetLoc_.Y, 0.f), errorTolerance_) && npcStruct.pathPoints.Num() == 0)
	{
		npcStruct.bIsNeedFindWaypoint = true;
	}
}

void DirectorThread::FindRandomLocation(FNpcStruct& npcStruct) const
{
	bool bIsCanUseNavSystem_(false);

	ADirectorNPC* myDirector_ = Cast<ADirectorNPC>(ownerActor);
	if (myDirector_)
	{
		bIsCanUseNavSystem_ = !myDirector_->bIsFixUE4261EditorCrash;
	}

	if (npcStruct.npcData.bIsWander && bIsCanUseNavSystem_ && npcStruct.npcData.bIsHasMoveComponent)
	{
		if (!IsGarbageCollecting())
		{
			if (navigationSystemTH)
			{
				FNavLocation resultLocation_;
				if (navigationSystemTH->GetRandomPointInNavigableRadius(npcStruct.npcSpawnLocation, npcStruct.npcData.wanderRadius, resultLocation_))
				{
					if (resultLocation_.Location != FVector::ZeroVector)
					{
						UNavigationPath* resultPath_ = navigationSystemTH->FindPathToLocationSynchronously(ownerActor, npcStruct.npcLocation, resultLocation_);
						if (resultPath_)
						{
							if (resultPath_->IsValid())
							{
								npcStruct.pathPoints = resultPath_->PathPoints;
								npcStruct.bIsNeedFindWaypoint = false;
							}
						}
					}
				}
			}
		}
	}
}

void ADirectorNPC::InsertNPCinBackground(AActor* setNPC)
{
	if (setNPC != nullptr)
	{
		bool bPopulationControlSupport_(false);
		FNpcStruct newNPC_;
		newNPC_.npcLocation = setNPC->GetActorLocation();
		newNPC_.npcRotation = setNPC->GetActorRotation();
		newNPC_.classNpc_ = setNPC->GetClass();
		newNPC_.npcScale = setNPC->GetActorScale3D();

		UNpcDataComponent* actorComp_ = Cast<UNpcDataComponent>(setNPC->FindComponentByClass(UNpcDataComponent::StaticClass()));
		if (actorComp_)
		{
			newNPC_.npcData = actorComp_->npcData;
			newNPC_.npcSpawnLocation = actorComp_->npcSpawnLocation;

			actorComp_->BroadcastOnPrepareForOptimization();

			// Save uniq name npc.
			if (actorComp_->GetNpcUniqName() == "")
			{
				newNPC_.npcUniqName = setNPC->GetName();
			}
			else
			{
				newNPC_.npcUniqName = actorComp_->GetNpcUniqName();
			}

			bPopulationControlSupport_ = actorComp_->bPopulationControlSupport;
		}

		newNPC_.npcTargetLocation = newNPC_.npcLocation;

		// Add to background.
		allBackgroundNPCArr.Add(newNPC_);

		// Add to array for find npc struct in BP.
		allNPCArr_ForBP.Add(newNPC_);
		InsertNPCInBackground_BP(setNPC);

		if (!bPopulationControlSupport_)
		{
			// Destroy actor.
			setNPC->Destroy();
		}
	}
}

void ADirectorNPC::ExchangeInformationTimer()
{
	if (!bIsActivate)
	{
		return;
	}

	APlayerCameraManager* playerCam_ = UGameplayStatics::GetPlayerCameraManager(this, 0);
	if (playerCam_)
	{
		TArray<APawn*> allRegPawnsArrTemp_;
		FVector camLoc_ = playerCam_->GetCameraLocation();

		for (int i = 0; i < allRegisteredPawnsArr.Num(); ++i)
		{
			UNpcDataComponent* actorComp_ = Cast<UNpcDataComponent>(allRegisteredPawnsArr[i]->FindComponentByClass(UNpcDataComponent::StaticClass()));

			if (actorComp_)
			{
				// Check override layers in component.
				if (actorComp_->npcData.bIsOverrideLayers)
				{
					if ((allRegisteredPawnsArr[i]->GetActorLocation() - playerCam_->GetCameraLocation()).Size() > actorComp_->npcData.cameraRadius + actorComp_->npcData.layerOffset && IsValid(allRegisteredPawnsArr[i]))
					{
						InsertNPCinBackground(allRegisteredPawnsArr[i]);
						++npcInBackground;
					}
					else if (IsValid(allRegisteredPawnsArr[i]))
					{
						allRegPawnsArrTemp_.Add(allRegisteredPawnsArr[i]);
					}
				}
				else if ((allRegisteredPawnsArr[i]->GetActorLocation() - playerCam_->GetCameraLocation()).Size() > directorParameters.cameraRadius + directorParameters.layerOffset && IsValid(allRegisteredPawnsArr[i]))
				{
					InsertNPCinBackground(allRegisteredPawnsArr[i]);
					++npcInBackground;
				}
				else if (IsValid(allRegisteredPawnsArr[i]))
				{
					allRegPawnsArrTemp_.Add(allRegisteredPawnsArr[i]);
				}
			}
		}

		allRegisteredPawnsArr = allRegPawnsArrTemp_;

		// Get NPC Data.
		TArray<FNpcStruct> restoreNPC_ = directorThreadRef->GetDataNPC(allBackgroundNPCArr, camLoc_, allThreadNPCArr_Debug);
		npcInBackground -= restoreNPC_.Num();

		if (bIsDebug)
		{
			if (StaticMeshInstanceComponent->GetInstanceCount() != allThreadNPCArr_Debug.Num())
			{
				StaticMeshInstanceComponent->ClearInstances();
				for (int i = 0; i < allThreadNPCArr_Debug.Num(); ++i)
				{
					FTransform newInstanceTransform_;
					newInstanceTransform_.SetLocation(allThreadNPCArr_Debug[i].npcLocation);
					StaticMeshInstanceComponent->AddInstance(newInstanceTransform_, true);
				}
			}
			else
			{
				for (int i = 0; i < allThreadNPCArr_Debug.Num(); ++i)
				{
					FTransform newInstanceTransform_;
					newInstanceTransform_.SetLocation(allThreadNPCArr_Debug[i].npcLocation);

					StaticMeshInstanceComponent->UpdateInstanceTransform(i, newInstanceTransform_, true, false);
				}
			}
		}
		else
		{
			if (StaticMeshInstanceComponent->GetInstanceCount() > 0)
			{
				StaticMeshInstanceComponent->ClearInstances();
			}
		}

		StaticMeshInstanceComponent->MarkRenderStateDirty();

		allBackgroundNPCArr.Empty();

		for (int restoreID_ = 0; restoreID_ < restoreNPC_.Num(); ++restoreID_)
		{
			FActorSpawnParameters spawnInfo_;
			spawnInfo_.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			
			FRotator spawnRotation_(FRotator::ZeroRotator);

			// Set random rotation.
			if (restoreNPC_[restoreID_].npcData.bRandomPitchRotationOnSpawn || restoreNPC_[restoreID_].npcData.bRandomRollRotationOnSpawn || restoreNPC_[restoreID_].npcData.bRandomYawRotationOnSpawn)
			{
				if (restoreNPC_[restoreID_].npcData.bRandomYawRotationOnSpawn)
				{
					spawnRotation_.Yaw = FMath::RandRange(0.f, 360.f);
				}
				if (restoreNPC_[restoreID_].npcData.bRandomPitchRotationOnSpawn)
				{
					spawnRotation_.Pitch = FMath::RandRange(0.f, 360.f);
				}
				if (restoreNPC_[restoreID_].npcData.bRandomRollRotationOnSpawn)
				{
					spawnRotation_.Roll = FMath::RandRange(0.f, 360.f);
				}
			}
			else // Restore origin rotation.
			{
				spawnRotation_ = restoreNPC_[restoreID_].npcRotation;
			}

			// Save transform.
			FTransform spawnTransform_;
			// Correct spawn Z location.
			restoreNPC_[restoreID_].npcLocation.Z += restoreNPC_[restoreID_].npcData.correctSpawnAxisZ;
			spawnTransform_.SetLocation(restoreNPC_[restoreID_].npcLocation);
			spawnTransform_.SetRotation(spawnRotation_.Quaternion());
			spawnTransform_.SetScale3D(restoreNPC_[restoreID_].npcScale);

			AActor* restoredNPC_ = GetWorld()->SpawnActor<AActor>(restoreNPC_[restoreID_].classNpc_, spawnTransform_, spawnInfo_);
			if (restoredNPC_)
			{
				UNpcDataComponent* actorComp_ = Cast<UNpcDataComponent>(restoredNPC_->FindComponentByClass(UNpcDataComponent::StaticClass()));

				if (actorComp_)
				{
					actorComp_->npcData = restoreNPC_[restoreID_].npcData;
					actorComp_->npcSpawnLocation = restoreNPC_[restoreID_].npcSpawnLocation;
					// Restore uniq name of npc.
					actorComp_->SetNpcUniqName(restoreNPC_[restoreID_].npcUniqName);

					actorComp_->BroadcastOnRecoveryFromOptimization();

					for (int findID_ = 0; findID_ < allNPCArr_ForBP.Num(); ++findID_)
					{
						if (allNPCArr_ForBP[findID_].npcUniqName == restoreNPC_[restoreID_].npcUniqName)
						{
							RestoreNPC_BP(findID_, restoredNPC_);
							allNPCArr_ForBP.RemoveAt(findID_);
						}
					}
				}
			}
		}
	}
}

bool ADirectorNPC::RegisterNPC(APawn* npc)
{
	bool bIsReg_ = false;
	if (npc)
	{
		allRegisteredPawnsArr.Add(npc);
		bIsReg_ = true;
	}
	return bIsReg_;
}

int ADirectorNPC::GetBackgroundNpcCount() const
{
	return npcInBackground;
}

DirectorThread::DirectorThread(AActor* newActor, FDirectorStruct setDirectorParameters)
{
	m_Kill = false;
	m_Pause = false;

	//Initialize FEvent (as a cross platform (Confirmed Mac/Windows))
	m_semaphore = FGenericPlatformProcess::GetSynchEventFromPool(false);

	ownerActor = newActor;
	directorParametersTH = setDirectorParameters;

	UNavigationSystemV1* navSystem_ = UNavigationSystemV1::GetNavigationSystem(ownerActor);
	if (navSystem_)
	{
		navigationSystemTH = navSystem_;
	}

	Thread = FRunnableThread::Create(this, (TEXT("%s_FSomeRunnable"), *newActor->GetName()), 0, TPri_Lowest);
}

DirectorThread::~DirectorThread()
{
	if (m_semaphore)
	{
		//Cleanup the FEvent
		FGenericPlatformProcess::ReturnSynchEventToPool(m_semaphore);
		m_semaphore = nullptr;
	}

	if (Thread)
	{
		//Cleanup the worker thread
		delete Thread;
		Thread = nullptr;
	}
}

void DirectorThread::EnsureCompletion()
{
	Stop();

	if (Thread)
	{
		Thread->WaitForCompletion();
	}
}

void DirectorThread::PauseThread()
{
	m_Pause = true;
}

void DirectorThread::ContinueThread()
{
	m_Pause = false;

	if (m_semaphore)
	{
		//Here is a FEvent signal "Trigger()" -> it will wake up the thread.
		m_semaphore->Trigger();
	}
}

bool DirectorThread::Init()
{
	return true;
}

uint32 DirectorThread::Run()
{
	//Initial wait before starting
	FPlatformProcess::Sleep(0.5f);

	threadDeltaTime = 0.f;

	while (!m_Kill)
	{
		if (m_Pause)
		{
			//FEvent->Wait(); will "sleep" the thread until it will get a signal "Trigger()"
			m_semaphore->Wait();

			if (m_Kill)
			{
				return 0;
			}
		}
		else
		{
			uint64 const timePlatform = FPlatformTime::Cycles64();

			m_mutex.Lock();

			TArray<FNpcStruct> allBackgroundNPCArrTemp_ = allBackgroundNPCArrTH;

			allBackgroundNPCArrTH.Empty();

			TArray<FNpcStruct> stayInBackgroundNPCArrMem_;
			TArray<FNpcStruct> restoreNPCArr_;
			TArray<FNpcStruct> nearCanMovableNPC_;
			TArray<FNpcStruct> farCanMovableNPC_;

			m_mutex.Unlock();

			for (int restoreID_ = 0; restoreID_ < allBackgroundNPCArrTemp_.Num(); ++restoreID_)
			{
				float distance_ = (allBackgroundNPCArrTemp_[restoreID_].npcLocation - playerCameraLoc_).Size();

				// If override control layers in component.
				if (allBackgroundNPCArrTemp_[restoreID_].npcData.bIsOverrideLayers)
				{
					if (distance_ < allBackgroundNPCArrTemp_[restoreID_].npcData.cameraRadius)
					{
						restoreNPCArr_.Add(allBackgroundNPCArrTemp_[restoreID_]);
					}
					else
					{
						// Filter NPC who can movable.
						if (distance_ <= allBackgroundNPCArrTemp_[restoreID_].npcData.nearFastMovableRadiusNPC)
						{
							allBackgroundNPCArrTemp_[restoreID_].bIsCanMove = true;
							allBackgroundNPCArrTemp_[restoreID_].bIsNearNPC = true;
							nearCanMovableNPC_.Add(allBackgroundNPCArrTemp_[restoreID_]);
						}
						else if (distance_ > allBackgroundNPCArrTemp_[restoreID_].npcData.nearFastMovableRadiusNPC && distance_ <= allBackgroundNPCArrTemp_[restoreID_].npcData.farSlowMovableRadiusNPC)
						{
							allBackgroundNPCArrTemp_[restoreID_].bIsCanMove = true;
							allBackgroundNPCArrTemp_[restoreID_].bIsNearNPC = false;
							farCanMovableNPC_.Add(allBackgroundNPCArrTemp_[restoreID_]);
						}
						else
						{
							allBackgroundNPCArrTemp_[restoreID_].bIsCanMove = false;
							allBackgroundNPCArrTemp_[restoreID_].bIsNearNPC = false;
							stayInBackgroundNPCArrMem_.Add(allBackgroundNPCArrTemp_[restoreID_]);
						}
					}
				}
				else
				{
					if (distance_ < directorParametersTH.cameraRadius)
					{
						restoreNPCArr_.Add(allBackgroundNPCArrTemp_[restoreID_]);
					}
					else
					{
						// Filter NPC who can movable.
						if (distance_ <= directorParametersTH.nearFastMovableRadiusNPC)
						{
							allBackgroundNPCArrTemp_[restoreID_].bIsCanMove = true;
							allBackgroundNPCArrTemp_[restoreID_].bIsNearNPC = true;
							nearCanMovableNPC_.Add(allBackgroundNPCArrTemp_[restoreID_]);
						}
						else if (distance_ > directorParametersTH.nearFastMovableRadiusNPC && distance_ <= directorParametersTH.farSlowMovableRadiusNPC)
						{
							allBackgroundNPCArrTemp_[restoreID_].bIsCanMove = true;
							allBackgroundNPCArrTemp_[restoreID_].bIsNearNPC = false;
							farCanMovableNPC_.Add(allBackgroundNPCArrTemp_[restoreID_]);
						}
						else
						{
							allBackgroundNPCArrTemp_[restoreID_].bIsCanMove = false;
							allBackgroundNPCArrTemp_[restoreID_].bIsNearNPC = false;
							stayInBackgroundNPCArrMem_.Add(allBackgroundNPCArrTemp_[restoreID_]);
						}
					}
				}
			}

			if (nowFrame > skipFrameValue)
			{
				////////////////// NEAR LAYER /////////////////////
				if (minNearGetPathNpcID < nearCanMovableNPC_.Num() - 1)
				{
					if (maxNearGetPathNpcID <= nearCanMovableNPC_.Num() - 1)
					{
						for (int nearMoveID_ = minNearGetPathNpcID; nearMoveID_ < maxNearGetPathNpcID; ++nearMoveID_)
						{
							WanderingNPCinBackground(nearCanMovableNPC_[nearMoveID_]);
						}

						minNearGetPathNpcID = maxNearGetPathNpcID;
						maxNearGetPathNpcID += incrementNearGetPathNpc;
					}
					else
					{
						for (int moveID = minNearGetPathNpcID; moveID < nearCanMovableNPC_.Num(); ++moveID)
						{
							WanderingNPCinBackground(nearCanMovableNPC_[moveID]);
						}

						minNearGetPathNpcID = 0;
						maxNearGetPathNpcID = incrementNearGetPathNpc;
					}
				}
				else
				{
					minNearGetPathNpcID = 0;
					maxNearGetPathNpcID = incrementNearGetPathNpc;
				}

				////////////////// FAR LAYER /////////////////////
				if (minFarGetPathNpcID < farCanMovableNPC_.Num() - 1)
				{
					if (maxFarGetPathNpcID <= farCanMovableNPC_.Num() - 1)
					{
						for (int farMoveID_ = minFarGetPathNpcID; farMoveID_ < maxFarGetPathNpcID; ++farMoveID_)
						{
							WanderingNPCinBackground(farCanMovableNPC_[farMoveID_]);
						}

						minFarGetPathNpcID = maxFarGetPathNpcID;
						maxFarGetPathNpcID += incrementFarGetPathNpc;
					}
					else
					{
						for (int moveID = minFarGetPathNpcID; moveID < farCanMovableNPC_.Num(); ++moveID)
						{
							WanderingNPCinBackground(farCanMovableNPC_[moveID]);
						}

						minFarGetPathNpcID = 0;
						maxFarGetPathNpcID = incrementFarGetPathNpc;
					}
				}
				else
				{
					minFarGetPathNpcID = 0;
					maxFarGetPathNpcID = incrementFarGetPathNpc;
				}

				// Reset skip frame.
				nowFrame = 0;
			}
			else
			{
				++nowFrame;
			}

			// Append all movable array for return data npc.
			stayInBackgroundNPCArrMem_.Append(nearCanMovableNPC_);
			stayInBackgroundNPCArrMem_.Append(farCanMovableNPC_);

			for (int moveID = 0; moveID < stayInBackgroundNPCArrMem_.Num(); ++moveID)
			{
				if (stayInBackgroundNPCArrMem_[moveID].npcData.bIsHasMoveComponent)
				{
					MoveNPCinBackground(stayInBackgroundNPCArrMem_[moveID], threadDeltaTime);
				}
			}

			m_mutex.Lock();

			// Save data.
			allBackgroundNPCArrTH.Append(stayInBackgroundNPCArrMem_);

			canRestoreNPCArrTH.Append(restoreNPCArr_);

			m_mutex.Unlock();

			threadDeltaTime = FTimespan(FPlatformTime::Cycles64() - timePlatform).GetTotalSeconds();

			threadDeltaTime += threadSleepTime;

			//A little sleep between the chunks (So CPU will rest a bit -- (may be omitted))
			FPlatformProcess::Sleep(threadSleepTime);
		}
	}
	return 0;
}

void DirectorThread::Stop()
{
	m_Kill = true; //Thread kill condition "while (!m_Kill){...}"
	m_Pause = false;

	if (m_semaphore)
	{
		//We shall signal "Trigger" the FEvent (in case the Thread is sleeping it shall wake up!!)
		m_semaphore->Trigger();
	}
}

bool DirectorThread::IsThreadPaused() const
{
	return (bool)m_Pause;
}

TArray<FNpcStruct> DirectorThread::GetDataNPC(TArray<FNpcStruct> setNewBackgroundNPCArr, FVector setPlayerCameraLoc, TArray<FNpcStruct>& getAllThreadNPCArr)
{
	m_mutex.Lock();

	allBackgroundNPCArrTH.Append(setNewBackgroundNPCArr);

	getAllThreadNPCArr = allBackgroundNPCArrTH;

	playerCameraLoc_ = setPlayerCameraLoc;

	TArray<FNpcStruct> canRestoreNPCTemp_ = canRestoreNPCArrTH;
	canRestoreNPCArrTH.Empty();

	m_mutex.Unlock();

	return canRestoreNPCTemp_;
}
