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
	TObjectPtr<class UAnimMontage> SprintBSMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash|Animation")
	TObjectPtr<class UAnimMontage> SprintStopMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash|Animation")
	TObjectPtr<class UAnimMontage> SprintToRunMontage = nullptr;

	// SprintToRun을 재생하기 시작할 비율이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash|Animation", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SprintToRunStartRatio = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash|Animation")
	bool bRunCycleIsMirrored = false;

	// SprintToRun 끝 포즈에 대응하는 RunCycle 재생 시간이며 단위는 초다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dash|Animation", meta = (ClampMin = "0.0", Units = "s"))
	float RunCyclePoseMatchTimeSeconds = 0.0f;
};

// 현재 대시가 어떻게 진행되고 있는지와 종료 결과를 저장하는 구조체
USTRUCT(BlueprintType)
struct GENSHIN_CLONE_API FDashRuntimeState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Dash")
	EDashPhase Phase = EDashPhase::None;

	// 현재 단계 몽타주의 시작 위치부터 경과한 재생 시간이다.
	UPROPERTY(BlueprintReadOnly, Category = "Dash")
	float PhaseMontageElapsedSeconds = 0.0f;

	// 현재 단계 몽타주의 재생 시작 위치이며 단위는 초다.
	UPROPERTY(BlueprintReadOnly, Category = "Dash")
	float PhaseMontageStartTimeSeconds = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Dash")
	FVector DashDirection = FVector::ZeroVector;

	// 대시를 새로 시작할 때 증가하며 종료 후에도 보존하는 실행 번호다.
	UPROPERTY(BlueprintReadOnly, Category = "Dash")
	int32 DashId = 0;

	// 종료 후 블렌딩에도 같은 세트를 쓰도록 선택을 유지한다.
	UPROPERTY(BlueprintReadOnly, Category = "Dash")
	EDashAnimationVariant AnimationVariant = EDashAnimationVariant::Normal;

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

	const FDashRuntimeState& GetDashRuntimeState() const { return DashRuntimeState; }

	// 호출 전에 현재 캐릭터 Mesh에 애님 인스턴스가 연결되어 있어야 한다.
	bool TryStartDash(EDashAnimationVariant DashVariant);
	void CancelDash();

	void SetMovementInputDirection(const FVector& InputDirection);
	class UAnimMontage* GetDashPhaseMontage(EDashPhase DashPhase, EDashAnimationVariant DashVariant) const;

	const FDashAnimationSet& GetDashAnimationSet(EDashAnimationVariant DashVariant) const;
	const FDashAnimationSet& GetCurrentDashAnimationSet() const;

	float GetRunCycleStartTimeAfterDash() const;
	int32 GetLastSprintStopDashId() const { return LastSprintStopDashId; }
	EDashAnimationVariant GetLastSprintStopVariant() const { return LastSprintStopVariant; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	bool BeginDash(const FVector& RequestedDashDirection, EDashAnimationVariant DashVariant);

	// [대시-10] 현재 대시 정보로 단계를 시작하고 재생 실패 시 취소한다.
	bool StartDashPhase(EDashPhase DashPhase);
	// [대시-11] 종료 결과를 기록하고 대시 상태와 소유한 재생을 정리한다.
	void EndDash(EDashEndReason DashEndReason, bool bStopDashMontage = true);

	// [대시-13] 몽타주 재생과 해당 재생의 종료·중단 알림을 연결한다.
	bool PlayDashPhaseMontage(class UAnimMontage* DashMontage, float MontageStartTimeSeconds, int32 DashId);

	// [대시-14] SprintStop의 종료·중단 알림으로 대시 종료 결과를 기록한다.
	void OnSprintStopMontageEnded(class UAnimMontage* DashMontage, bool bInterrupted, int32 SourceMontageInstanceId, int32 SourceDashId);
	// [대시-15] SprintToRun의 블렌드 아웃 알림으로 RunCycle 복귀 시간을 저장한다.
	void OnSprintToRunMontageBlendOutStarted(class UAnimMontage* DashMontage, bool bInterrupted, int32 SourceMontageInstanceId, int32 SourceDashId);

private:
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Dash", meta = (AllowPrivateAccess = "true"))
	FDashRuntimeState DashRuntimeState;

	// 현재 이동 입력 방향으로 대시 유지와 SprintStop 진입을 결정한다.
	FVector MovementInputDirection = FVector::ZeroVector;

	// [대시-02] 원본·미러 세트와 기본 변형 및 전환 시점을 설정한다.
	UPROPERTY(EditDefaultsOnly, Category = "Dash|Animation")
	FDashAnimationSet NormalDashAnimationSet;

	UPROPERTY(EditDefaultsOnly, Category = "Dash|Animation")
	FDashAnimationSet MirroredDashAnimationSet;

	// SprintBS 최소 재생 비율
	UPROPERTY(EditDefaultsOnly, Category = "Dash|Timing", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SprintBSStopAllowedRatio = 0.28f;

	// SprintStop 몽타주를 중간부터 재생할 시작 비율
	UPROPERTY(EditDefaultsOnly, Category = "Dash|Timing", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SprintStopStartRatio = 0.17f;

	// 입력 유지 시 SprintBS에서 SprintToRun으로 전환할 재생 비율
	UPROPERTY(EditDefaultsOnly, Category = "Dash|Timing", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SprintBSToSprintToRunRatio = 50.0f / 73.0f;

	// [대시-13] 현재 Mesh에서 시작한 대시 몽타주의 실행 번호만 보관한다.
	int32 DashMontageInstanceId = INDEX_NONE;

	// SprintToRun 블렌드 아웃 시작부터 RunCycle 위치 계산까지의 시간을 보존한다.
	float SprintToRunBlendOutSeconds = 0.1f;
	double SprintToRunBlendOutStartGameSeconds = 0.0;

	// SprintStop 재생 성공 이력을 애님 인스턴스에 전달한다.
	int32 LastSprintStopDashId = 0;
	EDashAnimationVariant LastSprintStopVariant = EDashAnimationVariant::Normal;


	// [대시-05] 연속 대시 판정 시간과 재사용 시각을 보관한다.
	UPROPERTY(EditDefaultsOnly, Category = "Dash|Timing", meta = (Units = "s"))
	float SecondDashTimeLimit = 0.8f;

	UPROPERTY(EditDefaultsOnly, Category = "Dash|Timing", meta = (Units = "s"))
	float DashCooldown = 1.5f;

	// 연속 대시의 첫 번째 성공 기록이 있다.
	bool bHasFirstDash = false;

	// 첫 대시 시작 후 두 번째 대시를 인정하는 마지막 게임 시각이다.
	double SecondDashDeadline = 0.0;

	// 직전 SprintBS의 재입력 차단이 끝나는 게임 시각이다.
	double DashUnlockTime = 0.0;

	// 두 번째 대시로 발생한 쿨타임이 끝나는 게임 시각이다.
	double DashCooldownEnd = 0.0;
};
