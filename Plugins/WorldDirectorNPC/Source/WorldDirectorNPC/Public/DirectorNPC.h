// Copyright 2020-2021 Fly Dream Dev. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "NpcDataComponent.h"
#include "GameFramework/Actor.h"
#include "Runtime/Core/Public/HAL/Runnable.h"
#include "Runtime/Core/Public/HAL/ThreadSafeBool.h"

#include "DirectorNPC.generated.h"


USTRUCT(BlueprintType)
struct FNpcStructInBP
{
	GENERATED_USTRUCT_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Director NPC Parameters")
	int indexNpc = 0;

	FNpcStructInBP()
	{
	};
};

USTRUCT(BlueprintType)
struct FNpcStruct
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Director NPC Struct")
	FVector npcLocation = FVector::ZeroVector;
	UPROPERTY()
	FRotator npcRotation = FRotator::ZeroRotator;
	UPROPERTY()
	FVector npcScale = FVector::ZeroVector;

	UPROPERTY()
	FVector npcSpawnLocation = FVector::ZeroVector;
	UPROPERTY()
	FVector npcTargetLocation = FVector::ZeroVector;
	UPROPERTY(BlueprintReadOnly, Category = "Director NPC Struct")
	TArray<FVector> pathPoints;

	UPROPERTY()
	float nowTime = 0.f;
	
	UPROPERTY()
	bool bIsNoPoints = false;
	UPROPERTY()
	bool bIsNeedFindWaypoint = true;

	UPROPERTY(BlueprintReadOnly, Category = "Director NPC Struct")
	bool bIsCanMove = false;

	UPROPERTY(BlueprintReadOnly, Category = "Director NPC Struct")
	bool bIsNearNPC = false;
	
	UPROPERTY()
	UClass* classNpc_ = nullptr;
	UPROPERTY()
	FNpcData npcData;

	UPROPERTY()
	FString npcUniqName;

	FNpcStruct()
	{
	};
};

USTRUCT(BlueprintType)
struct FDirectorStruct
{
	GENERATED_USTRUCT_BODY()
	
	// Global layer control.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Director NPC Parameters")
	float cameraRadius = 3000.f;
	// Global layer control.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Director NPC Parameters")
	float nearFastMovableRadiusNPC = 15000.f;
	// Global layer control.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Director NPC Parameters")
	float farSlowMovableRadiusNPC = 30000.f;
	// Global layer control.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Director Actor Parameters")
	float layerOffset = 500.f;

	FDirectorStruct()
	{
	};
};

UCLASS()
class WORLDDIRECTORNPC_API ADirectorNPC : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ADirectorNPC();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	//************************************************************************
	// Component                                                                  
	//************************************************************************

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Component")
	class UInstancedStaticMeshComponent* StaticMeshInstanceComponent;

	//************************************************************************

	bool RegisterNPC(APawn *npc);

	UFUNCTION(BlueprintPure, Category = "Director NPC Struct")
    int GetBackgroundNpcCount() const;

	// Small fix for UR 4.26 updated to 4.26.1. Use this for development in editor. Disable for shipping build.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Director NPC Parameters")
	bool bIsFixUE4261EditorCrash = true;

	// Set Enable Director NPC.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Director NPC Parameters")
	bool bIsActivate = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Director NPC Parameters")
	bool bIsDebug = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Director NPC Parameters")
	FDirectorStruct directorParameters;

	UFUNCTION(BlueprintImplementableEvent, Category = "Director NPC Parameters")
	void InsertNPCInBackground_BP(AActor* actorRef);
	UFUNCTION(BlueprintImplementableEvent, Category = "Director NPC Parameters")
	void RestoreNPC_BP(int indexNpc, AActor* actorRef);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void BeginDestroy() override;

	void InsertNPCinBackground(AActor* setNPC);

	void ExchangeInformationTimer();

private:

	TArray<FNpcStruct> allBackgroundNPCArr;

	TArray<FNpcStruct> allThreadNPCArr_Debug;

	TArray<FNpcStruct> allNPCArr_ForBP;

	UPROPERTY()
	TArray<APawn*> allRegisteredPawnsArr;

	class DirectorThread* directorThreadRef;

	FTimerHandle exchangeInformation_Timer;
	float exchangeInformationRate = 0.2;

	int npcInBackground = 0;
	

};

// Thread
class DirectorThread : public FRunnable
{
public:

	//================================= THREAD =====================================
    
	//Constructor
	DirectorThread(AActor *newActor, FDirectorStruct setDirectorParameters);
	//Destructor
	virtual ~DirectorThread() override;

	//Use this method to kill the thread!!
	void EnsureCompletion();
	//Pause the thread 
	void PauseThread();
	//Continue/UnPause the thread
	void ContinueThread();
	//FRunnable interface.
	virtual bool Init() override;
	virtual uint32 Run() override;
	virtual void Stop() override;
	bool IsThreadPaused() const;

	//================================= THREAD =====================================

	TArray<FNpcStruct> GetDataNPC(TArray<FNpcStruct> setNewBackgroundNPCArr, FVector setPlayerCameraLoc,TArray<FNpcStruct> &getAllThreadNPCArr);

	void WanderingNPCinBackground(FNpcStruct &npcStruct) const;

	static void MoveNPCinBackground(FNpcStruct &npcStruct, const float &inDeltaTime);

	void FindRandomLocation(FNpcStruct &npcStruct) const;
	
	AActor *ownerActor;

private:

	//================================= THREAD =====================================
	
	//Thread to run the worker FRunnable on
	FRunnableThread* Thread;

	FCriticalSection m_mutex;
	FEvent* m_semaphore;

	int m_chunkCount;
	int m_amount;
	int m_MinInt;
	int m_MaxInt;

	float threadSleepTime = 0.01f;

	//As the name states those members are Thread safe
	FThreadSafeBool m_Kill;
	FThreadSafeBool m_Pause;


	//================================= THREAD =====================================

	TArray<FNpcStruct> allBackgroundNPCArrTH;

	TArray<FNpcStruct> canRestoreNPCArrTH;

	class UNavigationSystemV1* navigationSystemTH = nullptr;

	FVector playerCameraLoc_;

	FDirectorStruct directorParametersTH;

	float threadDeltaTime = 0.f;

	int incrementNearGetPathNpc = 15;
	int minNearGetPathNpcID = 0;
	int maxNearGetPathNpcID = 0;

	int incrementFarGetPathNpc = 5;
	int minFarGetPathNpcID = 0;
	int maxFarGetPathNpcID = 0;

	// Skip frame for performance.
	int skipFrameValue = 25;
	int nowFrame = 0;
	

	
};
