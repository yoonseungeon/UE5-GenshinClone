// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DashComponent.generated.h"

// [대시-01] 대시의 단계·변형·종료 결과와 데이터를 정의한다.
UENUM(BlueprintType)
enum class EDashPhase : uint8
{
	None,				// 대시 x
	Starting,
	Stopping,
	ReturningToRun
};

UENUM(BlueprintType)
enum class EDashAnimationVariant : uint8
{
	Normal,
	Mirrored
};

UENUM(BlueprintType)
enum class EDashEndReason : uint8
{
	None,
	Stopped,
	ReturnedToRun,
	Canceled
};

// 대시 한 종류에 필요한 애니메이션과 연결 설정을 묶은 구조체
USTRUCT(BlueprintType)
struct GENSHIN_CLONE_API FDashAnimationSet
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash|Animation")
	TObjectPtr<class UAnimSequence> StartAnimation = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash|Animation")
	TObjectPtr<class UAnimSequence> StopAnimation = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash|Animation")
	TObjectPtr<class UAnimSequence> ReturnAnimation = nullptr;

	// SprintToRun을 재생하기 시작할 비율이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash|Animation", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ReturnAnimationStartRatio = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash|Animation")
	bool bUseMirroredRun = false;

	// SprintToRun 끝 포즈에 대응하는 Run 재생 시간이며 단위는 초다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash|Animation", meta = (ClampMin = "0.0", Units = "s"))
	float RunPoseMatchTime = 0.0f;
};

// 현재 대시가 어떻게 진행되고 있는지와 종료 결과를 저장하는 구조체
USTRUCT(BlueprintType)
struct GENSHIN_CLONE_API FDashRuntimeState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Dash")
	EDashPhase Phase = EDashPhase::None;

	// 현재 단계에 진입한 뒤 경과한 시간이다.
	UPROPERTY(BlueprintReadOnly, Category = "Dash")
	float PhaseElapsedTime = 0.0f;

	// 현재 단계 애니메이션의 재생 시작 위치이며 단위는 초다.
	UPROPERTY(BlueprintReadOnly, Category = "Dash")
	float AnimationStartTime = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Dash")
	FVector Direction = FVector::ZeroVector;

	// 대시를 새로 시작할 때 증가하며 종료 후에도 보존하는 실행 번호다.
	UPROPERTY(BlueprintReadOnly, Category = "Dash")
	int32 ActivationId = 0;

	// 종료 후 블렌딩에도 같은 세트를 쓰도록 선택을 유지한다.
	UPROPERTY(BlueprintReadOnly, Category = "Dash")
	EDashAnimationVariant Variant = EDashAnimationVariant::Normal;

	// 마지막 종료 결과는 다음 대시가 시작될 때까지 유지한다.
	UPROPERTY(BlueprintReadOnly, Category = "Dash")
	EDashEndReason EndReason = EDashEndReason::None;
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class GENSHIN_CLONE_API UDashComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDashComponent();

	const FDashRuntimeState& GetRuntimeState() const { return RuntimeState; }

	bool TryStartDash(EDashAnimationVariant Variant);
	void CancelDash();

	void UpdateMovementIntent(const FVector& Intent);
	class UAnimSequence* GetAnimationForPhase(EDashPhase Phase) const;

	const FDashAnimationSet& GetAnimationSet(EDashAnimationVariant Variant) const;
	const FDashAnimationSet& GetActiveAnimationSet() const;

	float GetReturnToRunBlendTime() const { return ReturnToRunBlendTime; }

protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	bool BeginDash(const FVector& Direction, EDashAnimationVariant Variant);

	void EnterPhase(EDashPhase Phase);

	void EndDash(EDashEndReason Reason);

private:
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Dash", meta = (AllowPrivateAccess = "true"))
	FDashRuntimeState RuntimeState;

	// 현재 사용자의 이동 입력 방향을 대시 컴포넌트가 보관하는 변수
	// 대시를 계속할지, Stop으로 넘어갈지 결정
	FVector CurrentMovementIntent = FVector::ZeroVector;

	// [대시-02] 원본·미러 세트와 기본 변형 및 전환 시점을 설정한다.
	UPROPERTY(EditDefaultsOnly, Category = "Dash|Animation")
	FDashAnimationSet NormalAnimationSet;

	UPROPERTY(EditDefaultsOnly, Category = "Dash|Animation")
	FDashAnimationSet MirroredAnimationSet;

	// SprintBS 최소 재생 비율
	UPROPERTY(EditDefaultsOnly, Category = "Dash|Timing", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float StopAllowedStartRatio = 0.28f;

	// Stop 클립을 중간부터 재생할 시작 비율
	UPROPERTY(EditDefaultsOnly, Category = "Dash|Timing", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float StopAnimationStartRatio = 0.17f;

	// 입력 유지 시 SprintBS에서 SprintToRun으로 전환할 재생 비율
	UPROPERTY(EditDefaultsOnly, Category = "Dash|Timing", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float StartToReturnRatio = 50.0f / 73.0f;

	// 복귀 클립 종료보다 Run 블렌드를 앞당길 시간이며 단위는 초다.
	UPROPERTY(EditDefaultsOnly, Category = "Dash|Timing", meta = (ClampMin = "0.0"))
	float ReturnToRunBlendTime = 0.1f;
};
